/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_audio_log.h"
#include "esp_system.h"
#include "reconnect_sm.h"
#include "bt_app_common.h"
#include "bt_app.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "key_control.h"
#include "bt.h"
#include "esp_bt_device.h"
#include "BluetoothControlService.h"

#include "../DeviceController/DeviceCommon.h"
#include "MediaControl.h"
#include "AudioCodec.h"
#include "led.h"
#include "toneuri.h"
#include "AudioDef.h"
#include "EspAudioAlloc.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "esp_heap_alloc_caps.h"
#include "esp_audio_log.h"

#define BT_TAG "BT_SERVICE"

#define BT_TRIGGER_URI  "raw://44100:2@from.pcm/to.pcm#i2s"   //TODO: put somewhere else for sharing

typedef struct {
    esp_a2d_audio_state_t audio_state;
    bool tone_stopped;
    bool raw_start;
    bool avrc_connected;
    uint8_t tl; // transaction label
    esp_bd_addr_t remote_bda;
    xQueueHandle player_status_que;
} bt_av_app_cb_t;


static BluetoothControlService *local_service;
static bt_av_app_cb_t m_bt_av;
static bt_av_app_cb_t *const p_bt_av = &m_bt_av;

static void bt_app_handle_evt(uint16_t event, void *p_param);
static void bt_app_handle_rec_evt(uint16_t event, void *p_param);

static void bt_app_a2d_sink_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    bt_app_transfer_context(bt_app_handle_evt, event, param, sizeof(bt_app_evt_arg), NULL);
}

static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    bt_app_transfer_context(bt_app_handle_rc_evt, event, param, sizeof(bt_app_evt_arg), NULL);
}

static void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
    // if (p_bt_av->audio_state != ESP_A2D_AUDIO_STATE_STARTED ||
    //     !p_bt_av->tone_stopped) {
    //     return;
    // }

    // MediaControl *ctrl = (MediaControl *)local_service->Based.instance;
    // ctrl->streamToBuffer(ctrl, data, len);
    local_service->Based.rawWrite(local_service, data, len, NULL);
}

static void bt_app_a2d_connect(void)
{
    uint8_t *p = p_bt_av->remote_bda;
    uint8_t null_bda[ESP_BD_ADDR_LEN] = {0};

    if (memcmp(p_bt_av->remote_bda, null_bda, ESP_BD_ADDR_LEN) == 0) {
        return;
    }
    ESP_AUDIO_LOGI(BT_TAG, "Reconnect [%02x:%02x:%02x:%02x:%02x:%02x]\n", p[0], p[1], p[2], p[3], p[4], p[5]);

    esp_a2d_sink_connect(p_bt_av->remote_bda);
}

void a2d_rec_alarm_to(void *p)
{
    bt_app_transfer_context(bt_app_handle_rec_evt, A2D_REC_EVT_ALARM_TO, NULL, 0, NULL);
}

static void bt_app_handle_evt(uint16_t event, void *p_param)
{
    ESP_AUDIO_LOGV(BT_TAG, "bt_app_handle_evt 0x%x\n", event);
    esp_a2d_cb_param_t *a2d = NULL;
    switch (event) {
    case BT_APP_EVT_STACK_ON: {
        uint8_t hwaddr[6] = {0};
        esp_efuse_read_mac(hwaddr);
        char dev_name[16];
        sprintf(dev_name, "ESP_BT%02X%02X%02X", hwaddr[3], hwaddr[4], hwaddr[5]);

        memset(p_bt_av, 0, sizeof(*p_bt_av));
        p_bt_av->audio_state = ESP_A2D_AUDIO_STATE_STOPPED;

        esp_bt_dev_set_device_name(dev_name);

        esp_a2d_register_callback(bt_app_a2d_sink_cb);
        esp_a2d_register_data_callback(bt_app_a2d_data_cb);

        esp_a2d_sink_init();

        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

        a2d_rec_sm_init(bt_app_a2d_connect);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        break;
    }
    case ESP_A2D_CONNECTION_STATE_EVT: {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_AUDIO_LOGI(BT_TAG, "A2DP connection state changed: state %d\n", a2d->conn_stat.state);
        if (ESP_A2D_CONNECTION_STATE_CONNECTED == a2d->conn_stat.state) {
            memcpy(p_bt_av->remote_bda, a2d->conn_stat.remote_bda, ESP_BD_ADDR_LEN);
            a2d_rec_sm_handler(A2D_REC_EVT_CONNECTED, NULL);
            LedIndicatorSet(LedIndexSys, LedWorkState_NetConnectOk);
            if (local_service->Based.playTone) {
                p_bt_av->tone_stopped = false;
                local_service->Based.playTone(&local_service->Based, tone_uri[TONE_TYPE_BT_SUCCESS], TERMINATION_TYPE_DONE);
            } else {
                p_bt_av->tone_stopped = true;
            }
            EspAudioPlayerSrcSet(MEDIA_SRC_BT);
        } else if (ESP_A2D_CONNECTION_STATE_DISCONNECTED == a2d->conn_stat.state) {
            if (a2d->conn_stat.disc_rsn == ESP_A2D_DISC_RSN_ABNORMAL) {
                a2d_rec_sm_handler(A2D_REC_EVT_DISCONNECTED, NULL);
            }
            EspAudioPlayerSrcSet(MEDIA_SRC_NULL);
            LedIndicatorSet(LedIndexSys, LedWorkState_NetDisconnect);
            if (local_service->Based.playTone) {
                local_service->Based.playTone(&local_service->Based, tone_uri[TONE_TYPE_BT_RECONNECT], TERMINATION_TYPE_DONE);
                p_bt_av->tone_stopped = false;
            }
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT: {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_AUDIO_LOGI(BT_TAG, "A2DP audio state changed: state %d\n", a2d->audio_stat.state);
        if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED) {
            if (p_bt_av->tone_stopped == true) {
                local_service->Based.rawStart(local_service, BT_TRIGGER_URI);
            } else {
                p_bt_av->raw_start = 1;// wait till the tone finished
            }
        } else if (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STOPPED) {
            local_service->Based.rawStop(local_service, TERMINATION_TYPE_DONE);
        }
        EspAudioPrintMemory(BT_TAG);
        p_bt_av->audio_state = a2d->audio_stat.state;
        break;
    }
    case ESP_A2D_AUDIO_CFG_EVT: {
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_AUDIO_LOGI(BT_TAG, "A2DP audio configured: type %d\n", a2d->audio_cfg.mcc.type);
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            int sample_rate = 16000;
            char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                sample_rate = 48000;
            }
            MediaControl *ctrl = (MediaControl *)local_service->Based.instance;
            CodecEvent evt;
            int setup_data[3];
            setup_data[0] = sample_rate;
            setup_data[1] = 2;
            setup_data[2] = 16;

            evt.type = CODEC_EVT_REQ_SETUP;
            evt.data = (void *)setup_data;
            evt.instance = ctrl;
            ctrl->decoderRequestSetup(ctrl, (void *)&evt);
            // temporarily hardcoded the PCM configuaration
            ESP_AUDIO_LOGI(BT_TAG, "configure audio player %x-%x-%x-%x\n",
                     a2d->audio_cfg.mcc.cie.sbc[0],
                     a2d->audio_cfg.mcc.cie.sbc[1],
                     a2d->audio_cfg.mcc.cie.sbc[2],
                     a2d->audio_cfg.mcc.cie.sbc[3]);
        }
        break;
    }
    default:
        ESP_AUDIO_LOGI(BT_TAG, "Invalid A2DP event: 0x%x\n", event);
        break;
    }

}

void bt_app_handle_rc_evt(uint16_t event, void *p_param)
{
    ESP_AUDIO_LOGV(BT_TAG, "Handle AVRC event: 0x%x\n", event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        p_bt_av->avrc_connected = rc->conn_stat.connected;
        if (rc->conn_stat.connected) {
            key_act_sm_init();
        } else if (rc->conn_stat.connected == 0) {
            key_act_sm_deinit();
        }
        ESP_AUDIO_LOGI(BT_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        break;
    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        if (p_bt_av->avrc_connected) {
            ESP_AUDIO_LOGI(BT_TAG, "AVRC passthroun cmd rsp: key_code 0x%x, key_state %d\n", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
            key_act_param_t param;
            memset(&param, 0, sizeof(key_act_param_t));
            param.evt = event;
            param.tl = rc->psth_rsp.tl;
            param.key_code = rc->psth_rsp.key_code;
            param.key_state = rc->psth_rsp.key_state;
            key_act_state_machine(&param);
        }
        break;
    }
    case ESP_AVRC_CT_KEY_STATE_CHG_EVT: {
        if (p_bt_av->avrc_connected) {
            esp_avrc_key_state_t *key_s = (esp_avrc_key_state_t *)(p_param);
            ESP_AUDIO_LOGI(BT_TAG, "AVRC send key id0x%x, state %d\n", key_s->key_code, key_s->key_state);
            key_act_param_t param;
            memset(&param, 0, sizeof(key_act_param_t));
            param.evt = event;
            param.key_code = key_s->key_code;
            param.key_state = key_s->key_state;
            param.tl = (p_bt_av->tl) & 0x0F;
            p_bt_av->tl = (p_bt_av->tl + 2) & 0x0f;
            key_act_state_machine(&param);
        }
        break;
    }
    case ESP_AVRC_CT_PT_RSP_TO_EVT: {
        ESP_AUDIO_LOGI(BT_TAG, "AVRC passthrough cmd rsp time out\n");
        if (p_bt_av->avrc_connected) {
            key_act_param_t param;
            memset(&param, 0, sizeof(key_act_param_t));
            param.evt = event;
            key_act_state_machine(&param);
        }
        break;
    }
#if IDF_3_0
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
        uint8_t *bda = rc->rmt_feats.remote_bda;
        ESP_AUDIO_LOGI(BT_TAG, "AVRC remote features: 0x%x, [%02x:%02x:%02x:%02x:%02x:%02x]\n",
                 rc->rmt_feats.feat_mask, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        break;
    }
#endif //IDF_3_0
    default:
        ESP_AUDIO_LOGI(BT_TAG, "Invalid AVRC event: 0x%x\n", event);
        break;
    }
}

static void bt_app_handle_rec_evt(uint16_t event, void *p_param)
{
    /* handle reconnection event */
    switch (event) {
    case A2D_REC_EVT_ALARM_TO: {
        a2d_rec_sm_handler(A2D_REC_EVT_ALARM_TO, NULL);
        break;
    }
    default:
        ESP_AUDIO_LOGE(BT_TAG, "Invalid reconnection event: 0x%x\n", event);
        break;
    }
}

void app_main_entry(void *param)
{
    if (esp_bluedroid_init() != ESP_OK) {
        ESP_AUDIO_LOGE(BT_TAG, "%s initialize bluedroid failed\n", __func__);
        return;
    }

    if (esp_bluedroid_enable() != ESP_OK) {
        ESP_AUDIO_LOGE(BT_TAG, "%s enable bluedroid failed\n", __func__);
        return;
    }
    bt_app_transfer_context(bt_app_handle_evt, BT_APP_EVT_STACK_ON, param, 0, NULL);
}

static void avrcpTransfer(int evt)
{
    bt_app_evt_arg param;
    memset(&param, 0, sizeof(bt_app_evt_arg));
    param.key.key_code = evt;
    // it will be ESP_AVRC_PT_CMD_PAUSE/ESP_AVRC_PT_CMD_PLAY/ESP_AVRC_PT_CMD_FORWARD/ESP_AVRC_PT_CMD_BACKWARD
    bt_app_transfer_context(bt_app_handle_rc_evt, ESP_AVRC_CT_KEY_STATE_CHG_EVT, &param, sizeof(bt_app_evt_arg), NULL);
}

void deviceEvtNotifiedToBt(DeviceNotification *event)
{
    DeviceNotification *evt = (DeviceNotification *)event;
    ESP_AUDIO_LOGI(BT_TAG, "deviceEvtNotifiedToBt type%x", evt->type);
    EspAudioPrintMemory(BT_TAG);
    if (event && (event->type == DEVICE_NOTIFY_TYPE_TOUCH)) {
        DeviceNotifyMsg msg = *((DeviceNotifyMsg *) event->data);
        static char playFlag = 1;
        switch (msg) {
        case DEVICE_NOTIFY_KEY_NEXT:
            avrcpTransfer(ESP_AVRC_PT_CMD_FORWARD);
            break;
        case DEVICE_NOTIFY_KEY_PREV:
            avrcpTransfer(ESP_AVRC_PT_CMD_BACKWARD);
            break;
        case DEVICE_NOTIFY_KEY_PLAY:
            if (playFlag) {
                avrcpTransfer(ESP_AVRC_PT_CMD_PLAY);
                ESP_AUDIO_LOGI(BT_TAG, "Play");
            } else {
                avrcpTransfer(ESP_AVRC_PT_CMD_PAUSE);
                ESP_AUDIO_LOGI(BT_TAG, "Pause");
            }
            playFlag = !playFlag;
            break;
        default:
            break;
        }
    }
}

void playerStatusUpdatedToBt(ServiceEvent *event)
{
}

static void player_status_updated(void *param)
{
    assert(param != NULL);
    PlayerStatus *p = (PlayerStatus *)param;

    ESP_AUDIO_LOGI(BT_TAG, "Player status updated: status 0x%x, err 0x%x\n", p->status, p->errMsg);
    if ((p->status == AUDIO_STATUS_STOP || p->status == AUDIO_STATUS_ERROR) && p->mode == PLAYER_WORKING_MODE_TONE) {
        p_bt_av->tone_stopped = true;
        if (p_bt_av->raw_start) {
            local_service->Based.rawStart(local_service, BT_TRIGGER_URI);
            p_bt_av->raw_start = 0;
        }
    }
}

void bluetoothControlActive(BluetoothControlService *service)
{
    ESP_AUDIO_LOGI(BT_TAG, "bluetoothControlActive, freemem %d", esp_get_free_heap_size());
    LedIndicatorSet(LedIndexSys, LedWorkState_NetSetting);

    p_bt_av->player_status_que = xQueueCreate(2, sizeof(PlayerStatus));

    local_service->Based.addListener((MediaService *)local_service, p_bt_av->player_status_que);

    bt_app_xque_item_t ext_que = {
        2,
        sizeof(PlayerStatus),
        p_bt_av->player_status_que,
        (bt_app_ext_que_hdl_t)(player_status_updated)
    };

    bt_app_task_start_up(&ext_que, 1);
}

void bluetoothControlDeactive(BluetoothControlService *service)
{
    ESP_AUDIO_LOGI(BT_TAG, "bluetoothControlStop\r\n");
}

BluetoothControlService *BluetoothControlCreate()
{
    ESP_AUDIO_LOGI(BT_TAG, "BluetoothControlCreate\r\n");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
        ESP_AUDIO_LOGE(BT_TAG, "%s initialize controller failed\n", __func__);
        return NULL;
    }
#if IDF_3_0
    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
#else
    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
#endif
        ESP_AUDIO_LOGE(BT_TAG, "%s enable controller failed\n", __func__);
        return NULL;
    }
    BluetoothControlService *bt = (BluetoothControlService *) EspAudioAlloc(1, sizeof(BluetoothControlService));
    ESP_ERROR_CHECK(!bt);
    bt->Based.deviceEvtNotified = deviceEvtNotifiedToBt;
    bt->Based.playerStatusUpdated = playerStatusUpdatedToBt;
    bt->Based.serviceActive = bluetoothControlActive;
    bt->Based.serviceDeactive = bluetoothControlDeactive;
    local_service = bt;
    return bt;
}

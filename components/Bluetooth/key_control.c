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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#include "esp_system.h"

#include "key_control.h"
#include "bt_app.h"

#include "esp_audio_log.h"

#define KEYCTRL_TAG "KEYCTRL"

static void key_act_state_hdl_idle(key_act_cb_t *key_cb, key_act_param_t *param);
static void key_act_state_hdl_press(key_act_cb_t *key_cb, key_act_param_t *param);
static void key_act_state_hdl_release(key_act_cb_t *key_cb, key_act_param_t *param);

static void key_act_time_out(void *p);

xTaskHandle xKeyCtrlTaskHandle = 0;

static key_act_cb_t key_cb;


void key_act_state_machine(key_act_param_t *param)
{
    ESP_AUDIO_LOGI(KEYCTRL_TAG, "key_ctrl cb: tl %d, state: %d\n", key_cb.tl, key_cb.state);
    switch (key_cb.state) {
    case KEY_ACT_STATE_IDLE:
        key_act_state_hdl_idle(&key_cb, param);
        break;
    case KEY_ACT_STATE_PRESS:
        key_act_state_hdl_press(&key_cb, param);
        break;
    case KEY_ACT_STATE_RELEASE:
        key_act_state_hdl_release(&key_cb, param);
        break;
    default:
        ESP_AUDIO_LOGI(KEYCTRL_TAG, "Invalid key_ctrl state: %d\n", key_cb.state);
        break;
    }
}

static void key_act_time_out(void *p)
{
    bt_app_evt_arg param;
    memset(&param, 0, sizeof(bt_app_evt_arg));
    bt_app_transfer_context(bt_app_handle_rc_evt, ESP_AVRC_CT_PT_RSP_TO_EVT, &param, sizeof(bt_app_evt_arg), NULL);
}

bool key_act_sm_init(void)
{
    if (key_cb.key_tmr) {
        ESP_AUDIO_LOGW(KEYCTRL_TAG, "%s timer not released\n", __func__);
        xTimerDelete(key_cb.key_tmr, portMAX_DELAY);
        key_cb.key_tmr = NULL;
    }
    memset(&key_cb, 0, sizeof(key_act_cb_t));
    int tmr_id = 0xfe;
    key_cb.tl = 0;
    key_cb.state = KEY_ACT_STATE_IDLE;
    key_cb.key_code = 0;
    key_cb.key_tmr = xTimerCreate("key_tmr", portMAX_DELAY,
                                  pdFALSE, (void *)tmr_id, key_act_time_out);
    if (key_cb.key_tmr == NULL) {
        ESP_AUDIO_LOGW(KEYCTRL_TAG, "%s timer creation failure\n", __func__);
        return false;
    }
    return true;
}

void key_act_sm_deinit(void)
{
    if (key_cb.key_tmr) {
        xTimerDelete(key_cb.key_tmr, portMAX_DELAY);
        key_cb.key_tmr = NULL;
    }

    memset(&key_cb, 0, sizeof(key_act_cb_t));
}

static void key_act_state_hdl_idle(key_act_cb_t *key_cb, key_act_param_t *param)
{
    assert(key_cb != NULL && param != NULL);
    assert(key_cb->state == KEY_ACT_STATE_IDLE);
    if (param->evt == ESP_AVRC_CT_KEY_STATE_CHG_EVT) {
        key_cb->tl = param->tl;
        key_cb->key_code = param->key_code;
        esp_avrc_ct_send_passthrough_cmd(param->tl, param->key_code, ESP_AVRC_PT_CMD_STATE_PRESSED);
        xTimerStart(key_cb->key_tmr, 500 / portTICK_RATE_MS);
        key_cb->state = KEY_ACT_STATE_PRESS;
    }
}

static void key_act_state_hdl_press(key_act_cb_t *key_cb, key_act_param_t *param)
{
    assert(key_cb != NULL && param != NULL);
    assert(key_cb->state == KEY_ACT_STATE_PRESS);
    if (param->evt == ESP_AVRC_CT_PASSTHROUGH_RSP_EVT) {
        if (key_cb->tl != param->tl || key_cb->key_code != param->key_code
            || ESP_AVRC_PT_CMD_STATE_PRESSED != param->key_state) {
            ESP_AUDIO_LOGW(KEYCTRL_TAG, "Key pressed hdlr: invalid state\n");
            return;
        }
        key_cb->tl = (key_cb->tl + 1) & 0x0F;
        esp_avrc_ct_send_passthrough_cmd(key_cb->tl, param->key_code, ESP_AVRC_PT_CMD_STATE_RELEASED);
        xTimerReset(key_cb->key_tmr, 500 / portTICK_RATE_MS);
        key_cb->state = KEY_ACT_STATE_RELEASE;
    } else if (param->evt == ESP_AVRC_CT_PT_RSP_TO_EVT) {
        key_cb->tl = 0;
        key_cb->key_code = 0;
        key_cb->state = KEY_ACT_STATE_IDLE;
    }
}

static void key_act_state_hdl_release(key_act_cb_t *key_cb, key_act_param_t *param)
{
    assert(key_cb != NULL);
    assert(key_cb->state == KEY_ACT_STATE_RELEASE);
    if (param->evt == ESP_AVRC_CT_PASSTHROUGH_RSP_EVT) {
        if (key_cb->tl != param->tl || key_cb->key_code != param->key_code
            || ESP_AVRC_PT_CMD_STATE_RELEASED != param->key_state) {
            return;
        }
        xTimerStop(key_cb->key_tmr, 500 / portTICK_RATE_MS);
        key_cb->state = KEY_ACT_STATE_IDLE;
        key_cb->key_code = 0;
    } else if (param->evt == ESP_AVRC_CT_PT_RSP_TO_EVT) {
        key_cb->tl = 0;
        key_cb->key_code = 0;
        key_cb->state = KEY_ACT_STATE_IDLE;
    }
}


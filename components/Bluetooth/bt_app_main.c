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
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "esp_audio_log.h"
#include "bt_app_common.h"
#include "freertos/xtensa_api.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_audio_log.h"

#define BT_TAG "BT_APP"

#define BT_APP_TASK_QUE_LEN   (10)

static xQueueHandle m_tsk_que = NULL;
static xTaskHandle m_tsk_hdl = NULL;
static QueueSetHandle_t m_tsk_que_set = NULL;
static bt_app_xque_item_t m_ext_que_cb[BT_APP_MAX_EXT_QUE_NUM];


static bool bt_app_post_msg(bt_app_msg_t *msg);
static void bt_app_context_switched(bt_app_msg_t *msg);
static void bt_app_task_handler(void *arg);
extern void app_main_entry(void *param);

bool bt_app_transfer_context (bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
    bt_app_msg_t msg;

    memset(&msg, 0, sizeof(bt_app_msg_t));

    msg.sig = BT_APP_SIG_CONTEXT_SWITCH;
    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return bt_app_post_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);
            /* check if caller has provided a copy callback to do the deep copy */
            if (p_copy_cback) {
                p_copy_cback(&msg, msg.param, p_params);
            }
            return bt_app_post_msg(&msg);
        }
    }

    return false;
}

static bool bt_app_post_msg(bt_app_msg_t *msg)
{
    if (msg == NULL) {
        return false;
    }

    if (xQueueSend(m_tsk_que, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        ESP_AUDIO_LOGE(BT_TAG, "App msg post failed\n");
        return false;
    }
    return true;
}

static void bt_app_context_switched(bt_app_msg_t *msg)
{
    ESP_AUDIO_LOGV(BT_TAG, "App context switched\n");
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}

static void m_tsk_que_handler(bt_app_msg_t *msg)
{
    if (msg == NULL)
        return;

    ESP_AUDIO_LOGV(BT_TAG, "App hdl evt: sig 0x%x, 0x%x\n", msg->sig, msg->event);
    switch (msg->sig) {
    case BT_APP_SIG_CONTEXT_SWITCH:
        bt_app_context_switched(msg);
        break;
    default:
        ESP_AUDIO_LOGW(BT_TAG, "Invalid app event: sig (0x%x)\n", msg->sig);
        break;
    } // switch (msg->sig)

    if (msg->param) {
        free(msg->param);
    }
}

static void bt_app_task_handler(void *arg)
{
    app_main_entry(arg);
    QueueSetMemberHandle_t act_que;
    for (;;) {
        act_que = xQueueSelectFromSet(m_tsk_que_set, (portTickType)portMAX_DELAY);
        if (act_que == m_tsk_que) {
            bt_app_msg_t msg;
            if (pdTRUE == xQueueReceive(act_que, &msg, 0)) {
                m_tsk_que_handler(&msg);
            }
        } else {
            for (int i = 0; i < BT_APP_MAX_EXT_QUE_NUM; i++) {
                const bt_app_xque_item_t *p_item = &m_ext_que_cb[i];
                if (p_item->xque == NULL) {
                    break;
                } else if (act_que == p_item->xque) {
                    ESP_AUDIO_LOGV(BT_TAG, "App msg received from ext_que\n");
                    void *xmsg = malloc(p_item->xque_item_sz);
                    assert(xmsg != NULL);
                    if (pdTRUE == xQueueReceive(act_que, xmsg, 0)) {
                        if (p_item->xque_hdl) {
                            p_item->xque_hdl(xmsg);
                        }
                    }
                    free(xmsg);
                }
            }
        }
    }
}

void bt_app_task_start_up(const bt_app_xque_item_t *param, uint32_t xque_num)
{
    uint32_t total_xque_len;
    memset(m_ext_que_cb, 0, sizeof(m_ext_que_cb));
    xque_num = (xque_num > BT_APP_MAX_EXT_QUE_NUM) ? BT_APP_MAX_EXT_QUE_NUM : xque_num;

    total_xque_len = 0;
    for (int i = 0; i < xque_num; i++) {
        assert(param[i].xque_len != 0);
        assert(param[i].xque_item_sz != 0);
        assert(param[i].xque != NULL);

        bt_app_xque_item_t *p_item = &m_ext_que_cb[i];
        memcpy(p_item, &param[i], sizeof(bt_app_xque_item_t));

        total_xque_len += param[i].xque_len;
    }

    m_tsk_que = xQueueCreate(BT_APP_TASK_QUE_LEN, sizeof(bt_app_msg_t));
    m_tsk_que_set = xQueueCreateSet(BT_APP_TASK_QUE_LEN + total_xque_len);
    xQueueAddToSet(m_tsk_que, m_tsk_que_set);

    for (int i = 0; i < BT_APP_MAX_EXT_QUE_NUM; i++) {
        bt_app_xque_item_t *p_item = &m_ext_que_cb[i];
        if (p_item->xque == NULL) {
            break;
        }
        xQueueAddToSet(p_item->xque, m_tsk_que_set);
    }

    xTaskCreate(bt_app_task_handler, "BtAppT", 3096, param, configMAX_PRIORITIES - 3, m_tsk_hdl);
    return;
}



void bt_app_task_shut_down(void)
{
    if (m_tsk_hdl) {
        vTaskDelete(m_tsk_hdl);
        m_tsk_hdl = NULL;
    }
    if (m_tsk_que) {
        vQueueDelete(m_tsk_que);
        m_tsk_que = NULL;
    }

    memset(m_ext_que_cb, 0, sizeof(m_ext_que_cb));
}

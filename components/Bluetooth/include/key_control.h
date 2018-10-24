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

#ifndef __KEY_CONTROL_H__
#define __KEY_CONTROL_H__

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "userconfig.h"
typedef enum {
    KEY_ACT_STATE_IDLE,
    KEY_ACT_STATE_PRESS,
    KEY_ACT_STATE_RELEASE
} key_act_state_t;

typedef struct {
    key_act_state_t state;
    uint32_t key_code;
    TimerHandle_t key_tmr;
    uint8_t tl;
} key_act_cb_t;

typedef struct {
    uint32_t evt;
    uint32_t key_code;
    uint8_t key_state;
    uint8_t tl;
} key_act_param_t;

/// AVRC callback events
#if IDF_3_0
#define ESP_AVRC_CT_MAX_EVT (ESP_AVRC_CT_REMOTE_FEATURES_EVT)
#endif // IDF_3_0

#define ESP_AVRC_CT_KEY_STATE_CHG_EVT (ESP_AVRC_CT_MAX_EVT + 1) // key-press action is triggered
#define ESP_AVRC_CT_PT_RSP_TO_EVT (ESP_AVRC_CT_MAX_EVT + 2) // passthrough-command response time-out

void key_control_task_handler(void *arg);

bool key_act_sm_init(void);
void key_act_sm_deinit(void);
void key_act_state_machine(key_act_param_t *param);

#endif /* __KEY_CONTROL_H__ */

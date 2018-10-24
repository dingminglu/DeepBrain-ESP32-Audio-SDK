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

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"

#if defined CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_types.h"
#include "esp_audio_log.h"
#include "userconfig.h"
#include "EspAUdio.h"
#include "toneuri.h"

#define POWER_OFF_TIME_MS                5000
static char *TAG = "PWR_ON";
static xTimerHandle pwr_timer_handle;

#define POWER_ON_OFF_PIN_SEL        ((1ULL<<POWER_ON_OFF_PIN))
#define POWER_DETECT_PIN_SEL        ((1ULL<<POWER_DETECT_PIN))

static void pwr_timer_cb(TimerHandle_t xTimer)
{
    int res;
    res = gpio_get_level(POWER_DETECT_PIN);

    if (res == 0) {
        ets_printf("pwr_timer triggered, will be shoutdown\r\n");
        EspAudioTonePlay(tone_uri[TONE_TYPE_SHUTDOWN], TERMINATION_TYPE_DONE, 0);
        vTaskDelay(3000 / portTICK_RATE_MS);
        gpio_set_level(POWER_ON_OFF_PIN, 0);
    } else {
        ets_printf("pwr_timer trigger failed\r\n");
    }
}

static int pwr_timer_init()
{
    pwr_timer_handle = xTimerCreate("pwr_timer", POWER_OFF_TIME_MS / portTICK_RATE_MS, pdFALSE, (void *) 0, pwr_timer_cb);
    if (pwr_timer_handle == NULL) {
        ESP_LOGE(TAG, "pwr_timer create err");
        return -1;
    }
    return 0;
}

static void pwr_timer_delete()
{
    xTimerDelete(pwr_timer_handle, 100 / portTICK_RATE_MS);
    pwr_timer_handle = NULL;
}


static void IRAM_ATTR pwr_gpio_intr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerResetFromISR(pwr_timer_handle, &xHigherPriorityTaskWoken);
    if ( xHigherPriorityTaskWoken != pdFALSE ) {
        portYIELD_FROM_ISR();
    }
}

int pwr_pin_status_detect()
{
    return gpio_get_level(POWER_DETECT_PIN);
}

void pwr_on_gpio_init()
{
    gpio_set_level(POWER_ON_OFF_PIN, 1);
    gpio_config_t  io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = POWER_ON_OFF_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    gpio_set_level(POWER_ON_OFF_PIN, 1);

    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = POWER_DETECT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    pwr_timer_init();
    gpio_isr_handler_add(POWER_DETECT_PIN, pwr_gpio_intr_handler, NULL);
}
#endif

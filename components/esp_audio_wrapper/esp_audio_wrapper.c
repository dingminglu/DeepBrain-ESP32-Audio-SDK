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

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/queue.h>

#include "esp_audio_wrapper.h"
#include "EspAudio.h"
#include "esp_log.h"

static QueueHandle_t s_que_quit;
static TaskHandle_t ea_wrapper_tsk_handle;
static char *wechat_reply_url = NULL;
static const char *TAG = "ESP_WRAPPER";
typedef struct {
    esp_audio_wrapper_cb    cb;
    void                    *user;
} ea_wrapper_attr_t;

static void esp_audio_wrapper_task (void *para)
{
    ea_wrapper_attr_t *wrapper_cb = (ea_wrapper_attr_t *) para;
    QueueHandle_t que = xQueueCreate(2, sizeof(PlayerStatus));
    s_que_quit = xQueueCreate(1, sizeof(int));
    QueueSetHandle_t xQueSet = xQueueCreateSet(1 + 2);
    if (que == NULL || s_que_quit == NULL || xQueSet == NULL) {
        goto wrapper_exit;
    }
    xQueueAddToSet(que, xQueSet);
    xQueueAddToSet(s_que_quit, xQueSet);
    EspAudioStatusListenerAdd(que);
    PlayerStatus player = {0};
    QueueSetMemberHandle_t activeMember = NULL;
    bool wrapper_tsk_run = true;
    while (wrapper_tsk_run) {
        activeMember = xQueueSelectFromSet( xQueSet, portMAX_DELAY);
        if (activeMember == que) {
            xQueueReceive(que, &player, 0);
            ESP_LOGW(TAG, "*** status:%d, errMsg:%x, mode:%d, musicSrc:%d ***", player.status, player.errMsg, player.mode, player.musicSrc);
            if (wrapper_cb && wrapper_cb->cb) {
                wrapper_cb->cb(player, wrapper_cb->user);
            }
        } else if (activeMember == s_que_quit) {
            int quit = 0;
            xQueueReceive(activeMember, &quit, 0);
            wrapper_tsk_run = false;
            ESP_LOGI(TAG, "esp_audio_wrapper_task will be destroy");
        }
    }
wrapper_exit:
    if (que) {
        vQueueDelete(que);
    }
    if (s_que_quit) {
        vQueueDelete(s_que_quit);
        s_que_quit = NULL;
    }
    if (xQueSet) {
        vQueueDelete(xQueSet);
    }
    if (wrapper_cb) {
        free(wrapper_cb);
    }
    ea_wrapper_tsk_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t esp_audio_wrapper_init(esp_audio_wrapper_cb reg_cb, void *user_data)
{
    ea_wrapper_attr_t *p_wrapper = malloc(sizeof(ea_wrapper_attr_t));
    if (p_wrapper == NULL) {
        ESP_LOGE(TAG, "%s p_wrapper malloc failed", __func__);
        return ESP_FAIL;
    }
    p_wrapper->cb = reg_cb;
    p_wrapper->user = user_data;
    if (NULL == ea_wrapper_tsk_handle) {
        if (xTaskCreate(esp_audio_wrapper_task, "wrapper_tsk", 3 * 1024, p_wrapper, 4, &ea_wrapper_tsk_handle) != pdPASS) {
            ESP_LOGE(TAG, "%s p_wrapper malloc failed", __func__);
            free(p_wrapper);
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t esp_audio_wrapper_deinit()
{
    if (ea_wrapper_tsk_handle) {
        int quit = 1;
        xQueueSend(s_que_quit, &quit, portMAX_DELAY);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t esp_audio_wrapper_music_play(const char *url, int pos, int type)
{
    EspAudioPlayerSrcSet(type);
    int ret = EspAudioPlay(url, pos);
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_music_stop()
{
    EspAudioStop();
    EspAudioPlayerSrcSet(MEDIA_SRC_NULL);
    return ESP_OK;
}

esp_err_t esp_audio_wrapper_http_music_play(const char *url, int pos)
{
    EspAudioPlayerSrcSet(MEDIA_SRC_HTTP);
    int ret = EspAudioPlay(url, pos);
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_sdcard_music_play(const char *url, int pos)
{
    EspAudioPlayerSrcSet(MEDIA_SRC_SD);
    int ret = EspAudioPlay(url, pos);
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_flash_music_play(const char *url, int pos)
{
    EspAudioPlayerSrcSet(MEDIA_SRC_FLASH);
    int ret = EspAudioPlay(url, pos);
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_flash_tone_play(const char *url, bool blocked, bool autoResume)
{
    ESP_LOGI(TAG, "%s, blocked:%d, autoResume:%d, URI:%s ", __func__, blocked, autoResume, url);
    int ret = 0;
    EspAudioPlayerSrcSet(MEDIA_SRC_RESERVE1);  // flash tone type
    if (blocked) {
        ret = EspAudioToneSyncPlay(url, autoResume);
    } else {
        ret = EspAudioTonePlay(url, TERMINATION_TYPE_NOW, autoResume);
    }
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_tone_play(const char *url, bool blocked, bool autoResume, int type)
{
    ESP_LOGI(TAG, "%s, blocked:%d, autoResume:%d, URI:%s ", __func__,  blocked, autoResume, url);
    int ret = 0;
    EspAudioPlayerSrcSet(type);  // flash tone type
    if (blocked) {
        ret = EspAudioToneSyncPlay(url, autoResume);
    } else {
        ret = EspAudioTonePlay(url, TERMINATION_TYPE_NOW, autoResume);
    }
    if (ret != AUDIO_ERR_NO_ERROR) {
        ESP_LOGE(TAG, "%s failed, ret:%x, URI:%s ", __func__, ret, url);
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t esp_audio_wrapper_tone_stop()
{
    EspAudioToneStop();
    EspAudioPlayerSrcSet(MEDIA_SRC_NULL);
    return ESP_OK;
}

esp_err_t esp_audio_wrapper_wechat_play(void)
{
    if (wechat_reply_url) {
        ESP_LOGI(TAG, "Playing wechat reply");
        esp_audio_wrapper_tone_play(wechat_reply_url, false, false, MEDIA_SRC_RESERVE2);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t esp_audio_wrapper_wechat_url_set(const char *url)
{
    if (wechat_reply_url) {
        free(wechat_reply_url);
        wechat_reply_url = NULL;
    }
    if (url) {
        wechat_reply_url = strdup(url);
        if (wechat_reply_url == NULL) {
            ESP_LOGE(TAG, "Store wechat reply failed");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

int esp_audio_wrapper_get_type()
{
    int src = 0;
    EspAudioPlayerSrcGet(&src);
    return src;
}

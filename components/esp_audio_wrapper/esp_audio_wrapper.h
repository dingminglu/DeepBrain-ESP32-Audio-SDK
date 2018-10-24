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

#ifndef ESP_AUDIO_WRAPPER_H
#define ESP_AUDIO_WRAPPER_H

#include "AudioDef.h"

typedef void (*esp_audio_wrapper_cb)(PlayerStatus stat, void * user);

/*
 * init audio wrapper
 *
 * @param cb  audio wrapper callback function
 *
 * @return ESP_OK or ESP_FAIL
 *
 */
esp_err_t esp_audio_wrapper_init(esp_audio_wrapper_cb reg_cb, void *user_data);

/*
 * deinit audio wrapper
 *
 * @param none
 *
 * @return ESP_OK or ESP_FAIL
 *
 */
esp_err_t esp_audio_wrapper_deinit();


esp_err_t esp_audio_wrapper_music_play(const char *url, int pos, int type);

esp_err_t esp_audio_wrapper_music_stop();

esp_err_t esp_audio_wrapper_http_music_play(const char *url, int pos);

esp_err_t esp_audio_wrapper_sdcard_music_play(const char *url, int pos);

esp_err_t esp_audio_wrapper_flash_music_play(const char *url, int pos);


esp_err_t esp_audio_wrapper_flash_tone_play(const char *url, bool blocked, bool autoResume);

esp_err_t esp_audio_wrapper_tone_play(const char *url, bool blocked, bool autoResume, int type);

esp_err_t esp_audio_wrapper_tone_stop();

esp_err_t esp_audio_wrapper_wechat_play(void);

esp_err_t esp_audio_wrapper_wechat_url_set(const char *url);

/*
 * Get audio wrapper audio type
 *
 * @param none
 *
 * @return esp_audio_type_t
 *
 */
int esp_audio_wrapper_get_type();

#endif //

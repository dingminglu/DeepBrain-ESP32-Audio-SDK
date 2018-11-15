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
#include <math.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_audio_log.h"
#include "esp_system.h"
#include "esp_deep_sleep.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"

#include "esp_shell.h"
#include "TerminalControlService.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "ES8388_interface.h"
#include "EspAudioAlloc.h"
#include "userconfig.h"
#include "EspAudio.h"
#include "player_middleware_interface.h"
#include "device_params_manage.h"
#ifdef CONFIG_CLASSIC_BT_ENABLED
#include "esp_a2dp_api.h"
#endif
#include "keyboard_service.h"

#define TERM_TAG                            "TERM_CTRL"
#define TERM_SERV_TASK_PRIORITY             5
#define TERM_SERV_TASK_STACK_SIZE           2048


void playerStatusUpdatedToTerm(ServiceEvent *evt)
{
}


/*---------------------
|    System Commands   |
----------------------*/

#include "soc/rtc_cntl_reg.h"
// need to 0xff600000
void RTC_IRAM_ATTR esp_wake_deep_sleep(void)
{
    esp_default_wake_deep_sleep();
    gpio_pad_unhold(16);
    REG_SET_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
    REG_SET_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x3, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x3, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x3, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);
}

#include "rom/gpio.h"
IRAM_ATTR void terSleep(void *ref, int argc, char *argv[])
{
    int mode;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        mode = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "Ready sleep,mode:%d", mode);
    const int ext_wakeup_pin_1 = 36;
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
    if (mode) {
        esp_deep_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
    } else {
        esp_wifi_stop();
        esp_deep_sleep_enable_ext0_wakeup(ext_wakeup_pin_1, ESP_EXT1_WAKEUP_ALL_LOW);
    }


    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x0, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x0, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x1, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);

#if 0 // RTC pins
    gpio_pulldown_dis(36);
    gpio_pullup_dis(36);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_36);

    gpio_pulldown_dis(37);
    gpio_pullup_dis(37);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_37);

    gpio_pulldown_dis(35);
    gpio_pullup_dis(35);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_35);

    gpio_pulldown_dis(34);
    gpio_pullup_dis(34);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_34);

    gpio_pulldown_dis(33);
    gpio_pullup_dis(33);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_33);

    gpio_pulldown_dis(32);
    gpio_pullup_dis(32);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_32);

    gpio_pulldown_dis(25);
    gpio_pullup_dis(25);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_25);

    gpio_pulldown_dis(26);
    gpio_pullup_dis(26);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_26);

    gpio_pulldown_dis(27);
    gpio_pullup_dis(27);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_27);

#endif


#if 0 // Disable SD card pins
    gpio_pulldown_dis(12);
    gpio_pullup_dis(12);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_12);

    gpio_pulldown_dis(13);
    gpio_pullup_dis(13);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_13);

    gpio_pulldown_dis(14);
    gpio_pullup_dis(14);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_14);

    gpio_pulldown_dis(15);
    gpio_pullup_dis(15);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_15);

    gpio_pulldown_dis(2);
    gpio_pullup_dis(2);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_2);

    gpio_pulldown_dis(0);
    gpio_pullup_dis(0);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_0);

    gpio_pulldown_dis(4);
    gpio_pullup_dis(4);
    PIN_INPUT_DISABLE(GPIO_PIN_REG_4);

#endif

    esp_deep_sleep_start();
}

static void reboot(void *ref, int argc, char *argv[])
{
    esp_restart();
}

static void SwitchBin(void *ref, int argc, char *argv[])
{
    MediaHalPaPwr(0);
    esp_restart();
}

static void getMem(void *ref, int argc, char *argv[])
{
    EspAudioPrintMemory(TERM_TAG);
}

extern void PlayerBackupInfoPrint();
static void backupinfoget(void *ref, int argc, char *argv[])
{
    PlayerBackupInfoPrint();
}

#ifndef CONFIG_CLASSIC_BT_ENABLED
static void wifiInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) {
        ESP_AUDIO_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\"", w_config.sta.ssid);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "failed");
    }
}

static void wifiSet(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    wifi_config_t w_config;
    memset(&w_config, 0x00, sizeof(wifi_config_t));
    if (argc == 2) {
        memcpy(w_config.sta.ssid, argv[0], strlen(argv[0]));
        memcpy(w_config.sta.password, argv[1], strlen(argv[1]));
        esp_wifi_disconnect();
        ESP_AUDIO_LOGI(TERM_TAG, "Connected Wifi SSID:\"%s\" PASSWORD:\"%s\"", w_config.sta.ssid, w_config.sta.password);
        esp_wifi_set_config(WIFI_IF_STA, &w_config);
        esp_wifi_connect();
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "argc != 2");
    }
}
#endif


/*---------------------
|     Codec Commands   |
----------------------*/

static void muteon(void *ref, int argc, char *argv[])
{
    (void)argv;
    MediaHalSetMute(CODEC_MUTE_ENABLE);
    ESP_AUDIO_LOGI(TERM_TAG, "muteon=%d\r\n", MediaHalGetMute());
}

static void muteoff(void *ref, int argc, char *argv[])
{
    (void)argv;
    MediaHalSetMute(CODEC_MUTE_DISABLE);
    ESP_AUDIO_LOGI(TERM_TAG, "muteoff=%d\r\n", MediaHalGetMute());
}

static void auxon(void *ref, int argc, char *argv[])
{
    MediaHalStart(CODEC_MODE_LINE_IN);
    ESP_AUDIO_LOGI(TERM_TAG, "Aux In ON");
}

static void auxoff(void *ref, int argc, char *argv[])
{
    MediaHalStop(CODEC_MODE_LINE_IN);
    MediaHalStart(CODEC_MODE_DECODE);
    ESP_AUDIO_LOGI(TERM_TAG, "Aux In OFF");
}

static void esset(void *ref, int argc, char *argv[])
{
    if (argc == 2) {
        int curVol = 0, reg = 0;
        char num[10] = {0};
        strncpy(num, argv[1], 9);
        curVol = atoi(num);
        strncpy(num, argv[0], 9);
        reg = atoi(num);
        ESP_AUDIO_LOGI(TERM_TAG, "reg:%d set:%d\n", reg, curVol);
        ES8388WriteReg(reg, curVol);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "in %s :argc != 2", __func__);
        return;
    }
}

static void esget(void *ref, int argc, char *argv[])
{
    Es8388ReadAll();
}


/*---------------------
|    Player Commands   |
----------------------*/

static void addUri(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *uri = NULL;
    if (argc == 1) {
        uri = argv[0];
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }

	audio_play_music(uri);
	/*
    if (term->Based.addUri) {
        term->Based.addUri((MediaService *)term, uri);
        term->Based.mediaPlay((MediaService *)term);
    }
    */
    ESP_AUDIO_LOGI(TERM_TAG, "addUri: %s", uri);
}

static void play(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_AUDIO_LOGI(TERM_TAG, "play\r\n");
    // vTaskDelay(2000/portTICK_RATE_MS);
    if (term->Based.mediaPlay)
        term->Based.mediaPlay((MediaService *)term);
}

static void _pause(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_AUDIO_LOGI(TERM_TAG, "pause\r\n");
    if (term->Based.mediaPause)
        term->Based.mediaPause((MediaService *)term);
}

static void resume(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_AUDIO_LOGI(TERM_TAG, "resume\r\n");
    if (term->Based.mediaResume)
        term->Based.mediaResume((MediaService *)term);
}

static void stop(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    (void)argv;
    ESP_AUDIO_LOGI(TERM_TAG, "stop\r\n");
    if (term->Based.mediaStop)
        term->Based.mediaStop((MediaService *)term);
}

static void seekTo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int time;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        time = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "Ready seek to  %ds", time);
    int ret = term->Based.seekByTime((MediaService *)term, time);
    ESP_AUDIO_LOGI(TERM_TAG, "seekByTime ret=%d", ret) ;
}

static void getPlayingTime(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int ret = term->Based.getPosByTime((MediaService *)term);
    ESP_AUDIO_LOGI(TERM_TAG, "getPosByTime %d ms", ret);
    float duration = (float)ret / 1000.0;
    int hours = floor(duration / 3600);
    int minutes = floor((duration - (hours * 3600)) / 60);
    int seconds = duration - (hours * 3600) - (minutes * 60);
    ESP_AUDIO_LOGI(TERM_TAG, "Time is %02d:%02d:%02d",  hours, minutes, seconds);
}

static void getPlayingPos(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int pos = 0;
    int ret = EspAudioPosGet(&pos);
    ESP_AUDIO_LOGI(TERM_TAG, "ret %d: pos is %d", ret, pos);
}

static void setvol(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int curVol = 0;
    if (argc == 1) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        curVol = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    int prevVol;
    prevVol = term->Based.getVolume((MediaService *)term);
    ESP_AUDIO_LOGI(TERM_TAG, "Prev value is %d", prevVol);
    term->Based.setVolume((MediaService *)term, 0, curVol);
    ESP_AUDIO_LOGI(TERM_TAG, "cur value is %d", curVol);
}

static void setvolAmp(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (argc != 1) {
        ESP_AUDIO_LOGE(TERM_TAG, "argc should be 1");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "Previous amplifier is %d", MediaHalGetVolumeAmplify());
    MediaHalSetVolumeAmplify(atof(argv[0]));
    ESP_AUDIO_LOGI(TERM_TAG, "current amplifier is %d", MediaHalGetVolumeAmplify());
}

static void setUpsampling(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int sampleRate;
    if (argc == 1) {
        sampleRate = atoi(argv[0]);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "setUpsampling %d", sampleRate);
    term->Based.upsamplingSetup(sampleRate);
}

static void setDecoder(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *str = NULL;
    if (argc == 1) {
        str = argv[0];
        ESP_AUDIO_LOGI(TERM_TAG, "str: %s", str);
    } else {
        ESP_AUDIO_LOGI(TERM_TAG, "set as NULL");
    }
    EspAudioSetDecoder(str);
}

// CMD: Playtone [URI/index] [TerminateType]
static void playtone(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *string;
    int index = 0;
    int termType = 0;
    if (argc > 0) {
        if (isdigit((int)argv[0][0])) {
            char num[10] = "";
            strncpy(num, argv[0], 9);
            index = atoi(num);
            if (index > getToneUriMaxIndex()) {
                ESP_AUDIO_LOGE(TERM_TAG, "index must be smaller than %d\n", getToneUriMaxIndex());
                return;
            }
            string = tone_uri[index];
            ESP_AUDIO_LOGI(TERM_TAG, "palytone index= %d", index);
        } else {
            ESP_AUDIO_LOGI(TERM_TAG, "palytone %s", argv[0]);
            string = argv[0];
        }
        // Terminate type
        if (argc == 2) {
            char num[10] = "";
            strncpy(num, argv[1], 9);
            termType = atoi(num);
            ESP_AUDIO_LOGI(TERM_TAG, "palytone termType= %d", termType);
        }

    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param, Support CMD: Playtone [URI/index]  or [terminateType]");
        return;
    }
    EspAudioTonePlay((const char *)string, termType, (argc == 3) ? 0 : 1);
}


static void stoptone(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (term->Based.stopTone)
        ESP_AUDIO_LOGI(TERM_TAG, "Tone stop,ret=%x, \r\n", term->Based.stopTone((MediaService *)term));
}

static int rawTaskRun;
static void rawstart(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    //record the data from mic and save into TF-Card
    const char *recUri[] = {
        "i2s://8000:1@record.amr#file",
        "i2s://16000:1@record.opus#file",
        "i2s://16000:1@record.wav#file"
    };
    const char *playbackUri[] = {
        //Play the file recorded in TF-Card
        "file:///sdcard/__record/record.amr",
        "file:///sdcard/__record/record.wav",
    };
    char *string = NULL;
    if (argc > 0) {
        if (isdigit((int)argv[0][0])) {
            char num[10] = "";
            strncpy(num, argv[0], 9);
            int index = atoi(num);
            if (index > sizeof(recUri) / sizeof(char *)) {
                ESP_AUDIO_LOGE(TERM_TAG, "raw start index not corrected[%d]", index);
                return;
            }
            string = recUri[index];
            ESP_AUDIO_LOGI(TERM_TAG, "raw start  index= %d, str: %s", index, string);
        } else {
            ESP_AUDIO_LOGI(TERM_TAG, "raw start  %s", argv[0]);
            string = argv[0];
        }
    } else {
        //record the data from mic and save them into internal buffer, call `EspAudioRawRead()` to read the data from internal buffer
        string = "i2s://16000:1@record.pcm#raw";
        ESP_AUDIO_LOGI(TERM_TAG, "default raw URI: %s", string);
    }
    term->Based.rawStart(term, string);
}

static void rawstop(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int ret = 0;
    if (argc > 0) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        int index = atoi(num);
        ret = term->Based.rawStop(term, index);
        ESP_AUDIO_LOGI(TERM_TAG, "Raw data Stop[%s], ret= %d", index == 0 ? "TERMINATION_TYPE_NOW" : "TERMINATION_TYPE_DONE", ret);
    } else {
        ret = term->Based.rawStop(term, TERMINATION_TYPE_DONE);
        ESP_AUDIO_LOGI(TERM_TAG, "Raw Data Default Stop[TERMINATION_TYPE_DONE], ret= %d", ret);
    }
}

void rawReadTask(void *para)
{
    TerminalControlService *term = (TerminalControlService *) para;
    char *buf = EspAudioAlloc(1, 4096);
    //record the data from mic and save them into internal buffer,
    term->Based.rawStart(term, "i2s://16000:1@record.pcm#raw");
    ESP_AUDIO_LOGI(TERM_TAG, "Raw data reading...,%s", "i2s://16000:1@record.pcm#raw");
    int len = 0;
    while (rawTaskRun) {
        //to read the data from internal buffer
        int ret  = term->Based.rawRead(term, buf, 4096, &len);
        ESP_AUDIO_LOGI(TERM_TAG, "writing..");
        if (AUDIO_ERR_NOT_READY == ret) {
            ESP_AUDIO_LOGE(TERM_TAG, "AUDIO_ERR_NOT_READY");
            break;
        }
    }
    free(buf);
    ESP_AUDIO_LOGI(TERM_TAG, "Raw data read end");
    vTaskDelete(NULL);
}

static void rawread(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    ESP_AUDIO_LOGI(TERM_TAG, "Raw data reading...");
    rawTaskRun = 1;
    if (xTaskCreate(rawReadTask,
                    "rawReadTask",
                    TERM_SERV_TASK_STACK_SIZE,
                    term,
                    TERM_SERV_TASK_PRIORITY,
                    NULL) != pdPASS) {

        ESP_AUDIO_LOGE(TERM_TAG, "ERROR creating rawReadTask task! Out of memory?");
    }
}

void rawWriteTask(void *para)
{
    TerminalControlService *term = (TerminalControlService *) para;
    char *buf = EspAudioAlloc(1, 4096);
    //initialize the player as "buffer to i2s driver" mode
    term->Based.rawStart(term, "raw://from.pcm/to.pcm#i2s");
    ESP_AUDIO_LOGI(TERM_TAG, "Raw data writing...,%s", "raw://from.pcm/to.pcm#i2s");
    while (rawTaskRun) {
        //feed the music data into i2s driver
        if (term->Based.rawWrite(term, buf, 4096, NULL)) {
            break;
        }
        vTaskDelay(10);
    }
    free(buf);
    ESP_AUDIO_LOGI(TERM_TAG, "Raw data read end");
    vTaskDelete(NULL);
}
static void rawwrite(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    rawTaskRun = 1;

    if (xTaskCreate(rawWriteTask,
                    "rawWriteTask",
                    TERM_SERV_TASK_STACK_SIZE,
                    term,
                    TERM_SERV_TASK_PRIORITY,
                    NULL) != pdPASS) {

        ESP_AUDIO_LOGE(TERM_TAG, "ERROR creating rawWriteTask task! Out of memory?");
    }
}

static void getPlayingSongInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    MusicInfo info = {0};
    PlaySrc src;
    int ret = term->Based.getSongInfo((MediaService *)term, &info);
    EspAudioPlayerSrcGet(&src);
    ESP_AUDIO_LOGI(TERM_TAG, "getSongInfo ret=%d", ret);
    ESP_AUDIO_LOGI(TERM_TAG, "source %d", src);
    if (info.name) {
        ESP_AUDIO_LOGI(TERM_TAG, "info.name=%s", info.name);
    }
    ESP_AUDIO_LOGI(TERM_TAG, "info.totalTime=%d ms", info.totalTime);
    ESP_AUDIO_LOGI(TERM_TAG, "info.totalBytes=%d byte", info.totalBytes) ;
    ESP_AUDIO_LOGI(TERM_TAG, "info.bitRates=%d bps", info.bitRates);
    ESP_AUDIO_LOGI(TERM_TAG, "info.sampleRates=%d Hz", info.sampleRates);
    ESP_AUDIO_LOGI(TERM_TAG, "info.channels=%d", info.channels);
    ESP_AUDIO_LOGI(TERM_TAG, "info.bits=%d bit", info.bits);
    if (info.dataLen > 0)
        ESP_AUDIO_LOGI(TERM_TAG, "data: %s, len: %d", info.data == NULL ? "NULL" : info.data, info.dataLen);
}


/*---------------------
|   Playlist Commands  |
----------------------*/

static void next(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 1) {
        ESP_AUDIO_LOGI(TERM_TAG, "next %d\r\n", atoi(argv[0]));

        for (int i = 0; i < atoi(argv[0]); i++) {
            term->Based.mediaNext((MediaService *)term);
        }
    } else  if (argc == 0) {
        ESP_AUDIO_LOGI(TERM_TAG, "next\r\n");
        term->Based.mediaNext((MediaService *)term);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
}

static void prev(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (argc == 1) {
        ESP_AUDIO_LOGI(TERM_TAG, "prev %d\r\n", atoi(argv[0]));

        for (int i = 0; i < atoi(argv[0]); i++) {
            term->Based.mediaPrev((MediaService *)term);
        }
    } else  if (argc == 0) {
        ESP_AUDIO_LOGI(TERM_TAG, "prev\r\n");
        term->Based.mediaPrev((MediaService *)term);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
}

static void setplaymode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    if (argc == 1) {
        if (strncmp(argv[0], "random", strlen("random")) == 0) {
            ESP_AUDIO_LOGI(TERM_TAG, "mode set: random\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_SHUFFLE);
        } else if (strncmp(argv[0], "seq", strlen("seq")) == 0) {
            ESP_AUDIO_LOGI(TERM_TAG, "mode set: sequential\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_SEQUENTIAL);
        } else if (strncmp(argv[0], "repeat", strlen("repeat")) == 0) {
            ESP_AUDIO_LOGI(TERM_TAG, "mode set: one repeat\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_REPEAT);
        } else if (strncmp(argv[0], "one", strlen("one")) == 0) {
            ESP_AUDIO_LOGI(TERM_TAG, "mode set: once\n");
            term->Based.setPlayMode((MediaService *)term, MEDIA_PLAY_ONE_SONG);
        } else {
            ESP_AUDIO_LOGI(TERM_TAG, "mode not support\n");
        }

    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "in %s :argc != 1", __func__);
        return;
    }
}

static void getplaymode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    int mod = 0;
    int ret = term->Based.getPlayMode(term, &mod);
    ESP_AUDIO_LOGI(TERM_TAG, "Play mode %d,ret=%d\n", mod, ret);
}

static void setPlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    EspAudioPlaylistSet(listId, 0);
    ESP_AUDIO_LOGI(TERM_TAG, "Current ListID = %d", EspAudioCurListGet());
}

static void getPlaylistId(void *ref, int argc, char *argv[])
{
    ESP_AUDIO_LOGI(TERM_TAG, "Current ListID = %d", EspAudioCurListGet());
}

static void clearPlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "clear %d list", (int)listId);
    EspAudioPlaylistClear(listId);
}

static void closePlaylist(void *ref, int argc, char *argv[])
{
    char listId;
    if (argc == 1) {
        char num[10] = { 0 };
        strncpy(num, argv[0], 9);
        listId = atoi(num);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "Invalid param");
        return;
    }
    ESP_AUDIO_LOGI(TERM_TAG, "Close %d list", listId);
    EspAudioPlaylistClose(listId);
}

/*
    How to use:

        To save      @ saveuri 7 http://iot.espressif.com:8008/file/Windows.wav
*/
static void saveuri(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 2) {
        if (argv[strlen(argv[1])] == 'n' && argv[strlen(argv[1]) - 1] == '\\') {
            argv[strlen(argv[1]) - 1] = "\n";
            argv[strlen(argv[1])] = "\0";
        }
        ESP_AUDIO_LOGI(TERM_TAG, "save %s to %d", argv[1], atoi(argv[0]));
        EspAudioPlaylistSave(atoi(argv[0]), argv[1]);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "argc != 1");
        return;
    }
}

/*
    How to use:

        To Read    @ readlist 500 10 7 0 (read 500 Bytes from the current playlist with 10 bytes offset in list 7)
                   @ readlist (any) 11 8 (any) (read the offset of 11th song from the current playlist attribution file in list 8)
*/
static void readPlaylist(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 4) {
        ESP_AUDIO_LOGI(TERM_TAG, "read %d bytes with %d offset in %d list", atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
        char *buf = calloc(1, atoi(argv[0]));
        if (buf == NULL) {
            ESP_AUDIO_LOGE(TERM_TAG, "malloc failed");
            return;
        }
        if (atoi(argv[3]) == 0) {
            EspAudioPlaylistRead(atoi(argv[2]), buf, atoi(argv[0]), atoi(argv[1]), 0);
            ESP_AUDIO_LOGI(TERM_TAG, "read %s end", buf);
        } else {
            int offset = 0;
            EspAudioPlaylistRead(atoi(argv[2]), &offset, sizeof(int), atoi(argv[1]), 1);
            ESP_AUDIO_LOGI(TERM_TAG, "offset is %d", offset);
        }
        free(buf);
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "argc != 4");
        return;
    }
}

/*
    How to use:

         @ getlistinfo 7 (get list 7 info)
*/
static void getPlaylistInfo(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;

    if (argc == 1) {
        int size, amount;
        if (EspAudioPlaylistInfo(atoi(argv[0]), &amount, &size) == 0) {
            ESP_AUDIO_LOGI(TERM_TAG, "list id: %d,amount: %d pieces,size: %d bytes", atoi(argv[0]), amount, size);
        }
    } else {
        ESP_AUDIO_LOGE(TERM_TAG, "argc != 1");
        return;
    }
}

#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static esp_err_t run_time_stats(void *ref, int argc, char *argv[])
{
    static char buf[1024];
    vTaskGetRunTimeStats(buf);
    printf("Run Time Stats:\nTask Name    Time    Percent\n%s\n", buf);
    return ESP_OK;
}
#endif

#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
static esp_err_t task_list(void *ref, int argc, char *argv[])
{
    static char buf[1024];
    vTaskList(buf);
    printf("Task List:\nTask Name    Status   Prio    HWM    Task Number\n%s\n", buf);
    return ESP_OK;
}
#endif

void reset_microsemi(void)
{
#if defined CONFIG_ESP_LYRATD_MSC_V2_1_BOARD || defined CONFIG_ESP_LYRATD_MSC_V2_2_BOARD
    gpio_set_level(MICRO_SEMI_RESET_CTRL, 0x00); // reset microsemi dsp
    ets_delay_us(1);
    gpio_set_level(MICRO_SEMI_RESET_CTRL, 0x01); // reset microsemi dsp
#endif
}
#include "tw_spi_access.h"
#include "vprocTwolf_access.h"

static void loadDsp(void *ref, int argc, char *argv[])
{
    ESP_LOGI(TERM_TAG, "MSC loading");
    tw_upload_dsp_firmware(-1);
}

static void get_dsp_status(void *ref, int argc, char *argv[])
{
    uint16_t sta = 0;
    VprocTwolfGetAppStatus(&sta);
    ESP_LOGI(TERM_TAG, "MSC status is %d", sta);
}

static void enPa(void *ref, int argc, char *argv[])
{
    int curVol = 0;
    if (argv[0]) {
        char num[10] = "";
        strncpy(num, argv[0], 9);
        curVol = atoi(num);
    }
    MediaHalPaPwr(curVol);
}
extern const char *ESP_PLAYER_VERSION;

static void show_version(void *ref, int argc, char *argv[])
{
    printf("\r\n");
    printf("----------------------------- Firmware Version -------------------------------\n");
    printf("|                                                                            |\n");
    printf("|                  Board is %s                              |\n",BOARD_INFO);
    int len = strlen(ESP_PLAYER_VERSION);
    int k = (78 - len) / 2;
    printf("|");
    for (int i = 0; i < k; ++i) {
        printf("%c", ' ');
    }
    printf("%s", ESP_PLAYER_VERSION);
    k = 78 - len - (78 - len) / 2 - 2;
    for (int i = 0; i < k; ++i) {
        printf("%c", ' ' );
    }
    printf("|\n" );
    printf("|                  Compile date: %s-%s                        |\n", __DATE__, __TIME__);
    printf("|                  Commit SHA1:%s      |\n", "549f635687dc8157c69aa3dec7710e3bf42e9ca4");
    printf("|                  ESP-IDF :%s                         |\n", IDF_VER);
    printf("------------------------------------------------------------------------------\r\n");
}

static void set_ota_mode(void *ref, int argc, char *argv[])
{
    TerminalControlService *term = (TerminalControlService *)ref;
    char *string;
    int index = 0;
    int termType = 0;
	OTA_UPDATE_MODE_T mode = OTA_UPDATE_MODE_FORMAL;
	
    if (argc > 0) 
	{
        if (strcmp(argv[0], "normal") == 0)
        {
        	ESP_LOGE(TERM_TAG, "otamode %s", argv[0]);
        	mode = OTA_UPDATE_MODE_FORMAL;
			set_flash_cfg(FLASH_CFG_OTA_MODE, &mode);
		}
		else if (strcmp(argv[0], "test") == 0)
		{
			ESP_LOGE(TERM_TAG, "otamode %s", argv[0]);
			mode = OTA_UPDATE_MODE_TEST;
			set_flash_cfg(FLASH_CFG_OTA_MODE, &mode);
		}
		else
		{
			ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: otamode normal or otamode test");
		}
    } 
	else 
	{
        ESP_LOGE(TERM_TAG, "Invalid param, Support CMD: otamode normal or otamode test");
        return;
    }
}

static void keyword_wakeup(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "keyword_wakeup now");
	app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_MSG_TO_ALL, APP_EVENT_KEYWORD_WAKEUP_NOTIFY, NULL, 0);
}

static void wechat_start(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "wechat_start now");
	int key_event = KEY_EVENT_WECHAT_START;
	app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
}

static void wechat_stop(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "wechat_stop now");
	int key_event = KEY_EVENT_WECHAT_STOP;
	app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
}

static void sd_music_start(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "sd music start now");
	app_send_message(APP_NAME_SD_MUSIC_MANAGE, APP_NAME_SD_MUSIC_MANAGE, APP_EVENT_SD_CARD_MUSIC_PLAY, NULL, 0);
}

static void sd_music_prev(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "sd music prev now");
	app_send_message(APP_NAME_SD_MUSIC_MANAGE, APP_NAME_SD_MUSIC_MANAGE, APP_EVENT_SD_CARD_MUSIC_PREV, NULL, 0);
}

static void sd_music_next(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "sd music next now");
	app_send_message(APP_NAME_SD_MUSIC_MANAGE, APP_NAME_SD_MUSIC_MANAGE, APP_EVENT_SD_CARD_MUSIC_NEXT, NULL, 0);
}

static void wifi_config(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "wifi_config now");
	int key_event = KEY_EVENT_WIFI_SETTING;
	app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
}

static void reset_params(void *ref, int argc, char *argv[])
{
	ESP_LOGI(TERM_TAG, "reset_params now");

	if (init_default_params())
	{
		ESP_LOGI(TERM_TAG, "init_default_params success");
	}
	else
	{
		ESP_LOGE(TERM_TAG, "init_default_params failed");
	}

	return;
}


const ShellCommand command[] = {
    //system
    {"------system-------", NULL},
    {"mem", getMem},
    {"reboot", reboot},
    {"version", show_version},
#ifdef CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    {"stat", run_time_stats},
#endif
#ifdef CONFIG_FREERTOS_USE_TRACE_FACILITY
    {"tasks", task_list},
#endif
#ifndef CONFIG_CLASSIC_BT_ENABLED
    {"wifiInfo", wifiInfo},
    {"wifiSet", wifiSet},
#endif

    {"sleep", terSleep},
    {"getback", backupinfoget}, // for debug
    {"SwitchBin", SwitchBin},

    //codec
    {"------codec-------", NULL},
    {"esset", esset},
    {"esget", esget},
    {"muteon", muteon},
    {"muteoff", muteoff},
    {"auxon", auxon},
    {"auxoff", auxoff},
    {"pa", enPa},

    //playlist
    {"------playlist-------", NULL},
    {"setplaylist", setPlaylist},
    {"getplaylistid", getPlaylistId},
    {"clearplaylist", clearPlaylist},
    {"closelist", closePlaylist},
    {"saveuri", saveuri},
    {"readlist", readPlaylist},
    {"setplaymode", setplaymode},
    {"getplaymode", getplaymode},
    {"getlistinfo", getPlaylistInfo},
    {"next", next},
    {"prev", prev},

    //player
    {"------player-------", NULL},
    {"adduri", addUri},
    {"play", play},
    {"pause", _pause},
    {"resume", resume},
    {"stop", stop},
    {"seek", seekTo},
    {"gettime", getPlayingTime},
    {"getpos", getPlayingPos},
    {"setvol", setvol},
    {"setvolAmp", setvolAmp},
    {"setusp", setUpsampling},
    {"setDecoder", setDecoder},

    {"playtone", playtone},
    {"stoptone", stoptone},

    {"rawstart", rawstart},
    {"rawstop", rawstop},
    {"rawread", rawread},
    {"rawwrite", rawwrite},

	{"reset_params", reset_params},
    {"getsonginfo", getPlayingSongInfo},
	{"otamode", set_ota_mode},
	{"keyword_wakeup", keyword_wakeup},
	{"wechat_start", wechat_start},
	{"wechat_stop", wechat_stop},
	{"wifi_config", wifi_config},
	{"sd_music_start", sd_music_start},
	{"sd_music_prev", sd_music_prev},
	{"sd_music_next", sd_music_next},
	
    {"------dsp debu-------", NULL},
    {"resetm", reset_microsemi},
    {"loaddsp", loadDsp},
    {"getdsp", get_dsp_status},
    {NULL, NULL}
};

void terminalControlActive(TerminalControlService *service)
{
    shell_init(command, service);
}
void terminalControlDeactive(TerminalControlService *service)
{
    ESP_AUDIO_LOGI(TERM_TAG, "terminalControlStop\r\n");
    shell_stop();
}

TerminalControlService *TerminalControlCreate()
{
    TerminalControlService *term = (TerminalControlService *) calloc(1, sizeof(TerminalControlService));
    ESP_ERROR_CHECK(!term);

    term->Based.playerStatusUpdated = playerStatusUpdatedToTerm;
    term->Based.serviceActive = terminalControlActive;
    term->Based.serviceDeactive = terminalControlDeactive;

    return term;
}

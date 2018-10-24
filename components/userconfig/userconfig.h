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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "app_config.h"
#include "sdkconfig.h"
#include "board.h"
#include "debug_log_interface.h"
#include "app_framework.h"
#include "task_thread_interface.h"
#include "memory_interface.h"
#include "time_interface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* System functions related */
#define SD_CARD_OPEN_FILE_NUM_MAX   5
#define IDF_3_0                     1
#define TEST_PLAYER                 0
#define EN_STACK_TRACKER            0   // STACK_TRACKER can track every task stack and print the info
#define DEVICE_SN_MAX_LEN 			18

#define MODULE_DAC_DEFFERENT_OUT 	0	//差分输出

#define MIQI_BOARD //米奇开发板

#define PROJECT_NAME  "PJ-20180830-0001-ludu-E3DlpUzSEh3UQSDb"

//app module control
#define AMC_AIP_SHARED_DATA 			1//录音数据共享
#define AMC_KEYWORD_WAKEUP_LEXIN		0//乐鑫唤醒
#define AMC_KEYWORD_WAKEUP_RENMAI		!AMC_KEYWORD_WAKEUP_LEXIN//人麦唤醒
#define AMC_RECORD_PLAYBACK_ENABLE		0//录音回放

#if 0
/******************************************************************
//WIFI AP信息
*******************************************************************/
#define WIFI_CONFIG_MODE_AP			1
#define WIFI_CONFIG_MODE_AIRKISS !WIFI_CONFIG_MODE_AP
#define WIFI_SSID_DEFAULT 		"yiyu"
#define WIFI_PASSWD_DEFAULT 	"12345678"
#define WIFI_DEFAULT_AP_NAME 	 "胡巴WIFI"
#define WIFI_DEFAULT_AP_IPADDR   "192.168.88.1"
#define WIFI_DEFAULT_AP_SUBNET   "255.255.255.0"
#define WIFI_DEFAULT_AP_GATEWAY  "192.168.88.1"
#define WIFI_DEFAULT_AP_DNS      "192.168.88.1"
#endif
/******************************************************************
//WIFI AP信息
*******************************************************************/
#define WIFI_SSID_DEFAULT 		"robot"
#define WIFI_PASSWD_DEFAULT 	"12345678"

/******************************************************************
//设备信息，包括序列号前缀、版本号、账号等
*******************************************************************/
#define PLATFORM_NAME		"Deepbrain-CHIP-ONE"
#define DEVICE_SN_PREFIX	"DBXB"
#define ESP32_FW_VERSION	"V1.0.0build20181024_01"
#define DEEP_BRAIN_APP_ID 	"A000000000000298"
#define DEEP_BRAIN_ROBOT_ID "b36eca71-6d4b-11e8-b77e-801844e30cac"

/******************************************************************
//代码中所有用到的服务器地址
*******************************************************************/
//百度语音识别URL
#define BAIDU_DEFAULT_ASR_URL		"http://vop.baidu.com/server_api"
#define BAIDU_DEFAULT_TTS_URL		"http://tsn.baidu.com/text2audio"
#define BAIDU_DEFAULT_APP_ID		"10734778"
#define BAIDU_DEFAULT_APP_KEY		"LDxCKuuug7qolGBUBWqecR0p"
#define BAIDU_DEFAULT_SECRET_KEY	"BAiGnLSxqGeKrg2f2HW7GCn7Nm8NoeTO"

//讯飞语音识别
#define XFL2W_DEFAULT_ASR_URL		"http://api.xfyun.cn/v1/service/v1/iat"
#define XFL2W_DEFAULT_APP_ID		"5ad6b053"
#define XFL2W_DEFAULT_API_KEY		"71278aebbe262a054e24b85395256916"

//捷通华声语音识别URL
#define SINOVOICE_DEFAULT_ASR_URL	"http://api.hcicloud.com:8880/asr/recognise"
#define SINOVOICE_DEFAULT_TTS_URL	"http://api.hcicloud.com:8880/tts/SynthText"
#define SINOVOICE_DEFAULT_APP_KEY	"745d54c3"
#define SINOVOICE_DEFAULT_DEV_KEY	"ba1a617ff498135a324a109c89db2823"

//云知声账号
#define UNISOUND_DEFAULT_APP_KEY	"gnhsr6flozejxxn4floxbny6nudx5dsmlhilo3qk"
#define UNISOUND_DEFAULT_USER_ID	"YZS15021927852069956"

//OTA升级服务器地址
#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20180425-0001-dabaoxiaobei/idf-3.0/version.txt"
//OTA升级服务器测试地址
#define OTA_UPDATE_SERVER_URL_TEST	"http://file.yuyiyun.com:2088/ota/test/PJ-20180425-0001-dabaoxiaobei/idf-3.0/version.txt"
//deepbrain open api 地址
#define DeepBrain_TEST_URL          "http://api.deepbrain.ai:8383/open-api/service"

/******************************************************************
//产品功能模块控制
*******************************************************************/
#define DEBUG_RECORD_PCM 			0//record pcm data, used to debug pcm
#define USED_ARRAY_MUSIC            0   // 0:disable array music table, 1: enable array music table.
// #define ESP_AUTO_PLAY                   // Define ESP_AUTO_PLAY will be auto select audio codec.

/******************************************************************
//任务优先级定义,数字越大，优先级越高
*******************************************************************/
#define TASK_PRIORITY_1	1
#define TASK_PRIORITY_2	2
#define TASK_PRIORITY_3	3
#define TASK_PRIORITY_4	4
#define TASK_PRIORITY_5	5

#endif /* __USER_CONFIG_H__ */


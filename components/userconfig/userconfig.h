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

#define PROJECT_NAME  "PJ-20180425-0001-dbxb-2z8qgKYsSrtLadUe"

//app module control
#define AMC_AIP_SHARED_DATA 			1	//录音数据共享
#define AMC_KEYWORD_WAKEUP_LEXIN		0	//乐鑫唤醒
#define AMC_KEYWORD_WAKEUP_RENMAI		!AMC_KEYWORD_WAKEUP_LEXIN//人麦唤醒
#define AMC_RECORD_PLAYBACK_ENABLE		0	//录音回放
#define AMC_WEIXIN_CLOUD_ENABLE 		0	//微信云接入
#define ASR_FORMAT_AMR_ENABLE 			0	//使用amr格式录音进行识别

//chat app module control
#define FREE_TALK_CONTINUE_MODE				1
#define FREE_TALK_AUTO_SINGLE_MODE			0
#define FREE_TALK_SINGLE_MODE				0

//wechat app module control
#define WECHAT_CONTINUE_MODE				1
#define WECHAT_SINGLE_MODE					!WECHAT_CONTINUE_MODE

//translate app module control
#define TRANSLATE_CONTINUE_MODE				1
#define TRANSLATE_SINGLE_MODE				!TRANSLATE_CONTINUE_MODE

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
#define ESP32_FW_VERSION	"V1.0.0build20181112"
#if AMC_WEIXIN_CLOUD_ENABLE == 1
#define DEEP_BRAIN_APP_ID 	"460180de15d811e7827e90b11c244b31"
#define DEEP_BRAIN_ROBOT_ID "0dee45b6-bc7f-11e8-86f9-90b11c244b31"
#else
#define DEEP_BRAIN_APP_ID 	"A000000000000298"
#define DEEP_BRAIN_ROBOT_ID "b36eca71-6d4b-11e8-b77e-801844e30cac"
#endif

//OTA升级服务器地址
#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20180425-0001-dabaoxiaobei/idf-3.0/version.txt"
//OTA升级服务器测试地址
#define OTA_UPDATE_SERVER_URL_TEST	"http://file.yuyiyun.com:2088/ota/test/PJ-20180425-0001-dabaoxiaobei/idf-3.0/version.txt"
//deepbrain open api 地址
#define DeepBrain_TEST_URL          "http://api.deepbrain.ai:8383/open-api/service"

/******************************************************************
//任务优先级定义,数字越大，优先级越高
*******************************************************************/
#define TASK_PRIORITY_1	1
#define TASK_PRIORITY_2	2
#define TASK_PRIORITY_3	3
#define TASK_PRIORITY_4	4
#define TASK_PRIORITY_5	5

#endif /* __USER_CONFIG_H__ */


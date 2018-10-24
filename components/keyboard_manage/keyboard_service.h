/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef __KEYBOARD_SERVICE_H__
#define __KEYBOARD_SERVICE_H__
#include <stdio.h>
#include "MediaService.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//按键指令
typedef enum KEY_EVENT_t
{
	KEY_EVENT_START = 0,

	KEY_EVENT_WIFI_SETTING,	//wifi配置
	KEY_EVENT_WIFI_STATUS,	//wifi状态
	KEY_EVENT_NEXT,		//下一曲
	KEY_EVENT_PREV, 	//上一曲
	KEY_EVENT_VOL_DOWN,	//音量-	
	KEY_EVENT_VOL_UP,	//音量+
	KEY_EVENT_CHAT_START,	//1  语聊
	KEY_EVENT_CHAT_STOP,	//2
	KEY_EVENT_CH2EN_START,	//中译英开始
	KEY_EVENT_CH2EN_STOP,	//中译英结束
	KEY_EVENT_SDCARD,		//sd卡播放
	KEY_EVENT_EN2CH_START,	//英译中开始
	KEY_EVENT_EN2CH_STOP,	//英译中结束
	KEY_EVENT_WECHAT_START,	//微聊启动
	KEY_EVENT_WECHAT_STOP,	//微聊停止
	KEY_EVENT_WECHAT_PLAY,	//微聊播放
	KEY_EVENT_PLAY_PAUSE,	//暂停/播放
	KEY_EVENT_LED_OPEN,     //LED open
	KEY_EVENT_LED_CLOSE,    //LED close
	KEY_EVENT_CYCLE_CLOUD, 	//循环云推送
	KEY_EVENT_CYCLE_PRIVATE,//循环私有库
	KEY_EVENT_CHOOSE_MODE,  //选择模式
	
	KEY_EVENT_END
}KEY_EVENT_t;

/**
 * keyborad key cancle
 *
 * @param void
 * @return int
 */
int keyborad_key_cancle(void);

/**
 * create keyboard service 
 *
 * @param self,media service
 * @return none
 */
APP_FRAMEWORK_ERRNO_t keyboard_service_create(int task_priority);

/**
 * keyboard service delete
 *
 * @param none
 * @return none
 */
APP_FRAMEWORK_ERRNO_t keyboard_service_delete(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif


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

#ifndef FREE_TALK_H
#define FREE_TALK_H

#include "userconfig.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define FREE_TALK_SINGLE_MAX_TIME (10*1000) 	//10秒
#define FREE_TALK_SINGLE_DEFAULT_TIME (8*1000) 	//8秒

/* free talk mode */
typedef enum FREE_TALK_MODE_t
{
	FREE_TALK_MODE_MANUAL_SINGLE = 0,			//单次对话,需要手动停止
	FREE_TALK_MODE_AUTO_SINGLE,					//单次对话,vad自动检测停止
	FREE_TALK_MODE_CONTINUE,					//连续对话
	FREE_TALK_MODE_CH2EN_MANUAL_SINGLE,			//单次翻译（中译英），需要手动结束
	FREE_TALK_MODE_EN2CH_MANUAL_SINGLE,			//单次翻译（英译中），需要手动结束
}FREE_TALK_MODE_t;

/* free talk config */
typedef struct FREE_TALK_CONFIG_t
{
	/*
	 * 自由对话模式
	*/
	FREE_TALK_MODE_t mode; 		

	/*
	 * 单次对话最大时长
	 * 默认8,000毫秒
	*/
	uint32_t single_talk_max_ms;	
	
	/*
	 * 静音检测时长,vad检测到多少毫秒时间没人说话，则停止录音
	 * 默认100ms
	*/
	uint32_t vad_detect_ms;	

	/*
	 * 当满足没人说话持续时间，则停止录音，结束对话
	 * 默认30,000毫秒
	*/
	uint32_t nobody_talk_max_ms;

	/*
	 * vad检测灵敏度
	 * 默认0
	*/
	uint32_t vad_detect_sensitivity;

	/*
	 * vad检测最短时间
	 * 默认2000毫秒
	*/
	uint32_t vad_min_detect_ms;
}FREE_TALK_CONFIG_t;

//free talk 提示音播放状态
typedef enum
{
	FREE_TALK_TONE_STATE_INIT,
	FREE_TALK_TONE_STATE_DISABLE_PLAY,
	FREE_TALK_TONE_STATE_ENABLE_PLAY,
}FREE_TALK_TONE_STATE_T;

//free talk 运行状态
typedef enum
{
	FREE_TALK_RUN_STATUS_STOP = 0,
	FREE_TALK_RUN_STATUS_START,
}FREE_TALK_RUN_STATUS_T;

/* free talk */

//当前自由对话状态
typedef enum 
{
	FREE_TALK_STATE_NORMAL,				//正常模式
	FREE_TALK_STATE_TRANSLATE_CH_EN,	//翻译：中译英模式
	FREE_TALK_STATE_TRANSLATE_EN_CH,	//翻译：英译中模式
}FREE_TALK_STATE_T;

APP_FRAMEWORK_ERRNO_t free_talk_create(int task_priority);
APP_FRAMEWORK_ERRNO_t free_talk_delete(void);

/**
 * start free talk
 *
 * @param [in]config free talk config params
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t free_talk_start(FREE_TALK_CONFIG_t *config);

/**
 * stop free talk
 *
 * @param void
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t free_talk_stop(void);

/**
 * terminate free talk all operation
 */
void free_talk_terminated();

/**
 * start translate
 *
 * @param [in]mode free talk mode params
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t translate_start(FREE_TALK_MODE_t mode);

/**
 * stop translate
 *
 * @param void
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t translate_stop(void);

/*
void free_talk_set_state(int state);
int free_talk_get_state(void);
int get_free_talk_status(void);
void set_free_talk_status(int status);
void free_talk_autostart_enable(void);
void free_talk_autostart_disable(void);
void free_talk_auto_start(void);
void free_talk_auto_stop(void);
int is_free_talk_running(void);

//对话前提示音是否播放接口，state为播放选项
#if SPEECH_CTRL_TRANSLATE_MODEL
void set_talk_tone_state(int state);
int get_talk_tone_state();
#endif
*/
#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* FREE_TALK_H */


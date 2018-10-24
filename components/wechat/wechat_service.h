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

#ifndef WECHAT_SERVICE_H
#define WECHAT_SERVICE_H

#include "app_config.h"
#include "device_params_manage.h"
#include "dcl_interface.h"
#include "asr_service.h"
#include "audio_amrnb_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define WECHAT_MAX_RECORD_MS	(10*1000)
#define ASR_MAX_AMR_AUDIO_SIZE 	(WECHAT_MAX_RECORD_MS*AMRNB_ENCODE_OUT_BUFF_SIZE/20 + 9)

/*微聊错误码*/
typedef enum WECHAT_SERVICE_ERRNO_t
{
	WECHAT_SERVICE_ERRNO_OK,	//成功
	WECHAT_SERVICE_ERRNO_FAIL,	//失败
}WECHAT_SERVICE_ERRNO_t;
	
/*amrnb编码缓存*/
typedef struct WECHAT_AMRNB_ENCODE_BUFF_T
{
	char 	 record_data_8k[RAW_PCM_LEN_MS(WECHAT_MAX_RECORD_MS, PCM_SAMPLING_RATE_8K)];
	uint32_t record_data_8k_len;
}WECHAT_AMRNB_ENCODE_BUFF_T;	

/*微聊发送消息队列*/
typedef struct WECHAT_SEND_MSG_t
{
	//tail queue entry
	TAILQ_ENTRY(WECHAT_SEND_MSG_t) next;

	int msg_sn; 								//消息序列号，同录音序列号
	uint64_t start_timestamp;					//录音开始时间
	char amrnb_data[ASR_MAX_AMR_AUDIO_SIZE];	//amrnb录音
	uint32_t amrnb_data_len;					//amrnb数据长度
	WECHAT_AMRNB_ENCODE_BUFF_T amrnb_encode_buff;//amrnb编码缓存
	char asr_result_text[128];					//语音识别结果
}WECHAT_SEND_MSG_t;

//interface

/**
 * create wechat service 
 *
 * @param  task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t wechat_service_create(int task_priority);

/**
 * delete wechat service 
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t wechat_service_delete(void);

/**
 * wechat record start
 *
 * @param none
 * @return none
 */
void wechat_record_start(void);

/**
 * wechat record stop
 *
 * @param none
 * @return none
 */
void wechat_record_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif


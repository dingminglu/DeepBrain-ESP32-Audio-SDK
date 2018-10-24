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

#ifndef DCL_ASR_API_H
#define DCL_ASR_API_H
 
#include "dcl_common_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

//每编码一帧所需PCM毫秒数
#define DEFAULT_PCM_ENCODE_BLOCK_MS (500)

//http发送超时设置
#define DEFAULT_HTTP_SEND_TIMEOUT (2500)

//计算每一帧编码数据大小
#define ADPCM_ONE_FRAME_SIZE (DEFAULT_PCM_ENCODE_BLOCK_MS*32/4)

//计算每一帧编码数据采样点
#define ADPCM_ONE_FRAME_SAMPLES (DEFAULT_PCM_ENCODE_BLOCK_MS*16 - 7)

//计算网速
#define CALC_NETWORK_SPEED(len, ms) ((1.0*len/1024)/(1.0*ms/1000))

/* dcl asr input params */
typedef struct DCL_ASR_INPUT_PARAMS_t
{
	char asr_server_url[128];				//语音识别服务地址
	DCL_ASR_LANG_t asr_lang;				//语音识别语言类型
	DCL_ASR_MODE_t asr_mode;				//语音识别模式
	DCL_AUTH_PARAMS_t dcl_auth_params; 		//权限认证参数
	uint32_t http_send_timeout;				//http发送超时，0~10000毫秒,默认2500毫秒
}DCL_ASR_INPUT_PARAMS_t;

/* dcl asr http buffer*/
typedef struct DCL_ASR_HTTP_BUFFER_t
{
	int32_t 	sock;
	
	uint8_t 	udid[64];	//uuid	
	int         index;		//数据发送索引, 1~n, -n表示录音发送结束
	
	char        http_request[1024*32];
	int         http_request_len;
	
	char      	time_stamp[32];
	char       	nonce[64];
	char       	private_key[64];

	char		domain[64];
	char		port[8];
	char		params[64];

	void		*cjson;
	char		*result_start;
}DCL_ASR_HTTP_BUFFER_t;

/* dcl asr encode buffer*/
typedef struct DCL_ASR_ENCODE_BUFFER_t
{
	void  		*adpcm_ctx; 						//adpcm encode handle
	char     	inpcm[32*1000];						//保存剩余PCM数据
	char     	inpcm_swap[32*1000];				//PCM数据交换缓存
	uint32_t    inpcm_len;							//保存pcm数据长度
	char        adpcm[RAW_PCM_LEN_S(1, PCM_SAMPLING_RATE_16K)/4]; //保存ADPCM数据
	uint32_t    adpcm_len;							//保存ADPCM数据长度
}DCL_ASR_ENCODE_BUFFER_t;

/* dcl network speed statistics */
typedef struct DCL_NETWORK_STATISTICS_t
{
	uint32_t upload_total_bytes;	//上行字节数(字节)
	uint32_t upload_total_time;		//上行时间(毫秒)
	uint32_t upload_speed;			//上行速度(kb/s)
	uint32_t download_total_bytes;	//下行字节数(字节)
	uint32_t download_total_time;	//下行时间(毫秒)
	uint32_t download_speed;		//下行速度(kb/s)
}DCL_NETWORK_STATISTICS_t;

/* dcl asr handle */
typedef struct DCL_ASR_HANDLE_t 
{
	//语音识别参数
	DCL_ASR_INPUT_PARAMS_t asr_params;

	//pcm编码缓存
	DCL_ASR_ENCODE_BUFFER_t encode_buffer;

	//HTTP缓存
	DCL_ASR_HTTP_BUFFER_t http_buffer;

	//网速统计
	DCL_NETWORK_STATISTICS_t network_statistics;
}DCL_ASR_HANDLE_t;

#ifdef __cplusplus
 }
#endif
 
#endif  /*DCL_ASR_API_H*/


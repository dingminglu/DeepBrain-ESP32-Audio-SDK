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
 
#ifndef DP_COMM_LIBRARY_INTERFACE_H
#define DP_COMM_LIBRARY_INTERFACE_H

#include <stdio.h>
#include "app_queue.h"
#include "ctypes_interface.h"
#include "audio_record_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//语义返回结果最大长度
#define MAX_NLP_RESULT_LENGTH (30*1024)

/*DCL global error code define*/
typedef enum DCL_ERROR_CODE_t
{
	//common error code
	DCL_ERROR_CODE_OK = 0,	//成功
	DCL_ERROR_CODE_FAIL,	//失败

	//system error code
	DCL_ERROR_CODE_SYS_INVALID_PARAMS,	//无效参数
	DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM,	//内存不足

	//network error code
	DCL_ERROR_CODE_NETWORK_DNS_FAIL,	//网络DNS解析失败
	DCL_ERROR_CODE_NETWORK_UNAVAILABLE,	//网络不可用
	DCL_ERROR_CODE_NETWORK_POOR,		//网络信号差

	//server error
	DCL_ERROR_CODE_SERVER_ERROR,		//服务器端错误

	//asr error code
	DCL_ERROR_CODE_ASR_SHORT_AUDIO,		//语音太短,小于1秒
	DCL_ERROR_CODE_ASR_ENCODE_FAIL,		//语音识别PCM数据编码失败
	DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL,	//语音识别组包失败	
}DCL_ERROR_CODE_t;

/*DCL asr language*/
typedef enum DCL_ASR_LANG_t
{
	DCL_ASR_LANG_CHINESE 		= 1536,	//普通话(支持简单的英文识别)
	DCL_ASR_LANG_PURE_CHINESE 	= 1537,	//普通话(纯中文识别)
	DCL_ASR_LANG_ENGLISH 		= 1737,	//英语
	DCL_ASR_LANG_CHINESE_YUEYU 	= 1637,	//粤语
	DCL_ASR_LANG_CHINESE_SICHUAN= 1837,	//四川话
	DCL_ASR_LANG_CHINESE_FAR 	= 1936,	//普通话远场
}DCL_ASR_LANG_t;

/*DCL asr mode*/
typedef enum DCL_ASR_MODE_t
{
	DCL_ASR_MODE_ASR = 0,		//语音识别
	DCL_ASR_MODE_ASR_NLP,		//语音识别+语义识别
}DCL_ASR_MODE_t;

/* dcl translate mode */
typedef enum DCL_TRANSLATE_MODE_t
{
	DCL_TRANSLATE_MODE_CH_AND_EN,	//中英互译
	DCL_TRANSLATE_MODE_CH_TO_EN,	//中译英
	DCL_TRANSLATE_MODE_EN_TO_CH		//英译中
}DCL_TRANSLATE_MODE_t;

/*DCL asr input params index*/
typedef enum DCL_ASR_PARAMS_INDEX_t
{
	DCL_ASR_PARAMS_INDEX_LANGUAGE = 0,	//语音识别语言,默认DP_ASR_LANG_CHINESE
	DCL_ASR_PARAMS_INDEX_MODE,			//语音识别模式,默认DCL_ASR_MODE_ASR
	DCL_ASR_PARAMS_INDEX_AUTH_PARAMS,	//权限认证参数,必须设置
}DCL_ASR_PARAMS_INDEX_t;

/*DCL auth params*/
typedef struct DCL_AUTH_PARAMS_t
{
	char str_app_id[64];
	char str_robot_id[64];
	char str_device_id[64];
	char str_user_id[64];
	char str_city_name[64];
	char str_city_longitude[16];
	char str_city_latitude[16];
}DCL_AUTH_PARAMS_t;

/******************************************************************
//语音识别接口
*******************************************************************/
/**
 * start a asr session
 *
 * @param [in,out]asr_handle
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_asr_session_begin(void **asr_handle);

/**
 * send pcm data to asr server
 *
 * @param [in]handle
 * @param [in]pcm_data
 * @param [in]pcm_len
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_asr_audio_write(
	void *handle, 
	const char* const pcm_data,
	const uint32_t pcm_len);

/**
 * send pcm data to asr server
 *
 * @param [in]handle
 * @param [out]asr_result
 * @param [in]result_len, if only run asr, at lease 1024bytes, if run asr and nlp, at least 30*1024 bytes
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_asr_get_result(
	void *handle,
	char* const asr_result, 
	const uint32_t result_len);

/**
 * stop a asr session
 *
 * @param [in]handle
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_asr_session_end(void *handle);

/**
 * set asr session params
 *
 * @param [in]handle
 * @param [in]index, reference enum DCL_ASR_PARAMS_INDEX_t
 * @param [in]param
 * @param [in]param_len
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_asr_set_param(
	void *handle,
	const DCL_ASR_PARAMS_INDEX_t index,
	const void* param,
	const uint32_t param_len);

/******************************************************************
//语义接口
*******************************************************************/
/**
 * get nlp result,input text, output nlp result,json format
 *
 * @param [in]input_params
 * @param [in]input_text
 * @param [out]out_url
 * @param [in]out_url_len, at least 30*1024 bytes
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_get_nlp_result(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text,
	char* const nlp_result,
	const uint32_t nlp_result_len);


/******************************************************************
//TTS接口
*******************************************************************/
/**
 * get tts url,input text, output tts url, mp3 format
 *
 * @param [in]input_params
 * @param [in]input_text
 * @param [out]out_url
 * @param [in]out_url_len, at least 2048 bytes
 * @return dcl error code,reference enum DCL_ERROR_CODE_t
 */
DCL_ERROR_CODE_t dcl_get_tts_url(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text,
	char* const out_url,
	const uint32_t out_url_len);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif


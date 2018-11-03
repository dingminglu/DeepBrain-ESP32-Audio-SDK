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

#include <string.h>
#include "dcl_asr_api.h"
#include "http_api.h"
#include "auth_crypto.h"
#include "adpcm-lib.h"
#include "socket.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "memory_interface.h"
#include "time_interface.h"

#define CONNECTION_KEEP_ALIVE "Keep-Alive"
#define CONNECTION_CLOSE      "Close"
#define X_NLP_SWITCH_YES      "yes"
#define X_NLP_SWITCH_NO       "no"

static const char *TAG_LOG = "[DCL ASR]";

static DCL_ERROR_CODE_t dcl_asr_pcm_encode(
	DCL_ASR_HANDLE_t *asr_handle,
	const char *pcm, 
	const uint32_t pcm_len)
{
	DCL_ASR_HTTP_BUFFER_t *http_buffer = &asr_handle->http_buffer;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = &asr_handle->encode_buffer;
	DCL_ASR_INPUT_PARAMS_t *asr_params = &asr_handle->asr_params;
	const int16_t *pcm_data = (int16_t*)pcm;
	int in_samples = pcm_len / 2;
	int pcm_copy_len = 0;
	int pcm_remain_len = 0;
	int min_pcm_frame_size = ADPCM_ONE_FRAME_SAMPLES * 2;//ADPCM编码一帧需要的最小字节数

	//创建adpcm编码句柄
	if (encode_buffer->adpcm_ctx == NULL) 
	{
		int32_t initial_deltas [2] = {0};
		initial_deltas[0] = pcm_data[0];
		
		while (in_samples--) 
		{
			initial_deltas[0] = (initial_deltas[0] + pcm_data[in_samples]) / 2;
		}
		
		initial_deltas[1] = initial_deltas[0];
		encode_buffer->adpcm_ctx = adpcm_create_context(1, 0, NOISE_SHAPING_OFF, initial_deltas);
		if (encode_buffer->adpcm_ctx == NULL)
		{
			DEBUG_LOGE(TAG_LOG, "adpcm_create_context failed");
			return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
		}
		else
		{
			DEBUG_LOGI(TAG_LOG, "adpcm_create_context success");
		}
	}

	//如果pcm为NULL，表示编码最后一帧数据
	if (pcm == NULL && pcm_len == 0)
	{
		if (encode_buffer->inpcm_len > 0)
		{//存在剩余数据
			adpcm_encode_block(encode_buffer->adpcm_ctx, (uint8_t *)encode_buffer->adpcm, &encode_buffer->adpcm_len, encode_buffer->inpcm, encode_buffer->inpcm_len/2);	
		}
		else
		{//没有剩余数据
			encode_buffer->adpcm_len = 0;
		}
	}
	else if (pcm != NULL && pcm_len > 0)
	{
		//复制pcm数据至inpcm缓存
		if (encode_buffer->inpcm_len + pcm_len > sizeof(encode_buffer->inpcm))
		{
			pcm_copy_len = sizeof(encode_buffer->inpcm) - encode_buffer->inpcm_len;
			pcm_remain_len = pcm_len - pcm_copy_len;
		}
		else
		{
			pcm_copy_len = pcm_len;
			pcm_remain_len = 0;
		}
		memcpy(encode_buffer->inpcm + encode_buffer->inpcm_len, pcm, pcm_copy_len);
		encode_buffer->inpcm_len += pcm_copy_len;
		//DEBUG_LOGI(TAG_LOG, "ctx->inpcm_len[%d],pcm_remain_len[%d],pcm_copy_len[%d][%d]", 
		//		encode_buffer->inpcm_len, pcm_remain_len, pcm_copy_len, sizeof(encode_buffer->inpcm));
		
		if (encode_buffer->inpcm_len >= min_pcm_frame_size)
		{//字节足够，编码一帧
			adpcm_encode_block(encode_buffer->adpcm_ctx, (uint8_t *)encode_buffer->adpcm, &encode_buffer->adpcm_len, encode_buffer->inpcm, ADPCM_ONE_FRAME_SAMPLES);
				encode_buffer->inpcm_len -= min_pcm_frame_size;
			if (encode_buffer->inpcm_len > 0)
			{
				memcpy(encode_buffer->inpcm_swap, encode_buffer->inpcm + min_pcm_frame_size, encode_buffer->inpcm_len);
			}
			
			//DEBUG_LOGI(TAG_LOG, "ctx->inpcm_len[%d],pcm_remain_len[%d]", 
			//	encode_buffer->inpcm_len, pcm_remain_len);
			
			if ((encode_buffer->inpcm_len + pcm_remain_len) > 0 
				&& encode_buffer->inpcm_len + pcm_remain_len <= sizeof(encode_buffer->inpcm))
			{
				memcpy(encode_buffer->inpcm_swap + encode_buffer->inpcm_len, pcm + pcm_copy_len, pcm_remain_len);
					encode_buffer->inpcm_len += pcm_remain_len;
				memcpy(encode_buffer->inpcm, encode_buffer->inpcm_swap, encode_buffer->inpcm_len);
			}
			else if ((encode_buffer->inpcm_len + pcm_remain_len) > 0 
				&& encode_buffer->inpcm_len + pcm_remain_len > sizeof(encode_buffer->inpcm))
			{
				memcpy(encode_buffer->inpcm_swap + encode_buffer->inpcm_len, pcm + pcm_copy_len, sizeof(encode_buffer->inpcm) - encode_buffer->inpcm_len);
					encode_buffer->inpcm_len = sizeof(encode_buffer->inpcm);
				memcpy(encode_buffer->inpcm, encode_buffer->inpcm_swap, encode_buffer->inpcm_len);
				DEBUG_LOGE(TAG_LOG, "pcm data overflow, throw [%d]bytes pcm data", encode_buffer->inpcm_len + pcm_remain_len - sizeof(encode_buffer->inpcm));
			}
			else
			{

			}
		}
		else
		{//字节不够，等到下一帧编码
			encode_buffer->adpcm_len = 0;
		}
	}
	else
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_asr_make_packet(
	DCL_ASR_HANDLE_t *asr_handle)
{
	char *asr_mode = NULL;
	char *connect_mode = NULL;
	DCL_ASR_HTTP_BUFFER_t *http_buffer = &asr_handle->http_buffer;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = &asr_handle->encode_buffer;
	DCL_ASR_INPUT_PARAMS_t *asr_params = &asr_handle->asr_params;
	DCL_AUTH_PARAMS_t *dcl_auth_params = &asr_params->dcl_auth_params; 
	
	if (http_buffer->udid[0] == 0) 
	{
		crypto_generate_nonce_hex(http_buffer->udid, sizeof(http_buffer->udid));
		http_buffer->index = 0;
	}
	
	if (http_buffer->index >= 0) 
	{
		http_buffer->index ++;
	} 
	else if (http_buffer->index < 0) 
	{
		http_buffer->index = http_buffer->index - 1;
	}

	crypto_generate_nonce((unsigned char *)http_buffer->nonce, sizeof(http_buffer->nonce) - 1);
	crypto_time_stamp((unsigned char*)http_buffer->time_stamp, sizeof(http_buffer->time_stamp));
	crypto_generate_private_key((uint8_t *)http_buffer->private_key, sizeof(http_buffer->private_key), http_buffer->nonce, http_buffer->time_stamp, dcl_auth_params->str_robot_id);
	if (asr_params->asr_mode == DCL_ASR_MODE_ASR)
	{
		asr_mode = X_NLP_SWITCH_NO;
	}
	else
	{
		asr_mode = X_NLP_SWITCH_YES;
	}

	if (http_buffer->index < 0)
	{
		connect_mode = CONNECTION_CLOSE;
	}
	else
	{
		connect_mode = CONNECTION_KEEP_ALIVE;
	}
		
	http_buffer->http_request_len = snprintf(http_buffer->http_request, sizeof(http_buffer->http_request), 
			"POST %s HTTP/1.0\r\n"
			"HOST: %s:%s\r\n"
			"Content-Type: application/octet-stream;charset=iso-8859-1\r\n"
			"Connection: %s\r\n"
			"Content-Length: %zu\r\n"
			"CreatedTime: %s\r\n"
			"PrivateKey: %s\r\n"
			"Key: %s\r\n"
			"Nonce: %s\r\n"
			"x-udid: %s\r\n"
			"x-task-config: audioformat=adpcm16k16bit,framesize=%zu,lan=%d,vadSeg=500,index=%d\r\n"
			"x-nlp-switch: %s\r\n"
			"x-user-id: %s\r\n"
			"x-device-id: %s\r\n"
			"x-app-id: %s\r\n"
			"x-robot-id: %s\r\n"
			"x-sdk-version: 0.1\r\n"
			"x-result-format: json\r\n\r\n", 
			http_buffer->params, 
			http_buffer->domain, http_buffer->port,
			connect_mode, 
			encode_buffer->adpcm_len, 
			http_buffer->time_stamp, 
			http_buffer->private_key, 
			dcl_auth_params->str_robot_id, 
			http_buffer->nonce, 
			http_buffer->udid, 
			encode_buffer->adpcm_len, asr_params->asr_lang, http_buffer->index, 
			asr_mode,
			dcl_auth_params->str_user_id, 
			dcl_auth_params->str_device_id, 
			dcl_auth_params->str_app_id, 
			dcl_auth_params->str_robot_id);
	
	//DEBUG_LOGW(TAG_LOG, "headerlen[%d], httpheader: %s", strlen(http_buffer->http_request), http_buffer->http_request);
	memcpy(http_buffer->http_request + http_buffer->http_request_len, encode_buffer->adpcm, encode_buffer->adpcm_len);
	http_buffer->http_request_len += encode_buffer->adpcm_len;

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_asr_decode_packet(
	DCL_ASR_HANDLE_t *asr_handle,
	char* const asr_result, 
	const uint32_t result_len)
{
	DCL_ASR_HTTP_BUFFER_t *http_buffer = &asr_handle->http_buffer;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = &asr_handle->encode_buffer;
	DCL_ASR_INPUT_PARAMS_t *asr_params = &asr_handle->asr_params;

	if (http_buffer->cjson == NULL)
	{
		return DCL_ERROR_CODE_NETWORK_POOR;
	}

	cJSON *pJson_head = cJSON_GetObjectItem(http_buffer->cjson, "responseHead");
	if (pJson_head == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "json node responseHead not found,[%s]", http_buffer->result_start);
		return DCL_ERROR_CODE_SERVER_ERROR;
	}

	if (asr_params->asr_mode == DCL_ASR_MODE_ASR)
	{
		cJSON *pJson_asr_data = cJSON_GetObjectItem(http_buffer->cjson, "asrData");
		if (pJson_asr_data == NULL)
		{
			DEBUG_LOGE(TAG_LOG, "json node asrData not found[%s]", http_buffer->result_start);
			return DCL_ERROR_CODE_SERVER_ERROR;
		}
		
		cJSON *pJson_asr_ret = cJSON_GetObjectItem(pJson_asr_data, "Result");
		if (pJson_asr_ret != NULL && pJson_asr_ret->valuestring != NULL)
		{
			snprintf(asr_result, result_len, "%s", pJson_asr_ret->valuestring);
		}
	}
	else
	{		
		cJSON *pJson_status = cJSON_GetObjectItem(pJson_head, "statusCode");
		if (pJson_status == NULL || pJson_status->valuestring == NULL)
		{
			DEBUG_LOGE(TAG_LOG, "statusCode not found,[%s]", http_buffer->result_start);
			return DCL_ERROR_CODE_SERVER_ERROR;
		}
		
		snprintf(asr_result, result_len, "%s", http_buffer->result_start);
	}

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_asr_reconnect(void *asr_handle)
{
	DCL_ASR_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ASR_HANDLE_t *handler = asr_handle;
	
	if (asr_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_asr_reconnect invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	http_buffer = &handler->http_buffer;

	if (http_buffer->sock != INVALID_SOCK)
	{
		sock_close(http_buffer->sock);
		http_buffer->sock = INVALID_SOCK;
	}

	http_buffer->sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_asr_reconnect fail,[%s:%s]", 
			http_buffer->domain, http_buffer->port);
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	else
	{
		DEBUG_LOGE(TAG_LOG, "dcl_asr_reconnect success,[%s:%s],sock[%d]", 
			http_buffer->domain, http_buffer->port, http_buffer->sock);
	}
	sock_set_nonblocking(http_buffer->sock);
	
	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_asr_session_begin(
	void **asr_handle)
{
	DCL_ASR_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = NULL;
	DCL_ASR_INPUT_PARAMS_t *asr_params = NULL;
	DCL_ASR_HANDLE_t *handler = NULL;
	
	if (asr_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_tts_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	handler = (void *)memory_malloc(sizeof(DCL_ASR_HANDLE_t));
	if (handler == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_tts_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(handler, 0, sizeof(DCL_ASR_HANDLE_t));
	http_buffer = &handler->http_buffer;
	encode_buffer = &handler->encode_buffer;
	asr_params = &handler->asr_params;
	http_buffer->sock = INVALID_SOCK;
	asr_params->asr_lang = DCL_ASR_LANG_CHINESE;
	asr_params->asr_mode = DCL_ASR_MODE_ASR;
	asr_params->http_send_timeout = DEFAULT_HTTP_SEND_TIMEOUT;
	snprintf(asr_params->asr_server_url, sizeof(asr_params->asr_server_url), "%s", DCL_ASR_API_URL);

	if (sock_get_server_info(asr_params->asr_server_url, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_get_server_info failed");
		memory_free(handler);
		handler = NULL;
		return DCL_ERROR_CODE_NETWORK_DNS_FAIL;
	}

	http_buffer->sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "sock_connect fail,[%s:%s]", 
			http_buffer->domain, http_buffer->port);
		memory_free(handler);
		handler = NULL;
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	else
	{
		DEBUG_LOGI(TAG_LOG, "sock_connect success,[%s:%s],sock[%d]", 
			http_buffer->domain, http_buffer->port, http_buffer->sock);
	}
	
	sock_set_nonblocking(http_buffer->sock);

	*asr_handle = handler;
	
	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_asr_audio_write_packet(DCL_ASR_HANDLE_t *handler)
{
	DCL_ASR_HTTP_BUFFER_t *http_buffer = &handler->http_buffer;
	DCL_ASR_INPUT_PARAMS_t *asr_params = &handler->asr_params;
	int ret = 0;
	int send_len = 0;
	int send_count = 0;
	int sock_err = -1;
	do
	{
		send_count++;
		uint64_t send_costtime1 = get_time_of_day();
		ret = sock_writen_with_timeout(
			http_buffer->sock, http_buffer->http_request + send_len, http_buffer->http_request_len - send_len, asr_params->http_send_timeout);
		uint64_t send_costtime2 = get_time_of_day();
		send_len += ret;
		if (send_len != http_buffer->http_request_len)
		{
			sock_err = sock_get_errno(http_buffer->sock);
			
			DEBUG_LOGE(TAG_LOG, "dcl_asr_audio_write_packet index[%d], len[%d], send_len[%d], cost[%llums], sock_err[%d]", 
				http_buffer->index, 
				http_buffer->http_request_len, 
				send_len, 
				send_costtime2 - send_costtime1,
				sock_err);
			
			if (sock_err == 104)
			{
				break;
			}
		}
	}while(send_count < 2);

	if (send_len == http_buffer->http_request_len)
	{
		return DCL_ERROR_CODE_OK;
	}
	else
	{
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
}

DCL_ERROR_CODE_t dcl_asr_audio_write(
	void *handle,
	const char* const pcm_data,
	const uint32_t pcm_len)
{
	int ret = 0;
	int drop = 0;
	DCL_ASR_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = NULL;
	DCL_ASR_INPUT_PARAMS_t *asr_params = NULL;
	DCL_NETWORK_STATISTICS_t *network_sta = NULL;
	DCL_ASR_HANDLE_t *asr_handle = handle;
	uint64_t costtime1 = 0;
	uint64_t costtime2 = 0;
	
	if (asr_handle == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	http_buffer = &asr_handle->http_buffer;
	encode_buffer = &asr_handle->encode_buffer;
	asr_params = &asr_handle->asr_params;
	network_sta = &asr_handle->network_statistics;
	
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "socket already closed");
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	
	//ADPCM编码
	if (dcl_asr_pcm_encode(asr_handle, pcm_data, pcm_len) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_asr_pcm_encode failed");
		return DCL_ERROR_CODE_ASR_ENCODE_FAIL;
	}

	//没有数据，则不需要发送
	if (encode_buffer->adpcm_len == 0 
		&& http_buffer->index >= 0)
	{
		return DCL_ERROR_CODE_OK;
	}

	//组包
	if (dcl_asr_make_packet(asr_handle) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_asr_make_packet failed");
		return DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL;
	}
	
	//发包
	costtime1 = get_time_of_day();
	ret = dcl_asr_audio_write_packet(asr_handle);
	if (ret == DCL_ERROR_CODE_NETWORK_UNAVAILABLE)
	{
		DEBUG_LOGE(TAG_LOG, "[1]dcl_asr_audio_write_packet failed");
		if (dcl_asr_reconnect(asr_handle) != DCL_ERROR_CODE_OK)
		{
			DEBUG_LOGE(TAG_LOG, "dcl_asr_reconnect failed");
			return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
		}

		ret = dcl_asr_audio_write_packet(asr_handle);
		if (ret != DCL_ERROR_CODE_OK)
		{
			DEBUG_LOGE(TAG_LOG, "[2]dcl_asr_audio_write_packet failed");
			return ret;
		}
	}

	if (ret != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "[3]dcl_asr_audio_write_packet failed");
		return ret;
	}
	costtime2 = get_time_of_day();
	
	//发送统计
	network_sta->upload_total_time += costtime2 - costtime1;
	network_sta->upload_total_bytes += http_buffer->http_request_len;
	if (network_sta->upload_total_time == 0)
	{
		network_sta->upload_total_time = 1;
	}
	network_sta->upload_speed = CALC_NETWORK_SPEED(network_sta->upload_total_bytes, network_sta->upload_total_time);
	DEBUG_LOGI(TAG_LOG, "dcl_asr_audio_write index[%d], len[%d], send_len[%d], cost[%llums], upload speed[%dkb/s]", 
		http_buffer->index, 
		http_buffer->http_request_len, 
		http_buffer->http_request_len, 
		costtime2 - costtime1,
		network_sta->upload_speed);
	
	//收包
	if (http_buffer->index > 0) 
	{
		costtime1 = get_time_of_day();
		do
		{		
			ret = recv(http_buffer->sock, &drop, sizeof(drop), MSG_DONTWAIT);
			if (ret > 0)
			{
				network_sta->download_total_bytes += ret;
			}
		}
		while(ret > 0);
		costtime2 = get_time_of_day();
		network_sta->download_total_time += costtime2 - costtime1;
	}

	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_asr_get_result(
	void *handle,
	char* const asr_result, 
	const uint32_t result_len)
{
	int ret = 0;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	DCL_ASR_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = NULL;
	DCL_ASR_INPUT_PARAMS_t *asr_params = NULL;
	DCL_NETWORK_STATISTICS_t *network_sta = NULL;
	DCL_ASR_HANDLE_t *asr_handle = handle;

	if (asr_handle == NULL
		|| asr_result == NULL
		|| result_len == 0)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
		
	http_buffer = &asr_handle->http_buffer;
	encode_buffer = &asr_handle->encode_buffer;
	asr_params = &asr_handle->asr_params;
	network_sta = &asr_handle->network_statistics;

	//发包
	http_buffer->index *= -1;
	uint64_t send_time1 = get_time_of_day();
	err_code = dcl_asr_audio_write(asr_handle, NULL, 0);
	uint64_t send_time2 = get_time_of_day();
	DEBUG_LOGI(TAG_LOG, "send last package audio cost time: %lld ms", send_time2 - send_time1);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		return err_code;
	}

	//接包
	int loop_num = 50;
	memset(http_buffer->http_request, 0, sizeof(http_buffer->http_request));
	while (loop_num--) 
	{
		int ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->http_request, sizeof(http_buffer->http_request), 100);
		if (ret < 0) 
		{
			break;
		}

		network_sta->download_total_bytes += ret;
		http_buffer->http_request[ret] = 0;
		if ((http_buffer->result_start = strstr(http_buffer->http_request, "{"))) 
		{
			http_buffer->cjson = cJSON_Parse(http_buffer->result_start);
			if (http_buffer->cjson == NULL) 
			{
				int start_pos = ret;
				ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->http_request + start_pos, sizeof(http_buffer->http_request) - start_pos, 3000);
				network_sta->download_total_bytes += ret;
				http_buffer->http_request[ret + start_pos] = 0;
				http_buffer->cjson = cJSON_Parse(http_buffer->result_start);
			}
			break;
		}
	}
	uint64_t recv_time = get_time_of_day();
	network_sta->download_total_time += recv_time - send_time2;
	if (network_sta->download_total_time == 0)
	{
		network_sta->download_total_time = 1;
	}
	network_sta->download_speed = CALC_NETWORK_SPEED(network_sta->download_total_bytes, network_sta->download_total_time);
	DEBUG_LOGI(TAG_LOG, "recv packet cost[%lldms], download speed[%dkb/s], sock_errno[%d]", 
		recv_time - send_time2,
		network_sta->download_speed,
		sock_get_errno(http_buffer->sock));
	
	//解包
	err_code = dcl_asr_decode_packet(asr_handle, asr_result, result_len);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		if (http_buffer->result_start != NULL)
		{
			DEBUG_LOGE(TAG_LOG, "dcl_asr_decode_packet failed,[%s]", http_buffer->result_start);
		}
		else
		{
			DEBUG_LOGE(TAG_LOG, "dcl_asr_decode_packet failed,[%s]", http_buffer->http_request);
		}
		
		return err_code;
	}

	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_asr_session_end(
	void *handle)
{
	DCL_ASR_HANDLE_t *asr_handle = handle;
	if (asr_handle == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	DCL_ASR_HTTP_BUFFER_t *http_buffer = &asr_handle->http_buffer;
	DCL_ASR_ENCODE_BUFFER_t *encode_buffer = &asr_handle->encode_buffer;

	//free socket
	if (http_buffer->sock != INVALID_SOCK)
	{
		sock_close(http_buffer->sock);
		http_buffer->sock = INVALID_SOCK;
	}

	//free json object
	if (http_buffer->cjson != NULL)
	{
		cJSON_Delete(http_buffer->cjson);
		http_buffer->cjson = NULL;
	}

	if (encode_buffer->adpcm_ctx != NULL)
	{
		adpcm_free_context(encode_buffer->adpcm_ctx);
		encode_buffer->adpcm_ctx = NULL;
	}

	//free memory
	memory_free(asr_handle);
	asr_handle = NULL;
	
	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_asr_set_param(
	void *handle,
	const DCL_ASR_PARAMS_INDEX_t index,
	const void* param,
	const uint32_t param_len)
{
	DCL_ASR_INPUT_PARAMS_t *asr_param = NULL;
	DCL_ASR_HANDLE_t *asr_handle = handle;
	
	if (asr_handle == NULL 
		|| param == NULL
		|| param_len == 0)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	asr_param = &asr_handle->asr_params;

	switch (index)
	{
		case DCL_ASR_PARAMS_INDEX_LANGUAGE:
		{
			asr_param->asr_lang = *((DCL_ASR_LANG_t*)param);
			break;
		}
		case DCL_ASR_PARAMS_INDEX_MODE:
		{
			asr_param->asr_mode = *((DCL_ASR_MODE_t*)param);
			break;
		}
		case DCL_ASR_PARAMS_INDEX_AUTH_PARAMS:
		{
			asr_param->dcl_auth_params = *((DCL_AUTH_PARAMS_t*)param);
			break;
		}
		default:
			break;
	}

	return DCL_ERROR_CODE_OK;
}
	

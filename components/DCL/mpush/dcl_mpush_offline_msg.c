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
#include "dcl_mpush_offline_msg.h"
#include "http_api.h"
#include "auth_crypto.h"
#include "socket.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "memory_interface.h"
#include "time_interface.h"

static const char *TAG_LOG = "[DCL MPUSH]";

static DCL_ERROR_CODE_t dcl_mpush_session_begin(
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t **handler)
{
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t *new_handler = NULL;
	
	if (handler == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	new_handler = (DCL_MPUSH_OFFLINE_MSG_HANDLER_t *)memory_malloc(sizeof(DCL_MPUSH_OFFLINE_MSG_HANDLER_t));
	if (new_handler == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(new_handler, 0, sizeof(DCL_MPUSH_OFFLINE_MSG_HANDLER_t));
	*handler = new_handler;
	http_buffer = &new_handler->http_buffer;
	http_buffer->sock = INVALID_SOCK;

	if (sock_get_server_info(MPUSH_GET_OFFLINE_MSG_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_get_server_info failed");
		return DCL_ERROR_CODE_NETWORK_DNS_FAIL;
	}

	http_buffer->sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "sock_connect fail,[%s:%s]", 
			http_buffer->domain, http_buffer->port);
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	sock_set_nonblocking(http_buffer->sock);
	
	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_mpush_session_end(
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t *handler)
{
	if (handler == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	DCL_HTTP_BUFFER_t *http_buffer = &handler->http_buffer;

	//free socket
	if (http_buffer->sock != INVALID_SOCK)
	{
		sock_close(http_buffer->sock);
		http_buffer->sock = INVALID_SOCK;
	}

	//free json object
	if (http_buffer->json_body != NULL)
	{
		cJSON_Delete(http_buffer->json_body);
		http_buffer->json_body = NULL;
	}
	
	//free memory
	memory_free(handler);
	handler = NULL;

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_mpush_make_packet(
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t *handler,
	const DCL_AUTH_PARAMS_t* const input_params)
{
	DCL_HTTP_BUFFER_t *http_buffer = &handler->http_buffer;
	
	//make body string
	crypto_generate_request_id(http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	snprintf(http_buffer->req_body, sizeof(http_buffer->req_body),
		"{\"content\":" 
		"{\"fromUserId\": \"\",\"userId\": \"%s\"},"
		"\"osType\": \"esp32\","
		"\"protocolVersion\": \"2.0.0.0\","
		"\"userId\": \"%s\","
		"\"requestId\": \"%s\","
		"\"appId\": \"%s\","
		"\"robotId\": \"%s\"}", 
		input_params->str_device_id,
		input_params->str_device_id,
		http_buffer->str_nonce,
		input_params->str_app_id,
		input_params->str_robot_id);
	
	//make header string
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), 
			http_buffer->str_nonce, 
			http_buffer->str_timestamp, 
			input_params->str_robot_id);
	snprintf(http_buffer->req_header, sizeof(http_buffer->req_header),
		"POST %s HTTP/1.0\r\n"
		"Host: %s:%s\r\n"
		"Accept: application/json\r\n"
		"Accept-Language: zh-cn\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: application/json\r\n"
		"Nonce: %s\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"Connection:close\r\n\r\n", 
		http_buffer->params, 
		http_buffer->domain, 
		http_buffer->port, 
		strlen(http_buffer->req_body), 
		http_buffer->str_nonce, 
		http_buffer->str_timestamp, 
		http_buffer->str_private_key, 
		input_params->str_robot_id);

	//make full request body
	snprintf(http_buffer->str_request, sizeof(http_buffer->str_request), "%s%s", http_buffer->req_header, http_buffer->req_body);

	return DCL_ERROR_CODE_OK;
}


static DCL_ERROR_CODE_t dcl_mpush_decode_packet(
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t *handler,
	char* const msg_result,
	const uint32_t msg_result_len)
{
	DCL_HTTP_BUFFER_t *http_buffer = &handler->http_buffer;

	if (http_get_error_code(http_buffer->str_response) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_response);
		if (pBody != NULL)
		{
			http_buffer->json_body = cJSON_Parse(pBody);
			if (http_buffer->json_body != NULL) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(http_buffer->json_body, "statusCode");
				if (pJson_status == NULL 
					|| pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode not found");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", strlen("OK")) != 0)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode:%s", pJson_status->valuestring);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}
				
		        snprintf(msg_result, msg_result_len, "%s", pBody);
			}
			else
			{
				DEBUG_LOGE(TAG_LOG, "invalid json[%s]", pBody);
				return DCL_ERROR_CODE_NETWORK_POOR;
			}
		}
	}
	else
	{
		DEBUG_LOGE(TAG_LOG, "http reply error[%s]", http_buffer->str_response);
		return DCL_ERROR_CODE_NETWORK_POOR;
	}

	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_mpush_get_offline_msg(
	const DCL_AUTH_PARAMS_t* const input_params,
	char* const msg_result,
	const uint32_t msg_result_len)
{
	int ret = 0;
	DCL_MPUSH_OFFLINE_MSG_HANDLER_t *handler = NULL;
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t start_time = get_time_of_day();

	if (input_params == NULL
		|| msg_result == NULL
		|| msg_result_len == 0)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//建立session
	err_code = dcl_mpush_session_begin(&handler);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin failed");
		goto dcl_mpush_get_offline_msg_error;
	}
	http_buffer = &handler->http_buffer;

	//组包
	err_code = dcl_mpush_make_packet(handler, input_params);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_make_packet failed");
		goto dcl_mpush_get_offline_msg_error;
	}
	//DEBUG_LOGI(TAG_LOG, "%s", http_buffer->str_request);

	//发包
	if (sock_writen_with_timeout(http_buffer->sock, http_buffer->str_request, strlen(http_buffer->str_request), 1000) != strlen(http_buffer->str_request)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_mpush_get_offline_msg_error;
	}
	
	//接包
	memset(http_buffer->str_response, 0, sizeof(http_buffer->str_response));
	ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->str_response, sizeof(http_buffer->str_response) - 1, 2000);
	if (ret <= 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_readn_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_mpush_get_offline_msg_error;
	}
	
	//解包
	err_code = dcl_mpush_decode_packet(handler, msg_result, msg_result_len);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_decode_packet failed");
		goto dcl_mpush_get_offline_msg_error;
	}
	
	DEBUG_LOGI(TAG_LOG, "dcl_mpush_get_offline_msg total time cost[%lldms]", get_time_of_day() - start_time);
	
dcl_mpush_get_offline_msg_error:

	//销毁session
	if (dcl_mpush_session_end(handler) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_end failed");
	}
	
	return err_code;
}


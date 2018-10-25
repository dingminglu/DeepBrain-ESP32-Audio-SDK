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
#include "dcl_nlp_api.h"
#include "http_api.h"
#include "auth_crypto.h"
#include "socket.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "memory_interface.h"
#include "time_interface.h"

static const char *TAG_LOG = "[DCL NLP]";

static DCL_ERROR_CODE_t dcl_nlp_session_begin(
	DCL_NLP_HANDLE_t **nlp_handle)
{
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_NLP_HANDLE_t *handle = NULL;
	
	if (nlp_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	*nlp_handle = (DCL_NLP_HANDLE_t *)memory_malloc(sizeof(DCL_NLP_HANDLE_t));
	if (*nlp_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(*nlp_handle, 0, sizeof(DCL_NLP_HANDLE_t));
	handle = *nlp_handle;
	http_buffer = &handle->http_buffer;
	http_buffer->sock = INVALID_SOCK;

	if (sock_get_server_info(DCL_NLP_API_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
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

static DCL_ERROR_CODE_t dcl_nlp_session_end(
	DCL_NLP_HANDLE_t *nlp_handle)
{
	if (nlp_handle == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	DCL_HTTP_BUFFER_t *http_buffer = &nlp_handle->http_buffer;

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
	memory_free(nlp_handle);
	nlp_handle = NULL;

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_nlp_make_packet(
	DCL_NLP_HANDLE_t *nlp_handle,
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text)
{
	DCL_HTTP_BUFFER_t *http_buffer = &nlp_handle->http_buffer;
	
	//make body string
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), 
			http_buffer->str_nonce, 
			http_buffer->str_timestamp, 
			input_params->str_robot_id);
	snprintf(http_buffer->req_body, sizeof(http_buffer->req_body),
		"{\"location\":"
			"{\"cityName\":\"%s\","
			"\"longitude\":\"%s\","
			"\"latitude\":\"%s\"},"
		"\"requestHead\":"
			"{\"accessToken\":"
				"{\"nonce\":\"%s\","
				"\"privateKey\":\"%s\","
				"\"createdTime\":\"%s\"},"
			"\"apiAccount\":"
				"{\"appId\":\"%s\","
				"\"robotId\":\"%s\","
				"\"userId\":\"%s\","
				"\"deviceId\":\"%s\"}},"
		"\"nlpData\":{\"inputText\":\"%s\"},\"simpleView\":true}",
		input_params->str_city_name, 
		input_params->str_city_longitude, 
		input_params->str_city_latitude,	
		http_buffer->str_nonce, 
		http_buffer->str_private_key,
		http_buffer->str_timestamp,	
		input_params->str_app_id, 
		input_params->str_robot_id, 
		input_params->str_user_id, 
		input_params->str_device_id, 
		input_text);
	
	//make header string
	snprintf(http_buffer->req_header, sizeof(http_buffer->req_header), 
		"POST %s HTTP/1.0\r\n"
	    "Host: %s:%s\r\n"
	    "User-Agent: esp-idf/1.0 esp32\r\n"
	    "Content-Type: application/json; charset=utf-8\r\n"
	    "Accept: */*\r\n"
	    "Content-Length: %d\r\n\r\n",
		http_buffer->params, 
		http_buffer->domain, 
		http_buffer->port, 
		strlen(http_buffer->req_body));

	//make full request body
	snprintf(http_buffer->str_request, sizeof(http_buffer->str_request), "%s%s", http_buffer->req_header, http_buffer->req_body);

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_nlp_decode_packet(
	DCL_NLP_HANDLE_t *nlp_handle,
	char* const out_url,
	const uint32_t out_url_len)
{
	DCL_HTTP_BUFFER_t *http_buffer = &nlp_handle->http_buffer;

	if (http_get_error_code(http_buffer->str_response) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_response);
		if (pBody != NULL)
		{
			http_buffer->json_body = cJSON_Parse(pBody);
			if (http_buffer->json_body != NULL) 
			{
				cJSON *pJson_head = cJSON_GetObjectItem(http_buffer->json_body, "responseHead");
				if (pJson_head == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "responseHead not found,[%s]", pBody);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}
				
				cJSON *pJson_status = cJSON_GetObjectItem(pJson_head, "statusCode");
				if (pJson_status == NULL || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode not found,[%s]", pBody);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", strlen("OK")) != 0)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode:%s", pJson_status->valuestring);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				snprintf(out_url, out_url_len, "%s", pBody);
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

DCL_ERROR_CODE_t dcl_get_nlp_result(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text,
	char* const nlp_result,
	const uint32_t nlp_result_len)
{
	int ret = 0;
	DCL_NLP_HANDLE_t *nlp_handle = NULL;
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t start_time = get_time_of_day();

	if (input_params == NULL
		|| input_text == NULL
		|| strlen(input_text) == 0
		|| nlp_result == NULL
		|| nlp_result_len == 0)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//建立session
	err_code = dcl_nlp_session_begin(&nlp_handle);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_session_begin failed");
		goto dcl_get_nlp_result_error;
	}
	http_buffer = &nlp_handle->http_buffer;
	uint64_t connect_time = get_time_of_day();
	DEBUG_LOGI(TAG_LOG, "connect server cost[%lldms]", connect_time - start_time);

	//组包
	err_code = dcl_nlp_make_packet(nlp_handle, input_params, input_text);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_make_packet failed");
		goto dcl_get_nlp_result_error;
	}
	//DEBUG_LOGI(TAG_LOG, "%s", http_buffer->str_request);

	//发包
	if (sock_writen_with_timeout(http_buffer->sock, http_buffer->str_request, strlen(http_buffer->str_request), 1000) != strlen(http_buffer->str_request)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_nlp_result_error;
	}	
	uint64_t send_time = get_time_of_day();
	DEBUG_LOGI(TAG_LOG, "send packet cost[%lldms]", send_time - connect_time);
	
	//接包
	memset(http_buffer->str_response, 0, sizeof(http_buffer->str_response));
	ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->str_response, sizeof(http_buffer->str_response) - 1, 5000);
	if (ret <= 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_readn_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_nlp_result_error;
	}
	uint64_t recv_time = get_time_of_day();
	DEBUG_LOGI(TAG_LOG, "recv packet cost[%lldms]", recv_time - send_time);
	
	//解包
	err_code = dcl_nlp_decode_packet(nlp_handle, nlp_result, nlp_result_len);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_decode_packet failed");
		goto dcl_get_nlp_result_error;
	}
	
	DEBUG_LOGI(TAG_LOG, "dcl_get_nlp_result total time cost[%lldms]", get_time_of_day() - start_time);
	
dcl_get_nlp_result_error:

	//销毁session
	if (dcl_nlp_session_end(nlp_handle) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_nlp_session_end failed");
	}
	
	return err_code;
}


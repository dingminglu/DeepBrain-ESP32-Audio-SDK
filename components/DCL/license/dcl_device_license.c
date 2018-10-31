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
#include "http_api.h"
#include "auth_crypto.h"
#include "socket.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "dcl_common_interface.h"
#include "dcl_device_license.h"
#include "time_interface.h"
#include "memory_interface.h"

/*
硬件终端获取平台授权接口 ：使用原来的sn生成接口，其中请求字段增加type字段，返回体增加deviceType、deviceId、deviceLicense字段，接口内容如下：
请求体：
{
  "app_id": "A000000000000276",
  "content": {
	"mac": "2ft1331341",
	"projectCode": "k435gfstrt",
	"type":"weChat" //新增字段，weChat即为需要申请微信license，不传或者传deviceSn则只生成sn
  },
  "device_id": "41rett3t34212142142343",
  "request_id": "123213213",
  "robot_id": "8a0997f1-575f-11e8-b77e-801844e30cac",
  "service": "generateDeviceSNService",
  "user_id": "23212141",
  "version": "2.0"
}

返回体：
{
  "responseTimestamp": 1536290100834,
  "serviceTimeInMs": 117805,
  "statusCode": "OK",
  "errorMsg": "",
  "service": "generateDeviceSNService",
  "requestId": "123213213",
  "content": {
    "deviceType": "gh_afbe1c4823a7", //微信平台公众号的id
    "sn": "k435gfstrt18090700000006",
    "deviceId": "gh_afbe1c4823a7_17c5b8a560cf7f98", //微信设备id
    "deviceLicense": "BBC39B4B46AF3C3C5CF8BFDCB09123D4804E0FD597F3268867FA4F5D54BD52CC0415791DE472907F440B5817AB18CE4E623E884B507D9AA83304030DADE74004338BA441D8A5F95BB56F3061DCF149FA" //设备liecense
  }
}

{
	"responseTimestamp": 1536313935682,
	"serviceTimeInMs": 6,
	"statusCode": "OK",
	"errorMsg": "",
	"service": "generateDeviceSNService",
	"requestId": "a04592cd573017462e1e834ce6410e9e",
	"content": {
		"deviceType": "gh_afbe1c4823a7",
		"sn": "HUBA_18090700000005",
		"deviceId": "gh_afbe1c4823a7_400cfd6a6863ad2c",
		"deviceLicense": "B2FAA336D9413B33364C42922EF299F8AA9BABE9F67685CAED41813EF5454B39D3124441EF5DDD584D77EB84EB6311B3F079F5195A3811903C7C396527223365EE21BD1BF75FF906D5176F2936B206B7"
	}
}

*/

static const char *TAG_LOG = "[DCL DEVICE LICENSE]";

/* dcl device license handle */
typedef struct DCL_DEV_LICENSE_HANDLE_t
{
	DCL_HTTP_BUFFER_t http_buffer;
}DCL_DEV_LICENSE_HANDLE_t;

static DCL_ERROR_CODE_t dlc_dev_license_session_begin(
	DCL_DEV_LICENSE_HANDLE_t **dev_handle)
{
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_DEV_LICENSE_HANDLE_t *handle = NULL;
	
	if (dev_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_dev_license_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	handle = (DCL_DEV_LICENSE_HANDLE_t *)memory_malloc(sizeof(DCL_DEV_LICENSE_HANDLE_t));
	if (handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_tts_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(handle, 0, sizeof(DCL_DEV_LICENSE_HANDLE_t));
	http_buffer = &handle->http_buffer;
	http_buffer->sock = INVALID_SOCK;

	if (sock_get_server_info(DCL_SERVER_API_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_get_server_info failed");
		memory_free(handle);
		handle = NULL;
		return DCL_ERROR_CODE_NETWORK_DNS_FAIL;
	}

	http_buffer->sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "sock_connect fail,[%s:%s]", 
			http_buffer->domain, http_buffer->port);
		memory_free(handle);
		handle = NULL;
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	sock_set_nonblocking(http_buffer->sock);

	*dev_handle = handle;
	
	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dlc_dev_license_session_end(
	DCL_DEV_LICENSE_HANDLE_t *handle)
{
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	
	if (handle == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	http_buffer = &handle->http_buffer;

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
	memory_free(handle);
	handle = NULL;

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dlc_dev_license_make_packet(
	DCL_DEV_LICENSE_HANDLE_t *handle,
	const DCL_AUTH_PARAMS_t* const input_params,
	const uint8_t *device_sn_prefix,
	const uint8_t *device_chip_id,
	const bool is_wechat_sn)
{
	DCL_HTTP_BUFFER_t *http_buffer = &handle->http_buffer;
	const char* sn_type_wechat = "weChat";
	const char* sn_type_factory = "deviceSn";
	char *sn_type = NULL;

	if (is_wechat_sn)
	{
		sn_type = sn_type_wechat;
	}
	else
	{
		sn_type = sn_type_factory;
	}
	
	//make body string
	crypto_generate_request_id(http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	snprintf(http_buffer->req_body, sizeof(http_buffer->req_body),
		"{\"app_id\": \"%s\","
	  		"\"content\":" 
				"{\"projectCode\": \"%s\","
				"\"mac\": \"%s\","
				"\"type\":\"%s\"},"
	  		"\"device_id\": \"%s\","
	  		"\"request_id\": \"%s\","
	  		"\"robot_id\": \"%s\","
	  		"\"service\": \"generateDeviceSNService\","
	  		"\"user_id\": \"%s\","
	 		"\"version\": \"2.0\"}", 
		input_params->str_app_id,
		device_sn_prefix,
		device_chip_id,
		sn_type,
		input_params->str_device_id,
		http_buffer->str_nonce,
		input_params->str_robot_id,
		input_params->str_user_id);
	
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

static DCL_ERROR_CODE_t dlc_dev_license_decode_packet(
	DCL_DEV_LICENSE_HANDLE_t *dev_handle,
	DCL_DEVICE_LICENSE_RESULT_t* result,
	const bool is_wechat_sn)
{
	DCL_HTTP_BUFFER_t *http_buffer = &dev_handle->http_buffer;

	if (http_get_error_code(http_buffer->str_response) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_response);
		if (pBody != NULL)
		{
			http_buffer->json_body = cJSON_Parse(pBody);
			if (http_buffer->json_body != NULL) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(http_buffer->json_body, "statusCode");
				if (pJson_status == NULL || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode not found");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", strlen("OK")) != 0)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode:%s", pJson_status->valuestring);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}
				
				cJSON *json_content = cJSON_GetObjectItem(http_buffer->json_body, "content");
				if (json_content == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "json string has no content node");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				cJSON *pJson_sn = cJSON_GetObjectItem(json_content, "sn");
				if (NULL == pJson_sn
					|| pJson_sn->valuestring == NULL
					|| strlen(pJson_sn->valuestring) <= 0)
				{
					DEBUG_LOGE(TAG_LOG, "sn not found");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				DEBUG_LOGI(TAG_LOG, "get device new sn:%s", pJson_sn->valuestring);
				snprintf((char*)&result->device_sn, sizeof(result->device_sn), "%s", pJson_sn->valuestring);

				if (is_wechat_sn)
				{
					cJSON *pJson_device_type = cJSON_GetObjectItem(json_content, "deviceType");
					if (NULL == pJson_device_type
						|| pJson_device_type->valuestring == NULL
						|| strlen(pJson_device_type->valuestring) <= 0)
					{
						DEBUG_LOGE(TAG_LOG, "wechat device type not found");
						return DCL_ERROR_CODE_SERVER_ERROR;
					}
					DEBUG_LOGI(TAG_LOG, "wechat device type:%s", pJson_device_type->valuestring);
					snprintf((char*)&result->wechat_device_type, sizeof(result->wechat_device_type), "%s", pJson_device_type->valuestring);

					cJSON *pJson_device_id = cJSON_GetObjectItem(json_content, "deviceId");
					if (NULL == pJson_device_id
						|| pJson_device_id->valuestring == NULL
						|| strlen(pJson_device_id->valuestring) <= 0)
					{
						DEBUG_LOGE(TAG_LOG, "wechat device type not found");
						return DCL_ERROR_CODE_SERVER_ERROR;
					}
					DEBUG_LOGI(TAG_LOG, "wechat device id:%s", pJson_device_id->valuestring);
					snprintf((char*)&result->wechat_device_id, sizeof(result->wechat_device_id), "%s", pJson_device_id->valuestring);

					cJSON *pJson_device_license = cJSON_GetObjectItem(json_content, "deviceLicense");
					if (NULL == pJson_device_license
						|| pJson_device_license->valuestring == NULL
						|| strlen(pJson_device_license->valuestring) <= 0)
					{
						DEBUG_LOGE(TAG_LOG, "wechat device type not found");
						return DCL_ERROR_CODE_SERVER_ERROR;
					}						
					DEBUG_LOGI(TAG_LOG, "wechat device license:%s", pJson_device_license->valuestring);
					snprintf((char*)&result->wechat_device_license, sizeof(result->wechat_device_license), "%s", pJson_device_license->valuestring);
				}
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

DCL_ERROR_CODE_t dcl_get_device_license(
	const DCL_AUTH_PARAMS_t* const input_params,
	const uint8_t *device_sn_prefix,
	const uint8_t *device_chip_id,
	const bool is_wechat_sn,
	DCL_DEVICE_LICENSE_RESULT_t* result)
{
	int ret = 0;
	DCL_DEV_LICENSE_HANDLE_t *handle = NULL;
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t start_time = get_time_of_day();

	if (input_params == NULL
		|| result == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//建立session
	err_code = dlc_dev_license_session_begin(&handle);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_dev_license_session_begin failed");
		goto dcl_get_device_license_error;
	}
	http_buffer = &handle->http_buffer;

	//组包
	err_code = dlc_dev_license_make_packet(handle, input_params, device_sn_prefix, device_chip_id, is_wechat_sn);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_dev_license_make_packet failed");
		goto dcl_get_device_license_error;
	}
	//DEBUG_LOGI(TAG_LOG, "%s", http_buffer->str_request);

	//发包
	if (sock_writen_with_timeout(http_buffer->sock, http_buffer->str_request, strlen(http_buffer->str_request), 1000) != strlen(http_buffer->str_request)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_device_license_error;
	}
	
	//接包
	memset(http_buffer->str_response, 0, sizeof(http_buffer->str_response));
	ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->str_response, sizeof(http_buffer->str_response) - 1, 2000);
	if (ret <= 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_readn_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_device_license_error;
	}
	
	//解包
	DEBUG_LOGE(TAG_LOG, "http_buffer->str_response[%s]", http_buffer->str_response);
	err_code = dlc_dev_license_decode_packet(handle, result, is_wechat_sn);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_dev_license_decode_packet failed");
		goto dcl_get_device_license_error;
	}
	
	DEBUG_LOGI(TAG_LOG, "dcl_get_device_license total time cost[%lldms]", get_time_of_day() - start_time);
	
dcl_get_device_license_error:

	//销毁session
	if (dlc_dev_license_session_end(handle) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dlc_tts_session_end failed");
	}
	
	return err_code;
}


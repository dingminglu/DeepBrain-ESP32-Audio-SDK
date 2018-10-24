#include "mpush_service.h"
#include "player_middleware_interface.h"
#include "cJSON.h"
#include "auth_crypto.h"
#include "userconfig.h"
#include "wifi_hw_interface.h"
#include "http_api.h"

#define LOG_TAG "MPUSH_SERVICE"

static void* g_lock_mpush_service = NULL;
static MPUSH_SERVICE_HANDLER_t *g_mpush_service_handle = NULL;

int mpush_get_run_status(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	int status = _pHandler->status;
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);
	
	return status;
}

void mpush_set_run_status(MPUSH_SERVICE_HANDLER_t *_pHandler, int status)
{
	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	_pHandler->status = status;
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);
	
	return;
}

int mpush_get_server_sock(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	int sock = _pHandler->server_sock;
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);
	
	return sock;
}

static int mpush_connect(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	MPUSH_SERVER_INFO_T *server_info = NULL;

	if (_pHandler->server_sock != INVALID_SOCK)
	{
		sock_close(_pHandler->server_sock);
		_pHandler->server_sock = INVALID_SOCK;
	}

	server_info = &_pHandler->server_list[_pHandler->server_current_index];
	DEBUG_LOGI(LOG_TAG, "mpush connect %s:%s", 
				server_info->str_server_addr,server_info->str_server_port);
	if (strlen(server_info->str_server_addr) > 0 
		&& strlen(server_info->str_server_port) > 0)
	{
		_pHandler->server_sock = sock_connect(server_info->str_server_addr, server_info->str_server_port);
		if (_pHandler->server_sock == INVALID_SOCK)
		{
			DEBUG_LOGE(LOG_TAG, "mpush sock_connect %s:%s failed", 
				server_info->str_server_addr,server_info->str_server_port);
			return MPUSH_ERROR_CONNECT_FAIL;
		}
		else
		{
			DEBUG_LOGI(LOG_TAG, "mpush sock_connect %s:%s success", 
				server_info->str_server_addr,server_info->str_server_port);
		}
		sock_set_nonblocking(_pHandler->server_sock);
	}
	else
	{
		return MPUSH_ERROR_INVALID_PARAMS;
	}

	return MPUSH_ERROR_CONNECT_OK;
}

static int mpush_disconnect(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	MPUSH_SERVER_INFO_T *server_info = NULL;

	if (_pHandler->server_sock != INVALID_SOCK)
	{
		sock_close(_pHandler->server_sock);
		_pHandler->server_sock = INVALID_SOCK;
	}

	return MPUSH_ERROR_GET_SERVER_OK;
}

static int mpush_handshake(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_handshake_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg, _pHandler->rsa);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_handshake sock_write failed");
		return MPUSH_ERROR_HANDSHAKE_FAIL;
	}
	DEBUG_LOGI(LOG_TAG, "mpush_handshake sock_write success");

	return MPUSH_ERROR_HANDSHAKE_OK;
}

static int mpush_heartbeat(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_heartbeat_msg(_pHandler->str_comm_buf, &len);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_heartbeat sock_write failed");
		return MPUSH_ERROR_HEART_BEAT_FAIL;
	}
	DEBUG_LOGI(LOG_TAG, "mpush_heartbeat sock_write success");

	return MPUSH_ERROR_HEART_BEAT_OK;
}

static int mpush_bind_user(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_bind_user_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg, _pHandler->rsa);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_bind_user sock_write failed");
		return MPUSH_ERROR_BIND_USER_FAIL;
	}
	DEBUG_LOGI(LOG_TAG, "mpush_bind_user sock_write success");

	return MPUSH_ERROR_BIND_USER_OK;
}

static int mpush_add_msg_queue(
	MPUSH_CLIENT_MSG_INFO_t *msg)
{ 
	if (msg == NULL || g_mpush_service_handle == NULL)
	{
		return MPUSH_ERROR_GET_SERVER_FAIL;
	}
	
	MPUSH_MSG_INFO_t *to_msg = memory_malloc(sizeof(MPUSH_MSG_INFO_t));
	if (to_msg == NULL)
	{
		return MPUSH_ERROR_GET_SERVER_FAIL;
	}
	to_msg->msg_info = *msg;

	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	TAILQ_INSERT_TAIL(&g_mpush_service_handle->msg_queue, to_msg, next);	
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);

	return MPUSH_ERROR_GET_SERVER_OK;
}

static int mpush_clear_msg_queue(void)
{
	MPUSH_MSG_INFO_t *msg = NULL;
	
	if (g_mpush_service_handle == NULL)
	{
		return MPUSH_ERROR_GET_SERVER_FAIL;
	}

	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	while ((msg = TAILQ_FIRST(&g_mpush_service_handle->msg_queue)))
	{
		TAILQ_REMOVE(&g_mpush_service_handle->msg_queue, msg, next);
		memory_free(msg);
	}
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);

	return MPUSH_ERROR_GET_SERVER_OK;
}

APP_FRAMEWORK_ERRNO_t mpush_get_msg_queue(MPUSH_CLIENT_MSG_INFO_t *msg_info)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	MPUSH_MSG_INFO_t *msg = NULL;
	
	if (msg_info == NULL || g_mpush_service_handle == NULL)
	{
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	SEMPHR_TRY_LOCK(g_lock_mpush_service);
	msg = TAILQ_FIRST(&g_mpush_service_handle->msg_queue);
	if (msg != NULL)
	{
		memcpy(msg_info, &msg->msg_info, sizeof(msg->msg_info));
		TAILQ_REMOVE(&g_mpush_service_handle->msg_queue, msg, next);
		memory_free(msg);
	}
	else
	{
		err = APP_FRAMEWORK_ERRNO_FAIL;
	}
	SEMPHR_TRY_UNLOCK(g_lock_mpush_service);

	return err;
}

void mpush_client_process_msg(
	MPUSH_SERVICE_HANDLER_t *_pHandler, int msg_type, MPUSH_MSG_HEADER_T *_head)
{
	switch (msg_type)
	{
		case MPUSH_MSG_CMD_HEARTBEAT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_HANDSHAKE:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_HANDSHAKE reply\r\n");
			mpush_set_run_status(_pHandler, MPUSH_STAUTS_HANDSHAK_OK);
			break;
		}
	    case MPUSH_MSG_CMD_LOGIN:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_LOGIN reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_LOGOUT:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_LOGOUT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_BIND:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_BIND reply\r\n");
			mpush_set_run_status(_pHandler, MPUSH_STAUTS_BINDING_OK);
			break;
		}
	    case MPUSH_MSG_CMD_UNBIND:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_UNBIND reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_FAST_CONNECT:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_FAST_CONNECT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_PAUSE:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_PAUSE reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_RESUME:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_RESUME reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_ERROR:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_ERROR reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_OK:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_OK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_HTTP_PROXY:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_HTTP_PROXY reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_KICK:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_KICK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_KICK:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GATEWAY_KICK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_PUSH:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_PUSH reply\r\n");
			MPUSH_CLIENT_MSG_INFO_t *from_msg = _pHandler->str_push_msg;
			
			char buf[32] = {0};
			size_t len = 0;
			mpush_make_ack_msg(buf, &len, _head->sessionId);
			if (sock_writen_with_timeout(_pHandler->server_sock, buf, len, 1000) != len)
			{
				DEBUG_LOGE(LOG_TAG, "sock_writen_with_timeout MPUSH_MSG_CMD_PUSH ack failed\r\n");
			}
			
			mpush_add_msg_queue(from_msg);
			
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_PUSH:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GATEWAY_PUSH reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_NOTIFICATION:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_NOTIFICATION reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_NOTIFICATION:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GATEWAY_NOTIFICATION reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_CHAT:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_CHAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_CHAT:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GATEWAY_CHAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GROUP:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GROUP reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_GROUP:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_GATEWAY_GROUP reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_ACK:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_ACK reply\r\n");
			break;
		}
		case MPUSH_HEARTBEAT_BYTE_REPLY:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_HEARTBEAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_UNKNOWN:
		{
			DEBUG_LOGI(LOG_TAG,"recv MPUSH_MSG_CMD_UNKNOWN reply\r\n");
			break;
		}
		default:
			DEBUG_LOGE(LOG_TAG,"recv MPUSH_MSG_CMD_UNKNOWN(%d) reply\r\n", msg_type);
			break;
	}	
}

int mpush_client_get_device_sn(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	sock_t	sock = INVALID_SOCK;
	MPUSH_HTTP_BUFFER_t *http_buffer = NULL;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto _mpush_client_get_device_sn_error;
	}

	http_buffer = (MPUSH_HTTP_BUFFER_t *)memory_malloc(sizeof(MPUSH_HTTP_BUFFER_t));
	if (http_buffer == NULL)
	{
		goto _mpush_client_get_device_sn_error;
	}
	memset(http_buffer, 0, sizeof(MPUSH_HTTP_BUFFER_t));
	
	if (sock_get_server_info(MPUSH_GET_OPEN_SERVICE_URL, 
			&http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(LOG_TAG,"sock_get_server_info fail\r\n");
		goto _mpush_client_get_device_sn_error;
	}

	sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect fail,%s,%s\r\n", http_buffer->domain, http_buffer->port);
		goto _mpush_client_get_device_sn_error;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect success,%s,%s\r\n", http_buffer->domain, http_buffer->port);
	}

	sock_set_nonblocking(sock);
	// make http head
	get_wifi_mac_str(http_buffer->str_device_id, sizeof(http_buffer->str_device_id));
	crypto_generate_request_id(http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	snprintf(http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1), 
		"{\"app_id\": \"%s\","
  		"\"content\":" 
		"{\"projectCode\": \"%s\",\"mac\": \"%s\"},"
  		"\"device_id\": \"%s\","
  		"\"request_id\": \"%s\","
  		"\"robot_id\": \"%s\","
  		"\"service\": \"generateDeviceSNService\","
  		"\"user_id\": \"%s\","
 		"\"version\": \"2.0\"}", 
 		DEEP_BRAIN_APP_ID, DEVICE_SN_PREFIX, http_buffer->str_device_id, 
 		http_buffer->str_device_id, http_buffer->str_nonce, DEEP_BRAIN_ROBOT_ID, http_buffer->str_device_id);
	
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), http_buffer->str_nonce, http_buffer->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(http_buffer->str_send_buf2, sizeof(http_buffer->str_send_buf2), 
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
		"Connection:close\r\n\r\n%s", 
		http_buffer->params, http_buffer->domain, http_buffer->port, strlen(http_buffer->str_send_buf1), http_buffer->str_nonce, http_buffer->str_timestamp, http_buffer->str_private_key, DEEP_BRAIN_ROBOT_ID, http_buffer->str_send_buf1);

	//DEBUG_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, http_buffer->str_send_buf2, strlen(http_buffer->str_send_buf2), 1000) != strlen(http_buffer->str_send_buf2)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http header fail\r\n");
		goto _mpush_client_get_device_sn_error;
	}

	/* Read HTTP response */
	memset(http_buffer->str_send_buf1, 0, sizeof(http_buffer->str_send_buf1));
	sock_readn_with_timeout(sock, http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(http_buffer->str_send_buf1) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_send_buf1);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode not found");
					goto _mpush_client_get_device_sn_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status->valuestring);
					goto _mpush_client_get_device_sn_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					DEBUG_LOGE(LOG_TAG, "content not found");
					goto _mpush_client_get_device_sn_error;
				}

				cJSON *pJson_sn = cJSON_GetObjectItem(pJson_content, "sn");
				if (NULL == pJson_sn
					|| pJson_sn->valuestring == NULL
					|| strlen(pJson_sn->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "sn not found");
					goto _mpush_client_get_device_sn_error;
				}

				DEBUG_LOGE(LOG_TAG, "get new sn:%s", pJson_sn->valuestring);
				if (set_flash_cfg(FLASH_CFG_DEVICE_ID, pJson_sn->valuestring) == 0)
				{
					snprintf((char*)&_pHandler->msg_cfg.str_device_id, sizeof(_pHandler->msg_cfg.str_device_id), "%s", pJson_sn->valuestring);
					DEBUG_LOGI(LOG_TAG, "save device id[%s] success", pJson_sn->valuestring);
				}
				else
				{
					DEBUG_LOGE(LOG_TAG, "save device id[%s] fail", pJson_sn->valuestring);
					goto _mpush_client_get_device_sn_error;
				}
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "[mpush_client_get_device_sn]%s", http_buffer->str_send_buf1);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "[mpush_client_get_device_sn]%s", http_buffer->str_send_buf1);
		goto _mpush_client_get_device_sn_error;
	}

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	return MPUSH_ERROR_GET_SN_OK;
	
_mpush_client_get_device_sn_error:
	
	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
		sock = INVALID_SOCK;
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_SN_FAIL;
}

static int mpush_client_get_acount_info(MPUSH_SERVICE_HANDLER_t *_pHandler)
{	
	sock_t	sock = INVALID_SOCK;
	MPUSH_HTTP_BUFFER_t *http_buffer = NULL;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto _mpush_client_get_acount_info_error;
	}

	http_buffer = (MPUSH_HTTP_BUFFER_t *)memory_malloc(sizeof(MPUSH_HTTP_BUFFER_t));
	if (http_buffer == NULL)
	{
		goto _mpush_client_get_acount_info_error;
	}
	memset(http_buffer, 0, sizeof(MPUSH_HTTP_BUFFER_t));
	
	if (sock_get_server_info(MPUSH_GET_OPEN_SERVICE_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(LOG_TAG,"sock_get_server_info fail\r\n");
		goto _mpush_client_get_acount_info_error;
	}

	sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect fail,%s,%s\r\n", http_buffer->domain, http_buffer->port);
		goto _mpush_client_get_acount_info_error;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect success,%s,%s\r\n", http_buffer->domain, http_buffer->port);
	}

	sock_set_nonblocking(sock);
	// make http head
	get_flash_cfg(FLASH_CFG_DEVICE_ID, http_buffer->str_device_id);
	crypto_generate_request_id(http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	snprintf(http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1), 
		"{\"app_id\": \"%s\","
  		"\"content\":{},"
  		"\"device_id\": \"%s\","
  		"\"request_id\": \"%s\","
  		"\"robot_id\": \"%s\","
  		"\"service\": \"initResourceService\","
  		"\"user_id\": \"%s\","
 		"\"version\": \"2.0\"}", 
 		DEEP_BRAIN_APP_ID, 
 		http_buffer->str_device_id, http_buffer->str_nonce, DEEP_BRAIN_ROBOT_ID, http_buffer->str_device_id);
	
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), http_buffer->str_nonce, http_buffer->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(http_buffer->str_send_buf2, sizeof(http_buffer->str_send_buf2), 
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
		"Connection:close\r\n\r\n%s", 
		http_buffer->params, http_buffer->domain, http_buffer->port, strlen(http_buffer->str_send_buf1), http_buffer->str_nonce, http_buffer->str_timestamp, http_buffer->str_private_key, DEEP_BRAIN_ROBOT_ID, http_buffer->str_send_buf1);

	//DEBUG_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, http_buffer->str_send_buf2, strlen(http_buffer->str_send_buf2), 1000) != strlen(http_buffer->str_send_buf2)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http header fail\r\n");
		goto _mpush_client_get_acount_info_error;
	}

	/* Read HTTP response */
	memset(http_buffer->str_send_buf1, 0, sizeof(http_buffer->str_send_buf1));
	sock_readn_with_timeout(sock, http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;
	
	//DEBUG_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
	memset(&_pHandler->ai_acounts, 0, sizeof(_pHandler->ai_acounts));
	if (http_get_error_code(http_buffer->str_send_buf1) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_send_buf1);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode not found");
					goto _mpush_client_get_acount_info_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status->valuestring);
					goto _mpush_client_get_acount_info_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					DEBUG_LOGE(LOG_TAG, "content not found");
					goto _mpush_client_get_acount_info_error;
				}

				cJSON *pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_ASR_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_ASR_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_ASR_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.asr_url, sizeof(_pHandler->ai_acounts.st_baidu_acount.asr_url),
					"%s", pJson_string->valuestring);
				
				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_TTS_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_TTS_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_TTS_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.tts_url, sizeof(_pHandler->ai_acounts.st_baidu_acount.tts_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_APP_ID");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_APP_ID not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_APP_ID:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.app_id, sizeof(_pHandler->ai_acounts.st_baidu_acount.app_id),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_APP_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_APP_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_APP_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.app_key, sizeof(_pHandler->ai_acounts.st_baidu_acount.app_key),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_SECRET_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_SECRET_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "BAIDU_DEFAULT_SECRET_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.secret_key, sizeof(_pHandler->ai_acounts.st_baidu_acount.secret_key),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_ASR_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_ASR_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_ASR_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.asr_url, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.asr_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_TTS_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_TTS_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_TTS_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.tts_url, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.tts_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_APP_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_APP_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_APP_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.app_key, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.app_key),
					"%s", pJson_string->valuestring);
				
				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_DEV_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_DEV_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				DEBUG_LOGE(LOG_TAG, "SINOVOICE_DEFAULT_DEV_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.dev_key, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.dev_key),
					"%s", pJson_string->valuestring);

				set_ai_acount(AI_ACOUNT_ALL, &_pHandler->ai_acounts);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "#1#%s", http_buffer->str_send_buf1);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "#2#%s", http_buffer->str_send_buf1);
		goto _mpush_client_get_acount_info_error;
	}

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	return MPUSH_ERROR_GET_ACOUNT_OK;
	
_mpush_client_get_acount_info_error:

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_ACOUNT_FAIL;
}


static int mpush_client_get_server_list(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	sock_t	sock = INVALID_SOCK;
	MPUSH_HTTP_BUFFER_t *http_buffer = NULL;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto mpush_client_get_server_list_error;
	}

	http_buffer = (MPUSH_HTTP_BUFFER_t *)memory_malloc(sizeof(MPUSH_HTTP_BUFFER_t));
	if (http_buffer == NULL)
	{
		goto mpush_client_get_server_list_error;
	}
	memset(http_buffer, 0, sizeof(MPUSH_HTTP_BUFFER_t));
	
	if (sock_get_server_info(MPUSH_GET_SERVER_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(LOG_TAG,"sock_get_server_info fail\r\n");
		goto mpush_client_get_server_list_error;
	}

	sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect fail,%s,%s\r\n", http_buffer->domain, http_buffer->port);
		goto mpush_client_get_server_list_error;
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "sock_connect success,%s,%s\r\n", http_buffer->domain, http_buffer->port);
	}

	sock_set_nonblocking(sock);
	// make http head
	get_flash_cfg(FLASH_CFG_DEVICE_ID, http_buffer->str_device_id);		
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), http_buffer->str_nonce, http_buffer->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(http_buffer->str_send_buf2, sizeof(http_buffer->str_send_buf2), 
		"POST %s HTTP/1.0\r\n"
		"Host: %s:%s\r\n"
		"Accept: application/json\r\n"
		"Accept-Language: zh-cn\r\n"
		"Content-Length: 0\r\n"
		"Content-Type: application/json\r\n"
		"Nonce: %s\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"Connection:close\r\n\r\n", 
		http_buffer->params, http_buffer->domain, http_buffer->port, http_buffer->str_nonce, http_buffer->str_timestamp, http_buffer->str_private_key, DEEP_BRAIN_ROBOT_ID);

	//DEBUG_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, http_buffer->str_send_buf2, strlen(http_buffer->str_send_buf2), 1000) != strlen(http_buffer->str_send_buf2)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http header fail\r\n");
		goto mpush_client_get_server_list_error;
	}

	/* Read HTTP response */
	memset(http_buffer->str_send_buf1, 0, sizeof(http_buffer->str_send_buf1));
	sock_readn_with_timeout(sock, http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1) - 1, 1000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(http_buffer->str_send_buf1) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_send_buf1);
		//snprintf(str_buf, sizeof(str_buf), "{\"serverlist\": [{\"server\": \"192.168.20.91\",\"port\": \"3000\"},{\"server\": \"192.168.20.91\",\"port\": \"3000\"}]}");
		//pBody = &str_buf;
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
		    if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode not found");
					goto mpush_client_get_server_list_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status->valuestring);
					goto mpush_client_get_server_list_error;
				}
	
		        cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					DEBUG_LOGE(LOG_TAG, "content not found");
					goto mpush_client_get_server_list_error;
				}

				cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
				if (NULL == pJson_data)
				{
					DEBUG_LOGE(LOG_TAG, "data not found");
					goto mpush_client_get_server_list_error;
				}

				int list_size = cJSON_GetArraySize(pJson_data);
				if (list_size <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "list_size is 0");
					goto mpush_client_get_server_list_error;
				}

				int i = 0;
				int n = 0;
				for (i = 0; i < list_size && n < MPUSH_SERVER_NUM; i++)
				{
					cJSON *pJson_item = cJSON_GetArrayItem(pJson_data, i);
					cJSON *pJson_server = cJSON_GetObjectItem(pJson_item, "ip");
					cJSON *pJson_port = cJSON_GetObjectItem(pJson_item, "port");
					if (NULL == pJson_server
						|| NULL == pJson_server->valuestring
						|| NULL == pJson_port
						|| NULL == pJson_port->valuestring
						|| strlen(pJson_server->valuestring) <= 0
						|| strlen(pJson_port->valuestring) <= 0)
					{
						continue;
					}

					_pHandler->server_list[n].isValid = 1;
					snprintf(_pHandler->server_list[n].str_server_addr, sizeof(_pHandler->server_list[n].str_server_addr),
						"%s", pJson_server->valuestring);
					snprintf(_pHandler->server_list[n].str_server_port, sizeof(_pHandler->server_list[n].str_server_port),
					"%s", pJson_port->valuestring);
					DEBUG_LOGI(LOG_TAG, "server:%s\tport:%s", 
						pJson_server->valuestring, pJson_port->valuestring);
					n++;
				}

				if (n <= 0)
				{
					goto mpush_client_get_server_list_error;
				}

				_pHandler->server_list_num = n;
		    }
			else
			{
				DEBUG_LOGE(LOG_TAG, "#1#%s", http_buffer->str_send_buf1);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "#2#%s", http_buffer->str_send_buf1);
		goto mpush_client_get_server_list_error;
	}

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	return MPUSH_ERROR_GET_SERVER_OK;
	
mpush_client_get_server_list_error:

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_SERVER_FAIL;
}


int mpush_client_get_offline_msg(MPUSH_SERVICE_HANDLER_t *_pHandler)
{
	sock_t	sock = INVALID_SOCK;
	MPUSH_HTTP_BUFFER_t *http_buffer = NULL;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto mpush_client_get_offline_msg_error;
	}

	http_buffer = (MPUSH_HTTP_BUFFER_t *)memory_malloc(sizeof(MPUSH_HTTP_BUFFER_t));
	if (http_buffer == NULL)
	{
		goto mpush_client_get_offline_msg_error;
	}
	memset(http_buffer, 0, sizeof(MPUSH_HTTP_BUFFER_t));
	
	if (sock_get_server_info(MPUSH_GET_OFFLINE_MSG_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(LOG_TAG,"sock_get_server_info fail\r\n");
		goto mpush_client_get_offline_msg_error;
	}

	sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect fail,%s,%s\r\n", http_buffer->domain, http_buffer->port);
		goto mpush_client_get_offline_msg_error;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect success,%s,%s\r\n", http_buffer->domain, http_buffer->port);
	}
	sock_set_nonblocking(sock);
	
	// make http head
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, http_buffer->str_device_id);
	snprintf(http_buffer->str_send_buf1, sizeof(http_buffer->str_send_buf1), 
		"{\"content\":" 
		"{\"fromUserId\": \"\",\"userId\": \"%s\"},"
		"\"osType\": \"esp32\","
		"\"protocolVersion\": \"2.0.0.0\","
		"\"userId\": \"%s\","
		"\"requestId\": \"%s\","
		"\"appId\": \"%s\","
		"\"robotId\": \"%s\"}", 
		http_buffer->str_device_id, http_buffer->str_device_id, http_buffer->str_nonce, DEEP_BRAIN_APP_ID, DEEP_BRAIN_ROBOT_ID);
	
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), http_buffer->str_nonce, http_buffer->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(http_buffer->str_send_buf2, sizeof(http_buffer->str_send_buf2), 
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
		"Connection:close\r\n\r\n%s", 
		http_buffer->params, http_buffer->domain, http_buffer->port, strlen(http_buffer->str_send_buf1), http_buffer->str_nonce, http_buffer->str_timestamp, http_buffer->str_private_key, DEEP_BRAIN_ROBOT_ID, http_buffer->str_send_buf1);

	//DEBUG_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, http_buffer->str_send_buf2, strlen(http_buffer->str_send_buf2), 1000) != strlen(http_buffer->str_send_buf2)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http header fail\r\n");
		goto mpush_client_get_offline_msg_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode not found");
					goto mpush_client_get_offline_msg_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status->valuestring);
					goto mpush_client_get_offline_msg_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					DEBUG_LOGE(LOG_TAG, "content not found");
					goto mpush_client_get_offline_msg_error;
				}

				cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
				if (NULL == pJson_data)
				{
					DEBUG_LOGE(LOG_TAG, "data not found");
					goto mpush_client_get_offline_msg_error;
				}

				int list_size = cJSON_GetArraySize(pJson_data);
				if (list_size <= 0)
				{
					DEBUG_LOGE(LOG_TAG, "list_size is 0, no offline msg");
				}
				else
				{
					int i = 0;
					for (i = 0; i < list_size; i++)
					{
						cJSON *pJson_item = cJSON_GetArrayItem(pJson_data, i);
						cJSON *pJson_msg_type = cJSON_GetObjectItem(pJson_item, "msgType");
						cJSON *pJson_data = cJSON_GetObjectItem(pJson_item, "data");
						if (NULL == pJson_msg_type
							|| NULL == pJson_msg_type->valuestring
							|| NULL == pJson_data
							|| NULL == pJson_data->valuestring
							|| strlen(pJson_msg_type->valuestring) <= 0
							|| strlen(pJson_data->valuestring) <= 0)
						{
							continue;
						}

						MPUSH_CLIENT_MSG_INFO_t *msg = &_pHandler->msg_info;
						memset(msg , 0, sizeof(MPUSH_CLIENT_MSG_INFO_t));
						if (strncasecmp(pJson_msg_type->valuestring, "text", strlen("text")) == 0)
						{
							msg->msg_type = MPUSH_SEND_MSG_TYPE_TEXT;
							DEBUG_LOGE(LOG_TAG, "[offline msg][text]");
						}
						else if (strncasecmp(pJson_msg_type->valuestring, "file", strlen("file")) == 0)
						{
							msg->msg_type = MPUSH_SEND_MSG_TYPE_FILE;
							DEBUG_LOGE(LOG_TAG, "[offline msg][file]");
						}
						else if (strncasecmp(pJson_msg_type->valuestring, "hylink", strlen("hylink")) == 0)
						{
							msg->msg_type = MPUSH_SEND_MSG_TYPE_LINK;
							DEBUG_LOGE(LOG_TAG, "[offline msg][hylink]");
						}
						else if (strncasecmp(pJson_msg_type->valuestring, "cmd", strlen("cmd")) == 0)
						{
							msg->msg_type = MPUSH_SEND_MSG_TYPE_CMD;
							DEBUG_LOGE(LOG_TAG, "[offline msg][cmd]");
						}
						else
						{
							DEBUG_LOGE(LOG_TAG, "not support msg type:%s", pJson_msg_type->valuestring);
							continue;
						}

						snprintf(msg->msg_content, sizeof(msg->msg_content), "%s", pJson_data->valuestring);
						DEBUG_LOGE(LOG_TAG, "[offline msg][%s]", pJson_data->valuestring);
						mpush_add_msg_queue(msg);
					}
				}
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "#1#%s", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "#2#%s", _pHandler->str_comm_buf);
		goto mpush_client_get_offline_msg_error;
	}

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	return MPUSH_ERROR_GET_OFFLINE_MSG_OK;
	
mpush_client_get_offline_msg_error:

	if (http_buffer != NULL)
	{
		memory_free(http_buffer);
		http_buffer = NULL;
	}
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_OFFLINE_MSG_FAIL;
}

int mpush_client_send_msg(
	MPUSH_SERVICE_HANDLER_t *_pHandler, 
	const int _msg_type, 
	const char *_data, 
	const size_t data_len,
	const char *_nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	char 	str_type[8] = {0};
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		return;
	}
	
	if (sock_get_server_info(MPUSH_SEND_MSG_URL, &domain, &port, &params) != 0)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,sock_get_server_info fail\r\n");
		goto mpush_client_send_msg_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,sock_connect fail,%s,%s\r\n", domain, port);
		goto mpush_client_send_msg_error;
	}

	sock_set_nonblocking(sock);

	if (_msg_type == MPUSH_SEND_MSG_TYPE_FILE)
	{
		snprintf(str_type, sizeof(str_type), "file");
	}
	else if (_msg_type == MPUSH_SEND_MSG_TYPE_TEXT)
	{
		snprintf(str_type, sizeof(str_type), "text");
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,wrong msg type(%d)", _msg_type);
		goto mpush_client_send_msg_error;
	}

	size_t len = 0;
	memset(_pHandler->str_comm_buf1, 0, sizeof(_pHandler->str_comm_buf1));
	if (_msg_type == MPUSH_SEND_MSG_TYPE_FILE)
	{
		crypto_base64_encode((uint8_t*)_pHandler->str_comm_buf1, sizeof(_pHandler->str_comm_buf1), &len, (uint8_t*)_data, data_len);
	}
	else
	{
		snprintf(_pHandler->str_comm_buf1, sizeof(_pHandler->str_comm_buf1), "%s", _data);
	}
	
	crypto_generate_request_id(str_nonce, sizeof(str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf),
		"{\"content\":"
			"{\"data\": \"%s\","
				"\"encrypt\": %s,"
		   		"\"fileType\":\"%s\","
		   		"\"inputTxt\":\"%s\","
		   		"\"msgType\":\"%s\","
		   		"\"toUserId\":\"%s\","
		   		"\"userId\": \"%s\","
		   		"\"userName\":\"%s\"},"
		  "\"appId\":\"%s\","
		  "\"osType\":\"esp32\","
		  "\"protocolVersion\": \"2.0.0.0\","
		  "\"requestId\": \"%s\","
		  "\"robotId\": \"%s\","
		  "\"userId\": \"%s\"}",
		_pHandler->str_comm_buf1, 
		"false", 
		_file_type,
		_nlp_text, 
		str_type,
		_to_user,
		str_device_id,
		str_device_id,
		DEEP_BRAIN_APP_ID,
		str_nonce, 
		DEEP_BRAIN_ROBOT_ID,
		str_device_id);
	
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(str_buf, sizeof(str_buf), 
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
		params, domain, port, strlen(_pHandler->str_comm_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID);

	//DEBUG_LOGE(PRINT_TAG, "%s", str_buf);
	//DEBUG_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, str_buf, strlen(str_buf), 1000) != strlen(str_buf)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http header fail\r\n");
		goto mpush_client_send_msg_error;
	}
	
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 3000) != strlen(_pHandler->str_comm_buf)) 
	{
		DEBUG_LOGE(LOG_TAG,"sock_writen http body fail\r\n");
		goto mpush_client_send_msg_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 8000);
	sock_close(sock);
	sock = INVALID_SOCK;
	
	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,statusCode not found");
					goto mpush_client_send_msg_error;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,statusCode:%s", pJson_status->valuestring);
					goto mpush_client_send_msg_error;
				}
				DEBUG_LOGI(LOG_TAG, "mpush_client_send_msg,statusCode:%s", pJson_status->valuestring);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,invalid json[%s]", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "mpush_client_send_msg,http reply error[%s]", _pHandler->str_comm_buf);
		goto mpush_client_send_msg_error;
	}
	
	return MPUSH_ERROR_SEND_MSG_OK;
	
mpush_client_send_msg_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_SEND_MSG_FAIL;
}


int mpush_client_send_text(
	MPUSH_SERVICE_HANDLER_t * _pHandler,
	const char * _text, 
	const char * _nlp_text,
	const char *_to_user)
{
	return mpush_client_send_msg(_pHandler, MPUSH_SEND_MSG_TYPE_TEXT, _text, strlen(_text), _nlp_text, "", _to_user);
}

int mpush_client_send_file(
	MPUSH_SERVICE_HANDLER_t * _pHandler,
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	return mpush_client_send_msg(_pHandler, MPUSH_SEND_MSG_TYPE_FILE, _data, data_len, _nlp_text, _file_type, _to_user);
}

int mpush_service_send_text(
	const char * _string, 
	const char * _nlp_text,
	const char *_to_user)
{
	if (g_mpush_service_handle == NULL)
	{
		return -1;
	}
	
	return mpush_client_send_text(g_mpush_service_handle, _string, _nlp_text, _to_user);
}

int mpush_service_send_file(
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	if (g_mpush_service_handle == NULL)
	{
		return -1;
	}
	
	return mpush_client_send_file(g_mpush_service_handle, _data, data_len, _nlp_text, _file_type, _to_user);
}

void mpush_client_receive(MPUSH_SERVICE_HANDLER_t *pHandler)
{	
	int sock = INVALID_SOCK;
	int ret = 0;
	MPUSH_MSG_HEADER_T msg_head = {0};
	char *recv_buf = (char *)&msg_head;
	
	sock = mpush_get_server_sock(pHandler);
	if (sock == INVALID_SOCK)
	{
		return;
	}

	//recv msg header
	memset((char*)&msg_head, 0, sizeof(msg_head));

	//recv one byte
	ret = sock_readn_with_timeout(sock, recv_buf, 1, 500);
	if (ret != 1)
	{
		//DEBUG_LOGI(LOG_TAG, "sock_readn msg head first byte failed, ret = %d", ret);
		return;
	}

	if (*recv_buf != 0)
	{
		mpush_client_process_msg(pHandler, *recv_buf, &msg_head);
		return;
	}

	//recv other byte
	ret = sock_readn_with_timeout(sock, recv_buf + 1, sizeof(msg_head) - 1, 500);
	if (ret != sizeof(msg_head) - 1)
	{
		DEBUG_LOGI(LOG_TAG, "sock_readn msg head last 12 bytes failed, ret = %d\r\n", ret);
		return;
	}

	convert_msg_head_to_host(&msg_head);
	print_msg_head(&msg_head);
	
	memset(pHandler->str_recv_buf, 0, sizeof(pHandler->str_recv_buf));
	//recv msg body
	if (msg_head.length == 0)
	{
		DEBUG_LOGE(LOG_TAG, "msg_head.length is 0");
		return;
	}
	else if (msg_head.length <= sizeof(pHandler->str_recv_buf))
	{
		ret = sock_readn_with_timeout(sock, pHandler->str_recv_buf, msg_head.length, 2000);
		if (ret != msg_head.length)
		{
			DEBUG_LOGE(LOG_TAG, "sock_readn msg body failed, ret = %d\r\n", ret);
			return;
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG,"msg is length is too large,length=%d\r\n", msg_head.length);
		int send_len = sizeof(pHandler->str_recv_buf);
		int send_total_len = 0;
		while (send_len > 0)
		{
			ret = sock_readn_with_timeout(sock, pHandler->str_recv_buf, send_len, 2000);
			if (ret != msg_head.length)
			{
				DEBUG_LOGE(LOG_TAG, "sock_readn msg body failed, ret = %d\r\n", ret);
				return;
			}
			else
			{
				send_total_len += ret;
				send_len = msg_head.length - send_total_len;
				if (send_len >= sizeof(pHandler->str_recv_buf))
				{
					send_len = sizeof(pHandler->str_recv_buf);
				}
			}
		}
	}		

	ret = mpush_msg_decode(&msg_head, pHandler->str_recv_buf, 
		pHandler->str_push_msg, sizeof(pHandler->str_push_msg));
	mpush_client_process_msg(pHandler, ret, &msg_head);
}

static void mpush_client_process(MPUSH_SERVICE_HANDLER_t *pHandler)
{
	static uint32_t heart_time = 0;

	switch (mpush_get_run_status(pHandler))
	{
		case MPUSH_STAUTS_INIT:
		{
			char device_sn[32] = {0};
			get_flash_cfg(FLASH_CFG_DEVICE_ID, device_sn);
			if (strlen(device_sn) > 0)
			{
				snprintf((char*)&pHandler->msg_cfg.str_device_id, sizeof(pHandler->msg_cfg.str_device_id), "%s", device_sn);
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_ACOUNT_INFO);
				break;
			}
			
			//设备激活操作，获取设备唯一序列号
			if (mpush_client_get_device_sn(pHandler) == MPUSH_ERROR_GET_SN_OK)
			{
				DEBUG_LOGI(LOG_TAG, "mpush_client_get_device_sn success");
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_ACOUNT_INFO);
				//激活成功提示音
				audio_play_tone_mem(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_OK, TERMINATION_TYPE_DONE);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "mpush_client_get_device_sn fail");				
				//激活失败提示音
				audio_play_tone_mem(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_FAIL, TERMINATION_TYPE_DONE);
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_GET_ACOUNT_INFO:
		{
			//获取账号信息
			if (mpush_client_get_acount_info(pHandler) == MPUSH_ERROR_GET_ACOUNT_OK)
			{
				DEBUG_LOGI(LOG_TAG, "mpush_client_get_acount_info success");
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_SERVER_LIST);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "mpush_client_get_acount_info fail");	
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_GET_SERVER_LIST:
		{
			//获取服务器列表
			if (mpush_client_get_server_list(pHandler) == MPUSH_ERROR_GET_SERVER_OK)
			{
				DEBUG_LOGI(LOG_TAG, "mpush_client_get_server_list success");
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_OFFLINE_MSG);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "mpush_client_get_server_list fail");	
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_GET_OFFLINE_MSG:
		{
			//获取离线消息
			if (mpush_client_get_offline_msg(pHandler) == MPUSH_ERROR_GET_OFFLINE_MSG_OK)
			{
				DEBUG_LOGI(LOG_TAG, "mpush_client_get_offline_msg success");
				mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "mpush_client_get_offline_msg fail");	
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_CONNECTING:
		{
			srand(pHandler->server_list_num);
			pHandler->server_current_index = rand();
			if (pHandler->server_current_index >= MPUSH_SERVER_NUM)
			{
				pHandler->server_current_index = 0;
			}
			
			if (mpush_connect(pHandler) == MPUSH_ERROR_CONNECT_OK)
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_HANDSHAKING);
			}
			else
			{
				task_thread_sleep(5000);
			}
			heart_time = time(NULL);
			break;
		}
		case MPUSH_STAUTS_HANDSHAKING:
		{
			if (mpush_handshake(pHandler) == MPUSH_ERROR_HANDSHAKE_OK)
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_HANDSHAK_WAIT_REPLY);
			}
			else
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
				task_thread_sleep(5000);
			}

			break;
		}
		case MPUSH_STAUTS_HANDSHAK_WAIT_REPLY:
		{
			mpush_client_receive(pHandler);
			break;
		}
		case MPUSH_STAUTS_HANDSHAK_OK:
		{
			mpush_set_run_status(pHandler, MPUSH_STAUTS_BINDING);
			break;
		}
		case MPUSH_STAUTS_HANDSHAK_FAIL:
		{
			break;
		}
		case MPUSH_STAUTS_BINDING:
		{
			if (mpush_bind_user(pHandler) == MPUSH_ERROR_BIND_USER_OK)
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_BINDING_WAIT_REPLY);
			}
			else
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_BINDING_WAIT_REPLY:
		{
			mpush_client_receive(pHandler);
			break;
		}
		case MPUSH_STAUTS_BINDING_OK:
		{
			mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTED);
			break;
		}
		case MPUSH_STAUTS_BINDING_FAIL:
		{
			break;
		}
		case MPUSH_STAUTS_CONNECTED:
		{
			//发送心跳
			if (abs(time(NULL) - heart_time) >= MPUSH_MAX_HEART_BEAT)
			{
				if (mpush_heartbeat(pHandler) != MPUSH_ERROR_HEART_BEAT_OK)
				{
					mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
					task_thread_sleep(5000);
				}
				heart_time = time(NULL);
			}

			mpush_client_receive(pHandler);
			break;
		}
		case MPUSH_STAUTS_DISCONNECT:
		{
			mpush_disconnect(pHandler);
			break;
		}
		default:
		{
			break;	
		}
	}
}

static void mpush_event_callback(
	APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	int err = 0;
	MPUSH_SERVICE_HANDLER_t *handle = g_mpush_service_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			mpush_client_process(handle);
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{
			mpush_set_run_status(handle, MPUSH_STAUTS_INIT);
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			mpush_set_run_status(handle, MPUSH_STAUTS_DISCONNECT);
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit(app);
			break;
		}
		default:
			break;
	}
}

static void task_mpush_service(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_MPUSH_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_MPUSH_SERVICE);
	}
	else
	{
		app_set_loop_timeout(app, 500, mpush_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, mpush_event_callback);
		app_add_event(app, APP_EVENT_AUDIO_PLAYER_BASE, mpush_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, mpush_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, mpush_event_callback);
		DEBUG_LOGE(LOG_TAG, "%s create success", APP_NAME_MPUSH_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t mpush_service_create(int task_priority)
{
	g_mpush_service_handle = (MPUSH_SERVICE_HANDLER_t*)memory_malloc(sizeof(MPUSH_SERVICE_HANDLER_t));
	if (g_mpush_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_service_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_mpush_service_handle, 0, sizeof(MPUSH_SERVICE_HANDLER_t));
	
	snprintf((char*)g_mpush_service_handle->msg_cfg.str_client_version, sizeof(g_mpush_service_handle->msg_cfg.str_client_version), "%s", ESP32_FW_VERSION);
	snprintf((char*)g_mpush_service_handle->msg_cfg.str_os_name, sizeof(g_mpush_service_handle->msg_cfg.str_os_name), PLATFORM_NAME);
	snprintf((char*)g_mpush_service_handle->msg_cfg.str_os_version, sizeof(g_mpush_service_handle->msg_cfg.str_os_version), "%s", ESP32_FW_VERSION);
	
	TAILQ_INIT(&g_mpush_service_handle->msg_queue);	
	SEMPHR_CREATE_LOCK(g_lock_mpush_service);

	if (!task_thread_create(task_mpush_service,
                    "task_mpush_service",
                    APP_NAME_MPUSH_SERVICE_STACK_SIZE,
                    g_mpush_service_handle,
                    task_priority)) 
    {
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_mpush_service task! Out of memory?");

		memory_free(g_mpush_service_handle);
		g_mpush_service_handle = NULL;
		SEMPHR_DELETE_LOCK(g_lock_mpush_service);
		
		return APP_FRAMEWORK_ERRNO_FAIL;
    }
	DEBUG_LOGI(LOG_TAG, "mpush_service_create success");

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t mpush_service_delete(void)
{
	return app_send_message(APP_NAME_MPUSH_SERVICE, APP_NAME_MPUSH_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


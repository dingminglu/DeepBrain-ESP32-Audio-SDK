#include "mpush_service.h"
#include "player_middleware_interface.h"
#include "cJSON.h"
#include "auth_crypto.h"
#include "userconfig.h"
#include "wifi_hw_interface.h"
#include "http_api.h"
#include "device_info_interface.h"

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
	DCL_MPUSH_SERVER_INFO_T *server_info = NULL;

	if (_pHandler->server_sock != INVALID_SOCK)
	{
		sock_close(_pHandler->server_sock);
		_pHandler->server_sock = INVALID_SOCK;
	}

	server_info = &_pHandler->server_list.server_info[_pHandler->server_index];
	DEBUG_LOGI(LOG_TAG, "mpush connect %s:%s", 
				server_info->server_addr, server_info->server_port);
	if (strlen(server_info->server_addr) > 0 
		&& strlen(server_info->server_port) > 0)
	{
		_pHandler->server_sock = sock_connect(server_info->server_addr, server_info->server_port);
		if (_pHandler->server_sock == INVALID_SOCK)
		{
			DEBUG_LOGE(LOG_TAG, "mpush sock_connect %s:%s failed", 
				server_info->server_addr, server_info->server_port);
			return MPUSH_ERROR_CONNECT_FAIL;
		}
		else
		{
			DEBUG_LOGI(LOG_TAG, "mpush sock_connect %s:%s success", 
				server_info->server_addr, server_info->server_port);
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
	
	mpush_make_handshake_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg);
	
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
	
	mpush_make_bind_user_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg);
	
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

int mpush_client_decode_offline_msg(
	MPUSH_SERVICE_HANDLER_t *_pHandler,
	char* const msg_result)
{
	if (_pHandler == NULL
		|| msg_result == NULL
		|| strlen(msg_result) == 0)
	{
		return MPUSH_ERROR_GET_OFFLINE_MSG_FAIL;
	}

	cJSON * pJson = cJSON_Parse(msg_result);
	if (NULL != pJson) 
	{
		cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
		if (NULL == pJson_status || pJson_status->valuestring == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "statusCode not found");
			goto mpush_client_decode_offline_msg_error;
		}
		
		if (strncasecmp(pJson_status->valuestring, "OK", strlen("OK")) != 0)
		{
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status->valuestring);
			goto mpush_client_decode_offline_msg_error;
		}

		cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
		if (NULL == pJson_content)
		{
			DEBUG_LOGE(LOG_TAG, "content not found");
			goto mpush_client_decode_offline_msg_error;
		}

		cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
		if (NULL == pJson_data)
		{
			DEBUG_LOGE(LOG_TAG, "data not found");
			goto mpush_client_decode_offline_msg_error;
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
					DEBUG_LOGI(LOG_TAG, "[offline msg][text]");
				}
				else if (strncasecmp(pJson_msg_type->valuestring, "file", strlen("file")) == 0)
				{
					msg->msg_type = MPUSH_SEND_MSG_TYPE_FILE;
					DEBUG_LOGI(LOG_TAG, "[offline msg][file]");
				}
				else if (strncasecmp(pJson_msg_type->valuestring, "hylink", strlen("hylink")) == 0)
				{
					msg->msg_type = MPUSH_SEND_MSG_TYPE_LINK;
					DEBUG_LOGI(LOG_TAG, "[offline msg][hylink]");
				}
				else if (strncasecmp(pJson_msg_type->valuestring, "cmd", strlen("cmd")) == 0)
				{
					msg->msg_type = MPUSH_SEND_MSG_TYPE_CMD;
					DEBUG_LOGI(LOG_TAG, "[offline msg][cmd]");
				}
				else
				{
					DEBUG_LOGE(LOG_TAG, "not support msg type:%s", pJson_msg_type->valuestring);
					continue;
				}

				snprintf(msg->msg_content, sizeof(msg->msg_content), "%s", pJson_data->valuestring);
				DEBUG_LOGI(LOG_TAG, "[offline msg][%s]", pJson_data->valuestring);
				mpush_add_msg_queue(msg);
			}
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "invalid json data[%s]", _pHandler->str_comm_buf);
	}

mpush_client_decode_offline_msg_error:

	if (pJson != NULL)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}

	return MPUSH_ERROR_GET_OFFLINE_MSG_OK;
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
			char chip_id[32] = {0};
			char device_sn[32] = {0};
			get_flash_cfg(FLASH_CFG_DEVICE_ID, device_sn);
			if (strlen(device_sn) > 0)
			{
				snprintf((char*)&pHandler->msg_cfg.str_device_id, sizeof(pHandler->msg_cfg.str_device_id), "%s", device_sn);
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_SERVER_LIST);
				break;
			}
			
			//设备激活操作，获取设备唯一序列号
			get_device_id(chip_id, sizeof(chip_id));

			if (!get_dcl_auth_params(&pHandler->auth_params))
			{
				DEBUG_LOGE(LOG_TAG, "get_dcl_auth_params failed"); 
				task_thread_sleep(5000);
				break;
			}
			
			if (dcl_get_device_license(
					&pHandler->auth_params, 
					(uint8_t *)DEVICE_SN_PREFIX, 
					(uint8_t *)chip_id, 
					false, 
					&pHandler->dcl_license) == DCL_ERROR_CODE_OK)
			{
				DEBUG_LOGI(LOG_TAG, "dcl_get_device_license success");

				if (set_flash_cfg(FLASH_CFG_DEVICE_ID, (void*)&pHandler->dcl_license.device_sn) != DEVICE_PARAMS_ERRNO_OK)
				{
					DEBUG_LOGE(LOG_TAG, "set_flash_cfg FLASH_CFG_DEVICE_ID failed"); 
					task_thread_sleep(5000);
					break;
				}
				
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_SERVER_LIST);
				//激活成功提示音
				audio_play_tone_mem(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_OK, TERMINATION_TYPE_DONE);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "dcl_get_device_license fail");				
				//激活失败提示音
				audio_play_tone_mem(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_FAIL, TERMINATION_TYPE_DONE);
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_GET_SERVER_LIST:
		{
			//获取服务器列表
			if (!get_dcl_auth_params(&pHandler->auth_params))
			{
				DEBUG_LOGE(LOG_TAG, "get_dcl_auth_params failed"); 
				task_thread_sleep(5000);
				break;
			}
			
			if (dcl_mpush_get_server_list(
				&pHandler->auth_params,
				&pHandler->server_list) == DCL_ERROR_CODE_OK)
			{
				DEBUG_LOGI(LOG_TAG, "dcl_mpush_get_server_list success");
				mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_OFFLINE_MSG);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "dcl_mpush_get_server_list fail");	
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_GET_OFFLINE_MSG:
		{	
			//获取离线消息
			if (!get_dcl_auth_params(&pHandler->auth_params))
			{
				DEBUG_LOGE(LOG_TAG, "get_dcl_auth_params failed"); 
				task_thread_sleep(5000);
				break;
			}
		
			if (dcl_mpush_get_offline_msg(
				&pHandler->auth_params,
				&pHandler->str_comm_buf,
				sizeof(pHandler->str_comm_buf)) == DCL_ERROR_CODE_OK)
			{
				DEBUG_LOGI(LOG_TAG, "dcl_mpush_get_offline_msg success");

				if (mpush_client_decode_offline_msg(pHandler, pHandler->str_comm_buf) != MPUSH_ERROR_GET_OFFLINE_MSG_OK)
				{
					DEBUG_LOGE(LOG_TAG, "mpush_client_decode_offline_msg fail");	
				}
				
				mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "dcl_mpush_get_offline_msg fail");	
				task_thread_sleep(5000);
			}
			break;
		}
		case MPUSH_STAUTS_CONNECTING:
		{
			srand(pHandler->server_list.server_num);
			pHandler->server_index = rand();
			if (pHandler->server_index >= DCL_MAX_MPUSH_SERVER_NUM)
			{
				pHandler->server_index = 0;
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


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

#ifndef MPUSH_SERVICE_H
#define MPUSH_SERVICE_H

#include "socket.h"
#include "app_config.h"
#include "mpush_message.h"
#include "device_params_manage.h"
#include "mpush_interface.h"
#include "dcl_device_license.h"
#include "dcl_mpush_server_list.h"
#include "dcl_mpush_offline_msg.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum MPUSH_ERROR_CODE_T
{
	MPUSH_ERROR_GET_SERVER_OK = 0,
	MPUSH_ERROR_GET_SERVER_FAIL,
	MPUSH_ERROR_GET_SN_OK,
	MPUSH_ERROR_GET_SN_FAIL,
	MPUSH_ERROR_GET_OFFLINE_MSG_OK,
	MPUSH_ERROR_GET_OFFLINE_MSG_FAIL,
	MPUSH_ERROR_SEND_MSG_OK,
	MPUSH_ERROR_SEND_MSG_FAIL,
	MPUSH_ERROR_START_OK,
	MPUSH_ERROR_START_FAIL,
	MPUSH_ERROR_INVALID_PARAMS,
	MPUSH_ERROR_CONNECT_OK,
	MPUSH_ERROR_CONNECT_FAIL,
	MPUSH_ERROR_HANDSHAKE_OK,
	MPUSH_ERROR_HANDSHAKE_FAIL,
	MPUSH_ERROR_BIND_USER_OK,
	MPUSH_ERROR_BIND_USER_FAIL,
	MPUSH_ERROR_HEART_BEAT_OK,
	MPUSH_ERROR_HEART_BEAT_FAIL,
}MPUSH_ERROR_CODE_T;

typedef enum MPUSH_STATUS_T
{
	MPUSH_STAUTS_IDEL = 0,
	MPUSH_STAUTS_INIT,
	MPUSH_STAUTS_GET_SERVER_LIST,
	MPUSH_STAUTS_GET_OFFLINE_MSG,
	MPUSH_STAUTS_CONNECTING,
	MPUSH_STAUTS_HANDSHAKING,
	MPUSH_STAUTS_HANDSHAK_WAIT_REPLY,
	MPUSH_STAUTS_HANDSHAK_OK,
	MPUSH_STAUTS_HANDSHAK_FAIL,
	MPUSH_STAUTS_BINDING,
	MPUSH_STAUTS_BINDING_WAIT_REPLY,
	MPUSH_STAUTS_BINDING_OK,
	MPUSH_STAUTS_BINDING_FAIL,
	MPUSH_STAUTS_CONNECTED,
	MPUSH_STAUTS_DISCONNECT,
}MPUSH_STATUS_T;

typedef struct MPUSH_SERVICE_T
{
	void *_mpush_handler;
}MPUSH_SERVICE_T;

//mpush msg define
typedef struct MPUSH_MSG_INFO_t
{
	//tail queue entry
	TAILQ_ENTRY(MPUSH_MSG_INFO_t) next;
	MPUSH_CLIENT_MSG_INFO_t msg_info;
}MPUSH_MSG_INFO_t;

//mpush service handle
typedef struct MPUSH_SERVICE_HANDLER_t
{
	MPUSH_STATUS_T status;
	sock_t	server_sock;								// server socket
	char str_server_list_url[128];						// server address
	char str_public_key[1024];				// public key
	char str_comm_buf[1024*30];				// commucation buffer
	char str_comm_buf1[1024*30];			// 
	char str_recv_buf[1024*20];				// recv buffer
	uint8_t str_push_msg[1024*10];			// recv push msg
	MPUSH_MSG_CONFIG_T msg_cfg;
	
	MPUSH_CLIENT_MSG_INFO_t msg_info;
	
	//dcl license result
	DCL_DEVICE_LICENSE_RESULT_t dcl_license;

	//dcl auth params
	DCL_AUTH_PARAMS_t auth_params;

	//mpush server list
	uint32_t server_index;
	DCL_MPUSH_SERVER_LIST_T server_list;
	
	TAILQ_HEAD(MPUSH_MSG_QUEUE_t, MPUSH_MSG_INFO_t) msg_queue;
}MPUSH_SERVICE_HANDLER_t;


APP_FRAMEWORK_ERRNO_t mpush_service_create(int task_priority);
APP_FRAMEWORK_ERRNO_t mpush_service_delete(void);
APP_FRAMEWORK_ERRNO_t mpush_get_msg_queue(MPUSH_CLIENT_MSG_INFO_t *msg_info);

#ifdef __cplusplus
 }
#endif

#endif


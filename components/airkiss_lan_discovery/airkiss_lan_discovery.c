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

#include "airkiss.h"
#include "airkiss_lan_discovery.h"
#include "socket.h"
#include "userconfig.h"
#include "device_params_manage.h"

#define LOG_TAG "airkiss lan discovery" 

#define AIKISS_LAN_DISCOVERY_PORT 12476

/* airkiss lan discovery status */
typedef enum AIRKISS_LAN_DISCOVERY_STATUS_t
{
	AIRKISS_LAN_DISCOVERY_STATUS_IDLE = 0,		//空闲
	AIRKISS_LAN_DISCOVERY_STATUS_ONLINE,		//已经联网
	AIRKISS_LAN_DISCOVERY_STATUS_OFFLINE,		//已经断网
}AIRKISS_LAN_DISCOVERY_STATUS_t;

/* airkiss lan discovery running handle */
typedef struct AIRKISS_LAN_DISCOVERY_HANDLE_t
{
	//局域网发现当前状态
	AIRKISS_LAN_DISCOVERY_STATUS_t discovery_status;
		
	//网络连接状态
	bool is_connected;

	//mutex lock
	void *mutex_lock;

	//udp socket
	int udp_socket;

	//encode buffer
	char encode_buffer[1024];

	//udp recv buffer
	char udp_recv_buffer[1024];

	//udp send buffer
	char udp_send_buffer[1024];

	//发送udp包时间戳
	uint32_t send_timestamp;

	//设备 license
	DEVICE_BASIC_INFO_T license;
}AIRKISS_LAN_DISCOVERY_HANDLE_t;

/* 全局处理句柄 */
static AIRKISS_LAN_DISCOVERY_HANDLE_t *g_airkiss_lan_discovery_handle = NULL;

const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	0
};

static void airkiss_lan_discovery_destroy(void)
{
	if (g_airkiss_lan_discovery_handle == NULL)
	{
		return;
	}

	memory_free(g_airkiss_lan_discovery_handle);
	g_airkiss_lan_discovery_handle = NULL;

	return;
}

static int create_udp_socket(void)
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) 
	{
        DEBUG_LOGE(LOG_TAG, "Failed to create socket.");
        return -1;
    }

    // Bind the socket to any address
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(AIKISS_LAN_DISCOVERY_PORT);
    saddr.sin_addr.s_addr = htonl(IPADDR_ANY);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0) 
	{
       	DEBUG_LOGE(LOG_TAG, "Failed to bind socket.");
        goto create_udp_socket_err;
    }
    
    return sock;

create_udp_socket_err:
    close(sock);
    return -1;
}

static bool send_udp_packet(
	AIRKISS_LAN_DISCOVERY_HANDLE_t *handle,
	struct sockaddr_in *from_addr)
{
	int ret = 0;
	char *DEVICE_BIND_INFO = "{\"deviceSN\": \"%s\",\"deviceVersion\": \"%s\",\"appId\": \"%s\",\"robotId\": \"%s\"}";
	struct sockaddr_in client_addr;
	int len = sizeof(client_addr);
	int lan_buf_len = sizeof(handle->udp_send_buffer);

	get_flash_cfg(FLASH_CFG_DEVICE_LICENSE, &handle->license);
	if (strlen(handle->license.device_sn) == 0
		|| strlen(handle->license.weixin_dev_type) == 0)
	{
		DEBUG_LOGE(LOG_TAG, "wexin license is empty");
		return false;
	}

	memset(&client_addr, 0, sizeof(client_addr));
	if (from_addr == NULL)
	{
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(AIKISS_LAN_DISCOVERY_PORT);
		client_addr.sin_addr.s_addr = INADDR_BROADCAST;
	}
	else
	{
		memcpy(&client_addr, from_addr, sizeof(client_addr));
	}

	ret = airkiss_lan_pack(
		AIRKISS_LAN_SSDP_NOTIFY_CMD, handle->license.weixin_dev_type, handle->license.device_sn, 
		0, 0, 
		handle->udp_send_buffer, &lan_buf_len, &akconf);
	if (ret != AIRKISS_LAN_PAKE_READY) 
	{
		DEBUG_LOGE(LOG_TAG, "airkiss_lan_pack failed[%d]", ret);
		return false;
	}

	ret = sendto(handle->udp_socket, handle->udp_send_buffer, lan_buf_len, MSG_DONTWAIT, (struct sockaddr *)&client_addr, sizeof(client_addr));
	if (ret != lan_buf_len)
	{
		DEBUG_LOGE(LOG_TAG, "send airkiss lan discovery packet failed,total len[%d], send len[%d]", lan_buf_len, ret);
		return false;
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "send airkiss lan discovery packet success");
	}
	
	return true;
}

static void recv_udp_packet(AIRKISS_LAN_DISCOVERY_HANDLE_t *handle)
{
	int ret = 0;
	struct sockaddr_in client_addr;
	uint32_t len = sizeof(client_addr);

	memset(handle->udp_recv_buffer, 0, sizeof(handle->udp_recv_buffer));
	ret = recvfrom(handle->udp_socket, handle->udp_recv_buffer, sizeof(handle->udp_recv_buffer), MSG_DONTWAIT, (struct sockaddr*)&client_addr, &len);
	if (ret > 0)
	{
		//DEBUG_LOGI(LOG_TAG, "udp recv len[%d]", ret);
		ret = airkiss_lan_recv(handle->udp_recv_buffer, ret, &akconf);
		//DEBUG_LOGI(LOG_TAG, "airkiss_lan_recv ret [%d]", ret);
		if (ret == AIRKISS_LAN_SSDP_REQ)
		{
			send_udp_packet(handle, &client_addr);
		}
	}
}

static void delete_udp_socket(int sock)
{
	if (sock >= 0)
	{
		close(sock);
	}
}

static void airkiss_lan_discovery_process(AIRKISS_LAN_DISCOVERY_HANDLE_t *handle)
{	
	int ret = 0;
	
	if (handle == NULL)
	{
		return;
	}

	switch (handle->discovery_status)
	{
		case AIRKISS_LAN_DISCOVERY_STATUS_IDLE:
		{
			handle->udp_socket = create_udp_socket();
			if (handle->udp_socket >= 0)
			{
				handle->discovery_status = AIRKISS_LAN_DISCOVERY_STATUS_ONLINE;
			}
			break;
		}
		case AIRKISS_LAN_DISCOVERY_STATUS_ONLINE:
		{
			recv_udp_packet(handle);

			if (time(NULL)%5 == 0)
			{
				if (!send_udp_packet(handle, NULL))
				{
					delete_udp_socket(handle->udp_socket);
					handle->udp_socket = INVALID_SOCK;
					handle->discovery_status = AIRKISS_LAN_DISCOVERY_STATUS_IDLE;
				}
			}
			break;
		}
		case AIRKISS_LAN_DISCOVERY_STATUS_OFFLINE:
		{
			break;
		}
		default:
			break;
	}
}

static void airkiss_lan_discovery_event_callback(
	void *app, 
	APP_EVENT_MSG_t *msg)
{	
	AIRKISS_LAN_DISCOVERY_HANDLE_t *handle = g_airkiss_lan_discovery_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED://已连接
		{
			handle->is_connected = true;
			handle->discovery_status = AIRKISS_LAN_DISCOVERY_STATUS_ONLINE;
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED://断开连接
		{
			handle->is_connected = false;
			handle->discovery_status = AIRKISS_LAN_DISCOVERY_STATUS_OFFLINE;
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			airkiss_lan_discovery_process(handle);
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

void task_airkiss_lan_discovery(void *pv)
{		
	APP_OBJECT_t *app = app_new(APP_NAME_AIRKISS_LAN_DISCOVERY);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_AIRKISS_LAN_DISCOVERY);
		airkiss_lan_discovery_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 500, airkiss_lan_discovery_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, airkiss_lan_discovery_event_callback);		
		app_add_event(app, APP_EVENT_WIFI_BASE, airkiss_lan_discovery_event_callback);
		DEBUG_LOGE(LOG_TAG, "%s create success", APP_NAME_AIRKISS_LAN_DISCOVERY);
	}

	app_msg_dispatch(app);
	
	app_delete(app);	

	airkiss_lan_discovery_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t airkiss_lan_discovery_create(int task_priority)
{
	if (g_airkiss_lan_discovery_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_airkiss_lan_discovery_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	g_airkiss_lan_discovery_handle = (AIRKISS_LAN_DISCOVERY_HANDLE_t *)memory_malloc(sizeof(AIRKISS_LAN_DISCOVERY_HANDLE_t));
	if (g_airkiss_lan_discovery_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_wechat_cloud_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset((char*)g_airkiss_lan_discovery_handle, 0, sizeof(AIRKISS_LAN_DISCOVERY_HANDLE_t));
	//TAILQ_INIT(&g_wechat_cloud_handle->pcm_queue);

	//初始化参数
	g_airkiss_lan_discovery_handle->is_connected = false;
	g_airkiss_lan_discovery_handle->discovery_status = AIRKISS_LAN_DISCOVERY_STATUS_IDLE;
	SEMPHR_CREATE_LOCK(g_airkiss_lan_discovery_handle->mutex_lock);
	if (g_airkiss_lan_discovery_handle->mutex_lock == NULL)
	{
		airkiss_lan_discovery_destroy();
		DEBUG_LOGE(LOG_TAG, "g_wechat_cloud_handle->mutex_lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	//创建线程
    if (!task_thread_create(task_airkiss_lan_discovery,
            "task_airkiss_lan_discovery",
            APP_NAME_AIRKISS_LAN_DISCOVERY_STACK_SIZE,
            g_airkiss_lan_discovery_handle,
            task_priority)) 
    {
			DEBUG_LOGE(LOG_TAG, "create task_airkiss_lan_discovery failed");
			airkiss_lan_discovery_destroy();
			return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t airkiss_lan_discovery_delete(void)
{
	return app_send_message(APP_NAME_AIRKISS_LAN_DISCOVERY, APP_NAME_AIRKISS_LAN_DISCOVERY, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


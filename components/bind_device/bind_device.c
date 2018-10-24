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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include "lwip/netdb.h"
#include "socket.h"
#include "bind_device.h"
#include "device_params_manage.h"
#include "player_middleware_interface.h"

#define LOG_TAG "BIND DEVICE"
#define BIND_ACCEPT_TIMEOUT_SEC 10

/*绑定设备句柄*/
typedef struct BIND_DEVICE_HANDLER_t
{
	char recv_buff[1024];
}BIND_DEVICE_HANDLER_t;

static BIND_DEVICE_HANDLER_t *g_bind_device_handler = NULL;

static void save_user_id(char *_content)
{
	int ret = 0;
	
	cJSON *pJson_body = cJSON_Parse(_content);
    if (NULL != pJson_body) 
	{
        cJSON *pJson_phone_num = cJSON_GetObjectItem(pJson_body, "phoneNumber");
		if (NULL == pJson_phone_num 
			|| pJson_phone_num->valuestring == NULL 
			|| strlen(pJson_phone_num->valuestring) <= 0)
		{
			goto save_user_id_error;
		}

		if (set_flash_cfg(FLASH_CFG_USER_ID, pJson_phone_num->valuestring) == ESP_OK)
		{
			DEBUG_LOGI(LOG_TAG, "save user id[%s] success", pJson_phone_num->valuestring);
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "save user id[%s] fail", pJson_phone_num->valuestring);
		}
		
    }
	else
	{
		DEBUG_LOGE(LOG_TAG, "cJSON_Parse error");
	}

save_user_id_error:

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}

	return;
}

void device_bind_process(BIND_DEVICE_HANDLER_t *handler)
{  
	static char *DEVICE_BIND_INFO = "{\"deviceSN\": \"%s\",\"deviceVersion\": \"%s\"}";

	int listenfd = INVALID_SOCK;
	int connectfd = INVALID_SOCK;
	struct sockaddr_in server;	//服务器地址信息结构体  
	struct sockaddr_in client;	//客户端地址信息结构体  
	int sin_size;  
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) 
	{ //调用socket，创建监听客户端的socket  
		DEBUG_LOGE(LOG_TAG, "socket create failed");
		return;
	}  
	 
	int opt = SO_REUSEADDR; 		 
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	  //设置socket属性，端口可以重用  
	//初始化服务器地址结构体  
	bzero(&server,sizeof(server));	
	server.sin_family=AF_INET;	
	server.sin_port=htons(9099);  
	server.sin_addr.s_addr = htonl(INADDR_ANY);  
	
	//调用bind，绑定地址和端口  
	if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
	{
		DEBUG_LOGE(LOG_TAG, "bind failed");
		goto device_bind_process_error; 
	}	  
	DEBUG_LOGE(LOG_TAG, "bind success");

	//调用listen，开始监听	 
	if (listen(listenfd, 2) == -1)
	{ 	 
		DEBUG_LOGE(LOG_TAG, "listen failed");
		goto device_bind_process_error;
	}  
	DEBUG_LOGE(LOG_TAG, "listen success");
	
	sin_size=sizeof(struct sockaddr_in); 
	uint32_t recv_len = 0;

	//设置accept超时
	if (sock_set_read_timeout(listenfd, BIND_ACCEPT_TIMEOUT_SEC, 0) != 0)
	{	
		DEBUG_LOGE(LOG_TAG, "set accept %d seconds timeout failed", BIND_ACCEPT_TIMEOUT_SEC);
	}

	connectfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&sin_size);
	if (connectfd == -1) 
	{ 					 
		//调用accept，返回与服务器连接的客户端描述符	
		DEBUG_LOGE(LOG_TAG, "accept failed");
		goto device_bind_process_error;
	}  
	sock_set_nonblocking(connectfd);
	DEBUG_LOGE(LOG_TAG, "accept one client %d", connectfd);
	
	//接收绑定信息
	memset(handler->recv_buff, 0, sizeof(handler->recv_buff));
	DEBUG_LOGE(LOG_TAG, "recving info");
	int ret = sock_readn_with_timeout(connectfd, (char*)&recv_len, sizeof(recv_len), 6000);
	if (ret == sizeof(recv_len))
	{		
		recv_len = ntohl(recv_len);
		if (recv_len > sizeof(handler->recv_buff) - 1)
		{
			DEBUG_LOGE(LOG_TAG, "recv_len:[%d] too long, max size = %d", recv_len, sizeof(handler->recv_buff) -1);
			goto device_bind_process_error;
		}
		
		int ret = sock_readn_with_timeout(connectfd, handler->recv_buff, recv_len, 2000);
		if (ret == recv_len)
		{	
			//解析数据
			DEBUG_LOGE(LOG_TAG, "recv info:[%d]%s", ret, handler->recv_buff);
			save_user_id(handler->recv_buff);

			//发送反馈包
			char device_id[32]= {0};
			get_flash_cfg(FLASH_CFG_DEVICE_ID, &device_id);
			snprintf(handler->recv_buff, sizeof(handler->recv_buff), DEVICE_BIND_INFO, device_id, ESP32_FW_VERSION);
			DEBUG_LOGE(LOG_TAG, "write info:[%d]%s", strlen(handler->recv_buff), handler->recv_buff);
			recv_len = htonl(strlen(handler->recv_buff));
			if (sock_writen_with_timeout(connectfd, (void*)&recv_len, sizeof(recv_len), 2000) != sizeof(recv_len))
			{
				DEBUG_LOGE(LOG_TAG, "sock_write header %d bytes fail", sizeof(recv_len));
			}
			
			if (sock_writen_with_timeout(connectfd, handler->recv_buff, strlen(handler->recv_buff), 2000) != strlen(handler->recv_buff))
			{
				DEBUG_LOGE(LOG_TAG, "sock_write body %d bytes fail", strlen(handler->recv_buff));
			}
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "data len =%d, recv len=%d", recv_len, ret);
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "recv info:ret = %d, is not 4 bytes length", ret);
	}
	
device_bind_process_error:			
	//关闭此次连接
	close(connectfd);
	close(listenfd);

	return;
}

void bind_device_destroy(void)
{
	if (g_bind_device_handler != NULL)
	{
		memory_free(g_bind_device_handler);
		g_bind_device_handler = NULL;
	}
}

static void bind_device_event_callback(void *app, APP_EVENT_MSG_t *msg)
{
	switch (msg->event)
	{
		case APP_EVENT_BIND_DEVICE_MSG:
		{ 
			device_bind_process(g_bind_device_handler);
			break;
		}
		default:
			break;
	}
}

static void task_device_bind(void *pv)
{
	APP_OBJECT_t *app = NULL;

	app = app_new(APP_MAIN_BIND_DEVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_MAIN_BIND_DEVICE);
		task_thread_exit();
		return;
	}
	else
	{
		app_add_event(app, APP_EVENT_BIND_DEVICE_BASE, bind_device_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_MAIN_BIND_DEVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	bind_device_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t bind_device_create(int task_priority)
{
	if (g_bind_device_handler != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_bind_device_handler already exist");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//申请句柄
	g_bind_device_handler = (BIND_DEVICE_HANDLER_t *)memory_malloc(sizeof(BIND_DEVICE_HANDLER_t));
	if (g_bind_device_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_bind_device_handler malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//初始化参数
	memset(g_bind_device_handler, 0, sizeof(BIND_DEVICE_HANDLER_t));

	//创建任务
	if (!task_thread_create(task_device_bind,
	        "task_device_bind",
	        APP_MAIN_BIND_DEVICE_STACK_SIZE,
	        g_bind_device_handler,
	        task_priority)) 
    {
		bind_device_destroy();
        DEBUG_LOGE(LOG_TAG,  "task_device_bind failed");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t bind_device_delete(void)
{
	return app_send_message(APP_MAIN_BIND_DEVICE, APP_MAIN_BIND_DEVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


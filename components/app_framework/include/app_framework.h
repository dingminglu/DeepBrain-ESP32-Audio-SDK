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

#ifndef APP_FRAMEWORK_H
#define APP_FRAMEWORK_H

#include "ctypes_interface.h"
#include "app_queue.h"
#include "app_event_base.h"
#include "app_event.h"

#define APP_NAME_LEN 32
#define APP_MSG_TO_ALL	"[all]"
#define APP_MAIN_NAME 	"main"

//event msg
typedef struct APP_EVENT_MSG_t
{
	//msg sender
	char from_app[APP_NAME_LEN];
	
	//msg receiver
	char to_app[APP_NAME_LEN];

	//event type
	uint32_t event;

	//event data
	void *event_data;

	//event data len
	uint32_t event_data_len;
}APP_EVENT_MSG_t;

//事件通知回调
typedef void (*event_callback_fn)(void *app, APP_EVENT_MSG_t *msg);

//platform api
typedef struct PLATFORM_API_t
{
	void *(*app_malloc_fn)(size_t sz);
	void (*app_free_fn)(void *p);
	void (*sleep)(int ms);
	uint64_t (*get_time_of_day)(void);
}PLATFORM_API_t;

//app framework error no
typedef enum
{
	APP_FRAMEWORK_ERRNO_OK,				//成功
	APP_FRAMEWORK_ERRNO_FAIL,			//失败
	APP_FRAMEWORK_ERRNO_INVALID_PARAMS,	//无效参数
	APP_FRAMEWORK_ERRNO_MALLOC_FAILED,	//申请内存失败
	APP_FRAMEWORK_ERRNO_TASK_FAILED,	//线程创建失败
	APP_FRAMEWORK_ERRNO_MAIN_NOT_EXIST,	//主线程不存在
	APP_FRAMEWORK_ERRNO_NO_MSG,			//无消息
}APP_FRAMEWORK_ERRNO_t;

//app object run status
typedef enum
{
	APP_OBJECT_STATUS_RUNNING = 0,
	APP_OBJECT_STATUS_EXIT,
}APP_OBJECT_STATUS_t;

//msg queue entry
typedef struct APP_MSG_QUEUE_ENTRY_t
{
	//tail queue entry
	TAILQ_ENTRY(APP_MSG_QUEUE_ENTRY_t) next;
	
	//msg send time
	uint64_t send_time_stamp;

	APP_EVENT_MSG_t msg;
}APP_MSG_QUEUE_ENTRY_t;

typedef struct APP_EVENT_ENTRY_t
{
	//list entry
	LIST_ENTRY(APP_OBJECT_t) next;

	//event type
	uint32_t event;

	//event callback
	event_callback_fn ev_cb;
}APP_EVENT_ENTRY_t;

//app object run handle
typedef struct APP_OBJECT_t
{	
	//list entry
	LIST_ENTRY(APP_OBJECT_t) next;
	
	//data lock, used to protect data queue operation
	void *lock;

	//msg queue
	TAILQ_HEAD(APP_MSG_LIST_t, APP_MSG_QUEUE_ENTRY_t) msg_queue;

	//evnet list
	LIST_HEAD(APP_EVENT_LIST_t, APP_EVENT_ENTRY_t) event_list;
	
	// app name
	char name[APP_NAME_LEN];	

	//是否退出标签
	APP_OBJECT_STATUS_t run_status;

	//default sys callback
	event_callback_fn loop_callback;

	//loop timeout, ms
	int loop_timeout;
}APP_OBJECT_t;

typedef struct APP_OBJECTS_LIST_t
{
	LIST_HEAD(APP_OBJECT_LIST_t, APP_OBJECT_t) list;
}APP_OBJECTS_LIST_t;

//app object 
void app_set_extern_functions(PLATFORM_API_t *api);
APP_OBJECT_t *app_new(const    char *name);
APP_FRAMEWORK_ERRNO_t app_delete(APP_OBJECT_t *app);
APP_FRAMEWORK_ERRNO_t app_exit(APP_OBJECT_t *app);
APP_FRAMEWORK_ERRNO_t app_msg_dispatch(APP_OBJECT_t *app);
APP_FRAMEWORK_ERRNO_t app_set_loop_timeout(APP_OBJECT_t *app, const uint32_t timeout, const event_callback_fn ev_cb);
APP_FRAMEWORK_ERRNO_t app_add_event(APP_OBJECT_t *app, const uint32_t event_base, const event_callback_fn ev_cb);
APP_FRAMEWORK_ERRNO_t app_remove_event(APP_OBJECT_t *app, const uint32_t event_base);
APP_FRAMEWORK_ERRNO_t app_send_msg(const APP_EVENT_MSG_t *msg);
APP_FRAMEWORK_ERRNO_t app_send_message(const char *from_app, const char *to_app, const uint32_t event, const void *event_data, const uint32_t event_data_len);
#endif


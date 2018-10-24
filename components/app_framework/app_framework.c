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

#include <stdio.h>
#include <string.h>
#include "debug_log_interface.h"
#include "semaphore_lock_interface.h"
#include "app_framework.h"

#define TAG_LOG "app framework"

//global vars
static APP_OBJECT_t *g_app_main = NULL;
#define APP_MAIN_OBJECT g_app_main

static PLATFORM_API_t g_apis = {0};
#define APP_API_MALLOC g_apis.app_malloc_fn
#define APP_API_FREE g_apis.app_free_fn
#define APP_API_SLEEP g_apis.sleep
#define APP_API_TIME_OF_DAY g_apis.get_time_of_day

APP_OBJECTS_LIST_t apps_list;
#define APPS_LIST apps_list.list

static APP_FRAMEWORK_ERRNO_t app_list_add_object(
	APP_OBJECT_t *app_obj)
{
	//check input params
	if (app_obj == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (APP_MAIN_OBJECT == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]APP_MAIN_OBJECT is NULL", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	//add app to the list
	SEMPHR_TRY_LOCK(APP_MAIN_OBJECT->lock);
	LIST_INSERT_HEAD(&APPS_LIST, app_obj, next);
	SEMPHR_TRY_UNLOCK(APP_MAIN_OBJECT->lock);

	return APP_FRAMEWORK_ERRNO_OK;
}

static APP_FRAMEWORK_ERRNO_t app_list_remove_object(
		APP_OBJECT_t *app_obj)
{
	//check input params
	if (app_obj == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (APP_MAIN_OBJECT == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]APP_MAIN_OBJECT is NULL", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	//add app to the list
	SEMPHR_TRY_LOCK(APP_MAIN_OBJECT->lock);
	LIST_REMOVE(app_obj, next);
	SEMPHR_TRY_UNLOCK(APP_MAIN_OBJECT->lock);

	return APP_FRAMEWORK_ERRNO_OK;
}


static APP_FRAMEWORK_ERRNO_t app_object_create(
	APP_OBJECT_t **app, 
	const char *name)
{
	if (app == NULL || name == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	//malloc app memory
	*app = (APP_OBJECT_t *)APP_API_MALLOC(sizeof(APP_OBJECT_t));
	if (*app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]%s failed", __FUNCTION__, name);
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	memset(*app, 0, sizeof(APP_OBJECT_t));

	//init app's name
	snprintf((*app)->name, sizeof((*app)->name), "%s", name);

	//init msg queue
	TAILQ_INIT(&(*app)->msg_queue);

	//init event list
	LIST_INIT(&(*app)->event_list);

	//create lock
	SEMPHR_CREATE_LOCK((*app)->lock);
	if ((*app)->lock == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]SEMPHR_CREATE_LOCK failed", __FUNCTION__);
		APP_API_FREE(*app);
		*app = NULL;
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

static APP_FRAMEWORK_ERRNO_t app_object_delete(
	APP_OBJECT_t *app)
{
	APP_MSG_QUEUE_ENTRY_t *app_msg = NULL;
	APP_EVENT_ENTRY_t *ev = NULL;
	
	if (app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}
	
	SEMPHR_TRY_LOCK(app->lock);
	//delete msg queue
	while ((app_msg = TAILQ_FIRST(&app->msg_queue)))
	{
		if (app_msg->msg.event_data != NULL
			&& app_msg->msg.event_data_len > 0)
		{
			APP_API_FREE(app_msg->msg.event_data);
			app_msg->msg.event_data = NULL;
			app_msg->msg.event_data_len = 0;
		}
		TAILQ_REMOVE(&app->msg_queue, app_msg, next);
		APP_API_FREE(app_msg);
	}

	//delete event list
	while ((ev = LIST_FIRST(&app->event_list))) 
	{
		LIST_REMOVE(ev, next);
		APP_API_FREE(ev);
	}
	SEMPHR_TRY_UNLOCK(app->lock);
	
	//delete lock
	SEMPHR_DELETE_LOCK(app->lock);
	app->lock = NULL;

	APP_API_FREE(app);
	app = NULL;

	return APP_FRAMEWORK_ERRNO_OK;
}

static APP_FRAMEWORK_ERRNO_t app_object_send_msg(
	APP_OBJECT_t *app,
	const APP_EVENT_MSG_t *msg)
{
	//check input params
	if (app == NULL || msg == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	APP_MSG_QUEUE_ENTRY_t *qe = APP_API_MALLOC(sizeof(APP_MSG_QUEUE_ENTRY_t));
	if (qe == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]APP_API_MALLOC qe failed", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//malloc msg event data
	memset(qe, 0, sizeof(APP_MSG_QUEUE_ENTRY_t));
	qe->msg = *msg;
	if (msg->event_data != NULL && msg->event_data_len > 0)
	{
		qe->msg.event_data = (void*)APP_API_MALLOC(msg->event_data_len);
		if (qe->msg.event_data == NULL)
		{
			APP_API_FREE(qe);
			qe = NULL;
			DEBUG_LOGE(TAG_LOG, "[%s]APP_API_MALLOC event_data[%d] failed", __FUNCTION__, msg->event_data_len);
			return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
		}
		memcpy(qe->msg.event_data, msg->event_data, msg->event_data_len);
	}

	//insert into msg queue
	SEMPHR_TRY_LOCK(app->lock);
	TAILQ_INSERT_TAIL(&app->msg_queue, qe, next);
	SEMPHR_TRY_UNLOCK(app->lock);

	return APP_FRAMEWORK_ERRNO_OK;
}

static APP_FRAMEWORK_ERRNO_t app_object_recv_msg(
	APP_OBJECT_t *app, 
	APP_MSG_QUEUE_ENTRY_t **qe)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	//check input params
	if (app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	*qe = TAILQ_FIRST(&app->msg_queue);
	if (*qe == NULL)
	{
		err = APP_FRAMEWORK_ERRNO_NO_MSG;
	}
	else
	{
		TAILQ_REMOVE(&app->msg_queue, *qe, next);
	}
	SEMPHR_TRY_UNLOCK(app->lock);
	
	return err;
}

static APP_FRAMEWORK_ERRNO_t app_object_proc_msg(
	APP_OBJECT_t *app,
	APP_MSG_QUEUE_ENTRY_t *qe)
{	
	APP_EVENT_ENTRY_t *ev = NULL;
	int is_find = 0;
	
	//1.check input parmas
	if (app == NULL || qe == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	//foreach event list
	SEMPHR_TRY_LOCK(app->lock);
	LIST_FOREACH(ev, &app->event_list, next)
	{
		//DEBUG_LOGE(TAG_LOG, "[%s][%08x][%08x]", __FUNCTION__, ev->event, qe->event);
		if (ev->event == (0xffffff00 & qe->msg.event))
		{
			is_find = 1;
			break;
		}
	}
	SEMPHR_TRY_UNLOCK(app->lock);


	if (is_find == 1)
	{
		ev->ev_cb(app, &qe->msg);
	}
	
	return APP_FRAMEWORK_ERRNO_OK;
}

static APP_FRAMEWORK_ERRNO_t app_object_main_distribute_msg(
	APP_OBJECT_t *app,
	APP_MSG_QUEUE_ENTRY_t *qe)
{
	APP_OBJECT_t *app_new = NULL;
	
	//1.check input parmas
	if (app == NULL || qe == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	if (strcmp(qe->msg.to_app, APP_MSG_TO_ALL) == 0)
	{//distribute msg to all app
		LIST_FOREACH(app_new, &APPS_LIST, next)
		{
			if (strcmp(app_new->name, qe->msg.from_app) != 0)
			{
				app_object_send_msg(app_new, &qe->msg);
			}				
		}	
	}
	else
	{//send msg to the named app
		//1.find the named app,then send the msg
		LIST_FOREACH(app_new, &APPS_LIST, next)
		{
			if (strcmp(qe->msg.to_app, app_new->name) == 0)
			{
				app_object_send_msg(app_new, &qe->msg);
				break;
			}				
		}
	}
	SEMPHR_TRY_UNLOCK(app->lock);

	return APP_FRAMEWORK_ERRNO_OK;
} 

static int app_name_is_valid(const char *name)
{
	if (name == NULL)
	{
		return 0;
	}

	return strlen(name);	
}

void app_set_extern_functions(
	PLATFORM_API_t *api)
{
	g_apis = *api;
}

APP_OBJECT_t *app_new(
	const char *name)
{
	APP_OBJECT_t *app_obj = NULL;
	
	//1.check input parmas
	if (name == NULL || strlen(name) == 0)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return NULL;
	}

	//2.create app object,check app main exist
	if (strcmp(name, APP_MAIN_NAME) == 0)
	{
		//create app main object
		if (APP_MAIN_OBJECT == NULL)
		{
			if (app_object_create(&APP_MAIN_OBJECT, APP_MAIN_NAME) != APP_FRAMEWORK_ERRNO_OK)
			{
				return NULL;
			}

			LIST_INIT(&APPS_LIST);
			return APP_MAIN_OBJECT;
		}
		else
		{
			return APP_MAIN_OBJECT;
		}
	}
	else
	{
		//create other app object
		
		//if app main object does not exist, return NULL, should create app main first
		if (app_object_create(&app_obj, name) != APP_FRAMEWORK_ERRNO_OK)
		{
			return NULL;
		}
	}
	
	//3.add app object to the list
	if (app_list_add_object(app_obj) != APP_FRAMEWORK_ERRNO_OK)
	{
		app_delete(app_obj);
		app_obj = NULL;
	}

	return app_obj;
}

APP_FRAMEWORK_ERRNO_t app_delete(
	APP_OBJECT_t *app)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	//remove app from the list
	err = app_list_remove_object(app);
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]app_list_remove_object failed", __FUNCTION__);
		return err;
	}
	
	//delete app object
	err = app_object_delete(app);
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]app_object_delete failed", __FUNCTION__);
		return err;
	}
	
	return APP_FRAMEWORK_ERRNO_OK;
}
	
APP_FRAMEWORK_ERRNO_t app_msg_dispatch(APP_OBJECT_t *app)
{
	uint64_t time_now = APP_API_TIME_OF_DAY();
	APP_MSG_QUEUE_ENTRY_t *qe = NULL;
	APP_EVENT_MSG_t default_msg = {0};
		
	//check input params
	if (app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	//msg process
	while(app->run_status == APP_OBJECT_STATUS_RUNNING)
	{
		//foreach msg queue
		if (app_object_recv_msg(app, &qe) == APP_FRAMEWORK_ERRNO_OK)
		{
			if (app == APP_MAIN_OBJECT)
			{//distribute msg to other app
				//DEBUG_LOGE(TAG_LOG, "[%s]1[%s][%s][%s][%p]", __FUNCTION__, qe->msg.from_app, qe->msg.to_app, app->name, qe);
				app_object_main_distribute_msg(app, qe);
			}
			else
			{//excute msg callback
				//DEBUG_LOGE(TAG_LOG, "[%s]2[%s][%s]", __FUNCTION__, qe->from_app.name, qe->to_app.name);
				app_object_proc_msg(app, qe);
			}
			
			//free msg
			if (qe->msg.event_data != NULL
				&& qe->msg.event_data_len > 0)
			{
				APP_API_FREE(qe->msg.event_data);
				qe->msg.event_data = NULL;
				qe->msg.event_data_len = 0;
			}
			APP_API_FREE(qe);
			qe = NULL;
		}

		if (app->loop_callback != NULL 
			&& app->loop_timeout > 0
			&& abs(time_now -  APP_API_TIME_OF_DAY()) >= app->loop_timeout)
		{
			default_msg.event = APP_EVENT_DEFAULT_LOOP_TIMEOUT;
			app->loop_callback(app, &default_msg);
			time_now = APP_API_TIME_OF_DAY();
		}
		
		//sleep 1ms
		APP_API_SLEEP(1);
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t app_exit(APP_OBJECT_t *app)
{
	//check input params
	if (app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	app->run_status = APP_OBJECT_STATUS_EXIT;
	SEMPHR_TRY_UNLOCK(app->lock);

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t app_set_loop_timeout(
	APP_OBJECT_t *app, 
	const uint32_t loop_timeout, 
	const event_callback_fn loop_cb)
{
	//check input params
	if (app == NULL || loop_cb == NULL || loop_timeout == 0)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	app->loop_timeout = loop_timeout;
	app->loop_callback = loop_cb;
	SEMPHR_TRY_UNLOCK(app->lock);

	return APP_FRAMEWORK_ERRNO_OK;
}	

APP_FRAMEWORK_ERRNO_t app_add_event(
	APP_OBJECT_t *app, 
	const uint32_t event_base, 
	const event_callback_fn ev_cb)
{
	int is_find = 0;
	APP_EVENT_ENTRY_t *ev = NULL;
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	//check input params
	if (app == NULL || ev_cb == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	//search the event whether exist in list or not, if the event exist,update the callback
	LIST_FOREACH(ev, &app->event_list, next)
	{
		if (ev->event == event_base)
		{
			ev->ev_cb = ev_cb;
			is_find = 1;
			break;
		}
	}
	
	//if a new event, insert into event list
	if (is_find == 0)
	{
		APP_EVENT_ENTRY_t *ev_new = APP_API_MALLOC(sizeof(APP_EVENT_ENTRY_t));
		if (ev_new == NULL)
		{
			err = APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
			DEBUG_LOGE(TAG_LOG, "[%s]malloc APP_EVENT_ENTRY_t failed", __FUNCTION__);
		}
		else
		{
			memset(ev_new, 0, sizeof(APP_EVENT_ENTRY_t));
			ev_new->event = event_base;
			ev_new->ev_cb = ev_cb;
			//DEBUG_LOGE(TAG_LOG, "[%s][%08x]", __FUNCTION__, event_base);
			LIST_INSERT_HEAD(&app->event_list, ev_new, next);
		}
	}
	SEMPHR_TRY_UNLOCK(app->lock);

	return err;
}

APP_FRAMEWORK_ERRNO_t app_remove_event(
	APP_OBJECT_t *app, 
	const uint32_t event_base)
{
	int is_find = 0;
	APP_EVENT_ENTRY_t *ev = NULL;
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	//check input params
	if (app == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(app->lock);
	//search the event whether exist in list or not, if the event exist,delete event
	LIST_FOREACH(ev, &app->event_list, next)
	{
		if (ev->event == event_base)
		{
			LIST_REMOVE(ev, next);
			APP_API_FREE(ev);
			ev = NULL;
			break;
		}
	}
	SEMPHR_TRY_UNLOCK(app->lock);

	return err;
}

APP_FRAMEWORK_ERRNO_t app_send_msg(
	const APP_EVENT_MSG_t *msg)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;

	if (msg == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]invalid input params", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (APP_MAIN_OBJECT == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]please create main app first", __FUNCTION__);
		return APP_FRAMEWORK_ERRNO_MAIN_NOT_EXIST;
	}

	err = app_object_send_msg(APP_MAIN_OBJECT, msg);
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_LOG, "[%s]app_object_send_msg failed", __FUNCTION__);
	}	

	return err;
}

APP_FRAMEWORK_ERRNO_t app_send_message(
	const char *from_app, 
	const char *to_app, 
	const uint32_t event, 
	const void *event_data, 
	const uint32_t event_data_len)
{
	APP_EVENT_MSG_t msg = {0};

	snprintf(msg.from_app, sizeof(msg.from_app), "%s", from_app);
	snprintf(msg.to_app, sizeof(msg.to_app), "%s", to_app);
	msg.event = event;
	msg.event_data = event_data;
	msg.event_data_len = event_data_len;
		
	return app_send_msg(&msg);
}
	

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

#include "app_config.h"
#include "userconfig.h"
#include "aip_interface.h"
#include "device_params_manage.h"
#include "player_middleware_interface.h"

#define LOG_TAG "AIP"

//audio record status
typedef enum AUDIO_RAW_READ_STATUS_t
{
	AUDIO_RAW_READ_STATUS_IDLE = 0,
	AUDIO_RAW_READ_STATUS_START,
	AUDIO_RAW_READ_STATUS_READING,
	AUDIO_RAW_READ_STATUS_STOP,
}AUDIO_PCM_READ_STATUS_t;

//录音参数
typedef struct AIP_RECORD_PARAMS_t
{
	TAILQ_ENTRY(AIP_RECORD_PARAMS_t) next;
	
	int record_sn;			//录音编号
	int record_max_ms;		//录音最大时长
	aip_record_cb cb;		//录音数据回调 
	uint64_t rec_start_time;//开始录音时间
	AIP_RECORD_RESULT_t rec_result;//录音数据结果
}AIP_RECORD_PARAMS_t;

//audio input process handle
typedef struct AIP_RECORDER_HANDLE_t
{
	//录音PCM数据缓冲区
	uint8_t audio_pcm_buffer[RAW_PCM_LEN_MS(100, PCM_SAMPLING_RATE_16K)];

	//录音队列
	TAILQ_HEAD(AIP_RECORD_QUEUE_t, AIP_RECORD_PARAMS_t) record_queue;
	
	//录音状态
	AUDIO_PCM_READ_STATUS_t rec_status;

	//资源互斥锁
	void *mutex_lock;

	//录音序列号
	uint32_t record_sn;
}AIP_RECORDER_HANDLE_t;

//global handle
static AIP_RECORDER_HANDLE_t *g_aip_handle = NULL;

static bool rec_queue_obj_new(AIP_RECORD_PARAMS_t **object)
{
	AIP_RECORD_PARAMS_t *obj = NULL;
	
	obj = (AIP_RECORD_PARAMS_t *)memory_malloc(sizeof(AIP_RECORD_PARAMS_t));
	if (obj == NULL)
	{
		return false;
	}

	memset(obj, 0, sizeof(AIP_RECORD_PARAMS_t));
	*object = obj;
	
	return true;
}

static bool rec_queue_obj_delete(AIP_RECORD_PARAMS_t *object)
{
	if (object == NULL)
	{
		return false;
	}

	memory_free(object);

	return true;
}

static bool rec_queue_add_tail(AIP_RECORD_PARAMS_t *object)
{
	if (g_aip_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_aip_handle->mutex_lock);
	TAILQ_INSERT_TAIL(&g_aip_handle->record_queue, object, next);	
	SEMPHR_TRY_UNLOCK(g_aip_handle->mutex_lock);

	return true;
}

static bool rec_queue_first(AIP_RECORD_PARAMS_t **object)
{
	bool ret = true;
	
	if (g_aip_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_aip_handle->mutex_lock);
	*object = TAILQ_FIRST(&g_aip_handle->record_queue);
	if (*object == NULL)
	{
		ret = false;
	}
	SEMPHR_TRY_UNLOCK(g_aip_handle->mutex_lock);

	return ret;
}


static bool rec_queue_remove(AIP_RECORD_PARAMS_t *object)
{
	if (g_aip_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_aip_handle->mutex_lock);
	TAILQ_REMOVE(&g_aip_handle->record_queue, object, next);
	SEMPHR_TRY_UNLOCK(g_aip_handle->mutex_lock);

	return true;
}

#if AMC_AIP_SHARED_DATA == 1

/* 多路录音处理 */
static void aip_record_process(AIP_RECORDER_HANDLE_t *handle)
{	
	int len = 0;
	static bool is_audio_record_start = false;
	AIP_RECORD_PARAMS_t *rec_obj = NULL;
	AIP_RECORD_RESULT_t *rec_result = NULL;
	
	switch (handle->rec_status)
	{
		case AUDIO_RAW_READ_STATUS_IDLE:
		{
			handle->rec_status = AUDIO_RAW_READ_STATUS_START;
			break;
		}
		case AUDIO_RAW_READ_STATUS_START:
		{
			//启动pcm录音
			if (audio_record_start(PCM_SAMPLING_RATE_16K))
			{
				handle->rec_status = AUDIO_RAW_READ_STATUS_READING;
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "audio_record_start failed");
			}
			break;
		}
		case AUDIO_RAW_READ_STATUS_READING:
		{
			len = audio_record_read(handle->audio_pcm_buffer, sizeof(handle->audio_pcm_buffer));
			//DEBUG_LOGI(LOG_TAG, "[%d]audio record read len=[%d]bytes", 
			//	rec_result->record_id, len);
			if (len > 0)
			{			
				AIP_RECORD_PARAMS_t *rec_obj_tmp = NULL;
				TAILQ_FOREACH(rec_obj, &handle->record_queue, next)
				{
					if (rec_obj == NULL)
					{
						break;
					}

					rec_result = &rec_obj->rec_result;
					rec_result->record_id++;
					rec_result->record_ms += len/RAW_PCM_LEN_MS(1, PCM_SAMPLING_RATE_16K);
					rec_result->record_data = &handle->audio_pcm_buffer;
					rec_result->record_len = len;

					rec_obj->cb(rec_result);
					
					if (rec_result->record_ms >= rec_obj->record_max_ms)
					{
						rec_result->record_id++;
						rec_result->record_id *= -1;
						rec_result->record_data = NULL;
						rec_result->record_len = 0;
						rec_obj->cb(rec_result);
						
						DEBUG_LOGI(LOG_TAG, "record sn:[%d], record total cost:[%lldms], record_ms:[%dms]", 
							rec_result->record_sn, 
							get_time_of_day() - rec_obj->rec_start_time, 
							rec_result->record_ms);

						AIP_RECORD_PARAMS_t *rec_obj_tmp = TAILQ_PREV(rec_obj, AIP_RECORD_QUEUE_t, next);
						rec_queue_remove(rec_obj);
						rec_queue_obj_delete(rec_obj);						
						rec_obj = rec_obj_tmp;
						if (rec_obj == NULL)
						{
							break;
						}
					}		
				}
			}
			else
			{
				handle->rec_status = AUDIO_RAW_READ_STATUS_STOP;
			}
			break;
		}
		case AUDIO_RAW_READ_STATUS_STOP:
		{			
			audio_record_stop();			
			handle->rec_status = AUDIO_RAW_READ_STATUS_IDLE;
			break;
		}
		default:
			break;
	}
}

/* 支持多路录音输出 */
static void aip_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	int err = 0;
	AIP_RECORDER_HANDLE_t *handle = g_aip_handle;
	AIP_RECORD_PARAMS_t *rec_obj = NULL;
	AIP_RECORD_PARAMS_t *rec_obj_new = NULL;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			aip_record_process(handle);
			break;
		}
		case APP_EVENT_AIP_START_RECORD:
		{
			if (msg->event_data_len != sizeof(AIP_RECORD_PARAMS_t)
				|| msg->event_data == NULL)
			{
				DEBUG_LOGE(LOG_TAG, "APP_EVENT_AIP_START_RECORD invalid event data size[%d][%d]", 
					msg->event_data_len, sizeof(AIP_RECORD_PARAMS_t));
				break;
			}

			if (!rec_queue_obj_new(&rec_obj_new))
			{
				DEBUG_LOGI(LOG_TAG, "rec_queue_obj_new failed");
				break;
			}
			memcpy(rec_obj_new, msg->event_data, sizeof(AIP_RECORD_PARAMS_t));
			rec_obj_new->rec_start_time = get_time_of_day();

			//多路模式，停止相同回调接口录音
			TAILQ_FOREACH(rec_obj, &handle->record_queue, next)
			{
				if (rec_obj == NULL)
				{
					break;
				}

				if (rec_obj->cb == rec_obj_new->cb)
				{
					rec_obj->rec_result.record_id++;
					rec_obj->rec_result.record_id *= -1;
					rec_obj->rec_result.record_data = NULL;
					rec_obj->rec_result.record_len = 0;
					rec_obj->cb(&rec_obj->rec_result);
					
					DEBUG_LOGI(LOG_TAG, "record sn:[%d], record total cost:[%lldms], record_ms:[%dms]", 
						rec_obj->rec_result.record_sn,
						get_time_of_day() - rec_obj->rec_start_time, 
						rec_obj->rec_result.record_ms);

					rec_queue_remove(rec_obj);
					rec_queue_obj_delete(rec_obj);
					rec_obj = NULL;
					break;
				}
			}
			
			if (!rec_queue_add_tail(rec_obj_new))
			{
				DEBUG_LOGI(LOG_TAG, "rec_queue_add_tail failed");
			}
			
			break;
		}
		case APP_EVENT_AIP_STOP_RECORD:
		{
			if (msg->event_data_len != sizeof(AIP_RECORD_PARAMS_t)
				|| msg->event_data == NULL)
			{
				DEBUG_LOGE(LOG_TAG, "APP_EVENT_AIP_START_RECORD invalid event data size[%d][%d]", 
					msg->event_data_len, sizeof(AIP_RECORD_PARAMS_t));
				break;
			}		

			rec_obj_new = (AIP_RECORD_PARAMS_t *)msg->event_data;

			//多路模式，停止相同回调接口录音
			TAILQ_FOREACH(rec_obj, &handle->record_queue, next)
			{
				if (rec_obj == NULL)
				{
					break;
				}

				if (rec_obj->cb == rec_obj_new->cb)
				{
					rec_obj->rec_result.record_id++;
					rec_obj->rec_result.record_id *= -1;
					rec_obj->rec_result.record_data = NULL;
					rec_obj->rec_result.record_len = 0;
					rec_obj->cb(&rec_obj->rec_result);
					
					DEBUG_LOGI(LOG_TAG, "record sn:[%d], record total cost:[%lldms], record_ms:[%dms]", 
						rec_obj->rec_result.record_sn,
						get_time_of_day() - rec_obj->rec_start_time, 
						rec_obj->rec_result.record_ms);

					rec_queue_remove(rec_obj);
					rec_queue_obj_delete(rec_obj);
					rec_obj = NULL;
					break;
				}
			}
						
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

#else

/* 单路录音处理 */
static void aip_record_process(AIP_RECORDER_HANDLE_t *handle)
{	
	int len = 0;
	AIP_RECORD_PARAMS_t *rec_obj = NULL;
	AIP_RECORD_RESULT_t *rec_result = NULL;

	if (rec_queue_first(&rec_obj))
	{
		rec_result = &rec_obj->rec_result;
	}
	else
	{
		return;
	}
	
	switch (handle->rec_status)
	{
		case AUDIO_RAW_READ_STATUS_IDLE:
		{
			break;
		}
		case AUDIO_RAW_READ_STATUS_START:
		{
			//启动pcm录音
			audio_record_start(PCM_SAMPLING_RATE_16K);
			handle->rec_status = AUDIO_RAW_READ_STATUS_READING;
			rec_obj->rec_start_time = get_time_of_day();
			break;
		}
		case AUDIO_RAW_READ_STATUS_READING:
		{
			len = audio_record_read(handle->audio_pcm_buffer, sizeof(handle->audio_pcm_buffer));
			DEBUG_LOGI(LOG_TAG, "sn:[%d], id:[%d], record_len=[%d]bytes", 
				rec_result->record_sn, rec_result->record_id, len);
			if (len > 0)
			{
				rec_result->record_id++;
				rec_result->record_ms += len/RAW_PCM_LEN_MS(1, PCM_SAMPLING_RATE_16K);
				rec_result->record_data = &handle->audio_pcm_buffer;
				rec_result->record_len = len;
					
				if (rec_result->record_ms >= rec_obj->record_max_ms)
				{
					handle->rec_status = AUDIO_RAW_READ_STATUS_STOP;
				}	
				rec_obj->cb(rec_result);
			}
			else
			{
				handle->rec_status = AUDIO_RAW_READ_STATUS_STOP;
			}
			break;
		}
		case AUDIO_RAW_READ_STATUS_STOP:
		{			
			audio_record_stop();

			rec_obj->rec_result.record_id++;
			rec_obj->rec_result.record_id *= -1;
			rec_obj->rec_result.record_data = NULL;
			rec_obj->rec_result.record_len = 0;
			rec_obj->cb(&rec_obj->rec_result);
			
			DEBUG_LOGI(LOG_TAG, "record sn:[%d], record total cost[%lldms], record_ms[%dms]", 
				rec_obj->rec_result.record_sn, 
				get_time_of_day() - rec_obj->rec_start_time, 
				rec_obj->rec_result.record_ms);

			rec_queue_remove(rec_obj);
			rec_queue_obj_delete(rec_obj);
			rec_obj = NULL;
			
			handle->rec_status = AUDIO_RAW_READ_STATUS_IDLE;
			break;
		}
		default:
			break;
	}
}

/* 支持单路录音输出 */
static void aip_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	int err = 0;
	AIP_RECORDER_HANDLE_t *handle = g_aip_handle;
	AIP_RECORD_PARAMS_t *rec_obj = NULL;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			aip_record_process(handle);
			break;
		}
		case APP_EVENT_AIP_START_RECORD:
		{
			if (msg->event_data_len != sizeof(AIP_RECORD_PARAMS_t)
				|| msg->event_data == NULL)
			{
				DEBUG_LOGE(LOG_TAG, "APP_EVENT_AIP_START_RECORD invalid event data size[%d][%d]", 
					msg->event_data_len, sizeof(AIP_RECORD_PARAMS_t));
				break;
			}

			//单路模式，先结束之前的录音
			if (rec_queue_first(&rec_obj))
			{
				rec_obj->rec_result.record_id++;
				rec_obj->rec_result.record_id *= -1;
				rec_obj->rec_result.record_data = NULL;
				rec_obj->rec_result.record_len = 0;
				rec_obj->cb(&rec_obj->rec_result);
				
				DEBUG_LOGI(LOG_TAG, "record sn:[%d], record total cost[%lldms], record_ms[%dms]", 
					rec_obj->rec_result.record_sn, 
					get_time_of_day() - rec_obj->rec_start_time, 
					rec_obj->rec_result.record_ms);

				rec_queue_remove(rec_obj);
				rec_queue_obj_delete(rec_obj);
				rec_obj = NULL;
			}
			
			if (rec_queue_obj_new(&rec_obj))
			{
				handle->rec_status = AUDIO_RAW_READ_STATUS_START;
				memcpy(rec_obj, msg->event_data, sizeof(AIP_RECORD_PARAMS_t));
				rec_queue_add_tail(rec_obj);
			}
			else
			{
				DEBUG_LOGI(LOG_TAG, "rec_queue_obj_new failed");
			}
			
			break;
		}
		case APP_EVENT_AIP_STOP_RECORD:
		{
			if (handle->rec_status == AUDIO_RAW_READ_STATUS_READING
				|| handle->rec_status == AUDIO_RAW_READ_STATUS_START)
			{
				handle->rec_status = AUDIO_RAW_READ_STATUS_STOP;
			}
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
	
#endif

static void aip_service_destroy(void)
{
	if (g_aip_handle == NULL)
	{
		return;
	}

	if (g_aip_handle->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_aip_handle->mutex_lock);
		g_aip_handle->mutex_lock = NULL;
	}

	memory_free(g_aip_handle);
	g_aip_handle = NULL;
}

static void task_aip_service(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_AIP_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_AIP_SERVICE);
		aip_service_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 10, aip_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, aip_event_callback);
		app_add_event(app, APP_EVENT_AIP_BASE, aip_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_AIP_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	aip_service_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t aip_service_create(int task_priority)
{
	if (g_aip_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_aip_handle already exist");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	g_aip_handle = (AIP_RECORDER_HANDLE_t *)memory_malloc(sizeof(AIP_RECORDER_HANDLE_t));
	if (g_aip_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_aip_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_aip_handle, 0, sizeof(AIP_RECORDER_HANDLE_t));
	TAILQ_INIT(&g_aip_handle->record_queue);

	SEMPHR_CREATE_LOCK(g_aip_handle->mutex_lock);
	if (g_aip_handle->mutex_lock == NULL)
	{
		aip_service_destroy();
		DEBUG_LOGE(LOG_TAG, "SEMPHR_CREATE_LOCK failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	if (!task_thread_create(task_aip_service,
	        "task_aip_service",
	        APP_NAME_AIP_SERVICE_STACK_SIZE,
	        g_aip_handle,
	        task_priority)) 
    {
		aip_service_destroy();
        DEBUG_LOGE(LOG_TAG,  "task_aip_service failed");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t aip_service_delete(void)
{
	return app_send_message(APP_NAME_MPUSH_SERVICE, APP_NAME_MPUSH_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t aip_start_record(
	uint32_t *record_sn, 
	uint32_t  record_ms,
	aip_record_cb cb)
{
	AIP_RECORD_PARAMS_t rec_params = {0};
	
	if (g_aip_handle == NULL 
		|| g_aip_handle->mutex_lock == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "g_aip_handle is NULL");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	if (record_sn == NULL 
		|| cb == NULL 
		|| record_ms == 0)
	{
		DEBUG_LOGE(LOG_TAG,  "invalid params");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(g_aip_handle->mutex_lock);
	*record_sn = ++g_aip_handle->record_sn;
	rec_params.record_sn = *record_sn;
	rec_params.record_max_ms = record_ms;
	rec_params.cb = cb;
	rec_params.rec_result.record_sn = rec_params.record_sn;
	SEMPHR_TRY_UNLOCK(g_aip_handle->mutex_lock);
	
	return app_send_message(APP_NAME_AIP_SERVICE, APP_NAME_AIP_SERVICE, APP_EVENT_AIP_START_RECORD, &rec_params, sizeof(rec_params));
}

APP_FRAMEWORK_ERRNO_t aip_stop_record(aip_record_cb cb)
{
	AIP_RECORD_PARAMS_t rec_params = {0};
	
	if (g_aip_handle == NULL 
		|| g_aip_handle->mutex_lock == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "g_aip_handle is NULL");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	if (cb == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "invalid params");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(g_aip_handle->mutex_lock);
	rec_params.cb = cb;
	SEMPHR_TRY_UNLOCK(g_aip_handle->mutex_lock);
	
	return app_send_message(APP_NAME_AIP_SERVICE, APP_NAME_AIP_SERVICE, APP_EVENT_AIP_STOP_RECORD, &rec_params, sizeof(rec_params));
}


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
#include "keyword_wakeup_lexin.h"
#include "recorder_engine.h"
#include "app_ring_buffer.h"
#include "aip_interface.h"

#define LOG_TAG "kw_lexin"

#define KW_RING_BUFFER_SIZE (32000*2)
#define KW_MAX_RECORD_TIME_MS (~0)


/* 乐鑫唤醒库运行状态 */
typedef enum KW_LEXIN_STATUS_t
{
	KW_LEXIN_STATUS_IDLE = 0,	
	KW_LEXIN_STATUS_INIT,
	KW_LEXIN_STATUS_RUNNING,
}KW_LEXIN_STATUS_t;

/* 乐鑫唤醒库运行句柄 */
typedef struct KW_LEXIN_HANDLE_t
{
	//唤醒状态
	KW_LEXIN_STATUS_t status;
	
	//资源互斥锁
	void *mutex_lock;

	//环形缓冲区，用于存储PCM数据
	void *ring_buffer;
}KW_LEXIN_HANDLE_t;

//全局唤醒handle
static KW_LEXIN_HANDLE_t *g_kw_lexin_handle = NULL;

void kw_audio_record_cb(AIP_RECORD_RESULT_t *rec_result)
{
	int ret = 0;
	
	if (g_kw_lexin_handle == NULL)
	{
		return;
	}

	if (rec_result->record_len > 0)
	{
		ret = app_ring_buffer_write(g_kw_lexin_handle->ring_buffer, rec_result->record_data, rec_result->record_len);
		if (ret != rec_result->record_len)
		{
			DEBUG_LOGE(LOG_TAG, "app_ring_buffer_write should write[%d], write[%d]", 
				rec_result->record_len, ret);
		}
	}

	return;
}

static esp_err_t kw_rec_open(void **handle)
{
	uint32_t record_sn = 0;
	DEBUG_LOGE(LOG_TAG, "kw_rec_open");

	if (aip_start_record(&record_sn, KW_MAX_RECORD_TIME_MS, kw_audio_record_cb) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "aip_start_record failed");
		return ESP_FAIL;
	}

    return ESP_OK;
}

static esp_err_t kw_rec_read(void *handle, char *data, int data_size)
{
	int ret = 0;

	do
	{
		task_thread_sleep(10);
		ret = app_ring_buffer_readn(g_kw_lexin_handle->ring_buffer, (uint8_t *)data, data_size);
	}
	while(ret != data_size);
	
	return ret;
}

static esp_err_t kw_rec_close(void *handle)
{
	DEBUG_LOGE(LOG_TAG, "kw_rec_close");
	aip_stop_record(kw_audio_record_cb);
	
    return ESP_OK;
}

static void kw_rec_engine_cb(rec_event_type_t type)
{
	switch (type)
	{
		case REC_EVENT_WAKEUP_START:
		{
			DEBUG_LOGE(LOG_TAG, "REC_EVENT_WAKEUP_START");
			app_send_message(APP_NAME_KEYWORD_WAKEUP, APP_MSG_TO_ALL, APP_EVENT_KEYWORD_WAKEUP_NOTIFY, NULL, 0);
			break;
		}
		case REC_EVENT_WAKEUP_END:
		{
			DEBUG_LOGE(LOG_TAG, "REC_EVENT_WAKEUP_END");
			break;
		}
		case REC_EVENT_VAD_START:
		{
			DEBUG_LOGE(LOG_TAG, "REC_EVENT_VAD_START");
			break;
		}
		case REC_EVENT_VAD_STOP:
		{
			DEBUG_LOGE(LOG_TAG, "REC_EVENT_VAD_STOP");
			break;
		}
		default:
			break;
	}
}

static void start_wakeup_engine(void)
{
	rec_config_t eng = DEFAULT_REC_ENGINE_CONFIG();
    eng.vad_off_delay_ms = 600;
    eng.wakeup_time_ms = 2000;
    eng.evt_cb = kw_rec_engine_cb;
    eng.open = kw_rec_open;
    eng.close = kw_rec_close;
    eng.fetch = kw_rec_read;
    rec_engine_create(&eng);
}

//唤醒算法处理
static void keyword_wakeup_process(KW_LEXIN_HANDLE_t *handle)
{
	switch (handle->status)
	{
		case KW_LEXIN_STATUS_IDLE:
		{
			//延时5秒启动
			task_thread_sleep(5*1000);
			handle->status = KW_LEXIN_STATUS_INIT;
			break;
		}
		case KW_LEXIN_STATUS_INIT:
		{
			start_wakeup_engine();
			handle->status = KW_LEXIN_STATUS_RUNNING;
			break;
		}
		case KW_LEXIN_STATUS_RUNNING:
		{			
			break;
		}
		default:
			break;
	}
}

static void keyword_wakeup_lexin_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	int err = 0;
	KW_LEXIN_HANDLE_t *handle = g_kw_lexin_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			keyword_wakeup_process(handle);
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

static void keyword_wakeup_lexin_destroy(void)
{
	if (g_kw_lexin_handle == NULL)
	{
		return;
	}

	if (g_kw_lexin_handle->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_kw_lexin_handle->mutex_lock);
		g_kw_lexin_handle->mutex_lock = NULL;
	}

	if (g_kw_lexin_handle->ring_buffer != NULL)
	{
		app_ring_buffer_delete(g_kw_lexin_handle->ring_buffer);
		g_kw_lexin_handle->ring_buffer = NULL;
	}

	memory_free(g_kw_lexin_handle);
	g_kw_lexin_handle = NULL;
}

static void task_keyword_wakeup_lexin(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_KEYWORD_WAKEUP_LEXIN);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_KEYWORD_WAKEUP_LEXIN);
		keyword_wakeup_lexin_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 500, keyword_wakeup_lexin_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, keyword_wakeup_lexin_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_KEYWORD_WAKEUP_LEXIN);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t keyword_wakeup_lexin_create(int task_priority)
{
	if (g_kw_lexin_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_kw_lexin_handle already exist");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	g_kw_lexin_handle = (KW_LEXIN_HANDLE_t *)memory_malloc(sizeof(KW_LEXIN_HANDLE_t));
	if (g_kw_lexin_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_aip_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_kw_lexin_handle, 0, sizeof(KW_LEXIN_HANDLE_t));
	//TAILQ_INIT(&g_aip_handle->record_queue);

	//初始化资源互斥锁
	SEMPHR_CREATE_LOCK(g_kw_lexin_handle->mutex_lock);
	if (g_kw_lexin_handle->mutex_lock == NULL)
	{
		keyword_wakeup_lexin_destroy();
		DEBUG_LOGE(LOG_TAG, "SEMPHR_CREATE_LOCK failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//创建环形缓冲区
	if (app_ring_buffer_new(KW_RING_BUFFER_SIZE, &g_kw_lexin_handle->ring_buffer) != APP_FRAMEWORK_ERRNO_OK)
	{
		keyword_wakeup_lexin_destroy();
		DEBUG_LOGE(LOG_TAG, "app_ring_buffer_new failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	if (!task_thread_create(task_keyword_wakeup_lexin,
	        "task_keyword_wakeup_lexin",
	        APP_NAME_KEYWORD_WAKEUP_LEXIN_STACK_SIZE,
	        g_kw_lexin_handle,
	        task_priority)) 
    {
		keyword_wakeup_lexin_destroy();
        DEBUG_LOGE(LOG_TAG,  "task_keyword_wakeup_lexin failed");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t keyword_wakeup_lexin_delete(void)
{
	return app_send_message(APP_NAME_KEYWORD_WAKEUP_LEXIN, APP_NAME_KEYWORD_WAKEUP_LEXIN, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


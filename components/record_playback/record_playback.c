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
#include "record_playback.h"
#include "recorder_engine.h"
#include "app_ring_buffer.h"
#include "aip_interface.h"
#include "EspAudio.h"

#define LOG_TAG "record-playback"

#define RECORD_PLAYBACK_MAX_RECORD_TIME_MS (10*1000)
#define RECORD_PLAYBACK_RING_BUFFER_SIZE (32*RECORD_PLAYBACK_MAX_RECORD_TIME_MS)

/* 录音回放运行句柄 */
typedef struct RECORD_PLAYBACK_HANDLE_t
{
	uint8_t record_data[3200];

	//环形缓冲区，用于存储PCM数据
	void *ring_buffer;
}RECORD_PLAYBACK_HANDLE_t;

//全局唤醒handle
static RECORD_PLAYBACK_HANDLE_t *g_record_playback_handle = NULL;

static FILE *pcm_record_open(void)
{
	FILE *p_file = NULL;
	static int count = 1;
	char str_record_path[32] = {0};
	snprintf(str_record_path, sizeof(str_record_path), "/sdcard/record_%d.pcm", count++);
	p_file = fopen(str_record_path, "w+");
	if (p_file == NULL)
	{
		ESP_LOGE(LOG_TAG, "fopen %s failed", str_record_path);
	}
	
	return p_file;
}

static void pcm_record_write(FILE *p_file, uint8_t *pcm_data, uint32_t pcm_len)
{
	if (p_file != NULL)
	{
		fwrite(pcm_data, pcm_len, 1, p_file);
		fflush(p_file);
	}
}

static void pcm_record_close(FILE *p_file)
{
	if (p_file != NULL)
	{
		fclose(p_file);
	}
}

void record_playback_set_record_data(AIP_RECORD_RESULT_t *rec_result)
{
	int ret = 0;
	
	if (g_record_playback_handle == NULL)
	{
		return;
	}

	if (rec_result->record_len > 0)
	{
		ret = app_ring_buffer_write(g_record_playback_handle->ring_buffer, rec_result->record_data, rec_result->record_len);;
		if (ret != rec_result->record_len)
		{
			DEBUG_LOGE(LOG_TAG, "app_ring_buffer_write should write[%d], write[%d]", 
				rec_result->record_len, ret);
		}
	}

	return;
}

//录音回放处理
static void record_playback_process(RECORD_PLAYBACK_HANDLE_t *handle)
{
	int ret = 0;

	ret = EspAudioRawStop(TERMINATION_TYPE_NOW);
	if (ret != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "EspAudioRawStop failed");
	}
	
	ret = EspAudioRawStart("raw://16000:1@from.pcm/to.pcm#i2s");
	if (ret != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "EspAudioRawStart failed");
		return;
	}
	
#if 0
	int writen_bytes = 0;

	FILE *pcm = fopen("/sdcard/1.pcm", "r");
	if (pcm == NULL)
	{
		return;
	}

	while (1)
	{
		ret = fread(handle->record_data, 1, sizeof(handle->record_data), pcm);
		if (ret <= 0)
		{
			break;
		} 

		if (EspAudioRawWrite(handle->record_data, ret, &writen_bytes) != ESP_OK)
		{
			DEBUG_LOGE(LOG_TAG, "EspAudioRawWrite failed");
		}
		
		DEBUG_LOGE(LOG_TAG, "EspAudioRawWrite [%d][%d]", ret, writen_bytes);
	}
	fclose(pcm);

	return;
#endif	
	FILE *file = pcm_record_open();

	while(1)
	{
		ret = app_ring_buffer_read(handle->ring_buffer, handle->record_data, sizeof(handle->record_data));
		if (ret > 0)
		{
			pcm_record_write(file, handle->record_data, ret);
			DEBUG_LOGI(LOG_TAG, "app_ring_buffer_read[%d][%d]", sizeof(handle->record_data), ret);
			ret = EspAudioRawWrite(handle->record_data, ret, NULL);
			if (ret != ESP_OK)
			{
				DEBUG_LOGE(LOG_TAG, "EspAudioRawWrite failed");
			}
		}
		else
		{
			break;
		}
		
		task_thread_sleep(50);
	}
	
	pcm_record_close(file);
	
	return;
}

static void record_playback_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	int err = 0;
	RECORD_PLAYBACK_HANDLE_t *handle = g_record_playback_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			break;
		}
		case APP_EVENT_RECORD_PLAYBACK_PLAY:
		{
			record_playback_process(handle);
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

static void record_playback_destroy(void)
{
	if (g_record_playback_handle == NULL)
	{
		return;
	}

	if (g_record_playback_handle->ring_buffer != NULL)
	{
		app_ring_buffer_delete(g_record_playback_handle->ring_buffer);
		g_record_playback_handle->ring_buffer = NULL;
	}

	memory_free(g_record_playback_handle);
	g_record_playback_handle = NULL;
}

static void task_record_playback(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_RECORD_PLAYBACK);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_RECORD_PLAYBACK);
		record_playback_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		//app_set_loop_timeout(app, 1000, record_playback_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, record_playback_event_callback);
		app_add_event(app, APP_EVENT_RECORD_PLAYBACK_BASE, record_playback_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_RECORD_PLAYBACK);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	record_playback_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t record_playback_create(int task_priority)
{
	if (g_record_playback_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_record_playback_handle already exist");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	g_record_playback_handle = (RECORD_PLAYBACK_HANDLE_t *)memory_malloc(sizeof(RECORD_PLAYBACK_HANDLE_t));
	if (g_record_playback_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_record_playback_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_record_playback_handle, 0, sizeof(RECORD_PLAYBACK_HANDLE_t));

	//创建环形缓冲区
	if (app_ring_buffer_new(RECORD_PLAYBACK_RING_BUFFER_SIZE, &g_record_playback_handle->ring_buffer) != APP_FRAMEWORK_ERRNO_OK)
	{
		record_playback_destroy();
		DEBUG_LOGE(LOG_TAG, "app_ring_buffer_new failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	if (!task_thread_create(task_record_playback,
	        "task_record_playback",
	        APP_NAME_RECORD_PLAYBACK_STACK_SIZE,
	        g_record_playback_handle,
	        task_priority)) 
    {
		record_playback_destroy();
        DEBUG_LOGE(LOG_TAG,  "task_record_playback failed");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t record_playback_delete(void)
{
	return app_send_message(APP_NAME_RECORD_PLAYBACK, APP_NAME_RECORD_PLAYBACK, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t record_playback_start(void)
{
	uint32_t record_sn = 0;
	int ret = 0;

	if (g_record_playback_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_record_playback_handle is NULL");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	app_ring_buffer_clear(g_record_playback_handle->ring_buffer);

	ret = aip_start_record(&record_sn, RECORD_PLAYBACK_MAX_RECORD_TIME_MS, record_playback_set_record_data);
	if (ret != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "aip_start_record failed");
		return ret;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t record_playback_stop(void)
{
	int ret = 0;
	
	if (g_record_playback_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_record_playback_handle is NULL");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	ret = aip_stop_record(record_playback_set_record_data);
	if (ret != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "aip_stop_record failed");
		return ret;
	}
	
	return app_send_message(APP_NAME_RECORD_PLAYBACK, APP_NAME_RECORD_PLAYBACK, APP_EVENT_RECORD_PLAYBACK_PLAY, NULL, 0);
}


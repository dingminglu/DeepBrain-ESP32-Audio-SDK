#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "player_middleware_interface.h"
#include "keyword_wakeup.h"
#include "speech_wakeup.h"
#include "aip_interface.h"
#include "app_ring_buffer.h"
#include "wifi_manage.h"

//#define DEBUG_PRINT
#define LOG_TAG "keyword wakeup"

#define KWW_MAX_RECORD_TIME_MS	(30*24*60*60*1000)
#define KWW_MAX_CALLBACK_NUM	(10)
#define KW_RING_BUFFER_SIZE (32*1000)
#define WIFI_CONNECTED				1
#define WIFI_DISCONNECTED           0

//唤醒状态
typedef enum KW_STATUS_t
{
	KW_STATUS_IDLE = 0,	
	KW_STATUS_INIT,
	KW_STATUS_RUNNING,
}KW_STATUS_t;

//录音数据
typedef struct 
{
	//唤醒句柄
	void *wakeup_handle;
	//唤醒状态
	KW_STATUS_t status;
	//资源互斥锁
	void *mutex_lock;
	//环形缓冲区，用于存储PCM数据
	void *ring_buffer;
	//录音数据
	uint8_t rec_data[RAW_PCM_LEN_MS(100, PCM_SAMPLING_RATE_16K)];
	//启动绝对时间
	uint64_t start_time;
	//wifi状态
	uint32_t wifi_status;
}KEYWORD_WAKEUP_HANDLE_T;

static KEYWORD_WAKEUP_HANDLE_T *g_keyword_wakeup_handle = NULL;

static void set_kww_record_data(AIP_RECORD_RESULT_t *rec_result)
{ 
	int ret = 0;

	if (g_keyword_wakeup_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "set_kww_record_data invalid parmas");
		return;
	}

	if (rec_result->record_len > 0)
	{
		ret = app_ring_buffer_write(g_keyword_wakeup_handle->ring_buffer, 
							rec_result->record_data, 
							rec_result->record_len);
		if (ret != rec_result->record_len)
		{
			DEBUG_LOGE(LOG_TAG, "app_ring_buffer_write should write[%d], write[%d]", 
				rec_result->record_len, ret);
		}
	}
	
	return;
}

static void kww_rec_start(void)
{
	uint32_t record_sn = 0;
	
	if (aip_start_record(&record_sn, KWW_MAX_RECORD_TIME_MS, set_kww_record_data) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "aip_start_record failed");
	}
}

static void kww_rec_stop(void)
{
	aip_stop_record(set_kww_record_data);
}

static void kkw_wakeup_process(KEYWORD_WAKEUP_HANDLE_T *handle)
{
	int ret = 0;
	int wakeup_count = 0;
	int frame_start = 0;
	
	switch (handle->status)
	{
		case KW_STATUS_IDLE:
		{	
			if(get_time_of_day() - handle->start_time >= 15000)
			{
				handle->status = KW_STATUS_INIT;
			}
			break;
		}
		case KW_STATUS_INIT:
		{
			keyword_wakeup_start();		
			
			//创建唤醒词句柄
			if (handle->wakeup_handle == NULL)
			{
				YQSW_SetCustomAddr(KWW_AUTH_ADDR);
				YQSW_CreateChannel(&handle->wakeup_handle, "none");
				if (handle->wakeup_handle != NULL)
				{
					YQSW_ResetChannel(handle->wakeup_handle);	
				}
				else
				{
					DEBUG_LOGE(LOG_TAG, "task_keyword_wakeup YQSW_CreateChannel failed");
					break;
				}
			}
			
			handle->status = KW_STATUS_RUNNING;
			break;
		}
		case KW_STATUS_RUNNING:
		{
			int frame_len =	app_ring_buffer_read(g_keyword_wakeup_handle->ring_buffer, 
								g_keyword_wakeup_handle->rec_data, 
								sizeof(g_keyword_wakeup_handle->rec_data));
			//DEBUG_LOGE(LOG_TAG, "frame_len=[%d]", frame_len);
			//唤醒词唤醒
			while (frame_len >= RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K))
			{
				ret = YQSW_ProcessWakeup(handle->wakeup_handle, 
					(int16_t*)(g_keyword_wakeup_handle->rec_data + frame_start), 
					RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K)/2);
				//DEBUG_LOGE(LOG_TAG, "YQSW_ProcessWakeup=[%d]", ret);
				if (ret == 1)
				{
					wakeup_count++;
					app_send_message(APP_NAME_KEYWORD_WAKEUP, APP_MSG_TO_ALL, APP_EVENT_KEYWORD_WAKEUP_NOTIFY, NULL, 0);
					DEBUG_LOGE(LOG_TAG, "keyword wakeup now,count[%d]", wakeup_count);
					YQSW_ResetChannel(handle->wakeup_handle);
					break;
				}
				frame_len -= RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K);
				frame_start += RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K);
			}			
			break;
		}
		default:
			break;
	}
}

static void keyword_wakeup_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	KEYWORD_WAKEUP_HANDLE_T *handle = g_keyword_wakeup_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED:
		{ 
			handle->status = KW_STATUS_INIT;
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			kkw_wakeup_process(handle);
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit(app);
			break;
		}
		case APP_EVENT_KEYWORD_WAKEUP_NOTIFY:
		{
			break;
		}
		default:
			break;
	}
}

static void wakeup_service_destroy(void)
{
	if (g_keyword_wakeup_handle == NULL)
	{
		return;
	}

	if (g_keyword_wakeup_handle->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_keyword_wakeup_handle->mutex_lock);
		g_keyword_wakeup_handle->mutex_lock = NULL;
	}

	if (g_keyword_wakeup_handle->ring_buffer != NULL)
	{
		app_ring_buffer_delete(g_keyword_wakeup_handle->ring_buffer);
		g_keyword_wakeup_handle->ring_buffer = NULL;
	}
	
	memory_free(g_keyword_wakeup_handle);
	g_keyword_wakeup_handle = NULL;
}

static void task_keyword_wakeup(void *pv)
{
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_KEYWORD_WAKEUP);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_KEYWORD_WAKEUP);
		wakeup_service_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 10, keyword_wakeup_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, keyword_wakeup_event_callback);
		app_add_event(app, APP_EVENT_KEYWORD_WAKEUP_BASE, keyword_wakeup_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, keyword_wakeup_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_KEYWORD_WAKEUP);
	}

	app_msg_dispatch(app);
	
	app_delete(app);	

	wakeup_service_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t keyword_wakeup_create(int task_priority)
{
	if (g_keyword_wakeup_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_keyword_wakeup_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	g_keyword_wakeup_handle = (KEYWORD_WAKEUP_HANDLE_T *)memory_malloc(sizeof(KEYWORD_WAKEUP_HANDLE_T));
	if (g_keyword_wakeup_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_keyword_wakeup_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset((char*)g_keyword_wakeup_handle, 0, sizeof(KEYWORD_WAKEUP_HANDLE_T));
	g_keyword_wakeup_handle->start_time = get_time_of_day();
	
	//初始化参数
	SEMPHR_CREATE_LOCK(g_keyword_wakeup_handle->mutex_lock);
	if (g_keyword_wakeup_handle->mutex_lock == NULL)
	{
		wakeup_service_destroy();
		DEBUG_LOGE(LOG_TAG, "g_keyword_wakeup_handle->mutex_lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//创建环形缓冲区
	if (app_ring_buffer_new(KW_RING_BUFFER_SIZE, &g_keyword_wakeup_handle->ring_buffer) != APP_FRAMEWORK_ERRNO_OK)
	{
		wakeup_service_destroy();
		DEBUG_LOGE(LOG_TAG, "app_ring_buffer_new failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	//创建线程
	if (!task_thread_create(task_keyword_wakeup,
			"task_keyword_wakeup",
			APP_NAME_KEYWORD_WAKEUP_STACK_SIZE,
			g_keyword_wakeup_handle,
			task_priority)) 
	{
			DEBUG_LOGE(LOG_TAG, "create task_keyword_wakeup failed");
			wakeup_service_destroy();
			return APP_FRAMEWORK_ERRNO_FAIL;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t keyword_wakeup_delete(void)
{
	return app_send_message(APP_NAME_KEYWORD_WAKEUP, APP_NAME_KEYWORD_WAKEUP, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

void keyword_wakeup_start(void)
{
	kww_rec_start();
}

void keyword_wakeup_stop(void)
{
	kww_rec_stop();
}


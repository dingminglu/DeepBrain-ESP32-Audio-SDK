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

#include "mpush_msg.h"
#include "mpush_interface.h"
#include "mpush_service.h"
#include "cJSON.h"
#include "play_list.h"
#include "player_middleware_interface.h"
#include "asr_service.h"
#include "free_talk.h"

#define LOG_TAG "MPUSH_MSG"

//mpush msg process handle
typedef struct MPUSH_MSG_HANDLE_t
{
	MPUSH_CLIENT_MSG_INFO_t msg_info;
	int play_status;
	char tts_url[2048];
	int charging_state;
}MPUSH_MSG_HANDLE_t;

static MPUSH_MSG_HANDLE_t *g_mpush_msg_handle = NULL;
static void* g_lock_mpush_msg = NULL;

void mpush_msg_event_notify(const AUDIO_PLAY_EVENTS_T *event)
{
	switch (event->play_status)
	{
		case AUDIO_PLAY_STATUS_UNKNOWN:
		{
			break;
		}
		case AUDIO_PLAY_STATUS_PLAYING:
		{
			break;
		}
		case AUDIO_PLAY_STATUS_PAUSED:
		case AUDIO_PLAY_STATUS_FINISHED:
		case AUDIO_PLAY_STATUS_STOP:
		case AUDIO_PLAY_STATUS_ERROR:
		{
			if (g_mpush_msg_handle != NULL)
			{
				g_mpush_msg_handle->play_status = 0;
			}

			if (g_mpush_msg_handle->charging_state == 1)
			{
				//充电时灯效
				app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_OFF, NULL, 0);
				app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_OFF, NULL, 0);
			}
			else
			{
				//恢复待机灯效
				app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
				app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			}
			
			break;
		}
		case AUDIO_PLAY_STATUS_AUX_IN:
		{
			break;
		}
		default:
			break;
	}
}

static void get_app_play_list(const char* playlist)
{	
	char buff[96] = {0};
	
	cJSON * pJson = cJSON_Parse(playlist);
	if (pJson == NULL)
	{
		return;
	}
	
	cJSON *pJson_command = cJSON_GetObjectItem(pJson, "command");
	if (NULL == pJson_command || pJson_command->valuestring == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "_pJson_command not found");
		goto get_app_play_list_error;
	}

	if (strcmp(pJson_command->valuestring, "playList") == 0)
	{
		cJSON *pJson_date = cJSON_GetObjectItem(pJson, "data");
		if (NULL == pJson_date)
		{
			DEBUG_LOGE(LOG_TAG, "_pJson_data not found");
			goto get_app_play_list_error;
		}
		
		int list_size = cJSON_GetArraySize(pJson_date);
		if (list_size <= 0)
		{
			DEBUG_LOGE(LOG_TAG, "list_size is 0, no offline msg");
			goto get_app_play_list_error;
		}
		else
		{
			void *playlist = NULL;

			if (new_playlist(&playlist) != APP_FRAMEWORK_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "new_playlist failed");
				goto get_app_play_list_error;
			}				
			
			int i = 0;
			for (i = 0; i < list_size; i++)
			{
				cJSON *pJson_item = cJSON_GetArrayItem(pJson_date, i);
				cJSON *pJson_name = cJSON_GetObjectItem(pJson_item, "name");
				cJSON *pJson_url = cJSON_GetObjectItem(pJson_item, "url");
				if (NULL == pJson_name
					|| NULL == pJson_name->valuestring
					|| strlen(pJson_name->valuestring) <= 0
					|| NULL == pJson_url
					|| NULL == pJson_url->valuestring
					|| strlen(pJson_url->valuestring) <= 0)
				{
					continue;
				}

				snprintf(buff, sizeof(buff), "正在给您播放%s", pJson_name->valuestring);
				memset(g_mpush_msg_handle->tts_url, 0, sizeof(g_mpush_msg_handle->tts_url));

				bool ret = get_tts_play_url(buff, g_mpush_msg_handle->tts_url, sizeof(g_mpush_msg_handle->tts_url));
				if (!ret)
				{
					ret = get_tts_play_url(buff, g_mpush_msg_handle->tts_url, sizeof(g_mpush_msg_handle->tts_url));
				}
				
				if (ret)
				{
					add_playlist_item(playlist, g_mpush_msg_handle->tts_url, NULL);
				}
				add_playlist_item(playlist, pJson_url->valuestring, NULL);
			}

			set_playlist_mode(playlist, PLAY_LIST_MODE_ALL);
			playlist_add(&playlist);
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "_pJson_command faild !");
		goto get_app_play_list_error;
	}

get_app_play_list_error:
	if (pJson != NULL)
	{
		cJSON_Delete(pJson);
	}

}

static void mpush_process_msg(MPUSH_MSG_HANDLE_t *handle)
{
	if (handle == NULL)
	{
		return;
	}

	if (handle->play_status == 1)
	{
		return;
	}
	
	if (mpush_get_msg_queue(&handle->msg_info) != APP_FRAMEWORK_ERRNO_OK)
	{
		return;
	}

	DEBUG_LOGE(LOG_TAG, "msg content[%s]", handle->msg_info.msg_content);
	switch (handle->msg_info.msg_type)
	{
		case MPUSH_SEND_MSG_TYPE_NONE:
		{
			
			break;
		}
		case MPUSH_SEND_MSG_TYPE_TEXT:
		{
			//来消息灯效
			app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_300_MS_FLICKER, NULL, 0);
			app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			free_talk_terminated();	
			memset(handle->tts_url, 0, sizeof(handle->tts_url));
			bool ret = get_tts_play_url(handle->msg_info.msg_content, handle->tts_url, sizeof(handle->tts_url));
			if (!ret)
			{
				ret = get_tts_play_url(handle->msg_info.msg_content, handle->tts_url, sizeof(handle->tts_url));
			}
			
			if (ret)
			{
				audio_play_tone_with_cb(handle->tts_url, AUDIO_TERM_TYPE_NOW, mpush_msg_event_notify);
				handle->play_status = 1;
			}
			break;
		}
		case MPUSH_SEND_MSG_TYPE_FILE:
		{
			break;
		}
		case MPUSH_SEND_MSG_TYPE_LINK:
		{
			//来消息灯效
			app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_300_MS_FLICKER, NULL, 0);
			app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			free_talk_terminated();
			audio_play_tone_with_cb(handle->msg_info.msg_content, AUDIO_TERM_TYPE_NOW, mpush_msg_event_notify);
			handle->play_status = 1;
			break;
		}
		case MPUSH_SEND_MSG_TYPE_CMD:
		{
			get_app_play_list(handle->msg_info.msg_content);
			break;
		}
		default:
			break;
	}
}

static void mpush_msg_event_callback(
	APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	int err = 0;
	MPUSH_MSG_HANDLE_t *handle = g_mpush_msg_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			mpush_process_msg(handle);
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{
			break;
		}
		case APP_EVENT_POWER_CHARGING:
		{
			g_mpush_msg_handle->charging_state = 1;
			break;
		}
		case APP_EVENT_POWER_CHARGING_STOP:
		{
			g_mpush_msg_handle->charging_state = 0;
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

static void task_mpush_msg(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_MPUSH_MESSAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_MPUSH_MESSAGE);
	}
	else
	{
		app_set_loop_timeout(app, 1000, mpush_msg_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, mpush_msg_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, mpush_msg_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, mpush_msg_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_MPUSH_MESSAGE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t mpush_msg_create(int task_priority)
{
	g_mpush_msg_handle = (MPUSH_MSG_HANDLE_t*)memory_malloc(sizeof(MPUSH_MSG_HANDLE_t));
	if (g_mpush_msg_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_msg_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_mpush_msg_handle, 0, sizeof(MPUSH_MSG_HANDLE_t));
	
	//TAILQ_INIT(&g_mpush_msg_handle->msg_queue);	
	SEMPHR_CREATE_LOCK(g_lock_mpush_msg);

	if (!task_thread_create(task_mpush_msg,
                    "task_mpush_msg",
                    APP_NAME_MPUSH_MESSAGE_STACK_SIZE,
                    g_mpush_msg_handle,
                    task_priority)) 
    {
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_mpush_msg task! Out of memory?");

		memory_free(g_mpush_msg_handle);
		g_mpush_msg_handle = NULL;
		SEMPHR_DELETE_LOCK(g_lock_mpush_msg);
		
		return APP_FRAMEWORK_ERRNO_FAIL;
    }
	DEBUG_LOGI(LOG_TAG, "mpush_msg_create success");

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t mpush_msg_delete(void)
{
	return app_send_message(APP_NAME_MPUSH_MESSAGE, APP_NAME_MPUSH_MESSAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


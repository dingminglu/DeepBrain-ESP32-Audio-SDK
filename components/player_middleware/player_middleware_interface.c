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

#include "player_middleware_interface.h"
#include "EspAudio.h"
#include "MediaService.h"
#include "array_music.h"
#include "asr_service.h"

#define LOG_TAG "player middlerware"

/* 音频播放对象 */
typedef struct AUDIO_PLAY_OBJECT_t
{
	//tail queue entry
	TAILQ_ENTRY(AUDIO_PLAY_OBJECT_t) next;

	AUDIO_PLAY_EVENTS_T 	play_event;
	audio_play_event_notify event_callback;	//事件回调通知函数 

	//错误处理
	uint32_t error_count;
}AUDIO_PLAY_OBJECT_t;

/* player middleware handle */
typedef struct PLAYER_MIDDLEWARE_HANDLE_t
{
	//tone queue
	TAILQ_HEAD(AUDIO_PLAY_TONE_QUEUE_t, AUDIO_PLAY_OBJECT_t) tone_queue;

	//music queue
	TAILQ_HEAD(AUDIO_PLAY_MUSIC_QUEUE_t, AUDIO_PLAY_OBJECT_t) music_queue;
	
	void *service;
	void *play_status_queue;
	void *mutex_lock;
}PLAYER_MIDDLEWARE_HANDLE_t;

/* play tts handler */
typedef struct PLAYER_MIDDLEWARE_TTS_HANDLER_t
{
	char tts_url[2048];
}PLAYER_MIDDLEWARE_TTS_HANDLER_t;

static PLAYER_MIDDLEWARE_HANDLE_t *g_player_middleware_handle = NULL;

/**
 * audio tone queue add tail
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_tone_queue_add_tail(AUDIO_PLAY_OBJECT_t *object)
{
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	TAILQ_INSERT_TAIL(&g_player_middleware_handle->tone_queue, object, next);	

	return true;
}

/**
 * audio tone queue add firstly
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_tone_queue_first(AUDIO_PLAY_OBJECT_t **object)
{
	bool ret = true;
	
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	*object = TAILQ_FIRST(&g_player_middleware_handle->tone_queue);
	if (*object == NULL)
	{
		ret = false;
	}

	return ret;
}

/**
 * audio tone queue remove
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_tone_queue_remove(AUDIO_PLAY_OBJECT_t *object)
{
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	TAILQ_REMOVE(&g_player_middleware_handle->tone_queue, object, next);

	return true;
}

/**
 * audio music queue add tail
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_music_queue_add_tail(AUDIO_PLAY_OBJECT_t *object)
{
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	TAILQ_INSERT_TAIL(&g_player_middleware_handle->music_queue, object, next);	

	return true;
}

/**
 * audio music queue add firstly
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_music_queue_first(AUDIO_PLAY_OBJECT_t **object)
{
	bool ret = true;
	
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	*object = TAILQ_FIRST(&g_player_middleware_handle->music_queue);
	if (*object == NULL)
	{
		ret = false;
	}

	return ret;
}

/**
 * audio music queue remove
 *
 * @param object,audio play object
 * @return true/false
 */
static bool audio_music_queue_remove(AUDIO_PLAY_OBJECT_t *object)
{
	if (g_player_middleware_handle == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	TAILQ_REMOVE(&g_player_middleware_handle->music_queue, object, next);

	return true;
}

/**
 * update audio play status
 *
 * @param object,audio play object
 * @param play_status,audio play status
 * @return true/false
 */
static bool update_audio_play_status(
	AUDIO_PLAY_OBJECT_t *object,
	AUDIO_PLAY_STATUS_t play_status)
{	
	if (object == NULL)
	{
		return false;
	}

	object->play_event.play_status = play_status;
	if (object->event_callback != NULL)
	{
		object->event_callback(&object->play_event);
	}

	return true;
}

/**
 * notify audio play status
 *
 * @param object,audio play object
 * @param play_status,PlayerStatus including the source of the music and so on.
 * @return none
 */
static void notify_audio_play_status(PlayerStatus *player_status)
{
	char mode[32]= {0};
	char status[32] = {0};
	bool is_need_free = false;
	AUDIO_PLAY_OBJECT_t *audio_obj = NULL;
	
	if (player_status == NULL)
	{
		return;
	}

	switch (player_status->mode)
	{
		case PLAYER_WORKING_MODE_NONE:
		{
			snprintf(mode, sizeof(mode), "PLAYER_WORKING_MODE_NONE");
			break;
		}
		case PLAYER_WORKING_MODE_MUSIC:
		{
			snprintf(mode, sizeof(mode), "PLAYER_WORKING_MODE_MUSIC");
			audio_music_queue_first(&audio_obj);
			break;
		}
		case PLAYER_WORKING_MODE_TONE:
		{
			snprintf(mode, sizeof(mode), "PLAYER_WORKING_MODE_TONE");
			audio_tone_queue_first(&audio_obj);
			break;
		}
		case PLAYER_WORKING_MODE_RAW:
		{
			snprintf(mode, sizeof(mode), "PLAYER_WORKING_MODE_RAW");
			break;
		}
		default:
			break;
	}

	switch (player_status->status)
	{
		case AUDIO_STATUS_UNKNOWN:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_UNKNOWN");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_UNKNOWN);
			break;
		}
		case AUDIO_STATUS_PLAYING:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_PLAYING");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_PLAYING);
			break;
		}
		case AUDIO_STATUS_PAUSED:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_PAUSED");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_PAUSED);
			break;
		}
		case AUDIO_STATUS_FINISHED:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_FINISHED");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_FINISHED);
			if (player_status->mode == PLAYER_WORKING_MODE_MUSIC)
			{
				audio_music_queue_remove(audio_obj);
				is_need_free = true;
			}
			else if (player_status->mode == PLAYER_WORKING_MODE_TONE)
			{
				audio_tone_queue_remove(audio_obj);
				is_need_free = true;
			}
			else 
			{

			}
			break;
		}
		case AUDIO_STATUS_STOP:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_STOP");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_STOP);
			if (player_status->mode == PLAYER_WORKING_MODE_MUSIC)
			{
				audio_music_queue_remove(audio_obj);
				is_need_free = true;
			}
			else if (player_status->mode == PLAYER_WORKING_MODE_TONE)
			{
				audio_tone_queue_remove(audio_obj);
				is_need_free = true;
			}
			else 
			{

			}
			break;
		}
		case AUDIO_STATUS_ERROR:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_ERROR");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_ERROR);
			if (player_status->mode == PLAYER_WORKING_MODE_MUSIC)
			{
				/*
				audio_obj->error_count++;
				if (audio_obj->error_count > 3)
				{
				*/
					audio_music_queue_remove(audio_obj);
					is_need_free = true;
					EspAudioStop();
				/*
				}
				else
				{
					int32_t pos = 0;
					EspAudioPosGet(&pos);
					EspAudioPlay(audio_obj, pos);
				}
				*/
			}
			else if (player_status->mode == PLAYER_WORKING_MODE_TONE)
			{
				audio_tone_queue_remove(audio_obj);
				is_need_free = true;
				EspAudioToneStop();
			}
			else 
			{

			}
			break;
		}
		case AUDIO_STATUS_AUX_IN:
		{
			snprintf(status, sizeof(status), "AUDIO_STATUS_AUX_IN");
			update_audio_play_status(audio_obj, AUDIO_PLAY_STATUS_AUX_IN);
			break;
		}
		default:
			break;
	}

	if (audio_obj != NULL)
	{
		DEBUG_LOGW(LOG_TAG, "mode:%s, status:%s, error code:0x%x, url:%s", 
		mode, status, player_status->errMsg, audio_obj->play_event.music_url);
	}
	else
	{
		DEBUG_LOGW(LOG_TAG, "mode:%s, status:%s, error code:0x%x", 
		mode, status, player_status->errMsg);
	}

	if (is_need_free
		&& audio_obj != NULL)
	{
		memory_free(audio_obj);
		audio_obj = NULL;
	}
}

static void player_middleware_callback(
	void *app, 
	APP_EVENT_MSG_t *msg)
{	
	PlayerStatus player_status = {0};
	MusicInfo music_info = {0};
	PLAYER_MIDDLEWARE_HANDLE_t *handle = g_player_middleware_handle;
		
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			if (handle == NULL || handle->play_status_queue == NULL)
			{
				break;
			}
			
			//int32_t pos = 0;
			//EspAudioPosGet(&pos);
			//DEBUG_LOGI(LOG_TAG, "pos[%d]", pos);
			
			if (xQueueReceive(handle->play_status_queue, &player_status, 1) == pdTRUE) 
			{
				
				SEMPHR_TRY_LOCK(g_player_middleware_handle->mutex_lock);
				notify_audio_play_status(&player_status);
				SEMPHR_TRY_UNLOCK(g_player_middleware_handle->mutex_lock);
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

static void player_middleware_destroy(void)
{
	if (g_player_middleware_handle == NULL)
	{
		return;
	}

	if (g_player_middleware_handle->play_status_queue != NULL)
	{
		vQueueDelete(g_player_middleware_handle->play_status_queue);
		g_player_middleware_handle->play_status_queue = NULL;
	}

	if (g_player_middleware_handle->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_player_middleware_handle->mutex_lock);
		g_player_middleware_handle->mutex_lock = NULL;
	}

	memory_free(g_player_middleware_handle);
	g_player_middleware_handle = NULL;

	return;
}

static void task_player_middleware(void *pv)
{
	APP_OBJECT_t *app = app_new(APP_NAME_PLAYER_MIDDLEWARE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_PLAYER_MIDDLEWARE);
		player_middleware_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 100, player_middleware_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, player_middleware_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_PLAYER_MIDDLEWARE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);	

	player_middleware_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t player_middleware_create(const uint32_t task_priority)
{	
	if (g_player_middleware_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_player_middleware_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	g_player_middleware_handle = (PLAYER_MIDDLEWARE_HANDLE_t*)memory_malloc(sizeof(PLAYER_MIDDLEWARE_HANDLE_t));
	if (g_player_middleware_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_player_middleware_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	memset(g_player_middleware_handle, 0, sizeof(PLAYER_MIDDLEWARE_HANDLE_t));
	TAILQ_INIT(&g_player_middleware_handle->tone_queue);
	TAILQ_INIT(&g_player_middleware_handle->music_queue);
		
	g_player_middleware_handle->play_status_queue = xQueueCreate(5, sizeof(PlayerStatus));
	if (g_player_middleware_handle->play_status_queue == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "play_status_queue malloc failed");
		player_middleware_destroy();
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	SEMPHR_CREATE_LOCK(g_player_middleware_handle->mutex_lock);
	if (g_player_middleware_handle->mutex_lock == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "mutex_lock malloc failed");
		player_middleware_destroy();
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	if (!task_thread_create(task_player_middleware, 
			"task_player_middleware", 
			APP_NAME_PLAYER_MIDDLEWARE_STACK_SIZE, 
			NULL, 
			task_priority)) 
    {
        DEBUG_LOGE(LOG_TAG, "player_middleware_create failed");
		player_middleware_destroy();
		return APP_FRAMEWORK_ERRNO_FAIL; 
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t player_middleware_delete(void)
{
	return app_send_message(APP_NAME_PLAYER_MIDDLEWARE, APP_NAME_PLAYER_MIDDLEWARE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

bool player_middleware_set_param(
	const PLAYER_MIDDLEWARE_PARAM_INDEX_t index,
	const void* param,
	const uint32_t param_len)
{
	if (param == NULL
		|| param_len == 0
		|| g_player_middleware_handle == NULL)
	{
		return false;
	}
	
	switch (index)
	{
		case PLAYER_MIDDLEWARE_PARAM_INDEX_SERVICE_HANDLE:
		{
			MediaService *service = (MediaService *)param;
			service->addListener((MediaService *)service, g_player_middleware_handle->play_status_queue);			
			DEBUG_LOGE(LOG_TAG, "addListener success");
			break;
		}
		default:
			break;
	}

	return true;
}

AUDIO_PLAY_ERROR_CODE_t audio_play_tone(
	const char* const audio_url,
	const AUDIO_TERMINATION_TYPE_t term_type)
{
	return audio_play_tone_with_cb(audio_url, term_type, NULL);
}

AUDIO_PLAY_ERROR_CODE_t audio_play_tone_with_cb(
	const char* const audio_url,
	const AUDIO_TERMINATION_TYPE_t term_type,
	const audio_play_event_notify cb)
{
	bool ret = false;
	AUDIO_PLAY_OBJECT_t *audio_obj = NULL;
	AUDIO_PLAY_OBJECT_t *last_obj = NULL;
	enum TerminationType tem_type = TERMINATION_TYPE_NOW;
	
	if (g_player_middleware_handle == NULL
		|| audio_url == NULL
		|| strlen(audio_url) == 0)
	{
		DEBUG_LOGE(LOG_TAG, "g_player_middleware_handle or audio_url is NULL");
		return AUDIO_PLAY_ERROR_CODE_INVALID_PARAMS;
	}

	if (term_type == AUDIO_TERM_TYPE_NOW)
	{
		tem_type = TERMINATION_TYPE_NOW;
	}
	else if (term_type == AUDIO_TERM_TYPE_DONE)
	{
		tem_type = TERMINATION_TYPE_DONE;
	}
	else
	{
		return AUDIO_PLAY_ERROR_CODE_FAIL;
	}

	audio_obj = (AUDIO_PLAY_OBJECT_t *)memory_malloc(sizeof(AUDIO_PLAY_OBJECT_t)); 
	if (audio_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "audio_obj memory_malloc failed");
		return AUDIO_PLAY_ERROR_CODE_MALLOC_FAILED;
	}

	memset(audio_obj, 0, sizeof(AUDIO_PLAY_OBJECT_t));
	snprintf((char*)audio_obj->play_event.music_url, sizeof(audio_obj->play_event.music_url), "%s", audio_url);
	audio_obj->event_callback = cb;

	//remove last object
	SEMPHR_TRY_LOCK(g_player_middleware_handle->mutex_lock);
	if (term_type == AUDIO_TERM_TYPE_NOW && 
		audio_tone_queue_first(&last_obj))
	{
		update_audio_play_status(last_obj, AUDIO_PLAY_STATUS_STOP);
		audio_tone_queue_remove(last_obj);
		memory_free(last_obj);
		last_obj = NULL;
	}

	ret = audio_tone_queue_add_tail(audio_obj);
	if (!ret)
	{
		DEBUG_LOGE(LOG_TAG, "audio_tone_queue_add_tail failed");
		memory_free(audio_obj);
		audio_obj = NULL;
	}
	SEMPHR_TRY_UNLOCK(g_player_middleware_handle->mutex_lock);

	if (!ret)
	{
		return AUDIO_PLAY_ERROR_CODE_FAIL;
	}

	if (EspAudioTonePlay(audio_url, tem_type, 1) != AUDIO_ERR_NO_ERROR)
	{
		DEBUG_LOGE(LOG_TAG, "EspAudioTonePlay failed");
		audio_tone_queue_remove(audio_obj);
		memory_free(audio_obj);
		audio_obj = NULL;
		return AUDIO_PLAY_ERROR_CODE_FAIL;
	}

	return AUDIO_PLAY_ERROR_CODE_OK;
}


AUDIO_PLAY_ERROR_CODE_t audio_play_music(const char* const audio_url)
{
	return audio_play_music_with_cb(audio_url, NULL);
}

AUDIO_PLAY_ERROR_CODE_t audio_play_music_with_cb(
	const char* const audio_url,
	const audio_play_event_notify cb)
{
	bool ret = false;
	AUDIO_PLAY_OBJECT_t *audio_obj = NULL;
	AUDIO_PLAY_OBJECT_t *last_obj = NULL;
	AUDIO_PLAY_EVENTS_T play_events = {0};
	
	if (g_player_middleware_handle == NULL
		|| audio_url == NULL
		|| strlen(audio_url) == 0)
	{
		DEBUG_LOGE(LOG_TAG, "g_player_middleware_handle or audio_url is NULL");
		return AUDIO_PLAY_ERROR_CODE_INVALID_PARAMS;
	}

	audio_obj = (AUDIO_PLAY_OBJECT_t *)memory_malloc(sizeof(AUDIO_PLAY_OBJECT_t)); 
	if (audio_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "audio_obj memory_malloc failed");
		return AUDIO_PLAY_ERROR_CODE_MALLOC_FAILED;
	}

	memset(audio_obj, 0, sizeof(AUDIO_PLAY_OBJECT_t));
	snprintf((char*)audio_obj->play_event.music_url, sizeof(audio_obj->play_event.music_url), "%s", audio_url);
	audio_obj->event_callback = cb;

	//remove last object
	SEMPHR_TRY_LOCK(g_player_middleware_handle->mutex_lock);
	if (audio_music_queue_first(&last_obj))
	{
		update_audio_play_status(last_obj, AUDIO_PLAY_STATUS_STOP);
		audio_music_queue_remove(last_obj);
		memory_free(last_obj);
		last_obj = NULL;
	}

	ret = audio_music_queue_add_tail(audio_obj);
	if (!ret)
	{
		DEBUG_LOGE(LOG_TAG, "audio_tone_queue_add_tail failed");
		memory_free(audio_obj);
		audio_obj = NULL;
	}
	SEMPHR_TRY_UNLOCK(g_player_middleware_handle->mutex_lock);

	if (!ret)
	{
		return AUDIO_PLAY_ERROR_CODE_FAIL;
	}
	
	if (EspAudioPlay(audio_url, 0) != AUDIO_ERR_NO_ERROR)
	{
		DEBUG_LOGE(LOG_TAG, "EspAudioPlay failed");
		audio_music_queue_remove(audio_obj);
		memory_free(audio_obj);
		audio_obj = NULL;
		return AUDIO_PLAY_ERROR_CODE_FAIL;
	}

	return AUDIO_PLAY_ERROR_CODE_OK;
}

AUDIO_PLAY_ERROR_CODE_t audio_play_tone_mem(
	const int flash_music_index, 
	const AUDIO_TERMINATION_TYPE_t term_type)
{
	return audio_play_tone_mem_with_cb(flash_music_index, term_type, NULL);
}

AUDIO_PLAY_ERROR_CODE_t audio_play_tone_mem_with_cb(
	const int flash_music_index, 
	const AUDIO_TERMINATION_TYPE_t term_type, 
	const audio_play_event_notify cb)
{
	char *audio_url = get_flash_music_name_ex(flash_music_index);
	if (audio_url == NULL)
	{
		return AUDIO_PLAY_ERROR_CODE_INVALID_PARAMS;
	}
	
	return audio_play_tone_with_cb(audio_url, term_type, cb);
}

AUDIO_PLAY_ERROR_CODE_t audio_play_tts(const char* text)
{
	bool ret = false;
	int err = AUDIO_PLAY_ERROR_CODE_OK;
	PLAYER_MIDDLEWARE_TTS_HANDLER_t *handler = NULL;
		
	if (text == NULL
		|| strlen(text) == 0)
	{
		return AUDIO_PLAY_ERROR_CODE_INVALID_PARAMS;
	}

	handler = (PLAYER_MIDDLEWARE_TTS_HANDLER_t *)memory_malloc(sizeof(PLAYER_MIDDLEWARE_TTS_HANDLER_t));
	if (handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc PLAYER_MIDDLEWARE_TTS_HANDLER_t failed");
		return AUDIO_PLAY_ERROR_CODE_MALLOC_FAILED;
	}

	ret = get_tts_play_url(text, (char*)handler->tts_url, sizeof(handler->tts_url));
	if (!ret)
	{
		ret = get_tts_play_url(text, (char*)handler->tts_url, sizeof(handler->tts_url));
	}
	
	if (ret)
	{
		audio_play_tone((char*)handler->tts_url, AUDIO_TERM_TYPE_NOW);
		err =  AUDIO_PLAY_ERROR_CODE_OK;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "get_tts_play_url failed");
		err = AUDIO_PLAY_ERROR_CODE_FAIL;
	}

	memory_free(handler);
	handler = NULL;
	
	return err;
}

void audio_play_stop(void)
{
	EspAudioStop();
	EspAudioToneStop();
}

void audio_play_pause(void)
{
	EspAudioPause();
}

void audio_play_resume(void)
{
	EspAudioResume();
}

bool is_audio_playing(void)
{
	PlayerStatus status = {0};
	
	if (EspAudioStatusGet(&status) == ESP_OK)
	{
		if (status.mode == PLAYER_WORKING_MODE_MUSIC
			&& status.status == AUDIO_STATUS_PLAYING)
		{
			return true;	
		}
	}

	return false;
}


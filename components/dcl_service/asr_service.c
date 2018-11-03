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

#include "asr_service.h"
#include "cJSON.h"
#include "userconfig.h"
#include "device_params_manage.h"
#include "dcl_tts_api.h"

#define LOG_TAG "asr service" 

static ASR_SERVICE_HANDLE_t *g_asr_service_handle = NULL;

//获取文本播放URL
bool get_tts_play_url(
	const char* const input_text, 
	char* const tts_url, 
	const uint32_t url_len)
{
	DCL_AUTH_PARAMS_t input_params = {0};

	snprintf(input_params.str_app_id, sizeof(input_params.str_app_id), "%s", DEEP_BRAIN_APP_ID);
	snprintf(input_params.str_robot_id, sizeof(input_params.str_robot_id), "%s", DEEP_BRAIN_ROBOT_ID);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, &input_params.str_device_id);
	
	if (dcl_get_tts_url(&input_params, input_text, tts_url, url_len) == DCL_ERROR_CODE_OK)
	{
		//DEBUG_LOGI(TAG_LOG, "ttsurl:[%s]", tts_url);
		return true;
	}
	else
	{
		//DEBUG_LOGE(TAG_LOG, "dcl_get_tts_url failed");
		return false;
	}
}

/**
 * @brief  add tail to pcm queue 
 * @param  [out]pcm_obj
 * @return app framework errno
 */
static APP_FRAMEWORK_ERRNO_t pcm_queue_add_tail(
	ASR_PCM_OBJECT_t *pcm_obj)
{
	if (g_asr_service_handle == NULL || pcm_obj == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_asr_service_handle->mutex_lock);
	TAILQ_INSERT_TAIL(&g_asr_service_handle->pcm_queue, pcm_obj, next);	
	SEMPHR_TRY_UNLOCK(g_asr_service_handle->mutex_lock);

	return APP_FRAMEWORK_ERRNO_OK;
}

/**
 * @brief  add tail to pcm queue firstly
 * @param  [out]pcm_obj
 * @return app framework errno
 */
static APP_FRAMEWORK_ERRNO_t pcm_queue_first(
	ASR_PCM_OBJECT_t **pcm_obj)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	if (g_asr_service_handle == NULL || pcm_obj == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_asr_service_handle->mutex_lock);
	*pcm_obj = TAILQ_FIRST(&g_asr_service_handle->pcm_queue);
	if (*pcm_obj == NULL)
	{
		err = APP_FRAMEWORK_ERRNO_FAIL;
	}
	SEMPHR_TRY_UNLOCK(g_asr_service_handle->mutex_lock);

	return err;
}

/**
 * @brief  remove pcm queue 
 * @param  [out]pcm_obj
 * @return app framework errno
 */
static APP_FRAMEWORK_ERRNO_t pcm_queue_remove(
	ASR_PCM_OBJECT_t *pcm_obj)
{
	if (g_asr_service_handle == NULL || pcm_obj == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_asr_service_handle->mutex_lock);
	TAILQ_REMOVE(&g_asr_service_handle->pcm_queue, pcm_obj, next);
	SEMPHR_TRY_UNLOCK(g_asr_service_handle->mutex_lock);

	return APP_FRAMEWORK_ERRNO_OK;
}

/**
 * @brief  asr rec begin
 * @param  [in]  asr_service_handle
 * @param  [out] pcm_obj
 * @return none
 */
static void asr_rec_begin(
	ASR_SERVICE_HANDLE_t *asr_service_handle, 
	ASR_PCM_OBJECT_t *pcm_obj)
{
	ASR_RESULT_t *asr_result = &asr_service_handle->asr_result;
	DCL_AUTH_PARAMS_t *auth_params = &asr_service_handle->auth_params;
	memset(asr_result, 0, sizeof(ASR_RESULT_t));
	asr_result->record_sn = pcm_obj->record_sn;
	
	switch (pcm_obj->asr_engine)
	{
		case ASR_ENGINE_TYPE_DP_ENGINE:
		{
			DCL_ERROR_CODE_t ret = DCL_ERROR_CODE_OK;

			if (asr_service_handle->dcl_asr_handle != NULL)
			{
				dcl_asr_session_end(asr_service_handle->dcl_asr_handle);
				asr_service_handle->dcl_asr_handle = NULL;
			}

			ret = dcl_asr_session_begin(&asr_service_handle->dcl_asr_handle);
			if (ret != DCL_ERROR_CODE_OK)
			{
				ret = dcl_asr_session_begin(&asr_service_handle->dcl_asr_handle);
			}
			
			if (ret == DCL_ERROR_CODE_OK)
			{
				DEBUG_LOGI(LOG_TAG, "dcl_asr_session_begin success"); 
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "dcl_asr_session_begin failed errno[%d]", ret); 
				asr_result->error_code = ret;
				break;
			}
			
			get_dcl_auth_params(auth_params);
			dcl_asr_set_param(asr_service_handle->dcl_asr_handle, DCL_ASR_PARAMS_INDEX_LANGUAGE, &pcm_obj->asr_lang, sizeof(pcm_obj->asr_lang));
			dcl_asr_set_param(asr_service_handle->dcl_asr_handle, DCL_ASR_PARAMS_INDEX_MODE, &pcm_obj->asr_mode, sizeof(pcm_obj->asr_mode));
			dcl_asr_set_param(asr_service_handle->dcl_asr_handle, DCL_ASR_PARAMS_INDEX_AUTH_PARAMS, auth_params, sizeof(DCL_AUTH_PARAMS_t));
			break;
		}
		default:
			break;
	}
}

/**
 * @brief  asr rec runing
 * @param  [in]  asr_service_handle
 * @param  [out] pcm_obj
 * @return none
 */
static void asr_rec_runing(
	ASR_SERVICE_HANDLE_t *asr_service_handle, 
	ASR_PCM_OBJECT_t *pcm_obj)
{
	int ret = 0;
	int i = 0;
	ASR_RESULT_t *asr_result = &asr_service_handle->asr_result;
	
	switch (pcm_obj->asr_engine)
	{
		case ASR_ENGINE_TYPE_DP_ENGINE:
		{
			if (asr_service_handle->dcl_asr_handle == NULL)
			{
				break;
			}
			
			ret = dcl_asr_audio_write(asr_service_handle->dcl_asr_handle, pcm_obj->record_data, pcm_obj->record_len);
			if (ret != DCL_ERROR_CODE_OK)
			{
				asr_result->error_code = ret;
				dcl_asr_session_end(asr_service_handle->dcl_asr_handle);
				asr_service_handle->dcl_asr_handle = NULL;
				DEBUG_LOGE(LOG_TAG, "dcl_asr_audio_write failed errno[%d]", ret); 
			}
			break;
		}
		default:
			break;
	}
}

/**
 * @brief  asr rec end
 * @param  [in]  asr_service_handle
 * @param  [out] pcm_obj
 * @return none
 */
static void asr_rec_end(
	ASR_SERVICE_HANDLE_t *asr_service_handle, 
	ASR_PCM_OBJECT_t *pcm_obj)
{
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t asr_cost = 0;
	uint64_t start_time = get_time_of_day();
	ASR_RESULT_t *asr_result = &asr_service_handle->asr_result;
	
	switch (pcm_obj->asr_engine)
	{
		case ASR_ENGINE_TYPE_DP_ENGINE:
		{
			if (asr_service_handle->dcl_asr_handle == NULL)
			{
				break;
			}

			err_code = dcl_asr_get_result(asr_service_handle->dcl_asr_handle, asr_result->str_result, sizeof(asr_result->str_result));
			if (err_code != DCL_ERROR_CODE_OK)
			{
				DEBUG_LOGE(LOG_TAG, "dcl_asr_get_result failed");
			}
			asr_result->error_code = err_code;
			asr_cost = get_time_of_day() - pcm_obj->time_stamp;
			dcl_asr_session_end(asr_service_handle->dcl_asr_handle);
			asr_service_handle->dcl_asr_handle = NULL;
			break;
		}
		default:
			break;
	}
	
	if (pcm_obj->asr_mode == DCL_ASR_MODE_ASR)
	{
		DEBUG_LOGI(LOG_TAG, "asr result:record_sn[%d], error_code[%d], mode[%d], cost[%lldms], result[%s]", 
			asr_result->record_sn, asr_result->error_code, pcm_obj->asr_mode, asr_cost, asr_result->str_result);
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "asr result:record_sn[%d], error_code[%d], mode[%d], cost[%lldms]", 
			asr_result->record_sn, asr_result->error_code, pcm_obj->asr_mode, asr_cost);
	}

	if (pcm_obj->asr_call_back != NULL)
	{
		pcm_obj->asr_call_back(asr_result);	
	}
}

/**
 * @brief  asr service process
 * @param  [out] pcm_obj
 * @return none
 */
static void asr_service_process(ASR_PCM_OBJECT_t *pcm_obj)
{
	ASR_SERVICE_HANDLE_t *asr_service_handle = g_asr_service_handle;
	ASR_RESULT_t *asr_result = &asr_service_handle->asr_result;

	if (asr_service_handle == NULL
		|| pcm_obj == NULL)
	{
		return;
	}
					
	switch (pcm_obj->msg_type)
	{
		case ASR_SERVICE_RECORD_START:
		{
			asr_rec_begin(asr_service_handle, pcm_obj);
		}
		case ASR_SERVICE_RECORD_READ:
		{
			asr_rec_runing(asr_service_handle, pcm_obj);
			break;
		}
		case ASR_SERVICE_RECORD_STOP:
		{
			//语音小于1秒，不做语音识别
			if (pcm_obj->record_ms < ASR_MIN_AUDIO_MS)
			{
				dcl_asr_session_end(asr_service_handle->dcl_asr_handle);
				asr_service_handle->dcl_asr_handle = NULL;
				asr_result->error_code = DCL_ERROR_CODE_ASR_SHORT_AUDIO;
				
				if (pcm_obj->asr_call_back != NULL)
				{
					pcm_obj->asr_call_back(asr_result);	
				}
				break;
			}
			
			//语音识别处理
			asr_rec_end(asr_service_handle, pcm_obj);
			
			break;
		}
		default:
			break;
	}
}

static void asr_service_destroy(void)
{
	if (g_asr_service_handle == NULL)
	{
		return;
	}

	if (g_asr_service_handle->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_asr_service_handle->mutex_lock);
		g_asr_service_handle->mutex_lock = NULL;
	}
	
	memory_free(g_asr_service_handle);
	g_asr_service_handle = NULL;
}

static void asr_service_event_callback(void *app, APP_EVENT_MSG_t *msg)
{
	ASR_PCM_OBJECT_t *pcm_obj = NULL;

	switch (msg->event)
	{
		case APP_EVENT_ASR_SERVICE_NEW_REQUEST:
		{
			if (pcm_queue_first(&pcm_obj) != APP_FRAMEWORK_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "chat_queue_first failed");
				break;
			}

			if (pcm_queue_remove(pcm_obj) != APP_FRAMEWORK_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "chat_queue_remove failed");
				break;
			}

			//语音数据处理
			asr_service_process(pcm_obj);

			//释放数据内容
			asr_service_del_asr_object(pcm_obj);
			pcm_obj = NULL;
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

void task_asr_service(void *pv)
{
	ASR_SERVICE_HANDLE_t *asr_service_handle = (ASR_SERVICE_HANDLE_t *)pv;
		
	APP_OBJECT_t *app = app_new(APP_NAME_ASR_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_ASR_SERVICE);
		asr_service_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_add_event(app, APP_EVENT_DEFAULT_BASE, asr_service_event_callback);
		app_add_event(app, APP_EVENT_ASR_SERVICE_BASE, asr_service_event_callback);
		DEBUG_LOGE(LOG_TAG, "%s create success", APP_NAME_ASR_SERVICE);
	}

	app_msg_dispatch(app);
	
	app_delete(app);	

	asr_service_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t asr_service_create(int task_priority)
{
	if (g_asr_service_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_asr_service_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	g_asr_service_handle = (ASR_SERVICE_HANDLE_t *)memory_malloc(sizeof(ASR_SERVICE_HANDLE_t));
	if (g_asr_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_asr_service_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset((char*)g_asr_service_handle, 0, sizeof(ASR_SERVICE_HANDLE_t));
	TAILQ_INIT(&g_asr_service_handle->pcm_queue);

	//初始化参数
	SEMPHR_CREATE_LOCK(g_asr_service_handle->mutex_lock);
	if (g_asr_service_handle->mutex_lock == NULL)
	{
		asr_service_destroy();
		DEBUG_LOGE(LOG_TAG, "g_asr_service_handle->mutex_lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	//创建线程
    if (!task_thread_create(task_asr_service,
            "task_asr_service",
            APP_NAME_ASR_SERVICE_STACK_SIZE,
            g_asr_service_handle,
            task_priority)) 
    {
			DEBUG_LOGE(LOG_TAG, "create task_asr_service failed");
			asr_service_destroy();
			return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t asr_service_delete(void)
{
	return app_send_message(APP_NAME_ASR_SERVICE, APP_NAME_ASR_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t asr_service_send_request(ASR_PCM_OBJECT_t *pcm_obj)
{
	static int record_sn = 0; 
		
	if (pcm_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "pcm_obj is NULL");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (g_asr_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_asr_service_handle is NULL");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	SEMPHR_TRY_LOCK(g_asr_service_handle->mutex_lock);
	if (pcm_obj->record_id == 1)
	{
		pcm_obj->msg_type = ASR_SERVICE_RECORD_START;
		record_sn = pcm_obj->record_sn;
	}
	else if (pcm_obj->record_id > 1)
	{
		pcm_obj->msg_type = ASR_SERVICE_RECORD_READ;
	}
	else
	{
		pcm_obj->msg_type = ASR_SERVICE_RECORD_STOP;
	}
	SEMPHR_TRY_UNLOCK(g_asr_service_handle->mutex_lock);

	if (record_sn != pcm_obj->record_sn)
	{
		DEBUG_LOGE(LOG_TAG, "record_sn is different:[%d][%d]", 
			record_sn, pcm_obj->record_sn);
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	if (pcm_queue_add_tail(pcm_obj) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "pcm_queue_add_tail failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	return app_send_message(APP_NAME_ASR_SERVICE, APP_NAME_ASR_SERVICE, APP_EVENT_ASR_SERVICE_NEW_REQUEST, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t asr_service_new_asr_object(ASR_PCM_OBJECT_t **pcm_obj)
{
	ASR_PCM_OBJECT_t *asr_obj = NULL;
		
	asr_obj = (ASR_PCM_OBJECT_t *)memory_malloc(sizeof(ASR_PCM_OBJECT_t));
	if (asr_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc asr_obj failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	memset(asr_obj, 0, sizeof(ASR_PCM_OBJECT_t));
	*pcm_obj = asr_obj;
	
	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t asr_service_del_asr_object(ASR_PCM_OBJECT_t *pcm_obj)
{
	if (pcm_obj != NULL)
	{
		memory_free(pcm_obj);
		pcm_obj = NULL;
	}
	else
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}


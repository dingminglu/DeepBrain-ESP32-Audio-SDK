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

#ifndef ASR_SERVICE_H
#define ASR_SERVICE_H

#include <stdio.h>
#include "app_config.h"
#include "dcl_service.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define ASR_MIN_AUDIO_MS 1000

/* asr service running handle */
typedef struct ASR_SERVICE_HANDLE_t
{
	//asr result
	ASR_RESULT_t asr_result;
	
	//dcl asr handle
	void *dcl_asr_handle;

	//pcm data queue
	TAILQ_HEAD(ASR_PCM_QUEUE_t, ASR_PCM_OBJECT_t) pcm_queue;

	//auth params
	DCL_AUTH_PARAMS_t auth_params;

	//mutex lock
	void *mutex_lock;
}ASR_SERVICE_HANDLE_t;

/**
 * @brief  Get text play url
 * @param  [in]  input_text
 * @param  [out] tts_url
 * @param  [in]  url_len
 * @return true/false
 */
bool get_tts_play_url(
	const char* const input_text, 
	char* const tts_url, 
	const uint32_t url_len);

/**
 * @brief  Asr service create
 * @param  task_priority, priority
 * @return See in APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t asr_service_create(int task_priority);

/**
 * @brief  Asr service delete
 * @return See in APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t asr_service_delete(void);

/**
 * @brief Asr service send request
 * @param pcm_obj
 * @return See in APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t asr_service_send_request(ASR_PCM_OBJECT_t *pcm_obj);

APP_FRAMEWORK_ERRNO_t asr_service_new_asr_object(ASR_PCM_OBJECT_t **pcm_obj);

APP_FRAMEWORK_ERRNO_t asr_service_del_asr_object(ASR_PCM_OBJECT_t *pcm_obj);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif


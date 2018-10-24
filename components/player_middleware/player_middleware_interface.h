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

#ifndef PLAYER_MIDDLEWARE_INTERFACE_H
#define PLAYER_MIDDLEWARE_INTERFACE_H

#include "ctypes_interface.h"
#include "userconfig.h"
#include "array_music.h"
#include "AudioDef.h"

#ifdef __cplusplus
extern "C" {
#endif

//audio mode
typedef enum
{
	AUDIO_MODE_TONE = 0,
	AUDIO_MODE_MUSIC,
}AUDIO_MODE_t;

/* player middleware params index */
typedef enum PLAYER_MIDDLEWARE_PARAM_INDEX_t
{
	PLAYER_MIDDLEWARE_PARAM_INDEX_SERVICE_HANDLE = 0,
}PLAYER_MIDDLEWARE_PARAM_INDEX_t;

/* audio player error code */
typedef enum AUDIO_PLAY_ERROR_CODE_t
{
	AUDIO_PLAY_ERROR_CODE_OK = 0,
	AUDIO_PLAY_ERROR_CODE_FAIL,
	AUDIO_PLAY_ERROR_CODE_INVALID_PARAMS,
	AUDIO_PLAY_ERROR_CODE_MALLOC_FAILED,
}AUDIO_PLAY_ERROR_CODE_t;

/* audio play status */
typedef enum AUDIO_PLAY_STATUS_t 
{
    AUDIO_PLAY_STATUS_UNKNOWN = 0,
    AUDIO_PLAY_STATUS_PLAYING = 1,
    AUDIO_PLAY_STATUS_PAUSED = 2,
    AUDIO_PLAY_STATUS_FINISHED = 3,
    AUDIO_PLAY_STATUS_STOP = 4,
    AUDIO_PLAY_STATUS_ERROR = 5,
    AUDIO_PLAY_STATUS_AUX_IN = 6,
} AUDIO_PLAY_STATUS_t;  /* Effective audio playing status */

//audio play events
typedef struct
{
	AUDIO_PLAY_STATUS_t play_status;
	uint8_t music_url[2048];
}AUDIO_PLAY_EVENTS_T;

typedef void (*audio_play_event_notify)(const AUDIO_PLAY_EVENTS_T *event);

typedef enum AUDIO_TERMINATION_TYPE_t 
{
    AUDIO_TERM_TYPE_NOW = 0,  //terminate the music immediately
    AUDIO_TERM_TYPE_DONE = 1, //the current music will not be terminated untill it is finished
    AUDIO_TERM_TYPE_MAX,
}AUDIO_TERMINATION_TYPE_t;
 
/**
 * set parameters for player middleware 
 *
 * @param index,handle
 * @param param,parameter
 * @param param_len,parameter length
 * @return true/false
 */
bool player_middleware_set_param(
	const PLAYER_MIDDLEWARE_PARAM_INDEX_t index,
	const void* param,
	const uint32_t param_len);

/**
 * play tone audio
 *
 * @param audio_url,audio url
 * @param term_type,audio termination type
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_tone(
	const char* const audio_url,
	const AUDIO_TERMINATION_TYPE_t term_type);

/**
 * play tone audio callback 
 *
 * @param audio_url,audio url
 * @param term_type,audio termination type
 * @param cb,callback function that returns audio play status
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_tone_with_cb(
	const char* const audio_url,
	const AUDIO_TERMINATION_TYPE_t term_type,
	const audio_play_event_notify cb);

/**
 * play music audio
 *
 * @param audio_url,audio url
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_music(const char* const audio_url);

/**
 * play music audio callback 
 *
 * @param audio_url,audio url
 * @param cb,callback function that returns audio play status
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_music_with_cb(
	const char* const audio_url,
	const audio_play_event_notify cb);

/**
 * play flash tone
 *
 * @param flash music index
 * @param term_type,audio termination type
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_tone_mem(
	const int flash_music_index, 
	const AUDIO_TERMINATION_TYPE_t term_type);

/**
 * play flash tone callback
 *
 * @param flash music index
 * @param term_type,audio termination type
 * @param cb,callback function that returns audio play status
 * @return audio play error code
 */
AUDIO_PLAY_ERROR_CODE_t audio_play_tone_mem_with_cb(
	const int flash_music_index, 
	const AUDIO_TERMINATION_TYPE_t term_type, 
	const audio_play_event_notify cb);

AUDIO_PLAY_ERROR_CODE_t audio_play_tts(const char* text);

void audio_play_stop(void);

void audio_play_pause(void);

void audio_play_resume(void);

bool is_audio_playing(void);

/**
 * create player middlerware service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t player_middleware_create(const uint32_t task_priority);

/**
 * player middlerware service delete
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t player_middleware_delete(void);

#ifdef __cplusplus
}
#endif

#endif  //PLAYER_MIDDLEWARE_INTERFACE_H


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

#ifndef PLAY_LIST_H
#define PLAY_LIST_H

#include <stdio.h>

#include "app_config.h"
#include "player_middleware_interface.h"

#define MAX_MUSIC_URL_LEN 2048

//列表播放模式
typedef enum 
{
	PLAY_LIST_MODE_SINGLE,		//单曲
	PLAY_LIST_MODE_SINGLE_LOOP,	//单曲循环
	PLAY_LIST_MODE_ALL,		 	//全部
	PLAY_LIST_MODE_ALL_LOOP,	//全部循环
	PLAY_LIST_MODE_ALL_RAND,	//全部随机
}PLAY_LIST_MODE_t;

//播放项目
typedef struct PLAY_LIST_ITEM_t
{
	//tail queue entry
	TAILQ_ENTRY(PLAY_LIST_ITEM_t) next;
	//music url
	char music_url[MAX_MUSIC_URL_LEN];
	//playing status
	int is_played;
	audio_play_event_notify call_back;
}PLAY_LIST_ITEM_t;

typedef void (*play_prev_cb)(void);
typedef void (*play_next_cb)(void);

//播放列表回调
typedef struct PLAYLIST_CALL_BACKS_t
{
	play_prev_cb prev;
	play_next_cb next;
}PLAYLIST_CALL_BACKS_t;

//播放列表
typedef struct PLAY_LIST_t
{
	uint32_t list_item_num;
	PLAY_LIST_MODE_t mode;
	PLAY_LIST_ITEM_t *cur_item;
	PLAYLIST_CALL_BACKS_t call_back;
	TAILQ_HEAD(PLAY_LIST_QUEUE_t, PLAY_LIST_ITEM_t) play_list_queue;	
}PLAY_LIST_t;

//播放列表句柄
typedef struct PLAY_LIST_HANDLE_t
{
	PLAY_LIST_t *playlist;	
}PLAY_LIST_HANDLE_t;

/**
 * create a new playlist
 *
 * @param playlist
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t new_playlist(void **playlist);

/**
 * delete playlist
 *
 * @param playlist
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t del_playlist(void *playlist);

/**
 * set playlist mode
 *
 * @param playlist
 * @param mode,play list mode
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t set_playlist_mode(
	void *playlist, PLAY_LIST_MODE_t mode);

/**
 * set playlist callback
 *
 * @param playlist
 * @param call_back,callback function that returns platlist
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t set_playlist_callback(
	void *playlist, PLAYLIST_CALL_BACKS_t *call_back);

/**
 * add playlist item
 *
 * @param playlist
 * @param url,audio url
 * @param cb,callback function that returns audio play status
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t add_playlist_item(
	void *playlist,
	const char *url,
	const audio_play_event_notify cb);

/**
 * create playlist service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_service_create(int task_priority);

/**
 * delete playlist service
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_service_delete(void);

/**
 * add playlist
 *
 * @param playlist
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_add(void **playlist);

/**
 * The prevent one on the playlist
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_prev(void);

/**
 * The next one on the playlist
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_next(void);

/**
 * clear playlist
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t playlist_clear(void);

/**
 * test playlist
 *
 * @param none
 * @return none
 */
void test_playlist(void);

#endif


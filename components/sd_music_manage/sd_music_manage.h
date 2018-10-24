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

#ifndef SD_MUSIC_MANAGE_H
#define SD_MUSIC_MANAGE_H

#include "userconfig.h"

/*
 * SD卡音乐播放模块：
 * 1、不支持热插拔
 *
 */

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//音乐资源类型
typedef enum FOLDER_INDEX_T
{
	FOLDER_INDEX_START = -1,
	FOLDER_INDEX_BAIKE = 0,
	FOLDER_INDEX_ERGE,
	FOLDER_INDEX_GUOXUE,
	FOLDER_INDEX_GUSHI,
	FOLDER_INDEX_KETANG,
	FOLDER_INDEX_QINZI,
	FOLDER_INDEX_XIAZAI,
	FOLDER_INDEX_ENGLISH,
	FOLDER_INDEX_MUSIC,
	FOLDER_INDEX_END
}FOLDER_INDEX_T;

//SD音乐返回情况
typedef enum SD_MUSIC_ERRNO_t
{
	SD_MUSIC_ERRNO_SUCCESS = 0,//成功
	SD_MUSIC_ERRNO_HANDLE_NULL,//句柄为空
	SD_MUSIC_ERRNO_OPEN_DIR_FAIL,//打开文件夹失败
	SD_MUSIC_ERRNO_READ_DIR_FAIL,//读取文件夹失败
	SD_MUSIC_ERRNO_GET_URL_FAIL,//获取URL失败
}SD_MUSIC_ERRNO_t;

//查找sd文件夹列表的方向
typedef enum SEARCHING_DIRECTION_t
{
	SEARCHING_DIRECTION_PREV,		//向上搜寻
	SEARCHING_DIRECTION_NEXT,		//向下搜寻
}SEARCHING_DIRECTION_t;

//operation of sd_music mode
typedef enum SD_MUSIC_OPT_MODE_t
{
	SD_MUSIC_OPT_MODE_START,	//启动播放
	SD_MUSIC_OPT_MODE_PREV,		//上一首
	SD_MUSIC_OPT_MODE_NEXT,		//下一首
}SD_MUSIC_OPT_MODE_t;

/**
 * Play sd music.
 */
void sd_music_play_start();

/**
 * Create sd music momory.
 *
 * @param [in]task_priority.
 * @return in APP FRAMEWORK various errors returned.
 */
APP_FRAMEWORK_ERRNO_t sd_music_manage_create(const int task_priority);

/**
 * Exit sd music event.
 *
 * @param void.
 * @return in APP FRAMEWORK various errors returned.
 */
APP_FRAMEWORK_ERRNO_t sd_music_manage_delete(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* SD_MUSIC_MANAGE_H */


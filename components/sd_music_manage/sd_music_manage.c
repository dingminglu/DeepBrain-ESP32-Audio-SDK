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
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "ctypes_interface.h"
#include "memory_interface.h"
#include "play_list.h"
#include "sd_music_manage.h"
#include "sdcard_interface.h"

#define LOG_TAG					"sd music manage"
#define SD_MUSIC_STRUCT_VERSION	0X20181011
#define FOLDER_MAX_NUM 			9
#define FAT32_MAX_FILE_SIZE		65534
#define OFFSET_INITIAL_VALUE	0

typedef struct
{
	unsigned char str_name[16];			
}FOLDER_NAME_T;

//total 128 bytes
typedef struct
{
	int64_t file_offset;		//文件在文件夹中的偏移量	
	char res[120];				//剩余可用
}SD_FILE_UNIT_t;

//total 64 bytes
typedef struct
{
	uint32_t version_info;					//结构体版本信息
	uint32_t folder_total_num;				//规定的文件夹总数
	int32_t need_play_folder_index_num;		//需要播放的文件夹号
	char res[52];							//剩余可用
}SD_PLAY_LIST_HEADER_t;

typedef struct
{
	SD_PLAY_LIST_HEADER_t play_list_header;			//文件夹播放头
	SD_FILE_UNIT_t folder_index[FOLDER_MAX_NUM];	//文件夹索引范围
}SD_PLAY_LIST_INFO_t;

//total size 2*1024 bytes,must 4 bytes align
typedef struct SD_MUSIC_MANAGE_HANDLE_t
{
	//SD_MUSIC_OPT_MODE_t opt_mode;				//操作模式
	SD_PLAY_LIST_INFO_t play_list_info;			//播放列表信息
	char music_url[280];						//music url
	char dir_path[32];							//dir path
	struct dirent *p_dir_node;
	DIR *p_dir;
	void *set_info_lock;						//保存到sd卡的信息锁
	void *sd_play_lock;							//播放锁
	char res[548];								//剩余可用
}SD_MUSIC_MANAGE_HANDLE_t;

static const FOLDER_NAME_T g_folder_list[] = 
{
	{{0xB0,0xD9,0xBF,0xC6}},//百科
	{{0xB6,0xF9,0xB8,0xE8}},//儿歌
	{{0xB9,0xFA,0xD1,0xA7}},//国学
	{{0xB9,0xCA,0xCA,0xC2}},//故事
	{{0xBF,0xCE,0xCC,0xC3}},//课堂
	{{0xC7,0xD7,0xD7,0xD3}},//亲子
	{{0xCF,0xC2,0xD4,0xD8}},//下载
	{{0xD3,0xA2,0xD3,0xEF}},//英语
	{{0xD2,0xF4,0xC0,0xD6}},//音乐
};

#define SD_FOLDER_LIST_SIZE (sizeof(g_folder_list)/sizeof(FOLDER_NAME_T))

static SD_MUSIC_MANAGE_HANDLE_t *g_sd_music_handle = NULL;

static void sd_music_prev();
static void sd_music_next();
static bool sd_music_get_music_url(SD_MUSIC_MANAGE_HANDLE_t *handle, SEARCHING_DIRECTION_t direction);
static void sd_music_switching_audio(SD_MUSIC_OPT_MODE_t opt_mode);

//获取SD卡文件夹名称
static unsigned char * get_sd_folder_name(int _index)
{
	return &g_folder_list[_index];
}

//判断文件格式是否有效
static bool is_valid_music_name(const char *_music_name)
{
	if (_music_name == NULL)
	{
		return false;
	}
	
	char str_name[256] = {0};

	snprintf(str_name, 256, "%s", _music_name);
	strlwr(str_name);
	if (strstr(str_name, ".m4a") != NULL
		|| strstr(str_name, ".flac") != NULL
		|| strstr(str_name, ".amr") != NULL
		|| strstr(str_name, ".mp3") != NULL)
	{
		return true;
	}
		
	return false;
}

//打印播放列表信息
void print_sdcard_dir_record_info(SD_PLAY_LIST_INFO_t *sd_list_info)
{
	DEBUG_LOGI(LOG_TAG, "\n%s\n*** version is [0X%0X]\n*** folder_total_num = [%d]\n*** playing folder index at NO.[%d], playing file_offset at NO.[%lld]\n%s",
		">>>>>>>> sd card music_list_info >>>>>>>",
		sd_list_info->play_list_header.version_info,
		sd_list_info->play_list_header.folder_total_num,
		sd_list_info->play_list_header.need_play_folder_index_num,
		sd_list_info->folder_index[sd_list_info->play_list_header.need_play_folder_index_num].file_offset,
		"<<<<<<<<<<<<<<<<<<<<<<<<<");
}

//检查播放列表头信息是否变更
static bool sd_music_check_play_list_header(SD_PLAY_LIST_INFO_t *sd_list_info)
{
	if (sd_list_info == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "sd_music_list_info is NULL");
		return false;
	}
			
	if (sd_list_info->play_list_header.version_info ==  SD_MUSIC_STRUCT_VERSION 
		&& sd_list_info->play_list_header.folder_total_num == FOLDER_MAX_NUM)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//在SD中建立play_log.bin文件，保存播放记录
static void sd_music_save_list_to_sd(SD_PLAY_LIST_INFO_t *sd_list_info)
{
	FILE *file = NULL;

	if (g_sd_music_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_save_list_to_sd g_sd_music_handle is NULL!");
		return;
	}
	
	SEMPHR_TRY_LOCK(g_sd_music_handle->set_info_lock);//加保存信息锁
	file = fopen("/sdcard/play_log.bin", "wb");
	if (file == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_save_list_to_sd fopen failed");
		SEMPHR_TRY_UNLOCK(g_sd_music_handle->set_info_lock);//解保存信息锁
		return;
	}
	
	if (fwrite(sd_list_info, sizeof(SD_PLAY_LIST_INFO_t), 1, file) != 1)
	{	
		DEBUG_LOGE(LOG_TAG, "fwrite failed");
	}
	
	fclose(file);
	SEMPHR_TRY_UNLOCK(g_sd_music_handle->set_info_lock);//解保存信息锁
}

//从SD中读取play_log.bin文件，获取播放记录
static bool sd_music_get_list_from_sd(SD_PLAY_LIST_INFO_t *sd_list_info)
{
	FILE *file = NULL;
	bool ret = true;

	if (g_sd_music_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in get_music_list_info_from_sd g_sd_music_handle is NULL!");
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_sd_music_handle->set_info_lock);//加保存信息锁
	file = fopen("/sdcard/play_log.bin", "rb");
	if (file == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in get_music_list_info_from_sd fopen failed");
		SEMPHR_TRY_UNLOCK(g_sd_music_handle->set_info_lock);//解保存信息锁
		return false;
	}
	
	if (fread(sd_list_info, sizeof(SD_PLAY_LIST_INFO_t), 1, file) == 1)
	{
		if (!sd_music_check_play_list_header(sd_list_info))
		{
			//重置存储信息
			memset(sd_list_info, 0, sizeof(SD_PLAY_LIST_INFO_t));
			sd_list_info->play_list_header.version_info = SD_MUSIC_STRUCT_VERSION;
			sd_list_info->play_list_header.folder_total_num = FOLDER_MAX_NUM;
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "in get_music_list_info fread failed");
		ret = false;
	}
	
	fclose(file);
	SEMPHR_TRY_UNLOCK(g_sd_music_handle->set_info_lock);//解保存信息锁
	return ret;
}

//播放文件夹名称
static void play_folder_name(SD_PLAY_LIST_HEADER_t *play_list_header)
{	
	if (play_list_header->need_play_folder_index_num > FOLDER_INDEX_MUSIC
		|| play_list_header->need_play_folder_index_num < FOLDER_INDEX_BAIKE)	//检查获取的文件夹名称是否在范围内
	{
		play_list_header->need_play_folder_index_num = FOLDER_INDEX_BAIKE;
	}
		
	switch (play_list_header->need_play_folder_index_num)	//播放文件夹名称
	{
		case FOLDER_INDEX_BAIKE:
		{
			audio_play_tone_mem(FLASH_MUSIC_BAI_KE, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_ERGE:
		{
			audio_play_tone_mem(FLASH_MUSIC_ER_GE, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_GUOXUE:
		{
			audio_play_tone_mem(FLASH_MUSIC_GUO_XUE, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_GUSHI:
		{
			audio_play_tone_mem(FLASH_MUSIC_GU_SHI, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_KETANG:
		{
			audio_play_tone_mem(FLASH_MUSIC_KE_TANG, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_QINZI:
		{
			audio_play_tone_mem(FLASH_MUSIC_QIN_ZI, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_XIAZAI:
		{
			audio_play_tone_mem(FLASH_MUSIC_XIA_ZAI, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_ENGLISH:
		{
			audio_play_tone_mem(FLASH_MUSIC_YING_YU, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case FOLDER_INDEX_MUSIC:
		{
			audio_play_tone_mem(FLASH_MUSIC_YIN_YUE, AUDIO_TERM_TYPE_NOW);
			break;
		}
		default:
			break;
	}
	task_thread_sleep(2200);//使文件夹名称播完
}

//打开文件夹及相应错误处理
static bool sd_music_open_dir(SD_MUSIC_MANAGE_HANDLE_t *handle)
{
	if (handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_open_dir handle is NULL!");
		return false;
	}
	
	snprintf(handle->dir_path, sizeof(handle->dir_path), "/sdcard/%s", get_sd_folder_name(handle->play_list_info.play_list_header.need_play_folder_index_num));
	handle->p_dir = opendir(handle->dir_path);  //打开文件夹
	if (handle->p_dir == NULL)
	{
		//失败后，创建目录
		mkdir(handle->dir_path, S_IRWXU|S_IRWXG|S_IRWXO);
		handle->p_dir = opendir(handle->dir_path);  //打开文件夹
		if (handle->p_dir == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "open %s failed", handle->dir_path);
			return false;
		}
	}
	
	return true;
}

//获取音频URL
static bool sd_music_get_music_url(
	SD_MUSIC_MANAGE_HANDLE_t *handle, 
	SEARCHING_DIRECTION_t direction)
{
	int i = 0;
	int frequency = 1;
	
	if (handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_read_dir handle is NULL !");
		return false;
	}

	SD_PLAY_LIST_INFO_t *play_list_info = &handle->play_list_info;
	int32_t need_play_folder_index_num = handle->play_list_info.play_list_header.need_play_folder_index_num;

	if (play_list_info->folder_index[need_play_folder_index_num].file_offset < OFFSET_INITIAL_VALUE 
		|| play_list_info->folder_index[need_play_folder_index_num].file_offset > FAT32_MAX_FILE_SIZE)//判断偏移量是否在有效范围
	{
		play_list_info->folder_index[need_play_folder_index_num].file_offset = OFFSET_INITIAL_VALUE;
	}
	seekdir(handle->p_dir, play_list_info->folder_index[need_play_folder_index_num].file_offset);
	handle->p_dir_node = readdir(handle->p_dir);
	if (handle->p_dir_node == NULL)
	{//下一个文件为空时
		play_list_info->folder_index[need_play_folder_index_num].file_offset = OFFSET_INITIAL_VALUE;//使偏移回到文件夹开头
		seekdir(handle->p_dir, play_list_info->folder_index[need_play_folder_index_num].file_offset);
		handle->p_dir_node = readdir(handle->p_dir);
		if (handle->p_dir_node == NULL)
		{//文件开头为空，判定此文件夹为空
			return false;
		}
	}

	while (!is_valid_music_name(handle->p_dir_node->d_name))
	{//文件格式不正确时
		sd_music_get_music_url_p_dir_node_null:
		if (direction == SEARCHING_DIRECTION_PREV)
		{
			play_list_info->folder_index[need_play_folder_index_num].file_offset--;//检查上一个文件格式
		}
		else
		{
			play_list_info->folder_index[need_play_folder_index_num].file_offset++;//检查下一个文件格式
		}
		
		if (play_list_info->folder_index[need_play_folder_index_num].file_offset < OFFSET_INITIAL_VALUE 
			|| play_list_info->folder_index[need_play_folder_index_num].file_offset > FAT32_MAX_FILE_SIZE)//判断偏移量是否在有效范围
		{
			play_list_info->folder_index[need_play_folder_index_num].file_offset = OFFSET_INITIAL_VALUE;
		}
		seekdir(handle->p_dir, play_list_info->folder_index[need_play_folder_index_num].file_offset);
			
		handle->p_dir_node = readdir(handle->p_dir);
		if (handle->p_dir_node == NULL)
		{//文件为空时
			if (frequency >= 2)
			{//判定文件夹为空
				return false;
			}
			
			play_list_info->folder_index[need_play_folder_index_num].file_offset = -1;//使偏移回到文件夹开头（配合下一步操作）
			frequency++;
			goto sd_music_get_music_url_p_dir_node_null;
		}
		
		i++;//往后顺延20个文件
		if (i >= 20)
		{
			return false;
		}
	}

	snprintf(handle->music_url, sizeof(handle->music_url),
		"file:///sdcard/%s/%s", 
		get_sd_folder_name(play_list_info->play_list_header.need_play_folder_index_num),
		handle->p_dir_node->d_name);

	return true;
}

//创建播放列表并开始播放
static bool sd_music_creat_playlist(char *music_url)
{
	void *playlist = NULL;
	PLAYLIST_CALL_BACKS_t call_back = {sd_music_prev, sd_music_next};
	
	if (playlist_clear() != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "playlist_clear failed");
	}
	audio_play_stop();
	task_thread_sleep(100);
	
	if (new_playlist(&playlist) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "new_playlist failed");
		return false;
	}
	
	if (add_playlist_item(playlist, music_url, NULL) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "add_playlist_item failed");
		return false;
	}
	
	if (set_playlist_mode(playlist, PLAY_LIST_MODE_ALL) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "set_playlist_mode failed");
		return false;
	}
	
	if (set_playlist_callback(playlist, &call_back) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "set_playlist_callback failed");
		return false;
	}
	
	if (playlist_add(&playlist) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "playlist_add failed");
		return false;
	}
	
	return true;
}

//切换SD播放列表音频
static void sd_music_switching_audio(SD_MUSIC_OPT_MODE_t opt_mode)
{
	SD_MUSIC_MANAGE_HANDLE_t *handle = g_sd_music_handle;	
	SEARCHING_DIRECTION_t direction = SEARCHING_DIRECTION_NEXT;
	static int first_start_flag = 0;

	task_thread_sleep(150);
	if (is_audio_playing())
	{
		audio_play_pause();//停止音乐
		task_thread_sleep(200);
	}
	
	if (!is_sdcard_inserted())	//判断有无SD卡
	{
		audio_play_tone_mem(FLASH_MUSIC_NO_TF_CARD, AUDIO_TERM_TYPE_NOW);
		return;
	}

	if (handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_play_prev handle is NULL !");
		return;
	}

	SD_PLAY_LIST_INFO_t *play_list_info = &handle->play_list_info;
	int32_t need_play_folder_index_num = handle->play_list_info.play_list_header.need_play_folder_index_num;
	
	SEMPHR_TRY_LOCK(handle->sd_play_lock);//加播放锁
	sd_music_get_list_from_sd(play_list_info);//从sd卡获取记录信息

	if (opt_mode == SD_MUSIC_OPT_MODE_START)
	{
		if (first_start_flag == 1)//使开机后首次启动不做文件夹切换
		{
			play_list_info->play_list_header.need_play_folder_index_num++;//切到下个文件夹
		}
		
		play_folder_name(&play_list_info->play_list_header);//播放文件夹名
		first_start_flag = 1;
	}
	
	if (!sd_music_open_dir(handle))//打开文件夹系列处理
	{
		audio_play_tone_mem(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER, AUDIO_TERM_TYPE_NOW);
		goto sd_music_switching_audio_err_3;
	}

	switch (opt_mode)	//获取当前操作模式
	{
		case SD_MUSIC_OPT_MODE_PREV:
		{
			play_list_info->folder_index[need_play_folder_index_num].file_offset--;//切换到上一个文件
			direction = SEARCHING_DIRECTION_PREV;
			break;
		}
		case SD_MUSIC_OPT_MODE_NEXT:
		{
			play_list_info->folder_index[need_play_folder_index_num].file_offset++;//切换到下一个文件
			direction = SEARCHING_DIRECTION_NEXT;
			break;
		}
		case SD_MUSIC_OPT_MODE_START:
		{
			direction = SEARCHING_DIRECTION_NEXT;
			break;
		}
		default:
			break;
	}

	if(!sd_music_get_music_url(handle, direction))//获取音频URL
	{
		audio_play_tone_mem(FLASH_MUSIC_NO_MUSIC, AUDIO_TERM_TYPE_NOW);
		goto sd_music_switching_audio_err_1;
	}

	if (!sd_music_creat_playlist(handle->music_url))//添加播放列表并播放
	{
		if (!sd_music_creat_playlist(handle->music_url))
		{
			audio_play_tone_mem(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER, AUDIO_TERM_TYPE_NOW);
			goto sd_music_switching_audio_err_2;
		}
	}

	sd_music_switching_audio_err_1:
	play_list_info->play_list_header.version_info = SD_MUSIC_STRUCT_VERSION;
	play_list_info->play_list_header.folder_total_num = FOLDER_MAX_NUM;
	sd_music_save_list_to_sd(play_list_info);//保存播放记录
	
	sd_music_switching_audio_err_2:
	closedir(handle->p_dir);//关闭文件夹
	
	sd_music_switching_audio_err_3:
	print_sdcard_dir_record_info(play_list_info);//打印播放列表信息
	SEMPHR_TRY_UNLOCK(handle->sd_play_lock);//解播放锁
}

//上一首操作
static void sd_music_prev()
{
	sd_music_switching_audio(SD_MUSIC_OPT_MODE_PREV);
}

//下一首操作
static void sd_music_next()
{
	sd_music_switching_audio(SD_MUSIC_OPT_MODE_NEXT);
}

//sd音乐播放启动命令
void sd_music_play_start()
{
	app_send_message(APP_NAME_SD_MUSIC_MANAGE, APP_NAME_SD_MUSIC_MANAGE, APP_EVENT_SD_CARD_MUSIC_PLAY, NULL, 0);
}

static void sd_music_manage_destroy(void)
{
	if (g_sd_music_handle == NULL)
	{
		return;
	}
	
	memory_free(g_sd_music_handle);
	g_sd_music_handle = NULL;
}

static void sd_music_event_callback(
	void *app, 
	APP_EVENT_MSG_t *msg)
{
	if (g_sd_music_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "in sd_music_event_callback g_sd_music_handle is NULL !");
		return;
	}
	
	switch (msg->event)
	{
		case APP_EVENT_SD_CARD_MUSIC_PLAY:
		{
			sd_music_switching_audio(SD_MUSIC_OPT_MODE_START);
			break;
		}
		case APP_EVENT_SD_CARD_MUSIC_PREV:
		{
			sd_music_prev();
			break;
		}
		case APP_EVENT_SD_CARD_MUSIC_NEXT:
		{
			sd_music_next();
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
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

static void task_sd_music_manage(void *pv)
{	
	APP_OBJECT_t *app = app_new(APP_NAME_SD_MUSIC_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_SD_MUSIC_MANAGE);
		sd_music_manage_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		//app_set_loop_timeout(app, 1000, sd_music_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, sd_music_event_callback);
		app_add_event(app, APP_EVENT_SDCARD_BASE, sd_music_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_SD_MUSIC_MANAGE);
	}

	app_msg_dispatch(app);
	
	app_delete(app);	

	sd_music_manage_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t sd_music_manage_create(const int task_priority)
{
	if (g_sd_music_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_sd_music_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//申请运行句柄
    g_sd_music_handle = (SD_MUSIC_MANAGE_HANDLE_t *)memory_malloc(sizeof(SD_MUSIC_MANAGE_HANDLE_t));
	if (g_sd_music_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc g_sd_music_handle failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//初始化参数
	memset(g_sd_music_handle, 0, sizeof(SD_MUSIC_MANAGE_HANDLE_t));

	//初始化SD卡
	sd_card_init();

	//初始化锁
	SEMPHR_CREATE_LOCK(g_sd_music_handle->set_info_lock);
	if (g_sd_music_handle->set_info_lock == NULL)
	{
		sd_music_manage_destroy();
		DEBUG_LOGE(LOG_TAG, "SEMPHR_CREATE_LOCK failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	SEMPHR_CREATE_LOCK(g_sd_music_handle->sd_play_lock);
	if (g_sd_music_handle->sd_play_lock == NULL)
	{
		sd_music_manage_destroy();
		DEBUG_LOGE(LOG_TAG, "SEMPHR_CREATE_LOCK failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//创建运行线程
	if (!task_thread_create(task_sd_music_manage,
		    "task_sd_music_manage",
		    APP_NAME_SD_MUSIC_MANAGE_STACK_SIZE,
		    g_sd_music_handle,
		    task_priority)) 
    {

		DEBUG_LOGE(LOG_TAG, "create task_sd_music_manage failed");
		sd_music_manage_destroy();
		return APP_FRAMEWORK_ERRNO_FAIL;
    }
    
    return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t sd_music_manage_delete(void)
{
	return app_send_message(APP_NAME_SD_MUSIC_MANAGE, APP_NAME_SD_MUSIC_MANAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


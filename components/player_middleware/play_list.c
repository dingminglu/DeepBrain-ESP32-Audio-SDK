#include "play_list.h"

#define LOG_TAG "play list"

//play list handle
PLAY_LIST_HANDLE_t *g_play_list_handle = NULL;

//新建播放列表
APP_FRAMEWORK_ERRNO_t new_playlist(void **playlist)
{
	PLAY_LIST_t *list = memory_malloc(sizeof(PLAY_LIST_t));
	if (list == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new_playlist failed, out of memory");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	memset(list, 0, sizeof(PLAY_LIST_t));
	TAILQ_INIT(&list->play_list_queue);
	*playlist = list;
	
	return APP_FRAMEWORK_ERRNO_OK;
}

//删除播放列表
APP_FRAMEWORK_ERRNO_t del_playlist(void *playlist)
{
	PLAY_LIST_t *list = playlist;
	if (playlist == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	PLAY_LIST_ITEM_t *item = NULL;
	while ((item = TAILQ_FIRST(&list->play_list_queue)))
	{
		TAILQ_REMOVE(&list->play_list_queue, item, next);
		memory_free(item);
	}

	memory_free(playlist);
	
	return APP_FRAMEWORK_ERRNO_NO_MSG;
}

APP_FRAMEWORK_ERRNO_t set_playlist_mode(
	void *playlist, PLAY_LIST_MODE_t mode)
{
	PLAY_LIST_t *list = (PLAY_LIST_t *)playlist;

	if (list != NULL)
	{
		list->mode = mode;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t set_playlist_callback(
	void *playlist, PLAYLIST_CALL_BACKS_t *call_back)
{
	PLAY_LIST_t *list = (PLAY_LIST_t *)playlist;

	if (list != NULL)
	{
		list->call_back = *call_back;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

//添加播放列表项
APP_FRAMEWORK_ERRNO_t add_playlist_item(
	void *playlist,
	const char *url,
	const audio_play_event_notify cb)
{
	PLAY_LIST_t *list = (PLAY_LIST_t *)playlist;
	
	if (playlist == NULL || url == NULL)
	{
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	PLAY_LIST_ITEM_t *item = memory_malloc(sizeof(PLAY_LIST_ITEM_t));
	if (item == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "add_playlist_item malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(item, 0, sizeof(PLAY_LIST_ITEM_t));
	item->call_back = cb;
	snprintf(item->music_url, sizeof(item->music_url), "%s", url);
	
	TAILQ_INSERT_TAIL(&list->play_list_queue, item, next);
	
	return APP_FRAMEWORK_ERRNO_OK;
}

/**
 * playlist status callback
 *
 * @param event,play status event
 * @return none
 */
static void play_list_status_cb(const AUDIO_PLAY_EVENTS_T *event)
{	
	//app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_PLAY_STAUTS, &event->play_status, sizeof(event->play_status));				
	DEBUG_LOGE(LOG_TAG, "play_list_status_cb CREATE SUCCESS");
}

/**
 * audio event notify callback
 *
 * @param event,play status event
 * @return none
 */
static void audio_event_notify_cb(const AUDIO_PLAY_EVENTS_T *event)
{	
	app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_PLAY_STAUTS, event, sizeof(AUDIO_PLAY_EVENTS_T));				
}

/**
 * playlist play item
 *
 * @param play list item
 * @return none 
 */
static void playlist_play_item(PLAY_LIST_ITEM_t *music)
{
	if (music == NULL)
	{
		return;
	}
	
	audio_play_music_with_cb(music->music_url, audio_event_notify_cb);
}

/**
 * playlist process
 *
 * @param handle
 * @param event
 * @return none
 */
static void playlist_process(PLAY_LIST_HANDLE_t *handle, int event)
{
	PLAY_LIST_t *playlist = handle->playlist;
	
	if (handle->playlist == NULL)
	{
		return;
	}
	
	switch (event)
	{
		case APP_EVENT_PLAYLIST_REQUEST://播放列表
		{
			handle->playlist->cur_item = TAILQ_FIRST(&handle->playlist->play_list_queue);
			if (playlist->cur_item != NULL)
			{
				playlist_play_item(playlist->cur_item);	
			}
			break;
		}
		case APP_EVENT_PLAYLIST_PREV://播放上一曲
		{
			switch (playlist->mode)
			{	
				case PLAY_LIST_MODE_SINGLE://单曲,不循环
				case PLAY_LIST_MODE_SINGLE_LOOP://单曲循环
				case PLAY_LIST_MODE_ALL://全部,不循环
				{					
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_PREV(handle->playlist->cur_item, PLAY_LIST_QUEUE_t, next);
						if (handle->playlist->cur_item == NULL)
						{
							if (handle->playlist->call_back.prev != NULL)
							{
								handle->playlist->call_back.prev();
							}
							else
							{
								handle->playlist->cur_item = TAILQ_FIRST(&handle->playlist->play_list_queue);
							}
						}
						
						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_LOOP://全部循环
				{
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_PREV(handle->playlist->cur_item, PLAY_LIST_QUEUE_t, next);
						if (handle->playlist->cur_item == NULL)
						{
							handle->playlist->cur_item = TAILQ_LAST(&handle->playlist->play_list_queue, PLAY_LIST_QUEUE_t);
						}
						
						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_RAND://全部随机,循环
				{
					break;
				}
				default:
					break;
			}
			break;
		}
		case APP_EVENT_PLAYLIST_NEXT://播放下一曲
		{
			switch (playlist->mode)
			{	
				case PLAY_LIST_MODE_SINGLE://单曲,不循环
				case PLAY_LIST_MODE_SINGLE_LOOP://单曲循环
				case PLAY_LIST_MODE_ALL://全部,不循环
				{	
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_NEXT(handle->playlist->cur_item, next);
						if (handle->playlist->cur_item == NULL)
						{
							if (handle->playlist->call_back.next != NULL)
							{
								handle->playlist->call_back.next();
							}
							else
							{
								handle->playlist->cur_item = TAILQ_LAST(&handle->playlist->play_list_queue, PLAY_LIST_QUEUE_t);
							}
						}

						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_LOOP://全部循环
				{
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_NEXT(handle->playlist->cur_item, next);
						if (handle->playlist->cur_item == NULL)
						{
							handle->playlist->cur_item = TAILQ_FIRST(&handle->playlist->play_list_queue);
						}
						
						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_RAND://全部随机,循环
				{
					break;
				}
				default:
					break;
			}
			break;
		}
		case APP_EVENT_PLAYLIST_STOP://歌曲播放完毕
		{
			switch (playlist->mode)
			{	
				case PLAY_LIST_MODE_SINGLE://单曲,不循环
				{			
					if (handle->playlist->call_back.next != NULL)
					{
						handle->playlist->call_back.next();
					}
					break;
				}
				case PLAY_LIST_MODE_SINGLE_LOOP://单曲循环
				{					
					if (handle->playlist->cur_item != NULL)
					{
						playlist_play_item(playlist->cur_item);
					}
					break;
				}
				case PLAY_LIST_MODE_ALL://全部,不循环
				{
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_NEXT(handle->playlist->cur_item, next);
						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
						else
						{
							if (handle->playlist->call_back.next != NULL)
							{
								handle->playlist->call_back.next();
							}
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_LOOP://全部循环
				{
					if (handle->playlist->cur_item != NULL)
					{
						handle->playlist->cur_item = TAILQ_NEXT(handle->playlist->cur_item, next);
						if (handle->playlist->cur_item == NULL)
						{
							handle->playlist->cur_item = TAILQ_FIRST(&handle->playlist->play_list_queue);
						}

						if (handle->playlist->cur_item != NULL)
						{
							playlist_play_item(playlist->cur_item);
						}
					}
					break;
				}
				case PLAY_LIST_MODE_ALL_RAND://全部随机,循环
				{
					break;
				}
				default:
					break;
			}
			break;
		}
		case APP_EVENT_PLAYLIST_CLEAR://清空播放列表
		{
			break;
		}
		default:
			break;
	}
}

static void playlist_event_callback(
	APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	PLAY_LIST_HANDLE_t *handle = g_play_list_handle;
	switch (msg->event)
	{
		case APP_EVENT_PLAYLIST_REQUEST:
		{
			if (msg->event_data_len != sizeof(void*)
				|| msg->event_data == NULL)
			{
				DEBUG_LOGE(LOG_TAG, "APP_EVENT_PLAYLIST_REQUEST invalid event data size[%d][%d]", 
					msg->event_data_len, sizeof(void*));
				break;
			}

			del_playlist(handle->playlist);
			handle->playlist = NULL;
			memcpy(&handle->playlist, msg->event_data, sizeof(void*));		
			playlist_process(handle, APP_EVENT_PLAYLIST_REQUEST);
			break;
		}
		case APP_EVENT_PLAYLIST_PREV:
		{
			playlist_process(handle, APP_EVENT_PLAYLIST_PREV);				
			break;
		}
		case APP_EVENT_PLAYLIST_NEXT:
		{
			playlist_process(handle, APP_EVENT_PLAYLIST_NEXT);
			break;
		}
		case APP_EVENT_PLAYLIST_CLEAR:
		{
			del_playlist(handle->playlist);
			handle->playlist = NULL;
			break;
		}
		case APP_EVENT_PLAYLIST_PLAY_STAUTS:
		{
			AUDIO_PLAY_EVENTS_T *play_event = (AUDIO_PLAY_EVENTS_T *)msg->event_data;
			
			if (handle->playlist == NULL
				|| handle->playlist->cur_item == NULL)
			{
				break;
			}

			if (strcmp((char*)play_event->music_url, (char*)handle->playlist->cur_item->music_url) != 0)
			{
				DEBUG_LOGE(LOG_TAG, "url is diffrent\r\n[%s]\r\n[%s]",
					play_event->music_url, handle->playlist->cur_item->music_url);
				break;
			}
			
			if(handle->playlist->cur_item->call_back != NULL)
			{
				handle->playlist->cur_item->call_back(play_event);
				break;
			}
			
			if(play_event->play_status == AUDIO_PLAY_STATUS_FINISHED || play_event->play_status == AUDIO_PLAY_STATUS_STOP 
				|| play_event->play_status == AUDIO_PLAY_STATUS_ERROR)
			{
				playlist_process(handle, APP_EVENT_PLAYLIST_STOP);				
			}
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


static void task_playlist(void *pv)
{
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_PLAY_LIST);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_PLAY_LIST);
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 500, playlist_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, playlist_event_callback);
		app_add_event(app, APP_EVENT_AUDIO_PLAYER_BASE, playlist_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, playlist_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, playlist_event_callback);
		app_add_event(app, APP_EVENT_PLAYLIST_BASE, playlist_event_callback);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	del_playlist(g_play_list_handle->playlist);
	g_play_list_handle->playlist = NULL;
	g_play_list_handle->playlist = NULL;
	memory_free(g_play_list_handle);
	g_play_list_handle = NULL;

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t playlist_service_create(int task_priority)
{	
	g_play_list_handle = (PLAY_LIST_HANDLE_t *)memory_malloc(sizeof(PLAY_LIST_HANDLE_t));
	if (g_play_list_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "playlist_create failed, out of memory");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset((char*)g_play_list_handle, 0, sizeof(PLAY_LIST_HANDLE_t));

    if (!task_thread_create(task_playlist,
	        "task_playlist",
	        APP_NAME_PLAY_LIST_STACK_SIZE,
	        g_play_list_handle,
	        task_priority)) 
    {
		memory_free(g_play_list_handle);
		g_play_list_handle = NULL;
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_playlist task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t playlist_service_delete(void)
{
	return app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

//添加播放列表
APP_FRAMEWORK_ERRNO_t playlist_add(void **playlist)
{
	return app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_REQUEST, playlist, sizeof(playlist));
}

//上一曲
APP_FRAMEWORK_ERRNO_t playlist_prev(void)
{
	return app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_PREV, NULL, 0);
}

//下一曲
APP_FRAMEWORK_ERRNO_t playlist_next(void)
{
	return app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_NEXT, NULL, 0);
}

//清空播放列表
APP_FRAMEWORK_ERRNO_t playlist_clear(void)
{
	return app_send_message(APP_NAME_PLAY_LIST, APP_NAME_PLAY_LIST, APP_EVENT_PLAYLIST_CLEAR, NULL, 0);
}

void test_playlist(void)
{
	void *playlist = NULL;

	if (new_playlist(&playlist) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "new_playlist failed");
		return;
	}

	add_playlist_item(playlist, "http://cdnmusic.hezi.360iii.net/prod/tts/childrenchat/source20/5aec5117fd18cb4981c1e260.mp3", play_list_status_cb);	
	add_playlist_item(playlist, "http://cdnmusic.hezi.360iii.net/hezimusic/103395.mp3", play_list_status_cb);	
	add_playlist_item(playlist, "http://cdnmusic.hezi.360iii.net/hezimusic/106913.mp3", play_list_status_cb);
	//add_playlist_item(playlist, AUDIO_MUSIC_TYPE_MEM, get_flash_music_data(2), 0, get_flash_music_size(2));
	//set_playlist_mode(playlist, PLAY_LIST_MODE_SINGLE);
	//set_playlist_mode(playlist, PLAY_LIST_MODE_SINGLE_LOOP);
	//set_playlist_mode(playlist, PLAY_LIST_MODE_ALL);
	set_playlist_mode(playlist, PLAY_LIST_MODE_SINGLE_LOOP);
	playlist_add(&playlist);
}


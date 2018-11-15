#include <time.h>
#include "memo_service.h"
#include <socket.h>
#include "cJSON.h"
#include "debug_log_interface.h"
#include "array_music.h"
#include "userconfig.h"
#include "power_manage.h"
#include "device_params_manage.h"
#include "auth_crypto.h"
#include "http_api.h"
#include "player_middleware_interface.h"

#define PRINT_TAG     "MEMO CLIENT"

//attrName
#define MEMO_TIME    "备忘时间"
#define MEMO_EVENT   "备忘事件"
#define EXECUTETIME  "executeTimeParseSecond"
#define EVENTCONTENT "eventContent"
#define EVENTTYPE    "eventType"

//attrValue
#define OUTDATE      "outDateError"
#define REMINDMEMO   "remindMemo"

MEMO_SERVICE_HANDLE_T *memo_struct_handle = NULL;

int get_memo_result(char *str_body)
{
	time_t exe_time = 0;
	struct tm tm = {0};
	int arry_size = 0;
	int memo_status = 0;
	
	//DEBUG_LOGW(PRINT_TAG, "str_body [%s]", str_body);
	cJSON *pJson_body = cJSON_Parse(str_body);
	if (NULL == pJson_body) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body cJSON_Parse fail");
		goto get_memo_result_error;
	}

	//命令名称
	cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
	if (NULL == pJson_cmd_data) 
	{
		DEBUG_LOGE(PRINT_TAG, "pJson_body, pJson_cmd_data not found\n");
		goto get_memo_result_error;
	}
	
	cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
	if (NULL == pJson_cmd_name) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body_buf,commandName not found\n");
		goto get_memo_result_error;
	}			
	DEBUG_LOGE(PRINT_TAG, "pJson_cmd_name = %s", pJson_cmd_name->valuestring);

	//JSON数组
	cJSON *pJson_array = cJSON_GetObjectItem(pJson_cmd_data, "commandAttrs");
	if (NULL == pJson_array) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body_buf,pJson_array not found\n");
		goto get_memo_result_error;
	}

	//JSON数组大小
	arry_size = cJSON_GetArraySize(pJson_array);
	if (arry_size == 0)
	{
		DEBUG_LOGE(PRINT_TAG, "arry_size[%d], error", arry_size);
		goto get_memo_result_error;
	}

	int i = 0;
	for (i = 0; i < arry_size; i++)
	{
		//获取数组成员
		cJSON *pJson_array_item = cJSON_GetArrayItem(pJson_array, i);
		if (NULL == pJson_array_item) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array, pJson_array_item not found\n");
			goto get_memo_result_error;
		}

		//成员 attrName
		cJSON *pJson_array_attrName = cJSON_GetObjectItem(pJson_array_item, "attrName");
		if (NULL == pJson_array_attrName
			|| pJson_array_attrName->valuestring == NULL) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array_item, pJson_array_attrName not found\n");
			goto get_memo_result_error;
		}

		//成员 attrValue
		cJSON *pJson_array_attrValue = cJSON_GetObjectItem(pJson_array_item, "attrValue");
		if (NULL == pJson_array_attrValue
			|| pJson_array_attrValue->valuestring == NULL) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array_item,pJson_array_attrValue not found\n");
			goto get_memo_result_error;
		}
		
		DEBUG_LOGE(PRINT_TAG, "pJson_array_attrName = %s", pJson_array_attrName->valuestring);
		DEBUG_LOGE(PRINT_TAG, "pJson_array_attrValue = %s", pJson_array_attrValue->valuestring);
		
		if (strncmp(pJson_array_attrName->valuestring, EXECUTETIME, sizeof(EXECUTETIME)) == 0)//执行时间
		{		
			exe_time = time(NULL) + atoi(pJson_array_attrValue->valuestring) + 30;
			DEBUG_LOGE(PRINT_TAG, "Execute Time = [%ld]", exe_time);
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTTYPE, sizeof(EVENTTYPE)) == 0)//事件类型
		{		
			if(strncmp(pJson_array_attrValue->valuestring, OUTDATE, sizeof(OUTDATE)) == 0) //日期过期			
			{
				memo_status = 0;				
			}
			else if (strncmp(pJson_array_attrValue->valuestring, REMINDMEMO, sizeof(REMINDMEMO)) == 0)//记录备忘
			{
				memo_status = 1;				
			}
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTCONTENT, sizeof(EVENTCONTENT)) == 0)//事件内容
		{		
			snprintf(memo_struct_handle->memo_event.str, sizeof(memo_struct_handle->memo_event.str), pJson_array_attrValue->valuestring);
		}		
		
		pJson_array_item      = NULL;
		pJson_array_attrName  = NULL;
		pJson_array_attrValue = NULL;
	}

	if (memo_status)
	{
		audio_play_tone_mem(FLASH_MUSIC_BEI_WANG_YI_TIAN_JIA, AUDIO_TERM_TYPE_NOW);
		task_thread_sleep(2000);
		memo_struct_handle->memo_event.time = exe_time;
		memo_struct_handle->memo_status = MEMO_ADD;	
	}
	else
	{
		audio_play_tone_mem(FLASH_MUSIC_BEI_WANG_YI_GUO_QI, AUDIO_TERM_TYPE_NOW);
		memset(memo_struct_handle->memo_event.str, 0, sizeof(memo_struct_handle->memo_event.str));
	}
	
	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}

	return 0;		

get_memo_result_error:	

	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}
	
	return -1;
}

//初始化时钟
static void init_clock(MEMO_SERVICE_HANDLE_T *handle)
{
	uint32_t time_now = 0;
	
	if (!get_dcl_auth_params(&handle->auth_params))
	{
		DEBUG_LOGE(PRINT_TAG, "get_dcl_auth_params failed"); 
		task_thread_sleep(3000);
		return;
	}

	if (dcl_update_time(&handle->auth_params, &time_now) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(PRINT_TAG, "dcl_update_time failed"); 
		task_thread_sleep(3000);
		return;
	}

	//校准系统时钟
 	struct timeval tv = { 
		.tv_sec  = time_now,
		.tv_usec = 0
	};
	settimeofday(&tv, NULL);
	update_power_mng_sleep_time();

	get_flash_cfg(FLASH_CFG_MEMO_PARAMS, &(handle->memo_array));
	if(handle->memo_array.current > 0)
	{
		handle->memo_status = MEMO_LOOP;
	}
	else
	{
		handle->memo_status = MEMO_IDLE;
	}
}

//添加提醒
static void add_memo(MEMO_SERVICE_HANDLE_T *handle)
{		
	int num_min  = 0;
	int num = 0;
	uint32_t time_min = 0;
	
	//备忘已满将覆盖最小日期
	if (handle->memo_array.current >= MEMO_EVEVT_MAX)
	{
		for (num = 0; num < MEMO_EVEVT_MAX; num++) 
		{
			if (time_min > handle->memo_array.event[num].time)
			{
				num_min  = num;
				time_min = handle->memo_array.event[num].time;
			}
		}
		handle->memo_array.event[num_min].time = handle->memo_event.time;
		snprintf(handle->memo_array.event[num_min].str, sizeof(handle->memo_event.str), handle->memo_event.str);
		memset(&handle->memo_event, 0, sizeof(MEMO_EVENT_T));
		set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &handle->memo_array);
	}
	else
	{
		for (num = 0; num < MEMO_EVEVT_MAX; num++) 
		{
			if (handle->memo_array.event[num].time == 0)
			{
				handle->memo_array.event[num].time = handle->memo_event.time;
				snprintf(handle->memo_array.event[num].str, sizeof(handle->memo_event.str), handle->memo_event.str);
				handle->memo_array.current++;
				memset(&handle->memo_event, 0, sizeof(MEMO_EVENT_T));
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &handle->memo_array);
				break;
			}
		}
	}
	memo_struct_handle->memo_status = MEMO_LOOP;
}

//执行提醒
static void memo_event_handle(MEMO_ARRAY_T *handle)
{
	int num = 0;
	time_t time_sec = time(NULL);

	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		if (handle->event[num].time != 0) 
		{
			update_power_mng_sleep_time();
			//清除过期备忘
			if (handle->event[num].time < time_sec - 30)
			{
				memset(&handle->event[num], 0, sizeof(MEMO_EVENT_T));
				handle->current--;
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, handle);
				continue;
			}

			//执行备忘提醒，并删除
			if ((handle->event[num].time - 30) < time_sec &&
				(handle->event[num].time + 30) > time_sec) 
			{
				audio_play_tts(handle->event[num].str);
				snprintf(memo_struct_handle->memo_content, sizeof(memo_struct_handle->memo_content), "%s", handle->event[num].str);
				memo_struct_handle->memo_remind_count = 1;
				memo_struct_handle->memo_remind_timestamp = time(NULL);
				memset(&handle->event[num], 0, sizeof(MEMO_EVENT_T));
				handle->current--;
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, handle);
			}

			if(handle->current == 0)
			{
				//memo_struct_handle->memo_status = MEMO_IDLE;
				break;
			}
		}
	}

	if (strlen(memo_struct_handle->memo_content) > 0
		&& abs(memo_struct_handle->memo_remind_timestamp - time(NULL)) >= 6)
	{
		audio_play_tts(memo_struct_handle->memo_content);
		memo_struct_handle->memo_remind_count++;
		memo_struct_handle->memo_remind_timestamp = time(NULL);
		
		if (memo_struct_handle->memo_remind_count == 20)
		{
			memset(memo_struct_handle->memo_content, 0, sizeof(memo_struct_handle->memo_content));
		}
	}
}

void show_memo(void)
{
	int num = 0;
	DEBUG_LOGE("TERM_CTRL", "current time = %ld\n", time(NULL));
	DEBUG_LOGE("TERM_CTRL", "Memo Number  = %d\n", memo_struct_handle->memo_array.current);
	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		DEBUG_LOGE("TERM_CTRL", "event[%d].time = %d\r\n", num, memo_struct_handle->memo_array.event[num].time);
		DEBUG_LOGE("TERM_CTRL", "event[%d].str  = %s\r\n", num, memo_struct_handle->memo_array.event[num].str);
	}
}

static void memo_manage_callback(APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	MEMO_SERVICE_HANDLE_T *handle = memo_struct_handle;

	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED:
		{
			handle->memo_status = MEMO_INIT;
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			if (handle->memo_status == MEMO_INIT)
			{
				handle->memo_status = MEMO_IDLE;
			}
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			switch (handle->memo_status)
			{
				case MEMO_IDLE:
				{
					break;
				}			
				case MEMO_INIT:
				{
					init_clock(handle);
					break;
				}
				case MEMO_ADD:
				{
					add_memo(handle);
					break;
				}		
				case MEMO_LOOP:
				{
					memo_event_handle(&handle->memo_array);
					break;
				}	
				default:
					break;
			}
			break;
		}
		case APP_EVENT_MEMO_CANCLE:
		{
			if (strlen(memo_struct_handle->memo_content) > 0)
			{
				memset(memo_struct_handle->memo_content, 0, sizeof(memo_struct_handle->memo_content));
			}
			break;
		}
		default:
			break;
	}
}

static void task_memo_manage(void *pv)
{
	APP_OBJECT_t *app = NULL;

	vTaskDelay(8*1000);
	
	app = app_new(APP_NAME_MEMO_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "new app[%s] failed, out of memory", APP_NAME_MEMO_MANAGE);
		task_thread_exit();
	}
	else
	{
		app_set_loop_timeout(app, 1000, memo_manage_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, memo_manage_callback);
		app_add_event(app, APP_EVENT_MEMO_BASE, memo_manage_callback);
		DEBUG_LOGE(PRINT_TAG, "%s create success", APP_NAME_MEMO_MANAGE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);
	
	memory_free(memo_struct_handle);

	task_thread_exit();
}


APP_FRAMEWORK_ERRNO_t memo_service_create(const int task_priority)
{
	if (memo_struct_handle != NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "memo_struct_handle already exists");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	memo_struct_handle = (MEMO_SERVICE_HANDLE_T *)memory_malloc(sizeof(MEMO_SERVICE_HANDLE_T));
	if (memo_struct_handle == NULL) 
	{
		DEBUG_LOGE(PRINT_TAG, "memory_malloc memo_struct_handle fail!");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}	
	memset(memo_struct_handle, 0, sizeof(MEMO_SERVICE_HANDLE_T));
	//set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &memo_struct_handle->memo_array);

    if (!task_thread_create(task_memo_manage,
	        "task_memo_manage",
	        APP_NAME_MEMO_MANAGE_STACK_SIZE,
	        memo_struct_handle,
	        task_priority)) {

    	DEBUG_LOGE("BIND DEVICE", "ERROR creating task_memo task! Out of memory?");
		memory_free(memo_struct_handle);
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	DEBUG_LOGI(PRINT_TAG, "memo_service_create\r\n");
	return APP_FRAMEWORK_ERRNO_OK;
}


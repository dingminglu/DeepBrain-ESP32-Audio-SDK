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

#include <time.h>
#include "app_config.h"
#include "deep_sleep_manage.h"
#include "player_middleware_interface.h"
#include "power_manage.h"
#include "power_interface.h"
#include "array_music.h"
#include "AudioDef.h"

#define PRINT_TAG 		        "POWER_MANAGE"
#define LOW_POWER_VOLTAGE		3.60				//播报低电量提示电压区间最高
#define DEVICE_SLEEP_TIME		(30*60)				//30分钟没有动作，则机器自动进入休眠
#define DEVICE_SHALLOW_SLEEP_TIME    (5*60)			//5分钟没有动作，则机器自动进入浅度休眠
#define REMIND_COUNT            2                   //低电量提示次数

typedef enum CHARGING_STATUS_t
{
	CHARGING_STATUS_INIT = 0,
	CHARGING_STATUS_IDEL,
	CHARGING_STATUS_CHARGING,
	CHARGING_STATUS_START_CHARGING,
	CHARGING_STATUS_STOP_CHARGING,
}CHARGING_STATUS_t;

//电源管理
typedef struct POWER_MANAGE_HANDLE_t
{
	//设备休眠参数
	bool auto_sleep_ctl; 		//自动休眠控制
	uint32_t sleep_timestamp;	//睡眠时间，实时
	uint32_t min_sleep_sec;		//设备固定深度睡眠时间，最短时间，单位秒
	uint32_t shallow_sleep_sec;		//浅度休眠时间

	//设备低电量参数
	float min_bat_vol;				//最低电量值,单位伏
	uint32_t low_power_remind_count;//低电量提醒次数

	//其他参数
	void *lock;	//资源互斥锁
}POWER_MANAGE_HANDLE_t;

static POWER_MANAGE_HANDLE_t *g_power_manage_handle = NULL;

bool update_power_mng_sleep_time(void)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}

	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	if (g_power_manage_handle != NULL)
	{
		g_power_manage_handle->sleep_timestamp = time(NULL);
	}
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_sleep_time(uint32_t *sleep_sec)
{
	if (g_power_manage_handle == NULL || sleep_sec == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*sleep_sec = g_power_manage_handle->sleep_timestamp;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool set_power_mng_auto_sleep(bool enable)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	g_power_manage_handle->auto_sleep_ctl = enable;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_auto_sleep(bool *enable)
{
	if (g_power_manage_handle == NULL 
		|| enable == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*enable = g_power_manage_handle->auto_sleep_ctl;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool set_power_mng_sleep_sec(uint32_t sleep_sec)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	g_power_manage_handle->min_sleep_sec = sleep_sec;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_sleep_sec(uint32_t *sleep_sec)
{
	if (g_power_manage_handle == NULL 
		|| sleep_sec == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*sleep_sec = g_power_manage_handle->min_sleep_sec;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool set_power_mng_shallow_sleep_sec(uint32_t sleep_sec)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	g_power_manage_handle->shallow_sleep_sec = sleep_sec;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_shallow_sleep_sec(uint32_t *sleep_sec)
{
	if (g_power_manage_handle == NULL 
		|| sleep_sec == NULL)
	{
		return false;
	}
		
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*sleep_sec = g_power_manage_handle->shallow_sleep_sec;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool set_power_mng_min_vol(float vol)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	g_power_manage_handle->min_bat_vol = vol;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_min_vol(float *vol)
{
	if (g_power_manage_handle == NULL 
		|| vol == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*vol = g_power_manage_handle->min_bat_vol;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool set_power_mng_remind_count(uint32_t count)
{
	if (g_power_manage_handle == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	g_power_manage_handle->low_power_remind_count = count;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

bool get_power_mng_remind_count(uint32_t *count)
{
	if (g_power_manage_handle == NULL 
		|| count == NULL)
	{
		return false;
	}
	
	SEMPHR_TRY_LOCK(g_power_manage_handle->lock);
	*count = g_power_manage_handle->low_power_remind_count;
	SEMPHR_TRY_UNLOCK(g_power_manage_handle->lock);	

	return true;
}

static void power_event_callback(void *app, APP_EVENT_MSG_t *msg)
{
	float battery_vol = 0.0;
	float low_battery_vol = 0.0;
	static uint32_t play_time = 0;
	static uint32_t lowpower_remind_count = 0;
	static uint32_t charge_status = 0;
	static uint32_t power_button_press_count = 0;
	bool sleep_enable = false;
	uint32_t min_sleep_sec = 0;
	uint32_t shallow_sleep_sec = 0;
	uint32_t remind_count = 0;
	static uint32_t sleep_timestamp = 0;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			//获取当前电池电压
			battery_vol = get_battery_voltage();
   			DEBUG_LOGI(PRINT_TAG, "Battery vol = %.2fv", battery_vol);			
			DEBUG_LOGI(PRINT_TAG, "free mem size:inter[%dkb],psram[%dkb]", 
				heap_caps_get_free_size(MALLOC_CAP_8BIT|MALLOC_CAP_INTERNAL)/1024, 
				xPortGetFreeHeapSize()/1024);
#if 0
			//按键检测
			if (get_power_button_status())
			{
				power_button_press_count++;
			}
			else
			{
				power_button_press_count = 0;
			}

			if (power_button_press_count >= 5)
			{
				DEBUG_LOGI(PRINT_TAG, "power button press,power off now");
				audio_play_tone_mem(FLASH_MUSIC_BYEBYE, TERMINATION_TYPE_NOW);
				vTaskDelay(4*1000);
				power_off();
			}
#endif
			//浅度休眠
			if (get_power_mng_auto_sleep(&sleep_enable)
				&& sleep_enable)
			{
				if (get_power_mng_shallow_sleep_sec(&shallow_sleep_sec)
					&& get_power_mng_sleep_time(&sleep_timestamp)
					&& abs(sleep_timestamp - time(NULL)) >= shallow_sleep_sec)
				{
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_OFF, NULL, 0);
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_OFF, NULL, 0);
					DEBUG_LOGW(PRINT_TAG, "shallow sleep");
				}
			}
			
			//设备自动关机（实际上是休眠）
			if (get_power_mng_auto_sleep(&sleep_enable)
				&& sleep_enable)
			{
				if (get_power_mng_sleep_sec(&min_sleep_sec)
					&& get_power_mng_sleep_time(&sleep_timestamp)
					&& abs(sleep_timestamp - time(NULL)) >= min_sleep_sec)
				{
					audio_play_tone_mem(FLASH_MUSIC_BYEBYE, TERMINATION_TYPE_NOW);
					vTaskDelay(4*1000);
					deep_sleep_manage();
				}
			}

			switch (charge_status)
			{
				case CHARGING_STATUS_INIT:
				{		
					if (get_charging_status())
					{
						charge_status = CHARGING_STATUS_CHARGING;
						app_send_message(APP_NAME_POWER_MANAGE, APP_MSG_TO_ALL, APP_EVENT_POWER_CHARGING, NULL, 0);
					}
					else
					{
						charge_status = CHARGING_STATUS_IDEL;
					}
					break;
				}
				case CHARGING_STATUS_IDEL:
				{				
					//低电提醒	
					get_power_mng_min_vol(&low_battery_vol);
					get_power_mng_remind_count(&remind_count);
					
					if(battery_vol <= low_battery_vol)
					{	
						if (lowpower_remind_count >= remind_count)
						{
							DEBUG_LOGI(PRINT_TAG, "Low battery sleep");
							audio_play_tone_mem(FLASH_MUSIC_BYEBYE, TERMINATION_TYPE_NOW);
							vTaskDelay(4*1000);
							deep_sleep_manage();
						}
						else
						{
							if (abs(play_time - time(NULL)) >= 10)
							{
								DEBUG_LOGI(PRINT_TAG, "Low battery remind");
								app_send_message(APP_NAME_POWER_MANAGE, APP_MSG_TO_ALL, APP_EVENT_POWER_LOW_POWER, NULL, 0);
								audio_play_tone_mem(FLASH_MUSIC_BATTERY_LOW, TERMINATION_TYPE_NOW); 		
								play_time = time(NULL);
								lowpower_remind_count++;
							}
						}
					}
					if (get_charging_status())
					{
						charge_status = CHARGING_STATUS_START_CHARGING;
					}
					break;
				}
				case CHARGING_STATUS_CHARGING:
				{				
					update_power_mng_sleep_time();
					DEBUG_LOGI(PRINT_TAG, "Usb charging now");
					if (!get_charging_status())
					{
						charge_status = CHARGING_STATUS_STOP_CHARGING;
					}
					
					break;
				}
				case CHARGING_STATUS_START_CHARGING:
				{
					audio_play_tone_mem(FLASH_MUSIC_START_CHARGING, TERMINATION_TYPE_NOW);
					task_thread_sleep(3*1000);	
					app_send_message(APP_NAME_POWER_MANAGE, APP_MSG_TO_ALL, APP_EVENT_POWER_CHARGING, NULL, 0);

					charge_status = CHARGING_STATUS_CHARGING;

					//接入充电待机状态灯效
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_OFF, NULL, 0);
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_OFF, NULL, 0);
					break;
				}
				case CHARGING_STATUS_STOP_CHARGING:
				{
					if(battery_vol <= 4.0)
					{
						audio_play_tone_mem(FLASH_MUSIC_EMO_HUNGRY, TERMINATION_TYPE_NOW);
						vTaskDelay(3*1000);
					}
					app_send_message(APP_NAME_POWER_MANAGE, APP_MSG_TO_ALL, APP_EVENT_POWER_CHARGING_STOP, NULL, 0);
					
					charge_status = CHARGING_STATUS_IDEL;

					//去除充电器待机状态灯效
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
					app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case APP_EVENT_AUDIO_PLAYER_PLAY:
		{	
			update_power_mng_sleep_time();
			break;
		}
		case APP_EVENT_POWER_SHUTDOWN_INSTRUCTION:
		{	
			audio_play_tone_mem(FLASH_MUSIC_BYEBYE, TERMINATION_TYPE_NOW);
			vTaskDelay(4*1000);
			deep_sleep_manage();
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

static void power_manage_destroy(void)
{
	if (g_power_manage_handle == NULL)
	{
		return;
	}

	if (g_power_manage_handle->lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_power_manage_handle->lock);
		g_power_manage_handle->lock = NULL;
	}
	
	memory_free(g_power_manage_handle);
	g_power_manage_handle = NULL;
}

static void task_power_manage(void *pv)
{	
	APP_OBJECT_t *app = app_new(APP_NAME_POWER_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "new app[%s] failed, out of memory", APP_NAME_POWER_MANAGE);
		power_manage_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 1000, power_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, power_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, power_event_callback);
		app_add_event(app, APP_EVENT_LED_CTRL_BASE, power_event_callback);
		DEBUG_LOGI(PRINT_TAG, "%s create success", APP_NAME_POWER_MANAGE);
	}

	app_msg_dispatch(app);
	
	app_delete(app);	

	power_manage_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t power_manage_create(const int task_priority)
{
	if (g_power_manage_handle != NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "g_power_manage handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//申请运行句柄
	g_power_manage_handle = (POWER_MANAGE_HANDLE_t *)memory_malloc(sizeof(POWER_MANAGE_HANDLE_t));
	if (g_power_manage_handle == NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "memory_malloc g_power_manage_handle failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_power_manage_handle, 0, sizeof(POWER_MANAGE_HANDLE_t));

	//初始化参数
	SEMPHR_CREATE_LOCK(g_power_manage_handle->lock);
	if (g_power_manage_handle->lock == NULL)
	{
		power_manage_destroy();
		DEBUG_LOGE(PRINT_TAG, "g_power_manage->lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	g_power_manage_handle->auto_sleep_ctl = true;
	g_power_manage_handle->sleep_timestamp = time(NULL);
	g_power_manage_handle->min_sleep_sec = DEVICE_SLEEP_TIME;
	g_power_manage_handle->shallow_sleep_sec = DEVICE_SHALLOW_SLEEP_TIME;
	
	g_power_manage_handle->min_bat_vol = LOW_POWER_VOLTAGE;
	g_power_manage_handle->low_power_remind_count = REMIND_COUNT;
	

	//平台硬件接口初始化
	power_peripheral_init();

	//创建运行线程
	if (!task_thread_create(task_power_manage, 
			"task_power_manage", 
			APP_NAME_POWER_MANAGE_STACK_SIZE, 
			g_power_manage_handle, 
			task_priority)) 
    {
        DEBUG_LOGE(PRINT_TAG, "create task_power_manage failed");
		power_manage_destroy();
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t power_manage_delete(void)
{
	return app_send_message(APP_NAME_POWER_MANAGE, APP_NAME_POWER_MANAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


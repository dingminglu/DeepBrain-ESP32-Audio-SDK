#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "led_ctrl_interface.h"
#include "led_ctrl_service.h"
#include "userconfig.h"

#define LOG_TAG 						"led ctrl service"
#define LOOP_TIMEOUT					40
#define EYE_LED_FLICKER_ON				1
#define EYE_LED_FLICKER_OFF				0
#define EAR_LED_FLICKER_ON				1
#define EAR_LED_FLICKER_OFF				0

typedef struct LED_CTRL_SERVICE_HANDLE_T
{
	bool is_wifi_connected;
	void *lock;
	LED_CTRL_LED_STATUS_t led_status;
}LED_CTRL_SERVICE_HANDLE_T;

static LED_CTRL_SERVICE_HANDLE_T *g_led_ctrl_service_handle = NULL;

static void set_eye_led_state(APP_EVENT_LED_CTRL_t eye_led_state)
{
	g_led_ctrl_service_handle->led_status.eye_led_state = eye_led_state;
}

static void set_ear_led_state(APP_EVENT_LED_CTRL_t ear_led_state)
{
	g_led_ctrl_service_handle->led_status.ear_led_state = ear_led_state;
}

LED_CTRL_LED_STATUS_t get_led_status()
{
	LED_CTRL_LED_STATUS_t led_status = g_led_ctrl_service_handle->led_status;
	return led_status;
}

static void eye_led_flicker_status(int eye_led_event, int eye_led_flicker_switch, int eye_time_flag)
{
	static long int eye_time = 0;
	long int curr_time = 0;
	static int temp_eye_time_flag = 1;
	static int flag = 0;

	if (eye_led_event == APP_EVENT_LED_CTRL_EYE_300_MS_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 300)
		{
			temp_eye_time_flag = 0;
			led_eye_on();
		}
		else if ((curr_time - eye_time) >= 300 && (curr_time - eye_time) < 600)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
	else if (eye_led_event == APP_EVENT_LED_CTRL_EYE_500_MS_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		/*
		if (eye_time_flag == 1 && flag == 1)
		{
			temp_eye_time_flag = 1;
			flag = 0;
		}
		*/
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 500)
		{
			//if (eye_time_flag = 1)
			//{
				temp_eye_time_flag = 0;
				led_eye_on();				
			//}
		}
		else if ((curr_time - eye_time) >= 500 && (curr_time - eye_time) < 1000)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
	else if (eye_led_event == APP_EVENT_LED_CTRL_EYE_800_MS_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 800)
		{
			temp_eye_time_flag = 0;
			led_eye_on();
		}
		else if ((curr_time - eye_time) >= 800 && (curr_time - eye_time) < 1600)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
	else if (eye_led_event == APP_EVENT_LED_CTRL_EYE_1000_MS_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 1000)
		{
			temp_eye_time_flag = 0;
			led_eye_on();
		}
		else if ((curr_time - eye_time) >= 1000 && (curr_time - eye_time) < 2000)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
	else if (eye_led_event == APP_EVENT_LED_CTRL_EYE_3000_MS_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 3000)
		{
			temp_eye_time_flag = 0;
			led_eye_on();
		}
		else if ((curr_time - eye_time) >= 3000 && (curr_time - eye_time) < 6000)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
	else if (eye_led_event == APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER && eye_led_flicker_switch == EYE_LED_FLICKER_ON)
	{
		if (temp_eye_time_flag == 1)
		{
			eye_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - eye_time) >= 0 && (curr_time - eye_time) < 1500)
		{
			temp_eye_time_flag = 0;
			led_eye_on();
		}
		else if ((curr_time - eye_time) >= 1500 && (curr_time - eye_time) < 3000)
		{
			led_eye_off();
		}
		else if ((curr_time - eye_time) >= 3000 && (curr_time - eye_time) < 4500)
		{
			led_eye_on();
		}else if ((curr_time - eye_time) >= 4500 && (curr_time - eye_time) < 7500)
		{
			led_eye_off();
		}
		else if ((curr_time - eye_time) >= 7500 && (curr_time - eye_time) < 9000)
		{
			led_eye_on();
		}else if ((curr_time - eye_time) >= 9000 && (curr_time - eye_time) < 17000)
		{
			led_eye_off();
		}
		else
		{
			temp_eye_time_flag = 1;
			led_eye_on();
		}
	}
}

static void ear_led_flicker_status(int ear_led_event, int ear_led_flicker_switch, int ear_time_flag)
{
	static long int ear_time = 0;
	long int curr_time = 0;
	static int temp_ear_time_flag = 1;

	//temp_ear_time_flag = ear_time_flag;
	
	if (ear_led_event == APP_EVENT_LED_CTRL_EAR_500_MS_FLICKER && ear_led_flicker_switch == EAR_LED_FLICKER_ON)
	{
		if (temp_ear_time_flag == 1)
		{
			ear_time = get_time_of_day();
		}
		curr_time = get_time_of_day();
		
		if ((curr_time - ear_time) >= 0 && (curr_time - ear_time) < 500)
		{
			temp_ear_time_flag = 0;
			led_ear_on();
		}
		else if ((curr_time - ear_time) >= 500 && (curr_time - ear_time) < 1000)
		{
			led_ear_off();
		}
		else
		{
			temp_ear_time_flag = 1;
			led_ear_on();
		}
	}
}

static void led_event_process()
{
}

static void led_event_callback(void *app, APP_EVENT_MSG_t *msg)
{
	static uint32_t temp_eye_led_event = APP_EVENT_LED_CTRL_IDLE;
	static uint32_t temp_ear_led_event = APP_EVENT_LED_CTRL_IDLE;
	static int eye_led_flicker_switch = EYE_LED_FLICKER_OFF;
	static int ear_led_flicker_switch = EAR_LED_FLICKER_OFF;
	static int eye_time_flag = 0;
	static int ear_time_flag = 0;
	static int charging_state = 0;
	
	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTING:
		case APP_EVENT_WIFI_CONFIG:
		{//正在连接网络
			app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
			app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_500_MS_FLICKER, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		case APP_EVENT_WIFI_CONNECT_FAIL:			
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			if (charging_state == 1)
			{
				app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_OFF, NULL, 0);
				app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_OFF, NULL, 0);
			}
			else
			{
				app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
				app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			}
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			eye_led_flicker_status(temp_eye_led_event, eye_led_flicker_switch, eye_time_flag);
			ear_led_flicker_status(temp_ear_led_event, ear_led_flicker_switch, ear_time_flag);
			break;
		}
		case APP_EVENT_POWER_CHARGING:
		{
			charging_state = 1;
			break;
		}
		case APP_EVENT_POWER_CHARGING_STOP:
		{
			charging_state = 0;
			break;
		}
		case APP_EVENT_LED_CTRL_EYE_ON:
		{
			eye_led_flicker_switch = EYE_LED_FLICKER_OFF;
			eye_time_flag = 0;
			led_eye_on();
			set_eye_led_state(APP_EVENT_LED_CTRL_EYE_ON);
			break;
		}
		case APP_EVENT_LED_CTRL_EYE_OFF:
		{
			eye_led_flicker_switch = EYE_LED_FLICKER_OFF;
			eye_time_flag = 0;
			led_eye_off();
			set_eye_led_state(APP_EVENT_LED_CTRL_EYE_OFF);
			break;
		}
		case APP_EVENT_LED_CTRL_EYE_300_MS_FLICKER:
		case APP_EVENT_LED_CTRL_EYE_500_MS_FLICKER:
		case APP_EVENT_LED_CTRL_EYE_800_MS_FLICKER:
		case APP_EVENT_LED_CTRL_EYE_1000_MS_FLICKER:
		case APP_EVENT_LED_CTRL_EYE_3000_MS_FLICKER:
		case APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER:
		{
			eye_led_flicker_switch = EYE_LED_FLICKER_ON;
			temp_eye_led_event = msg->event;
			eye_time_flag = 1;
			set_eye_led_state(APP_EVENT_LED_CTRL_EYE_ON);
			break;
		}
		case APP_EVENT_LED_CTRL_EAR_ON:
		{
			ear_led_flicker_switch = EAR_LED_FLICKER_OFF;
			ear_time_flag = 0;
			led_ear_on();
			set_ear_led_state(APP_EVENT_LED_CTRL_EAR_ON);
			break;
		}
		case APP_EVENT_LED_CTRL_EAR_OFF:
		{
			ear_led_flicker_switch = EAR_LED_FLICKER_OFF;
			ear_time_flag = 0;
			led_ear_off();
			set_ear_led_state(APP_EVENT_LED_CTRL_EAR_OFF);
			break;
		}
		case APP_EVENT_LED_CTRL_EAR_500_MS_FLICKER:
		{
			ear_led_flicker_switch = EAR_LED_FLICKER_ON;
			temp_ear_led_event = msg->event;
			ear_time_flag = 1;
			set_ear_led_state(APP_EVENT_LED_CTRL_EAR_500_MS_FLICKER);
			break;
		}
		default:
			break;
	}
}

static void task_led_ctrl_service(void *pv)
{
	APP_OBJECT_t *app = NULL;

	app = app_new(APP_NAME_LED_CTRL_SERVICE);
	
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_LED_CTRL_SERVICE);
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, LOOP_TIMEOUT, led_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, led_event_callback);
		app_add_event(app, APP_EVENT_LED_CTRL_BASE, led_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, led_event_callback);
		app_add_event(app, APP_EVENT_FREE_TALK_BASE, led_event_callback);
		app_add_event(app, APP_EVENT_KEYBOARD_BASE, led_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_LED_CTRL_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t led_ctrl_service_create(int task_priority)
{
	if (g_led_ctrl_service_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_led_ctrl_service_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//申请运行句柄
	g_led_ctrl_service_handle = (LED_CTRL_SERVICE_HANDLE_T *)memory_malloc(sizeof(LED_CTRL_SERVICE_HANDLE_T));
	if (g_led_ctrl_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc g_led_ctrl_service_handle failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_led_ctrl_service_handle, 0, sizeof(LED_CTRL_SERVICE_HANDLE_T));

	//初始化参数
	SEMPHR_CREATE_LOCK(g_led_ctrl_service_handle->lock);
	if (g_led_ctrl_service_handle->lock == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_led_ctrl_service_handle->lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	//创建运行线程
    if (!task_thread_create(task_led_ctrl_service,
                    "task_led_ctrl_service",
                    APP_NAME_LED_CTRL_SERVICE_STACK_SIZE,
                    NULL,
                    task_priority)) {

        DEBUG_LOGE(LOG_TAG, "ERROR creating task_led_ctrl_service task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t led_ctrl_service_delete(void)
{
	return app_send_message(APP_NAME_LED_CTRL_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


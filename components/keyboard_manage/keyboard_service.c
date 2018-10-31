#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <dirent.h>

#include "cJSON.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "player_middleware_interface.h"
#include "speaker_interface.h"
#include "play_list.h"
#include "audio_record_interface.h"
#include "wechat_service.h"
#include "wifi_manage.h"
#include "keyboard_service.h"
#include "sd_music_manage.h"
#include "power_interface.h"
#include "led_ctrl_service.h"
#include "power_manage.h"
#include "keyboard_event.h"
#include "esp_adc_cal.h"

#if AMC_RECORD_PLAYBACK_ENABLE == 1
#include "record_playback.h"
#endif

#include "free_talk.h"

#define LOG_TAG 						"keyboard service"
#define KEYBOARD_SERVICE_QUIT 			-1
#define VOLUME_STEP               		10
#define CHAT_OPEN						1
#define CHAT_CLOSE						0

typedef struct KEYBOARD_SERVICE_HANDLE_T
{
	bool is_wifi_connected;
	void *lock;
	esp_adc_cal_characteristics_t adc_chars;
}KEYBOARD_SERVICE_HANDLE_T;

static KEYBOARD_SERVICE_HANDLE_T *g_keyboard_service_handle = NULL;
KEYBOARD_OBJ_T *g_keyboard_obj = NULL;

void adc_key_init(void)
{
    //Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC_CHANNEL_3, ADC_ATTEN_DB_11);

    //Characterize ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &g_keyboard_service_handle->adc_chars);
}

void keyboard_init(void)
{
	KEYBOARD_CFG_T keyboard_cfg = {
		.key_num        = 7,   //按键数量	
		.fault_tol      = 80, //容错值
		.scan_cycle     = 40 , //扫描周期ms
		.longpress_time = 800, //长按时间ms
		
		.keyboard[0] = {//空闲状态
			.adc_value = 2565,
		},
		.keyboard[1] = {//按键K1
			.adc_value   = 65,
			.push        = KEY_EVENT_WECHAT_START,
			.release     = KEY_EVENT_WECHAT_STOP,
		},
		.keyboard[2] = {//按键K2
			.adc_value   = 475,
#if FREE_TALK_CONTINUE_MODE
			.sigleclick  = KEY_EVENT_FREE_TALK_CONTINUE,
#else
			.push        = KEY_EVENT_CHAT_START,
			.release     = KEY_EVENT_CHAT_STOP,
#endif
			.longpress   = KEY_EVENT_CHILD_LOCK,
		},
		.keyboard[3] = {//按键K3
			.adc_value   = 850,
			.longpress   = KEY_EVENT_NEXT,
			.sigleclick  = KEY_EVENT_VOL_UP,
		},
		.keyboard[4] = {//按键K4
			.adc_value   = 1235,
			.longpress   = KEY_EVENT_WIFI_SETTING,
			.sigleclick  = KEY_EVENT_PLAY_PAUSE,
		},
		.keyboard[5] = {//按键K5
			.adc_value   = 1605,
			.longpress   = KEY_EVENT_PREV,
			.sigleclick  = KEY_EVENT_VOL_DOWN,
		},
		.keyboard[6] = {//按键K6
			.adc_value   = 1980,				
			.longpress   = KEY_EVENT_HEAD_LED_SWITCH,
			.sigleclick  = KEY_EVENT_SDCARD,
		},
		.keyboard[7] = {//按键K7
			.adc_value   = 2325,
#if TRANSLATE_CONTINUE_MODE
			.sigleclick  = KEY_EVENT_TRANSLATE,
#else
#endif
			.longpress   = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
		},
	};

	g_keyboard_obj = keyboard_create(&keyboard_cfg);
	
	if (g_keyboard_obj == NULL) 
	{
		DEBUG_LOGE("KEYBOARD", "keyboard_create fail !");
		return;
	}
	adc_key_init();
}

bool check_key_event_value(KEY_EVENT_t key_event)
{
	bool ret = true;
	
	switch (key_event)
	{
		case KEY_EVENT_VOL_UP:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_VOL_UP");
			break;
		}
		case KEY_EVENT_VOL_DOWN:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_VOL_DOWN");
			break;
		}
		case KEY_EVENT_PREV:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_PREV");
			break;
		}
		case KEY_EVENT_NEXT:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_NEXT");
			break;
		}
		case KEY_EVENT_PLAY_PAUSE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_PLAY_PAUSE");
			break;
		}
		case KEY_EVENT_WIFI_SETTING:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WIFI_SETTING");
			break;
		}
		case KEY_EVENT_SDCARD:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_SDCARD");
			break;
		}
		case KEY_EVENT_HEAD_LED_SWITCH:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_HEAD_LED_SWITCH");
			break;
		}
#if WECHAT_CONTINUE_MODE
		case KEY_EVENT_TRANSLATE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_TRANSLATE");
			break;
		}
#else
		case KEY_EVENT_CH2EN_START:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CH2EN_START");
			break;
		}
		case KEY_EVENT_CH2EN_STOP:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CH2EN_STOP");
			break;
		}
		case KEY_EVENT_EN2CH_START:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_EN2CH_START");
			break;
		}
		case KEY_EVENT_EN2CH_STOP:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_EN2CH_STOP");
			break;
		}
#endif
		case KEY_EVENT_WECHAT_START:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_START");
			break;
		}
		case KEY_EVENT_WECHAT_STOP:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_STOP");
			break;
		}
#if FREE_TALK_CONTINUE_MODE
		case KEY_EVENT_FREE_TALK_CONTINUE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_FREE_TALK_CONTINUE");
			break;
		}
#else
		case KEY_EVENT_CHAT_START:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_START");
			break;
		}
		case KEY_EVENT_CHAT_STOP:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_STOP");
			break;
		}
#endif
		case KEY_EVENT_CHILD_LOCK:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHILD_LOCK");
			break;
		}
		case KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY");
			break;
		}
		default:
			//DEBUG_LOGE(LOG_TAG, "unknow key num %d", key_event);
			ret = false;
			break;
	}

	return ret;
}

int keyborad_key_cancle(void)
{
	static uint32_t key_press_time = 0;
	uint32_t time_now = time(NULL);
	
	if (abs(key_press_time - time_now) < 2)
	{
		key_press_time = time_now;
		return 1;
	}
	else
	{
		key_press_time = time_now;
		return 0;
	}
}

static void set_wifi_status(bool status)
{
	if (g_keyboard_service_handle == NULL)
	{
		return;
	}
	
	g_keyboard_service_handle->is_wifi_connected = status;

	return;
}

static bool get_wifi_status(void)
{
	if (g_keyboard_service_handle == NULL)
	{
		return false;
	}
	
	return g_keyboard_service_handle->is_wifi_connected;
}

static void adc_key_detect(void)
{	
	int key_event = 0;
	int key_status = 0;
	uint32_t adc_reading = 0;
	
    for (int i = 0; i < 30; i++) 
	{
	    adc_reading += adc1_get_raw((adc1_channel_t)ADC_CHANNEL_3);
    }	
    adc_reading /= 30;
	
	//DEBUG_LOGE(LOG_TAG, "adc_value[%d]", adc_reading);

	key_event = adc_keyboard_process(g_keyboard_obj, adc_reading, NULL);
	if (key_event > 0)
	{
		app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
	}
}

static void key_event_process(KEY_EVENT_t key_event)
{	
	static int wechat_flag = 0;
	static int wifi_status = WIFI_MANAGE_STATUS_STA_DISCONNECTED;
	
	switch (key_event)
	{
#if AMC_RECORD_PLAYBACK_ENABLE
		case KEY_EVENT_WECHAT_START:
		{
			record_playback_start();
			break;
		}
		case KEY_EVENT_WECHAT_STOP:
		{
			record_playback_stop();
			break;
		}
#else
		case KEY_EVENT_WECHAT_START:
		{//微聊
			if (keyborad_key_cancle())
			{
				break;
			}

			if (!get_wifi_status())
			{
				audio_play_tone_mem(FLASH_MUSIC_TONE_NO_NET, TERMINATION_TYPE_NOW);
				break;
			}

			wechat_flag = 1;
			update_power_mng_sleep_time();
			free_talk_terminated();
			
			audio_play_pause();
			wechat_record_start();
			break;
		}
		case KEY_EVENT_WECHAT_STOP:
		{//微聊
			if (!get_wifi_status())
			{
				break;
			}

			wechat_flag = 0;
			wechat_record_stop();
			break;
		}
#endif 
		case KEY_EVENT_VOL_DOWN:
		{//音量减
			int8_t volume = 0;

			update_power_mng_sleep_time();
			get_volume(&volume);
			if (volume > 0)
			{
				set_volume(volume-10);
			}
			else
			{
				set_volume(0);
			}
			DEBUG_LOGI(LOG_TAG, "vol down[%d]", volume);
			if (!is_audio_playing())
			{
				audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
			}
			break;
		}
		case KEY_EVENT_VOL_UP:
		{//音量加
			int8_t volume = 0;

			update_power_mng_sleep_time();
			get_volume(&volume);
			if (volume < 100)
			{
				set_volume(volume+10);
			}
			else
			{
				set_volume(100);
			}
			DEBUG_LOGI(LOG_TAG, "vol up[%d]", volume);
			if (!is_audio_playing())
			{
				audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
			}
			break;
		}
		case KEY_EVENT_PREV:
		{//上一曲
			update_power_mng_sleep_time();
			free_talk_terminated();
			audio_play_tone_mem(FLASH_MUSIC_PREV_SONG, TERMINATION_TYPE_NOW);
			task_thread_sleep(1000);
			playlist_prev();
			break;
		}
		case KEY_EVENT_NEXT:
		{//下一曲
			update_power_mng_sleep_time();
			free_talk_terminated();
			audio_play_tone_mem(FLASH_MUSIC_NEXT_SONG, TERMINATION_TYPE_NOW);
			task_thread_sleep(1000);
			playlist_next();
			break;
		}
#if TRANSLATE_CONTINUE_MODE
		case KEY_EVENT_TRANSLATE:
		{
			break;
		}
#else
		case KEY_EVENT_CH2EN_START:
		{//中译英开始
			if (!get_wifi_status())
			{
				audio_play_tone_mem(FLASH_MUSIC_TONE_NO_NET, TERMINATION_TYPE_NOW);
				break;
			}
			update_power_mng_sleep_time();
			audio_play_pause();
			free_talk_terminated();
			//播放提示音
			audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
			task_thread_sleep(800);
			translate_start(FREE_TALK_MODE_CH2EN_MANUAL_SINGLE);
			break;
		}
		case KEY_EVENT_CH2EN_STOP:
		{//中译英结束
			if (!get_wifi_status())
			{
				break;
			}
			audio_play_tone_mem(FLASH_MUSIC_MSG_SEND, TERMINATION_TYPE_NOW);
			translate_stop();
			break;
		}
		case KEY_EVENT_EN2CH_START:
		{//英译中开始
			if (!get_wifi_status())
			{
				audio_play_tone_mem(FLASH_MUSIC_TONE_NO_NET, TERMINATION_TYPE_NOW);
				break;
			}
			update_power_mng_sleep_time();
			free_talk_terminated();
			//播放提示音
			audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
			task_thread_sleep(800);
			audio_play_pause();
			translate_start(FREE_TALK_MODE_EN2CH_MANUAL_SINGLE);
			break;
		}
		case KEY_EVENT_EN2CH_STOP:
		{//英译中结束
			if (!get_wifi_status())
			{
				break;
			}
			audio_play_tone_mem(FLASH_MUSIC_MSG_SEND, TERMINATION_TYPE_NOW);
			translate_stop();
			break;
		}
#endif
		case KEY_EVENT_SDCARD:
		{//播放SD卡
			update_power_mng_sleep_time();
			free_talk_terminated();
			sd_music_play_start();
			break;
		}
		case KEY_EVENT_PLAY_PAUSE:	
		{//暂停/播放			
			if (keyborad_key_cancle())
			{
				break;
			}
			update_power_mng_sleep_time();
			free_talk_terminated();
			if (is_audio_playing())
			{
				audio_play_pause();				
				audio_play_tone_mem(FLASH_MUSIC_MUSIC_PAUSE, TERMINATION_TYPE_NOW);
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_MUSIC_PLAY, TERMINATION_TYPE_NOW);
				task_thread_sleep(2000);
				audio_play_resume();	
			}
			break;
		}
		case KEY_EVENT_WIFI_SETTING:	
		{//配网
			update_power_mng_sleep_time();
			free_talk_terminated();
			audio_play_pause();
			wifi_manage_start_smartconfig();
			break;
		}
#if FREE_TALK_CONTINUE_MODE
		case KEY_EVENT_FREE_TALK_CONTINUE:
		{
			if (!get_wifi_status())
			{
				audio_play_tone_mem(FLASH_MUSIC_TONE_NO_NET, TERMINATION_TYPE_NOW);
				break;
			}

			update_power_mng_sleep_time();
			app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_MSG_TO_ALL, APP_EVENT_KEYWORD_WAKEUP_NOTIFY, NULL, 0);
			break;
		}
#else
		case KEY_EVENT_CHAT_START:
		{
			break;
		}
		case KEY_EVENT_CHAT_STOP:
		{
			break;
		}
#endif
		case KEY_EVENT_CHILD_LOCK:
		{
			break;
		}
		case KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY:
		{
			break;
		}
		default:
			break;
	}
}

static void keyboard_event_callback(void *app, APP_EVENT_MSG_t *msg)
{
	static bool ota_status = true;
	static bool led_button_status = false;
	static int led_button_time = 0;
	
	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED:
		{ 
			set_wifi_status(true);
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			set_wifi_status(false);
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{	
			if(ota_status == false)
			{
				break;
			}
			
			adc_key_detect();
			break;
		}
		case APP_EVENT_OTA_START:
		{
			ota_status = false;
			break;
		}
		case APP_EVENT_OTA_STOP:
		{
			ota_status = true;
			break;
		}
		case APP_EVENT_KEYBOARD_EVENT:
		{	
			if(ota_status == false)
			{
				break;
			}

			if (check_key_event_value(*(KEY_EVENT_t *)msg->event_data))
			{
				app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_MEMO_MANAGE, APP_EVENT_MEMO_CANCLE, NULL, 0);
				key_event_process(*(KEY_EVENT_t *)msg->event_data);
			}

			break;
		}
		default:
			break;
	}
}

static void task_keyboard_service(void *pv)
{
	APP_OBJECT_t *app = NULL;

	task_thread_sleep(5*1000);
	keyboard_init();
	
	app = app_new(APP_NAME_KEYBOARD_SERVICE);

	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_KEYBOARD_SERVICE);
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 40, keyboard_event_callback);
		app_add_event(app, APP_EVENT_OTA_BASE, keyboard_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, keyboard_event_callback);
		app_add_event(app, APP_EVENT_KEYBOARD_BASE, keyboard_event_callback);
		app_add_event(app, APP_EVENT_LED_CTRL_BASE, keyboard_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_KEYBOARD_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t keyboard_service_create(int task_priority)
{		
	if (g_keyboard_service_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_keyboard_service_handle already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//申请运行句柄
	g_keyboard_service_handle = (KEYBOARD_SERVICE_HANDLE_T *)memory_malloc(sizeof(KEYBOARD_SERVICE_HANDLE_T));
	if (g_keyboard_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc g_keyboard_service_handle failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_keyboard_service_handle, 0, sizeof(KEYBOARD_SERVICE_HANDLE_T));

	//初始化参数
	SEMPHR_CREATE_LOCK(g_keyboard_service_handle->lock);
	if (g_keyboard_service_handle->lock == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_power_manage->lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//创建运行线程
    if (!task_thread_create(task_keyboard_service,
                    "task_keyboard_service",
                    APP_NAME_KEYBOARD_SERVICE_STACK_SIZE,
                    NULL,
                    task_priority)) {

        DEBUG_LOGE(LOG_TAG, "ERROR creating task_key_event_process task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t keyboard_service_delete(void)
{
	return app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


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
KEYBOARD_OBJ_T *g_keyboard_obj1 = NULL;
//KEYBOARD_OBJ_T *g_keyboard_obj2 = NULL;

void adc_key_init(void)
{
    //Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    //adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_DB_11);
	adc1_config_channel_atten(ADC_CHANNEL_3, ADC_ATTEN_DB_11);

    //Characterize ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &g_keyboard_service_handle->adc_chars);
}

void keyboard_init(void)
{
	KEYBOARD_CFG_T keyboard_cfg1 = {
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
			.sigleclick  = KEY_EVENT_CHAT_START,
			//.longpress   = KEY_EVENT_CHILD_LOCK,
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
			//.longpress   = KEY_EVENT_HEAD_LED_SWITCH,
			.sigleclick  = KEY_EVENT_SDCARD,
		},
		.keyboard[7] = {//按键K7
			.adc_value   = 2325,
			//.sigleclick  = KEY_EVENT_TRANSLATE,
			//.longpress   = KEY_EVENT_OTHER_KEY_CHILD_LOCK_PLAY,
		},
	};
#if 0			
	KEYBOARD_CFG_T keyboard_cfg2 = {
		.key_num		 = 5,	//按键数量
		.fault_tol		 = 100, //容错值
		.scan_cycle 	 = 40 , //扫描周期ms
		.longpress_time  = 800, //长按时间ms
		
		.keyboard[0] = {//空闲状态
			.adc_value	 = 2547,
		},
	
		.keyboard[1] = {//按键6
			.adc_value	= 40,	
			.longpress	= KEY_EVENT_LED_OPEN,
		},
		.keyboard[2] = {//按键7
			.adc_value	= 451,	
			.sigleclick = KEY_EVENT_CYCLE_CLOUD,
			.longpress	= KEY_EVENT_CYCLE_PRIVATE, 
		},
		.keyboard[3] = {//按键8
			.adc_value	= 833,	
			.push		= KEY_EVENT_WECHAT_START,
			.release	= KEY_EVENT_WECHAT_STOP,
		},
		.keyboard[4] = {//按键9
			.adc_value	= 1200, 
			.sigleclick = KEY_EVENT_CHOOSE_MODE,
		},
		.keyboard[5] = {//按键10
			.adc_value	= 1587,
			.push		= KEY_EVENT_EN2CH_START,
			.release	= KEY_EVENT_EN2CH_STOP,
		},
	};
#endif
	g_keyboard_obj1 = keyboard_create(&keyboard_cfg1);
	//g_keyboard_obj2 = keyboard_create(&keyboard_cfg2);
#if 0
	if (g_keyboard_obj1 == NULL || g_keyboard_obj2 == NULL) 
	{
		DEBUG_LOGE("KEYBOARD", "keyboard_create fail !");
		return;
	}
#endif	
	if (g_keyboard_obj1 == NULL) 
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
		case KEY_EVENT_WIFI_SETTING:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WIFI_SETTING");
			break;
		}
		case KEY_EVENT_WIFI_STATUS:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WIFI_STATUS");
			break;
		}
		case KEY_EVENT_SDCARD:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_SDCARD");
			break;
		}
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
		case KEY_EVENT_WECHAT_PLAY:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_WECHAT_PLAY");
			break;
		}
		case KEY_EVENT_PLAY_PAUSE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_PLAY_PAUSE");
			break;
		}
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
		case KEY_EVENT_LED_OPEN:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_STOP");
			break;
		}
		case KEY_EVENT_LED_CLOSE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_STOP");
			break;
		}
		case KEY_EVENT_CYCLE_CLOUD:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_STOP");
			break;
		}
		case KEY_EVENT_CYCLE_PRIVATE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHAT_STOP");
			break;
		}
		case KEY_EVENT_CHOOSE_MODE:
		{
			DEBUG_LOGE(LOG_TAG, "KEY_EVENT_CHOOSE_MODE");
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

static void touch_jaw_play_audio()
{
	static int index = 2;
	
	switch (index)
	{
		case 1:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_001, TERMINATION_TYPE_NOW);
			break;
		}
		case 2:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_002, TERMINATION_TYPE_NOW);
			break;
		}
		case 3:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_003, TERMINATION_TYPE_NOW);
			break;
		}
		case 4:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_004, TERMINATION_TYPE_NOW);
			break;
		}
		case 5:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_005, TERMINATION_TYPE_NOW);
			break;
		}
		case 6:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_006, TERMINATION_TYPE_NOW);
			break;
		}
		case 7:
		{
			audio_play_tone_mem(FLASH_MUSIC_TOUCH_007, TERMINATION_TYPE_NOW);
			break;
		}
		default:
			break;
	}
	index++;
	if (index > 7)
	{
		index = 2;
	}
}

static void adc_key_detect(void)
{	
	int key_event = 0;
	int key_status = 0;
	uint32_t adc_reading1 = 0;
	uint32_t adc_reading2 = 0;
	
    for (int i = 0; i < 30; i++) 
	{
	    //adc_reading1 += adc1_get_raw((adc1_channel_t)ADC_CHANNEL_0);
	    adc_reading2 += adc1_get_raw((adc1_channel_t)ADC_CHANNEL_3);
    }	
    //adc_reading1 /= 30;
    adc_reading2 /= 30;
	
	//DEBUG_LOGE(LOG_TAG, "adc_value1[%d]2[%d]", adc_reading1, adc_reading2);

	key_event = adc_keyboard_process(g_keyboard_obj1, adc_reading2, NULL);
	if (key_event > 0)
	{
		app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
	}
#if 0
	key_event = adc_keyboard_process(g_keyboard_obj2, adc_reading2, NULL);
	if (key_event > 0)
	{
		app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
	}
#endif
}

static void key_event_process(KEY_EVENT_t key_event)
{	
	static int wechat_flag = 0;
	static int wifi_status = WIFI_MANAGE_STATUS_STA_DISCONNECTED;
	
#if 1
	switch (key_event)
	{
#if AMC_RECORD_PLAYBACK_ENABLE == 1
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
		
#if 0
		case DEVICE_NOTIFY_KEY_LED_PUSH:
		{//聊天
			//sd_music_manage_stop_first_play();
			if (keyborad_key_cancle())
			{
				break;
			}
			
			if (get_wifi_status())
			{	
				FREE_TALK_CONFIG_t free_config = {0};

#if 0
				free_config.mode = FREE_TALK_MODE_MANUAL_SINGLE;
				free_config.single_talk_max_ms = 10*1000;
#else
				free_config.mode = FREE_TALK_MODE_AUTO_SINGLE;
				free_config.single_talk_max_ms = 10*1000;
				free_config.vad_detect_sensitivity = 1;
#endif				
				audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, AUDIO_TERM_TYPE_NOW);
				task_thread_sleep(500);
				free_talk_start(&free_config);
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE, AUDIO_TERM_TYPE_NOW);
			}
			break;
		}
		case DEVICE_NOTIFY_KEY_LED_RELEASE:
		{//聊天
			//free_talk_stop();
			break;
		}

		case KEY_EVENT_TOUCH_HEAD:	//触摸头部
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
		case KEY_EVENT_TOUCH_JAW:	//触摸下巴
		{	
			update_power_mng_sleep_time();
			free_talk_terminated();
			if (wechat_flag == 0)
			{
				touch_jaw_play_audio();
			}
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
		case KEY_EVENT_SDCARD:
		{//播放SD卡
			update_power_mng_sleep_time();
			free_talk_terminated();
			sd_music_play_start();
			break;
		}
/*
		case DEVICE_NOTIFY_KEY_PREV_TAP:	
		{//上一曲
			if (keyborad_key_cancle())
			{
				break;
			}
		#if MODULE_FREE_TALK
			if (is_free_talk_running())
			{
				free_talk_stop();//关闭录音
			}
		#endif	
			auto_play_pause();
			vTaskDelay(100);
			//EspAudioPause();
			player_mdware_play_tone(FLASH_MUSIC_PREV_SONG);
			vTaskDelay(2*1000);
			auto_play_prev();
			break;
		}
		case DEVICE_NOTIFY_KEY_NEXT_TAP:	
		{//下一曲
			if (keyborad_key_cancle())
			{
				break;
			}
		#if MODULE_FREE_TALK	
			if (is_free_talk_running())
			{
				free_talk_stop();//关闭录音
			}
		#endif	
			auto_play_pause();
			vTaskDelay(100);
			player_mdware_play_tone(FLASH_MUSIC_NEXT_SONG);
			vTaskDelay(2*1000);
			auto_play_next();
			break;
		}
		case DEVICE_NOTIFY_KEY_MENU_TAP:	
		{//切换TF卡目录
			sd_music_manage_stop_first_play();
			if (keyborad_key_cancle())
			{
				break;
			}
		#if MODULE_FREE_TALK
			if (is_free_talk_running())
			{
				free_talk_stop();//关闭录音
			}
		#endif	
			sd_music_start();
			test_playlist();
			break;
		}
*/
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
		case KEY_EVENT_WIFI_STATUS:
		{//提示网络状态
			wifi_status = get_wifi_manage_status();
			if(wifi_status == WIFI_MANAGE_STATUS_STA_CONNECTED)
			{
				audio_play_tone_mem(FLASH_MUSIC_WIFI_CONNECTED, TERMINATION_TYPE_NOW);
			}
			else if(wifi_status == WIFI_MANAGE_STATUS_STA_CONNECTING)
			{
				audio_play_tone_mem(FLASH_MUSIC_WIFI_CONNECTING, TERMINATION_TYPE_NOW);
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_WIFI_DISCONNECTED, TERMINATION_TYPE_NOW);
			}
		}
	/*
		case DEVICE_NOTIFY_KEY_STORY_TAP:
		{//迪士尼故事
			sd_music_manage_stop_first_play();
			if (keyborad_key_cancle())
			{
				break;
			}
			
			auto_play_stop();//停止自动播放
			vTaskDelay(200);
		#if MODULE_FREE_TALK
			if (is_free_talk_running())
			{
				free_talk_stop();//关闭录音
			}
		#endif	
			player_mdware_play_tone(FLASH_MUSIC_DISNEY_STORY);
			vTaskDelay(2500);
			flash_music_start();
			set_sd_music_stop_state();
			break;
		}
		case DEVICE_NOTIFY_KEY_STORY_LONGPRESSED:
		{//七彩灯
			switch_color_led();
			break;
		}
*/
		default:
			break;
	}
#endif
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


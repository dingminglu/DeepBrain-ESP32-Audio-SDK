#include "app_config.h"
#include "userconfig.h"
#include "ota_update.h"
#include "ota_service.h"
#include "player_middleware_interface.h"
#include "partitions_config.h"
#include "device_params_manage.h"
#include "play_list.h"
#include "esp_spi_flash.h"
#include "speaker_interface.h"

#define PRINT_TAG "OTA_SERVICE"

static void ota_service_callback(void *app, APP_EVENT_MSG_t *msg)
{
	OTA_UPDATE_MODE_T ota_mode = OTA_UPDATE_MODE_FORMAL;
	char str_fw_url[256] = {0};

	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED:
		{
			task_thread_sleep(8*1000);
			DEBUG_LOGW(PRINT_TAG, "ota_update_check_fw_version begin");
			memset(str_fw_url, 0, sizeof(str_fw_url));

		    get_flash_cfg(FLASH_CFG_OTA_MODE, &ota_mode);
			
			if (ota_mode == OTA_UPDATE_MODE_FORMAL)
			{
				if (ota_update_check_fw_version(OTA_UPDATE_SERVER_URL, str_fw_url, sizeof(str_fw_url)) == OTA_NONEED_UPDATE_FW)
				{
					break;
				}
			}
			else
			{
				if (ota_update_check_fw_version(OTA_UPDATE_SERVER_URL_TEST, str_fw_url, sizeof(str_fw_url)) == OTA_NONEED_UPDATE_FW)
				{
					break;
				}
			}

			app_send_message(APP_NAME_OTA_SERVICE, APP_MSG_TO_ALL, APP_EVENT_OTA_START, NULL, 0);
			audio_play_stop();
			playlist_clear();
			task_thread_sleep(200);
			audio_play_tone_mem(FLASH_MUSIC_EQUIPMENT_UPGRADING, TERMINATION_TYPE_NOW);
			task_thread_sleep(5000);
			DEBUG_LOGI(PRINT_TAG, "ota_update_from_wifi begin");
			
			if (ota_update_from_wifi(str_fw_url) == OTA_SUCCESS)
			{
				audio_play_tone_mem(FLASH_MUSIC_UPGRADE_COMPLETE, TERMINATION_TYPE_NOW);
				task_thread_sleep(3000);
				DEBUG_LOGE(PRINT_TAG, "ota_update_from_wifi success");
				set_mute(true);
				esp_restart();
			}
			else
			{	
				app_send_message(APP_NAME_OTA_SERVICE, APP_MSG_TO_ALL, APP_EVENT_OTA_STOP, NULL, 0);
				audio_play_tone_mem(FLASH_MUSIC_UPDATE_FAIL, TERMINATION_TYPE_NOW);
				task_thread_sleep(3000);
				DEBUG_LOGE(PRINT_TAG, "ota_update_from_wifi failed");
			}
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

static void task_ota_service(void *pv)
{
	APP_OBJECT_t *app = NULL;

	app = app_new(APP_NAME_OTA_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "new app[%s] failed, out of memory", APP_NAME_OTA_SERVICE);
		task_thread_exit();
	}
	else
	{
		app_add_event(app, APP_EVENT_DEFAULT_BASE, ota_service_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, ota_service_callback);
		DEBUG_LOGE(PRINT_TAG, "%s create success", APP_NAME_OTA_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

void ota_service_create(int task_priority)
{
    if (!task_thread_create(task_ota_service,
            "task_ota_service",
            APP_NAME_OTA_SERVICE_STACK_SIZE,
            NULL,
            task_priority)) 
    {
        DEBUG_LOGE(PRINT_TAG, "create task_ota_service failed");
    }
}


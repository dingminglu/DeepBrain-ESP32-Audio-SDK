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

#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#include "sdcard_interface.h"
#include "userconfig.h"

#define LOG_TAG "sdcard interface"

/* sd card detect status */
typedef enum SDCARD_DETECT_STATUS_t
{
	SDCARD_DETECT_STATUS_INSERTED = 0,
	SDCARD_DETECT_STATUS_MOUNTED,
	SDCARD_DETECT_STATUS_UNMOUNTED,
	SDCARD_DETECT_STATUS_PULL_OUT,
}SDCARD_DETECT_STATUS_t;

/* sd card running handle */
typedef struct SDCARD_DETECT_HANDLE_t
{
	xQueueHandle sdcard_detect_queue;
	SDCARD_DETECT_STATUS_t status;
}SDCARD_DETECT_HANDLE_t;

static SDCARD_DETECT_HANDLE_t *g_sdcard_detect_handle = NULL;

bool is_sdcard_inserted(void)
{	
    if (gpio_get_level(SD_CARD_INTR_GPIO))
    {
		return false;
	}

	return true;
}

bool sdcard_mount(const char *basePath)
{
	bool result = true;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
	{
        .format_if_mount_failed = false,
        .max_files = SD_CARD_OPEN_FILE_NUM_MAX
    };

    sdmmc_card_t *card = NULL;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(basePath, &host, &slot_config, &mount_config, &card);
    switch (ret) 
	{
        case ESP_OK:
            // Card has been initialized, print its properties
            sdmmc_card_print_info(stdout, card);
            DEBUG_LOGI(LOG_TAG, "CID name %s!\n", card->cid.name);
			result = true;
            break;
        case ESP_ERR_INVALID_STATE:
            DEBUG_LOGE(LOG_TAG, "File system already mounted");
			result = true;
            break;
        case ESP_FAIL:
            DEBUG_LOGE(LOG_TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
			result = false;
            break;
        default:
            DEBUG_LOGE(LOG_TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
			result = false;
            break;
    }

    return result;
}

bool sdcard_unmount(void)
{
    if (esp_vfs_fat_sdmmc_unmount() == ESP_ERR_INVALID_STATE) 
	{
        DEBUG_LOGE(LOG_TAG, "File system not mounted");
		return false;
    }
	
    return true;
}

static void sdcard_detect_process(
	SDCARD_DETECT_HANDLE_t *handle)
{
	switch (handle->status)
	{
		case SDCARD_DETECT_STATUS_PULL_OUT:
		case SDCARD_DETECT_STATUS_INSERTED:
		case SDCARD_DETECT_STATUS_UNMOUNTED:
		{
			if (is_sdcard_inserted())
			{
				handle->status = SDCARD_DETECT_STATUS_INSERTED;
				if (sdcard_mount("/sdcard"))
				{
					DEBUG_LOGI(LOG_TAG, "sdcard_mount success");
					handle->status = SDCARD_DETECT_STATUS_MOUNTED;
				}
				else
				{
					if (!sdcard_unmount())
					{
						DEBUG_LOGE(LOG_TAG, "sdcard_unmount failed");
					}
					handle->status = SDCARD_DETECT_STATUS_UNMOUNTED;
				}
			}
			else
			{
				handle->status = SDCARD_DETECT_STATUS_PULL_OUT;
			}
			break;
		}
		case SDCARD_DETECT_STATUS_MOUNTED:
		{
			if (!is_sdcard_inserted())
			{
				if (!sdcard_unmount())
				{
					DEBUG_LOGE(LOG_TAG, "sdcard_unmount failed");
				}
				handle->status = SDCARD_DETECT_STATUS_PULL_OUT;
			}
			break;
		}
		default:
			break;
	}
}

static void sdcard_detect_task(void *pv)
{
    int trigger = 0;
	SDCARD_DETECT_HANDLE_t *handle = (SDCARD_DETECT_HANDLE_t *)pv;
	
	sdcard_detect_process(handle);
	
    while (1) 
	{
        if (xQueuePeek(handle->sdcard_detect_queue, &trigger, portMAX_DELAY)) 
		{
			sdcard_detect_process(handle);
			xQueueReceive(handle->sdcard_detect_queue, &trigger, portMAX_DELAY);
        }
    }

	task_thread_exit();
}


static void IRAM_ATTR sd_card_gpio_intr_handler(void *arg)
{
	xQueueHandle queue = (xQueueHandle) arg;
    gpio_num_t gpioNum = (gpio_num_t) SD_CARD_INTR_GPIO;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
    if (queue)
    {
        xQueueSendFromISR(queue, &gpioNum, &xHigherPriorityTaskWoken);
    }
	
    if (xHigherPriorityTaskWoken) 
	{
        portYIELD_FROM_ISR();
    }
}

bool sd_card_init(void)
{
    gpio_config_t  io_conf;

	if (g_sdcard_detect_handle == NULL)
	{
		g_sdcard_detect_handle = (SDCARD_DETECT_HANDLE_t *)memory_malloc(sizeof(SDCARD_DETECT_HANDLE_t));
		if (g_sdcard_detect_handle == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "memory_malloc g_sdcard_detect_handle failed");
			return false;
		}
		memset(g_sdcard_detect_handle, 0, sizeof(SDCARD_DETECT_HANDLE_t));
		g_sdcard_detect_handle->status = SDCARD_DETECT_STATUS_PULL_OUT;
	}

	if (g_sdcard_detect_handle->sdcard_detect_queue == NULL)
	{
		g_sdcard_detect_handle->sdcard_detect_queue = xQueueCreate(1, sizeof(int));
    	configASSERT(g_sdcard_detect_handle->sdcard_detect_queue);
	}
	
    memset(&io_conf, 0, sizeof(io_conf));
#if IDF_3_0
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
#else
    io_conf.intr_type = GPIO_PIN_INTR_ANYEGDE;
#endif
    io_conf.pin_bit_mask = SD_CARD_INTR_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(SD_CARD_INTR_GPIO, sd_card_gpio_intr_handler, (void *)g_sdcard_detect_handle->sdcard_detect_queue);

	if (!task_thread_create(
	 		sdcard_detect_task,
            "sdcard_detect_task",
            1024*3,
            g_sdcard_detect_handle,
            3)) 
    {
        DEBUG_LOGE(LOG_TAG, "task_thread_create sdcard_detect_task failed");
        return false;
    }

	return true;
}


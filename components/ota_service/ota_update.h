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

#ifndef _SD_FIRMWARE_UPDATE_H_
#define _SD_FIRMWARE_UPDATE_H_

#include "esp_ota_ops.h"
#include "esp_partition.h"

typedef enum 
{
	OTA_SUCCESS,
	OTA_INIT_FAIL,
	OTA_SD_FAIL,
	OTA_WIFI_FAIL,
	OTA_NO_SD_CARD,
	OTA_NEED_UPDATE_FW,
	OTA_NONEED_UPDATE_FW,
}OTA_ERROR_NUM_T;

/**
 * ota update from sd
 *
 * @param  _firmware_path
 * @return ota errno
 */
OTA_ERROR_NUM_T ota_update_from_sd(const char *_firmware_path);

/**
 * ota update from wifi
 *
 * @param  _firmware_url
 * @return ota errno
 */
OTA_ERROR_NUM_T ota_update_from_wifi(const char *_firmware_url);

/**
 * ota update check fw version 
 *
 * @param  _server_url,server fw url
 * @param  _out_fw_url,out fw url
 * @param  _out_len,out fw length
 * @return ota errno
 */
OTA_ERROR_NUM_T ota_update_check_fw_version(const char *_server_url, char *_out_fw_url, size_t _out_len);

#endif


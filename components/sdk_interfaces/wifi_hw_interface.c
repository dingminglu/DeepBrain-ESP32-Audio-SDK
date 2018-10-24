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

#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "debug_log_interface.h"

#define TAG_LOG "Esp wifi get mac"

bool get_wifi_mac(uint8_t eth_mac[6])
{
	esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
	
	return true;
}

bool get_wifi_mac_str(
	char* const str_mac_addr, 
	const uint32_t str_len)
{
	uint8_t eth_mac[6] = {0};
	esp_err_t status = ESP_OK;
	
	status = esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
	if(status == ESP_OK)
	{
		snprintf(str_mac_addr, str_len, "%02X%02X%02X%02X%02X%02X", 
				 eth_mac[0],eth_mac[1],eth_mac[2],eth_mac[3],eth_mac[4],eth_mac[5]);
		return true;
	}
	else if(status == ESP_ERR_WIFI_NOT_INIT)
	{
		DEBUG_LOGE(TAG_LOG, "WiFi is not initialized by esp_wifi_init failed, error=0x%x", status);
		return false;
	}
	else if(status == ESP_ERR_WIFI_ARG)
	{
		DEBUG_LOGE(TAG_LOG, "invalid argument failed, error=0x%x", status);
		return false;
	}
	else
	{
		DEBUG_LOGE(TAG_LOG, "invalid interface failed, error=0x%x", status);
		return false;
	}
}
	

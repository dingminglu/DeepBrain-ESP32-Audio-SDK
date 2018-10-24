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
#include "device_info_interface.h"
#include "wifi_hw_interface.h"

/**
 * get device id
 *
 * @param str_device_id,device id string
 * @param str_len,string length
 * @return true/false
 */
bool get_device_id(
	char* const str_device_id, 
	const uint32_t str_len)
{
	return get_wifi_mac_str(str_device_id, str_len);
}


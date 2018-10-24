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

#ifndef WIFI_HW_INTERFACE_H
#define WIFI_HW_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * get wifi hardware mac address, six bytes.
 *
 * @param [out]mac_addr.
 * @return true/false.
 */
bool get_wifi_mac(uint8_t mac_addr[6]);

/**
 * get wifi hardware mac address to string formate.
 *
 * @param [out]str_mac_addr.
 * @param [in]str_len.
 * @return true/false.
 */
bool get_wifi_mac_str(char* const str_mac_addr, const uint32_t str_len);

#ifdef __cplusplus
}
#endif

#endif //WIFI_HW_INTERFACE_H


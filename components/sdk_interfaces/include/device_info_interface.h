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

#ifndef DEVICE_INFO_INTERFACE_H
#define DEVICE_INFO_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * get device unique id
 *
 * @param [out]str_device_id
 * @param [in]str_len, >= 32 bytes
 * @return true/false
 */
bool get_device_id(
	char* const str_device_id, 
	const uint32_t str_len);

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_INFO_INTERFACE_H */


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

#ifndef DCL_UPDATE_DEVICE_INFO_H
#define DCL_UPDATE_DEVICE_INFO_H

#include "dcl_interface.h"
#include "ctypes_interface.h"

#ifdef __cplusplus
 extern "C" {
#endif

DCL_ERROR_CODE_t dcl_update_device_info(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char *firmware_version);

#ifdef __cplusplus
 }
#endif

#endif /* DCL_UPDATE_DEVICE_INFO_H */


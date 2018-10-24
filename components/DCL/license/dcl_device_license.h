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

#ifndef DCL_DEVICE_LICENSE_H
#define DCL_DEVICE_LICENSE_H

#include "dcl_interface.h"
#include "ctypes_interface.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* 设备许可认证返回 */
typedef struct DCL_DEVICE_LICENSE_RESULT_t
{
	uint8_t device_sn[64];				//平台设备序列号
	uint8_t wechat_device_type[64];		//微信公众号ID
	uint8_t wechat_device_id[64];		//微信设备ID
	uint8_t wechat_device_license[256];	//微信设备许可
}DCL_DEVICE_LICENSE_RESULT_t;

DCL_ERROR_CODE_t dcl_get_device_license(
	const DCL_AUTH_PARAMS_t* const input_params,
	const uint8_t *device_sn_prefix,
	const uint8_t *device_chip_id,
	const bool is_wechat_sn,
	DCL_DEVICE_LICENSE_RESULT_t* result);

void unit_test_dcl_get_device_license(void);

#ifdef __cplusplus
 }
#endif

#endif /* DCL_DEVICE_LICENSE_H */


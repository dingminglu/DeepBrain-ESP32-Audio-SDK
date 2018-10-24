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

#ifndef APP_RING_BUFFER_H
#define APP_RING_BUFFER_H

#include "ctypes_interface.h"
#include "app_framework.h"

#ifdef __cplusplus
 extern "C" {
#endif

APP_FRAMEWORK_ERRNO_t app_ring_buffer_new(
	const uint32_t ring_buffer_size, 
	void **ring_buffer);

APP_FRAMEWORK_ERRNO_t app_ring_buffer_delete(void *ring_buffer);

APP_FRAMEWORK_ERRNO_t app_ring_buffer_clear(void *ring_buffer);

// -1 报错 >0 表示成功读取数据内容
int app_ring_buffer_read(
	void *ring_buffer, 
	uint8_t *data, 
	const uint32_t size);

// -1 报错 >0 表示成功读取数据内容
int app_ring_buffer_readn(
	void *ring_buffer, 
	uint8_t *data, 
	const uint32_t size);


// -1 报错 >0 表示成功写入数据内容
int app_ring_buffer_write(
	void *ring_buffer, 
	const uint8_t *data, 
	const uint32_t size);

#ifdef __cplusplus
 }
#endif

#endif /* APP_RING_BUFFER_H */


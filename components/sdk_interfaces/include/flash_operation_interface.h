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

#ifndef FLASH_OPERATION_INTERFACE_H
#define FLASH_OPERATION_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * read flash operation.
 *
 * @param addr[in],flash address.
 * @param data[in].
 * @param data_len[in],data length.
 * @return Whether the operation was successful.
 */
bool flash_op_read(
	const uint32_t addr, 
	uint8_t* const data, 
	const uint32_t data_len);

/**
 * write flash operation.
 *
 * @param addr[in],flash address.
 * @param data[in].
 * @param data_len[in],data length.
 * @return Whether the operation was successful.
 */
bool flash_op_write(
	const uint32_t addr, 
	const uint8_t* const data, 
	const uint32_t data_len);

bool flash_op_erase(
	const uint32_t addr, 
	const uint32_t length);


#ifdef __cplusplus
}
#endif

#endif  //FLASH_OPERATION_INTERFACE_H

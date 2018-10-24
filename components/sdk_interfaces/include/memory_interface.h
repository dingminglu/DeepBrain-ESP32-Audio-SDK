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

#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The memory interface is concerned with the memory malloc and free functions.
 */

/**
 * memory malloc function
 *
 * @param mem_size, the size malloc from memory 
 * @return the memory address
 */
void *memory_malloc(const uint32_t mem_size);

/**
 * memory free function
 *
 * @param mem_addr
 * @return
 */
void memory_free(const void *mem_addr);

#ifdef __cplusplus
}
#endif

#endif  //MEMORY_INTERFACE_H


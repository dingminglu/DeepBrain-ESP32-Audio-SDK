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

#ifndef TASK_THREAD_INTERFACE_H
#define TASK_THREAD_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * create task thread
 *
 * @param task_func,a function pointer,the return value is a pointer
 * @param task_name
 * @param static_size,the amount of memory occupied by the task
 * @param task_params,task required parameters
 * @param task_priority
 * @return Whether the operation was successful
 */
bool task_thread_create(
	void (*task_func)(void *), 
	const char * const task_name,
	const uint32_t static_size,
	void * const task_params,
	const uint32_t task_priority);

/**
 * exit task thread
 *
 * @param none
 * @return none
 */
void task_thread_exit(void);

/**
 * Set the mute of the speaker.
 *
 * @param ms,mount of time,Unit: ms
 * @return Whether the operation was successful
 */
void task_thread_sleep(const uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif  //TASK_THREAD_INTERFACE_H


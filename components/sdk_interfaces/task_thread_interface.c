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

#include "task_thread_interface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define SUPPORT_DUAL_CORE 0

bool task_thread_create(
	void (*task_func)(void *), 
	const char * const task_name,
	const uint32_t static_size,
	void * const task_params,
	const uint32_t task_priority)
{	
#if SUPPORT_DUAL_CORE == 1
	if (xTaskCreate(
		task_func, 
		task_name, 
		static_size, 
		task_params, 
		task_priority, 
		NULL) != pdPASS) 
#else
	if (xTaskCreatePinnedToCore(
		task_func, 
		task_name, 
		static_size, 
		task_params, 
		task_priority, 
		NULL, 0) != pdPASS) 
#endif
    {
        return false;
    }

	return true;
}

void task_thread_exit(void)
{
	vTaskDelete(NULL);
	return;
}

void task_thread_sleep(const uint32_t ms)
{
	vTaskDelay(ms/portTICK_PERIOD_MS);
}


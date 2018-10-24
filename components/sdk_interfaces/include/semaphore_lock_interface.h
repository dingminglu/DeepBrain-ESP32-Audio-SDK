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

#ifndef SEMAPHONE_LOCK_INTERFACE_H
#define SEMAPHONE_LOCK_INTERFACE_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define SEMPHR_CREATE_LOCK(lock) 	do{lock = xSemaphoreCreateMutex();}while(0)
#define SEMPHR_TRY_LOCK(lock) 		do{if (lock != NULL) xSemaphoreTake(lock, portMAX_DELAY);}while(0)
#define SEMPHR_TRY_UNLOCK(lock) 	do{if (lock != NULL) xSemaphoreGive(lock);}while(0)
#define SEMPHR_DELETE_LOCK(lock) 	do{if (lock != NULL) vSemaphoreDelete(lock);}while(0)

#endif /*SEMAPHONE_LOCK_INTERFACE_H*/


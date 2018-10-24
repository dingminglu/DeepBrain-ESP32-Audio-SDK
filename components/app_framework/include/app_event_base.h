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

#ifndef APP_EVENT_BASE_H
#define APP_EVENT_BASE_H

//default event, can't modify
typedef enum APP_EVENT_DEFAULT_t
{
	APP_EVENT_DEFAULT_BASE = 0x00000100,
	APP_EVENT_DEFAULT_LOOP_TIMEOUT,
	APP_EVENT_DEFAULT_EXIT,
}APP_EVENT_DEFAULT_t;

//other event define, can custom by users
#define APP_EVENT_BASE_STEP (0x100)
#define APP_EVENT_BASE_ADDR (APP_EVENT_DEFAULT_BASE + APP_EVENT_BASE_STEP)//0x200
#define APP_EVENT_BASE_NEXT(base) (base + APP_EVENT_BASE_STEP) 

#endif /*APP_EVENT_BASE_H*/

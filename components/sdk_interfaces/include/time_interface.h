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

#ifndef TIME_INTERFACE_H
#define TIME_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * get current time in ms
 *
 * @param void
 * @return current time in ms
 */
uint64_t get_time_of_day(void);

#ifdef __cplusplus
}
#endif

#endif //TIME_INTERFACE_H


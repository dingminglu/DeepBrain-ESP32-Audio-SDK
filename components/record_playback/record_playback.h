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

#ifndef RECORD_PLAYBACK_H
#define RECORD_PLAYBACK_H
 
#include "ctypes_interface.h"
#include "userconfig.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

/**
 * Create record playback service
 *
 * @param [in]task_priority
 * @return in APP FRAMEWORK various errors returned
 */
APP_FRAMEWORK_ERRNO_t record_playback_create(int task_priority);

/**
 * Exit record playback service
 *
 * @param void
 * @return in APP FRAMEWORK various errors returned
 */
APP_FRAMEWORK_ERRNO_t record_playback_delete(void);

/**
 * Start record playback
 *
 * @param void
 * @return in APP FRAMEWORK various errors returned
 */
APP_FRAMEWORK_ERRNO_t record_playback_start(void);

/**
 * Stop record playback
 *
 * @param void
 * @return in APP FRAMEWORK various errors returned
 */
APP_FRAMEWORK_ERRNO_t record_playback_stop(void);

#ifdef __cplusplus
 }
#endif
 
#endif  //RECORD_PLAYBACK_H



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

#ifndef KEYWORD_WAKEUP_LEXIN_H
#define KEYWORD_WAKEUP_LEXIN_H
 
#include "ctypes_interface.h"
#include "userconfig.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

/**
 * create keyword lexin wakeup service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t keyword_wakeup_lexin_create(int task_priority);

/**
 * keyword wakeup lexin service delete
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t keyword_wakeup_lexin_delete(void);

/**
 * record playback start
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t record_playback_start(void);

/**
 * record playback stop
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t record_playback_stop(void);

#ifdef __cplusplus
 }
#endif
 
#endif  //KEYWORD_WAKEUP_LEXIN_H



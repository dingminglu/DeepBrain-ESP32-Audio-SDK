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

#ifndef AIRKISS_LAN_DISCOVERY_H
#define AIRKISS_LAN_DISCOVERY_H

#include <stdio.h>
#include <string.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/**
 * @brief  wechat cloud service create
 * @param  task_priority, priority
 * @return See in APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t airkiss_lan_discovery_create(int task_priority);

/**
 * @brief  wechat cloud service delete
 * @return See in APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t airkiss_lan_discovery_delete(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* AIRKISS_LAN_DISCOVERY_H */



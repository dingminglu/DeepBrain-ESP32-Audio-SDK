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

#ifndef POWER_INDICATION_H
#define POWER_INDICATION_H

#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Update sleep(idle) time
 * @return true/false
 */
bool update_power_mng_sleep_time(void);

/**
 * @brief  Get sleep(idle) time
 * @param  sleep_sec, time will be written to it
 * @return true/false
 */
bool get_power_mng_sleep_time(uint32_t *sleep_sec);

/**
 * @brief  Enable or disable auto sleep
 * @param  true/false
 * @return true/false
 */
bool set_power_mng_auto_sleep(bool enable);

/**
 * @brief  Get auto sleep status
 * @param  enable, status will be written to it
 * @return true/false
 */
bool get_power_mng_auto_sleep(bool *enable);

/**
 * @brief  Set no operation sleep time(second)
 * @param  sleep_sec, time(second)
 * @return true/false
 */
bool set_power_mng_sleep_sec(uint32_t sleep_sec);

/**
 * @brief  Get sleep time(second)
 * @param  sleep_sec, time will be written to it
 * @return true/false
 */
bool get_power_mng_sleep_sec(uint32_t *sleep_sec);

/**
 * @brief  Set the minimum voltage that the system can operate
 * @param  voltage
 * @return true/false
 * @note
 */
bool set_power_mng_min_vol(float vol);

/**
 * @brief  Get the minimum voltage that the system can operate
 * @param  voltage, voltage will be written to it
 * @return true/false
 * @note
 */
bool get_power_mng_min_vol(float *vol);

/**
 * @brief  Set low power remind count
 * @param  count
 * @return true/false
 * @note
 */
bool set_power_mng_remind_count(uint32_t count);

/**
 * @brief  Get low power remind count 
 * @param  count, count will be written to it
 * @return true/false
 * @note
 */
bool get_power_mng_remind_count(uint32_t *count);


/**
 * power manage service create
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t power_manage_create(const int task_priority);

/**
 * power manage service delete
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t power_manage_delete(void);

#ifdef __cplusplus
}
#endif

#endif /*POWER_MANAGE_H*/

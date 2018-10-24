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

#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init I/O and ADC
 * @return true/false
 */
bool power_peripheral_init(void);

//bool power_on(void);

//bool power_off(void); 

//bool get_power_button_status(void);

/**
 * @brief  Get charging state
 * @return true/false
 */
bool get_charging_status(void);

/**
 * @brief  Get battery voltage  
 * @return Voltage Unit: V
 * @note   A simple filtering algorithm is implemented in this function.
 *         Generally the minimum voltage for system operation is : 3.6 ~ 3.7 V.
 */
float get_battery_voltage(void);

/**
 * @brief  Set battery voltage  
 * @return set successfully return true,set fail return false.
 * @note   set battery voltage and judge weather battery_vol set successfully.
 */
bool set_battery_voltage(const float battery_vol);

/**
 * @brief System shutdown
 * @return true/false
 * @note  Generally used for low battery or no operation shutdown.
 */
bool system_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif  //POWER_INTERFACE_H
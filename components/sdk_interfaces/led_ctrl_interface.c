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

/*platform header file*/
#include "speaker_interface.h"
#include "mcu_serial_comm.h"

//led控制命令
typedef enum LED_CTRL_CMD_t
{
	LED_CTRL_CMD_EAR_ON = 0,
	LED_CTRL_CMD_EAR_OFF,
	LED_CTRL_CMD_EYE_ON,
	LED_CTRL_CMD_EYE_OFF,
}LED_CTRL_CMD_t;

bool led_eye_on(void)
{
	uint8_t cmd = LED_CTRL_CMD_EYE_ON;
	return uart_send_cmd(SERIAL_COMM_CMD_LED, &cmd, sizeof(cmd));
}

bool led_eye_off(void)
{
	uint8_t cmd = LED_CTRL_CMD_EYE_OFF;
	return uart_send_cmd(SERIAL_COMM_CMD_LED, &cmd, sizeof(cmd));
}

bool led_ear_on(void)
{
	uint8_t cmd = LED_CTRL_CMD_EAR_ON;
	return uart_send_cmd(SERIAL_COMM_CMD_LED, &cmd, sizeof(cmd));
}

bool led_ear_off(void)
{
	uint8_t cmd = LED_CTRL_CMD_EAR_OFF;
	return uart_send_cmd(SERIAL_COMM_CMD_LED, &cmd, sizeof(cmd));
}


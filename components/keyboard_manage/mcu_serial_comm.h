#ifndef __MCU_SERIAL_COMM_H__
#define __MCU_SERIAL_COMM_H__
#include <stdio.h>
#include "app_framework.h"

typedef enum SERIAL_COMM_CMD_T
{
	SERIAL_COMM_CMD_KEY = 0x01,
	SERIAL_COMM_CMD_BATTERY_VOL,
	SERIAL_COMM_CMD_START = 0x50,
	SERIAL_COMM_CMD_SLEEP,
	SERIAL_COMM_CMD_LED,
	SERIAL_COMM_CMD_MCU_VERSION,
}SERIAL_COMM_CMD_T;

	
typedef enum MCU_SERIAL_PARSE_STATUS_T
{
	MCU_SERIAL_COMM_STATUS_HEAD1 = 0,
	MCU_SERIAL_COMM_STATUS_HEAD2,
	MCU_SERIAL_COMM_STATUS_DATA_LEN,
	MCU_SERIAL_COMM_STATUS_DATA,
	MCU_SERIAL_COMM_STATUS_DATA_END,
}MCU_SERIAL_PARSE_STATUS_T;

/**
 * create mcu serial comm service
 *
 * @param task_priority
 * @return APP_FRAMEWORK_ERRNO_t
 */
APP_FRAMEWORK_ERRNO_t mcu_serial_comm_create(int task_priority);

bool uart_send_cmd(
	uint8_t cmd, 
	uint8_t *data, 
	uint8_t data_len);


#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "time_interface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#include "userconfig.h"
#include "mcu_serial_comm.h"
#include "debug_log_interface.h"
#include "power_interface.h"

#define TAG_MCU_COMM "MCU_COMM"

#define SERIAL_TXD GPIO_NUM_27
#define SERIAL_RXD GPIO_NUM_19
#define SERIAL_UART_NUM UART_NUM_1

#define SERIAL_COMM_HEAD 0x55
	
typedef struct
{
	MCU_SERIAL_PARSE_STATUS_T _status;
	int recv_count;
	uint8_t rxd_buf[16];
	void *mutex_lock;
}MCU_SERIAL_PARSE_HANDLER_T;


MCU_SERIAL_PARSE_HANDLER_T *g_uart_comm_handler = NULL;

/**
 * mcu serial check sum
 *
 * @param _data
 * @param _data_len,data length
 * @return int
 */
static int mcu_serial_check_sum(unsigned char *_data, size_t _data_len);

/**
 * mcu serial init
 *
 * @param none
 * @return none
 */
static void mcu_serial_init(void);

/**
 * mcu serial cmd process
 *
 * @param _data
 * @param _data_len,data length
 * @return none
 */
static void mcu_serial_cmd_process(unsigned char *_data, size_t _data_len);

/**
 * mcu serial parse
 * 
 * @param _handler
 * @param _data
 * @param _data_len,data length
 * @return none
 */
static void mcu_serial_parse(
	MCU_SERIAL_PARSE_HANDLER_T *_handler, unsigned char *_data, size_t _data_len);

/**
 * mcu serial process
 *
 * @param none
 * @return none
 */
static void mcu_serial_process(void);

static SemaphoreHandle_t g_lock_keyborad = NULL;
static int g_keyboard_lock_status = 0;

static int mcu_serial_check_sum(unsigned char *_data, size_t _data_len)
{
	int i = 0;
	unsigned char sum = 0;
	
	for (i=0; i < _data_len - 1; i++)
	{
		sum += *(_data+i);
	}
	//printf("sum=%02x,%02x\n", sum, *(_data+_data_len-1));
	if (*(_data+_data_len-1) == sum)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void mcu_serial_init(void)
{
	uart_config_t uart_config = 
	{
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
	};
	//Configure UART1 parameters
	uart_param_config(SERIAL_UART_NUM, &uart_config);
	//Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
	uart_set_pin(SERIAL_UART_NUM, SERIAL_TXD, SERIAL_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//Install UART driver (we don't need an event queue here)
	//In this example we don't even use a buffer for sending data.
	uart_driver_install(SERIAL_UART_NUM, 512, 512, 0, NULL, 0);
}


static void mcu_serial_cmd_process(unsigned char *_data, size_t _data_len)
{
	SERIAL_COMM_CMD_T cmd = _data[3];

	switch (cmd)
	{
		case SERIAL_COMM_CMD_KEY:
		{
			//printf("cmd=%02x,key=%02x\n", cmd, _data[4]);
			int key_event = _data[4];
			//app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_KEYBOARD_SERVICE, APP_EVENT_KEYBOARD_EVENT, &key_event, sizeof(key_event));
			break;
		}
		case SERIAL_COMM_CMD_BATTERY_VOL:
		{
			float f_bat_vol = 0.0;
			unsigned short i_bat_vol = _data[4];
			i_bat_vol = i_bat_vol << 8;
			i_bat_vol += _data[5];
			f_bat_vol = (3.3/4095)*i_bat_vol;
			DEBUG_LOGE(TAG_MCU_COMM, "battery vol:i_bat_vol = %d, f_bat_vol=%.2f", i_bat_vol, f_bat_vol);
			set_battery_voltage(f_bat_vol*2);
			break;
		}
		default:
			break;
	}
}

static void mcu_serial_parse(
	MCU_SERIAL_PARSE_HANDLER_T *_handler, 
	uint8_t *_data, 
	size_t _data_len)
{
	int i = 0;
	
	if (_handler == NULL || _data == NULL || _data_len <= 0)
	{
		return;
	}
	
	for (i=0; i<_data_len; i++)
	{
		//DEBUG_LOGE(TAG_MCU_COMM, "status:%d byte:%02x\n", _handler->_status, *(_data+i));
		switch (_handler->_status)
		{
			case MCU_SERIAL_COMM_STATUS_HEAD1:
			{
				memset(_handler, 0, sizeof(MCU_SERIAL_PARSE_HANDLER_T));
				_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				
				if (*(_data+i) == SERIAL_COMM_HEAD)
				{
					_handler->rxd_buf[0] = SERIAL_COMM_HEAD;
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD2;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_HEAD2:
			{
				if (*(_data+i) == SERIAL_COMM_HEAD)
				{
					_handler->rxd_buf[1] = SERIAL_COMM_HEAD;
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA_LEN;
				}
				else
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_DATA_LEN:
			{
				if ((unsigned char)*(_data+i) > 32)
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				}
				else
				{
					_handler->rxd_buf[2] = *(_data+i);
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_DATA:
			{
				_handler->rxd_buf[3+_handler->recv_count] = *(_data+i);
				_handler->recv_count++;
				if (_handler->recv_count == _handler->rxd_buf[2])
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA_END;
				}
				break;
			}
			default:
				break;
		}

		if (_handler->_status == MCU_SERIAL_COMM_STATUS_DATA_END)
		{
			if (mcu_serial_check_sum(&_handler->rxd_buf, _handler->rxd_buf[2] + 3))
			{
				mcu_serial_cmd_process(&_handler->rxd_buf, _handler->rxd_buf[2] + 3);
			}

			_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
		}
	}
}

static void mcu_serial_process(void)
{
	uint8_t recv_buff[32] = {0};
	mcu_serial_init();
	
	if (g_uart_comm_handler == NULL)
	{
		g_uart_comm_handler = (MCU_SERIAL_PARSE_HANDLER_T *)memory_malloc(sizeof(MCU_SERIAL_PARSE_HANDLER_T));
		if (g_uart_comm_handler == NULL)
		{
			DEBUG_LOGE(TAG_MCU_COMM, "memory_malloc g_uart_comm_handler failed");
			return;
		}
		memset(g_uart_comm_handler, 0, sizeof(MCU_SERIAL_PARSE_HANDLER_T));

		SEMPHR_CREATE_LOCK(g_uart_comm_handler->mutex_lock);
		if (g_uart_comm_handler->mutex_lock == NULL)
		{
			DEBUG_LOGE(TAG_MCU_COMM, "SEMPHR_CREATE_LOCK failed");
			memory_free(g_uart_comm_handler);
			g_uart_comm_handler = NULL;
			return;
		}
	}

	while (1)
	{		
		int len = uart_read_bytes(SERIAL_UART_NUM, recv_buff, sizeof(recv_buff), 20 / portTICK_RATE_MS);
		if (len <= 0)
		{
			vTaskDelay(10);
			continue;
		}
		
		//DEBUG_LOGE(TAG_MCU_COMM, "uart_read_bytes len = %d\n", len);
		mcu_serial_parse(g_uart_comm_handler, recv_buff, len);
	}
	vTaskDelete(NULL);
}


void mcu_serial_comm_create(void)
{
	xTaskCreate(mcu_serial_process, "mcu_serial_process", 4096, NULL, 10, NULL);
}

bool uart_send_cmd(
	uint8_t cmd, 
	uint8_t *data, 
	uint8_t data_len)
{
	int i = 0;
	int index = 0;
	int send_len = 0;
	char rxd_buf[16] = {0};
	
	if (g_uart_comm_handler == NULL)
	{
		DEBUG_LOGE(TAG_MCU_COMM, "g_uart_comm_handler is NULL");
		return false;
	}
	
	rxd_buf[0] = SERIAL_COMM_HEAD;
	rxd_buf[1] = SERIAL_COMM_HEAD;
	rxd_buf[2] = 2 + data_len;
	rxd_buf[3] = cmd;
	if (data_len > 0)
	{
		memcpy(&rxd_buf[4], data, data_len);
	}
	index = 4+data_len;
	for (i=0; i<index; i++)
	{
		rxd_buf[index] += rxd_buf[i];
	}
	index++;
	/*
	DEBUG_LOGE(TAG_MCU_COMM, "send len = %d\n", index);
	for (i=0; i<index; i++)
	{
		DEBUG_LOGE(TAG_MCU_COMM, "byte:%02x\n", rxd_buf[i]);
	}
	*/
	SEMPHR_TRY_LOCK(g_uart_comm_handler->mutex_lock);
	send_len = uart_write_bytes(SERIAL_UART_NUM, rxd_buf, index);
	SEMPHR_TRY_UNLOCK(g_uart_comm_handler->mutex_lock);
	if (send_len == index)
	{
		return true;
	}
	else
	{
		DEBUG_LOGE(TAG_MCU_COMM, "uart_write_bytes failed,[%d]!=[%d]", 
			index, send_len);
		return false;
	}
}


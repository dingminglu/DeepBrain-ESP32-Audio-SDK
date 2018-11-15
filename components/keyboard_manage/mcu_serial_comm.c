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

#define LOG_TAG 			"MCU COMM"
#define MCU_LOOP_TIMEOUT	20

#define SERIAL_TXD 			GPIO_NUM_27
#define SERIAL_RXD 			GPIO_NUM_19
#define SERIAL_UART_NUM 	UART_NUM_1

#define SERIAL_COMM_HEAD 	0x55
	
typedef struct
{
	MCU_SERIAL_PARSE_STATUS_T _status;
	int recv_count;
	uint8_t rxd_buf[16];
	void *mutex_lock;
}MCU_SERIAL_PARSE_HANDLER_T;


static MCU_SERIAL_PARSE_HANDLER_T *g_uart_comm_handler = NULL;

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
			//DEBUG_LOGE(LOG_TAG, "battery vol:i_bat_vol = %d, f_bat_vol=%.2f", i_bat_vol, f_bat_vol);
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
		//DEBUG_LOGE(LOG_TAG, "status:%d byte:%02x\n", _handler->_status, *(_data+i));
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
	
	if (g_uart_comm_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_uart_comm_handler is NULL");
		return;
	}

	SEMPHR_TRY_LOCK(g_uart_comm_handler->mutex_lock);
	int len = uart_read_bytes(SERIAL_UART_NUM, recv_buff, sizeof(recv_buff), 20 / portTICK_RATE_MS);
	SEMPHR_TRY_UNLOCK(g_uart_comm_handler->mutex_lock);
	if (len <= 0)
	{
		return;
	}
	
	//DEBUG_LOGE(LOG_TAG, "uart_read_bytes len = %d\n", len);
	mcu_serial_parse(g_uart_comm_handler, recv_buff, len);
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
		DEBUG_LOGE(LOG_TAG, "g_uart_comm_handler is NULL");
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

	//DEBUG_LOGE(LOG_TAG, "rxd_buf[4] is [%d]", rxd_buf[4]);
#if 0
	DEBUG_LOGE(LOG_TAG, "send len = %d\n", index);
	for (i=0; i<index; i++)
	{
		DEBUG_LOGE(LOG_TAG, "byte:%02x\n", rxd_buf[i]);
	}
#endif

	SEMPHR_TRY_LOCK(g_uart_comm_handler->mutex_lock);
	send_len = uart_write_bytes(SERIAL_UART_NUM, rxd_buf, index);
	SEMPHR_TRY_UNLOCK(g_uart_comm_handler->mutex_lock);
	if (send_len == index)
	{
		return true;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "uart_write_bytes failed,[%d]!=[%d]", 
			index, send_len);
		return false;
	}
}

static void mcu_event_callback(void *app, APP_EVENT_MSG_t *msg)
{	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			mcu_serial_process();
			break;
		}
		case APP_EVENT_MCU_SERIAL_COMM_SERVICE_IDLE:
		{
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit(app);
			break;
		}
		default:
			break;
	}
}

static void mcu_serial_comm_destroy(void)
{
	if (g_uart_comm_handler == NULL)
	{
		return;
	}

	if (g_uart_comm_handler->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_uart_comm_handler->mutex_lock);
		g_uart_comm_handler->mutex_lock = NULL;
	}
	
	memory_free(g_uart_comm_handler);
	g_uart_comm_handler = NULL;
}

static void mcu_serial_comm_service(void *pv)
{
	APP_OBJECT_t *app = NULL;

	app = app_new(APP_NAME_MCU_SERIAL_COMM_SERVICE);	
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_MCU_SERIAL_COMM_SERVICE);
		mcu_serial_comm_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, MCU_LOOP_TIMEOUT, mcu_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_MCU_SERIAL_COMM_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);
	
	mcu_serial_comm_destroy();
	
	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t mcu_serial_comm_create(int task_priority)
{
	if (g_uart_comm_handler != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_uart_comm_handler already exists");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}
	
	//申请运行句柄
	g_uart_comm_handler = (MCU_SERIAL_PARSE_HANDLER_T *)memory_malloc(sizeof(MCU_SERIAL_PARSE_HANDLER_T));
	if (g_uart_comm_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc g_uart_comm_handler failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_uart_comm_handler, 0, sizeof(MCU_SERIAL_PARSE_HANDLER_T));

	//初始化参数
	SEMPHR_CREATE_LOCK(g_uart_comm_handler->mutex_lock);
	if (g_uart_comm_handler->mutex_lock == NULL)
	{
		mcu_serial_comm_destroy();
		DEBUG_LOGE(LOG_TAG, "g_uart_comm_handler->lock init failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	//初始化MCU配置
	mcu_serial_init();

	//创建运行线程
    if (!task_thread_create(mcu_serial_comm_service,
                    "mcu_serial_comm_service",
                    APP_NAME_MCU_SERIAL_COMM_SERVICE_STACK_SIZE,
                    NULL,
                    task_priority)) {

        DEBUG_LOGE(LOG_TAG, "ERROR creating mcu_serial_process task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t mcu_serial_comm_delete(void)
{
	return app_send_message(APP_NAME_MCU_SERIAL_COMM_SERVICE, APP_NAME_MCU_SERIAL_COMM_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}


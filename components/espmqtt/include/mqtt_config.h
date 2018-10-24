/* 
 * ESPRESSIF MIT License 
 * 
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD> 
 * 
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case, 
 * it is free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished 
 * to do so, subject to the following conditions: 
 * 
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * 
 */

#ifndef _MQTT_CONFIG_H_
#define _MQTT_CONFIG_H_
#include "sdkconfig.h"
#include <stdio.h>

#define CONFIG_MQTT_PROTOCOL_311 1
//#define CONFIG_MQTT_SECURITY_ON 1
#define CONFIG_MQTT_PRIORITY 5
#define CONFIG_MQTT_LOG_ERROR_ON
#define CONFIG_MQTT_LOG_WARN_ON
#define CONFIG_MQTT_LOG_INFO_ON
#define CONFIG_MQTT_RECONNECT_TIMEOUT 60
#define CONFIG_MQTT_QUEUE_BUFFER_SIZE_WORD 1024
#define CONFIG_MQTT_BUFFER_SIZE_BYTE 1024
#define CONFIG_MQTT_MAX_HOST_LEN 64
#define CONFIG_MQTT_MAX_CLIENT_LEN 64
#define CONFIG_MQTT_MAX_USERNAME_LEN 32
#define CONFIG_MQTT_MAX_PASSWORD_LEN 32
#define CONFIG_MQTT_MAX_LWT_TOPIC 64
#define CONFIG_MQTT_MAX_LWT_MSG 32



#ifdef CONFIG_MQTT_LOG_ERROR_ON
#define mqtt_error(format, ... ) printf( "[MQTT ERROR] " format "\n", ##__VA_ARGS__)
#else
#define mqtt_error( format, ... )
#endif
#ifdef CONFIG_MQTT_LOG_WARN_ON
#define mqtt_warn(format, ... ) printf( "[MQTT WARN] " format "\n", ##__VA_ARGS__)
#else
#define mqtt_warn( format, ... )
#endif
#ifdef CONFIG_MQTT_LOG_INFO_ON
#define mqtt_info(format, ... ) printf( "[MQTT INFO] " format "\n", ##__VA_ARGS__)
#else
#define mqtt_info(format, ... )
#endif

#ifndef CONFIG_MQTT_QUEUE_BUFFER_SIZE_WORD
#define CONFIG_MQTT_QUEUE_BUFFER_SIZE_WORD 1024
#endif

#endif

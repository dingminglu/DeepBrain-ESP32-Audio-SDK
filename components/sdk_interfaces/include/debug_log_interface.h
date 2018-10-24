/* debug_log.h
** debug log api
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

#ifndef __DEBUG_LOG_INTERFACE_H__
#define __DEBUG_LOG_INTERFACE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "semaphore_lock_interface.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define LOG_COLOR_BLINK   "5"
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_YELLOW  "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR_WHITE   "37"

extern void *g_log_lock;

#define ENABLE_DEBUG_LOG 1

#if ENABLE_DEBUG_LOG == 1
#define DEBUG_LOGE( tag, format, ... ) ESP_LOGE( tag, "L[%d]"format, __LINE__, ##__VA_ARGS__ )   
#define DEBUG_LOGW( tag, format, ... ) ESP_LOGW( tag, "L[%d]"format, __LINE__, ##__VA_ARGS__ )  
#define DEBUG_LOGI( tag, format, ... ) ESP_LOGI( tag, "L[%d]"format, __LINE__, ##__VA_ARGS__ ) 
#define DEBUG_LOGD( tag, format, ... ) ESP_LOGD( tag, "L[%d]"format, __LINE__, ##__VA_ARGS__ )  
#define DEBUG_LOGV( tag, format, ... ) ESP_LOGV( tag, "L[%d]"format, __LINE__, ##__VA_ARGS__ )
#else
#define DEBUG_LOGE( tag, format, ... )
#define DEBUG_LOGW( tag, format, ... )
#define DEBUG_LOGI( tag, format, ... )
#define DEBUG_LOGD( tag, format, ... ) 
#define DEBUG_LOGV( tag, format, ... )
#endif

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __DEBUG_LOG_H__ */

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

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "lwip/netdb.h"
#include "time_interface.h"

uint64_t get_time_of_day(void)
{
	struct timeval cur_time;
	
	gettimeofday(&cur_time, NULL);
	
	return (cur_time.tv_sec*1000 + cur_time.tv_usec/1000);
}

void get_time_stamp_string(
	uint8_t * str_time, 
	const uint32_t str_time_len)
{
	time_t tNow = time(NULL);   
    struct tm tmNow = { 0 };  
	
    localtime_r(&tNow, &tmNow);//localtime_r为可重入函数，不能使用localtime  
    snprintf((char *)str_time, str_time_len, "%04d-%02d-%02dT%02d:%02d:%02d", 
		tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
		tmNow.tm_hour + 8, tmNow.tm_min, tmNow.tm_sec); 

	return;
}


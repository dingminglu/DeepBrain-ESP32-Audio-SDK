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


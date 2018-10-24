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

#ifndef CTYPES_INTERFACE_H
#define CTYPES_INTERFACE_H

#include <stdio.h>
#include <stdbool.h>

typedef signed char 			int8_t;
typedef unsigned char 			uint8_t;
typedef signed short int 		int16_t;
typedef unsigned short int 		uint16_t;
typedef signed int 				int32_t;
typedef unsigned int 			uint32_t; 

#if __WORDSIZE == 64

typedef signed long int 		int64_t;
typedef unsigned long int 		uint64_t;

#else

typedef signed long long int 	int64_t;
typedef unsigned long long int 	uint64_t;

#endif

#endif //CTYPES_INTERFACE_H



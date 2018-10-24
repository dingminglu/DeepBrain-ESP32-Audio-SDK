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
 
#ifndef DCL_VOLUME_DECODE_H
#define DCL_VOLUME_DECODE_H

#include "cJSON.h"

typedef enum VOLUME_CONTROL_TYPE_t
{
	VOLUME_CONTROL_INITIAL,			//初始状态,无控制命令状态
	VOLUME_CONTROL_HIGHER,			//调高音量
	VOLUME_CONTROL_LOWER,			//调低音量
	VOLUME_CONTROL_MODERATE,		//音量适中
	VOLUME_CONTROL_MAX,				//音量最大
	VOLUME_CONTROL_MIN				//音量最小
}VOLUME_CONTROL_TYPE_t;

typedef enum
{
	VOLUME_TYPE_INITIAL,			//初始状态
	VOLUME_TYPE_SCALE_MODE,			//有音量刻度模式
	VOLUME_TYPE_NOT_SCALE_MODE		//无音量刻度模式
}VOLUME_TYPE_T;

VOLUME_CONTROL_TYPE_t dcl_nlp_volume_cmd_decode(const char* json_string);

#endif
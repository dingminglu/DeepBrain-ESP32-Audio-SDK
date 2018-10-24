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

#ifndef AIP_INTERFACE_H
#define AIP_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include "app_framework.h"
#include "app_config.h"
#include "audio_record_interface.h"

/* AIP录音数据 */
typedef struct AIP_RECORD_RESULT_t
{
	uint32_t record_sn;
	int32_t record_id;
	uint8_t *record_data;
	uint32_t record_len;
	uint32_t record_ms;
}AIP_RECORD_RESULT_t;

/* 录音数据回调 */
typedef void (*aip_record_cb)(AIP_RECORD_RESULT_t *rec_result);

/**
 * create aip service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t aip_service_create(int task_priority);

/**
 * aip service delete
 *
 * @param none
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t aip_service_delete(void);

/**
 * aip start record
 *
 * @param [out]record_sn
 * @param [in]record_ms
 * @param [in]cb,aip record callback
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t aip_start_record(uint32_t *record_sn, uint32_t record_ms, aip_record_cb cb);

/**
 * aip stop record
 *
 * @param none
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t aip_stop_record(aip_record_cb cb);
#endif


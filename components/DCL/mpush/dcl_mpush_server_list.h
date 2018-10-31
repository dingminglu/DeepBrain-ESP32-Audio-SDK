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

#ifndef DCL_MPUSH_SERVER_LIST_H
#define DCL_MPUSH_SERVER_LIST_H
 
#include "dcl_common_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

/* dcl mpush server info */
#define DCL_MAX_MPUSH_SERVER_NUM 5

typedef struct DCL_MPUSH_SERVER_INFO_T
{
	char server_addr[64];
	char server_port[8];
}DCL_MPUSH_SERVER_INFO_T;

typedef struct DCL_MPUSH_SERVER_LIST_T
{
	uint32_t server_num;
	DCL_MPUSH_SERVER_INFO_T server_info[DCL_MAX_MPUSH_SERVER_NUM];
}DCL_MPUSH_SERVER_LIST_T;

/* dcl mpush handle */
typedef struct DCL_MPUSH_SERVER_HANDLER_t
{
	DCL_HTTP_BUFFER_t http_buffer;
}DCL_MPUSH_SERVER_HANDLER_t;

DCL_ERROR_CODE_t dcl_mpush_get_server_list(
	const DCL_AUTH_PARAMS_t* const input_params,
	DCL_MPUSH_SERVER_LIST_T* const server_list);

#ifdef __cplusplus
 }
#endif
 
#endif  /* DCL_MPUSH_SERVER_LIST_H */


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

#ifndef DCL_MPUSH_PUSH_MSG_H
#define DCL_MPUSH_PUSH_MSG_H
 
#include "dcl_common_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

/* dcl mpush push msg handle */
typedef struct DCL_MPUSH_PUSH_MSG_HANDLER_t
{
	DCL_HTTP_BUFFER_t http_buffer;
	char base64_enode_buffer[60*1024];
	char send_buffer[60*1024];
}DCL_MPUSH_PUSH_MSG_HANDLER_t;

DCL_ERROR_CODE_t dcl_mpush_push_msg(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char *data, 
	const uint32_t data_len,
	const char *nlp_text,
	const char *to_user);

#ifdef __cplusplus
 }
#endif
 
#endif  /* DCL_MPUSH_PUSH_MSG_H */


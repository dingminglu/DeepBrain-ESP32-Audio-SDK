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

#ifndef DCL_TTS_API_H
#define DCL_TTS_API_H
 
#include "dcl_common_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

/* dcl tts handle */
typedef struct DCL_TTS_HANDLE_t
{
	DCL_HTTP_BUFFER_t http_buffer;
}DCL_TTS_HANDLE_t;

void unit_test_dcl_get_tts_url(const char* const text);

DCL_ERROR_CODE_t dcl_get_tts_url(
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text,
	char* const out_url,
	const uint32_t out_url_len);

#ifdef __cplusplus
 }
#endif
 
#endif  /*DCL_TTS_API_H*/


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

#ifndef DCL_COMMON_INTERFACE_H
#define DCL_COMMON_INTERFACE_H
 
#include "dcl_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

//#define DCL_TEST_MODE		//测试环境
//#define DCL_DEMO_MODE 	//demo环境
#define DCL_NORMAL_MODE		//正式环境

//dcl api请求URL
#ifdef DCL_TEST_MODE
#define DCL_SERVER_API_URL  "http://192.168.20.14:9030/open-api/service"
#endif

#ifdef DCL_DEMO_MODE
#define DCL_SERVER_API_URL  "http://api-demo.deepbrain.ai/open-api/service"
#endif

#ifdef DCL_NORMAL_MODE
#define DCL_SERVER_API_URL "http://api.deepbrain.ai:8383/open-api/service"
#endif

//dcl语义请求URL
#ifdef DCL_TEST_MODE
#define DCL_NLP_API_URL  "http://192.168.20.14:9030/deep-brain-api/ask"
#endif

#ifdef DCL_DEMO_MODE
#define DCL_NLP_API_URL  "http://api-demo.deepbrain.ai/deep-brain-api/ask"
#endif

#ifdef DCL_NORMAL_MODE
#define DCL_NLP_API_URL "http://api.deepbrain.ai:8383/deep-brain-api/ask"
#endif

//dcl asr请求URL
#ifdef DCL_TEST_MODE
#define DCL_ASR_API_URL  "http://192.168.20.14:9030/open-api/asr/recognise"
#endif

#ifdef DCL_DEMO_MODE
#define DCL_ASR_API_URL  "http://asr-demo.deepbrain.ai/open-api/asr/recognise"
#endif

#ifdef DCL_NORMAL_MODE
#define DCL_ASR_API_URL "http://asr.deepbrain.ai/open-api/asr/recognise"
#endif

#ifdef DCL_DEMO_MODE
#define MPUSH_GET_SERVER_URL 		"http://192.168.20.91:9034/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://192.168.20.91:9034/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://192.168.20.91:9034/open-api/testFetchOfflineMessage"
#endif

#ifdef DCL_NORMAL_MODE
#define MPUSH_GET_SERVER_URL 		"http://message.deepbrain.ai:9134/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://message.deepbrain.ai:9134/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://message.deepbrain.ai:9134/open-api/testFetchOfflineMessage"	
#endif

/* http buffer */
typedef struct DCL_HTTP_BUFFER_t
{
	int32_t sock;
	char 	domain[128];
	char 	port[8];
	char 	params[1024];
	char 	req_header[1024*2];
	char	req_body[1024*2];
	char	str_request[1024*4];
	char	str_response[MAX_NLP_RESULT_LENGTH];
	
	char    str_nonce[64];
	char    str_timestamp[64];
	char    str_private_key[64];
	
	void	*json_body;
}DCL_HTTP_BUFFER_t;

#ifdef __cplusplus
 }
#endif
 
#endif  /*DCL_COMMON_INTERFACE_H*/


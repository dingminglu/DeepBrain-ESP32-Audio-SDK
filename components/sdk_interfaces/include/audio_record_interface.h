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

#ifndef AUDIO_RECORD_INTERFACE_H
#define AUDIO_RECORD_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

//计算 存储 milliseconds毫秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_MS(milliseconds, samples_rate)  (2L*(milliseconds)*(samples_rate)/ 1000) 

//计算 存储 seconds秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_S(seconds, samples_rate)  (2L*(seconds)*(samples_rate))

/* pcm sampling rate definition */
typedef enum PCM_SAMPLING_RATE_t
{
	PCM_SAMPLING_RATE_8K = 8000, //8K采样
	PCM_SAMPLING_RATE_16K = 16000, //16K采样
}PCM_SAMPLING_RATE_t;

/**
 * start pcm record
 *
 * @param [in]sampling_rate, 8000 or 16000
 * @return true/false
 */
bool audio_record_start(const PCM_SAMPLING_RATE_t sampling_rate);

/**
 * read pcm data
 *
 * @param [out] pcm_data
 * @param [in] pcm_size, read pcm size
 * @return actually read pcm data size
 */
uint32_t audio_record_read(uint8_t* const pcm_data, const uint32_t pcm_size);

/**
 * stop pcm record
 *
 * @param void
 * @return true/false
 */
bool audio_record_stop(void);

#ifdef __cplusplus
}
#endif

#endif  //AUDIO_RECORD_INTERFACE_H


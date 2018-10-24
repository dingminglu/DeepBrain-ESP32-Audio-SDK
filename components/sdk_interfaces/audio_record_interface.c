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

#include "audio_record_interface.h"
#include "task_thread_interface.h"
#include "Recorder.h"
#include "debug_log_interface.h"

#define TAG_LOG "EspAudioRecorder"

bool audio_record_start(const PCM_SAMPLING_RATE_t sampling_rate)
{	
	int ret = 0;
	
	if(sampling_rate == PCM_SAMPLING_RATE_8K)
	{
		ret = EspAudioRecorderStart("i2s://8000:1@record.pcm#raw", 10 * 1024, 0);
	}
	else
	{
		ret = EspAudioRecorderStart("i2s://16000:1@record.pcm#raw", 10 * 1024, 0);
	}
	
	if (ret != AUDIO_ERR_NO_ERROR) 
	{
		DEBUG_LOGE(TAG_LOG, "### EspAudioRecorderStart failed, error=%d ###", ret);
		return false;
	}
	
	return true;
}

uint32_t audio_record_read(uint8_t* const pcm_data, const uint32_t pcm_size)
{
	int len = 0;

#if 0
		static FILE *pcm = NULL;

		task_thread_sleep(100);
		
		if (pcm == NULL)
		{
			pcm = fopen("/sdcard/1.pcm", "r");
		}
		
		if (pcm != NULL)
		{
			int ret = 0;
			ret = fread(pcm_data, 1, pcm_size, pcm);
			if (ret <= 0)
			{
				fseek(pcm, 0, SEEK_SET);
			} 

			return ret;
		}
#endif	
	
	int ret = EspAudioRecorderRead((uint8_t*)pcm_data, pcm_size, &len);
	if (ret != AUDIO_ERR_NO_ERROR) 
	{
		DEBUG_LOGE(TAG_LOG, "### EspAudioRecorderRead failed, error=%d ###", ret);
		return 0;
	}
	
	return len;
}

bool audio_record_stop(void)
{
	int ret = EspAudioRecorderStop(TERMINATION_TYPE_NOW);
	if (ret != AUDIO_ERR_NO_ERROR)
	{
		DEBUG_LOGE(TAG_LOG, "### EspAudioRecorderStop failed, error=%d ###", ret);
		return false;
	}

	return true;
}


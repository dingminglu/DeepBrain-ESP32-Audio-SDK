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

/*platform header file*/
#include "ctypes_interface.h"
#include "MediaHal.h"
#include "speaker_interface.h"
#include <math.h>

#define MIN_VOLUME 0
#define MAX_VOLUME 100

static int8_t g_sys_volume = MAX_VOLUME;

/**
 * volume calculate expression:
 * x range:0x0000 ~ 0x0FFF
 * y = 20log（x）- 72(dB)
 * x = 10^((72+y)/20)
 */
static uint32_t calculate_volume(int8_t vol)
{
	if (vol < MIN_VOLUME || vol > MAX_VOLUME)
	{
		return 0;
	}
	
	double y = pow(10, 0.72*vol/20);
	
	return (uint32_t)y;
}

bool set_volume(int8_t volume)
{
	uint32_t vol = 0;
	
	if (volume < MIN_VOLUME || volume > MAX_VOLUME)
	{
		return false;
	}

	g_sys_volume = volume;
	
	MediaHalSetVolume(volume);

	return true;
}

bool get_volume(int8_t *volume)
{
	if (volume == NULL)
	{
		return false;
	}
	
	*volume = g_sys_volume;
	
	return true;
}

bool set_mute(bool mute)
{	
	if (mute)
	{
		MediaHalPaPwr(0);
	}
	else
	{
		MediaHalPaPwr(1);
	}

	return true;
}


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

#ifndef LED_CTRL_INTERFACE_H
#define LED_CTRL_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The SpeakerInterface is concerned with the control of volume and mute settings of a speaker.
 * The two settings are independent of each other, and the respective APIs shall not affect
 * the other setting in any way. Compound behaviors (such as unmuting when volume is adjusted) will
 * be handled at a layer above this interface.
 */


bool led_eye_on(void);

bool led_eye_off(void);

bool led_ear_on(void);

bool led_ear_off(void);

#ifdef __cplusplus
}
#endif

#endif  //SPEAKER_INTERFACE_H
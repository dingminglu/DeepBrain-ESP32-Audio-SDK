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

#ifndef SPEAKER_INTERFACE_H
#define SPEAKER_INTERFACE_H

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

/**
 * Set the absolute volume of the speaker. @c volume
 * will be [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX], and implementers of the interface must normalize
 * the volume to fit the needs of their drivers.
 *
 * @param volume A volume to set the speaker to.
 * @return Whether the operation was successful.
 */
bool set_volume(int8_t volume);

/**
 * Get the absolute volume of the speaker. @c volume
 * will be [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX], and implementers of the interface must normalize
 * the volume to fit the needs of their drivers.
 *
 * @param volume A volume get from the speaker.
 * @return Whether the operation was successful.
 */
bool get_volume(int8_t *volume);

/**
 * Set the mute of the speaker.
 *
 * @param mute Represents whether the speaker should be muted (true) or unmuted (false).
 * @return Whether the operation was successful.
 */
bool set_mute(bool mute);

#ifdef __cplusplus
}
#endif

#endif  //SPEAKER_INTERFACE_H
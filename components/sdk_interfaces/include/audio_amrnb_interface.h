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

#ifndef AUDIO_AMRNB_INTERFACE_H
#define AUDIO_AMRNB_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMRNB_HEADER "#!AMR\n"
#define AMRNB_ENCODE_FRAME_MS	20
#define AMRNB_ENCODE_IN_BUFF_SIZE 320
#define AMRNB_ENCODE_OUT_BUFF_SIZE (AMRNB_ENCODE_IN_BUFF_SIZE/10)

#ifdef __cplusplus
}
#endif

#endif  //AUDIO_AMRNB_INTERFACE_H


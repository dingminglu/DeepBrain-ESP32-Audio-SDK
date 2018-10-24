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

#ifndef SDCARD_INTERFACE_H
#define SDCARD_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize sd card
 *
 * @param none
 * @return true/false
 */
bool sd_card_init(void);

/**
 * judge whether sdcard is inserted or not
 *
 * @param none
 * @return true/false
 */
bool is_sdcard_inserted(void);

/**
 * sdcard mount
 *
 * @param basePath
 * @return true/false
 */
bool sdcard_mount(const char *basePath);

/**
 * sdcard unmount
 *
 * @param none
 * @return true/false
 */
bool sdcard_unmount(void);

#ifdef __cplusplus
}
#endif

#endif  /* SDCARD_INTERFACE_H */


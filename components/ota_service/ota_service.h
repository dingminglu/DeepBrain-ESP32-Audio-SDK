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

#ifndef __OTA_SERVICE_H__
#define __OTA_SERVICE_H__

#include <stdio.h>

typedef enum
{
	OTA_UPGRADE_STATE_UPGRADING = 0,
	OTA_UPGRADE_STATE_NOT_UPGRADING,
}OTA_UPGRADE_STATE;

typedef enum
{
	OTA_PLAY_TONE_MSG_NONE,
	OTA_PLAY_TONE_MSG_START,
	OTA_PLAY_TONE_MSG_STOP,
}OTA_PLAY_TONE_MSG_T;

struct AudioTrack;

extern QueueHandle_t g_ota_queue;
extern OTA_UPGRADE_STATE g_ota_upgrade_state;

typedef struct
{
    /*relation*/
    //MediaService Based;
    struct AudioTrack *_receivedTrack;  //FIXME: is this needed??
} OTA_SERVICE_T;

typedef enum
{
	MSG_OTA_IDEL,
	MSG_OTA_SD_UPDATE,
	MSG_OTA_WIFI_UPDATE
}MSG_OTA_SERVICE_T;

/**
 * create ota service 
 *
 * @param  task_priority, the priority of running thread
 * @return none
 */
void ota_service_create(int task_priority);

/**
 * ota send message  
 *
 * @param  _msg, the upgrade way
 * @return none
 */
void ota_service_send_msg(MSG_OTA_SERVICE_T _msg);

/**
 * get ota upgrade state
 *
 * @param  none
 * @return ota upgrade state
 */
OTA_UPGRADE_STATE get_ota_upgrade_state();

/**
 * create ota service play tone
 *
 * @param  none
 * @return none 
 */
void ota_service_play_tone_create(void);

#endif


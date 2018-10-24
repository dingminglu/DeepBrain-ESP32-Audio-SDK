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

#ifndef WIFI_MANAGE_H
#define WIFI_MANAGE_H

#include "app_config.h"
#include "device_params_manage.h"

/* wifi manage status */
typedef enum WIFI_MANAGE_STATUS_t
{
	WIFI_MANAGE_STATUS_IDLE = 0,
	WIFI_MANAGE_STATUS_STA = 0x1100,
	WIFI_MANAGE_STATUS_STA_ON,
	WIFI_MANAGE_STATUS_STA_OFF,
	WIFI_MANAGE_STATUS_STA_CONNECTING,
	WIFI_MANAGE_STATUS_STA_CONNECT_WAIT,
	WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS,
	WIFI_MANAGE_STATUS_STA_CONNECT_FAIL,
	WIFI_MANAGE_STATUS_STA_CONNECTED,
	WIFI_MANAGE_STATUS_STA_DISCONNECTED,
	WIFI_MANAGE_STATUS_SMARTCONFIG = 0x2200,
	WIFI_MANAGE_STATUS_SMARTCONFIG_ON,
	WIFI_MANAGE_STATUS_SMARTCONFIG_OFF,
	WIFI_MANAGE_STATUS_SMARTCONFIG_WAITING,
	WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_WAIT,
	WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_SUCCESS,
	WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_FAIL,
}WIFI_MANAGE_STATUS_t;

typedef enum WIFI_MANAGE_ERRNO_t
{
	WIFI_MANAGE_ERRNO_OK,
	WIFI_MANAGE_ERRNO_FAIL,
}WIFI_MANAGE_ERRNO_t;

typedef enum WIFI_AP_SERVER_STATUS_t
{
	WIFI_AP_SERVER_STATUS_IDEL,
	WIFI_AP_SERVER_STATUS_ACCEPTING,
	WIFI_AP_SERVER_STATUS_COMMUNICATION,
}WIFI_AP_SERVER_STATUS_t;

/**
 * get wifi manage status
 *
 * @param void
 * @return wifi status
 */	
int get_wifi_manage_status(void);

/**
 * create wifi manage service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t wifi_manage_create(int task_priority);

/**
 * wifi manage service delete
 *
 * @param none
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t wifi_manage_delete(void);

/**
 * wifi manage start smartconfig
 *
 * @param none
 * @return app framework errno
 */	
APP_FRAMEWORK_ERRNO_t wifi_manage_start_smartconfig(void);

#endif


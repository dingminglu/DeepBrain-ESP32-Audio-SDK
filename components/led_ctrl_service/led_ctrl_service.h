#ifndef LED_CTRL_SERVICE_H
#define LED_CTRL_SERVICE_H

#include "app_framework.h"

typedef struct
{
	APP_EVENT_LED_CTRL_t eye_led_state;
	APP_EVENT_LED_CTRL_t ear_led_state;
}LED_CTRL_LED_STATUS_t;

/**
 * get led status 
 * @return led status struct
 */
LED_CTRL_LED_STATUS_t get_led_status();

/**
 * create keyboard service 
 *
 * @param self,media service
 * @return none
 */
APP_FRAMEWORK_ERRNO_t led_ctrl_service_create(int task_priority);

/**
 * keyboard service delete
 *
 * @param none
 * @return none
 */
APP_FRAMEWORK_ERRNO_t led_ctrl_service_delete(void);

#endif

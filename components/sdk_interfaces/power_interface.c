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
#include "driver/gpio.h"
#include "app_config.h"
#include "userconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "power_interface.h"

static float g_battery_vol = 4.0;

float get_battery_voltage(void)
{
	return g_battery_vol;
}

bool set_battery_voltage(const float battery_vol)
{
	g_battery_vol = battery_vol;
	
	return true;
}

bool get_charging_status(void)
{
	if(gpio_get_level(GPIO_POWER_CHARGE_NUM))
	{
		return true;
	}
	else
	{
		return false;
	}
}

#if 0
bool get_power_button_status(void)
{
	if(gpio_get_level(GPIO_POWER_BUTTON_NUM))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool power_on(void)
{
	gpio_set_level(GPIO_POWER_ON_NUM, 1);
	gpio_set_level(GPIO_POWER_OFF_NUM, 0);

	return true;
}

bool power_off(void)
{
	//gpio_set_level(GPIO_POWER_ON_NUM, 0);
	gpio_set_level(GPIO_POWER_OFF_NUM, 1);

	return true;
}

#endif

bool system_shutdown(void)
{
	REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
	REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
	
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x0, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x0, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x1, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);

    esp_deep_sleep_start();
	
	return true;
}

bool power_peripheral_init(void)
{
	static bool is_inited = false;

	if (is_inited == true)
	{
		return true;
	}
	
	gpio_config_t  io_conf;
	io_conf.pin_bit_mask = GPIO_POWER_CHARGE_SEL;// | GPIO_POWER_BUTTON_SEL;
	io_conf.mode		 = GPIO_MODE_INPUT;
	io_conf.pull_up_en	 = 0;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf.intr_type	 = GPIO_PIN_INTR_DISABLE;
	gpio_config(&io_conf);
#if 0	
	io_conf.pin_bit_mask = GPIO_POWER_ON_SEL | GPIO_POWER_OFF_SEL;
	io_conf.mode		 = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en	 = 0;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf.intr_type	 = GPIO_PIN_INTR_DISABLE;
	gpio_config(&io_conf);


	is_inited = true;

	power_on();
#endif	
	return true;
}


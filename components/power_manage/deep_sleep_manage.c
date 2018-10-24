#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_deep_sleep.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"

#include "esp_shell.h"
#include "TerminalControlService.h"
#include "ES8388_interface.h"
#include "userconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "rom/gpio.h"

void deep_sleep_manage()
{
    // SET_PERI_REG_MASK(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);
    // CLEAR_PERI_REG_MASK(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);

    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_XPD_SDIO_REG);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_FORCE);

    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFH_SDIO, 0x0, RTC_CNTL_DREFH_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFM_SDIO, 0x0, RTC_CNTL_DREFM_SDIO_S);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_DREFL_SDIO, 0x1, RTC_CNTL_DREFL_SDIO_S);
    REG_CLR_BIT(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_TIEH);
    SET_PERI_REG_BITS(RTC_CNTL_SDIO_CONF_REG, RTC_CNTL_SDIO_PD_EN, 1, RTC_CNTL_SDIO_PD_EN_S);

    esp_deep_sleep_start();
}

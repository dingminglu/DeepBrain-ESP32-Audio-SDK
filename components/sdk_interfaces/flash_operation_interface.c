#include "esp_spi_flash.h"

bool flash_op_read(
	const uint32_t addr, 
	uint8_t* const data, 
	const uint32_t data_len)
{
	if (spi_flash_read(addr, data, data_len) == ESP_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool flash_op_write(
	const uint32_t addr, 
	const uint8_t* const data, 
	const uint32_t data_len)
{
	if (spi_flash_erase_range(addr, data_len) != ESP_OK)
	{
		return false;
	}

	if (spi_flash_write(addr, data, data_len) == ESP_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool flash_op_erase(
	const uint32_t addr, 
	const uint32_t length)
{
	if (spi_flash_erase_range(addr, length) == ESP_OK)
	{
		return true;
	}
	else
	{
		return false;
	}
}


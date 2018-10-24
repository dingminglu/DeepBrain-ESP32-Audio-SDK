#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "device_params_manage.h"
#include "debug_log_interface.h"
#include "userconfig.h"
#include "asr_service.h"
#include "crc_interface.h"

#define LOG_TAG "DEVICE PARAMS"

static DEVICE_PARAMS_CONFIG_T *g_device_params = NULL;
static PLATFORM_AI_ACOUNTS_T *g_ai_acounts = NULL;
static void* g_lock_flash_config = NULL;
//static int g_asr_mode = ASR_ENGINE_TYPE_AMRNB;

uint32_t get_device_params_crc(DEVICE_PARAMS_CONFIG_T *device_params)
{
	uint32_t crc = crypto_crc32(UINT32_MAX, &device_params->params_version, sizeof(DEVICE_PARAMS_CONFIG_T) - sizeof(DEVICE_PARAMS_BASE_t));
	return crc;
}

void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos)
{
	int i = 0;
	
	if (_wifi_infos == NULL)
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, ">>>>>>>>wifi info>>>>>>>>");
	DEBUG_LOGI(LOG_TAG, "connect_index[%d],storage_index[%d]",
		_wifi_infos->wifi_connect_index+1, _wifi_infos->wifi_storage_index+1);
	
	for (i=0; i < MAX_WIFI_NUM; i++)
	{
		DEBUG_LOGI(LOG_TAG, "[%02d]:ssid[%s]:password[%s]", 
			i+1, _wifi_infos->wifi_info[i].wifi_ssid, _wifi_infos->wifi_info[i].wifi_passwd);
	}
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_basic_info(DEVICE_BASIC_INFO_T *_basic_info)
{
	if (_basic_info == NULL)
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, ">>>>>>>>basic info>>>>>>>");
	DEBUG_LOGI(LOG_TAG, "bind user id:%s", _basic_info->bind_user_id);
	DEBUG_LOGI(LOG_TAG, "device sn:%s", _basic_info->device_sn);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_other_params(DEVICE_PARAMS_CONFIG_T *_other_info)
{
	if (_other_info == NULL)
	{
		return;
	}
	
	DEBUG_LOGI(LOG_TAG, ">>>>>>>>other info>>>>>>>");
	DEBUG_LOGI(LOG_TAG, "flash music index:%d", _other_info->flash_music_index);
	DEBUG_LOGI(LOG_TAG, "sd music index:%d", _other_info->sd_music_folder_index);
	if (_other_info->ota_mode == OTA_UPDATE_MODE_FORMAL)
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	else if (_other_info->ota_mode == OTA_UPDATE_MODE_TEST)
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:测试地址[0x%x]", _other_info->ota_mode);
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	DEBUG_LOGI(LOG_TAG, ">>>>>>>>other info>>>>>>>");
	//DEBUG_LOGI(LOG_TAG, "asr mode:%d", g_asr_mode);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_device_params(void)
{
	xSemaphoreTake(g_lock_flash_config, portMAX_DELAY);
	DEBUG_LOGI(LOG_TAG, ">>>>>>params version>>>>>>");	
	DEBUG_LOGI(LOG_TAG, "param size:%d", sizeof(DEVICE_PARAMS_CONFIG_T));
	DEBUG_LOGI(LOG_TAG, "param version:%08x", g_device_params->params_version);
	DEBUG_LOGI(LOG_TAG, "sdk version:%s", ESP32_FW_VERSION);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");

	print_wifi_infos(&g_device_params->wifi_infos);
	print_basic_info(&g_device_params->basic_info);
	print_other_params(g_device_params);
	
	xSemaphoreGive(g_lock_flash_config);
}

//保存参数
static DEVICE_PARAMS_ERRNO_t save_device_params(void)
{
	DEVICE_PARAMS_ERRNO_t ret = DEVICE_PARAMS_ERRNO_OK;
	
	g_device_params->device_params_header.crc32 = get_device_params_crc(g_device_params);
	if (!flash_op_write(PARTITIONS_ADDR_CONFIG_1, g_device_params, PARTITIONS_SIZE_CONFIG_1))
	{
		DEBUG_LOGE(LOG_TAG, "write_flash_params g_device_params failed");
		ret = DEVICE_PARAMS_ERRNO_FAIL;
	}
				
	return ret;
}

int get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			*(int*)_value = g_device_params->flash_music_index;
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			*(int*)_value = g_device_params->sd_music_folder_index;
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			memcpy((char*)_value, (char*)&g_device_params->wifi_infos, sizeof(g_device_params->wifi_infos));
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.bind_user_id), 
				"%s", g_device_params->basic_info.bind_user_id);
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.device_sn), 
				"%s", g_device_params->basic_info.device_sn);
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{
			memcpy((MEMO_ARRAY_T *)_value, (MEMO_ARRAY_T *)&g_device_params->memo_array, sizeof(MEMO_ARRAY_T));
			break;
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode == OTA_UPDATE_MODE_FORMAL)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			else if (g_device_params->ota_mode == OTA_UPDATE_MODE_TEST)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_TEST;
			}
			else
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			//*(uint32_t*)_value = g_asr_mode;
			break;
		}
		case FLASH_CFG_APP_ID:
		{
			strcpy((char*)_value, DEEP_BRAIN_APP_ID);
			break;
		}
		case FLASH_CFG_ROBOT_ID:
		{
			strcpy((char*)_value, DEEP_BRAIN_ROBOT_ID);
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);

	return 0;
}

bool get_dcl_auth_params(DCL_AUTH_PARAMS_t *dcl_auth_params)
{
	if (dcl_auth_params == NULL)
	{
		return false;
	}

	snprintf(dcl_auth_params->str_app_id, sizeof(dcl_auth_params->str_app_id), "%s", DEEP_BRAIN_APP_ID);
	snprintf(dcl_auth_params->str_robot_id, sizeof(dcl_auth_params->str_robot_id), "%s", DEEP_BRAIN_ROBOT_ID);
	get_flash_cfg(FLASH_CFG_USER_ID, dcl_auth_params->str_user_id);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, dcl_auth_params->str_device_id);
	snprintf(dcl_auth_params->str_city_name, sizeof(dcl_auth_params->str_city_name), " ");

	return true;
}

int set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	int ret = 0;

	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			if (g_device_params->flash_music_index != *(int*)_value)
			{
				g_device_params->flash_music_index = *(int*)_value;
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_FLASH_MUSIC_INDEX fail");
				}
			}
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			if (g_device_params->sd_music_folder_index != *(int*)_value)
			{
				g_device_params->sd_music_folder_index = *(int*)_value;
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_SDCARD_FOLDER_INDEX fail");
				}
			}
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			if (memcmp((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos)) != 0)
			{
				memcpy((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos));
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_WIFI_INFO fail");
				}
			}
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			if (strcmp(g_device_params->basic_info.bind_user_id, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.bind_user_id, sizeof(g_device_params->basic_info.bind_user_id),
					"%s", (char*)_value);
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGI(LOG_TAG, "save_device_params FLASH_CFG_USER_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			if (strcmp(g_device_params->basic_info.device_sn, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.device_sn, sizeof(g_device_params->basic_info.device_sn),
					"%s", (char*)_value);
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_DEVICE_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{
			memcpy((MEMO_ARRAY_T *)&g_device_params->memo_array, (MEMO_ARRAY_T *)_value, sizeof(MEMO_ARRAY_T));
			ret = save_device_params();
			if (ret == DEVICE_PARAMS_ERRNO_FAIL)
			{
				DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_MEMO_PARAMS fail");
			}
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode != *(uint32_t*)_value)
			{
				if (*(uint32_t*)_value == OTA_UPDATE_MODE_FORMAL)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				else if (*(uint32_t*)_value == OTA_UPDATE_MODE_TEST)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_TEST;
				}
				else
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				
				ret = save_device_params();
				if (ret == DEVICE_PARAMS_ERRNO_FAIL)
				{
					DEBUG_LOGI(LOG_TAG, "save_device_params FLASH_CFG_OTA_MODE fail");
				}
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			int mode = *(uint32_t*)_value;
//			if (mode >= ASR_RECORD_TYPE_AMRNB && mode <= ASR_RECORD_TYPE_PCM_SINOVOICE_16K)
//			{
//				g_asr_mode = mode;
//			}
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);
	
	return ret;
}

void init_device_params_v1(DEVICE_PARAMS_CONFIG_T *_config)
{
	memset(_config, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	_config->params_version = DEVICE_PARAMS_VERSION_1;
	snprintf(_config->wifi_infos.wifi_info[0].wifi_ssid, sizeof(_config->wifi_infos.wifi_info[0].wifi_ssid),
		WIFI_SSID_DEFAULT);
	snprintf(_config->wifi_infos.wifi_info[0].wifi_passwd, sizeof(_config->wifi_infos.wifi_info[0].wifi_passwd),
		WIFI_PASSWD_DEFAULT);
}

void init_default_ai_sinovoice_acount(SINOVOICE_ACOUNT_T *_sinovice_acount)
{
	if (_sinovice_acount == NULL)
	{
		return;
	}

	snprintf(_sinovice_acount->asr_url, sizeof(_sinovice_acount->asr_url), "%s", SINOVOICE_DEFAULT_ASR_URL);
	snprintf(_sinovice_acount->tts_url, sizeof(_sinovice_acount->tts_url), "%s", SINOVOICE_DEFAULT_TTS_URL);
	snprintf(_sinovice_acount->app_key, sizeof(_sinovice_acount->app_key), "%s", SINOVOICE_DEFAULT_APP_KEY);
	snprintf(_sinovice_acount->dev_key, sizeof(_sinovice_acount->dev_key), "%s", SINOVOICE_DEFAULT_DEV_KEY);
}

void init_default_ai_baidu_acount(BAIDU_ACOUNT_T *_baidu_acount)
{
	if (_baidu_acount == NULL)
	{
		return;
	}

	snprintf(_baidu_acount->asr_url, sizeof(_baidu_acount->asr_url), "%s", BAIDU_DEFAULT_ASR_URL);
	snprintf(_baidu_acount->tts_url, sizeof(_baidu_acount->tts_url), "%s", BAIDU_DEFAULT_TTS_URL);
	snprintf(_baidu_acount->app_id, sizeof(_baidu_acount->app_id), "%s", BAIDU_DEFAULT_APP_ID);
	snprintf(_baidu_acount->app_key, sizeof(_baidu_acount->app_key), "%s", BAIDU_DEFAULT_APP_KEY);
	snprintf(_baidu_acount->secret_key, sizeof(_baidu_acount->secret_key), "%s", BAIDU_DEFAULT_SECRET_KEY);
}

void init_default_ai_acounts(PLATFORM_AI_ACOUNTS_T *_accounts)
{
	if (_accounts == NULL)
	{
		return;
	}

	memset(_accounts, 0, sizeof(PLATFORM_AI_ACOUNTS_T));
	
	init_default_ai_sinovoice_acount(&_accounts->st_sinovoice_acount);
	init_default_ai_baidu_acount(&_accounts->st_baidu_acount);
}

DEVICE_PARAMS_ERRNO_t init_device_params(void)
{
	DEVICE_PARAMS_ERRNO_t ret = DEVICE_PARAMS_ERRNO_OK;

	if (PARTITIONS_SIZE_CONFIG_1 != sizeof(DEVICE_PARAMS_CONFIG_T))
	{
		DEBUG_LOGE(LOG_TAG, "device params size error, [%d]!=[%d]", 
			PARTITIONS_SIZE_CONFIG_1, sizeof(PLATFORM_AI_ACOUNTS_T));
		while(1);	
	}

	SEMPHR_CREATE_LOCK(g_lock_flash_config);

	g_ai_acounts = memory_malloc(sizeof(PLATFORM_AI_ACOUNTS_T));
	if (g_ai_acounts == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "esp32_malloc g_ai_acounts fail");
		return DEVICE_PARAMS_ERRNO_FAIL;
	}
	init_default_ai_acounts(g_ai_acounts);
	
	g_device_params = memory_malloc(sizeof(DEVICE_PARAMS_CONFIG_T));
	if (g_device_params == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "esp32_malloc g_device_params fail");
		return DEVICE_PARAMS_ERRNO_FAIL;
	}
	memset(g_device_params, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	
	flash_op_read(PARTITIONS_ADDR_CONFIG_1, g_device_params, PARTITIONS_SIZE_CONFIG_1);
	if (get_device_params_crc(g_device_params) != g_device_params->device_params_header.crc32)
	{
		DEBUG_LOGE(LOG_TAG, "g_device_params crc have a change!");
		memset(g_device_params, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	}
	DEBUG_LOGE(LOG_TAG, "curr crc is [%d] !", get_device_params_crc(g_device_params));

	if (g_device_params->params_version != DEVICE_PARAMS_VERSION_1) 
	{
		init_device_params_v1(g_device_params);
		save_device_params();
	}

	print_device_params();
	
	return ret;
}

void get_ai_acount(AI_ACOUNT_T _type, void *_out)
{
	if (g_ai_acounts == NULL)
	{
		return;
	}
	
	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_type)
	{
		case AI_ACOUNT_BAIDU:
		{
			if (g_ai_acounts == NULL)
			{
				init_default_ai_baidu_acount((BAIDU_ACOUNT_T *)_out);
			}
			else
			{
				memcpy(_out, &g_ai_acounts->st_baidu_acount, sizeof(g_ai_acounts->st_baidu_acount));
			}
			break;
		}
		case AI_ACOUNT_SINOVOICE:
		{
			if (g_ai_acounts == NULL)
			{
				init_default_ai_sinovoice_acount((SINOVOICE_ACOUNT_T *)_out);
			}
			else
			{
				memcpy(_out, &g_ai_acounts->st_sinovoice_acount, sizeof(g_ai_acounts->st_sinovoice_acount));
			}
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);
}

void set_ai_acount(AI_ACOUNT_T _type, void *_out)
{
	if (g_ai_acounts == NULL)
	{
		return;
	}
	
	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_type)
	{
		case AI_ACOUNT_BAIDU:
		{
			memcpy(&g_ai_acounts->st_baidu_acount, _out, sizeof(g_ai_acounts->st_baidu_acount));
			break;
		}
		case AI_ACOUNT_SINOVOICE:
		{
			memcpy(&g_ai_acounts->st_sinovoice_acount, _out, sizeof(g_ai_acounts->st_sinovoice_acount));
			break;
		}
		case AI_ACOUNT_ALL:
		{
			memcpy(g_ai_acounts, _out, sizeof(PLATFORM_AI_ACOUNTS_T));
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);
}



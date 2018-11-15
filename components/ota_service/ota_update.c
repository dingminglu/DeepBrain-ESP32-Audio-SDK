/* OTA_TAG audio example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "socket.h"
#include "http_api.h"
#include "ota_update.h"
#include "cJSON.h"
#include "ota_service.h"
#include "userconfig.h"
#include "device_params_manage.h"
#include "player_middleware_interface.h"

typedef struct SOCKET_DATA_TMP_T
{
	sock_t sock;
	char port[6];
	char domain[64];
	char params[128];
	char header[256];
	char str_buf[1024];
	char str_buf_1[128];
	char str_buf_2[128];
	char device_sn[32];
}SOCKET_DATA_TMP_T;

//ota pack header 192字节
typedef struct OTA_PACK_HEADER_t
{
	char project_name[64];
	char res[192];
}OTA_PACK_HEADER_t;

static char *OTA_TAG = "OTA_UPDATE";
static char ota_write_data[1024] = {0};
static const char *GET_OTA_REQUEST_HEADER = "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: application/json; charset=utf-8\r\n\r\n";

static bool ota_fw_header_check(const char *data)
{
	OTA_PACK_HEADER_t *header = data;
	
	if (strncmp(data, PROJECT_NAME, strlen(PROJECT_NAME)) == 0)
	{	
		DEBUG_LOGI(OTA_TAG, "ota_fw_header_checksuccess, data=[%s]", header->project_name);
		return true;
	}
	else
	{
		DEBUG_LOGE(OTA_TAG, "ota_fw_header_check failed, data=[%s]", header->project_name);
		return false;		
	}
}

static esp_err_t ota_update_audio_begin(esp_partition_t *_wifi_partition, esp_ota_handle_t *out_handle)
{
	esp_err_t ret = esp_ota_begin(_wifi_partition, 0x0, out_handle);
    if (ret != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error: ota begin wifi failed,ret=%x", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_write(esp_ota_handle_t _handle, const void *_data, size_t _size)
{
	esp_err_t ret = esp_ota_write(_handle, _data, _size);
	
    if (ret != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error: write wifi handle failed,ret=%x, size=%d", ret, _size);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_end(esp_ota_handle_t _handle)
{
    if (esp_ota_end(_handle) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error: end wifi handle failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_set_boot_partition(esp_partition_t *_wifi_partition)
{
    if (esp_ota_set_boot_partition(_wifi_partition) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error: set boot wifi  failed");
        return -1;
    }

    return ESP_OK;
}

static OTA_ERROR_NUM_T ota_update_init(esp_ota_handle_t *_out_handle, esp_partition_t *_out_wifi_partition)
{
    const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();
    if ((esp_current_partition == NULL) || (esp_current_partition->type != ESP_PARTITION_TYPE_APP)) 
	{
        DEBUG_LOGE(OTA_TAG, "Error esp_current_partition->type[%x] != ESP_PARTITION_TYPE_APP[%x]",
			esp_current_partition->type,
			ESP_PARTITION_TYPE_APP);
        return OTA_INIT_FAIL;
    }
    DEBUG_LOGI("esp_current_partition:", "current type[%x]", esp_current_partition->type);
    DEBUG_LOGI("esp_current_partition:", "current subtype[%x]", esp_current_partition->subtype);
    DEBUG_LOGI("esp_current_partition:", "current address:0x%x", esp_current_partition->address);
    DEBUG_LOGI("esp_current_partition:", "current size:0x%x", esp_current_partition->size);
    DEBUG_LOGI("esp_current_partition:", "current labe:%s", esp_current_partition->label);

    esp_partition_t find_wifi_partition;
    memset(_out_wifi_partition, 0, sizeof(esp_partition_t));
    memset(&find_wifi_partition, 0, sizeof(esp_partition_t));
    /*choose which OTA_TAG audio image should we write to*/
    switch (esp_current_partition->subtype) {
    case ESP_PARTITION_SUBTYPE_APP_OTA_0:
        find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
        break;
    case  ESP_PARTITION_SUBTYPE_APP_OTA_1:
        find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    default:
		find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    }
    find_wifi_partition.type = ESP_PARTITION_TYPE_APP;
	DEBUG_LOGI("find_wifi_partition", "%d: subtype[%x]", __LINE__, find_wifi_partition.subtype);
    const esp_partition_t *wifi_partition = NULL;
    wifi_partition = esp_partition_find_first(find_wifi_partition.type, find_wifi_partition.subtype, NULL);
    assert(wifi_partition != NULL);
    memcpy(_out_wifi_partition, wifi_partition, sizeof(esp_partition_t));
    /*actual : we would not assign size for it would assign by default*/
    DEBUG_LOGI("wifi_partition", "%d: type[%x]", __LINE__, wifi_partition->type);
    DEBUG_LOGI("wifi_partition", "%d: subtype[%x]", __LINE__, wifi_partition->subtype);
    DEBUG_LOGI("wifi_partition", "%d: address:0x%x", __LINE__, wifi_partition->address);
    DEBUG_LOGI("wifi_partition", "%d: size:0x%x", __LINE__, wifi_partition->size);
    DEBUG_LOGI("wifi_partition", "%d: labe:%s", __LINE__,  wifi_partition->label);
	
    if (ota_update_audio_begin(_out_wifi_partition, _out_handle) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error: esp_ota_begin failed!");
        return OTA_INIT_FAIL;
    } 
	else 
    {
        DEBUG_LOGI(OTA_TAG, "Info: esp_ota_begin init OK!");
        return OTA_SUCCESS;
    }

    return OTA_INIT_FAIL;
}

static OTA_ERROR_NUM_T ota_update_data_from_sd(
	const char *_firmware_path,
	esp_ota_handle_t _sd_ota_handle,
	size_t *_out_firmware_lenght)
{
	OTA_ERROR_NUM_T ret = OTA_SD_FAIL;
	FILE *fp = NULL;
    fp = fopen(_firmware_path, "r");
    if (NULL == fp)
    {
    	DEBUG_LOGE(OTA_TAG, "open %s failed!", _firmware_path);
        return OTA_NO_SD_CARD;
    }
	DEBUG_LOGI(OTA_TAG, "open %s success", _firmware_path);
	
	int b_first_read = 1;
    while(1) 
	{
        memset(ota_write_data, 0, sizeof(ota_write_data));
		int buff_len = fread(ota_write_data, sizeof(char), sizeof(ota_write_data), fp);
		if (buff_len > 0) 
		{
			if (b_first_read == 1)
			{
				if (ota_write_data[0] != 0xE9 || buff_len < 2)
				{
					goto ota_update_data_from_sd_error;
				}
				b_first_read = 0;
			}
			
            if (ota_update_audio_write(_sd_ota_handle, (const void *)ota_write_data, buff_len) != ESP_OK) 
			{
                DEBUG_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_update_data_from_sd_error;
            }
            *_out_firmware_lenght += buff_len;
            DEBUG_LOGI(OTA_TAG, "Info: had written image length %d,%x", *_out_firmware_lenght, *_out_firmware_lenght);
        } 
		else if (buff_len == 0) 
       	{  /*read over*/
       		ret = OTA_SUCCESS;
            DEBUG_LOGI(OTA_TAG, "Info: receive all packet over!");
			break;
        } 
		else 
		{
            DEBUG_LOGI(OTA_TAG, "Warning: uncontolled event!");
            goto ota_update_data_from_sd_error;
        }
    }
	
ota_update_data_from_sd_error:
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	
    return ret;
}

static void str_to_lower(char *_src, char *_out_dest, size_t _len)
{
	int i = 0;

	if (_src == NULL || _out_dest == NULL || _len == 0)
	{
		return;
	}

	memset(_out_dest, 0, _len);
	for (i=0; i<_len - 1 && *(_src+i) != '\0'; i++)
	{
		*(_out_dest+i) = tolower((int)*(_src+i));
	}

	return;
}

static OTA_ERROR_NUM_T ota_update_data_from_server(
	const char *_server_url, 
	const esp_ota_handle_t _handle,
	size_t *_out_firmware_len)
{
	int 	total_len 	= 0;
	SOCKET_DATA_TMP_T *socket_data = (SOCKET_DATA_TMP_T *)memory_malloc(sizeof(SOCKET_DATA_TMP_T));
	if (socket_data == NULL)
	{
		DEBUG_LOGE(OTA_TAG, "memory_malloc fail");
		goto ota_data_from_server_error;
	}
	memset(socket_data, 0, sizeof(SOCKET_DATA_TMP_T));

	socket_data->sock = INVALID_SOCK;
	
	if (sock_get_server_info(_server_url, (char*)&socket_data->domain, (char*)&socket_data->port, (char*)&socket_data->params) != 0)
	{
		DEBUG_LOGE(OTA_TAG, "sock_get_server_info fail");
		goto ota_data_from_server_error;
	}

	socket_data->sock = sock_connect(socket_data->domain, socket_data->port);
	if (socket_data->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(OTA_TAG, "sock_connect fail");
		goto ota_data_from_server_error;
	}

	sock_set_nonblocking(socket_data->sock);

	snprintf(socket_data->header, sizeof(socket_data->header), GET_OTA_REQUEST_HEADER, socket_data->params, socket_data->domain);
	if (sock_writen_with_timeout(socket_data->sock, socket_data->header, strlen(socket_data->header), 1000) != strlen(socket_data->header)) 
	{
		DEBUG_LOGE(OTA_TAG, "sock_write http header fail");
		goto ota_data_from_server_error;
	}
	DEBUG_LOGE(OTA_TAG, "header=%s\n", socket_data->header);
	
    bool pkg_body_start = false;
	//uint32_t play_tone_time = time(NULL);
	
    /*deal with all receive packet*/
    while (1) 
	{
	
#if 0
		if (abs(play_tone_time - time(NULL)) >= 5)
		{
			play_tone_time = time(NULL);
			audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
		}
	
		vTaskDelay(5);//避免完全占用cpu，导致提示音无法播报
#endif

        memset(socket_data->str_buf, 0, sizeof(socket_data->str_buf));
        int buff_len = sock_readn_with_timeout(socket_data->sock, socket_data->str_buf, sizeof(socket_data->str_buf) - 1, 5000);
        if (buff_len < 0 && (errno == EINTR || errno == EAGAIN))
		{
			DEBUG_LOGE(OTA_TAG, "recv total len: %d bytes", *_out_firmware_len);
			if (total_len == *_out_firmware_len)
			{
				break;
			}
			
			continue;
		}
		else if (buff_len < 0)
		{ /*receive error*/
            DEBUG_LOGE(OTA_TAG, "Error: receive data error!");
            goto ota_data_from_server_error;
        } 
		else if (buff_len > 0 && !pkg_body_start) 
		{ /*deal with packet header*/
			if (http_get_error_code(socket_data->str_buf) != 200)
			{
				DEBUG_LOGE(OTA_TAG, "http reply=%s", socket_data->str_buf);
				goto ota_data_from_server_error;
			}

			total_len = http_get_content_length(socket_data->str_buf);
			if (total_len <= sizeof(OTA_PACK_HEADER_t))
			{
				DEBUG_LOGE(OTA_TAG, "http reply=%s", socket_data->str_buf);
				goto ota_data_from_server_error;
			}
			total_len -= sizeof(OTA_PACK_HEADER_t);
			DEBUG_LOGE(OTA_TAG, "ota_update_data_from_server firmware total len = %d\r\n", total_len);

			char* pBody = http_get_body(socket_data->str_buf);
			if (NULL == pBody)
			{
				DEBUG_LOGE(OTA_TAG, "http reply=%s", socket_data->str_buf);
				goto ota_data_from_server_error;
			}
			buff_len -= pBody - socket_data->str_buf;

			if (!ota_fw_header_check(pBody))
			{
				DEBUG_LOGE(OTA_TAG, "ota_fw_header_check failed");
				goto ota_data_from_server_error;
			}
			pBody += sizeof(OTA_PACK_HEADER_t);
			buff_len -= sizeof(OTA_PACK_HEADER_t);
		
			//check firmware head byte
			if (*pBody != 0xE9 || buff_len < 2)
			{
				DEBUG_LOGE(OTA_TAG, "buff_len=%d,pBody=%s", buff_len, pBody);
				goto ota_data_from_server_error;
			}
			if (ota_update_audio_write(_handle, (const void *)pBody, buff_len) != ESP_OK) 
			{
                DEBUG_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_data_from_server_error;
            }
			*_out_firmware_len = buff_len;
            pkg_body_start = true;
        } 
		else if (buff_len > 0 && pkg_body_start) 
		{ /*deal with packet body*/
            if (ota_update_audio_write(_handle, (const void *)socket_data->str_buf, buff_len) != ESP_OK) 
			{
                DEBUG_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_data_from_server_error;
            }
            *_out_firmware_len += buff_len;
        } 
		else if (buff_len == 0) 
		{  /*packet over*/
			if (total_len == *_out_firmware_len)
			{
				break;
			}
			else
			{
				goto ota_data_from_server_error;
			}
        } 
		else 
		{
            goto ota_data_from_server_error;
        }
    }
	
	DEBUG_LOGI(OTA_TAG, "Info: total len=%d, had written image length %d", total_len, *_out_firmware_len);
	sock_close(socket_data->sock);
	return OTA_SUCCESS;
	
ota_data_from_server_error:
	
	DEBUG_LOGE(OTA_TAG, "Info: total len=%d, had written image length %d", total_len, *_out_firmware_len);
	sock_close(socket_data->sock);
	return OTA_WIFI_FAIL;
}


OTA_ERROR_NUM_T ota_update_from_sd(const char *_firmware_path)
{
	size_t firmware_len = 0;
	esp_ota_handle_t sd_ota_handle;
	esp_partition_t wifi_partition;
    memset(&wifi_partition, 0, sizeof(wifi_partition));

	if (_firmware_path == NULL || strlen(_firmware_path) <= 10)
	{
		return OTA_SD_FAIL;
	}

    /*begin ota*/
    if (ota_update_init(&sd_ota_handle, &wifi_partition) == OTA_SUCCESS) 
	{
        DEBUG_LOGI(OTA_TAG, "Info: OTA Init success!");
    } 
	else 
   	{
        DEBUG_LOGE(OTA_TAG, "Error: OTA Init failed");		
        return OTA_SD_FAIL;
    }
	
    /*if wifi ota exist,would ota it*/
	if (ota_update_data_from_sd(_firmware_path, sd_ota_handle, &firmware_len) != OTA_SUCCESS)
	{
		DEBUG_LOGI(OTA_TAG, "sd_ota_data_from_sd failed!");		
		return OTA_SD_FAIL;
	}
	DEBUG_LOGI(OTA_TAG, "Info: Total Write wifi binary data length : %d", firmware_len);

	/*ota end*/
    if (ota_update_audio_end(sd_ota_handle) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error : esp_ota_end failed!");		
        return OTA_SD_FAIL;
    }
    /*set boot*/
    if (ota_update_audio_set_boot_partition(&wifi_partition) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error : esp_ota_set_boot_partition failed!");
        return OTA_SD_FAIL;
    }
    DEBUG_LOGI(OTA_TAG, "Prepare to restart system!");

    return OTA_SUCCESS;
}

OTA_ERROR_NUM_T ota_update_from_wifi(const char *_firmware_url)
{
	size_t firmware_len = 0;
	esp_ota_handle_t sd_ota_handle;
	esp_partition_t wifi_partition;
    memset(&wifi_partition, 0, sizeof(wifi_partition));

	if (_firmware_url == NULL || strlen(_firmware_url) <= 10)
	{
		return OTA_WIFI_FAIL;
	}

    /*begin ota*/
    if (ota_update_init(&sd_ota_handle, &wifi_partition) == OTA_SUCCESS)
	{
        DEBUG_LOGI(OTA_TAG, "Info: OTA Init success!");
    } 
	else 
   	{
        DEBUG_LOGE(OTA_TAG, "Error: OTA Init failed");
        return OTA_WIFI_FAIL;
    }
    /*if wifi ota exist,would ota it*/
	if (ota_update_data_from_server(_firmware_url, sd_ota_handle, &firmware_len) != OTA_SUCCESS)
	{
		DEBUG_LOGE(OTA_TAG, "ota_data_from_server failed!");
		return OTA_WIFI_FAIL;
	}
	DEBUG_LOGI(OTA_TAG, "Info: Total Write wifi binary data length : %d", firmware_len);

	/*ota end*/
    if (ota_update_audio_end(sd_ota_handle) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error : esp_ota_end failed!");
        return OTA_WIFI_FAIL;
    }

	/*set boot*/
    if (ota_update_audio_set_boot_partition(&wifi_partition) != ESP_OK) 
	{
        DEBUG_LOGE(OTA_TAG, "Error : esp_ota_set_boot_partition failed!");
        return OTA_WIFI_FAIL;
    }

    return OTA_SUCCESS;
}

OTA_ERROR_NUM_T ota_update_check_fw_version(
	const char *_server_url,
	char *_out_fw_url,
	size_t _out_len)
{
	cJSON   *pJson_body = NULL;

	SOCKET_DATA_TMP_T *socket_data = (SOCKET_DATA_TMP_T *)memory_malloc(sizeof(SOCKET_DATA_TMP_T));
	if (socket_data == NULL)
	{
		DEBUG_LOGE(OTA_TAG, "memory_malloc fail");
		goto ota_update_check_fw_version_error;
	}
	memset(socket_data, 0, sizeof(SOCKET_DATA_TMP_T));

	socket_data->sock = INVALID_SOCK;
	get_flash_cfg(FLASH_CFG_DEVICE_ID, socket_data->device_sn);
	
	//char *socket_data->device_sn = "huba18012000000001";
	/*
	if (strlen(socket_data->device_sn) <= 0)
	{
		DEBUG_LOGE(OTA_TAG, "device sn is empty, please activate device first");
		goto ota_update_check_fw_version_error;
	}
	*/
	if (sock_get_server_info(_server_url, (char*)&socket_data->domain, (char*)&socket_data->port, (char*)&socket_data->params) != 0)
	{
		DEBUG_LOGE(OTA_TAG, "sock_get_server_info fail");
		goto ota_update_check_fw_version_error;
	}

	socket_data->sock = sock_connect(socket_data->domain, socket_data->port);
	if (socket_data->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(OTA_TAG, "sock_connect fail");
		goto ota_update_check_fw_version_error;
	}
	sock_set_nonblocking(socket_data->sock);

	snprintf(socket_data->str_buf, sizeof(socket_data->str_buf), GET_OTA_REQUEST_HEADER, socket_data->params, socket_data->domain);
	if (sock_writen_with_timeout(socket_data->sock, socket_data->str_buf, strlen(socket_data->str_buf), 2000) != strlen(socket_data->str_buf)) 
	{
		DEBUG_LOGE(OTA_TAG, "sock_write http header fail");
		goto ota_update_check_fw_version_error;
	}
	//DEBUG_LOGE(OTA_TAG, "socket_data->header=%s\n", socket_data->str_buf);
	
	memset(socket_data->str_buf, 0, sizeof(socket_data->str_buf));
	sock_readn_with_timeout(socket_data->sock, socket_data->str_buf, sizeof(socket_data->str_buf) - 1, 5000);
	sock_close(socket_data->sock);
	//DEBUG_LOGE(OTA_TAG, "reply=%s\n", socket_data->str_buf);
	if (http_get_error_code(socket_data->str_buf) != 200)
	{
		DEBUG_LOGE(OTA_TAG, "%s", socket_data->str_buf);
		goto ota_update_check_fw_version_error;
	}
	else
	{
		char* str_body = http_get_body(socket_data->str_buf);
		//DEBUG_LOGE(OTA_TAG, "str_body is %s", str_reply);
		if (str_body == NULL)
		{
			goto ota_update_check_fw_version_error;
		}
	
		pJson_body = cJSON_Parse(str_body);
	    if (NULL != pJson_body) 
		{
	        cJSON *pJson_version = cJSON_GetObjectItem(pJson_body, "version");
			if (NULL == pJson_version || pJson_version->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}

			str_to_lower(pJson_version->valuestring, socket_data->str_buf_1, sizeof(socket_data->str_buf_1));
			str_to_lower(ESP32_FW_VERSION, socket_data->str_buf_2, sizeof(socket_data->str_buf_2));
			DEBUG_LOGE(OTA_TAG, "server version=%s", socket_data->str_buf_1);
			DEBUG_LOGE(OTA_TAG, "client version=%s", socket_data->str_buf_2);
			if (strcmp(socket_data->str_buf_1, socket_data->str_buf_2) == 0)
			{
				goto ota_update_check_fw_version_error;
			}
/*
			cJSON *pJson_start_sn = cJSON_GetObjectItem(pJson_body, "start_sn");
			if (NULL == pJson_start_sn || pJson_start_sn->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}
			
			str_to_lower(pJson_start_sn->valuestring, socket_data->str_buf_1, sizeof(socket_data->str_buf_1));
			str_to_lower(socket_data->device_sn, socket_data->str_buf_2, sizeof(socket_data->str_buf_2));
			DEBUG_LOGE(OTA_TAG, "server sn=%s", socket_data->str_buf_1);
			DEBUG_LOGE(OTA_TAG, "client sn=%s", socket_data->str_buf_2);
			if (strlen(socket_data->str_buf_1) != DEVICE_SN_MAX_LEN || strlen(socket_data->str_buf_2) != DEVICE_SN_MAX_LEN)
			{
				DEBUG_LOGE(OTA_TAG, "sn len is not %d", DEVICE_SN_MAX_LEN);
				goto ota_update_check_fw_version_error;
			}
			
			if (strcmp(socket_data->str_buf_1 + 10, socket_data->str_buf_2 + 10) > 0)
			{
				DEBUG_LOGE(OTA_TAG, "%s > %s", socket_data->str_buf_1, socket_data->str_buf_2);
				goto ota_update_check_fw_version_error;
			}

			cJSON *pJson_end_sn = cJSON_GetObjectItem(pJson_body, "end_sn");
			if (NULL == pJson_end_sn || pJson_end_sn->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}

			str_to_lower(pJson_end_sn->valuestring, socket_data->str_buf_1, sizeof(socket_data->str_buf_1));
			DEBUG_LOGE(OTA_TAG, "server sn=%s", socket_data->str_buf_1);
			DEBUG_LOGE(OTA_TAG, "client sn=%s", socket_data->str_buf_2);

			if (strlen(socket_data->str_buf_1) != DEVICE_SN_MAX_LEN || strlen(socket_data->str_buf_2) != DEVICE_SN_MAX_LEN)
			{
				DEBUG_LOGE(OTA_TAG, "sn len is not %d", DEVICE_SN_MAX_LEN);
				goto ota_update_check_fw_version_error;
			}
			
			if (strcmp(socket_data->str_buf_1 + 10, socket_data->str_buf_2 + 10) < 0)
			{
				DEBUG_LOGE(OTA_TAG, "%s < %s", socket_data->str_buf_1, socket_data->str_buf_2);
				goto ota_update_check_fw_version_error;
			}
*/		
			cJSON *pJson_download_url = cJSON_GetObjectItem(pJson_body, "download_url");
			if (NULL == pJson_download_url || pJson_download_url->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}
			
			snprintf(_out_fw_url, _out_len, "%s", pJson_download_url->valuestring);
			
			if (NULL != pJson_body)
			{
				cJSON_Delete(pJson_body);
			}
	    }
		else
		{
			goto ota_update_check_fw_version_error;
		}
	}

	if (socket_data->sock != INVALID_SOCK)
	{
		sock_close(socket_data->sock);
	}

	if (socket_data != NULL)
	{
		memory_free(socket_data);
	}

	return OTA_NEED_UPDATE_FW;
	
ota_update_check_fw_version_error:
	
	if (socket_data->sock != INVALID_SOCK)
	{
		sock_close(socket_data->sock);
	}

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	if (socket_data != NULL)
	{
		memory_free(socket_data);
	}
	
	return OTA_NONEED_UPDATE_FW;
}


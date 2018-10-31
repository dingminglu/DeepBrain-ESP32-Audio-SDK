#include <stdlib.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "userconfig.h"
#include "wifi_manage.h"
#include "socket.h"
#include "player_middleware_interface.h"
#include "cJSON.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_event.h"

#define LOG_TAG "wifi manage"

//wifi ap server handle
typedef struct WIFI_AP_SERVER_HANDLE_t
{
	WIFI_AP_SERVER_STATUS_t status;
	sock_t listen_sock;
	sock_t connect_sock;
	char buffer[512];
}WIFI_AP_SERVER_HANDLE_t;

//wifi manage handle
typedef struct WIFI_MANAGE_HANDLE_t
{
	WIFI_MANAGE_STATUS_t status;
	WIFI_AP_SERVER_HANDLE_t ap_server;
	DEVICE_WIFI_INFOS_T wifi_infos;
	DEVICE_WIFI_INFO_T curr_wifi;
	uint64_t connecting_start_time;
}WIFI_MANAGE_HANDLE_t;

static WIFI_MANAGE_HANDLE_t* g_wifi_manage_handle = NULL;
void *g_lock_wifi_manage = NULL;

/**
 * update wifi connect time
 *
 * @param none
 * @return true/false
 */
static bool update_wifi_connect_time(void)
{
	if (g_wifi_manage_handle == NULL || g_lock_wifi_manage == NULL)
	{
		return false;
	}

	SEMPHR_TRY_LOCK(g_lock_wifi_manage);
	g_wifi_manage_handle->connecting_start_time = get_time_of_day();
	SEMPHR_TRY_UNLOCK(g_lock_wifi_manage);

	return true;
}

/**
 * update current wifi infomation
 *
 * @param [in]ssid,the name of the wifi
 * @param [in]password
 * @return true/false
 */
static bool update_current_wifi_info(
	const char *ssid, 
	const char *password)
{
	DEVICE_WIFI_INFO_T *wifi_info = NULL;
	
	if (g_wifi_manage_handle == NULL 
		|| g_lock_wifi_manage == NULL
		|| ssid == NULL
		|| strlen(ssid) == 0
		|| password == NULL)
	{
		return false;
	}

	wifi_info = &g_wifi_manage_handle->curr_wifi;
	SEMPHR_TRY_LOCK(g_lock_wifi_manage);
	snprintf(wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid), "%s", ssid);
	snprintf(wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd), "%s", password);
	SEMPHR_TRY_UNLOCK(g_lock_wifi_manage);

	return true;
}

/**
 * set wifi status
 *
 * @param [in]status,wifi manage status
 * @return none
 */
static void set_wifi_manage_status(WIFI_MANAGE_STATUS_t status)
{
	if (g_wifi_manage_handle == NULL || g_lock_wifi_manage == NULL)
	{
		return false;
	}

	SEMPHR_TRY_LOCK(g_lock_wifi_manage);
	g_wifi_manage_handle->status = status;
	SEMPHR_TRY_UNLOCK(g_lock_wifi_manage);

	return true;
}

/**
 * get wifi status
 *
 * @param none
 * @return int
 */
int get_wifi_manage_status(void)
{
	int status = -1;
	
	if (g_wifi_manage_handle == NULL || g_lock_wifi_manage == NULL)
	{
		return status;
	}

	SEMPHR_TRY_LOCK(g_lock_wifi_manage);
	status = g_wifi_manage_handle->status;
	SEMPHR_TRY_UNLOCK(g_lock_wifi_manage);
	//DEBUG_LOGE(LOG_TAG, "get_wifi_manage_status[%d]", status);
	
	return status;
}

/**
 * judge wheather the mode is wifi sta mode
 *
 * @param none
 * @return true/false
 */
static bool is_wifi_sta_mode(void)
{	
	int status = get_wifi_manage_status();

	if (status < 0)
	{
		return false;
	}
	
	if ((status&0xff00) == WIFI_MANAGE_STATUS_STA)
	{
		return true;
	}

	return false;
}

/**
 * judge wheather the mode is wifi smartconfig mode
 *
 * @param none
 * @return true/false
 */
static bool is_wifi_smartconfig_mode(void)
{	
	int status = get_wifi_manage_status();

	if (status < 0)
	{
		return false;
	}
	
	if ((status&0xff00) == WIFI_MANAGE_STATUS_SMARTCONFIG)
	{
		return true;
	}

	return false;
}

/**
 * wifi event handle callback
 *
 * @param [out]ctx
 * @param [in]evt,system event
 * @return esp_err_t
 */
static esp_err_t wifi_event_handle_cb(void* ctx, system_event_t* evt)
{
    if (evt == NULL) 
	{
        return ESP_FAIL;
    }
	
    switch (evt->event_id) 
	{
	    case SYSTEM_EVENT_WIFI_READY:
	    {
	        DEBUG_LOGI(LOG_TAG, "+WIFI:READY");
	        break;
	    }
	    case SYSTEM_EVENT_SCAN_DONE:
	    {
			DEBUG_LOGI(LOG_TAG, "+SCANDONE");
	        break;
	    }
	    case SYSTEM_EVENT_STA_START:
	    {
			DEBUG_LOGI(LOG_TAG, "+WIFI:STA_START");
	        break;
	    }
	    case SYSTEM_EVENT_STA_STOP:
	    {
	    	DEBUG_LOGI(LOG_TAG, "+WIFI:STA_STOP");
	        break;
	    }
	    case SYSTEM_EVENT_STA_CONNECTED:
	    {
	        DEBUG_LOGI(LOG_TAG, "+JAP:WIFICONNECTED");
	        break;
	    }
	    case SYSTEM_EVENT_STA_DISCONNECTED:
	    {
	        DEBUG_LOGI(LOG_TAG, "+JAP:DISCONNECTED,%u,%u", 0, evt->event_info.disconnected.reason);
			if (is_wifi_sta_mode())
			{
				app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
			}
	        break;
	    }
	    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
	    {
	        DEBUG_LOGI(LOG_TAG, "+JAP:AUTHCHANGED,%d,%d",
	                 evt->event_info.auth_change.new_mode,
	                 evt->event_info.auth_change.old_mode);
	        break;
	    }
	    case SYSTEM_EVENT_STA_GOT_IP: 
		{
	        wifi_config_t w_config;
	        memset(&w_config, 0x00, sizeof(wifi_config_t));

	        if (ESP_OK == esp_wifi_get_config(WIFI_IF_STA, &w_config)) 
			{
	            DEBUG_LOGI(LOG_TAG, ">>>>>> Connected Wifi SSID:%s <<<<<<< \r\n", w_config.sta.ssid);
	        } 
			else 
			{
	            DEBUG_LOGI(LOG_TAG, "Got wifi config failed");
	        }

	        DEBUG_LOGI(LOG_TAG, "SYSTEM_EVENT_STA_GOTIP");
			if (is_wifi_sta_mode())
			{
				app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			}
	        break;
	  	}
	    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
	        DEBUG_LOGI(LOG_TAG, "+WPS:SUCCEED");
	        // esp_wifi_wps_disable();
	        //esp_wifi_connect();
	        break;

	    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
	        DEBUG_LOGI(LOG_TAG, "+WPS:FAILED");
	        break;

	    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
	        DEBUG_LOGI(LOG_TAG, "+WPS:TIMEOUT");
	        // esp_wifi_wps_disable();
	        //esp_wifi_disconnect();
	        break;

	    case SYSTEM_EVENT_STA_WPS_ER_PIN: {
	//                char pin[9] = {0};
	//                memcpy(pin, event->event_info.sta_er_pin.pin_code, 8);
	//                ESP_AUDIO_LOGI(APP_TAG, "+WPS:PIN: [%s]", pin);
	            break;
	        }

	    case SYSTEM_EVENT_AP_START:
	        DEBUG_LOGI(LOG_TAG, "+WIFI:AP_START");
	        break;

	    case SYSTEM_EVENT_AP_STOP:
	        DEBUG_LOGI(LOG_TAG, "+WIFI:AP_STOP");
	        break;

	    case SYSTEM_EVENT_AP_STACONNECTED:
	        DEBUG_LOGI(LOG_TAG, "+SOFTAP:STACONNECTED,"MACSTR,
	                 MAC2STR(evt->event_info.sta_connected.mac));
	        break;

	    case SYSTEM_EVENT_AP_STADISCONNECTED:
	        DEBUG_LOGI(LOG_TAG, "+SOFTAP:STADISCONNECTED,"MACSTR,
	                 MAC2STR(evt->event_info.sta_disconnected.mac));
	        break;

	    case SYSTEM_EVENT_AP_PROBEREQRECVED:
	        DEBUG_LOGI(LOG_TAG, "+PROBEREQ:"MACSTR",%d",MAC2STR(evt->event_info.ap_probereqrecved.mac),
	                evt->event_info.ap_probereqrecved.rssi);
	        break;
	    default:
	        break;
	    }
	    return ESP_OK;
}

/**
 * start wifi sta mode
 *
 * @param none
 * @return true/false
 */
static bool start_wifi_sta_mode(void)
{
	wifi_mode_t mode = WIFI_MODE_NULL;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();	

	//关闭smartconfig
	esp_smartconfig_stop();
	
    if (esp_wifi_init(&cfg) != ESP_OK)
    {
		DEBUG_LOGE(LOG_TAG, "esp_wifi_init failed");
		return false;
	}
	else
	{
		if (esp_wifi_set_ps(WIFI_PS_MODEM) != ESP_OK)
		{
			DEBUG_LOGE(LOG_TAG, "esp_wifi_set_ps DEFAULT_PS_MODE failed");
		}
	}

	if (esp_wifi_get_mode(&mode) != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "esp_wifi_get_mode failed");
		return false;
	}
	
	switch (mode)
	{
		case WIFI_MODE_NULL:
		{
			DEBUG_LOGE(LOG_TAG, "wifi mode: WIFI_MODE_NULL");	
			break;
		}
		case WIFI_MODE_STA:
		{
			DEBUG_LOGE(LOG_TAG, "wifi mode: WIFI_MODE_STA");	
			break;
		}
		case WIFI_MODE_APSTA:
		{
			DEBUG_LOGE(LOG_TAG, "wifi mode: WIFI_MODE_APSTA");	
			break;
		}
		default:
			break;
	}
    
    if (esp_event_loop_init(wifi_event_handle_cb, NULL) != ESP_OK)
    {
    	DEBUG_LOGE(LOG_TAG, "esp_event_loop_init failed");
		//return false;
	}
	
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
    {
    	DEBUG_LOGE(LOG_TAG, "esp_wifi_set_mode failed");
		return false;
	}
	
	if (esp_wifi_start() != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "esp_wifi_start failed");
		return false;
	}

	return true;
}

/**
 * smartconfig event handle callback
 *
 * @param [in]status,smartconfig status
 * @param [in]pdata
 * @return none
 */
static void smartconfig_event_handle_cb(
	smartconfig_status_t status, 
	void *pdata)
{
    wifi_config_t sta_conf;

    switch (status) 
	{
	    case SC_STATUS_WAIT:
	    {
	        DEBUG_LOGI(LOG_TAG, "SC_STATUS_WAIT");
	        break;
	    }
	    case SC_STATUS_FIND_CHANNEL:
	    {
	        DEBUG_LOGI(LOG_TAG, "SC_STATUS_FIND_CHANNEL");
	        break;
	    }
	    case SC_STATUS_GETTING_SSID_PSWD:
	    {
	        DEBUG_LOGI(LOG_TAG, "SC_STATUS_GETTING_SSID_PSWD");
	        smartconfig_type_t *type = pdata;
	        if (*type == SC_TYPE_ESPTOUCH) 
			{
	           	DEBUG_LOGD(LOG_TAG, "SC_TYPE:SC_TYPE_ESPTOUCH");
	        } 
			else 
			{
	            DEBUG_LOGD(LOG_TAG, "SC_TYPE:SC_TYPE_AIRKISS");
	        }
	        break;
	    }
	    case SC_STATUS_LINK:
	    {
	        DEBUG_LOGI(LOG_TAG, "SC_STATUS_LINK");
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECTING, AUDIO_TERM_TYPE_NOW);

			memset(&sta_conf, 0, sizeof(sta_conf));
	        memcpy(&(sta_conf.sta), pdata, sizeof(wifi_sta_config_t));
	        DEBUG_LOGI(LOG_TAG, "<link>ssid:%s", sta_conf.sta.ssid);
	        DEBUG_LOGI(LOG_TAG, "<link>pass:%s", sta_conf.sta.password);
			
	        if (esp_wifi_set_config(WIFI_IF_STA, &sta_conf) != ESP_OK) 
			{
	            DEBUG_LOGE(LOG_TAG, "[%s] set_config fail", __func__);
				if (is_wifi_smartconfig_mode())
				{
					set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_FAIL);
				}
				break;
	        }
			
	        if (esp_wifi_connect() != ESP_OK) 
			{
				DEBUG_LOGE(LOG_TAG, "[%s] wifi_connect fail", __func__);
				if (is_wifi_smartconfig_mode())
				{
					set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_FAIL);
				}
				break;
	        }

			if (is_wifi_smartconfig_mode())
			{
				update_current_wifi_info((const char*)sta_conf.sta.ssid, (const char*)sta_conf.sta.password);
				update_wifi_connect_time();
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_WAIT);
				app_send_message(APP_NAME_WIFI_MANAGE, APP_MAIN_BIND_DEVICE, APP_EVENT_BIND_DEVICE_MSG, NULL, 0);
			}
	        break;
	    }
	    case SC_STATUS_LINK_OVER:
	    {
	        DEBUG_LOGI(LOG_TAG, "SC_STATUS_LINK_OVER");
	        if (pdata != NULL) 
			{
	            uint8_t phone_ip[4] = {0};
	            memcpy(phone_ip, (const void *)pdata, 4);
	            DEBUG_LOGI(LOG_TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
	        }
			
			if (is_wifi_smartconfig_mode())
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_SUCCESS);
			}
	        break;
		}
		default:
			break;
	}
}

/**
 * start wifi smartconfig mode
 *
 * @param none
 * @return true/false
 */
static bool start_wifi_smartconfig_mode(void)
{	
	esp_wifi_disconnect();

	esp_smartconfig_stop();

	esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS);

	esp_smartconfig_fast_mode(true);
	
	if (esp_smartconfig_start(smartconfig_event_handle_cb, 1) != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "esp_smartconfig_start failed");
		return false;
	}
	
	return true;
}

/**
 * stop wifi smartconfig mode
 *
 * @param none
 * @return true/false
 */
static bool stop_wifi_smartconfig_mode(void)
{
	esp_smartconfig_stop();

	return true;
}

/**
 * wifi connect
 *
 * @param [in]wifi_info,device wifi infomation
 * @return true/false
 */
static bool wifi_connect(const DEVICE_WIFI_INFO_T *wifi_info)
{
	wifi_config_t w_config = 
	{
		.sta = 
		{
	        .ssid = "",
	        .password = "",
	        .bssid_set = false
        }
    };
	

	if (wifi_info == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "invalid params");
		return false;
	}
	
	snprintf((char*)w_config.sta.ssid, sizeof(w_config.sta.ssid), "%s", wifi_info->wifi_ssid);
	snprintf((char*)w_config.sta.password, sizeof(w_config.sta.password), "%s", wifi_info->wifi_passwd);

	if (esp_wifi_set_config(WIFI_IF_STA, &w_config) != ESP_OK) 
	{
		DEBUG_LOGE(LOG_TAG, "esp_wifi_set_config failed");
		return false;
	} 

	if (esp_wifi_connect() != ESP_OK)
	{
		DEBUG_LOGE(LOG_TAG, "esp_wifi_connect failed");
		return false;
	}

	return true;
}

/**
 * save wifi infomation
 *
 * @param [in]ssid,the name of wifi
 * @param [in]password
 * @return true/false
 */
static bool save_wifi_info(
	const char *ssid, 
	const char *password)
{
	int i = 0;
	int is_find_wifi = 0;
	int ret = WIFI_MANAGE_ERRNO_FAIL;
	DEVICE_WIFI_INFOS_T wifi_infos;
	DEVICE_WIFI_INFO_T *wifi_info = NULL;

	if (ssid == NULL 
		|| password == NULL
		|| strlen(ssid) <= 0)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info invalid params");
		return false;
	}
	
	memset(&wifi_infos, 0, sizeof(wifi_infos));
	ret = get_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (ret != 0)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info get_flash_cfg failed");
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	print_wifi_infos(&wifi_infos);

	for (i=1; i < MAX_WIFI_NUM; i++)
	{
		wifi_info = &wifi_infos.wifi_info[i];
		if (strncmp(ssid, wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid)) == 0
			&& strncmp(password, wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd)) == 0)
		{
			wifi_infos.wifi_connect_index = i;
			is_find_wifi = 1;
			break;
		}
	}

	if (is_find_wifi == 0)
	{
		if (wifi_infos.wifi_storage_index < MAX_WIFI_NUM)
		{
			if (wifi_infos.wifi_storage_index == (MAX_WIFI_NUM - 1))
			{
				wifi_infos.wifi_storage_index = 1;
				wifi_infos.wifi_connect_index = 1;
				wifi_info = &wifi_infos.wifi_info[1];
			}
			else
			{
				wifi_infos.wifi_storage_index++;
				wifi_infos.wifi_connect_index = wifi_infos.wifi_storage_index;
				wifi_info = &wifi_infos.wifi_info[wifi_infos.wifi_storage_index];
			}
		}
		else
		{
			wifi_infos.wifi_storage_index = 1;
			wifi_infos.wifi_connect_index = 1;
			wifi_info = &wifi_infos.wifi_info[1];
		}

		snprintf(&wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid), "%s", ssid);
		snprintf(&wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd), "%s", password);
	}
	
	print_wifi_infos(&wifi_infos);
	
	ret = set_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (ret != 0)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info set_flash_cfg failed(%x)", ret);
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	return WIFI_MANAGE_ERRNO_OK;
}

/**
 * get wifi infomation
 *
 * @param [out]handle,wifi manage handle
 * @return int
 */
static int get_wifi_info(WIFI_MANAGE_HANDLE_t *handle)
{
	int i = 0;
	static int connect_index = -1;
	static int connect_count = 0;
	int ret = WIFI_MANAGE_ERRNO_FAIL;
	DEVICE_WIFI_INFO_T *wifi_info = NULL;

	if (handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "get_wifi_info invalid params");
		return WIFI_MANAGE_ERRNO_FAIL;
	}
	
	memset(&handle->wifi_infos, 0, sizeof(handle->wifi_infos));
	ret = get_flash_cfg(FLASH_CFG_WIFI_INFO, &handle->wifi_infos);
	if (ret != 0)
	{
		DEBUG_LOGE(LOG_TAG, "get_flash_cfg FLASH_CFG_WIFI_INFO failed");
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	print_wifi_infos(&handle->wifi_infos);

	for (i=0; i<MAX_WIFI_NUM; i++)
	{
		if (connect_index == -1)
		{
			connect_index = handle->wifi_infos.wifi_connect_index;
			if (connect_index < 0 || connect_index > MAX_WIFI_NUM)
			{
				connect_index = 0;
			}
		}
		else
		{
			connect_index++;
			if (connect_index >= MAX_WIFI_NUM)
			{
				connect_index = 0;
			}
		}

		connect_count++;
		if (connect_count > MAX_WIFI_NUM)
		{
			connect_count = 0;
			return WIFI_MANAGE_ERRNO_FAIL;
		}
		
		if (connect_index >= 0 && connect_index < MAX_WIFI_NUM)
		{
			wifi_info = &handle->wifi_infos.wifi_info[connect_index];
			if (strlen(wifi_info->wifi_ssid) == 0)
			{
				continue;
			}

			update_current_wifi_info(wifi_info->wifi_ssid, wifi_info->wifi_passwd);
			break;
		}
	}

	if (wifi_info != NULL
		&& strlen(wifi_info->wifi_ssid) == 0)
	{
		return WIFI_MANAGE_ERRNO_FAIL;
	}
	
	return WIFI_MANAGE_ERRNO_OK;
}

/**
 * decode wifi infomation from app
 *
 * @param [out]handle,wifi manage handle
 * @param [in]_content
 * @return none
 */
static void decode_wifi_info_from_app(WIFI_MANAGE_HANDLE_t *handle, char *_content)
{
	int ret = 0;
	#if 0
	cJSON *pJson_body = cJSON_Parse(_content);
    if (NULL != pJson_body) 
	{
        cJSON *pJson_user_id = cJSON_GetObjectItem(pJson_body, "userid");
		if (NULL == pJson_user_id 
			|| pJson_user_id->valuestring == NULL 
			|| strlen(pJson_user_id->valuestring) <= 0)
		{
			goto save_wifi_info;
		}
		DEBUG_LOGI(LOG_TAG, "user_id[%s]", pJson_user_id->valuestring);

		cJSON *pJson_ssid = cJSON_GetObjectItem(pJson_body, "wifi_ssid");
		if (NULL == pJson_ssid 
			|| pJson_ssid->valuestring == NULL 
			|| strlen(pJson_ssid->valuestring) <= 0)
		{
			goto save_wifi_info;
		}
		DEBUG_LOGI(LOG_TAG, "wifi ssid[%s]", pJson_ssid->valuestring);
		snprintf(handle->wifi_info.ssid.ssid, sizeof(handle->wifi_info.ssid.ssid), "%s", pJson_ssid->valuestring);
        handle->wifi_info.ssid.ssid_len = strlen(handle->wifi_info.ssid.ssid);

		cJSON *pJson_passwd = cJSON_GetObjectItem(pJson_body, "wifi_passwd");
		if (NULL == pJson_passwd 
			|| pJson_passwd->valuestring == NULL 
			|| strlen(pJson_passwd->valuestring) <= 0)
		{
			DEBUG_LOGI(LOG_TAG, "wifi passwd[]");
			memset(handle->wifi_info.password, 0, sizeof(handle->wifi_info.password));
		}
		else
		{	
			DEBUG_LOGI(LOG_TAG, "wifi passwd[%s]", pJson_passwd->valuestring);
			snprintf(handle->wifi_info.password, sizeof(handle->wifi_info.password), "%s", pJson_passwd->valuestring);
		}
		
		if (set_flash_cfg(FLASH_CFG_USER_ID, pJson_user_id->valuestring) == 0)
		{
			DEBUG_LOGI(LOG_TAG, "save user id[%s] success", pJson_user_id->valuestring);
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "save user id[%s] fail", pJson_user_id->valuestring);
		}
		
    }
	else
	{
		DEBUG_LOGE(LOG_TAG, "cJSON_Parse error");
	}

save_wifi_info:

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	#endif
	return;
}

/**
 * receive wifi infomation from app
 *
 * @param [out]handle,wifi manage handle
 * @return none
 */
void recv_wifi_info_from_app(WIFI_MANAGE_HANDLE_t *handle)
{  
	static char *DEVICE_BIND_INFO = "{\"device_sn\": \"%s\",\"device_version\": \"%s\",\"appid\": \"%s\",\"robotid\": \"%s\"}";
	struct sockaddr_in server;	//服务器地址信息结构体  
	struct sockaddr_in client;	//客户端地址信息结构体  
	int sin_size;  
	WIFI_AP_SERVER_HANDLE_t *ap_server = &handle->ap_server;

	switch (ap_server->status)
	{
		case WIFI_AP_SERVER_STATUS_IDEL:
		{
			if (ap_server->listen_sock >= 0)
			{
				sock_close(ap_server->listen_sock);
				ap_server->listen_sock = INVALID_SOCK;
			}

			if (ap_server->connect_sock >= 0)
			{
				sock_close(ap_server->connect_sock);
				ap_server->connect_sock = INVALID_SOCK;
			}

			//创建socket
			ap_server->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (ap_server->listen_sock < 0) 
			{ 
				DEBUG_LOGE(LOG_TAG, "ap server create socket failed");
				break; 
			}  
			  
			int opt = SO_REUSEADDR; 		 
			setsockopt(ap_server->listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	  //设置socket属性，端口可以重用  
			sock_set_nonblocking(ap_server->listen_sock);

			//调用bind，绑定地址和端口  
			memset(&server, 0, sizeof(server));	
			server.sin_family=AF_INET;	
			server.sin_port=htons(9099);  
			server.sin_addr.s_addr = htonl(INADDR_ANY);  
			
			if (bind(ap_server->listen_sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
			{
				DEBUG_LOGE(LOG_TAG, "bind failed");
				break;
			}	  
			DEBUG_LOGI(LOG_TAG, "bind success");

			//调用listen，开始监听	 
			if (listen(ap_server->listen_sock, 2) == -1)
			{ 	 
				DEBUG_LOGE(LOG_TAG, "listen failed");
				break;
			}  
			DEBUG_LOGI(LOG_TAG, "listen success");
			ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
			
			break;
		}
		case WIFI_AP_SERVER_STATUS_ACCEPTING:
		{
			//调用accept，返回与服务器连接的客户端描述符	
			if (ap_server->connect_sock >= 0)
			{
				sock_close(ap_server->connect_sock);
				ap_server->connect_sock = INVALID_SOCK;
			}
			
			sin_size = sizeof(struct sockaddr_in); 
			ap_server->connect_sock = accept(ap_server->listen_sock, (struct sockaddr *)&client,(socklen_t *)&sin_size);
			if (ap_server->connect_sock < 0) 
			{ 		
				break;
			}  
			sock_set_nonblocking(ap_server->connect_sock);
			DEBUG_LOGI(LOG_TAG, "accept one client %d", ap_server->connect_sock);
			ap_server->status = WIFI_AP_SERVER_STATUS_COMMUNICATION;
			break;
		}
		case WIFI_AP_SERVER_STATUS_COMMUNICATION:
		{
			int recv_len = 0;
			//接收wifi名称和密码
			memset(ap_server->buffer, 0, sizeof(ap_server->buffer));
			DEBUG_LOGI(LOG_TAG, "recving app wifi ssid and password");
			int ret = sock_readn_with_timeout(ap_server->connect_sock, (char*)&recv_len, sizeof(recv_len), 3000);
			if (ret == sizeof(recv_len))
			{		
				recv_len = ntohl(recv_len);
				if (recv_len > sizeof(ap_server->buffer) - 1)
				{
					DEBUG_LOGE(LOG_TAG, "recv_len:[%d] too long, max size = %d", recv_len, sizeof(ap_server->buffer) - 1);
					ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
					break;
				}
				
				ret = sock_readn_with_timeout(ap_server->connect_sock, ap_server->buffer, recv_len, 2000);
				if (ret == recv_len)
				{
					DEBUG_LOGE(LOG_TAG, "recv app msg:[%d]%s", ret, ap_server->buffer);
					decode_wifi_info_from_app(handle, ap_server->buffer);
					
					char device_id[32]= {0};
					get_flash_cfg(FLASH_CFG_DEVICE_ID, &device_id);
					snprintf(ap_server->buffer, sizeof(ap_server->buffer), DEVICE_BIND_INFO, 
						device_id, ESP32_FW_VERSION, DEEP_BRAIN_APP_ID, DEEP_BRAIN_ROBOT_ID);
					
					DEBUG_LOGE(LOG_TAG, "send app msg:[%d]%s", strlen(ap_server->buffer), ap_server->buffer);

					recv_len = htonl(strlen(ap_server->buffer));
					if (sock_writen_with_timeout(ap_server->connect_sock, (void*)&recv_len, sizeof(recv_len), 1000) != sizeof(recv_len))
					{
						DEBUG_LOGE(LOG_TAG, "sock_write header %d bytes fail", sizeof(recv_len));
						ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
						break;
					}
					
					if (sock_writen_with_timeout(ap_server->connect_sock, ap_server->buffer, strlen(ap_server->buffer), 2000) != strlen(ap_server->buffer))
					{
						DEBUG_LOGE(LOG_TAG, "sock_write body %d bytes fail", strlen(ap_server->buffer));
						ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
						break;
					}
				}
				else
				{
					DEBUG_LOGE(LOG_TAG, "data len =%d, recv len=%d", recv_len, ret);
					ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
				}
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "recv info:ret = %d, is not 4 bytes length", ret);
				ap_server->status = WIFI_AP_SERVER_STATUS_ACCEPTING;
			}
			break;
		}
		default:
			break;
	}
}

/**
 * wifi event process
 *
 * @param [out]handle,wifi manage handle
 * @return none
 */
static void wifi_event_process(WIFI_MANAGE_HANDLE_t *handle)
{
	int err = 0;
	
	if (handle == NULL)
	{
		return;
	}
	
	switch(handle->status)
    {
    	case WIFI_MANAGE_STATUS_IDLE:
    	{
			break;
    	}
		//联网模式
		case WIFI_MANAGE_STATUS_STA_ON:
		{					
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECTING, AUDIO_TERM_TYPE_DONE);
			task_thread_sleep(3000);
			
			if (start_wifi_sta_mode())
			{
				DEBUG_LOGI(LOG_TAG, "start wifi sta mode success");
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTING);
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "start wifi sta mode failed");
				task_thread_sleep(5000);
			}
            break;
        }
        case WIFI_MANAGE_STATUS_STA_CONNECTING:
        {		
			//没有wifi信息，则获取wifi配置信息
			if (get_wifi_info(handle) == WIFI_MANAGE_ERRNO_FAIL)
			{
				//set_wifi_manage_status(WIFI_MANAGE_STATUS_WIFI_CONFIG_ON);
				break;
			}
			
			//连接wifi
            if (wifi_connect(&handle->curr_wifi))
            {	        
            	handle->connecting_start_time = get_time_of_day();
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_WAIT);
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s]", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
            }
			else
			{
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s] failed", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			}
            break;
        }
		case WIFI_MANAGE_STATUS_STA_CONNECT_WAIT:
		{
			if (abs(get_time_of_day() - handle->connecting_start_time) >= 6000)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_FAIL);
			}
			break;
		}
    	case WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS:
        {  
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			save_wifi_info(handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTED);
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_SUCCESS, AUDIO_TERM_TYPE_DONE);
            break;
        }
		case WIFI_MANAGE_STATUS_STA_CONNECT_FAIL:
		{
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNEC_FAILURE, TERMINATION_TYPE_NOW);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTING);
			break;
		}
		case WIFI_MANAGE_STATUS_STA_CONNECTED:
		{
			break;
		}
		case WIFI_MANAGE_STATUS_STA_DISCONNECTED:
		{
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_DISCONNECTED, AUDIO_TERM_TYPE_NOW);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_ON);
			break;
		}
		//smart-config配网
		case WIFI_MANAGE_STATUS_SMARTCONFIG_ON:
		{
			if (start_wifi_smartconfig_mode())
			{
				DEBUG_LOGI(LOG_TAG, "start_wifi_smartconfig_mode success");
				audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONFIG, AUDIO_TERM_TYPE_DONE);
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_WAITING);
				handle->ap_server.status = WIFI_AP_SERVER_STATUS_IDEL;
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_DYY_MATCHING_NETWORK_FAILURE, AUDIO_TERM_TYPE_DONE);
				DEBUG_LOGE(LOG_TAG, "start_wifi_smartconfig_mode failed");
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_OFF);
			}
			break;
		}
		case WIFI_MANAGE_STATUS_SMARTCONFIG_WAITING:
		{
			break;
		}
		case WIFI_MANAGE_STATUS_SMARTCONFIG_OFF:
		{
			break;
		}
		case WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_WAIT:
		{
			if (abs(get_time_of_day() - handle->connecting_start_time) >= 5000)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_FAIL);
			}
			break;
		}
		case WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_SUCCESS:
		{	    
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			stop_wifi_smartconfig_mode();
			save_wifi_info(handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTED);
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_SUCCESS, AUDIO_TERM_TYPE_DONE);
            break;
        }
		case WIFI_MANAGE_STATUS_SMARTCONFIG_CONNECT_FAIL:
		{
			audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_FAILURE, AUDIO_TERM_TYPE_NOW);
			task_thread_sleep(2000);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_ON);
			break;
		}
    	default:
        	break;

    }
}

static void wifi_event_callback(
	APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	int err = 0;
	WIFI_MANAGE_HANDLE_t *handle = g_wifi_manage_handle;
	
	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONFIG:
		{
			if (is_wifi_smartconfig_mode())
			{
				app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_ON);
			}
			else
			{
				app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
				set_wifi_manage_status(WIFI_MANAGE_STATUS_SMARTCONFIG_ON);
			}
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTING, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS);			
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_CONNECT_FAIL:
		{
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_FAIL);
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECT_FAIL, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			if (get_wifi_manage_status() == WIFI_MANAGE_STATUS_STA_CONNECTED)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_DISCONNECTED);
			}
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			wifi_event_process(handle);
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit(app);
			break;
		}
		default:
			break;
	}
}

static void task_wifi_manage(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_WIFI_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_WIFI_MANAGE);
	}
	else
	{
		app_set_loop_timeout(app, 500, wifi_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_AUDIO_PLAYER_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, wifi_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_WIFI_MANAGE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t wifi_manage_create(int task_priority)
{
	g_wifi_manage_handle = (WIFI_MANAGE_HANDLE_t*)memory_malloc(sizeof(WIFI_MANAGE_HANDLE_t));
	if (g_wifi_manage_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_service_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_wifi_manage_handle, 0, sizeof(WIFI_MANAGE_HANDLE_t));
	g_wifi_manage_handle->status = WIFI_MANAGE_STATUS_STA_ON;
	
	SEMPHR_CREATE_LOCK(g_lock_wifi_manage);

	if (xTaskCreatePinnedToCore(task_wifi_manage,
                    "task_wifi_manage",
                    APP_NAME_WIFI_MANAGE_STACK_SIZE,
                    g_wifi_manage_handle,
                    task_priority, 
                    NULL, 
                    xPortGetCoreID()) != pdPASS) 
    {
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_wifi_manage task! Out of memory?");

		memory_free(g_wifi_manage_handle);
		g_wifi_manage_handle = NULL;
		SEMPHR_DELETE_LOCK(g_lock_wifi_manage);
		
		return APP_FRAMEWORK_ERRNO_FAIL;
    }
	DEBUG_LOGI(LOG_TAG, "wifi_manage_create success");

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t wifi_manage_delete(void)
{
	return app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t wifi_manage_start_smartconfig(void)
{
	return app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_WIFI_CONFIG, NULL, 0);
}


#ifndef __FLASH_CONFIG_MANAGE_H__
#define __FLASH_CONFIG_MANAGE_H__

#include <stdio.h>
#include <ctypes_interface.h>
#include <stdlib.h>
#include "partitions_config.h"
#include "dcl_interface.h"
#include "flash_operation_interface.h"
#include "memo_service.h"

#define MAX_WIFI_NUM 			5 
#define DEVICE_PARAMS_VERSION_1 0x20180921

typedef enum DEVICE_PARAMS_ERRNO_t
{
	DEVICE_PARAMS_ERRNO_OK,
	DEVICE_PARAMS_ERRNO_FAIL,
}DEVICE_PARAMS_ERRNO_t;

typedef struct DEVICE_PARAMS_BASE_t
{
	uint32_t crc32;
}DEVICE_PARAMS_BASE_t;

//参数类型
typedef enum
{
	FLASH_CFG_START = 0,
	FLASH_CFG_FLASH_MUSIC_INDEX,
	FLASH_CFG_SDCARD_FOLDER_INDEX,
	FLASH_CFG_WIFI_INFO,
	FLASH_CFG_USER_ID,
	FLASH_CFG_DEVICE_ID,
	FLASH_CFG_MEMO_PARAMS,
	FLASH_CFG_OTA_MODE,//OTA升级模式，采用正式还是测试服务器
	FLASH_CFG_ASR_MODE,
	FLASH_CFG_APP_ID,
	FLASH_CFG_ROBOT_ID,
	FLASH_CFG_END
}FLASH_CONFIG_PARAMS_T;

//OTA升级模式
typedef enum
{
	OTA_UPDATE_MODE_FORMAL = 0x12345678, //正式版本
	OTA_UPDATE_MODE_TEST = 0x87654321,	//测试版本
}OTA_UPDATE_MODE_T;

typedef enum
{
	AI_ACOUNT_BAIDU = 0,
	AI_ACOUNT_SINOVOICE,
	AI_ACOUNT_ALL,
}AI_ACOUNT_T;

//nvs has only 16kb size

//total 64 bytes
typedef struct
{
	char wifi_ssid[64];		//wifi name
	char wifi_passwd[64];	//wifi password
}DEVICE_WIFI_INFO_T;

//total 644 bytes
typedef struct
{
	uint8_t wifi_storage_index;					// 最新wifi信息存储位置
	uint8_t wifi_connect_index;					// 最新连接wifi位置
	uint8_t res[2];
	DEVICE_WIFI_INFO_T wifi_info[MAX_WIFI_NUM];
}DEVICE_WIFI_INFOS_T;

//total 512 bytes
typedef struct
{
	char device_sn[32];		//device serial num
	char bind_user_id[32];	//user id,use to bind user
	char res[448];			//res
}DEVICE_BASIC_INFO_T;

//total size 4*1024 bytes,must 4 bytes align
typedef struct
{
	//crc32校验 4字节
	DEVICE_PARAMS_BASE_t device_params_header;//CRC32校验
	
	//param version 4 bytes
	uint32_t params_version;
	
	//wifi info 644 bytes
	DEVICE_WIFI_INFOS_T wifi_infos;
	
	//device basic info 512 bytes
	DEVICE_BASIC_INFO_T basic_info;

	//other params 4字节
	uint8_t flash_music_index;		// falsh music play index
	uint8_t sd_music_folder_index;	// sd music folder index
	uint8_t res1[2];

	//ota update mode
	uint32_t ota_mode;
	
	//memo array
	MEMO_ARRAY_T memo_array;

	//Remaining
	uint8_t res2[2240];
}DEVICE_PARAMS_CONFIG_T;

typedef struct
{
	char asr_url[64];
	char tts_url[64];
	char app_key[64];
	char dev_key[64];
}SINOVOICE_ACOUNT_T;

typedef struct
{
	char asr_url[64];
	char tts_url[64];
	char app_id[16];
	char app_key[64];
	char secret_key[64];
}BAIDU_ACOUNT_T;

typedef struct
{
	SINOVOICE_ACOUNT_T st_sinovoice_acount;
	BAIDU_ACOUNT_T st_baidu_acount;
}PLATFORM_AI_ACOUNTS_T;

/**
 * @brief  print wifi infos 
 *  
 * @param  [in]_wifi_infos
 * @return none
 */
void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos);

/**
 * @brief  print basic info 
 *  
 * @param  [in]_basic_info
 * @return none
 */
void print_basic_info(DEVICE_BASIC_INFO_T *_basic_info);

/**
 * @brief  print other params 
 *  
 * @param  [in]_other_info
 * @return none
 */
void print_other_params(DEVICE_PARAMS_CONFIG_T *_other_info);

/**
 * @brief  print device params 
 *  
 * @param  none
 * @return none
 */
void print_device_params(void);

/**
 * @brief  save device params
 *  
 * @param  none
 * @return device parameters errno
 */
static DEVICE_PARAMS_ERRNO_t save_device_params(void);

/**
 * @brief  remove pcm queue 
 *  
 * @param  [in]_params
 * @param  [out]_value
 * @return int
 */
int get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);

/**
 * @brief  get dcl auth params 
 *  
 * @param  [in]dcl_auth_params
 * @return true/false
 */
bool get_dcl_auth_params(DCL_AUTH_PARAMS_t *dcl_auth_params);

/**
 * @brief  set flash cfg 
 *  
 * @param  [in]_params
 * @param  [out]_value
 * @return int
 */
int set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);

/**
 * @brief  init device params 
 *  
 * @param  [out]_config
 * @return none
 */
void init_device_params_v1(DEVICE_PARAMS_CONFIG_T *_config);

/**
 * @brief  init default ai sinovoice acount 
 *  
 * @param  [out]_sinovice_acount
 * @return none
 */
void init_default_ai_sinovoice_acount(SINOVOICE_ACOUNT_T *_sinovice_acount);

/**
 * @brief  init default ai baidu acount 
 * @param  [out]_baidu_acount
 * @return none
 */
void init_default_ai_baidu_acount(BAIDU_ACOUNT_T *_baidu_acount);

/**
 * @brief  init default ai acounts
 * @param  [out]_accounts
 * @return none
 */
void init_default_ai_acounts(PLATFORM_AI_ACOUNTS_T *_accounts);

/**
 * @brief  remove pcm queue 
 * @param  none
 * @return device parameters errno
 */
DEVICE_PARAMS_ERRNO_t init_device_params(void);

/**
 * @brief  get ai acount 
 * @param  [in]_type
 * @param  [out]_out
 * @return none
 */
void get_ai_acount(AI_ACOUNT_T _type, void *_out);

/**
 * @brief  set ai acount 
 * @param  [in]_type
 * @param  [out]_out
 * @return none
 */
void set_ai_acount(AI_ACOUNT_T _type, void *_out);

#endif


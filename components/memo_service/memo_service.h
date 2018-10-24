#ifndef MEMO_SERVICE_H
#define MEMO_SERVICE_H

#include "app_config.h"

#define MEMO_EVEVT_MAX 10

typedef enum  
{
	MEMO_IDLE,
	MEMO_INIT,
	MEMO_ADD,
	MEMO_LOOP,
}MEMO_STATUS_T; 

// 68字节
typedef struct 
{	
	uint32_t time;
	char str[64];
}MEMO_EVENT_T;

//684字节
typedef struct 
{	
	uint32_t current;
	MEMO_EVENT_T event[MEMO_EVEVT_MAX];
}MEMO_ARRAY_T;

typedef struct 
{
	char domain[64];
	char port[6];
	char params[128];
	char str_nonce[64];
	char str_timestamp[64];
	char str_private_key[64];
	char str_device_id[32];
	char str_buf[512];
	char str_comm_buf[1024*2]; //校时：http请求参数
}TEMP_PARAM_T;

typedef struct 
{	
	uint32_t initial_time_sec;    //初始时间（秒）
	uint32_t initial_time_usec;   //毫秒
	MEMO_EVENT_T  memo_event; 
	MEMO_ARRAY_T  memo_array;
	MEMO_STATUS_T memo_status;

	//提醒设置
	uint32_t memo_remind_count;//闹铃提醒次数
	uint32_t memo_remind_timestamp;//提醒时间戳
	char memo_content[512];			//闹铃提醒内容
}MEMO_SERVICE_HANDLE_T;

/**
 * create memo service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
extern APP_FRAMEWORK_ERRNO_t memo_service_create(const int task_priority);

/**
 * get memo result
 *
 * @param str_body, string body 
 * @return int
 */
extern int get_memo_result(char *str_body);

/**
 * show memo
 *
 * @param none
 * @return none
 */
extern void show_memo(void);

#endif

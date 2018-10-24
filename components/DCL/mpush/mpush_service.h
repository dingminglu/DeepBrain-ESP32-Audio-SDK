#ifndef _MPUSH_SERVICE_H_
#define _MPUSH_SERVICE_H_
#include "socket.h"
#include "app_config.h"
#include "mpush_message.h"
#include "device_params_manage.h"
#include "mpush_interface.h"

#define MPUSH_SERVER_NUM 10

typedef enum
{
	MPUSH_ERROR_GET_SERVER_OK = 0,
	MPUSH_ERROR_GET_SERVER_FAIL,
	MPUSH_ERROR_GET_SN_OK,
	MPUSH_ERROR_GET_SN_FAIL,
	MPUSH_ERROR_GET_ACOUNT_OK,
	MPUSH_ERROR_GET_ACOUNT_FAIL,
	MPUSH_ERROR_GET_OFFLINE_MSG_OK,
	MPUSH_ERROR_GET_OFFLINE_MSG_FAIL,
	MPUSH_ERROR_SEND_MSG_OK,
	MPUSH_ERROR_SEND_MSG_FAIL,
	MPUSH_ERROR_START_OK,
	MPUSH_ERROR_START_FAIL,
	MPUSH_ERROR_INVALID_PARAMS,
	MPUSH_ERROR_CONNECT_OK,
	MPUSH_ERROR_CONNECT_FAIL,
	MPUSH_ERROR_HANDSHAKE_OK,
	MPUSH_ERROR_HANDSHAKE_FAIL,
	MPUSH_ERROR_BIND_USER_OK,
	MPUSH_ERROR_BIND_USER_FAIL,
	MPUSH_ERROR_HEART_BEAT_OK,
	MPUSH_ERROR_HEART_BEAT_FAIL,
}MPUSH_ERROR_CODE_T;

typedef enum
{
	MPUSH_STAUTS_IDEL = 0,
	MPUSH_STAUTS_INIT,
	MPUSH_STAUTS_GET_ACOUNT_INFO,
	MPUSH_STAUTS_GET_SERVER_LIST,
	MPUSH_STAUTS_GET_OFFLINE_MSG,
	MPUSH_STAUTS_CONNECTING,
	MPUSH_STAUTS_HANDSHAKING,
	MPUSH_STAUTS_HANDSHAK_WAIT_REPLY,
	MPUSH_STAUTS_HANDSHAK_OK,
	MPUSH_STAUTS_HANDSHAK_FAIL,
	MPUSH_STAUTS_BINDING,
	MPUSH_STAUTS_BINDING_WAIT_REPLY,
	MPUSH_STAUTS_BINDING_OK,
	MPUSH_STAUTS_BINDING_FAIL,
	MPUSH_STAUTS_CONNECTED,
	MPUSH_STAUTS_DISCONNECT,
}MPUSH_STATUS_T;

typedef struct
{
	void *_mpush_handler;
}MPUSH_SERVICE_T;

//http comm buffer
typedef struct MPUSH_HTTP_BUFFER_t
{
	char	domain[64];
	char	port[8];
	char	params[128];
	char	str_nonce[64];
	char	str_timestamp[64];
	char	str_private_key[64];
	char	str_device_id[32];
	char	str_send_buf1[2*1024];
	char	str_send_buf2[2*1024];
}MPUSH_HTTP_BUFFER_t;

//mpush msg define
typedef struct MPUSH_MSG_INFO_t
{
	//tail queue entry
	TAILQ_ENTRY(MPUSH_MSG_INFO_t) next;
	MPUSH_CLIENT_MSG_INFO_t msg_info;
}MPUSH_MSG_INFO_t;

//server info
typedef struct
{
	int  isValid;										//0-invalid, 1-valid
	char str_server_addr[64];
	char str_server_port[8];
}MPUSH_SERVER_INFO_T;

//mpush service handle
typedef struct MPUSH_SERVICE_HANDLER_t
{
	MPUSH_STATUS_T status;
	sock_t	server_sock;								// server socket
	char str_server_list_url[128];						// server address
	int server_list_num;								// server list num
	int server_current_index;							// current server index
	MPUSH_SERVER_INFO_T server_list[MPUSH_SERVER_NUM];	//server list array
	char str_public_key[1024];				// public key
	char str_comm_buf[1024*30];				// commucation buffer
	char str_comm_buf1[1024*30];			// 
	char str_recv_buf[1024*20];				// recv buffer
	uint8_t str_push_msg[1024*10];			// recv push msg
	MPUSH_MSG_CONFIG_T msg_cfg;
	PLATFORM_AI_ACOUNTS_T ai_acounts;
	MPUSH_CLIENT_MSG_INFO_t msg_info;
	TAILQ_HEAD(MPUSH_MSG_QUEUE_t, MPUSH_MSG_INFO_t) msg_queue;
	mbedtls_rsa_context *rsa;
}MPUSH_SERVICE_HANDLER_t;

#define MPUSH_DEBUG 1

#if MPUSH_DEBUG == 0
#define MPUSH_GET_SERVER_URL 		"http://192.168.20.91:9034/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://192.168.20.91:9034/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://192.168.20.91:9034/open-api/testFetchOfflineMessage"
#define MPUSH_GET_OPEN_SERVICE_URL 	"http://api.deepbrain.ai:8383/open-api/service"		
#else
#define MPUSH_GET_SERVER_URL 		"http://message.deepbrain.ai:9134/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://message.deepbrain.ai:9134/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://message.deepbrain.ai:9134/open-api/testFetchOfflineMessage"	
#define MPUSH_GET_OPEN_SERVICE_URL 	"http://api.deepbrain.ai:8383/open-api/service"	
#endif

APP_FRAMEWORK_ERRNO_t mpush_service_create(int task_priority);
APP_FRAMEWORK_ERRNO_t mpush_service_delete(void);
APP_FRAMEWORK_ERRNO_t mpush_get_msg_queue(MPUSH_CLIENT_MSG_INFO_t *msg_info);


int mpush_service_send_text(
	const char * _string, 
	const char * _nlp_text,
	const char *_to_user);

int mpush_service_send_file(
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user);

#endif

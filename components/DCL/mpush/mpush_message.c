
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <fcntl.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "mpush_message.h"
#include "auth_crypto.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "userconfig.h"

#define PRINT_TAG "MPUSH MESSAGE"

static void _encode_head(char *_out_buf, size_t *_out_len, MPUSH_MSG_HEADER_T *_head)
{
	memcpy(_out_buf + *_out_len, _head, sizeof(MPUSH_MSG_HEADER_T));
	*_out_len += sizeof(MPUSH_MSG_HEADER_T);
}

static void _encode_byte(char *_out_buf, size_t *_out_len, uint8_t _byte)
{
	memcpy(_out_buf + *_out_len, &_byte, sizeof(_byte));
	*_out_len += sizeof(_byte);
}

static void _encode_int(char *_out_buf, size_t *_out_len, int _value)
{
	_value = htonl(_value);
	memcpy(_out_buf + *_out_len, &_value, sizeof(_value));
	*_out_len += sizeof(_value);
}

static void _encode_long(char *_out_buf, size_t *_out_len, uint64_t _value)
{
	_value = htonl(_value);
	memcpy(_out_buf + *_out_len, &_value, sizeof(_value));
	*_out_len += sizeof(_value);
}


static void _encode_bytes(char *_out_buf, size_t *_out_len, uint8_t *_bytes, size_t _byte_len)
{
	uint16_t len = _byte_len;
	len = htons(len);
	memcpy(_out_buf + *_out_len, &len, sizeof(len));
	*_out_len += sizeof(len);
	memcpy(_out_buf + *_out_len, _bytes, _byte_len);
	*_out_len += _byte_len;
}

static void _encode_string(char *_out_buf, size_t *_out_len, uint8_t *_string)
{
	uint16_t len = strlen((char*)_string);
	len = htons(len);
	memcpy(_out_buf + *_out_len, &len, sizeof(len));
	*_out_len += sizeof(len);
	memcpy(_out_buf + *_out_len, _string, strlen((char*)_string));
	*_out_len += strlen((char*)_string);
}


static void _encode_body()
{
	
}

static void _decode_body()
{
	
}


void _mpush_msg_handshake_encode(MPUSH_MSG_HANDSHAKE_T *_msg_handshake, MPUSH_MSG_CONFIG_T *_config)
{	 
	snprintf((char*)_msg_handshake->str_device_id, sizeof(_msg_handshake->str_device_id), "%s", _config->str_device_id);
	
	snprintf((char*)_msg_handshake->str_os_name, sizeof(_msg_handshake->str_os_name), "%s", _config->str_os_name);
	
	snprintf((char*)_msg_handshake->str_os_version, sizeof(_msg_handshake->str_os_version), "%s", _config->str_os_version);
	
	snprintf((char*)_msg_handshake->str_client_version, sizeof(_msg_handshake->str_client_version), "%s", _config->str_client_version);
	
	crypto_random_byte(_msg_handshake->str_iv);
	
	crypto_random_byte(_msg_handshake->str_client_key);
	
	_msg_handshake->minHeartbeat = MPUSH_MIN_HEART_BEAT;
	_msg_handshake->maxHeartbeat = MPUSH_MAX_HEART_BEAT;
	_msg_handshake->timestamp = time(NULL);
}

void _mpush_msg_bind_user_encode(MPUSH_MSG_BIND_T *_msg_bind_user, MPUSH_MSG_CONFIG_T *_config)
{	 
	snprintf((char*)_msg_bind_user->str_user_id, sizeof(_msg_bind_user->str_user_id), "%s###%s###%s", DEEP_BRAIN_APP_ID, DEEP_BRAIN_ROBOT_ID, _config->str_device_id);
	snprintf((char*)_msg_bind_user->str_alias, sizeof(_msg_bind_user->str_alias), "0");
	snprintf((char*)_msg_bind_user->str_tags, sizeof(_msg_bind_user->str_tags), "0");
}

void _mpush_make_packet(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_HEADER_T *_head,
	void *_body,
	size_t _body_len)
{
	*_out_len = 0;
	memcpy(_out_buf, _head, sizeof(MPUSH_MSG_HEADER_T));
	*_out_len += sizeof(MPUSH_MSG_HEADER_T);
	memcpy(_out_buf + *_out_len, _body, _body_len);
	*_out_len += _body_len;

	return;
}

void mpush_make_bind_user_msg(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_CONFIG_T *_config)
{
	MPUSH_MSG_HEADER_T head = {0};
	MPUSH_MSG_BIND_T msg_bind_user;

	memset(&msg_bind_user, 0, sizeof(msg_bind_user));
	//add head
	head.length = 0;
	head.cmd = MPUSH_MSG_CMD_BIND; 				//命令
	head.cc = 0;								//校验码 暂时没有用到
	head.flags = 0; 							//特性，如是否加密，是否压缩等
	head.sessionId = htonl(1);					//会话id
	head.lrc = 0;								//校验，纵向冗余校验。只校验header

	*_out_len = 0;
	_encode_head(_out_buf, _out_len, &head);
	_mpush_msg_bind_user_encode(&msg_bind_user, _config);
	_encode_string(_out_buf, _out_len, msg_bind_user.str_user_id);
	_encode_string(_out_buf, _out_len, msg_bind_user.str_alias);
	_encode_string(_out_buf, _out_len, msg_bind_user.str_tags);

	MPUSH_MSG_HEADER_T *phead = _out_buf;
	phead->length = htonl(*_out_len - sizeof(head));

	return 0;
}

void mpush_make_heartbeat_msg(
	char *_out_buf,
	size_t *_out_len)
{
	*_out_buf = MPUSH_HEARTBEAT_BYTE;
	*_out_len = 1;
	
	return;
}

void mpush_make_ack_msg(
	char *_out_buf,
	size_t *_out_len,
	uint32_t _session_id)
{
	MPUSH_MSG_HEADER_T head = {0};

	//add head
	head.length = 0;
	head.cmd = MPUSH_MSG_CMD_ACK;				//命令
	head.cc = 0;								//校验码 暂时没有用到
	head.flags = 0; 							//特性，如是否加密，是否压缩等
	head.sessionId = htonl(_session_id);		//会话id
	head.lrc = 0;								//校验，纵向冗余校验。只校验header

	*_out_len = 0;
	_encode_head(_out_buf, _out_len, &head);

	return;
}


void mpush_make_handshake_msg(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_CONFIG_T *_config)
{
	MPUSH_MSG_HEADER_T head = {0};
	MPUSH_MSG_HANDSHAKE_T msg_handshake;

	memset(&msg_handshake, 0, sizeof(msg_handshake));
	//add head
	head.length = htonl(sizeof(msg_handshake));
	head.cmd = MPUSH_MSG_CMD_HANDSHAKE; 		//命令
    head.cc = 0; 								//校验码 暂时没有用到
    head.flags = 0; 							//特性，如是否加密，是否压缩等
    head.sessionId = htonl(1); 					//会话id
    head.lrc = 0; 								//校验，纵向冗余校验。只校验header
	
	_mpush_msg_handshake_encode(&msg_handshake, _config);	
	*_out_len = 0;
	_encode_head(_out_buf, _out_len, &head);
	_encode_string(_out_buf, _out_len, msg_handshake.str_device_id);
	_encode_string(_out_buf, _out_len, msg_handshake.str_os_name);
	_encode_string(_out_buf, _out_len, msg_handshake.str_os_version);
	_encode_string(_out_buf, _out_len, msg_handshake.str_client_version);
	_encode_bytes(_out_buf, _out_len, msg_handshake.str_iv, sizeof(msg_handshake.str_iv));
	_encode_bytes(_out_buf, _out_len, msg_handshake.str_client_key, sizeof(msg_handshake.str_client_key));
	_encode_int(_out_buf, _out_len, msg_handshake.minHeartbeat);
	_encode_int(_out_buf, _out_len, msg_handshake.maxHeartbeat);
	_encode_long(_out_buf, _out_len, msg_handshake.timestamp);

	MPUSH_MSG_HEADER_T *phead = _out_buf;
	phead->length = htonl(*_out_len - sizeof(head));

	return;
}

void mpush_encode_push_msg(
	char *_out_buf,
	size_t *_out_len,
	MPUSH_MSG_CONFIG_T *_config)
{
	MPUSH_MSG_HEADER_T head = {0};
	MPUSH_MSG_HANDSHAKE_T msg_handshake;

	memset(&msg_handshake, 0, sizeof(msg_handshake));
	//add head
	head.length = htonl(sizeof(msg_handshake));
	head.cmd = MPUSH_MSG_CMD_HANDSHAKE; 		//命令
    head.cc = 0; 								//校验码 暂时没有用到
    head.flags = 0; 							//特性，如是否加密，是否压缩等
    head.sessionId = htonl(1); 					//会话id
    head.lrc = 0; 								//校验，纵向冗余校验。只校验header
	
	_mpush_msg_handshake_encode(&msg_handshake, _config);	
	*_out_len = 0;
	_encode_head(_out_buf, _out_len, &head);
	_encode_string(_out_buf, _out_len, msg_handshake.str_device_id);
	_encode_string(_out_buf, _out_len, msg_handshake.str_os_name);
	_encode_string(_out_buf, _out_len, msg_handshake.str_os_version);
	_encode_string(_out_buf, _out_len, msg_handshake.str_client_version);
	_encode_bytes(_out_buf, _out_len, msg_handshake.str_iv, sizeof(msg_handshake.str_iv));
	_encode_bytes(_out_buf, _out_len, msg_handshake.str_client_key, sizeof(msg_handshake.str_client_key));
	_encode_int(_out_buf, _out_len, msg_handshake.minHeartbeat);
	_encode_int(_out_buf, _out_len, msg_handshake.maxHeartbeat);
	_encode_long(_out_buf, _out_len, msg_handshake.timestamp);

	MPUSH_MSG_HEADER_T *phead = _out_buf;
	phead->length = htonl(*_out_len - sizeof(head));

	return;
}	

int _mpush_decode_push_msg(char *_body, size_t _body_len, uint8_t *_out, size_t _out_len)
{
	DEBUG_LOGI(PRINT_TAG, "push msg:%s", _body);

	memset(_out, 0, _out_len);
	MPUSH_CLIENT_MSG_INFO_t *msg = (MPUSH_CLIENT_MSG_INFO_t *)_out;
	cJSON * pJson = cJSON_Parse(_body);
	if (NULL != pJson) 
	{
		cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
		if (NULL == pJson_content)
		{
			DEBUG_LOGE(PRINT_TAG, "content not found");
			goto _mpush_decode_push_msg_error;
		}
		
        cJSON *pJson_msg_type = cJSON_GetObjectItem(pJson_content, "msgType");
		if (NULL == pJson_msg_type || pJson_msg_type->valuestring == NULL)
		{
			DEBUG_LOGE(PRINT_TAG, "msgType not found");
			goto _mpush_decode_push_msg_error;
		}
		
		if (strncasecmp(pJson_msg_type->valuestring, "text", strlen("text")) == 0)
		{
			msg->msg_type = MPUSH_SEND_MSG_TYPE_TEXT;
		}
		else if (strncasecmp(pJson_msg_type->valuestring, "file", strlen("file")) == 0)
		{
			msg->msg_type = MPUSH_SEND_MSG_TYPE_FILE;
		}
		else if (strncasecmp(pJson_msg_type->valuestring, "hylink", strlen("hylink")) == 0)
		{
			msg->msg_type = MPUSH_SEND_MSG_TYPE_LINK;
		}
		else if (strncasecmp(pJson_msg_type->valuestring, "cmd", strlen("cmd")) == 0)
		{
			msg->msg_type = MPUSH_SEND_MSG_TYPE_CMD;
		}
		else
		{
			DEBUG_LOGE(PRINT_TAG, "not support msg type:%s", pJson_msg_type->valuestring);
			goto _mpush_decode_push_msg_error;
		}

		cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
		if (NULL == pJson_data || pJson_data->valuestring == NULL)
		{
			DEBUG_LOGE(PRINT_TAG, "data not found");
			goto _mpush_decode_push_msg_error;
		}

		snprintf(msg->msg_content, sizeof(msg->msg_content), "%s", pJson_data->valuestring);
		
		//size_t len = 0;
		//crypto_base64_decode((uint8_t*)msg->msg_content, sizeof(msg->msg_content), &len, (uint8_t*)pJson_data->valuestring, strlen(pJson_data->valuestring));
    }
	else
	{
		DEBUG_LOGE(PRINT_TAG, "invalid json content[%s]", _body);
	}

_mpush_decode_push_msg_error:

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}

	return 0;

} 

int _mpush_decode_ok_msg(char *_body, size_t _body_len)
{
	uint16_t len = 0;
	uint16_t total_len = 0;
	uint32_t heartbeat = 0;
	uint64_t expiretime = 0;
	uint8_t msg_buf[64] = {0};
	
	//cmd,byte
	uint8_t cmd = _body[0];
	uint8_t code = _body[1];

	//data,string
	total_len = 2;
	memcpy(&len, _body + total_len, sizeof(len));	
	len = ntohs(len);
	total_len += 2;
	memcpy(msg_buf, _body + total_len, len > sizeof(msg_buf) ? sizeof(msg_buf) - 1 : len);
	DEBUG_LOGI(PRINT_TAG,"cmd=%d,code=%d,data len=%u,%s", cmd, code, len, msg_buf);

	return cmd;
}

int _mpush_decode_error_msg(char *_body, size_t _body_len)
{
	uint16_t len = 0;
	uint16_t total_len = 0;
	uint32_t heartbeat = 0;
	uint64_t expiretime = 0;
	
	//cmd,byte
	uint8_t cmd = _body[0];
	uint8_t code = _body[1];

	//reason,string
	total_len = 2;
	memcpy(&len, _body + total_len, sizeof(len));	
	len = ntohs(len);
	total_len += 2;
	//data,string
	DEBUG_LOGI(PRINT_TAG, "cmd=%d,code=%d,data len=%u,%s", cmd, code, len, _body + total_len);

	return 0;
}



int _mpush_decode_handshake_ok_msg(char *_body, size_t _body_len)
{
	uint16_t len = 0;
	uint16_t total_len = 0;
	uint32_t heartbeat = 0;
	uint64_t expiretime = 0;
	
	//server key,byte
	total_len = sizeof(len);
	memcpy(&len, _body, sizeof(len));	
	len = ntohs(len);
	total_len += len;
	DEBUG_LOGI(PRINT_TAG, "server key len=%u", len);
	if (len != 16)
	{
		return -1;
	}
	
	//heartbeat, int
	memcpy(&heartbeat, _body+total_len, sizeof(heartbeat));	
	heartbeat = ntohl(heartbeat);
	DEBUG_LOGI(PRINT_TAG, "heartbeat=%u", heartbeat);
	total_len += sizeof(heartbeat);
	
	//session, string
	memcpy(&len, _body + total_len, sizeof(len));	
	len = ntohs(len);
	total_len += len + 2;
	DEBUG_LOGI(PRINT_TAG, "session len=%u", len);
	
	//expiretime,long
	memcpy(&expiretime, _body + total_len, sizeof(expiretime));	
	expiretime = ntohl(expiretime);
	DEBUG_LOGI(PRINT_TAG, "heartbeat=%llu", expiretime);
	total_len += sizeof(expiretime);

	return 0;
}

//msg解码
int mpush_msg_decode(
	MPUSH_MSG_HEADER_T *_head, char*_body,
	uint8_t *_out, size_t _out_len)
{
	int ret = 0;
	
	if (_head == NULL || _body == NULL)
	{
		return -1;
	}
	
	ret = _head->cmd;
	switch (_head->cmd)
	{
		case MPUSH_MSG_CMD_HEARTBEAT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_HANDSHAKE:
		{
			_mpush_decode_handshake_ok_msg(_body, _head->length);
			break;
		}
	    case MPUSH_MSG_CMD_LOGIN:
		{
			break;
		}
	    case MPUSH_MSG_CMD_LOGOUT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_BIND:
		{
			break;
		}
	    case MPUSH_MSG_CMD_UNBIND:
		{
			break;
		}
	    case MPUSH_MSG_CMD_FAST_CONNECT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_PAUSE:
		{
			break;
		}
	    case MPUSH_MSG_CMD_RESUME:
		{
			break;
		}
	    case MPUSH_MSG_CMD_ERROR:
		{
			_mpush_decode_error_msg(_body, _head->length);
			break;
		}
	    case MPUSH_MSG_CMD_OK:
		{
			ret = _mpush_decode_ok_msg(_body, _head->length);
			break;
		}
	    case MPUSH_MSG_CMD_HTTP_PROXY:
		{
			break;
		}
	    case MPUSH_MSG_CMD_KICK:
		{
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_KICK:
		{
			break;
		}
	    case MPUSH_MSG_CMD_PUSH:
		{
			_mpush_decode_push_msg(_body, _head->length, _out, _out_len);
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_PUSH:
		{
			break;
		}
	    case MPUSH_MSG_CMD_NOTIFICATION:
		{
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_NOTIFICATION:
		{
			break;
		}
	    case MPUSH_MSG_CMD_CHAT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_CHAT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_GROUP:
		{
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_GROUP:
		{
			break;
		}
	    case MPUSH_MSG_CMD_ACK:
		{
			break;
		}
	    case MPUSH_MSG_CMD_UNKNOWN:
		{
			break;
		}
		default:
			ret = MPUSH_MSG_CMD_UNKNOWN;
			break;
	}

	return ret;
}

void convert_msg_head_to_host(MPUSH_MSG_HEADER_T *_head)
{
	if (_head == NULL)
	{
		return;
	}

	_head->length = ntohl(_head->length);
	_head->cc = ntohs(_head->cc);
	_head->sessionId = ntohl(_head->sessionId);
}

void convert_msg_head_to_net(MPUSH_MSG_HEADER_T *_head)
{
	if (_head == NULL)
	{
		return;
	}

	_head->length = htonl(_head->length);
	_head->cc = htons(_head->cc);
	_head->sessionId = htonl(_head->sessionId);
}


void print_msg_head(MPUSH_MSG_HEADER_T *_head)
{
	if (_head == NULL)
	{
		return;
	}
	
	DEBUG_LOGI(PRINT_TAG, ">>>>>>>>>> mpush msg head start >>>>>>>>>>");
	DEBUG_LOGI(PRINT_TAG, "length=%d", _head->length);
	DEBUG_LOGI(PRINT_TAG, "cmd=%d", 	_head->cmd);
	DEBUG_LOGI(PRINT_TAG, "cc=%d", 	_head->cc);
	DEBUG_LOGI(PRINT_TAG, "flags=%02x",_head->flags);
	DEBUG_LOGI(PRINT_TAG, "sessionId=%d",_head->sessionId);
	DEBUG_LOGI(PRINT_TAG, "lrc=%d",	_head->lrc);
	DEBUG_LOGI(PRINT_TAG, "<<<<<<<<<< mpush msg head end <<<<<<<<<<");
}

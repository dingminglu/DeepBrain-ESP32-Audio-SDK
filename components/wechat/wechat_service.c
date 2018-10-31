
#include "wechat_service.h"
#include "userconfig.h"
#include "player_middleware_interface.h"
#include "AudioDef.h"
#include "interf_enc.h"
#include "audio_amrnb_interface.h"
#include "mpush_service.h" 
#include "asr_service.h"
#include "aip_interface.h"
#include "dcl_mpush_push_msg.h"

#define LOG_TAG "wechat service"

static void* g_lock_wechat_service = NULL;

/*微聊运行句柄*/
typedef struct WECHAT_SERVICE_HANDLE_t
{	
	/*发送统计*/
	int total_send_count;		//总发送次数
	int total_send_fail_count;	//发送失败次数
	int total_send_ok_count;	//发送成功次数

	DCL_AUTH_PARAMS_t dcl_auth_params;

	//缓存队列
	TAILQ_HEAD(WECHAT_UPLOAD_CACHE_QUEUE_t, WECHAT_SEND_MSG_t) upload_cache_queue;
	
	//发送队列
	TAILQ_HEAD(WECHAT_UPLOAD_QUEUE_t, WECHAT_SEND_MSG_t) upload_queue;
}WECHAT_SERVICE_HANDLE_t;

//全局变量
static WECHAT_SERVICE_HANDLE_t *g_wechat_handle = NULL;

/*创建上传消息对象*/
/**
 * wechat new upload msg
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_new_upload_msg(
	WECHAT_SEND_MSG_t **msg_obj)
{
	if (msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}

	*msg_obj = (WECHAT_SEND_MSG_t *)memory_malloc(sizeof(WECHAT_SEND_MSG_t));
	if (*msg_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "wechat_new_upload_msg memory_malloc failed");
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	memset(*msg_obj, 0, sizeof(WECHAT_SEND_MSG_t));

	return WECHAT_SERVICE_ERRNO_OK;
}

/*微聊上传消息队列-添加*/
/**
 * wechat upload queue add
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_upload_queue_add(
	WECHAT_SEND_MSG_t *msg_obj)
{
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	TAILQ_INSERT_HEAD(&g_wechat_handle->upload_queue, msg_obj, next);	
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return WECHAT_SERVICE_ERRNO_OK;
}
	
/**
 * wechat upload queue remove
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_upload_queue_remove(
	WECHAT_SEND_MSG_t *msg_obj)
{
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	TAILQ_REMOVE(&g_wechat_handle->upload_queue, msg_obj, next);
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return WECHAT_SERVICE_ERRNO_OK;
}

/**
 * wechat upload queue first
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_upload_queue_first(
	WECHAT_SEND_MSG_t **msg_obj)
{
	WECHAT_SERVICE_ERRNO_t err = WECHAT_SERVICE_ERRNO_OK;
	
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	*msg_obj = TAILQ_FIRST(&g_wechat_handle->upload_queue);
	if (*msg_obj == NULL)
	{
		err = WECHAT_SERVICE_ERRNO_FAIL;
	}
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return err;
}

/**
 * wechat upload queue add
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_cache_queue_add(
	WECHAT_SEND_MSG_t *msg_obj)
{
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	TAILQ_INSERT_HEAD(&g_wechat_handle->upload_cache_queue, msg_obj, next);	
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return WECHAT_SERVICE_ERRNO_OK;
}

/**
 * wechat upload queue remove
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_cache_queue_remove(
	WECHAT_SEND_MSG_t *msg_obj)
{
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	TAILQ_REMOVE(&g_wechat_handle->upload_cache_queue, msg_obj, next);
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return WECHAT_SERVICE_ERRNO_OK;
}

/**
 * wechat cache queue first
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_cache_queue_first(
	WECHAT_SEND_MSG_t **msg_obj)
{
	WECHAT_SERVICE_ERRNO_t err = WECHAT_SERVICE_ERRNO_OK;
	
	if (g_wechat_handle == NULL || msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	*msg_obj = TAILQ_FIRST(&g_wechat_handle->upload_cache_queue);
	if (*msg_obj == NULL)
	{
		err = WECHAT_SERVICE_ERRNO_FAIL;
	}
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	return err;
}

/**
 * wechat cache queue match sn
 *
 * @param [out]msg_obj
 * @param [in]msg_sn
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_cache_queue_match_sn(
	WECHAT_SEND_MSG_t **msg_obj, int msg_sn)
{
	WECHAT_SEND_MSG_t *msg = NULL;
	
	if (g_wechat_handle == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_lock_wechat_service);
	TAILQ_FOREACH(msg, &g_wechat_handle->upload_cache_queue, next)
	{
		if (msg->msg_sn == msg_sn)
		{				
			TAILQ_REMOVE(&g_wechat_handle->upload_cache_queue, msg, next);
			*msg_obj = msg;
			break;
		}
	}
	SEMPHR_TRY_UNLOCK(g_lock_wechat_service);

	if (*msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	else
	{
		return WECHAT_SERVICE_ERRNO_OK;
	}
}

/*语音识别结果回调*/
static void wechat_asr_result_cb(
	ASR_RESULT_t *result)
{
	WECHAT_SEND_MSG_t *msg_obj = NULL;

	if (g_wechat_handle == NULL || result == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "wechat_asr_result_cb invalid parmas");
		return;
	}

	if (wechat_cache_queue_match_sn(&msg_obj, result->record_sn) != WECHAT_SERVICE_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "wechat_cache_queue_match_sn failed, msg_sn[%d], record_sn[%d]", 
			msg_obj->msg_sn, result->record_sn);
		return;
	}
	
	switch (result->error_code)
	{
		case DCL_ERROR_CODE_ASR_SHORT_AUDIO:
		{			
			audio_play_tone_mem(FLASH_MUSIC_WECHAT_SHORT_AUDIO, TERMINATION_TYPE_DONE);
			//释放资源
			memory_free(msg_obj);
			break;
		}
		default:
		{
			snprintf(msg_obj->asr_result_text, sizeof(msg_obj->asr_result_text), "%s", result->str_result);
			wechat_upload_queue_add(msg_obj);
			app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_WECHAT_SERVICE, APP_EVENT_WECHAT_NEW_MSG, NULL, 0);
			break;
		}
	}
	
	return;
}

/**
 * wechat pcm to 8k
 *
 * @param [out]msg_obj
 * @param [in]record_data
 * @param [in]record_len
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_pcm_to_8k(
	WECHAT_SEND_MSG_t *msg_obj,
	uint8_t *record_data,
	uint32_t record_len)
{
	if (msg_obj == NULL || record_data == NULL || record_len == 0)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}

	//降采样,16k 转 8k
	short * src = (short *)record_data;
	short * dst = (short *)(msg_obj->amrnb_encode_buff.record_data_8k + msg_obj->amrnb_encode_buff.record_data_8k_len);
	int 	total_buffer_len = sizeof(msg_obj->amrnb_encode_buff.record_data_8k);
	int     samples = record_len/2;
	int 	idx = 0;
	
	if (msg_obj->amrnb_encode_buff.record_data_8k_len + samples > total_buffer_len)
	{
		DEBUG_LOGE(LOG_TAG, "wechat_pcm_to_8k out of buffer");
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	for (idx = 0; idx < samples; idx += 2) 
	{
		dst[idx/2] = src[idx];
	}
	msg_obj->amrnb_encode_buff.record_data_8k_len += samples;

	return WECHAT_SERVICE_ERRNO_OK;
}

/**
 * wechat amrnb encode
 *
 * @param [out]msg_obj
 * @return wechat service errno
 */
static WECHAT_SERVICE_ERRNO_t wechat_amrnb_encode(
	WECHAT_SEND_MSG_t *msg_obj)
{
	static void *amrnb_encode = NULL;

	if (msg_obj == NULL)
	{
		return WECHAT_SERVICE_ERRNO_FAIL;
	}
	
	//创建amrnb句柄
	if (amrnb_encode == NULL)
	{
		amrnb_encode = Encoder_Interface_init(0);
		if (amrnb_encode == NULL)
		{	
			DEBUG_LOGE(LOG_TAG, "Encoder_Interface_init failed");
			return WECHAT_SERVICE_ERRNO_FAIL;
		}
	}

	//amrnb编码
	const int enc_frame_size = AMRNB_ENCODE_OUT_BUFF_SIZE;
	const int pcm_frame_size = AMRNB_ENCODE_IN_BUFF_SIZE;
	
	//8k pcm 转 amrnb
	int i = 0;
	for (i=0; (i * pcm_frame_size) < msg_obj->amrnb_encode_buff.record_data_8k_len; i++)
	{
		if ((msg_obj->amrnb_data_len + enc_frame_size) > sizeof(msg_obj->amrnb_data))
		{
			DEBUG_LOGE(LOG_TAG, "wechat_amrnb_encode size is out of buffer");
			break;
		}

		int ret = Encoder_Interface_Encode(amrnb_encode, MR122, 
				(short*)(msg_obj->amrnb_encode_buff.record_data_8k + i * pcm_frame_size), 
				(unsigned char*)msg_obj->amrnb_data + msg_obj->amrnb_data_len, 0);
		if (ret > 0)
		{
			msg_obj->amrnb_data_len += ret;
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "Encoder_Interface_Encode failed[%d]", ret);
		}
	}
	
	return WECHAT_SERVICE_ERRNO_OK;
}

/**
 * set wechat record data
 *
 * @param [in]sn
 * @param [in]id
 * @param [out]data
 * @param [in]data_len
 * @param [in]record_ms
 * @param [in]is_max
 * @return none
 */
static void set_wechat_record_data(AIP_RECORD_RESULT_t *rec_result)
{
	ASR_PCM_OBJECT_t *pcm_obj = NULL;
	WECHAT_SEND_MSG_t *msg_obj = NULL;

	if (g_wechat_handle == NULL || rec_result == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "set_wechat_record_data invalid parmas");
		return;
	}

	pcm_obj = (ASR_PCM_OBJECT_t *)memory_malloc(sizeof(ASR_PCM_OBJECT_t));
	if (pcm_obj == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "set_wechat_record_data memory_malloc failed");
		return;
	}
	memset(pcm_obj, 0, sizeof(ASR_PCM_OBJECT_t));
	
	if (rec_result->record_id == 1)
	{
		//创建缓存对象
		if (wechat_cache_queue_first(&msg_obj) != WECHAT_SERVICE_ERRNO_OK)
		{
			if (wechat_new_upload_msg(&msg_obj) != WECHAT_SERVICE_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "wechat_new_upload_msg failed");
			}
		}
		else
		{
			wechat_cache_queue_remove(msg_obj);
		}

		if (msg_obj != NULL)
		{
			memset(msg_obj, 0, sizeof(WECHAT_SEND_MSG_t));
			msg_obj->msg_sn = rec_result->record_sn;
			msg_obj->start_timestamp = get_time_of_day();
			snprintf(msg_obj->amrnb_data, sizeof(msg_obj->amrnb_data), "%s", AMRNB_HEADER);
			msg_obj->amrnb_data_len = strlen(AMRNB_HEADER);
			wechat_cache_queue_add(msg_obj);
		}		
	}
	else if (rec_result->record_id < 0)
	{
		audio_play_tone_mem(FLASH_MUSIC_MSG_SEND, TERMINATION_TYPE_NOW);
	}
	else
	{
		//do nothing
	}

	pcm_obj->record_sn = rec_result->record_sn;
	pcm_obj->record_id = rec_result->record_id;
	pcm_obj->record_len = rec_result->record_len;
	pcm_obj->record_ms = rec_result->record_ms;
	
	pcm_obj->time_stamp = get_time_of_day();
	pcm_obj->asr_call_back = wechat_asr_result_cb;
	pcm_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
	pcm_obj->asr_lang 	= DCL_ASR_LANG_CHINESE;
	pcm_obj->asr_mode 	= DCL_ASR_MODE_ASR;
	
	if (rec_result->record_len > 0)
	{
		memcpy(pcm_obj->record_data, rec_result->record_data, rec_result->record_len);
		
		//amr编码数据并缓存
		if (wechat_cache_queue_first(&msg_obj) == WECHAT_SERVICE_ERRNO_OK)
		{
			wechat_pcm_to_8k(msg_obj, rec_result->record_data, rec_result->record_len);
		}
	}

	if (asr_service_send_request(pcm_obj) != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
		memory_free(pcm_obj);
		pcm_obj = NULL;
	}
}

static void wechat_event_callback(
	APP_OBJECT_t *app, APP_EVENT_MSG_t *msg)
{
	int ret = 0;
	WECHAT_SERVICE_HANDLE_t *handle = g_wechat_handle;
	WECHAT_SEND_MSG_t *msg_obj = NULL; 
	
	switch (msg->event)
	{
		case APP_EVENT_WECHAT_NEW_MSG:
		{
			uint64_t start_time = get_time_of_day();
			
			if (wechat_upload_queue_first(&msg_obj) != WECHAT_SERVICE_ERRNO_OK)
			{
				break;
			}

			if (wechat_amrnb_encode(msg_obj) != WECHAT_SERVICE_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "wechat_amrnb_encode failed");
			}

			//微聊推送
			uint64_t encode_time = get_time_of_day();
			get_dcl_auth_params(&handle->dcl_auth_params);
			ret = dcl_mpush_push_msg(&handle->dcl_auth_params, msg_obj->amrnb_data, msg_obj->amrnb_data_len, msg_obj->asr_result_text, "");
			if (ret == DCL_ERROR_CODE_OK)
			{
				audio_play_tone_mem(FLASH_MUSIC_SEND_SUCCESS, TERMINATION_TYPE_NOW);
				DEBUG_LOGI(LOG_TAG, "mpush_service_send_file success"); 
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_SEND_FAIL, TERMINATION_TYPE_NOW);
				DEBUG_LOGE(LOG_TAG, "mpush_service_send_file failed"); 
			}
			uint64_t send_time = get_time_of_day();
			
			DEBUG_LOGI(LOG_TAG, "[wechat push time]:amr encode len[%d], cost[%lldms], send cost[%lldms]", 
				msg_obj->amrnb_data_len, encode_time - start_time, send_time - encode_time);
			wechat_upload_queue_remove(msg_obj);
			memory_free(msg_obj);
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

static void task_wechat_service(void *pv)
{

	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_WECHAT_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_WECHAT_SERVICE);
	}
	else
	{
		app_add_event(app, APP_EVENT_DEFAULT_BASE, wechat_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, wechat_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, wechat_event_callback);
		app_add_event(app, APP_EVENT_WECHAT_BASE, wechat_event_callback);
		DEBUG_LOGE(LOG_TAG, "%s create success", APP_NAME_WECHAT_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	memory_free(g_wechat_handle);
	g_wechat_handle = NULL;
	
	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t wechat_service_create(int task_priority)
{		
	if (g_wechat_handle != NULL)
	{
		return APP_FRAMEWORK_ERRNO_OK;
	}

	//创建运行句柄
	g_wechat_handle = (WECHAT_SERVICE_HANDLE_t *)memory_malloc(sizeof(WECHAT_SERVICE_HANDLE_t));
	if (g_wechat_handle == NULL)
	{
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_wechat_handle, 0, sizeof(WECHAT_SERVICE_HANDLE_t));
	TAILQ_INIT(&g_wechat_handle->upload_queue);
	TAILQ_INIT(&g_wechat_handle->upload_cache_queue);
	SEMPHR_CREATE_LOCK(g_lock_wechat_service);
	if (g_lock_wechat_service == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_lock_wechat_service create failed");
		memory_free(g_wechat_handle);
		g_wechat_handle = NULL;
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//创建任务线程
    if (!task_thread_create(task_wechat_service,
            "task_wechat_service",
            APP_NAME_WECHAT_SERVICE_STACK_SIZE,
            g_wechat_handle,
            task_priority)) 
	{
		memory_free(g_wechat_handle);
		g_wechat_handle = NULL;
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_wechat_service task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t wechat_service_delete(void)
{
	return app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_WECHAT_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

void wechat_record_start(void)
{
	uint32_t record_sn = 0;
	WECHAT_SEND_MSG_t *msg_obj = NULL;

	//发送和缓存队列中还存在数据，不启动录音
	wechat_upload_queue_first(&msg_obj);
	if (msg_obj != NULL)
	{	
		DEBUG_LOGE(LOG_TAG, "wechat_upload_queue has data, can't start wechat record");
		return;
	}

	wechat_cache_queue_first(&msg_obj);
	if (msg_obj != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "wechat_cache_queue has data, can't start wechat record");
		return;
	}

	//开始录音启动指示灯
	//app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);

	app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_FREE_TALK, APP_EVENT_WECHAT_START, NULL, 0);
	
	//播放提示音
	audio_play_tone_mem(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW);
	task_thread_sleep(800);
	//启动录音
	aip_start_record(&record_sn, WECHAT_MAX_RECORD_MS, set_wechat_record_data);
}

void wechat_record_stop(void)
{
	//恢复待机状态灯效
	//app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
	
	aip_stop_record(set_wechat_record_data);
	app_send_message(APP_NAME_WECHAT_SERVICE, APP_NAME_FREE_TALK, APP_EVENT_WECHAT_STOP, NULL, 0);
}


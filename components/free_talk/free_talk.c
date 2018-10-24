/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "free_talk.h"
#include "aip_interface.h"
#include "asr_service.h"
#include "db_vad.h"
#include "dcl_volume_decode.h"
#include "asr_service.h"
#include "dcl_nlp_decode.h"
#include "player_middleware_interface.h"
#include "play_list.h"
#include "dcl_translate_api.h"
#include "device_params_manage.h"
#include "power_manage.h"

#define LOG_TAG  "free talk"

/* AIP录音数据 */
typedef struct FREE_TALK_PCM_OBJECT_t
{
	TAILQ_ENTRY(FREE_TALK_PCM_OBJECT_t) next;
	
	uint32_t record_sn;
	int32_t record_id;
	uint8_t record_data[RAW_PCM_LEN_MS(100, PCM_SAMPLING_RATE_16K)];
	uint32_t record_len;
	uint32_t record_ms;
}FREE_TALK_PCM_OBJECT_t;

/* free talk handle */
typedef struct FREE_TALK_HANDLER_t
{
	/* free talk config */
	FREE_TALK_CONFIG_t config;

	/* mutex lock */
	void *mutex_lock;

	/* record sn */
	uint32_t record_sn;

	/* need cancel record sn */
	uint32_t need_cancel_record_sn;

	/* record id */
	uint32_t record_id;

	/* free talk completely terminated flag*/
	bool free_talk_terminated;

	/* pcm data queue */
	TAILQ_HEAD(FREE_TALK_PCM_QUEUE_t, FREE_TALK_PCM_OBJECT_t) pcm_queue;

	/* vad process handler */
	void *vad_handler;
	bool vad_enable;
	uint32_t vad_slient_ms;

	/* nlp result */
	NLP_RESULT_T nlp_result;
	uint32_t no_result_count;
	char tts_url[2048];

	/* translate text*/
	char translate_text[1024];
}FREE_TALK_HANDLER_t;

/* falk talk global handler */
static FREE_TALK_HANDLER_t *g_free_talk_handler = NULL;

static uint32_t ft_get_record_sn(void)
{
	if (g_free_talk_handler == NULL)
	{
		return 0;
	}

	return g_free_talk_handler->record_sn;
}

static uint32_t ft_get_record_id(void)
{
	if (g_free_talk_handler == NULL)
	{
		return 0;
	}

	return g_free_talk_handler->record_id;
}

static void ft_asr_result_cb(ASR_RESULT_t *result)
{
	uint32_t record_sn = ft_get_record_sn();
	
	if (record_sn != result->record_sn)
	{
		DEBUG_LOGE(LOG_TAG, "current_record_sn[%d], result_record_sn[%d] is different", 
			record_sn, result->record_sn);
		return;
	}

	if (app_send_message(APP_NAME_FREE_TALK, 
		APP_NAME_FREE_TALK, 
		APP_EVENT_FREE_TALK_RESULT, 
		result, sizeof(ASR_RESULT_t)) != APP_FRAMEWORK_ERRNO_OK)
	{
		//do something
		DEBUG_LOGE(LOG_TAG, "app_send_message ASR_RESULT_t failed");
	}
}

static bool ft_rec_queue_obj_new(FREE_TALK_PCM_OBJECT_t **object)
{
	FREE_TALK_PCM_OBJECT_t *obj = NULL;
	
	obj = (FREE_TALK_PCM_OBJECT_t *)memory_malloc(sizeof(FREE_TALK_PCM_OBJECT_t));
	if (obj == NULL)
	{
		return false;
	}

	memset(obj, 0, sizeof(FREE_TALK_PCM_OBJECT_t));
	*object = obj;
	
	return true;
}

static bool ft_rec_queue_obj_delete(FREE_TALK_PCM_OBJECT_t *object)
{
	if (object == NULL)
	{
		return false;
	}

	memory_free(object);

	return true;
}

static bool ft_rec_queue_add_tail(FREE_TALK_PCM_OBJECT_t *object)
{
	if (g_free_talk_handler == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_free_talk_handler->mutex_lock);
	TAILQ_INSERT_TAIL(&g_free_talk_handler->pcm_queue, object, next);	
	SEMPHR_TRY_UNLOCK(g_free_talk_handler->mutex_lock);

	return true;
}

static bool ft_rec_queue_first(FREE_TALK_PCM_OBJECT_t **object)
{
	bool ret = true;
	
	if (g_free_talk_handler == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_free_talk_handler->mutex_lock);
	*object = TAILQ_FIRST(&g_free_talk_handler->pcm_queue);
	if (*object == NULL)
	{
		ret = false;
	}
	SEMPHR_TRY_UNLOCK(g_free_talk_handler->mutex_lock);

	return ret;
}

static bool ft_rec_queue_remove(FREE_TALK_PCM_OBJECT_t *object)
{
	if (g_free_talk_handler == NULL || object == NULL)
	{
		return false;
	}
	
	//insert into msg queue
	SEMPHR_TRY_LOCK(g_free_talk_handler->mutex_lock);
	TAILQ_REMOVE(&g_free_talk_handler->pcm_queue, object, next);
	SEMPHR_TRY_UNLOCK(g_free_talk_handler->mutex_lock);

	return true;
}

static void free_talk_record_cb(AIP_RECORD_RESULT_t *rec_result)
{
	FREE_TALK_PCM_OBJECT_t *rec_obj = NULL;

	g_free_talk_handler->record_id = rec_result->record_id;
	
	//创建pcm对象
	if (!ft_rec_queue_obj_new(&rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_obj_new failed");
		return;
	}

	//pcm对象赋值
	rec_obj->record_sn = rec_result->record_sn;
	rec_obj->record_id = rec_result->record_id;
	rec_obj->record_ms = rec_result->record_ms;
	rec_obj->record_len = rec_result->record_len;
	if (rec_result->record_len > 0)
	{
		memcpy(rec_obj->record_data, rec_result->record_data, rec_result->record_len);
	}
	
	//pcm对象插入PCM队列
	if (!ft_rec_queue_add_tail(rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_add_tail failed");
		return;
	}
	
	return;
}

static void start_default_continue_talk(void)
{
	FREE_TALK_CONFIG_t free_config = {0};
			
	free_config.mode = FREE_TALK_MODE_CONTINUE;
	free_config.single_talk_max_ms = 10*1000;
	free_config.vad_detect_sensitivity = 3;
	free_talk_start(&free_config);

	return;
}

static void free_talk_manual_single_process(FREE_TALK_HANDLER_t *handler)
{
	ASR_PCM_OBJECT_t *asr_obj = NULL;
	FREE_TALK_PCM_OBJECT_t *rec_obj = NULL;
	
	if (handler == NULL)
	{
		return;
	}

	//取出PCM队列数据
	if (!ft_rec_queue_first(&rec_obj))
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, "record_sn[%d],record_id[%d],record_ms[%d],record_len[%d]",
		rec_obj->record_sn,
		rec_obj->record_id,
		rec_obj->record_ms,
		rec_obj->record_len);
	
	//语音识别请求
	if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
	{
		asr_obj->asr_call_back = ft_asr_result_cb;
		asr_obj->record_sn = rec_obj->record_sn;
		asr_obj->record_id = rec_obj->record_id;
		asr_obj->record_len = rec_obj->record_len;
		asr_obj->record_ms = rec_obj->record_ms;
		asr_obj->time_stamp = get_time_of_day();
		asr_obj->asr_mode = DCL_ASR_MODE_ASR_NLP;
		asr_obj->asr_lang = DCL_ASR_LANG_CHINESE;
		asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
		if (rec_obj->record_len > 0)
		{
			memcpy(asr_obj->record_data, rec_obj->record_data, rec_obj->record_len);
		}

		if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
		{
			asr_service_del_asr_object(asr_obj);
			asr_obj = NULL;
			DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
	}

	//删除队列中数据
	if (!ft_rec_queue_remove(rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_remove failed");
	}
	memory_free(rec_obj);
	rec_obj = NULL;
}

static void translate_ch_to_en_manual_single_process(FREE_TALK_HANDLER_t *handler)
{
	ASR_PCM_OBJECT_t *asr_obj = NULL;
	FREE_TALK_PCM_OBJECT_t *rec_obj = NULL;
	
	if (handler == NULL)
	{
		return;
	}

	//取出PCM队列数据
	if (!ft_rec_queue_first(&rec_obj))
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, "record_sn[%d],record_id[%d],record_ms[%d],record_len[%d]",
		rec_obj->record_sn,
		rec_obj->record_id,
		rec_obj->record_ms,
		rec_obj->record_len);

	//语音识别请求
	if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
	{
		asr_obj->asr_call_back = ft_asr_result_cb;
		asr_obj->record_sn = rec_obj->record_sn;
		asr_obj->record_id = rec_obj->record_id;
		asr_obj->record_len = rec_obj->record_len;
		asr_obj->record_ms = rec_obj->record_ms;
		asr_obj->time_stamp = get_time_of_day();
		asr_obj->asr_mode = DCL_ASR_MODE_ASR;
		asr_obj->asr_lang = DCL_ASR_LANG_PURE_CHINESE;
		asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
		if (rec_obj->record_len > 0)
		{
			memcpy(asr_obj->record_data, rec_obj->record_data, rec_obj->record_len);
		}

		if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
		{
			asr_service_del_asr_object(asr_obj);
			asr_obj = NULL;
			DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
	}

	//删除队列中数据
	if (!ft_rec_queue_remove(rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_remove failed");
	}
	memory_free(rec_obj);
	rec_obj = NULL;
}

static void translate_en_to_ch_manual_single_process(FREE_TALK_HANDLER_t *handler)
{
	ASR_PCM_OBJECT_t *asr_obj = NULL;
	FREE_TALK_PCM_OBJECT_t *rec_obj = NULL;
	
	if (handler == NULL)
	{
		return;
	}

	//取出PCM队列数据
	if (!ft_rec_queue_first(&rec_obj))
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, "record_sn[%d],record_id[%d],record_ms[%d],record_len[%d]",
		rec_obj->record_sn,
		rec_obj->record_id,
		rec_obj->record_ms,
		rec_obj->record_len);

	//语音识别请求
	if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
	{
		asr_obj->asr_call_back = ft_asr_result_cb;
		asr_obj->record_sn = rec_obj->record_sn;
		asr_obj->record_id = rec_obj->record_id;
		asr_obj->record_len = rec_obj->record_len;
		asr_obj->record_ms = rec_obj->record_ms;
		asr_obj->time_stamp = get_time_of_day();
		asr_obj->asr_mode = DCL_ASR_MODE_ASR;
		asr_obj->asr_lang = DCL_ASR_LANG_ENGLISH;
		asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
		if (rec_obj->record_len > 0)
		{
			memcpy(asr_obj->record_data, rec_obj->record_data, rec_obj->record_len);
		}

		if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
		{
			asr_service_del_asr_object(asr_obj);
			asr_obj = NULL;
			DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
	}

	//删除队列中数据
	if (!ft_rec_queue_remove(rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_remove failed");
	}
	memory_free(rec_obj);
	rec_obj = NULL;
}

static void free_talk_auto_single_process(FREE_TALK_HANDLER_t *handler)
{
	int ret = 0;
	ASR_PCM_OBJECT_t *asr_obj = NULL;
	FREE_TALK_PCM_OBJECT_t *rec_obj = NULL;
	static uint32_t vad_sensitive = 0;
	
	if (handler == NULL)
	{
		return;
	}
	
	//初始化vad静音检测库
	if (handler->vad_handler == NULL)
	{
		vad_sensitive = handler->config.vad_detect_sensitivity;
		handler->vad_handler = DB_Vad_Create(vad_sensitive);
		if (handler->vad_handler == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "DB_Vad_Create failed");
			return;
		}
	}

	//检测灵敏度不一样，则重启vad
	if (vad_sensitive != handler->config.vad_detect_sensitivity)
	{
		if (handler->vad_handler != NULL)
		{
			DB_Vad_Free(handler->vad_handler);
			handler->vad_handler = NULL;
		}

		vad_sensitive = handler->config.vad_detect_sensitivity;
		handler->vad_handler = DB_Vad_Create(vad_sensitive);
		if (handler->vad_handler == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "DB_Vad_Create failed");
			return;
		}
	}
	
	//取出PCM队列数据
	if (!ft_rec_queue_first(&rec_obj))
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, "record_sn[%d],record_id[%d],record_ms[%d],record_len[%d]",
		rec_obj->record_sn,
		rec_obj->record_id,
		rec_obj->record_ms,
		rec_obj->record_len);

	//vad检测
	if (rec_obj->record_id == 1)
	{
		handler->vad_slient_ms = 0;
		handler->vad_enable = true;
	}

	if (rec_obj->record_id > 0 
		&& rec_obj->record_len > 0
		&& handler->vad_enable)
	{
		int frame_len = rec_obj->record_len;
		int frame_start = 0;
		while (frame_len >= RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K))
		{
			ret = DB_Vad_Process(handler->vad_handler, 
				PCM_SAMPLING_RATE_16K, 
				RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K)/2, 
				(int16_t*)(rec_obj->record_data + frame_start));
			//DEBUG_LOGI(LOG_TAG, "DB_Vad_Process[%d]", ret);
			if (ret == 1)
			{					
				handler->vad_slient_ms = 0;
			}
			else
			{
				handler->vad_slient_ms += 10;
			}
			frame_len -= RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K);
			frame_start += RAW_PCM_LEN_MS(10, PCM_SAMPLING_RATE_16K);

			if (rec_obj->record_ms >= handler->config.vad_min_detect_ms 
				&& handler->vad_slient_ms >= handler->config.vad_detect_ms)
			{
				DEBUG_LOGI(LOG_TAG, "speech off");
				free_talk_stop();
				handler->vad_enable = false;
				break;
			}
		}
	}
	
	//语音识别请求
	if (asr_service_new_asr_object(&asr_obj) == APP_FRAMEWORK_ERRNO_OK)
	{

		asr_obj->asr_call_back = ft_asr_result_cb;
		asr_obj->record_sn = rec_obj->record_sn;
		asr_obj->record_id = rec_obj->record_id;
		asr_obj->record_len = rec_obj->record_len;
		asr_obj->record_ms = rec_obj->record_ms;
		asr_obj->time_stamp = get_time_of_day();
		asr_obj->asr_mode = DCL_ASR_MODE_ASR_NLP;
		asr_obj->asr_lang = DCL_ASR_LANG_CHINESE;
		asr_obj->asr_engine = ASR_ENGINE_TYPE_DP_ENGINE;
		if (rec_obj->record_len > 0)
		{
			memcpy(asr_obj->record_data, rec_obj->record_data, rec_obj->record_len);
		}

		if (asr_service_send_request(asr_obj) != APP_FRAMEWORK_ERRNO_OK)
		{
			asr_service_del_asr_object(asr_obj);
			asr_obj = NULL;
			DEBUG_LOGE(LOG_TAG, "asr_service_send_request failed");
		}
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "asr_service_new_asr_object failed");
	}

	//删除队列中数据
	if (!ft_rec_queue_remove(rec_obj))
	{
		DEBUG_LOGE(LOG_TAG, "ft_rec_queue_remove failed");
	}
	memory_free(rec_obj);
	rec_obj = NULL;
}

static void free_talk_single_result_process(
	FREE_TALK_HANDLER_t *handler,
	ASR_RESULT_t *asr_result)
{
	int i = 0;
	
	switch (asr_result->error_code)
	{
		case DCL_ERROR_CODE_OK:		//成功
		{
			break;
		}
		case DCL_ERROR_CODE_FAIL:	//失败
		case DCL_ERROR_CODE_SYS_INVALID_PARAMS:	//无效参数
		case DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM:	//内存不足
		{
			//系统错误
			audio_play_tone_mem(FLASH_MUSIC_XI_TONG_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//network error code
		case DCL_ERROR_CODE_NETWORK_DNS_FAIL:	//网络DNS解析失败
		{
			
			audio_play_tone_mem(FLASH_MUSIC_DNS_JIE_XI_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_UNAVAILABLE://网络不可用
		{	
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_KE_YONG, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_POOR:		//网络信号差
		{
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_JIA, AUDIO_TERM_TYPE_NOW);
			return;
		}

		//server error
		case DCL_ERROR_CODE_SERVER_ERROR:		//服务器端错误
		{
			audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//asr error code
		case DCL_ERROR_CODE_ASR_SHORT_AUDIO:	//语音太短,小于1秒
		{
			audio_play_tone_mem(FLASH_MUSIC_CHAT_SHORT_AUDIO, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_ASR_ENCODE_FAIL:	//语音识别PCM数据编码失败
		case DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL:	//语音识别组包失败	
		{
			//语音识别错误
			audio_play_tone_mem(FLASH_MUSIC_BIAN_MA_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		default:
			break;
	}

	NLP_RESULT_T *nlp_result = &handler->nlp_result;
	memset(nlp_result, 0, sizeof(NLP_RESULT_T));
	if (dcl_nlp_result_decode(asr_result->str_result, nlp_result) != NLP_DECODE_ERRNO_OK)
	{
		//to do something
		audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
		DEBUG_LOGE(LOG_TAG, "dcl_nlp_result_decode failed");
		return;
	}

	switch (nlp_result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		case NLP_RESULT_TYPE_ERROR:
		{
			audio_play_tone_mem(FLASH_MUSIC_TONE_AI_NO_SPEAK_02, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case NLP_RESULT_TYPE_NO_ASR:
		{
			audio_play_tone_mem(FLASH_MUSIC_TONE_AI_NO_SPEAK_01, AUDIO_TERM_TYPE_NOW);
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			if (strlen(nlp_result->chat_result.link) > 0)
			{
				audio_play_tone(nlp_result->chat_result.link, AUDIO_TERM_TYPE_NOW);
			}
			else
			{
				bool ret = get_tts_play_url(nlp_result->chat_result.text, (char*)handler->tts_url, sizeof(handler->tts_url));
				if (!ret)
				{
					ret = get_tts_play_url(nlp_result->chat_result.text, (char*)handler->tts_url, sizeof(handler->tts_url));
				}
				
				if (ret)
				{
					audio_play_tone((char*)handler->tts_url, AUDIO_TERM_TYPE_NOW);
				}
				else
				{
					audio_play_tone_mem(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT, AUDIO_TERM_TYPE_NOW);
				}
			}
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			NLP_RESULT_LINKS_T *links = &nlp_result->link_result;
			DEBUG_LOGI(LOG_TAG, "p_links->link_size=[%d]", links->link_size);
			if (links->link_size > 0)
			{
				void *playlist = NULL;

				if (new_playlist(&playlist) != APP_FRAMEWORK_ERRNO_OK)
				{
					DEBUG_LOGE(LOG_TAG, "new_playlist failed");
					audio_play_tone_mem(FLASH_MUSIC_NO_MUSIC, AUDIO_TERM_TYPE_NOW);
					break;
				}				

				for (i=0; i<links->link_size; i++)
				{
					bool ret = get_tts_play_url(links->link[i].link_name, handler->tts_url, sizeof(handler->tts_url));
					if (!ret)
					{
						bool ret = get_tts_play_url(links->link[i].link_name, handler->tts_url, sizeof(handler->tts_url));
					}

					if (ret)
					{
						add_playlist_item(playlist, handler->tts_url, NULL);
					}
					add_playlist_item(playlist, links->link[i].link_url, NULL);
				}

				set_playlist_mode(playlist, PLAY_LIST_MODE_ALL_LOOP);
				playlist_add(&playlist);
			}
			else
			{
				audio_play_tone_mem(FLASH_MUSIC_NO_MUSIC, AUDIO_TERM_TYPE_NOW);
			}
			break;
		}
		case NLP_RESULT_TYPE_VOL_CMD:
		{
			VOLUME_CONTROL_TYPE_t vol_ret = dcl_nlp_volume_cmd_decode(asr_result->str_result);//音量调节
			switch (vol_ret)
			{
				case VOLUME_CONTROL_INITIAL://error state
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_HIGHER:
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_UP, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_LOWER:
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_DOWN, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MODERATE:
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_CHANGE_OK, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MAX:
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST, AUDIO_TERM_TYPE_NOW);
					break;
				}
				case VOLUME_CONTROL_MIN:
				{
					audio_play_tone_mem(FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST, AUDIO_TERM_TYPE_NOW);
					break;
				}
				default:
					break;
			}
			break;
		}
		case NLP_RESULT_TYPE_CMD:
		{
			break;
		}
		default:
			break;
	}

	return;
}

void continue_music_event_callback(const AUDIO_PLAY_EVENTS_T *event)
{
	switch (event->play_status)
	{
		case AUDIO_PLAY_STATUS_UNKNOWN:
		{
			break;
		}
		case AUDIO_PLAY_STATUS_PLAYING:
		{
			//播放TTS灯效
			//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
			//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			
			break;
		}
		case AUDIO_PLAY_STATUS_PAUSED:
		case AUDIO_PLAY_STATUS_FINISHED:
		case AUDIO_PLAY_STATUS_STOP:
		case AUDIO_PLAY_STATUS_ERROR:
		{
		 	app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_FREE_TALK_START_SILENT, NULL, 0);
			
			//恢复待机时灯效
			//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
			//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
			
			break;
		}
		case AUDIO_PLAY_STATUS_AUX_IN:
		{
			break;
		}
		default:
			break;
	}
}


static void free_talk_continue_result_process(
	FREE_TALK_HANDLER_t *handler,
	ASR_RESULT_t *asr_result)
{
	int i = 0;
	
	switch (asr_result->error_code)
	{
		case DCL_ERROR_CODE_OK: 	//成功
		{
			break;
		}
		case DCL_ERROR_CODE_FAIL:	//失败
		case DCL_ERROR_CODE_SYS_INVALID_PARAMS: //无效参数
		case DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM: //内存不足
		{
			//系统错误
			audio_play_tone_mem(FLASH_MUSIC_XI_TONG_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//network error code
		case DCL_ERROR_CODE_NETWORK_DNS_FAIL:	//网络DNS解析失败
		{
			
			audio_play_tone_mem(FLASH_MUSIC_DNS_JIE_XI_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_UNAVAILABLE://网络不可用
		{	
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_KE_YONG, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_POOR:		//网络信号差
		{
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_JIA, AUDIO_TERM_TYPE_NOW);
			return;
		}

		//server error
		case DCL_ERROR_CODE_SERVER_ERROR:		//服务器端错误
		{
			audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//asr error code
		case DCL_ERROR_CODE_ASR_SHORT_AUDIO:	//语音太短,小于1秒
		{
			app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_FREE_TALK_START_SILENT, NULL, 0);
			//audio_play_tone_mem(FLASH_MUSIC_CHAT_SHORT_AUDIO, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_ASR_ENCODE_FAIL:	//语音识别PCM数据编码失败
		case DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL: //语音识别组包失败	
		{
			//语音识别错误
			audio_play_tone_mem(FLASH_MUSIC_BIAN_MA_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		default:
			break;
	}

	NLP_RESULT_T *nlp_result = &handler->nlp_result;
	memset(nlp_result, 0, sizeof(NLP_RESULT_T));
	if (dcl_nlp_result_decode(asr_result->str_result, nlp_result) != NLP_DECODE_ERRNO_OK)
	{
		//to do something
		audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
		DEBUG_LOGE(LOG_TAG, "dcl_nlp_result_decode failed");
		return;
	}

	if (nlp_result->type != NLP_RESULT_TYPE_NO_ASR)
	{
		handler->no_result_count = 0;
	}

	if (strstr(nlp_result->input_text, "闭嘴")
		|| strstr(nlp_result->input_text, "再见")
		|| strstr(nlp_result->input_text, "拜拜")
		|| strstr(nlp_result->input_text, "回聊"))
	{
		audio_play_tone_mem(FLASH_MUSIC_TONE_HAO_DE, AUDIO_TERM_TYPE_NOW);
		return;
	}

	if (strstr(nlp_result->input_text, "关机"))
	{
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_POWER_MANAGE, APP_EVENT_POWER_SHUTDOWN_INSTRUCTION, NULL, 0);
		return;
	}

	if (strstr(nlp_result->input_text, "开灯"))
	{
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_FREE_TALK_START_SILENT, NULL, 0);
		return;
	}
	
	if (strstr(nlp_result->input_text, "关灯"))
	{
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_OFF, NULL, 0);
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_OFF, NULL, 0);
		app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_FREE_TALK_START_SILENT, NULL, 0);
		return;
	}

	switch (nlp_result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		case NLP_RESULT_TYPE_ERROR:
		{
			audio_play_tone_mem_with_cb(FLASH_MUSIC_TONE_AI_NO_SPEAK_02, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
			break;
		}
		case NLP_RESULT_TYPE_NO_ASR:
		{
			handler->no_result_count++;
			if (handler->no_result_count <= 10)
			{
		 		app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_FREE_TALK_START_SILENT, NULL, 0);
			}
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			if (strlen(nlp_result->chat_result.link) > 0)
			{
				audio_play_tone_with_cb(nlp_result->chat_result.link, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
			}
			else
			{
				bool ret = get_tts_play_url(nlp_result->chat_result.text, (char*)handler->tts_url, sizeof(handler->tts_url));
				if (!ret)
				{
					ret = get_tts_play_url(nlp_result->chat_result.text, (char*)handler->tts_url, sizeof(handler->tts_url));
				}
				
				if (ret)
				{
					audio_play_tone_with_cb((char*)handler->tts_url, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
				}
				else
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
				}
			}
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			NLP_RESULT_LINKS_T *links = &nlp_result->link_result;
			DEBUG_LOGI(LOG_TAG, "p_links->link_size=[%d]", links->link_size);
			if (links->link_size > 0)
			{
				void *playlist = NULL;

				if (new_playlist(&playlist) != APP_FRAMEWORK_ERRNO_OK)
				{
					DEBUG_LOGE(LOG_TAG, "new_playlist failed");
					audio_play_tone_mem_with_cb(FLASH_MUSIC_NO_MUSIC, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}				

				for (i=0; i < links->link_size; i++)
				{
					bool ret = get_tts_play_url(links->link[i].link_name, handler->tts_url, sizeof(handler->tts_url));
					if (!ret)
					{
						ret = get_tts_play_url(links->link[i].link_name, handler->tts_url, sizeof(handler->tts_url));
					}

					if (ret)
					{
						add_playlist_item(playlist, handler->tts_url, NULL);
					}
					add_playlist_item(playlist, links->link[i].link_url, NULL);
				}

				//URL灯效
				//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
				//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
				
				set_playlist_mode(playlist, PLAY_LIST_MODE_ALL);
				playlist_add(&playlist);
			}
			else
			{
				audio_play_tone_mem_with_cb(FLASH_MUSIC_NO_MUSIC, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
			}
			break;
		}
		case NLP_RESULT_TYPE_VOL_CMD:
		{
			VOLUME_CONTROL_TYPE_t vol_ret = dcl_nlp_volume_cmd_decode(asr_result->str_result);//音量调节
			switch (vol_ret)
			{
				case VOLUME_CONTROL_INITIAL://error state
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				case VOLUME_CONTROL_HIGHER:
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_VOLUME_UP, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				case VOLUME_CONTROL_LOWER:
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_VOLUME_DOWN, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				case VOLUME_CONTROL_MODERATE:
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_VOLUME_CHANGE_OK, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				case VOLUME_CONTROL_MAX:
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_VOLUME_ALREADY_HIGHEST, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				case VOLUME_CONTROL_MIN:
				{
					audio_play_tone_mem_with_cb(FLASH_MUSIC_DYY_VOLUME_ALREADY_LOWEST, AUDIO_TERM_TYPE_NOW, continue_music_event_callback);
					break;
				}
				default:
					break;
			}
			break;
		}
		case NLP_RESULT_TYPE_CMD:
		{
			break;
		}
		default:
			break;
	}

	return;
}


static void translate_single_result_process(
	DCL_TRANSLATE_MODE_t translate_mode,
	FREE_TALK_HANDLER_t *handler,
	ASR_RESULT_t *asr_result)
{
	int i = 0;	
	DCL_AUTH_PARAMS_t input_params = {0};
	
	switch (asr_result->error_code)
	{
		case DCL_ERROR_CODE_OK: 	//成功
		{
			break;
		}
		case DCL_ERROR_CODE_FAIL:	//失败
		case DCL_ERROR_CODE_SYS_INVALID_PARAMS: //无效参数
		case DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM: //内存不足
		{
			//系统错误
			audio_play_tone_mem(FLASH_MUSIC_XI_TONG_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//network error code
		case DCL_ERROR_CODE_NETWORK_DNS_FAIL:	//网络DNS解析失败
		{
			
			audio_play_tone_mem(FLASH_MUSIC_DNS_JIE_XI_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_UNAVAILABLE://网络不可用
		{	
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_KE_YONG, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_NETWORK_POOR:		//网络信号差
		{
			audio_play_tone_mem(FLASH_MUSIC_WANG_LUO_BU_JIA, AUDIO_TERM_TYPE_NOW);
			return;
		}

		//server error
		case DCL_ERROR_CODE_SERVER_ERROR:		//服务器端错误
		{
			audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			return;
		}
		//asr error code
		case DCL_ERROR_CODE_ASR_SHORT_AUDIO:	//语音太短,小于1秒
		{
			audio_play_tone_mem(FLASH_MUSIC_CHAT_SHORT_AUDIO, AUDIO_TERM_TYPE_NOW);
			return;
		}
		case DCL_ERROR_CODE_ASR_ENCODE_FAIL:	//语音识别PCM数据编码失败
		case DCL_ERROR_CODE_ASR_MAKE_PACK_FAIL: //语音识别组包失败	
		{
			//语音识别错误
			audio_play_tone_mem(FLASH_MUSIC_BIAN_MA_SHI_BAI, AUDIO_TERM_TYPE_NOW);
			return;
		}
		default:
			break;
	}

	if ((strlen(asr_result->str_result) <= 0))
	{
		audio_play_tone_mem(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT, AUDIO_TERM_TYPE_NOW);
		DEBUG_LOGE(LOG_TAG, "asr str_result NULL !");
		return;
	}
		
	snprintf(input_params.str_app_id, sizeof(input_params.str_app_id), "%s", DEEP_BRAIN_APP_ID);
	snprintf(input_params.str_robot_id, sizeof(input_params.str_robot_id), "%s", DEEP_BRAIN_ROBOT_ID);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, &input_params.str_device_id);
	
	if (dcl_get_translate_text(translate_mode, &input_params, asr_result->str_result, &handler->translate_text, 1024) != DCL_ERROR_CODE_OK)
	{
		if (dcl_get_translate_text(translate_mode, &input_params, asr_result->str_result, &handler->translate_text, 1024) != DCL_ERROR_CODE_OK)
		{
			audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			DEBUG_LOGE(LOG_TAG, "dcl_get_translate_text failed");
			return;
		}
	}

	if (dcl_get_tts_url(&input_params, &handler->translate_text, (char*)handler->tts_url, sizeof(handler->tts_url)) != DCL_ERROR_CODE_OK)
	{
		if (dcl_get_tts_url(&input_params, &handler->translate_text, (char*)handler->tts_url, sizeof(handler->tts_url)) != DCL_ERROR_CODE_OK)
		{
			audio_play_tone_mem(FLASH_MUSIC_FU_WU_QI_CUO_WU, AUDIO_TERM_TYPE_NOW);
			DEBUG_LOGE(LOG_TAG, "dcl_get_tts_url failed");
			return;
		}
	}

	DEBUG_LOGI(LOG_TAG, "out_tts_url:[%s]", handler->tts_url);
	if (audio_play_tone((char*)handler->tts_url, AUDIO_TERM_TYPE_NOW) != AUDIO_PLAY_ERROR_CODE_OK)
	{
		audio_play_tone_mem(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT, AUDIO_TERM_TYPE_NOW);
	}

	return;
}

static void free_talk_result_process(
	FREE_TALK_HANDLER_t *handler,
	ASR_RESULT_t *asr_result)
{
	DEBUG_LOGI(LOG_TAG, "error_code[%d], record_sn[%d], str_result_size[%d]", 
			asr_result->error_code, 
			asr_result->record_sn, 
			strlen(asr_result->str_result));

	//DEBUG_LOGI(LOG_TAG, "str_result[%s]", asr_result->str_result);
	
	switch (handler->config.mode)
	{
		case FREE_TALK_MODE_MANUAL_SINGLE:
		case FREE_TALK_MODE_AUTO_SINGLE:
		{
			free_talk_single_result_process(handler, asr_result);
			break;
		}
		case FREE_TALK_MODE_CONTINUE:
		{
			free_talk_continue_result_process(handler, asr_result);
			break;
		}
		case FREE_TALK_MODE_CH2EN_MANUAL_SINGLE:
		{
			translate_single_result_process(DCL_TRANSLATE_MODE_CH_TO_EN, handler, asr_result);
			break;
		}
		case FREE_TALK_MODE_EN2CH_MANUAL_SINGLE:
		{
			translate_single_result_process(DCL_TRANSLATE_MODE_EN_TO_CH, handler, asr_result);
			break;
		}
		default:
			break;
	}
}

static void free_talk_process(FREE_TALK_HANDLER_t *handler)
{
	if (handler == NULL)
	{
		return;
	}

	switch (handler->config.mode)
	{
		case FREE_TALK_MODE_MANUAL_SINGLE:
		{
			free_talk_manual_single_process(handler);
			break;
		}
		case FREE_TALK_MODE_AUTO_SINGLE:
		case FREE_TALK_MODE_CONTINUE:
		{
			free_talk_auto_single_process(handler);
			break;
		}
		case FREE_TALK_MODE_CH2EN_MANUAL_SINGLE:
		{
			translate_ch_to_en_manual_single_process(handler);
			break;
		}
		case FREE_TALK_MODE_EN2CH_MANUAL_SINGLE:
		{
			translate_en_to_ch_manual_single_process(handler);
			break;
		}
		default:
			break;
	}
}

void wakeup_music_event_callback(const AUDIO_PLAY_EVENTS_T *event)
{
	switch (event->play_status)
	{
		case AUDIO_PLAY_STATUS_UNKNOWN:
		{
			break;
		}
		case AUDIO_PLAY_STATUS_PLAYING:
		{
			break;
		}
		case AUDIO_PLAY_STATUS_PAUSED:
		case AUDIO_PLAY_STATUS_FINISHED:
		case AUDIO_PLAY_STATUS_STOP:
		case AUDIO_PLAY_STATUS_ERROR:
		{
			start_default_continue_talk();
			break;
		}
		case AUDIO_PLAY_STATUS_AUX_IN:
		{
			break;
		}
		default:
			break;
	}
}

static void wakeup_event_process(FREE_TALK_HANDLER_t *handler)
{
	static uint32_t last_index = 0;
	
	if (handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "handler is NULL");
		return;
	}

	//随机数计算
	srand(get_time_of_day());	
	uint32_t index = rand()%5;
	
	if (last_index == index)
	{
		last_index = index + 1;
		if (last_index > 5)
		{
			last_index = 0;
		}
	}
	else
	{
		last_index = index;
	}
	
	audio_play_tone_mem_with_cb(FLASH_MUSIC_WAKEUP_NI_HAO_ZHU_REN + last_index, AUDIO_TERM_TYPE_NOW, wakeup_music_event_callback);

	return;
}

static void free_talk_event_callback(
	APP_OBJECT_t *app, 
	APP_EVENT_MSG_t *msg)
{
	FREE_TALK_HANDLER_t *handler = g_free_talk_handler;
	static bool ota_status = true;
	static bool wifi_status = false;
	static bool wechat_status = false;
	
	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{	
			if(ota_status == false || wifi_status == false)
			{
				break;
			}
			
			if (g_free_talk_handler->need_cancel_record_sn != ft_get_record_sn())
			{
				free_talk_process(handler);
			}
			break;
		}
		case APP_EVENT_OTA_START:
		{
			ota_status = false;
			break;
		}
		case APP_EVENT_OTA_STOP:
		{
			ota_status = true;
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			wifi_status = false;
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{
			wifi_status = true;
			break;
		}
		case APP_EVENT_FREE_TALK_RESULT:
		{
			ASR_RESULT_t *asr_result = NULL;
			
			if (msg->event_data_len != sizeof(ASR_RESULT_t))
			{
				DEBUG_LOGE(LOG_TAG, "msg->event_data_len[%d] != sizeof(ASR_RESULT_t)[%d]", 
					msg->event_data_len, sizeof(ASR_RESULT_t));
				break;
			}

			if (g_free_talk_handler->need_cancel_record_sn != ft_get_record_sn())
			{
				asr_result = (ASR_RESULT_t *)msg->event_data;
				free_talk_result_process(handler, asr_result);
			}
			break;
		}
		case APP_EVENT_WECHAT_START:
		{
			wechat_status = true;
			break;
		}
		case APP_EVENT_WECHAT_STOP:
		{
			wechat_status = false;
			break;
		}
		case APP_EVENT_KEYWORD_WAKEUP_NOTIFY:
		{
			free_talk_terminated();
			audio_play_pause();
			update_power_mng_sleep_time();
			app_send_message(APP_NAME_KEYBOARD_SERVICE, APP_NAME_MEMO_MANAGE, APP_EVENT_MEMO_CANCLE, NULL, 0);
			
			if(ota_status == false || wechat_status == true)
			{
				break;
			}
			
			if(wifi_status == false)
			{
				audio_play_tone_mem(FLASH_MUSIC_TONE_NO_NET, TERMINATION_TYPE_NOW);
				break;
			}
			
			handler->no_result_count = 0;
			wakeup_event_process(handler);
			break;
		}
		case APP_EVENT_FREE_TALK_START_SILENT:
		{
			if (g_free_talk_handler->need_cancel_record_sn != ft_get_record_sn())
			{
				update_power_mng_sleep_time();
				start_default_continue_talk();
			}
			break;
		}
		case APP_EVENT_FREE_TALK_START_PROMPT:
		{
			if (g_free_talk_handler->need_cancel_record_sn != ft_get_record_sn())
			{
				audio_play_pause();
				audio_play_tone_mem_with_cb(FLASH_MUSIC_KEY_PRESS, TERMINATION_TYPE_NOW, wakeup_music_event_callback);
			}
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

static void free_talk_destroy(void)
{
	if (g_free_talk_handler == NULL)
	{
		return;
	}

	if (g_free_talk_handler->mutex_lock != NULL)
	{
		SEMPHR_DELETE_LOCK(g_free_talk_handler->mutex_lock);
		g_free_talk_handler->mutex_lock = NULL;
	}

	memory_free(g_free_talk_handler);
	g_free_talk_handler = NULL;
}

static void task_free_talk(const void *pv)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_FREE_TALK);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_FREE_TALK);
		free_talk_destroy();
		task_thread_exit();
		return;
	}
	else
	{
		app_set_loop_timeout(app, 50, free_talk_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_OTA_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, free_talk_event_callback);	
		app_add_event(app, APP_EVENT_FREE_TALK_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_KEYWORD_WAKEUP_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_WECHAT_BASE, free_talk_event_callback);
		app_add_event(app, APP_EVENT_LED_CTRL_BASE, free_talk_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_FREE_TALK);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	free_talk_destroy();

	task_thread_exit();
}

APP_FRAMEWORK_ERRNO_t free_talk_create(int task_priority)
{
	if (g_free_talk_handler != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_free_talk_handler already exist");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//申请句柄
	g_free_talk_handler = (FREE_TALK_HANDLER_t *)memory_malloc(sizeof(FREE_TALK_HANDLER_t));
	if (g_free_talk_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_aip_handle malloc failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//初始化参数
	memset(g_free_talk_handler, 0, sizeof(FREE_TALK_HANDLER_t));
	TAILQ_INIT(&g_free_talk_handler->pcm_queue);

	SEMPHR_CREATE_LOCK(g_free_talk_handler->mutex_lock);
	if (g_free_talk_handler->mutex_lock == NULL)
	{
		free_talk_destroy();
		DEBUG_LOGE(LOG_TAG, "SEMPHR_CREATE_LOCK failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}

	//创建任务
	if (!task_thread_create(task_free_talk,
	        "task_free_talk",
	        APP_NAME_FREE_TALK_STACK_SIZE,
	        g_free_talk_handler,
	        task_priority)) 
    {
		free_talk_destroy();
        DEBUG_LOGE(LOG_TAG,  "task_free_talk failed");
		return APP_FRAMEWORK_ERRNO_TASK_FAILED;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t free_talk_delete(void)
{
	return app_send_message(APP_NAME_FREE_TALK, APP_NAME_FREE_TALK, APP_EVENT_DEFAULT_EXIT, NULL, 0);
}

APP_FRAMEWORK_ERRNO_t free_talk_start(FREE_TALK_CONFIG_t *config)
{
	uint32_t record_sn = 0;
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	if (config == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "config is NULL");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (g_free_talk_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "g_free_talk_handler is NULL");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (config->single_talk_max_ms > FREE_TALK_SINGLE_MAX_TIME)
	{
		DEBUG_LOGE(LOG_TAG,  "single_talk_max_ms can't > [%d]", FREE_TALK_SINGLE_MAX_TIME);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (config->vad_detect_ms == 0)
	{
		config->vad_detect_ms = 100;
	}

	if (config->vad_min_detect_ms == 0)
	{
		config->vad_min_detect_ms = 2000;
	}

	SEMPHR_TRY_LOCK(g_free_talk_handler->mutex_lock);
	g_free_talk_handler->config = *config;

	err = aip_start_record(&record_sn, config->single_talk_max_ms, free_talk_record_cb);
	
	//开始录音启动指示灯
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
	
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG,  "aip_start_record failed");
	}

	g_free_talk_handler->record_sn = record_sn;
	
	SEMPHR_TRY_UNLOCK(g_free_talk_handler->mutex_lock);

	return err;
}

APP_FRAMEWORK_ERRNO_t free_talk_stop(void)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	
	err = aip_stop_record(free_talk_record_cb);
	
	//恢复待机时灯效
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
	
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG,  "aip_stop_record failed");
	}

	return err;
}

APP_FRAMEWORK_ERRNO_t translate_start(FREE_TALK_MODE_t mode)
{
	uint32_t record_sn = 0;
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	FREE_TALK_CONFIG_t free_config = {0};

	free_config.mode = mode;
	free_config.single_talk_max_ms = 10*1000;
	free_config.vad_detect_sensitivity = 1;

	if (g_free_talk_handler == NULL)
	{
		DEBUG_LOGE(LOG_TAG,  "g_free_talk_handler is NULL");
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (free_config.single_talk_max_ms > FREE_TALK_SINGLE_MAX_TIME)
	{
		DEBUG_LOGE(LOG_TAG,  "single_talk_max_ms can't > [%d]", FREE_TALK_SINGLE_MAX_TIME);
		return APP_FRAMEWORK_ERRNO_INVALID_PARAMS;
	}

	if (free_config.vad_detect_ms == 0)
	{
		free_config.vad_detect_ms = 100;
	}

	if (free_config.vad_min_detect_ms == 0)
	{
		free_config.vad_min_detect_ms = 2000;
	}

	SEMPHR_TRY_LOCK(g_free_talk_handler->mutex_lock);
	g_free_talk_handler->config = free_config;

	err = aip_start_record(&record_sn, free_config.single_talk_max_ms, free_talk_record_cb);
	
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG,  "aip_start_record failed");
	}

	g_free_talk_handler->record_sn = record_sn;

	//开始录音启动指示灯
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);
	SEMPHR_TRY_UNLOCK(g_free_talk_handler->mutex_lock);

	return err;
}

APP_FRAMEWORK_ERRNO_t translate_stop(void)
{
	APP_FRAMEWORK_ERRNO_t err = APP_FRAMEWORK_ERRNO_OK;
	err = aip_stop_record(free_talk_record_cb);
	
	if (err != APP_FRAMEWORK_ERRNO_OK)
	{
		DEBUG_LOGE(LOG_TAG,  "aip_stop_record failed");
	}

	//恢复待机时灯效
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EYE_GRADUAL_CHANGE_FLICKER, NULL, 0);
	//app_send_message(APP_NAME_FREE_TALK, APP_NAME_LED_CTRL_SERVICE, APP_EVENT_LED_CTRL_EAR_ON, NULL, 0);

	return err;
}

/* 使free_talk操作完全终止 */
void free_talk_terminated()
{
	g_free_talk_handler->need_cancel_record_sn = ft_get_record_sn();
	free_talk_stop();
}


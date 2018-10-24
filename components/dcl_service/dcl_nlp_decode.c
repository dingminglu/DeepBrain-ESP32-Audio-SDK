#include "dcl_nlp_decode.h"
#include "debug_log_interface.h"
#include "cJSON.h"
#include "speaker_interface.h"
#include "player_middleware_interface.h"
#include "time.h"
#include "memo_service.h"
#include "MediaHal.h"

#define LOG_TAG "nlp decode" 

#define COMMAND_NAME_VOLUME_CTRL "音量调节"
#define COMMAND_NAME_MEMO_CTRL   "备忘"

static int music_source_decode(
	cJSON *json_data, 
	NLP_RESULT_T *nlp_result)
{
	int i = 0;
	int m = 0;
	int attrs_size = 0;
	int array_size = 0;
	int has_music = 0;
	char artist_name[32] = {0};
	NLP_RESULT_LINKS_T *p_links = &nlp_result->link_result;

	cJSON *pJson_music = cJSON_GetObjectItem(json_data, "linkResources");
	if (NULL != pJson_music)
	{
		array_size = cJSON_GetArraySize(pJson_music);
	}
	
	if (array_size > 0)
	{
		for (i = 0; i < array_size && i < NLP_SERVICE_LINK_MAX_NUM; i++)
		{
			cJSON *pJson_music_info = cJSON_GetArrayItem(pJson_music, i);
			cJSON *pJson_music_url = cJSON_GetObjectItem(pJson_music_info, "accessURL");
			cJSON *pJson_music_name = cJSON_GetObjectItem(pJson_music_info, "resName");
			if (NULL == pJson_music_url 
				&& NULL == pJson_music_url->valuestring)
			{
				continue;
			}

			nlp_result->type = NLP_RESULT_TYPE_LINK;
			snprintf(p_links->link[p_links->link_size].link_url, sizeof(p_links->link[p_links->link_size].link_url),
				"%s", pJson_music_url->valuestring);
			cJSON *pJson_attrs = cJSON_GetObjectItem(pJson_music_info, "resAttrs");
			if (pJson_attrs != NULL)
			{
				attrs_size =  cJSON_GetArraySize(pJson_attrs);
				if (attrs_size > 0)
				{
					for (m=0; m < attrs_size; m++)
					{
						cJSON *pJson_attrs_item = cJSON_GetArrayItem(pJson_attrs, m);
						if (pJson_attrs_item == NULL)
						{
							break;
						}

						cJSON *pJson_attrs_item_name = cJSON_GetObjectItem(pJson_attrs_item, "attrName");
						cJSON *pJson_attrs_item_value = cJSON_GetObjectItem(pJson_attrs_item, "attrValue");
						if (pJson_attrs_item_name->valuestring != NULL 
							&& pJson_attrs_item_value->valuestring != NULL
							&& strcmp(pJson_attrs_item_name->valuestring, "artist") == 0)
						{
							snprintf(artist_name, sizeof(artist_name), "%s的", pJson_attrs_item_value->valuestring);
						}
					}
				}
			}
			
			if (pJson_music_name != NULL && pJson_music_name->valuestring != NULL)
			{
				snprintf(p_links->link[p_links->link_size].link_name, sizeof(p_links->link[p_links->link_size].link_name),
					"正在给您播放%s%s", artist_name, pJson_music_name->valuestring);
			}
			else
			{
				snprintf(p_links->link[p_links->link_size].link_name, sizeof(p_links->link[p_links->link_size].link_name), 
					"%s", "现在开始播放");
			}
			p_links->link_size++;
		}
	}

	if (p_links->link_size == 0)
	{
		cJSON *pJson_tts = cJSON_GetObjectItem(json_data, "ttsText");
		cJSON *pJson_tts_link = cJSON_GetObjectItem(json_data, "ttsLink");
		nlp_result->type = NLP_RESULT_TYPE_NONE;		
		if (pJson_tts != NULL && pJson_tts->valuestring != NULL)
		{
			DEBUG_LOGE(LOG_TAG, "nlp tts result:%s", pJson_tts->valuestring);
			nlp_result->type = NLP_RESULT_TYPE_CHAT;
			snprintf(nlp_result->chat_result.text, sizeof(nlp_result->chat_result.text), "%s", pJson_tts->valuestring);
		}

		if (pJson_tts_link != NULL && pJson_tts_link->valuestring != NULL)
		{
			DEBUG_LOGE(LOG_TAG, "nlp tts link:%s", pJson_tts_link->valuestring);
			nlp_result->type = NLP_RESULT_TYPE_CHAT;
			snprintf(nlp_result->chat_result.link, sizeof(nlp_result->chat_result.link), "%s", pJson_tts_link->valuestring);
		}
	}
	
	return NLP_DECODE_ERRNO_OK;
}

NLP_DECODE_ERRNO_t dcl_nlp_result_decode(
	const char* json_string, 
	NLP_RESULT_T *nlp_result)
{	
	NLP_DECODE_ERRNO_t err = NLP_DECODE_ERRNO_OK;
	
	cJSON *pJson_body = cJSON_Parse(json_string);
    if (NULL != pJson_body) 
	{
		cJSON *pJson_head = cJSON_GetObjectItem(pJson_body, "responseHead");
		if (pJson_head == NULL)
		{
			err = NLP_DECODE_ERRNO_INVALID_JSON;
			goto dp_nlp_result_decode_error;
		}
		
		cJSON *pJson_status_code = cJSON_GetObjectItem(pJson_head, "statusCode");
		if (NULL == pJson_status_code || pJson_status_code->valuestring == NULL)
		{
			err = NLP_DECODE_ERRNO_INVALID_JSON;
			goto dp_nlp_result_decode_error;
		}

		cJSON *pJson_asr_data = cJSON_GetObjectItem(pJson_body, "asrData");
		if (NULL != pJson_asr_data)
		{
			cJSON *pJson_asr_ret = cJSON_GetObjectItem(pJson_asr_data, "Result");
			if (pJson_asr_ret != NULL && pJson_asr_ret->valuestring != NULL)
			{
				DEBUG_LOGE(LOG_TAG, "asrData:[%s]", pJson_asr_ret->valuestring);
				snprintf(nlp_result->input_text, sizeof(nlp_result->input_text), "%s", pJson_asr_ret->valuestring);
			}
		}

		if (strcasecmp(pJson_status_code->valuestring, "OK") == 0)
		{
			
		}
		else if (strcasecmp(pJson_status_code->valuestring, "RECOGNISE_FAIL") == 0)
		{
			nlp_result->type = NLP_RESULT_TYPE_NO_ASR;
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			goto dp_nlp_result_decode_error;
		}
		else if (strcasecmp(pJson_status_code->valuestring, "NO_INPUT_TEXT") == 0)
		{
			nlp_result->type = NLP_RESULT_TYPE_NO_ASR;
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			goto dp_nlp_result_decode_error;
		}
		else if (strcasecmp(pJson_status_code->valuestring, "NO_PRIVILEGE") == 0)
		{
			nlp_result->type = NLP_RESULT_TYPE_NO_LICENSE;
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			goto dp_nlp_result_decode_error;
		}
		else
		{
			nlp_result->type = NLP_RESULT_TYPE_ERROR;
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			goto dp_nlp_result_decode_error;
		}
	
        cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
		if (NULL == pJson_cmd_data)
		{
			err = NLP_DECODE_ERRNO_INVALID_JSON;
			goto dp_nlp_result_decode_error;
		}

		cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
		if (pJson_cmd_name == NULL || pJson_cmd_name->valuestring == NULL)
		{
			err = NLP_DECODE_ERRNO_INVALID_JSON;
			goto dp_nlp_result_decode_error;
		}
		
		if (strcmp(pJson_cmd_name->valuestring, COMMAND_NAME_VOLUME_CTRL) == 0)
		{
			nlp_result->type = NLP_RESULT_TYPE_VOL_CMD;
		}
		else if (strcmp(pJson_cmd_name->valuestring, COMMAND_NAME_MEMO_CTRL) == 0)
		{
			get_memo_result(json_string);//获取提醒结果
			nlp_result->type = NLP_RESULT_TYPE_CMD;
		}
		else
		{
			err = music_source_decode(pJson_cmd_data, nlp_result);
			if (err != NLP_DECODE_ERRNO_OK)
			{
				goto dp_nlp_result_decode_error;
			}
		}		
    }
	else
	{
		DEBUG_LOGE(LOG_TAG, "nlp_result_decode failed[%d][%s]", strlen(json_string), json_string);
		err = NLP_DECODE_ERRNO_INVALID_JSON;
		goto dp_nlp_result_decode_error;
	}

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return err;
	
dp_nlp_result_decode_error:
	
	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}

	return err;
}


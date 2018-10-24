#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "speaker_interface.h"
#include "dcl_volume_decode.h"
#include "debug_log_interface.h"
#include "cJSON.h"
#include "player_middleware_interface.h"

#define VOL_STEP               		10
#define ATTR_NAME_VOL_CTRL			"音量控制"
#define ATTR_NAME_VOL_SCALE			"音量调节刻度"
#define LOG_TAG						"volume manage"

static void get_vol_ctrl_cmd(cJSON *pJson_cmd_attrs_array_item, VOLUME_CONTROL_TYPE_t *volume_control_state)
{
	cJSON *pJson_attr_value_type = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrValue");
	if (pJson_attr_value_type != NULL && pJson_attr_value_type->valuestring != NULL)
	{		
		if (strcmp(pJson_attr_value_type->valuestring, "调高") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_HIGHER;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "调低") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_LOWER;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "适中") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MODERATE;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "最大") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MAX;
		}
		if (strcmp(pJson_attr_value_type->valuestring, "最小") == 0)
		{
			*volume_control_state = VOLUME_CONTROL_MIN;
		}			
	}
	else
	{
		*volume_control_state = VOLUME_CONTROL_INITIAL;
		DEBUG_LOGE(LOG_TAG, "pJson_attr_value_type wrong\n");	
	}			
}

static void get_vol_scale_need_to_change(cJSON *pJson_cmd_attrs_array_item, VOLUME_TYPE_T *volume_type, int *scale)
{
	cJSON *pJson_attr_value_scale = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrValue");
	if (pJson_attr_value_scale != NULL && pJson_attr_value_scale->valuestring != NULL)
	{
		if (strcmp(pJson_attr_value_scale->valuestring, "") != 0)
		{
			*scale = atoi(pJson_attr_value_scale->valuestring);
			*volume_type = VOLUME_TYPE_SCALE_MODE;
		}
		else
		{
			*volume_type = VOLUME_TYPE_NOT_SCALE_MODE;
		}
	}
}

static VOLUME_CONTROL_TYPE_t volume_control_process(VOLUME_CONTROL_TYPE_t volume_control_state, int volume_change_value)
{
	int current_vol = 0;
	VOLUME_CONTROL_TYPE_t ret = VOLUME_CONTROL_INITIAL;
	
	if (!get_volume(&current_vol))
	{
		DEBUG_LOGE(LOG_TAG, "GetVolume failed! (line %d)", __LINE__);
		return VOLUME_CONTROL_INITIAL;
	}

	DEBUG_LOGI(LOG_TAG, "now_vol = %d", current_vol);
	
	switch (volume_control_state)
	{
		case VOLUME_CONTROL_HIGHER:
		{
			DEBUG_LOGI(LOG_TAG, "vol up");
        	int need_to_set_vol = current_vol + volume_change_value;
			if (need_to_set_vol > 100)
			{
				if (!set_volume(100))
				{
	                if (!set_volume(100))
					{
						DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
						ret =  VOLUME_CONTROL_INITIAL;
						break;
		            }
	            }
				DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", 100, __LINE__);
				ret = VOLUME_CONTROL_MAX;
			}
			else
			{
				if (!set_volume(need_to_set_vol))
				{
	                if (!set_volume(need_to_set_vol))
					{
						DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
						ret = VOLUME_CONTROL_INITIAL;
						break;
		            }
	            }
				DEBUG_LOGI(LOG_TAG, "after vol = %d (line %d)", need_to_set_vol, __LINE__);
				ret = VOLUME_CONTROL_HIGHER;
			}
			break;
		}
		case VOLUME_CONTROL_LOWER:
		{
			DEBUG_LOGI(LOG_TAG, "vol down");
        	int need_to_set_vol = current_vol - volume_change_value;
			if (need_to_set_vol < 0)
			{
				if (!set_volume(0))
				{
	                if (!set_volume(0))
					{
						DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
						ret = VOLUME_CONTROL_INITIAL;
						break;
		            }
	            }
				DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", 0, __LINE__);
				ret = VOLUME_CONTROL_MIN;
			}
			else
			{
				if (!set_volume(need_to_set_vol))
				{
	                if (!set_volume(need_to_set_vol))
					{
						DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
						ret = VOLUME_CONTROL_INITIAL;
						break;
		            }
	            }
				DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", need_to_set_vol, __LINE__);
				ret = VOLUME_CONTROL_LOWER;
			}
			break;
		}
		case VOLUME_CONTROL_MODERATE:
		{
			DEBUG_LOGI(LOG_TAG, "vol moderate");
			if (!set_volume(55))
			{
                if (!set_volume(55))
				{
					DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
					ret = VOLUME_CONTROL_INITIAL;
					break;
	            }
            }
			DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", 55, __LINE__);
			ret = VOLUME_CONTROL_MODERATE;
			break;
		}
		case VOLUME_CONTROL_MAX:
		{
			DEBUG_LOGI(LOG_TAG, "vol max");
			if (!set_volume(100))
			{
                if (!set_volume(100))
				{
					DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
					ret = VOLUME_CONTROL_INITIAL;
					break;
	            }
            }
			DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", 100, __LINE__);
			ret = VOLUME_CONTROL_MAX;
			break;
		}
		case VOLUME_CONTROL_MIN:
		{
			DEBUG_LOGI(LOG_TAG, "vol min");
			if (!set_volume(0))
			{
                if (!set_volume(0))
				{
					DEBUG_LOGE(LOG_TAG, "SetVolume failed! (line %d)", __LINE__);
					ret = VOLUME_CONTROL_INITIAL;
					break;
	            }
            }
			DEBUG_LOGI(LOG_TAG, "after set vol = %d (line %d)", 0, __LINE__);
			ret = VOLUME_CONTROL_MIN;
			break;
		}
		default:
		{
			ret = VOLUME_CONTROL_INITIAL;
			break;
		}
	}
	
	return ret;
}

static int choose_vol_value(VOLUME_TYPE_T volume_type, int constant_vol_value, int scale_mode_vol_value)
{
	if (volume_type == VOLUME_TYPE_SCALE_MODE)
	{
		return scale_mode_vol_value;
	}
	else
	{
		return constant_vol_value;
	}
}

VOLUME_CONTROL_TYPE_t dcl_nlp_volume_cmd_decode(const char* json_string)
{
	int err = 0;
	int array_size = 0;
	int volume_change_scale = 0;
	int constant_vol_value = VOL_STEP;
	VOLUME_TYPE_T volume_type = VOLUME_TYPE_INITIAL;
	VOLUME_CONTROL_TYPE_t volume_control_state = VOLUME_CONTROL_INITIAL;
	
	//DEBUG_LOGE(LOG_TAG, "json_string is \n[%s]\n (line %d)", json_string, __LINE__);

	cJSON *pJson_body = cJSON_Parse(json_string);
    if (NULL != pJson_body) 
	{
		cJSON *pJson_head = cJSON_GetObjectItem(pJson_body, "responseHead");
		if (pJson_head == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "pJson_head failed! (line %d)", __LINE__);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		
		cJSON *pJson_status_code = cJSON_GetObjectItem(pJson_head, "statusCode");
		if (NULL == pJson_status_code || pJson_status_code->valuestring == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "pJson_status_code failed! (line %d)", __LINE__);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}

		cJSON *pJson_asr_data = cJSON_GetObjectItem(pJson_body, "asrData");
		if (NULL != pJson_asr_data)
		{
			cJSON *pJson_asr_ret = cJSON_GetObjectItem(pJson_asr_data, "Result");
			if (pJson_asr_ret != NULL && pJson_asr_ret->valuestring != NULL)
			{
				DEBUG_LOGE(LOG_TAG, "asrData:[%s]", pJson_asr_ret->valuestring);
			}
		}

		if (strcasecmp(pJson_status_code->valuestring, "OK") == 0)
		{
			
		}
		else if (strcasecmp(pJson_status_code->valuestring, "RECOGNISE_FAIL") == 0)
		{
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		else if (strcasecmp(pJson_status_code->valuestring, "NO_INPUT_TEXT") == 0)
		{
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		else if (strcasecmp(pJson_status_code->valuestring, "NO_PRIVILEGE") == 0)
		{
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "statusCode:%s", pJson_status_code->valuestring);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
	
        cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
		if (NULL == pJson_cmd_data)
		{
			DEBUG_LOGE(LOG_TAG, "pJson_cmd_data failed! (line %d)", __LINE__);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		
		cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
		if (pJson_cmd_name == NULL || pJson_cmd_name->valuestring == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "pJson_cmd_name failed! (line %d)", __LINE__);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
		
		cJSON *pJson_cmd_attrs = cJSON_GetObjectItem(pJson_cmd_data, "commandAttrs");
		if (pJson_cmd_attrs != NULL)
		{
			array_size = cJSON_GetArraySize(pJson_cmd_attrs);
			if (array_size > 0)
			{
				for (int i = 0; i < array_size; i++)
				{
					cJSON *pJson_cmd_attrs_array_item = cJSON_GetArrayItem(pJson_cmd_attrs, i);
					if (pJson_cmd_attrs_array_item != NULL)
					{
						cJSON *pJson_attr_name = cJSON_GetObjectItem(pJson_cmd_attrs_array_item, "attrName");
						if (strcmp(pJson_attr_name->valuestring, ATTR_NAME_VOL_CTRL) == 0)
						{
							get_vol_ctrl_cmd(pJson_cmd_attrs_array_item, &volume_control_state);
						}
						else if (strcmp(pJson_attr_name->valuestring, ATTR_NAME_VOL_SCALE) == 0)
						{
							get_vol_scale_need_to_change(pJson_cmd_attrs_array_item, &volume_type, &volume_change_scale);
						}
					}
					else
					{
						DEBUG_LOGE(LOG_TAG, "pJson failed! (line %d)", __LINE__);
						err = VOLUME_CONTROL_INITIAL;
						goto dcl_nlp_volume_cmd_decode_error;
					}
				}
			}
		}
		else
		{
			DEBUG_LOGE(LOG_TAG, "pJson_cmd_attrs is NULL! (line %d)", __LINE__);
			err = VOLUME_CONTROL_INITIAL;
			goto dcl_nlp_volume_cmd_decode_error;
		}
    }

	err = volume_control_process(volume_control_state, choose_vol_value(volume_type, constant_vol_value, volume_change_scale));
	dcl_nlp_volume_cmd_decode_error:
	
	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return err;
}


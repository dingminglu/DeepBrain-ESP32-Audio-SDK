#include <stdio.h>
#include <string.h>
#include "string.h"
#include "keyboard_event.h"
#include "debug_log_interface.h"
#include "memory_interface.h"

#define KEY_IDEL 0
#define VALUE_RANGE(a,b,c) ((a >= (b-c)) && (a <= (b+c)))
#define KEY_LOG(a,b,c,d,e) DEBUG_LOGI("Keyevent", "===========[value-%d] [key-%d] [event-%d] [%s-%dms]\n", a, b, c, d, e)

//static KEYBOARD_CFG_T keyboard_cfg = {0};

int adc_keyboard_process(KEYBOARD_OBJ_T *kobj, int adc_value, int *key_status)
{	
	int i = 0;
	int event_result = KEY_IDEL;

#if ADC_OFFSET_EN
	if (VALUE_RANGE(adc_value, kobj->k_cfg.keyboard[0].adc_value + kobj->ofs_val,
			kobj->k_cfg.fault_tol))
	{
		kobj->idelnum++;
		kobj->idelsum += adc_value;
		
		if (kobj->idelnum == 200) 
		{
			kobj->ofs_val = (kobj->idelsum / kobj->idelnum) - kobj->k_cfg.keyboard[0].adc_value;
			kobj->idelsum = 0;
			kobj->idelnum = 0;
		}
	}
#endif

	for (i = 0; i <= kobj->k_cfg.key_num; i++)//查找匹配的按键
	{
		if (VALUE_RANGE(adc_value, kobj->k_cfg.keyboard[i].adc_value + kobj->ofs_val,
			kobj->k_cfg.fault_tol))
		{
			kobj->curr_key_num = i;
			break;
		}
	
		if (i == kobj->k_cfg.key_num)
		{
			KEY_LOG(adc_value, 0, 0, "Undefined", 0);//没有匹配值
			return 0;
		}
	}
	
	kobj->press_hold_ms += kobj->k_cfg.scan_cycle;
	if (kobj->curr_key_num == KEY_IDEL && kobj->curr_key_num == kobj->last_key_num) //空闲
	{
		if(key_status != NULL)
		{
			*key_status = 0;
		}	
		if (kobj->click_flag == 1)/* 有双击定义时执行 */
		{
			kobj->click_time += kobj->k_cfg.scan_cycle;
			if (kobj->click_time >= DOUBLETIME)
			{
				if (kobj->k_cfg.keyboard[kobj->last_click_num].sigleclick == 0)//单击未定义返回release
				{
					event_result = kobj->k_cfg.keyboard[kobj->last_click_num].release;//释放
					KEY_LOG(adc_value, kobj->last_click_num, event_result, "**release", kobj->click_time);
					kobj->click_flag = 0;
					kobj->click_time = 0;
					return event_result;
				}
				
				event_result = kobj->k_cfg.keyboard[kobj->last_click_num].sigleclick;//单击
				KEY_LOG(adc_value, kobj->last_click_num, event_result, "*sigleclick", kobj->click_time);
				kobj->click_flag = 0;
				kobj->click_time = 0;
				return event_result;
			}
		}
		
		return KEY_IDEL;
	}
	else if (kobj->curr_key_num == KEY_IDEL && kobj->curr_key_num != kobj->last_key_num)//按下后弹起
	{
		if (kobj->press_hold_ms <= kobj->k_cfg.longpress_time)//短按
		{
			if (kobj->k_cfg.keyboard[kobj->last_key_num].doubleclick != 0)//有双击定义
			{
				if ((kobj->click_time > 0 && kobj->click_time < DOUBLETIME) && kobj->last_click_num == kobj->last_key_num)
				{							
					event_result = kobj->k_cfg.keyboard[kobj->last_click_num].doubleclick;//双击
					KEY_LOG(adc_value, kobj->last_click_num, event_result, "doubleclick", kobj->click_time);
					kobj->last_click_num  = 0;
					kobj->click_flag = 0;
					kobj->click_time = 0;
					kobj->last_key_num = kobj->curr_key_num;
					return event_result;
				}
				
				kobj->click_flag = 1;
				kobj->click_time = 0;
				kobj->last_click_num = kobj->last_key_num;
				kobj->last_key_num   = kobj->curr_key_num;
				return KEY_IDEL;
			}
			else//无双击定义
			{	
				if (kobj->k_cfg.keyboard[kobj->last_key_num].sigleclick == 0)//单击未定义返回release
				{
					event_result = kobj->k_cfg.keyboard[kobj->last_key_num].release;//释放
					KEY_LOG(adc_value, kobj->last_key_num, event_result, "*release", kobj->press_hold_ms);
					
					kobj->press_hold_ms = 0;
					kobj->last_key_num  = kobj->curr_key_num;
					return event_result;
				}
				
				event_result = kobj->k_cfg.keyboard[kobj->last_key_num].sigleclick;//单击	
				KEY_LOG(adc_value, kobj->last_key_num, event_result, "sigleclick", kobj->press_hold_ms);
				
				kobj->press_hold_ms = 0;
				kobj->last_key_num  = kobj->curr_key_num;
				return event_result;
			}
		}
		else//长按后释放
		{
			event_result = kobj->k_cfg.keyboard[kobj->last_key_num].release;//释放
			KEY_LOG(adc_value, kobj->last_key_num, event_result, "release", kobj->press_hold_ms);
			
			kobj->press_hold_ms  = 0;
			kobj->press_hold_flag = 0;
			kobj->custom_hold_flag = 0;
			kobj->last_key_num = kobj->curr_key_num;
			return event_result;
		}
	}	
	else//有键按下
	{
		if(key_status != NULL)
		{
			*key_status = 1;
		}
		if (kobj->curr_key_num != kobj->last_key_num)//与上次不同
		{
			event_result = kobj->k_cfg.keyboard[kobj->curr_key_num].push;//按下
			KEY_LOG(adc_value, kobj->curr_key_num, event_result, "push", 0);
			
			kobj->press_hold_ms = 0;
			kobj->last_key_num  = kobj->curr_key_num;
			return event_result;
		}
		else//与上次相同
		{
			if (kobj->k_cfg.keyboard[kobj->curr_key_num].custompress != 0)//自定义长按
			{
				if (kobj->press_hold_ms > kobj->k_cfg.keyboard[kobj->curr_key_num].custom_time && kobj->custom_hold_flag == 0)
				{					
					event_result = kobj->k_cfg.keyboard[kobj->curr_key_num].custompress;				
					KEY_LOG(adc_value, kobj->curr_key_num, event_result, "customlongpress", kobj->press_hold_ms);
					
					kobj->custom_hold_flag = 1;
					return event_result;
				}
			}
			
			if (kobj->press_hold_ms > kobj->k_cfg.longpress_time && kobj->press_hold_flag == 0)
			{
				event_result = kobj->k_cfg.keyboard[kobj->curr_key_num].longpress;//长按
				KEY_LOG(adc_value, kobj->curr_key_num, event_result, "longpress", kobj->press_hold_ms);
				
				kobj->press_hold_flag = 1;
				return event_result;
			}
			
			return KEY_IDEL;
		}	
	}	
}

KEYBOARD_OBJ_T *keyboard_create(KEYBOARD_CFG_T *keyboard_cfg)
{
	if (keyboard_cfg == NULL) {
		return NULL;
	}
	
	KEYBOARD_OBJ_T *kobj = NULL;	
	kobj = (KEYBOARD_OBJ_T *)memory_malloc(sizeof(KEYBOARD_OBJ_T));	
	if (kobj == NULL) {
		return NULL;
	}
	memset(kobj, 0, sizeof(KEYBOARD_OBJ_T));

	kobj->k_cfg = *keyboard_cfg;
	
	return kobj;
}



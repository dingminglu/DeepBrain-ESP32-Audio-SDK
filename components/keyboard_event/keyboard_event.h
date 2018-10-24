#ifndef KEYBOARD_EVENT_H
#define KEYBOARD_EVENT_H

#define KEY_NUM         16      /* 最大按键数量 */
#define DOUBLETIME      280     /* 双击间隔 */
#define ADC_OFFSET_EN	0       /* ADC偏移使能 */

/* 
 * 按键属性 KEY_ATTR_T
 * adc_value：	ADC值
 * custom_time：	自定义长按时长         任意值
 * push：		按下
 * release：	    释放
 * longpress：	长按 
 * sigleclick：	单击   sigleclick与release互斥，两者都定义时，只返回sigleclick
 * doubleclick：	双击 doubleclick与release互斥，两者都定义时，只返回doubleclick
 * custompress：自定义长按
 * combine1：	组合1
 * combine2：	组合2
*/
typedef struct{
    int adc_value;
	int custom_time;
	int push;
	int release;
	int longpress;
	int sigleclick;
	int doubleclick;
	int custompress;
	int combine1;
	int combine2;
}KEY_ATTR_T;

/*
 * 键盘配置信息 KEYBOARD_CFG_T：
 * key_num：         键盘数量
 * fault_tol：       容错值
 * scan_cycle：      扫描周期ms
 * longpress_time ：长按时间ms    (大于为长按，小于为单击)
 * KEY_NUM：         最大键盘数量：16
 */
typedef struct{
	int key_num;
	int fault_tol;
	int scan_cycle;
	int longpress_time;
	KEY_ATTR_T keyboard[KEY_NUM];
}KEYBOARD_CFG_T;

typedef struct{	
	int ofs_val;
	int idelnum;
	int idelsum;
	int click_time;
	int click_flag;
	int last_click_num;
	int curr_key_num;
	int last_key_num;
	int press_hold_ms;
	int press_hold_flag;
	int custom_hold_flag;
	KEYBOARD_CFG_T k_cfg;
}KEYBOARD_OBJ_T;

/*
 *创建键盘
 */
KEYBOARD_OBJ_T *keyboard_create(KEYBOARD_CFG_T *keyboard_cfg);

/*
 *按键处理
 */
int adc_keyboard_process(KEYBOARD_OBJ_T *k_obj, int adc_value, int *key_status);



#endif

#ifndef KEYWORD_WAKEUP_H
#define KEYWORD_WAKEUP_H

#include "userconfig.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define KWW_AUTH_ADDR PARTITIONS_ADDR_WAKEUP_AUTH //唤醒库认证文件存储地址 

//消息通知回调函数
typedef void (*keyword_wakeup_event_cb)(APP_EVENT_KEYWORD_WAKEUP_t event);

/**
 * create keyword wakeup service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t keyword_wakeup_create(int task_priority);

/**
 * keyword wakeup service delete
 *
 * @param none
 * @return app framework errno
 */
APP_FRAMEWORK_ERRNO_t keyword_wakeup_delete(void);

/**
 * keyword wakeup start
 *
 * @param none
 * @return none
 */
void keyword_wakeup_start(void);

/**
 * keyword wakeup stop
 *
 * @param none
 * @return none
 */
void keyword_wakeup_stop(void);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif


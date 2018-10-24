
#include "esp_audio_log.h"


/*
    comment out any if branch to hide corresponding print information
*/

int esp_audio_log_switch(esp_log_level_t level)
{
    int ret = 0;

    if (level == ESP_LOG_ERROR) {
        ret = 1;
    }
    if (level == ESP_LOG_WARN) {
        ret = 1;
    }
    if (level == ESP_LOG_INFO) {
        ret = 1;
    }
//    if (level == ESP_LOG_DEBUG) {
//        ret = 1;
//    }
//    if (level == ESP_LOG_VERBOSE) {
//        ret = 1;
//    }

    return ret;
}


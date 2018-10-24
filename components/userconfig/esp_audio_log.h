#ifndef __ESP_AUDIO_LOG_CONFIG_H__
#define __ESP_AUDIO_LOG_CONFIG_H__

#include "esp_log.h"

int esp_audio_log_switch(esp_log_level_t level);

#define AUDIO_LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define AUDIO_LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define AUDIO_LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define AUDIO_LOG_COLOR_D       LOG_COLOR(LOG_COLOR_CYAN)
#define AUDIO_LOG_COLOR_V       LOG_COLOR(LOG_COLOR_PURPLE)

#define AUDIO_LOG_FORMAT(letter, format)  AUDIO_LOG_COLOR_ ## letter #letter " (%d) %s: " format LOG_RESET_COLOR "\n"


#define ESP_AUDIO_LOGE(tag, format, ...) if (esp_audio_log_switch(ESP_LOG_ERROR)) { esp_log_write(ESP_LOG_ERROR, tag, AUDIO_LOG_FORMAT(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_AUDIO_LOGW(tag, format, ...) if (esp_audio_log_switch(ESP_LOG_WARN)) { esp_log_write(ESP_LOG_WARN, tag, AUDIO_LOG_FORMAT(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_AUDIO_LOGI(tag, format, ...) if (esp_audio_log_switch(ESP_LOG_INFO)) { esp_log_write(ESP_LOG_INFO, tag, AUDIO_LOG_FORMAT(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_AUDIO_LOGD(tag, format, ...) if (esp_audio_log_switch(ESP_LOG_DEBUG)) { esp_log_write(ESP_LOG_DEBUG, tag, AUDIO_LOG_FORMAT(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }

#define ESP_AUDIO_LOGV(tag, format, ...) if (esp_audio_log_switch(ESP_LOG_VERBOSE)) { esp_log_write(ESP_LOG_VERBOSE, tag, AUDIO_LOG_FORMAT(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__); }


#endif /* __ESP_AUDIO_LOG_CONFIG_H__ */


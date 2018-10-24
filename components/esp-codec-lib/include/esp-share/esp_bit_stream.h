// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_BIT_STREAM_H_
#define _ESP_BIT_STREAM_H_

int get_esp_stream_data(void *codec_data, void *buf, unsigned int length);
void stream_byte_pos_update(void *codec_data, unsigned int length);

#endif
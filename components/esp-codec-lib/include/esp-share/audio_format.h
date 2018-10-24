// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_FORMAT_H_
#define _AUDIO_FORMAT_H_

typedef struct {
    int audio_type;
    unsigned char *head_buf;
    unsigned int head_buf_size;
    unsigned int head_buf_datanum;
} audio_type_buf_t;

void *audio_type_detect(void *codec_data);
int read_audio_type_info(void *audio_type_data);

#endif
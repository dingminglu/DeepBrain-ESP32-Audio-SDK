// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_DECODER_H_
#define _ESP_DECODER_H_

#include "esp_codec_config.h"

#if defined(USE_LIB_FLUENDO_MP3) || defined(USE_LIB_OPUS)
    #define ESP_DEC_STACK_SIZE (30)
#elif defined(USE_LIB_VORBIS) || defined(USE_LIB_MAD_MP3)
    #define ESP_DEC_STACK_SIZE (10)
#elif defined(USE_LIB_FDKAAC)
    #define ESP_DEC_STACK_SIZE (6)
#elif defined(USE_LIB_AAC) || defined(USE_LIB_AMR) | defined(USE_LIB_HELIX_MP3) || defined(USE_LIB_MP3)
    #define ESP_DEC_STACK_SIZE (5)
#elif defined(USE_LIB_FLAC)
    #define ESP_DEC_STACK_SIZE (4)
#else
    #define ESP_DEC_STACK_SIZE (3)
#endif

#include "SoftCodec.h"

#ifdef ESP_AUTO_PLAY

#define AUDIO_TYPE_UNKNOW (0)
#define AUDIO_TYPE_WAV (1)
#define AUDIO_TYPE_AMRNB (2)
#define AUDIO_TYPE_AMRWB (3)
#define AUDIO_TYPE_MP3 (4)
#define AUDIO_TYPE_AAC (5)
#define AUDIO_TYPE_M4A (6)
#define AUDIO_TYPE_VORBIS (7)
#define AUDIO_TYPE_OPUS (8)
#define AUDIO_TYPE_RAWFLAC (9)
#define AUDIO_TYPE_OGGFLAC (10)
#define AUDIO_TYPE_UNSUPPORT (11)

int esp_decoder_open(SoftCodec *softCodec);
int esp_decoder_close(SoftCodec *softCodec);
int esp_decoder_process(SoftCodec *softCodec);
int esp_decoder_trigger_stop(SoftCodec *softCodec);
int esp_decoder_get_pos(SoftCodec *softCodec, int pos);
void *audio_type_detect(void *codec_data);
int read_audio_type_info(void *audio_type_data);
#endif

#endif  ////#ifndef _ESP_DECODER_H_

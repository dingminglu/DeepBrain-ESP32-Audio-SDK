// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP32_SOFT_CODEC_H_
#define _ESP32_SOFT_CODEC_H_
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "AudioCodec.h"

#define CODEC_DONE  -2
#define CODEC_FAIL  -1
#define CODEC_OK     0

typedef struct {
    int enableEncoder;
} SoftCodecCfg;


typedef struct SoftCodec {
    AudioCodec Based;
    void *instance;
    /* private Properties */
    int _run;
    int _isOpen;
    int _pausing;
    int _isPause;
    int _isMute;
    int _currentPtr;
    int _audio_type;
    int _codec_run;
    int _startPtr;
    int _metadata[5];  /* sample rate, nchannels, bit rate, pointer, length*/
    int _break;
    SemaphoreHandle_t _sem;
    SemaphoreHandle_t _lock;
    void (*lock)(struct SoftCodec *softCodec);
    void (*unlock)(struct SoftCodec *softCodec);
    int (*setup)(struct SoftCodec *softCodec, int samplerate, int channel, int bits);
} SoftCodec;

extern signed char SoftCodecWaitInput; //when flac_decoder_read_callback read data SoftCodecWaitInput will be set as 0

SoftCodec *SoftDecoderCreate(const SoftCodecCfg *cfg);
SoftCodec *SoftEncoderCreate(const SoftCodecCfg *cfg);

#endif

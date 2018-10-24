// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _PCM_ENCODEC_H_
#define _PCM_ENCODEC_H_

#include "SoftCodec.h"

int pcm_encoder_open(SoftCodec *softCodec);
int pcm_encoder_close(SoftCodec *softCodec);
int pcm_encoder_process(SoftCodec *softCodec);

#endif
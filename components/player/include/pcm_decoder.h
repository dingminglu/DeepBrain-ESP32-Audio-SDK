// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _PCM_DECODER_H_
#define _PCM_DECODER_H_

#include "SoftCodec.h"

int pcm_decoder_open(SoftCodec *softCodec);
int pcm_decoder_close(SoftCodec *softCodec);
int pcm_decoder_process(SoftCodec *softCodec);
int pcm_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

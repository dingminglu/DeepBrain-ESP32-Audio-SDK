// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _WAV_DECODER_H_
#define _WAV_DECODER_H_

#include "SoftCodec.h"

int wav_decoder_open(SoftCodec *softCodec);
int wav_decoder_close(SoftCodec *softCodec);
int wav_decoder_process(SoftCodec *softCodec);
int wav_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

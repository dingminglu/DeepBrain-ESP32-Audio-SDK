// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _OPUS_DECODER_H_
#define _OPUS_DECODER_H_

#include "SoftCodec.h"

int opus_decoder_open(SoftCodec *softCodec);
int opus_decoder_close(SoftCodec *softCodec);
int opus_decoder_process(SoftCodec *softCodec);
int opus_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

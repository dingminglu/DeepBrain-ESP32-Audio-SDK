// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _OPUS_ENCODER_H_
#define _OPUS_ENCODER_H_

#include "SoftCodec.h"

int opus_encoder_open(SoftCodec *softCodec);
int opus_encoder_close(SoftCodec *softCodec);
int opus_encoder_process(SoftCodec *softCodec);

#endif
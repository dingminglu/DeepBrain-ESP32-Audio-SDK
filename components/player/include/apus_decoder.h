// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _APUS_DECODER_H_
#define _APUS_DECODER_H_

#include "SoftCodec.h"

int apus_decoder_open(SoftCodec *softCodec);
int apus_decoder_close(SoftCodec *softCodec);
int apus_decoder_process(SoftCodec *softCodec);
int apus_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

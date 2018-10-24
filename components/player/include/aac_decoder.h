// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AAC_DECODER_H_
#define _AAC_DECODER_H_

#include "SoftCodec.h"

int aac_decoder_open(SoftCodec *softCodec);
int aac_decoder_close(SoftCodec *softCodec);
int aac_decoder_process(SoftCodec *softCodec);
int aac_decoder_trigger_stop(SoftCodec *softCodec);
int aac_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

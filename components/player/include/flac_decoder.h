// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _FLAC_DECODER_H_
#define _FLAC_DECODER_H_

#include "SoftCodec.h"

int flac_decoder_open(SoftCodec *softCodec);
int flac_decoder_close(SoftCodec *softCodec);
int flac_decoder_process(SoftCodec *softCodec);
int flac_decoder_trigger_stop(SoftCodec *softCodec);
int flac_decoder_get_pos(SoftCodec *softCodec, int timePos);
#endif

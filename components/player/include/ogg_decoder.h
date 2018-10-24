// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _OGG_DECODER_H_
#define _OGG_DECODER_H_

#include "SoftCodec.h"

int ogg_decoder_open(SoftCodec *softCodec);
int ogg_decoder_close(SoftCodec *softCodec);
int ogg_decoder_process(SoftCodec *softCodec);
int ogg_decoder_trigger_stop(SoftCodec *softCodec);
int ogg_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

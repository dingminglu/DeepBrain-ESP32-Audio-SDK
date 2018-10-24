// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MP3_DECODER_H_
#define _MP3_DECODER_H_

#include "SoftCodec.h"

int mp3_decoder_open(SoftCodec *softCodec);
int mp3_decoder_close(SoftCodec *softCodec);
int mp3_decoder_process(SoftCodec *softCodec);
int mp3_decoder_trigger_stop(SoftCodec *softCodec);
int mp3_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

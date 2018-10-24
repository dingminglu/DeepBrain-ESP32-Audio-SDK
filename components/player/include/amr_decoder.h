// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AMR_DECODER_H_
#define _AMR_DECODER_H_

#include "SoftCodec.h"

int amr_decoder_open(SoftCodec *softCodec);
int amr_decoder_close(SoftCodec *softCodec);
int amr_decoder_process(SoftCodec *softCodec);
int amr_decoder_get_pos(SoftCodec *softCodec, int timePos);

#endif

// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _FDKAAC_DECODER_H_
#define _FDKAAC_DECODER_H_

#include "SoftCodec.h"

int fdkaac_decoder_open(SoftCodec *softCodec);
int fdkaac_decoder_close(SoftCodec *softCodec);
int fdkaac_decoder_process(SoftCodec *softCodec);
int fdkaac_decoder_trigger_stop(SoftCodec *softCodec);

int fdkm4a_decoder_open(SoftCodec *softCodec);
int fdkm4a_decoder_close(SoftCodec *softCodec);
int fdkm4a_decoder_process(SoftCodec *softCodec);
int fdkm4a_decoder_trigger_stop(SoftCodec *softCodec);

#endif
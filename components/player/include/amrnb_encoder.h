// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _AMRNB_ENCODER_H_
#define _AMRNB_ENCODER_H_

#include "SoftCodec.h"

int amrnb_encoder_open(SoftCodec *softCodec);
int amrnb_encoder_close(SoftCodec *softCodec);
int amrnb_encoder_process(SoftCodec *softCodec);
int amrnb_encoder_trigger_stop(SoftCodec *softCodec);

#endif
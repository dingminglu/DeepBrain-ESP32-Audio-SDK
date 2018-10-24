// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _APUS_ENCODER_H_
#define _APUS_ENCODER_H_

#include "SoftCodec.h"

int apus_encoder_open(SoftCodec *softCodec);
int apus_encoder_close(SoftCodec *softCodec);
int apus_encoder_process(SoftCodec *softCodec);

#endif
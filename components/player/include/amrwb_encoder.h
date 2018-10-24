// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _AMRWB_ENCODER_H_
#define _AMRWB_ENCODER_H_

#include "SoftCodec.h"

int amrwb_encoder_open(SoftCodec *softCodec);
int amrwb_encoder_close(SoftCodec *softCodec);
int amrwb_encoder_process(SoftCodec *softCodec);
int amrwb_encoder_trigger_stop(SoftCodec *softCodec);

#endif
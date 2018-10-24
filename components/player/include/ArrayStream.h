// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _ARRAY_STREAM_H_
#define _ARRAY_STREAM_H_

#include "AudioStream.h"

typedef enum {
    ARRAY_STREAM_WRITER = 0x00,
    ARRAY_STREAM_READER
} ArrayStreamType;

typedef struct
{
    /*relation*/
    AudioStream Based;
    ArrayStreamType type;
    int _isOpen; /*private*/
    int _run;
    char *_uri;
} ArrayStream;
ArrayStream *CreateArrayStreamReader(void);

#endif

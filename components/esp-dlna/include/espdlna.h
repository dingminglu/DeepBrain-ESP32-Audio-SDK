// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESPDLNA_H_
#define _ESPDLNA_H_

#include "renderer.h"
#include "upnp.h"
#include "ssdp.h"

typedef struct {
    renderer_t* renderer;
    upnp_t* upnp;
    ssdp_t* ssdp;
} dlna_t;

dlna_t* dlna_init(const char* name, const char* udn, const char* serial, int port, void* context, renderer_request cb);
void dlna_start();
void dlna_destroy(dlna_t* dlna);
#endif

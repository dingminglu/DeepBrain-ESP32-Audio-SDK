// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _UPNP_H_
#define _UPNP_H_
#include "httpd.h"

struct notify_t;
struct renderer_t;

typedef struct {
    struct notify_t *notify;
    struct renderer_t *renderer;
    char *friendly_name;
    char *udn;
    char *serial_number;
    int port;
    void *context;
    char *content_buffer;
    char *buffer[2];
} upnp_t;

typedef struct {
    const char *action;
    const char *service;
    int (*process)(HttpdConnData *connData, char *action);
    void *template;
} soap_action_t;

upnp_t *upnp_init(const char *name, const char *udn, const char *serial, int port, void *context);
void upnp_destroy(upnp_t *upnp);

#define UPNP_BUFFER_0 (2048)
#define UPNP_BUFFER_1 (1024)
#endif

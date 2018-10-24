// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _PLAYLIST_DEF_H_
#define _PLAYLIST_DEF_H_
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "playlistNameDef.h"

typedef struct UriList ListStruct;

typedef enum {
    PLAYLIST_PERMIT_WRITABLE,
    PLAYLIST_PERMIT_READ_ONLY,
} PlaylistPermission;

typedef struct PlaylistInfo {
    int isOpen;
    PlayListId id;
    PlaylistPermission permission;
    ListStruct *handle;
    xSemaphoreHandle _lock;
} PlaylistInfo;
#endif

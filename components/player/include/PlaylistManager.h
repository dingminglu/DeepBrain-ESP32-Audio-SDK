// Copyright 2018 Espressif Systems (Shanghai) PTE LTD 
// All rights reserved.

#ifndef _PLAYLIST_MANAGER_H_
#define _PLAYLIST_MANAGER_H_
#include "PlaylistDef.h"

struct AudioTrack;

struct PlaylistTracker;

struct PlaylistInfo;

struct PlaylistInfoNode;

struct MediaControl;

struct DeviceNotification;

typedef struct PlaylistManager PlaylistManager;

struct PlaylistManager {
    void *instance;
    int (*active) (PlaylistManager *manager);
    void (*deactive) (PlaylistManager *manager);
    int (*prev) (struct PlaylistTracker *tracker, int random, char **entryName);
    int (*next) (struct PlaylistTracker *tracker, int random, char **entryName);
    PlayListId (*getCurrentList) (struct PlaylistManager *manager);
    int (*setPlaylist) (PlaylistManager *manager, PlayListId listId, int modifyExisting);
    int (*save) (PlaylistManager *manager, PlayListId listId, const char *uri);
    int (*read) (PlaylistManager *manager, PlayListId listId, char *buf, int len, int offset, int isAttr);
    int (*close) (PlaylistManager *manager, PlayListId listId);
    int (*clearList) (PlaylistManager *manager, PlayListId listId);
    void (*closeAll) (PlaylistManager *manager);
    void (*deleteAll) (PlaylistManager *manager);
    int (*getInfo) (PlaylistManager *manager, PlayListId listId, int *amount, int *size);
    void (*load) (PlaylistManager *manager);
    void (*unload) (PlaylistManager *manager);
    void (*deviceEvtNotified) (struct DeviceNotification *evt);
    void (*_lock) (xSemaphoreHandle lock);
    void (*_unlock) (xSemaphoreHandle lock);
    int (*isCodecLib)(const char *ext);
    xSemaphoreHandle _manager_lock;

    /*private*/
    struct PlaylistTracker *_sd_tracker;
    struct PlaylistTracker *_flash_tracker;
    struct PlaylistTracker *_current_tracker;
    struct PlaylistInfoNode *_playlists;
    struct AudioTrack *_container;
};

PlaylistManager *PlaylistManagerCreate(struct MediaControl *ctrl);
void PlaylistManagerDestroy(void);

#endif

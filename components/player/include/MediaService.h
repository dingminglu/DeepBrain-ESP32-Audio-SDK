// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MEDIA_SERVICE_
#define _MEDIA_SERVICE_
#include "AudioDef.h"

typedef enum {
    PLAYER_CHANGE_STATUS = 0,
    PLAYER_ENCODE_DATA,
    PLAYER_DECODE_DATA,
    MEDIA_WIFI_SMART_CONFIG_REQUEST,
    MEDIA_WIFI_SMART_CONFIG_STOP,
    MEDIA_WIFI_BLE_CONFIG_REQUEST,
    MEDIA_WIFI_BLE_CONFIG_STOP,
} ServiceEventType;

typedef struct {
    void* instance;
    ServiceEventType type;
    void* data;
    int len;
} ServiceEvent;

//TODO: Add follow action to service
//Play
//Stop
//Pause
//SetupStream (addUri, setUri, clearUri)
//SetPosition(uri/index, bytes position)
//GetPlayerState => Playing, Stopped, Pausing, Transit
//GetTimePosition => Time in sec
//GetTimeTotal => Total time in sec
//GetBytesPosition => Current bytes position
//GetBytesTotal => Total bytes
//GetVolume
//SetVolume
//GetMute
//SetMute

struct AudioTrack;

struct PlaylistInfo;

struct DeviceNotification;

typedef struct MediaService MediaService;

struct MediaService { //extern from TreeUtility
    /*relation*/
    struct MediaService* next;
    struct MediaService* prev;
    void* instance; //ref instance
    ServiceEvent* evt;

    // struct AudioTrack *playlist;
    void (*addListener)(struct MediaService* serv, void* handle); //Add queue to receive player status channge
    void (*playerStatusUpdated)(ServiceEvent* evt); //call this function when player status was updated/change
    void (*deviceEvtNotified)(struct DeviceNotification* evt);
    void (*mediaPlay)(struct MediaService* serv); //1
    void (*mediaStop)(struct MediaService* serv); // 2
    void (*mediaPause)(struct MediaService* serv);
    void (*mediaResume)(struct MediaService* serv);
    void (*mediaNext)(struct MediaService* serv);
    void (*mediaPrev)(struct MediaService* serv);
    void (*volUp)(struct MediaService* serv);
    void (*volDown)(struct MediaService* serv);
    void (*mute)(struct MediaService* serv);

    int (*getPlayerStatus)(struct MediaService* serv, PlayerStatus* player);
    int (*getSongInfo)(struct MediaService* serv, struct MusicInfo* info); // totaltime, totalByte,name
    int (*getPosByTime)(struct MediaService* serv); // milliseconds
    int (*seekByTime)(struct MediaService* serv, int time);  // seconds
    int (*playTone)(struct MediaService* serv, const char* uri, enum TerminationType type);
    int (*stopTone)(struct MediaService* service);
    int (*upsamplingSetup)(int sampleRate);//aim sample rate

    struct AudioTrack* (*addUri)(struct MediaService* serv, const char* uri); //file:///root/path/to/song.mp3, http://song.mp3
    // int (*setTrack)(struct AudioTrack *playlist, struct AudioTrack *oneTrack);
    // void (*clearTrack)(struct AudioTrack *playlist);
    void (*serviceActive)(struct MediaService* serv);
    void (*serviceDeactive)(struct MediaService* serv);
    int (*setVolume)(struct MediaService* serv, int relative, int volume);
    int (*getVolume)(struct MediaService* serv);
    int (*setPlayMode)(struct MediaService* serv, PlayMode mode);
    int (*getPlayMode)(struct MediaService* serv,  PlayMode* mode);

    int (*getCurrentList)(void);
    int (*setPlaylist)(int listId, int modifyExisting);
    int (*saveToPlaylist)(int listId, const char* uri);
    int (*readPlaylist)(int listId, char* buf, int len, int offset, int isAttr);
    int (*closePlaylist)(int listId);
    int (*clearPlaylist)(int listId);

    int (*rawStart)(struct MediaService* service, const char* uri); // Support URI_RAW* definition
    int (*rawStop)(struct MediaService* service, enum TerminationType type);
    int (*rawRead)(struct MediaService* service, void* data, int bufSize, int* outSize);
    int (*rawWrite)(struct MediaService* service, void* data, int bufSize, int* writtenSize);

    int (*onServiceRequest)(ServiceEvent* evt);
    int (*sendEvent)(void* codec, ServiceEventType type, void* data, int len);
    int _blocking;
};

void MediaServicesActive(MediaService* service);
void MediaServicesDeactive(MediaService* service);
void MediaServicesNotify(MediaService* service);
void MediaServicesNotifyDecoded(MediaService* service, void* data, int len);
#endif

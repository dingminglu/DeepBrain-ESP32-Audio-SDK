// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_CODEC_
#define _AUDIO_CODEC_
#include "AudioDef.h"

typedef enum {
    CODEC_EVT_READ = 1,
    CODEC_EVT_WRITE = 2,
    CODEC_EVT_STARTING = 3,
    CODEC_EVT_STOPPING = 4,
    CODEC_EVT_PAUSING = 5,
    CODEC_EVT_REQ_SETUP = 6, /*request setup from codec*/
    CODEC_EVT_ERROR = 7,
} CodecEventType;

typedef enum {
    CODEC_MP3 = 0x00,
    CODEC_AAC,
    CODEC_OGG,
    CODEC_FLAC,
    CODEC_OPUS,
    CODEC_WAV,
    CODEC_PCM,
    CODEC_M3U8
} AudioCoding;

typedef struct {
    int type;
    void *instance;
    int len;
    void *data;
} CodecEvent;

typedef struct {

} MediaInfo;

typedef struct {

} PositionInfo;

typedef struct {
    char *data;
    int size;
} CodecReadMsg;

typedef struct CodecLib {
    struct CodecLib *next;
    struct CodecLib *prev;
    void *instance;
    // AudioCoding codingType;
    char *codecType;
    int taskHeapSize;
    int (*codecOpen)(void *codec);
    int (*codecClose)(void *codec);
    int (*codecProcess)(void *codec);
    int (*triggerStop)(void *codec);
    int (*codecGetPos)(void *codec, int timePos);
} CodecLib;

typedef struct AudioCodec { //extern from TreeUtility
    /*relation*/
    struct AudioCodec *next;
    struct AudioCodec *prev;
    void *instance;
    /*coding*/
    CodecLib *coding;
    CodecLib *currentCoding;
    int (*addCodec)(void *codec, int heap, const char *type, void *open, void *process, void *close, void *triggerStop);
    int (*setCodec)(void *codec, const char *type);
    void *(*getCodec)(void *codec);
    int (*clearCodec)(void *codec);
    /*codec control*/
    AudioCodecType codecType; /* encoder or decoder */
    char *tag; /* for identify codec name: soft, apxx, ... */
    int bitrate; /* codec bitrate */
    int skipData; /* bytes to skip */
    float duration; /* codec bitrate */
    int (*codecActive)(void *codec);
    int (*codecDeactive)(void *codec);
    int (*setCoding)(void *codec, const char *extension); /* setDecoder("soft/apxx/...") */
    int (*setEncoder)(void *codec, const char *extension);  /* setEncoder("soft/apxx/...") */
    int (*setConfig)(void *codec, void *config);
    int (*start)(void *codec);
    int (*pause)(void *codec);
    int (*resume)(void *codec);
    int (*stop)(void *codec);
    int (*volUp)(void *codec, int val);
    int (*volDown)(void *codec, int val);
    int (*mute)(void *codec);
    int (*seek)(void *codec, int pos);
    int (*getMediaInfo)(void *codec, void *info);

    int (*getCodecPosition)(void *codec);
    void (*setCodecPosition)(void *codec, int info);
    void (*getCodecMetadata)(void *codec, int *buffer);
    void (*setCodecMetadata)(void *codec, int *metadata);
    void (*softCodecDestroy)(void *codec);
    int (*getPos)(void *codec, int timePos);
    void (*setPause)(void *codec, int isPause);
    /*service control*/
    int (*getStatus)(void);
    void (*run)(void);
    void (*close)(void);
    /*callback*/
    int (*onCodecRequest)(CodecEvent *evt);
    int (*sendEvent)(void *codec, int type, void *data, int len);
    int (*codecInput)(void *audio_codec, void *data, int len);
    int (*codecOutput)(void *audio_codec, void *data, int len);
} AudioCodec;


void AudioCodecActive(AudioCodec *codec);
AudioCodec *AudioCodecSet(AudioCodec *codec, const char *tag);
AudioCodec *AudioCodecGet(AudioCodec *codec);
void AudioCodecDestroy(AudioCodec *codec);

#endif

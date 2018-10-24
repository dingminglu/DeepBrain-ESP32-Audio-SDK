// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MEDIA_CONTROL_H_
#define _MEDIA_CONTROL_H_
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "AudioDef.h"
#include "esp_audio_log.h"

#define MEDIACTRL_ERROR  (-1)
#define MEDIACTRL_OK     (0)
#define MEDIACTRL_KEEP    (1)

typedef enum MediaState {
    // MEDIACONTROL_TRANSIT                  = 0,
    MEDIACONTROL_PLAYING                  = 1,
    MEDIACONTROL_PAUSE                      = 2,
    MEDIACONTROL_STOP                        = 3,
    MEDIACONTROL_STOPPING               = 4,
    MEDIACONTROL_FINISHING               = 5,
} MediaState;  /* MediaControl state */

enum TerminationType;

struct MediaService;

struct AudioStream;

struct AudioCodec;

struct AudioTrack;

struct RingBuf;

struct DeviceController;

struct TonePlayer;

struct PlaylistManager;

struct BroadcastObj;

typedef enum MediaCtrlEvt {
    MEDIA_CTRL_EVT_PLAY              = 0,
    MEDIA_CTRL_EVT_STOP             = 1,
    MEDIA_CTRL_EVT_PAUSE           = 2,
    MEDIA_CTRL_EVT_RESUME        = 3,
    MEDIA_CTRL_EVT_SEEK              = 4,
    MEDIA_CTRL_EVT_NEXT             = 5,
    MEDIA_CTRL_EVT_PREV             = 6,
    MEDIA_CTRL_EVT_FINISH          = 7,
    MEDIA_CTRL_EVT_EXIT              = 8,
} MediaCtrlEvt;  /* MediaControl Event */

typedef struct PlayerEvt {
    MediaCtrlEvt evt;
    void *msg;
} PlayerEvt;

typedef struct PlayerSetupInfo {
    char *uri;
    int pos;
    PlayerWorkingMode mode; // enum PlayerWorkingMode
} PlayerSetupInfo;

typedef struct {
    PlayerStatus player;  // back up player status
    struct AudioTrack *track;
    int codecPos; // BytePosition
    int timePos; // millisecond
    int skipSize;//some audio format contains useless data to be skipped, this may be used in EspAudioTimeGet() so need to be recorded.
    int metadata[5]; /* sample rate, nchannels, bit rate, pointer , length*/
} AudioInfo;

typedef struct MediaControl MediaControl;

struct MediaControl {
    struct MediaService *services;
    struct MediaService *currentService;
    MediaState workingState;
    PlayMode mode;
    char extension[MAX_EXTENSION_LENGTH]; //default music extension
    AudioInfo audioInfo; // For backup
    PlayerStatus curPlayer;  // Current player status
    int autoResume;

    struct TonePlayer *tonePlayer;
    void (*addTonePlayer) (struct MediaControl *ctrl, struct TonePlayer *indicator);

    struct RingBuf *inputRb;
    struct RingBuf *outputRb;
    struct RingBuf *processRb;
    xSemaphoreHandle ctrlLock;
    void (*lock)(xSemaphoreHandle lock);
    void (*unlock)(xSemaphoreHandle lock);
    void (*addService)(struct MediaControl *ctrl, struct MediaService *service);

    struct DeviceController *deviceController;
    int (*queryCodecLib)(struct MediaControl *ctrl, AudioCodecType type, const char *extension);

    struct AudioCodec *decoder;
    struct AudioCodec *currentDecoder;
    struct RingBuf *decoderInputBuffer;
    void (*addDecoder)(struct MediaControl *ctrl, struct AudioCodec *decoder);
    struct AudioCodec *(*setDecoder)(struct AudioCodec *codec, const char *tag);
    struct AudioCodec *(*getDecoder)(struct AudioCodec *codec);
    int (*addDecoderCodecLib)(void *codec, int heap, const char *type, void *open, void *process, void *close, void *triggerStop, void *getPos);
    int (*setDecoderInputBuffer)(struct MediaControl *ctrl, struct RingBuf *buffer);

    int (*decoderRequestSetup)(struct MediaControl *ctrl, void *data);

    struct AudioCodec *encoder;
    struct AudioCodec *currentEncoder;
    struct RingBuf *encoderOutputBuffer;
    void (*addEncoder)(struct MediaControl *ctrl, struct AudioCodec *encoder);
    struct AudioCodec *(*setEncoder)(struct AudioCodec *codec, const char *tag);
    struct AudioCodec *(*getEncoder)(struct AudioCodec *codec);
    int (*setEncoderOutputBuffer)(struct MediaControl *ctrl, struct RingBuf *buffer);
    int (*addEncoderCodecLib)(void *codec, int heap, const char *type, void *open, void *process, void *close, void *triggerStop);
    int (*encoderRequestSetup)(struct MediaControl *ctrl, void *data);

    struct AudioStream *inputStream;
    struct AudioStream *currentInputStream;
    void (*addInputStream)(struct MediaControl *ctrl, struct AudioStream *stream);
    struct AudioStream *(*getInputStream)(struct AudioStream *stream);
    struct AudioStream *(*setInputStream)(struct AudioStream *stream, const char *scheme);

    struct AudioStream *outputStream;
    struct AudioStream *currentOutputStream;
    void (*addOutputStream)(struct MediaControl *ctrl, struct AudioStream *stream);
    struct AudioStream *(*setOutputStream)(struct AudioStream *stream, const char *scheme);
    struct AudioStream *(*getOutputStream)(struct AudioStream *stream);
    struct AudioStream *(*getI2sStream)(struct AudioStream *stream);


    struct AudioTrack *workModeList;
    struct AudioTrack *currentTrack;
    int (*setTrack)(struct AudioTrack *playlist, struct AudioTrack *oneTrack);
    struct AudioTrack *(*getTrack)(struct AudioTrack *playlist);
    struct AudioTrack *(*nextTrack)(struct AudioTrack *playlist);
    struct AudioTrack *(*prevTrack)(struct AudioTrack *playlist);
    void (*removeTrack)(struct AudioTrack *playlist, struct AudioTrack *track);
    void (*clearTrack)(struct AudioTrack *playlist);

    struct PlaylistManager *playlistManager;
    int (*getCurrentList)(void);
    int (*setPlaylist)(int listId, int modifyExisting);
    int (*saveToPlaylist)(int listId, const char *uri);
    int (*closePlaylist)(int listId);
    int (*clearPlaylist) (int listId);

    int (*streamToBuffer)(struct MediaControl *ctrl, void *data, int len);
    int (*bufferToDecoder)(struct MediaControl *ctrl, void *data, int len);
    int (*decoderToBuffer)(struct MediaControl *ctrl, void *data, int len);
    int (*bufferToEncoder)(struct MediaControl *ctrl, void *data, int len);
    int (*encoderToBuffer)(struct MediaControl *ctrl, void *data, int len);
    int (*bufferToStream)(struct MediaControl *ctrl, void *data, int len, int waitTicks);

    int (*clearOutputData)(struct MediaControl *ctrl);
    int (*resetBuffer)(struct MediaControl *ctrl);

    void (*active)(struct MediaControl *ctrl);
    void (*deactive)(struct MediaControl *ctrl);
    void (*activeServices)(struct MediaControl *ctrl);
    void (*deactiveServices)(struct MediaControl *ctrl);
    void (*requestState)(struct MediaControl *ctrl, enum MediaState newState, void *msg, int dir);

    /***
     * friend member function for DeviceController or PlaylistManager
     * (should only be used by DeviceController or PlaylistManager)
     */
    //XXX: do not use these if have a better idea decoupling MediaControl and DeviceController
    int (*_pause)(struct MediaControl *ctrl);
    int (*_resume)(struct MediaControl *ctrl);

    struct BroadcastObj *_broadcast;
    MusicInfo _musicInfo;
    PlaySrc _newSrc;
    void *_taskhandle;
    void *_ctrlEvtQue;
    void *_ctrlSyncQue;
};

volatile struct MediaControl *CreateMediaPlayer(const struct PlayerBufCfg *cfg);

#endif

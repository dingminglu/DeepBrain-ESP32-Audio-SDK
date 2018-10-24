// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_DEF_H_
#define _AUDIO_DEF_H_

#define MAX_EXTENSION_LENGTH            6

typedef enum AudioErr {
    /* System error */
    AUDIO_ERR_NO_ERROR = 0,
    AUDIO_ERR_NULL_ERROR = 1 ,
    AUDIO_ERR_MEMORY_LACK = 2,
    AUDIO_ERR_NOT_READY = 3,
    AUDIO_ERR_INVALID_PARAMETER = 4,
    AUDIO_ERR_CODEC = 5,
    AUDIO_ERR_NOT_SUPPORT = 6,
    AUDIO_ERR_TIMEOUT = 7,

    /* Stream error */
    AUDIO_ERR_NO_STREAM = 0x100,

    /* Playlist error */
    AUDIO_ERR_PLAYLIST_NOT_AVAILABLE = 0x200,

    /* FafsStream error */
    AUDIO_ERR_STREAM_FATFS_PATH_INVALID = 0x10000,
    AUDIO_ERR_STREAM_FATFS_FILE_OPEN_FAILED = 0x10001,
    AUDIO_ERR_STREAM_FATFS_FILE_SEEK_FAILED = 0x10002,
    AUDIO_ERR_STREAM_FATFS_FILE_READ_ERROR = 0x10003,
    AUDIO_ERR_STREAM_FATFS_FILE_WRITE_ERROR = 0x10004,

    /* I2sStream error */
    AUDIO_ERR_STREAM_I2S_READ_ERROR = 0x11000,
    AUDIO_ERR_STREAM_I2S_WRITE_ERROR = 0x11001,

    /* HttpStream request error */
    AUDIO_ERR_STREAM_HTTP_GET_HOST_IP_FAILED = 0x12000,
    AUDIO_ERR_STREAM_HTTP_SOCKET_CREATE_FAILD = 0x12001,
    AUDIO_ERR_STREAM_HTTP_CONNECTION_TIMEOUT = 0x12002,
    AUDIO_ERR_STREAM_HTTP_REQUEST_SEND_FAILED = 0x12003,
    AUDIO_ERR_STREAM_HTTP_GET_CONTENT_LENGTH_FAILED = 0x12004,
    AUDIO_ERR_STREAM_HTTP_CLIENT_READ_ERROR = 0x12005,
    AUDIO_ERR_STREAM_HTTP_UNHANDLED_REPOSONSE = 0x12006,
    AUDIO_ERR_STREAM_HTTP_CLIENT_ERROR = 0x12007,
    AUDIO_ERR_STREAM_HTTP_SERVER_ERROR = 0x12008,

    /* Livestream error */
    AUDIO_ERR_STREAM_LIVE_UNSUPPORTED_INPUT = 0x13000,
    AUDIO_ERR_STREAM_LIVE_UNPARSABLE_M3U8 = 0x13001,

    /* FlashToneStream error */
    AUDIO_ERR_STREAM_FLASH_TONE_LOST_HEADER = 0x14000,
    AUDIO_ERR_STREAM_FLASH_TONE_OUT_RANGE = 0x14001,
    AUDIO_ERR_STREAM_FLASH_TONE_READ_FAILED = 0x14002,

    /* FlashToneStream error */
    AUDIO_ERR_STREAM_ARRAY_IS_NULL = 0x15000,
    AUDIO_ERR_STREAM_ARRAY_NO_MUSIC_AVAILABLE = 0x15001,

    /* softcodec error */
    AUDIO_ERR_SOFTCODEC_INTI_ERROR = 0x30000,
    AUDIO_ERR_SOFTCODEC_ENCODE_DECODE_ERROR = 0x30001,

    /* mediacontrol error */
    AUDIO_ERR_PLAYER_SET_UP_MEDIACTRL_FAILED = 0x40000,  //set up player failed (missing some components or invalid url)
    AUDIO_ERR_PLAYER_NO_AUDIO_AVAILABLE = 0x40001,
    AUDIO_ERR_PLAYER_TRACK_INVALID = 0x40002,

} AudioErr;

typedef enum PlayMode {
    MEDIA_PLAY_SEQUENTIAL = 0,
    MEDIA_PLAY_SHUFFLE,
    MEDIA_PLAY_REPEAT,
    MEDIA_PLAY_ONE_SONG,
} PlayMode;

typedef enum PlayerWorkingMode {
    PLAYER_WORKING_MODE_NONE,
    PLAYER_WORKING_MODE_MUSIC,
    PLAYER_WORKING_MODE_TONE,
    PLAYER_WORKING_MODE_RAW,
} PlayerWorkingMode;

typedef enum PlaySrc {
    MEDIA_SRC_NULL          = 0,
    MEDIA_SRC_BT            = 1,
    MEDIA_SRC_HTTP          = 2,
    MEDIA_SRC_DLNA          = 3,
    MEDIA_SRC_AIRPLAY       = 4,
    MEDIA_SRC_RAW           = 5,
    MEDIA_SRC_SD            = 6,
    MEDIA_SRC_FLASH         = 7,

    MEDIA_SRC_RESERVE1      = 10,
    MEDIA_SRC_RESERVE2      = 11,
    MEDIA_SRC_RESERVE3      = 12,
    MEDIA_SRC_RESERVE4      = 13,
    MEDIA_SRC_RESERVE5      = 14,
    MEDIA_SRC_RESERVE6      = 15,
    MEDIA_SRC_RESERVE7      = 16,
    MEDIA_SRC_RESERVE8      = 17,
    MEDIA_SRC_MAX,
} PlaySrc;

/* Warning:
       do not change the values of these AudioStatus enum*/
typedef enum AudioStatus {
    AUDIO_STATUS_UNKNOWN = 0,
    AUDIO_STATUS_PLAYING = 1,
    AUDIO_STATUS_PAUSED = 2,
    AUDIO_STATUS_FINISHED = 3,
    AUDIO_STATUS_STOP = 4,
    AUDIO_STATUS_ERROR = 5,
    AUDIO_STATUS_AUX_IN = 6,
} AudioStatus;  /* Effective audio playing status */

typedef struct PlayerStatus {
    AudioStatus status;
    AudioErr errMsg;
    PlayerWorkingMode mode;
    PlaySrc musicSrc;//the source of the music
} PlayerStatus;

typedef struct MusicInfo {
    char *name;
    int totalTime;        // millisec
    int totalBytes;       // Byte
    int bitRates;         // bps
    int sampleRates;      // Hz
    int channels;
    int bits;
    char *data; //other data like id3 frame in mp3 music
    int dataLen;//data length in bytes
} MusicInfo;

typedef enum {
    CODEC_ENCODER = 0x00,
    CODEC_DECODER
} AudioCodecType;

typedef enum TerminationType {
    TERMINATION_TYPE_NOW = 0,  //terminate the music immediately
    TERMINATION_TYPE_DONE = 1, //the current music will not be terminated untill it is finished
    TERMINATION_TYPE_MAX,
} TerminationType;

typedef struct PlayerBufCfg {
    int inputBufferLength;
    int outputBufferLength;
    int processBufferLength;
} PlayerBufCfg;

#endif

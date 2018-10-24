// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ESP_CODEC_CONFIG_H_
#define _ESP_CODEC_CONFIG_H_

#include "userconfig.h"


#ifdef ESP_AUTO_PLAY
#define USE_LIB_WAV
#define USE_LIB_AMR
#define USE_LIB_FLAC
#define USE_LIB_VORBIS
#define USE_LIB_OPUS
//#define USE_LIB_FDKAAC
#define USE_LIB_AAC
//#define USE_LIB_FLUENDO_MP3
//#define USE_LIB_HELIX_MP3
//#define USE_LIB_MAD_MP3
#define USE_LIB_MP3

///following macro define is used to check the lib use case conflict
///at least one lib should be use
///at most 1 aac lib can be used
///at most 1 mp3 lib can be used
#ifdef USE_LIB_WAV
    #define LIB_WAV_ON 1
#else
    #define LIB_WAV_ON 0
#endif
#ifdef USE_LIB_AMR
    #define LIB_AMR_ON 1
#else
    #define LIB_AMR_ON 0
#endif
#ifdef USE_LIB_FLAC
    #define LIB_FLAC_ON 1
#else
    #define LIB_FLAC_ON 0
#endif
#ifdef USE_LIB_VORBIS
    #define LIB_VORBIS_ON 1
#else
    #define LIB_VORBIS_ON 0
#endif
#ifdef USE_LIB_OPUS
    #define LIB_OPUS_ON 1
#else
    #define LIB_OPUS_ON 0
#endif
#ifdef USE_LIB_FDKAAC
    #define LIB_FDKAAC_ON 1
#else
    #define LIB_FDKAAC_ON 0
#endif
#ifdef USE_LIB_AAC
    #define LIB_GAAC_ON 1
#else
    #define LIB_GAAC_ON 0
#endif
#ifdef USE_LIB_FLUENDO_MP3
    #define LIB_FLUENDO_MP3_ON 1
#else
    #define LIB_FLUENDO_MP3_ON 0
#endif
#ifdef USE_LIB_HELIX_MP3
    #define LIB_HELIX_MP3_ON 1
#else
    #define LIB_HELIX_MP3_ON 0
#endif
#ifdef USE_LIB_MAD_MP3
    #define LIB_MAD_MP3_ON 1
#else
    #define LIB_MAD_MP3_ON 0
#endif
#ifdef USE_LIB_MP3
    #define LIB_SMP3_ON 1
#else
    #define LIB_SMP3_ON 0
#endif
#define LIB_AAC_ON (LIB_FDKAAC_ON + LIB_GAAC_ON)
#define LIB_MP3_ON (LIB_FLUENDO_MP3_ON + LIB_HELIX_MP3_ON + LIB_MAD_MP3_ON + LIB_SMP3_ON)
#define LIB_ALL_ON (LIB_WAV_ON + LIB_AMR_ON + LIB_FLAC_ON +LIB_VORBIS_ON + LIB_OPUS_ON + LIB_FDKAAC_ON + LIB_GAAC_ON + LIB_FLUENDO_MP3_ON + LIB_HELIX_MP3_ON + LIB_MAD_MP3_ON + LIB_SMP3_ON)

#ifdef ESP_AUTO_PLAY
#if (LIB_AAC_ON > 1)
    #error at most 1 aac lib can be used
#endif
#if (LIB_MP3_ON > 1)
    #error at most 1 mp3 lib can be used
#endif
#if (LIB_ALL_ON == 0)
    #error at least one lib should be used
#endif
#endif

#endif ///#ifdef ESP_AUTO_PLAY

#endif

#ifndef _STAGEFRIGHT_MP3_CODEC_H_
#define _STAGEFRIGHT_MP3_CODEC_H_
#include "SoftCodec.h"
#include "pvmp3decoder_api.h"
#include "mp3reader.h"

#define STAGEFRIGHT_MP3_BUFFER_SZ (2106)

#ifdef FORCE_DEC_MONO_CHANNEL
#define STAGEFRIGHT_MP3_SAMPLE_SIZE (2304)
#else
#define STAGEFRIGHT_MP3_SAMPLE_SIZE (4608)
#endif

typedef struct {
    int _run;

    int id3_checked;
    mp3_buffer mp3buf;
    mp3_callbacks *mp3data;
    uint32_t mFixedHeader;

    int consumed_bytes;

    void *decoderBuf;
    uint8_t *inputBuf;
    int16_t *outputBuf;

    tPVMP3DecoderExternal config;

    int framecnt;
    int pcmcnt;

    int _sample_rate;
    int _channels;

} StageFrightMP3Codec;

int StageFrightMP3Open(SoftCodec *softCodec);
int StageFrightMP3Close(SoftCodec *softCodec);
int StageFrightMP3Process(SoftCodec *softCodec);
int StageFrightMP3TriggerStop(SoftCodec *softCodec);
#endif


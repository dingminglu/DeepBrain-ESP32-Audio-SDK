// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MP3_READER_H_
#define _MP3_READER_H_

#include <stdint.h>

typedef struct {
    int buffer_size;
    unsigned char *buffer;
    int at_eof;
    int totalbytesread;
    int bytes_into_buffer;
    int bytes_consumed;
} mp3_buffer_t;

typedef struct {
    int(*fill)(void *user_data, void *b);
    void(*advance)(void *b, int bytes);
    //int(*skip)(void* user_data, unsigned int length);
    //void(*set)(void* user_data, unsigned int position);
    void *codec_data;
} mp3_callbacks_t;

// Mask to extract the version, layer, sampling rate parts of the MP3 header,
// which should be same for all MP3 frames.
#define MP3_HEADER_MASK (0xfffe0c00)

int Mp3ReadInit(int *id3_checked, mp3_callbacks_t *streamfunc, void *b, uint32_t *mFixedHeader);
int Mp3GetFrame(mp3_callbacks_t *streamfunc, void *b,  uint32_t *size, uint32_t mFixedHeader, int *id3_checked);
void mp3_buffer_advance(void *user_data, int bytes);
int mp3_fill_buffer(void *codec_data, void *user_data);
void mp3_buf_init(void *user_data, uint8_t *buffer, int bufsize);
int id3_header_size(unsigned char *header);
int id3_header_skip(void *codec_data, unsigned char *in_buf, int bufsize, int *bytes_into_buffer, int *totalbytesread);
uint32_t U32_AT_MP3(const uint8_t *ptr) ;
int parseHeader_mp3(
    uint32_t header, size_t *frame_size,
    uint32_t *out_sampling_rate, uint32_t *out_channels,
    uint32_t *out_bitrate, uint32_t *out_num_samples);

#endif
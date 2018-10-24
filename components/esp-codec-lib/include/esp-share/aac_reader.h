// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AAC_READER_H_
#define _AAC_READER_H_
#include <stdint.h>

typedef struct {
    unsigned char *buf; //the ts buffer for input and output
    int buf_size; ///the tsbuf size, currently is 188 because only support 188 packet ts stream
    int buf_num; /////data in ts_buf
    void *ts_anlysis;  ///the ts analysis main process structure
    void *ts_dec_data; ///the output info structure
    void *m_param;  ///the parameter info
} ts_buf_t;

typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    long buffer_size;
    unsigned char *buffer;
    int at_eof;
    ////read byte from file. Attention, for both raw aac or TS aac, should be always the real used bytes. Not de-wrapped data for TS case 
    ////so for raw AAC case, we can get correct AAC position. But TS case, it will be not so correct, just close to the correct position
    int totalbytesread;
    void *ts_buf; //if defined means this is ts stream, should first do ts de-wrapper. Currently only valid for aac
} aac_buffer_t;

typedef struct {
    int(*fill)(void *codec_data, void *user_data);
    void(*advance)(void *user_data, int bytes);
    //int(*skip)(void* user_data, unsigned int length);
    //void(*set)(void* user_data, unsigned int position);
    void *codec_data;
} aac_callbacks_t;

// Mask to extract the version, layer, sampling rate parts of the AAC header,
// which should be same for all AAC frames.
#define AAC_HEADER_MASK (0xFFFFFFF0)

int AacReadInit(aac_callbacks_t *streamfunc, void *b, uint32_t *mFixedHeader);
int AacGetFrame(aac_callbacks_t *streamfunc, void *b,  uint32_t *size, uint32_t mFixedHeader);
int parseHeader_aac(uint32_t header, uint32_t *out_sampling_rate, uint32_t *out_channels);
uint32_t get_aac_frame_size(const uint8_t *buf);
uint32_t U32_AT_AAC(const uint8_t *ptr) ;
int read_mp4_stream(void *mp4_codec, void *user_data, void *buffer, unsigned int length);
int skip_mp4_stream(void *mp4_codec, void *user_data, unsigned int length);

int prefilled_data_ts_dewrapper(void *aac_codec, unsigned char *data, int bufsize, int datanum, void *ts_buf);

void aac_buffer_init(void *user_data, uint8_t *buffer, int size);
void aac_buffer_advance(void *user_data, int bytes);
int aac_fill_buffer(void *codec_data, void *user_data);

int ts_aacdec_init(void *user_data);
void ts_aacdec_close(void *user_data);
int ts_aacstream_search(unsigned char *buf, int len);
void set_ts_pmtpid(void *user_data, int pid);
void set_ts_audiopid(void *user_data, int pid);
int get_ts_pmtpid(void *user_data);
int get_ts_audiopid(void *user_data);

#endif
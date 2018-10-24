// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _MP4_PARSER_H_
#define _MP4_PARSER_H_

#define MAX_TRACKS (16)
//#define MP4_PARSE_ENABLE_PRINT

#define MP4_PARSE_ERROR 0
#define MP4_PARSE_OK 1
#define MP4_INFO_ERROR 0
#define MP4_INFO_OK 1

///box item
#define BOX_MOOV 0
#define BOX_TRAK 1
#define BOX_MDIA 2
#define BOX_MINF 3
#define BOX_STBL 4
#define BOX_SUBBOX (64)
#define BOX_FTYP (BOX_SUBBOX + 1)
#define BOX_MDAT (BOX_SUBBOX + 2)
#define BOX_MVHD (BOX_SUBBOX + 3)
#define BOX_STSZ (BOX_SUBBOX + 4)
#define BOX_STSC (BOX_SUBBOX + 5)
#define BOX_STCO (BOX_SUBBOX + 6)
#define BOX_STSD (BOX_SUBBOX + 7)
#define BOX_ESDS (BOX_SUBBOX + 8)
#define BOX_MP4A (BOX_SUBBOX + 9)
#define BOX_UNKNOW 255

typedef struct {
    int(*read)(void *mp4_codec, void *user_data, void *buffer, unsigned int length);
    int(*skip)(void *mp4_codec, void *user_data, unsigned int length);
    void *user_data;
    void *mp4_codec;
} mp4_callbacks_t;

typedef struct {
    int *sample_size_tab;
    int sample_num;
} info_stsz_t;

typedef struct {
    int *chunk_offset_tab;
    int chunk_num;
} info_stco_t;

typedef struct {
    int first_trunk;
    int samples_per_trunk;
    int sample_description_id;
} stsc_t;

typedef struct {
    stsc_t *stsc_tab;
    int stsc_num;
} info_stsc_t;

typedef struct {
    info_stsz_t stsz;
    info_stco_t stco;
    info_stsc_t stsc;

    unsigned char *asc_buf; // AudioSpecificConfig
    int asc_len;

    int *sample_offset;
    int *sample_size;
    int sample_num;
} track_info_t;

typedef struct {
    mp4_callbacks_t *stream;

    int moov_read;
    int moov_ahead;
    int moov_offset;
    int moov_size;
    unsigned long long current_position;

    ///use this two value can get the audio duration
    int time_scale;
    int duration;

    int total_tracks;
    track_info_t *track[MAX_TRACKS];
} mp4_info_t;

///int type
#define SEEK_TAB_SIZE (1024 * 2)  ///byte unit
#define SAMPLE_SKIPBUF_MAXSIZE (1024 / 4) ///byte unit
typedef struct {
    int freq_index;
    int channel;
    int object_type;
    int sample_rate;

    ///for seek when playing
    int *sample_bit_skip; ///the sample skip bits indicate by sample_idx_need_bit_skip
    int *sample_idx_need_bit_skip; ///the sample index which has offset, need to skip extra bits. Here not direct use the offset because we don't need to flush the buffer
    int bit_skip_buf_max_size; ////the maximum sample bit skip buffer, if exceed than this value, error
    int bit_skip_buf_size; ///at least one for first sample raw data offset skip
    int num_samples; ////all sample number
    int *seek_table;
    int track_num; ///the track number of the mp4 stream
    int audio_track_idx; ///the audio track index
    int seek_table_size;
    int audio_duration;

    int configbuf_size;
    char *config_buf;

} audio_info_t;

void mp4close(mp4_info_t *mp4_info);
int get_mp4_info(audio_info_t *audio_info, mp4_info_t *mp4_info);
int mp4_get_decoder_info(audio_info_t *audio_info, mp4_info_t *mp4_info);
void audioinfo_close(audio_info_t *audio_info);
void m4a_audioinfo_copy(audio_info_t *tar, audio_info_t *src);

#endif

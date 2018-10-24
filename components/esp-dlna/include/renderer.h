// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _RENDERER_H_
#define _RENDERER_H_

#define TIME_NOT_IMPLEMENTED        2147483647
#define TIME_STRING_NOT_IMPLEMENTED "00:00:00"

typedef int (*renderer_request)(void *context, int type, void *data);

typedef struct {
    char *title;
    char *creator;
    char *artist;
    char *publisher;
    char *genre;
} track_meta_t;

typedef struct renderer_t {
    void *upnp;
    int (*request_rcs_change)(void *upnp, int);
    int (*request_avt_change)(void *upnp, int);
    track_meta_t *track_meta;
    char *track_uri;
    int track_no;
    int total_track;
    int track_rel_count;
    int track_abs_count;
    char *current_duration;
    int duration_interger;
    char *total_duration;
    char *track_rel_time;
    char *track_abs_time;
    int vol;
    int mute;
    int current_playmode;
    int transport_state;
    int second;
    renderer_request request_player;
    void *context;
    int run;
    int running;
} renderer_t;

typedef enum {
    STOPPED = 0x00,
    PAUSED_PLAYBACK,
    PLAYING,
    TRANSITIONING,
    NO_MEDIA_PRESENT
} playmode_t;

typedef enum {
    RENDERER_SET_URI = 0x01,
    RENDERER_PLAYMODE_PLAY,
    RENDERER_PLAYMODE_STOP,
    RENDERER_GET_CURRENT_POS,
    RENDERER_PLAYMODE_PAUSE,
    RENDERER_GET_TOTAL_DURATION,
    RENDERER_GETSET_VOLUME,
    RENDERER_SET_TIMEPOS,
    RENDERER_CHECK_PLAYING
} renderer_req_type_t;

int renderer_play(renderer_t *renderer, int speed);
int renderer_pause(renderer_t *renderer);
int renderer_stop(renderer_t *renderer);
int renderer_next(renderer_t *renderer);
int renderer_prev(renderer_t *renderer);
int renderer_mute(renderer_t *renderer, int value);
int renderer_seek(renderer_t *renderer, int time);
int renderer_vol(renderer_t *renderer, int value);
int renderer_setup_uri(renderer_t *renderer, const char *uri);
renderer_t *renderer_init();
void renderer_destroy(renderer_t *r);
const char *renderer_playmode(renderer_t *renderer, int mode);
const char *renderer_transport_state(renderer_t *renderer, int state);
void renderer_playing_notify(renderer_t *renderer, int track_no, int total_track, int total_duration);
void renderer_stop_notify(renderer_t *renderer);
void renderer_check_time_pos(renderer_t *r);
#endif

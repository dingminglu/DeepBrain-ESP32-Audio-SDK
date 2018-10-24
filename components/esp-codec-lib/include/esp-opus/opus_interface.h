#include "opus_types.h"
#include "opus_header.h"

void *opus_ogg_buffer_calloc_oy();

void *opus_ogg_buffer_calloc_og();

void *opus_ogg_buffer_calloc_op();

void *opus_ogg_buffer_calloc_os();

void opus_ogg_buffer_release_oy(void *ptr);

void opus_ogg_buffer_release_os(void *ptr);

void opus_ogg_buffer_release_op(void *ptr);

void opus_ogg_buffer_release_og(void *ptr);

unsigned int get_opus_og_last_head(void *og);

int get_opus_os_serialno(void *os);
  
int get_opus_os_pageno(void *os);

int get_opus_oy_fill(void *oy);

int get_opus_oy_returned(void *oy);

long get_opus_op_bos(void *op);

long get_opus_op_eos(void *op);

long get_opus_op_bytes(void *op);

unsigned char * get_opus_op_packet(void *op);

void set_opus_os_pageno(void *os, int pageno);

int opus_stream_packetout(void *os, void *op);

int opus_stream_pagein(void *os, void *og);

opus_int64 opus_page_granulepos(void *og);

int opus_page_serialno(void *og);

int opus_stream_init(void *os,int serialno);

int opus_sync_wrote(void *oy, long bytes);

char *opus_sync_buffer(void *oy, long size);

int opus_sync_pageout(void *oy, void *og);

int opus_sync_clear(void *oy);

int opus_stream_clear(void *os);

int opus_sync_init(void *oy);

int opus_stream_reset_serialno(void *os,int serialno);

int opus_process_header(void *op, opus_int32 *rate, int *channels, int *streams, OpusHeader *header);

#if 0
void *get_opus_os_lacing_vals(void *os);
void *get_opus_os_granule_vals(void *os);
void *get_opus_os_body_data(void *os);
#endif
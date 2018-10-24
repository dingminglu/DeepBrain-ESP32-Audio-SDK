/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _ESP_RESAMPLE_H_
#define _ESP_RESAMPLE_H_

////all positive value is valid, means the consumed in bytes
typedef enum {
    RESAMPLE_SUCCESS                 = 1, ///any positive value means success, 1 is just an example, should be the real consumed in pcm bytes
    RESAMPLE_FAIL                    = 0,  ///the resample process meet with error
    RESAMPLE_SRC_RATE_CHANGE         = -1,   ////the source sample rate changed
    RESAMPLE_SRC_CH_CHANGE           = -2,   ////the source channel changed
    RESAMPLE_DEST_RATE_CHANGE        = -3,   ////the destination sample rate changed
    RESAMPLE_DEST_CH_CHANGE          = -4,   ////the destination channel changed
    RESAMPLE_MODE_CHANGE             = -5,   ////the resample mode changed
    RESAMPLE_TYPE_CHANGE             = -6,   ////the resample type changed
    RESAMPLE_COMPLEXITY_CHANGE       = -7,   ////the fir filter length, 0~4, 0 means lowest accuracy and faster speed. And 4 is vice verser
    RESAMPLE_IN_DATA_OVER_LENGTH     = -8,  ///the input data size exceed set buf size when open
    RESAMPLE_SAMPLE_BITS_NOT_SUPPORT = -9,   ////the sample bits size not support, here should always 16
    RESAMPLE_IN_DATA_BUF_TOO_SMALL   = -10,    ///for encode mode code, the set output pcm number is too big, which case the input buffer size is not enough
    RESAMPLE_IN_DATA_NUM_ERROR       = -11,   /// the input pcm number should be multiply of channel number
    RESAMPLE_OUT_DATA_NUM_ERROR      = -12,   ////encode mode, output value setup error
    RESAMPLE_IN_BUF_ALIGNED_ERROR    = -13,   ////the in pcm buffer not bytes aligned
} ESP_RESP_ERR;

/// for decoder mode
typedef enum {
    RESAMPLE_DECODE_MODE = 0,  /// for decode mode, the input size will be constant, the output size is decided by input size and will be change, major used for audio decoding case
    RESAMPLE_ENCODE_MODE = 1,  /// for encode mode, the output size will be constant, you should prepare enough data in input buffer, the process will return the really used input data size.
} RESAMPLE_MODE;

typedef struct {
    int src_rate;           /// input pcm sample-rate
    int src_ch;             /// input pcm channel number
    int dest_rate;          /// output pcm sample-rate
    int dest_ch;            /// output pcm channel number
    int sample_bits;        /// pcm bit number, current only support 16 bits pcm
    int mode;               /// encode or decoder mode. decode mode, input pcm number is constant. encode mode, output pcm number is constant
    int max_indata_bytes;   /// this is used for buffer allocation, the maximum possible input pcm bytes number.
    int out_len_bytes;      /// this is used to setup encode mode output data size. The output pcm bytes number
    int type;               /// resample method type, -1 means autiomatic select type.decimate, resample or interplate.You can manual select the type for some interger time resampling.
    int complexity;         /// only valid for fir resample case, linear interploation not needed. Used to control the fir window length. 0~4. 0 is lowest accuracy but fastest speed and 4 vice versa
} resample_info_t;

///sampler-rate change for source sample-rate to target sample-rate,
///p_in: in pcm buffer ;
///p_out: out pcm buffer;
///info: control info
///in_data_size: pcm bytes number in p_in, real pcm number in in_pcm is in_data_size / 2 for short type or in_data_size / 4 for float type
///out_data_size: cm bytes number in p_out, real pcm number in in_pcm is in_data_size / 2 for short type or in_data_size / 4 for float type
///return value: the consumed input pcm bytes number, any positive value is valid. Any negective value refer to ESP_RESP_ERR
///support two mode, mode 0 means decode mode, 1 means encode mode
void *esp_resample_create(void *info, unsigned  char **p_in, unsigned  char **p_out);
ESP_RESP_ERR esp_resample_run(void *handle, void *info, unsigned char *p_in, unsigned char *p_out, int in_data_size, int *out_data_size);
void esp_resample_destroy(void *handle);

#endif
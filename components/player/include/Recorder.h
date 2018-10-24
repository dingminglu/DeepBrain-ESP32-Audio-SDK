// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef __RECORDER_H__
#define __RECORDER_H__
#include "Recorder.h"
#include "AudioDef.h"

/*
 * @brief Get recorder status
 *
 * @param mode See 'AudioStatus' for more help
 *
 * @return See AudioErr
 */
int EspAudioRecorderStateGet(enum AudioStatus *state);

/**
 * @brief According to 'uri' param starting recorder.
 *
 * @param uri  Must conform to uri's rule like ( "raw://%d:%d@from.pcm/to.%s#i2s", 16000[sampleRate], 2[channel], "mp3"[music type] ) to start,
 *              while "i2s://16000:1@record.pcm#raw" for record
 *
 * @param bufSize  Specific recorder ringbuffer size.
 * @param i2sNum   I2S0 or I2S1
 *
 * @return See AudioErr structure
 */
int EspAudioRecorderStart(const char *uri, int bufSize, int i2sNum);

/**
 * @brief Read recording data for internal ringbuffer.
 *
 * @note This function will blocked when data is not enough.
 *
 *
 * @param data      Pointer to a buffer to store the recoding data
 * @param bufSize   Wanted read buffer size
 * @param outSize   Bytes read
 *
 * @return See AudioErr structure
 */
int EspAudioRecorderRead(uint8_t *data, int bufSize, int *outSize);

/**
 * @brief Stop recoding
 *
 * @param type See TerminationType enum for more help
 *
 * @return See AudioErr structure
 */
int EspAudioRecorderStop(enum TerminationType type);

#endif //__RECORDER_H__

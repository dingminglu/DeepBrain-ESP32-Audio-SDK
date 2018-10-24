// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: chenguoguo (chenguoguo@baidu.com)
//         xingrentai (xingrentai@baidu.com)
//
// snowboy detect

#ifndef DUER_SNOWBOY_SRC_WRAPPER_SNOWBOY_DETECT_H
#define DUER_SNOWBOY_SRC_WRAPPER_SNOWBOY_DETECT_H

#define DUER_PLATFORM_ESPRESSIF
#ifdef DUER_PLATFORM_ESPRESSIF
#include <stdbool.h>
#else
#include "utils/snowboy-types.h"
#endif

#ifndef INPUT_PCM_MAX_LENGTH
#define INPUT_PCM_MAX_LENGTH  (2560)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SnowboyInit();
// Constructor that takes a resource file, and a list of hotword models which
// are separated by comma. In the case that more than one hotword exist in the
// provided models, RunDetection() will return the index of the hotword, if
// the corresponding hotword is triggered.
//
// CAVEAT: a personal model only contain one hotword, but an universal model
//         may contain multiple hotwords. It is your responsibility to figure
//         out the index of the hotword. For example, if your model string is
//         "foo.pmdl,bar.umdl", where foo.pmdl contains hotword x, bar.umdl
//         has two hotwords y and z, the indices of different hotwords are as
//         follows:
//         x 1
//         y 2
//         z 3
//
// @param [in]  resource_filename   Filename of resource file.
// @param [in]  model_str           A string of multiple hotword models,
//                                  separated by comma.
void SnowboySetResource(const char* resource_filename, const char* model_str);

// Resets the detection. This class handles voice activity detection (VAD)
// internally. But if you have an external VAD, you should call Reset()
// whenever you see segment end from your VAD.
bool SnowboyReset(void);

// Runs hotword detection. Supported audio format is WAVE (with linear PCM,
// 16-bits signed integer).
// See SampleRate(), NumChannels() and BitsPerSample() for the required
// sampling rate, number of channels and bits per sample values. You are
// supposed to provide a small chunk of data (e.g., 0.1 second) each time you
// call RunDetection(). Larger chunk usually leads to longer delay, but less
// CPU usage.
//
// Definition of return values:
// -2: Silence.
// -1: Error.
//  0: No event.
//  1: Hotword 1 triggered.
//  2: Hotword 2 triggered.
//  ...
//
// @param [in]  data               Small chunk of data to be detected. See
//                                 above for the supported data format.
// @param [in]  array_length       Length of the data array.
//
// @param [in]  is_end             Set it to true if it is the end of a
//                                 utterance or file.
int SnowboyRunDetection(const short* const data,
                        const int array_length, bool is_end);

// Sets the sensitivity string for the loaded hotwords. A <sensitivity> is
// a list of floating numbers between 0 and 1, and separated by comma. For
// example, if there are 3 loaded hotwords, your string should looks something
// like this:
//   0.4,0.5,0.8
// Make sure you properly align the sensitivity value to the corresponding
// hotword.
void SnowboySetSensitivity(const char* sensitivity);

// Sets the high_sensitivity string for the loaded hotwords. A <high_sensitivity> is
// a list of floating numbers between 0 and 1, and separated by comma. For
// example, if there are 3 loaded hotwords, your string should looks something
// like this:
//   0.59,0.60,0.8
// Make sure you properly align the high_sensitivity value to the corresponding
// hotword.
void SnowboySetHighSensitivity(const char* high_sensitivity);

// Returns the sensitivity string for the current hotwords.
void SnowboyGetSensitivity(char* sensitivity);

// Applied a fixed gain to the input audio. In case you have a very weak
// microphone, you can use this function to boost input audio level.
void SnowboySetAudioGain(const float audio_gain);

// Writes the models to the model filenames specified in <model_str> in the
// constructor. This overwrites the original model with the latest parameter
// setting. You are supposed to call this function if you have updated the
// hotword sensitivities through SetSensitivity(), and you would like to store
// those values in the model as the default value.
void SnowboyUpdateModel(void);

// Returns the number of the loaded hotwords. This helps you to figure the
// index of the hotwords.
int SnowboyNumHotwords(void);

// If <apply_frontend> is true, then apply frontend audio processing;
// otherwise turns the audio processing off.
void SnowboyApplyFrontend(const bool apply_frontend);

#ifdef __cplusplus
}
#endif

#endif  // DUER_SNOWBOY_SRC_WRAPPER_SNOWBOY_DETECT_H


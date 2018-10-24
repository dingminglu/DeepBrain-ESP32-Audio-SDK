/*
 * This header file includes the VAD API calls.
 */

#ifndef DB_VAD_H
#define DB_VAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Creates an instance to the VAD structure.
/*
int mode: 
0 - Quality mode.
1 - Low bitrate mode.
2 - Aggressive mode.
3 - Very aggressive mode.
*/
void* DB_Vad_Create(int mode);

// Frees the dynamic memory of a specified VAD instance.
//
// - handle [i] : Pointer to VAD instance that should be freed.
void DB_Vad_Free(void* handle);

// Calculates a VAD decision for the |audio_frame|.
//
// - handle       [i/o] : VAD Instance. 
// - fs           [i]   : Sampling frequency (Hz): 8000, 16000, or 32000
// - frame_length [i]   : Length of audio frame buffer in number of samples. 
//						  support 10ms, 20ms, 30ms, for example [16000m,20ms], 
//						  the frame length is 16000*20/1000
// - audio_frame  [i]   : Audio frame buffer.
//
// returns              : 1 - (Active Voice),
//                        0 - (Non-active Voice),
//                       -1 - (Error)
//						 -2 - fs and frame length error
int DB_Vad_Process(void* handle, int fs, size_t frame_length, const int16_t* audio_frame);

#ifdef __cplusplus
}
#endif

#endif

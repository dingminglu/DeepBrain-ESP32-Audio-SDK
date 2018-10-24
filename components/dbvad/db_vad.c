
#include "webrtc_vad.h"

// Creates an instance to the VAD structure.
void* DB_Vad_Create(int mode)
{
	if (mode < 0 || mode > 3)
	{
		return NULL;
	}

	VadInst* state = WebRtcVad_Create();
	WebRtcVad_Init(state);
	WebRtcVad_set_mode(state, mode);
	
	return (void*)state;
}

// Frees the dynamic memory of a specified VAD instance.
//
// - handle [i] : Pointer to VAD instance that should be freed.
void DB_Vad_Free(void* handle)
{
	return WebRtcVad_Free((VadInst*)handle);
}

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
int DB_Vad_Process(void* handle, int fs, size_t frame_length, const int16_t* audio_frame)
{
	if (WebRtcVad_ValidRateAndFrameLength(fs, frame_length) != 0)
	{
		return -2;
	}
	
	return WebRtcVad_Process((VadInst*)handle, fs, audio_frame, frame_length);
}



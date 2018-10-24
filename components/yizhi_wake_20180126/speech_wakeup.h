// Copyright 2016-2017 Shanghai YuanQu Info Tech Ltd.
// All rights reserved.


#ifndef _SPEECH_WAKEUP_
#define _SPEECH_WAKEUP_


#define FRAME_LEN 160

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * @brief set the address to save auth info, should be invoked before @YQSW_CreateChannel
 * @param [in] addr  the flash address (default value :(0x200000-0x1000) )
 */
void YQSW_SetCustomAddr(unsigned int addr);


/**
 * @brief create a speech wakeup channel
 * @param [out] channel
 * @param [in] modelFile    not used
 * @return 1 if successful. On error, return negative.
 */
int    YQSW_CreateChannel(void **channel, char *modelFile);

/**
 * @brief reset speech wakeup channel
 * @param [in] channel  pointer to a wakeup channel
 */
void    YQSW_ResetChannel(void *channel);

/**
 * @brief process each speech frame and return wakeup result
 * @param [in] channel  pointer to a wakeup channel
 *        [in] wavBuff  pointer to a speech data buffer
 *        [in] nSamples  number of samples in buffer (must be FRAME_LEN)
 * @return 1 if find a configured keyword
 */
int     YQSW_ProcessWakeup(void *channel, short *wavBuff, int nSamples);

/**
 * @brief destroy speech wakeup channel
 * @param [in] channel  pointer to a wakeup channel
 */
void    YQSW_DestroyChannel(void *channel);

/**
 * @brief get keyword id
 * @param [in] channel  pointer to a wakeup channel
 */
int YQSW_GetKeywordId(void *channel);


#ifdef __cplusplus
}
#endif

#endif

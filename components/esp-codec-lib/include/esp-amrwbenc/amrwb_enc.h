// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AMRWB_ENC_H_
#define _AMRWB_ENC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VOAMRWB_MDNONE		= -1,	/*!< Invalid mode */
	VOAMRWB_MD66		= 0,	/*!< 6.60kbps   */
	VOAMRWB_MD885		= 1,    /*!< 8.85kbps   */
	VOAMRWB_MD1265		= 2,	/*!< 12.65kbps  */
	VOAMRWB_MD1425		= 3,	/*!< 14.25kbps  */
	VOAMRWB_MD1585		= 4,	/*!< 15.85bps   */
	VOAMRWB_MD1825		= 5,	/*!< 18.25bps   */
	VOAMRWB_MD1985		= 6,	/*!< 19.85kbps  */
	VOAMRWB_MD2305		= 7,    /*!< 23.05kbps  */
	VOAMRWB_MD2385          = 8,    /*!< 23.85kbps> */
	VOAMRWB_N_MODES 	= 9,	/*!< Invalid mode */	
}VOAMRWBMODE;

/*!* the frame format the codec supports*/
typedef enum {
	VOAMRWB_DEFAULT  	= 0,	/*!< the frame type is the header (defined in RFC3267) + rawdata*/
	/*One word (2-byte) for sync word (0x6b21)*/
	/*One word (2-byte) for frame length N.*/
	/*N words (2-byte) containing N bits (bit 0 = 0x007f, bit 1 = 0x0081).*/
	VOAMRWB_ITU         = 1,
	/*One word (2-byte) for sync word (0x6b21).*/
	/*One word (2-byte) to indicate the frame type.*/
	/*One word (2-byte) to indicate the mode.*/
	/*N words  (2-byte) containing N bits (bit 0 = 0xff81, bit 1 = 0x007f).*/
	VOAMRWB_RFC3267		= 2,	/* see RFC 3267 */
}VOAMRWBFRAMETYPE;

/**
 * @brief      Obtain the header size of AMRWB data
 *
 * @param      buf input buffer
 * @param      frameType the type of frame
 *
 * @return     The size of header for AMRWB 
 */
int amrwb_enc_header_info(unsigned char *buf, int frameType);

/**
 * @brief      Initialize and start the AMRWB Encoder 
 *
 * @param      allow_dtx the mode of allow_dtx
 * @param      frameType the type of frame
 * @param      mode the mode of the AMRWB Encoder
 *
 * @return     NONE 
 */
void *amrwb_enc_open(int allow_dtx, int frameType, int mode);

/**
 * @brief      Stop and cleanup the amrwb coding 
 *
 * @param      amrwb_handle Amrwb Element hanle
 *
 * @return     NONE 
 */
void amrwb_enc_close(void *amrwb_handle);

/**
 * @brief      Process of the AMRWB Encoder
 *
 * @param      amrwb_handle the AMRWB Encoder handle
 * @param      inbuf input buffer
 * @param      length size of input buffer
 * @param      outbuf output buffer
 *
 * @return     The  size of output data
 */
int amrwb_enc_process(void *amrwb_handle, unsigned char *inbuf, int length, unsigned char *outbuf);


#ifdef __cplusplus
}
#endif

#endif

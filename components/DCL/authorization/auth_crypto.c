/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include<stdlib.h>
#include <string.h>
#include <time.h>

#include "auth_crypto.h"
#include "mbedtls/sha1.h"
#include "mbedtls/md5.h"
#include "mbedtls/base64.h"
#include "time_interface.h"
#include "wifi_hw_interface.h"

static const uint16_t CrcCCITTTable[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

//init CRC with 0x0000
uint16_t CRC16(uint8_t* Buf,uint32_t BufLen,uint16_t CRC)
{
    register uint32_t i;

    for (i = 0 ; i < BufLen ; i++) 
	{
        CRC = (CRC << 8) ^ CrcCCITTTable[((CRC >> 8) ^ *(uint8_t*)Buf ++) & 0x00FF];
    }
    return CRC;
}

void crypto_sha1(const unsigned char *input, const size_t ilen, unsigned char output[20])
{
	return mbedtls_sha1(input, ilen, output);	
}

void crypto_md5(const unsigned char *input, const size_t ilen, unsigned char output[16])
{
	return mbedtls_md5(input, ilen, output);
}

int crypto_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
	return mbedtls_base64_encode(dst, dlen, olen, src, slen);
}

int crypto_base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
	return mbedtls_base64_decode(dst, dlen, olen, src, slen);
}

void crypto_rand_str(char *strRand, const size_t iLen)
{
	int i = 0;
	char metachar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	
	srand(get_time_of_day());
	memset(strRand, 0, iLen);
	for (i = 0; i < (iLen - 1); i++)
	{
		*(strRand+i) = metachar[rand() % 62];
	}

	return;
}

void crypto_sha1prng(unsigned char sha1prng[16], const unsigned char *strMac)
{
	unsigned char strSeedFull[64] = {0};
	unsigned char strSeed[16+1] = {0};
	unsigned char digest1[20];
	unsigned char digest2[20];

	//生成16位随机数种子
	crypto_rand_str((char*)strSeed, sizeof(strSeed));
	snprintf((char*)strSeedFull, sizeof(strSeedFull), "%s%s", strMac, strSeed);

	//第一次sha1加密
	crypto_sha1(strSeedFull, strlen((char*)strSeedFull), digest1); 

	//第二次sha1加密
	crypto_sha1(digest1, sizeof(digest1), digest2); 
	
	memcpy((char*)sha1prng, (char*)digest2, 16);

	return;
}

void crypto_time_stamp(unsigned char* strTimeStamp, const size_t iLen)
{
	time_t tNow =time(NULL);   
    struct tm tmNow = { 0 };  
	
    localtime_r(&tNow, &tmNow);//localtime_r为可重入函数，不能使用localtime  
    snprintf((char *)strTimeStamp, iLen, "%04d-%02d-%02dT%02d:%02d:%02d", 
		tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
		tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec); 
}

void crypto_random_byte(unsigned char _byte[16])
{
	unsigned char eth_mac[6] = {0};
	get_wifi_mac(eth_mac);

	crypto_sha1prng(_byte, eth_mac);

	return;
}

void crypto_generate_request_id(char *_out, size_t _out_len)
{
	unsigned char byte[16] = {0};
	unsigned char eth_mac[6] = {0};
	
	get_wifi_mac(eth_mac);
	crypto_sha1prng(byte, eth_mac);

	snprintf(_out, _out_len, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		byte[0], byte[1], byte[2], byte[3], byte[4], byte[5], byte[6], byte[7], byte[8], byte[9], 
		byte[10], byte[11], byte[12], byte[13], byte[14], byte[15]);

	return;
}

void crypto_generate_nonce(unsigned char* strNonce, const size_t iStrLen)
{
	int 			i			= 0;
	size_t 			len 		= 0;
	uint8_t 		eth_mac[6]	= {0};
	unsigned char 	strSha1[16] = {0};
	unsigned char 	str_buf[3] 	= {0};
	unsigned char 	str_mac[13] = {0};

	memset(eth_mac, 0, sizeof(eth_mac));
    get_wifi_mac(eth_mac);
	for (i=0; i < sizeof(eth_mac); i++)
	{
		snprintf((char*)str_buf, sizeof(str_buf), "%02X", eth_mac[i]);
		strcat((char*)str_mac, (char*)str_buf);
	}
	crypto_sha1prng(strSha1, str_mac);
	memset(strNonce, 0, iStrLen);
	crypto_base64_encode(strNonce, iStrLen, &len, strSha1, sizeof(strSha1));

	return;
}

void crypto_generate_nonce_hex(unsigned char* strNonce, const size_t iStrLen)
{
	int 			i			= 0;
	size_t 			len 		= 0;
	uint8_t 		eth_mac[6]	= {0};
	unsigned char 	strSha1[16] = {0};
	unsigned char 	str_buf[3] 	= {0};
	unsigned char 	str_mac[13] = {0};

	memset(eth_mac, 0, sizeof(eth_mac));
    get_wifi_mac(eth_mac);
	for (i=0; i < sizeof(eth_mac); i++)
	{
		snprintf((char*)str_buf, sizeof(str_buf), "%02X", eth_mac[i]);
		strcat((char*)str_mac, (char*)str_buf);
	}
	crypto_sha1prng(strSha1, str_mac);

	memset(strNonce, 0, iStrLen);
	snprintf((char*)strNonce, iStrLen, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", 
		strSha1[0],
		strSha1[1],strSha1[2],strSha1[3],strSha1[4],strSha1[5],
		strSha1[6],strSha1[7],strSha1[8],strSha1[9],strSha1[10],
		strSha1[11],strSha1[12],strSha1[13],strSha1[14],strSha1[15]);	
	
	return;
}

void crypto_generate_private_key(
	unsigned char* strPrivateKey, const size_t iStrLen, 
	const char* strNonce, const char* strTimeStamp, const char* strRobotID)
{
	size_t 			len 		= 0;
	char 			strBuf[256] = {0};
	unsigned char 	digest[20] 	= {0};

	snprintf(strBuf, sizeof(strBuf), "%s%s%s", strNonce, strTimeStamp, strRobotID); 
	crypto_sha1((unsigned char*)strBuf, strlen(strBuf), (unsigned char*)&digest); 

	crypto_base64_encode(strPrivateKey, iStrLen, &len, digest, sizeof(digest));

	return;
}


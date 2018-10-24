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

#ifndef AUTH_CRYPTO_H
#define AUTH_CRYPTO_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

void crypto_sha1(const unsigned char *input, const size_t ilen, unsigned char output[20]);
void crypto_md5(const unsigned char *input, const size_t ilen, unsigned char output[16]);
void crypto_rand_str(char *strRand, const size_t iLen);
void crypto_sha1prng(unsigned char sha1prng[16], const unsigned char *strMac);
void crypto_time_stamp(unsigned char* strTimeStamp, const size_t iLen);
int crypto_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen);
int crypto_base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen);
void crypto_random_byte(unsigned char _byte[16]);
void crypto_generate_nonce(unsigned char* strNonce, const size_t iStrLen);
void crypto_generate_nonce_hex(unsigned char* strNonce, const size_t iStrLen);
void crypto_generate_private_key(
	unsigned char* strPrivateKey, const size_t iStrLen, 
	const char* strNonce, const char* strTimeStamp, const char* strRobotID);
void crypto_generate_request_id(char *_out, size_t _out_len);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* AUTH_CRYPTO_H */


/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/

/**@file tsk_hmac.h
 * @brief HMAC: Keyed-Hashing for Message Authentication (RFC 2104) / FIPS-198-1.
 * HMAC-MD5 and HMAC-SHA-1 are also implemented.
 *
 *
 *

 */
#ifndef _TINYSAK_HMAC_H_
#define _TINYSAK_HMAC_H_

#include "tinysak_config.h"

#include "tsk_sha1.h"
#include "tsk_md5.h"

TSK_BEGIN_DECLS

TINYSAK_API int hmac_md5_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_md5string_t *result);
TINYSAK_API int hmac_md5digest_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_md5digest_t result);

TINYSAK_API int hmac_sha1_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_sha1string_t *result);
TINYSAK_API int hmac_sha1digest_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_sha1digest_t result);

TSK_END_DECLS

#endif /* _TINYSAK_HMAC_H_ */

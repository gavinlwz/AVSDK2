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

/**@file tsk_uuid.h
 * @brief Universally Unique Identifier (UUID version 5) implementation (RFC 4122).
 *		  This implementation is not fully conform to RFC 4122 but could be safely used to generate random UUIDs.
 *
 *
 *

 */
#ifndef _TINYSAK_UUID_H_
#define _TINYSAK_UUID_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

#define TSK_UUID_DIGEST_SIZE			16
#define TSK_UUID_STRING_SIZE			((TSK_UUID_DIGEST_SIZE*2)+4/*-*/)

typedef char tsk_uuidstring_t[TSK_UUID_STRING_SIZE+1]; /**< Hexadecimal UUID digest string. */
typedef char tsk_uuiddigest_t[TSK_UUID_DIGEST_SIZE]; /**< UUID digest bytes. */

TINYSAK_API int tsk_uuidgenerate(tsk_uuidstring_t *result);

TSK_END_DECLS

#endif /* _TINYSAK_UUID_H_ */

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

/**@file tnet_endianness.h
 * @brief Byte Ordering.
 *
 *
 *

 */
#ifndef TNET_ENDIANNESS_H
#define TNET_ENDIANNESS_H

#include "tinynet_config.h"

#include "tinysak_config.h" /* tsk_bool_t */

TNET_BEGIN_DECLS

#if	defined(TINYNET_IMPORTS_IGNORE)
#undef TNET_INLINE
#define TNET_INLINE
#endif

TINYNET_API TNET_INLINE unsigned short tnet_htons(unsigned short x);
TINYNET_API TNET_INLINE unsigned short tnet_htons_2(const void* px);
TINYNET_API TNET_INLINE unsigned long tnet_htonl(unsigned long x);
TINYNET_API TNET_INLINE unsigned long tnet_htonl_2(const void* px);


TINYNET_API TNET_INLINE uint64_t tnet_htonll(uint64_t x);
TINYNET_API TNET_INLINE uint64_t tnet_ntohll(uint64_t x);



TINYNET_API TNET_INLINE tsk_bool_t tnet_is_BE();

#define tnet_ntohs(x) tnet_htons(x)
#define tnet_ntohs_2(px) tnet_htons_2(px)
#define tnet_ntohl(x) tnet_htonl(x)
#define tnet_ntohl_2(px) tnet_htonl_2(px)

TNET_END_DECLS

#endif /*TNET_ENDIANNESS_H*/


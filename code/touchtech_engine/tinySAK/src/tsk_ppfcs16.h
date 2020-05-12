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

/**@file tsk_ppfcs16.h
 * @brief PPP in HDLC-like Framing (RFC 1662).
 *
 *
 *

 */
#ifndef _TINYSAK_PPFCS16_H_
#define _TINYSAK_PPFCS16_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

#define TSK_PPPINITFCS16    0xffff  /* Initial FCS value */
#define TSK_PPPGOODFCS16    0xf0b8  /* Good final FCS value */

TINYSAK_API uint16_t tsk_pppfcs16(register uint16_t fcs, register const uint8_t* cp, register int32_t len);

TSK_END_DECLS

#endif /* _TINYSAK_PPFCS16_H_ */


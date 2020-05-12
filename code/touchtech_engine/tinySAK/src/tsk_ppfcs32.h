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

/**@file tsk_ppfcs32.h
 * @brief PPP in HDLC-like Framing (RFC 1662).
 *
 *
 *

 */
#ifndef _TINYSAK_PPFCS32_H_
#define _TINYSAK_PPFCS32_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

#define TSK_PPPINITFCS32  0xffffffff   /* Initial FCS value */
#define TSK_PPPGOODFCS32  0xdebb20e3   /* Good final FCS value */

TINYSAK_API uint32_t tsk_pppfcs32(register uint32_t fcs, register const uint8_t* cp, register int32_t len);

TSK_END_DECLS

#endif /* _TINYSAK_PPFCS32_H_ */


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

/**@file tsk_url.h
 * @brief Useful string functions to manipulate strings.
 *
 *
 *

 */
#ifndef _TINYSAK_URL_H_
#define _TINYSAK_URL_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

TINYSAK_API char* tsk_url_encode(const char* url);
TINYSAK_API char* tsk_url_decode(const char* url);

TSK_END_DECLS

#endif /* _TINYSAK_URL_H_ */


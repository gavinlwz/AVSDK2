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

/**@file tnet.h
 * @brief Network stack.
 *
 *
 *

 */
#ifndef TNET_TNET_H
#define TNET_TNET_H

#include "tinynet_config.h"

#include "tinysak_config.h"

TNET_BEGIN_DECLS

TINYNET_API int tnet_startup();
TINYNET_API int tnet_cleanup();

TNET_END_DECLS

#endif /* TNET_TNET_H */


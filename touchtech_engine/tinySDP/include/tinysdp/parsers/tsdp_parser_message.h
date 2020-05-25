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

/**@file tsdp_parser_message.h
 * @brief SDP message parser.
 *
 *
 *

 */
#ifndef TINYSDP_PARSER_MESSAGE_H
#define TINYSDP_PARSER_MESSAGE_H

#include "tinysdp_config.h"
#include "tinysdp/tsdp_message.h"
#include "tsk_ragel_state.h"

TSDP_BEGIN_DECLS

TINYSDP_API tsdp_message_t* tsdp_message_parse(const char*  pszServerIP , int iRtpPort,int iSampleRate);

TSDP_END_DECLS

#endif /* TINYSDP_PARSER_MESSAGE_H */


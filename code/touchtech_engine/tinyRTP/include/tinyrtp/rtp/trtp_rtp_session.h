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
/**@file trtp_rtp_session.h
 * @brief RTP session.
 *
*
 *

 */
#ifndef TINYMEDIA_RTP_SESSION_H
#define TINYMEDIA_RTP_SESSION_H

#include "tinyrtp_config.h"

TRTP_BEGIN_DECLS

struct trtp_rtp_packet_s;

typedef int (*trtp_rtp_cb_f)(const void* callback_data, const struct trtp_rtp_packet_s* packet);

TRTP_END_DECLS

#endif /* TINYMEDIA_RTP_SESSION_H */

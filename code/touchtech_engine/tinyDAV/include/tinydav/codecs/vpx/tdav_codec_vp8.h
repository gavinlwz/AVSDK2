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

/**@file tdav_codec_vp8.h
 * @brief Google's VP8 (http://www.webmproject.org/) encoder/decoder
 * The RTP packetizer/depacketizer follows draft-ietf-payload-vp8 and draft-bankoski-vp8-bitstream-05
 *
 *
 */
#ifndef TINYDAV_CODEC_VP8_H
#define TINYDAV_CODEC_VP8_H

#include "tinydav_config.h"

#if HAVE_LIBVPX

#include "tinymedia/tmedia_codec.h"

TDAV_BEGIN_DECLS

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_vp8_plugin_def_t;

TDAV_END_DECLS

#endif /* HAVE_LIBVPX */


#endif /* TINYDAV_CODEC_VP8_H */
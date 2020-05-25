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

#ifndef TINYMEDIA_TDAV_H
#define TINYMEDIA_TDAV_H

#include "tinysak_config.h"
#include "tinydav_config.h"
#include "tinymedia/tmedia_codec.h"


TDAV_BEGIN_DECLS


TINYDAV_API int tdav_init ();
TINYDAV_API int tdav_codec_set_priority (tmedia_codec_id_t codec_id, int priority);
TINYDAV_API int tdav_set_codecs (tmedia_codec_id_t codecs);
TINYDAV_API tsk_bool_t tdav_codec_is_supported (tmedia_codec_id_t codec);
TINYDAV_API tsk_bool_t tdav_codec_is_enabled (tmedia_codec_id_t codec);
TINYDAV_API tsk_bool_t tdav_ipsec_is_supported ();
TINYDAV_API int tdav_deinit ();

TDAV_END_DECLS

#endif

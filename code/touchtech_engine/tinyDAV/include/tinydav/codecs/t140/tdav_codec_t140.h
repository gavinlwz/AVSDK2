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

/**@file tdav_codec_t140.h
 * @brief T140 codec implementation (RFC 4103)
 */
#ifndef TINYDAV_CODEC_T140_H
#define TINYDAV_CODEC_T140_H

#include "tinydav_config.h"

#include "tinymedia/tmedia_codec.h"

TDAV_BEGIN_DECLS

typedef struct tdav_codec_t140_s
{
	TMEDIA_DECLARE_CODEC;
}
tdav_codec_t140_t;

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_t140_plugin_def_t;

TDAV_END_DECLS

#endif /* TINYDAV_CODEC_T140_H */

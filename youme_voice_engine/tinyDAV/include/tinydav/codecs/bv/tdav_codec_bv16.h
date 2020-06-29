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

/**@file tdav_codec_bv16.h
 * @brief BroadVoice16 codec
 * The payloader/depayloader follow RFC 4298
*
 *

 */
#ifndef TINYDAV_CODEC_BV16_H
#define TINYDAV_CODEC_BV16_H

#include "tinydav_config.h"

#if HAVE_BV16

#include "tinymedia/tmedia_codec.h"


TDAV_BEGIN_DECLS

/** BV16 codec */
typedef struct tdav_codec_bv16_s
{
	TMEDIA_DECLARE_CODEC_AUDIO;

	struct {
		void *state;
		void *bs;
		void *x;
	} encoder;

	struct {
		void *state;
		void *bs;
		void *x;
	} decoder;
}
tdav_codec_bv16_t;

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_bv16_plugin_def_t;

TDAV_END_DECLS

#endif /* HAVE_BV16 */

#endif /* TINYDAV_CODEC_BV16_H */

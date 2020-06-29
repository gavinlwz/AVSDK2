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

/**@file tdav_codec_gsm.h
 * @brief GSM Full Rate Codec (Based on libgsm)
 *
*
 *

 */
#ifndef TINYDAV_CODEC_GSM_H
#define TINYDAV_CODEC_GSM_H

#include "tinydav_config.h"

#if HAVE_LIBGSM

#include "tinymedia/tmedia_codec.h"

#include <gsm.h>

TDAV_BEGIN_DECLS

/** GSM codec */
typedef struct tdav_codec_gsm_s
{
	TMEDIA_DECLARE_CODEC_AUDIO;

	gsm encoder;
	gsm decoder;
}
tdav_codec_gsm_t;

TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_gsm_plugin_def_t;

TDAV_END_DECLS

#endif /* HAVE_LIBGSM */

#endif /* TINYDAV_CODEC_GSM_H */

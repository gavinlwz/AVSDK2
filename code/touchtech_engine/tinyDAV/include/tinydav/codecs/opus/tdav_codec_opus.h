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

/**@file tdav_codec_opus.h
 * @brief OPUS audio codec.
 */
#ifndef TINYDAV_CODEC_OPUS_H
#define TINYDAV_CODEC_OPUS_H

#include "tinydav_config.h"

#if HAVE_LIBOPUS

#include "tinymedia/tmedia_codec.h"
#include "tinymedia/tmedia_params.h"

#ifdef  __cplusplus
extern "C" {
#endif
	TINYDAV_GEXTERN const tmedia_codec_plugin_def_t *tdav_codec_opus_plugin_def_t;
#ifdef  __cplusplus
}
#endif

	

#endif /* HAVE_LIBOPUS */

#endif /* TINYDAV_CODEC_OPUS_H */

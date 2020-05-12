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

/**@file tmedia_codec_dummy.h
 * @brief Dummy codecs used for test only.
 *
 *
 *

 */
#ifndef TINYMEDIA_CODEC_DUMMY_H
#define TINYMEDIA_CODEC_DUMMY_H

#include "tinymedia_config.h"

#include "tmedia_codec.h"

#include "tsk_object.h"

TMEDIA_BEGIN_DECLS

/** Dummy PCMU codec */
typedef struct tmedia_codec_dpcmu_s
{
	TMEDIA_DECLARE_CODEC_AUDIO;
}
tmedia_codec_dpcmu_t;

/** Dummy PCMA codec */
typedef struct tmedia_codec_dpcma_s
{
	TMEDIA_DECLARE_CODEC_AUDIO;
}
tmedia_codec_dpcma_t;

TINYMEDIA_GEXTERN const tmedia_codec_plugin_def_t *tmedia_codec_dpcma_plugin_def_t;
TINYMEDIA_GEXTERN const tmedia_codec_plugin_def_t *tmedia_codec_dpcmu_plugin_def_t;

TINYMEDIA_GEXTERN const tmedia_codec_plugin_def_t *tmedia_codec_dh263_plugin_def_t;
TINYMEDIA_GEXTERN const tmedia_codec_plugin_def_t *tmedia_codec_dh264_plugin_def_t;

TMEDIA_END_DECLS

#endif /* TINYMEDIA_CODEC_DUMMY_H */

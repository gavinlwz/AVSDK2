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

/**@file tdav_consumer_audio.h
 * @brief Base class for all Audio consumers.
 *
*
 */
#ifndef TINYDAV_CONSUMER_AUDIO_H
#define TINYDAV_CONSUMER_AUDIO_H

#include "tinysak_config.h"

#include "tinydav_config.h"

#include "tinymedia/tmedia_consumer.h"
#include "tinydav/codecs/mixer/speex_resampler.h"

#include "tsk_safeobj.h"

TDAV_BEGIN_DECLS

#define TDAV_CONSUMER_AUDIO(self)		((tdav_consumer_audio_t*)(self))

#define PCM_FILE_PUREVOICE        1
#define PCM_FILE_PLAYBACK         2
typedef struct tdav_consumer_audio_s
{
	TMEDIA_DECLARE_CONSUMER;
	
	struct tmedia_denoise_s* denoise;
	struct tmedia_resampler_s* resampler;
	struct tmedia_jitterbuffer_s* jitterbuffer;
	
	void *pvPureVoice;
    
    uint32_t output_frame_count;
    
	TSK_DECLARE_SAFEOBJ;
}
tdav_consumer_audio_t;

TINYDAV_API int tdav_consumer_audio_init(tdav_consumer_audio_t* self);
TINYDAV_API int tdav_consumer_audio_cmp(const tsk_object_t* consumer1, const tsk_object_t* consumer2);
#define tdav_consumer_audio_prepare(self, codec) tmedia_consumer_prepare(TDAV_CONSUMER_AUDIO(self), codec)
#define tdav_consumer_audio_start(self) tmedia_consumer_start(TDAV_CONSUMER_AUDIO(self))
#define tdav_consumer_audio_consume(self, buffer, size) tmedia_consumer_consume(TDAV_CONSUMER_AUDIO(self), buffer, size)
#define tdav_consumer_audio_pause(self) tmedia_consumer_pause(TDAV_CONSUMER_AUDIO(self))
#define tdav_consumer_audio_stop(self) tmedia_consumer_stop(TDAV_CONSUMER_AUDIO(self))
TINYDAV_API int tdav_consumer_audio_set_param(tdav_consumer_audio_t* self, const tmedia_param_t* param);
TINYDAV_API int tdav_consumer_audio_get_param(tdav_consumer_audio_t* self, tmedia_param_t* param);
TINYDAV_API int tdav_consumer_audio_put(tdav_consumer_audio_t* self, const void* data, tsk_size_t data_size, tsk_object_t* proto_hdr);
TINYDAV_API tsk_size_t tdav_consumer_audio_get(tdav_consumer_audio_t* self, void* out_data, tsk_size_t out_size);
TINYDAV_API int tdav_consumer_audio_tick(tdav_consumer_audio_t* self);
void tdav_consumer_audio_set_denoise(tdav_consumer_audio_t* self, struct tmedia_denoise_s* denoise);
void tdav_consumer_audio_set_jitterbuffer(tdav_consumer_audio_t* self, struct tmedia_jitterbuffer_s* jitterbuffer);
TINYDAV_API int tdav_consumer_audio_reset(tdav_consumer_audio_t* self);
TINYDAV_API int tdav_consumer_audio_deinit(tdav_consumer_audio_t* self);

#define TDAV_DECLARE_CONSUMER_AUDIO tdav_consumer_audio_t __consumer_audio__

TDAV_END_DECLS

#endif /* TINYDAV_CONSUMER_AUDIO_H */

//
//  tdav_audio_unit.h
//  youme_voice_engine
//
//  Created by mac on 2017/8/10.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tdav_audio_unit_h
#define tdav_audio_unit_h

#include "tinydav_config.h"
#include "tsk_object.h"
#include "tsk_thread.h"
#include "tsk_list.h"
#include "tinydav/codecs/mixer/speex_resampler.h"

TDAV_BEGIN_DECLS


#define TDAV_AUDIO_UNIT(self)   ((tdav_audio_unit_t*)(self))

typedef struct tdav_audio_unit_s {
    TSK_DECLARE_OBJECT;
    void* data;
    uint32_t size;
    uint64_t timestamp;
    uint32_t sessionid;
} tdav_audio_unit_t;

tdav_audio_unit_t* tdav_audio_unit_create(uint32_t size);

#define TDAV_AUDIO_RESAMPLE(self)   ((tdav_audio_resample_t*)(self))
typedef struct tdav_audio_resample_s {
    TSK_DECLARE_OBJECT;
    uint32_t sessionid;
    tsk_boolean_t isMixing;
    SpeexResamplerState* resampler;
    tsk_mutex_handle_t* resampler_mutex;
    tsk_bool_t running;
    tsk_thread_handle_t *thread_handle;
    uint32_t samplerate_in;
    uint32_t samplerate_out;
    tsk_list_t *audio_in;
    uint32_t temp_buffer_size;
    void* temp_buffer;
} tdav_audio_resample_t;

tdav_audio_resample_t* tdav_audio_resample_create(uint32_t sessionid, tsk_boolean_t isMixing, uint32_t channels, uint32_t samplerate_in, uint32_t samplerate_out);
int tdav_audio_resample_push(tdav_audio_resample_t* self, tdav_audio_unit_t* audio_unit);

TDAV_END_DECLS

#endif /* tdav_audio_unit_h */

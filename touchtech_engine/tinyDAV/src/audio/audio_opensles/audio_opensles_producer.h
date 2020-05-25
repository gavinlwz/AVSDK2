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
#ifndef _YOUME_AUDIO_OPENSLES_PRODUCER_H
#define _YOUME_AUDIO_OPENSLES_PRODUCER_H

#include "tsk.h"
#include "audio_opensles.h"
#include "audio_opensles_config.h"
#include "tinydav/audio/tdav_producer_audio.h"

AUDIO_OPENSLES_BEGIN_DECLS

typedef struct audio_producer_opensles_s
{
    TDAV_DECLARE_PRODUCER_AUDIO;
    
    tsk_bool_t isMuted;
    audio_opensles_instance_handle_t *audioInstHandle;
    struct
    {
        void *ptr;
        int size;
        int index;
    } buffer;
} audio_producer_opensles_t;

extern const struct tmedia_producer_plugin_def_s *audio_producer_opensles_plugin_def_t;

// handle recorded data
int audio_producer_opensles_handle_data_10ms (const struct audio_producer_opensles_s *self, const void *audioSamples, int nSamples, int nBytesPerSample, int samplesPerSec, int nChannels);

AUDIO_OPENSLES_END_DECLS

#endif /* _YOUME_AUDIO_OPENSLES_PRODUCER_H */

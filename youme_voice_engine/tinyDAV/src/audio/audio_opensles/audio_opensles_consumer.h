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
#ifndef _YOUME_AUDIOOPENSLES_CONSUMER_H
#define _YOUME_AUDIOOPENSLES_CONSUMER_H

#include "tinysak_config.h"
#include "audio_opensles_config.h"
#include "audio_opensles.h"
#include "tinydav/audio/tdav_consumer_audio.h"

AUDIO_OPENSLES_BEGIN_DECLS

typedef struct audio_consumer_opensles_s
{
    TDAV_DECLARE_CONSUMER_AUDIO;
    audio_opensles_instance_handle_t *audioInstHandle;
    tsk_bool_t isSpeakerOn;
    tsk_bool_t isSpeakerMute;
    struct
    {
        void *ptr;
        tsk_bool_t isFull;
        int size;
        int index;
    } buffer;
} audio_consumer_opensles_t;

extern const struct tmedia_consumer_plugin_def_s *audio_consumer_opensles_plugin_def_t;

int audio_consumer_opensles_get_data_10ms (const struct audio_consumer_opensles_s *self, void *audioSamples, int nSamples, int nBytesPerSample, int nChannels, int samplesPerSec);
tsk_bool_t audio_consumer_opensles_is_speakerOn (const struct audio_consumer_opensles_s *self);

AUDIO_OPENSLES_END_DECLS

#endif /* _YOUME_AUDIOOPENSLES_CONSUMER_H */

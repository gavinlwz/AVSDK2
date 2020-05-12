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
#ifndef TINYDAV_CONSUMER_COREAUDIO_AUDIO_UNIT_H
#define TINYDAV_CONSUMER_COREAUDIO_AUDIO_UNIT_H

#include "tinydav_config.h"

#if TARGET_OS_IPHONE

#include <AudioToolbox/AudioToolbox.h>
#include "tinydav/audio/coreaudio/tdav_audiounit.h"
#include "tinydav/audio/tdav_consumer_audio.h"
#import "tinymedia/tmedia_resampler.h"
#import "youme_buffer.h"

#include "tsk_mutex.h"

TDAV_BEGIN_DECLS

typedef struct tdav_consumer_audiounit_s
{
    TDAV_DECLARE_CONSUMER_AUDIO;

    tdav_audiounit_handle_t *audioUnitHandle;
    unsigned started : 1;
    unsigned paused : 1;

    struct
    {
        struct
        {
            void *buffer;
            tsk_size_t size;
        } chunck;
        tsk_ssize_t leftBytes;
        YOUMEBuffer *buffer;
        tsk_size_t size;
        tsk_mutex_handle_t *mutex;
    } ring;

    tmedia_resampler_t *resampler;
} tdav_consumer_audiounit_t;

TINYDAV_GEXTERN const tmedia_consumer_plugin_def_t *tdav_consumer_audiounit_plugin_def_t;

TDAV_END_DECLS

#endif /* TARGET_OS_IPHONE */

#endif /* TINYDAV_CONSUMER_COREAUDIO_AUDIO_UNIT_H */

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
#ifndef TINYDAV_PRODUCER_COREAUDIO_AUDIO_UNIT_H
#define TINYDAV_PRODUCER_COREAUDIO_AUDIO_UNIT_H

#include "tinydav_config.h"

#ifdef __APPLE__

#include <AudioToolbox/AudioToolbox.h>
#include "youme_buffer.h"
#include "tinydav/audio/coreaudio/tdav_audiounit.h"
#include "tinydav/audio/tdav_producer_audio.h"
#include "tsk_mutex.h"
#include "tsk_thread.h"


TDAV_BEGIN_DECLS

typedef struct tdav_producer_audiounit_s
{
    TDAV_DECLARE_PRODUCER_AUDIO;

    tdav_audiounit_handle_t *audioUnitHandle;
    unsigned started : 1;
    unsigned paused : 1;
    unsigned muted;

    // If no recording is allowed (such as no recording permission, airplay is on, etc.)
    // we will not set a category playAndRecord, instead, we will set it to playback only,
    // and produce silence data through a thread to simulate the recorder.
    // By doing this, we can allow a third party app with recording or airplay working to
    // continue to work.
    // Unfortunately, there seems no way to detect if other app is using the recording devices,
    // so we can only detect the recording permission and ask the users to disable the recording
    // permission if they want a third party app to continue to record.
    //pthread_t       silenceThreadId;
    tsk_thread_handle_t *silenceThreadId;
    pthread_mutex_t silenceThreadMutex;
    pthread_cond_t  silenceThreadCond;
    uint8_t         silenceThreadStarted;

    struct
    {
        struct
        {
            void *buffer;
            tsk_size_t size;
        } chunck;
        
        //hal模式下，录音输出的采样率由设备决定，
        struct
        {
            void * buffer;
            tsk_size_t size;
            UInt32 recordSampleRate;
            void   *resampler;
        } resampleBuffer;
        
        YOUMEBuffer *buffer;
        tsk_size_t size;
    } ring;

} tdav_producer_audiounit_t;

TINYDAV_GEXTERN const tmedia_producer_plugin_def_t *tdav_producer_audiounit_plugin_def_t;

TDAV_END_DECLS

#endif /* __APPLE__ */

#endif /* TINYDAV_PRODUCER_COREAUDIO_AUDIO_UNIT_H */

/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:paul
 *  日期:2016.9.19
 *  说明:对外发布接口
 ******************************************************************/
#ifndef _YOUME_AUDIO_ANDROID_PRODUCER_H
#define _YOUME_AUDIO_ANDROID_PRODUCER_H

#include "tsk.h"
#include "audio_android.h"
#include "tinydav/audio/tdav_producer_audio.h"
#include "tinydav/audio/tdav_consumer_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_producer_android_s
{
    TDAV_DECLARE_PRODUCER_AUDIO;
    
    tsk_bool_t isMuted;
    void *audioInstHandle;
    struct
    {
        void *ptr;
        int size;
        int index;
    } buffer;
} audio_producer_android_t;

extern const struct tmedia_producer_plugin_def_s *audio_producer_android_plugin_def_t;

// handle recorded data
int audio_producer_android_handle_data_20ms (const struct audio_producer_android_s *self, const void *audioSamples, int nSamples, int nBytesPerSample, int samplesPerSec, int nChannels);

extern void JNI_Init_Audio_Record  (int sampleRate, int channelNum, int bytesPerSample, int typeSpeech, tmedia_producer_t *pProducerInstanse);
extern void JNI_Start_Audio_Record ();
extern void JNI_Stop_Audio_Record  ();
extern int  JNI_Get_Audio_Record_Status ();
    
enum AUDIO_RECORD_INIT_STATUS { // MUST sync with ANDROID's define, pls reference AudioRecorder.java
    AUDIO_RECORD_BAD_VALUE     = -2,
    AUDIO_RECORD_ERROR         = -1,
    AUDIO_RECORD_OPERATION     = -3,
    AUDIO_RECORD_PERMISSION    = -4,
    AUDIO_RECORD_NO_DATA       = -5,
    AUDIO_RECORD_UNINITIALIZED = 0,
    AUDIO_RECORD_INIT_SUCCESS  = 100,
};
#ifdef __cplusplus
}
#endif

#endif /* _YOUME_AUDIO_OPENSLES_PRODUCER_H */

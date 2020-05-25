/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:paul
 *  日期:2016.9.22
 *  说明:对外发布接口
 ******************************************************************/
#ifndef _YOUME_AUDIO_ANDROID_CONSUMER_H
#define _YOUME_AUDIO_ANDROID_CONSUMER_H

#include "tinysak_config.h"
#include "audio_android.h"
#include "tinydav/audio/tdav_consumer_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_consumer_android_s
{
    TDAV_DECLARE_CONSUMER_AUDIO;
    void *audioInstHandle;
    tsk_bool_t isSpeakerMute;
    struct
    {
        void *ptr;
        tsk_bool_t isFull;
        int size;
        int index;
    } buffer;
} audio_consumer_android_t;

extern const struct tmedia_consumer_plugin_def_s *audio_consumer_android_plugin_def_t;

int audio_consumer_android_get_data_20ms (const struct audio_consumer_android_s *self, void *audioSamples, int nSamples, int nBytesPerSample, int nChannels, int samplesPerSec);

extern void JNI_Init_Audio_Player  (int sampleRate, int channelNum, int bytesPerSample, int isStreamVoice, tmedia_consumer_t *pConsumerInstanse);
extern void JNI_Start_Audio_Player ();
extern void JNI_Stop_Audio_Player  ();
extern int  JNI_Get_Audio_Player_Status ();
#ifdef __cplusplus
}
#endif

#endif /* _YOUME_AUDIO_ANDROID_CONSUMER_H */

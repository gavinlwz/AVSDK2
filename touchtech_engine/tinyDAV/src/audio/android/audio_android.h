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
#ifndef _YOUME_AUDIO_ANDROID_H
#define _YOUME_AUDIO_ANDROID_H

#include "tinysak_config.h"
#include "tsk_plugin.h"
#include "tsk_safeobj.h"
//#include "audio_android_device_impl.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct audio_android_instance_s
{
    TSK_DECLARE_OBJECT;
        
    uint64_t sessionId;
        
    tsk_bool_t isStarted;
        
    tsk_bool_t isConsumerPrepared;
    tsk_bool_t isConsumerStarted;
    tsk_bool_t isProducerPrepared;
    tsk_bool_t isProducerStarted;
        
    //bool isSpeakerAvailable;
    //bool isPlayoutAvailable;
    //bool isRecordingAvailable;
    
    //SLAudioDevice *device;
    //AudioAndroidDeviceCallbackImpl *callback;
    void *callback;
        
    TSK_DECLARE_SAFEOBJ;
} audio_android_instance_t;
    
// TODO!!
//AUDIO_OPENSLES_API int __plugin_get_def_count ();
//AUDIO_OPENSLES_API tsk_plugin_def_type_t __plugin_get_def_type_at (int index);
//AUDIO_OPENSLES_API tsk_plugin_def_media_type_t __plugin_get_def_media_type_at (int index);
//AUDIO_OPENSLES_API tsk_plugin_def_ptr_const_t __plugin_get_def_at (int index);

void *audio_android_instance_create (uint64_t session_id, int isForRecording);
int audio_android_instance_prepare_consumer (void *_self, struct tmedia_consumer_s **consumer);
int audio_android_instance_prepare_producer (void *_self, struct tmedia_producer_s **producer);
int audio_android_instance_start_consumer (void *_self);
int audio_android_instance_start_producer (void *_self);
int audio_android_instance_stop_consumer (void *_self);
int audio_android_instance_stop_producer (void *_self);
//int audio_opensles_instance_set_speakerOn (audio_opensles_instance_handle_t *self, tsk_bool_t speakerOn);
//int audio_opensles_instance_set_microphone_volume (audio_opensles_instance_handle_t *self, int32_t volume);
int audio_android_instance_get_recording_error (void *_self, int32_t *pErrCode);
int audio_android_instance_get_recording_error_extra (void *_self, int32_t *pErrExtra);
int audio_android_instance_destroy (void **self);

#ifdef __cplusplus
}
#endif

#endif /* _YOUME_AUDIO_ANDROID_H */

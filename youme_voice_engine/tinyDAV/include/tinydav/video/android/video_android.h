/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.2.23
 *  说明:对外发布接口
 ******************************************************************/

#ifndef YOUME_VIDEO_ANDROID_H_
#define YOUME_VIDEO_ANDROID_H_

#include "tinysak_config.h"
#include "tsk_plugin.h"
#include "tsk_safeobj.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    typedef struct video_android_instance_s
    {
        TSK_DECLARE_OBJECT;
        
        uint64_t sessionId;
        
        tsk_bool_t isStarted;
        
        tsk_bool_t isConsumerPrepared;
        tsk_bool_t isConsumerStarted;
        tsk_bool_t isProducerPrepared;
        tsk_bool_t isProducerStarted;
        
        void *callback;
        
        TSK_DECLARE_SAFEOBJ;
    } video_android_instance_t;
    
    void *video_android_instance_create (uint64_t session_id, int isForRecording);
    int video_android_instance_prepare_consumer (void *_self, struct tmedia_consumer_s **consumer);
    int video_android_instance_prepare_producer (void *_self, struct tmedia_producer_s **producer);
    int video_android_instance_start_consumer (void *_self);
    int video_android_instance_start_producer (void *_self);
    int video_android_instance_stop_consumer (void *_self);
    int video_android_instance_stop_producer (void *_self);
    int video_android_instance_get_recording_error (void *_self, int32_t *pErrCode);
    int video_android_instance_get_recording_error_extra (void *_self, int32_t *pErrExtra);
    int video_android_instance_destroy (void **self);
    
#ifdef __cplusplus
}
#endif

#endif /* YOUME_VIDEO_ANDROID_H_ */

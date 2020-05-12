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
#include "tinydav/video/android/video_android.h"

#include "tinydav/video/android/video_android_producer.h"

#include "tinymedia/tmedia_consumer.h"
#include "tinymedia/tmedia_producer.h"
#include "tinydav/video/android/video_android_device_impl.h"

#include "tsk_list.h"
#include "tsk_safeobj.h"
#include "tsk_debug.h"

/*
typedef enum PLUGIN_INDEX_E
{
    PLUGIN_INDEX_VIDEO_CONSUMER,
    PLUGIN_INDEX_VIDEO_PRODUCER,
    PLUGIN_INDEX_COUNT
} PLUGIN_INDEX_T;

int __plugin_get_def_count ()
{
    return PLUGIN_INDEX_COUNT;
}

tsk_plugin_def_type_t __plugin_get_def_type_at (int index)
{
    switch (index)
    {
        case PLUGIN_INDEX_VIDEO_CONSUMER:
            return tsk_plugin_def_type_consumer;
        case PLUGIN_INDEX_VIDEO_PRODUCER:
            return tsk_plugin_def_type_producer;
        default:
        {
            TSK_DEBUG_ERROR ("No plugin at index %d", index);
            return tsk_plugin_def_type_none;
        }
    }
}

tsk_plugin_def_media_type_t __plugin_get_def_media_type_at (int index)
{
    switch (index)
    {
        case PLUGIN_INDEX_VIDEO_CONSUMER:
        case PLUGIN_INDEX_VIDEO_PRODUCER:
        {
            return tsk_plugin_def_media_type_video;
        }
        default:
        {
            TSK_DEBUG_ERROR ("No plugin at index %d", index);
            return tsk_plugin_def_media_type_none;
        }
    }
}

tsk_plugin_def_ptr_const_t __plugin_get_def_at (int index)
{
    switch (index)
    {
        case PLUGIN_INDEX_VIDEO_PRODUCER:
        {
            return video_producer_android_plugin_def_t;
        }
        default:
        {
            TSK_DEBUG_ERROR ("No plugin at index %d", index);
            return tsk_null;
        }
    }
}
*/

//
// SLES AudioInstance
//
typedef tsk_list_t video_android_instances_L_t;

static video_android_instances_L_t *__videoInstances = tsk_null;

static tsk_object_t *video_android_instance_ctor (tsk_object_t *self, va_list *app)
{
    video_android_instance_t *videoInstance = (video_android_instance_t *)self;
    if (videoInstance)
    {
        tsk_safeobj_init (videoInstance);
    }
    return self;
}
static tsk_object_t *video_android_instance_dtor (tsk_object_t *self)
{
    TSK_DEBUG_INFO ("Video Instance destroyed");
    video_android_instance_t *videoInstance = (video_android_instance_t *)self;
    if (videoInstance)
    {
        tsk_safeobj_lock (videoInstance);
        
        if (videoInstance->callback)
        {
            delete (VideoAndroidDeviceCallbackImpl*)(videoInstance->callback);
            videoInstance->callback = tsk_null;
        }

        tsk_safeobj_unlock (videoInstance);
        
        tsk_safeobj_deinit (videoInstance);
    }
    return self;
}
static int video_android_instance_cmp (const tsk_object_t *_ai1, const tsk_object_t *_ai2)
{
    return ((long)_ai1 - (long)_ai2);
}
static const tsk_object_def_t video_android_instance_def_s = {
    sizeof (video_android_instance_t),
    video_android_instance_ctor,
    video_android_instance_dtor,
    video_android_instance_cmp,
};
const tsk_object_def_t *video_android_instance_def_t = &video_android_instance_def_s;


void *video_android_instance_create (uint64_t sessionId, int isForRecording)
{
    
    video_android_instance_t *videoInstance = tsk_null;
    
    // create list used to hold instances
    if (!__videoInstances && !(__videoInstances = tsk_list_create ()))
    {
        TSK_DEBUG_ERROR ("Failed to create new list");
        return tsk_null;
    }
    
    //= lock the list
    tsk_list_lock (__videoInstances);
    
    // find the instance from the list
    const tsk_list_item_t *item;
    tsk_list_foreach (item, __videoInstances)
    {
        if (((video_android_instance_t *)item->data)->sessionId == sessionId)
        {
            videoInstance = (video_android_instance_t *)tsk_object_ref (item->data);
            break;
        }
    }
    
    if (!videoInstance)
    {
        video_android_instance_t *_videoInstance = tsk_null;
        if (!(_videoInstance = (video_android_instance_t *)tsk_object_new (&video_android_instance_def_s)))
        {
            TSK_DEBUG_ERROR ("Failed to create new video instance");
            goto done;
        }

        if (!(_videoInstance->callback = new VideoAndroidDeviceCallbackImpl ()))
        {
            TSK_DEBUG_ERROR ("Failed to create video transport");
            TSK_OBJECT_SAFE_FREE (_videoInstance);
            goto done;
        }

        _videoInstance->sessionId = sessionId;
        videoInstance = _videoInstance;
        tsk_list_push_back_data (__videoInstances, (void **)&_videoInstance);
    }
    
done:
    //= unlock the list
    tsk_list_unlock (__videoInstances);
    
    return (void *)videoInstance;
}

int video_android_instance_prepare_consumer (void *_self, tmedia_consumer_t **_consumer)
{
    video_android_instance_t *self = (video_android_instance_t *)_self;
    VideoAndroidDeviceCallbackImpl *cbFunc = NULL;
    
    if (!self || !self->callback || !_consumer || !*_consumer)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    if (self->isConsumerPrepared)
    {
        TSK_DEBUG_WARN ("Consumer already prepared");
        return 0;
    }
    
    int ret = 0;
    
    tsk_safeobj_lock (self);
    
    cbFunc = (VideoAndroidDeviceCallbackImpl*)self->callback;
    cbFunc->SetConsumer ((const struct video_consumer_android_s *)*_consumer);
    
done:
    tsk_safeobj_unlock (self);
    
    self->isConsumerPrepared = (ret == 0);
    
    return ret;
}

int video_android_instance_prepare_producer (void *_self, tmedia_producer_t **_producer)
{
    video_android_instance_t *self = (video_android_instance_t *)_self;
    VideoAndroidDeviceCallbackImpl *cbFunc = NULL;
    if (!self || !_producer || !*_producer || !self->callback)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    if (self->isProducerPrepared)
    {
        TSK_DEBUG_WARN ("Producer already prepared");
        return 0;
    }
    
    int ret = 0;
    
    tsk_safeobj_lock (self);
    
    cbFunc = (VideoAndroidDeviceCallbackImpl*)self->callback;
    cbFunc->SetProducer ((const struct video_producer_android_s *)*_producer);
    
done:
    tsk_safeobj_unlock (self);
    
    self->isProducerPrepared = (ret == 0);
    
    return ret;
}

int video_android_instance_start_consumer (void *_self)
{
    video_android_instance_t *self = (video_android_instance_t *)_self;
    if (!self || !self->callback)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock (self);
    if (!self->isConsumerPrepared)
    {
        TSK_DEBUG_ERROR ("Consumer not prepared");
        goto done;
    }
    
    if (self->isConsumerStarted)
    {
        TSK_DEBUG_WARN ("Consumer already started");
        goto done;
    }

    self->isConsumerStarted = 1;
done:
    tsk_safeobj_unlock (self);
    return (self->isConsumerStarted ? 0 : -1);
}

int video_android_instance_start_producer (void *_self)
{
    video_android_instance_t *self = (video_android_instance_t *)_self;
    if (!self || !self->callback)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock (self);
    if (!self->isProducerPrepared)
    {
        TSK_DEBUG_ERROR ("Producer not prepared");
        goto done;
    }
    
    if (self->isProducerStarted)
    {
        TSK_DEBUG_WARN ("Consumer already started");
        goto done;
    }
#if 0 // TODO!!
    if (self->isRecordingAvailable)
    {
        int ret;
        if ((ret = self->device->StartRecording ()))
        {
            TSK_DEBUG_ERROR ("StartRecording() failed with error code = %d", ret);
        }
        
        self->isProducerStarted = self->device->Recording ();
        TSK_DEBUG_INFO ("isRecording=%s", (self->isProducerStarted ? "true" : "false"));
    }
#endif
    self->isProducerStarted = 1;
done:
    tsk_safeobj_unlock (self);
    return (self->isProducerStarted ? 0 : -1);
}

int video_android_instance_stop_consumer (void *_self)
{
    
    video_android_instance_t *self = (video_android_instance_t *)_self;
    if (!self || !self->callback)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock (self);
    
    if (!self->isConsumerStarted)
    {
        goto done;
    }

    self->isConsumerStarted = 0;
done:
    tsk_safeobj_unlock (self);
    return (self->isConsumerStarted ? -1 : 0);
}

int video_android_instance_stop_producer (void *_self)
{
    video_android_instance_t *self = (video_android_instance_t *)_self;
    
    if (!self || !self->callback)
    {
        TSK_DEBUG_ERROR ("invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock (self);
    
    if (!self->isProducerStarted)
    {
        goto done;
    }

    self->isProducerStarted = 0;
done:
    tsk_safeobj_unlock (self);
    return (self->isProducerStarted ? -1 : 0);
}


int video_android_instance_destroy (void **_self)
{
    if (!_self || !*_self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    tsk_list_lock (__videoInstances);
    if (tsk_object_get_refcount (*_self) == 1)
    {
        tsk_list_remove_item_by_data (__videoInstances, *_self);
    }
    else
    {
        tsk_object_unref (*_self);
    }
    tsk_list_unlock (__videoInstances);
    *_self = tsk_null;
    return 0;
}

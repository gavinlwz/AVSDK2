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
#include "audio_android.h"

//#include "audio_opensles_consumer.h"
#include "audio_android_producer.h"
//#include "audio_opensles_device.h"

#include "tinymedia/tmedia_consumer.h"
#include "tinymedia/tmedia_producer.h"
#include "audio_android_device_impl.h"

#include "tsk_list.h"
#include "tsk_safeobj.h"
#include "tsk_debug.h"

typedef enum PLUGIN_INDEX_E
{
    PLUGIN_INDEX_AUDIO_CONSUMER,
    PLUGIN_INDEX_AUDIO_PRODUCER,
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
    case PLUGIN_INDEX_AUDIO_CONSUMER:
        return tsk_plugin_def_type_consumer;
    case PLUGIN_INDEX_AUDIO_PRODUCER:
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
    case PLUGIN_INDEX_AUDIO_CONSUMER:
    case PLUGIN_INDEX_AUDIO_PRODUCER:
    {
        return tsk_plugin_def_media_type_audio;
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
    // TODO!!
    //case PLUGIN_INDEX_AUDIO_CONSUMER:
    //{
    //    return audio_consumer_opensles_plugin_def_t;
    //}
    case PLUGIN_INDEX_AUDIO_PRODUCER:
    {
        return audio_producer_android_plugin_def_t;
    }
    default:
    {
        TSK_DEBUG_ERROR ("No plugin at index %d", index);
        return tsk_null;
    }
    }
}

//
// SLES AudioInstance
//
typedef tsk_list_t audio_android_instances_L_t;

static audio_android_instances_L_t *__audioInstances = tsk_null;

static tsk_object_t *audio_android_instance_ctor (tsk_object_t *self, va_list *app)
{
    audio_android_instance_t *audioInstance = (audio_android_instance_t *)self;
    if (audioInstance)
    {
        tsk_safeobj_init (audioInstance);
    }
    return self;
}
static tsk_object_t *audio_android_instance_dtor (tsk_object_t *self)
{
    TSK_DEBUG_INFO ("Audio Instance destroyed");
    audio_android_instance_t *audioInstance = (audio_android_instance_t *)self;
    if (audioInstance)
    {
        tsk_safeobj_lock (audioInstance);
#if 0 // TODO!!
        if (audioInstance->device)
        {
            audioInstance->device->SetCallback (NULL);
            audioInstance->device->Terminate ();
            delete audioInstance->device;
            audioInstance->device = tsk_null;
        }
#endif
        if (audioInstance->callback)
        {
            delete (AudioAndroidDeviceCallbackImpl*)(audioInstance->callback);
            audioInstance->callback = tsk_null;
        }
        tsk_safeobj_unlock (audioInstance);

        tsk_safeobj_deinit (audioInstance);
    }
    return self;
}
static int audio_android_instance_cmp (const tsk_object_t *_ai1, const tsk_object_t *_ai2)
{
    return ((long)_ai1 - (long)_ai2);
}
static const tsk_object_def_t audio_android_instance_def_s = {
    sizeof (audio_android_instance_t),
    audio_android_instance_ctor,
    audio_android_instance_dtor,
    audio_android_instance_cmp,
};
const tsk_object_def_t *audio_android_instance_def_t = &audio_android_instance_def_s;


void *audio_android_instance_create (uint64_t sessionId, int isForRecording)
{

    audio_android_instance_t *audioInstance = tsk_null;

    // create list used to hold instances
    if (!__audioInstances && !(__audioInstances = tsk_list_create ()))
    {
        TSK_DEBUG_ERROR ("Failed to create new list");
        return tsk_null;
    }

    //= lock the list
    tsk_list_lock (__audioInstances);

    // find the instance from the list
    const tsk_list_item_t *item;
    tsk_list_foreach (item, __audioInstances)
    {
        if (((audio_android_instance_t *)item->data)->sessionId == sessionId)
        {
            audioInstance = (audio_android_instance_t *)tsk_object_ref (item->data);
            break;
        }
    }

    if (!audioInstance)
    {
        audio_android_instance_t *_audioInstance;
        if (!(_audioInstance = (audio_android_instance_t *)tsk_object_new (&audio_android_instance_def_s)))
        {
            TSK_DEBUG_ERROR ("Failed to create new audio instance");
            goto done;
        }
#if 0 // TODO!!
        if (!(_audioInstance->device = new SLAudioDevice (isForRecording ? true : false)))
        {
            TSK_DEBUG_ERROR ("Failed to create audio device");
            TSK_OBJECT_SAFE_FREE (_audioInstance);
            goto done;
        }
#endif
        if (!(_audioInstance->callback = new AudioAndroidDeviceCallbackImpl ()))
        {
            TSK_DEBUG_ERROR ("Failed to create audio transport");
            TSK_OBJECT_SAFE_FREE (_audioInstance);
            goto done;
        }
#if 0 // TODO!!
        if ((_audioInstance->device->SetCallback (_audioInstance->callback)))
        {
            TSK_DEBUG_ERROR ("AudioDeviceModule::RegisterAudioCallback() failed");
            TSK_OBJECT_SAFE_FREE (_audioInstance);
            goto done;
        }
        if ((_audioInstance->device->Init ()))
        {
            TSK_DEBUG_ERROR ("AudioDeviceModule::Init() failed");
            TSK_OBJECT_SAFE_FREE (_audioInstance);
            goto done;
        }
#endif
        _audioInstance->sessionId = sessionId;
        audioInstance = _audioInstance;
        tsk_list_push_back_data (__audioInstances, (void **)&_audioInstance);
    }

done:
    //= unlock the list
    tsk_list_unlock (__audioInstances);

    return (void *)audioInstance;
}

int audio_android_instance_prepare_consumer (void *_self, tmedia_consumer_t **_consumer)
{
    audio_android_instance_t *self = (audio_android_instance_t *)_self;
    AudioAndroidDeviceCallbackImpl *cbFunc = NULL;
    
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
    
    cbFunc = (AudioAndroidDeviceCallbackImpl*)self->callback;
    cbFunc->SetConsumer ((const struct audio_consumer_android_s *)*_consumer);
    
done:
    tsk_safeobj_unlock (self);

    self->isConsumerPrepared = (ret == 0);

    return ret;
}

int audio_android_instance_prepare_producer (void *_self, tmedia_producer_t **_producer)
{
    audio_android_instance_t *self = (audio_android_instance_t *)_self;
    AudioAndroidDeviceCallbackImpl *cbFunc = NULL;
    // TODO!! if (!self || !self->device || !self->callback || !_producer || !*_producer)
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

    cbFunc = (AudioAndroidDeviceCallbackImpl*)self->callback;
    cbFunc->SetProducer ((const struct audio_producer_android_s *)*_producer);

done:
    tsk_safeobj_unlock (self);

    self->isProducerPrepared = (ret == 0);

    return ret;
}

int audio_android_instance_start_consumer (void *_self)
{
    audio_android_instance_t *self = (audio_android_instance_t *)_self;
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
#if 0 // TODO!!
    if (self->isPlayoutAvailable)
    {
        int ret;
        if ((ret = self->device->StartPlayout ()))
        {
            TSK_DEBUG_ERROR ("StartPlayout() failed with error code = %d", ret);
        }

        self->isConsumerStarted = self->device->Playing ();
        TSK_DEBUG_INFO ("isPlaying=%s", (self->isConsumerPrepared ? "true" : "false"));
    }
#endif
    self->isConsumerStarted = 1;
done:
    tsk_safeobj_unlock (self);
    return (self->isConsumerStarted ? 0 : -1);
}

int audio_android_instance_start_producer (void *_self)
{
    audio_android_instance_t *self = (audio_android_instance_t *)_self;
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

int audio_android_instance_stop_consumer (void *_self)
{

    audio_android_instance_t *self = (audio_android_instance_t *)_self;
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
#if 0 // TODO!!
    int ret;
    if ((ret = self->device->StopPlayout ()))
    {
        TSK_DEBUG_ERROR ("StopPlayout() failed with error code = %d", ret);
    }
    else
    {
        self->isConsumerStarted = self->device->Playing ();
        self->isConsumerPrepared = false;
    }
#endif
    self->isConsumerStarted = 0;
done:
    tsk_safeobj_unlock (self);
    return (self->isConsumerStarted ? -1 : 0);
}

int audio_android_instance_stop_producer (void *_self)
{
    audio_android_instance_t *self = (audio_android_instance_t *)_self;
    
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
#if 0 // TODO!!
    int ret;
    if ((ret = self->device->StopRecording ()))
    {
        TSK_DEBUG_ERROR ("StopRecording() failed with error code = %d", ret);
    }
    else
    {
        self->isProducerStarted = self->device->Recording ();
        self->isProducerPrepared = false;
    }
#endif
    self->isProducerStarted = 0;
done:
    tsk_safeobj_unlock (self);
    return (self->isProducerStarted ? -1 : 0);
}

#if 0 // TODO!!
int audio_opensles_instance_set_speakerOn (audio_opensles_instance_handle_t *_self, tsk_bool_t speakerOn)
{

    audio_opensles_instance_t *self = (audio_opensles_instance_t *)_self;
    if (!self || !self->device)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->device->SetSpeakerOn (speakerOn);
}

int audio_opensles_instance_set_microphone_volume (audio_opensles_instance_handle_t *_self, int32_t volume)
{
    audio_opensles_instance_t *self = (audio_opensles_instance_t *)_self;
    if (!self || !self->device)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return self->device->SetMicrophoneVolume (volume);
}
#endif

int audio_android_instance_destroy (void **_self)
{
    if (!_self || !*_self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    tsk_list_lock (__audioInstances);
    if (tsk_object_get_refcount (*_self) == 1)
    {
        tsk_list_remove_item_by_data (__audioInstances, *_self);
    }
    else
    {
        tsk_object_unref (*_self);
    }
    tsk_list_unlock (__audioInstances);
    *_self = tsk_null;
    return 0;
}

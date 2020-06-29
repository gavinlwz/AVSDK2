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
#include "audio_android.h"
#include "audio_android_consumer.h"

#include "tinydav/audio/tdav_consumer_audio.h"
#include "tinymedia/tmedia_defaults.h"

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"


int audio_consumer_android_get_data_20ms (const audio_consumer_android_t *_self, void *audioSamples, int nSamples, int nBytesPerSample, int nChannels, int samplesPerSec)
{
    int nSamplesOut = 0;
    if (!_self || !audioSamples || !nSamples)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if ((nSamples != (samplesPerSec / 100 * 2))) // 20ms data
    {
        TSK_DEBUG_ERROR ("Not producing 10ms samples (nSamples=%d, samplesPerSec=%d)", nSamples, samplesPerSec);
        return -2;
    }
    if ((nBytesPerSample != (TMEDIA_CONSUMER (_self)->audio.bits_per_sample >> 3)))
    {
        TSK_DEBUG_ERROR ("%d not valid bytes/samples", nBytesPerSample);
        return -3;
    }
    if ((nChannels != TMEDIA_CONSUMER (_self)->audio.in.channels))
    {
        TSK_DEBUG_ERROR ("Playout - %d not the expected number of channels but should be %d", nChannels, TMEDIA_CONSUMER (_self)->audio.in.channels);
        return -4;
    }

    audio_consumer_android_t *self = const_cast<audio_consumer_android_t *> (_self);

    if (self->buffer.index == self->buffer.size)
    {
        if ((tdav_consumer_audio_get (TDAV_CONSUMER_AUDIO (self), self->buffer.ptr, self->buffer.size)) != self->buffer.size)
        {
            self->buffer.index = self->buffer.size;
            return 0;
        }
        self->buffer.index = 0;
        tdav_consumer_audio_tick (TDAV_CONSUMER_AUDIO (self));
    }
    
    int nSamplesInBytes = (nSamples * nBytesPerSample);
    if (_self->buffer.index + nSamplesInBytes <= _self->buffer.size)
    {
        //如果扬声器静音，就赋值全零数据
        if (_self->isSpeakerMute)
        {
            memset (audioSamples, 0, nSamplesInBytes);
        }
        else
        {
            memcpy (audioSamples, (((uint8_t *)self->buffer.ptr) + self->buffer.index), nSamplesInBytes);
        }
    }
    else
    {
        TSK_DEBUG_ERROR ("_self->buffer.index(%d) + nSamplesInBits(%d) > _self->buffer.size(%d)", _self->buffer.index, nSamplesInBytes, _self->buffer.size);
    }
    self->buffer.index += nSamplesInBytes;
    TSK_CLAMP (0, self->buffer.index, self->buffer.size);
    nSamplesOut = nSamples;
    
    return nSamplesOut;
}

#if 0 // TODO!!
tsk_bool_t audio_consumer_opensles_is_speakerOn (const audio_consumer_opensles_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return false;
    }
    return self->isSpeakerOn;
}
#endif

/* ============ Media Consumer Interface ================= */
static int audio_consumer_android_set_param (tmedia_consumer_t *_self, const tmedia_param_t *param)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    int ret = tdav_consumer_audio_set_param (TDAV_CONSUMER_AUDIO (self), param);

    if (ret == 0)
    {
        if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_VOLUME))
        {
        }
        else if (tsk_striequals (param->key, "speaker-on"))
        {
#if 0
            self->isSpeakerOn = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
            if (self->audioInstHandle)
            {
                return audio_opensles_instance_set_speakerOn (self->audioInstHandle, self->isSpeakerOn);
            }
            else
            {
                return 0; // will be set when instance is initialized
            }
#endif
            return 0;
        }
    }

    return ret;
}

static int audio_consumer_android_get_param (tmedia_consumer_t *_self, tmedia_param_t *param)
{
    return tdav_consumer_audio_get_param (TDAV_CONSUMER_AUDIO (_self), param);
}

static int audio_consumer_android_prepare (tmedia_consumer_t *_self, const tmedia_codec_t *codec)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    // create audio instance
    if (!(self->audioInstHandle = audio_android_instance_create (TMEDIA_CONSUMER (self)->session_id, 0)))
    {
        TSK_DEBUG_ERROR ("Failed to create audio instance handle");
        return -1;
    }

    // initialize input parameters from the codec information
    TMEDIA_CONSUMER (self)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_DECODING (codec);
    TMEDIA_CONSUMER (self)->audio.in.channels = TMEDIA_CODEC_CHANNELS_AUDIO_DECODING (codec);

    TSK_DEBUG_INFO ("audio_consumer_android_prepare(channels=%d, rate=%d, ptime=%d)", TMEDIA_CONSUMER (self)->audio.in.channels, TMEDIA_CONSUMER (self)->audio.out.rate, TMEDIA_CONSUMER (self)->audio.ptime);

    // prepare playout device and update output parameters
    int ret = audio_android_instance_prepare_consumer (self->audioInstHandle, &_self);

    // now that the producer is prepared we can initialize internal buffer using device caps
    if (ret == 0)
    {
        int isStreamVoice = 0;
        if (tmedia_defaults_get_comm_mode_enabled())
        {
            isStreamVoice = 1;
        }
        JNI_Init_Audio_Player(TMEDIA_CONSUMER (self)->audio.out.rate,
                              TMEDIA_CONSUMER (self)->audio.in.channels,
                              TMEDIA_CONSUMER (self)->audio.bits_per_sample >> 3,
                              isStreamVoice,
                              _self);
        
        // allocate buffer
        int xsize = ((TMEDIA_CONSUMER (self)->audio.ptime * TMEDIA_CONSUMER (self)->audio.out.rate) / 1000) * (TMEDIA_CONSUMER (self)->audio.bits_per_sample >> 3);
        TSK_DEBUG_INFO ("consumer buffer xsize = %d", xsize);
        if (!(self->buffer.ptr = tsk_realloc (self->buffer.ptr, xsize)))
        {
            TSK_DEBUG_ERROR ("Failed to allocate buffer with size = %d", xsize);
            self->buffer.size = 0;
            return -1;
        }
        memset (self->buffer.ptr, 0, xsize);
        self->buffer.size = xsize;
        self->buffer.index = 0;
        self->buffer.isFull = false;
    }
    return ret;
}

static int audio_consumer_android_start (tmedia_consumer_t *_self)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    TSK_DEBUG_INFO ("audio_consumer_android_start");
    
    audio_android_instance_start_consumer (self->audioInstHandle);
    
    JNI_Start_Audio_Player();
    
    return 0;
}

static int audio_consumer_android_consume (tmedia_consumer_t *_self, const void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (!self || !data || !data_size)
    {
        TSK_DEBUG_ERROR ("1Invalid parameter");
        return -1;
    }
    /* buffer is already decoded */
    return tdav_consumer_audio_put (TDAV_CONSUMER_AUDIO (self), data, data_size, proto_hdr);
}

static int audio_consumer_android_pause (tmedia_consumer_t *self)
{
    return 0;
}

static int audio_consumer_android_stop (tmedia_consumer_t *_self)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    audio_android_instance_stop_consumer (self->audioInstHandle);
    
    JNI_Stop_Audio_Player();
    
    return 0;
}


//
//	SLES audio consumer object definition
//
/* constructor */
static tsk_object_t *audio_consumer_android_ctor (tsk_object_t *_self, va_list *app)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (self)
    {
        /* init base */
        tdav_consumer_audio_init (TDAV_CONSUMER_AUDIO (self));
        /* init self */
        self->isSpeakerMute = false;
    }
    return self;
}
/* destructor */
static tsk_object_t *audio_consumer_android_dtor (tsk_object_t *_self)
{
    audio_consumer_android_t *self = (audio_consumer_android_t *)_self;
    if (self)
    {
        /* stop */
        audio_consumer_android_stop (TMEDIA_CONSUMER (self));
        /* deinit self */
        if (self->audioInstHandle)
        {
            audio_android_instance_destroy (&self->audioInstHandle);
        }
        TSK_FREE (self->buffer.ptr);
        /* deinit base */
        tdav_consumer_audio_deinit (TDAV_CONSUMER_AUDIO (self));
    }

    return self;
}
/* object definition */
static const tsk_object_def_t audio_consumer_android_def_s = {
    sizeof (audio_consumer_android_t), audio_consumer_android_ctor, audio_consumer_android_dtor, tdav_consumer_audio_cmp,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t audio_consumer_android_plugin_def_s = { &audio_consumer_android_def_s,

                                                                                   tmedia_audio,
                                                                                   "ANDROID audio consumer",

                                                                                   audio_consumer_android_set_param,
                                                                                   audio_consumer_android_get_param,
                                                                                   audio_consumer_android_prepare,
                                                                                   audio_consumer_android_start,
                                                                                   audio_consumer_android_consume,
                                                                                   audio_consumer_android_pause,
                                                                                   audio_consumer_android_stop };
const tmedia_consumer_plugin_def_t *audio_consumer_android_plugin_def_t = &audio_consumer_android_plugin_def_s;

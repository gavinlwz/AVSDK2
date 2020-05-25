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
#include "audio_opensles_producer.h"
#include "audio_opensles.h"

#include "tinydav/audio/tdav_producer_audio.h"

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tinymedia/tmedia_defaults.h"


int audio_producer_opensles_handle_data_10ms (const audio_producer_opensles_t *_self, const void *audioSamples, int nSamples, int nBytesPerSample, int samplesPerSec, int nChannels)
{
    if (!_self || !audioSamples || !nSamples)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!TMEDIA_PRODUCER (_self)->enc_cb.callback)
    {
        TSK_DEBUG_WARN ("No callback function is registered for the producer");
        return 0;
    }
    if ((nSamples != (samplesPerSec / 100)))
    {
        TSK_DEBUG_ERROR ("Not producing 10ms samples (nSamples=%d, samplesPerSec=%d)", nSamples, samplesPerSec);
        return -2;
    }
    if ((nBytesPerSample != (TMEDIA_PRODUCER (_self)->audio.bits_per_sample >> 3)))
    {
        TSK_DEBUG_ERROR ("%d not valid bytes/samples", nBytesPerSample);
        return -3;
    }
    if ((nChannels != TMEDIA_PRODUCER (_self)->audio.channels))
    {
        TSK_DEBUG_ERROR ("Recording - %d not the expected number of channels but should be %d", nChannels, TMEDIA_PRODUCER (_self)->audio.channels);
        return -4;
    }

    int nSamplesInBits = (nSamples * nBytesPerSample);
    if (_self->buffer.index + nSamplesInBits > _self->buffer.size)
    {
        TSK_DEBUG_ERROR ("Buffer overflow");
        return -5;
    }

    audio_producer_opensles_t *self = const_cast<audio_producer_opensles_t *> (_self);

    if (self->isMuted)
    {
        memset ((((uint8_t *)self->buffer.ptr) + self->buffer.index), 0, nSamplesInBits);
    }
    else
    {
        memcpy ((((uint8_t *)self->buffer.ptr) + self->buffer.index), audioSamples, nSamplesInBits);
    }
    self->buffer.index += nSamplesInBits;

    if (self->buffer.index == self->buffer.size)
    {
        self->buffer.index = 0;
        TMEDIA_PRODUCER (self)->enc_cb.callback (TMEDIA_PRODUCER (self)->enc_cb.callback_data, self->buffer.ptr, self->buffer.size);
    }

    return 0;
}


/* ============ Media Producer Interface ================= */
static int audio_producer_opensles_set (tmedia_producer_t *_self, const tmedia_param_t *param)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MICROPHONE_MUTE))
            {
                self->isMuted = (*((int32_t *)param->value) != 0);
                TSK_DEBUG_INFO ("Set mic mute:%d", self->isMuted);
                return 0;
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_VOLUME))
            {
                return audio_opensles_instance_set_microphone_volume (self->audioInstHandle, *((int32_t *)param->value));
            }
        }
    }
    return tdav_producer_audio_set (TDAV_PRODUCER_AUDIO (self), param);
}

static int audio_producer_opensles_get (tmedia_producer_t *_self, tmedia_param_t *param)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR)) {
                return audio_opensles_instance_get_recording_error (self->audioInstHandle, (int32_t*)param->value);
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA)) {
                return audio_opensles_instance_get_recording_error_extra (self->audioInstHandle, (int32_t*)param->value);
            }
        }
    }
    
    return -1;
}

static int audio_producer_opensles_prepare (tmedia_producer_t *_self, const tmedia_codec_t *codec)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (!self || !codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    // create audio instance
    if (!(self->audioInstHandle = audio_opensles_instance_create (TMEDIA_PRODUCER (self)->session_id)))
    {
        TSK_DEBUG_ERROR ("Failed to create audio instance handle");
        return -2;
    }

    // check that ptime is mutiple of 10
    if ((codec->plugin->audio.ptime % 10))
    {
        TSK_DEBUG_ERROR ("ptime=%d not multiple of 10", codec->plugin->audio.ptime);
        return -3;
    }

    // init input parameters from the codec
    TMEDIA_PRODUCER (self)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (codec);
    TMEDIA_PRODUCER (self)->audio.rate = TMEDIA_CODEC_RATE_ENCODING (codec);
    TMEDIA_PRODUCER (self)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING (codec);

    TSK_DEBUG_INFO ("audio_producer_opensles_prepare(channels=%d, rate=%d, ptime=%d)", TMEDIA_PRODUCER (self)->audio.channels,
                                                                                      TMEDIA_PRODUCER (self)->audio.rate,
                                                                                      TMEDIA_PRODUCER (self)->audio.ptime);

    // prepare playout device and update output parameters
    int ret;
    ret = audio_opensles_instance_prepare_producer (self->audioInstHandle, &_self);

    // now that the producer is prepared we can initialize internal buffer using device caps
    if (ret == 0)
    {
        // allocate buffer
        int xsize = ((TMEDIA_PRODUCER (self)->audio.ptime * TMEDIA_PRODUCER (self)->audio.rate) / 1000) * (TMEDIA_PRODUCER (self)->audio.bits_per_sample >> 3);
        TSK_DEBUG_INFO ("producer buffer xsize = %d", xsize);
        if (!(self->buffer.ptr = tsk_realloc (self->buffer.ptr, xsize)))
        {
            TSK_DEBUG_ERROR ("Failed to allocate buffer with size = %d", xsize);
            self->buffer.size = 0;
            return -1;
        }
        self->buffer.size = xsize;
        self->buffer.index = 0;
    }
    return audio_opensles_instance_set_microphone_volume (self->audioInstHandle, 255);
}

static int audio_producer_opensles_start (tmedia_producer_t *_self)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    TSK_DEBUG_INFO ("audio_producer_opensles_start");

    return audio_opensles_instance_start_producer (self->audioInstHandle);
}

static int audio_producer_opensles_pause (tmedia_producer_t *self)
{
    return 0;
}

static int audio_producer_opensles_stop (tmedia_producer_t *_self)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    return audio_opensles_instance_stop_producer (self->audioInstHandle);
}


//
//	SLES audio producer object definition
//
/* constructor */
static tsk_object_t *audio_producer_opensles_ctor (tsk_object_t *_self, va_list *app)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (self)
    {
        /* init base */
        tdav_producer_audio_init (TDAV_PRODUCER_AUDIO (self));
        /* init self */
        self->isMuted = 1;
        TSK_DEBUG_INFO ("Initial mic mute:%d", self->isMuted);

    }
    return self;
}
/* destructor */
static tsk_object_t *audio_producer_opensles_dtor (tsk_object_t *_self)
{
    audio_producer_opensles_t *self = (audio_producer_opensles_t *)_self;
    if (self)
    {
        /* stop */
        audio_producer_opensles_stop (TMEDIA_PRODUCER (self));
        /* deinit self */
        if (self->audioInstHandle)
        {
            audio_opensles_instance_destroy (&self->audioInstHandle);
        }
        TSK_FREE (self->buffer.ptr);

        /* deinit base */
        tdav_producer_audio_deinit (TDAV_PRODUCER_AUDIO (self));
    }

    return self;
}
/* object definition */
static const tsk_object_def_t audio_producer_opensles_def_s = {
    sizeof (audio_producer_opensles_t),
    audio_producer_opensles_ctor,
    audio_producer_opensles_dtor,
    tdav_producer_audio_cmp,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t audio_producer_opensles_plugin_def_s = { &audio_producer_opensles_def_s,

                                                                                   tmedia_audio,
                                                                                   "SLES audio producer",

                                                                                   audio_producer_opensles_set,
                                                                                   audio_producer_opensles_get,
                                                                                   audio_producer_opensles_prepare,
                                                                                   audio_producer_opensles_start,
                                                                                   audio_producer_opensles_pause,
                                                                                   audio_producer_opensles_stop };
const tmedia_producer_plugin_def_t *audio_producer_opensles_plugin_def_t = &audio_producer_opensles_plugin_def_s;
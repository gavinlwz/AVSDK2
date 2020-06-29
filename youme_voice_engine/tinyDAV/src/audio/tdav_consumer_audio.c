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

/**@file tdav_consumer_audio.c
* @brief Base class for all Audio consumers.
*/
#include "tinydav/audio/tdav_consumer_audio.h"
#ifdef TDAV_UNDER_APPLE
#include "tdav_apple.h"
#endif

#include "tinymedia/tmedia_defaults.h"
#include "tinymedia/tmedia_denoise.h"
#include "tinymedia/tmedia_jitterbuffer.h"
#include "tinyrtp/rtp/trtp_rtp_header.h"

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_time.h"
#include "tsk_debug.h"
#include "tinydav/audio/tdav_session_audio.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"

#if TSK_UNDER_WINDOWS
#include <Winsock2.h> // timeval
#elif defined(__SYMBIAN32__)
#include <_timeval.h>
#else
#include <sys/time.h>
#endif

#define TDAV_BITS_PER_SAMPLE_DEFAULT 16
//float格式下的sample bits
#define TDAV_BITS_PER_SAMPLE_FLOAT  32
#define TDAV_CHANNELS_DEFAULT 2
#define TDAV_RATE_DEFAULT 8000
#define TDAV_PTIME_DEFAULT 20

#define TDAV_AUDIO_GAIN_MAX 15

/** Initialize audio consumer */
int tdav_consumer_audio_init (tdav_consumer_audio_t *self)
{
    int ret;

    TSK_DEBUG_INFO ("tdav_consumer_audio_init()");

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    /* base */
    if ((ret = tmedia_consumer_init (TMEDIA_CONSUMER (self))))
    {
        return ret;
    }

    /* self (should be update by prepare() by using the codec's info)*/
    //mac下16位的播放不成功
#ifndef MAC_OS
    TMEDIA_CONSUMER (self)->audio.bits_per_sample = TDAV_BITS_PER_SAMPLE_DEFAULT;
#else
    TMEDIA_CONSUMER (self)->audio.bits_per_sample = TDAV_BITS_PER_SAMPLE_FLOAT;
#endif
    
    TMEDIA_CONSUMER (self)->audio.ptime = TDAV_PTIME_DEFAULT;
    TMEDIA_CONSUMER (self)->audio.in.channels = TDAV_CHANNELS_DEFAULT;
    TMEDIA_CONSUMER (self)->audio.in.rate = tmedia_defaults_get_record_sample_rate ();
    TMEDIA_CONSUMER (self)->audio.out.rate = tmedia_defaults_get_playback_sample_rate ();
    TMEDIA_CONSUMER (self)->audio.gain = TSK_MIN (tmedia_defaults_get_audio_consumer_gain (), TDAV_AUDIO_GAIN_MAX);
    TMEDIA_CONSUMER (self)->audio.volume = 1.0f;
    TMEDIA_CONSUMER (self)->isSpeakerMute = tsk_true;

	self->pvPureVoice = tsk_malloc(TMEDIA_CONSUMER (self)->audio.in.rate / 100 * 2 * sizeof(int16_t));
    
    self->output_frame_count = 0;
    
    TMEDIA_CONSUMER (self)->audio.fadeInVolume = 0;
    TMEDIA_CONSUMER (self)->audio.fadeInStep = 0;
    TMEDIA_CONSUMER (self)->audio.fadeInTime = tmedia_defaults_get_spkout_fade_time(); // default 6s fade in
    if (TMEDIA_CONSUMER (self)->audio.fadeInTime != 0) {
        TMEDIA_CONSUMER (self)->audio.fadeInTime = (TMEDIA_CONSUMER (self)->audio.fadeInTime < 20) ? 20 : TMEDIA_CONSUMER (self)->audio.fadeInTime;
        TMEDIA_CONSUMER (self)->audio.fadeInStep = 0x7fffffff / TMEDIA_CONSUMER (self)->audio.fadeInTime * TMEDIA_CONSUMER (self)->audio.ptime;
    }

    tsk_safeobj_init (self);

    return 0;
}

/**
* Generic function to compare two consumers.
* @param consumer1 The first consumer to compare.
* @param consumer2 The second consumer to compare.
* @retval Returns an integral value indicating the relationship between the two consumers:
* <0 : @a consumer1 less than @a consumer2.<br>
* 0  : @a consumer1 identical to @a consumer2.<br>
* >0 : @a consumer1 greater than @a consumer2.<br>
*/
int tdav_consumer_audio_cmp (const tsk_object_t *consumer1, const tsk_object_t *consumer2)
{
    int ret;
    tsk_subsat_int32_ptr (consumer1, consumer2, &ret);
    return ret;
}

int tdav_consumer_audio_set_param (tdav_consumer_audio_t *self, const tmedia_param_t *param)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (param->plugin_type == tmedia_ppt_consumer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, "gain"))
            {
                int32_t gain = *((int32_t *)param->value);
                if (gain < TDAV_AUDIO_GAIN_MAX && gain >= 0)
                {
                    TMEDIA_CONSUMER (self)->audio.gain = (uint8_t)gain;
                    TSK_DEBUG_INFO ("audio consumer gain=%u", gain);
                }
                else
                {
                    TSK_DEBUG_ERROR ("%u is invalid as gain value", gain);
                    return -2;
                }
            }
            else if (tsk_striequals (param->key, "speaker-on"))
            {
#if TARGET_OS_IPHONE
                tdav_apple_resetOutputTarget (TSK_TO_INT32 ((uint8_t *)param->value));
#endif
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SPEAKER_MUTE))
            {
                TMEDIA_CONSUMER (self)->isSpeakerMute = TSK_TO_INT32 ((uint8_t *)param->value);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SPEAKER_VOLUME))
            {
                float v = ((float)TSK_TO_INT32 ((uint8_t *)param->value)) / 100.0f;
                TMEDIA_CONSUMER (self)->audio.volume = v;
            }
        }
    } else if (param->plugin_type == tmedia_ppt_session) {
        if (param->value_type == tmedia_pvt_int32) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_ENABLED)) {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME)) {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MAX_FAREND_VOICE_LEVEL)) {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_FLAG))
            {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_CHANNEL))
            {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_SAMPLE_RATE))
            {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            else if( tsk_striequals( param->key, TMEDIA_PARAM_KEY_TRANSCRIBER_ENABLED ))
            {
                tmedia_jitterbuffer_set_param( TMEDIA_JITTER_BUFFER( self->jitterbuffer), param );
            }
            else if( tsk_striequals (param->key, TMEDIA_PARAM_KEY_SAVE_SCREEN) )
            {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
        } else if (param->value_type == tmedia_pvt_pvoid) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK)) {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_FAREND_VOICE_LEVEL_CALLBACK)) {
                tmedia_jitterbuffer_set_param (TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
            }
        }
    }

    return 0;
}

int tdav_consumer_audio_get_param (tdav_consumer_audio_t *self, tmedia_param_t *param)
{
    int ret = 0;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    if (tmedia_ppt_jitterbuffer == param->plugin_type) {
        tsk_safeobj_lock (self);
        ret = tmedia_jitterbuffer_get_param(TMEDIA_JITTER_BUFFER (self->jitterbuffer), param);
        tsk_safeobj_unlock (self);
        return ret;
    }
    
    return 0;
}

/* put data (bytes not shorts) into the jitter buffer (consumers always have ptime of 20ms) */
int tdav_consumer_audio_put (tdav_consumer_audio_t *self, const void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{
    int ret = 0;

    if (!self || !data || !self->jitterbuffer || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock (self);

    if (!TMEDIA_JITTER_BUFFER (self->jitterbuffer)->opened)
    {
        uint32_t in_rate  = TMEDIA_CONSUMER (self)->audio.in.rate; // Todo, should be the decoder output sample rate!
        uint32_t out_rate = TMEDIA_CONSUMER (self)->audio.out.rate;
        uint32_t channels = TMEDIA_CONSUMER (self)->audio.out.channels ? TMEDIA_CONSUMER (self)->audio.out.channels : tmedia_defaults_get_audio_channels_playback ();
        if ((ret = tmedia_jitterbuffer_open (self->jitterbuffer, TMEDIA_CONSUMER (self)->audio.ptime, in_rate, out_rate, channels)))
        {
            TSK_DEBUG_ERROR ("Failed to open jitterbuffer (%d)", ret);
            tsk_safeobj_unlock (self);
            return ret;
        }
    }

    trtp_rtp_header_t *header = (trtp_rtp_header_t *)proto_hdr;
    if (header->bkaud_source == BKAUD_SOURCE_NULL) {
        ret = tmedia_jitterbuffer_put (self->jitterbuffer, (void *)data, data_size, proto_hdr);
    }

    tsk_safeobj_unlock (self);
    
    // Must move this part outside of obj lock, otherwise will dead lock in jitterbuffer
    if ((header->bkaud_source == BKAUD_SOURCE_GAME) || (header->bkaud_source == BKAUD_SOURCE_MICBYPASS) || (header->bkaud_source == BKAUD_SOURCE_PCMCALLBACK)) {
        ret = tmedia_jitterbuffer_put_bkaud(self->jitterbuffer, (void *)data, proto_hdr);
    }

    return ret;
}

/* get data from the jitter buffer (consumers should always have ptime of 20ms) */
tsk_size_t tdav_consumer_audio_get (tdav_consumer_audio_t *self, void *out_data, tsk_size_t out_size)
{
    tsk_size_t ret_size = 0;
    uint32_t frame_duration, channels;
    uint32_t in_rate = 16000, out_rate = 16000;
    void* pureVoiceBuf = NULL;
    
    if (!self || !self->jitterbuffer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    tsk_safeobj_lock (self);

    frame_duration = TMEDIA_CONSUMER (self)->audio.ptime;
    in_rate        = TMEDIA_CONSUMER (self)->audio.in.rate;
    out_rate       = TMEDIA_CONSUMER (self)->audio.out.rate;
    channels       = TMEDIA_CONSUMER (self)->audio.out.channels ? TMEDIA_CONSUMER (self)->audio.out.channels : tmedia_defaults_get_audio_channels_playback ();
    if (!TMEDIA_JITTER_BUFFER (self->jitterbuffer)->opened)
    {
        int ret;
        if ((ret = tmedia_jitterbuffer_open (TMEDIA_JITTER_BUFFER (self->jitterbuffer), frame_duration, in_rate, out_rate, channels)))
        {
            TSK_DEBUG_ERROR ("Failed to open jitterbuffer (%d)", ret);
            tsk_safeobj_unlock (self);
            return 0;
        }
    }
    
    ret_size = tmedia_jitterbuffer_get (TMEDIA_JITTER_BUFFER (self->jitterbuffer), out_data, self->pvPureVoice, out_size);

    tsk_safeobj_unlock (self);

    if (ret_size <= 0)
    {
        // TSK_DEBUG_WARN ("Failed to get jitterbuffer (%d)", ret_size);
        return ret_size;
    }

    // Apply the volume gain before feeding back to the preprocessing modules.
    tdav_codec_apply_volume_gain(TMEDIA_CONSUMER (self)->audio.volume, out_data, out_size, 2);
    
    // Apply the fade in gain before farend voice starting output
    if ((TMEDIA_CONSUMER (self)->audio.fadeInVolume < 0x7fffffff) && (TMEDIA_CONSUMER (self)->audio.fadeInTime != 0)) {
        int32_t *pTemp = (int32_t*)&(TMEDIA_CONSUMER (self)->audio.fadeInVolume);
        //TSK_DEBUG_INFO("[paul debug] (0x%x, 0x%x)", TMEDIA_CONSUMER (self)->audio.fadeInVolume, TMEDIA_CONSUMER (self)->audio.fadeInStep);
        if ((uint32_t)(TMEDIA_CONSUMER (self)->audio.fadeInVolume + TMEDIA_CONSUMER (self)->audio.fadeInStep) <= 0x7fffffff) {
            tdav_codec_volume_smooth_gain(out_data, out_size / sizeof(int16_t), out_size / sizeof(int16_t), TMEDIA_CONSUMER (self)->audio.fadeInVolume + TMEDIA_CONSUMER (self)->audio.fadeInStep, pTemp);
        } else {
            tdav_codec_volume_smooth_gain(out_data, out_size / sizeof(int16_t), out_size / sizeof(int16_t), 0x7fffffff, pTemp);
        }
    }
    
    // denoiser
    if (self->denoise && self->denoise->opened)
    {
        uint32_t size_pureVoice = ret_size * in_rate / out_rate;
        
        // Echo process last frame
        if ((self->pvPureVoice) && (size_pureVoice))
        {
            tmedia_denoise_echo_playback (self->denoise, out_data, ret_size, self->pvPureVoice, size_pureVoice);
        }
    }

    if (TMEDIA_CONSUMER (self)->isSpeakerMute) {
        memset ((void *)out_data, 0, out_size);
    }
    
    if ( ((self->output_frame_count < 1000) && ((self->output_frame_count % 200) == 0))
        || ((self->output_frame_count >= 1000) && ((self->output_frame_count % 1000) == 0)) ) {
        if (out_size >= 16) {
            uint8_t *tmpBuf = (uint8_t*)out_data;
            TSK_DEBUG_INFO("Speaker: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                           tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3], tmpBuf[4], tmpBuf[5], tmpBuf[6], tmpBuf[7],
                           tmpBuf[8], tmpBuf[9], tmpBuf[10], tmpBuf[11], tmpBuf[12], tmpBuf[13], tmpBuf[14], tmpBuf[15]);
        }
    }
    self->output_frame_count++;
    
    return ret_size;
}

int tdav_consumer_audio_tick (tdav_consumer_audio_t *self)
{
    if (!self || !self->jitterbuffer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    return tmedia_jitterbuffer_tick (TMEDIA_JITTER_BUFFER (self->jitterbuffer));
}

/* set denioiser */
void tdav_consumer_audio_set_denoise (tdav_consumer_audio_t *self, struct tmedia_denoise_s *denoise)
{
    tsk_safeobj_lock (self);
    TSK_OBJECT_SAFE_FREE (self->denoise);
    self->denoise = (struct tmedia_denoise_s *)tsk_object_ref (denoise);
    tsk_safeobj_unlock (self);
}

void tdav_consumer_audio_set_jitterbuffer (tdav_consumer_audio_t *self, struct tmedia_jitterbuffer_s *jitterbuffer)
{
    tsk_safeobj_lock (self);
    TSK_OBJECT_SAFE_FREE (self->jitterbuffer);
    self->jitterbuffer = (struct tmedia_jitterbuffer_s *)tsk_object_ref (jitterbuffer);
    tsk_safeobj_unlock (self);
}

/** Reset jitterbuffer */
int tdav_consumer_audio_reset (tdav_consumer_audio_t *self)
{
    int ret;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock (self);
    ret = tmedia_jitterbuffer_reset (TMEDIA_JITTER_BUFFER (self->jitterbuffer));
    tsk_safeobj_unlock (self);

    return ret;
}

/* tsk_safeobj_lock(self); */
/* tsk_safeobj_unlock(self); */

/** DeInitialize audio consumer */
int tdav_consumer_audio_deinit (tdav_consumer_audio_t *self)
{
    int ret;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* base */
    if ((ret = tmedia_consumer_deinit (TMEDIA_CONSUMER (self))))
    {
        /* return ret; */
    }

    /* self */
    TSK_OBJECT_SAFE_FREE (self->denoise);
    TSK_OBJECT_SAFE_FREE (self->resampler);
    TSK_OBJECT_SAFE_FREE (self->jitterbuffer);
    
    if (self->pvPureVoice) {
        TSK_SAFE_FREE(self->pvPureVoice);
        self->pvPureVoice = NULL;
    }
    
    tsk_safeobj_deinit (self);

    return 0;
}

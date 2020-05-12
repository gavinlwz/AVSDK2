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
#include "tinydav/audio/tdav_producer_audio.h"

#include "tinymedia/tmedia_defaults.h"

#define TDAV_PRODUCER_BITS_PER_SAMPLE_DEFAULT 16
#define TDAV_PRODUCER_CHANNELS_DEFAULT 1
#define TDAV_PRODUCER_RATE_DEFAULT 8000
#define TDAV_PRODUCER_PTIME_DEFAULT 20
#define TDAV_PRODUCER_AUDIO_GAIN_MAX 15

#include "tsk_string.h"
#include "tsk_debug.h"

/** Initialize Audio producer
* @param self The producer to initialize
*/
int tdav_producer_audio_init (tdav_producer_audio_t *self)
{
    int ret;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    /* base */
    if ((ret = tmedia_producer_init (TMEDIA_PRODUCER (self))))
    {
        return ret;
    }

    /* self (should be update by prepare() by using the codec's info)*/
    TMEDIA_PRODUCER (self)->audio.bits_per_sample = TDAV_PRODUCER_BITS_PER_SAMPLE_DEFAULT;
    TMEDIA_PRODUCER (self)->audio.channels = TDAV_PRODUCER_CHANNELS_DEFAULT;
    TMEDIA_PRODUCER (self)->audio.rate = TDAV_PRODUCER_RATE_DEFAULT;
    TMEDIA_PRODUCER (self)->audio.ptime = TDAV_PRODUCER_PTIME_DEFAULT;
    TMEDIA_PRODUCER (self)->audio.gain = TSK_MIN (tmedia_defaults_get_audio_producer_gain (), TDAV_PRODUCER_AUDIO_GAIN_MAX);

    return 0;
}

/**
* Generic function to compare two producers.
* @param producer1 The first producer to compare.
* @param producer2 The second producer to compare.
* @retval Returns an integral value indicating the relationship between the two producers:
* <0 : @a producer1 less than @a producer2.<br>
* 0  : @a producer1 identical to @a producer2.<br>
* >0 : @a producer1 greater than @a producer2.<br>
*/
int tdav_producer_audio_cmp (const tsk_object_t *producer1, const tsk_object_t *producer2)
{
    int ret;
    tsk_subsat_int32_ptr (producer1, producer2, &ret);
    return ret;
}

int tdav_producer_audio_set (tdav_producer_audio_t *self, const tmedia_param_t *param)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (param->plugin_type == tmedia_ppt_producer)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, "gain"))
            {
                int32_t gain = *((int32_t *)param->value);
                if (gain < TDAV_PRODUCER_AUDIO_GAIN_MAX && gain >= 0)
                {
                    TMEDIA_PRODUCER (self)->audio.gain = (uint8_t)gain;
                    TSK_DEBUG_INFO ("audio producer gain=%u", gain);
                }
                else
                {
                    TSK_DEBUG_ERROR ("%u is invalid as gain value", gain);
                    return -2;
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_VOLUME))
            {
                TMEDIA_PRODUCER (self)->audio.volume = TSK_TO_INT32 ((uint8_t *)param->value);
                TMEDIA_PRODUCER (self)->audio.volume = TSK_CLAMP (0, TMEDIA_PRODUCER (self)->audio.volume, 100);
                TSK_DEBUG_INFO ("audio producer volume=%u", TMEDIA_PRODUCER (self)->audio.volume);
            }
        }
    }

    return 0;
}

/** Deinitialize a producer
*/
int tdav_producer_audio_deinit (tdav_producer_audio_t *self)
{
    int ret;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* base */
    if ((ret = tmedia_producer_deinit (TMEDIA_PRODUCER (self))))
    {
        return ret;
    }

    return ret;
}
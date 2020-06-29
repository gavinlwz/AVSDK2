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


#include "tdav_producer_file.h"
#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "tsk_thread.h"

int tsk_timer_callback(const void*arg,long timerid)
{
    tdav_producer_file_t *producer = (tdav_producer_file_t *)arg;
    
    if (!producer->started)
    {
        return -1;
    }
    
    // Alert the session that there is new data to send
    if (TMEDIA_PRODUCER (producer)->enc_cb.callback)
    {
        TMEDIA_PRODUCER (producer)->enc_cb.callback (TMEDIA_PRODUCER (producer)->enc_cb.callback_data, NULL, 0);
    }
    return 0;
}

/* ============ Media Producer Interface ================= */

static int tdav_producer_file_prepare (tmedia_producer_t *self, const tmedia_codec_t *codec)
{
    tdav_producer_file_t *producer = (tdav_producer_file_t *)self;

    if (!producer || (!codec && codec->plugin))
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    TMEDIA_PRODUCER (producer)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (codec);
    TMEDIA_PRODUCER (producer)->audio.rate = TMEDIA_CODEC_RATE_ENCODING (codec);
    TMEDIA_PRODUCER (producer)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING (codec);
    return 0;
}

static int tdav_producer_file_start (tmedia_producer_t *self)
{
    int ret = 0;
    tdav_producer_file_t *producer = (tdav_producer_file_t *)self;

    if (!producer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (producer->started)
    {
        TSK_DEBUG_WARN ("Producer already started");
        return 0;
    }
    producer->timerID = xt_timer_mgr_global_schedule_loop(20, tsk_timer_callback, producer);
    producer->started = tsk_true;

    return ret;
}

static int tdav_producer_file_pause (tmedia_producer_t *self)
{
    int ret = 0;
    tdav_producer_file_t *producer = (tdav_producer_file_t *)self;

    if (!producer)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    return ret;
}

static int tdav_producer_file_stop (tmedia_producer_t *self)
{
    int ret = 0;
    tdav_producer_file_t *producer = (tdav_producer_file_t *)self;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!producer->started)
    {
        TSK_DEBUG_WARN ("Producer not started");
        return 0;
    }

    xt_timer_mgr_global_cancel(producer->timerID);
    producer->started = tsk_false;

    return ret;
}


//
//	CoreAudio producer object definition
//
/* constructor */
static tsk_object_t *tdav_producer_file_ctor (tsk_object_t *self, va_list *app)
{
    tdav_producer_file_t *producer = self;
    if (producer)
    {
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_producer_file_dtor (tsk_object_t *self)
{
    tdav_producer_file_t *producer = self;
    if (producer)
    {
        // Stop the producer if not done
        if (producer->started)
        {
            tdav_producer_file_stop (self);
        }

        /* deinit base */
        tdav_producer_audio_deinit (TDAV_PRODUCER_AUDIO (producer));
    }

    return self;
}
/* object definition */
static const tsk_object_def_t tdav_producer_file_def_s = {
    sizeof (tdav_producer_file_t), tdav_producer_file_ctor, tdav_producer_file_dtor, tdav_producer_audio_cmp,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t tdav_producer_file_plugin_def_s = { &tdav_producer_file_def_s,

                                                                              tmedia_audio,
                                                                              "Apple CoreAudio producer (file)",
                                                                              tsk_null,
                                                                              tsk_null,
                                                                              tdav_producer_file_prepare,
                                                                              tdav_producer_file_start,
                                                                              tdav_producer_file_pause,
                                                                              tdav_producer_file_stop };
const tmedia_producer_plugin_def_t *tdav_producer_file_plugin_def_t = &tdav_producer_file_plugin_def_s;

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
#include "tinymedia/tmedia_denoise.h"
#include "tinymedia/tmedia_defaults.h"

#include "tsk_debug.h"
#include "XConfigCWrapper.hpp"

static const tmedia_denoise_plugin_def_t *__tmedia_denoise_plugin = tsk_null;

int tmedia_denoise_init (tmedia_denoise_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    self->agc_level_min            = Config_GetInt("AGC_MIN",0);
    self->agc_level_max            = Config_GetInt("AGC_MAX",255);
    self->agc_mode                 = Config_GetInt("AGC_M",2);
    self->aec_echomode             = Config_GetInt("AEC_MODE", 3);
	self->aec_nlpmode              = Config_GetInt("AEC_NLPMODE", 1);// 0: kAecNlpConservative, 1: kAecNlpModerate, 2: kAecNlpAggressive
    self->noise_supp_level         = Config_GetInt("NS_LEVEL", 3);
	self->noise_rnn_enabled = Config_GetBool("RNN", 0);
	self->noise_rnn_model		   = Config_GetInt("RNNModel", 0);
	self->noise_rnn_db             = Config_GetInt("RNNDB", 16);
    self->aec_buffer_farend_max_size = Config_GetInt("AEC_BUFFER_FAREND_MAX_SIZE", 50);// aec buffer farend deque max size
    self->echo_tail                = tmedia_defaults_get_echo_tail ();
    self->echo_skew                = tmedia_defaults_get_echo_skew ();
    self->aec_enabled              = tmedia_defaults_get_aec_enabled ();
    self->aec_opt                  = tmedia_defaults_get_aec_option (); // 0: AECM, 1: AEC
    
    self->agc_enabled              = tmedia_defaults_get_agc_enabled ();
    self->agc_target_level         = tmedia_defaults_get_agc_target_level ();
    self->agc_compress_gain        = tmedia_defaults_get_agc_compress_gain ();
    self->vad_enabled              = tmedia_defaults_get_vad_enabled ();
    self->vad_smooth_proc          = Config_GetBool("VAD_SMOOTH_PROC", 1);
    self->noise_supp_enabled       = tmedia_defaults_get_noise_supp_enabled ();
    self->howling_supp_enabled     = tmedia_defaults_get_howling_supp_enabled ();
    self->reverb_filter_enabled    = tmedia_defaults_get_reverb_filter_enabled ();
    self->reverb_filter_enabled_ap = tsk_false;
    self->headset_plugin           = tsk_false;
    self->highpass_filter_enabled  = Config_GetBool("HF", 1);
    self->voice_boost_enabled      = Config_GetBool("VOICE_BOOST_ENABLE", 0);
    self->voice_boost_dbgain       = Config_GetInt("VOICE_BOOST_DBGAIN", 4);
    
    //变声相关    
    self->soundtouch_enabled = Config_GetInt("Soundtouch_Enabled", 0) != 0 ? tsk_true : tsk_false;
    self->soundtouch_tempo = ((float)Config_GetInt("Soundtouch_Tempo", 100))/100.0f;
    self->soundtouch_rate = ((float)Config_GetInt("Soundtouch_Rate", 100))/100.0f;
    self->soundtouch_pitch = ((float)Config_GetInt("Soundtouch_Pitch", 100))/100.0f;
    self->soundtouch_paraChanged = tsk_false;
    
    return 0;
}

int tmedia_denoise_set (tmedia_denoise_t *self, const tmedia_param_t *param)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (self->plugin->set)
    {
        return self->plugin->set (self, param);
    }
    return 0;
}

int tmedia_denoise_open (tmedia_denoise_t *self,
                         uint32_t record_frame_size_samples, uint32_t record_sampling_rate, uint32_t record_channels,
                         uint32_t playback_frame_size_samples, uint32_t playback_sampling_rate, uint32_t playback_channels,
                         uint32_t purevoice_frame_size_samples, uint32_t purevoice_sampling_rate, uint32_t purevoice_channels)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (self->opened)
    {
        TSK_DEBUG_WARN ("Denoiser already opened");
        return 0;
    }

    if (self->plugin->open)
    {
        int ret;
        if ((ret = self->plugin->open (self,
                                       record_frame_size_samples, record_sampling_rate, record_channels,
                                       playback_frame_size_samples, playback_sampling_rate, playback_channels,
                                       purevoice_frame_size_samples, purevoice_sampling_rate, purevoice_channels)))
        {
            TSK_DEBUG_ERROR ("Failed to open [%s] denoiser", self->plugin->desc);
            return ret;
        }
        else
        {
            self->opened = tsk_true;
            return 0;
        }
    }
    else
    {
        self->opened = tsk_true;
        return 0;
    }
}

int tmedia_denoise_echo_playback (tmedia_denoise_t *self, const void *echo_frame, uint32_t echo_frame_size_bytes, const void *echo_voice_frame, uint32_t echo_voice_frame_size_bytes)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("Denoiser not opened");
        return -2;
    }

    if (self->plugin->echo_playback)
    {
        return self->plugin->echo_playback (self, echo_frame, echo_frame_size_bytes, echo_voice_frame, echo_voice_frame_size_bytes);
    }
    else
    {
        return 0;
    }
}

int tmedia_denoise_process_record (tmedia_denoise_t *self, void *audio_frame, uint32_t audio_frame_size_bytes, tsk_bool_t *silence_or_noise)
{
    if (!self || !self->plugin || !silence_or_noise)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!self->opened)
    {
        TSK_DEBUG_ERROR ("Denoiser not opened");
        return -2;
    }

    if (self->plugin->process_record)
    {
        return self->plugin->process_record (self, audio_frame, audio_frame_size_bytes, silence_or_noise);
    }
    else
    {
        *silence_or_noise = tsk_false;
        return 0;
    }
}

int tmedia_denoise_close (tmedia_denoise_t *self)
{
    if (!self || !self->plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!self->opened)
    {
        return 0;
    }

    if (self->plugin->close)
    {
        int ret;

        if ((ret = self->plugin->close (self)))
        {
            TSK_DEBUG_ERROR ("Failed to close [%s] denoiser", self->plugin->desc);
            return ret;
        }
        else
        {
            self->opened = tsk_false;
            return 0;
        }
    }
    else
    {
        self->opened = tsk_false;
        return 0;
    }
}

int tmedia_denoise_deinit (tmedia_denoise_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (self->opened)
    {
        tmedia_denoise_close (self);
    }

    return 0;
}

int tmedia_denoise_plugin_register (const tmedia_denoise_plugin_def_t *plugin)
{
    if (!plugin)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!__tmedia_denoise_plugin)
    {
        TSK_DEBUG_INFO ("Register denoiser: %s", plugin->desc);
        __tmedia_denoise_plugin = plugin;
    }
    return 0;
}

int tmedia_denoise_plugin_unregister (const tmedia_denoise_plugin_def_t *plugin)
{
    (void)(plugin);
    __tmedia_denoise_plugin = tsk_null;
    return 0;
}

tmedia_denoise_t *tmedia_denoise_create ()
{
    tmedia_denoise_t *denoise = tsk_null;

    if (__tmedia_denoise_plugin)
    {
        if ((denoise = tsk_object_new (__tmedia_denoise_plugin->objdef)))
        {
            denoise->plugin = __tmedia_denoise_plugin;
        }
    }
    return denoise;
}

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

/**@file tmedia_denoise.h
 * @brief Denoiser (Noise suppression, AGC, AEC, VAD) Plugin
 */
#ifndef TINYMEDIA_DENOISE_H
#define TINYMEDIA_DENOISE_H

#include "tinymedia/tmedia_params.h"
#include "tinymedia_config.h"

#include "tsk_buffer.h"
#include "tsk_object.h"

TMEDIA_BEGIN_DECLS

/** cast any pointer to @ref tmedia_denoise_t* object */
#define TMEDIA_DENOISE(self) ((tmedia_denoise_t *)(self))

/** Base object for all Denoisers */
typedef struct tmedia_denoise_s
{
    TSK_DECLARE_OBJECT;

    tsk_bool_t opened;
    tsk_bool_t aec_enabled;
    uint32_t aec_echomode;
    uint32_t aec_nlpmode;
    uint32_t aec_buffer_farend_max_size;
    uint32_t aec_opt;
    uint32_t echo_tail;
    uint32_t echo_skew;
    tsk_bool_t agc_enabled;
    int agc_level_min; // Default:0
    int agc_level_max; // Default:255
    int agc_mode;
    int agc_mic_levelin;
    int agc_mic_levelout;
    int agc_target_level;
    int agc_compress_gain;
    tsk_bool_t vad_enabled;
    tsk_bool_t vad_smooth_proc;
    tsk_bool_t pre_frame_is_silence;  // Use for fade audio
    tsk_bool_t noise_supp_enabled;
    int32_t noise_supp_level;
	int32_t noise_rnn_model;
	int32_t noise_rnn_db;
	tsk_bool_t noise_rnn_enabled;
    tsk_bool_t howling_supp_enabled;
    tsk_bool_t highpass_filter_enabled;
    tsk_bool_t voice_boost_enabled;
    int voice_boost_dbgain;
    tsk_bool_t reverb_filter_enabled;
    tsk_bool_t reverb_filter_enabled_ap;
    tsk_bool_t soundtouch_enabled; // Enable soundtouch or not
    float soundtouch_tempo;
    float soundtouch_rate;
    float soundtouch_pitch;
    tsk_bool_t soundtouch_paraChanged; // Soundtouch parameters change or not
    tsk_bool_t headset_plugin;
    
    const struct tmedia_denoise_plugin_def_s *plugin;
} tmedia_denoise_t;

#define TMEDIA_DECLARE_DENOISE tmedia_denoise_t __denoise__

/** Virtual table used to define a consumer plugin */
typedef struct tmedia_denoise_plugin_def_s
{
    //! object definition used to create an instance of the denoiser
    const tsk_object_def_t *objdef;

    //! full description (usefull for debugging)
    const char *desc;

    int (*set) (tmedia_denoise_t *, const tmedia_param_t *);
    int (*open) (tmedia_denoise_t *,
                 uint32_t record_frame_size_samples, uint32_t record_sampling_rate, uint32_t record_channels,
                 uint32_t playback_frame_size_samples, uint32_t playback_sampling_rate, uint32_t playback_channels,
                 uint32_t purevoice_frame_size_samples, uint32_t purevoice_sampling_rate, uint32_t purevoice_channels);
    int (*echo_playback) (tmedia_denoise_t *self, const void *echo_frame, uint32_t echo_frame_size_bytes, const void *echo_voice_frame, uint32_t echo_voice_frame_size_bytes);
    //! aec, vad, noise suppression, echo cancellation before sending packet over network
    int (*process_record) (tmedia_denoise_t *, void *audio_frame, uint32_t audio_frame_size_bytes, tsk_bool_t *silence_or_noise);
    //! noise suppression before playing sound
    int (*process_playback) (tmedia_denoise_t *, void *audio_frame, uint32_t audio_frame_size_bytes);
    int (*close) (tmedia_denoise_t *);
} tmedia_denoise_plugin_def_t;

TINYMEDIA_API int tmedia_denoise_init (tmedia_denoise_t *self);
TINYMEDIA_API int tmedia_denoise_set (tmedia_denoise_t *self, const tmedia_param_t *param);
TINYMEDIA_API int
tmedia_denoise_open (tmedia_denoise_t *self,
                     uint32_t record_frame_size_samples, uint32_t record_sampling_rate, uint32_t record_channels,
                     uint32_t playback_frame_size_samples, uint32_t playback_sampling_rate, uint32_t playback_channels,
                     uint32_t purevoice_frame_size_samples, uint32_t purevoice_sampling_rate, uint32_t purevoice_channels);
TINYMEDIA_API int tmedia_denoise_echo_playback (tmedia_denoise_t *self, const void *echo_frame, uint32_t echo_frame_size_bytes, const void *echo_voice_frame, uint32_t echo_voice_frame_size_bytes);
TINYMEDIA_API int tmedia_denoise_process_record (tmedia_denoise_t *self, void *audio_frame, uint32_t audio_frame_size_bytes, tsk_bool_t *silence_or_noise);
TINYMEDIA_API int tmedia_denoise_process_playback (tmedia_denoise_t *self, void *audio_frame, uint32_t audio_frame_size_bytes);
TINYMEDIA_API int tmedia_denoise_close (tmedia_denoise_t *self);
TINYMEDIA_API int tmedia_denoise_deinit (tmedia_denoise_t *self);

TINYMEDIA_API int tmedia_denoise_plugin_register (const tmedia_denoise_plugin_def_t *plugin);
TINYMEDIA_API int tmedia_denoise_plugin_unregister (const tmedia_denoise_plugin_def_t *plugin);
TINYMEDIA_API tmedia_denoise_t *tmedia_denoise_create ();

TMEDIA_END_DECLS


#endif /* TINYMEDIA_DENOISE_H */

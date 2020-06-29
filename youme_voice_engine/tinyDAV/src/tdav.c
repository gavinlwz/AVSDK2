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
#include "tinydav/tdav.h"
#include "XConfigCWrapper.hpp"
#include "version.h"

static tsk_bool_t __b_initialized = tsk_false;
static tsk_bool_t __b_ipsec_supported = tsk_false;
static const struct tmedia_codec_plugin_def_s *__codec_plugins_all[0xFF] = { tsk_null }; // list of all codecs BEFORE filtering
static const tsk_size_t __codec_plugins_all_count = sizeof (__codec_plugins_all) / sizeof (__codec_plugins_all[0]);

#if TDAV_UNDER_APPLE
#include "tinydav/tdav_apple.h"
#endif

// Media Contents, plugins defintion...
#include "tinymedia.h"


// Sessions
#include "tinydav/audio/tdav_session_audio.h"
#include "tinydav/video/tdav_session_video.h"

// Codecs
#include "tinydav/codecs/opus/tdav_codec_opus.h"
#include "tinydav/codecs/h264/tdav_codec_h264.h"

// Consumers
#include "tinydav/audio/coreaudio/tdav_consumer_audiounit.h"
#if AUDIO_OPENSLES_UNDER_ANDROID
#include "tinydav/src/audio/audio_opensles/audio_opensles_consumer.h"
#include "tinydav/src/audio/android/audio_android_consumer.h"
#endif

// Producers
#if TARGET_OS_IPHONE
#include "tinydav/audio/coreaudio/tdav_producer_audiounit.h"
#endif
#if AUDIO_OPENSLES_UNDER_ANDROID
#include "tinydav/src/audio/audio_opensles/audio_opensles_producer.h"
#include "tinydav/src/audio/android/audio_android_producer.h"
#endif
#ifdef MAC_OS
#include "tinydav/audio/coreaudio/tdav_producer_file.h"
#include "tinydav/audio/coreaudio/tdav_producer_audiounit.h"
#endif
#ifdef WIN32
#include "tinydav/audio/waveapi/tdav_producer_waveapi.h"
#include "tinydav/audio/dmoapi/tdav_producer_dmoapi.h"
#include "tinydav/audio/waveapi/tdav_consumer_waveapi.h"
#endif
#ifdef OS_LINUX
#include "tinydav/audio/alsa/tdav_producer_alsa.h"
#include "tinydav/audio/alsa/tdav_consumer_alsa.h"
#endif
// Audio Denoise (AGC, Noise Suppression, VAD and AEC)
#if HAVE_WEBRTC && (!defined(HAVE_WEBRTC_DENOISE) || HAVE_WEBRTC_DENOISE)
#include "tinydav/audio/tdav_webrtc_denoise.h"
#endif

#include "tinydav/audio/tdav_youme_neteq_jitterbuffer.h"

#include "tinydav/video/android/video_android_producer.h"
#include "tinydav/video/android/video_android_consumer.h"

static inline int _tdav_codec_plugins_collect ();
static inline int _tdav_codec_plugins_disperse ();
static inline tsk_bool_t _tdav_codec_is_supported (tmedia_codec_id_t codec, const tmedia_codec_plugin_def_t *plugin);

int tdav_init ()
{
    int ret = 0;

    if (__b_initialized)
    {
        TSK_DEBUG_INFO ("TINYDAV already initialized");
        return 0;
    }
    
/* === OS specific === */
#ifdef __APPLE__
    if ((ret = tdav_apple_init ()))
    {
        return ret;
    }
#endif
    /* === Register Audio/video JitterBuffer === */
    tmedia_jitterbuffer_plugin_register (tdav_youme_neteq_jitterbuffer_plugin_def_t);
    
    /* === Register sessions === */
    
    tmedia_session_plugin_register (tdav_session_audio_plugin_def_t);
	tmedia_session_plugin_register(tdav_session_video_plugin_def_t);

#if HAVE_LIBOPUS
    tmedia_codec_plugin_register (tdav_codec_opus_plugin_def_t);
#endif

    tmedia_codec_plugin_register (tdav_codec_h264_main_plugin_def_t);


/* === Register consumers === */
#if TARGET_OS_IPHONE
    tmedia_consumer_plugin_register (tdav_consumer_audiounit_plugin_def_t);
#endif

#if AUDIO_OPENSLES_UNDER_ANDROID
    // Register audio path in android(1: opensles, 0: android)
    if (tmedia_defaults_get_android_opensles_enabled()) {
        TSK_DEBUG_INFO ("Android consumer use opensles path.");
        tmedia_consumer_plugin_register (audio_consumer_opensles_plugin_def_t);
    } else {
        TSK_DEBUG_INFO ("Android consumer use android audio track path.");
        tmedia_consumer_plugin_register (audio_consumer_android_plugin_def_t);
    }
#endif
#ifdef WIN32
	tmedia_consumer_plugin_register(tdav_consumer_waveapi_plugin_def_t);
#endif
#ifdef MAC_OS
    tmedia_consumer_plugin_register (tdav_consumer_audiounit_plugin_def_t);
#endif
#ifdef OS_LINUX
    tmedia_consumer_plugin_register (tdav_consumer_alsa_plugin_def_t);
#endif

/* === Register producers === */

#if TARGET_OS_IPHONE // CoreAudio based on AudioUnit
    tmedia_producer_plugin_register (tdav_producer_audiounit_plugin_def_t);
#endif
#if AUDIO_OPENSLES_UNDER_ANDROID
    // Register audio path in android(1: opensles, 0: android)
    if (tmedia_defaults_get_android_opensles_enabled()) {
        TSK_DEBUG_INFO ("Android producer use opensles path.");
        tmedia_producer_plugin_register (audio_producer_opensles_plugin_def_t);
    } else {
        TSK_DEBUG_INFO ("Android producer use android audio record path.");
        tmedia_producer_plugin_register (audio_producer_android_plugin_def_t);
    }
#endif
#ifdef MAC_OS
    tmedia_producer_plugin_register (tdav_producer_audiounit_plugin_def_t);
    
    //todopinky:可以同时有file和采集吗
    //tmedia_producer_plugin_register (tdav_producer_file_plugin_def_t);
#endif
#ifdef WIN32
	if (Config_GetBool("DMOAPI", 1) && tdav_producer_dmoapi_issupported()) {
		TSK_DEBUG_INFO("Windows producer use dmo api.");
		tmedia_producer_plugin_register(tdav_producer_dmoapi_plugin_def_t);
	}
	else {
		TSK_DEBUG_INFO("Windows producer use wave api.");
		tmedia_producer_plugin_register(tdav_producer_waveapi_plugin_def_t);
	}
#endif
#ifdef OS_LINUX
    tmedia_producer_plugin_register (tdav_producer_alsa_plugin_def_t);
#endif

/* === Register Audio Denoise (AGC, VAD, Noise Suppression and AEC) === */
#if HAVE_WEBRTC && (!defined(HAVE_WEBRTC_DENOISE) || HAVE_WEBRTC_DENOISE)
    tmedia_denoise_plugin_register (tdav_webrtc_denoise_plugin_def_t);
#endif
    

    /* === Register video producer&consumer === */
    tmedia_consumer_plugin_register(video_consumer_android_plugin_def_t);
    tmedia_producer_plugin_register(video_producer_android_plugin_def_t);

    /* === Register Audio/video JitterBuffer === */
   // tmedia_jitterbuffer_plugin_register (tdav_youme_neteq_jitterbuffer_plugin_def_t);

    // collect all codecs before filtering
    _tdav_codec_plugins_collect ();

    __b_initialized = tsk_true;

    return ret;
}

int tdav_codec_set_priority (tmedia_codec_id_t codec_id, int priority)
{
    tsk_size_t i;

    if (priority < 0)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    for (i = 0; i < __codec_plugins_all_count && __codec_plugins_all[i]; ++i)
    {
        if (__codec_plugins_all[i]->codec_id == codec_id)
        {
            const struct tmedia_codec_plugin_def_s *codec_decl_1, *codec_decl_2;
            priority = TSK_MIN (priority, (int)__codec_plugins_all_count - 1);
            codec_decl_1 = __codec_plugins_all[priority];
            codec_decl_2 = __codec_plugins_all[i];

            __codec_plugins_all[i] = codec_decl_1;
            __codec_plugins_all[priority] = codec_decl_2;

            // change priority if already registered and supported
            if (_tdav_codec_is_supported (codec_decl_2->codec_id, codec_decl_2) && tmedia_codec_plugin_is_registered (codec_decl_2))
            {
                return tmedia_codec_plugin_register_2 (codec_decl_2, priority);
            }
            return 0;
        }
    }

    TSK_DEBUG_INFO ("Cannot find codec with id=%d for priority setting", (int)codec_id);
    return 0;
}

int tdav_set_codecs (tmedia_codec_id_t codecs)
{
    tsk_size_t i, prio;

    // unregister all codecs
    tmedia_codec_plugin_unregister_all ();
    // register "selected" and "fake" codecs. "fake" codecs have "none" as id (e.g. MSRP or DTMF)
    for (i = 0, prio = 0; i < __codec_plugins_all_count && __codec_plugins_all[i]; ++i)
    {
        if ((codecs & __codec_plugins_all[i]->codec_id) || __codec_plugins_all[i]->codec_id == tmedia_codec_id_none)
        {
            if (_tdav_codec_is_supported (__codec_plugins_all[i]->codec_id, __codec_plugins_all[i]))
            {
                tmedia_codec_plugin_register_2 (__codec_plugins_all[i], (int)prio++);
            }
        }
    }
    return 0;
}

static inline int _tdav_codec_plugins_collect ()
{
    extern const tmedia_codec_plugin_def_t *__tmedia_codec_plugins[TMED_CODEC_MAX_PLUGINS];

    static const tsk_size_t __codec_plugins_all_count = sizeof (__codec_plugins_all) / sizeof (__codec_plugins_all[0]);

    int ret = _tdav_codec_plugins_disperse ();
    if (ret == 0)
    {
        tsk_size_t i, count_max = sizeof (__tmedia_codec_plugins) / sizeof (__tmedia_codec_plugins[0]);
        for (i = 0; i < count_max && i < __codec_plugins_all_count; ++i)
        {
            __codec_plugins_all[i] = __tmedia_codec_plugins[i];
        }
    }
    return ret;
}

static inline int _tdav_codec_plugins_disperse ()
{
    memset ((void *)__codec_plugins_all, 0, sizeof (__codec_plugins_all));
    return 0;
}


/*
 Must be called after tdav_init()
*/
static inline tsk_bool_t _tdav_codec_is_supported (tmedia_codec_id_t codec, const tmedia_codec_plugin_def_t *plugin)
{
    tsk_size_t i;
    for (i = 0; i < __codec_plugins_all_count && __codec_plugins_all[i]; ++i)
    {
        if ((plugin && __codec_plugins_all[i] == plugin) || __codec_plugins_all[i]->codec_id == codec)
        {
            return tsk_true;
        }
    }
    return tsk_false;
}

/**
* Checks whether a codec is supported. Being supported doesn't mean it's enabled and ready for use.
* @return @ref tsk_true if supported and @tsk_false otherwise.
* @sa @ref tdav_codec_is_enabled()
*/
tsk_bool_t tdav_codec_is_supported (tmedia_codec_id_t codec)
{
    return _tdav_codec_is_supported (codec, tsk_null);
}

/**
* Checks whether a codec is enabled.
* @return @ref tsk_true if enabled and @tsk_false otherwise.
* @sa @ref tdav_codec_is_supported()
*/
tsk_bool_t tdav_codec_is_enabled (tmedia_codec_id_t codec)
{
    return tmedia_codec_plugin_is_registered_2 ((tmedia_codec_id_t)codec);
}

/**
* Checks whether a IPSec is supported.
* @return @ref tsk_true if supported and @tsk_false otherwise.
*/
tsk_bool_t tdav_ipsec_is_supported ()
{
    return __b_ipsec_supported;
}

int tdav_deinit ()
{
    int ret = 0;

    if (!__b_initialized)
    {
        TSK_DEBUG_INFO ("TINYDAV not initialized");
        return 0;
    }

/* === OS specific === */
#ifdef __APPLE__
    if ((ret = tdav_apple_deinit ()))
    {
        return ret;
    }
#endif

    tmedia_session_plugin_unregister (tdav_session_audio_plugin_def_t);
	tmedia_session_plugin_unregister(tdav_session_video_plugin_def_t);

    /* === UnRegister codecs === */
    tmedia_codec_plugin_unregister_all ();

#if TARGET_OS_IPHONE // CoreAudio based on AudioUnit
    tmedia_consumer_plugin_unregister (tdav_consumer_audiounit_plugin_def_t);
    tmedia_producer_plugin_unregister (tdav_producer_audiounit_plugin_def_t);
#endif


/* === UnRegister Audio Denoise (AGC, VAD, Noise Suppression and AEC) === */
#if HAVE_WEBRTC && (!defined(HAVE_WEBRTC_DENOISE) || HAVE_WEBRTC_DENOISE)
    tmedia_denoise_plugin_unregister (tdav_webrtc_denoise_plugin_def_t);
#endif

    /* === UnRegister Audio/video JitterBuffer === */
        tmedia_jitterbuffer_plugin_unregister (tdav_youme_neteq_jitterbuffer_plugin_def_t);

    // disperse all collected codecs
    _tdav_codec_plugins_disperse ();

    __b_initialized = tsk_false;

    return ret;
}

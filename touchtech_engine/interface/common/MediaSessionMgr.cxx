
#include "MediaSessionMgr.h"

//
//	Codec
//
Codec::Codec (const struct tmedia_codec_s *pWrappedCodec)
{
    m_pWrappedCodec = (struct tmedia_codec_s *)tsk_object_ref (TSK_OBJECT (pWrappedCodec));
}

Codec::~Codec ()
{
    TSK_OBJECT_SAFE_FREE (m_pWrappedCodec);
}

twrap_media_type_t Codec::getMediaType ()
{
    if (m_pWrappedCodec)
    {
        switch (m_pWrappedCodec->type)
        {
        case tmedia_audio:
            return twrap_media_audio;
        case tmedia_video:
            return twrap_media_video;
        
        default:
            break;
        }
    }
    return twrap_media_none;
}

const char *Codec::getName ()
{
    if (m_pWrappedCodec)
    {
        return m_pWrappedCodec->name;
    }
    return tsk_null;
}

const char *Codec::getDescription ()
{
    if (m_pWrappedCodec)
    {
        return m_pWrappedCodec->desc;
    }
    return tsk_null;
}

const char *Codec::getNegFormat ()
{
    if (m_pWrappedCodec)
    {
        return m_pWrappedCodec->neg_format ? m_pWrappedCodec->neg_format : m_pWrappedCodec->format;
    }
    return tsk_null;
}

int Codec::getAudioSamplingRate ()
{
    if (m_pWrappedCodec && m_pWrappedCodec->plugin)
    {
        return m_pWrappedCodec->plugin->rate;
    }
    return 0;
}

int Codec::getAudioChannels ()
{
    if (m_pWrappedCodec && (m_pWrappedCodec->type & tmedia_audio) && m_pWrappedCodec->plugin)
    {
        return m_pWrappedCodec->plugin->audio.channels;
    }
    return 0;
}

int Codec::getAudioPTime ()
{
    if (m_pWrappedCodec && (m_pWrappedCodec->type & tmedia_audio) && m_pWrappedCodec->plugin)
    {
        return m_pWrappedCodec->plugin->audio.ptime;
    }
    return 0;
}

//
//	MediaSessionMgr
//
MediaSessionMgr::MediaSessionMgr (tmedia_session_mgr_t *pWrappedMgr)
{
    m_pWrappedMgr = (tmedia_session_mgr_t *)tsk_object_ref (pWrappedMgr);
}

MediaSessionMgr::~MediaSessionMgr ()
{
    TSK_OBJECT_SAFE_FREE (m_pWrappedMgr);
}

bool MediaSessionMgr::sessionSetInt32 (twrap_media_type_t media, const char *key, int32_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_SET_INT32 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

int32_t MediaSessionMgr::sessionGetInt32 (twrap_media_type_t media, const char *key)
{
    int32_t value = 0;
    tmedia_type_t _media = twrap_get_native_media_type (media);
    (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_GET_INT32 (_media, key, &value),
                             TMEDIA_SESSION_GET_NULL ()));
    return value;
}

bool MediaSessionMgr::sessionSetInt64 (twrap_media_type_t media, const char *key, int64_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_SET_INT64 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

int64_t MediaSessionMgr::sessionGetInt64 (twrap_media_type_t media, const char *key)
{
    int64_t value = 0;
    tmedia_type_t _media = twrap_get_native_media_type (media);
    (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_GET_INT64 (_media, key, &value),
                             TMEDIA_SESSION_GET_NULL ()));
    return value;
}

void* MediaSessionMgr::sessionGetObject(twrap_media_type_t media, const char *key)
{
    void* pobj = NULL;
    tmedia_type_t _media = twrap_get_native_media_type (media);
    (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_GET_POBJECT (_media, key, &pobj),
                             TMEDIA_SESSION_SET_NULL ()));
    return pobj;
}

bool MediaSessionMgr::sessionSetObject(twrap_media_type_t media, const char *key, void* value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_SET_POBJECT (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::sessionSetPvoid(twrap_media_type_t media, const char *key, void* value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_SET_PVOID (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::consumerSetInt32 (twrap_media_type_t media, const char *key, int32_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_CONSUMER_SET_INT32 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::consumerSetInt64 (twrap_media_type_t media, const char *key, int64_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_CONSUMER_SET_INT64 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::consumerSetPvoid(twrap_media_type_t media, const char *key, void* value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_CONSUMER_SET_PVOID (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::producerSetPvoid(twrap_media_type_t media, const char *key, void* value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_SET_PVOID (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::producerSetInt32 (twrap_media_type_t media, const char *key, int32_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_SET_INT32 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::producerGetInt32 (twrap_media_type_t media, const char *key, int32_t *pValue)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_GET_INT32 (_media, key, pValue),
                             TMEDIA_SESSION_GET_NULL ()) == 0);
}

bool MediaSessionMgr::producerSetInt64 (twrap_media_type_t media, const char *key, int64_t value)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_set (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_SET_INT64 (_media, key, value),
                                    TMEDIA_SESSION_SET_NULL ()) == 0);
}

bool MediaSessionMgr::producerGetInt64 (twrap_media_type_t media, const char *key, int64_t *pValue)
{
    tmedia_type_t _media = twrap_get_native_media_type (media);
    return (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_GET_INT64 (_media, key, pValue),
                             TMEDIA_SESSION_GET_NULL ()) == 0);
}

Codec *MediaSessionMgr::producerGetCodec (twrap_media_type_t media)
{
    tmedia_codec_t *_codec = tsk_null;
    tmedia_type_t _media = twrap_get_native_media_type (media);
    (tmedia_session_mgr_get (m_pWrappedMgr, TMEDIA_SESSION_PRODUCER_GET_POBJECT (_media, "codec", &_codec),
                             TMEDIA_SESSION_GET_NULL ()));

    if (_codec)
    {
        Codec *pCodec = new Codec (_codec);
        TSK_OBJECT_SAFE_FREE (_codec);
        return pCodec;
    }
    return tsk_null;
}

#include "tinydav/audio/tdav_session_audio.h"




// FIXME: create generic function to register any kind and number of plugin from a file
unsigned int MediaSessionMgr::registerAudioPluginFromFile (const char *path)
{
    static struct tsk_plugin_s *__plugin = tsk_null;
    if (__plugin)
    {
        TSK_DEBUG_ERROR ("Audio plugin already registered");
        return 0;
    }
    if ((__plugin = tsk_plugin_create (path)))
    {
        unsigned int count = 0;
        tsk_plugin_def_ptr_const_t plugin_def_ptr_const;
        if ((plugin_def_ptr_const = tsk_plugin_get_def (__plugin, tsk_plugin_def_type_consumer,
                                                        tsk_plugin_def_media_type_audio)))
        {
            if (tmedia_consumer_plugin_register ((const tmedia_consumer_plugin_def_t *)plugin_def_ptr_const) == 0)
            {
                ++count;
            }
        }
        if ((plugin_def_ptr_const = tsk_plugin_get_def (__plugin, tsk_plugin_def_type_producer,
                                                        tsk_plugin_def_media_type_audio)))
        {
            if (tmedia_producer_plugin_register ((const tmedia_producer_plugin_def_t *)plugin_def_ptr_const) == 0)
            {
                ++count;
            }
        }
        return count;
    }
    TSK_DEBUG_ERROR ("Failed to create plugin with path=%s", path);
    return 0;
}

bool MediaSessionMgr::defaultsSetProfile (tmedia_profile_t profile)
{
    return (tmedia_defaults_set_profile (profile) == 0);
}

tmedia_profile_t MediaSessionMgr::defaultsGetProfile ()
{
    return tmedia_defaults_get_profile ();
}

bool MediaSessionMgr::defaultsSetBandwidthLevel (tmedia_bandwidth_level_t bl) // @deprecated
{
    return tmedia_defaults_set_bl (bl) == 0;
}
tmedia_bandwidth_level_t MediaSessionMgr::defaultsGetBandwidthLevel () // @deprecated
{
    return tmedia_defaults_get_bl ();
}
bool MediaSessionMgr::defaultsSetCongestionCtrlEnabled (bool enabled)
{
    return tmedia_defaults_set_congestion_ctrl_enabled (enabled ? tsk_true : tsk_false) == 0;
}
bool MediaSessionMgr::defaultsSetJbMargin (uint32_t jb_margin_ms)
{
    return tmedia_defaults_set_jb_margin (jb_margin_ms) == 0;
}

bool MediaSessionMgr::defaultsSetJbMaxLateRate (uint32_t jb_late_rate_percent)
{
    return tmedia_defaults_set_jb_max_late_rate (jb_late_rate_percent) == 0;
}

bool MediaSessionMgr::defaultsSetEchoTail (uint32_t echo_tail)
{
    return tmedia_defaults_set_echo_tail (echo_tail) == 0;
}

uint32_t MediaSessionMgr::defaultsGetEchoTail ()
{
    return tmedia_defaults_get_echo_tail ();
}

bool MediaSessionMgr::defaultsSetEchoSkew (uint32_t echo_skew)
{
    return tmedia_defaults_set_echo_skew (echo_skew) == 0;
}

bool MediaSessionMgr::defaultsSetAecEnabled (bool aec_enabled)
{
    return tmedia_defaults_set_aec_enabled (aec_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetAecEnabled ()
{
    return tmedia_defaults_get_aec_enabled () ? true : false;
}

bool MediaSessionMgr::defaultsSetAecOption (uint32_t aec_option)
{
    return tmedia_defaults_set_aec_option (aec_option) == 0;
}

uint32_t MediaSessionMgr::defaultsGetAecOption ()
{
    return tmedia_defaults_get_aec_option ();
}

bool MediaSessionMgr::defaultsSetAgcEnabled (bool agc_enabled)
{
    return tmedia_defaults_set_agc_enabled (agc_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetAgcEnabled ()
{
    return tmedia_defaults_get_agc_enabled () ? true : false;
}

bool MediaSessionMgr::defaultsSetAgcLevel (float agc_level)
{
    return tmedia_defaults_set_agc_level (agc_level) == 0;
}

float MediaSessionMgr::defaultsGetAgcLevel ()
{
    return tmedia_defaults_get_agc_level ();
}

bool MediaSessionMgr::defaultsSetAgcTargetLevel (int32_t agc_target_level)
{
    return tmedia_defaults_set_agc_target_level (agc_target_level) == 0;
}

bool MediaSessionMgr::defaultsSetAgcCompressGain (int32_t agc_compress_gain)
{
    return tmedia_defaults_set_agc_compress_gain(agc_compress_gain) == 0;
}

bool MediaSessionMgr::defaultsSetVadEnabled (bool vad_enabled)
{
    return tmedia_defaults_set_vad_enabled (vad_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetGetVadEnabled ()
{
    return tmedia_defaults_get_vad_enabled () ? true : false;
}

bool MediaSessionMgr::defaultsSetNoiseSuppEnabled (bool noise_supp_enabled)
{
    return tmedia_defaults_set_noise_supp_enabled (noise_supp_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetNoiseSuppEnabled ()
{
    return tmedia_defaults_get_noise_supp_enabled () ? true : false;
}

bool MediaSessionMgr::defaultsSetHowlingSuppEnabled (bool howling_supp_enabled)
{
    return tmedia_defaults_set_howling_supp_enabled (howling_supp_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetHowlingSuppEnabled ()
{
    return tmedia_defaults_get_howling_supp_enabled () ? true : false;
}

bool MediaSessionMgr::defaultsSetReverbFilterEnabled (bool reverb_filter_enabled)
{
    return tmedia_defaults_set_reverb_filter_enabled (reverb_filter_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGetReverbFilterEnabled()
{
    return tmedia_defaults_get_reverb_filter_enabled() ? true : false;
}

bool MediaSessionMgr::defaultsSet100relEnabled (bool _100rel_enabled)
{
    return tmedia_defaults_set_100rel_enabled (_100rel_enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsGet100relEnabled ()
{
    return tmedia_defaults_get_100rel_enabled () == 0;
}

bool MediaSessionMgr::defaultsSetScreenSize (int32_t sx, int32_t sy)
{
    return tmedia_defaults_set_screen_size (sx, sy) == 0;
}

bool MediaSessionMgr::defaultsSetAudioGain (int32_t producer_gain, int32_t consumer_gain)
{
    return tmedia_defaults_set_audio_gain (producer_gain, consumer_gain) == 0;
}

bool MediaSessionMgr::defaultsSetAudioPtime (int32_t ptime)
{
    return tmedia_defaults_set_audio_ptime (ptime) == 0;
}
bool MediaSessionMgr::defaultsSetAudioChannels (int32_t channel_playback, int32_t channel_record)
{
    return tmedia_defaults_set_audio_channels (channel_playback, channel_record) == 0;
}

bool MediaSessionMgr::defaultsSetMixedCallbackSamplerate (int32_t mixecCallbackSamplerate)
{
    return tmedia_defaults_set_mixed_callback_samplerate (mixecCallbackSamplerate) == 0;
}

bool MediaSessionMgr::defaultsSetRtpPortRange (uint16_t range_start, uint16_t range_stop)
{
    return tmedia_defaults_set_rtp_port_range (range_start, range_stop) == 0;
}

bool MediaSessionMgr::defaultsSetRtpSymetricEnabled (bool enabled)
{
    return tmedia_defaults_set_rtp_symetric_enabled (enabled ? tsk_true : tsk_false) == 0;
}

bool MediaSessionMgr::defaultsSetMediaType (twrap_media_type_t media_type)
{
    return (tmedia_defaults_set_media_type (twrap_get_native_media_type (media_type)) == 0);
}

bool MediaSessionMgr::defaultsSetVolume (int32_t volume)
{
    return (tmedia_defaults_set_volume (volume) == 0);
}

void MediaSessionMgr::setVideoNetAdjustMode( int mode )
{
    tmedia_defaults_set_video_net_adjustmode(mode);
}

bool MediaSessionMgr::setVideoNetResolution( uint32_t width, uint32_t height )
{
    if (defaultsGetExternalInputMode()){
        //对于外部输入模式，不区分摄像头本地渲染和网维传输的size
        //方向问题，要反过来
        tmedia_defaults_set_video_size( width, height, 1 );
        tmedia_defaults_set_camera_size( width, height );
    }else {
        //对于视频聊天模式，只设置网络传输的分辨率
        tmedia_defaults_set_video_size( width, height, 1 );
    }
    return true;
}

bool MediaSessionMgr::getVideoNetResolution(uint32_t &width, uint32_t &height)
{
	tmedia_defaults_get_video_size(&width, &height);
	return true;
}

bool MediaSessionMgr::setVideoNetResolutionForSecond( uint32_t width, uint32_t height )
{
    //对于视频聊天模式，只设置网络传输的分辨率
    tmedia_defaults_set_video_size_child( width, height, 1 );
    return true;
}

bool MediaSessionMgr::getVideoNetResolutionForSecond(uint32_t &width, uint32_t &height)
{
	//对于视频聊天模式，只设置网络传输的分辨率
	tmedia_defaults_get_video_size_child(&width, &height);
	return true;
}

bool MediaSessionMgr::setVideoNetResolutionForShare( uint32_t width, uint32_t height )
{
    tmedia_defaults_set_video_size_share( width, height, 1 );
    return true;
}


bool MediaSessionMgr::setVideoLocalResolution( uint32_t width, uint32_t height )
{
    //对于视频聊天模式，只设置摄像头采集的分辨率
    tmedia_defaults_set_camera_size( width, height );
    return true;
}

bool MediaSessionMgr::getVideoLocalResolution( uint32_t &width, uint32_t &height )
{
    //对于视频聊天模式，只设置摄像头采集的分辨率
    tmedia_defaults_get_camera_size( &width, &height );
    return true;
}

bool MediaSessionMgr::setVideoFps( uint32_t fps )
{
    tmedia_defaults_set_video_fps( fps);
    return true;
}

bool MediaSessionMgr::setVideoFpsForSecond( uint32_t fps )
{
    tmedia_defaults_set_video_fps_child( fps);
    return true;
}

bool MediaSessionMgr::setVideoFpsForShare( uint32_t fps )
{
    tmedia_defaults_set_video_fps_share( fps);
    return true;
}


int32_t MediaSessionMgr::defaultsGetVolume ()
{
    return tmedia_defaults_get_volume ();
}

bool MediaSessionMgr::defaultsSetInviteSessionTimers (int32_t timeout, const char *refresher)
{
    int ret = tmedia_defaults_set_inv_session_expires (timeout);
    ret &= tmedia_defaults_set_inv_session_refresher (refresher);
    return (ret == 0);
}

bool MediaSessionMgr::defaultsSetByPassEncoding (bool enabled)
{
    return (tmedia_defaults_set_bypass_encoding (enabled ? tsk_true : tsk_false) == 0);
}
bool MediaSessionMgr::defaultsGetByPassEncoding ()
{
    return (tmedia_defaults_get_bypass_encoding () == tsk_true);
}
bool MediaSessionMgr::defaultsSetByPassDecoding (bool enabled)
{
    return (tmedia_defaults_set_bypass_decoding (enabled ? tsk_true : tsk_false) == 0);
}
bool MediaSessionMgr::defaultsGetByPassDecoding ()
{
    return (tmedia_defaults_get_bypass_decoding () == tsk_true);
}


bool MediaSessionMgr::defaultsSetRtpBuffSize (unsigned buffSize)
{
    return (tmedia_defaults_set_rtpbuff_size (buffSize) == 0);
}
unsigned MediaSessionMgr::defaultsGetRtpBuffSize ()
{
    return tmedia_defaults_get_rtpbuff_size ();
}

bool MediaSessionMgr::defaultsSetAvpfTail (unsigned tail_min, unsigned tail_max)
{
    return (tmedia_defaults_set_avpf_tail (tail_min, tail_max) == 0);
}

bool MediaSessionMgr::defaultsSetAvpfMode (enum tmedia_mode_e mode)
{
    return (tmedia_defaults_set_avpf_mode (mode) == 0);
}

bool MediaSessionMgr::defaultsSetRecordSampleRate (int32_t record_sample_rate)
{
    return (tmedia_defaults_set_record_sample_rate (record_sample_rate, 0 ) == 0);
}
bool MediaSessionMgr::defaultsSetPlaybackSampleRate (int32_t playback_sample_rate)
{
    return (tmedia_defaults_set_playback_sample_rate (playback_sample_rate, 0 ) == 0);
}

bool MediaSessionMgr::defaultsSetMaxFds (int32_t max_fds)
{
    return (tmedia_defaults_set_max_fds (max_fds) == 0);
}

bool MediaSessionMgr::defaultsSetOpusInbandFecEnabled (bool enabled)
{
    return (tmedia_defaults_set_opus_inband_fec_enabled (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetOpusOutbandFecEnabled (bool enabled)
{
    return (tmedia_defaults_set_opus_outband_fec_enabled (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetOpusPacketLossPerc (int packet_loss_perc)
{
    return (tmedia_defaults_set_opus_packet_loss_perc (packet_loss_perc) == 0);
}

bool MediaSessionMgr::defaultsSetOpusDtxPeroid (int peroid)
{
    return (tmedia_defaults_set_opus_dtx_peroid (peroid) == 0);
}

bool MediaSessionMgr::defaultsSetOpusEncoderComplexity (int complexity)
{
    return (tmedia_defaults_set_opus_encoder_complexity(complexity) == 0);
}

bool MediaSessionMgr::defaultsSetOpusEncoderVbrEnabled (bool enabled)
{
    return (tmedia_defaults_set_opus_encoder_vbr_enabled (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetOpusEncoderMaxBandwidth (int bandwidth)
{
    return (tmedia_defaults_set_opus_encoder_max_bandwidth(bandwidth) == 0);
}

bool MediaSessionMgr::defaultsSetOpusEncoderBitrate (int bitrate)
{
    return (tmedia_defaults_set_opus_encoder_bitrate(bitrate) == 0);
}

bool MediaSessionMgr::defaultsSetRtpFeedbackPeriod (int period)
{
    return (tmedia_defaults_set_rtp_feedback_period(period) == 0);
}

bool MediaSessionMgr::defaultsSetAndroidFakedRecording (bool fakedRec)
{
    return (tmedia_defaults_set_android_faked_recording(fakedRec ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetIosNoPermissionNoRec (bool enabled)
{
    return (tmedia_defaults_set_ios_no_permission_no_rec (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetIosAirplayNoRec (bool enabled)
{
    return (tmedia_defaults_set_ios_airplay_no_rec (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetIosAlwaysPlayCategory (bool enabled)
{
    return (tmedia_defaults_set_ios_always_play_category (enabled ? tsk_true : tsk_false) == 0);
}
bool MediaSessionMgr::defaultsSetIosNeedSetMode (bool enabled)
{
    return (tmedia_defaults_set_ios_need_set_mode (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetIosRecoveryDelayFromCall (uint32_t iosRecoveryDelayFromCall)
{
    return (tmedia_defaults_set_ios_recovery_delay_from_call (iosRecoveryDelayFromCall) == 0);
}

bool MediaSessionMgr::defaultsSetAppDocumentPath (const char* path)
{
    return (tmedia_defaults_set_app_document_path (path) == 0);
}

bool MediaSessionMgr::defaultsSetPcmFileSizeKb (uint32_t size_kb)
{
    return (tmedia_defaults_set_pcm_file_size_kb (size_kb) == 0);
}

bool MediaSessionMgr::defaultsSetH264FileSizeKb (uint32_t size_kb)
{
    return (tmedia_defaults_set_h264_file_size_kb (size_kb) == 0);
}

bool MediaSessionMgr::defaultsSetVadLenMs(uint32_t vadLenMs)
{
    return (tmedia_defaults_set_vad_len_ms (vadLenMs) == 0);
}

bool MediaSessionMgr::defaultsSetVadSilencePercent(uint32_t vadSilencePercent)
{
    return (tmedia_defaults_set_vad_silence_percent (vadSilencePercent) == 0);
}

bool MediaSessionMgr::defaultsSetCommModeEnabled(bool enabled)
{
    return (tmedia_defaults_set_comm_mode_enabled (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetAndroidAudioUnderOpensles(bool enabled)
{
    return (tmedia_defaults_set_android_opensles_enabled (enabled ? tsk_true : tsk_false) == 0);
}

bool MediaSessionMgr::defaultsSetBackgroundMusicDelay (uint32_t delay)
{
    return (tmedia_defaults_set_backgroundmusic_delay (delay) == 0);
}

bool MediaSessionMgr::defaultsSetMaxJitterBufferNum(uint32_t maxJbNum)
{
    return (tmedia_defaults_set_max_jitter_buffer_num (maxJbNum) == 0);
}

bool MediaSessionMgr::defaultsSetMaxNetEqDelayMs(uint32_t maxDelayMs)
{
    return (tmedia_defaults_set_max_neteq_delay_ms (maxDelayMs) == 0);
}

bool MediaSessionMgr::defaultsSetPacketStatLogPeriod(uint32_t periodMs)
{
    return (tmedia_defaults_set_packet_stat_log_period_ms (periodMs) == 0);
}

bool MediaSessionMgr::defaultsSetPacketStatReportPeriod(uint32_t periodMs)
{
    return (tmedia_defaults_set_packet_stat_report_period_ms (periodMs) == 0);
}

bool MediaSessionMgr::defaultsSetVideoPacketStatReportPeriod( uint32_t periodMs )

{
    return (tmedia_defaults_set_video_packet_stat_report_period_ms (periodMs) == 0);
}
bool MediaSessionMgr::defaultsSetMicVolumeGain(uint32_t gain)
{
    return (tmedia_defaults_set_mic_volume_gain (gain) == 0);
}

bool MediaSessionMgr::defaultsSetCameraRotation (int cameraRotation)
{
    return tmedia_defaults_set_camera_rotation (cameraRotation) == 0;
}

int MediaSessionMgr::defaultsGetCameraRotation ()
{
    return tmedia_defaults_get_camera_rotation ();
}

bool MediaSessionMgr::defaultsSetVideoSize (int sizeIdx)
{
    unsigned width = 0 ;
    unsigned height = 0 ;
    auto ret = tmedia_video_get_size((tmedia_pref_video_size_t)sizeIdx, &width, &height);
    if( ret == 0 )
    {
        defaultsSetVideoSize( width, height );
        return true;
    }
    else{
        return false;
    }
}

void MediaSessionMgr::defaultsSetVideoSize ( uint32_t width, uint32_t height )
{
    tmedia_defaults_set_video_size( width, height, 0  );

}
void  MediaSessionMgr::defaultsGetVideoSize ( uint32_t& width, uint32_t& height )
{
    tmedia_defaults_get_video_size( &width, &height );
}

void MediaSessionMgr::defaultsSetCameraCaptureMode(int mode)
{
    tmedia_defaults_set_video_sampling_mode(mode);
}

void MediaSessionMgr::defaultsSetExternalInputMode ( bool enabled )
{    
    tmedia_defaults_set_external_input_mode( enabled );
}

bool MediaSessionMgr::defaultsGetExternalInputMode ( )
{
    auto ret = tmedia_defaults_get_external_input_mode( );
    if (ret == tsk_true)
    {
        return true;
    }
    return false;
}

bool MediaSessionMgr::defaultsSetSpkOutFadeTimeMs (uint32_t timeMs)
{
    return (tmedia_defaults_set_spkout_fade_time(timeMs) == 0);
}

void MediaSessionMgr::defaultsSetMaxBitrateLevelSfu (uint32_t bitrate_level)
{
    tmedia_set_max_video_bitrate_level_for_sfu(bitrate_level);
}

void MediaSessionMgr::defaultsSetMinBitrateLevelSfu (uint32_t bitrate_level)
{
    tmedia_set_min_video_bitrate_level_for_sfu(bitrate_level);
}

void MediaSessionMgr::defaultsSetMaxBitrateLevelP2p (uint32_t bitrate_level)
{
    tmedia_set_max_video_bitrate_level_for_p2p(bitrate_level);
}


void MediaSessionMgr::defaultsSetRtcpMemberCount(uint32_t count)
{
    tmedia_defaults_set_rtcp_memeber_count(count);
}

void MediaSessionMgr::defaultsSetMaxVideoBitrateMultiple (uint32_t multiple)
{
    tmedia_set_max_video_bitrate_multiple(multiple);
}

void MediaSessionMgr::defaultsSetMaxVideoRefreshTimeout (uint32_t timeout)
{
    tmedia_set_max_video_refresh_timeout(timeout);
}

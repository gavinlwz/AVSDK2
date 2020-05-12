
#ifndef TINYWRAP_MEDIA_SESSIONMGR_H
#define TINYWRAP_MEDIA_SESSIONMGR_H

#include "tinyWRAP_config.h"

#include "tinymedia.h"
#include "Common.h"

class TINYWRAP_API Codec
{
    public:
#if !defined(SWIG)
    Codec (const struct tmedia_codec_s *pWrappedCodec);
#endif
    virtual ~Codec ();

    public:
#if !defined(SWIG)
    const struct tmedia_codec_s *getWrappedCodec ()
    {
        return m_pWrappedCodec;
    }
    inline bool isOpened ()
    {
        return (m_pWrappedCodec && (m_pWrappedCodec->opened == tsk_true));
    }
#endif
    twrap_media_type_t getMediaType ();
    const char *getName ();
    const char *getDescription ();
    const char *getNegFormat ();
    int getAudioSamplingRate ();
    int getAudioChannels ();
    int getAudioPTime ();

    private:
    struct tmedia_codec_s *m_pWrappedCodec;
};

class TINYWRAP_API MediaSessionMgr
{
    public:
#if !defined(SWIG)
    MediaSessionMgr (tmedia_session_mgr_t *pWrappedMgr);
#endif
    virtual ~MediaSessionMgr ();

    public:
    bool sessionSetInt32 (twrap_media_type_t media, const char *key, int32_t value);
    int32_t sessionGetInt32 (twrap_media_type_t media, const char *key);
    bool sessionSetInt64 (twrap_media_type_t media, const char *key, int64_t value);
    int64_t sessionGetInt64 (twrap_media_type_t media, const char *key);
    void* sessionGetObject(twrap_media_type_t media, const char *key);
    bool sessionSetObject(twrap_media_type_t media, const char *key, void* value);
    bool sessionSetPvoid(twrap_media_type_t media, const char *key, void* value);

    bool consumerSetInt32 (twrap_media_type_t media, const char *key, int32_t value);
    bool consumerSetInt64 (twrap_media_type_t media, const char *key, int64_t value);
    bool consumerSetPvoid(twrap_media_type_t media, const char *key, void* value);

    bool producerSetInt32 (twrap_media_type_t media, const char *key, int32_t value);
    bool producerGetInt32 (twrap_media_type_t media, const char *key, int32_t *pValue);
    bool producerSetInt64 (twrap_media_type_t media, const char *key, int64_t value);
    bool producerGetInt64 (twrap_media_type_t media, const char *key, int64_t *pValue);
    bool producerSetPvoid (twrap_media_type_t media, const char *key, void* value);
    Codec*  producerGetCodec (twrap_media_type_t media);


    static unsigned int registerAudioPluginFromFile (const char *path);

    uint64_t getSessionId (twrap_media_type_t media) const;

#if !defined(SWIG)
    inline const tmedia_session_mgr_t *getWrappedMgr () const
    {
        return m_pWrappedMgr;
    }
#endif

    // Defaults
    static bool defaultsSetProfile (tmedia_profile_t profile);
    static tmedia_profile_t defaultsGetProfile ();
    static bool defaultsSetBandwidthLevel (tmedia_bandwidth_level_t bl); // @deprecated
    static tmedia_bandwidth_level_t defaultsGetBandwidthLevel ();        // @deprecated
    static bool defaultsSetCongestionCtrlEnabled (bool enabled);
   
    static bool defaultsSetJbMargin (uint32_t jb_margin_ms);
    static bool defaultsSetJbMaxLateRate (uint32_t jb_late_rate_percent);
    static bool defaultsSetEchoTail (uint32_t echo_tail);
    static uint32_t defaultsGetEchoTail ();
    static bool defaultsSetEchoSkew (uint32_t echo_skew);
    static bool defaultsSetAecEnabled (bool aec_enabled);
    static bool defaultsGetAecEnabled ();
    static bool defaultsSetAecOption (uint32_t aec_option);
    static uint32_t defaultsGetAecOption ();
    static bool defaultsSetAgcEnabled (bool agc_enabled);
    static bool defaultsGetAgcEnabled ();
    static bool defaultsSetAgcLevel (float agc_level);
    static float defaultsGetAgcLevel ();
    static bool defaultsSetAgcTargetLevel (int32_t agc_target_level);
    static bool defaultsSetAgcCompressGain (int32_t agc_compress_gain);
    static bool defaultsSetVadEnabled (bool vad_enabled);
    static bool defaultsGetGetVadEnabled ();
    static bool defaultsSetNoiseSuppEnabled (bool noise_supp_enabled);
    static bool defaultsGetNoiseSuppEnabled ();
    static bool defaultsSetHowlingSuppEnabled (bool howling_supp_enabled);
    static bool defaultsGetHowlingSuppEnabled ();
    static bool defaultsSetReverbFilterEnabled (bool reverb_filter_enabled);
    static bool defaultsGetReverbFilterEnabled ();

    static bool defaultsSet100relEnabled (bool _100rel_enabled);
    static bool defaultsGet100relEnabled ();
    static bool defaultsSetScreenSize (int32_t sx, int32_t sy);
    static bool defaultsSetAudioGain (int32_t producer_gain, int32_t consumer_gain);
    static bool defaultsSetAudioPtime (int32_t ptime);
    static bool defaultsSetAudioChannels (int32_t channel_playback, int32_t channel_record);
    static bool defaultsSetMixedCallbackSamplerate (int32_t mixecCallbackSamplerate);
    static bool defaultsSetRtpPortRange (uint16_t range_start, uint16_t range_stop);
    static bool defaultsSetRtpSymetricEnabled (bool enabled);
    static bool defaultsSetMediaType (twrap_media_type_t media_type);
    static bool defaultsSetVolume (int32_t volume);
    static int32_t defaultsGetVolume ();
    static bool defaultsSetInviteSessionTimers (int32_t timeout, const char *refresher);
    
    static void setVideoNetAdjustMode( int mode );
    static bool setVideoNetResolution( uint32_t width, uint32_t height );
	static bool getVideoNetResolution(uint32_t &width, uint32_t &height);
	static bool setVideoNetResolutionForSecond(uint32_t width, uint32_t height);
	static bool getVideoNetResolutionForSecond(uint32_t &width, uint32_t &height);
    static bool setVideoNetResolutionForShare( uint32_t width, uint32_t height );
    static bool setVideoLocalResolution( uint32_t width, uint32_t height );
    static bool getVideoLocalResolution( uint32_t &width, uint32_t &height );
    static bool setVideoFps( uint32_t fps );
    static bool setVideoFpsForSecond( uint32_t fps );
    static bool setVideoFpsForShare( uint32_t fps );

    static bool defaultsSetByPassEncoding (bool enabled);
    static bool defaultsGetByPassEncoding ();
    static bool defaultsSetByPassDecoding (bool enabled);
    static bool defaultsGetByPassDecoding ();
    static bool defaultsSetRtpBuffSize (unsigned buffSize);
    static unsigned defaultsGetRtpBuffSize ();
    static bool defaultsSetAvpfTail (unsigned tail_min, unsigned tail_max);
    static bool defaultsSetAvpfMode (enum tmedia_mode_e mode);
    static bool defaultsSetRecordSampleRate (int32_t record_sample_rate);
    static bool defaultsSetPlaybackSampleRate (int32_t playback_sample_rate);
    static bool defaultsSetMaxFds (int32_t max_fds);

    static bool defaultsSetOpusInbandFecEnabled (bool enabled);
    static bool defaultsSetOpusOutbandFecEnabled (bool enabled);
    static bool defaultsSetOpusPacketLossPerc (int packet_loss_perc);
    static bool defaultsSetOpusDtxPeroid (int peroid);
    static bool defaultsSetOpusEncoderComplexity (int complexity);
    static bool defaultsSetOpusEncoderVbrEnabled (bool enabled);
    static bool defaultsSetOpusEncoderMaxBandwidth (int bandwidth);
    static bool defaultsSetOpusEncoderBitrate (int bitrate);
    static bool defaultsSetRtpFeedbackPeriod (int period);
    static bool defaultsSetAndroidFakedRecording (bool fakedRec);
    static bool defaultsSetIosNoPermissionNoRec (bool enabled);
    static bool defaultsSetIosAirplayNoRec (bool enabled);
    static bool defaultsSetIosAlwaysPlayCategory (bool enabled);
    static bool defaultsSetIosNeedSetMode (bool enabled);
    static bool defaultsSetIosRecoveryDelayFromCall (uint32_t iosRecoveryDelayFromCall);
    static bool defaultsSetAppDocumentPath (const char* path);
    static bool defaultsSetPcmFileSizeKb (uint32_t size_kb);
    static bool defaultsSetH264FileSizeKb (uint32_t size_kb);
    static bool defaultsSetVadLenMs(uint32_t vadLenMs);
    static bool defaultsSetVadSilencePercent(uint32_t vadSilencePercent);
    
    static bool defaultsSetCommModeEnabled(bool enabled);
    static bool defaultsSetAndroidAudioUnderOpensles(bool enabled);
    
    static bool defaultsSetBackgroundMusicDelay (uint32_t delay);
    static bool defaultsSetMaxJitterBufferNum(uint32_t maxJbNum);
    static bool defaultsSetMaxNetEqDelayMs(uint32_t maxDelayMs);
    static bool defaultsSetPacketStatLogPeriod(uint32_t periodMs);
    static bool defaultsSetPacketStatReportPeriod(uint32_t periodMs);
    static bool defaultsSetVideoPacketStatReportPeriod( uint32_t periodMs );
    static bool defaultsSetMicVolumeGain(uint32_t gain);
    static bool defaultsSetSpkOutFadeTimeMs(uint32_t timeMs);
    
    static bool defaultsSetCameraRotation (int camRotation);
    static int  defaultsGetCameraRotation ();
    static bool defaultsSetVideoSize (int sizeIdx);
    
    static void defaultsSetCameraCaptureMode(int mode);

    static void defaultsSetVideoSize ( uint32_t width, uint32_t height );
    static void  defaultsGetVideoSize ( uint32_t& width, uint32_t& height );
    
    static void defaultsSetExternalInputMode ( bool enabled );
    static bool  defaultsGetExternalInputMode ();

    static void defaultsSetMaxBitrateLevelSfu (uint32_t bitrate_level);
    static void defaultsSetMinBitrateLevelSfu (uint32_t bitrate_level);
    static void defaultsSetMaxBitrateLevelP2p (uint32_t bitrate_level);
    static void defaultsSetRtcpMemberCount(uint32_t count);

    static void defaultsSetMaxVideoBitrateMultiple (uint32_t multiple);
    static void defaultsSetMaxVideoRefreshTimeout (uint32_t timeout);
    
    private:
    tmedia_session_mgr_t *m_pWrappedMgr;
};

#endif /* TINYWRAP_MEDIA_SESSIONMGR_H */

//
//  NgnEngine.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "XOsWrapper.h"
#include "NgnEngine.h"
#include "NgnNetworkService.h"
#include "MediaSessionMgr.h"
#include "NgnConfigurationEntry.h"
#include <minizip/MiniZip.h>
#if defined(__APPLE__)
#include "Application.h"
#endif
#include "minizip/MiniZip.h"
#include "YouMeCommonVersion.h"
extern int g_serverMode;

extern YOUME_RTC_SERVER_REGION g_serverRegionId;


NgnEngine *NgnEngine::sInstance = NULL;
NgnEngine::NgnEngine ()
{
}

NgnEngine::~NgnEngine ()
{
    stop ();
    mPNetworkService->stop();
    delete mPNetworkService;
    mPNetworkService = NULL;
    
}


void NgnEngine::initialize ()
{
    NgnApplication::getInstance ()->initPara ();
//    //如果日志大于10M 就删掉日志文件,重新备份
//    FILE* fLog = fopen(NgnApplication::getInstance()->getLogPath().c_str(), "r");
//    if (NULL != fLog) {
//        //获取文件长度
//		XFSeek(fLog, 0, SEEK_END);
//		long long iFileSize = XFTell(fLog);
//        fclose(fLog);
//        if (iFileSize >= 10 *1024 *1024) {
//            //大于10M了，移除以前的备份文件，重新备份一个
//            remove(NgnApplication::getInstance()->getLastBackupLogPath().c_str());
//            //重新备份一个
//            CMiniZip logZip;
//            if (logZip.Open(NgnApplication::getInstance()->getLastBackupLogPath()))
//            {
//                logZip.AddOneFileToZip(NgnApplication::getInstance()->getLogPath());
//                logZip.Close();
//            }
//            remove(NgnApplication::getInstance()->getLogPath().c_str());
//            tsk_uninit_log();
//        }
//    }
    
#ifndef FFMPEG_SUPPORT
#define FFMPEG_SUPPORT 0
#endif

    if (NgnApplication::getInstance()->getUserLogPath().empty()) {
        tsk_init_log(NgnApplication::getInstance()->getLogPath().c_str(), NgnApplication::getInstance()->getBackupLogPath().c_str());
    }else {
        tsk_init_log(NgnApplication::getInstance()->getUserLogPath().c_str(), NULL);
    }
    
    string strPlatform = NgnApplication::getInstance()->getPlatformName();
    TSK_DEBUG_INFO("Brand:%s\n\
        Model:%s\n\
        CPU:%s\n\
        IMEI:%s\n\
        UUID:%s\n\
        sysver:%s\n\
        package:%s\n\
        sdkver:%s-%d.%d.%d.%d\n\
        sdknum:%d\n\
        ffmpeg-support:%d\n\
        commonLibVer:%s\n\
        CPUChip:%s\n\
        ServerArea:%d\n\
        Platform:%s\n\
        TestMode:%d\n",
        NgnApplication::getInstance()->getBrand().c_str(),
        NgnApplication::getInstance()->getModel().c_str(),
        NgnApplication::getInstance()->getCPUArch().c_str(),
        NgnApplication::getInstance()->getDeviceIMEI().c_str(),
        NgnApplication::getInstance()->getUUID().c_str(),
        NgnApplication::getInstance()->getSysVersion().c_str(),
        NgnApplication::getInstance()->getPackageName().c_str(),
        BRANCH_TAG_NAME,
        MAIN_VER,
        MINOR_VER,
        RELEASE_NUMBER,
        BUILD_NUMBER,
        SDK_NUMBER,
        FFMPEG_SUPPORT,
        XStringToUTF8(CYouMeCommonVersion::GetVersion()).c_str(),
        NgnApplication::getInstance()->getCPUChip().c_str(),
                   g_serverRegionId,
                   strPlatform.c_str(),
                   g_serverMode
                   );

    getNetworkService ()->start ();
}


NgnEngine *NgnEngine::getInstance ()
{
    if (sInstance == NULL)
    {
        sInstance = new NgnEngine ();
    }
    return sInstance;
}
void NgnEngine::destroy ()
{
    if (sInstance) {
        sInstance->stop ();
        delete sInstance;
        sInstance = NULL;
    }
}
bool NgnEngine::start ()
{
    if (mStarted == true)
    {
        return true;
    }
    // Profile
    tmedia_profile_t profile = (tmedia_profile_t)(CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MEDIA_PROFILE, NgnConfigurationEntry::DEFAULT_MEDIA_PROFILE));
    MediaSessionMgr::defaultsSetProfile (profile);

    // Set default mediaType to use when receiving bodiless INVITE
    MediaSessionMgr::defaultsSetMediaType (twrap_media_audio);



  
    // codecs, AEC, NoiseSuppression, Echo cancellation, ....
    const bool aec = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AEC, NgnConfigurationEntry::DEFAULT_GENERAL_AEC);
    const bool vad = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_VAD, NgnConfigurationEntry::DEFAULT_GENERAL_VAD);
    const bool nr  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_NR, NgnConfigurationEntry::DEFAULT_GENERAL_NR);
    const bool agc = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AGC, NgnConfigurationEntry::DEFAULT_GENERAL_AGC);
    const bool hs  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_HS, NgnConfigurationEntry::DEFAULT_GENERAL_HS);
    const bool rev = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_REV, NgnConfigurationEntry::DEFAULT_GENERAL_REV);
    const uint32_t aec_opt = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AEC_OPT, NgnConfigurationEntry::DEFAULT_GENERAL_AEC_OPT);

    int echo_tail = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_ECHO_TAIL, NgnConfigurationEntry::DEFAULT_GENERAL_ECHO_TAIL);
    int echo_skew = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::ECHO_SKEW, NgnConfigurationEntry::DEF_ECHO_SKEW);
    int agc_target_level  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AGC_TARGET_LEVEL, NgnConfigurationEntry::DEFAULT_GENERAL_AGC_TARGET_LEVEL);
    int agc_compress_gain = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AGC_COMPRESS_GAIN, NgnConfigurationEntry::DEFAULT_GENERAL_AGC_COMPRESS_GAIN);

    const int jitbuffer_margin = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::GENERAL_JITBUFFER_MARGIN, NgnConfigurationEntry::DEFAULT_JITBUFFER_MARGIN);

    const int jitbuffer_maxlaterate = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_JITBUFFER_MAXLATERATE, NgnConfigurationEntry::DEFAULT_JITBUFFER_MAXLATERATE);

    MediaSessionMgr::defaultsSetAecEnabled (aec);
    MediaSessionMgr::defaultsSetAecOption (aec_opt);
    // EchoTail is in milliseconds,the maximum value is 500ms
    MediaSessionMgr::defaultsSetEchoTail (echo_tail);
    MediaSessionMgr::defaultsSetEchoSkew (echo_skew);
    
    MediaSessionMgr::defaultsSetAgcEnabled (agc);
    MediaSessionMgr::defaultsSetAgcTargetLevel (agc_target_level);
    MediaSessionMgr::defaultsSetAgcCompressGain (agc_compress_gain);
    MediaSessionMgr::defaultsSetVadEnabled (vad);
    MediaSessionMgr::defaultsSetNoiseSuppEnabled (nr);
    MediaSessionMgr::defaultsSetHowlingSuppEnabled (hs);
    MediaSessionMgr::defaultsSetReverbFilterEnabled(rev);
    MediaSessionMgr::defaultsSetJbMargin (jitbuffer_margin);
    MediaSessionMgr::defaultsSetJbMaxLateRate (jitbuffer_maxlaterate);
    
    // supported opus mw_rates: 8000,12000,16000,24000,48000
    // opensl-es playback_rates: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 64000, 88200,
    // 96000, 192000
    // webrtc aec record_rates: 8000, 16000, 32000
    // Record and playback sample rate config
    int iRecordSampleRate   = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_RECORD_SAMPLE_RATE, NgnConfigurationEntry::DEFAULT_RECORD_SAMPLE_RATE);
    int iPlaybackSampleRate = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_RECORD_SAMPLE_RATE, NgnConfigurationEntry::DEFAULT_RECORD_SAMPLE_RATE);
    MediaSessionMgr::defaultsSetRecordSampleRate(iRecordSampleRate);
    MediaSessionMgr::defaultsSetPlaybackSampleRate(iPlaybackSampleRate);
    
    MediaSessionMgr::defaultsSetCongestionCtrlEnabled (false);
  
    // (mono, mono)
    MediaSessionMgr::defaultsSetAudioChannels (1, 1);
    MediaSessionMgr::defaultsSetAudioPtime (20);

    MediaSessionMgr::defaultsSetAvpfMode (tmedia_mode_optional);
    MediaSessionMgr::defaultsSetAvpfTail (30, 160);
    
    bool bOpusInbandFecEnabled  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_INBANDFEC, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_INBANDFEC);
    bool bOpusOutbandFecEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_OUTBANDFEC, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_OUTBANDFEC);
    int  nOpusPacketLossPerc    = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_PACKET_LOSS_PERC, NgnConfigurationEntry::DEFAULT_OPUS_PACKET_LOSS_PERC);
    int nOpusDtxPeroid          = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_DTX_PEROID, NgnConfigurationEntry::DEFAULT_OPUS_DTX_PEROID);
    bool bOpusVbrEnabled        = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_VBR, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_VBR);
    int  nOpusComplexity        = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_COMPLEXITY, NgnConfigurationEntry::DEFAULT_OPUS_COMPLEXITY);
    int  nOpusMaxBandwidth      = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_MAX_BANDWIDTH, NgnConfigurationEntry::DEFAULT_OPUS_MAX_BANDWIDTH);
    int  nOpusBitrate           = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::OPUS_BITRATE, NgnConfigurationEntry::DEFAULT_OPUS_BITRATE);
    int  nRtpFeedbackPeriod     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::RTP_FEEDBACK_PERIOD, NgnConfigurationEntry::DEF_RTP_FEEDBACK_PERIOD);
    bool bIosNoPermissionNoRec  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::IOS_NO_PERMISSION_NO_REC, NgnConfigurationEntry::DEF_IOS_NO_PERMISSION_NO_REC);
    bool bIosAirplayNoRec       = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::IOS_AIRPLAY_NO_REC, NgnConfigurationEntry::DEF_IOS_AIRPLAY_NO_REC);
    bool bIosAlwaysPlayCategory = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::IOS_ALWAYS_PLAY_CATEGORY, NgnConfigurationEntry::DEF_IOS_ALWAYS_PLAY_CATEGORY);
    bool bIosNeedSetMode        = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::IOS_NEED_SET_MODE, NgnConfigurationEntry::DEF_IOS_NEED_SET_MODE);
    uint32_t iosRecoveryDelayFromCall   = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::IOS_RECOVERY_DELAY_FROM_CALL, NgnConfigurationEntry::DEF_IOS_RECOVERY_DELAY_FROM_CALL);
    uint32_t pcmFileSizeKb      = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::PCM_FILE_SIZE_KB, NgnConfigurationEntry::DEF_PCM_FILE_SIZE_KB);
    uint32_t h264FileSizeKb      = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::H264_FILE_SIZE_KB, NgnConfigurationEntry::DEF_H264_FILE_SIZE_KB);
    uint32_t vadLenMs           = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VAD_LEN_MS, NgnConfigurationEntry::DEF_VAD_LEN_MS);
    uint32_t vadSilencePercent  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VAD_SILENCE_PERCENT, NgnConfigurationEntry::DEF_VAD_SILENCE_PERCENT);
    bool bCommModeEnabled       = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
    bool bOpenslesEnabled       = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::ANDROID_UNDER_OPENSLES, NgnConfigurationEntry::DEFAULT_ANDROID_UNDER_OPENSLES);
    uint32_t delay              = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::BACKGROUND_MUSIC_DELAY, NgnConfigurationEntry::DEFAULT_BACKGROUND_MUSIC_DELAY);
    uint32_t maxJitterBufferNum = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_JITTER_BUFFER_NUM, NgnConfigurationEntry::DEF_MAX_JITTER_BUFFER_NUM);
    uint32_t maxNetEqDelayMs    = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_NETEQ_DELAY_MS, NgnConfigurationEntry::DEF_MAX_NETEQ_DELAY_MS);
    uint32_t packetStatLogPeriodMs = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::PACKET_STAT_LOG_PERIOD_MS, NgnConfigurationEntry::DEF_PACKET_STAT_LOG_PERIOD_MS);
    uint32_t packetStatReportPeriodMs   = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::PACKET_STAT_REPORT_PERIOD_MS, NgnConfigurationEntry::DEF_PACKET_STAT_REPORT_PERIOD_MS);
    uint32_t videoPacketStatReportPeriodMs = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_STAT_REPORT_PERIOD_MS,
                                                                                                      NgnConfigurationEntry::DEFAULT_VIDEO_STAT_REPORT_PERIOD_MS );
    uint32_t micVolumeGain              = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MIC_VOLUME_GAIN, NgnConfigurationEntry::DEF_MIC_VOLUME_GAIN);
    uint32_t fadeInTimeMs               = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::FADEIN_TIME_MS, NgnConfigurationEntry::DEF_FADEIN_TIME_MS);
    
    uint32_t maxBitrareLevelSfu     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_VIDEO_BITRATE_LEVEL_SERVER, NgnConfigurationEntry::DEFAULT_MAX_VIDEO_BITRATE_LEVEL_SERVER);
    uint32_t minBitrareLevelSfu     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MIN_VIDEO_BITRATE_LEVEL_SERVER, NgnConfigurationEntry::DEFAULT_MIN_VIDEO_BITRATE_LEVEL_SERVER);
    uint32_t maxBitrateLevelP2p     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_VIDEO_BITRATE_LEVEL_P2P, NgnConfigurationEntry::DEFAULT_MAX_VIDEO_BITRATE_LEVEL_P2P);
    uint32_t maxRtcpMemberCount = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_RTCP_MEMBER_COUNT, NgnConfigurationEntry::DEFAULT_MAX_RTCP_MEMBER_COUNT);

    int cameraCaptureMode = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CAMERA_CAPTURE_MODE, NgnConfigurationEntry::DEF_CAMERA_CAPTURE_MODE);
    
    uint32_t maxVideoBitrateMultiple    = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_BITRATE_MAX_MULTIPLE, NgnConfigurationEntry::DEFAULT_VIDEO_BITRATE_MAX_MULTIPLE);
    uint32_t maxVideoRefreshTimeout     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_FRAME_REFREASH_TIME, NgnConfigurationEntry::DEFAULT_VIDEO_FRAME_REFREASH_TIME);
    uint32_t videoNackFlag     = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_NACK_FLAG, NgnConfigurationEntry::DEFAULT_VIDEO_NACK_FLAG);

    MediaSessionMgr::defaultsSetOpusInbandFecEnabled(bOpusInbandFecEnabled);
    MediaSessionMgr::defaultsSetOpusOutbandFecEnabled(bOpusOutbandFecEnabled);
    MediaSessionMgr::defaultsSetOpusPacketLossPerc(nOpusPacketLossPerc);
    MediaSessionMgr::defaultsSetOpusDtxPeroid(nOpusDtxPeroid);
    MediaSessionMgr::defaultsSetOpusEncoderVbrEnabled(bOpusVbrEnabled);
    MediaSessionMgr::defaultsSetOpusEncoderComplexity(nOpusComplexity);
    MediaSessionMgr::defaultsSetOpusEncoderMaxBandwidth(nOpusMaxBandwidth);
    MediaSessionMgr::defaultsSetOpusEncoderBitrate(nOpusBitrate);
    MediaSessionMgr::defaultsSetRtpFeedbackPeriod(nRtpFeedbackPeriod);
    MediaSessionMgr::defaultsSetIosNoPermissionNoRec(bIosNoPermissionNoRec);
    MediaSessionMgr::defaultsSetIosAirplayNoRec(bIosAirplayNoRec);
    MediaSessionMgr::defaultsSetIosAlwaysPlayCategory(bIosAlwaysPlayCategory);
    MediaSessionMgr::defaultsSetIosNeedSetMode(bIosNeedSetMode);
    MediaSessionMgr::defaultsSetIosRecoveryDelayFromCall(iosRecoveryDelayFromCall);
    
    MediaSessionMgr::defaultsSetPcmFileSizeKb(pcmFileSizeKb);
    MediaSessionMgr::defaultsSetH264FileSizeKb(h264FileSizeKb);
    MediaSessionMgr::defaultsSetVadLenMs(vadLenMs);
    MediaSessionMgr::defaultsSetVadSilencePercent(vadSilencePercent);
    MediaSessionMgr::defaultsSetCommModeEnabled(bCommModeEnabled);
    MediaSessionMgr::defaultsSetAndroidAudioUnderOpensles(bOpenslesEnabled);
    
    MediaSessionMgr::defaultsSetBackgroundMusicDelay(delay);
    
    MediaSessionMgr::defaultsSetAppDocumentPath( NgnApplication::getInstance()->getDocumentPath().c_str() );
    MediaSessionMgr::defaultsSetMaxJitterBufferNum(maxJitterBufferNum);
    MediaSessionMgr::defaultsSetMaxNetEqDelayMs(maxNetEqDelayMs);
    MediaSessionMgr::defaultsSetPacketStatLogPeriod(packetStatLogPeriodMs);
    MediaSessionMgr::defaultsSetPacketStatReportPeriod(packetStatReportPeriodMs);
    MediaSessionMgr::defaultsSetVideoPacketStatReportPeriod( videoPacketStatReportPeriodMs );
    MediaSessionMgr::defaultsSetMicVolumeGain(micVolumeGain);
    MediaSessionMgr::defaultsSetSpkOutFadeTimeMs(fadeInTimeMs);
    MediaSessionMgr::defaultsSetRtcpMemberCount(maxRtcpMemberCount);
    
    MediaSessionMgr::defaultsSetMaxBitrateLevelSfu(maxBitrareLevelSfu);
    MediaSessionMgr::defaultsSetMinBitrateLevelSfu(minBitrareLevelSfu);
    MediaSessionMgr::defaultsSetMaxBitrateLevelP2p(maxBitrateLevelP2p);
    MediaSessionMgr::defaultsSetCameraCaptureMode(cameraCaptureMode);

    MediaSessionMgr::defaultsSetMaxVideoBitrateMultiple(maxVideoBitrateMultiple);
    MediaSessionMgr::defaultsSetMaxVideoRefreshTimeout(maxVideoRefreshTimeout);
    MediaSessionMgr::defaultsSetVideoNackFlag(videoNackFlag);

    // Video part
    int  nCameraRotation        = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CAMERA_ROTATION, NgnConfigurationEntry::DEFAULT_CAMERA_ROTATION);
    int  nVideoSizeIndex        = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_SIZE_INDEX, NgnConfigurationEntry::DEFAULT_VIDEO_SIZE_INDEX);
    
    MediaSessionMgr::defaultsSetCameraRotation(nCameraRotation);
    
/*#ifdef YOUME_FOR_QINIU
    //七牛自己设置分辨率，这里要避免被服务器下发的配置覆盖
#else
    MediaSessionMgr::defaultsSetVideoSize(nVideoSizeIndex);
#endif*/

    //外部输入模式下，如七牛自己设置分辨率，这里要避免被服务器下发的配置覆盖
    if (!MediaSessionMgr::defaultsGetExternalInputMode())
    {
        MediaSessionMgr::defaultsSetVideoSize(nVideoSizeIndex);
    }
    return true;
}

bool NgnEngine::stop ()
{
    if (!mStarted)
    {
        return true;
    }


    bool success = true;
   
    TSK_DEBUG_INFO ("Configuration service stoped.");
       TSK_DEBUG_INFO ("Sip service stoped.");
    success &= getNetworkService ()->stop ();
    TSK_DEBUG_INFO ("Network service stoped.");

    if (success)
    {
        mStarted = false;
    }
    else
    {
        TSK_DEBUG_INFO ("Failed to stop services!");
        mStarted = true;
    }
    return success;
}

bool NgnEngine::isStarted ()
{
    return mStarted;
}

INgnNetworkService *NgnEngine::getNetworkService ()
{
    if (mPNetworkService == NULL)
    {
        mPNetworkService = new NgnNetworkService ();
    }
    return mPNetworkService;
}

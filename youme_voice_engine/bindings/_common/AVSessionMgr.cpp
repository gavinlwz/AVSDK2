//
//  AVSessionMgr.cpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/6.
//  Copyright © 2016年 youme. All rights reserved.
//

#include "AVSessionMgr.hpp"
#include "NgnConfigurationEntry.h"
#include <string>
#include "tinysdp/parsers/tsdp_parser_message.h"
#include "NgnEngine.h"
#include "Common.h"
#include <assert.h>
#include "tnet.h"
#include "tinydav/tdav.h"
#include "tsk_debug.h"
#include "tinydav/audio/tdav_session_audio.h"
#include "tsk_object.h"
#include <YouMeCommon/rscode-1.3/RscodeWrapper2.h>

#include "CustomInputManager.h"
bool CAVSessionMgr::s_bIsInit = false;
CAVSessionMgr::CAVSessionMgr(const std::string&strServerIP,int iRtpPort,int iSessionID)
{
    m_strServerIP = strServerIP;
    m_iRtpPort = iRtpPort;
    m_iSessionID = iSessionID;
}
bool CAVSessionMgr::initialize()
{
    if (s_bIsInit) {
        return s_bIsInit;
    }
    int ret;
    
    if ((ret = tnet_startup ()))
    {
        TSK_DEBUG_ERROR ("tnet_startup failed with error code=%d", ret);
        return false;
    }
    if ((ret = tdav_init ()))
    {
        TSK_DEBUG_ERROR ("tdav_init failed with error code=%d", ret);
        return false;
    }
    rscode_init2();
    s_bIsInit = true;
    return  true;
}
void CAVSessionMgr::deinitialize()
{
    if (s_bIsInit)
    {
        tdav_deinit ();
        tnet_cleanup ();
        s_bIsInit = false;
    }
}
bool CAVSessionMgr::ReStart()
{
    TSK_DEBUG_INFO("Enter");
    UnInit();
    TSK_DEBUG_INFO ("init");
    Init();
    TSK_DEBUG_INFO ("Leave");
    return true;
}

bool CAVSessionMgr::Restart( tmedia_type_t media_type )
{
    if( !m_bInit )
    {
        return false ;
    }
    
    int ret = tmedia_session_mgr_restart( m_mediaMgr, media_type);
    
    return ret == 0 ;
}

bool CAVSessionMgr::Init()
{
    //下面是静态函数，内部会判断
    initialize();
	CCustomInputManager::getInstance()->Init();
    if (m_bInit) {
        return true;
    }
    //获取一下本机的IP地址，不能使用回环地址
    INgnNetworkService* pNetService = NgnEngine::getInstance ()->getNetworkService();
    if (NULL == pNetService) {
        TSK_DEBUG_ERROR ("Failed to get network service");
        return false;
    }

    if (checkSupportIPV6()) { // ipv6
        m_mediaMgr = tmedia_session_mgr_create(tmedia_audiovideo, pNetService->getLocalIP(true).c_str(), true, true);
    } else {
        m_mediaMgr = tmedia_session_mgr_create(tmedia_audiovideo, pNetService->getLocalIP(false).c_str(), false, true);
    }
    
    if (NULL == m_mediaMgr) {
        TSK_DEBUG_ERROR ("tmedia_session_mgr_create failed");
        return false;
    }
    //int iSampleRate = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_RECORD_SAMPLE_RATE, NgnConfigurationEntry::DEFAULT_RECORD_SAMPLE_RATE);
    int iSampleRate = tmedia_defaults_get_record_sample_rate();
    
    m_pMediaSessionMgr = new MediaSessionMgr(m_mediaMgr);
    tsdp_message_t* sdp =tsdp_message_parse(m_strServerIP.c_str(),m_iRtpPort,iSampleRate);
    assert(sdp != NULL);
    if (NULL == sdp) {
        TSK_DEBUG_ERROR ("tsdp_message_parse failed");
        return false;
    }
    const tsdp_message_t *sdp_lo = tmedia_session_mgr_get_lo(m_mediaMgr,m_iSessionID);
    if (NULL == sdp_lo) {
        TSK_DEBUG_ERROR ("tmedia_session_mgr_get_lo failed");
        return false;
    }
    tmedia_session_mgr_set_ro(m_mediaMgr,sdp,tmedia_ro_type_answer,m_iSessionID);
    TSK_OBJECT_SAFE_FREE(sdp);
    m_bInit = true;
    return true;
}

void CAVSessionMgr::startSession()
{
    tmedia_session_mgr_start(m_mediaMgr);
}

void CAVSessionMgr::setSpeakerMute (bool mute)
{
    if (m_pMediaSessionMgr->consumerSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SPEAKER_MUTE, mute ? 1 : 0)
        && (m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SPEAKER_MUTE, mute ? 1 : 0)) )
    {
        mSpeakerMuteOn = mute;
    }

}
bool CAVSessionMgr::isSpeakerMute ()
{
    return mSpeakerMuteOn;
}

bool CAVSessionMgr::isMicrophoneMute ()
{
    return mMicphoneMuteOn;
}
void CAVSessionMgr::setMicrophoneMute (bool mute)
{
    if (m_pMediaSessionMgr->producerSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MICROPHONE_MUTE, mute ? 1 : 0)
        && (m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MICROPHONE_MUTE, mute ? 1 : 0)) )
    {
        mMicphoneMuteOn = mute;
    }
}

bool CAVSessionMgr::getRecordingError (int32_t *pErrCode, int32_t *pExtraCode)
{
    if ((NULL == pErrCode) || (NULL == pExtraCode)) {
        return false;
    }
    
    bool ret;
    
    ret = m_pMediaSessionMgr->producerGetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_RECORDING_ERROR, pErrCode);
    if (!ret) {
        return false;
    }
    
    ret = m_pMediaSessionMgr->producerGetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA, pExtraCode);
    if (!ret) {
        *pExtraCode = 0;
    }

    return true;
}

void CAVSessionMgr::onHeadsetPlugin (int state)
{
    if (NULL == m_pMediaSessionMgr) {
        return;
    }
    m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_HEADSET_PLUGIN,state);
    return;
}

bool CAVSessionMgr::setVolume (unsigned int uiVolume)
{
    if (m_pMediaSessionMgr->consumerSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SPEAKER_VOLUME, uiVolume))
    {
        return true;
    }
    return false;
}
bool CAVSessionMgr::setSmartSettingEnabled (bool enabled)
{
    return m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SMART_SETTING, enabled ? 1 : 0);
}

bool CAVSessionMgr::setAGCEnabled (bool enabled)
{
   return m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_AGC_ENABLED, enabled ? 1 : 0);
}


bool CAVSessionMgr::setAECEnabled (bool enabled)
{
    return  m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_AEC_ENABLED, enabled ? 1 : 0);
}
bool CAVSessionMgr::setNSEnabled (bool enabled)
{
    return m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_NS_ENABLED, enabled ? 1 : 0);
}

bool CAVSessionMgr::setVADEnabled (bool enabled)
{
    return m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_VAD_ENABLED, enabled ? 1 : 0);
}

//变声接口
void CAVSessionMgr::setSoundtouchEnabled(bool enabled)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SOUND_TOUCH_ENABLED, enabled ? 1 : 0);
}

void CAVSessionMgr::setSoundtouchTempo(float fTempo)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SOUND_TOUCH_TEMPO, fTempo * 100);
}

void CAVSessionMgr::setSoundtouchRate(float fRate)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SOUND_TOUCH_RATE, fRate * 100);
}

void CAVSessionMgr::setSoundtouchPitch(float fPitch)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SOUND_TOUCH_PITCH, fPitch * 100);
}

bool CAVSessionMgr::setMixAudioTrack(const void* pBuf, int nSizeInByte, int nChannelNum, int nSampleRateHz,
                                           int nBytesPerSample, uint64_t nTimeStamp, bool bFloat, bool bLittleEndian, bool bInterleaved, bool bForSpeaker)
{
    bool bRet = false;
    
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    tdav_session_audio_frame_buffer_t *frame_buf =
        (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, 0);
    if (!frame_buf) {
        return false;
    }

    frame_buf->pcm_data = (void*)pBuf;
    frame_buf->pcm_size = nSizeInByte;
    frame_buf->alloc_size = 0; // indicate the object destructor not to free the memory pointed to by pcm_data
    frame_buf->channel_num = (uint8_t)nChannelNum;
    frame_buf->sample_rate = (uint32_t)nSampleRateHz;
    frame_buf->is_float = bFloat ? 1 : 0;
    frame_buf->is_interleaved = bInterleaved ? 1 : 0;
    frame_buf->bytes_per_sample = (uint8_t)nBytesPerSample;
    frame_buf->timeStamp = nTimeStamp;
    if (bForSpeaker) {
        bRet = m_pMediaSessionMgr->sessionSetObject(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_SPEAKER, (void*)frame_buf);
    } else {
        bRet = m_pMediaSessionMgr->sessionSetObject(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_MICPHONE, (void*)frame_buf);
    }
    tsk_object_unref(frame_buf);
    
    return bRet;
}


//
// Enable/disable mixing audio track into the speaker/microphone.
// setMixAudioTrack() is a blocking function, it may block to wait for the audio data
// to be consumed by the low level. If at some point, we want setMixAudioTrack() to
// return immediately(for example, responding to certain UI operations).
// Note: By default, mixAudioTrack is enabled. You disable it only when you need to make sure setMixAudioTrack()
//       returns immediately(usually for cleanup). After you have done with it, you need to enable it again.
//       So the mixAudioTrack() is kept in enabled state whenever it's possible to mix audio.
bool CAVSessionMgr::setMixAudioTrackEnabled(uint8_t u1PathId, uint8_t u1Enable)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    int32_t i4Paras = 0;
    i4Paras |= (u1PathId << 8);
    i4Paras |= (u1Enable);
    bool bRet = m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_ENABLED, i4Paras);
    return bRet;
}

bool CAVSessionMgr::setSaveScreenEnabled( bool enabled )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return  m_pMediaSessionMgr->sessionSetInt32 (twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_SAVE_SCREEN, enabled ? 1 : 0);
}

//
// Set the volume (0 - 100) of the audio track that's going to be mixed.
// The volume will be adjusted before mixing into the speaker or microphone.
//
bool CAVSessionMgr::setMixAudioTrackVolume(uint8_t u1PathId, uint8_t u1Volume)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    int32_t i4Paras = 0;
    i4Paras |= (u1PathId << 8);
    i4Paras |= (u1Volume);
    bool bRet = m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME, i4Paras);
    return bRet;
}

void CAVSessionMgr::setMicBypassToSpeaker(bool enabled)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIC_BYPASS_TO_SPEAKER, enabled ? 1 : 0);
}

void CAVSessionMgr::setBgmBypassToSpeaker(bool enabled)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_BGM_BYPASS_TO_SPEAKER, enabled ? 1 : 0);
}

void CAVSessionMgr::setReverbEnabled(bool enabled)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_REVERB_ENABLED, enabled ? 1 : 0);
}

void CAVSessionMgr::setBackgroundMusicDelay(uint32_t delay)
{
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_BGM_DELAY, delay);
}


void CAVSessionMgr::UnInit()
{
    if (!m_bInit) {
        return ;
    }
	CCustomInputManager::getInstance()->UnInit();
    delete m_pMediaSessionMgr;
    m_pMediaSessionMgr = nullptr;
    tmedia_session_mgr_stop(m_mediaMgr);
    tmedia_session_mgr_cleanup(m_mediaMgr);
   
    m_bInit = false;
}

void CAVSessionMgr::releaseUserResource(int iSessionID, tmedia_type_t type)
{
    tmedia_session_mgr_clear(m_mediaMgr, iSessionID, type);
}

bool CAVSessionMgr::setVadCallback(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_VAD_CALLBACK, callback);
}

bool CAVSessionMgr::setRecordingTimeMs(uint32_t timeMs)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_RECORDING_TIME_MS, (int32_t)timeMs);
}

bool CAVSessionMgr::setPlayingTimeMs(uint32_t timeMs)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PLAYING_TIME_MS, (int32_t)timeMs);
}

bool CAVSessionMgr::setPcmCallback(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PCM_CALLBACK, callback);
}

bool CAVSessionMgr::setPcmCallbackFlag( int flag )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PCM_CALLBACK_FLAG,  flag );
}

bool CAVSessionMgr::setTranscriberEnable( bool enable )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_TRANSCRIBER_ENABLED,  int(enable) );
}

bool CAVSessionMgr::setPCMCallbackChannel( int nPCMCallbackChannel )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PCM_CALLBACK_CHANNEL,  nPCMCallbackChannel );
}

bool CAVSessionMgr::setPCMCallbackSampleRate( int nPCMCallbackSampleRate )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PCM_CALLBACK_SAMPLE_RATE,  nPCMCallbackSampleRate );
}

uint32_t CAVSessionMgr::getRtpTimestamp()
{
    return  (uint32_t)m_pMediaSessionMgr->sessionGetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_RTP_TIMESTAMP);
}

bool CAVSessionMgr::setVideoRenderCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->consumerSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_RENDER_CB, callback);
    m_pMediaSessionMgr->producerSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_RENDER_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setVideoRuntimeEventCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_RUNTIME_EVENT_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setVideoEncodeAdjustCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_ENCODE_ADJUST_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setVideoDecodeParamCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_DECODE_PARAM_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setVideoEventCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_EVENT_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setStaticsQosCb(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_STATICS_QOS_CB, callback);
    
    return true;
}

bool CAVSessionMgr::setSessionId(int32_t sessionId)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->producerSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_SESSION_ID, sessionId);
    
    return true;
}

bool CAVSessionMgr::setNeedOpenCamera(tsk_bool_t needOpen)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->producerSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_NEED_OPEN_CAMERA, needOpen?1:0);
    
    return true;
}

void CAVSessionMgr::setSessionStartFailCallback( void* callback )
{
    if (!m_mediaMgr) {
        return ;
    }
    
    tmedia_session_set_session_start_fail_callback( m_mediaMgr, (SessionStartFailCallback_t)callback );
}

bool CAVSessionMgr::setVideoWidth(int32_t value)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->producerSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_WIDTH, value);
    
    return true;
}

bool CAVSessionMgr::setVideoHeight(int32_t value)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->producerSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_HEIGHT, value);
    
    return true;
}

bool CAVSessionMgr::setScreenOrientation(int32_t value)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_SCREEN_ORIENTATION, value);
    m_pMediaSessionMgr->producerSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_SCREEN_ORIENTATION, value);
    
    return true;
}

bool CAVSessionMgr::setVideoPreDecodeCallback(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_CB, callback);
}

bool CAVSessionMgr::setVideoPreDecodeNeedDecodeandRender( bool needDecodeandRender )
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_NEED_DECODE_AND_RENDER, needDecodeandRender?1:0);
}

uint32_t CAVSessionMgr::getMixEffectFreeBuffCount()
{
    if (!m_pMediaSessionMgr) {
        TSK_DEBUG_ERROR ("getMixEffectFreeBuffCount error: m_pMediaSessionMgr is null");
        return 0;
    }
    return  (uint32_t)(m_pMediaSessionMgr->sessionGetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_EFFECT_FREE_BUFF_COUNT));
}

uint32_t CAVSessionMgr::getMixAudioFreeBuffCount()
{
    if (!m_pMediaSessionMgr) {
        TSK_DEBUG_ERROR ("getMixEffectFreeBuffCount error: m_pMediaSessionMgr is null");
        return 0;
    }
    return  (uint32_t)(m_pMediaSessionMgr->sessionGetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIX_AUDIO_FREE_BUFF_COUNT));
}

bool CAVSessionMgr::setMicLevelCallback(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MIC_LEVEL_CALLBACK, callback);
}

bool CAVSessionMgr::setMaxMicLevelCallback(int32_t maxLevel)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }

    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MAX_MIC_LEVEL_CALLBACK, maxLevel);
}

bool CAVSessionMgr::setFarendVoiceLevelCallback(void* callback)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetPvoid(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_FAREND_VOICE_LEVEL_CALLBACK, callback);
}

bool CAVSessionMgr::setMaxFarendVoiceLevel(int32_t maxLevel)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_MAX_FAREND_VOICE_LEVEL, maxLevel);
}

bool CAVSessionMgr::setOpusInbandFecEnabled(bool enabled)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_OPUS_INBAND_FEC_ENABLE, enabled?1:0);
}

bool CAVSessionMgr::setOpusPacketLossPerc(int32_t packetLossPerc)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_OPUS_SET_PACKET_LOSS_PERC, packetLossPerc);
}

bool CAVSessionMgr::setOpusBitrate(int32_t bitrate)
{
    if (!m_pMediaSessionMgr) {
        return false;
    }
    
    return m_pMediaSessionMgr->sessionSetInt32(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_OPUS_SET_BITRATE, bitrate);
}

tdav_session_av_stat_info_t* CAVSessionMgr::getPacketStat()
{
    if (!m_pMediaSessionMgr) {
        return tsk_null;
    }
    
    tdav_session_av_stat_info_t *stat_info = (tdav_session_av_stat_info_t *)m_pMediaSessionMgr->sessionGetObject(twrap_media_type_t::twrap_media_audio, TMEDIA_PARAM_KEY_PACKET_STAT);
    return stat_info;
}

tdav_session_av_qos_stat_t* CAVSessionMgr::getAVQosStat()
{
    if (!m_pMediaSessionMgr) {
        return tsk_null;
    }
    
    tdav_session_av_qos_stat_t *stat_info = (tdav_session_av_qos_stat_t *)m_pMediaSessionMgr->sessionGetObject(twrap_media_type_t::twrap_media_video, TMEDIA_PARAM_KEY_AV_QOS_STAT);
    return stat_info;
}

tmedia_session_t* CAVSessionMgr::getSession(tmedia_type_t type) {
    tmedia_session_t* session = tsk_null;
    if(!m_mediaMgr) {
        TSK_DEBUG_ERROR ("m_mediaMgr is null");
        return tsk_null;
    }
    tsk_list_item_t *item = tsk_null;
    tsk_list_lock(m_mediaMgr->sessions);
    tsk_list_foreach(item, m_mediaMgr->sessions) {
        if(type == TMEDIA_SESSION(item->data)->type) {
            session = TMEDIA_SESSION(item->data);
        }
    }
    tsk_list_unlock(m_mediaMgr->sessions);
    return session;
}

tmedia_session_t* CAVSessionMgr::getMediaSession(tmedia_type_t type) {
    return tmedia_session_mgr_find(m_mediaMgr, type);
}

bool CAVSessionMgr::checkSupportIPV6()
{
    bool supportIpv6 = false;
    
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    struct addrinfo * result = NULL;
    
    int iRet = getaddrinfo(m_strServerIP.c_str(), "http", &hints, &result);
    
    if (iRet == 0)
    {
        struct addrinfo* pAddrInfo = NULL;
        
        for (pAddrInfo = result; pAddrInfo != NULL; pAddrInfo = pAddrInfo->ai_next)
        {
            if (pAddrInfo->ai_family == AF_INET)
            {
                char buf[32] = {0};
                struct sockaddr_in addr_in = *((struct sockaddr_in *)pAddrInfo->ai_addr);
                tnet_inet_ntop(AF_INET, &addr_in.sin_addr, buf, 32);
                m_strServerIP = buf;
                supportIpv6 = false;
                break;
            }
            
            else if (pAddrInfo->ai_family == AF_INET6)
            {
                char buf[128] = {0};
                struct sockaddr_in6 addr_in6 = *((struct sockaddr_in6 *)pAddrInfo->ai_addr);
                tnet_inet_ntop(AF_INET6, &addr_in6.sin6_addr, buf, 128);
                m_strServerIP = buf;
                supportIpv6 = true;
                break;
            }
        }
        
        freeaddrinfo(result);
    }
    
    return supportIpv6;
}


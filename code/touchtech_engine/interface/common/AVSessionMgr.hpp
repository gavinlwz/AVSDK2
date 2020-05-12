//
//  AVSessionMgr.hpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/6.
//  Copyright © 2016年 youme. All rights reserved.
//

#ifndef AVSessionMgr_hpp
#define AVSessionMgr_hpp

#include <stdio.h>
#include <string>
#include "MediaSessionMgr.h"
#include "tinymedia/tmedia_session.h"
#include "tinydav/tdav_session_av.h"
class CAVSessionMgr
{
public:
    CAVSessionMgr(const std::string&strServerIP,int iRtpPort,int iSessionID);
    static bool initialize();
    static void deinitialize();
    
    
    bool ReStart();
    bool Restart( tmedia_type_t media_type );
    
    bool Init();
    void UnInit();
    //对外接口
    void setSpeakerMute (bool mute);
    bool isSpeakerMute ();
    bool isMicrophoneMute ();
    void setMicrophoneMute (bool mute);
    bool setVolume (unsigned int uiVolume);
    bool setSmartSettingEnabled (bool enabled);
    bool getRecordingError (int32_t *pErrCode, int32_t *pExtraCode);
    
    
    
    bool setAECEnabled (bool enabled);
    bool setNSEnabled (bool enabled);
     bool setAGCEnabled (bool enabled);
    bool setVADEnabled (bool enabled);
    void onHeadsetPlugin (int state);
    
    //变声接口
    void setSoundtouchEnabled(bool enabled);
    void setSoundtouchTempo(float fTempo);
    void setSoundtouchRate(float fRate);
    void setSoundtouchPitch(float fPitch);
    
    bool setMixAudioTrack(const void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRateHz,
                                int nBytesPerSample, uint64_t nTimeStamp, bool bFloat, bool bLittleEndian, bool bInterleaved, bool bForSpeaker);

    bool setSaveScreenEnabled( bool enabled );
    bool setMixAudioTrackEnabled(uint8_t u1PathId, uint8_t u1Enable);
    bool setMixAudioTrackVolume(uint8_t u1PathId, uint8_t u1Enable);
    void setBgmBypassToSpeaker(bool enabled);
    void setMicBypassToSpeaker(bool enabled);
    void setReverbEnabled(bool enabled);
    void setBackgroundMusicDelay(uint32_t delay);

    uint32_t getMixEffectFreeBuffCount();
    uint32_t getMixAudioFreeBuffCount();

    uint32_t getRtpTimestamp();
    tdav_session_av_stat_info_t* getPacketStat();
    tdav_session_av_qos_stat_t* getAVQosStat();     // 用于audio/video qos数据上报
    bool setVadCallback(void* callback);
    bool setRecordingTimeMs(uint32_t timeMs);
    bool setPlayingTimeMs(uint32_t timeMs);
    bool setPcmCallback(void* callback);
    bool setPcmCallbackFlag( int flag );
    bool setPCMCallbackChannel( int nPCMCallbackChannel );
    bool setPCMCallbackSampleRate( int nPCMCallbackSampleRate );
    bool setTranscriberEnable( bool enable );
    bool setVideoPreDecodeCallback(void* callback);
    bool setVideoPreDecodeNeedDecodeandRender( bool needDecodeandRender );
    bool setMicLevelCallback(void* callback);
    bool setMaxMicLevelCallback(int32_t maxLevel);
    bool setFarendVoiceLevelCallback(void* callback);
    bool setMaxFarendVoiceLevel(int32_t maxLevel);
    bool setOpusInbandFecEnabled(bool enabled);
    bool setOpusPacketLossPerc(int32_t packetLossPerc);
    bool setOpusBitrate(int32_t bitrate);
    bool setSessionId(int32_t sessionId);
    bool setNeedOpenCamera(tsk_bool_t needOpen);
    bool setVideoWidth(int32_t value);
    bool setVideoHeight(int32_t value);
    bool setScreenOrientation(int32_t value);
    tmedia_session_t* getSession(tmedia_type_t type);
    tmedia_session_t* getMediaSession(tmedia_type_t type);
    void setSessionStartFailCallback( void* callback );
    void startSession();

    void releaseUserResource(int iSessionID, tmedia_type_t type);
private:
    bool checkSupportIPV6();

private:
    bool mIOSMuteOn = false;
    bool mSpeakerMuteOn = false;
    bool mMicphoneMuteOn = false;
    bool m_bInit= false;
    std::string m_strServerIP;
    int m_iRtpPort;
    int m_iSessionID;
    static bool s_bIsInit;
    tmedia_session_mgr_t* m_mediaMgr=NULL;
    MediaSessionMgr*m_pMediaSessionMgr =NULL;
    
// video part
public:
    bool setVideoRenderCb(void* callback);
    bool setVideoRuntimeEventCb(void* callback);
    bool setStaticsQosCb(void* callback);
    bool setVideoEncodeAdjustCb(void* callback);
    bool setVideoDecodeParamCb(void* callback);
    bool setVideoEventCb(void* callback);
};
#endif /* AVSessionMgr_hpp */

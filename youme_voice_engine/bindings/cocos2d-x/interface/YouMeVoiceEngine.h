//
//  YouMeVoiceEngine.h
//  cocos2d-x sdk
//
//  Created by wanglei on 15/9/6.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __cocos2d_x_sdk__YouMeVoiceEngine__
#define __cocos2d_x_sdk__YouMeVoiceEngine__
#include <mutex>
#include <string>
using namespace std;

#include "INgnNetworkService.h"
#include "IYouMeEventCallback.h"
#include "IYouMeVideoCallback.h"
#include "IYouMeVoiceEngine.h"
#include "NgnLoginService.hpp"
#include "YouMeConstDefine.h"
#include "YouMeInternalDef.h"
#include "tsk_thread.h"
#include "AVSessionMgr.hpp"
#include "NgnMemoryConfiguration.hpp"
#include "serverlogin.pb.h"
#include "YoumeRunningState.pb.h"
#include "XCondWait.h"
#include "YouMeConstDefine.h"
#include "SDKValidateTalk.h"
#include "tdav_session_audio.h"
#include <map>
#include <mutex>
#include <future>
#include <queue>
#include <condition_variable>
#include "ICameraManager.hpp"
#include "YMAudioMixer.hpp"

#include "YouMeCommon/XSemaphore.h"


class NgnEngine;
class INgnConfigurationService;
class INgnSipService;
class NgnAVSession;

class CMessageBlock;
class CMessageLoop;
struct QueryHttpInfo;

class CRoomManager;

struct TranslateInfo
{
    unsigned int requestID;
    YouMeLanguageCode srcLangCode;
    YouMeLanguageCode destLangCode;
    std::string text;
    TranslateInfo() : requestID(0), srcLangCode(LANG_AUTO), destLangCode(LANG_AUTO){}
};

class CYouMeVoiceEngine : public INgnNetworkChangCallback, public NgnLoginServiceCallback
{
    enum ROOM_MODE_e {
        ROOM_MODE_NONE = 0,
        ROOM_MODE_SINGLE,
        ROOM_MODE_MULTI
    };

    // TODO: remove it
    typedef enum YouMeRtcBackgroundMusicMode {
        YOUME_RTC_BGM_TO_SPEAKER = 0,  ///< 背景音乐混合到输出语音，并通过扬声器播出
        YOUME_RTC_BGM_TO_MIC = 1  ///<　背景音乐混合到麦克风语音，并一起发送给接收端
    } YouMeRtcBackgroundMusicMode_t;
    
public:
    static CYouMeVoiceEngine *getInstance ();
    static void destroy ();

public:
	static bool ToYMData(int msg, int msgex, int err, YouMeEvent* ym_evt, YouMeErrorCode* ym_err);
	std::string ToYMRoomID(const std::string& room_id);
	bool NeedMic(){ return mNeedMic || mInviteMicOK; }
	bool NeedMic(YouMeUserRole_t eUserRole){ return eUserRole != YOUME_USER_LISTENER && eUserRole != YOUME_USER_NONE; }
public:
    //公共接口
    YouMeErrorCode init (IYouMeEventCallback *pEventCallback, const std::string &strAPPKey, const std::string &strAPPSecret,
                         YOUME_RTC_SERVER_REGION serverRegionId);
    
    YouMeErrorCode setJoinChannelKey( const std::string &strAPPKey );
    void setToken( const char* pToken, uint32_t uTimeStamp = 0);
    YouMeErrorCode unInit ();
    void setServerRegion(YOUME_RTC_SERVER_REGION serverRegionId, const std::string &extServerRegionName, bool bAppend);

    YouMeErrorCode start ();
    YouMeErrorCode stop ();

    // YouMeErrorCode login (const string &strUserName, const string &strPwd);

    // YouMeErrorCode logout ();

    /**
     *  功能描述:扬声器静音打开/关闭
     *
     *  @param bOn:true——打开，false——关闭
     *  @return 无
     */
    YouMeErrorCode setSpeakerMute (bool bOn);

    /**
     *  功能描述:获取扬声器静音状态
     *
     *  @return true——打开，false——关闭
     */
    bool getSpeakerMute ();

    bool isMicrophoneMute ();
    YouMeErrorCode setMicrophoneMute (bool mute);
    
    void setAutoSendStatus( bool bAutoSend );

    //获取当前音量大小
    unsigned int getVolume ();
    void setVolume (const unsigned int &uiVolume);

    YouMeErrorCode setLocalConnectionInfo(std::string strLocalIP, int iLocalPort, std::string strRemoteIP, int iRemotePort, int iNetType = 0);
    YouMeErrorCode clearLocalConnectionInfo();
    YouMeErrorCode setRouteChangeFlag(int enable);

    //多人语音接口
    YouMeErrorCode joinChannelSingleMode(const std::string & strUserID, const std::string & strChannelID, YouMeUserRole_t eUserRole, bool bVideoAutoRecv);
    YouMeErrorCode joinChannelMultiMode(const std::string & strUserID, const std::string & strChannelID, YouMeUserRole_t eUserRole);
    
    YouMeErrorCode joinChannelSingleMode(const std::string & strUserID, const std::string & strChannelID, YouMeUserRole_t eUserRole, const std::string & strJoinAppKey, bool bVideoAutoRecv);

    
    YouMeErrorCode speakToChannel(const std::string & strRoomID);
    
    YouMeErrorCode leaveChannelMultiMode (const string &strChannelID);
    YouMeErrorCode leaveChannelAll();
    
    YouMeErrorCode getChannelUserList( const char*  channelID,int maxCount, bool notifyMemChange );

    //双人语音接口

    //设置接口，可选
    //是否使用移动网路
    bool getUseMobileNetWorkEnabled ();
    void setUseMobileNetworkEnabled (bool bEnabled = false);

    /**
     *  功能描述:获取启用/禁用声学回声消除功能状态，启用智能参数后AEC和ANS的手动设置失效，SDK会根据需要智能控制，即可保证通话效果又能降低CPU和电量消耗
     *
     *  @return true-启用，false-禁用
     */


    //启用/不启用声学回声消除,默认启用
    bool getAECEnabled ();
    void setAECEnabled (bool bEnabled = true);

    //启用/不启用背景噪音抑制功能,默认启用
    bool getANSEnabled ();
    void setANSEnabled (bool bEnabled = true);

    //启用/不启用自动增益补偿功能,默认启用
    bool getAGCEnabled ();
    void setAGCEnabled (bool bEnabled = true);

    //启用/不启用静音检测功能,默认启用
    bool getVADEnabled ();
    void setVADEnabled (bool bEnabled = true);

    //变声功能参数设置
    void setSoundtouchEnabled(bool bEnabled);
    void setSoundtouchTempo(float fTempo);
    void setSoundtouchRate(float fRate);
    void setSoundtouchPitch(float fPitch);
    YouMeErrorCode sendSessionUserIdMapRequest(int sessionID);
    
    void sendAVStaticReport( YouMeProtocol::YouMeVoice_Command_BussReport_Req&  request );
	
	void setRestApiCallback(IRestApiCallback* cb );
    
	void setMemberChangeCallback( IYouMeMemberChangeCallback* cb );
	void setNotifyCallback(IYouMeChannelMsgCallback* cb);
    void setAVStatisticCallback( IYouMeAVStatisticCallback* cb );
	// 抢麦接口
	YouMeErrorCode setGrabMicOption(const std::string & strChannelID, int mode, int maxAllowCount, int maxTalkTime, unsigned int voteTime);
	YouMeErrorCode startGrabMicAction(const std::string & strChannelID, const string& strContent);
	YouMeErrorCode stopGrabMicAction(const std::string & strChannelID, const string& strContent);
	YouMeErrorCode requestGrabMic(const std::string & strChannelID, int score, bool isAutoOpenMic, const string& strContent);
	YouMeErrorCode releaseGrabMic(const std::string & strChannelID);
	// 连麦接口
	YouMeErrorCode setInviteMicOption(const std::string & strChannelID, int waitTimeout, int maxTalkTime);
	YouMeErrorCode requestInviteMic(const std::string& strChannelID, const std::string & strUserID, const string& strContent);
	YouMeErrorCode responseInviteMic(const std::string & strUserID, bool isAccept, const string& strContent);
	YouMeErrorCode stopInviteMic();

	//房间内点对点发送信令消息
    YouMeErrorCode  sendMessage( const char* pChannelID,  const char* pContent, const char* pUserID, int* requestID );
    
    YouMeErrorCode kickOther( const char* pUserID, const char* pChannelID , int lastTime );
    //美颜开关
    YouMeErrorCode openBeautify(bool open) ;
    //美颜参数，0.0 - 1.0
    YouMeErrorCode beautifyChanged(float param) ;
    //瘦脸开关，true 开启瘦脸，false关闭
    YouMeErrorCode stretchFace(bool stretch) ;
	
    tmedia_session_t* getMediaSession(tmedia_type_t type);
    void restartAVSessionAudio( );
	// 进入房间后，切换身份
	YouMeErrorCode setUserRole(YouMeUserRole_t eUserRole);
	// 获取身份
	YouMeUserRole_t getUserRole();
	// 背景音乐是否在播放
	bool isBackgroundMusicPlaying();
	// 是否初始化成功
	bool isInited();
	// 是否在某个语音房间内
	bool isInRoom(const string& strChannelID);
    // 是否在语音房间内
    bool isInRoom();
    // 设置用户自定义Log路径
    YouMeErrorCode setUserLogPath(const std::string& strFilePath);
    
    bool releaseMicSync();
    bool resumeMicSync();
    
    int  getCameraCount();
    std::string getCameraName(int cameraId);
    YouMeErrorCode setOpenCameraId(int cameraId);
    
    int getRecordDeviceCount();
    bool getRecordDeviceInfo(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceUId[MAX_DEVICE_ID_LENGTH]);
    YouMeErrorCode setRecordDevice( const char* deviceUId);
    
    void setMixVideoSize(int width, int height);
    void addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height);
    void removeMixOverlayVideo(std::string userId);
    void removeAllOverlayVideo();
    YouMeErrorCode setExternalFilterEnabled(bool enabled);
    
    void setOpusPacketLossPerc(int audio_loss);
    void setOpusBitrate(int audio_loss);
    
public:
	std::string GetJoinUserName()
	{
		return mStrUserID;
	}

    uint64_t getBusinessId();

    virtual void onNetWorkChanged (NETWORK_TYPE type) override;

   //心跳通知过来的断开了
    virtual void OnDisconnect() override;
    virtual void OnKickFromChannel( const std::string& strRoomID, const std::string& strParam  ) override;
    virtual void OnCommonStatusEvent(STATUS_EVENT_TYPE_t eventType, const std::string&strUserID,int iStauts) override;
    virtual void OnRoomEvent(const std::string& strRoomIDFull, RoomEventType eventType, RoomEventResult result) override;
//    //接收成员麦克风状态控制指令
//    virtual void OnReciveMicphoneControllMsg(const std::string&strUserID,int iStauts) override;
//    //接收成员扬声器状态控制指令
//    virtual void OnReciveSpeakerControllMsg(const std::string&strUserID,int iStauts) override;
    //接收成员列表变化通知
    //virtual void OnReciveRoomMembersChangeNotify(const std::string &strUserIDs) override;
    virtual void OnReciveOtherVideoDataOnNotify(const std::string &strUserIDs, const std::string & strRoomId,uint16_t videoID, uint16_t width, uint16_t height);
//    virtual void OnReciveOtherVideoDataOffNotify(const std::string &strUserIDs, const std::string & strRoomId);
//    virtual void OnReciveOtherCameraStatusChgNotify(const std::string &strUserIDs, const std::string & strRoomId, int status);
//    //接收语音消息被屏蔽一路的通知
//    virtual void OnReciveAvoidControllMsg(const std::string&strUserID,int iStauts) override;
    void OnReceiveSessionUserIdPair(const SessionUserIdPairVector_t &idPairVector) override;
	
	void OnMemberChange( const std::string& strRoomID, std::list<MemberChangeInner>& listMemberChange, bool bUpdate ) override;

    void OnUserVideoInfoNotify( const struct SetUserVideoInfoNotify &info) override;

	// 抢麦通知事件
	void OnGrabMicNotify(int mode, int ntype, int getMicFlag, int autoOpenMicFlag, int hasMicFlag,
		int talkTime, const std::string& strRoomID, const std::string strUserID, const std::string& strContent) override;
	// 连麦通知事件
	void OnInviteMicNotify(int mode, int ntype, int errCode, int talkTime,
		const std::string& strRoomID, const std::string& fromUserID, const std::string& toUserID, const std::string& strContent)override;
	// 消息通用回复事件
	void OnCommonEvent(int msgID, int wparam, int lparam, int errCode, const std::string strRoomID, int nSessionID , std::string strParam = "" )override;

    // 接收到被屏蔽视频通知
    void OnMaskVideoByUserIdNfy(const std::string &selfUserId, const std::string &fromUserId, int mask) override;
    // 别人摄像头状态改变通知
//    void OnWhoseCamStatusChgNfy(const std::string &camChgUserId, int camStatus) override;
    // 接收到别人的视频输入状态改变通知
    void OnOtherVideoInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus, bool bLeave, int videoId) override;
    // 接收到别人的音频输入状态改变通知
    void OnOtherAudioInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus) override;
	// 接收到别人的共享视频输入状态改变通知
	void OnOtheShareInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus) override;
	//=========== 发送 ===========
    /**
     *  功能描述:控制其他人的麦克风开关
     *
     *  @param pUserID:用户ID，要保证全局唯一
     *  @param mute: true关闭对方麦克风，false打开对方麦克风
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    virtual YouMeErrorCode setOtherMicMute(const std::string&strUserID,bool mute);
    /**
     *  功能描述:切换语音输出设备
     *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
     *
     *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    virtual YouMeErrorCode setOutputToSpeaker (bool bOutputToSpeaker);
    /**
     *  功能描述:控制其他人的扬声器开关
     *
     *  @param pUserID:用户ID，要保证全局唯一
     *  @param mute:true关闭对方扬声器，false打开对方扬声器
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    virtual YouMeErrorCode setOtherSpeakerMute (const std::string& strUserID,bool mute);
    /**
     *  功能描述:选择消除其他人的语音
     *
     *  @param pUserID:用户ID，要保证全局唯一
     *  @param on:false屏蔽对方语音，true取消屏蔽
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    virtual YouMeErrorCode setListenOtherVoice (const std::string& strUserID,bool on);
    
    YouMeErrorCode inputVideoFrame(void* data, int len, int width, int	height, int fmt, int rotation, int mirror, uint64_t timestamp, int streamID = 0);
    
    
    YouMeErrorCode inputVideoFrameForShare(void* data, int len, int width, int   height, int fmt, int rotation, int mirror, uint64_t timestamp);
    YouMeErrorCode inputVideoFrameForMacShare(void* data, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);

    YouMeErrorCode inputVideoFrameForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    YouMeErrorCode inputVideoFrameForIOSShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    
#if ANDROID
    YouMeErrorCode inputVideoFrameForAndroid(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    YouMeErrorCode inputVideoFrameForAndroidShare(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
#endif
    
    YouMeErrorCode inputAudioFrameForMix(int streamId, void* data, int len, YMAudioFrameInfo info, uint64_t timestamp);
    
	//输入自定义数据
	void setRecvCustomDataCallback(IYouMeCustomDataCallback* pCallback);
	YouMeErrorCode inputCustomData(const void* data, int len, uint64_t timestamp, std::string userId);
    
    void setTranslateCallback( IYouMeTranslateCallback* pCallback );
    
    YouMeErrorCode setVideoFrameRawCbEnabled(bool enabled);

    // only for android
    YouMeErrorCode setVideoDecodeRawCbEnabled(bool enabled);

    YouMeErrorCode stopInputVideoFrame();
    
    YouMeErrorCode stopInputVideoFrameForShare();
    
    YouMeErrorCode queryUsersVideoInfo(std::vector<std::string>& userList);
    
    YouMeErrorCode setUsersVideoInfo(std::vector<IYouMeVoiceEngine::userVideoInfo>& videoInfoList);
	YouMeErrorCode setTCPMode(int iUseTCP);
    void onHeadSetPlugin (int state);

    virtual YouMeErrorCode mixAudioTrack(const void* pBuf, int nSizeInByte,
                                         int nChannelNUm, int nSampleRate,
                                         int nBytesPerSample, uint64_t nTimestamp,
                                         bool bFloat, bool bLittleEndian,
                                         bool bInterleaved, bool bForSpeaker);

    virtual YouMeErrorCode playBackgroundMusic(const std::string& strFilePath, bool bRepeat);
    virtual YouMeErrorCode pauseBackgroundMusic();
    virtual YouMeErrorCode resumeBackgroundMusic();
    virtual YouMeErrorCode stopBackgroundMusic();
    virtual YouMeErrorCode setBackgroundMusicVolume(int vol);
    virtual YouMeErrorCode setHeadsetMonitorOn(bool bMicEnabled, bool bBgmEnabled = true);
    virtual YouMeErrorCode setReverbEnabled(bool bEnabled);
    virtual YouMeErrorCode setVadCallbackEnabled(bool bEnabled);
    virtual YouMeErrorCode pauseChannel(bool needCallback = true);
    virtual YouMeErrorCode resumeChannel(bool needCallback = true);
    virtual void setRecordingTimeMs(unsigned int timeMs);
    virtual void setPlayingTimeMs(unsigned int timeMs);
    virtual YouMeErrorCode setPcmCallbackEnable(IYouMePcmCallback* pcmCallback, int flag, bool bOutputToSpeaker = true, int nOutputSampleRate = 16000, int nOutputChannel=1);
    virtual YouMeErrorCode setTranscriberEnabled(bool enable, IYouMeTranscriberCallback* transcriberCallback );
    virtual YouMeErrorCode setVideoPreDecodeCallbackEnable(IYouMeVideoPreDecodeCallback* preDecodeCallback, bool needDecodeandRender = true);
    virtual YouMeErrorCode setMicLevelCallback(int maxLevel);
    virtual YouMeErrorCode setFarendVoiceLevelCallback(int maxLevel);
    virtual YouMeErrorCode setReleaseMicWhenMute(bool bEnabled);
    virtual YouMeErrorCode setExitCommModeWhenHeadsetPlugin(bool bEnabled);
    
    // For debug
    virtual void getSdkInfo(string &strInfo);
    
    virtual void setExternalInputMode( bool bInputModeEnabled );
    virtual YouMeErrorCode openVideoEncoder(const std::string& strFilePath);
    virtual YouMeErrorCode setVideoCallback(IYouMeVideoFrameCallback * cb);
    //virtual IYouMeVideoCallback * getVideoCallback();
    virtual int createRender(const std::string & userId);
    virtual int deleteRender(int renderId);
    virtual int deleteRender(const std::string & userID);
    virtual YouMeErrorCode setVideoRenderCbEnable(bool bEnabled);
    virtual YouMeErrorCode startCapture();
    virtual YouMeErrorCode stopCapture();
    virtual YouMeErrorCode setVideoPreviewFps(int fps);
    virtual YouMeErrorCode setVideoFps(int fps);
    virtual YouMeErrorCode setVideoFpsForSecond(int fps);
    virtual YouMeErrorCode setVideoFpsForShare(int fps);
    virtual YouMeErrorCode setVideoLocalResolution(int width, int height);
    virtual YouMeErrorCode setCaptureFrontCameraEnable(bool enable);
    virtual YouMeErrorCode switchCamera();
    virtual YouMeErrorCode resetCamera();
    virtual YouMeErrorCode maskVideoByUserId(const std::string& userId, bool mask);
    virtual YouMeErrorCode setVideoRuntimeEventCb();
    virtual YouMeErrorCode setStaticsQosCb();

	virtual YouMeErrorCode requestRestApi( const std::string& strCommand, const std::string& strQueryBody, int* requestID  );
    
    YouMeErrorCode setVideoSmooth(int enable);
    YouMeErrorCode setVideoNetAdjustmode(int mode);
    void setLogLevel( YOUME_LOG_LEVEL consoleLevel, YOUME_LOG_LEVEL fileLevel);
    YouMeErrorCode setExternalInputSampleRate( YOUME_SAMPLE_RATE inputSampleRate, YOUME_SAMPLE_RATE mixedCallbackSampleRate );
    YouMeErrorCode setVideoNetResolution( int width, int height );
    YouMeErrorCode setVideoNetResolutionForSecond( int width, int height );
    YouMeErrorCode setVideoNetResolutionForShare( int width, int height );
    void setAVStatisticInterval( int interval  );
    void setAudioQuality (YOUME_AUDIO_QUALITY quality );
    void setScreenSharedEGLContext(void* gles);
    
    void setVideoCodeBitrate( unsigned int maxBitrate,  unsigned int minBitrate);
    void setVideoCodeBitrateForSecond( unsigned int maxBitrate,  unsigned int minBitrate);
    void setVideoCodeBitrateForShare( unsigned int maxBitrate,  unsigned int minBitrate);

    YouMeErrorCode setVBR(bool useVBR);
    YouMeErrorCode setVBRForSecond(bool useVBR);
    YouMeErrorCode setVBRForShare(bool useVBR);

    unsigned int getCurrentVideoCodeBitrate( );
    void setVideoHardwareCodeEnable( bool bEnable );
    bool getVideoHardwareCodeEnable( );
    bool getUseGL( );
    void setVideoNoFrameTimeout(int timeout);
    
    YouMeErrorCode startPush( std::string pushUrl );
    YouMeErrorCode stopPush();
    
    YouMeErrorCode setPushMix( std::string pushUrl , int width, int height );
    YouMeErrorCode clearPushMix();
    YouMeErrorCode addPushMixUser( std::string userId, int x, int y, int z, int width, int height );
    YouMeErrorCode removePushMixUser( std::string userId );
    

    
    bool sendCbMsgCallAVStatistic( YouMeAVStatisticType avType,  int32_t sessionId,  int value  );
    int getSelfSessionID();
    
    void removeAppKeyFromRoomId(const std::string& strRoomIdFull, std::string& strRoomIdShort);
	void NotifyCustomData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSendSessionId);
    ///////////声网兼容接口//////////////////////////////////////////////
    int enableLocalVideoRender(bool enabled);
    int enableLocalVideoSend(bool enabled);
    int muteAllRemoteVideoStreams(bool mute);
    int setDefaultMuteAllRemoteVideoStreams(bool mute);
    int muteRemoteVideoStream(std::string& uid, bool mute);
    
#if TARGET_OS_IPHONE || defined(ANDROID)
    bool isCameraZoomSupported();
    float setCameraZoomFactor(float zoomFactor);
    bool isCameraFocusPositionInPreviewSupported();
    bool setCameraFocusPositionInPreview( float x , float y );
    bool isCameraTorchSupported();
    bool setCameraTorchOn( bool isOn );
    bool isCameraAutoFocusFaceModeSupported();
    bool setCameraAutoFocusFaceModeEnabled( bool enable );
#endif
   
    void setLocalVideoMirrorMode( YouMeVideoMirrorMode mode );

#ifdef WIN32
    YouMeErrorCode startCaptureAndEncode(int mode, HWND renderHandle, HWND captureHandle);
    YouMeErrorCode startShareStream(int mode, HWND renderHandle, HWND captureHandle);
	YouMeErrorCode setShareExclusiveWnd(HWND windowid);
    YouMeErrorCode getWindowInfoList(HWND *pHwnd, char *pWinName, int *winCount);
    YouMeErrorCode getScreenInfoList (HWND *pHwnd, char *pScreenName, int *ScreenCount);
    YouMeErrorCode startSaveScreen( const std::string&  filePath, HWND renderHandle, HWND captureHandle);
#elif TARGET_OS_OSX
    YouMeErrorCode checkSharePermission();
    YouMeErrorCode startShareStream(int mode, int windowid);
    YouMeErrorCode setShareExclusiveWnd( int windowid );
    void setShareContext(void* context);
    YouMeErrorCode getWindowInfoList(char *windowOwner, char *windowName, int *windowId, int *winCount);
	YouMeErrorCode startCaptureAndEncode(int mode, int windowid);
#endif
    YouMeErrorCode startSaveScreen(const std::string& filePath);
    YouMeErrorCode checkCondition(const std::string&  filePath);
    YouMeErrorCode prepareSaveScreen(const std::string&  filePath);
    void stopCaptureAndEncode();
    YouMeErrorCode stopShareStream();
    YouMeErrorCode stopSaveScreen();
    
    
    YouMeErrorCode switchResolutionForLandscape(); //进行横屏切换
    YouMeErrorCode resetResolutionForPortrait();   //进行竖屏切换
    
    YouMeErrorCode translateText(unsigned int* requestID, const char* text, YouMeLanguageCode destLangCode, YouMeLanguageCode srcLangCode);
	YouMeErrorCode setVideoEncodeParamCallbackEnable(bool isReport); // 设置视频编码参数是否通知上层
private:
    static void videoRenderCb(int32_t sessionId, int32_t i4Width, int32_t i4Height, int32_t i4Rotation, int32_t i4BufSize, const void *vBuf, uint64_t timestamp, int videoid);
    static void videoRuntimeEventCb(int type, int32_t sessionId, bool needUploadPic, bool needReport);
    static void videoShareCb(void* handle, char *data, uint32_t len, uint64_t timestamp, uint32_t w, uint32_t h);
    static void onStaticsQosCb(int media_type, int32_t sessionId, int source_type, int audio_ppl);
    static void videoShareCbForMac(char *data, uint32_t len, int width, int height, uint64_t timestamp);
    static void onVideoEncodeAdjustCb(int width, int height, int bitrate, int fps);
    static void onVideoEventCb(int type, void *data, int error);
    static void onVideoDecodeParamCb(int32_t sessionId, int width, int height);
    
    static std::string getShareUserName( std::string userName );
    
    void TranslateThread();
    void ReportTranslateStatus(short status, const XString& srcLanguage, const XString& destLanguage, XUINT64 chaterCount, int translateMethod);
	static const std::string addNewSession(int32_t sessionId, int32_t i4Width, int32_t i4Height, int videoid);
private:
    CYouMeVoiceEngine ();
    ~CYouMeVoiceEngine ();


private:
    static CYouMeVoiceEngine *mPInstance;
    static mutex mInstanceMutex;
private:
    // 1. 当需要改变状态并开始新状态的处理流程时，如STATE_RECONNECTING_FIRST, 需要先锁住再改变状态。状态改变成功后可以解锁。
    // 2. 当某些操作需要在特定状态下进行时，需要先锁住状态再判断状态，再进行操作。整个过程都要锁住状态。
    // 3. 把状态改为表明某些操作已经完成的，如STATE_CONNECTED, 可以不锁。当为了简明，凡是改变状态的动作，最好都加锁。
    // 4. mState用于管理API层的状态，mIsReconnecting用于管理内部的重连状态，mIsReconnecting 的设置是依赖于 mState的
    //    (比如：mState 变为 STATE_TERMINATED 时，mIsReconnecting 必须变为 false。所以，它们共用一个
    //    锁进行保护).
    ConferenceState mState = STATE_UNINITIALIZED;
	YouMeUserRole_t mRole = YOUME_USER_NONE;
	YouMeUserRole_t mTmpRole = YOUME_USER_NONE;
    bool mIsReconnecting = false;
    NETWORK_TYPE preNetworkType = NETWORK_TYPE_WIFI;
    ROOM_MODE_e m_roomMode = ROOM_MODE_NONE;
    bool mAllowPlayBGM = false;
    bool mAllowMonitor = false;
    // The "AboutToUninit" is a special state. This state is for such a period: when unInit is called,
    // and before the state is set to STATE_UNINITIALIZED. When it's in this state, all the current
    // operations should be cancelled, and no further API call is allowed. This variable makes the state
    // STATE_UNINITIALIZING obsolete.
    bool mIsAboutToUninit = false;
    bool mIsWaitingForLeaveAll = false;
    bool mPcmOutputToSpeaker = true;
    bool mTranscriberEnabled = false;
    std::recursive_mutex mStateMutex;
    // IYouMeCommonCallback *mPCommonCallback = NULL;
    // IYouMeConferenceCallback *mPConferenceCallback = NULL;
    IYouMeEventCallback *mPEventCallback = NULL;
    IYouMePcmCallback *mPPcmCallback = NULL;
    IYouMeTranscriberCallback* mPTranscriberCallback = NULL;
    int mPcmCallbackFlag = 0;
    int m_nPCMCallbackSampleRate = 16000;
    int m_nPCMCallbackChannel = 1;
    IYouMeVideoPreDecodeCallback *mPVideoPreDecodeCallback = NULL;
    bool mPreVideoNeedDecodeandRender = true;
	IRestApiCallback* mYouMeApiCallback = NULL;
 	IYouMeMemberChangeCallback* mMemChangeCallback = NULL;
	IYouMeChannelMsgCallback* mNotifyCallback = NULL;
    IYouMeAVStatisticCallback* mAVStatistcCallback = NULL;
	IYouMeCustomDataCallback* m_pCustomDataCallback = nullptr;
    IYouMeTranslateCallback* m_pTranslateCallback = nullptr;
    IYouMeVideoFrameCallback *mPVideoCallback = nullptr;
    NgnEngine *mPNgnEngine = NULL;
  

    INgnNetworkService *mPNetworkService = NULL;

    // Memorizing the parameters for joining the room.
    std::string mRoomID;
    std::string mStrUserID;
    std::string mAppKey;
    std::string mAppSecret;
    std::string mAppKeySuffix;
    std::string mJoinAppKey;    //加入房间的appKey

    bool mVideoAutoRecv = true;
    bool mNeedMic = true;
    bool mNeedInputAudioFrame = true;
	bool mInviteMicOK = false;//special: invite microphone 
    bool mAutoSendStatus = false;
    bool mInCommMode = false;
    
    int mSessionID = -1;
    string mMcuAddr;
    int mMcuRtpPort = -1;
    int mMcuSignalPort = -1;
    uint64_t mBusinessID = 0;
    bool bMcuChanged = false;
    RedirectServerInfoVec_t mRedirectServerInfoVec;

    ServerRegionNameMap_t  mLastServerRegionNameMap;
    ServerRegionNameMap_t  mServerRegionNameMap;
    bool mCanRefreshRedirect = false;
    //记录临时释放麦克风之前的麦克风状态
    bool preReleaseMicStatus = false;
    
    // For logging into the MCU server, and communicate with the MCU server.
    // All the time consuming functions should be called in the main loop thread.
    NgnLoginService m_loginService;
    
    // Main object for controlling the audio/video data.
    // The mutex is to protect m_avSessionMgr from being accessed while it's
    // being deleting or initializing. Since all deleting/initialization will be performed
    // in the main loop thread, no mutex protection is needed while accessing m_avSessionMgr
    // in the main loop thread except deteting/initializing it. In all the other threads,
    // if you need to access m_avSessionMgr, you need the mutex to protect it.
    CAVSessionMgr* m_avSessionMgr = NULL;
    std::atomic<bool> m_avSessionExit;
    recursive_mutex m_avSessionMgrMutex;
    
    // Map between the sessionID and the userID. This member should be accessed in the main
    // loop thread only, so no lock is needed.
    typedef std::map<int32_t, std::string> SessionUserIdMap_t;
    std::mutex m_mutexSessionMap;
    SessionUserIdMap_t m_SessionUserIdMap;
    
    // 被订阅用户列表
    typedef std::map<std::string, int32_t> UserVideoInfoMap_t;
    std::mutex m_mutexVideoInfoMap;
    UserVideoInfoMap_t m_UserVideoInfoMap;

    // 订阅用户列表
    std::mutex m_mutexVideoInfoCache;
    std::vector<IYouMeVoiceEngine::userVideoInfo> m_UserVideoInfoList;

    // Map between the userID and the start timing of video render
    typedef std::map<std::string, uint64_t> UserIdStartRenderMap_t;
    UserIdStartRenderMap_t m_UserIdStartRenderMap;
    int m_NumOfCamera = 0;
    uint64_t m_LastCheckTimeMs = 0;

    // This can only be accessed in the main loop thread, so no protection is needed.
    CRoomManager *m_pRoomMgr = NULL;

	CRoomManager* m_RoomPropMgr = NULL;
    // This function should be call by the API functions. It's equipped with full parameter list.
    YouMeErrorCode joinChannelProxy (const std::string& strUserID, const string &strChannelID,
                                        YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv);

private:
    //启动一个线程用来重连
    //static TSK_STDCALL void *autoReconnect (void *arg);
    //tsk_thread_handle_t *mReConnectThreadHandle = tsk_null;
    //CXCondWait m_reconnectCondwait;
    
    //
    // Internal implementations. These functions should only be called in the main message loop.
    //
    void setState(ConferenceState state);
    bool isStateInitialized();
    static const char* stateToString(ConferenceState state);
    //static const char* commonEventToString(YouMeEvent event);
    static const char* eventToString(YouMeEvent event);
    

    void doInit();
    void doSetServerRegion(YOUME_RTC_SERVER_REGION serverRegionId, bool bAppend);
    bool leaveConfForUninit();
    void doJoinConferenceSingle(const std::string& strUserID, const string &strRoomID,
                                YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv);
    void doJoinConferenceMulti(const std::string& strUserID, const string &strRoomID, YouMeUserRole_t eUserRole);
    void doJoinConferenceFirst(const std::string& strUserID, const string &strRoomID,
                               YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv);
    void doJoinConferenceMore(const std::string& strUserID, const string &strRoomID, YouMeUserRole_t eUserRole);
    void doJoinConferenceMoreDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result);
    
//    void doJoinConferenceMulti(const std::string & strUserID, const string & strRoomID, YouMeUserRole_t eUserRole); /* 加入会议（多房间模式）*/
//	void joinRoomFirst(const std::string & strUserID, const string & strRoomID, YouMeUserRole_t eUserRole); /* 加入第一个房间 */
//	void joinRoomNext(const int sessionID, const string & strRoomID, YouMeUserRole_t eUserRole); /* 加入后面的房间 */
	void doSpeakToConference(const std::string & strRoomID);
    void doSpeakToConferenceDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result);

    void doLeaveConferenceMulti(const string &strRoomID);
    void doLeaveConferenceAll(bool needCallback);
    void doLeaveConferenceAllProxy( bool bKick = false );
    void doLeaveConferenceMultiDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result);
    
    void doBeKickFromChannel( const string& strRoomID , const string& param);
    
    void doSetAutoSend( bool bAutoSend );
    
    void doSetRecordDevice(  std::string& deviceUid  );
    void doOnRoomEvent(const std::string& strRoomID, NgnLoginServiceCallback::RoomEventType eventType, NgnLoginServiceCallback::RoomEventResult result);

	YouMeErrorCode startAvSessionManager(bool needMic, bool outputToSpeaker,bool bJoinRoom);
    YouMeErrorCode restoreEngine();
    void stopAvSessionManager(bool isReconnect=false);
    void resetConferenceProperties();
    void ReportLeaveConferenceFun (std::string& strRoomID, uint64_t joinRoomTime);

    void checkRecoringError();
    void sendEventToServer(YouMeProtocol::STATUS_EVENT_TYPE eventType,bool isOn,const std::string& strUserID);
    void sendSessionUserIdMapRequest(YouMeProtocol::YouMeVoice_Command_Session2UserIdRequest &session2UserIdReq);
    void applyMicMute(bool mute, bool notify=true);
    void applySpeakerMute(bool mute,bool notify=true);
    void applyVolume(uint32_t volume);
    void triggerCheckRecordingError();
    YouMeErrorCode loginToMcu(std::string &strRoomID, YouMeUserRole_t eUserRole, bool bRelogin, bool bVideoAutoRecv);
    void doReconnect();
    void doPauseConference(bool needCallback);
    void doResumeConference(bool needCallback);
    void doOnReceiveSessionUserIdPair(SessionUserIdPairVector_t &idPairVector);
    std::string getUserNameBySessionId( int32_t sessionid );
    int getSessionIdByUserName( const std::string& userName );
    
    void doQueryHttpInfo(  int requestID, const std::string& strCommand, const std::string& strQueryBody );
    void doGetChannelUserList( const std::string& strRoomID, int maxCount, bool bNotifyMemChange );
    
    void QueryHttpInfoThreadProc();
    std::string getHttpHost();
    std::string getQueryHttpUrl( std::string& strCommand );
    
    void doMaskVideoByUserId(std::string &userId, int mask);
    void doCamStatusChgReport(int cameraStatus);
    void doVideoInputStatusChgReport(int inputStatus, YouMeErrorCode errorCode);
    void doAudioInputStatusChgReport(int inputStatus);
    void doShareInputStatusChgReport(int inputStatus);
    YouMeErrorCode doQueryUsersVideoInfo(std::vector<std::string>* userList);
    YouMeErrorCode doSetUsersVideoInfo(std::vector<IYouMeVoiceEngine::userVideoInfo>* videoInfoList);
    //
    // Helpers for send message to the callback message queue.
    //
    bool sendCbMsgCallCommonStatus(const YouMeEvent eventType, const string &strUserID, int status);
    bool sendCbMsgCallOtherVideoDataOn(const std::string & strUserIDs, const std::string & strRoomId, uint16_t width, uint16_t height);
    bool sendCbMsgCallOtherVideoDataOff(const std::string & strUserIDs, const std::string & strRoomId);
    bool sendCbMsgCallOtherCameraStatus(const std::string & strUserIDs, const std::string & strRoomId, int status);
    uint32_t getReconnectWaitMsec(int retry); /** 获取重连间隔时间 毫秒*/

	bool sendCbMsgCallBroadcastEvent(YouMeBroadcast bc,
		const std::string& roomID, const std::string& param1, const std::string& param2, const std::string& content);
	void doStartGrabMicAction(const std::string & strChannelID, int maxAllowCount, int maxMicTime, int mode, unsigned int voteTime, const string& strContent);
	void doStopGrabMicAction(const std::string & strChannelID, const string& strContent);
	void doRequestGrabMic(const std::string & strChannelID, int score, bool isAutoOpenMic, const string& strContent);
	void doFreeGrabMic(const std::string & strChannelID);
	void doRequestInviteMic(const std::string & strChannelID, const std::string & strUserID, int waitTimeout, int maxMicTime, bool isNeedBroadcast, const string& strContent);
	void doResponseInviteMic(const std::string & strChannelID, const std::string & strUserID, bool isAccept, const string& strContent);
	void doStopInviteMic(const std::string & strChannelID);
	void doInitInviteMic(const std::string & strChannelID, int waitTimeout, int maxMicTime);
	void doCallEvent(const YouMeEvent eventType, const YouMeErrorCode errCode = YOUME_SUCCESS, const std::string& strRoomID = "", const std::string& strParam = "");
    
    void doSendMessage( int requestID, const std::string & strChannelID, const std::string & strContent, const std::string & strUserID);
    
    void doKickOther( const std::string& strChannelID, const std::string& strUserID,  int lastTime );
    void doSetOutputToSpeaker(bool bOutputToSpeaker);
    void doStartPush( std::string roomid, std::string userid,std::string pushUrl  );
    void doStopPush( std::string roomid, std::string userid );
    
    void doSetPushMix(  std::string roomid, std::string userid, std::string pushUrl, int width, int height  );
    void doClearPushMix(  std::string roomid );
    
    void doAddPushMixUser( std::string roomid, std::string userid, int x, int y, int z, int widht, int height );
    void doRemovePushMixUser( std::string roomid, std::string userid );
    
    void doCheckVideoSendStatus();

    //获取一个唯一的标识
    static int GetUniqueSerial()
    {
        int iTmp = 0;
        {
            std::lock_guard<std::mutex> lock(m_serialMutex);
            iTmp = ++s_iSerial;
        }
        return iTmp;
    }

    bool checkFrameTstoDrop(uint64_t timestamp);
    bool checkFrameTstoDropForShare(uint64_t timestamp);
    bool checkInputVideoFrameLen(int width, int height, int fmt, int len);
    
public:
    bool sendCbMsgCallEvent(const YouMeEvent eventType, const YouMeErrorCode errCode,
                            const std::string& strRoomID, const std::string& strParam = "");

	//会议转写的回调
	void notifySentenceBegin(int sessionId, int setenceIndex, int sentenceTime);
	void notifySentenceEnd(int sessionId, int setenceIndex, std::string result, int sentenceTime);
	void notifySentenceChanged(int sessionId, int sentenceIndex, std::string result, int sentenceTime);


private:
    uint64_t m_ulInitStartTime =0;//单位毫秒
    bool     m_bSetLogLevel = false;
    
    // Channel properties. These will be reset everytime joining a new channel.
    // All the other properties should be set before m_bSpeakerMute and m_bMicMute
    bool     m_bMicMute = false; // 默认进房间打开麦克风
    bool     m_bSpeakerMute = true;
    bool     m_bMicBypassToSpeaker = false;
    bool     m_bBgmBypassToSpeaker = false;
    bool     m_bReverbEnabled = false;
	// Whether auto SetMicMute or not
	bool	 m_bAutoOpenMic = false;
	bool	 m_hasGrabMic = false;
	bool	 m_hasInviteMic = false;

    int64_t  m_nPlayingTimeMs = -1;
    int64_t  m_nRecordingTimeMs = -1;
    
    // These should be kept over the whole life time
    bool     m_bVadCallbackEnabled = false;
    uint32_t m_nVolume = 100;
    uint32_t m_nMixToMicVol = 100;
    uint32_t m_nMixToSpeakerVol = 100;
    int      m_nMaxMicLevelCallback = 0; // The max mic level detection for callback. 0 indicates no mic level callback is needed.
    int      m_nMaxFarendVoiceLevel = 0; // The max farend voice level detection for callback. 0 indicates no farend voice level callback is needed.

    bool     m_bReleaseMicWhenMute = false;
    bool     m_bExitCommModeWhenHeadsetPlugin = false;
    bool     m_bOutputToSpeaker = true;
    bool     m_bCurrentSilentMic = false;

    // Set by the system monitor, not by the user
#ifdef WIN32
    bool     m_bHeadsetPlugin = true;
#else
    bool     m_bHeadsetPlugin = false;
#endif
    bool     m_bCameraIsOpen = false;
    bool     m_bInputVideoIsOpen = false;
    bool     m_bInputAudioIsOpen = false;
    bool     m_bInputShareIsOpen = false;

    // For background music
    std::string mLastMusicPath;
    bool mbMusicRepeat = false;
    
    void PlayBackgroundMusicThreadFunc(const string strFilePath, bool bRepeat);
    void doPlayBackgroundMusic(const string &strFilePath, bool bRepeat);
    void doPauseBackgroundMusic(bool bPaused);
    void doStopBackgroundMusic();
    std::thread m_playBgmThread;
    bool m_bBgmStarted = false;
    std::mutex m_bgmPauseMutex;
    std::condition_variable m_bgmPauseCond;
    bool m_bBgmPaused = false;

    // for ios video process
    uint32_t m_iosRetrans = 0;      // 是否打开缓存重发功能，该标志仅对ios屏幕共享有效
    std::thread m_iosVideoThread;
    std::mutex m_videoQueueMutex;
    std::condition_variable m_videoQueueCond;
    bool m_videoQueueStarted = false;
    std::deque<FrameImage*> m_videoQueue;
    uint64_t m_lastVideoTimestamp = 0;
    uint32_t m_refreshTimeout = 0;
    #if TARGET_OS_IPHONE
    void processIosVideothreadFunc();
    void startIosVideoThread();
    void stopIosVideoThread();
    #endif
    
    // For packet statistics report
    void doPacketStatReport();
    void packetStatReportThreadFunc(uint32_t reportPeroidMs);
    void startPacketStatReportThread();
    void stopPacketStatReportThread();
    std::thread m_packetStatReportThread;
    bool m_bPacketStatReportEnabled = false;
    youmecommon::CXCondWait m_packetStatReportThreadCondWait;

    // For Audio/Video qos statistics report
    void doAVQosStatReport();
    void avQosStatReportThreadFunc(uint32_t reportPeroidMs);
    void startAVQosStatReportThread();
    void stopAVQosStatReportThread();
    std::thread m_avQosStatReportThread;
    bool m_bAVQosStatReportEnabled = false;
    youmecommon::CXCondWait m_avQosStatReportThreadCondWait;
    uint64_t m_lastAVQosStatReportTime = 0;

    static void micLevelCallback(int32_t micLevel);
    static void farendVoiceLevelCallback(int32_t micLevel, uint32_t sessionId);
    static void onSessionStartFail(tmedia_type_t type );
    
    void initLangCodeMap();

 	bool m_bExitHttpQuery;
    youmecommon::CXSemaphore m_httpSemaphore;
    std::thread m_queryHttpThread;
    std::mutex m_queryHttpInfoLock;
    std::list<QueryHttpInfo>  m_queryHttpList;    
	
    static void vadCallback(int32_t sessionId, tsk_bool_t isSilence);
    void notifyVadStatus(int32_t sessionId, tsk_bool_t isSilence);
    void doNotifyVadStatus(int32_t sessionId, bool bSilence);

    void doSetPcmCallback(IYouMePcmCallback* callback, int flag );
    static void pcmCallback(tdav_session_audio_frame_buffer_t* frameBuffer, int flag);
    void notifyPcmCallback(tdav_session_audio_frame_buffer_t* frameBuffer, int flag);
    void doSetTranscriberCallback( IYouMeTranscriberCallback* callback, bool enable );
    
    static void videoPreDecodeCallback(void* data, tsk_size_t size, int32_t sessionId, int64_t timestamp);
    static void audioMixCallback(void* data, size_t length, YMAudioFrameInfo info);

    void doOpenVideoEncoder(const string &strFilePath);
    
    // Message handlers and queues
    CMessageLoop *m_pMainMsgLoop = NULL;
    static void MainMessgeHandler(void* pContext, CMessageBlock* pMsg);
    CMessageLoop *m_pCbMsgLoop = NULL;
    static void CbMessgeHandler(void* pContext, CMessageBlock* pMsg);
    CMessageLoop *m_pWorkerMsgLoop = NULL;
    static void WorkerMessgeHandler(void* pContext, CMessageBlock* pMsg);
    CMessageLoop *m_pPcmCallbackLoop = NULL;
    recursive_mutex m_pcmCallbackLoopMutex;
    static void PcmCallbackHandler(void* pContext, CMessageBlock* pMsg);
    
    //为上报计算远端的总分辨率
    int calcSumResolution( const std::string& exceptUser );
    
    youmecommon::CXCondWait m_workerThreadCondWait;
    youmecommon::CXCondWait m_reconnctWait;
    uint64_t video_duration = 0;//单位毫秒
	
	static int s_iSerial;	//序列号
    static std::mutex m_serialMutex;
    
    uint64_t m_externalInputStartTime= 0;
    uint32_t m_externalVideoDuration = 0;
    
    std::map<std::string , YouMeEvent> m_mapShutdownReason;
    
    struct InnerSize{
        int width;
        int height;
        InnerSize():width(0), height(0){
        }
    };
    static std::mutex m_mutexOtherResolution;      //为了数据上报~~~，锁下面的两个数据
    static std::map<int, InnerSize> m_mapOtherResolution;
    std::map<std::string, uint64_t> m_mapCurOtherUserID;

    std::string m_strMicDeviceChoosed;  //选择的麦克风设备uid
    
    int m_mixedCallbackSampleRate;

    int m_previewFps = 0;
    
    // for share video stream
    int shareFps = 0;
    int shareWidth = 0;
    int shareHeight = 0;
    YOUME_VIDEO_SHARE_TYPE_t shareMode = VIDEO_SHARE_TYPE_UNKNOW;  

    // 用于窗口录制时检查是否调整编码分辨率，如果当前窗口录制采集分辨率和上一帧采集分辨率相同则不调整编码分辨率
    int lastShareCaptrureWidth = 0;
    int lastShareCaptureHeight = 0;

    int videoWidth = 0;
    int videoHeight = 0;
    int lastRotation = 0;
    
    // for opus packet loss perc
    int m_nOpusPacketLossPerc = 10;
    int m_nOpusPacketBitrate = 25000;
    
	bool m_bStartshare = false;
    bool m_bSaveScreen = false;

    #ifdef MAC_OS
    void * capture = nullptr;
    std::vector<int> m_vecShareExclusiveWnd; //共享屏幕的排除窗口，可能早于共享设置，得记下来,可能排除多个
    #endif
	bool bInitCaptureAndEncode = false;
    
    std::map<YouMeLanguageCode, XString> m_langCodeMap;
    std::thread m_translateThread;
    std::mutex m_translateLock;
    static unsigned int m_iTranslateRequestID;
    std::list<TranslateInfo> m_translateInfoList;
    youmecommon::CXSemaphore m_translateSemaphore;
    bool m_bTranslateThreadExit;
	bool m_bVideoEncodeParamReport = false;
    YMAudioMixer* m_audioMixer = NULL;

private:
    uint64_t m_nextVideoTs = 0;
    uint64_t m_nextVideoTsForShare = 0;
	int32_t m_frameCount = 0;
    int32_t m_frameCountForShare = 0;

public:
    int m_inputSampleRate;
    bool m_isInRoom;        //m_roomMgr不能在非主线程访问，所以加这个吧

};

#endif /* defined(__cocos2d_x_sdk__YouMeVoiceEngine__) */

//
//  IYouMeCInterface.hpp
//  youme_voice_engine
//
//  Created by YouMe.im on 15/12/10.
//  Copyright © 2015年 youme. All rights reserved.
//

#ifndef IYouMeCInterface_hpp
#define IYouMeCInterface_hpp


//这个文件对内封装的C 接口，不需要给外部。C# 只能调用C接口的函数
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "YouMeConstDefine.h"

#ifdef WIN32

#ifdef _DLLExport
#define YOUME_API _declspec(dllexport)
#else
#define YOUME_API _declspec(dllimport)
#endif

#else
#define YOUME_API __attribute ((visibility("default")))
#endif // WIN32

typedef enum _youme_video_fmt{
    YOUME_VIDEO_FMT_I420 = 0,
    YOUME_VIDEO_FMT_BGRA = 1,
    YOUME_VIDEO_FMT_RGB24 = 2
}C_YOUME_VIDEO_FMT;
#pragma pack (1)

typedef struct _I420Frame
{
    int  renderId;
    int  width;
    int  height;
    int  degree;
    int  len;
    char * data;
    char * datacopy;
    bool bIsNew;
    int fmt;
} I420Frame;
#pragma pack ()

#ifdef __cplusplus
extern "C" {
#endif

	YOUME_API typedef void(*VideoFrameCallbackForFFI)(const char *,const char *, int , int , int , int );
    
    YOUME_API typedef void(*VideoEventCallbackForFFI)(const char *);

    // event callback
    YOUME_API typedef void(*EventCb)(const enum YouMeEvent event, const enum YouMeErrorCode error, const char * channel, const char * param);

    // onRequestRestAPI callback
    YOUME_API typedef void(*RequestRestAPICb)(int requestID, const enum YouMeErrorCode iErrorCode, const char* strQuery,  const char* strResult);

    // member change callback
    YOUME_API typedef void(*MemberChangeCb)(const char * channel, const char* userID, bool isJoin, bool bUpdate);

    // broadcast callback
    YOUME_API typedef void(*BroadcastCb)(const int bc, const char*  channel, const char*  param1, const char*  param2,const char*  strContent);

    // AVStatistic callback
    YOUME_API typedef void(*AVStatisticCb)( int type,  const char* userID,  int value );
    
    //Transcriber callback
    YOUME_API typedef void(*SentenceBeginCb)( const char* userid , int sentenceIndex);
	YOUME_API typedef void(*SentenceEndCb)(const char* userid, int sentenceIndex, const char*  result, const char*  transLang, const char*  transResult);
	YOUME_API typedef void(*SentenceChangedCb)(const char* userid, int sentenceIndex, const char*  result, const char*  transLang, const char*  transResult);
    YOUME_API typedef void(*TranslateTextCompleteCb)(enum YouMeErrorCode errorcode, unsigned int requestID, const char* text, enum YouMeLanguageCode srcLangCode, enum YouMeLanguageCode destLangCode);
    
	YOUME_API typedef void(*OnPcmDataCallback)(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte,int type) ;
    // 获取回调消息
    YOUME_API const XCHAR* youme_getCbMessage();
    // 释放回调消息
	YOUME_API void youme_freeCbMessage(const XCHAR* pMsg);
    
	//返回UTF8 的cbMessage
	YOUME_API const char* youme_getCbMessage2();
	// 释放回调消息
	YOUME_API void youme_freeCbMessage2(const char* pMsg);

    /**
     * 功能描述: 外部回调注册接口
     * SentencexxCb，如果不开启会议纪要，可以为nullptr
     */
    YOUME_API void youme_setCallback( EventCb YMEventCallback,
                                    RequestRestAPICb YMRestApiCallback,
                                    MemberChangeCb YMMemberChangeCallback,
                                    BroadcastCb YMBroadcastCallback,
                                    AVStatisticCb YMAVStatisticCallback,
                                    SentenceBeginCb YMSentenceBeginCallback,
                                    SentenceEndCb YMSentenceEndCallback,
                                    SentenceChangedCb YMSentenceChangedCallback,
                                    TranslateTextCompleteCb YMTranslateTextCompleteCallback );

	YOUME_API void youme_setPCMCallback(OnPcmDataCallback pcmCallback, int flag, bool bOutputToSpeaker, int nOutputSampleRate, int nOutputChannel);
	YOUME_API void youme_setSentenceEnd(const char* userid, int sentenceIndex, const char* result, const char* transLang, const char* transResult);
	YOUME_API void youme_setSentenceChanged(const char* userid, int sentenceIndex, const char* result, const char* transLang, const char* transResult);
	YOUME_API void youme_setEvent(int event, int error, const char * channel, const char * param);
	YOUME_API void youme_Log(const char* msg);

    /**
     * 功能描述:   设置是否由外部输入音视频
     * @param bInputModeEnabled: true:外部输入模式，false:SDK内部采集模式
     */
    YOUME_API void youme_setExternalInputMode( bool bInputModeEnabled );
    
    /**
     * 功能描述:初始化引擎
     * 这是一个异步调用接口，如果函数返回 YOUME_SUCCESS， 则需要等待以下事件回调达到才表明初始化完成
     * YOUME_EVENT_INIT_OK - 表明初始化成功
     * YOUME_EVENT_INIT_FAILED - 表明初始化失败，最常见的失败原因是网络错误或者 AppKey/AppSecret 错误
     *
     * @param strAPPKey - 从游密申请到的 app key, 这个你们应用程序的唯一标识
     * @param strAPPSecret - 对应 strAPPKey 的私钥, 这个需要妥善保存，不要暴露给其他人
     * @param serverRegionId -
     *        设置首选连接服务器的区域码
     *        如果在初始化时不能确定区域，可以填RTC_DEFAULT_SERVER，后面确定时通过 SetServerRegion 设置。
     *        如果YOUME_RTC_SERVER_REGION定义的区域码不能满足要求，可以把这个参数设为 RTC_EXT_SERVER，然后
     *        通过后面的参数 strExtServerRegionName 设置一个自定的区域值（如中国用 "cn" 或者 “ch"表示），然后把这个自定义的区域值同步给游密。
     *        我们将通过后台配置映射到最佳区域的服务器。
     * @pExtServerRegionName - 扩展的服务器区域简称
     *
     * @return - YOUME_SUCCESS
     *           其他返回值表明发生了错误，详见YouMeConstDefine.h定义
     */
	YOUME_API int youme_init(const char* strAPPKey, const char* strAPPSecret,
                             enum YOUME_RTC_SERVER_REGION serverRegionId, const char* pExtServerRegionName);
    
    /**
     * 功能描述:反初始化引擎，在应用退出之前需要调用这个接口释放资源。
     *        这是一个同步调用接口，函数返回时表明操作已经完成。
     *
     * @return - YOUME_SUCCESS
     *           其他返回值表明发生了错误，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_unInit();

    //是否是测试模式,测试模式使用测试服
    YOUME_API void youme_setTestConfig(int bTest);
    YOUME_API void youme_setServerIpPort(const char* ip, int port);
    //设置服务器区域
    YOUME_API void youme_setServerRegion(int regionId, const char* extRegionName, bool bAppend);
    
	YOUME_API int youme_setTCPMode(int iUseTCP);



    /**
     *  功能描述:切换语音输出设备
     *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
     *
     *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_setOutputToSpeaker (bool bOutputToSpeaker);
    
    /**
     *  功能描述:扬声器静音打开/关闭
     *
     *  @param bOn:true——打开，false——关闭
     *  @return 无
     */
    YOUME_API void youme_setSpeakerMute(bool bOn);
    
    /**
     *  功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
     *
     *  @param bAutoSend:true——通知，false——不通知
     *  @return 无
     */
    YOUME_API void youme_setAutoSendStatus( bool bAutoSend );
    
    /**
     *  功能描述:获取扬声器静音状态
     *
     *  @return true——打开，false——关闭
     */
    YOUME_API bool youme_getSpeakerMute();
    
    /**
     *  功能描述:获取麦克风静音状态
     *
     *  @return true——打开，false——关闭
     */
    YOUME_API bool youme_getMicrophoneMute();
    
    /**
     *  功能描述:麦克风静音打开/关闭
     *
     *  @param bOn:true——打开，false——关闭
     *  @return 无
     */
    YOUME_API void youme_setMicrophoneMute(bool mute);
    
    /**
     *  功能描述:获取音量大小,此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量
     *
     *  @return 音量值[0,100]
     */
    YOUME_API unsigned int youme_getVolume();
    
    /**
     *  功能描述:设置音量，取值范围是[0-100] 100表示最大音量， 默认音量是100
     *
     *  @return 无
     */
    YOUME_API void youme_setVolume(unsigned int uiVolume);
    
    /**
     *  功能描述:是否可使用移动网络
     *
     *  @return true-可以使用，false-禁用
     */
    YOUME_API bool youme_getUseMobileNetworkEnabled();
    
    /**
     *  功能描述:启用/禁用移动网络
     *
     *  @param bEnabled:true-可以启用，false-禁用，默认禁用
     *
     *  @return 无
     */
    YOUME_API void youme_setUseMobileNetworkEnabled(bool bEnabled);

     /**
     *  功能描述:设置本地连接信息，用于p2p传输，本接口在join房间之前调用
     *
     *  @param pLocalIP:本端ip
     *  @param iLocalPort:本端数据端口
     *  @param pRemoteIP:远端ip
     *  @param iRemotePort:远端数据端口
     *  @param iNetType: 0:ipv4, 1:ipv6, 目前仅支持ipv4
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_setLocalConnectionInfo(const char* pLocalIP, int iLocalPort, const char* pRemoteIP, int iRemotePort, int iNetType);

    /**
     *  功能描述:清除本地局域网连接信息，强制server转发
     *
     *  @return 无
     */
    YOUME_API void youme_clearLocalConnectionInfo();

    /**
     *  功能描述:设置是否切换server通路
     *
     *  @param enable: 设置是否切换server通路标志
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_setRouteChangeFlag(bool enable);   
    
    /**
     *  功能描述:设置身份验证的token和用户加入房间的时间
     *  @param pToken: 身份验证用token，设置为NULL或者空字符串，清空token值。
     *  @param uTimeStamp: 用户加入房间的时间, v3 token使用，默认不启用设置为0即可
     *  @return 无
     */
    YOUME_API void youme_setToken( const char* pToken, uint32_t uTimeStamp );

    /**
     *  功能描述: 设置用户自定义Log路径
     *  @param pFilePath Log文件的路径
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setUserLogPath(const char* pFilePath);
    
    //---------------------多人语音接口---------------------//
    
    /**
     *  功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
     *
     *  @param pUserID: 用户ID，要保证全局唯一
     *  @param pChannelID: 频道ID，要保证全局唯一
     *  @param userRole: 用户角色，用于决定讲话/播放背景音乐等权限
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_joinChannelSingleMode(const char* pUserID, const char* pChannelID, int userRole, bool autoRecvVideo);

    /**
     *  功能描述：加入语音频道（多频道模式，可以同时听多个语音频道的内容，但每个时刻只能对着一个频道讲话）
     *
     *  @param pUserID: 用户ID，要保证全局唯一
     *  @param pChannelID: 频道ID，要保证全局唯一
     *  @param eUserRole: 用户角色，用于决定讲话/播放背景音乐等权限
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
	YOUME_API int youme_joinChannelMultiMode(const char * pUserID, const char * pChannelID, int userRole);

	/**
	*  功能描述：对指定频道说话
	*
	*  @param strChannelID: 频道ID，要保证全局唯一
	*
	*  @return 错误码，详见YouMeConstDefine.h定义
	*/
	YOUME_API int youme_speakToChannel(const char * pChannelID);
    
    /**
     *  功能描述:退出多频道模式下的某一个频道
     *
     *  @param strChannelID:频道ID，要保证全局唯一
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_leaveChannelMultiMode(const char* pChannelID);

    /**
     *  功能描述:退出所有语音频道
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    YOUME_API int youme_leaveChannelAll();

    
    YOUME_API void youme_setSoundtouchEnabled(bool bEnable);
    
    
    /**
     *  功能描述:获取是否开启变声效果
     *
     *  @return 是否开启变声
     */
    YOUME_API bool youme_getSoundtouchEnabled();
    
    //变速,1为正常值
    YOUME_API float youme_getSoundtouchTempo();
    YOUME_API void youme_setSoundtouchTempo(float nTempo);
    
    //节拍,1为正常值
    YOUME_API float youme_getSoundtouchRate();
    YOUME_API void youme_setSoundtouchRate(float nRate);
    
    //变调,1为正常值
    YOUME_API float youme_getSoundtouchPitch();
    YOUME_API void youme_setSoundtouchPitch(float nPitch);
    
    //获取版本号
    YOUME_API int youme_getSDKVersion();
////    设置其他人的麦克风、扬声器、语音消除状态
//    int youme_setOtherStatus(STATUS_EVENT_TYPE_t eventType,const std::string &strUserID,bool isOn);
    YOUME_API int youme_setOtherMicMute(const char*  pUserID,bool mute);
    YOUME_API int youme_setOtherSpeakerMute(const char*  pUserID,bool mute);
    YOUME_API int youme_setListenOtherVoice(const char*  pUserID, bool isOn);

    /**
     *  功能描述: (七牛接口)将提供的音频数据混合到麦克风或者扬声器的音轨里面。
     *  @param data 指向PCM数据的缓冲区
     *  @param len  音频数据的大小
     *  @param timestamp 时间戳
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_inputAudioFrame(void* data, int len, uint64_t timestamp);
    
    /**
     *  功能描述: (七牛接口)将提供的视频数据到producer。
     *  @param data 指向视频数据的缓冲区
     *  @param len  视频数据的大小
     *  @param timestamp 时间戳
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_inputVideoFrame(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    
    /**
     * 功能描述: ios视频数据输入，硬件处理方式
     * @param pixelbuffer CVPixelBuffer
     * @param width 视频图像宽
     * @param height 视频图像高
     * @param fmt 视频格式
     * @param rotation 视频角度
     * @param mirror 镜像
     * @param timestamp 时间戳
     * @return YOUME_SUCCESS - 成功
     *         其他 - 具体错误码
     */
    YOUME_API int youme_inputVideoFrameForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);

#if TARGET_OS_IPHONE
    /**
    * 功能描述: ios桌面共享数据输入，硬件处理方式
    * @param pixelbuffer CVPixelBuffer
    * @param width 视频图像宽
    * @param height 视频图像高
    * @param fmt 视频格式
    * @param rotation 视频角度
    * @param mirror 镜像
    * @param timestamp 时间戳
    * @return YOUME_SUCCESS - 成功
    *         其他 - 具体错误码
    */
    YOUME_API int youme_inputVideoFrameForIOSShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
#endif

    /**
     * 功能描述: android视频数据输入，硬件处理方式
     * @param textureId 纹理id
     * @param matrix 纹理矩阵
     * @param width 视频图像宽
     * @param height 视频图像高
     * @param fmt 视频格式
     * @param rotation 视频角度
     * @param mirror 镜像
     * @param timestamp 时间戳
     * @return YOUME_SUCCESS - 成功
     *         其他 - 具体错误码
     */
    YOUME_API int youme_inputVideoFrameForAndroid(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);

    /**
     * 功能描述: 停止视频数据输入(七牛接口，在youme_inputVideoFrame之后调用，房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP事件)
     * @return YOUME_SUCCESS - 成功
     *         其他 - 具体错误码
     */
    YOUME_API int youme_stopInputVideoFrame();

    // 播放背景音乐
    YOUME_API int youme_playBackgroundMusic(const char* pFilePath, bool bRepeat);
    // 暂停背景音乐
    YOUME_API int youme_pauseBackgroundMusic();
    // 恢复背景音乐
    YOUME_API int youme_resumeBackgroundMusic();
    // 停止背景音乐
    YOUME_API int youme_stopBackgroundMusic();
    // 设置背景音乐音量
    YOUME_API int youme_setBackgroundMusicVolume(int vol);
    // 设置是否将通过耳机监听自己的声音
    YOUME_API int youme_setHeadsetMonitorOn(bool micEnabled, bool bgmEnabled);
    // 设置是否设置主播混响模式
    YOUME_API int youme_setReverbEnabled(bool enabled);
    // 设置是否启动语音检测回调
    YOUME_API int youme_setVadCallbackEnabled(bool enabled);
    // 设置是否启动讲话音量回调
    YOUME_API int youme_setMicLevelCallback(int maxLevel);
    // 设置是否启动远端语音音量回调
    YOUME_API int youme_setFarendVoiceLevelCallback(int maxLevel);
    // 设置当麦克风静音时，是否释放麦克风设备，在初始化之后、加入房间之前调用
    YOUME_API int youme_setReleaseMicWhenMute(bool enabled);
    // 设置插入耳机时，是否自动退出系统通话模式(禁用手机系统硬件信号前处理)，在初始化之后、加入房间之前调用
    YOUME_API int youme_setExitCommModeWhenHeadsetPlugin(bool enabled);

    // 暂停通话，释放对麦克风等设备资源的占用
    YOUME_API int youme_pauseChannel();
    // 恢复通话
    YOUME_API int youme_resumeChannel();

    // 设置当前录音的时间戳
    YOUME_API void youme_setRecordingTimeMs(unsigned int timeMs);
    // 设置当前播放的时间戳
    YOUME_API void youme_setPlayingTimeMs(unsigned int timeMs);
    
    YOUME_API void youme_setServerMode(int mode);
	
	YOUME_API int  youme_requestRestApi(  const char* pCommand , const char*  pQueryBody, int* requestID );
    
	//查询频道当前的用户列表，maxCount表明最多获取多少，-1表示获取所有
    YOUME_API int  youme_getChannelUserList( const char* pChannelID , int maxCount ,  bool  notifyMemChange );
	
	// 进入房间后，切换身份
	YOUME_API int  youme_setUserRole(int userRole);
	// 获取身份
	YOUME_API int  youme_getUserRole();
	// 背景音乐是否在播放
	YOUME_API bool  youme_isBackgroundMusicPlaying();
	// 是否初始化成功
	YOUME_API bool  youme_isInited();
	// 是否在某个语音房间内
	YOUME_API bool  youme_isInChannel(const char* pChannelID);
	//---------------------抢麦接口---------------------//
	/**
	* 功能描述:    抢麦相关设置（抢麦活动发起前调用此接口进行设置）
	* @param const char * pChannelID: 抢麦活动的频道id
	* @param int mode: 抢麦模式（1:先到先得模式；2:按权重分配模式）
	* @param int maxAllowCount: 允许能抢到麦的最大人数
	* @param int maxTalkTime: 允许抢到麦后使用麦的最大时间（秒）
	* @param unsigned int voteTime: 抢麦仲裁时间（秒），过了X秒后服务器将进行仲裁谁最终获得麦（仅在按权重分配模式下有效）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_setGrabMicOption(const char* pChannelID, int mode, int maxAllowCount, int maxTalkTime, unsigned int voteTime);

	/**
	* 功能描述:    发起抢麦活动
	* @param const char * pChannelID: 抢麦活动的频道id
	* @param const char * pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_startGrabMicAction(const char* pChannelID, const char* pContent);

	/**
	* 功能描述:    停止抢麦活动
	* @param const char * pChannelID: 抢麦活动的频道id
	* @param const char * pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_stopGrabMicAction(const char* pChannelID, const char* pContent);

	/**
	* 功能描述:    发起抢麦请求
	* @param const char * pChannelID: 抢麦的频道id
	* @param int score: 积分（权重分配模式下有效，游戏根据自己实际情况设置）
	* @param bool isAutoOpenMic: 抢麦成功后是否自动开启麦克风权限
	* @param const char * pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_requestGrabMic(const char* pChannelID, int score, bool isAutoOpenMic, const char* pContent);

	/**
	* 功能描述:    释放抢到的麦
	* @param const char * pChannelID: 抢麦活动的频道id
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_releaseGrabMic(const char* pChannelID);


	//---------------------连麦接口---------------------//
	/**
	* 功能描述:    连麦相关设置（角色是频道的管理者或者主播时调用此接口进行频道内的连麦设置）
	* @param const char * pChannelID: 连麦的频道id
	* @param int waitTimeout: 等待对方响应超时时间（秒）
	* @param int maxTalkTime: 最大通话时间（秒）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_setInviteMicOption(const char* pChannelID, int waitTimeout, int maxTalkTime);

	/**
	* 功能描述:    发起与某人的连麦请求（主动呼叫）
	* @param const char * pUserID: 被叫方的用户id
	* @param const char * pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_requestInviteMic(const char* pChannelID, const char* pUserID, const char* pContent);

	/**
	* 功能描述:    对连麦请求做出回应（被动应答）
	* @param const char * pUserID: 主叫方的用户id
	* @param bool isAccept: 是否同意连麦
	* @param const char * pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_responseInviteMic(const char* pUserID, bool isAccept, const char* pContent);

	/**
	* 功能描述:    停止连麦
	* @return   YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_stopInviteMic();

	/**
     * 功能描述:   向房间广播消息
     * @param pChannelID: 广播房间
     * @param pContent: 广播内容-文本串
     * @param requestID:返回消息标识，回调的时候会回传该值
     * @return   YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_sendMessage( const char* pChannelID,  const char* pContent, int* requestID );

    	/**
     * 功能描述:   房间内点对点发送消息
     * @param pChannelID: 房间Id
     * @param pContent: 消息内容-文本串
     * @param pUserID: 接收用户
     * @param requestID:返回消息标识，回调的时候会回传该值
     * @return   YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_sendMessageToUser( const char* pChannelID,  const char* pContent, const char* pUserID, int* requestID );
    
    /**
     *  功能描述: 把某人踢出房间
     *  @param  pUserID: 被踢的用户ID
     *  @param  pChannelID: 从哪个房间踢出
     *  @param  lastTime: 踢出后，多长时间内不允许再次进入
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_kickOtherFromChannel( const char* pUserID, const char* pChannelID , int lastTime );
	
    // 设置是否开启视频编码器
    YOUME_API int youme_openVideoEncoder(const char* pFilePath);
    
    // 创建渲染ID
    YOUME_API int youme_createRender(const char * userId);
    
    // 删除渲染ID
    YOUME_API int youme_deleteRender(int renderId);
    // 删除userid对应的渲染
    YOUME_API int youme_deleteRenderByUserID(const char * userId);
    
    // 设置视频回调
    YOUME_API int youme_setVideoCallback(const char * strObjName);
    
    // 开始camera capture
    YOUME_API int youme_startCapture();
    
    // 停止camera capture
    YOUME_API int youme_stopCapture();
  
    /**
     *  功能描述: 设置预览帧率
     *  @param  fps:帧率（1-60），默认15帧，必须在设置分辨率之前调用
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setVideoPreviewFps(int fps);

    /**
     *  功能描述: 设置帧率
     *  @param  fps:帧率（1-60），默认15帧，必须在设置分辨率之前调用
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setVideoFps(int fps);

    /**
     *  功能描述: 设置本地视频预览镜像模式，目前仅对前置摄像头有效
     *  @param  enabled:true表示打开镜像，false表示关闭镜像
     *  @return void
     */
    YOUME_API void youme_setLocalVideoPreviewMirror(bool enable);
    
    // 切换前后摄像头
    YOUME_API int youme_switchCamera();
    
    /**
     *  功能描述: 权限检测结束后重置摄像头
     *  @param
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_resetCamera();

    // 设置本地视频渲染回调的分辨率
    YOUME_API int youme_setVideoLocalResolution(int width, int height);
    
    // 设置是否前置摄像头
    YOUME_API int youme_setCaptureFrontCameraEnable(bool enable);
    
    // 屏蔽/恢复某个UserId的视频流
    YOUME_API int youme_maskVideoByUserId(const char *userId, bool mask);
    
    //获取视频数据
    /*****
    YOUME_API int youme_getVideoFrame(int renderId, I420Frame * frame);
     */
    
    // 获取视频数据
    YOUME_API char * youme_getVideoFrame(const char* userId, int * len, int * width, int * height);
    
    
    YOUME_API char * youme_getVideoFrameNew(const char* userId, int * len, int * width, int * height, int * fmt);

	YOUME_API void youme_setVideoFrameCallback(VideoFrameCallbackForFFI callback);
    
    YOUME_API void youme_setVideoEventCallback(VideoEventCallbackForFFI callback);
    
    /**
     *  功能描述: 设置日志等级
     *  @param level: 日志等级
     */
    YOUME_API void youme_setLogLevel( int consoleLevel, int fileLevel);
    
    /**
     *  功能描述: 设置外部输入模式的语音采样率
     *  @param inputSampleRate: 输入语音采样率
     *  @param mixedCallbackSampleRate: mix后输出语音采样率
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setExternalInputSampleRate( int inputSampleRate, int mixedCallbackSampleRate);
    
    /**
     *  功能描述: 设置视频网络传输过程的分辨率,高分辨率
     *  @param width:宽
     *  @param height:高
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int  youme_setVideoNetResolution( int width, int height );
    
    /**
     *  功能描述: 设置视频网络传输过程的分辨率，低分辨率
     *  @param width:宽
     *  @param height:高
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int  youme_setVideoNetResolutionForSecond( int width, int height );
    
    /**
     *  功能描述: 设置音视频统计数据时间间隔
     *  @param interval: 时间间隔，单位毫秒
     */
    YOUME_API void youme_setAVStatisticInterval( int interval  );
    
    /**
     *  功能描述: 设置Audio的传输质量
     *  @param quality: 0: low 1: high
     *
     *  @return None
     */
    YOUME_API void youme_setAudioQuality( int quality );
    
    /**
     *  功能描述: 设置视频数据上行的码率的上下限。
     *  @param maxBitrate: 最大码率，单位kbit/s.  0无效
     *  @param minBitrate: 最小码率，单位kbit/s.  0无效
     
     *  @return None
     *
     *  @warning:需要在进房间之前设置
     */
    YOUME_API void youme_setVideoCodeBitrate( unsigned int maxBitrate,  unsigned int minBitrate );
    
    /**
     *  功能描述: 设置视频数据上行的码率的上下限,第二路(默认不传)
     *  @param maxBitrate: 最大码率，单位kbit/s.  0无效
     *  @param minBitrate: 最小码率，单位kbit/s.  0无效
     
     *  @return None
     *
     *  @warning:需要在进房间之前设置
     */
    YOUME_API void youme_setVideoCodeBitrateForSecond( unsigned int maxBitrate,  unsigned int minBitrate );
    
    /**
     *  功能描述: 获取视频数据上行的当前码率。
     *
     *  @return 视频数据上行的当前码率
     */
    YOUME_API unsigned int youme_getCurrentVideoCodeBitrate( );
    
    /**
     *  功能描述: 设置视频数据是否同意开启硬编硬解
     *  @param bEnable: true:开启，false:不开启
     *
     *  @return None
     *
     *  @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
     *  @warning:需要在进房间之前设置
     */
    YOUME_API void youme_setVideoHardwareCodeEnable( bool bEnable );
    
    /**
     *  功能描述: 获取视频数据是否同意开启硬编硬解
     *  @return true:开启，false:不开启， 默认为true;
     *
     *  @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
     */
    YOUME_API bool youme_getVideoHardwareCodeEnable( );
    
    /**
     *  功能描述: 获取android下是否启用GL
     *  @return true:开启，false:不开启， 默认为true;
     *
     *  @note: 获取android下是否启用GL。
     */
    YOUME_API bool youme_getUseGL( );
    
    /**
     *  功能描述: 设置视频无帧渲染的等待超时时间，超过这个时间会给上层回调
     *  @param timeout: 超时时间，单位为毫秒
    */
    YOUME_API void youme_setVideoNoFrameTimeout(int timeout);
    
    /**
     *  功能描述: 查询多个用户视频信息（支持分辨率）
     *  @param userList: 用户ID列表的json数组
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_queryUsersVideoInfo( char* userList);
    
    /**
     *  功能描述: 设置多个用户视频信息（支持分辨率）
     *  @param videoinfoList: 用户对应分辨率列表的json数组
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int  youme_setUsersVideoInfo( char* videoinfoList );
    
    /**
     *  功能描述: 美颜开关，默认是关闭美颜
     *  @param open: true表示开启美颜，false表示关闭美颜
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_openBeautify(bool open) ;
    
    /**
     *  功能描述: 美颜强度参数设置
     *  @param param: 美颜参数，0.0 - 1.0 ，默认为0，几乎没有美颜效果，0.5左右效果明显
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_beautifyChanged(float param) ;
    
//    /**
//     *  功能描述: 瘦脸开关
//     *  @param param: true 开启瘦脸，false关闭，默认 false
//     *  @return YOUME_SUCCESS - 成功
//     *          其他 - 具体错误码
//     */
//    YOUME_API int youme_stretchFace(bool stretch) ;
    
    /**
     *  功能描述: 调用后同步完成麦克风释放，只是为了方便使用 IM 的录音接口时切换麦克风使用权。
     *  @return bool - 成功
     */
    YOUME_API bool youme_releaseMicSync();
    
    /**
     *  功能描述: 调用后恢复麦克风到释放前的状态，只是为了方便使用 IM 的录音接口时切换麦克风使用权。
     *  @return bool - true 成功
     */
    YOUME_API bool youme_resumeMicSync();
    


	YOUME_API void youme_setMixVideoSize(int width, int height);

	YOUME_API void youme_addMixOverlayVideo(char* userId, int x, int y, int z, int width, int height);

	/**
	*  功能描述: enabeld , true 适用于unity 和 cocos lua / cocos js 游戏引擎, false 专门用于electron 引擎，两者在回调数据格式上有差异
	*/
    YOUME_API void youme_videoEngineModelEnabled(bool enabeld);

	YOUME_API int youme_getCameraCount();

	YOUME_API int youme_getCameraName(int cameraid, char*cameraName);

	YOUME_API int youme_setOpenCameraId(int cameraid);

    YOUME_API int youme_setVideoFrameRawCbEnabled(bool enabeld);

    /**
     *  功能描述: 获取windwos/macos平台，audio采集设备数量
     *  @return int - 数目，
     */
    YOUME_API int youme_getRecordDeviceCount();
    
    /**
     *  功能描述: 获取windows/macos平台 record设备 对应信息
     *  @param  index:列表中的位置
     *  @param  deviceName:设备名称
     *  @param  deviceUid:设备唯一ID，用于设置设备
     *  @return string - 成功:非空name 失败:空字符串
     */
    YOUME_API bool youme_getRecordDeviceInfo(int index, char* deviceName, char* deviceUId);
    
    /**
     *  功能描述: 设置windows/macos平台打开录音设备id
     *  @param  deviceUId: record设备的Uid
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setRecordDevice( const char* deviceUId);

	/**
	* 功能描述: 外部输入共享视频数据输入，和摄像头视频流一起发送出去
	* @param data 视频帧数据
	* @param len 视频数据大小
	* @param width 视频图像宽
	* @param height 视频图像高
	* @param fmt 视频格式
	* @param rotation 视频角度
	* @param mirror 镜像
	* @param timestamp 时间戳
	* @return YOUME_SUCCESS - 成功
	*         其他 - 具体错误码
	*/
	YOUME_API int youme_inputVideoFrameForShare(void* data, int len, int width, int  height, int fmt, int rotation, int mirror, uint64_t timestamp);

	/**
	 *  功能描述: 设置小流视频帧率
     *  @param  fps:帧率（1-30），默认15帧
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setVideoFpsForSecond(int fps);

	/**
	*  功能描述: 设置共享视频帧率
	*  @param  fps:帧率（1-30），默认15帧
	*  @return YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_setVideoFpsForShare(int fps);

	/**
	*  功能描述: 设置共享视频网络传输过程的分辨率
	*  @param width:宽
	*  @param height:高
	*  @return YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_setVideoNetResolutionForShare(int width, int height);

	/**
	*  功能描述: 设置共享视频数据上行的码率的上下限
	*  @param maxBitrate: 最大码率，单位kbps.  0：使用默认值
	*  @param minBitrate: 最小码率，单位kbps.  0：使用默认值

	*  @return None
	*
	*  @warning:需要在进房间之前设置
	*/
	YOUME_API void youme_setVideoCodeBitrateForShare(unsigned int maxBitrate, unsigned int minBitrate);

	YOUME_API int youme_setVBR(int useVBR);

	YOUME_API int youme_setVBRForSecond(int useVBR);

	/**
	*  功能描述: 设置共享流视频编码是否采用VBR动态码率方式
	#  @param useVBR 0为不使用vbr，1是使用vbr，默认为0
	*
	*  @return None
	*
	*  @warning:需要在进房间之前设置
	*/
	YOUME_API int youme_setVBRForShare(int useVBR);

    /**
    *  检查桌面/窗口共享权限，在开始共享之前调用（主要是macos 10.15增加屏幕录制权限隐私设置）
    * @return   YOUME_SUCCESS - 成功
    *                 YOUME_ERROR_SHARE_NO_PERMISSION - 没有共享权限
    */
    YOUME_API int youme_checkSharePermission();
    
    /**
     *  功能描述: 设置视频平滑开关
     *  @param mode: 0: 关闭；1: 打开平滑
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setVideoSmooth( int enable );

#ifdef WIN32
	/**
	* 开始采集共享区域视频并开始共享，设置共享视频流采集模式和参数
	* @param mode: 共享视频采集模式  1：采集设备 2：窗口录制模式 3：桌面录制模式
	* @param renderHandle: 共享视频preview窗口句柄，必选
	* @param captureHandle: 指定需要采集的窗口句柄，采集桌面则可以设置为NULL
	* @return YOUME_SUCCESS - 成功
	*          其他 - 具体错误码
	*/
	YOUME_API int youme_startShareStream(int mode, HWND renderHandle, HWND captureHandle);


    /**
    * 录屏的时候排除指定窗口
    * @param windowid: 被排除的窗口id
    * @return YOUME_SUCCESS - 成功
    *          其他 - 具体错误码
    */
    YOUME_API int youme_setShareExclusiveWnd( HWND windowid );

    /**
     * 开始屏幕采集并录像
     * @param filePath:录像文件路径，目前只支持mp4格式
     * @param renderHandle:渲染窗口句柄，调用libscreen依赖库必带参数，不为空
     * @param captureHandle:录制窗口句柄。为空，则表示录制整个屏幕，不为空，则表示录制某个窗口
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_startSaveScreen( const char*  filePath, HWND renderHandle, HWND captureHandle);

#elif MAC_OS
    /**
    * 开始采集共享区域视频并开始共享，设置共享视频流采集模式和参数
    * @param mode: 共享视频采集模式  1：采集设备 2：窗口录制模式 3：桌面录制模式
    * @param windowid: 对于窗口录制模式，为窗口id; 对于桌面录制模式，为screen id
    * @return YOUME_SUCCESS - 成功
    *          其他 - 具体错误码
    */
    YOUME_API int youme_startShareStream(int mode, int windowid);


    /**
    * 录屏的时候排除指定窗口
    * @param windowid: 被排除的窗口id
    * @return YOUME_SUCCESS - 成功
    *          其他 - 具体错误码
    */
YOUME_API int youme_setShareExclusiveWnd( int windowid );

    /**
     * 开始屏幕采集并录像
     * @param filePath:录像文件路径，目前只支持mp4格式
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_startSaveScreen(const char*  filePath);

#endif //WIN32

#if defined(WIN32) || defined(MAC_OS) 
	/**
	* 结束采集共享区域视频并结束共享
	* @return 无返回码
	*/
	YOUME_API void youme_stopShareStream();

    /**
    * 窗口模式下, 获取当前所有窗口信息，以json格式字符串返回
    * @return 无返回码
    */
	YOUME_API void youme_getWinList(char* winList);

	/**
     * 录屏模式下, 获取当前所有显示器信息，以json格式字符串返回
     * @return 无返回码
     */
    YOUME_API void youme_getScreenList (char *screenList);

    /**
    * 获取共享视频流数据
    * @param len: 共享视频流数据长度
    * @param width: 共享视频流分辨率宽度
    * @param width: 共享视频流分辨率高度
    * @param width: 共享视频流数据格式，默认是yv12
    * @return char *buf - 返回数据buffer
    *          需要检查是否为null
    */
    YOUME_API char *youme_getShareVideoData(int * len, int * width, int * height, int * fmt);
    
    /**
     *  功能描述: 设置视频网络传大小流自动调整
     *  @param mode: 0: 自动调整；1: 上层设置大小流接收选择
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setVideoNetAdjustmode( int mode );
    
    
#endif //defined(WIN32) || defined(MAC_OS)    
    /**
     * 停止屏幕采集录像
     * @return 无返回码
     */
    YOUME_API void youme_stopSaveScreen();
    
    /**
     *  功能描述: 设置是否开启会议纪要
     *  @param  enable: true,开启，false，不开启，默认不开启
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    YOUME_API int youme_setTranscriberEnabled( bool enable);
    
    /*
     * 功能：国际语言文本翻译
     * @param requestID：请求ID
     * @param text：内容
     * @param destLangCode：目标语言编码
     * @param srcLangCode：源语言编码
     * @return 错误码
     */
    YOUME_API int  youme_translateText(unsigned int* requestID, const char* text, enum YouMeLanguageCode destLangCode, enum YouMeLanguageCode srcLangCode);
    

#ifdef __cplusplus
}
#endif

#endif //IYouMeCInterface_hpp
 /* IYouMeCInterface_hpp */


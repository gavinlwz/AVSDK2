//
//  YouMeVoiceEngineOC++.h
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#ifndef __SDKTest__YouMeVoiceEngineOC____
#define __SDKTest__YouMeVoiceEngineOC____
#include <dispatch/dispatch.h>


#import "IYouMeVoiceEngine.h"
#import "YouMeConstDefine.h"
#import "IYouMeEventCallback.h"
#include <string>

class YouMeVoiceEngineImp : public IYouMeEventCallback
{
    public:
    static YouMeVoiceEngineImp *getInstance ();
    static void destroy ();

    public:
    //公共接口
    YouMeErrorCode init ();

    YouMeErrorCode unInit ();


    YouMeErrorCode login (const std::string &strUserName, const std::string &strPwd);

    YouMeErrorCode logout ();

    //打开扬声器，默认是打开的
    YouMeErrorCode setSpeakerOn ();
    //关闭扬声器，从听筒输出声音
    YouMeErrorCode setSpeakerOff ();
    
    void setMicrophoneMute (bool muteOn);

    //获取当前音量大小
    YouMeErrorCode getCurrentVolume (unsigned int &uiCurrentVolume);

    //多人语音接口
    YouMeErrorCode joinChannelSingle (const std::string &strUserID,const std::string &strChannelID, YouMeUserRole_t eUserRole,
                                         bool micMute, bool needUserList, bool autoSendStatus);
    YouMeErrorCode leaveChannel (const std::string &strChannelID);
    YouMeErrorCode leaveChannelAll ();
    
    // 多频道接口
    YouMeErrorCode joinChannelMulti(const std::string & strUserID, const std::string & strChannelID);
    YouMeErrorCode speakToChannel(const std::string & strChannelID);

    //双人语音接口

    //设置接口，可选
    //是否使用移动网络,默认不用
    YouMeErrorCode setUseMobileNetworkEnabled (bool bEnabled = false);

    //启用/不启用声学回声消除,默认启用
    YouMeErrorCode setAECEnabled (bool bEnabled = true);

    //启用/不启用背景噪音抑制功能,默认启用
    YouMeErrorCode setANSEnabled (bool bEnabled = true);

    //启用/不启用自动增益补偿功能,默认启用
    YouMeErrorCode setAGCEnabled (bool bEnabled = true);

    //启用/不启用静音检测功能,默认启用
    YouMeErrorCode setVADEnabled (bool bEnabled = true);
    void testfun();
    public:
    virtual void onEvent (const YouMeEvent_t event, const YouMeErrorCode_t error, const char * channel, const char * param);

/*****
    // virtual void onCallEvent (const YouMeCallEvent_t &event, const YouMeErrorCode_t &errCode, const std::string & strRoomID);
    // iStatus 0 关闭  1 打开。用来通知有其他用户打开，关闭麦克风了
    // virtual void onCommonStatusEvent (STATUS_EVENT_TYPE_t eventType, const std::string &strUserID, int iStatus) ;
    // iStatus 0 打开  1 关闭。用来通知有其他用户打开，关闭麦克风
    // virtual void onMicControllStatusEvent (const std::string &strUserID, int iStatus) ;
    // iStatus 0 打开  1 关闭。用来通知有其他用户打开，关闭扬声器
    // virtual void onSpeakerControllStatusEvent (const std::string &strUserID, int iStatus);
    // iStatus 0 打开  1 关闭。被屏蔽通知
    // virtual void onAvoidStatusEvent (const std::string &strUserID, int iStatus);
    // 房间用户变化通知
    // virtual void onMemberChangeEvent (const std::string &strUserIDs, const std::string & strRoomID);
*****/
    
//    //设置其他成员麦克风权限
//    virtual YouMeErrorCode setOtherMicStatus(const std::string&strUserID,bool isOn);
//    //功能描述:控制其他人的扬声器开关
//    virtual YouMeErrorCode setOtherSpeakerStatus (const std::string& strUserID,bool isOn);
//    //选择消除其他人的语音
//    virtual YouMeErrorCode avoidOtherVoiceStatus (const std::string& strUserID,bool isOn);
//
//    virtual YouMeErrorCode mixAudioTrack(const void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate,
//                                         int nBytesPerSample, bool bFloat, bool bLittleEndian,
//                                         bool bInterleaved, bool bForSpeaker);
    // 播放指定路径的背景音乐
    virtual YouMeErrorCode playBackgroundMusic(const std::string& strFilePath, bool bRepeat);
    // 停止播放背景音乐
    virtual YouMeErrorCode stopBackgroundMusic();
    // 设置背景音乐音量
    virtual YouMeErrorCode setBackgroundMusicVolume(uint8_t vol);
    // 开关耳机监听自己的声音
    virtual YouMeErrorCode setHeadsetMonitorOn(bool isOn);
    
    private:
    YouMeVoiceEngineImp ();
    ~YouMeVoiceEngineImp ();

    private:
    static YouMeVoiceEngineImp *mPInstance;
};


#endif /* defined(__SDKTest__YouMeVoiceEngineOC____) */

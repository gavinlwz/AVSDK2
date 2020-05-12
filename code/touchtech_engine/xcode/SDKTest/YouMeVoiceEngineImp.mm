//
//  YouMeVoiceEngineOC++.cpp
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#include "IMyYouMeInterface.h"
#import "IYouMeVoiceEngine.h"
#import "YouMeConstDefine.h"
#include "YouMeVoiceEngineImp.h"
#import "YouMeVoiceEngineOC.h"
#include <thread>
#include <sstream>
#include <future>

extern void SetTestConfig(TEST_MODE bTest);

YouMeVoiceEngineImp *YouMeVoiceEngineImp::mPInstance = NULL;

YouMeVoiceEngineImp *YouMeVoiceEngineImp::getInstance ()
{
    if (mPInstance == NULL)
    {
        mPInstance = new YouMeVoiceEngineImp ();
    }
    return mPInstance;
}
void YouMeVoiceEngineImp::destroy ()
{
    delete mPInstance;
    mPInstance = NULL;
}

//公共接口
void YouMeVoiceEngineImp::testfun()
{
    int seeds =0;
    while (true) {
        seeds++;
        srand(seeds);
        int r =  rand() %8;
        if (0 == r) {
            IYouMeVoiceEngine::getInstance ()->init (this, "YOUMEBC2B3171A7A165DC10918A7B50A4B939F2A187D0", "r1+ih9rvMEDD3jUoU+nj8C7VljQr7Tuk4TtcByIdyAqjdl5lhlESU0D+SoRZ30sopoaOBg9EsiIMdc8R16WpJPNwLYx2WDT5hI/HsLl1NJjQfa9ZPuz7c/xVb8GHJlMf/wtmuog3bHCpuninqsm3DRWiZZugBTEj2ryrhK7oZncBAAE=");
        }
        else if(1 == r)
        {
            IYouMeVoiceEngine::getInstance ()->unInit();
        }
        else if (2 == r)
        {
            IYouMeVoiceEngine::getInstance()->joinChannelSingle("tester1", "abcdef", YOUME_USER_TALKER, false, false, false);
        }
        else if (3 == r)
        {
            IYouMeVoiceEngine::getInstance()->leaveChannel("abcdef");
        }
        else if(4 == r)
        {
            if (YOUME_SUCCESS == IYouMeVoiceEngine::getInstance ()->init (this, "YOUMEBC2B3171A7A165DC10918A7B50A4B939F2A187D0", "r1+ih9rvMEDD3jUoU+nj8C7VljQr7Tuk4TtcByIdyAqjdl5lhlESU0D+SoRZ30sopoaOBg9EsiIMdc8R16WpJPNwLYx2WDT5hI/HsLl1NJjQfa9ZPuz7c/xVb8GHJlMf/wtmuog3bHCpuninqsm3DRWiZZugBTEj2ryrhK7oZncBAAE="))
            {
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else if (5 == r)
        {
            if (YOUME_SUCCESS == IYouMeVoiceEngine::getInstance()->joinChannelSingle("tester1","abcdef", YOUME_USER_TALKER, false, false, false))
            {
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else if (6 == r)
        {
            if (YOUME_SUCCESS == IYouMeVoiceEngine::getInstance()->leaveChannel("abcdef"))
            {
                 std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        else if (7 == r)
        {
            if (YOUME_SUCCESS == IYouMeVoiceEngine::getInstance()->unInit())
            {
                 std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
    }
}

YouMeErrorCode YouMeVoiceEngineImp::init ()
{
    //SetFixedIP("10.0.0.167");
   // SetFixedIP("120.25.77.221"); //正式服务器
   // SetFixedIP("10.0.0.167");
 //   SetFixedIP("123.59.69.198"); //压力测试
//    SetFixedIP("120.25.89.106");//测试的服务器
      //暴力测试
   // SetTestConfig(true);
   
   /* for (int i=0; i<10; i++) {
        std::thread *test = new std::thread(&YouMeVoiceEngineImp::testfun,this);
        
        test->detach();
        delete test;
    }
   */
    SetTestConfig(TEST_SERVER );
    
    IYouMeVoiceEngine::getInstance ()->init (this, "YOUME5BE427937AF216E88E0F84C0EF148BD29B691556", "y1sepDnrmgatu/G8rx1nIKglCclvuA5tAvC0vXwlfZKOvPZfaUYOTkfAdUUtbziW8Z4HrsgpJtmV/RqhacllbXD3abvuXIBlrknqP+Bith9OHazsC1X96b3Inii6J7Und0/KaGf3xEzWx/t1E1SbdrbmBJ01D1mwn50O/9V0820BAAE=");
    
    
    return YOUME_SUCCESS;
}

YouMeErrorCode YouMeVoiceEngineImp::unInit ()
{
    return IYouMeVoiceEngine::getInstance ()->unInit ();
}


//打开扬声器，默认是打开的
YouMeErrorCode YouMeVoiceEngineImp::setSpeakerOn ()
{
   // IYouMeVoiceEngine::getInstance ()->setSpeakerMute (true);
    /*unsigned int iVol = IYouMeVoiceEngine::getInstance()->getVolume();
    IYouMeVoiceEngine::getInstance()->setVolume(45);
    iVol = IYouMeVoiceEngine::getInstance()->getVolume();*/
    return YOUME_SUCCESS;
}
//关闭扬声器，从听筒输出声音
YouMeErrorCode YouMeVoiceEngineImp::setSpeakerOff ()
{
    return YOUME_SUCCESS;
}

void YouMeVoiceEngineImp::setMicrophoneMute (bool muteOn)
{
    IYouMeVoiceEngine::getInstance()->setMicrophoneMute(muteOn);
}


//获取当前音量大小
YouMeErrorCode YouMeVoiceEngineImp::getCurrentVolume (unsigned int &uiCurrentVolume)
{
    uiCurrentVolume = IYouMeVoiceEngine::getInstance ()->getVolume ();
    return YOUME_SUCCESS;
}


//多人语音接口
YouMeErrorCode YouMeVoiceEngineImp::joinChannelSingle (const std::string &strUserID,const std::string &strChannelID, YouMeUserRole_t eUserRole,
                                                    bool micMute, bool needUserList, bool autoSendStatus)
{
    return IYouMeVoiceEngine::getInstance ()->joinChannelSingle (strUserID.c_str(), strChannelID.c_str(), eUserRole, micMute, needUserList, autoSendStatus);
}

YouMeErrorCode YouMeVoiceEngineImp::leaveChannel (const std::string &strChannelID)
{
    return IYouMeVoiceEngine::getInstance ()->leaveChannel (strChannelID.c_str());
}

YouMeErrorCode YouMeVoiceEngineImp::leaveChannelAll ()
{
    return IYouMeVoiceEngine::getInstance ()->leaveChannelAll ();
}

// 多频道接口
YouMeErrorCode YouMeVoiceEngineImp::joinChannelMulti(const std::string & strUserID, const std::string & strChannelID)
{
    return IYouMeVoiceEngine::getInstance()->joinChannelMulti(strUserID.c_str(), strChannelID.c_str());

}

YouMeErrorCode YouMeVoiceEngineImp::speakToChannel(const std::string & strChannelID)
{
    return IYouMeVoiceEngine::getInstance()->speakToChannel(strChannelID.c_str());
}

//双人语音接口

//设置接口，可选
//是否使用移动网络,默认不用
YouMeErrorCode YouMeVoiceEngineImp::setUseMobileNetworkEnabled (bool bEnabled)
{
    IYouMeVoiceEngine::getInstance ()->setUseMobileNetworkEnabled (bEnabled);
    return YOUME_SUCCESS;
}

////设置其他成员麦克风权限
//YouMeErrorCode YouMeVoiceEngineImp::setOtherMicStatus(const std::string&strUserID,bool isOn){
//    return IYouMeVoiceEngine::getInstance()->setOtherMicStatus(strUserID.c_str(), isOn);
//}
////功能描述:控制其他人的扬声器开关
//YouMeErrorCode YouMeVoiceEngineImp::setOtherSpeakerStatus (const std::string& strUserID,bool isOn){
//    return IYouMeVoiceEngine::getInstance()->setOtherSpeakerStatus(strUserID.c_str(), isOn);
//}
////选择消除其他人的语音
//YouMeErrorCode YouMeVoiceEngineImp::avoidOtherVoiceStatus (const std::string& strUserID,bool isOn){
//    return IYouMeVoiceEngine::getInstance()->avoidOtherVoiceStatus(strUserID.c_str(), isOn);
//}

//启用/不启用声学回声消除,默认启用
YouMeErrorCode YouMeVoiceEngineImp::setAECEnabled (bool bEnabled)
{
    // IYouMeVoiceEngine::getInstance ()->setAECEnabled (bEnabled);
    return YOUME_SUCCESS;
}

//启用/不启用背景噪音抑制功能,默认启用
YouMeErrorCode YouMeVoiceEngineImp::setANSEnabled (bool bEnabled)
{
    // IYouMeVoiceEngine::getInstance ()->setANSEnabled (bEnabled);
    return YOUME_SUCCESS;
}

//启用/不启用自动增益补偿功能,默认启用
YouMeErrorCode YouMeVoiceEngineImp::setAGCEnabled (bool bEnabled)
{
    // IYouMeVoiceEngine::getInstance ()->setAGCEnabled (bEnabled);
    return YOUME_SUCCESS;
}

//启用/不启用静音检测功能,默认启用
YouMeErrorCode YouMeVoiceEngineImp::setVADEnabled (bool bEnabled)
{
    // IYouMeVoiceEngine::getInstance ()->setVADEnabled (bEnabled);
    return YOUME_SUCCESS;
}

//YouMeErrorCode YouMeVoiceEngineImp::mixAudioTrack(const void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate,
//                                     int nBytesPerSample, bool bFloat, bool bLittleEndian,
//                                     bool bInterleaved, bool bForSpeaker)
//{
//    return IYouMeVoiceEngine::getInstance()->mixAudioTrack(pBuf, nSizeInByte, nChannelNUm, nSampleRate,
//                                                           nBytesPerSample, bFloat, bLittleEndian,
//                                                           bInterleaved, bForSpeaker);
//    return YOUME_ERROR_UNKNOWN;
//}

YouMeErrorCode YouMeVoiceEngineImp::playBackgroundMusic(const std::string& strFilePath, bool bRepeat)
{
    return IYouMeVoiceEngine::getInstance()->playBackgroundMusic(strFilePath.c_str(), bRepeat);
}

YouMeErrorCode YouMeVoiceEngineImp::stopBackgroundMusic()
{
    return IYouMeVoiceEngine::getInstance()->stopBackgroundMusic();
}

YouMeErrorCode YouMeVoiceEngineImp::setBackgroundMusicVolume(uint8_t vol)
{
    return IYouMeVoiceEngine::getInstance()->setBackgroundMusicVolume(vol);
}

YouMeErrorCode YouMeVoiceEngineImp::setHeadsetMonitorOn(bool isOn)
{
    return IYouMeVoiceEngine::getInstance()->setHeadsetMonitorOn(isOn);
}

void YouMeVoiceEngineImp::onEvent (const YouMeEvent event, const YouMeErrorCode error, const char * channel , const char * param)
{
    return [[YouMeVoiceEngineOC getInstance] onEvent:event errcode:error channel:[NSString stringWithUTF8String:channel] param:[NSString stringWithUTF8String:param]];
}

/******
void YouMeVoiceEngineImp::onCallEvent (const YouMeCallEvent_t &event, const YouMeErrorCode_t &errCode, const std::string & strRoomID)
{
    return [[YouMeVoiceEngineOC getInstance] onCallEvent:event errcode:errCode roomid: [NSString stringWithUTF8String: strRoomID.c_str()]];
}

//iStatus 0 关闭  1 打开。用来通知有其他用户打开，关闭麦克风了
void YouMeVoiceEngineImp::onCommonStatusEvent (STATUS_EVENT_TYPE_t eventType,const std::string &strUserID, int iStatus) {
    NSLog(@"onMicStatusEvent:%d %s %d",eventType,strUserID.c_str(),iStatus);
}
////iStatus 0 打开  1 关闭。用来通知有其他用户打开，关闭麦克风
//void YouMeVoiceEngineImp::onMicControllStatusEvent (const std::string &strUserID, int iStatus){
//    NSLog(@"onMicControllStatusEvent:%s %d",strUserID.c_str(),iStatus);
//}
////iStatus 0 打开  1 关闭。用来通知有其他用户打开，关闭扬声器
//void YouMeVoiceEngineImp::onSpeakerControllStatusEvent (const std::string &strUserID, int iStatus){
//    NSLog(@"onSpeakerControllStatusEvent:%s %d",strUserID.c_str(),iStatus);
//}
////iStatus 0 打开  1 关闭。被屏蔽通知
//void YouMeVoiceEngineImp::onAvoidStatusEvent (const std::string &strUserID, int iStatus){
//    NSLog(@"onAvoidStatusEvent:%s %d",strUserID.c_str(),iStatus);
//}
//房间用户变化通知
void YouMeVoiceEngineImp::onMemberChangeEvent (const std::string &strUserIDs, const std::string & strRoomID){
    NSLog(@"onMemberChangeEvent Userid: %s, RoomID: %s.",strUserIDs.c_str(), strRoomID.c_str());
}
*****/

YouMeVoiceEngineImp::YouMeVoiceEngineImp ()
{
}
YouMeVoiceEngineImp::~YouMeVoiceEngineImp ()
{
}

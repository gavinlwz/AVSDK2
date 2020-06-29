//
//  YouMeVoiceEngine.m
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import "YouMeVoiceEngineOC.h"
#import "YouMevoiceEngineImp.h"


extern void SetTestConfig(TEST_MODE bTest);

static YouMeVoiceEngineOC *sharedInstance = nil;
@implementation YouMeVoiceEngineOC
//公共接口
+ (YouMeVoiceEngineOC *)getInstance
{
    @synchronized (self)
    {
        if (sharedInstance == nil)
        {
            sharedInstance = [self alloc];
        }
    }

    return sharedInstance;
}

+ (void)destroy
{
    YouMeVoiceEngineImp::destroy ();
}

- (id)init:(id<VoiceEngineCallback>)delegate
{
    self = [super init];
    if (self)
    {
        SetTestConfig(TEST_SERVER);
        self.delegate = delegate;
        YouMeVoiceEngineImp::getInstance ()->init ();
    }
    return self;
}

- (int)unInit
{
    return YouMeVoiceEngineImp::getInstance ()->unInit ();
}



//打开扬声器，默认是打开的
- (int)setSpeakerOn
{
    return YouMeVoiceEngineImp::getInstance ()->setSpeakerOn ();
}
//关闭扬声器，从听筒输出声音
- (int)setSpeakerOff
{
    return YouMeVoiceEngineImp::getInstance ()->setSpeakerOff ();
}

- (void)setMicrophoneMute:(bool)isOn
{
    YouMeVoiceEngineImp::getInstance ()->setMicrophoneMute(isOn);
}

- (int)playBackgroundMusic:(NSString *)path
{
    return YouMeVoiceEngineImp::getInstance ()->playBackgroundMusic([path UTF8String], true);
}

- (int)stopBackgroundMusic
{
    return YouMeVoiceEngineImp::getInstance ()->stopBackgroundMusic();
}

- (int)setBackgroundMusicVolume:(uint8_t)vol
{
    return YouMeVoiceEngineImp::getInstance ()->setBackgroundMusicVolume (vol);
}

- (int)setHeadsetMonitorOn:(bool)isOn;
{
    return YouMeVoiceEngineImp::getInstance ()->setHeadsetMonitorOn(isOn);
}

//获取当前音量大小
- (unsigned int)getCurrentVolume
{
    unsigned int uiCurrentVolume = 0;
    YouMeVoiceEngineImp::getInstance ()->getCurrentVolume (uiCurrentVolume);
    return uiCurrentVolume;
}


//多人语音接口
- (int)joinChannelSingle:(NSString *)strUserID strChannelID:(NSString *)strChannelID userRole:(int)userRole
              micMute:(bool)micMute needUserList:(bool)needUserList autoSendStatus:(bool)autoSendStatus
{
    return YouMeVoiceEngineImp::getInstance ()->joinChannelSingle ([strUserID UTF8String],[strChannelID UTF8String],
                                                                      (YouMeUserRole_t)userRole, micMute, needUserList, autoSendStatus);
}

- (int)joinConferenceMulti:(NSString *)strUserID strChannelID:(NSString *)strChannelID
{
    return YouMeVoiceEngineImp::getInstance()->joinChannelMulti([strUserID UTF8String], [strChannelID UTF8String]);
}

- (int)leaveChannel:(NSString *)strChannelID
{
    
    return YouMeVoiceEngineImp::getInstance ()->leaveChannel ([strChannelID UTF8String]);
}

- (int)leaveChannelAll
{
    
    return YouMeVoiceEngineImp::getInstance ()->leaveChannelAll ();
}

- (int)speakToChannel:(NSString *)strChannelID
{
    return YouMeVoiceEngineImp::getInstance()->speakToChannel([strChannelID UTF8String]);
}

//- (int)setOtherMicStatus:(NSString *)strUserID isOn:(bool )isON{
//    return YouMeVoiceEngineImp::getInstance()->setOtherMicStatus([strUserID UTF8String], isON);
//}
//
//- (int)setOtherSpeakerStatus:(NSString *)strUserID isOn:(bool )isON{
//    return YouMeVoiceEngineImp::getInstance()->setOtherSpeakerStatus([strUserID UTF8String], isON);
//}
//
//- (int)avoidOtherVoiceStatus:(NSString *)strUserID isOn:(bool )isON{
//    return YouMeVoiceEngineImp::getInstance()->avoidOtherVoiceStatus([strUserID UTF8String], isON);
//}
//
//双人语音接口

//设置接口，可选
//是否使用移动网络,默认不用
- (int)setUseMobileNetworkEnabled:(bool)bEnabled
{
    return YouMeVoiceEngineImp::getInstance ()->setUseMobileNetworkEnabled (bEnabled);
}

//启用/不启用声学回声消除,默认启用
- (int)setAECEnabled:(bool)bEnabled
{
    return YouMeVoiceEngineImp::getInstance ()->setAECEnabled (bEnabled);
}

//启用/不启用背景噪音抑制功能,默认启用
- (int)setANSEnabled:(bool)bEnabled
{
    return YouMeVoiceEngineImp::getInstance ()->setANSEnabled (bEnabled);
}

//启用/不启用自动增益补偿功能,默认启用
- (int)setAGCEnabled:(bool)bEnabled
{
    return YouMeVoiceEngineImp::getInstance ()->setAGCEnabled (bEnabled);
}

//启用/不启用静音检测功能,默认启用
- (int)setVADEnabled:(bool)bEnabled
{
    return YouMeVoiceEngineImp::getInstance ()->setVADEnabled (bEnabled);
}


- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param
{
    [self.delegate onEvent:event errcode:error channel:channel param:param];
}

/*****
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid
{
    [self.delegate onCallEvent:eventType errcode:iErrorCode roomid: roomid];
}
*****/
 
@end

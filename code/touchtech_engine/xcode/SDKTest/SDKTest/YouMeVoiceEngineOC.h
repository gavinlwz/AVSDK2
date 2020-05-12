//
//  YouMeVoiceEngine.h
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "VoiceEngineCallback.h"
@interface YouMeVoiceEngineOC : NSObject
@property (nonatomic, retain) id<VoiceEngineCallback> delegate;

//公共接口
+ (YouMeVoiceEngineOC *)getInstance;
+ (void)destroy;

- (id)init:(id<VoiceEngineCallback>)delegate;

- (int)unInit;


//打开扬声器，默认是打开的
- (int)setSpeakerOn;
//关闭扬声器，从听筒输出声音
- (int)setSpeakerOff;

- (void)setMicrophoneMute:(bool)isOn;



//获取当前音量大小
- (unsigned int)getCurrentVolume;

- (int)playBackgroundMusic:(NSString *)path;
- (int)stopBackgroundMusic;
- (int)setBackgroundMusicVolume:(uint8_t)vol;
- (int)setHeadsetMonitorOn:(bool)isOn;

//多人语音接口
- (int)leaveChannel:(NSString *)strRoomID;
- (int)leaveChannelAll;

- (int)joinChannelSingle:(NSString *)strUserID strChannelID:(NSString *)strChannelID userRole:(int)userRole
              micMute:(bool)micMute needUserList:(bool)needUserList autoSendStatus:(bool)autoSendStatus;
- (int)joinChannelMulti:(NSString *)strUserID strChannelID:(NSString *)strChannelID;
- (int)speakToChannel:(NSString *)strChannelID;

- (int)setOtherMicStatus:(NSString *)strUserID isOn:(bool )isON;
- (int)setOtherSpeakerStatus:(NSString *)strUserID isOn:(bool )isON;
- (int)avoidOtherVoiceStatus:(NSString *)strUserID isOn:(bool )isON;


//双人语音接口

//设置接口，可选
//是否使用移动网络,默认不用
- (int)setUseMobileNetworkEnabled:(bool)bEnabled;

//启用/不启用声学回声消除,默认启用
- (int)setAECEnabled:(bool)bEnabled;

//启用/不启用背景噪音抑制功能,默认启用
- (int)setANSEnabled:(bool)bEnabled;

//启用/不启用自动增益补偿功能,默认启用
- (int)setAGCEnabled:(bool)bEnabled;

//启用/不启用静音检测功能,默认启用
- (int)setVADEnabled:(bool)bEnabled;

//回调接口
- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param;

/*****
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid;
*****/

@end

//
//  SampleHandler.m
//  游密
//
//  Created by liumingxing on 2019/8/20.
//  Copyright © 2019 LB. All rights reserved.
//


#import "SampleHandler.h"
#import "NSObject+SelectorOnMainThreadMutipleObjects.h"
#import "YMVoiceService.h"

#define     strJoinAppKey   @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"
#define     strAppKey       @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"
#define     strAppSecret    @"y1sepDnrmgatu/G8rx1nIKglCclvuA5tAvC0vXwlfZKOvPZfaUYOTkfAdUUtbziW8Z4HrsgpJtmV/RqhacllbXD3abvuXIBlrknqP+Bith9OHazsC1X96b3Inii6J7Und0/KaGf3xEzWx/t1E1SbdrbmBJ01D1mwn50O/9V0820BAAE="

@interface SampleHandler ()<VoiceEngineCallback>

@property (nonatomic, assign) BOOL isYMSDKInitOK;
@property (nonatomic, assign) BOOL isInRoom;

@end

@implementation SampleHandler

- (void)broadcastStartedWithSetupInfo:(NSDictionary<NSString *,NSObject *> *)setupInfo {
    // User has requested to start the broadcast. Setup info from the UI extension can be supplied but optional.
    [self setupYMSDK];
    
//    [self joinChannel:@"752015108" userID:@"user_345"];
}

- (void)broadcastPaused {
    // User has requested to pause the broadcast. Samples will stop being delivered.
}

- (void)broadcastResumed {
    // User has requested to resume the broadcast. Samples delivery will resume.
}

- (void)broadcastFinished {
    // User has requested to finish the broadcast.
    [self leaveChannel];
}

- (void)processSampleBuffer:(CMSampleBufferRef)sampleBuffer withType:(RPSampleBufferType)sampleBufferType {
    
    if (!self.isInRoom) {
        NSLog(@"还没进入房间，不推流");
        return;
    }

    [[YMVoiceService getInstance] inputPixelBufferShare:sampleBufferType withBuffer:sampleBuffer];
}

#pragma mark - SDK接口封装

- (void)leaveChannel {
    NSLog(@"离开房间。");
    [[YMVoiceService getInstance] leaveChannelAll];
}

- (void)joinChannel:(NSString *)channelID userID:(NSString *)userID{
    if (channelID == nil || channelID.length == 0 || [[YMVoiceService getInstance] isInChannel:channelID]) {
        return;
    }
    NSLog(@"------------------------ joinChannel");
    
    // 加入频道前，设置参数
    // 功能描述: 设置视频网络传输过程的分辨率, 第一路高分辨率
    [[YMVoiceService getInstance] setVideoNetResolutionWidth:1080 height:1920];
    // 功能描述: 设置视频数据上行的码率的上下限。最大码率、最小码率，单位kbps
    [[YMVoiceService getInstance] setVideoCodeBitrate:0  minBitrate:0];
    // 功能描述: 设置视频数据是否同意开启硬编硬解
    [[YMVoiceService getInstance] setVideoHardwareCodeEnable:TRUE];
    // 功能描述: 设置视频编码是否采用VBR动态码率方式
    [[YMVoiceService getInstance] setVBR:false];
    // 功能描述:设置是否使用TCP模式来收发数据，针对特殊网络没有UDP端口使用，必须在加入房间之前调用
    [[YMVoiceService getInstance] setTCPMode:false];
    
    [[YMVoiceService getInstance] setVideoFps:30];
    
    //设置共享流分辨率和帧率
    [[YMVoiceService getInstance] setVideoNetResolutionWidthForShare:1080 height:1920];
    [[YMVoiceService getInstance] setVideoFpsForShare:30];
    
    // 设置token
    [[YMVoiceService getInstance] setToken:nil];
    
//    [[YMVoiceService getInstance] setVideoFrameRawCbEnable:true];
    // 加入chanel
    [[YMVoiceService getInstance] joinChannelSingleMode:userID channelID:channelID userRole:YOUME_USER_HOST autoRecv:false];
    
    // 功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
    [[YMVoiceService getInstance] setAutoSendStatus: true];
    
}

- (void)setupYMSDK {
    
    // 设置测试服
    [[YMVoiceService getInstance] setTestServer:false];
    
    // 设置外部音视频模式
    [[YMVoiceService getInstance] setExternalInputMode:true];
//    [[YMVoiceService getInstance] setExternalInputSampleRate:SAMPLE_RATE_44 mixedCallbackSampleRate:SAMPLE_RATE_44];
    
    // 设置log等级
    [[YMVoiceService getInstance] setLogLevelforConsole:LOG_INFO forFile:LOG_INFO];
    
    // 设置音视频质量统计数据的回调频率
    [[YMVoiceService getInstance] setAVStatisticInterval:5000];
    
    // 初始化SDK
    [[YMVoiceService getInstance] initSDK:self
                                   appkey:strAppKey
                                appSecret:strAppSecret
                                 regionId:RTC_DEFAULT_SERVER
                         serverRegionName:@""];
    
    // 设置视频无渲染帧超时等待时间，单位毫秒
    [[YMVoiceService getInstance] setVideoNoFrameTimeout:5000];
    
}

#pragma mark - VoiceEngineCallback - onYouMeEvent

- (void)onYouMeEvent:(YouMeEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid param:(NSString *)param {
    NSLog(@"onYouMeEvent: type:%d, err:%d, room:%@,param:%@", eventType, iErrorCode, roomid, param );
    
    NSDictionary* dictPramEventMap = @{
                                       @(YOUME_EVENT_INIT_OK) : NSStringFromSelector(@selector(youmeEventInitOKWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_INIT_FAILED) : NSStringFromSelector(@selector(youmeEventInitFailedWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_JOIN_OK) : NSStringFromSelector(@selector(youmeEventJoinOKWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_JOIN_FAILED) : NSStringFromSelector(@selector(youmeEventJoinFailedWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_LEAVED_ALL) : NSStringFromSelector(@selector(youmeEventLeavedAllWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_ON) : NSStringFromSelector(@selector(youmeEventOthersVideoOnWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_FAREND_VOICE_LEVEL) : NSStringFromSelector(@selector(youmeEventFarendVoiceLevelOnWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_INPUT_START) : NSStringFromSelector(@selector(youmeEventOthersVideoInputStartOnWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP) : NSStringFromSelector(@selector(youmeEventOthersVideoInputStopOnWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN) : NSStringFromSelector(@selector(youmeEventOthersVideoShutDownWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_RESUMED) :NSStringFromSelector(@selector(youmeEventResume:roomid:param:)),
                                       @(YOUME_EVENT_PAUSED) :NSStringFromSelector(@selector(youmeEventPause:roomid:param:))
                                       };
    
    if ([dictPramEventMap objectForKey:@(eventType)]) {
        [self performSelectorOnMainThread:NSSelectorFromString([dictPramEventMap objectForKey:@(eventType)])
                               withObject:@(iErrorCode)
                               withObject:roomid
                               withObject:param
                            waitUntilDone:TRUE];
    } else {
        dispatch_async (dispatch_get_main_queue (), ^{
            NSString* strTmp = @"Evt: %d, err:%d, param:%@ ,room:%@ ";
            NSString* showInfo = [NSString stringWithFormat: strTmp, eventType, iErrorCode, param, roomid ];
            NSLog(@"%@", showInfo);
        });
    }
    
}

// 会议相关回调逻辑处理
-(void)youmeEventInitOKWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"SDK验证成功");
    // SDK验证成功
    self.isYMSDKInitOK = YES;
    
    //设置服务器区域，在请求进入频道前调用生效
    [[YMVoiceService getInstance] setServerRegion:RTC_DEFAULT_SERVER regionName:@"" bAppend:false];
    
    // 加入房间
    [self joinChannel:@"test001" userID:@"user_889"];
}

-(void)youmeEventInitFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    //SDK验证失败
    NSLog(@"SDK验证失败");
    self.isYMSDKInitOK = NO;
}


-(void)youmeEventJoinOKWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"进入房间成功");
    self.isInRoom = YES;
    
}

-(void)youmeEventJoinFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"加入房间失败 %@",iErrorCodeNum);
    self.isInRoom = NO;
}


-(void)youmeEventLeavedAllWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"退出房间");
    self.isInRoom = NO;
}

-(void)youmeEventOthersVideoOnWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
}

-(void)youmeEventFarendVoiceLevelOnWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    //远端音量回调
}

-(void)youmeEventOthersVideoInputStartOnWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    
}

-(void)youmeEventOthersVideoInputStopOnWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    
}

-(void)youmeEventOthersVideoShutDownWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    
}

-(void)youmeEventResume:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param  {
    //    [m_capture onResume];
}

-(void)youmeEventPause:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    //    [m_capture onPause];
}

#pragma mark - VoceEngineCallback

- (void)onMemberChange:(NSString*)channelID changeList:(NSArray*) changeList isUpdate:(bool) isUpdate {
    
}

@end

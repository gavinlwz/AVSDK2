//
//  ViewController.m
//  YMSDKTest
//
//  Created by liumingxing on 2019/8/9.
//  Copyright © 2019 LB. All rights reserved.
//

#import "ViewController.h"
#import "YMVoiceService.h"
#import <ReplayKit/ReplayKit.h>
#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>
#import <UIKit/UIKit.h>
#import "NSObject+SelectorOnMainThreadMutipleObjects.h"

#define     strJoinAppKey   @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"


#define     strAppKey       @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"
#define     strAppSecret    @"y1sepDnrmgatu/G8rx1nIKglCclvuA5tAvC0vXwlfZKOvPZfaUYOTkfAdUUtbziW8Z4HrsgpJtmV/RqhacllbXD3abvuXIBlrknqP+Bith9OHazsC1X96b3Inii6J7Und0/KaGf3xEzWx/t1E1SbdrbmBJ01D1mwn50O/9V0820BAAE="

@interface ViewController ()<VoiceEngineCallback>

@property (weak, nonatomic) IBOutlet UITextField *channelTextField;
@property (weak, nonatomic) IBOutlet UITextField *userNameTextField;
@property (weak, nonatomic) IBOutlet UIButton *joinChannelBtn;
@property (weak, nonatomic) IBOutlet UIButton *openCaptureBtn;

@property (nonatomic, assign) BOOL isYMSDKInitOK;
@property (nonatomic, assign) BOOL isInRoom;

@property (nonatomic, strong) AVPlayerViewController * avPlayer;

@property (strong, nonatomic) AVPlayer *player;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    self.joinChannelBtn.enabled = NO;
    self.openCaptureBtn.enabled = NO;
    [self setupYMSDK];
    
    {
        NSString *txtPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"dump_app1.pcm"];
        NSFileManager *fileManager = [NSFileManager defaultManager];
        if(![fileManager fileExistsAtPath:txtPath isDirectory:FALSE]){
            [fileManager removeItemAtPath:txtPath error:nil];
        }
    }
    
    {
        NSString *txtPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"dump_app1.pcm"];
        NSFileManager *fileManager = [NSFileManager defaultManager];
        if(![fileManager fileExistsAtPath:txtPath isDirectory:FALSE]){
            [fileManager removeItemAtPath:txtPath error:nil];
        }
    }
}
- (IBAction)openTestVideoBtnClicked:(UIButton *)sender {
    sender.enabled = NO;
    
    [self presentViewController:self.avPlayer animated:YES completion:^{
        sender.enabled = YES;
    }];
    
//    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:nil];
//    [[AVAudioSession sharedInstance] setActive:YES error:nil];
//
//    NSString *audioPath = [[NSBundle mainBundle] pathForResource:@"dukou" ofType:@"mp3"];
//    NSURL *audioUrl = [NSURL fileURLWithPath:audioPath];
//    NSError *playerError;
//    _player = [[AVPlayer alloc] initWithURL:audioUrl];
//    if (_player == NULL)
//    {
//        NSLog(@"fail to play audio :(");
//        return;
//    }
//
//    [_player setVolume:1];
//    [_player play];
    
}

- (IBAction)joinRoomBtnClicked:(UIButton *)sender {
    sender.enabled = NO;
    if ([sender.titleLabel.text isEqualToString:@"进入房间"]) {
        
        if (!self.channelTextField.text.length) {
            sender.enabled = YES;
            return;
        }
        
        if (!self.userNameTextField.text.length) {
            sender.enabled = YES;
            return;
        }
        
        [self joinChannel:self.channelTextField.text userID:self.userNameTextField.text];
    }
    
    if ([sender.titleLabel.text isEqualToString:@"离开房间"]) {
        [self leaveChannel];
    }
}

static inline uint16_t bswap_16(uint16_t x)
{
    return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

- (IBAction)openScreenCaptureBtnClicked:(UIButton *)sender {
    
    
    if (@available(iOS 11.0, *)) {
        sender.enabled = NO;
        
        if ([sender.titleLabel.text isEqualToString:@"开启应用内录屏"]) {
            
            if ([RPScreenRecorder sharedRecorder].isRecording) {
                sender.enabled = YES;
                NSLog(@"isRecording");
                return;
            }
            // 是否开启麦克风
            [[RPScreenRecorder sharedRecorder] setMicrophoneEnabled:YES];
            
            [[RPScreenRecorder sharedRecorder] startCaptureWithHandler:^(CMSampleBufferRef  _Nonnull sampleBuffer, RPSampleBufferType bufferType, NSError * _Nullable error) {
//                NSLog(@"startCaptureWithHandler error = %@", error);
//                NSLog(@"startCaptureWithHandler type = %d", bufferType);

                [[YMVoiceService getInstance] inputPixelBufferShare:bufferType withBuffer:sampleBuffer];
                
            } completionHandler:^(NSError * _Nullable error) {
                NSLog(@"completionHandler error = %@", error);
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (error == nil) {
                        [sender setTitle:@"关闭应用内录屏" forState:UIControlStateNormal];
                        
                        // 开启连接
                        
                    } else {
                        //                        [[LBSREngine sharedInstance] stopEngine];
                    }
                    sender.enabled = YES;
                });
            }];
            
        } else if ([sender.titleLabel.text isEqualToString:@"关闭应用内录屏"]) {
            
            if (![RPScreenRecorder sharedRecorder].isRecording) {
                sender.enabled = YES;
                return;
            }
            
            
            [[RPScreenRecorder sharedRecorder] stopCaptureWithHandler:^(NSError * _Nullable error) {
                NSLog(@"stopCaptureWithHandler error = %@", error);
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (error == nil) {
                        [sender setTitle:@"开启应用内录屏" forState:UIControlStateNormal];
                    }
                    sender.enabled = YES;
                });
            }];
        }
        
    } else {
        [self showAlertWithMessage:@"您的系统低于iOS 11，请升级。"];
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.view endEditing:YES];
}

#pragma mark - private

- (void)showAlertWithMessage:(NSString *)message {
    UIAlertController * alertController = [UIAlertController alertControllerWithTitle:@"温馨提示" message:message preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction * ok = [UIAlertAction actionWithTitle:@"知道了" style:UIAlertActionStyleCancel handler:nil];
    [alertController addAction:ok];
    [self presentViewController:alertController animated:YES completion:nil];
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
    [[YMVoiceService getInstance] setExternalInputSampleRate:SAMPLE_RATE_44 mixedCallbackSampleRate:SAMPLE_RATE_44];
    
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
    
    self.joinChannelBtn.enabled = YES;
    self.openCaptureBtn.enabled = YES;
    
    //设置服务器区域，在请求进入频道前调用生效
    [[YMVoiceService getInstance] setServerRegion:RTC_DEFAULT_SERVER regionName:@"" bAppend:false];
}

-(void)youmeEventInitFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    //SDK验证失败
    NSLog(@"SDK验证失败");
    self.isYMSDKInitOK = NO;
}


-(void)youmeEventJoinOKWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"进入房间成功");
    self.isInRoom = YES;
    [self.joinChannelBtn setTitle:@"离开房间" forState:UIControlStateNormal];
    self.joinChannelBtn.enabled = YES;    
}

-(void)youmeEventJoinFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"加入房间失败 %@",iErrorCodeNum);
    self.isInRoom = NO;
    self.joinChannelBtn.enabled = YES;
}


-(void)youmeEventLeavedAllWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSLog(@"退出房间");
    self.isInRoom = NO;
    [self.joinChannelBtn setTitle:@"进入房间" forState:UIControlStateNormal];
    self.joinChannelBtn.enabled = YES;
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


#pragma mark - getter

- (AVPlayerViewController *)avPlayer{
    if (nil == _avPlayer) {
        //http://player.acfun.cn/route_m3u8?sign=ct5a6e899f0cf23e0adeb11ef8&sid=32987656&uid=ACFUN&ran=10101996123119be22ff3f02932dc329957257bf48bfrh5x12119p137p54p3da03bd0691c194&dt=0
        //        NSURL * videoURL = [NSURL URLWithString:@"https://devstreaming-cdn.apple.com/videos/wwdc/2017/102xyar2647hak3e/102/hls_vod_mvp.m3u8"];
        NSURL * videoURL = [NSURL URLWithString:@"https://devstreaming-cdn.apple.com/videos/wwdc/2017/703muvahj3880222/703/hls_vod_mvp.m3u8"];
        _avPlayer = [[AVPlayerViewController alloc] init];
        _avPlayer.player = [[AVPlayer alloc] initWithURL:videoURL];
        _avPlayer.videoGravity = AVLayerVideoGravityResizeAspect;
    }
    return _avPlayer;
}

@end

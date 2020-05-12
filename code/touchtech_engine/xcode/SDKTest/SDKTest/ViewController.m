//
//  ViewController.m
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import "ViewController.h"
#import "YouMeVoiceEngineOC.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    YouMeVoiceEngineOC *youMeVoiceEngineOC = [[YouMeVoiceEngineOC getInstance] init:self];
    
    [youMeVoiceEngineOC setUseMobileNetworkEnabled:true];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    [[YouMeVoiceEngineOC getInstance] unInit];
    [YouMeVoiceEngineOC destroy];
}

/** 多房间模式 */
- (IBAction)onClickMultiRoomModel:(id)sender {
    
}

- (IBAction)onClickButtonLoginLogout:(id)sender
{
}
- (IBAction)onClickButtonJoinConference:(id)sender
{
    if (self.mBInitOK == FALSE)
    {
        NSLog (@"has not login!");
    }
    else
    {
        if (self.mBInRoom == FALSE)
        {
            self.mBInRoom = TRUE;
            int value = (arc4random() % 1000) + 1;
            NSString* strTmp = @"tester";
            NSString* strValue = [NSString stringWithFormat:@"%d",value];
            NSString* strUserID = [strTmp stringByAppendingString:strValue];
            [[YouMeVoiceEngineOC getInstance] joinChannelSingle:strUserID strChannelID:@"2418" userRole:YOUME_USER_HOST
                                                           micMute:false needUserList:true autoSendStatus:true];
            NSString *buttonTittle = @"离开会议";
            [self.buttonJoinRoom setTitle:buttonTittle forState:UIControlStateNormal];
        }
        else
        {
            self.mBInRoom = FALSE;
            [[YouMeVoiceEngineOC getInstance] leaveChannel:@"2418"];
            NSString *buttonTittle = @"加入会议";
            [self.buttonJoinRoom setTitle:buttonTittle forState:UIControlStateNormal];
        }
    }
}

- (IBAction)onSwitchSpeaker:(id)sender
{
    UISwitch *switchButton = (UISwitch *)sender;
    BOOL isButtonOn = [switchButton isOn];
    if (isButtonOn)
    {
        [[YouMeVoiceEngineOC getInstance] setSpeakerOn];
    }
    else
    {
        [[YouMeVoiceEngineOC getInstance] setSpeakerOff];
    }
}

- (IBAction)onSwitchMute:(id)sender
{
    UISwitch *switchButton = (UISwitch *)sender;
    BOOL isButtonOn = [switchButton isOn];
    if (isButtonOn)
    {
        [[YouMeVoiceEngineOC getInstance] setMicrophoneMute:true];
    }
    else
    {
        [[YouMeVoiceEngineOC getInstance] setMicrophoneMute:false];
    }
}

- (IBAction)onClickButtonPlayBackgroundMusic:(id)sender
{
    NSString *path = [[NSBundle mainBundle] pathForResource:@"kanong" ofType:@"mp3"];
    NSLog(@"Music file path:%@", path);
    [[YouMeVoiceEngineOC getInstance] playBackgroundMusic:path];
}

- (IBAction)onClickButtonStopBackgroundMusic:(id)sender
{
    [[YouMeVoiceEngineOC getInstance] stopBackgroundMusic];
}


- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param
{
    NSLog(@"onEventCallbak event:%d, error:%d, channel:%@, param:%@", event, error, channel, param);
    switch (event)
    {
    case YOUME_EVENT_INIT_OK:
        self.mBInitOK = TRUE;
        break;
 
    case YOUME_EVENT_INIT_NOK:
        self.mBInitOK = FALSE;
        break;

    case YOUME_EVENT_CONNECTED:
        break;
            
    default:
        break;
    }
    dispatch_async (dispatch_get_main_queue (), ^{
      [self.buttonLoginLogout setEnabled:self.mBInitOK];
    });

    //通知主线程刷新
    dispatch_async (dispatch_get_main_queue (), ^{
      [self.buttonJoinRoom setEnabled:self.mBInitOK];
    });
}

/****
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid
{

    switch (eventType)
    {
    case YOUME_CALL_INPROGRESS:

        break;
    case YOUME_CALL_CONNECTED:

        break;
    case YOUME_CALL_TERMWAITING:

        break;
    case YOUME_CALL_TERMINATED:
        break;
    default:
        break;
    }
}

- (void)onCommonStatusEvent:(STATUS_EVENT_TYPE_t)eventType  strUserID:(NSString *)strUserID  iStatus:(int)iStatus
{
    // TODO
}

- (void)onMemberChangeEvent:(NSString *)strUserIDs strRoomID:(NSString *)strRoomID
{
    // TODO
}
****/

- (IBAction)DisableA:(id)sender {
    [[YouMeVoiceEngineOC getInstance] setOtherMicStatus:@"tester1" isOn:false] ;
}

- (IBAction)DisableSpeaker:(id)sender {
    [[YouMeVoiceEngineOC getInstance] setOtherSpeakerStatus:@"tester1" isOn:false] ;
}

- (IBAction)onSliderChangeVol:(id)sender {
    UISlider *slider = (UISlider *)sender;
    int vol = (int)(slider.value * 100.0);
    NSString *str = [[NSString alloc] initWithFormat:@"音乐音量:%d", vol];
    _tCurVol.text = str;
}

- (IBAction)onSliderChangeVolSet:(id)sender {
    UISlider *slider = (UISlider *)sender;
    int vol = (int)(slider.value * 100.0);
    [[YouMeVoiceEngineOC getInstance] setBackgroundMusicVolume:(uint8_t)vol];
}

- (IBAction)onSwitchMicBypassToSpeaker:(id)sender
{
    UISwitch *switchButton = (UISwitch *)sender;
    BOOL isButtonOn = [switchButton isOn];
    if (isButtonOn)
    {
        [[YouMeVoiceEngineOC getInstance] setHeadsetMonitorOn:true];
    }
    else
    {
        [[YouMeVoiceEngineOC getInstance] setHeadsetMonitorOn:false];
    }
}

- (IBAction)onSegmentMonitorMode:(id)sender {
    switch ([sender selectedSegmentIndex])
    {
        case 0:
            [[YouMeVoiceEngineOC getInstance] setHeadsetMonitorOn:false];
            break;
        case 1:
            [[YouMeVoiceEngineOC getInstance] setHeadsetMonitorOn:true];
            break;
        default:
            [[YouMeVoiceEngineOC getInstance] setHeadsetMonitorOn:false];
            break;
    }
}

@end

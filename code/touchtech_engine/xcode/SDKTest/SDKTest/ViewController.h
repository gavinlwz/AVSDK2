//
//  ViewController.h
//  SDKTest
//
//  Created by wanglei on 15/9/10.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "VoiceEngineCallback.h"

@interface ViewController : UIViewController <VoiceEngineCallback>

@property (atomic, assign) BOOL mBInitOK;
@property (atomic, assign) BOOL mBInRoom;

- (IBAction)onClickButtonLoginLogout:(id)sender;

- (IBAction)onClickButtonJoinConference:(id)sender;
- (IBAction)onSwitchSpeaker:(id)sender;
- (IBAction)onSwitchMute:(id)sender;

- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param;

/*****
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid;
- (void)onCommonStatusEvent:(STATUS_EVENT_TYPE_t)eventType  strUserID:(NSString *)strUserID  iStatus:(int)iStatus;
- (void)onMemberChangeEvent:(NSString *)strUserIDs strRoomID:(NSString *)strRoomID;
****/

@property (weak, nonatomic) IBOutlet UIButton *buttonLoginLogout;
@property (weak, nonatomic) IBOutlet UIButton *buttonJoinRoom;
@property (weak, nonatomic) IBOutlet UISwitch *switchSpeaker;
@property (weak, nonatomic) IBOutlet UISwitch *switchMute;
@property (weak, nonatomic) IBOutlet UILabel  *tCurVol;
@property (weak, nonatomic) IBOutlet UILabel  *tCurDelay;

- (IBAction)DisableA:(id)sender;
- (IBAction)DisableSpeaker:(id)sender;
- (IBAction)onSliderChangeVol:(id)sender;
- (IBAction)onSliderChangeVolSet:(id)sender;

@end

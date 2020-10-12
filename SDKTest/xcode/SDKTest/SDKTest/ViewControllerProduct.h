//
//  ViewControllerProduct.h
//  SDKTest
//
//  Created by fire on 2016/11/21.
//  Copyright © 2016年 tencent. All rights reserved.
//

#ifndef ViewControllerProduct_h
#define ViewControllerProduct_h

#import <UIKit/UIKit.h>
#import "VoiceEngineCallback.h"

@class RadioButton;

@interface ViewControllerProduct : UIViewController <VoiceEngineCallback>


- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param;

/***
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid;
- (void)onCommonStatusEvent:(STATUS_EVENT_TYPE_t)eventType  strUserID:(NSString *)strUserID  iStatus:(int)iStatus;
- (void)onMemberChangeEvent:(NSString *)strUserIDs strRoomID:(NSString *)strRoomID;
***/



- (IBAction)OnClickRoomUniversity:(id)sender;
- (IBAction)OnClickRoomWorld:(id)sender;
- (IBAction)OnClickRoomCountry:(id)sender;
- (IBAction)OnClickRoomFaction:(id)sender;
- (IBAction)OnClickRoomTeam:(id)sender;
- (IBAction)OnClickRoomQuit:(id)sender;
- (IBAction)OnClickUserIDRefush:(id)sender;


@property (weak, nonatomic) IBOutlet UILabel *universityMsg;
@property (weak, nonatomic) IBOutlet UILabel *universitySpeak;
@property (weak, nonatomic) IBOutlet UILabel *worldMsg;
@property (weak, nonatomic) IBOutlet UILabel *worldSpeak;
@property (weak, nonatomic) IBOutlet UILabel *countryMsg;
@property (weak, nonatomic) IBOutlet UILabel *countrySpeak;
@property (weak, nonatomic) IBOutlet UILabel *factionMsg;
@property (weak, nonatomic) IBOutlet UILabel *factionSpeak;
@property (weak, nonatomic) IBOutlet UILabel *teamMsg;
@property (weak, nonatomic) IBOutlet UILabel *teamSpeak;
@property (weak, nonatomic) IBOutlet UITextField *commonMsg;
@property (weak, nonatomic) IBOutlet UITextField *userID;
@property (weak, nonatomic) IBOutlet UIButton *btnUniversity;
@property (weak, nonatomic) IBOutlet UIButton *btnWorld;
@property (weak, nonatomic) IBOutlet UIButton *btnCountry;
@property (weak, nonatomic) IBOutlet UIButton *btnFaction;
@property (weak, nonatomic) IBOutlet UIButton *btnTeam;

@property (nonatomic, strong) IBOutlet RadioButton* radioButton;
-(IBAction)onRadioBtn:(id)sender;


@end

#endif /* ViewControllerProduct_h */

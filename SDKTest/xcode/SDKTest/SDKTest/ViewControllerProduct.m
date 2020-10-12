//
//  ViewControllerProduct.m
//  SDKTest
//
//  Created by fire on 2016/11/21.
//  Copyright © 2016年 tencent. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ViewControllerProduct.h"
#import "YouMeVoiceEngineOC.h"
#import "RadioButton.h"


@interface ViewControllerProduct ()

@end

@implementation ViewControllerProduct


NSString * ROOM_UNIVERSITY  = @"university";
NSString * ROOM_WORLD       = @"world";
NSString * ROOM_COUNTRY     = @"country";
NSString * ROOM_FACTION     = @"faction";
NSString * ROOM_TEAM        = @"team";

NSString * STATE_CLICKIN    = @"点击进入";
NSString * STATE_IN         = @"已进入";
NSString * STATE_OUT        = @"已退出";

NSString * ROLE_LISTEN      = @"听众模式";
NSString * ROLE_COMMAND     = @"指挥官模式";
NSString * ROLE_HOST        = @"主播模式";

NSString * SPEAK_NONE       = @"-";
NSString * SPEAK_IN         = @"通话中";
NSString * SPEAK_AUTH       = @"无权限";

int  universityState        = YOUME_EVENT_TERMINATED;
int  worldState             = YOUME_EVENT_TERMINATED;
int  countryState           = YOUME_EVENT_TERMINATED;
int  factionState           = YOUME_EVENT_TERMINATED;
int  teamState              = YOUME_EVENT_TERMINATED;

bool isFirstIn              = true;
int  roomNumber             = 0;
int  currUserRole           = 0;
NSString * currSpeakRoom    = @"";

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[YouMeVoiceEngineOC getInstance] setDelegate:self]; // 设置回调

    self.userID.enabled = false;
    self.commonMsg.enabled = false;
    
    [self GenUserID];
   
    [self.universityMsg setText:STATE_CLICKIN];
    [self.worldMsg setText:STATE_CLICKIN];
    [self.countryMsg setText:STATE_CLICKIN];
    [self.factionMsg setText:STATE_CLICKIN];
    [self.teamMsg setText:STATE_CLICKIN];
    
    [self.universitySpeak setText:SPEAK_NONE];
    [self.worldSpeak setText:SPEAK_NONE];
    [self.countrySpeak setText:SPEAK_NONE];
    [self.factionSpeak setText:SPEAK_NONE];
    [self.teamSpeak setText:SPEAK_NONE];
    
    
    UILongPressGestureRecognizer *universityLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(btnUniversityLongPress:)];
    universityLongPress.minimumPressDuration = 1;
    [_btnUniversity addGestureRecognizer:universityLongPress];
    
    
    UILongPressGestureRecognizer *worldLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(btnWorldLongPress:)];
    worldLongPress.minimumPressDuration = 1;
    [_btnWorld addGestureRecognizer:worldLongPress];
    
    UILongPressGestureRecognizer *countryLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(btnCountryLongPress:)];
    countryLongPress.minimumPressDuration = 1;
    [_btnCountry addGestureRecognizer:countryLongPress];
    
    UILongPressGestureRecognizer *factionLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(btnFactionLongPress:)];
    factionLongPress.minimumPressDuration = 1;
    [_btnFaction addGestureRecognizer:factionLongPress];
    
    UILongPressGestureRecognizer *teamLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(btnTeamLongPress:)];
    teamLongPress.minimumPressDuration = 1;
    [_btnTeam addGestureRecognizer:teamLongPress];
}


- (IBAction)OnClickRoomUniversity:(id)sender {
    [self doClickButton:ROOM_UNIVERSITY];
}

-(void)btnUniversityLongPress:(UILongPressGestureRecognizer *)gestureRecognizer{
    if ([gestureRecognizer state] == UIGestureRecognizerStateBegan) {
        [self doLongPressButton:ROOM_UNIVERSITY];
    }
}

- (IBAction)OnClickRoomWorld:(id)sender {
    [self doClickButton:ROOM_WORLD];
}

-(void)btnWorldLongPress:(UILongPressGestureRecognizer *)gestureRecognizer{
    if ([gestureRecognizer state] == UIGestureRecognizerStateBegan) {
        [self doLongPressButton:ROOM_WORLD];
    }
}

- (IBAction)OnClickRoomCountry:(id)sender {
    [self doClickButton:ROOM_COUNTRY];
}

-(void)btnCountryLongPress:(UILongPressGestureRecognizer *)gestureRecognizer{
    if ([gestureRecognizer state] == UIGestureRecognizerStateBegan) {
        [self doLongPressButton:ROOM_COUNTRY];
    }
}

- (IBAction)OnClickRoomFaction:(id)sender {
    [self doClickButton:ROOM_FACTION];
}

-(void)btnFactionLongPress:(UILongPressGestureRecognizer *)gestureRecognizer{
    if ([gestureRecognizer state] == UIGestureRecognizerStateBegan) {
        [self doLongPressButton:ROOM_FACTION];
    }
}

- (IBAction)OnClickRoomTeam:(id)sender {
    [self doClickButton:ROOM_TEAM];
}

-(void)btnTeamLongPress:(UILongPressGestureRecognizer *)gestureRecognizer{
    if ([gestureRecognizer state] == UIGestureRecognizerStateBegan) {
        [self doLongPressButton:ROOM_TEAM];
    }
}

- (IBAction)OnClickRoomQuit:(id)sender {
    NSLog (@"OnClickRoomQuit, leave all conference");
    
    int *state = NULL;
    state = [self getRoomState:ROOM_UNIVERSITY];
    if (*state != YOUME_EVENT_TERMINATED) {
        [[YouMeVoiceEngineOC getInstance] leaveChannel:ROOM_UNIVERSITY];
    }
    state = [self getRoomState:ROOM_WORLD];
    if (*state != YOUME_EVENT_TERMINATED) {
        [[YouMeVoiceEngineOC getInstance] leaveChannel:ROOM_WORLD];
    }
    state = [self getRoomState:ROOM_COUNTRY];
    if (*state != YOUME_EVENT_TERMINATED) {
        [[YouMeVoiceEngineOC getInstance] leaveChannel:ROOM_COUNTRY];
    }
    state = [self getRoomState:ROOM_FACTION];
    if (*state != YOUME_EVENT_TERMINATED) {
        [[YouMeVoiceEngineOC getInstance] leaveChannel:ROOM_FACTION];
    }
    state = [self getRoomState:ROOM_TEAM];
    if (*state != YOUME_EVENT_TERMINATED) {
        [[YouMeVoiceEngineOC getInstance] leaveChannel:ROOM_TEAM];
    }
    
    [self dismissViewControllerAnimated:YES completion:^{
        
    }];
}

- (IBAction)OnClickUserIDRefush:(id)sender {
    [self GenUserID];
    NSLog (@"create userid: %@", self.userID.text);
}

- (void)doClickButton: (NSString *)room
{
    int *state = [self getRoomState:room];
    
    if (state == NULL) {
        NSLog(@"Wrong roomid : %@", room);
        return;
    }
    
    switch (*state) {
        case YOUME_EVENT_CONNECTED:
            [[YouMeVoiceEngineOC getInstance] leaveChannel:room];
            break;
            
        case YOUME_EVENT_TERMINATED:
            [[YouMeVoiceEngineOC getInstance] joinChannelMulti:self.userID.text strChannelID:room];
            break;
            
        default:
            NSLog(@"当前状态不对，请等待房间连接或者结束！room: %@ state: %d", room, *state);
            break;
    }
}


- (void)doLongPressButton: (NSString *)room
{
    int *state = [self getRoomState:room];
    
    if (state == NULL) {
        NSLog(@"Wrong roomid : %@", room);
        return;
    }
    
    if (*state != YOUME_EVENT_CONNECTED) {
        NSLog(@"Warning 请先加入房间 : %@", room);
        return;
    }
    
    if ( [currSpeakRoom compare:room] == NSOrderedSame) {
        NSLog(@"Warning 当前正在通话中 : %@", room);
        return;
    }
    
    [[YouMeVoiceEngineOC getInstance] speakToChannel:room];
}

- (void) GenUserID
{
    int rand = 10000000 + arc4random() % 1000000;
    NSString * userID = [@"YM_" stringByAppendingFormat:@"%d", rand];
    [self.userID setText:userID];
}

- (int *) getRoomState: (NSString *) roomid
{
    int * state = NULL;
    
    if ([roomid compare:ROOM_UNIVERSITY] == NSOrderedSame) {
        state = &universityState;
    }
    if ([roomid compare:ROOM_WORLD] == NSOrderedSame) {
        state = &worldState;
    }
    if ([roomid compare:ROOM_COUNTRY] == NSOrderedSame) {
        state = &countryState;
    }
    if ([roomid compare:ROOM_FACTION] == NSOrderedSame) {
        state = &factionState;
    }
    if ([roomid compare:ROOM_TEAM] == NSOrderedSame) {
        state = &teamState;
    }
    
    return state;
}

- (UILabel *)getRoomMsg: (NSString *) roomid
{
    UILabel * msg = NULL;
    
    if ([roomid compare:ROOM_UNIVERSITY] == NSOrderedSame) {
        msg = _universityMsg;
    }
    if ([roomid compare:ROOM_WORLD] == NSOrderedSame) {
        msg = _worldMsg;
    }
    if ([roomid compare:ROOM_COUNTRY] == NSOrderedSame) {
        msg = _countryMsg;
    }
    if ([roomid compare:ROOM_FACTION] == NSOrderedSame) {
        msg = _factionMsg;
    }
    if ([roomid compare:ROOM_TEAM] == NSOrderedSame) {
        msg = _teamMsg;
    }
    
    return msg;
}

- (UILabel *)getRoomSpeak: (NSString *) roomid
{
    UILabel * speak = NULL;
    
    if ([roomid compare:ROOM_UNIVERSITY] == NSOrderedSame) {
        speak = _universitySpeak;
    }
    if ([roomid compare:ROOM_WORLD] == NSOrderedSame) {
        speak = _worldSpeak;
    }
    if ([roomid compare:ROOM_COUNTRY] == NSOrderedSame) {
        speak = _countrySpeak;
    }
    if ([roomid compare:ROOM_FACTION] == NSOrderedSame) {
        speak = _factionSpeak;
    }
    if ([roomid compare:ROOM_TEAM] == NSOrderedSame) {
        speak = _teamSpeak;
    }
    
    return speak;
}

- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param
{
    NSLog(@"product onEvent %d, %d, %@, %@", event, error, channel, param);
    
    int *state = [self getRoomState:channel];
    UILabel * msg = [self getRoomMsg:channel];
    UILabel * speak = [self getRoomSpeak:channel];
    
    if (event != YOUME_EVENT_INIT_OK || event != YOUME_EVENT_INIT_NOK)
    {
        if (state == NULL) {
            NSLog(@"Wrong roomid : %@", channel);
            return;
        }
    
        *state = event;
    }
    
    switch (event) {
        case YOUME_EVENT_INIT_OK:
            break;
        case YOUME_EVENT_INIT_NOK:
            break;
        case YOUME_EVENT_CONNECTED:
        {
            NSLog(@"product 进入房间%@成功！", channel);
            dispatch_async (dispatch_get_main_queue (), ^{
                [msg setText:STATE_IN];
            });
            break;
        }
        case YOUME_EVENT_TERMINATED:
        {
            NSLog(@"product 离开房间%@成功！", channel);
            dispatch_async (dispatch_get_main_queue (), ^{
                [msg setText:STATE_OUT];
            });
            break;
        }
        case YOUME_EVENT_CONNECT_FAILED:
            break;
        case YOUME_EVENT_REC_PERMISSION_STATUS:
            break;
        case YOUME_EVENT_BGM_STOPPED:
            break;
        case YOUME_EVENT_BGM_FAILED:
            break;
        case YOUME_EVENT_PAUSED:
            break;
        case YOUME_EVENT_PAUSE_FAILED:
            break;
        case YOUME_EVENT_RESUMED:
            break;
        case YOUME_EVENT_RESUME_FAILED:
            break;
        case YOUME_EVENT_SPEAK_SUCCESS:
        {
            NSLog(@"product 切换房间%@成功！", channel);
            dispatch_async (dispatch_get_main_queue (), ^{
                [speak setText:SPEAK_IN];
            });
            currSpeakRoom = channel;
            break;
        }
        case YOUME_EVENT_SPEAK_FAILED:
        {
            NSLog(@"product 切换房间%@成功！", channel);
            dispatch_async (dispatch_get_main_queue (), ^{
                [speak setText:SPEAK_NONE];
            });
            break;
        }
            
        case YOUME_EVENT_MEMBER_CHANGE:
            break;
            
        case YOUME_EVENT_OTHERS_MIC_ON:
            break;
            
        case YOUME_EVENT_OTHERS_MIC_OFF:
            break;
        
        case YOUME_EVENT_OTHERS_SPEAKER_ON:
            break;
            
        case YOUME_EVENT_OTHERS_SPEAKER_OFF:
            break;
        
        default:
            break;
    }
}


/****
- (void)onCallEvent:(YouMeEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid
{
    NSLog(@"product onCallEvent %d, %d, %@", eventType, iErrorCode, roomid);
    
    int *state = [self getRoomState:roomid];
    
    if (state == NULL) {
        NSLog(@"Wrong roomid : %@", roomid);
        return;
    }
    
    *state = eventType;
    
    UILabel * msg = [self getRoomMsg:roomid];
    UILabel * speak = [self getRoomSpeak:roomid];
    
    switch (eventType) {
        case YOUME_EVENT_INCOMING:
            break;
        case YOUME_EVENT_INPROGRESS:
            break;
        case YOUME_EVENT_RINGING:
            break;
        case YOUME_EVENT_CONNECTED:
        {
            NSLog(@"product 进入房间%@成功！", roomid);
            dispatch_async (dispatch_get_main_queue (), ^{
                [msg setText:STATE_IN];
            });
            break;
        }
        case YOUME_EVENT_TERMWAITING:
            break;
        case YOUME_EVENT_TERMINATED:
        {
            NSLog(@"product 离开房间%@成功！", roomid);
            dispatch_async (dispatch_get_main_queue (), ^{
                [msg setText:STATE_OUT];
            });
            break;
        }
        case YOUME_EVENT_FAILED:
            break;
        case YOUME_EVENT_REC_PERMISSION_STATUS:
            break;
        case YOUME_EVENT_BGM_STOPPED:
            break;
        case YOUME_EVENT_BGM_FAILED:
            break;
        case YOUME_EVENT_PAUSE:
            break;
        case YOUME_EVENT_RESUME:
            break;
        case YOUME_EVENT_SPEAK_SUCCESS:
        {
            NSLog(@"product 切换房间%@成功！", roomid);
            dispatch_async (dispatch_get_main_queue (), ^{
                [speak setText:SPEAK_IN];
            });
            currSpeakRoom = roomid;
            break;
        }
        case YOUME_EVENT_SPEAK_FAILED:
        {
            NSLog(@"product 切换房间%@成功！", roomid);
            dispatch_async (dispatch_get_main_queue (), ^{
                [speak setText:SPEAK_NONE];
            });
            break;
        }
        default:
            break;
    }
}

- (void)onCommonStatusEvent:(STATUS_EVENT_TYPE_t)eventType  strUserID:(NSString *)strUserID  iStatus:(int)iStatus
{
    NSLog(@"product onCommonStatusEvent %d, %@, %d", eventType, strUserID, iStatus);
    switch (eventType) {
        case MIC_STATUS:
            break;
        case SPEAKER_STATUS:
            break;
        case MIC_CTR_STATUS:
            break;
        case SPEAKER_CTR_STATUS:
            break;
        case AVOID_STATUS:
            break;
        default:
            break;
    }
    
}

- (void)onMemberChangeEvent:(NSString *)strUserIDs strRoomID:(NSString *)strRoomID
{
    NSLog (@"product onMemberChangeEvent users: %@, room: %@", strUserIDs, strRoomID);
}

****/

-(IBAction)onRadioBtn:(RadioButton *)sender
{
    NSLog (@"Selected: %@", sender.titleLabel.text);
    
    if ([sender.titleLabel.text compare:ROLE_LISTEN] == NSOrderedSame) {
        currUserRole = 0;
    } else if ([sender.titleLabel.text compare:ROLE_COMMAND] == NSOrderedSame) {
        currUserRole = 1;
    } else if ([sender.titleLabel.text compare:ROLE_HOST] == NSOrderedSame) {
        currUserRole = 2;
    } else {
        currUserRole = 0;
    }
    
}


@end


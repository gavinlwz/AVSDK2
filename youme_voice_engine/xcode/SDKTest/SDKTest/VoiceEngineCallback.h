//
//  VoiceEngineCommonCallback.h
//  SDKTest
//
//  Created by wanglei on 15/9/11.
//  Copyright (c) 2015年 tencent. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "YouMeConstDefine.h"

@protocol VoiceEngineCallback <NSObject>

- (void)onEvent:(YouMeEvent_t)event errcode:(YouMeErrorCode_t)error channel:(NSString *)channel param:(NSString *)param;

/******
- (void)onCallEvent:(YouMeCallEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid: (NSString *)roomid;
- (void)onCommonStatusEvent:(STATUS_EVENT_TYPE_t)eventType  strUserID:(NSString *)strUserID  iStatus:(int)iStatus;
- (void)onMemberChangeEvent:(NSString *)strUserIDs strRoomID:(NSString *)strRoomID;
*****/
 
@end

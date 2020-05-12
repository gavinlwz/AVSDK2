//
//  ParamSetting.h
//  YmTalkTest
//
//  Created by zalejiang on 2017/11/24.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ParamSetting : NSObject
{
@public
    int videoNetWidth;
    int videoNetHeight;
    int videoLocalWidth;
    int videoLocalHeight;
    int videoMinorWidth;
    int videoMinorHeight;
    int videoShareWidth;
    int videoShareHeight;
    int reportInterval;
    int maxBitrate;
    int minBitrate;
    int farendLevel;
    int fps;
    bool bHWEnable;
    bool bHighAudio;
    bool beauty;
}


+ (ParamSetting *)sharedInstance;
+(instancetype) alloc __attribute__((unavailable("call sharedInstance instead")));
+(instancetype) new __attribute__((unavailable("call sharedInstance instead")));
-(instancetype) copy __attribute__((unavailable("call sharedInstance instead")));
-(instancetype) mutableCopy __attribute__((unavailable("call sharedInstance instead")));

@end


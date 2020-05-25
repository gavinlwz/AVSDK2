//
//  ParamSetting.m
//  YmTalkTest
//
//  Created by zalejiang on 2017/11/24.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import "ParamSetting.h"

@implementation ParamSetting


+ (ParamSetting *)sharedInstance{
    static ParamSetting *theSharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        theSharedInstance = [[super alloc] init];
        theSharedInstance->videoNetWidth = 640;
        theSharedInstance->videoNetHeight = 480;
        theSharedInstance->videoLocalWidth = 640;
        theSharedInstance->videoLocalHeight = 480;
        theSharedInstance->videoMinorWidth = 0;
        theSharedInstance->videoMinorHeight = 0;
        theSharedInstance->videoShareWidth = 1280;
        theSharedInstance->videoShareHeight = 720;
        
        theSharedInstance->reportInterval = 5000;
        theSharedInstance->maxBitrate = 0;
        theSharedInstance->minBitrate = 0;
        theSharedInstance->farendLevel = 10;
        theSharedInstance->bHWEnable = true;
        theSharedInstance->bHighAudio = false;
        theSharedInstance->fps = 15;
        theSharedInstance->beauty = true;
    });
    return theSharedInstance;
}


@end

//
//  ParamViewController.h
//  YmVideoMacRef
//
//  Created by youmi on 2018/6/20.
//  Copyright © 2018年 youme. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ViewController.h"

@interface ParamMacViewController : NSViewController
{
@public
    ParamSetting* params;
    BOOL bInited;
}

@property (retain, nonatomic) IBOutlet NSTextField *tf_local_width;
@property (retain, nonatomic) IBOutlet NSTextField *tf_local_height;
@property (retain, nonatomic) IBOutlet NSTextField *tf_net_width;
@property (retain, nonatomic) IBOutlet NSTextField *tf_net_height;

@property (retain, nonatomic) IBOutlet NSTextField *tf_minor_width;
@property (retain, nonatomic) IBOutlet NSTextField *tf_minor_height;

@property (retain, nonatomic) IBOutlet NSTextField *tf_share_width;
@property (retain, nonatomic) IBOutlet NSTextField *tf_share_height;

@property (retain, nonatomic) IBOutlet NSTextField *tf_maxBitrate;
@property (retain, nonatomic) IBOutlet NSTextField *tf_minBitrate;
@property (retain, nonatomic) IBOutlet NSTextField *tf_interval;
@property (retain, nonatomic) IBOutlet NSTextField *tf_farendLevel;
@property (retain, nonatomic) IBOutlet NSButton *tf_high_audio;
@property (retain, nonatomic) IBOutlet NSButton *tf_open_hw;
@property (retain, nonatomic) IBOutlet NSTextField *tf_video_fps;
@property (retain, nonatomic) IBOutlet NSButton *tf_open_beauty;

@end

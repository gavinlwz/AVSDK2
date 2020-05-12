//
//  ParamViewController.m
//  YmVideoMacRef
//
//  Created by youmi on 2018/6/20.
//  Copyright © 2018年 youme. All rights reserved.
//

#import "ParamMacViewController.h"

@implementation ParamMacViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    //    @property (retain, nonatomic) IBOutlet UITextField *tf_width;
    //    @property (retain, nonatomic) IBOutlet UITextField *tf_height;
    //    @property (retain, nonatomic) IBOutlet UITextField *tf_interval;
    //    @property (retain, nonatomic) IBOutlet UITextField *tf_bitrate;
    //
    //    @property (retain, nonatomic) IBOutlet UISwitch *tf_open_hw;
    //    @property (retain, nonatomic) IBOutlet UISwitch *tf_high_audio;
    params = [ParamSetting sharedInstance];
    _tf_local_width.stringValue = [NSString stringWithFormat:@"%d", params->videoLocalWidth];
    _tf_local_height.stringValue = [NSString stringWithFormat:@"%d", params->videoLocalHeight];
    _tf_net_width.stringValue = [NSString stringWithFormat:@"%d", params->videoNetWidth];
    _tf_net_height.stringValue = [NSString stringWithFormat:@"%d", params->videoNetHeight];
    
    _tf_minor_width.stringValue = [NSString stringWithFormat:@"%d", params->videoMinorWidth];
    _tf_minor_height.stringValue = [NSString stringWithFormat:@"%d", params->videoMinorHeight];
    _tf_share_width.stringValue = [NSString stringWithFormat:@"%d", params->videoShareWidth];
    _tf_share_height.stringValue = [NSString stringWithFormat:@"%d", params->videoShareHeight];
    
    _tf_interval.stringValue = [NSString stringWithFormat:@"%d", params->reportInterval];
    _tf_maxBitrate.stringValue = [NSString stringWithFormat:@"%d", params->maxBitrate];
    _tf_minBitrate.stringValue = [NSString stringWithFormat:@"%d", params->minBitrate];
    _tf_farendLevel.stringValue = [NSString stringWithFormat:@"%d", params->farendLevel];
    _tf_video_fps.stringValue = [NSString stringWithFormat:@"%d", params->fps];
    
    _tf_open_hw.state = params->bHWEnable ? NSOnState : NSOffState;
    _tf_high_audio.state = params->bHighAudio ? NSOnState : NSOffState;
    _tf_open_beauty.state = params->beauty ? NSOnState : NSOffState;
    
    if( bInited )
    {
        _tf_local_width.enabled = false ;
        _tf_local_height.enabled = false ;
        
        _tf_net_width.enabled = false ;
        _tf_net_height.enabled = false ;
        
        _tf_open_hw.enabled = false ;
        _tf_high_audio.enabled = false ;
        
    }
}

- (IBAction)onClickButtonConfirm:(id)sender {
    if( !bInited ){
        params->videoLocalWidth = _tf_local_width.stringValue.intValue;
        params->videoLocalHeight = _tf_local_height.stringValue.intValue;
        params->videoNetWidth = _tf_net_width.stringValue.intValue;
        params->videoNetHeight = _tf_net_height.stringValue.intValue;
        
        params->videoMinorWidth = _tf_minor_width.stringValue.intValue;
        params->videoMinorHeight = _tf_minor_height.stringValue.intValue;
        params->videoShareWidth = _tf_share_width.stringValue.intValue;
        params->videoShareHeight = _tf_share_height.stringValue.intValue;
        
        params->reportInterval = _tf_interval.stringValue.intValue;
        params->maxBitrate = _tf_maxBitrate.stringValue.intValue;
        params->minBitrate = _tf_minBitrate.stringValue.intValue;
        params->farendLevel = _tf_farendLevel.stringValue.intValue;
        params->fps = _tf_video_fps.stringValue.intValue;
        
        params->bHWEnable  = _tf_open_hw.state == NSOnState;
        params->bHighAudio = _tf_high_audio.state == NSOnState;
        params->beauty     = _tf_open_beauty.state == NSOnState;
    }
    [self dismissController:self];
}

@end

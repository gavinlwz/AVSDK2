//
//  ViewController.h
//  YmVideoMacRef
//
//  Created by pinky on 2018/5/10.
//  Copyright © 2018年 youme. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "VoiceEngineCallback.h"
#import "ParamSetting.h"

@interface ViewController : NSViewController<VoiceEngineCallback, NSComboBoxDelegate>

@property (weak, nonatomic) IBOutlet NSTextField *tfChannelID;
@property (weak, nonatomic) IBOutlet NSTextField *tfUserID;

@property (weak, nonatomic) IBOutlet NSTextField *labelVer;
@property (weak, nonatomic) IBOutlet NSTextField *labelServer;

@property (weak, nonatomic) IBOutlet NSButton *buttonTcp;
@property (weak, nonatomic) IBOutlet NSButton *buttonEnter;
@property (weak, nonatomic) IBOutlet NSButton *buttonOpenParam;
@property (weak) IBOutlet NSButton *buttonTestmode;

///Audio
@property (weak, nonatomic) IBOutlet NSButton *buttonMic;
@property (weak, nonatomic) IBOutlet NSButton *buttonSpeak;
@property (weak, nonatomic) IBOutlet NSTextField * labelVolume;
@property (weak, nonatomic) IBOutlet NSSlider *sliderVolume;

@property (weak, nonatomic) IBOutlet NSButton *buttonMonitorMicphone;
@property (weak, nonatomic) IBOutlet NSButton *buttonMonitorBg;

@property (weak, nonatomic) IBOutlet NSButton *buttonPlayMusic;
@property (weak, nonatomic) IBOutlet NSButton *buttonPauseMusic;
@property (weak, nonatomic) IBOutlet NSTextField * labelBgVolume;
@property (weak, nonatomic) IBOutlet NSSlider *sliderBgVolume;
@property (weak, nonatomic) IBOutlet NSTextField *tfTips;

@property (weak) IBOutlet NSComboBox *comboBoxSetMic;

@property (weak) IBOutlet NSComboBox *comboBoxSetWinList;


///Video

@property (weak) IBOutlet NSButton *buttonOpenCamera;
@property (weak) IBOutlet NSComboBox *comboBoxSetCamera;
@property (weak) IBOutlet NSButton *buttonOpenShare;
@property (weak) IBOutlet NSButton *buttonOpenWindowShare;
@property (weak) IBOutlet NSButton *buttonOpenSaveShare;
@property (weak) IBOutlet NSButton *buttonSwitchStream;

@property (weak) IBOutlet NSButton *fps3Button;
@property (weak) IBOutlet NSButton *fps5Button;
@property (weak) IBOutlet NSButton *fps10Button;
@property (weak) IBOutlet NSButton *fps15Button;
@property (weak) IBOutlet NSButton *fps20Button;
@property (weak) IBOutlet NSButton *fps30Button;
@property (weak) IBOutlet NSButton *fps60Button;
@property (weak) IBOutlet NSTextField *fpsLabel;
@property (weak) IBOutlet NSTextField *minBitrateText;
@property (weak) IBOutlet NSTextField *maxBitrateText;
@property (weak) IBOutlet NSButton *smoothButton;


- (IBAction)onClickButtonUDPMode:(id)sender;
- (IBAction)onClickButtonEnterRoom:(id)sender;

///Audio
- (IBAction)onClickButtonOpenMic:(id)sender;
- (IBAction)onClickButtonOpenSpeaker:(id)sender;

- (IBAction)onClickButtonMonitorMic:(id)sender;
- (IBAction)onClickButtonMonitorBgMusic:(id)sender;

- (IBAction)onClickButtonBackgroundMusic:(id)sender;
- (IBAction)onClickButtonPauseMusic:(id)sender;

- (IBAction)sliderActionVolume:(id)sender;
- (IBAction)sliderActionBgVolume:(id)sender;

- (IBAction)onClickButtonRefreshMics:(id)sender;

- (void) updateButtonState;




///Video
- (IBAction)onClickButtonOpenCamera:(id)sender;
- (IBAction)onClickButtonOpenShare:(id)sender;

- (IBAction)onClickButtonOpenSaveShare:(id)sender;
- (IBAction)onClickButtonSwitchStream:(id)sender;
- (IBAction)onClickButtonPreviewMirror:(id)sender;

- (void)onYouMeEvent:(YouMeEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid param:(NSString *)param;


- (void)onVideoFrameCallback: (NSString*)userId data:(void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp;
- (void)onVideoFrameMixedCallback: (void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp;
- (void)onVideoFrameCallbackForGLES:(NSString*)userId  pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp;
- (void)onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp;


@end


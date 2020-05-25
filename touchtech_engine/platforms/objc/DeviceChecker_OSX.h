//
//  DeviceChecker_OSX.h
//  youme_voice_engine
//
//  Created by pinky on 2018/7/23.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifndef DeviceChecker_OSX_h
#define DeviceChecker_OSX_h

#ifdef MAC_OS

#import <CoreAudio/CoreAudio.h>
@protocol IDeviceCheckerDeal <NSObject>
@required
-(void) Restart;
-(void) ReCheckHeadset;
-(void) Notify:(NSString*) deviceUid  isPlugin:(bool)isPlugin;
@end


@interface  DeviceChecker_OSX : NSObject

@property (nonatomic, weak) id<IDeviceCheckerDeal> m_delegate;

+ (DeviceChecker_OSX *)getInstance;
-(void)startCheck;
-(void)stopCheck;

-(bool) hasHeadSet;

-(void) updateCurUseDevice:(int)inputDevice outputDevice:(int)outputDevice;

-(void)chooseDevice:(NSString*) deviceUid;

//-(void)startCheckDevicesChange;
//-(void)startCheckDefaultInputDeviceIDChagne;
//-(void)startCheckDefaultOutputDeviceIDChagne;
//-(void)startCheckDefaultOutputDeviceDataSourceChange;
//
//-(void)initCurDeviceInfo;
//-(void)dealDeviceChange;
//
//-(void)updateDeviceUIDs;
//
//
//-(void)onDeviceChange:(int)deviceID  deviceUid:(NSString*)deviceUid  isInput:(bool)isInput isPlugin:(bool) isPlugin;
//-(void)onDefaultDeviceIDChange:(int)curDefaultID isInput:(bool)isInput;
//-(void)onDefaultDeviceDataSourceChanged:(bool)isInput;

@end

#endif //MAC_OS

#endif /* DeviceChecker_OSX_h */

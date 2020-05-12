/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/
#include "tinydav/tdav_apple.h"


#if TDAV_UNDER_APPLE
#ifdef MAC_OS
#import <AppKit/AppKit.h>
#import <IOKit/audio/IOAudioTypes.h>
#include "DeviceChecker_OSX.h"
#else
#import <UIKit/UIKit.h>
#endif

#import "HeadsetMonitor.h"
#include "Application.h"

#import <Foundation/Foundation.h>
#include <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <MediaToolbox/MediaToolbox.h>

#include "tsk_debug.h"
#include "tinymedia/tmedia_defaults.h"
#include "YouMeVoiceEngine.h"
#include "AVRuntimeMonitor.h"
#include "XConfigCWrapper.hpp"

#include "XTimer.h"

#if defined(__APPLE__)
#define IS_UNDER_APP_EXTENSION ([[[NSBundle mainBundle] executablePath] containsString:@".appex/"])
#endif

static tsk_bool_t g_apple_initialized = tsk_false;
static tsk_bool_t g_apple_audio_enabled = tsk_false;
static int32_t g_init_playback_sample_rate = 16000;
static int32_t g_init_record_sample_rate = 16000;

// Indicate if the current audio session category is AVAudioSessionCategoryPlayAndRecord or AVAudioSessionCategoryRecord
static int g_isPlayOnlyCategory = 0;

static tsk_boolean_t g_isSpeakerOn = true;

int tdav_IsPlayOnlyCategory()
{
    return g_isPlayOnlyCategory;
}

#if !TARGET_OS_IPHONE
static long g_resetTimer = 0;
static std::mutex* g_resetTimerMutex = new std::mutex;

static AudioDeviceID g_curInputDevice = 0;
static AudioDeviceID g_curOutputDevice = 0;

void tdav_check_headset();
int resetAudio( const void* pParam, long iTimerID )
{
    std::lock_guard<std::mutex> lock(*g_resetTimerMutex);
    CXTimer::GetInstance()->CancleTimer( g_resetTimer );
    g_resetTimer = 0;
    
//    UInt32 param = 0;
//    OSStatus status;
//    param = sizeof (AudioDeviceID);
//    AudioDeviceID inputDeviceID;
//    status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultInputDevice, &param, &inputDeviceID);
//
//    AudioDeviceID outputDeviceID;
//    status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &param, &outputDeviceID);
    
    int preInputDevice = g_curInputDevice;
    int preOutputDevice = g_curOutputDevice;
    
    tdav_mac_update_use_audio_device();
    
    //如果发现使用的设备号改变了，才需要重置
    if( g_curInputDevice != preInputDevice ||  g_curOutputDevice != preOutputDevice)
    {
        CYouMeVoiceEngine::getInstance()->restartAVSessionAudio();
        TSK_DEBUG_INFO("Reset Audio ");
    }
    
    tdav_check_headset();
    
    return 0;
}

@interface DeviceCheckerDeal : NSObject<IDeviceCheckerDeal>
-(void) Restart;
-(void) ReCheckHeadset;
-(void) Notify:(NSString*) deviceUid  isPlugin:(bool)isPlugin;
@end

@implementation DeviceCheckerDeal
-(void) Restart
{
    int waitTime = 100;
    
    std::lock_guard<std::mutex> lock(*g_resetTimerMutex);
    if( g_resetTimer != 0 )
    {
        return ;    //已经启动reset了
    }
    
    g_resetTimer = CXTimer::GetInstance()->PostTimer( waitTime ,  &resetAudio, nullptr );
}

-(void) ReCheckHeadset
{
    tdav_check_headset();
}

-(void) Notify:(NSString*) deviceUid  isPlugin:(bool)isPlugin
{
    if( isPlugin )
    {
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent( YOUME_EVENT_AUDIO_INPUT_DEVICE_CONNECT, YOUME_SUCCESS , "",  std::string([deviceUid UTF8String]));
    }
    else{
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent( YOUME_EVENT_AUDIO_INPUT_DEVICE_DISCONNECT,  YOUME_SUCCESS, "",  std::string([deviceUid UTF8String]));
    }
}

@end

DeviceCheckerDeal * g_deviceCheckerDeal = [[DeviceCheckerDeal alloc] init];

#endif


#if TARGET_OS_IPHONE



enum {
    IOS_OUTPUT_DEVICE_SPEAKER = 0,
    IOS_OUTPUT_DEVICE_HEADPHONE,
    IOS_OUTPUT_DEVICE_HEADSET,
    IOS_OUTPUT_DEVICE_HEADSETBT,
    IOS_OUTPUT_DEVICE_MAXNUM
};

///////////////////////////////////////////////////////////////////////////
@interface NotificationHandler : NSObject

-(void)handleAVSessionInterrupt:(NSNotification *)notification;

@end // @interface NotificationHandler

@implementation NotificationHandler

-(void)handleAVSessionInterrupt:(NSNotification *)notification {
    NSDictionary *userInfo = [notification userInfo];
    NSNumber *interruptionTypeValue = [userInfo valueForKey:AVAudioSessionInterruptionTypeKey];
    NSUInteger interruptionType = [interruptionTypeValue integerValue];
    
    if( AVAudioSessionInterruptionTypeBegan == interruptionType ){
        TSK_DEBUG_INFO("Interrupted by other apps begin");
        CYouMeVoiceEngine::getInstance()->pauseChannel();
    }else if( AVAudioSessionInterruptionTypeEnded == interruptionType ){
        TSK_DEBUG_INFO("Interrupted by other apps ended");
        uint32_t iosRecoveryDelayFromCall = tmedia_defaults_get_ios_recovery_delay_from_call();
        TSK_DEBUG_INFO("recovery after %f ms", (float)iosRecoveryDelayFromCall/1000);
        [NSThread sleepForTimeInterval:(float)iosRecoveryDelayFromCall/1000];
        CYouMeVoiceEngine::getInstance()->pauseChannel();
        CYouMeVoiceEngine::getInstance()->resumeChannel();
    }
}

@end // @implementation NotificationHandler
///////////////////////////////////////////////////////////////////////////
static NotificationHandler* g_notificationHandler = nil;



int hasHeadset ()
{
#if TARGET_IPHONE_SIMULATOR
    return 0;
#else
    AVAudioSessionRouteDescription* route = [[AVAudioSession sharedInstance] currentRoute];
    for (AVAudioSessionPortDescription* desc in [route outputs]) {
        TSK_DEBUG_INFO ("AudioRoute:%s", [[desc portType] UTF8String]);
        NSString *routeStr = [desc portType];
        
        NSRange HeadphonesBT = [routeStr rangeOfString:@"HeadphonesBT"];
        if (HeadphonesBT.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADSETBT;
        }
        
       
        NSRange headsetPhone = [routeStr rangeOfString:@"Headphone"];
        if (headsetPhone.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADPHONE;
        }
        
        NSRange headsetBT = [routeStr rangeOfString:@"BluetoothA2DPOutput"];
        if (headsetBT.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADSETBT;
        }
        headsetBT = [routeStr rangeOfString:@"BluetoothHFP"];
        if (headsetBT.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADSETBT;
        }
        // BT headset not be treated as headset, we treat it as speaker
        headsetBT = [routeStr rangeOfString:@"HeadsetBT"];
        if (headsetBT.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADSETBT;
        }
        NSRange headsetRange = [routeStr rangeOfString:@"Headset"];
        if (headsetRange.location != NSNotFound)
        {
            return IOS_OUTPUT_DEVICE_HEADSET;
        }
    }
    return IOS_OUTPUT_DEVICE_SPEAKER;
#endif
}

void tdav_InitCategory(int needRecord, int outputToSpeaker)
{
    //g_lastCategory = [NSString stringWithString: [[AVAudioSession sharedInstance]category]];
    //NSLog(@"get g_lastCategory:%@",g_lastCategory);
    tsk_bool_t usePlayAndRecord = tsk_true;
    if (!needRecord) {
        usePlayAndRecord = tsk_false;
    } else if (tmedia_defaults_get_ios_no_permission_no_rec() && (0 == tdav_apple_getRecordPermission())) {
        usePlayAndRecord = tsk_false;
    } else if (tmedia_defaults_get_ios_airplay_no_rec() && tdav_apple_isAirPlayOn()) {
        usePlayAndRecord = tsk_false;
    }
    
    if (!outputToSpeaker) {
        g_isSpeakerOn = false;
    }else {
        g_isSpeakerOn = true;
    }
    
    if (usePlayAndRecord) {
        TSK_DEBUG_INFO ("set AVAudioSession category PlayAndRecord, SpeakerOn:%d", g_isSpeakerOn);
        BOOL ret = YES;
        int ver = YouMeApplication::getInstance()->getSystemVersionMainInt();
        NSError *categoryError = nil;
        if( ver >= 10){
            if(g_isSpeakerOn && tmedia_defaults_get_comm_mode_enabled() && tmedia_defaults_get_ios_need_set_mode()){
                ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeVideoChat options:
                       AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers|AVAudioSessionCategoryOptionAllowAirPlay error:&categoryError];
            }else{
                ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeVoiceChat options:
                       AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers error:&categoryError];
            }
            NSLog(@"set mode: %@",[[AVAudioSession sharedInstance] mode]);
        }else{
            ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord  withOptions:
                   AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers error:&categoryError];
        }
        if( !ret ){
            TSK_DEBUG_ERROR ("set AVAudioSession category PlayAndRecord failed:%s",[categoryError.localizedDescription UTF8String]);
        }
        
        if(ver < 10 && tmedia_defaults_get_comm_mode_enabled() && tmedia_defaults_get_ios_need_set_mode()){
            NSError *modelError = nil;
            NSLog(@"ver < 10 and comm_enable and need set, current mode: %@",[[AVAudioSession sharedInstance] mode]);
            if( ver >= 7 && g_isSpeakerOn != 0){
                ret = [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeVideoChat error:&modelError];
            }else{
                ret = [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeVoiceChat error:&modelError];
            }
            
            NSLog(@"mode: %@",[[AVAudioSession sharedInstance] mode]);
            if( !ret ){
                TSK_DEBUG_ERROR ("set AVAudioSession setMode failed:%s",[modelError.localizedDescription UTF8String]);
            }
            // 参考webrtc录音模块， Sometimes category options don't stick after setting mode.
            ret = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:
                   AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers error:&categoryError];
            if( !ret ){
                TSK_DEBUG_ERROR ("set AVAudioSession category PlayAndRecord failed:%s",[categoryError.localizedDescription UTF8String]);
            }
        }else{
            TSK_DEBUG_ERROR ("can not call setMode because comm_mode_enabled:%d, ios_need_set_mode:%d", tmedia_defaults_get_comm_mode_enabled(), tmedia_defaults_get_ios_need_set_mode());
        }
        
        g_isPlayOnlyCategory = 0;
    } else {
        // AVAudioSessionCategoryPlayback can support bluetooth output, no need to change the option to AVAudioSessionCategoryOptionAllowBluetooth
        // Pls reference https://developer.apple.com/reference/avfoundation/avaudiosessioncategoryoptions/avaudiosessioncategoryoptionallowbluetooth
        if (tmedia_defaults_get_external_input_mode()){
            //#ifdef YOUME_FOR_QINIU
            TSK_DEBUG_INFO ("did not change AVAudioSession category ");
        }else {
            // AVAudioSessionCategoryPlayback can support bluetooth output, no need to change the option to AVAudioSessionCategoryOptionAllowBluetooth
            // Pls reference https://developer.apple.com/reference/avfoundation/avaudiosessioncategoryoptions/avaudiosessioncategoryoptionallowbluetooth
            if (tmedia_defaults_get_ios_always_play_category()) {
                TSK_DEBUG_INFO ("set AVAudioSession category Playback,always_play_category");
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
            } else {
                if(Config_GetInt("USE_AMBIENT",0) != 0){
                    TSK_DEBUG_INFO ("set AVAudioSession category to Ambient");
                    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:nil];
                }else{
                    TSK_DEBUG_INFO ("set AVAudioSession category keep AVAudioSession category");
                    // [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient error:nil];
                }
                
                // TSK_DEBUG_INFO ("keep AVAudioSession category");
                // [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient error:nil];
            }
        }
        g_isPlayOnlyCategory = 1;
    }
    
    // Listen to the event of interruption by other appps such as phone call.
    if (nil == g_notificationHandler && !tmedia_defaults_get_external_input_mode()) {
        g_notificationHandler = [[NotificationHandler alloc] init];

        [[NSNotificationCenter defaultCenter] addObserver:g_notificationHandler
                                                 selector:@selector(handleAVSessionInterrupt:)
                                                     name:AVAudioSessionInterruptionNotification
                                                   object:[AVAudioSession sharedInstance]];
    }
}

void tdav_RestoreCategory()
{
    if (tmedia_defaults_get_external_input_mode()){
        //#ifdef YOUME_FOR_QINIU
        TSK_DEBUG_INFO ("did not Restore AVAudioSession category to SoloAmbient for qiuniu");
    }else {
        int ver = YouMeApplication::getInstance()->getSystemVersionMainInt();
        if (tmedia_defaults_get_ios_always_play_category()) {
            TSK_DEBUG_INFO ("Restore AVAudioSession category to Playback");
            if(ver >= 10 && Config_GetInt("RESET_MODE",1) != 0){
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionMixWithOthers error:nil];
            }else{
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
                if(Config_GetInt("RESET_MODE",1) != 0) [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:nil];
            }
        } else {
            NSString *category = (Config_GetInt("USE_AMBIENT",0) != 0) ? AVAudioSessionCategoryAmbient : AVAudioSessionCategorySoloAmbient;
            TSK_DEBUG_INFO ("Restore AVAudioSession category to: %s",[category UTF8String]);
            if(ver >= 10 && Config_GetInt("RESET_MODE",1) != 0){
                [[AVAudioSession sharedInstance] setCategory:category mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionMixWithOthers error:nil];
            }else{
                [[AVAudioSession sharedInstance] setCategory:category withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
                if(Config_GetInt("RESET_MODE",1) != 0) [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeDefault error:nil];
            }
        }
    }
}

int tdav_apple_setOutputoSpeaker (int enable) {
    TSK_DEBUG_INFO ("setOutputoSpeaker enable: %d",enable);
    g_isSpeakerOn = enable == 1 ? true : false;
    int ver = YouMeApplication::getInstance()->getSystemVersionMainInt();
    AVAudioSessionCategoryOptions options = [[AVAudioSession sharedInstance] categoryOptions];
    NSError *categoryError = nil;
    if (g_isPlayOnlyCategory == 0 && [[[AVAudioSession sharedInstance] category] isEqualToString:AVAudioSessionCategoryPlayAndRecord]) {
        TSK_DEBUG_INFO ("allow setOutputoSpeaker");
        if (enable) {
            options |= AVAudioSessionCategoryOptionDefaultToSpeaker;
            if(ver >= 10){
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeVideoChat options:options error:&categoryError];
            }else{
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:options error:&categoryError];
                if (categoryError != nil) {
                    TSK_DEBUG_ERROR ("Error setCategory:%s",[categoryError.localizedDescription UTF8String]);
                }
                [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeVideoChat error:&categoryError];
                if (categoryError != nil) {
                    TSK_DEBUG_ERROR ("Error setMode:%s",[categoryError.localizedDescription UTF8String]);
                }
            }
        } else {
            options &= ~AVAudioSessionCategoryOptionDefaultToSpeaker;
            if(ver >= 10){
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeVoiceChat options:options error:&categoryError];
            }else{
                [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:options error:&categoryError];
                if (categoryError != nil) {
                    TSK_DEBUG_ERROR ("Error setCategory:%s",[categoryError.localizedDescription UTF8String]);
                }
                [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeVoiceChat error:&categoryError];
                if (categoryError != nil) {
                    TSK_DEBUG_ERROR ("Error setMode:%s",[categoryError.localizedDescription UTF8String]);
                }
            }
        }
        if (categoryError != nil) {
            TSK_DEBUG_ERROR ("Error changing default output route:%s",[categoryError.localizedDescription UTF8String]);
            return -1;
        }
    } else {
        TSK_DEBUG_INFO ("not allow setOutputoSpeaker");
        return -1;
    }
    
    tdav_apple_resetOutputTarget(enable);
    return 0;
}

void tdav_apple_resetOutputTarget (int isSpeakerOn)
{
    g_isSpeakerOn = isSpeakerOn;
    int iOSOutputDevice = hasHeadset ();
    TSK_DEBUG_INFO ("Will Set output target hasheadset = %d", iOSOutputDevice);
    if (!tmedia_defaults_get_external_input_mode() && iOSOutputDevice != IOS_OUTPUT_DEVICE_HEADSETBT && (tmedia_defaults_get_playback_sample_rate() != g_init_playback_sample_rate || tmedia_defaults_get_record_sample_rate() != g_init_record_sample_rate))
    {
        // 内置采集下非蓝牙状态下恢复之前设置的采样率
        TSK_DEBUG_INFO ("Output target to non-Bluetooth, restore init samplerate");
        tmedia_defaults_set_record_sample_rate(g_init_record_sample_rate, 1);
        tmedia_defaults_set_playback_sample_rate(g_init_playback_sample_rate, 1);
        CYouMeVoiceEngine::getInstance()->pauseChannel();
        CYouMeVoiceEngine::getInstance()->resumeChannel();
    }
    if (!tmedia_defaults_get_external_input_mode() && iOSOutputDevice == IOS_OUTPUT_DEVICE_HEADSETBT && (tmedia_defaults_get_playback_sample_rate() != 16000 || tmedia_defaults_get_record_sample_rate() != 16000)) {
        // 内置采集下为防止蓝牙耳机在48k下不出声音，目前蓝牙耳机仅支持16k采样率
        TSK_DEBUG_INFO ("Bluetooth not run in 16k, set samplerate to 16k and restart AVSessionMgr");
        tmedia_defaults_set_playback_sample_rate(16000, 1);
        tmedia_defaults_set_record_sample_rate(16000, 1);
        CYouMeVoiceEngine::getInstance()->pauseChannel();
        CYouMeVoiceEngine::getInstance()->resumeChannel();
    }
    if (iOSOutputDevice == IOS_OUTPUT_DEVICE_HEADSET || iOSOutputDevice == IOS_OUTPUT_DEVICE_HEADPHONE)
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"PluggingHeadset" object:nil];
        TSK_DEBUG_INFO ("#### Headset plugin");
    }
    else
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"unPluggingHeadset" object:nil];
        TSK_DEBUG_INFO ("#### Headset pullout");
    }
    
    AVAudioSessionPortOverride audioRouteOverride = AVAudioSessionPortOverrideNone;
    if ((isSpeakerOn != 0) && (iOSOutputDevice == IOS_OUTPUT_DEVICE_SPEAKER) && !tdav_apple_isAirPlayOn())
    {
        NSLog (@"set to AVAudioSessionPortOverrideSpeaker");
        audioRouteOverride = AVAudioSessionPortOverrideSpeaker;
    }else{
        NSLog (@"set to AVAudioSessionPortOverrideNone");
    }
    BOOL isSucced = [[AVAudioSession sharedInstance] overrideOutputAudioPort:audioRouteOverride error:NULL];
    
    if(!isSucced){
        TSK_DEBUG_ERROR ("overrideOutputAudioPort to Port error");
    }else{
        isSucced = [[AVAudioSession sharedInstance] setActive:YES error:NULL];
        if (!isSucced)
        {
            TSK_DEBUG_ERROR ("Reset audio session settings failed!");
        }
    }
}

// deprecated in iOS7+
static void _AudioSessionInterruptionListener (void *inClientData, UInt32 inInterruptionState)
{
    switch (inInterruptionState)
    {
    case kAudioSessionBeginInterruption:
    {
        TSK_DEBUG_INFO ("_AudioSessionInterruptionListener:kAudioSessionBeginInterruption");
        break;
    }
    case kAudioSessionEndInterruption:
    {
        TSK_DEBUG_INFO ("_AudioSessionInterruptionListener:kAudioSessionEndInterruption");
        break;
    }
    default:
    {
        TSK_DEBUG_INFO ("_AudioSessionInterruptionListener:%u", (unsigned int)inInterruptionState);
        break;
    }
    }
}

static void _AudioRouteChangeListenerCallback (void *inUserData, AudioSessionPropertyID inPropertyID, UInt32 inPropertyValueS, const void *inPropertyValue)
{
    if (inPropertyID != kAudioSessionProperty_AudioRouteChange)
    {
        return;
    }
    // Determines the reason for the route change, to ensure that it is not because of a category change.
    CFDictionaryRef routeChangeDictionary = (CFDictionaryRef)inPropertyValue;

    CFNumberRef routeChangeReasonRef = (CFNumberRef)(CFDictionaryGetValue (routeChangeDictionary, CFSTR (kAudioSession_AudioRouteChangeKey_Reason)));

    SInt32 routeChangeReason = 0;

    CFNumberGetValue (routeChangeReasonRef, kCFNumberSInt32Type, &routeChangeReason);
    NSLog (@"RouteChangeReason : %d", (int)routeChangeReason);

    //旧设备移除
    if (routeChangeReason == kAudioSessionRouteChangeReason_OldDeviceUnavailable)
    {
        tdav_apple_resetOutputTarget (g_isSpeakerOn);
    }
    //新设备插入
    else if (routeChangeReason == kAudioSessionRouteChangeReason_NewDeviceAvailable)
    {
        tdav_apple_resetOutputTarget (g_isSpeakerOn);
    }
    //没有设备
    else if (routeChangeReason == kAudioSessionRouteChangeReason_NoSuitableRouteForCategory)
    {
        tdav_apple_resetOutputTarget (g_isSpeakerOn);
    }
    else if (routeChangeReason == kAudioSessionRouteChangeReason_CategoryChange)
    {
    }
}
#else
void tdav_InitCategory(int needRecord, int outputToSpeaker)
{
    tsk_bool_t usePlayAndRecord = tsk_true;
    if (!needRecord) {
        usePlayAndRecord = tsk_false;
    }
    
    if( usePlayAndRecord )
    {
        g_isPlayOnlyCategory = 0;
    }
    else{
        g_isPlayOnlyCategory = 1;
    }
}

void tdav_check_headset()
{
    int hasHeadSet = hasHeadset();
    if ( hasHeadSet == 1)
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"PluggingHeadset" object:nil];
        TSK_DEBUG_INFO ("#### Headset plugin");
    }
    else
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:@"unPluggingHeadset" object:nil];
        TSK_DEBUG_INFO ("#### Headset pullout");
    }
}

void tdav_apple_resetOutputTarget (int isSpeakerOn)
{
    g_isSpeakerOn = isSpeakerOn;
    CYouMeVoiceEngine::getInstance()->pauseChannel();
    CYouMeVoiceEngine::getInstance()->resumeChannel();
//
    tdav_check_headset();
}
                                           
                                           
                                           
int hasHeadset()
{
    bool hasHeadSet = [[DeviceChecker_OSX getInstance] hasHeadSet];
    return hasHeadSet ? 1 : 0 ;
}

#include "AudioDeviceMgr_OSX.h"

void tdav_mac_set_choose_device(const char* deviceUid)
{
    [[DeviceChecker_OSX getInstance] chooseDevice: [[NSString alloc] initWithUTF8String: deviceUid] ];
}

int tdav_mac_update_use_audio_device()
{
    UInt32 param = 0;
    OSStatus status;
    param = sizeof (AudioDeviceID);
    AudioDeviceID inputDeviceID;
    AudioDeviceID outputDeviceID;
    
    int chooseId  = tmedia_get_record_device();
    if( chooseId != 0 && AudioDeviceMgr_OSX::getInstance()->isAudioDeviceIDValid( chooseId ) )
    {
        inputDeviceID = chooseId;
    }
    else{
        //选择record设备无效，就使用默认设备吧
        status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultInputDevice, &param, &inputDeviceID);
    }
    
    status = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &param, &outputDeviceID);
    
    //我简直要疯啦，有时候明明没有麦克风,input设备却会取到ouput设备的ID，这个再加个检查保护一下吧，如果两个ID相同，这个设备到底是不是input设备
    //选择的虚拟设备可能同时有外部和内部采集，所以这里得判断chooseId==0
    if( !( AudioDeviceMgr_OSX::getInstance()->isInputDevice( inputDeviceID) ) )
    {
        inputDeviceID = 0;
    }
    
    if( inputDeviceID == 0 || outputDeviceID == 0 )
    {
        TSK_DEBUG_INFO("device not ok:%d, %d ", inputDeviceID, outputDeviceID );
        return 0;
    }
    else{
        TSK_DEBUG_INFO("device all OK!:%d, %d ", inputDeviceID, outputDeviceID);
    }
    
    g_curInputDevice = inputDeviceID;
    g_curOutputDevice = outputDeviceID;
    
    [[DeviceChecker_OSX getInstance] updateCurUseDevice: g_curInputDevice outputDevice:g_curOutputDevice];
    
    return 1;
}

int tdav_mac_has_both_audio_device()
{
    if( g_curInputDevice != 0 && g_curOutputDevice != 0 )
    {
        return 1;
    }
    else{
        return 0;
    }
}

int tdav_mac_cur_record_device()
{
    return g_curInputDevice ;
}

#endif

void tdav_apple_monitor_add(int bIsConsumer)
{
    if (bIsConsumer)
    {
        AVRuntimeMonitor::getInstance()->ConsumerCountAdd();
    }else
    {
        AVRuntimeMonitor::getInstance()->ProducerCountAdd();
    }
}

void tdav_apple_monitor_test_disable()
{
#if TARGET_OS_IPHONE
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
#endif //TARGET_OS_IPHONE
}

void tdav_SetTestCategory(int bCategory)
{
#if TARGET_OS_IPHONE
    if (bCategory == 1)
    {
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:
               AVAudioSessionCategoryOptionAllowBluetooth|AVAudioSessionCategoryOptionMixWithOthers error:nil];
        
        tdav_apple_resetOutputTarget (1);
        
//        AudioSessionSetActiveWithFlags(true, kAudioSessionSetActiveFlag_NotifyOthersOnDeactivation);
    }
    else if (bCategory == 2)
    {
        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback withOptions:AVAudioSessionCategoryOptionMixWithOthers error:nil];
        
//        AudioSessionSetActiveWithFlags(false, kAudioSessionSetActiveFlag_NotifyOthersOnDeactivation);
    }
#endif //TARGET_OS_IPHONE
}

int tdav_getIsAudioSessionInChatMode()
{
#if TARGET_OS_IPHONE
    if([AVAudioSession sharedInstance]){
        if (YouMeApplication::getInstance()->getSystemVersionMainInt() >= 7)
            return [[AVAudioSession sharedInstance].mode isEqualToString:AVAudioSessionModeVoiceChat]
                    ||
                   [[AVAudioSession sharedInstance].mode isEqualToString:AVAudioSessionModeVideoChat] ;
        else
            return [[AVAudioSession sharedInstance].mode isEqualToString:AVAudioSessionModeVoiceChat];
    }
#endif //TARGET_OS_IPHONE
    return 0;
}

void tdav_SetCategory()
{
    /*
    NSError *setCategoryError = nil;
    BOOL setCategorySuccess = [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionMixWithOthers error:&setCategoryError];
    if (!setCategorySuccess) {
        TSK_DEBUG_ERROR("set setCategory fail");
    }*/
}
int tdav_apple_init ()
{
    if (g_apple_initialized)
    {
        return 0;
    }
    g_init_playback_sample_rate = tmedia_defaults_get_playback_sample_rate();
    g_init_record_sample_rate = tmedia_defaults_get_record_sample_rate();
// initialize audio session
#if TARGET_OS_IPHONE
    if (YouMeApplication::getInstance()->getSystemVersionMainInt() >= 7)
    {
        // Listening to interruption must be done in your AppDelegate:
        // https://code.google.com/p/idoubs/source/browse/branches/2.0/ios-idoubs/Classes/idoubs2AppDelegate.mm?r=264#433

        AudioSessionAddPropertyListener (kAudioSessionProperty_AudioRouteChange, _AudioRouteChangeListenerCallback, NULL);
        tdav_apple_resetOutputTarget (g_isSpeakerOn);
    }
    else
    {
        OSStatus status;
        status = AudioSessionInitialize (NULL, NULL, _AudioSessionInterruptionListener, NULL);
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioSessionInitialize() failed with status code=%d", (int32_t)status);
            return -1;
        }

        // enable record/playback
        UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
        status = AudioSessionSetProperty (kAudioSessionProperty_AudioCategory, sizeof (audioCategory), &audioCategory);
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioSessionSetProperty(kAudioSessionProperty_AudioCategory) failed with status code=%d", (int32_t)status);
            return -2;
        }

        // allow mixing
        UInt32 allowMixing = true;
        status = AudioSessionSetProperty (kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof (allowMixing), &allowMixing);
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioSessionSetProperty(kAudioSessionProperty_OverrideCategoryMixWithOthers) failed with status code=%d", (int32_t)status);
            return -3;
        }
       
    }
#else
    [[HeadsetMonitor getInstance] start];
    
    [DeviceChecker_OSX getInstance].m_delegate = g_deviceCheckerDeal;
    [[DeviceChecker_OSX getInstance] startCheck];

    tdav_check_headset();
#endif
    g_apple_initialized = tsk_true;


    return 0;
}

int tdav_apple_enable_audio ()
{
#if TARGET_OS_IPHONE
    if (IS_UNDER_APP_EXTENSION) {
        // ios APP extension 模式下不打开speaker
        return -1;
    }
    
    tdav_apple_resetOutputTarget (g_isSpeakerOn);
#endif //TARGET_OS_IPHONE
    
    if (g_apple_audio_enabled)
    {
        return 0;
    }
    int ret = 0;
    if (!g_apple_initialized)
    {
        if ((ret = tdav_apple_init ()))
        {
            return ret;
        }
    }

#if TARGET_OS_IPHONE
    if (YouMeApplication::getInstance()->getSystemVersionMainInt() >= 7)
    {
        NSError *activationError = nil;
        BOOL activationResult = [[AVAudioSession sharedInstance] setActive:YES error:&activationError];

        if (activationResult == NO)
        {
            NSInteger code = [activationError code];
            if (code == AVAudioSessionErrorInsufficientPriority || [[AVAudioSession sharedInstance] isOtherAudioPlaying])
            {
                TSK_DEBUG_WARN ("Delaying audio initialization because another app is using it");
                // application is interrupted -> wait for notification -> not error
                return 0;
            }
            else
            {
                TSK_DEBUG_ERROR ("AVAudioSession.setActive(YES) faile with error code: %ld", (long)code);
                return -1;
            }
        }
    }
    else
    {
        // enable audio session
        OSStatus status = AudioSessionSetActive (true);
        if (status)
        {
            TSK_DEBUG_ERROR ("AudioSessionSetActive(true) failed with status code=%d", (int32_t)status);
            ret = -1;
        }
    }
#endif /* TARGET_OS_IPHONE */
    g_apple_audio_enabled = (ret == 0) ? tsk_true : tsk_false;
    return ret;
}

int tdav_apple_deinit ()
{
#if TARGET_OS_IPHONE
    AudioSessionRemovePropertyListenerWithUserData (kAudioSessionProperty_AudioRouteChange, _AudioRouteChangeListenerCallback, NULL);
#else
    [[DeviceChecker_OSX getInstance] stopCheck];
    [DeviceChecker_OSX getInstance].m_delegate = nil ;
#endif
    return 0;
}

int tdav_apple_getRecordPermission()
{
#if TARGET_OS_IPHONE
    int ver = YouMeApplication::getInstance()->getSystemVersionMainInt();
    if( ver >= 8 || ver == 0){
        AVAudioSessionRecordPermission recPermission = AVAudioSessionRecordPermissionUndetermined;
        recPermission = [[AVAudioSession sharedInstance] recordPermission];
        if (AVAudioSessionRecordPermissionGranted == recPermission) {
            return 1;
        } else if (AVAudioSessionRecordPermissionDenied == recPermission) {
            return 0;
        } else {
            return -1;
        }
    }else if( ver >=7 ){
        [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
            if( granted ){
                TSK_DEBUG_INFO("ios record permission : %d", granted);
            }else{
                TSK_DEBUG_ERROR("ios record permission : %d", granted);
            }
        }];
        return 1;
    }else{
        return 1;
    }
#else
    //todopinky这里要不要检查沙盒的权限啊
    return 1;
#endif
}

int tdav_apple_isAirPlayOn()
{
#if TARGET_OS_IPHONE
    AVAudioSession* audioSession = [AVAudioSession sharedInstance];
    if ([audioSession respondsToSelector:@selector(currentRoute)]) {
        AVAudioSessionRouteDescription* currentRoute = audioSession.currentRoute;
        for (AVAudioSessionPortDescription* outputPort in currentRoute.outputs){
            if ([outputPort.portType isEqualToString:AVAudioSessionPortAirPlay])
                return 1;
        }
    }
#endif
    return 0;
}


//
//int tdav_apple_screenNum()
//{
//    return [[YMScreen screens] count];
//}
#endif /* TDAV_UNDER_APPLE */

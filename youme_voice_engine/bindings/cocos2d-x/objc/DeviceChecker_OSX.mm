//
//  DeviceChecker_OSX.mm
//  SimpleTestMAC
//
//  Created by pinky on 2018/7/20.
//  Copyright © 2018年 youme. All rights reserved.
//

#ifdef MAC_OS

#import <Foundation/Foundation.h>
#import <IOKit/audio/IOAudioTypes.h>
#import "DeviceChecker_OSX.h"
#include <vector>
#include <map>
#include <string>
#include "AudioDeviceMgr_OSX.h"
#include "tdav_apple.h"

#include "tsk_debug.h"

static DeviceChecker_OSX *s_DeviceChecker_OSX_sharedInstance = nil;


@interface  DeviceChecker_OSX()
{
    //记录当前的input,output设备，用于改变的时候进行对比
    std::vector<int> m_vecInputDevices;
    std::vector<int> m_vecOutputDevices;
    std::map<int, std::string> m_mapDeviceIdToName;
    
    //记录当前的默认设备
    AudioDeviceID  defaultInputDeviceID;
    AudioDeviceID  defaultOutputDeviceID;
    
    AudioDeviceID  m_curInputID;
    AudioDeviceID  m_curOutputID;
    
    ///记录注册的block，之后好清理
    //检测所有音频设备的ID变化，能检测插拔（耳机插孔跟内置的默认设备使用同一个ID，这种检测不到）
    AudioObjectPropertyListenerBlock devicesChangeBlock;
    //默认设备ID从0到有，或者从有到0都一定有插拔操作发生，只有在设备间切换时才需要处理
    AudioObjectPropertyListenerBlock inputChangeBlock;
    AudioObjectPropertyListenerBlock outputChangeBlock;
    //设备源改变，似乎只对默认设备有效，设备ID不变，不需要重启音频，只需要重新检测是否有麦克风
    AudioDeviceID m_initOutputDefaultDeviceID;
    AudioObjectPropertyListenerBlock outputChangeBlockData;
    
    NSString* m_choosedDevice;
}


@end

@implementation  DeviceChecker_OSX

+ (DeviceChecker_OSX *)getInstance
{
    @synchronized (self)
    {
        if (s_DeviceChecker_OSX_sharedInstance == nil)
        {
            s_DeviceChecker_OSX_sharedInstance = [self alloc];
        }
    }
    
    return s_DeviceChecker_OSX_sharedInstance;
}

-(void)startCheck
{
    TSK_DEBUG_INFO("startCheck");
    [self initCurDeviceInfo];
    [self updateDeviceUIDs];
    
    [self startCheckDevicesChange];
    [self startCheckDefaultInputDeviceIDChagne];
    [self startCheckDefaultOutputDeviceIDChagne];
    [self startCheckDefaultOutputDeviceDataSourceChange];
}

-(void)stopCheck
{
    TSK_DEBUG_INFO("stopCheck");
    const AudioObjectPropertyAddress defaultInputAddr = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    AudioObjectRemovePropertyListenerBlock( kAudioObjectSystemObject, &defaultInputAddr,dispatch_get_current_queue(), inputChangeBlock );
    
    const AudioObjectPropertyAddress defaultOutputAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    AudioObjectRemovePropertyListenerBlock( kAudioObjectSystemObject, &defaultOutputAddr,dispatch_get_current_queue(), outputChangeBlock );
    
    if( outputChangeBlockData )
    {
        AudioObjectPropertyAddress sourceAddr;
        sourceAddr.mSelector = kAudioDevicePropertyDataSource;
        sourceAddr.mScope = kAudioDevicePropertyScopeOutput;
        sourceAddr.mElement = kAudioObjectPropertyElementMaster;
        AudioObjectRemovePropertyListenerBlock( m_initOutputDefaultDeviceID, &sourceAddr,dispatch_get_current_queue(), outputChangeBlockData );
    }
    
    if( devicesChangeBlock )
    {
        const AudioObjectPropertyAddress defaultAddr = {
            kAudioHardwarePropertyDevices,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster
        };
        
        AudioObjectRemovePropertyListenerBlock( kAudioObjectSystemObject, &defaultAddr,dispatch_get_current_queue(), devicesChangeBlock );
    }
}

-(void)chooseDevice:(NSString*) deviceUid
{
    if( m_choosedDevice == nil || deviceUid == nil )
    {
        if( m_choosedDevice == nil && deviceUid == nil )
        {
            return;
        }
        
    }
    else{
        if( [m_choosedDevice compare: deviceUid] == NSOrderedSame )
        {
            return;
        }
    }
    
    m_choosedDevice = deviceUid;
    TSK_DEBUG_INFO("______Restart for chooseDevice:(%s)", m_choosedDevice?[m_choosedDevice UTF8String]:"Default");
    [self Restart];
}

-(void) updateCurUseDevice:(int)inputDevice outputDevice:(int)outputDevice
{
    TSK_DEBUG_INFO("updateCurUseDevice:input(%d), output:(%d)", inputDevice, outputDevice);
    m_curInputID = inputDevice;
    m_curOutputID = outputDevice;
}
//-(void) Restart
//{
//    int newInputID = defaultInputDeviceID;
//    int newOutputID = defaultOutputDeviceID;
//    if( m_choosedDevice )
//    {
//        newInputID = AudioDeviceMgr_OSX::getInstance()->getAudioDeviceID( [m_choosedDevice UTF8String] );
//        if( newInputID == 0 )
//        {
//            newInputID = defaultInputDeviceID;
//        }
//    }
//    
//    if( newInputID != m_curInputID || newOutputID != m_curOutputID )
//    {
//        m_curInputID = newInputID;
//        m_curOutputID = newOutputID;
//        NSLog(@"______Restart: Input:(%d), Output:(%d)", m_curInputID, m_curOutputID);
//    }
//}

-(void)initCurDeviceInfo
{
    AudioDeviceMgr_OSX::getInstance()->getInputDeviceList( m_vecInputDevices );
    AudioDeviceMgr_OSX::getInstance()->getOutputDeviceList( m_vecOutputDevices );
    
    OSStatus status = 0;
    UInt32 defaultSize = sizeof( AudioDeviceID );
    AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    status  = AudioObjectGetPropertyData( kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultInputDeviceID );
    
    defaultAddr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    status  = AudioObjectGetPropertyData(kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultOutputDeviceID );
    
    TSK_DEBUG_INFO("Init default Input:%d", defaultInputDeviceID );
    TSK_DEBUG_INFO("Init default Output:%d", defaultOutputDeviceID );
    
    m_curInputID = defaultInputDeviceID;
    m_curOutputID = defaultOutputDeviceID;
}

-(bool) hasHeadSet
{
    AudioDeviceID defaultDevice = 0;
    UInt32 defaultSize = sizeof( AudioDeviceID );
    const AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    AudioObjectGetPropertyData( kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultDevice );
    TSK_DEBUG_INFO("cur output device :%d", defaultDevice );
    
    //read its current data source
    AudioObjectPropertyAddress sourceAddr;
    sourceAddr.mSelector = kAudioDevicePropertyDataSource;
    sourceAddr.mScope = kAudioDevicePropertyScopeOutput;
    sourceAddr.mElement = kAudioObjectPropertyElementMaster;
    UInt32 dataSourceId = 0;
    UInt32 dataSourceIdSize = sizeof(UInt32);
    OSStatus status = 0;
    status = AudioObjectGetPropertyData(defaultDevice, &sourceAddr, 0, NULL, &dataSourceIdSize, &dataSourceId);
    
    if( dataSourceId == kIOAudioOutputPortSubTypeHeadphones)
    {
        return true;
    }
    
    int tran = 0;
    AudioObjectPropertyAddress transportAddrIn;
    transportAddrIn.mSelector = kAudioDevicePropertyTransportType;
    transportAddrIn.mScope = kAudioDevicePropertyScopeInput;
    transportAddrIn.mElement = kAudioObjectPropertyElementMaster;
    tran = 0;
    dataSourceIdSize = sizeof(UInt32);
    status = AudioObjectGetPropertyData(defaultDevice, &transportAddrIn, 0, NULL, &dataSourceIdSize, &tran);
    if( status == 0 )
    {
        switch( tran )
        {
            case kAudioDeviceTransportTypeBluetooth:
            case kAudioDeviceTransportTypeBluetoothLE:
                return true;
                break;
            default:
                break;
        }
    }
    
    return false;
}

-(void)startCheckDevicesChange
{
    const AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    DeviceChecker_OSX* _self = self;
    devicesChangeBlock = ^(UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses){
        AudioObjectPropertyAddress* pAddr = (AudioObjectPropertyAddress*)inAddresses;
        for( int i = 0 ;i < inNumberAddresses; i++, pAddr++ )
        {
            if( pAddr->mSelector != kAudioHardwarePropertyDevices )
            {
                continue;
            }
            
            [_self dealDeviceChange];
        }
    };
    
    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &defaultAddr, dispatch_get_current_queue(),devicesChangeBlock );
    
}
-(void)startCheckDefaultInputDeviceIDChagne
{
    const AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    DeviceChecker_OSX* _self = self;
    inputChangeBlock = ^(UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses){
        AudioObjectPropertyAddress* pAddr = (AudioObjectPropertyAddress*)inAddresses;
        for( int i = 0 ;i < inNumberAddresses; i++, pAddr++ )
        {
            if( pAddr->mSelector != kAudioHardwarePropertyDefaultInputDevice )
            {
                NSLog(@"Selector not same");
                continue;
            }
            
            if( pAddr->mScope != kAudioObjectPropertyScopeGlobal )
            {
                NSLog(@"Scope not same");
                continue;
            }
            
            OSStatus status = 0;
            AudioDeviceID defaultDevice;
            UInt32 defaultSize = sizeof( AudioDeviceID );
            const AudioObjectPropertyAddress defaultAddr = {
                kAudioHardwarePropertyDefaultInputDevice,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster
            };
            status  = AudioObjectGetPropertyData( kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultDevice );
            
            [_self onDefaultDeviceIDChange:defaultDevice isInput:true ];
            
        }
        
    };
    
    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &defaultAddr, dispatch_get_current_queue(), inputChangeBlock );
    
}

-(void)startCheckDefaultOutputDeviceIDChagne
{
    const AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    DeviceChecker_OSX* _self = self;
    outputChangeBlock = ^(UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses){
        
        AudioObjectPropertyAddress* pAddr = (AudioObjectPropertyAddress*)inAddresses;
        for( int i = 0 ;i < inNumberAddresses; i++, pAddr++ )
        {
            if( pAddr->mSelector != kAudioHardwarePropertyDefaultOutputDevice )
            {
                continue;
            }
            
            OSStatus status = 0;
            AudioDeviceID defaultDevice;
            UInt32 defaultSize = sizeof( AudioDeviceID );
            const AudioObjectPropertyAddress defaultAddr = {
                kAudioHardwarePropertyDefaultOutputDevice,
                kAudioObjectPropertyScopeGlobal,
                kAudioObjectPropertyElementMaster
            };
            status  = AudioObjectGetPropertyData( kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultDevice );
            [_self onDefaultDeviceIDChange: defaultDevice isInput:false];
        }
    } ;
    
    AudioObjectAddPropertyListenerBlock(kAudioObjectSystemObject, &defaultAddr, dispatch_get_current_queue(),outputChangeBlock );
    
}

-(void)startCheckDefaultOutputDeviceDataSrouceChangeFor:(int)deviceID
{
    TSK_DEBUG_INFO("start Check output Datasource:%d", deviceID );
    m_initOutputDefaultDeviceID = deviceID;
    if( deviceID == 0 )
    {
        return;
    }
    
    AudioObjectPropertyAddress sourceAddr;
    sourceAddr.mSelector = kAudioDevicePropertyDataSource;
    sourceAddr.mScope = kAudioDevicePropertyScopeOutput;
    sourceAddr.mElement = kAudioObjectPropertyElementMaster;

    
    DeviceChecker_OSX* _self = self;
    outputChangeBlockData = ^(UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses){
        
        AudioObjectPropertyAddress* pAddr = (AudioObjectPropertyAddress*)inAddresses;
        for( int i = 0 ;i < inNumberAddresses; i++, pAddr++ )
        {
            if( pAddr->mSelector != kAudioDevicePropertyDataSource )
            {
                continue;
            }
            
            [_self onDefaultDeviceDataSourceChanged: false];
        }
        
    } ;
    
    AudioObjectAddPropertyListenerBlock(deviceID, &sourceAddr, dispatch_get_current_queue(),outputChangeBlockData );
}

-(void)startCheckDefaultOutputDeviceDataSourceChange
{
    const AudioObjectPropertyAddress defaultAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    OSStatus status = 0;
    AudioDeviceID defaultDevice;
    UInt32 defaultSize = sizeof( AudioDeviceID );
    
    AudioObjectPropertyAddress sourceAddr;
    sourceAddr.mSelector = kAudioDevicePropertyDataSource;
    sourceAddr.mScope = kAudioDevicePropertyScopeOutput;
    sourceAddr.mElement = kAudioObjectPropertyElementMaster;
    
    
    status  = AudioObjectGetPropertyData( kAudioObjectSystemObject, & defaultAddr, 0 , NULL, &defaultSize, &defaultDevice );
    NSLog(@"Pinkydodo Headset , device:%d   ", defaultDevice );
    
    [self startCheckDefaultOutputDeviceDataSrouceChangeFor: defaultDevice];
}

-(void)restartCheckDefaultOutputDeviceDataSourceChange
{
    if( outputChangeBlockData != nil )
    {
        AudioObjectPropertyAddress sourceAddr;
        sourceAddr.mSelector = kAudioDevicePropertyDataSource;
        sourceAddr.mScope = kAudioDevicePropertyScopeOutput;
        sourceAddr.mElement = kAudioObjectPropertyElementMaster;
        AudioObjectRemovePropertyListenerBlock( m_initOutputDefaultDeviceID, &sourceAddr,dispatch_get_current_queue(), outputChangeBlockData );
        
        outputChangeBlockData = nil;
    }
    
    [self startCheckDefaultOutputDeviceDataSourceChange];
}

-(void)updateDeviceUIDs{
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH]={0};
    std::vector<int> inputDevices;
    AudioDeviceMgr_OSX::getInstance()->getInputDeviceList( inputDevices );
    for( int i = 0; i < inputDevices.size(); i++ )
    {
        AudioDeviceMgr_OSX::getInstance()->getDevice( inputDevices[i], deviceName, deviceId );
        m_mapDeviceIdToName[ inputDevices[i] ] = std::string( deviceId );
    }
}

-(void) checkDevices:(boolean_t)isInput curDevices:( std::vector<int>& ) curDevices  beforeDevices:(std::vector<int>&)beforeDevices
{
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH]={0};
    
    for( int i = 0 ; i < curDevices.size(); i++ )
    {
        bool bFind = false;
        for( int j = 0 ; j < beforeDevices.size(); j++ )
        {
            if( curDevices[i] == beforeDevices[j] )
            {
                bFind = true;
            }
        }
        
        if( !bFind )
        {
            AudioDeviceMgr_OSX::getInstance()->getDevice( curDevices[i], deviceName, deviceId );
            NSString* deviceUUID = [[NSString alloc] initWithUTF8String: deviceId];
            [self onDeviceChange:curDevices[i] deviceUid:deviceUUID isInput:isInput isPlugin: true];
        }
    }
    
    for( int i = 0 ; i < beforeDevices.size(); i++ )
    {
        bool bFind = false;
        for( int j = 0 ; j < curDevices.size(); j++ )
        {
            if( beforeDevices[i] == curDevices[j] )
            {
                bFind = true;
            }
        }
        
        if( !bFind )
        {
            auto iter = m_mapDeviceIdToName.find( beforeDevices[i] );
            if( iter == m_mapDeviceIdToName.end() )
            {
                continue;
            }
            
            NSString* deviceUUID = [[NSString alloc] initWithUTF8String: iter->second.c_str()];
            [self onDeviceChange:beforeDevices[i] deviceUid:deviceUUID isInput:isInput isPlugin: false];
        }
    }
}

-(void)dealDeviceChange
{
    std::vector<int> vecInputDevices;
    std::vector<int> vecOutputDevices;
    
    AudioDeviceMgr_OSX::getInstance()->getInputDeviceList( vecInputDevices );
    AudioDeviceMgr_OSX::getInstance()->getOutputDeviceList( vecOutputDevices );
    
    [self checkDevices: true  curDevices:vecInputDevices beforeDevices: m_vecInputDevices];
    [self checkDevices: false  curDevices:vecOutputDevices beforeDevices:m_vecOutputDevices];
    
    m_vecInputDevices.swap( vecInputDevices );
    m_vecOutputDevices.swap( vecOutputDevices );
    
    [self updateDeviceUIDs];
}

-(void)onDeviceChange:(int)deviceID  deviceUid:(NSString*)deviceUid  isInput:(bool)isInput isPlugin:(bool) isPlugin
{
    TSK_DEBUG_INFO("%s device(%d : %s) Plug %s", isInput?"Input ":"Output", deviceID, [deviceUid UTF8String], isPlugin?"In ":"Out");
    if( isInput )
    {
        [self Notify: deviceUid isPlugin:isPlugin];
        if( isPlugin )
        {
            if( m_curInputID == 0 )
            {
                TSK_DEBUG_INFO("______Restart     onDeviceChange Input Plugin ");
                [self Restart];
            }
            else    //如果之前用了默认设备，现在插入的是选择的设备，这个时候就要换选择的设备了。
            {
                if( m_choosedDevice != nil && [deviceUid compare: m_choosedDevice] == NSOrderedSame )
                {
                    TSK_DEBUG_INFO("______Restart     onDeviceChange Choose Input Plugin");
                    [self Restart];
                }
                
            }
        }
        else{
            if( deviceID == m_curInputID )
            {
                TSK_DEBUG_INFO("______Restart     onDeviceChange Input plugout ");
                [self Restart];
            }
        }
    }
    else{
        //output 设备只使用默认设备，所以这里不需要管
        return;
    }
    
}

-(void)onDefaultDeviceIDChange:(int)curDefaultID isInput:(bool)isInput
{
    //这种情况，必然会有设备ID列表的变化
    if( isInput )
    {
        int preInputDeviceID = defaultInputDeviceID;
        defaultInputDeviceID = curDefaultID;
        TSK_DEBUG_INFO("onDefaultDeviceIDChange: %s(%d  ->  %d)", "Input ", preInputDeviceID, curDefaultID );
        if( curDefaultID == 0 || preInputDeviceID == 0 || curDefaultID == preInputDeviceID )
        {
            defaultInputDeviceID = curDefaultID;
            return ;
        }
        
        
        //之前用的设备不是默认设备，那默认设备的变化跟它就没什么关系。
        if( m_curInputID != preInputDeviceID )
        {
            return ;
        }
        
        //如果之前使用的正是选择的设备，那就不需要管默认设备的变化
        if( m_choosedDevice )
        {
            int choosedDeviceID = AudioDeviceMgr_OSX::getInstance()->getAudioDeviceID( [m_choosedDevice UTF8String] );
            if( m_curInputID == choosedDeviceID )
            {
                return ;
            }
        }
        
        
        //新设备不是Input设备，多半有错吧
        if( !AudioDeviceMgr_OSX::getInstance()->isInputDevice( curDefaultID ) )
        {
            //这里改回来
            defaultInputDeviceID = preInputDeviceID;
            TSK_DEBUG_INFO("onDefaultDeviceIDChange input err:%d", curDefaultID);
            return ;
        }
        
        TSK_DEBUG_INFO("______Restart     DefaultDeviceID Input");
        
        [self Restart];
        
    }
    else{
        TSK_DEBUG_INFO("onDefaultDeviceIDChange: %s(%d  ->  %d)", "Output", defaultOutputDeviceID, curDefaultID );
        if(  curDefaultID == defaultOutputDeviceID )
        {
            return ;
        }
        
        defaultOutputDeviceID = curDefaultID;
        
        //新设备不是Output设备，多半有错吧
        if( !AudioDeviceMgr_OSX::getInstance()->isOutputDevice( curDefaultID ) )
        {
            TSK_DEBUG_INFO("onDefaultDeviceIDChange output err:%d", curDefaultID);
            return ;
        }
        
        [self restartCheckDefaultOutputDeviceDataSourceChange];
        TSK_DEBUG_INFO("______Restart     DefaultDeviceID Output");
        TSK_DEBUG_INFO("______Headset     DefaultDeviceID Output ");
        [self Restart];
    }
}

-(void)onDefaultDeviceDataSourceChanged:(bool)isInput
{
    if( !isInput )
    {
        [self ReCheckHeadSet];
    }
}

-(void) Restart
{
    if( self.m_delegate )
    {
        [self.m_delegate Restart];
    }
}

-(void) Notify:(NSString*) deviceUid  isPlugin:(bool)isPlugin
{
    TSK_DEBUG_INFO("______Notify Mic plug %s(%s) ", isPlugin?"In ":"Out", [deviceUid UTF8String]);
    if( self.m_delegate )
    {
        [self.m_delegate Notify: deviceUid isPlugin: isPlugin ];
    }
}

-(void) ReCheckHeadSet
{
    TSK_DEBUG_INFO("______Headset       DeviceDataSource ");
    if( self.m_delegate )
    {
        [self.m_delegate ReCheckHeadset];
    }
    
}

@end

#endif //MAC_OS

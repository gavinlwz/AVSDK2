//
//  AudioDeviceMgr_OSX.m
//  youme_voice_engine
//
//  Created by pinky on 2018/7/13.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifdef MAC_OS
#import <Foundation/Foundation.h>

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include "AudioDeviceMgr_OSX.h"
#import <CoreAudio/CoreAudio.h>
#import <IOKit/audio/IOAudioTypes.h>
#import <CoreAudio/AudioHardware.h>


AudioDeviceMgr_OSX* AudioDeviceMgr_OSX::getInstance()
{
    static AudioDeviceMgr_OSX* s_ins = new AudioDeviceMgr_OSX();
    return s_ins;
}


int AudioDeviceMgr_OSX::getInputDeviceCount()
{
    std::vector<int> vecinputDevices;
    getInputDeviceList( vecinputDevices );
    return vecinputDevices.size();
}

int AudioDeviceMgr_OSX::getOutputDeviceCount()
{
    std::vector<int> vecoutputDevices;
    getOutputDeviceList( vecoutputDevices );
    return vecoutputDevices.size();
}

void AudioDeviceMgr_OSX::getDevice( int deviceID, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH])
{
    UInt32 dataSize = 0;
    
    CFStringRef uid = NULL;
    OSStatus status = 0;
    dataSize = sizeof(uid);
    
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDeviceUID,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    status = AudioObjectGetPropertyData( deviceID, &propertyAddress, 0, NULL, &dataSize, &uid);
    if(kAudioHardwareNoError == status) {
        CFStringGetCString( uid, deviceId, MAX_DEVICE_ID_LENGTH, kCFStringEncodingUTF8 );
    }
    
    // Query device name
    CFStringRef name = NULL;
    dataSize = sizeof(name);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
    status = AudioObjectGetPropertyData( deviceID, &propertyAddress, 0, NULL, &dataSize, &name);
    if(kAudioHardwareNoError == status) {
        CFStringGetCString( name, deviceName, MAX_DEVICE_ID_LENGTH, kCFStringEncodingUTF8 );
    }
}

bool AudioDeviceMgr_OSX::getOutputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH])
{
    std::vector<int> vecoutputDevices;
    getOutputDeviceList( vecoutputDevices );
    
    if( index < 0 || index >= vecoutputDevices.size() )
    {
        return false;
    }
    
    int deviceID = vecoutputDevices[index];
    
    getDevice( deviceID, deviceName, deviceId );
    
    return true;
    
}

bool AudioDeviceMgr_OSX::getInputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH])
{
    std::vector<int> vecinputDevices;
    getInputDeviceList( vecinputDevices );
    
    if( index < 0 || index >= vecinputDevices.size() )
    {
        return false;
    }
    
    int deviceID = vecinputDevices[index];
    getDevice( deviceID, deviceName, deviceId );
    return true;
}

int AudioDeviceMgr_OSX::getAudioDeviceID( const char deviceId[MAX_DEVICE_ID_LENGTH] )
{
    std::vector<int> vecAudioDevices;
    getAudioDeviceList( vecAudioDevices );
    OSStatus status = 0;
    UInt32 dataSize = 0;
    
    
    for(UInt32 i = 0; i < vecAudioDevices.size(); ++i) {
        AudioDeviceID deviceID = vecAudioDevices[i];
        
        AudioObjectPropertyAddress propertyAddress = {
            kAudioDevicePropertyDeviceUID,
            kAudioObjectPropertyScopeGlobal,
            kAudioObjectPropertyElementMaster
        };
        CFStringRef uid = NULL;
        dataSize = sizeof(uid);
        status = AudioObjectGetPropertyData( deviceID, &propertyAddress, 0, NULL, &dataSize, &uid);
        if(kAudioHardwareNoError == status) {
            char curdeviceId[MAX_DEVICE_ID_LENGTH];
            CFStringGetCString( uid, curdeviceId, MAX_DEVICE_ID_LENGTH, kCFStringEncodingUTF8 );
            
            if( strcmp( deviceId, curdeviceId ) == 0 )
            {
                return deviceID;
            }
        }
    }
    
    return 0;
}

bool AudioDeviceMgr_OSX::isAudioDeviceIDValid( int  deviceID )
{
    std::vector<int> vecAudioDevices;
    getAudioDeviceList( vecAudioDevices );
    for( int i = 0 ; i < vecAudioDevices.size(); ++i )
    {
        if( deviceID == vecAudioDevices[i] )
        {
            return true;
        }
    }
    
    return false;
}

bool AudioDeviceMgr_OSX::isInputDevice( int deviceID )
{
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeInput,
        kAudioObjectPropertyElementMaster
    };
    
    OSStatus status = 0;
    UInt32 dataSize = 0;
    
    // Determine if the device is an input device (it is an input device if it has input channels)
    dataSize = 0;
    status = AudioObjectGetPropertyDataSize( deviceID, &propertyAddress, 0, NULL, &dataSize);
    if(kAudioHardwareNoError != status) {
        fprintf(stderr, "AudioObjectGetPropertyDataSize (kAudioDevicePropertyStreamConfiguration) failed: %i\n", status);
        return false ;
    }
    
    UInt32 streamCount  = dataSize / sizeof(AudioStreamID);
    
    if (streamCount > 0)
    {
        return true;
    }
    else{
        return false;
    }
}

bool AudioDeviceMgr_OSX::isOutputDevice( int deviceID )
{
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyStreams,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };
    
    OSStatus status = 0;
    UInt32 dataSize = 0;
    
    // Determine if the device is an input device (it is an input device if it has input channels)
    dataSize = 0;
    status = AudioObjectGetPropertyDataSize( deviceID, &propertyAddress, 0, NULL, &dataSize);
    if(kAudioHardwareNoError != status) {
        fprintf(stderr, "AudioObjectGetPropertyDataSize (kAudioDevicePropertyStreamConfiguration) failed: %i\n", status);
        return false ;
    }
    
    UInt32 streamCount  = dataSize / sizeof(AudioStreamID);
    
    if (streamCount > 0)
    {
        return true;
    }
    else{
        return false;
    }
}





void AudioDeviceMgr_OSX::getAudioDeviceList( std::vector<int>& vecAudioDevices )
{
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    
    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
    if(kAudioHardwareNoError != status) {
        fprintf(stderr, "AudioObjectGetPropertyDataSize (kAudioHardwarePropertyDevices) failed: %i\n", status);
        return ;
    }
    
    UInt32 deviceCount = (UInt32)(dataSize / sizeof(AudioDeviceID));
    
    AudioDeviceID *audioDevices = (AudioDeviceID*)(malloc(dataSize));
    if(NULL == audioDevices) {
        fputs("Unable to allocate memory", stderr);
        return ;
    }
    
    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, audioDevices);
    if(kAudioHardwareNoError != status) {
        fprintf(stderr, "AudioObjectGetPropertyData (kAudioHardwarePropertyDevices) failed: %i\n", status);
        free(audioDevices), audioDevices = NULL;
        return ;
    }
    
    for(UInt32 i = 0; i < deviceCount; ++i) {
        vecAudioDevices.push_back( audioDevices[i] );
    }
    
    free(audioDevices), audioDevices = NULL;
    
    return ;
}



void AudioDeviceMgr_OSX::getInputDeviceList( std::vector<int>& vecInputDevices )
{
    std::vector<int> vecAudioDevices;
    getAudioDeviceList( vecAudioDevices );
    
    for(UInt32 i = 0; i < vecAudioDevices.size(); ++i) {
        AudioDeviceID deviceID = vecAudioDevices[i];
        
        if( !isInputDevice( deviceID ) )
        {
            continue;
        }
        
        char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
        char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
        
        getDevice( deviceID, deviceName, deviceId );
        
        if( strstr( deviceId, "AppleHDAEngineOutput" )  == NULL &&
           strstr( deviceId, "VPAUAggregateAudioDevice") == NULL )
        {
            vecInputDevices.push_back( deviceID );
        }
    }
    
    return ;
}

void AudioDeviceMgr_OSX::getOutputDeviceList( std::vector<int>& vecInputDevices )
{
    std::vector<int> vecAudioDevices;
    getAudioDeviceList( vecAudioDevices );
    
    for(UInt32 i = 0; i < vecAudioDevices.size(); ++i) {
        AudioDeviceID deviceID = vecAudioDevices[i];
        
        if( isOutputDevice( deviceID ) )
        {
            vecInputDevices.push_back( deviceID );
        }
    }
    
    return ;
}

void AudioDeviceMgr_OSX::dumpInputDevicesInfo()
{
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    
    NSLog(@"__________________input_________________________");
    int count = getInputDeviceCount();
    for( int i = 0 ; i < count; i++ )
    {
        bool ret = getInputDevice( i , deviceName, deviceId );
        if( ret )
        {
            int cid = getAudioDeviceID( deviceId );
            NSLog(@"InputDevice[%d]: id(%d),name:(%s),uid:(%s)", i, cid, deviceName, deviceId );
        }
        
    }
    NSLog(@"__________________input end_________________________");
    
}
void AudioDeviceMgr_OSX::dumpOutputDevicesInfo()
{
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    
    NSLog(@"");
    NSLog(@"__________________output_________________________");
    int count = getOutputDeviceCount();
    for( int i = 0 ; i < count; i++ )
    {
        bool ret = getOutputDevice( i , deviceName, deviceId );
        if( ret )
        {
            int cid = getAudioDeviceID( deviceId );
            NSLog(@"OutputDevice[%d]: id(%d),name:(%s),uid:(%s)", i, cid, deviceName, deviceId );
        }
    }
    
    NSLog(@"__________________output end_________________________");
    
}

#endif

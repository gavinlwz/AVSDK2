//
//  AudioDeviceMgr_OSX.h
//  youme_voice_engine
//
//  Created by pinky on 2018/7/13.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifndef AudioDeviceMgr_OSX_h
#define AudioDeviceMgr_OSX_h

#ifdef MAC_OS

#ifndef MAX_DEVICE_ID_LENGTH
#define MAX_DEVICE_ID_LENGTH 512
#endif

#include <vector>

class AudioDeviceMgr_OSX
{
public:
    static AudioDeviceMgr_OSX* getInstance();
    ///input device
    int getInputDeviceCount();
    bool getInputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]);
    bool isInputDevice( int deviceID );
    
    ///output device
    int getOutputDeviceCount();
    bool getOutputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]);
    bool isOutputDevice( int deviceID );
    
    //get AudioDeviceID by uuid
    int getAudioDeviceID( const char deviceId[MAX_DEVICE_ID_LENGTH] );
    //check audio device is still here or not
    bool isAudioDeviceIDValid( int  deviceID );
    
    //debug info
    void dumpInputDevicesInfo();
    void dumpOutputDevicesInfo();

    void getInputDeviceList( std::vector<int>& vecInputDevices );
    void getOutputDeviceList( std::vector<int>& vecInputDevices );
    
    void getAudioDeviceList( std::vector<int>& vecAudioDevices );
    void getDevice( int deviceID, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]);
};

#endif


#endif /* AudioDeviceMgr_OSX_h */

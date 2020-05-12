//
//  YMTranscriberManager.hpp
//  youme_voice_engine
//
//  Created by pinky on 2019/8/2.
//  Copyright © 2019 Youme. All rights reserved.
//

#ifndef YMTranscriberManager_hpp
#define YMTranscriberManager_hpp

#include "YMTranscriberItem.hpp"
#include "YouMeCommon/XCondWait.h"
#include <stdio.h>
#include <string>
#include <map>
#include <thread>

class YMTranscriberManager
{
public:
    static YMTranscriberManager* getInstance();
private:
    YMTranscriberManager();
    ~YMTranscriberManager();
    
public:
    int getSupportSampleRate();
    void setToken( std::string token );
    
    void Start();
    void Stop();
    
    void inputAudioData( int sessionId,  uint8_t* data, int data_size );
    
    static void ThreadFun(  void* data );
    
private:
    std::string m_token;
	long m_expireTime;
    std::map<int, YMTranscriberItem*> m_mapSessionToTranscriber;
    std::thread m_thread;
    bool m_stop;
    std::mutex m_mutex;
    std::mutex m_dataMutex;
    youmecommon::CXCondWait m_cond;
};


#endif /* YMTranscriberManager_hpp */

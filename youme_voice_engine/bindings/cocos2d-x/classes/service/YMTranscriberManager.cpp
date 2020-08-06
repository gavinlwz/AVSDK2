//
//  YMTranscriberManager.cpp
//  youme_voice_engine
//
//  Created by pinky on 2019/8/2.
//  Copyright © 2019 Youme. All rights reserved.
//

#include "YMTranscriberManager.hpp"
#include "XOsWrapper.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include <sstream>

YMTranscriberManager* YMTranscriberManager::getInstance()
{
    static YMTranscriberManager* s_mgr = new YMTranscriberManager();
    return s_mgr;
}

YMTranscriberManager::YMTranscriberManager()
:m_stop(false)
{
    //setToken( "abcd" );
	m_expireTime = 0;
    
}

YMTranscriberManager::~YMTranscriberManager()
{
    
}

int YMTranscriberManager::getSupportSampleRate()
{
    return 16000;
}
#ifdef TRANSSCRIBER
#include "nlsCommonSdk/Token.h"
#include <iostream>
int generateToken(std::string akId, std::string akSecret, std::string* token, long* expireTime) {
	AlibabaNlsCommon::NlsToken nlsTokenRequest;
	nlsTokenRequest.setAccessKeyId(akId);
	nlsTokenRequest.setKeySecret(akSecret);

	if (-1 == nlsTokenRequest.applyNlsToken()) {
		std::cout << "Failed: " << nlsTokenRequest.getErrorMsg() << std::endl; /*获取失败原因*/

		return -1;
	}

	*token = nlsTokenRequest.getToken();
	*expireTime = nlsTokenRequest.getExpireTime();

	return 0;
}
#endif

void YMTranscriberManager::setToken( std::string token )
{
	//todopinky:临时在本地生成token
#ifdef TRANSSCRIBER
	long expireTime = 0;
	if (m_token.empty() || m_expireTime ==0 || (tsk_gettimeofday_ms() / 1000) > m_expireTime){
		generateToken("LTAILAkh3ja0DNSE", "wlH9aPOa9AfnD4Kwhci4iehQvyvO1h", &m_token, &expireTime);
		m_expireTime = expireTime;
	}
#else
    m_token = token ;
#endif
}

void YMTranscriberManager::inputAudioData( int sessionId,  uint8_t* data, int data_size )
{
#ifdef TRANSSCRIBER
    if( m_stop )
    {
        return ;
    }
    
    std::lock_guard<std::mutex> lock( m_dataMutex );
    auto iterFind = m_mapSessionToTranscriber.find( sessionId );
    if( iterFind == m_mapSessionToTranscriber.end()  )
    {
        if( !m_token.empty() )
        {
            m_mapSessionToTranscriber[ sessionId ] = new YMTranscriberItem( sessionId );
            iterFind = m_mapSessionToTranscriber.find( sessionId );
            
            if( iterFind != m_mapSessionToTranscriber.end() )
            {
                iterFind->second->setToken( m_token );
            }
        }
    }
    
    if( iterFind != m_mapSessionToTranscriber.end()  )
    {
        iterFind->second->inputAudioData( data, data_size );
    }
#endif
}

void YMTranscriberManager::Start()
{
#ifdef TRANSSCRIBER
    Stop();
    std::lock_guard<std::mutex> lock(m_mutex);
    TSK_DEBUG_INFO("YMTranscriberManager Start OK");
    m_thread = std::thread( &YMTranscriberManager::ThreadFun, this );
#endif
}

void YMTranscriberManager::Stop()
{
#ifdef TRANSSCRIBER
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = true;
    if (m_thread.joinable()) {
        m_cond.SetSignal();
        if (m_thread.get_id() == std::this_thread::get_id()) {
            m_thread.detach();
        }
        else {
            m_thread.join();
        }
        TSK_DEBUG_INFO("YMTranscriberManager Stop OK");
    }
    m_stop = false;
    m_cond.Reset();
#endif
}

void YMTranscriberManager::ThreadFun( void* data)
{
#ifdef TRANSSCRIBER
    YMTranscriberManager* pMgr = (YMTranscriberManager*)data;
    if( !pMgr )
    {
        return ;
    }
    do{
        if(pMgr->m_stop)
        {
            break;
        }
        
        //在这个线程外，只会添加，不会删除，所以，拷贝出来的数据就是安全的。
        std::map<int, YMTranscriberItem*> mapTranscriber;
        {
            std::lock_guard<std::mutex> lock( pMgr->m_dataMutex);
            mapTranscriber = pMgr->m_mapSessionToTranscriber;
        }
        
        for( auto iter = mapTranscriber.begin(); iter != mapTranscriber.end(); ++iter  )
        {
            if( !iter->second->isStarted() )
            {
                iter->second->start();
            }
            
            iter->second->sendAudioData();
        }
        
         pMgr->m_cond.WaitTime( 5 );
    }while(true );
    
    std::lock_guard<std::mutex> lock( pMgr->m_dataMutex);
    for( auto iter = pMgr->m_mapSessionToTranscriber.begin(); iter != pMgr->m_mapSessionToTranscriber.end(); ++iter )
    {
        iter->second->stop();
        delete iter->second;
    }
    
    pMgr->m_mapSessionToTranscriber.clear();
    pMgr->m_stop = false;
#endif
}

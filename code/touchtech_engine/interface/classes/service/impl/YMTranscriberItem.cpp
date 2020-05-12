//
//  YMTranscriberItem.cpp
//  youme_voice_engine
//
//  Created by pinky on 2019/8/2.
//  Copyright © 2019 Youme. All rights reserved.
//

#include "YMTranscriberItem.hpp"

#include "YouMeVoiceEngine.h"

#include "tsk_debug.h"

#ifdef TRANSSCRIBER //windows support only
#include "AliTranscriberWin.h"
#endif

#include "tsk_debug.h"

void CTranscriber::setToken( std::string token )
{
    
}
bool CTranscriber::start()
{
	return true;
}
bool CTranscriber::stop()
{
	return true;
}
bool CTranscriber::sendAudioData( uint8_t* data, int data_size )
{
	return true;
}

void CTranscriber::setCallback(ITranscriberCallback* pCallback)
{

}

YMTranscriberItem::YMTranscriberItem( int sessionId )
:m_sessionId( sessionId )
,m_isStarted(false)
,m_pTranscriber( nullptr )
,m_buffer(nullptr)
,m_bufferSize( 0 )
,m_dataSize(0)
, m_userid("")
{
    m_bufferSize = 16000 * 20 / 1000 * 2 ;  // 默认一次放入采样率16K，深度2，20毫秒的数据
    m_buffer = new uint8_t[ m_bufferSize ];

#ifdef TRANSSCRIBER
    m_pTranscriber = new AliTranscriberWin();
#endif

	if (m_pTranscriber)
	{
		m_pTranscriber->setCallback(this);
	}	
}

YMTranscriberItem::~YMTranscriberItem()
{
    if( m_buffer )
    {
        delete [] m_buffer;
    }
}


void YMTranscriberItem::setToken( std::string token)
{
    m_pTranscriber->setToken( token );
}

bool YMTranscriberItem::isStarted()
{
    return m_isStarted;
}
void YMTranscriberItem::start()
{
	if (m_pTranscriber)
	{
		m_pTranscriber->start();
	}
    
    m_isStarted = true;
}
void YMTranscriberItem::inputAudioData( uint8_t* data, int data_size )
{   
	std::lock_guard<std::mutex> lock(m_dataMutex);
    if( m_bufferSize < data_size )
    {
        delete [] m_buffer;
        m_bufferSize = data_size;
        m_buffer = new uint8_t[ m_bufferSize ];
    }
    
    memcpy( m_buffer,  data ,  data_size );
    m_dataSize = data_size;
}

void YMTranscriberItem::sendAudioData()
{
	std::lock_guard<std::mutex> lock(m_dataMutex);
    if( m_dataSize != 0 && m_pTranscriber )
    {
        m_pTranscriber->sendAudioData( m_buffer, m_dataSize );
        m_dataSize = 0;

		//TSK_DEBUG_INFO("sendAudioData");
    }
}

void YMTranscriberItem::stop()
{
	if (m_pTranscriber)
	{
		m_pTranscriber->stop();
	}
   
    m_isStarted = false;
}

void YMTranscriberItem::onTranscribeStarted( int statusCode, std::string taskId )
{
	TSK_DEBUG_INFO("onTranscribeStarted:%d", m_sessionId );
}
void YMTranscriberItem::onTranscribeCompleted( int statusCode, std::string taskId )
{
	TSK_DEBUG_INFO("onTranscribeCompleted:%d", m_sessionId);
}
void YMTranscriberItem::onTranscribeFailed( int statusCode, std::string taskId, std::string errMsg )
{
	TSK_DEBUG_ERROR("onTranscribeFailed failed:%d, status:%d, taskId：%s, errMsg:%s!", m_sessionId, statusCode, taskId.c_str(), errMsg.c_str() );
}

void YMTranscriberItem::onSentenceBegin( int statusCode, std::string taskId, int sentenceIndex, int beforeSentenceTime )
{
	if (statusCode == 0 || statusCode == 20000000)
	{
		CYouMeVoiceEngine::getInstance()->notifySentenceBegin(m_sessionId, sentenceIndex, beforeSentenceTime);
	}
	else {
		TSK_DEBUG_ERROR("onSentenceBegin failed:%d, status:%d taskId：%s,!", m_sessionId, statusCode, taskId.c_str());
	}
}
void YMTranscriberItem::onSentenceEnd( int statusCode, std::string taskId, int sentenceIndex , std::string result, int sentenceTime)
{
	if (statusCode == 0 || statusCode == 20000000)
	{
		CYouMeVoiceEngine::getInstance()->notifySentenceEnd(m_sessionId, sentenceIndex, result, sentenceTime);
	}
	else {
		TSK_DEBUG_ERROR("onSentenceEnd failed:%d, tatus:%d, taskId：%s,!", m_sessionId, statusCode, taskId.c_str());
	}
}

void YMTranscriberItem::onSentenceChanged(int statusCode, std::string taskId, int sentenceIndex, std::string result, int sentenceTime)
{
	if (statusCode == 0 || statusCode == 20000000)
	{
		CYouMeVoiceEngine::getInstance()->notifySentenceChanged(m_sessionId, sentenceIndex, result, sentenceTime);
	}
	else {
		TSK_DEBUG_ERROR("onSentenceChanged failed:%d, status:%d taskId：%s,!", m_sessionId, statusCode, taskId.c_str());
	}
}

//
//  YMVideoRecorderManager.cpp
//  VideoRecord
//
//  Created by pinky on 2019/5/16.
//  Copyright © 2019 youme. All rights reserved.
//

#if !defined(OS_LINUX)
#include "YMVideoRecorderManager.hpp"
#include "tsk_debug.h"
#include <sstream>

YMVideoRecorderManager* YMVideoRecorderManager::getInstance()
{
    static YMVideoRecorderManager* s_mgr = new YMVideoRecorderManager();
    return s_mgr;
}

YMVideoRecorderManager::YMVideoRecorderManager()
{
    
}

YMVideoRecorderManager::~YMVideoRecorderManager()
{
    
}

//void YMVideoRecorderManager::setDirPath( std::string path )
//{
//    printf("Pinky:setDirPath:%s\n", path.c_str() );
//    m_dirPath = path;
//    //todopinky:
//    //if dir not exist, create
//}

//std::string YMVideoRecorderManager::getFilePath( std::string userid )
//{
//    std::stringstream ss;
//
//    //todopinky
////    ss<<m_dirPath<<"/"<<userid<<"_"<<time(0)<<".mp4";
//    ss<<m_dirPath<<"/"<<userid<<".mp4";
//
//    return ss.str();
//
//}

void YMVideoRecorderManager::NotifyUserName( int32_t sessionId, const std::string& userID , bool isLocalUser  )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
//    printf("Pinky:NotifyUserName:%s,:%d\n", userID.c_str(), sessionId );
//    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_mapSessionToName.find( sessionId );
    if( iter == m_mapSessionToName.end() ){
        m_mapSessionToName[sessionId] = userID;
    }
    
    if( isLocalUser )
    {
        m_localSessionId = sessionId;
    }
}

void YMVideoRecorderManager::stopRecord( std::string  userid )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    TSK_DEBUG_INFO("stopRecord");
    auto pRecorder = getRecorder( userid );
    if( pRecorder )
    {
        pRecorder->stopRecord();
    }
    
    deleteRecord( userid );
}

//离开房间的时候，全都停了
void YMVideoRecorderManager::stopRecordAll()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    TSK_DEBUG_INFO("stopRecordAll");
    for( auto iter = m_mapRecorder.begin(); iter != m_mapRecorder.end(); ++iter )
    {
        iter->second->stopRecord();
        delete iter->second;
    }
    
    m_mapRecorder.clear();
}

void YMVideoRecorderManager::deleteRecord( std::string userid )
{
    auto iter =  m_mapRecorder.find( userid ) ;
    if( iter != m_mapRecorder.end() )
    {
        delete iter->second;
        m_mapRecorder.erase( iter );
    }
}

YMVideoRecorder* YMVideoRecorderManager::startRecord( std::string  userid , std::string saveFile )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    TSK_DEBUG_INFO("startRecord:%s, save:%s", userid.c_str(), saveFile.c_str());
    if( m_mapRecorder.find( userid ) == m_mapRecorder.end() )
    {
        m_mapRecorder[ userid ] = new YMVideoRecorder();
    }
    
    auto pRecorder =  getRecorder( userid );
    if( pRecorder )
    {
        pRecorder->setFilePath( saveFile );
        pRecorder->startRecord();
    }
    
    return pRecorder;
}

bool YMVideoRecorderManager::needRecord( std::string userid  )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if( m_mapRecorder.find( userid ) != m_mapRecorder.end() )
    {
        return false;
    }
    
    return true;
}

void YMVideoRecorderManager::setAudioInfo( int sampleRate )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    for( auto iter = m_mapRecorder.begin(); iter != m_mapRecorder.end(); ++iter )
    {
        iter->second->setInputAudioInfo( sampleRate );
    }
}

void YMVideoRecorderManager::setVideoInfo(int sessionId, int width, int height, int fps, int birrate, int timebaseden)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	auto pRecorder = getRecorderBySession(sessionId);
	if (pRecorder)
	{
		pRecorder->setVideoInfo(width, height, fps, birrate, timebaseden);
	}
}

void YMVideoRecorderManager::inputAudioData( uint8_t* data, int data_size, int timestamp )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto pRecorder = getRecorderBySession( m_localSessionId );
    if( pRecorder )
    {
        pRecorder->setInputAudioInfo( 16000 );
        pRecorder->inputAudioData( data , data_size,  timestamp );
    }
}

void YMVideoRecorderManager::inputVideoData( int sessionId, uint8_t* data, int data_size, int timestamp, int nalu_type )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto pRecorder = getRecorderBySession( sessionId );
    if( pRecorder )
    {
        pRecorder->inputVideoData( data , data_size,  timestamp, nalu_type );
    }
}

void YMVideoRecorderManager::annexbToAvcc(uint8_t* input, int in_size, uint8_t** output, int *out_size) {
    int tmp_size = in_size + 4;
    uint8_t* tmp = new uint8_t[tmp_size];
    tmp[0] = (in_size & 0xff000000) >> 24;
    tmp[1] = (in_size & 0x00ff0000) >> 16;
    tmp[2] = (in_size & 0x0000ff00) >> 8;
    tmp[3] = in_size & 0x000000ff;
    memcpy(tmp+4, input, in_size);
    
    *output = tmp;
    *out_size = tmp_size;
}


void YMVideoRecorderManager::setSpsAndPps( int sessionId, uint8_t* sps, int sps_size, uint8_t* pps, int pps_size )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto pRecorder = getRecorderBySession( sessionId );
    if( pRecorder )
    {
        pRecorder->setSpsAndPps( sps , sps_size,  pps, pps_size );
    }
}

YMVideoRecorder* YMVideoRecorderManager::getRecorder( std::string userid )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto iter = m_mapRecorder.find( userid );
    if( iter != m_mapRecorder.end() )
    {
        return iter->second;
    }
    else{
        return nullptr;
    }
}

YMVideoRecorder* YMVideoRecorderManager::getRecorderBySession( int sessionId )
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto iterName = m_mapSessionToName.find( sessionId );
    if( iterName == m_mapSessionToName.end() )
    {
        return NULL;
    }
    
    std::string userid = iterName->second;
    return getRecorder( userid );
}
#endif

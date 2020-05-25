//
//  AVStatistic.cpp
//  youme_voice_engine
//
//  Created by pinky on 2017/9/6.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "AVStatistic.h"
#include "tsk_debug.h"
#include "YouMeVoiceEngine.h"
#include "NgnMemoryConfiguration.hpp"
#include "NgnConfigurationEntry.h"
#include "ReportService.h"
#include "NgnApplication.h"

#define AVSTATISTIC_USE_RTCP 1

ReportQuitData *ReportQuitData::sInstance = NULL;

ReportQuitData *ReportQuitData::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new ReportQuitData();
    }
    return sInstance;
}

void ReportQuitData::destroy ()
{
    delete sInstance;
    sInstance = NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////////////

struct AVNotifyData
{
    AVNotifyData( YouMeAVStatisticType type, int session, int value  )
    :m_type(type),
    m_sessionID( session ),
    m_value(value)
    {
        
    }
    
    YouMeAVStatisticType m_type;
    int m_sessionID;
    int m_value;
    
};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GetReportServerData( std::map<int, UserVideoData >& userDatas , std::vector<AVNotifyData>& vecNotify,
                         std::map<int, int>& mapVideoBlockTime, std::map<int, int64_t>& mapVideoPackageTime , std::map<int,int>& mapVideoPackageCount,
                         std::map<int, int >& mapVideoNetDelay, int nUpAudioPacketLoss, int nUpAudioPacketTotal, int nUpVideoPacketLoss, int nUpVideoPacketTotal);

void sendNotifyEvent( const char* typeName,  int value  )
{
    YouMeEvent evt = YOUME_EVENT_EOF;
    if( strcmp( typeName ,"media_road_pass" ) == 0  ){
        evt = YOUME_EVENT_MEDIA_DATA_ROAD_PASS;
    }
    else if ( strcmp( typeName , "media_road_block") == 0   )
    {
        evt = YOUME_EVENT_MEDIA_DATA_ROAD_BLOCK;
    }
    
    CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent( evt , YOUME_SUCCESS , "","");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AVSPacketRecvData::AVSPacketRecvData()
{
    m_seq_max = -1;
    m_seq_min = -1;
    m_recv_count = 0 ;
    m_circleCount = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AVStatistic *AVStatistic::sInstance = NULL;

AVStatistic *AVStatistic::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new AVStatistic();
    }
    return sInstance;
}

void AVStatistic::destroy ()
{
    delete sInstance;
    sInstance = NULL;
}

AVStatistic::AVStatistic()
{
    m_client.setToClient( true, 0 );
    m_server.setToClient( false, AVSTATIC_REPORT_SERVER_VIDEO_STAT );
    m_server_avqos.setToClient( false, AVSTATIC_REPORT_SERVER_AVQOS);
}

void AVStatistic::StartThread()
{
    m_client.StartThread();
    m_server.StartThread();
    m_server_avqos.StartThread();
}

void AVStatistic::StopThread()
{
    m_client.StopThread();
    m_server.StopThread();
    m_server_avqos.StopThread();
}

void AVStatistic::addAudioCode( int codeCount, int32_t sessionId  )
{
    m_client.addAudioCode( codeCount, sessionId );
    m_server.addAudioCode( codeCount, sessionId );
    m_server_avqos.addAudioCode( codeCount, sessionId );
}

void AVStatistic::addVideoCode( int codeCount, int32_t sessionId )
{
    m_client.addVideoCode( codeCount, sessionId );
    m_server.addVideoCode( codeCount, sessionId );
    m_server_avqos.addVideoCode( codeCount, sessionId );
}

void AVStatistic::addVideoShareCode(int codeCount, int32_t sessionId)
{
    m_server_avqos.addVideoShareCode( codeCount, sessionId );
}

void AVStatistic::addVideoFrame( int frameCount, int32_t sessionId )
{
    m_client.addVideoFrame( frameCount, abs(sessionId) );
    m_server.addVideoFrame( frameCount, abs(sessionId) );
    m_server_avqos.addVideoFrame( frameCount, abs(sessionId) );

}

void AVStatistic::addVideoBlock( int count, int32_t sessionId )
{
    m_client.addVideoBlock( count, sessionId );
    m_server.addVideoBlock( count, sessionId );
    m_server_avqos.addVideoBlock( count, sessionId );
}

void AVStatistic::addVideoBlockTime( int time, int32_t sessionId )
{
    m_client.addVideoBlockTime( time, abs(sessionId) );
    m_server.addVideoBlockTime( time, abs(sessionId) );
    m_server_avqos.addVideoBlockTime( time, abs(sessionId) );

}

void AVStatistic::addAudioPacket( int seqNum, int32_t sessionId )
{
    m_client.addAudioPacket( seqNum, sessionId );
    m_server.addAudioPacket( seqNum, sessionId );
    m_server_avqos.addAudioPacket( seqNum, sessionId );
}
void AVStatistic::addVideoPacket( int seqNum, int32_t sessionId )
{
    m_client.addVideoPacket( seqNum, sessionId );
    m_server.addVideoPacket( seqNum, sessionId );
    m_server_avqos.addVideoPacket( seqNum, sessionId );
}

void AVStatistic::addVideoPacketTimeGap( int timegap, int sessionId   )
{
    m_client.addVideoPacketTimeGap( timegap, sessionId );
    m_server.addVideoPacketTimeGap( timegap, sessionId );
    m_server_avqos.addVideoPacketTimeGap( timegap, sessionId );
}

void AVStatistic::addVideoPacketDelay( int timeDelay, int sessionId )
{
    m_client.addVideoPacketDelay( timeDelay, sessionId );
    m_server.addVideoPacketDelay( timeDelay, sessionId );
    m_server_avqos.addVideoPacketDelay( timeDelay, sessionId );
}

void AVStatistic::addSelfVideoPacket( int lost, int totalCount, int sessionId )
{
    m_client.addSelfVideoPacket( lost, totalCount, sessionId );
    m_server.addSelfVideoPacket( lost, totalCount, sessionId );
    m_server_avqos.addSelfVideoPacket( lost, totalCount, sessionId );
}

void AVStatistic::addSelfAudioPacket( int lost, int totalCount, int sessionId )
{
    m_client.addSelfAudioPacket( lost, totalCount, sessionId);
    m_server.addSelfAudioPacket( lost, totalCount, sessionId);
    m_server_avqos.addSelfAudioPacket( lost, totalCount, sessionId);
}

void AVStatistic::setAudioUpPacketLossRtcp( int lost, int sessionId )
{
    m_client.setAudioUpPacketLossRtcp( lost, sessionId);
    m_server.setAudioUpPacketLossRtcp( lost, sessionId);
    m_server_avqos.setAudioUpPacketLossRtcp( lost, sessionId);
}

void AVStatistic::setAudioDnPacketLossRtcp( int lost, int sessionId )
{
    m_client.setAudioDnPacketLossRtcp( lost, sessionId);
    m_server.setAudioDnPacketLossRtcp( lost, sessionId);
    m_server_avqos.setAudioDnPacketLossRtcp( lost, sessionId);
}

void AVStatistic::setVideoUpPacketLossRtcp( int lost, int sessionId )
{
    m_client.setVideoUpPacketLossRtcp( lost, sessionId);
    m_server.setVideoUpPacketLossRtcp( lost, sessionId);
    m_server_avqos.setVideoUpPacketLossRtcp( lost, sessionId);
}

void AVStatistic::setVideoDnPacketLossRtcp( int lost, int sessionId )
{
    m_client.setVideoDnPacketLossRtcp( lost, sessionId);
    m_server.setVideoDnPacketLossRtcp( lost, sessionId);
    m_server_avqos.setVideoDnPacketLossRtcp( lost, sessionId);
}

void AVStatistic::setAudioPacketDelayRtcp( int delay, int sessionId )
{
    m_client.setAudioPacketDelayRtcp( delay, sessionId);
    m_server.setAudioPacketDelayRtcp( delay, sessionId);
    m_server_avqos.setAudioPacketDelayRtcp( delay, sessionId);
}

void AVStatistic::setVideoPacketDelayRtcp( int delay, int sessionId ) 
{
    m_client.setVideoPacketDelayRtcp( delay, sessionId);
    m_server.setVideoPacketDelayRtcp( delay, sessionId);
    m_server_avqos.setVideoPacketDelayRtcp( delay, sessionId);
}

void AVStatistic::setRecvDataStat( uint64_t recvStat)
{
    m_client.setRecvDataStat( recvStat );
}

void AVStatistic::setInterval( int interval )
{
    m_client.setInterval( interval );
}

//通知视频开关
void  AVStatistic::NotifyVideoStat( const std::string& userID , bool bOn )
{
    m_client.NotifyVideoStat( userID, bOn );
    m_server.NotifyVideoStat( userID, bOn );
}

void AVStatistic::NotifyUserName( int32_t sessionId, const std::string& userID ){
    m_client.NotifyUserName( sessionId, userID );
    m_server.NotifyUserName( sessionId, userID );
    m_server_avqos.NotifyUserName( sessionId, userID );
}

void AVStatistic::NotifyRoomName( const std::string& roomName,  int selfSessionID, bool bEnter )
{
    m_client.NotifyRoomName( roomName, selfSessionID, bEnter );
    m_server.NotifyRoomName( roomName, selfSessionID, bEnter );
    m_server_avqos.NotifyRoomName( roomName, selfSessionID, bEnter );
}

void AVStatistic::NotifyStartVideo()
{
    m_client.NotifyStartVideo();
    m_server.NotifyStartVideo();
}

void AVStatistic::NotifyGetRenderFrame()
{
    m_client.NotifyGetRenderFrame();
    m_server.NotifyGetRenderFrame();
}


///////////////////////////////////////////////////////////////////////////////////////////////////




AVStatisticImpl::AVStatisticImpl()
{
    ResetData();
    m_monitorCond.Reset();
    m_interval = 10000;
}

AVStatisticImpl::~AVStatisticImpl()
{
    StopThread();
}

void AVStatisticImpl::ResetData()
{
    m_mapVideoFrameCount.clear();
    m_mapVideoCodeCount.clear();
    m_mapVideoShareCodeCount.clear();
    m_mapAudioCodeCount.clear();
    
    m_mapVideoBlock.clear();
    m_mapAudioPacketLoss.clear();
    m_mapVideoPacketLoss.clear();

    m_mapAudioUpPacketLoss_Rtcp.clear();
    m_mapAudioDnPacketLoss_Rtcp.clear();
    m_mapVideoUpPacketLoss_Rtcp.clear();
    m_mapVideoDnPacketLoss_Rtcp.clear();
    m_mapAudioNetDelay_Rtcp.clear();
    m_mapVideoNetDelay_Rtcp.clear();

    m_nRecvBitrateStat = 0;
    m_nSelfVideoPacketTotal = 0 ;
    m_nSelfVideoPacketLoss = 0 ;
//    m_nSessionId = 0;
    
    m_nSelfAudioPacketTotal = 0 ;
    m_nSelfAudioPacketLoss = 0 ;
    
    m_mapVideoBlockTime.clear();
    m_mapVideoPacketTime.clear();
    //videoClose不能清空，谁知道帧间隔的上报会不会跨周期呢
//    m_mapVideoClose.clear();
    
    for( auto iter = m_mapUserVideoTimeData.begin(); iter != m_mapUserVideoTimeData.end(); ++iter ){
        //不能重置user和sessionID的对应关系
        //如果开始了，不能丢失开始信息，要重新计算开始时间
        if( iter->second.m_startTimeMs != 0 ){
            iter->second.m_startTimeMs = tsk_time_now();
        }
        iter->second.m_totalTimeMs = 0;
    }
}

void AVStatisticImpl::StartThread()
{
    if( !m_bClient ){
        if (AVSTATIC_REPORT_SERVER_VIDEO_STAT == m_nReportCmd) {
            m_interval = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_STAT_REPORT_PERIOD_MS,
                                                                                        NgnConfigurationEntry::DEFAULT_VIDEO_STAT_REPORT_PERIOD_MS);
        } else {
            m_interval = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AV_QOS_STAT_REPORT_PERIOD_MS,
                                                                                        NgnConfigurationEntry::DEF_AV_QOS_STAT_REPORT_PERIOD_MS);
        }
    }

    StopThread();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nLastReportTime = 0;
    
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_roomName = "";
    }
    
    if ( m_interval > 0) {
        TSK_DEBUG_INFO("StartThread OK");
        m_moniterThread = std::thread(&AVStatisticImpl::StatisticMonitering, this);
    }
}

void AVStatisticImpl::StopThread()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bMonitorAbort = true;
    
    if (m_moniterThread.joinable()) {
        m_monitorCond.SetSignal();
        if (m_moniterThread.get_id() == std::this_thread::get_id()) {
            m_moniterThread.detach();
        }
        else {
            m_moniterThread.join();
        }
        TSK_DEBUG_INFO("StopThread OK");
    }
    m_bMonitorAbort = false;
    m_monitorCond.Reset();
    
    std::lock_guard<std::mutex> lockData(m_dataMutex);
    ResetData();
}

void AVStatisticImpl::setInterval( int interval )
{
    m_interval = interval;
    
    if( m_interval <= 500 )
    {
        m_interval = 500;
    }
}

void AVStatisticImpl::setToClient( bool bClient, int reportCmd )
{
    m_bClient = bClient;
    m_nReportCmd = reportCmd;
}

void  AVStatisticImpl::NotifyVideoStat( const std::string& userID, bool bOn )
{
    //TSK_DEBUG_INFO("PinkyLog__:NotifyVideoStat:user:%s, bOn:%d", userID.c_str(), bOn );
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_mapUserVideoTimeData.find( userID );
    if( bOn ){
        if( iter == m_mapUserVideoTimeData.end() ){
            UserVideoTimeData data;
            data.strUserID = userID;
            m_mapUserVideoTimeData[userID] = data;
            iter = m_mapUserVideoTimeData.find( userID );
        }
        
        if( iter->second.m_startTimeMs == 0 ){
            iter->second.m_startTimeMs = tsk_time_now();
        }
    }
    else{
        //都没记录开始，结束个屁哟
        if( iter == m_mapUserVideoTimeData.end() ){
            return ;
        }
        
        if( iter->second.m_startTimeMs == 0 ){
            return ;
        }
     
        //记录有过关闭摄像头的行为
        m_mapVideoClose[ iter->second.m_sessionID ] = true;
        
        iter->second.m_totalTimeMs += tsk_time_now() - iter->second.m_startTimeMs ;
        iter->second.m_startTimeMs = 0;
    }
}

void AVStatisticImpl::NotifyUserName( int32_t sessionId, const std::string& userID )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_mapUserVideoTimeData.find( userID );
    if( iter == m_mapUserVideoTimeData.end() ){
        UserVideoTimeData data ;
        data.strUserID = userID;
        m_mapUserVideoTimeData[userID] = data;
        iter = m_mapUserVideoTimeData.find( userID );
    }
    
    iter->second.m_sessionID = sessionId ;
}

void AVStatisticImpl::NotifyStartVideo()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_nVideoBeginTime = tsk_time_now();
    
}

void AVStatisticImpl::NotifyGetRenderFrame()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    if( m_nVideoBeginTime != 0 )
    {
        m_nFirstFrameTime = tsk_time_now() - m_nVideoBeginTime;
        m_nVideoBeginTime = 0;
    }
}

void AVStatisticImpl::NotifyRoomName( const std::string& roomName,int selfSessionID, bool bEnter )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_roomName = roomName;
    m_nSessionId = selfSessionID;
}

int AVStatisticImpl::getStatInterval( bool bVideo , int32_t sessionId )
{
    if( !bVideo ){
        return m_nThisInterval;
    }
    else{
        for( auto iter = m_mapUserVideoTimeData.begin(); iter != m_mapUserVideoTimeData.end(); ++iter  )
        {
            if( iter->second.m_sessionID == sessionId ){
                int totalTime = iter->second.m_totalTimeMs;
                if( iter->second.m_startTimeMs != 0 ){
                    totalTime += tsk_time_now() - iter->second.m_startTimeMs;
                }
                
                if( totalTime == 0 ){
                    return m_nThisInterval;
                }
                else{
                    return totalTime;
                }
            }
        }
        
        return 0;
    }
}

void AVStatisticImpl::StatisticMonitering()
{
    while( true ) {
        m_monitorCond.WaitTime( m_interval );
        
        if( !m_bMonitorAbort ){
            m_nThisInterval = m_interval;
        }
        else{
            m_nThisInterval = tsk_time_now() - m_nLastReportTime;
        }
        
        int m_nTempLastReportTime = m_nLastReportTime;
        m_nLastReportTime = tsk_time_now();
        
        if (!m_nTempLastReportTime) {
            m_nTempLastReportTime = m_nLastReportTime;
        }

        int selfSession = CYouMeVoiceEngine::getInstance()->getSelfSessionID();
        
        std::map<std::string, UserVideoTimeData >  mapUserVideoTimeData;
        std::map<int,int > mapVideoBlockTime;
        std::map<int,int64_t > mapVideoPacketTime;
        std::map<int,int> mapVideoPacketCount;
        std::map<int, int> mapVideoDelay;
        std::vector<AVNotifyData> vecNotify;
        std::string strRoomName = "";
        int nUpAudioPacketTotal = 0, nUpAudioPacketLoss = 0;
        int nUpVideoPacketTotal = 0, nUpVideoPacketLoss = 0;

        //搜集所需要的通知，为了避免死锁，先收集，再通知
        {
            //音频码率
            std::lock_guard<std::mutex> lock(m_dataMutex);
            
            strRoomName = m_roomName;
            
            for( auto iter = m_mapAudioCodeCount.begin(); iter !=  m_mapAudioCodeCount.end(); ++iter )
            {
                int value = int(iter->second * 1000 / m_nThisInterval );
                
                vecNotify.push_back( AVNotifyData(YOUME_AVS_AUDIO_CODERATE,  iter->first, value) );
            }
            
            //视频码率
            for( auto iter = m_mapVideoCodeCount.begin(); iter !=  m_mapVideoCodeCount.end(); ++iter )
            {
                int statInterval = getStatInterval( true ,  iter->first );
                if( statInterval == 0 ){
                    statInterval = m_nThisInterval;
                }
                
                int value = int(iter->second * 1000 / statInterval);
                vecNotify.push_back( AVNotifyData(YOUME_AVS_VIDEO_CODERATE,  iter->first, value) );
            }
            
            //视频帧率
            for( auto iter = m_mapVideoFrameCount.begin(); iter !=  m_mapVideoFrameCount.end(); ++iter )
            {
                int statInterval = getStatInterval( true ,  iter->first );
                if( statInterval == 0 ){
                    statInterval = m_nThisInterval;
                }
                
                int value = int(iter->second * 1000 / statInterval );
                vecNotify.push_back( AVNotifyData(YOUME_AVS_VIDEO_FRAMERATE,  iter->first, value) );
            }

            //视频卡顿
            for( auto iter = m_mapVideoBlock.begin(); iter !=  m_mapVideoBlock.end(); ++iter )
            {
                int value = iter->second;
                vecNotify.push_back( AVNotifyData(YOUME_AVS_VIDEO_BLOCK,  iter->first,  value ) );
            }

            //下行带宽, 单位Bps
            int value = m_nRecvBitrateStat;
            vecNotify.push_back(AVNotifyData(YOUME_AVS_RECV_DATA_STAT, m_nSessionId, value));

#if AVSTATISTIC_USE_RTCP
            //音频上行丢包率
            for( auto iter = m_mapAudioUpPacketLoss_Rtcp.begin(); iter !=  m_mapAudioUpPacketLoss_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_AUDIO_PACKET_UP_LOSS_RATE ,  iter->first, iter->second) );
                
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Audio Up Lost Rate: %d _ %d", iter->first, iter->second );
                }
                iter->second = 0;
            }

            //音频下行丢包率
            for( auto iter = m_mapAudioDnPacketLoss_Rtcp.begin(); iter !=  m_mapAudioDnPacketLoss_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_AUDIO_PACKET_DOWN_LOSS_RATE ,  iter->first, iter->second) );
                
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Audio Down Lost Rate: %d _ %d", iter->first, iter->second );
                }
                iter->second = 0;
            }

            //视频上行丢包率
            for( auto iter = m_mapVideoUpPacketLoss_Rtcp.begin(); iter !=  m_mapVideoUpPacketLoss_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_VIDEO_PACKET_UP_LOSS_RATE ,  iter->first, iter->second) );
                
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Video Up Lost Rate: %d _ %d", iter->first, iter->second );
                }
                iter->second = 0;
            }

            //视频下行丢包率
            for( auto iter = m_mapVideoDnPacketLoss_Rtcp.begin(); iter !=  m_mapVideoDnPacketLoss_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_VIDEO_PACKET_DOWN_LOSS_RATE ,  iter->first, iter->second) );
                
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Video Down Lost Rate: %d _ %d", iter->first, iter->second );
                }
                iter->second = 0;
            }

            //音频延时
            for( auto iter = m_mapAudioNetDelay_Rtcp.begin(); iter !=  m_mapAudioNetDelay_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_AUDIO_DELAY_MS ,  iter->first, iter->second) );
               
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Audio Delay: %d _ %d", iter->first, iter->second );
                }
            }

            //视频延时
            for( auto iter = m_mapVideoNetDelay_Rtcp.begin(); iter !=  m_mapVideoNetDelay_Rtcp.end(); ++iter )
            {   
                vecNotify.push_back( AVNotifyData( YOUME_AVS_VIDEO_DELAY_MS ,  iter->first, iter->second) );
               
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Video Delay: %d _ %d", iter->first, iter->second );
                }
            }

            // 音频上行丢包率（半程）
            if( m_nSelfAudioPacketTotal > 0 )
            {
                int lossRate = m_nSelfAudioPacketLoss * 1000 / m_nSelfAudioPacketTotal;
                vecNotify.push_back( AVNotifyData(YOUME_AVS_AUDIO_PACKET_UP_LOSS_HALF ,  m_nSessionId, lossRate ) );
                
                if(!m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Audio Lost Rate(Self): %d _ %d", m_nSessionId, lossRate );
                }
            }

            // 视频上行丢包率（半程）
            if( m_nSelfVideoPacketTotal > 0 )
            {
                int lossRate = m_nSelfVideoPacketLoss * 1000 / m_nSelfVideoPacketTotal;
                vecNotify.push_back( AVNotifyData(YOUME_AVS_VIDEO_PACKET_UP_LOSS_HALF ,  m_nSessionId, lossRate ) );
                
                if( !m_bClient )
                {
                    TSK_DEBUG_INFO("AVStatistic:Video Lost Rate(Self): %d _ %d", m_nSessionId, lossRate );
                }
            }

            //共享视频码率
            for( auto iter = m_mapVideoShareCodeCount.begin(); iter !=  m_mapVideoShareCodeCount.end(); ++iter )
            {
                int statInterval = getStatInterval( true ,  iter->first );
                if( statInterval == 0 ){
                    statInterval = m_nThisInterval;
                }
                
                int value = int(iter->second * 1000 / statInterval);
                vecNotify.push_back( AVNotifyData(YOUME_AVS_VIDEO_SHARE_CODERATE,  iter->first, value) );
            }
#endif
            
            mapUserVideoTimeData = m_mapUserVideoTimeData;
            mapVideoBlockTime = m_mapVideoBlockTime;
            mapVideoPacketTime = m_mapVideoPacketTime;
            mapVideoDelay = m_mapNetDelay;
            
            nUpAudioPacketLoss = m_nSelfAudioPacketLoss;
            nUpAudioPacketTotal = m_nSelfAudioPacketTotal;

            nUpVideoPacketLoss = m_nSelfVideoPacketLoss;
            nUpVideoPacketTotal = m_nSelfVideoPacketTotal;
            
            ResetData();
        }
        
        if( m_bClient ){
            //客户端通知时间短，离开房间可以不报
            if( !m_bMonitorAbort ){
                for( auto it = vecNotify.begin(); it != vecNotify.end(); ++it )
                {
                    CYouMeVoiceEngine::getInstance()->sendCbMsgCallAVStatistic( it->m_type, it->m_sessionID, it->m_value);
                }
            }
        }
        else{
            std::map<int, UserVideoData > userDatas;
            GetReportServerData( userDatas, vecNotify , mapVideoBlockTime, mapVideoPacketTime, mapVideoPacketCount, mapVideoDelay, nUpAudioPacketLoss, nUpAudioPacketTotal, nUpVideoPacketLoss, nUpVideoPacketTotal);

            if (AVSTATIC_REPORT_SERVER_VIDEO_STAT == m_nReportCmd) {
                // 3004数据上报
                ReportToServer( mapUserVideoTimeData, userDatas , m_nSessionId , strRoomName);
            } else {
                // 3005数据上报
                ReportToServerAvqos(m_nLastReportTime - m_nTempLastReportTime, userDatas , m_nSessionId , strRoomName );
            }
        }
        
        //如果停止监控，要把当前数据上报了再退出
        if( m_bMonitorAbort ){
            return ;
        }
    }    
}

void GetReportServerData( std::map<int, UserVideoData >& userDatas , std::vector<AVNotifyData>& vecNotify ,
                         std::map<int, int>& mapVideoBlockTime, std::map<int, int64_t>& mapVideoPackageTime ,
                         std::map<int,int>& mapVideoPackageCount, std::map<int, int>& mapVideoNetDelay,
                         int nUpAudioPacketLoss, int nUpAudioPacketTotal, int nUpVideoPacketLoss, int nUpVideoPacketTotal)
{
    
    for( int i = 0 ; i < vecNotify.size(); i++ ){
        int sessionID = vecNotify[i].m_sessionID;
        auto iter =  userDatas.find( sessionID );
        if( iter == userDatas.end() ){
            UserVideoData  data ;
            data.m_sessionID = sessionID;
            userDatas[ sessionID] = data;
            iter = userDatas.find(sessionID);
            
            // 卡顿时间统计
            auto iterBlockTime = mapVideoBlockTime.find( sessionID );
            if( iterBlockTime != mapVideoBlockTime.end() )
            {
                iter->second.m_blockTime = iterBlockTime->second;
            }
            
            // 收包间隔统计
            auto iterVideoPackageTime = mapVideoPackageTime.find( sessionID );
            auto iterVideoPackageCount = mapVideoPackageCount.find( sessionID );
            if( iterVideoPackageTime != mapVideoPackageTime.end() && iterVideoPackageCount != mapVideoPackageCount.end() && iterVideoPackageCount->second != 0 )
            {
                iter->second.m_packetGapAvg = iterVideoPackageTime->second / iterVideoPackageCount->second ;
            }
            
            // rtt统计
            auto iterDelay = mapVideoNetDelay.find( sessionID );
            if( iterDelay != mapVideoNetDelay.end() )
            {
                iter->second.m_networkDelay = iterDelay->second;
            }

            // 音视频上行丢包率统计，仅针对本端（半程）
            iter->second.m_upLostCountAudio  = nUpAudioPacketLoss;
            iter->second.m_upTotalCountAudio = nUpAudioPacketTotal;
            iter->second.m_upLostCountVideo  = nUpVideoPacketLoss;
            iter->second.m_upTotalCountVideo = nUpVideoPacketTotal;
        }

        switch ( vecNotify[i].m_type) {
            case YOUME_AVS_AUDIO_CODERATE:
                iter->second.m_bitrateAudio  = vecNotify[i].m_value * 8 / 1000; //kbps
                break;
            case YOUME_AVS_VIDEO_CODERATE:
                iter->second.m_bitrateVideo  = vecNotify[i].m_value * 8 / 1000;//kbps
                break;
            case YOUME_AVS_VIDEO_FRAMERATE:
                iter->second.m_fps  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_AUDIO_PACKET_UP_LOSS_RATE:
                iter->second.m_lostrateAudio  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_AUDIO_PACKET_DOWN_LOSS_RATE:
                iter->second.m_lostrateAudio  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_VIDEO_PACKET_UP_LOSS_RATE:
                iter->second.m_lostrateVideo  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_VIDEO_PACKET_DOWN_LOSS_RATE:
                iter->second.m_lostrateVideo  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_AUDIO_DELAY_MS:
                iter->second.m_audioJitter  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_VIDEO_DELAY_MS:
                iter->second.m_networkDelay  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_VIDEO_BLOCK:
                iter->second.m_blockCount  = vecNotify[i].m_value ;
                break;
            case YOUME_AVS_VIDEO_SHARE_CODERATE:
                iter->second.m_bitrateShare  = vecNotify[i].m_value * 8 / 1000;//kbps
                break;
                
            default:
                break;
        }
    }
}

void AVStatisticImpl::ReportToServerAvqos(int reportPeriod, std::map<int, UserVideoData >& userDatas,
                                     int selfSessionID , std::string strRoomName )
{

    ReportService * report = ReportService::getInstance();
    youmeRTC::ReportAVQosStatisticInfo  avQosStat;

    avQosStat.business_id = 0;              // not use so far
    avQosStat.sdk_version = SDK_NUMBER;
    avQosStat.platform = NgnApplication::getInstance()->getPlatform();
    avQosStat.canal_id = NgnApplication::getInstance()->getCanalID();
    
    avQosStat.user_type = 0;    // 仅上报本端
    int reportInterval = reportPeriod;
    if (!reportPeriod) {
        reportInterval = m_nThisInterval;
    } 
    
    avQosStat.report_period = reportInterval;
    avQosStat.join_sucess_count = 1;
    avQosStat.join_count = 1;

    for ( auto iter = userDatas.begin(); iter != userDatas.end(); ++iter ) {
        if (selfSessionID == iter->first) {
            avQosStat.stream_id_def = 1;
            avQosStat.audio_up_lost_cnt = iter->second.m_upLostCountAudio;
            avQosStat.audio_up_total_cnt = iter->second.m_upTotalCountAudio;
            avQosStat.video_up_lost_cnt = iter->second.m_upLostCountVideo;
            avQosStat.video_up_total_cnt = iter->second.m_upTotalCountVideo;

            avQosStat.audio_up_bitrate = iter->second.m_bitrateAudio;
            avQosStat.video_up_camera_bitrate = iter->second.m_bitrateVideo;
            avQosStat.video_up_share_bitrate = iter->second.m_bitrateShare;

            avQosStat.video_up_fps = iter->second.m_fps;
        } else {
            if (avQosStat.audio_dn_lossrate < iter->second.m_lostrateAudio) {
                avQosStat.audio_dn_lossrate = iter->second.m_lostrateAudio;
            }

            if (avQosStat.video_dn_lossrate < iter->second.m_lostrateVideo) {
                avQosStat.video_dn_lossrate = iter->second.m_lostrateVideo;
            }

            if (avQosStat.video_rtt < iter->second.m_networkDelay) {
                avQosStat.video_rtt = iter->second.m_networkDelay;
                avQosStat.audio_rtt = avQosStat.video_rtt;
            }
            
            avQosStat.audio_dn_bitrate += iter->second.m_bitrateAudio;
            avQosStat.video_dn_camera_bitrate += iter->second.m_bitrateVideo;
            avQosStat.video_dn_share_bitrate += iter->second.m_bitrateShare;

            if (avQosStat.block_count < iter->second.m_blockCount) {
                avQosStat.block_count = iter->second.m_blockCount;
                avQosStat.block_time = iter->second.m_blockTime;
            }

            // 下行视频帧率取最小值
            if (!avQosStat.video_dn_fps || avQosStat.video_dn_fps > iter->second.m_fps) {
                avQosStat.video_dn_fps = iter->second.m_fps;
            }
        }
    }
            
    avQosStat.room_name = strRoomName;
    avQosStat.brand = NgnApplication::getInstance()->getBrand();
    avQosStat.model = NgnApplication::getInstance()->getModel();

    // report->report(avQosStat);
    TSK_DEBUG_INFO("doAVQosStatReport period:%u, stream:%x audioup:%u-%u, videoup:%u-%u, fps[%u-%u] rtt:%u, dn-loss:%u-%u, bitrate:up[%u-%u-%u]dn[%u-%u-%u], block[%u-%u]"
    , avQosStat.report_period, avQosStat.stream_id_def, avQosStat.audio_up_lost_cnt, avQosStat.audio_up_total_cnt
    , avQosStat.video_up_lost_cnt, avQosStat.video_up_total_cnt, avQosStat.video_up_fps, avQosStat.video_dn_fps, avQosStat.audio_rtt
    , avQosStat.audio_dn_lossrate , avQosStat.video_dn_lossrate, avQosStat.audio_up_bitrate, avQosStat.video_up_camera_bitrate
    , avQosStat.video_up_share_bitrate, avQosStat.audio_dn_bitrate, avQosStat.video_dn_camera_bitrate, avQosStat.video_dn_share_bitrate
    , avQosStat.block_count, avQosStat.block_time);
}

void AVStatisticImpl::ReportToServer(  std::map< std::string , UserVideoTimeData >& userTimeData,  std::map<int, UserVideoData >& userDatas,
                                     int selfSessionID , std::string strRoomName )
{
    for( auto iterTime = userTimeData.begin(); iterTime != userTimeData.end(); ++iterTime )
    {
        auto iter = userDatas.find( iterTime->second.m_sessionID );
        //这一轮这个userID没数据
        if( iter == userDatas.end() ){
            continue;
        }
        
        //不知道userID,没法上报
        if( iterTime->second.strUserID.empty() ){
            continue;
        }
        
        int reportPeriod = iterTime->second.m_totalTimeMs;
        if( iterTime->second.m_startTimeMs != 0 ){
            reportPeriod += tsk_time_now() - iterTime->second.m_startTimeMs;
        }
        //既然有收到数据，却不知道数据持续的时常，就当是整个周期吧
        if( reportPeriod == 0 ){
            reportPeriod = m_nThisInterval;
        }
        
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportVideoStatisticInfo  videoData;
        videoData.sdk_version = SDK_NUMBER;
        videoData.platform = NgnApplication::getInstance()->getPlatform();
        videoData.canal_id = NgnApplication::getInstance()->getCanalID();
        
        if( iter->second.m_sessionID == selfSessionID ){
            videoData.user_type = 0;
            videoData.firstFrameTime = m_nFirstFrameTime;
            
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_nFirstFrameTime = 0;
        }
        else{
            videoData.user_type = 1;
            videoData.firstFrameTime = 0;
        }
        videoData.user_name = iterTime->second.strUserID;
        
        videoData.fps = iter->second.m_fps;
        videoData.bit_rate = iter->second.m_bitrateVideo;
        videoData.lost_packet_rate =  iter->second.m_lostrateVideo;
        videoData.block_count =  iter->second.m_blockCount;
        videoData.report_period =  reportPeriod;       //上报的数据间隔比较长，直接用这个间隔影响会比较大，需要处理下
        videoData.room_name = strRoomName;
        
        videoData.brand = NgnApplication::getInstance()->getBrand();
        videoData.model = NgnApplication::getInstance()->getModel();
        
        videoData.block_time = iter->second.m_blockTime;
        videoData.networkDelayMs = iter->second.m_networkDelay;
        videoData.averagePacketTimeMs = iter->second.m_packetGapAvg;
        
//        TSK_DEBUG_INFO("PinkyLog__ReportToServer:sessionID:%d, user:%s, fps:%d, bitrate:%d, lost:%d, block:%d, time:%d, room:%s",
//                       iterTime->second.m_sessionID,
//                       videoData.user_name.c_str(),
//                       videoData.fps,
//                       videoData.bit_rate,
//                       videoData.lost_packet_rate,
//                       videoData.block_count,
//                       videoData.report_period,
//                       videoData.room_name.c_str() );
        
        report->report(videoData);
    }
}

void AVStatisticImpl::addAudioCode( int codeCount, int32_t sessionId  )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto iter = m_mapAudioCodeCount.find( sessionId );
    if( iter == m_mapAudioCodeCount.end() )
    {
        m_mapAudioCodeCount[ sessionId ] = 0;
    }
    
    m_mapAudioCodeCount[ sessionId ] += codeCount;
}

void AVStatisticImpl::addVideoCode( int codeCount , int32_t sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto iter = m_mapVideoCodeCount.find( sessionId );
    if( iter == m_mapVideoCodeCount.end() )
    {
        m_mapVideoCodeCount[ sessionId ] = 0;
    }
    
    m_mapVideoCodeCount[ sessionId ] += codeCount;

}

void AVStatisticImpl::addVideoShareCode( int codeCount , int32_t sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto iter = m_mapVideoShareCodeCount.find( sessionId );
    if( iter == m_mapVideoShareCodeCount.end() )
    {
        m_mapVideoShareCodeCount[ sessionId ] = 0;
    }
    
    m_mapVideoShareCodeCount[ sessionId ] += codeCount;

}

void AVStatisticImpl::addVideoFrame( int frameCount, int32_t sessionId  )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    if( sessionId == 0 )
    {
        sessionId = m_nSessionId;
    }
    
    auto iter = m_mapVideoFrameCount.find( sessionId );
    if( iter == m_mapVideoFrameCount.end() )
    {
        m_mapVideoFrameCount[ sessionId ] = 0;
    }
    
    m_mapVideoFrameCount[ sessionId ] += frameCount;
}

void addCount( std::map<int, int >& mapData,  int count , int32_t sessionId )
{
    auto iter = mapData.find( sessionId );
    if( iter == mapData.end() )
    {
        mapData[ sessionId ] = 0;
    }
    
    mapData[ sessionId ] += count;
}

void AVStatisticImpl::addVideoBlock( int count, int32_t sessionId )
{
    // 暂时废弃，block根据解码帧间延时来判断是否需要增加计数
    //blockCount 两帧间隔超过200毫秒，与blockTime对应
    
//    std::lock_guard<std::mutex> lock(m_dataMutex);
//
//    auto iter = m_mapVideoBlock.find( sessionId );
//    if( iter == m_mapVideoBlock.end() )
//    {
//        m_mapVideoBlock[ sessionId ] = 0;
//    }
//
//    m_mapVideoBlock[ sessionId ] += count;
}

void AVStatisticImpl::addAudioPacket( int seqNum, int32_t sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_mapAudioPacketLoss.find( sessionId );
    if( iter == m_mapAudioPacketLoss.end() )
    {
        m_mapAudioPacketLoss[ sessionId ] = AVSPacketRecvData();
    }
    
    auto data = m_mapAudioPacketLoss.find( sessionId );
    if( data == m_mapAudioPacketLoss.end() )
    {
        return ;
    }
    
    if( data->second.m_seq_min == -1 )
    {
        data->second.m_seq_min = seqNum;
    }
    
    if( data->second.m_seq_max != -1 )
    {
        //ID 发生会换了
        if( seqNum - data->second.m_seq_max < -60000 )
        {
            data->second.m_circleCount++;
            
        }
        //因为乱序，ID又回到回环之前
        else if( seqNum - data->second.m_seq_max > 60000 )
        {
            data->second.m_circleCount++;
        }
    }
    
    data->second.m_seq_max = seqNum;
    data->second.m_recv_count++;
}

void AVStatisticImpl::addVideoPacket( int seqNum, int32_t sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    auto iter = m_mapVideoPacketLoss.find( sessionId );
    if( iter == m_mapVideoPacketLoss.end() )
    {
        m_mapVideoPacketLoss[ sessionId ] = AVSPacketRecvData();
    }
    
    auto data = m_mapVideoPacketLoss.find( sessionId );
    if( data == m_mapVideoPacketLoss.end() )
    {
        return ;
    }
    
    if( data->second.m_seq_min == -1 )
    {
        data->second.m_seq_min = seqNum;
    }
    
    if( data->second.m_seq_max != -1 )
    {
        //ID 发生会换了
        if( seqNum - data->second.m_seq_max < -60000 )
        {
            data->second.m_circleCount++;
            
        }
        //因为乱序，ID又回到回环之前
        else if( seqNum - data->second.m_seq_max > 60000 )
        {
            data->second.m_circleCount++;
        }
    }
    
    data->second.m_seq_max = seqNum;
    data->second.m_recv_count++;
    
}

void AVStatisticImpl::addVideoPacketTimeGap( int timegap, int sessionId   )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto iter = m_mapVideoPacketTime.find( sessionId );
    if( iter == m_mapVideoPacketTime.end() )
    {
        m_mapVideoPacketTime[ sessionId ] = 0;
    }
    
    m_mapVideoPacketTime[ sessionId ] += timegap;
    
}

void AVStatisticImpl::addVideoPacketDelay( int timeDelay, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    auto iter = m_mapNetDelay.find( sessionId );
    if( iter == m_mapNetDelay.end() )
    {
        m_mapNetDelay[ sessionId ] = 0;
    }
    
    m_mapNetDelay[ sessionId ] = timeDelay;
}

void AVStatisticImpl::addSelfVideoPacket( int lost, int totalCount, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_nSelfVideoPacketTotal += totalCount;
    m_nSelfVideoPacketLoss += lost;
    m_nSessionId = sessionId;
}

void AVStatisticImpl::addSelfAudioPacket( int lost, int totalCount, int sessionId ){
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_nSelfAudioPacketTotal += totalCount;
    m_nSelfAudioPacketLoss += lost;
    m_nSessionId = sessionId;
}

void AVStatisticImpl::addVideoBlockTime( int time, int32_t sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    auto iter = m_mapVideoBlockTime.find( sessionId );
    if( iter == m_mapVideoBlockTime.end() )
    {
        m_mapVideoBlockTime[ sessionId ] = 0;
    }
    
    auto iterCount = m_mapVideoBlock.find( sessionId );
    if( iterCount == m_mapVideoBlock.end() )
    {
        m_mapVideoBlock[ sessionId ] = 0;
    }
    
    //如果这期间没关闭过视频数据
    auto iterClose = m_mapVideoClose.find( sessionId );
    if( iterClose == m_mapVideoClose.end() || iterClose->second == false )
    {
        //超过200毫秒才认为是卡顿
        if( time >= 200 )
        {
            m_mapVideoBlockTime[ sessionId ] += time;
            m_mapVideoBlock[ sessionId ] += 1;
        }
        
    }
    else{
        m_mapVideoClose[ sessionId ] = false;
    }
}

void AVStatisticImpl::setAudioUpPacketLossRtcp( int lost, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setAudioUpPacketLossRtcp:%d, sessionId:%d", lost, sessionId);
    auto iter = m_mapAudioUpPacketLoss_Rtcp.find( sessionId );
    if( iter == m_mapAudioUpPacketLoss_Rtcp.end() )
    {
        m_mapAudioUpPacketLoss_Rtcp[ sessionId ] = lost;
    }else {
        m_mapAudioUpPacketLoss_Rtcp[ sessionId ] = (int)(lost * 0.75 + m_mapAudioUpPacketLoss_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setAudioDnPacketLossRtcp( int lost, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setAudioDnPacketLossRtcp:%d, sessionId:%d", lost, sessionId);
    auto iter = m_mapAudioDnPacketLoss_Rtcp.find( sessionId );
    if( iter == m_mapAudioDnPacketLoss_Rtcp.end() )
    {
        m_mapAudioDnPacketLoss_Rtcp[ sessionId ] = lost;
    }else {
        m_mapAudioDnPacketLoss_Rtcp[ sessionId ] = (int)(lost * 0.75 + m_mapAudioDnPacketLoss_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setVideoUpPacketLossRtcp( int lost, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setVideoUpPacketLossRtcp:%d, sessionId:%d", lost, sessionId);
    auto iter = m_mapVideoUpPacketLoss_Rtcp.find( sessionId );
    if( iter == m_mapVideoUpPacketLoss_Rtcp.end() )
    {
        m_mapVideoUpPacketLoss_Rtcp[ sessionId ] = lost;
    }else {
        m_mapVideoUpPacketLoss_Rtcp[ sessionId ] = (int)(lost * 0.75 + m_mapVideoUpPacketLoss_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setVideoDnPacketLossRtcp( int lost, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setVideoDnPacketLossRtcp:%d, sessionId:%d", lost, sessionId);
    auto iter = m_mapVideoDnPacketLoss_Rtcp.find( sessionId );
    if( iter == m_mapVideoDnPacketLoss_Rtcp.end() )
    {
        m_mapVideoDnPacketLoss_Rtcp[ sessionId ] = lost;
    }else {
        m_mapVideoDnPacketLoss_Rtcp[ sessionId ] = (int)(lost * 0.75 + m_mapVideoDnPacketLoss_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setAudioPacketDelayRtcp( int delay, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setAudioPacketDelayRtcp:%d, sessionId:%d", delay, sessionId);
    auto iter = m_mapAudioNetDelay_Rtcp.find( sessionId );
    if( iter == m_mapAudioNetDelay_Rtcp.end() )
    {
        m_mapAudioNetDelay_Rtcp[ sessionId ] = delay;
    }else {
        m_mapAudioNetDelay_Rtcp[ sessionId ] = (int)(delay * 0.75 + m_mapAudioNetDelay_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setVideoPacketDelayRtcp( int delay, int sessionId )
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    // TSK_DEBUG_INFO("setVideoPacketDelayRtcp:%d, sessionId:%d", delay, sessionId);
    auto iter = m_mapVideoNetDelay_Rtcp.find( sessionId );
    if( iter == m_mapVideoNetDelay_Rtcp.end() )
    {
        m_mapVideoNetDelay_Rtcp[ sessionId ] = delay;
    }else {
        m_mapVideoNetDelay_Rtcp[ sessionId ] = (int)(delay * 0.75 + m_mapVideoNetDelay_Rtcp[ sessionId ] * 0.25);
    }
}

void AVStatisticImpl::setRecvDataStat( uint64_t recvStat)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    // TSK_DEBUG_INFO("setRecvDataStat old:%llu, new:%llu", m_nRecvBitrateStat, recvStat);

    // 取统计周期内的最大值, 在周期上报时会将m_nRecvBitrateStat复位
    if (m_nRecvBitrateStat < recvStat) {
        m_nRecvBitrateStat = recvStat;
    }
    
}

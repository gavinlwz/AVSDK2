#ifndef AVStatistic_h
#define AVStatistic_h

#include <stdio.h>
#include "YouMeCommon/XCondWait.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>

typedef enum AvstaticReportCmd
{
    AVSTATIC_REPORT_SERVER_VIDEO_STAT = 3004,
    AVSTATIC_REPORT_SERVER_AVQOS = 3005,
} AvstaticReportCmd; 

class ReportQuitData
{
public:
    static ReportQuitData *getInstance ();
    static void destroy();
    
    static ReportQuitData *sInstance;
    //统计
    int m_dns_count = 0;
    int m_valid_count = 0;
    int m_redirect_count = 0;
    int m_login_count = 0;
    int m_join_count = 0;
    int m_leave_count = 0;
    int m_kickout_count = 0;
    int m_uploadlog_count = 0;
};

class AVSPacketRecvData
{
public:
    int m_seq_max;
    int m_seq_min;
    int m_recv_count;
    
    int m_circleCount;  //seq可以用完又从0开始，
    
    AVSPacketRecvData();
};



struct UserVideoTimeData{
    std::string strUserID;
    int m_sessionID;
    uint64_t m_startTimeMs;//视频启动的时间
    int m_totalTimeMs;//视频已经持续的时间
    
    UserVideoTimeData()
    :strUserID(""),
    m_sessionID(0),
    m_startTimeMs(0),
    m_totalTimeMs(0){
        
    }
};

struct UserVideoData{
    int m_sessionID;
    int m_fps;          // 视频帧率统计
    int m_bitrateAudio; // 音频码率统计
    int m_bitrateVideo; // 视频码率统计(camera)
    int m_bitrateShare; // 视频码率统计(共享流)
    int m_lostrateAudio;// 音频丢包率
    int m_lostrateVideo;// 视频丢包率
    int m_blockCount;   // 解码卡顿次数统计
    int m_blockTime;    // 总共卡顿时长
    int m_packetGapAvg; // 收包间隔
    int m_audioJitter;  // 音频抖动(jitter)
    int m_networkDelay; // 平均延时(audio&video rtt)
    int m_upLostCountAudio; // 以下音视频上行丢包统计仅针对本端
    int m_upTotalCountAudio;
    int m_upLostCountVideo;
    int m_upTotalCountVideo;
    
    UserVideoData()
        :m_sessionID(0),
        m_fps(0),
        m_bitrateAudio(0),
        m_bitrateVideo(0),
        m_bitrateShare(0),
        m_lostrateAudio(0),
        m_lostrateVideo(0),
        m_blockCount(0),
        m_blockTime(0),
        m_packetGapAvg(0),
        m_audioJitter(0),
        m_networkDelay(0),
        m_upLostCountAudio(0),
        m_upTotalCountAudio(0),
        m_upLostCountVideo(0),
        m_upTotalCountVideo(0){
    }
};

void sendNotifyEvent( const char* typeName,  int value  );



class AVStatisticImpl
{
public:
    void StartThread();
    
    void StopThread();
    
    void StatisticMonitering();
    
    void addAudioCode( int codeCount, int32_t sessionId  );
    
    void addVideoCode( int codeCount, int32_t sessionId );
    void addVideoShareCode( int codeCount , int32_t sessionId );

    void addVideoFrame( int frameCount, int32_t sessionId );
    
    void addVideoBlock( int count, int32_t sessionId );
    
    void addAudioPacket( int seqNum, int32_t sessionId );
    void addVideoPacket( int seqNum, int32_t sessionId );
    
    void addVideoPacketTimeGap( int timegap, int sessionId   );
    void addVideoPacketDelay( int timeDelay, int sessionId );
    
    void addSelfVideoPacket( int lost, int totalCount, int sessionId );
    void addSelfAudioPacket( int lost, int totalCount, int sessionId );
    
    void addVideoBlockTime( int time, int32_t sessionId );
    
    void setAudioUpPacketLossRtcp( int lost, int sessionId );
    void setAudioDnPacketLossRtcp( int lost, int sessionId );
    void setVideoUpPacketLossRtcp( int lost, int sessionId );
    void setVideoDnPacketLossRtcp( int lost, int sessionId );
    void setAudioPacketDelayRtcp( int delay, int sessionId );
    void setVideoPacketDelayRtcp( int delay, int sessionId );
    
    void setRecvDataStat( uint64_t recvStat);
    void setInterval( int interval );
    void setToClient( bool bClient, int reportCmd );
    
    //获取统计的时间间隔，视频仔细计算的，音频就算统计时间间隔
    int getStatInterval( bool bVideo , int32_t sessionId );
    //通知视频开关
    void  NotifyVideoStat( const std::string& userID, bool bOn );
    
    //通知userID ,不能上报的时候才获取，不然，人家离开了可能拿不到啊
    void NotifyUserName( int32_t sessionId, const std::string& userID );
    
    void NotifyRoomName( const std::string& roomName, int selfSessionID, bool bEnter );
    
    void NotifyStartVideo();
    void NotifyGetRenderFrame();
        
    AVStatisticImpl();
    ~AVStatisticImpl();
private:
    void ResetData();
    void ReportToServer( std::map< std::string , UserVideoTimeData >& userTimeData,std::map<int, UserVideoData >& userDatas, int selfSessionID, std::string strRoomName);
    void ReportToServerAvqos(int reportPeriod, std::map<int, UserVideoData >& userDatas, int selfSessionID , std::string strRoomName );

    std::mutex m_mutex;
    std::mutex m_dataMutex;
    
    uint32_t m_interval;
    bool  m_bClient = false;
    int   m_nReportCmd = 0; // 数据上报命令字，仅用于server数据上报，目前使用到的是3004和3005
    
    std::map<int, int64_t > m_mapVideoFrameCount;       // for video fps stat
    std::map<int, int64_t > m_mapVideoCodeCount;        // for camera video bitrate stat
    std::map<int, int64_t > m_mapVideoShareCodeCount;   // for share video bitrate stat

    std::map<int, int64_t > m_mapAudioCodeCount;        // for audio bitrate stat
    std::map<int, int64_t> m_mapVideoPacketTime;
    
    std::map<int, int > m_mapVideoBlock;
    std::map<int, int > m_mapVideoBlockTime;
    
    std::map<int, bool > m_mapVideoClose;
    
    std::map<int, int> m_mapNetDelay;
    
    std::map<int, AVSPacketRecvData > m_mapAudioPacketLoss;     // for audio halfway lossrate
    std::map<int, AVSPacketRecvData > m_mapVideoPacketLoss;     // for video halfway lossrate

    std::map<int, int> m_mapAudioUpPacketLoss_Rtcp;             // for audio full up lossrate
    std::map<int, int> m_mapAudioDnPacketLoss_Rtcp;             // for audio full dn lossrate
    std::map<int, int> m_mapVideoUpPacketLoss_Rtcp;             // for video full up lossrate
    std::map<int, int> m_mapVideoDnPacketLoss_Rtcp;             // for video full up lossrate
    std::map<int, int> m_mapAudioNetDelay_Rtcp;
    std::map<int, int> m_mapVideoNetDelay_Rtcp;
    
    bool m_bMonitorAbort;
    
    std::thread m_moniterThread;
    
    int m_nSelfVideoPacketTotal;
    int m_nSelfVideoPacketLoss;
    int m_nSessionId;       //记录的自己的sessionId ,有视频过来的上传丢包率时才有
    
    uint64_t m_nVideoBeginTime;  //内部采集是调用开启摄像头的时间，外部采集是第一次inputVideo的时间
    int m_nFirstFrameTime;  //从m_nVideoBeginTime到获得第一帧数据回调的时间
    
    int m_nSelfAudioPacketTotal;
    int m_nSelfAudioPacketLoss;

    uint64_t m_nRecvBitrateStat;    //下行带宽统计，统计单位间隔内的平均带宽

    std::map< std::string , UserVideoTimeData >  m_mapUserVideoTimeData;
    
    uint64_t m_nLastReportTime;  //离开房间的时候要上报一次，得知道上报的时间间隔
    int m_nThisInterval;        //这次上报离上次上报的时间，如果正常，就是interval,如果是离开房间，是当前时间减去上次上报时间
    
    std::string m_roomName;
    
    
    youmecommon::CXCondWait m_monitorCond;
    
};

class AVStatistic{
public:
    static AVStatistic *getInstance ();
    static void destroy();
    
    void StartThread();
    
    void StopThread();
    
    AVStatistic();
    
    void addAudioCode( int codeCount, int32_t sessionId  );
    
    void addVideoCode( int codeCount, int32_t sessionId );
    void addVideoShareCode( int codeCount, int32_t sessionId );

    void addVideoFrame( int frameCount, int32_t sessionId );
    
    void addVideoBlock( int count, int32_t sessionId );
    void addVideoBlockTime( int time, int32_t sessionId );
    
    void addAudioPacket( int seqNum, int32_t sessionId );
    void addVideoPacket( int seqNum, int32_t sessionId );
    
    void addVideoPacketTimeGap( int timegap, int sessionId   );
    void addVideoPacketDelay( int timeDelay, int sessionId );
    
    void addSelfVideoPacket( int lost, int totalCount, int sessionId );
    void addSelfAudioPacket( int lost, int totalCount, int sessionId );

    void setAudioUpPacketLossRtcp( int lost, int sessionId );
    void setAudioDnPacketLossRtcp( int lost, int sessionId );
    void setVideoUpPacketLossRtcp( int lost, int sessionId );
    void setVideoDnPacketLossRtcp( int lost, int sessionId );
    void setAudioPacketDelayRtcp( int delay, int sessionId );
    void setVideoPacketDelayRtcp( int delay, int sessionId );
    void setRecvDataStat( uint64_t recvStat);

    void setInterval( int interval );
    //通知视频开关
    void  NotifyVideoStat( const std::string& userID, bool bOn );
    void NotifyUserName( int32_t sessionId, const std::string& userID );
    void NotifyRoomName( const std::string& roomName, int selfSessionID, bool bEnter );
    
    void NotifyStartVideo();
    void NotifyGetRenderFrame();
    
private:
    AVStatisticImpl  m_client;          // 用于回调通知给客户端上层
    AVStatisticImpl  m_server;          // 用于上报给server，3004
    AVStatisticImpl  m_server_avqos;    // 用于上报给server，3005
    static AVStatistic *sInstance;
};


#endif /* AVStatistic_hpp */

/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.3.10
 *  说明:对外发布接口
 ******************************************************************/

#ifndef TINYDAV_YOUME_VIDEO_JITTER_H_
#define TINYDAV_YOUME_VIDEO_JITTER_H_

#include "tsk_time.h"
#include "mem_pool.h"
#include <deque>
#include <memory>
#include <set>

class RTPVideoPacket
{
    
public:
    unsigned char *pData;
    unsigned short nLen;
    unsigned short nSeq;
    unsigned int timestamp;
    uint32_t flag; // "bit-wise or" of packet attributes, such as "if this packet is decoded from FEC"
    RTPVideoPacket ()
    {
        pData = NULL;
        nLen = 0;
        nSeq = 0;
        timestamp = 0;
        flag = 0;
    }
};


class YOUMEVideoJitter
{
public:
    enum RESULT
    {
        SUCCESS = 0,
        BUFFERING,
        BAD_PACKET,
        BUFFER_OVERFLOW,
        BUFFER_EMPTY,
        DROPPED_PACKET
    };
    
    
    YOUMEVideoJitter (const unsigned depth, unsigned int ptime = 50);
    ~YOUMEVideoJitter ();
    
    void init (const unsigned depth, unsigned int ptime = 50);
    RESULT push (RTPVideoPacket packet);
    RESULT pop (RTPVideoPacket &packet);
    RESULT reset ();
    void set_depth (const unsigned ms_depth, const unsigned max_depth = 0);
    int get_depth ();
    int get_depth_ms ();
    int get_nominal_depth ();
    bool buffering ()
    {
        return m_is_buffering;
    }
    void eot_detected ();
    
    int overflow_count ()
    {
        return m_stats.overflow_count;
    }
    int out_of_order_count ()
    {
        return m_stats.ooo_count;
    }
    int underflow_count ()
    {
        return m_stats.underflow_count;
    }
    unsigned int jitter ()
    {
        return (unsigned int)m_stats.jitter;
    }
    unsigned int max_jitter ()
    {
        return (unsigned int)m_stats.max_jitter;
    }
    //MemPool m_mem_pool;
    
private:
    bool isReverse (unsigned int front, unsigned int back);
    
private:
    static const int DEFAULT_BUFFER_ELEMENTS = 10; // 按照fps 20计算，50ms一个包，共500ms
    static const int DEFAULT_MS_PER_PACKET = 50;
    
    std::deque<RTPVideoPacket> m_buffer;
    unsigned short m_nominal_depth_ms;    // 缓存深度，会动态调整
    unsigned short m_max_buffer_depth;    // 最大缓存深度，以ms为单位
    unsigned short m_payload_sample_rate; // 采样率
    unsigned short m_first_buf_sequence;  // 队列头序列号，下一次取数据的序列号
    unsigned short m_last_buf_sequence;   // 队列尾序列号，最新的数据放在队尾
    unsigned short m_last_pop_sequence;   // 最后一次取出的数据的序列号
    bool m_is_buffering;                  // 正在缓存中，不能取数据
    unsigned short m_ptime;
    unsigned long m_buffering_timestamp; // 启动缓存的时间戳
    
    struct stats
    {
        uint32_t ooo_count;              // 乱序包的个数
        uint32_t underflow_count;        // 取数据时队列为空或数据不够（即下溢）的次数
        uint32_t overflow_count;         // 放数据时队列为满（即上溢）的次数
        uint32_t cont_underflow_count;   // 连续下溢的次数
        uint32_t cont_overflow_count;    // 连续上溢的次数
        double jitter;
        double max_jitter;
        unsigned int prev_arrival;
        unsigned int prev_transit;
        unsigned long prev_rx_timestamp;
        int conversion_factor_timestamp_units;
    } m_stats;
    
    void StatusLossPacket ();
    
    void _clean_buffer ();
    
    void _calc_jitter (RTPVideoPacket packet);
    void _reset_buffer_stats ();
    
    
    unsigned short m_iRecvCount; //一秒钟之内实际接收的个数
    unsigned short m_iMinSeq;    //一秒钟之内接收到包的最小序号
    unsigned short m_iMaxSeq;    //一秒钟之内接收到包的最大序号
    
    uint64_t m_ulNowTime;
    
    uint32_t m_fecPacketCount;
    uint32_t m_plcPacketCount;
};


#endif

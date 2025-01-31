﻿
#ifndef __YOUME_JITTER_H__
#define __YOUME_JITTER_H__

#include "mem_pool.h"
#include "stdinc.h"
#include "tsk_time.h"
#include <deque>
#include <memory>
#include <set>
class RTPPacket
{

public:
    unsigned char *pData;
    unsigned short nLen;
    unsigned short nSeq;
    unsigned int timestamp;
    uint32_t flag; // "bit-wise or" of packet attributes, such as "if this packet is decoded from FEC"
    RTPPacket ()
    {
        pData = NULL;
        nLen = 0;
        nSeq = 0;
        timestamp = 0;
        flag = 0;
    }
};


class YOUMEJitter
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


    YOUMEJitter (const unsigned depth, const unsigned int sample_rate = 16000, unsigned int ptime = 20);
    ~YOUMEJitter ();

    void init (const unsigned depth, const unsigned int sample_rate = 16000, unsigned int ptime = 20);
    RESULT push (RTPPacket packet);
    RESULT pop (RTPPacket &packet);
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
    static const int DEFAULT_BUFFER_ELEMENTS = 18; // 20ms一个包，共360ms
    static const int DEFAULT_MS_PER_PACKET = 20;

    std::deque<RTPPacket> m_buffer;
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

    void _calc_jitter (RTPPacket packet);
    void _reset_buffer_stats (const unsigned int sample_rate);


    unsigned short m_iRecvCount; //一秒钟之内实际接收的个数
    unsigned short m_iMinSeq;    //一秒钟之内接收到包的最小序号
    unsigned short m_iMaxSeq;    //一秒钟之内接收到包的最大序号

    uint64_t m_ulNowTime;
    
    uint32_t m_fecPacketCount;
    uint32_t m_plcPacketCount;
};

#endif

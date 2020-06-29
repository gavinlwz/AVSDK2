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

#ifndef __YOUME_VIDEO_JITTER_H__
#define __YOUME_VIDEO_JITTER_H__

//#include <sys/time.h>
#include "tsk_time.h"
#include "tsk_debug.h"
#include "tsk_thread.h"
#include "tsk_list.h"
#include "tinydav/video/jb/tdav_video_jb.h"

#include <set>
//using namespace std;

typedef tsk_list_t tdav_video_frames_L_t;

typedef enum video_jitter_cb_data_type_e
{
    video_jitter_cb_data_type_rtp,
    video_jitter_cb_data_type_fl, // frame lost
    video_jitter_cb_data_type_tmfr, // too many frames removed
    video_jitter_cb_data_type_fdd, // average frame decoding duration
    video_jitter_cb_data_type_fps_changed, // fps changed, detection done using the timestamp
}
video_jitter_cb_data_type_t;

typedef struct video_jitter_cb_data_xs
{
    video_jitter_cb_data_type_t type;
    uint32_t ssrc;
    const void* usr_data;
    union{
        struct{
            const struct trtp_rtp_packet_s* pkt;
        }rtp;
        struct{
            uint16_t seq_num;
            tsk_size_t count;
            uint32_t sessionid;
            uint64_t goptime;
        }fl;
        struct{
            uint32_t x_dur; // expected duration in milliseconds
            uint32_t a_dur; // actual duration in milliseconds
        }fdd;
        struct{
            uint32_t old_fps;
            uint32_t new_fps;
        }fps;
        struct{
             int sessionid;
             int videoid;
        }avpf;
    };
}
video_jitter_cb_data_xt;

#define TDAV_VIDEO_JITTER_NACK(self) ((video_jitter_nack_t*)(self))

typedef struct video_jitter_nack_s 
{
    TSK_DECLARE_OBJECT;
    
    uint16_t video_id;          // 视频流ID，0:camera 大流，1:camera小流，2:共享流
    uint8_t frame_type;
    // uint16_t seq_count;         // nack list中需要重传的packet count
    uint32_t timestamp;         // 根据timestamp做同一帧数据包分类，提高查找和删除效率
    uint64_t insert_timestamp;  // 插入nack list时的本地时间戳，根据这个来做nacklist清理
    uint64_t request_count;
    std::set<uint16_t> *nack_list;

    TSK_DECLARE_SAFEOBJ;
} video_jitter_nack_t;

typedef tsk_list_t tdav_video_nack_L_t;

typedef int (*video_jitter_cb_f)(const video_jitter_cb_data_xt* data);
typedef int (*video_jitter_nack_cb_f)(void* usr_data, uint32_t ssrc_media_source, const tdav_video_nack_L_t* data);

class YoumeVideoJitter
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
    
    YoumeVideoJitter ();
    ~YoumeVideoJitter ();
    
    int32_t m_fps;
    int32_t m_fps_prob; // counter, 隔多长时间更新等待超时时间
    int32_t m_avg_duration;
    int32_t m_rate; // in Khz
    uint32_t m_last_timestamp;
    int32_t m_conseq_frame_drop;
    int32_t m_tail_max;
    tdav_video_frames_L_t *m_frames;
    
    tsk_size_t m_latency_start;
    tsk_size_t m_latency_min;
    tsk_size_t m_latency_max;
    uint32_t m_decode_last_timestamp;
    uint32_t m_decode_last_seq_num_with_mark;
    
    video_jitter_cb_f m_callback;
    const void* m_callback_data;
    
    // to avoid locking use different cb_data
    video_jitter_cb_data_xt cb_data_rtp;
    video_jitter_cb_data_xt cb_data_fdd;
    video_jitter_cb_data_xt cb_data_any;

    // Function
    void setCallback(video_jitter_cb_f callback);
    
    // Mutex
    tsk_mutex_handle_t *videoJitterMutex;

    //nack callback info
    void*  cb_data_nack;
    video_jitter_nack_cb_f m_nack_callback;

    tdav_video_nack_L_t* m_nack_list;   // nack seq list
    uint64_t m_last_frame_timestamp;
    uint16_t m_last_pkt_seq;
    uint16_t m_last_frame_type;

    // nack list fuction
    void setNACKCallback(video_jitter_nack_cb_f callback);
    void insert_seq_to_nack_list(uint16_t seq_num, uint32_t timestamp, uint8_t frame_type, uint16_t video_id);
    int remove_seq_from_nack_list(uint16_t seq_num, uint32_t timestamp, uint16_t video_id);
    void update_nack_list(uint64_t local_timestamp, int clear);    // 清理nack seq list, 将超时的seq从列表中清除，防止重传风暴

    //for test only
    void print_nack_list();
};


#if 0
YOUMEVideoJitter::YOUMEVideoJitter(const unsigned depth, unsigned int ptime)
{
    m_iRecvCount = 0;  //一秒钟之内实际接收的个数
    m_iMinSeq = 65535; //一秒钟之内接收到包的最小序号
    m_iMaxSeq = 0;     //一秒钟之内接收到包的最大序号
    m_fecPacketCount = 0;
    m_plcPacketCount = 0;
    
    m_ulNowTime = 0;
    init(depth, ptime);
}

YOUMEVideoJitter::~YOUMEVideoJitter()
{
    _clean_buffer();
}


void YOUMEVideoJitter::init(const unsigned depth, unsigned int ptime)
{
    if (!m_buffer.empty())
    {
        _clean_buffer();
    }
    _reset_buffer_stats();
    
    m_ulNowTime = tsk_time_now();
    m_first_buf_sequence = 0;
    m_last_buf_sequence = 0;
    m_last_pop_sequence = 0;
    TSK_DEBUG_INFO("init:first=%d,last=%d,last_pop=%d", m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
    m_ptime = ptime;
    set_depth(depth);
    m_payload_sample_rate = 0;
    
    m_is_buffering = true;
    m_buffering_timestamp = 0;
}

void YOUMEVideoJitter::StatusLossPacket()
{

    int iTotalSize = m_iMaxSeq - m_iMinSeq + 1;

    if (0 == m_iRecvCount)
    {
        TSK_DEBUG_INFO("Not received any pakcet");
        return;
    }
    if (1 == m_iRecvCount)
    {
        TSK_DEBUG_INFO("Only one packet");
        return;
    }
    //序号反转的情况忽视。按照每秒20个包，可以支持50分钟不反转
    
    TSK_DEBUG_INFO("Packets received:%d packets expected:%d  lossRate:%0.2f%%,buffsize=%d,first=%d,last=%d,last_pop=%d,fec=%d,plc=%d",
                   m_iRecvCount, iTotalSize, (float)(iTotalSize - m_iRecvCount) / iTotalSize * 100, m_buffer.size(), m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence,
                   m_fecPacketCount, m_plcPacketCount);
    if (iTotalSize != m_iRecvCount)
    {
        TSK_DEBUG_WARN("PacketLoss: Total%d received:%d", iTotalSize, m_iRecvCount);
    }
}

YOUMEVideoJitter::RESULT YOUMEVideoJitter::push(RTPVideoPacket packet)
{
    RESULT rc = SUCCESS;
    
    if (packet.pData != NULL)
    {
        uint64_t ulNow = tsk_time_now();
        if (ulNow - m_ulNowTime >= 1000)
        {
            StatusLossPacket();
            m_iRecvCount = 0;
            m_iMinSeq = 65535;
            m_iMaxSeq = 0;
            m_ulNowTime = ulNow;
            m_fecPacketCount = 0;
            m_plcPacketCount = 0;
        }
        m_iRecvCount++;
        if (RTP_PACKET_FLAG_CONTAINS(packet.flag, RTP_PACKET_FLAG_FEC)) {
            m_fecPacketCount++;
        }
        else if (RTP_PACKET_FLAG_CONTAINS(packet.flag, RTP_PACKET_FLAG_PLC)) {
            m_plcPacketCount++;
        }
        if (packet.nSeq > m_iMaxSeq)
        {
            m_iMaxSeq = packet.nSeq;
        }
        if (packet.nSeq < m_iMinSeq)
        {
            m_iMinSeq = packet.nSeq;
        }
        
        if ((unsigned long)(m_buffer.size() * m_ptime) >= m_max_buffer_depth)
        {
            rc = BUFFER_OVERFLOW;
            m_stats.overflow_count++;
            m_stats.cont_overflow_count++;
            
            RTPVideoPacket &old_packet = m_buffer.front();
            //m_mem_pool.free (old_packet.pData);
            if (old_packet.pData)
            {
                delete[] old_packet.pData;
                old_packet.pData = NULL;
            }
            
            m_last_pop_sequence = old_packet.nSeq;
            m_first_buf_sequence = m_buffer.front().nSeq;
            m_buffer.pop_front();
            
            if ((1 == m_stats.cont_overflow_count) || ((m_stats.cont_overflow_count % 100) == 0)) {
                TSK_DEBUG_INFO("buffer overflow: buf_depth=%lu(ms), cur_packet=%d, first=%d, last=%d, last_pop=%d",
                               (m_buffer.size() + 1) * m_ptime, packet.nSeq,
                               m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
            }
        }
        else if (m_stats.cont_overflow_count > 0) {
            TSK_DEBUG_INFO("buffer overflow end: overflow_count=%d, cont_overflow_count=%d, first=%d,last=%d,last_pop=%d",
                           m_stats.overflow_count, m_stats.cont_overflow_count,
                           m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
            m_stats.cont_overflow_count = 0;
        }
        
        if (m_is_buffering && (m_buffering_timestamp == 0))
        {
            struct timeval now;
            tsk_gettimeofday(&now, NULL);
            m_buffering_timestamp = ((long)now.tv_sec) * 1000 + (long)now.tv_usec / 1000;
        }
        _calc_jitter(packet);
        
        //判断是否乱序
        bool bReverse = isReverse(m_last_buf_sequence, packet.nSeq);
        //如果后来的包序号比最后一个包需要小并且没有发生序号反转，一定是乱序了
        if (m_buffer.empty())
        {
            m_buffer.push_back(packet);
            m_last_buf_sequence = packet.nSeq;
            
            m_first_buf_sequence = packet.nSeq;
            m_last_pop_sequence = packet.nSeq - 1;
        }
        else if ((packet.nSeq < m_last_buf_sequence) && false == bReverse)
        {
            TSK_DEBUG_WARN("jitter.push(): out of order packet #%d", packet.nSeq);
            ++m_stats.ooo_count;
            if (bReverse)
            {
                m_buffer.push_back(packet);
            }
            else
            {
                
                if (packet.nSeq < (m_first_buf_sequence - 1))
                {
                    rc = BAD_PACKET;
                }
                else
                {
                    for (deque<RTPVideoPacket>::reverse_iterator i = m_buffer.rbegin(); i != m_buffer.rend(); ++i)
                    {
                        RTPVideoPacket item = *i;
                        if (packet.nSeq < item.nSeq)
                        {
                            deque<RTPVideoPacket>::iterator it(i.base());
                            m_buffer.insert(it, packet);
                            break;
                        }
                    }
                    TSK_DEBUG_INFO("insert:first=%d,last=%d,last_pop=%d", m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
                }
            }
        }
        else
        {
            m_buffer.push_back(packet);
            m_last_buf_sequence = packet.nSeq;
        }
    }
    else
    {
        rc = BAD_PACKET;
    }
    return rc;
}


YOUMEVideoJitter::RESULT YOUMEVideoJitter::pop(RTPVideoPacket &packet)
{
    RTPVideoPacket bp;
    if (m_buffer.empty()) {
        // enter buffering state
        m_is_buffering = true;
    }
    else if (m_is_buffering) {
        // check if we can exit from the buffering state
        struct timeval now;
        tsk_gettimeofday(&now, NULL);
        unsigned long ulNow = ((long)now.tv_sec) * 1000 + (long)now.tv_usec / 1000;
        unsigned long buffer_time = ulNow - m_buffering_timestamp;
        if ((buffer_time >= m_nominal_depth_ms) || (m_buffer.size() * m_ptime >= m_nominal_depth_ms))
        {
            m_is_buffering = false;
            m_buffering_timestamp = 0;
        }
    }
    
    if (m_is_buffering) {
        m_stats.underflow_count++;
        m_stats.cont_underflow_count++;
        // Print the underflow packet information, for the first one, and every 2 seconds once.
        if ((1 == m_stats.cont_underflow_count) || ((m_stats.cont_underflow_count % 250) == 0)) {
            TSK_DEBUG_INFO("BUFFERING: first=%d,last=%d,last_pop=%d", m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
        }
        return YOUMEVideoJitter::BUFFERING;
    }
    else if (m_stats.cont_underflow_count > 0) {
        TSK_DEBUG_INFO("BUFFERING end: first=%d,last=%d,last_pop=%d, underflow_count:%d, cont_underflow_count:%d",
                       m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence,
                       m_stats.underflow_count, m_stats.cont_underflow_count);
        m_stats.cont_underflow_count = 0;
    }
    
    bp = m_buffer.front();
    
    if (m_last_pop_sequence == (m_first_buf_sequence - 1) || isReverse(m_last_pop_sequence, m_first_buf_sequence))
    {
        packet = bp;
        m_buffer.pop_front();
        m_last_pop_sequence = packet.nSeq;
        
        if (m_buffer.empty())
        {
            m_first_buf_sequence = m_last_pop_sequence + 1;
        }
        else
        {
            bp = m_buffer.front();
            m_first_buf_sequence = bp.nSeq;
        }
        return YOUMEVideoJitter::SUCCESS;
    }
    else
    {
        ++m_last_pop_sequence;
        TSK_DEBUG_INFO("miss packet: first=%d,last=%d,last_pop=%d", m_first_buf_sequence, m_last_buf_sequence, m_last_pop_sequence);
        return YOUMEVideoJitter::DROPPED_PACKET;
    }
}


YOUMEVideoJitter::RESULT YOUMEVideoJitter::reset()
{
    _clean_buffer();
    return SUCCESS;
}


void YOUMEVideoJitter::set_depth(const unsigned ms_depth, const unsigned max_depth)
{
    
    m_nominal_depth_ms = ms_depth;
    if (max_depth >= ms_depth)
    {
        m_max_buffer_depth = max_depth;
    }
    else
    {
        m_max_buffer_depth = (m_nominal_depth_ms * 4);
    }
    //m_mem_pool.init (640, m_max_buffer_depth / m_ptime + 1);
}

int YOUMEVideoJitter::get_depth()
{
    return (int)(m_buffer.size());
}


int YOUMEVideoJitter::get_depth_ms()
{
    return (int)m_buffer.size() * m_ptime;
}

int YOUMEVideoJitter::get_nominal_depth()
{
    return m_nominal_depth_ms;
}


void YOUMEVideoJitter::eot_detected()
{
    m_first_buf_sequence = 0;
    m_last_buf_sequence = 0;
    m_last_pop_sequence = 0;
}


void YOUMEVideoJitter::_calc_jitter(RTPVideoPacket packet)
{
    unsigned int arrival;
    int d;
    unsigned int transit;
    
    struct timeval now;
    tsk_gettimeofday(&now, NULL);
    unsigned long ulNow = ((long)now.tv_sec) * 1000 + (long)now.tv_usec / 1000;
    int interarrival_time_ms = (int)(ulNow - m_stats.prev_rx_timestamp);
    
    arrival = interarrival_time_ms * m_stats.conversion_factor_timestamp_units;
    if (m_stats.prev_arrival == 0)
    {
        arrival = packet.timestamp;
    }
    else
    {
        arrival += m_stats.prev_arrival;
    }
    
    m_stats.prev_arrival = packet.timestamp;
    
    transit = arrival - packet.timestamp;
    d = transit - m_stats.prev_transit;
    m_stats.prev_transit = transit;
    if (d < 0)
    {
        d = -d;
    }
    
    m_stats.jitter += (1.0 / 16.0) * ((double)d - m_stats.jitter);
    
    m_stats.prev_rx_timestamp = ulNow;
    if (m_stats.max_jitter < m_stats.jitter)
    {
        m_stats.max_jitter = m_stats.jitter;
    }
}


void YOUMEVideoJitter::_clean_buffer()
{
    TSK_DEBUG_INFO("clear jitbuffer memery,buffer size=%d", (int)m_buffer.size());
    for (deque<RTPVideoPacket>::iterator it = m_buffer.begin(); it != m_buffer.end(); it++)
    {
        if (it->pData)
        {
            delete[] it->pData;
            it->pData = NULL;
        }
    }
    
    m_buffer.clear();
}

void YOUMEVideoJitter::_reset_buffer_stats()
{
    m_stats.ooo_count = 0;
    m_stats.underflow_count = 0;
    m_stats.overflow_count = 0;
    m_stats.cont_underflow_count = 0;
    m_stats.cont_overflow_count = 0;
    m_stats.jitter = 0.0;
    m_stats.max_jitter = 0.0;
    m_stats.prev_arrival = 0;
    m_stats.prev_transit = 0;
    m_stats.prev_rx_timestamp = 0;
    m_stats.conversion_factor_timestamp_units = 168;
}

bool YOUMEVideoJitter::isReverse(unsigned int front, unsigned int back)
{
    if (front < back && ((back - front) < 1000))
    {
        return false;
    }
    else
    {
        return true;
    }
}
#endif
#endif

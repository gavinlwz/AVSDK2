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

//#include <sys/time.h>
#include "tsk_time.h"
#include "tsk_debug.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"

#include "tinydav/video/jb/youme_video_jitter.h"

// default frame rate
// the corret fps will be computed using the RTP timestamps
#define TDAV_VIDEO_JB_FPS		TDAV_VIDEO_JB_FPS_MAX
#define TDAV_VIDEO_JB_FPS_MIN	10
#define TDAV_VIDEO_JB_FPS_MAX	120
// Number of correct consecutive video frames to receive before computing the FPS
#define TDAV_VIDEO_JB_FPS_PROB	(TDAV_VIDEO_JB_FPS << 1)
// Maximum gap allowed (used to detect seqnum wrpping)
#define TDAV_VIDEO_JB_MAX_DROPOUT		0xFD9B

#define TDAV_VIDEO_JB_TAIL_MAX_LOG2		1
#if TDAV_UNDER_MOBILE /* to avoid too high memory usage */
#	define TDAV_VIDEO_JB_TAIL_MAX		40
#else
#	define TDAV_VIDEO_JB_TAIL_MAX		(TDAV_VIDEO_JB_FPS_MAX << TDAV_VIDEO_JB_TAIL_MAX_LOG2)
#endif

//#define TDAV_VIDEO_JB_RATE				90 /* KHz */
#define TDAV_VIDEO_JB_RATE              48 // sync with send rtp routine

#define TDAV_VIDEO_JB_LATENCY_MIN		5 /* Must be > 0 */
#define TDAV_VIDEO_JB_LATENCY_MAX		15 /* Default, will be updated using fps */

YoumeVideoJitter::YoumeVideoJitter()
{
    cb_data_fdd.type = video_jitter_cb_data_type_fdd;
    cb_data_rtp.type = video_jitter_cb_data_type_rtp;
    
    m_fps = TDAV_VIDEO_JB_FPS;
    m_fps_prob = TDAV_VIDEO_JB_FPS_PROB;
    m_tail_max = TDAV_VIDEO_JB_TAIL_MAX;
    m_avg_duration = 0;
    m_rate = TDAV_VIDEO_JB_RATE;
    m_conseq_frame_drop = 0;
    m_decode_last_timestamp = 0;
    m_decode_last_seq_num_with_mark = -1;
    m_last_timestamp = 0;
    
    m_latency_min = TDAV_VIDEO_JB_LATENCY_MIN;
    m_latency_max = TDAV_VIDEO_JB_LATENCY_MAX;
    
    if(!(m_frames = tsk_list_create())){
        TSK_DEBUG_ERROR("Failed to create list");
    }

    videoJitterMutex = tsk_mutex_create_2(tsk_false);
}

YoumeVideoJitter::~YoumeVideoJitter()
{
    m_fps = TDAV_VIDEO_JB_FPS;
    m_fps_prob = TDAV_VIDEO_JB_FPS_PROB;
    m_tail_max = TDAV_VIDEO_JB_TAIL_MAX;
    m_avg_duration = 0;
    m_rate = TDAV_VIDEO_JB_RATE;
    m_conseq_frame_drop = 0;
   
    m_decode_last_timestamp = 0;
    m_decode_last_seq_num_with_mark = -1;
    m_last_timestamp = 0;
    
    m_latency_min = TDAV_VIDEO_JB_LATENCY_MIN;
    m_latency_max = TDAV_VIDEO_JB_LATENCY_MAX;
    
    if (m_frames) {
        TSK_OBJECT_SAFE_FREE(m_frames);
        m_frames = NULL;
    }
    
    if (videoJitterMutex) {
        tsk_mutex_destroy(&videoJitterMutex);
    }
}

void YoumeVideoJitter::setCallback(video_jitter_cb_f callback)
{
    m_callback = callback;
}

#if 0
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

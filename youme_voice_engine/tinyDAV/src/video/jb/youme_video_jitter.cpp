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

    // for nack process
    if(!(m_nack_list = tsk_list_create())){
        TSK_DEBUG_ERROR("Failed to create video nack list");
    }
    m_last_frame_timestamp = 0;
    m_last_pkt_seq = 0;
    m_last_frame_type = 0;
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
    
    if (m_nack_list) {
        TSK_OBJECT_SAFE_FREE(m_nack_list);
        m_nack_list = NULL;
    }

    if (videoJitterMutex) {
        tsk_mutex_destroy(&videoJitterMutex);
    }
}

void YoumeVideoJitter::setCallback(video_jitter_cb_f callback)
{
    m_callback = callback;
}

void YoumeVideoJitter::setNACKCallback(video_jitter_nack_cb_f nack_callback){
    m_nack_callback = nack_callback;
}

static tsk_object_t* tdav_video_nack_ctor(tsk_object_t * self, va_list * app)
{
    video_jitter_nack_t *video_nack = (video_jitter_nack_t *)self;
    if(video_nack){
        tsk_safeobj_init(video_nack);
        video_nack->nack_list = new std::set<uint16_t>;
    }
    
    return self;
}

static tsk_object_t* tdav_video_nack_dtor(tsk_object_t * self)
{
    video_jitter_nack_t *video_nack = (video_jitter_nack_t *)self;
    if (video_nack && video_nack->nack_list) {
        video_nack->nack_list->clear();
        delete video_nack->nack_list;
    }

    if(video_nack){        
        tsk_safeobj_deinit(video_nack);
    }
    
    return self;
}

static int tdav_video_nack_cmp(const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const video_jitter_nack_t *p1 = (video_jitter_nack_t *)_p1;
    const video_jitter_nack_t *p2 = (video_jitter_nack_t *)_p2;
    
    if(p1 && p2){
        return (int)(p1->timestamp - p2->timestamp);
    }
    else if(!p1 && !p2) return 0;
    else return -1;
}

static const tsk_object_def_t tdav_video_nack_def_s =
{
    sizeof(video_jitter_nack_t),
    tdav_video_nack_ctor,
    tdav_video_nack_dtor,
    tdav_video_nack_cmp,
};

const tsk_object_def_t *tdav_video_nack_def_t = &tdav_video_nack_def_s;

void YoumeVideoJitter::insert_seq_to_nack_list(uint16_t seq_num, uint32_t timestamp, uint8_t frame_type, uint16_t video_id) {

    // sanity check
    if (!seq_num || !timestamp) {
        return;
    }

    tsk_list_item_t* item = nullptr;

    tsk_list_lock(m_nack_list);
    tsk_list_foreach(item, m_nack_list) {
        struct video_jitter_nack_s * video_nack = TDAV_VIDEO_JITTER_NACK(item->data);
        if (video_nack->timestamp == timestamp && video_nack->video_id == video_id) {
            
            video_nack->nack_list->emplace(seq_num);
            tsk_list_unlock(m_nack_list);
            return;
        }
    }
    tsk_list_unlock(m_nack_list);

    // 创建新的nack节点, 并将节点插入到m_nack_list
    uint64_t current_time = tsk_gettimeofday_ms();
    video_jitter_nack_t* pnack = (video_jitter_nack_t*)tsk_object_new(tdav_video_nack_def_t);
    pnack->video_id = video_id;
    pnack->frame_type = frame_type;
    pnack->timestamp = timestamp;
    pnack->insert_timestamp = current_time;
    pnack->request_count = 0;

    pnack->nack_list->emplace(seq_num);

    tsk_list_lock(m_nack_list);
    tsk_list_push_back_data(m_nack_list, (void **)&pnack);
    tsk_list_unlock(m_nack_list);

    // update m_nack_list, 根据插入时间戳更新nack list, 清理超时包
    update_nack_list(current_time, 0);

}

// 注意seq num 为0 表示清理该timestamp对应的frame节点
// return 0:重复包，1:重传包，2:晚到的包
int YoumeVideoJitter::remove_seq_from_nack_list(uint16_t seq_num, uint32_t timestamp, uint16_t video_id) {
    int is_find = 0;
    
    tsk_list_item_t* item = nullptr;
    tsk_list_lock(m_nack_list);
    tsk_list_foreach(item, m_nack_list) {
        struct video_jitter_nack_s * video_nack = TDAV_VIDEO_JITTER_NACK(item->data);
        if (seq_num) {
            auto iter = video_nack->nack_list->find(seq_num);
            if (iter != video_nack->nack_list->end()) {
                video_nack->nack_list->erase(iter);
                if (video_nack->request_count) {
                    is_find = 1;
                } else {
                    is_find = 2;
                }
                // 两种情况下删除该frame节点
                // 1、如果该frame 重传packet数量为0，则清理该节点
                if (!video_nack->nack_list->size()) {
                    tsk_list_remove_item(m_nack_list, item);
                    break;
                }
            }
        } else {
                    
            // 2、输入seq_num为 0
            if (timestamp == video_nack->timestamp) {
                tsk_list_remove_item(m_nack_list, item);
                break;
            }
        }
        
        if (is_find) break;
    }
    tsk_list_unlock(m_nack_list);

    return is_find;
}

//以现在时间为起点，清除超时时间之前的RTP序列号
void YoumeVideoJitter::update_nack_list(uint64_t local_timestamp, int clear) {

    // todo: idr_interval should config
    uint32_t timeout_interval = 1000;
    uint32_t redundant_time = 100;
    
    uint64_t cls_timestamp = local_timestamp - timeout_interval - redundant_time;
    
    // 大小流切换时清除之前的所有cache
    if (clear) {
        cls_timestamp = local_timestamp;
    }
    
    tsk_list_item_t* item;
    tsk_list_lock(m_nack_list);

again:    
    tsk_list_foreach(item, m_nack_list) {
        struct video_jitter_nack_s * video_nack = TDAV_VIDEO_JITTER_NACK(item->data);
        if (video_nack->insert_timestamp < cls_timestamp) {
            video_nack->nack_list->clear();
            tsk_list_remove_item(m_nack_list, item);
            //注意：这里有个问题，在删除item之后，迭代器是否会错乱！！后面调试的时候注意下
            goto again;
        }
    }
    // TSK_DEBUG_INFO("nacklist frame size:%d\n", tsk_list_count_all(m_nack_list));
    // print_nack_list();
    tsk_list_unlock(m_nack_list);
}

//for test only
void YoumeVideoJitter::print_nack_list(){
    
    int count = tsk_list_count_all(m_nack_list);
    TSK_DEBUG_INFO("nacklist frame size:%d", count);

    tsk_list_item_t* item;
    tsk_list_foreach(item, m_nack_list) {
        struct video_jitter_nack_s * video_nack = TDAV_VIDEO_JITTER_NACK(item->data);
        TSK_DEBUG_INFO("Frame type:%u, ts:%u, nack_count:%u ",video_nack->frame_type, video_nack->timestamp, video_nack->nack_list->size());

        // std::list<uint16_t>::iterator iter = video_nack->nack_list->begin();
        // for (; iter != video_nack->nack_list->end(); ++iter) {
        //     TSK_DEBUG_INFO("lost packet ts:%u, seq:%u", video_nack->timestamp, *iter);
        // }
    }
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

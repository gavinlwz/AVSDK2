/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.3.8
 *  说明:对外发布接口
 ******************************************************************/
#include "tinyrtp/rtp/trtp_rtp_header.h"
#include "tinymedia/tmedia_defaults.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include "tinymedia/tmedia_codec.h"
#include <map>

#include "tinydav/video/tdav_youme_video_jitter.h"
#include "tinydav/video/tdav_youme_video_jitterbuffer.h"


typedef struct video_jb_manager_s
{
    YOUMEVideoJitter *state;
    
    uint64_t num_pkt_in;       // Number of incoming pkts since the last reset
    uint64_t num_pkt_miss;     // Number of times we got consecutive "JITTER_BUFFER_MISSING" results
    uint64_t num_pkt_miss_max; // Max value for "num_pkt_miss" before reset()ing the jitter buffer
    uint64_t last_pkt_ts;      // Timestamp when receiving the last packet.
} video_jb_manager_t;
typedef std::map<int32_t, video_jb_manager_t*>  video_jb_map_t;

/** youme Video JitterBuffer*/
typedef struct tdav_youme_video_jitterBuffer_s
{
    TMEDIA_DECLARE_JITTER_BUFFER;
    
    uint32_t frame_duration;
    uint32_t fps;
    uint32_t rotation;
    uint32_t width;
    uint32_t height;
    
    video_jb_map_t jb_map;
    
} tdav_youme_video_jitterbuffer_t;


static int tdav_youme_video_jitterbuffer_set (tmedia_jitterbuffer_t *self, const tmedia_param_t *param)
{
    TSK_DEBUG_ERROR ("Not implemented");
    return -2;
}

static int create_video_jitterbuffer_for_new_session (tdav_youme_video_jitterbuffer_t *jitterbuffer, int32_t session_id)
{
    std::pair< video_jb_map_t::iterator, bool > insertRet;
    video_jb_manager_t *pJbMgr = new video_jb_manager_t;
    if (NULL == pJbMgr) {
        goto bail;
    }
    
    pJbMgr->state = new YOUMEVideoJitter (tmedia_defaults_get_jb_margin (), jitterbuffer->frame_duration);
    if (NULL == pJbMgr->state) {
        goto bail;
    }
    
    pJbMgr->num_pkt_in = 0;
    pJbMgr->num_pkt_miss = 0;
    pJbMgr->num_pkt_miss_max = (1000 / jitterbuffer->frame_duration) * 2;
    pJbMgr->last_pkt_ts = tsk_gettimeofday_ms();
    
    insertRet = jitterbuffer->jb_map.insert( video_jb_map_t::value_type(session_id, pJbMgr) );
    if (!insertRet.second) {
        goto bail;
    }
    
    return 0;
bail:
    TSK_DEBUG_ERROR ("Failed to create a new jitter buffer for session:%d", session_id);
    if (NULL != pJbMgr) {
        if (NULL != pJbMgr->state) {
            delete pJbMgr->state;
            pJbMgr->state = NULL;
        }
        delete pJbMgr;
        pJbMgr = NULL;
    }
    return -1;
}

static int tdav_youme_video_jitterbuffer_open (tmedia_jitterbuffer_t *self, uint32_t frame_duration, uint32_t in_rate, uint32_t out_rate, uint32_t channels)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    
    TSK_DEBUG_INFO ("Open youme jb (ptime=%u, in_rate=%u, out_rate=%u)", frame_duration, in_rate, out_rate);
    
    jitterbuffer->frame_duration = frame_duration;
    jitterbuffer->jb_map.clear();
    
    return 0;
}

static int tdav_youme_video_jitterbuffer_tick (tmedia_jitterbuffer_t *self)
{
    // TSK_DEBUG_ERROR ("Not implemented");
    return 0;
}

static int tdav_youme_video_jitterbuffer_put_bkaudio (tmedia_jitterbuffer_t *self, void *data, tsk_object_t *proto_hdr)
{
    return 0;
}

static int tdav_youme_video_jitterbuffer_put (tmedia_jitterbuffer_t *self, void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    trtp_rtp_header_t *rtp_hdr;
    
    
    if (!data || !data_size || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    rtp_hdr = TRTP_RTP_HEADER (proto_hdr);
    video_jb_map_t::iterator it = jitterbuffer->jb_map.find(rtp_hdr->session_id);
    if (it == jitterbuffer->jb_map.end()) {
        if (create_video_jitterbuffer_for_new_session(jitterbuffer, rtp_hdr->session_id) != 0) {
            return -1;
        }
        it = jitterbuffer->jb_map.find(rtp_hdr->session_id);
        if (it == jitterbuffer->jb_map.end()) {
            TSK_DEBUG_ERROR ("impossible");
            return -1;
        }
    }
    
    video_jb_manager_t* jb = it->second;
    if (NULL == jb->state)
    {
        TSK_DEBUG_ERROR ("Invalid state");
        return -2;
    }
    
    RTPVideoPacket packet;
    packet.pData = new unsigned char[data_size];
    packet.nLen = data_size;
    memcpy (packet.pData, data, data_size);
    packet.timestamp = rtp_hdr->timestamp;
    packet.nSeq = rtp_hdr->seq_num;
    packet.flag = rtp_hdr->flag;
    jb->state->push (packet);
    ++jb->num_pkt_in;
    
    return 0;
}

static int get_data_from_video_jitterbuffer (video_jb_manager_t *jb, RTPVideoPacket &packet)
{
    int ret = YOUMEVideoJitter::SUCCESS;
    
    if (!jb->state)
    {
        TSK_DEBUG_ERROR ("Invalid state");
        return -1;
    }
    
    if ((ret = jb->state->pop (packet)) != YOUMEVideoJitter::SUCCESS)
    {
        ++jb->num_pkt_miss;
        switch (ret)
        {
            case YOUMEVideoJitter::BUFFERING:
                // TSK_DEBUG_INFO ("JITTER_BUFFER_MISSING - %d", ret);
                if (jb->num_pkt_miss > jb->num_pkt_miss_max && jb->num_pkt_in > jb->num_pkt_miss_max)
                {
                    if (jb->state)
                    {
                        jb->state->reset ();
                    }
                    jb->num_pkt_in = 0;
                    jb->num_pkt_miss = 0;
                    
                    TSK_DEBUG_WARN ("Too much missing audio pkts");
                }
                break;
            case YOUMEVideoJitter::DROPPED_PACKET:
                break;
            default:
                TSK_DEBUG_INFO ("jitter_buffer_get() failed - %d", ret);
                break;
        }
    }
    else
    {
        jb->num_pkt_miss = 0;
    }
    
    
    return ret;
}

static tsk_size_t tdav_youme_video_jitterbuffer_get (tmedia_jitterbuffer_t *self, void *out_data, void *video_data, tsk_size_t out_size)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    
    int ret = 0;
    tsk_size_t ret_size = 0;
    video_jb_map_t::iterator it;
    
    if (!out_data || !out_size)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    
    // No audio data coming in yet.
    if (jitterbuffer->jb_map.empty()) {
        return 0;
    }
    
    if (jitterbuffer->jb_map.size() == 1) {
        RTPVideoPacket packet;
        packet.pData = NULL;
        packet.nLen = 0;
        
        it = jitterbuffer->jb_map.begin();
        ret = get_data_from_video_jitterbuffer(it->second, packet);
        if (YOUMEVideoJitter::SUCCESS != ret) {
            return 0;
        }
        
        if (out_size >= packet.nLen) {
            ret_size = packet.nLen;
            memcpy (out_data, packet.pData, ret_size);
        } else {
            ret_size = out_size;
            memcpy (out_data, packet.pData, ret_size);
            TSK_DEBUG_WARN ("invalid size:%d", packet.nLen);
        }
        
        if (packet.pData)
        {
            delete[] packet.pData;
            packet.pData = NULL;
        }
        
        packet.nLen = 0;
    } else {
        
        //TODO :video mixer
        TSK_DEBUG_INFO("VIDEO MIXER TODO");
    }
    
    if (NULL != video_data) {
        memcpy(video_data, out_data, ret_size);
    }
    
    return ret_size;
}

static int tdav_youme_video_jitterbuffer_reset (tmedia_jitterbuffer_t *self)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    video_jb_map_t::iterator it;
    for (it = jitterbuffer->jb_map.begin(); it != jitterbuffer->jb_map.end(); it++) {
        video_jb_manager_t *jb = it->second;
        if (jb) {
            if (jb->state)
            {
                jb->state->reset ();
            }
            jb->num_pkt_in = 0;
            jb->num_pkt_miss = 0;
        }
    }
    
    return 0;
}

static int tdav_youme_video_jitterbuffer_close (tmedia_jitterbuffer_t *self)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    video_jb_map_t::iterator it;
    for (it = jitterbuffer->jb_map.begin(); it != jitterbuffer->jb_map.end(); it++) {
        video_jb_manager_t *jb = it->second;
        if (jb) {
            if (jb->state) {
                delete jb->state;
                jb->state = NULL;
            }
            delete jb;
            jb = NULL;
        }
    }
    jitterbuffer->jb_map.clear();
    
    return 0;
}


//
//	youme jitterbufferr Plugin definition
//

/* constructor */
static tsk_object_t *tdav_youme_video_jitterbuffer_ctor (tsk_object_t *self, va_list *app)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    TSK_DEBUG_INFO ("Create youme video jitter buffer");
    if (jitterbuffer)
    {
        /* init base */
        tmedia_jitterbuffer_init (TMEDIA_JITTER_BUFFER (jitterbuffer));
        /* init self */
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_youme_video_jitterbuffer_dtor (tsk_object_t *self)
{
    tdav_youme_video_jitterbuffer_t *jitterbuffer = (tdav_youme_video_jitterbuffer_t *)self;
    if (jitterbuffer)
    {
        /* deinit base */
        tmedia_jitterbuffer_deinit (TMEDIA_JITTER_BUFFER (jitterbuffer));
        /* deinit self */
        video_jb_map_t::iterator it;
        for (it = jitterbuffer->jb_map.begin(); it != jitterbuffer->jb_map.end(); it++) {
            video_jb_manager_t *jb = it->second;
            if (jb) {
                if (jb->state) {
                    delete jb->state;
                    jb->state = NULL;
                }
                delete jb;
                jb = NULL;
            }
        }
        jitterbuffer->jb_map.clear();
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t tdav_youme_video_jitterbuffer_def_s = {
    sizeof (tdav_youme_video_jitterbuffer_t),
    tdav_youme_video_jitterbuffer_ctor,
    tdav_youme_video_jitterbuffer_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_jitterbuffer_plugin_def_t tdav_youme_video_jitterbuffer_plugin_def_s = {
    &tdav_youme_video_jitterbuffer_def_s,
    tmedia_video,
    "Video JitterBuffer based on youme",
    
    tdav_youme_video_jitterbuffer_set,
    tdav_youme_video_jitterbuffer_open,
    tdav_youme_video_jitterbuffer_tick,
    tdav_youme_video_jitterbuffer_put,
    tdav_youme_video_jitterbuffer_get,
    tdav_youme_video_jitterbuffer_reset,
    tdav_youme_video_jitterbuffer_close,
    tsk_null,
};
const tmedia_jitterbuffer_plugin_def_t *tdav_youme_video_jitterbuffer_plugin_def_t = &tdav_youme_video_jitterbuffer_plugin_def_s;

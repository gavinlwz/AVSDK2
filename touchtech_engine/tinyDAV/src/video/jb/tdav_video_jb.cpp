/*
 * Copyright (C) 2011-2015 Mamadou DIOP
 * Copyright (C) 2011-2015 Doubango Telecom <http://www.doubango.org>
 *
 * This file is part of Open Source Doubango Framework.
 *
 * DOUBANGO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DOUBANGO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DOUBANGO.
 *
 */

/**@file tdav_video_jb.c
 * @brief Video Jitter Buffer
 */
#include "tinydav/video/jb/tdav_video_jb.h"
#include "tinydav/video/jb/youme_video_jitter.h"
#include "tinydav/video/jb/tdav_video_frame.h"
#include "tinydav/codecs/h264/tdav_codec_h264_rtp.h"

#include "tinyrtp/rtp/trtp_rtp_packet.h"
 
// #include "tsk_timer.h"
#include "tsk_memory.h"
#include "tsk_thread.h"
// #include "tsk_condwait.h"
#include "tsk_debug.h"
#include "tmedia_utils.h"

#if TSK_UNDER_WINDOWS
#	include <windows.h>
#endif

#include "XConfigCWrapper.hpp"

#include <map>
#include "tinymedia/tmedia_defaults.h"

#define TDAV_VIDEO_JB_DISABLE           0

// default frame rate
// the corret fps will be computed using the RTP timestamps
#define TDAV_VIDEO_JB_FPS		TDAV_VIDEO_JB_FPS_MAX
#define TDAV_VIDEO_JB_FPS_MIN	10
#define TDAV_VIDEO_JB_FPS_MAX	120
// Number of correct consecutive video frames to receive before computing the FPS
#define TDAV_VIDEO_JB_FPS_PROB	(TDAV_VIDEO_JB_FPS) //(TDAV_VIDEO_JB_FPS << 1)
// Maximum gap allowed (used to detect seqnum wrpping)
#define TDAV_VIDEO_JB_MAX_DROPOUT		0xFD9B

#define TDAV_VIDEO_JB_TAIL_MAX_LOG2		2
#if TDAV_UNDER_MOBILE /* to avoid too high memory usage */
#	define TDAV_VIDEO_JB_TAIL_MAX		(TDAV_VIDEO_JB_FPS_MIN << TDAV_VIDEO_JB_TAIL_MAX_LOG2)
#else
#	define TDAV_VIDEO_JB_TAIL_MAX		(TDAV_VIDEO_JB_FPS_MAX << TDAV_VIDEO_JB_TAIL_MAX_LOG2)
#endif

//#define TDAV_VIDEO_JB_RATE				90 /* KHz */
#define TDAV_VIDEO_JB_RATE				48 // sync with send rtp routine

#define TDAV_VIDEO_JB_LATENCY_MIN		2 /* Must be > 0 */
#define TDAV_VIDEO_JB_LATENCY_MAX		15 /* Default, will be updated using fps */

typedef struct video_jb_manager_s
{
    YoumeVideoJitter *state;
    bool     first_packet;
    int32_t  session_id;
    int32_t  ssrc;
    
    tsk_bool_t      jb_decode_thread_start;
    tsk_thread_handle_t* jb_decode_thread_handle;
    tsk_thread_handle_t* jb_smooth_thread_handle;
    //pthread_t       jb_decode_thread_handle;
    //是否已经有完整的Frame
    tsk_semaphore_handle_t* jb_full_frame_sema;

    //为了统计的增加的
    int32_t iTotalIFrame;
    int32_t iLostIFrame;
    uint64_t diffMs;
    uint64_t lastCallBackTime;
    uint64_t smoothTimeMs;
    bool smooth; //是否开启smooth
    bool bHasLostPacket;
	uint16_t nextFrameSerial;
    int iPFrameLossCount;

    uint64_t iLastPktTs;      // Timestamp when receiving the last packet.
    
    tdav_rscode_t *rscode;
    

} video_jb_manager_t;

typedef std::map<int32_t, video_jb_manager_t*>  video_jb_manager_map_t;

static int _tdav_video_jb_set_defaults(struct tdav_video_jb_s* self);
static const tdav_video_frame_t* _tdav_video_jb_mgr_get_frame(video_jb_manager_t* self, trtp_rtp_packet_s* p, tsk_bool_t *pt_matched);
static void* TSK_STDCALL _tdav_video_jb_decode_thread_func(void *param);
static void* TSK_STDCALL _tdav_video_jb_smooth_thread_func(void *param);
static void tdav_clear_frame_pkts(tdav_video_frame_t* frame);

typedef struct tdav_video_jb_s
{
    TSK_DECLARE_OBJECT;

    tsk_bool_t started;
#if 0
    int32_t fps;
    int32_t fps_prob;
    int32_t avg_duration;
    int32_t rate; // in Khz
    uint32_t last_timestamp;
    int32_t conseq_frame_drop;
    int32_t tail_max;
    tdav_video_frames_L_t *frames;
    int64_t frames_count;
    
    tsk_size_t latency_min;
    tsk_size_t latency_max;
    
    uint32_t decode_last_timestamp;
    int32_t decode_last_seq_num_with_mark; // -1 = unset
    uint64_t decode_last_time;

    tsk_thread_handle_t* decode_thread[1];
    // TODO:use common lib
    // tsk_condwait_handle_t* decode_thread_cond;
#endif
    
    // uint16_t seq_nums[0xFF];
    tdav_video_jb_cb_f callback;
    const void* callback_data;
    
    // to avoid locking use different cb_data
    tdav_video_jb_cb_data_xt cb_data_rtp;
    tdav_video_jb_cb_data_xt cb_data_fdd;
    tdav_video_jb_cb_data_xt cb_data_any;
    
    struct{
        void* ptr;
        tsk_size_t size;
    } buffer;
    
    video_jb_manager_map_t* video_jb_mgr_map;
    uint32_t         max_jb_num;
    uint32_t         jb_num;
    uint32_t         counter;
    
    TSK_DECLARE_SAFEOBJ;
}
tdav_video_jb_t;

//
// Free resources for a jb manager.
//
static void free_jb_manager(video_jb_manager_t **jb_mgr)
{
    if (jb_mgr && (*jb_mgr)) {
        delete (*jb_mgr);
        (*jb_mgr) = NULL;
    }
}

static tsk_object_t* tdav_video_jb_ctor(tsk_object_t * self, va_list * app)
{
    tdav_video_jb_t *jb = (tdav_video_jb_t *)self;
    if(jb){
        jb->video_jb_mgr_map = new video_jb_manager_map_t;
        jb->video_jb_mgr_map->clear();
        jb->max_jb_num = tmedia_defaults_get_max_jitter_buffer_num();
        jb->jb_num  = 0;
        jb->counter = 0;
        jb->buffer.ptr = NULL;
        
        tsk_safeobj_init(jb);
    }
    
    return self;
}
static tsk_object_t* tdav_video_jb_dtor(tsk_object_t * self)
{
    tdav_video_jb_t *jb = (tdav_video_jb_t *)self;
    video_jb_manager_t *jb_mgr;
    int32_t sessionId;
    if(jb){
        if(jb->started){
            tdav_video_jb_stop(jb);
        }
        
        TSK_SAFE_FREE(jb->buffer.ptr);
        
        if (NULL != jb->video_jb_mgr_map) {
            video_jb_manager_map_t::iterator it;
            for (it = jb->video_jb_mgr_map->begin(); it != jb->video_jb_mgr_map->end(); it++) {
                jb_mgr = it->second;
                sessionId = it->first;
                if (jb_mgr) {
                    //需要加锁，否则有crash可能
                    // if(jb_mgr->state->m_frames)
                    //    tsk_list_lock(jb_mgr->state->m_frames);
                    jb_mgr->jb_decode_thread_start = tsk_false;
                    // if(jb_mgr->state->m_frames)
                    //    tsk_list_unlock(jb_mgr->state->m_frames);
                    // Send signal to stop jb decode thread
                    
                    tsk_semaphore_increment(jb_mgr->jb_full_frame_sema);
                    
                    tsk_thread_join(&jb_mgr->jb_decode_thread_handle);
                    tsk_thread_join(&jb_mgr->jb_smooth_thread_handle);
                    // pthread_join(jb_mgr->jb_decode_thread_handle, NULL);
                    TSK_DEBUG_INFO("Stop jb decode thread OK for sessionId(%d)", sessionId);
                    tsk_semaphore_destroy(&jb_mgr->jb_full_frame_sema);
                    delete jb_mgr->state;
                    jb_mgr->state = NULL;
                }
                free_jb_manager(&jb_mgr);
            }
            jb->video_jb_mgr_map->clear();
            delete jb->video_jb_mgr_map;
            jb->video_jb_mgr_map = NULL;
        }
        
        tsk_safeobj_deinit(jb);
    }
    
    return self;
}
static const tsk_object_def_t tdav_video_jb_def_s =
{
    sizeof(tdav_video_jb_t),
    tdav_video_jb_ctor,
    tdav_video_jb_dtor,
    tsk_null,
};

tdav_video_jb_t* tdav_video_jb_create()
{
    tdav_video_jb_t* jb;
    
    if ((jb = (tdav_video_jb_t *)tsk_object_new(&tdav_video_jb_def_s))) {
        if (_tdav_video_jb_set_defaults(jb) != 0) {
            TSK_OBJECT_SAFE_FREE(jb);
        }
    }
    return jb;
}

static int create_video_jb_for_new_session (tdav_video_jb_t *jb, int32_t sessionId)
{
    std::pair<video_jb_manager_map_t::iterator, bool> insertRet;
    video_jb_manager_t *jb_mgr = new video_jb_manager_t;
    
    // Initial
    jb_mgr->first_packet = true;
    jb_mgr->session_id   = sessionId;
    jb_mgr->state = new YoumeVideoJitter;
    jb_mgr->jb_decode_thread_start = tsk_false;
    jb_mgr->iTotalIFrame=0;
    jb_mgr->iLostIFrame=0;
    if (NULL == jb_mgr->state) {
        goto bail;
    }
    insertRet = jb->video_jb_mgr_map->insert(video_jb_manager_map_t::value_type(sessionId, jb_mgr));
    TSK_DEBUG_INFO("Create the new jb for session(%d)", sessionId);
    
    return 0;
bail:
    TSK_DEBUG_ERROR ("Failed to create a new jitter buffer for session:%d", sessionId);
    if (NULL != jb_mgr) {
        if (NULL != jb_mgr->state) {
            delete jb_mgr->state;
            jb_mgr->state = NULL;
        }
        delete jb_mgr;
        jb_mgr = NULL;
    }
    return -1;
}

int delete_video_jb_session(tdav_video_jb_t *jb, int32_t sessionId) {
    TSK_DEBUG_INFO ("@@ delete_video_jb_session session_id:%d", sessionId);
    if (!jb) {
        TSK_DEBUG_ERROR ("delete_video_jb_session invalid parameter");
        return -1;
    }

    video_jb_manager_t *jb_mgr;
    if (NULL != jb->video_jb_mgr_map && jb->video_jb_mgr_map->size() > 0) {
        video_jb_manager_map_t::iterator it;
        for (it = jb->video_jb_mgr_map->begin(); it != jb->video_jb_mgr_map->end(); ++it) {
            jb_mgr = it->second;
            
            if (sessionId == it->first) {
                if (jb_mgr) {
                    //需要加锁，否则有crash可能
                    // if(jb_mgr->state->m_frames) {
                    //    tsk_list_lock(jb_mgr->state->m_frames);
                    // }

                    jb_mgr->jb_decode_thread_start = tsk_false;
                    // if(jb_mgr->state->m_frames) {
                    //    tsk_list_unlock(jb_mgr->state->m_frames);
                    // }

                    // Send signal to stop jb decode thread
                    tsk_semaphore_increment(jb_mgr->jb_full_frame_sema);
                    
                    tsk_thread_join(&jb_mgr->jb_decode_thread_handle);
                    tsk_thread_join(&jb_mgr->jb_smooth_thread_handle);
                    // pthread_join(jb_mgr->jb_decode_thread_handle, NULL);
                    TSK_DEBUG_INFO("Stop jb decode thread OK for sessionId(%d)", sessionId);
                    tsk_semaphore_destroy(&jb_mgr->jb_full_frame_sema);
                    delete jb_mgr->state;
                    jb_mgr->state = NULL;
                }
                free_jb_manager(&jb_mgr);
                jb->video_jb_mgr_map->erase(it);
                TSK_DEBUG_ERROR ("== delete_video_jb_session success");
                return 0;
            }

        }
    }

    TSK_DEBUG_INFO ("== delete_video_jb_session failed");
    return 0;
}

#define tdav_video_jb_reset_fps_prob(self) {\
(self)->fps_prob = TDAV_VIDEO_JB_FPS_PROB; \
(self)->last_timestamp = 0; \
(self)->avg_duration = 0; \
}
#define tdav_video_jb_reset_tail_min_prob(self) {\
(self)->tail_prob = TDAV_VIDEO_JB_TAIL_MIN_PROB; \
(self)->tail_min = TDAV_VIDEO_JB_TAIL_MIN_MAX; \
}

int tdav_video_jb_set_callback(tdav_video_jb_t* self, tdav_video_jb_cb_f callback, void* usr_data)
{
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    self->callback = callback;
    self->cb_data_any.usr_data = usr_data;
    self->cb_data_fdd.usr_data = usr_data;
    self->cb_data_rtp.usr_data = usr_data;
    return 0;
}

int tdav_video_jb_start(tdav_video_jb_t* self)
{
    int ret = 0;
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(self->started){
        return 0;
    }
    
    self->started = tsk_true;
    
    return ret;
}

int tdav_video_jb_decode_thread_start(video_jb_manager_t *self)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if (self->jb_decode_thread_start) {
        return 0;
    }
    
    self->jb_full_frame_sema = tsk_semaphore_create();
    self->diffMs = 0;
    self->smoothTimeMs = 25;
    self->smooth = false;
    self->bHasLostPacket = true;
	self->nextFrameSerial = 0;
    self->iPFrameLossCount = 0;
    self->lastCallBackTime = 0;
    self->iLastPktTs = 0;
    self->rscode = tsk_null;
    
    
    self->jb_decode_thread_start = tsk_true;
    if (0 != tsk_thread_create(&self->jb_decode_thread_handle, &_tdav_video_jb_decode_thread_func, (void*)self)) {
        TSK_DEBUG_ERROR("Jb decode thread create failed");
    } else {
        tsk_thread_set_priority(self->jb_decode_thread_handle, TSK_THREAD_PRIORITY_HIGH);
    }

    if (0 != tsk_thread_create(&self->jb_smooth_thread_handle, &_tdav_video_jb_smooth_thread_func, (void*)self)) {
        TSK_DEBUG_ERROR("Jb smooth thread create failed");
    } else {
        tsk_thread_set_priority(self->jb_smooth_thread_handle, TSK_THREAD_PRIORITY_HIGH);
    }

    return 0;
}


static void tdav_video_request_pli(video_jb_manager_t* self, int sessionid, int videoid){
    if (self->state->m_callback) {
        video_jitter_cb_data_xt data;
        data.type = video_jitter_cb_data_type_tmfr;
        data.usr_data = self->state->cb_data_rtp.usr_data;
        data.ssrc = self->ssrc;
        data.avpf.sessionid = sessionid;
        data.avpf.videoid = videoid;
        self->state->m_callback(&data);
    }
}


//重新设置csrc[2]
static void tdav_video_jb_reset_csrc2(tdav_video_frame_t* frames){

     tsk_list_item_t* item;
     tsk_list_lock(frames->pkts);
     tsk_list_foreach(item, frames->pkts){
          ((trtp_rtp_packet_t*)(item->data))->header->csrc[2] = ((frames->first_seq_num << 16) | ((uint16_t)frames->framePktCount));
     }
     tsk_list_unlock(frames->pkts);
}

int tdav_video_jb_put(void* video, tdav_video_jb_t* self, trtp_rtp_packet_t* rtp_pkt, uint16_t iNAP, uint32_t iCheckCodeLen,tdav_rscode_t *rscode)
{
#if TDAV_VIDEO_JB_DISABLE
    self->cb_data_rtp.rtp.pkt = rtp_pkt;
    self->callback(&self->cb_data_rtp);
#else
    
    tdav_video_frame_t* old_frame;
    tsk_bool_t pt_matched = tsk_false;
    uint16_t* seq_num;
    
    if(!self || !rtp_pkt || !rtp_pkt->header || !rtp_pkt->payload.size){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if(!self->started){
        TSK_DEBUG_INFO("Video jitter buffer not started");
        return 0;
    }

#if 1
    // for share video stream
    if (2 == rtp_pkt->header->video_id) {
        TMEDIA_I_AM_ACTIVE(1000, "tdav_video_jb_put share stream session[%u] videoid[%d]", rtp_pkt->header->session_id, rtp_pkt->header->video_id);
        rtp_pkt->header->session_id = -rtp_pkt->header->session_id;
    }
    AddVideoPacket( rtp_pkt->header->seq_num , rtp_pkt->header->session_id);
    
    video_jb_manager_map_t::iterator it = self->video_jb_mgr_map->find(rtp_pkt->header->session_id);
    video_jb_manager_t *jb_mgr = NULL;
    if (it == self->video_jb_mgr_map->end()) { // 表示map里面没有对应的jb实例，需要先创建
        if (create_video_jb_for_new_session(self, rtp_pkt->header->session_id) != 0) {
            return -1;
        }
        it = self->video_jb_mgr_map->find(rtp_pkt->header->session_id); // 再遍历一次
        if (it == self->video_jb_mgr_map->end()) {
            TSK_DEBUG_ERROR("Impossible");
        }
        self->jb_num++;
        // Set callback function
        jb_mgr = it->second;
        jb_mgr->state->setCallback((video_jitter_cb_f)self->callback);
        jb_mgr->state->cb_data_any.usr_data = video;
        jb_mgr->state->cb_data_fdd.usr_data = video;
        jb_mgr->state->cb_data_rtp.usr_data = video;
        jb_mgr->rscode = rscode;
 
        // Create the jb_decode_thread
        tdav_video_jb_decode_thread_start(jb_mgr);
    }
    jb_mgr = it->second;
    
    if( jb_mgr->iLastPktTs != 0 )
    {
        AddVideoPacketTimeGap( rtp_pkt->header->receive_timestamp_ms - jb_mgr->iLastPktTs,  rtp_pkt->header->session_id );
    }
    
    jb_mgr->iLastPktTs = rtp_pkt->header->receive_timestamp_ms;
    jb_mgr->rscode = rscode;
    if (NULL == jb_mgr->state) {
        TSK_DEBUG_ERROR("Invalid state");
        return -1;
    }
    
    // seq_num = &self->seq_nums[rtp_pkt->header->payload_type]; // sequence number数组指针
    
    // TODO：这个地方可以优化下，记录当前所有frame的timestamp, 当新到达数据包比timestamp最旧的还要之前，就之前丢弃

    //TSK_DEBUG_INFO("Video get: sessionId=%d, seq_num=%d, marker=%d, timestamp=%lld", rtp_pkt->header->session_id, rtp_pkt->header->seq_num, rtp_pkt->header->marker, rtp_pkt->header->timestamp);
    // 找出当前packet的timestamp在list里面是否有相同的item包含，表示可能是同一个frame的不同packet
    old_frame = (tdav_video_frame_t*)_tdav_video_jb_mgr_get_frame(jb_mgr, rtp_pkt, &pt_matched);
    uint8_t webrtc_pkt_flag = 0;
    uint8_t packet_type = 0, start_bit = 0, end_bit = 0;
    uint8_t uFrameType = 0;
    
    packet_type = ((uint8_t*)(rtp_pkt->payload.data))[0] & 0x1F;
    if (!rtp_pkt->header->csrc[2]) { // pkt from web(rtc) only have csrc[0](sessionId), native csrc[1] is KeyFrame flag
        webrtc_pkt_flag = 1; // video packt from web(rtc)
        uFrameType = (packet_type == H264_NAL_SLICE_IDR || packet_type == H264_NAL_SPS || packet_type == H264_NAL_PPS);  //IDR or SPS or PPS
        // fu-a
        if (packet_type == H264_NAL_FU_A) {
            start_bit = ((uint8_t*)(rtp_pkt->payload.data))[1] & 0x80;  // FU-A start bit
            end_bit = ((uint8_t*)(rtp_pkt->payload.data))[1] & 0x40;    // FU-A end bit
            uint8_t nalu_type= (((uint8_t*)(rtp_pkt->payload.data))[1] & 0x1f); // FU-A orignal nalu type
            if (H264_NAL_SLICE_IDR == nalu_type) {   // IDR
                uFrameType = 1;
            }
        }
    } else {    // pkt from native
        uFrameType = rtp_pkt->header->csrc[1];  // native csrc[1] is KeyFrame flag
    }

    // stap-a (sps/pps/sei) or frame type 1
    // 目前在native 报文结构中，只有sps和pps会组成stap-a
    // TODO: 这里有个优化点，video中比较小的包可以做合并处理(stap-a),这样就在低码率下减小包量
    if (H264_NAL_STAP_A == packet_type) {
        do {
            tsk_list_lock(jb_mgr->state->m_frames);
            tsk_size_t frame_count = tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null);
            if(frame_count >= jb_mgr->state->m_tail_max) {
                tsk_list_unlock(jb_mgr->state->m_frames);
                break;
            }
            tsk_list_unlock(jb_mgr->state->m_frames);

            int payload_offset = 0, data_offset = 0;
            uint8_t * payload = (uint8_t*)rtp_pkt->payload.data;
            uint8_t * rtp_data = (uint8_t *)tsk_calloc(1400, sizeof(uint8_t)); // less than mtu(1500)
            uint32_t nalu_len = 0;
            uint32_t nalu_type = 0;
            
            payload_offset += 1;    // stap-a nalu header length
            while (payload_offset < rtp_pkt->payload.size) {
                
                // notify: first nalu not need start code
                if (data_offset != 0) {
                    rtp_data[data_offset] = 0x0;
                    rtp_data[data_offset+1] = 0x0;
                    rtp_data[data_offset+2] = 0x0;
                    rtp_data[data_offset+3] = 0x1;
                    data_offset += 4;               // start code : 0x00 0x00 0x00 0x01
                }
                
                nalu_len = (payload[payload_offset] << 8) | payload[payload_offset + 1];
                nalu_type = payload[payload_offset + 2] & 0x1F;

                // 检查stap-a中的nalu类型，便于后续处理
                // 如果确认是I帧 则不需要在进行判定
                if (!uFrameType) {
                    if (nalu_type == H264_NAL_SPS || nalu_type == H264_NAL_PPS || nalu_type == H264_NAL_SEI)
                        uFrameType=1;
                    else
                        uFrameType=0;
                }

                payload_offset += 2;
                memcpy(&rtp_data[data_offset], payload+payload_offset, nalu_len);
                payload_offset += nalu_len; // 聚合包里子nalu长度
                data_offset += nalu_len;
            }
            
//            trtp_rtp_header_t* rtp_header = trtp_rtp_header_create_null();
//            trtp_rtp_packet_t* rtp_packet =  trtp_rtp_packet_create_null();
//            rtp_packet->header = rtp_header;
//            rtp_packet->payload.data = (void *)rtp_data;
//            rtp_packet->payload.size = data_offset;
//            rtp_pkt->extension.size = 0;
//            rtp_packet->header->frameType=uFrameType;
//            memcpy(rtp_header, rtp_pkt->header, sizeof(trtp_rtp_header_t));
            
            tsk_free(&rtp_pkt->payload.data);

            rtp_pkt->payload.data = (void *)rtp_data;
            rtp_pkt->payload.size = data_offset;
            rtp_pkt->header->frameType = uFrameType;
            rtp_pkt->extension.size = 0;
            
            // 1. 新的一帧数据到来，创建新的frame后，则不必进行后续处理
            // 2. 若是在frame链表中存在同一帧的数据，则进行后续判断，看是否需要通知解码线程取数据
            if (!old_frame) {
                tdav_video_frame_t * new_frame = tdav_video_frame_create(rtp_pkt, iNAP, iCheckCodeLen);
                tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(new_frame));
                tsk_list_lock(jb_mgr->state->m_frames);
                if( new_frame->iFrameSerial < 1000){
                    tsk_list_push_back_data(jb_mgr->state->m_frames, (void**)&temp);
                }else{
                    tsk_list_push_ascending_data(jb_mgr->state->m_frames, (void**)&temp);
                }
                    
                tsk_list_unlock(jb_mgr->state->m_frames);
                if (webrtc_pkt_flag) {
                    new_frame->iFrameSerial = new_frame->first_seq_num;
                }          
                new_frame->iFrameType = uFrameType;
                new_frame->framePktCount = 0; // SPS/PPS need to wait IDR in native code
                new_frame->marker = rtp_pkt->header->marker;
                
                // 兼容webrtc pkt处理
                // 针对sps/pps数据包，直接放入链表，等待后续的IDR帧合并处理
                // 针对新收到的stap-a非关键帧并且markerbit为true的情况下，直接发送信号
                // 注意：这里有一个问题就是假如一个非关键帧有多个stap-a，但是由于乱序到达，最后一个包先到
                //      这时通知该数据包出队，导致该帧数据错误，所以这里还是先等等
                // 目前暂时解决方法是 收到一个独立的新的markerbit包，暂时先不发信号
                // 后续jitterbuffer来处理序列号乱序
                if (new_frame->marker && !uFrameType) {
                    new_frame->framePktCount = 1;
                    //收到一个独立的新的markerbit包，暂时先不发信号，看看后面是否有乱序的数据包到达
                    tdav_video_jb_reset_csrc2(new_frame);
                }
                tsk_object_unref(new_frame);
                return 0;
            }

        } while (0);
    }
    
    /*
    // packet lose
    if((*seq_num && *seq_num != 0xFFFF) && (*seq_num + 1) != rtp_pkt->header->seq_num){
        int32_t diff = ((int32_t)rtp_pkt->header->seq_num - (int32_t)*seq_num);
        tsk_bool_t is_frame_loss = (diff > 0);
        is_restarted = (TSK_ABS(diff) > TDAV_VIDEO_JB_MAX_DROPOUT);
        is_frame_late_or_dup = !is_frame_loss;
        tdav_video_jb_reset_fps_prob(self);
        TSK_DEBUG_INFO("Packet %s (from JB) [%hu - %hu]", is_frame_loss ? "loss" : "late/duplicated/nack", *seq_num, rtp_pkt->header->seq_num);
        
        if(is_frame_loss && !is_restarted){
            if(self->callback){
                self->cb_data_any.type = tdav_video_jb_cb_data_type_fl;
                self->cb_data_any.ssrc = rtp_pkt->header->ssrc;
                self->cb_data_any.fl.seq_num = (*seq_num + 1);
                self->cb_data_any.fl.count = diff - 1;
                self->callback(&self->cb_data_any);
            }
        }
    }*/
    
    // 当前packet的timestamp没有在list里面找到匹配的item，表示是新的一帧数据，所以要新建一个frame item
    if(!old_frame){
        tdav_video_frame_t* new_frame;
        if(pt_matched){
            // if we have a frame with the same payload type but without this timestamp this means that we moved to a new frame
            // this happens if the frame is waiting to be decoded or the marker is lost
        }
        
        if ((new_frame = tdav_video_frame_create(rtp_pkt, iNAP, iCheckCodeLen))){
            if (webrtc_pkt_flag) {
                new_frame->iFrameType = uFrameType;
            }

            // compute avg frame duration
            if (jb_mgr->state->m_last_timestamp && jb_mgr->state->m_last_timestamp < rtp_pkt->header->timestamp) {
                uint32_t duration = 0;
                // webrtc过来的视频数据包时间戳单位是1/90K; video SDK过来的视频数据包时间戳单位是ms
                if (webrtc_pkt_flag) {
                    duration = (rtp_pkt->header->timestamp - jb_mgr->state->m_last_timestamp) / jb_mgr->state->m_rate; // 单位: ms, 90/ms
                } else {
                    duration = (rtp_pkt->header->timestamp - jb_mgr->state->m_last_timestamp);
                }

                //TSK_DEBUG_ERROR(" check duration to calculate fps, session_id=%u, duration=%u",jb_mgr->session_id, duration);
                // 通过接收到视频的timestamp估计fps，从而调整jitterbuffer长度
                jb_mgr->state->m_avg_duration = jb_mgr->state->m_avg_duration ? ((jb_mgr->state->m_avg_duration + duration) >> 1) : duration;
                --jb_mgr->state->m_fps_prob;
            }
            jb_mgr->state->m_last_timestamp = rtp_pkt->header->timestamp;

            tsk_list_lock(jb_mgr->state->m_frames);
            // 保护，就是frame链表最大只能装tail_max这么多的frame，超过了就要丢
            tsk_size_t frame_count = tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null);
            
            if(frame_count >= jb_mgr->state->m_tail_max){
                if(++jb_mgr->state->m_conseq_frame_drop >= jb_mgr->state->m_tail_max){
                    TSK_DEBUG_ERROR("Too many frames dropped and tail_max=%d", jb_mgr->state->m_tail_max);
                    tsk_list_clear_items(jb_mgr->state->m_frames);
                    jb_mgr->state->m_conseq_frame_drop = 0;
                    jb_mgr->ssrc = rtp_pkt->header->ssrc;
                    tdav_video_request_pli(jb_mgr, rtp_pkt->header->session_id, rtp_pkt->header->video_id);
                    /*
                    if(self->callback){
                        self->cb_data_any.type = tdav_video_jb_cb_data_type_tmfr;
                        self->cb_data_any.ssrc = rtp_pkt->header->ssrc;
                        self->callback(&self->cb_data_any);
                    }*/
                }
                else{
                    /*const tdav_video_frame_t* head = (const tdav_video_frame_t*)jb_mgr->state->m_frames->head->data;
                    tsk_size_t list_count = tsk_list_count(head->pkts, tsk_null, tsk_null);
                    if (list_count != head->framePktCount) {
                        TSK_DEBUG_INFO("Dropping video frame :%d  %d",(int)head->framePktCount ,list_count);
                        
                    }*/
                    tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                    tsk_list_remove_first_item(jb_mgr->state->m_frames);
                    
                    //移除后面所有的包，知道I 帧出现
                   /* while (tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null) !=0) {
                        tdav_video_frame_t*tmpFrame =  (tdav_video_frame_t*)jb_mgr->state->m_frames->head->data;
                        if (tmpFrame == nullptr) {
                            continue;
                        }
                    

                        if (tmpFrame->iFrameType == 1) {
                            break;
                        }
                    }*/

                };
                //tdav_video_jb_reset_fps_prob(self);
                jb_mgr->state->m_fps_prob = TDAV_VIDEO_JB_FPS_PROB;
                jb_mgr->state->m_last_timestamp = 0;
                jb_mgr->state->m_avg_duration = 0;
                jb_mgr->rscode = rscode;
            }
           
            
            tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(new_frame));
            if( new_frame->iFrameSerial < 1000){
               tsk_list_push_back_data(jb_mgr->state->m_frames, (void**)&temp);
            }else{
               tsk_list_push_ascending_data(jb_mgr->state->m_frames, (void**)&temp);
            }
            tsk_list_unlock(jb_mgr->state->m_frames);
            
            //判断是否可以通知上层
            if(webrtc_pkt_flag)
            {
                // 兼容Safari处理, 根据markerbit作为帧结束标志
                // 视频帧中包类型可能包含type_1，type_fu-a, type_stap-a
                if (rtp_pkt->header->marker) {
                    new_frame->marker = rtp_pkt->header->marker;
                    new_frame->framePktCount = rtp_pkt->header->seq_num - new_frame->first_seq_num + 1;;
                    new_frame->iFrameSerial = new_frame->first_seq_num;
                    tdav_video_jb_reset_csrc2(new_frame);
                    
                    // 收到一个独立的新的markerbit包，暂时先不发信号，看看后面是否有乱序的数据包到达
                    return 0;
                }

            }

            tsk_size_t list_count = tsk_list_count(new_frame->pkts, tsk_null, tsk_null);
            if (list_count == new_frame->framePktCount) {
                tsk_semaphore_increment(jb_mgr->jb_full_frame_sema);
            }
            tsk_object_unref(new_frame);
        }
        if(jb_mgr->state->m_fps_prob <= 0 && jb_mgr->state->m_avg_duration){
            // compute FPS using timestamp values
            int32_t fps_new = (1000 / jb_mgr->state->m_avg_duration);
//            int32_t fps_old = jb_mgr->state->m_fps;
            jb_mgr->state->m_fps = TSK_CLAMP(TDAV_VIDEO_JB_FPS_MIN, fps_new, TDAV_VIDEO_JB_FPS_MAX);
            jb_mgr->state->m_tail_max = (jb_mgr->state->m_fps << TDAV_VIDEO_JB_TAIL_MAX_LOG2); // maximum delay = 2 seconds
            jb_mgr->state->m_latency_max = 13; // maximum = 1 second
            TSK_DEBUG_INFO("According to rtp-timestamps ...FPS = %d (clipped to %d) tail_max=%d, latency_max=%u", fps_new, jb_mgr->state->m_fps, jb_mgr->state->m_tail_max, (unsigned)jb_mgr->state->m_latency_max);
            //tdav_video_jb_reset_fps_prob(self);
            jb_mgr->state->m_fps_prob = TDAV_VIDEO_JB_FPS_PROB;
            jb_mgr->state->m_last_timestamp = 0;
            jb_mgr->state->m_avg_duration = 0;
            jb_mgr->rscode = rscode;
            /*
            if(self->callback && (fps_old != self->fps)){
                self->cb_data_any.type = tdav_video_jb_cb_data_type_fps_changed;
                self->cb_data_any.ssrc = rtp_pkt->header->ssrc;
                self->cb_data_any.fps.new_fps = self->fps; // clipped value
                self->cb_data_any.fps.old_fps = fps_old;
                self->callback(&self->cb_data_any);
            }*/
        }
    } else { // 当前packet的timestamp能在frame list里面找到，所以表示当前packet是属于上一个frame的切包，所以要合并到那个frame里面
        tdav_video_frame_put((tdav_video_frame_t*)old_frame, rtp_pkt);
        if(webrtc_pkt_flag)
        {
            // 视频数据以markerbit作为帧结束标志
            if (rtp_pkt->header->marker)
                old_frame->marker = rtp_pkt->header->marker;
                
            // 考虑该帧第一个包晚到的情况，包括fu-a第一个分包，或者safari的第一个数据包
            if (rtp_pkt->header->seq_num < old_frame->first_seq_num) {
                old_frame->first_seq_num = rtp_pkt->header->seq_num;
            }
            
            // 防止数据包乱序到达，以该帧最大序列号来计算包数量
            tsk_size_t packet_count = old_frame->highest_seq_num - old_frame->first_seq_num + 1;
            if (old_frame->marker)
            {
                old_frame->framePktCount = packet_count;
                old_frame->iFrameSerial = old_frame->first_seq_num;
                tdav_video_jb_reset_csrc2(old_frame);
            }
        }
        
        tsk_size_t list_count = tsk_list_count(old_frame->pkts, tsk_null, tsk_null);
        if (list_count == old_frame->framePktCount) {
            tsk_semaphore_increment(jb_mgr->jb_full_frame_sema);
        }
        
        tsk_object_unref( TSK_OBJECT(old_frame) );
    }
    
    // *seq_num = rtp_pkt->header->seq_num;
    //tsk_mutex_unlock(jb_mgr->state->videoJitterMutex);
    

    
#else
    // 用简单的jitter buffer，链表实现, 不考虑乱序
    tsk_safeobj_lock(self);
    tdav_video_frame_t* new_frame;
    new_frame = tdav_video_frame_create(rtp_pkt,0,0);
    tsk_list_lock(self->frames);
    tsk_list_push_back_data(self->frames, (void**)&new_frame);
    tsk_list_unlock(self->frames);
    self->frames_count++; // 链表frame count自增
    tsk_safeobj_unlock(self);
    
#endif
#endif
    
    return 0;
}

int tdav_video_jb_stop(tdav_video_jb_t* self)
{
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(!self->started){
        return 0;
    }
    
    TSK_DEBUG_INFO("tdav_video_jb_stop()");
    
    self->started = tsk_false;
    
    // reset default values to make sure next start will be called with right defaults
    // do not call this function in start to avoid overriding values defined between prepare() and start()
    _tdav_video_jb_set_defaults(self);
    
    return 0;
}

static int _tdav_video_jb_set_defaults(struct tdav_video_jb_s* self)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    return 0;
}

static const tdav_video_frame_t* _tdav_video_jb_mgr_get_frame(video_jb_manager_t* self, trtp_rtp_packet_t* packet, tsk_bool_t *pt_matched)
{
    const tdav_video_frame_t* old_frame = tsk_null;
    const tsk_list_item_t *item;
    
    *pt_matched =tsk_false;
    uint16_t uFrameSerial = packet->header->csrc[2] >> 16;
    uint16_t uFrameCount = packet->header->csrc[2] & 0x0000ffff;
    if (uFrameCount) {
        tsk_list_lock(self->state->m_frames);
        tsk_list_foreach(item, self->state->m_frames) {
            if (TDAV_VIDEO_FRAME(item->data)->payload_type == packet->header->payload_type
                && TDAV_VIDEO_FRAME(item->data)->iVideoId == packet->header->video_id) {
                if (!(*pt_matched)) *pt_matched = tsk_true;
                uint8_t nalu_type = ((uint8_t*)(packet->payload.data))[0] & 0x1F;
                if (H264_NAL_FU_A == nalu_type) {
                    nalu_type = ((uint8_t*)(packet->payload.data))[1] & 0x1F;
                }
                // ugly code!!
                // 判断条件：
                // 1、一个较大的nalu(例如I帧)分片成多个RTP包，需要聚合在一起再发送给硬解码器
                // 2、由于兼容webrtc浏览器，SPS/PPS与IDR独立发送，在接收端需要聚合一起发送给硬解码器，否则会出现解码延迟
                //   目前SPS/PPS的序列号比IDR的序列号小1, SPS/PPS和IDR的时间戳相同
                if (TDAV_VIDEO_FRAME(item->data)->iFrameSerial == uFrameSerial
                || (TDAV_VIDEO_FRAME(item->data)->timestamp == packet->header->timestamp)) {
                    if (nalu_type == H264_NAL_SLICE_IDR
                        || (nalu_type == H264_NAL_SLICE && TDAV_VIDEO_FRAME(item->data)->framePktCount == 0)) {
                        // IDR frame count + SPS/PPS
                        // hack: qcom 650 出现在sps之后，nalu_type 是 1
                        TDAV_VIDEO_FRAME(item->data)->framePktCount = uFrameCount + 1;
                        TDAV_VIDEO_FRAME(item->data)->iFrameSerial = uFrameSerial - 1;
                    }
                    old_frame = (const tdav_video_frame_t*)item->data;
                    tsk_object_ref( TSK_OBJECT(old_frame) );
                    break;
                }
            }
        }
        tsk_list_unlock(self->state->m_frames);
    } else {
        // safari浏览器对视频分片的处理是根据ts来区分，以marker位来区分结束
        // 即可能会有多个slice，同时每个slice的封包处理都不一样
        // chrome和firefox也可以此来作为参考

        tsk_list_lock(self->state->m_frames);
        tsk_list_foreach(item, self->state->m_frames) {
            if (TDAV_VIDEO_FRAME(item->data)->timestamp == packet->header->timestamp) {
                old_frame = (const tdav_video_frame_t*)item->data;
                tsk_object_ref( TSK_OBJECT(old_frame));
                break;
            }
        }
        tsk_list_unlock(self->state->m_frames);
    }
    return old_frame;
}

static void tdav_clear_frame_pkts(tdav_video_frame_t* frame)
{
    if (!frame) {
        return;
    }
    
    tsk_list_item_t* item;
    tsk_list_lock(frame->pkts);
    tsk_list_foreach(item, frame->pkts){
        trtp_rtp_packet_t* pkt = (trtp_rtp_packet_t*)item->data;
        tsk_free(&pkt->payload.data);
        
//        TSK_DEBUG_ERROR("pkt_ref[%d] header_ref[%d]", tsk_object_get_refcount(pkt), tsk_object_get_refcount(pkt->header));
    }
    tsk_list_clear_items(frame->pkts);
    tsk_list_unlock(frame->pkts);
    
    tsk_object_unref(frame->pkts);
    frame->pkts = nullptr;
}

static void* TSK_STDCALL _tdav_video_jb_smooth_thread_func(void *arg)
{
    tsk_list_item_t* item;
    trtp_rtp_packet_t* pkt;
    video_jb_manager_t* jb_mgr = (video_jb_manager_t*)arg;
    tdav_video_frame_t* frame = nullptr;
    while(true) {
        
        // tsk_list_lock(jb_mgr->state->m_frames);
        if (!jb_mgr->jb_decode_thread_start) {
        //     tsk_list_unlock(jb_mgr->state->m_frames);
            break;
        }
        
        int64_t curTimeMs = tsk_gettimeofday_ms();
        jb_mgr->diffMs = curTimeMs - jb_mgr->lastCallBackTime;
        
        //TSK_DEBUG_INFO("mark smooth:%d, diff:%lld, smoothms:%lld", jb_mgr->smooth, jb_mgr->diffMs, jb_mgr->smoothTimeMs);
        // 丢包率大于5则跳过smooth，减小延时
        
        if (jb_mgr->smooth && jb_mgr->diffMs < jb_mgr->smoothTimeMs) {
            xt_timer_wait(5);
            continue;
        }

        if(!jb_mgr->state || !jb_mgr->state->m_frames){
            xt_timer_wait(10);
            continue;
        }
        
        if (!jb_mgr->jb_decode_thread_start) {
            break;
        }

        tsk_list_lock(jb_mgr->state->m_frames);
        if ((jb_mgr->state->m_frames->head == nullptr) || ((frame = (tdav_video_frame_t*)jb_mgr->state->m_frames->head->data) == nullptr)){
            tsk_list_unlock(jb_mgr->state->m_frames);
            xt_timer_wait(10);
            continue;
        }
        
        tsk_size_t list_count = tsk_list_count(frame->pkts, tsk_null, tsk_null);
        if (list_count > 0 && list_count >= frame->framePktCount && frame->framePktCount) {
            if (frame->framePktCount < list_count) {
                TSK_DEBUG_WARN("check frame, framePktCount:%d listCount:%d", frame->framePktCount, list_count);
                frame->framePktCount = list_count;
            }
			uint8_t iFrameType = frame->iFrameType;
			if (!jb_mgr->bHasLostPacket && iFrameType == 0 && jb_mgr->nextFrameSerial != 0 && frame->iFrameSerial != jb_mgr->nextFrameSerial){
                // 丢弃序号不连续的p帧，避免花屏
                jb_mgr->iPFrameLossCount ++;
                int serialDiff = jb_mgr->nextFrameSerial - frame->iFrameSerial;
                //TSK_DEBUG_WARN("drop need serial:%d geted:%d", (int)jb_mgr->nextFrameSerial, (int)frame->iFrameSerial);
                
                int retry_cnt = 10;
                int max_video_dn_lossrate = Config_GetInt("video_dn_lossrate", 0);
                if (max_video_dn_lossrate > 30) {
                    retry_cnt = 25;
                } else if (max_video_dn_lossrate >= 20) {
                    retry_cnt = 20;
                } else if (max_video_dn_lossrate >= 10) {
                    retry_cnt = 20;
                } else if (max_video_dn_lossrate >= 5) {
                    retry_cnt = 15;
                }
                
                if(jb_mgr->smooth && serialDiff != -1)
                {
                    retry_cnt -= 5;
                }
                if(frame->iVideoId == 1 && jb_mgr->rscode && !jb_mgr->rscode->rscodeBreak){
                    retry_cnt += 20;
                }
                if(serialDiff < -2 || serialDiff > 0 || jb_mgr->iPFrameLossCount > retry_cnt){
                    TSK_DEBUG_WARN("drop break frame,need serial:%d geted:%d retry:%d", (int)jb_mgr->nextFrameSerial, (int)frame->iFrameSerial,jb_mgr->iPFrameLossCount);
                    jb_mgr->iPFrameLossCount = 0;
                    tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                    tsk_list_remove_first_item(jb_mgr->state->m_frames);
                    if(serialDiff < 0 ) jb_mgr->bHasLostPacket = true;
                }
				tsk_list_unlock(jb_mgr->state->m_frames);

                xt_timer_wait(5);
				continue;
			}
			 
			if ((!jb_mgr->bHasLostPacket) || (iFrameType == 1)) {
                 jb_mgr->lastCallBackTime = curTimeMs;
                //收完了一帧了，给解码器
//                TSK_DEBUG_ERROR("渲染");
                jb_mgr->iPFrameLossCount = 0;
                tsk_list_foreach(item, frame->pkts){
                    tsk_boolean_t last_pkt = tsk_false;
                    if(!(pkt = (trtp_rtp_packet_t*)item->data) || !pkt->payload.size || !pkt->header){
                        TSK_DEBUG_ERROR("Skipping invalid rtp packet (do not decode!)");
                        continue;
                    }
                    list_count--;
                    if (!list_count) {
                        last_pkt = tsk_true;
                    }
                    jb_mgr->state->cb_data_rtp.rtp.pkt = pkt;
                    jb_mgr->state->cb_data_rtp.rtp.pkt->header->is_last_pkts = last_pkt;
                    jb_mgr->state->m_callback(&jb_mgr->state->cb_data_rtp);
                    if(pkt->header->marker){
                        jb_mgr->state->m_decode_last_seq_num_with_mark = pkt->header->seq_num;
                    }
                }
				jb_mgr->nextFrameSerial = frame->iFrameSerial + frame->framePktCount;
				//TSK_DEBUG_ERROR("cost frame,type:%d num.:%d,nnum:%d pktcount:%d", (int)frame->iFrameType, (int)frame->iFrameSerial, (int)frame->iFrameSerial + (int)frame->framePktCount, (int)frame->framePktCount);
				if (iFrameType == 1) {
                    //遇到了I帧 重置
                    jb_mgr->bHasLostPacket=false;
                    jb_mgr->iTotalIFrame++;
                    //                    if (jb_mgr->state->m_callback) {
                    //                        jb_mgr->state->cb_data_any.type=video_jitter_cb_data_type_fl;
                    //                        jb_mgr->state->cb_data_any.fl.count=jb_mgr->iTotalIFrame;
                    //                        jb_mgr->state->cb_data_any.fl.seq_num=jb_mgr->iLostIFrame;
                    //                        jb_mgr->state->m_callback(&jb_mgr->state->cb_data_any);
                    //                    }
                    
                }
                tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                tsk_list_remove_first_item(jb_mgr->state->m_frames);
                
             }else{
                 tsk_list_unlock(jb_mgr->state->m_frames);
                 xt_timer_wait(5);
                 continue;
//                 jb_mgr->lastCallBackTime = curTimeMs;
//                 TSK_DEBUG_ERROR("渲染 丢");
             }
        } else {
            tsk_list_unlock(jb_mgr->state->m_frames);
            xt_timer_wait(5);
            continue;
        }
        tsk_list_unlock(jb_mgr->state->m_frames);
        
    }
    return tsk_null;
}

static void* TSK_STDCALL _tdav_video_jb_decode_thread_func(void *param)
{
    video_jb_manager_t* jb_mgr = (video_jb_manager_t*)param;
    //uint64_t delay;

    //tsk_list_item_t* item;
    //trtp_rtp_packet_t* pkt;
    //uint64_t next_decode_duration = 0, now, _now, latency = 0;
    //uint64_t x_decode_duration = (1000 / jb->fps); // expected
    //uint64_t x_decode_time = tsk_time_now();//expected
    
    TSK_DEBUG_INFO("Video jitter buffer thread - ENTER");
    
    int smoothCountTimeMs = 1000;
    /*
#if defined(ANDROID)
    int smoothCheckTimeMs  = 6;
    float smoothMultiple = 1.26;//丢包情况下，收包不均匀，配置一个平滑播放倍数
#else
    int smoothCheckTimeMs  = 6;
    float smoothMultiple = 1.2;//丢包情况下，收包不均匀，配置一个平滑播放倍数
#endif
     */
 
    int retryWaitTimeMs = 25;
    
    int totalNoLossTimes = 450; //连续不丢帧次数
    int lossTimesCheckTimes = 0;
    int smoothOnCount = Config_GetInt("SMOOTH_FRAME", 2);
    
    int frameCountPer5Seconds = 0;
    int64_t lastCountTime = tsk_gettimeofday_ms();
    //uint64_t lastFrameTime = 0;
    int64_t lastLossTimestampMs = 0;
    const int lossFrameRequestIdrTimeMs = 1000; //Maximum frame loss interval
    
    //long lSmoothTimer = xt_timer_mgr_global_schedule_loop(smoothCheckTimeMs, smooth_tsk_timer_callback_f, param);
    while(true) {
        if (!jb_mgr->jb_decode_thread_start) {
            break;
        }
        
        if (0 != tsk_semaphore_decrement(jb_mgr->jb_full_frame_sema)) {
            TSK_DEBUG_ERROR("Fatal error: jb full_sema failed");
            continue;
        }
        if (!jb_mgr->jb_decode_thread_start) {
            break;
        }
        const tdav_video_frame_t* frame = nullptr;
        tsk_list_lock(jb_mgr->state->m_frames);
        if ((jb_mgr->state->m_frames->head == nullptr) || ((frame = (const tdav_video_frame_t*)jb_mgr->state->m_frames->head->data) == nullptr)){
            tsk_list_unlock(jb_mgr->state->m_frames);
            continue;
        }
  
        //用来标识是否需要等待一下，如果跳过了I帧率，需要等待一下
        bool bWaitIFrame = false;
        tsk_size_t list_count = tsk_list_count(frame->pkts, tsk_null, tsk_null);
        if (list_count == frame->framePktCount) {
            //收完了一帧了，计算帧率和平滑参数
            ++ totalNoLossTimes;      //连续不丢帧次数
            ++ frameCountPer5Seconds; //帧累计
            if( frameCountPer5Seconds % 10 == 0){
                int64_t curTimeMs = tsk_gettimeofday_ms();
                int64_t diffTime = curTimeMs - lastCountTime;
                if(diffTime > smoothCountTimeMs ){
                    lastCountTime = curTimeMs;
                    tsk_size_t frameCount = tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null);
                    if(frameCount > 2 * jb_mgr->state->m_fps){
                        tsk_list_item_t* item = jb_mgr->state->m_frames->head->next;
                        while ( item ) {
                            tdav_video_frame_t* tmpFrame = (tdav_video_frame_t*)item->data;
                            if (tmpFrame == nullptr) {
                                break;
                            }
                            if (tmpFrame->iFrameType == 1) {
                                jb_mgr->bHasLostPacket = true;
                                TSK_DEBUG_ERROR("drop frame t1: %d serial:%d", (int)tmpFrame->iFrameType , tmpFrame->iFrameSerial);
                                if(jb_mgr->rscode != tsk_null){
                                    jb_mgr->rscode->rscodeBreak = tsk_false;
                                }
                                
                                tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                                tsk_list_remove_first_item(jb_mgr->state->m_frames);
                                frame =  nullptr;
                                break;
                            }
                            item = item->next;
                        }
                        
                        jb_mgr->smooth = tsk_false;
                        if (!frame) {
                            tsk_list_unlock(jb_mgr->state->m_frames);
                            continue;
                        }
                    }
                    int max_video_dn_lossrate = Config_GetInt("video_dn_lossrate", 0);

                    //根据下行丢包率决定smooth, 丢包率大时smooth长一些便于等待后面frame到达
                    float speed = 1.0f; //((frameCount > 5) ? 1.2f : 1.0f );
                    if (frameCount > 15 || max_video_dn_lossrate < 5) {
                        speed = 1.4f;
                    } else if (max_video_dn_lossrate <= 10) {
                        speed = 1.2f;
                    } else if (max_video_dn_lossrate <= 18) {
                        speed = 1.1f;
                    } else if (max_video_dn_lossrate <= 28) {
                        speed = 1.0f;
                    } else {
                        speed = 0.8f;
                    }
                    
                    uint32_t fps = frameCountPer5Seconds / (diffTime/1000.0f);
                    uint32_t smoothTime = TSK_CLAMP(1,(int)(diffTime/ frameCountPer5Seconds / speed),fps >= 20 ? 33 : (frameCountPer5Seconds > 15 ? 45 : 60)) ;
                    jb_mgr->smoothTimeMs  = smoothTime;
                    lossTimesCheckTimes = 0;
                    //jb_mgr->smooth = frameCount > smoothOnCount; //出现帧积累就开启平滑
                    
                    if (max_video_dn_lossrate < 5) {
                        jb_mgr->smooth = (smoothOnCount != 0 && frameCount > smoothOnCount); //出现帧积累就开启平滑
                    } else {
                        jb_mgr->smooth = (smoothOnCount > 0);   //如果服务器配置 smoothOnCount == 0 就表示关闭平滑
                    }
                    
                    TMEDIA_I_AM_ACTIVE(10,"smooth ms update to %lld ms, fps:%d ,frames:%ld smooth:%d",jb_mgr->smoothTimeMs,fps,frameCount,jb_mgr->smooth);
                    frameCountPer5Seconds = 0;
                }
            }
            
            if (frame->iFrameType == 1) {
                //遇到了I帧 重置
                jb_mgr->bHasLostPacket=false;
            }
            if (jb_mgr->bHasLostPacket && frame->iFrameType !=1) {
                //丢帧时，丢掉之后的P帧，直到下一个I帧
                TSK_DEBUG_ERROR("drop P 3 %d", frame->iFrameSerial);
                
                tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                tsk_list_remove_first_item(jb_mgr->state->m_frames);
            }
            
            if(((frameCountPer5Seconds % 3) == 0) && jb_mgr->rscode != tsk_null){
                jb_mgr->rscode->rscodeBreak = tsk_false;
            }
            
        }
        else
        {
            //检测积累情况，积累太多就直接跳到最近的I帧
            int listCount = tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null);
            int maxCount = 2 * jb_mgr->state->m_fps;
            maxCount = TSK_MIN(maxCount, 40);

            if(listCount > maxCount ){ //避免积累太多
                tsk_list_item_t* item = jb_mgr->state->m_frames->head->next;
                while ( item ) {
                    tdav_video_frame_t* tmpFrame = (tdav_video_frame_t*)item->data;
                    if (tmpFrame == nullptr) {
                        break;
                    }
                    if (tmpFrame->iFrameType == 1) { // 1 means key frame
                        jb_mgr->bHasLostPacket = true;
                        TSK_DEBUG_ERROR("drop frame t2: %d %u %d", (int)tmpFrame->iFrameType, tmpFrame->timestamp,maxCount);
                        if(jb_mgr->rscode != tsk_null){
                            jb_mgr->rscode->rscodeBreak = tsk_false;
                        }
                        
                        tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                        tsk_list_remove_first_item(jb_mgr->state->m_frames);
                        break;
                    }
                    item = item->next;
                }
            }
            int videoId  = 0;
            int64_t lastFrameTiemstampMs = 0;
            // 遍历所有的frame，找到一个满的
            // P帧也添加了重试，这儿不用设置为丢包，但是可能花屏
            while ((listCount = tsk_list_count(jb_mgr->state->m_frames, tsk_null, tsk_null)) !=0) {
                //如果是I帧的话
                tdav_video_frame_t* tmpFrame = (tdav_video_frame_t*)jb_mgr->state->m_frames->head->data;
				
                //I 帧率，并且满了就留下
                if ((tmpFrame->framePktCount == tsk_list_count(tmpFrame->pkts, tsk_null, tsk_null))
                        &&(tmpFrame->iFrameType == 1))
                {
                    jb_mgr->bHasLostPacket = false;
                    lastLossTimestampMs = 0;
                    break;
                }else if(jb_mgr->bHasLostPacket && tmpFrame->iFrameType == 0){
                    ++ frameCountPer5Seconds;
                    //TSK_DEBUG_ERROR("drop P 1: %d %d",(int)tmpFrame->framePktCount,tsk_list_count(tmpFrame->pkts, tsk_null, tsk_null));
                    
                    if (!lastLossTimestampMs) {
                        lastLossTimestampMs = tmpFrame->timestamp;
                        jb_mgr->ssrc = tmpFrame->ssrc;
                        videoId = tmpFrame->iVideoId;
                    }
                    lastFrameTiemstampMs = tmpFrame->timestamp;
                    tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                    tsk_list_remove_first_item(jb_mgr->state->m_frames);
                    continue;
                }
                jb_mgr->bHasLostPacket = false; //先设置为 false
                
                retryWaitTimeMs = 15;
                
                //都重试n次
                // if(listCount < 15 && jb_mgr->rscode != tsk_null && jb_mgr->rscode->rscodeBreak == tsk_false && tmpFrame->iCheckCodeLen>0){//rscode没有出现解包失败再重试
                //     if(tmpFrame->iCheckCodeLen == 1) {
                //         retryWaitTimeMs = 30;
                //         maxRetryTimes = 3;
                //     }else{
                //         int fecRate = (tmpFrame->iNAP / tmpFrame->iCheckCodeLen);
                //         fecRate = fecRate > 0 ? fecRate : 1;
                //         if(fecRate)
                //         {
                //             maxRetryTimes = TSK_CLAMP(4, 50/fecRate, 36);
                //             if(maxRetryTimes > 16){
                //                 retryWaitTimeMs = 50;
                //                 maxRetryTimes = 15;
                //             }
                //         }
                //     }
                // }
                if(jb_mgr->state->m_frames->tail != tsk_null && tmpFrame->iVideoId != ((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->iVideoId){
                    TSK_DEBUG_ERROR("drop remove last item: current:%d  last:%d",tmpFrame->iVideoId,((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->iVideoId);
                    tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data);
                    tsk_list_remove_last_item(jb_mgr->state->m_frames);
                }
                
                uint32_t maxGroup = tmpFrame->iFrameType == 1 ? tmpFrame->packetGroup + 1 : tmpFrame->packetGroup;
                uint16_t tailGroup = jb_mgr->state->m_frames->tail == tsk_null ? 1 : ((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->packetGroup;
                int32_t groupDiff = jb_mgr->state->m_frames->tail == tsk_null ? 1 : (maxGroup - tailGroup);
                if(jb_mgr->rscode != tsk_null && tailGroup !=0 &&
                   groupDiff >= 0 && groupDiff< 2
                )
                {
                    /*
                    if(jb_mgr->state->m_frames->tail != tsk_null){
                        TSK_DEBUG_ERROR("drop retry last videoid:%d",((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->iVideoId);
                    }
                    TSK_DEBUG_ERROR("drop retry:%d %d %d videoid:%d",((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->packetGroup, maxGroup,tmpFrame->iRetryCount,tmpFrame->iVideoId);
                     */
                    ++tmpFrame->iRetryCount;
                    if(tmpFrame->iRetryCount < 100){
                        //避免发送端突然不发包，一直重试
                        bWaitIFrame = true;
                        break;
                    }
                }
                
                //TSK_DEBUG_ERROR("drop maxRetryTimes:%d %d %d",((tdav_video_frame_t*)jb_mgr->state->m_frames->tail->data)->packetGroup, maxGroup,tmpFrame->iRetryCount);
                /*
                int maxGroup = tmpFrame->iFrameType == 1 ? tmpFrame->packetGroup + 2 : tmpFrame->packetGroup + 1;
                if (jb_mgr->rscode != tsk_null && jb_mgr->rscode->iPacketGroupSeq <= maxGroup) {
                    tmpFrame->iRetryCount++;
                    bWaitIFrame = true;
                    //TSK_DEBUG_ERROR("drop maxRetryTimes:%d %d %d",tmpFrame->iRetryCount,tmpFrame->packetGroup, jb_mgr->rscode->iPacketGroupSeq );
                    //TSK_DEBUG_ERROR("retry:%d %d",(int)tmpFrame->framePktCount,tsk_list_count(tmpFrame->pkts, tsk_null, tsk_null));
                    break;
                }
                
                if(jb_mgr->rscode != tsk_null){
                    jb_mgr->rscode->rscodeBreak = tsk_false;
                }
                 */
                
                jb_mgr->bHasLostPacket = true; //设置为true之后，就会等到下一个I帧才渲染
                
                if ((tmpFrame->iFrameType == 1) && jb_mgr->bHasLostPacket)
                {
                    jb_mgr->iTotalIFrame++;
                    jb_mgr->iLostIFrame++;
                    
                    if (jb_mgr->state->m_callback) {
                        jb_mgr->state->cb_data_any.type = video_jitter_cb_data_type_fl;
                        jb_mgr->state->cb_data_any.fl.count = jb_mgr->iTotalIFrame;
                        jb_mgr->state->cb_data_any.fl.seq_num = jb_mgr->iLostIFrame;
                        jb_mgr->state->cb_data_any.fl.sessionid = jb_mgr->session_id;
                        jb_mgr->state->m_callback(&jb_mgr->state->cb_data_any);
                    }
                    TSK_DEBUG_ERROR("drop I: %d %d %d %u"
                        ,(int)tmpFrame->framePktCount, tsk_list_count(tmpFrame->pkts, tsk_null, tsk_null)
                        , tmpFrame->iRetryCount, tmpFrame->timestamp);
                    // ======================= 重新计算重试次数,已经废弃，保留了丢帧计数器，备用 ======================
                    if (lossTimesCheckTimes % 2 == 0) {
                        lossTimesCheckTimes = 1;
                    } else{
                        ++ lossTimesCheckTimes;
                    }
                    lastLossTimestampMs = 0;
                    jb_mgr->ssrc = tmpFrame->ssrc;
                    tdav_video_request_pli(jb_mgr, jb_mgr->session_id, videoId);
                    // =============================================================
                }
                else
                {
                    TSK_DEBUG_ERROR("drop P 2: %d %d %d %d %u"
                        ,tmpFrame->iFrameSerial,(int)tmpFrame->framePktCount, tsk_list_count(tmpFrame->pkts, tsk_null, tsk_null)
                        , tmpFrame->iRetryCount, tmpFrame->timestamp);
                    if (!lastLossTimestampMs) {
                        lastLossTimestampMs = tmpFrame->timestamp;
                        jb_mgr->ssrc = tmpFrame->ssrc;
                        videoId = tmpFrame->iVideoId;
                    }
                    lastFrameTiemstampMs = tmpFrame->timestamp;
                }
                
                totalNoLossTimes = 0; //丢帧了，这个连续不丢帧次数计数器置为 0
                ++ frameCountPer5Seconds;
                //先清理当前帧
                tdav_clear_frame_pkts((tdav_video_frame_t*)jb_mgr->state->m_frames->head->data);
                tsk_list_remove_first_item(jb_mgr->state->m_frames);
                
                // AddVideoBlock( 1 , jb_mgr->session_id );
            
            }
            if (lastLossTimestampMs && lastFrameTiemstampMs - lastLossTimestampMs > lossFrameRequestIdrTimeMs) {
                tdav_video_request_pli(jb_mgr, jb_mgr->session_id, videoId);
                lastLossTimestampMs = 0;
            }
        }
        tsk_list_unlock(jb_mgr->state->m_frames);
        
        if (bWaitIFrame) {
            tsk_thread_sleep(retryWaitTimeMs);
            tsk_semaphore_increment(jb_mgr->jb_full_frame_sema);
        }
    }
    //xt_timer_mgr_global_cancel(lSmoothTimer);
    tsk_list_lock(jb_mgr->state->m_frames);
    tsk_list_clear_items(jb_mgr->state->m_frames);
    tsk_list_unlock(jb_mgr->state->m_frames);
    TSK_OBJECT_SAFE_FREE(jb_mgr->state->m_frames);
    
    TSK_DEBUG_INFO("Video jitter buffer thread - EXIT");
    
    return tsk_null;
}

/*
 * Copyright (C) 2010-2014 Mamadou DIOP.
 * Copyright (C) 2011-2014 Doubango Telecom.
 *
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

/**@file tdav_session_video.c
 * @brief Video Session plugin.
 *
 */
#include "tinysak_config.h"
#include "tinydav/video/tdav_session_video.h"
#include "tinydav/video/tdav_converter_video.h"
#include "tinydav/video/jb/tdav_video_jb.h"
#include "tinydav/codecs/h264/tdav_codec_h264_rtp.h"

// #include "tinydav/codecs/fec/tdav_codec_red.h"
// #include "tinydav/codecs/fec/tdav_codec_ulpfec.h"
#include "tinydav/codecs/rtp_extension/tdav_codec_rtp_extension.h"

#include "tinymedia/tmedia_converter_video.h"
#include "tinymedia/tmedia_consumer.h"
#include "tinymedia/tmedia_producer.h"
#include "tinymedia/tmedia_defaults.h"
#include "tinymedia/tmedia_params.h"

#include "tinyrtp/trtp_manager.h"
#include "tinyrtp/rtcp/trtp_rtcp_header.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinyrtp/rtcp/trtp_rtcp_packet.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_rr.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_sr.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_fb.h"
#include "tinydav/video/jb/youme_video_jitter.h"
#include "webrtc/common_video/h264/sps_parser.h"

#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_string.h"
#include "tmedia_utils.h"
#include <math.h>
#include "XOsWrapper.h"
#include "XConfigCWrapper.hpp"
#include <map>
#include <list>
#include <set>

#include "YouMeConstDefine.h"
// Minimum time between two incoming FIR. If smaller, the request from the remote party will be ignored
// Tell the encoder to send IDR frame if condition is met
#if METROPOLIS
#	define TDAV_SESSION_VIDEO_AVPF_FIR_HONOR_INTERVAL_MIN		0 // millis
#else
#	define TDAV_SESSION_VIDEO_AVPF_FIR_HONOR_INTERVAL_MIN		750 // millis
#endif
// Minimum time between two outgoing FIR. If smaller, the request from the remote party will be ignored
// Tell the RTCP session to request IDR if condition is met
#if METROPOLIS
#	define TDAV_SESSION_VIDEO_AVPF_FIR_REQUEST_INTERVAL_MIN		0 // millis
#else
#	define TDAV_SESSION_VIDEO_AVPF_FIR_REQUEST_INTERVAL_MIN		500 // millis
#endif

#define TDAV_SESSION_VIDEO_PKT_LOSS_PROB_BAD	2
#define TDAV_SESSION_VIDEO_PKT_LOSS_PROB_GOOD	6
#define TDAV_SESSION_VIDEO_PKT_LOSS_FACT_MIN	0
#define TDAV_SESSION_VIDEO_PKT_LOSS_FACT_MAX	8
#define TDAV_SESSION_VIDEO_PKT_LOSS_LOW			9
#define TDAV_SESSION_VIDEO_PKT_LOSS_MEDIUM		22
#define TDAV_SESSION_VIDEO_PKT_LOSS_HIGH		63

#if !defined(TDAV_SESSION_VIDEO_PKT_LOSS_NO_REPORT_BEFORE_INCREASING_BW)
#   define TDAV_SESSION_VIDEO_PKT_LOSS_NO_REPORT_BEFORE_INCREASING_BW  5000 // millis
#endif

// The maximum number of pakcet loss allowed
#define TDAV_SESSION_VIDEO_PKT_LOSS_MAX_COUNT_TO_REQUEST_FIR	50

#define TDAV_VIDEO_FRAME_CHECK_TIMEOUT         100 // unit: ms

#define TDAV_VIDEO_QOS_STAT_CHECK_TIMEOUT      1000 // unit: ms

#if !defined (TDAV_GOOG_REMB_FULL_SUPPORT)
#   define TDAV_GOOG_REMB_FULL_SUPPORT 0
#endif

static const tmedia_codec_action_t __action_encode_idr = tmedia_codec_action_encode_idr;
// static const tmedia_codec_action_t __action_encode_bw_up = tmedia_codec_action_bw_up;
// static const tmedia_codec_action_t __action_encode_bw_down = tmedia_codec_action_bw_down;

#define TDAV_SESSION_VIDEO(self) ((tdav_session_video_t*)(self))


// FIXME: lock ?
#define _tdav_session_video_codec_set(__self, __key, __value) \
{ \
static tmedia_param_t* __param = tsk_null; \
if(!__param){ \
__param = tmedia_param_create(tmedia_pat_set,  \
tmedia_video,  \
tmedia_ppt_codec,  \
tmedia_pvt_int32, \
__key, \
(void*)&__value); \
} \
if((__self)->encoder.codec && __param){ \
/*tsk_mutex_lock((__self)->encoder.h_mutex);*/ \
if(TDAV_SESSION_AV(__self)->producer && TDAV_SESSION_AV(__self)->producer->encoder.codec_id == (__self)->encoder.codec->id) { /* Whether the producer ourput encoded frames */ \
tmedia_producer_set(TDAV_SESSION_AV(__self)->producer, __param); \
} \
else { \
tmedia_codec_set((tmedia_codec_t*)(__self)->encoder.codec, __param); \
} \
/*tsk_mutex_unlock((__self)->encoder.h_mutex);*/ \
} \
/* TSK_OBJECT_SAFE_FREE(param); */ \
}

#define _tdav_session_video_remote_requested_idr(__self, __ssrc_media) { \
uint64_t __now = tsk_time_now(); \
tsk_bool_t too_close = tsk_false; \
if((__now - (__self)->avpf.last_fir_time) > TDAV_SESSION_VIDEO_AVPF_FIR_HONOR_INTERVAL_MIN){ /* guard to avoid sending too many FIR */ \
_tdav_session_video_codec_set((__self), "action", __action_encode_idr); \
}else { too_close = tsk_true; TSK_DEBUG_INFO("***IDR request tooo close(%llu ms)...ignoring****", (__now - (__self)->avpf.last_fir_time)); } \
if((__self)->cb_rtcpevent.func){ \
(__self)->cb_rtcpevent.func((__self)->cb_rtcpevent.context, tmedia_rtcp_event_type_fir, (__ssrc_media)); \
} \
if (!too_close) { /* if too close don't update "last_fir_time" to "now" to be sure interval will increase */ \
(__self)->avpf.last_fir_time = __now; \
} \
}
#define _tdav_session_video_local_request_idr(_session, _reason, _ssrc) \
{ \
tdav_session_video_t* video = (tdav_session_video_t*)_session; \
tdav_session_av_t* _base = (tdav_session_av_t*)_session; \
if ( (tmedia_get_video_nack_flag() & tdav_session_video_nack_fir) && ((_base)->avpf_mode_neg || (_base)->is_fb_fir_neg)) { \
/*return*/ trtp_manager_signal_frame_corrupted((_base)->rtp_manager, _ssrc); \
TSK_DEBUG_INFO("send fri request, ssrc:%u", _ssrc); \
} else { TSK_DEBUG_INFO("****fri request disable, ignoring****"); } }
//else if ((_session)->rfc5168_cb.fun) { \
///*return*/ (_session)->rfc5168_cb.fun((_session)->rfc5168_cb.usrdata, (_session), (_reason), tmedia_session_rfc5168_cmd_picture_fast_update); \
//} \
//}
#define _tdav_session_video_bw_up(__self) _tdav_session_video_codec_set(__self, "action", __action_encode_bw_up)
#define _tdav_session_video_bw_down(__self) _tdav_session_video_codec_set(__self, "action", __action_encode_bw_down)
#define _tdav_session_video_bw_kbps(__self, __bw_kbps) _tdav_session_video_codec_set(__self, "bw_kbps", __bw_kbps)


#define _tdav_session_video_reset_loss_prob(__self) \
{ \
(__self)->encoder.pkt_loss_level = tdav_session_video_pkt_loss_level_low; \
(__self)->encoder.pkt_loss_prob_bad = TDAV_SESSION_VIDEO_PKT_LOSS_PROB_BAD; \
(__self)->encoder.pkt_loss_prob_good = TDAV_SESSION_VIDEO_PKT_LOSS_PROB_GOOD; \
}

#define MAX_VIDEO_CONSUMER_FRAME_NUM 10

typedef struct decoder_frame_s
{
    TSK_DECLARE_OBJECT;
    
    void *_buffer;
    trtp_rtp_header_t *_rtp_hdr;
    tsk_size_t _size;
} decoder_frame_t;

extern const tsk_object_def_t *tdav_session_video_decoded_frame_def_t;

typedef struct decoder_consumer_manager_s
{
    tsk_list_t *decoder_frame_lst;
    tsk_bool_t decoder_first_put_frame;
    tsk_bool_t decoder_started;
    tsk_bool_t decoder_reported_no_frame;
    int32_t    decoder_session_id;
    uint32_t   decoder_last_frame_timeStamp;
    uint16_t   decoder_fps;
    uint16_t   decoder_lst_cnt;
    int32_t    decoder_no_frame_cnt;
    int32_t    decoder_frame_cnt;
    int32_t    decoder_last_frame_cnt;
    
    tsk_bool_t decoder_check_yuv_enable;
    uint32_t   decoder_check_yuv_cnt;
    uint32_t   decoder_check_yuv_cont;
    uint32_t   decoder_check_yuv_unit;
    tsk_bool_t   decoder_check_yuv_act;
    
    tsk_bool_t  decoder_codec_check_enable;
    uint32_t    decoder_codec_check_cnt;
    uint64_t    decoder_codec_input_time;
    uint64_t    decoder_codec_frame_cnt;
    uint32_t    decoder_jb_cnt;
    
    void                    *video_session;
    tsk_bool_t              consumer_thread_running;
    tsk_thread_handle_t*    consumer_thread_handle;
//    tsk_cond_handle_t*      consumer_thread_cond;
//    pthread_t               consumer_thread_handle;
//    pthread_mutex_t         consumer_thread_mutex;
//    pthread_cond_t          consumer_thread_cond;
    tsk_semaphore_handle_t  *consumer_thread_sema;
} decoder_consumer_manager_t;

typedef std::map<int32_t, decoder_consumer_manager_t*> decoder_consumer_map_t;
typedef std::map<int32_t, bool>  video_status_map_t;

typedef struct tdav_session_video_s
{
    TDAV_DECLARE_SESSION_AV;
    
    struct tdav_video_jb_s* jb;
    tsk_bool_t jb_enabled;
    tsk_bool_t zero_artifacts;
    tsk_bool_t fps_changed;
    tsk_bool_t started;
    
    struct{
        const void* context;
        tmedia_session_rtcp_onevent_cb_f func;
    } cb_rtcpevent;
    
    struct{
        void* buffer;
        tsk_size_t buffer_size;
        
        int rotation;
        tsk_bool_t scale_rotated_frames;
        
        void* conv_buffer;
        tsk_size_t conv_buffer_size;
        
        tdav_session_video_pkt_loss_level_t pkt_loss_level;
        int32_t pkt_loss_fact;
        int32_t pkt_loss_prob_good;
        int32_t pkt_loss_prob_bad;
        
        uint64_t last_frame_time;
        
        uint8_t payload_type;
        struct tmedia_codec_s* codec;
        tsk_mutex_handle_t* h_mutex;
        
        tsk_bool_t isFirstFrame;
        
        void *rtpExtBuffer;
        tsk_size_t rtpExtBufferSize;
    } encoder;
    
    struct{
        void* buffer;
        tsk_size_t buffer_size;
        
        predecodeCb_t predecodeCb;
        tsk_bool_t predecode_need_decode_and_render;
        
        void* conv_buffer;
        tsk_size_t conv_buffer_size;
        
        // latest decoded RTP seqnum
        uint16_t last_seqnum;
        // stream is corrupted if packets are lost
        tsk_bool_t stream_corrupted;
        uint64_t stream_corrupted_since;
        uint32_t last_corrupted_timestamp;
        
        uint8_t codec_payload_type;
        struct tmedia_codec_s* codec;
        uint64_t codec_decoded_frames_count;
    } decoder;
    
    struct {
        tsk_size_t consumerLastWidth;
        tsk_size_t consumerLastHeight;
        struct tmedia_converter_video_s* fromYUV420;
        
        tsk_size_t producerWidth;
        tsk_size_t producerHeight;
        tsk_size_t xProducerSize;
        struct tmedia_converter_video_s* toYUV420;
    } conv;
    
    struct{
        tsk_list_t* packets;
        tsk_size_t count;
        tsk_size_t max;
        uint64_t last_fir_time;
        uint64_t last_pli_time;
    } avpf;
    
    unsigned q1_n;
    unsigned q2_n;
    unsigned q3_n;
    unsigned q4_n;
    unsigned q5_n;
    tsk_mutex_handle_t* h_mutex_qos;
    
    tsk_list_t* rscode_list;
    
   // pthread_t              videoMonitorThread;
   // pthread_mutex_t        videoMonitorThreadMutex;
   // pthread_cond_t         videoMonitorThreadCond;
	tsk_thread_handle_t*    videoMonitorThread;
	tsk_cond_handle_t*      videoMonitorThreadCond;
    tsk_bool_t              videoMonitorThreadStart;
    decoder_consumer_map_t *video_consumer_map;
	video_status_map_t *    video_status_map;
	
    videoRuntimeEventCb_t   videoRuntimeEventCb;
    staticsQoseCb_t         staticsQosCb;

    videoEncodeAdjustCb_t   videoAdjustCb;
    videoTransRouteCb_t     videoTransRouteCb;
    videoDecodeParamCb_t    videoDecodeParamCb;

    int32_t                 lastUpAudioLoss_max;     // audio up lossrate of last tick
    int32_t                 lastDnAudioLoss_max;     // audio dn lossrate of last tick
    struct
    {
        // the time when the audio session was initialized
        uint64_t time_base_ms;
        // the last time when we sent a timestamp
        uint64_t last_send_timestamp_time_ms;
        uint32_t send_timestamp_period_ms;
        tsk_bool_t may_send_timestamp;
        // A buffer for holding timestamps from the senders, these timestamps will be bounced back
        // to the senders later.
        stat_timestamp_info_t timestamps[MAX_TIMESTAMP_NUM];
        // Number of timestamps held in the array "timestamps" above
        uint32_t timestamp_num;
        // A buffer for encoding the timestamps into the RTP headder extension
        uint8_t  timestampBouncedBuf[BOUNCED_TIMESTAMP_INFO_SIZE * MAX_TIMESTAMP_NUM];
        TSK_DECLARE_SAFEOBJ;
    } stat;
    
    tdav_session_av_qos_stat_t avQosStat;
    long        avQosStatTimer;

    // 外部输入长时间没有视频输入，强制刷最后一帧
    long    check_timer_id;
    uint64_t last_check_timestamp;
    uint8_t * last_frame_buf;
    uint32_t last_frame_fmt;
    uint32_t last_frame_size;
    uint64_t last_frame_timestamp;
    uint32_t refresh_timeout;
    uint32_t refresh_limit;
    uint32_t refresh_copy_interval;
    
    std::set<int>*  session_map;
    
    // nack flag
    uint32_t last_nack_flag;
}
tdav_session_video_t;

extern int trtp_rtcp_session_signal_pkt_loss(struct trtp_rtcp_session_s* self, trtp_rtcp_source_seq_map * source_seqs);
static int _tdav_session_video_set_defaults(tdav_session_video_t* self);
static int _tdav_session_video_jb_cb(const tdav_video_jb_cb_data_xt* data);
static int _tdav_session_video_open_decoder(tdav_session_video_t* self, uint8_t payload_type);
static int _tdav_session_video_decode(tdav_session_video_t* self, const trtp_rtp_packet_t* packet);
static int _tdav_session_video_set_callbacks(tmedia_session_t* self);

static void tdav_webrtc_decode_h264_callback(const unsigned char * buff, int bufflen, int64_t timestamp, const void* session, void* header, tsk_bool_t isTexture, int video_id);
static void tdav_webrtc_encode_h264_callback(const unsigned char * buff, int bufflen, int64_t timestamp, int video_id, const void* session);


static tsk_size_t tdav_session_video_encode_rtp_header_ext(tdav_session_video_t *video, void* payload, tsk_size_t payload_size);
static int tdav_session_video_decode_rtp_header_ext(tdav_session_video_t *video, const struct trtp_rtp_packet_s *packet);

//视频检测
static  tdav_session_video_check_unusual_t tdav_session_video_yuv_check(uint8_t* yuv_data, int width, int height);
static  tsk_bool_t tdav_session_save_file(uint8_t* data, uint32_t data_len);

#define TDAV_SESSION_VIDEO_CHECK_TOTAL              30         //检测次数
#define TDAV_SESSION_VIDEO_CHECK_UNIT               1000       //检测间隔时间（ms）
#define TDAV_SESSION_VIDEO_CHECK_CONT               3          //连续出现次数证明有问题
#define TDAV_SESSION_VIDEO_ENCODER_TIMEOUT          5000       //编码超时（ms）
#define TDAV_SESSION_VIDEO_DECODER_TIMEOUT          10000       //解码超时（ms）
#define TDAV_SESSION_VIDEO_LOST_PACKET_TIMEOUT      15000      //丢包超时时间（ms）

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint8_t VIDEO_MASK_TIMESTAMP_ORIGINAL      = 0x01;
static const uint8_t VIDEO_MASK_TIMESTAMP_BOUNCED            = 0x02;

#define CHECK_SIZE_LEFT(size, offset, minSize) \
if ((size - offset) < minSize) { \
return -1; \
}

int tdav_codec_video_rtp_extension_decode( tdav_session_video_t* video, uint8_t* pExtData, uint32_t extSize, VideoRtpHeaderExt_t* pDecodedExt )
{
    // Get number of mask bytes
    uint32_t offset = 0;
    uint8_t  maskByteNum = 0;
    uint8_t  maskByte1 = 0;
    
    if (!pExtData || !pDecodedExt || (extSize < 4)) {
        return -1;
    }
    
    memset(pDecodedExt, 0, sizeof(VideoRtpHeaderExt_t));
    
    //////////////////////////////////////////////////////////////////////////////////
    // Get the mask byte number
    maskByteNum = pExtData[offset++];
    if (0 == maskByteNum) {
        return -1;
    }
    
    if (maskByteNum >= 1) {
        maskByte1 = pExtData[offset++];
        offset += 2; // skip the header extension size field(2 bytes)
        
    }
    
    CHECK_SIZE_LEFT(extSize, offset, (maskByteNum - 1));
    
    /////////////////////////////////////////////////////////////////////////////////
    // Parse the data sizes
    
    if (maskByte1 & VIDEO_MASK_TIMESTAMP_BOUNCED) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint8_t));
        pDecodedExt->timestampBouncedSize = pExtData[offset];
        offset += sizeof(uint8_t);
    }
    
    
    if (maskByte1 & VIDEO_MASK_TIMESTAMP_ORIGINAL) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint32_t));
        pDecodedExt->hasTimestampOriginal = 1;
        pDecodedExt->timestampOriginal = tnet_ntohl(*((uint32_t*)&pExtData[offset]));
        offset += sizeof(uint32_t);
    }
    

    
    if ( pDecodedExt->timestampBouncedSize > 0) {
        CHECK_SIZE_LEFT(extSize, offset, (pDecodedExt->timestampBouncedSize));
        pDecodedExt->pTimestampBouncedData = &pExtData[offset];
        offset += pDecodedExt->timestampBouncedSize;
    }
    
    return 0;
}

tsk_size_t tdav_codec_video_rtp_extension_encode(tdav_session_video_t *video, VideoRtpHeaderExt_t *pHeaderExt, void **ppEncodedExt, tsk_size_t *pEncodedExtSize)
{
    tsk_size_t totalSize = 4; // the fixed 4 bytes per RFC3550
    uint32_t byteNumForSizes = 0; // number of bytes for the payload sizes
    uint8_t  maskByte1 = 0;
    uint8_t  *pEncodedExt = NULL; // pointer to the output buffer
    uint32_t sizeOff = 0; // offset for the payload size fields
    uint32_t payloadOff = 0; // offset for the payload
    
    
    if (!pHeaderExt || !ppEncodedExt || !pEncodedExtSize) {
        return 0;
    }
    
    if (pHeaderExt->hasTimestampOriginal) {
        // The timestamp field sizes are fixed, so no size field is needed
        maskByte1 |= VIDEO_MASK_TIMESTAMP_ORIGINAL;
        totalSize += sizeof(uint32_t);
    }
    if (pHeaderExt->pTimestampBouncedData && (pHeaderExt->timestampBouncedSize > 0)) {
        maskByte1 |= VIDEO_MASK_TIMESTAMP_BOUNCED;
        byteNumForSizes += sizeof(uint8_t);
        totalSize += (tsk_size_t)pHeaderExt->timestampBouncedSize;
    }
    
    totalSize += (tsk_size_t)byteNumForSizes;
    totalSize = ((totalSize + 3) / 4) * 4; // word(4 bytes) algined
    
    // If no payload is going to be carried in the header extension, just return 0
    if (totalSize <= 4) {
        return 0;
    }
    
    // Allocate or enlarge the output buffer if necessary
    if ((NULL == *ppEncodedExt) || (*pEncodedExtSize < totalSize)) {
        *ppEncodedExt = tsk_realloc(*ppEncodedExt, totalSize);
        if (NULL == *ppEncodedExt) {
            TSK_DEBUG_ERROR ("Failed to allocate rtp header extension buffer with size = %zu", totalSize);
            *pEncodedExtSize = 0;
            return 0;
        }
        *pEncodedExtSize = totalSize;
    }
    pEncodedExt = (uint8_t*)*ppEncodedExt;
    
    // Populate the output buffer
    pEncodedExt[0] = 1; // set the mask byte number, currently only one mask byte
    pEncodedExt[1] = maskByte1;
    *((uint16_t*)&pEncodedExt[2]) = tnet_htons((uint16_t)((totalSize / 4) - 1)); // number of words(4 octets) except the first 1 word
    
    sizeOff = 4; // starts after the 4 fixed bytes per RFC3550
    payloadOff = sizeOff + byteNumForSizes; // payloads starts after the fields for the sizes
    
    if (maskByte1 & VIDEO_MASK_TIMESTAMP_ORIGINAL) {
        *((uint32_t*)&pEncodedExt[payloadOff]) = tnet_htonl(pHeaderExt->timestampOriginal);
        payloadOff += sizeof(uint32_t);
    }
    if ( (maskByte1 & VIDEO_MASK_TIMESTAMP_BOUNCED ) && (pHeaderExt->pTimestampBouncedData != NULL) ) {
        pEncodedExt[sizeOff] = pHeaderExt->timestampBouncedSize;
        sizeOff += sizeof(uint8_t);
        memcpy(&pEncodedExt[payloadOff], pHeaderExt->pTimestampBouncedData, pHeaderExt->timestampBouncedSize);
        payloadOff += pHeaderExt->timestampBouncedSize;
    }
    //
    // Added a new data field above ^^^^^^^^^^
    //////////////////////////////////////////////////////////////////////////////////
    
    return totalSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// producer相关
// static int tdav_session_video_producer_handle_captured_data(const tmedia_session_t *self,
//                                                             const void *buffer,
//                                                             tsk_size_t size);

static void* TSK_STDCALL tdav_video_consumer_thread(void *param);
static void* TSK_STDCALL tdav_video_monotor_thread(void *param);
tsk_size_t tdav_session_video_send_data (tdav_session_video_t* video,
                                         const void *data,
                                         tsk_size_t size,
                                         uint64_t time,
                                         tsk_bool_t first_packet,
                                         tsk_bool_t marker,
                                         tsk_size_t len,
                                         const void *extension,
                                         tsk_size_t ext_size,
                                         tsk_bool_t iFrame,
                                         tsk_bool_t timeStampFromUser,
                                         tsk_size_t video_id);
tdav_rscode_t* tdav_session_video_select_rscode_by_sessionid (tdav_session_video_t* self, int session_id,int iVideoid, RscType type);
int tdav_session_video_stop_rscode (tdav_session_video_t* self);
// Codec callback (From codec to the network)
// or Producer callback to sendRaw() data "as is"
static int tdav_session_video_raw_cb(const tmedia_video_encode_result_xt* result)
{
    tdav_session_av_t* base = (tdav_session_av_t*)result->usr_data;
    tdav_session_video_t* video = (tdav_session_video_t*)result->usr_data;
    trtp_rtp_header_t* rtp_header = (trtp_rtp_header_t*)result->proto_hdr;
    trtp_rtp_packet_t* packet = tsk_null;
    int ret = 0;
    tsk_size_t s;
    
    if(base->rtp_manager && base->rtp_manager->is_started){
        if(rtp_header){
            // uses negotiated SSRC (SDP)
            rtp_header->ssrc = base->rtp_manager->rtp.ssrc.local;
            // uses negotiated payload type
            if(base->pt_map.local != base->rtp_manager->rtp.payload_type || base->pt_map.remote != rtp_header->payload_type || base->pt_map.neg == -1){
                if(rtp_header->codec_id == tmedia_codec_id_none){
                    TSK_DEBUG_WARN("Internal codec id is equal to none");
                }
                else{
                    const tsk_list_item_t* item;
                    tsk_bool_t found = tsk_false;
                    tsk_list_lock(TMEDIA_SESSION(base)->neg_codecs);
                    tsk_list_foreach(item, TMEDIA_SESSION(base)->neg_codecs){
                        if((item->data) && ((const tmedia_codec_t*)item->data)->id == rtp_header->codec_id){
                            base->pt_map.local = base->rtp_manager->rtp.payload_type;
                            base->pt_map.remote = rtp_header->payload_type;
                            base->pt_map.neg = atoi(((const tmedia_codec_t*)item->data)->neg_format);
                            found = tsk_true;
                            break;
                        }
                    }
                    tsk_list_unlock(TMEDIA_SESSION(base)->neg_codecs);
                    if(found){
                        TSK_DEBUG_INFO("Codec PT mapping: local=%d, remote=%d, neg=%d", base->pt_map.local, base->pt_map.remote, base->pt_map.neg);
                    }
                    else{
                        TSK_DEBUG_ERROR("Failed to map codec PT: local=%d, remote=%d", base->rtp_manager->rtp.payload_type, rtp_header->payload_type);
                    }
                }
            }
            rtp_header->payload_type = base->pt_map.neg;
        }
        packet = rtp_header
        ? trtp_rtp_packet_create_2(rtp_header)
        : trtp_rtp_packet_create(base->rtp_manager->rtp.ssrc.local, base->rtp_manager->rtp.video_seq_num, base->rtp_manager->rtp.video_timestamp, base->rtp_manager->rtp.payload_type, result->last_chunck);
        
        if(packet ){
            tsk_size_t rtp_hdr_size;
            if(result->last_chunck){
#if 1
#if 1
                /*	http://www.cs.columbia.edu/~hgs/rtp/faq.html#timestamp-computed
                 For video, time clock rate is fixed at 90 kHz. The timestamps generated depend on whether the application can determine the frame number or not.
                 If it can or it can be sure that it is transmitting every frame with a fixed frame rate, the timestamp is governed by the nominal frame rate. Thus, for a 30 f/s video, timestamps would increase by 3,000 for each frame, for a 25 f/s video by 3,600 for each frame.
                 If a frame is transmitted as several RTP packets, these packets would all bear the same timestamp.
                 If the frame number cannot be determined or if frames are sampled aperiodically, as is typically the case for software codecs, the timestamp has to be computed from the system clock (e.g., gettimeofday())
                 */
                
                if(!video->encoder.last_frame_time){
                    // For the first frame it's not possible to compute the duration as there is no previous one.
                    // In this case, we trust the duration from the result (computed based on the codec fps and rate).
                    video->encoder.last_frame_time = tsk_time_now();
                    base->rtp_manager->rtp.video_timestamp += result->duration;
                }
                else{
                    uint64_t now = tsk_time_now();
                    uint32_t duration = (uint32_t)(now - video->encoder.last_frame_time);
                    base->rtp_manager->rtp.video_timestamp += (duration * 90/* 90KHz */);
                    video->encoder.last_frame_time = now;
                }
#else
                base->rtp_manager->rtp.video_timestamp = (uint32_t)(tsk_gettimeofday_ms() * 90/* 90KHz */);
#endif
#else
                base->rtp_manager->rtp.video_timestamp += result->duration;
#endif
                
            }
            
            packet->payload.data_const = result->buffer.ptr;
            packet->payload.size = result->buffer.size;
            s = trtp_manager_send_rtp_packet(base->rtp_manager, packet, tsk_false); // encrypt and send data
            
            // AddVideoCode( (int)packet->payload.size, TMEDIA_SESSION(video)->sessionid  );
            
            
            ++base->rtp_manager->rtp.video_seq_num; // seq_num must be incremented here (before the bail) because already used by SRTP context
            if(s < TRTP_RTP_HEADER_MIN_SIZE) {
                // without audio session iOS "audio" background mode is useless and UDP sockets will be closed: e.g. GE's video-only sessions
#if TDAV_UNDER_IPHONE
                if (tnet_geterrno() == EPIPE) {
                    TSK_DEBUG_INFO("iOS UDP pipe is broken (restoration is progress): failed to send packet with seqnum=%u. %u expected but only %u sent", (unsigned)packet->header->seq_num, (unsigned)packet->payload.size, (unsigned)s);
                }
#endif /* TDAV_UNDER_IPHONE */
                TSK_DEBUG_ERROR("Failed to send packet with seqnum=%u. %u expected but only %u sent", (unsigned)packet->header->seq_num, (unsigned)packet->payload.size, (unsigned)s);
                // save data expected to be sent in order to honor RTCP-NACK requests
                // s = base->rtp_manager->rtp.serial_buffer.index;
            }
            
            rtp_hdr_size = TRTP_RTP_HEADER_MIN_SIZE + (packet->header->csrc_count << 2);
            // Save packet
            if (base->avpf_mode_neg && (s > TRTP_RTP_HEADER_MIN_SIZE)) {
                trtp_rtp_packet_t* packet_avpf = (trtp_rtp_packet_t*)tsk_object_ref(packet);
                // when SRTP is used, "serial_buffer" will contains the encoded buffer with both RTP header and payload
                // Hack the RTP packet payload to point to the the SRTP data instead of unencrypted ptr
                packet_avpf->payload.size = (s - rtp_hdr_size);
                packet_avpf->payload.data_const = tsk_null;
                if(!(packet_avpf->payload.data = tsk_malloc(packet_avpf->payload.size))){// FIXME: to be optimized (reuse memory address)
                    TSK_DEBUG_ERROR("failed to allocate buffer");
                    goto bail;
                }
                memcpy(packet_avpf->payload.data, (((const uint8_t*)base->rtp_manager->rtp.serial_buffer.ptr) + rtp_hdr_size), packet_avpf->payload.size);
                tsk_list_lock(video->avpf.packets);
                if(video->avpf.count > video->avpf.max){
                    tsk_list_remove_first_item(video->avpf.packets);
                }
                else{
                    ++video->avpf.count;
                }
                
                // The packet must not added 'ascending' but 'back' because the sequence number coult wrap
                // For example:
                //	- send(65533, 65534, 65535, 0, 1)
                //  - will be stored as (if added 'ascending'): 0, 1, 65533, 65534, 65535
                //  - this means there is no benefit (if added 'ascending') as we cannot make 'smart search' using seqnums
                // tsk_list_push_ascending_data(video->avpf.packets, (void**)&packet_avpf); // filtered per seqnum
                tsk_list_push_back_data(video->avpf.packets, (void**)&packet_avpf);
                tsk_list_unlock(video->avpf.packets);
            }
            
            /*
             // FEC功能暂时屏蔽
             // Send FEC packet
             // FIXME: protect only Intra and Params packets
             if(base->ulpfec.codec && (s > TRTP_RTP_HEADER_MIN_SIZE)){
             packet->payload.data_const = (((const uint8_t*)base->rtp_manager->rtp.serial_buffer.ptr) + rtp_hdr_size);
             packet->payload.size = (s - rtp_hdr_size);
             ret = tdav_codec_ulpfec_enc_protect((struct tdav_codec_ulpfec_s*)base->ulpfec.codec, packet);
             if(result->last_chunck){
             trtp_rtp_packet_t* packet_fec;
             if((packet_fec = trtp_rtp_packet_create(base->rtp_manager->rtp.ssrc.local, base->ulpfec.seq_num++, base->ulpfec.timestamp, base->ulpfec.payload_type, tsk_true))){
             // serialize the FEC payload packet packet
             s = tdav_codec_ulpfec_enc_serialize((const struct tdav_codec_ulpfec_s*)base->ulpfec.codec, &video->encoder.buffer, &video->encoder.buffer_size);
             if(s > 0){
             packet_fec->payload.data_const = video->encoder.buffer;
             packet_fec->payload.size = s;
             s = trtp_manager_send_rtp_packet(base->rtp_manager, packet_fec, tsk_true;
             }
             TSK_OBJECT_SAFE_FREE(packet_fec);
             }
             base->ulpfec.timestamp += result->duration;
             ret = tdav_codec_ulpfec_enc_reset((struct tdav_codec_ulpfec_s*)base->ulpfec.codec);
             }
             }
             */
            
        }
        else {
            TSK_DEBUG_ERROR("Failed to create packet");
        }
    }
    else{
        //--TSK_DEBUG_WARN("Session not ready yet");
    }
    
bail:
    TSK_OBJECT_SAFE_FREE(packet);
    return ret;
}

// Codec Callback after decoding
static int tdav_session_video_decode_cb(const tmedia_video_decode_result_xt* result)
{
    tdav_session_av_t* base = (tdav_session_av_t*)result->usr_data;
    tdav_session_video_t* video = (tdav_session_video_t*)base;
    
    switch(result->type){
        case tmedia_video_decode_result_type_idr:
        {
            if(video->decoder.last_corrupted_timestamp != ((const trtp_rtp_header_t*)result->proto_hdr)->timestamp){
                //TSK_DEBUG_INFO("IDR frame decoded");
                video->decoder.stream_corrupted = tsk_false;
            }
            else{
                TSK_DEBUG_INFO("IDR frame decoded but corrupted :(");
            }
            break;
        }
        case tmedia_video_decode_result_type_error:
        {
            TSK_DEBUG_INFO("Decoding failed -> request Full Intra Refresh (FIR)");
            _tdav_session_video_local_request_idr(TMEDIA_SESSION(video), "DECODED_FAILED", ((const trtp_rtp_header_t*)result->proto_hdr)->ssrc);
            break;
        }
        default: break;
    }
    return 0;
}

static void tdav_session_video_encoder_cb(const uint8_t * input_buff, int bufflen, int64_t timestamp, int video_id, const void* callback_data)
{
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;
    tdav_session_av_t* base = (tdav_session_av_t*)callback_data;
    if(!video->started || !input_buff || !bufflen || !callback_data)
        return;
    int i = 0;
    int out_size = bufflen;
    h264_nal_type_t h264_nal_type;
    tsk_bool_t bIsIFrame;
    uint8_t * buff = (uint8_t *)input_buff;
    uint8_t * control_buf = tsk_null;

    // 计算duration，用于计算timestamp
    uint64_t now = tsk_time_now();
    // uint32_t duration = (uint32_t)(now - video->encoder.last_frame_time); // 单位ms
    video->encoder.last_frame_time = now;
    // 第一个frame到来时，要用audio当前的timestamp初始化video的timestamp
    if (video->encoder.isFirstFrame == tsk_true) {
        base->rtp_manager->rtp.video_timestamp = TSK_MAX(base->rtp_manager->rtp.video_timestamp, base->rtp_manager->rtp.timestamp);
        video->encoder.isFirstFrame = tsk_false;
        // duration = 0;
    }
    
    int H264_start_code_len = 4;    // default slice start code length
    if (buff[0] == 0x0 && buff[1] == 0x0 && buff[2] == 0x0 && buff[3] == 0x1)
    {
        H264_start_code_len = 4;
    } else if (buff[0] == 0x0 && buff[1] == 0x0 && buff[2] == 0x1)
    {
        H264_start_code_len = 3;
    }
    
    h264_nal_type = (h264_nal_type_t)(*((uint8_t*)buff + H264_start_code_len) & 0x1f);
    if (h264_nal_type == H264_NAL_SPS) {
        // 将sps/pps构造成stap-a, 此逻辑仅适用于外部输入H264裸流
        // startcode(4) + stap-a nalutype(1) + spslen(2) + spsbody + ppslen(2) + ppsbody
        uint32_t control_size = bufflen + 5; // 5 is stap-a nalutype(1) + spslen(2) + ppslen(2)
        uint8_t * control_buf = (uint8_t *)tsk_malloc(control_size);

        int sps_position = 0;
        int pps_position = 0;

        int sps_len = 0, pps_len = 0;
        for (int offset = 0; offset < bufflen; ++offset) {
            if (buff[offset] == 0 && buff[offset+1] == 0 && buff[offset+2] == 0 && buff[offset+3] == 1) {
                if ((buff[offset+4] & 0x1F) == 0x7) {
                    sps_position = offset+4;
                } else if ((buff[offset+4] & 0x1F) == 0x8) {
                    pps_position = offset+4;
                } 
            }
        }
        sps_len = pps_position - sps_position - 4;
        pps_len = bufflen - pps_position ;
        
        uint32_t buf_offset = 0;
        memcpy(control_buf, buff, 4); // start code
        buf_offset += 4;

        control_buf[buf_offset] = 0x58;
        buf_offset += 1;

        // sps
        control_buf[buf_offset] = (uint8_t)((sps_len >> 8) & 0xFF);
        control_buf[buf_offset+1] = (uint8_t)((sps_len) & 0xFF);
        buf_offset += 2;

        memcpy(control_buf + buf_offset, buff + sps_position, sps_len);
        buf_offset += sps_len;

        // pps
        control_buf[buf_offset] = (uint8_t)((pps_len >> 8) & 0xFF);
        control_buf[buf_offset+1] = (uint8_t)((pps_len) & 0xFF);
        buf_offset += 2;

        memcpy(control_buf + buf_offset, buff + pps_position, pps_len);
        buf_offset += pps_len;


        buff = control_buf;
        out_size = buf_offset;
        H264_start_code_len = 4;
        h264_nal_type = H264_NAL_STAP_A;
    }    

    bIsIFrame = (h264_nal_type == H264_NAL_STAP_A || h264_nal_type == H264_NAL_SLICE_IDR) ? tsk_true : tsk_false;
    
    tsk_size_t ext_size = tdav_session_video_encode_rtp_header_ext(video, video->encoder.buffer, out_size );
    int max_mtu_size = Config_GetInt("MAX_MTU_SIZE", DEF_MAX_H264_STREAM_SLICE);
    int sliceSize = Config_GetInt("H264_SLICE_SIZE", max_mtu_size);

    // 考虑是否分块:
    // 如果size小于1024byte，就用单一NAL单元模式,需要进行timestamp更新
    // 如果size大于1024byte，就用切包处理，封装fu_header和fu_indicator, 第一个切包需要进行timestamp更新，
    if (out_size < sliceSize + H264_start_code_len + H264_NALU_HEADER_LEN + MAX_H264_STREAM_SLICE_MERGE_SIZE) { // 单包传输，不需要传前4个字节的开始码 [00 00 00 01]
        tsk_bool_t marker = tsk_true;
        // sps and pps is not end pkt,should bound with idr
        if (h264_nal_type == H264_NAL_STAP_A) {
            marker = tsk_false;
        }
        
        tdav_session_video_send_data(video,
                                     (const void*)((uint8_t*)buff + H264_start_code_len),
                                     out_size - H264_start_code_len,
                                     timestamp,
                                     tsk_true, // first packet
                                     marker, // marker
                                     1,        // len
                                     video->encoder.rtpExtBuffer,
                                     ext_size,
                                     bIsIFrame,
                                     tsk_true,
                                     video_id);
    } else {
        int pos           = H264_start_code_len + H264_NALU_HEADER_LEN;
        int numOfFragment = (out_size - H264_start_code_len - H264_NALU_HEADER_LEN) / sliceSize;
        int remain        = (out_size - H264_start_code_len - H264_NALU_HEADER_LEN) % sliceSize;
        int fu_indicator  = (*((uint8_t*)buff + pos - H264_NALU_HEADER_LEN) & 0x60) | 0x1c;
        int fu_header_first_fragment = (*((uint8_t*)buff + pos - H264_NALU_HEADER_LEN) & 0x1f) | 0x80; // bit 7是起始fragment的标志位
        int fu_header_other_fragment = (*((uint8_t*)buff + pos - H264_NALU_HEADER_LEN) & 0x1f);
        int fu_header_last_fragment  = (*((uint8_t*)buff + pos - H264_NALU_HEADER_LEN) & 0x1f) | 0x40; // bit 6是最后一个fragment的标志位
        int packetNum = numOfFragment;
        if (remain > MAX_H264_STREAM_SLICE_MERGE_SIZE) {
            packetNum++;
        }
        for (i = 0; i < numOfFragment; i++) {
            if (i == 0) {
                *((uint8_t*)buff + pos - 1) = fu_header_first_fragment;
                *((uint8_t*)buff + pos - 2) = fu_indicator;
                tdav_session_video_send_data(video,
                                             (const void*)((uint8_t*)buff + pos - 2),
                                             sliceSize + 2,
                                             timestamp,
                                             tsk_true,  // first packet∂∂∂
                                             tsk_false, // marker
                                             packetNum, // len
                                             video->encoder.rtpExtBuffer,
                                             ext_size,
                                             bIsIFrame,
                                             tsk_true,
                                             video_id);
            } else {
                if ((!remain && i == numOfFragment - 1) || (remain <= MAX_H264_STREAM_SLICE_MERGE_SIZE && i == numOfFragment - 1)) {
                    *((uint8_t*)buff + pos - 1) = fu_header_last_fragment;
                    *((uint8_t*)buff + pos - 2) = fu_indicator;
                    tdav_session_video_send_data(video,
                                                 (const void*)((uint8_t*)buff + pos - 2),
                                                 sliceSize + 2 + remain,
                                                 timestamp,
                                                 tsk_false, // first packet
                                                 tsk_true,  // marker
                                                 packetNum, // len
                                                 video->encoder.rtpExtBuffer,
                                                 ext_size,
                                                 bIsIFrame,
                                                 tsk_true,
                                                 video_id);
                } else {
                    *((uint8_t*)buff + pos - 1) = fu_header_other_fragment;
                    *((uint8_t*)buff + pos - 2) = fu_indicator;
                    tdav_session_video_send_data(video,
                                                 (const void*)((uint8_t*)buff + pos - 2),
                                                 sliceSize + 2,
                                                 timestamp,
                                                 tsk_false, // first packet
                                                 tsk_false, // marker
                                                 packetNum, // len
                                                 video->encoder.rtpExtBuffer,
                                                 ext_size,
                                                 bIsIFrame,
                                                 tsk_true,
                                                 video_id);
                }
                
            }
            pos += sliceSize;
        }
        if (remain > MAX_H264_STREAM_SLICE_MERGE_SIZE ) {
            *((uint8_t*)buff + pos - 2) = fu_indicator;
            *((uint8_t*)buff + pos - 1) = fu_header_last_fragment;
            tdav_session_video_send_data(video,
                                         (const void*)((uint8_t*)buff + pos - 2),
                                         remain + 2,
                                         timestamp,
                                         tsk_false, // first packet
                                         tsk_true,  // marker
                                         packetNum, // len
                                         video->encoder.rtpExtBuffer,
                                         ext_size,
                                         bIsIFrame,
                                         tsk_true,
                                         video_id);
        }
    }

    if (control_buf) {
        TSK_FREE(control_buf);
    }   
}

static int tdav_session_video_producer_enc_cb_new(const void* callback_data, const void* buffer, tsk_size_t size, uint64_t timestamp, int video_id, int fmt, tsk_bool_t force_key_frame)
{
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;
    tdav_session_av_t* base = (tdav_session_av_t*)callback_data;
    int ret = 0;
    
    // TSK_DEBUG_WARN("tdav_session_video_producer_enc_cb_new videoid:%d, fmt:%d, ts:%llu size[%d]", video_id, fmt, timestamp, size);
    if(!base){
        TSK_DEBUG_ERROR("Null session");
        return 0;
    }
    // do nothing if session is held
    // when the session is held the end user will get feedback he also has possibilities to put the consumer and producer on pause
    if (TMEDIA_SESSION(base)->lo_held) {
        return 0;
    }
    // do nothing if not started yet
    if (!video->started) {
        TSK_DEBUG_INFO("Video session not started yet");
        return 0;
    }
    
    if (VIDEO_FMT_H264 == fmt) {
        // TSK_DEBUG_WARN("tdav_session_video_producer_enc_cb_new send frame to rtp,  fmt:%d, ts:%llu size[%d]", fmt, timestamp, size);
        tdav_session_video_encoder_cb((uint8_t*)buffer, size, timestamp, 0, callback_data);
        return 0;
    }

#if ANDROID
    // backup frame, only for share stream (videoid=2)
    if(2 == video_id)
    {
        uint64_t timestamp_now = tsk_gettimeofday_ms();
        if (size != 0 && timestamp_now - video->last_check_timestamp > video->refresh_copy_interval) {//输入间隔大于500ms才复制帧
            if (video->last_frame_buf && video->last_frame_size != size) {
                TSK_FREE(video->last_frame_buf);
                video->last_frame_buf = tsk_null;
            }
            
            if (!video->last_frame_buf) {
                video->last_frame_buf = (uint8_t *)tsk_calloc(size, 1);
            }
            
            if (video->last_frame_buf) {
             //    TSK_DEBUG_INFO("mark back up last frame");
                if(!force_key_frame) memcpy(video->last_frame_buf, buffer, size);
                video->last_frame_size = size;
                video->last_frame_fmt = fmt;
                video->last_frame_timestamp = timestamp;
            }
        }
        video->last_check_timestamp = timestamp_now;
    }
#endif

    // get best negotiated codec if not already done
    // the encoder codec could be null when session is renegotiated without re-starting (e.g. hold/resume)
    if (!video->encoder.codec) {
        const tmedia_codec_t* codec;
        tsk_safeobj_lock(base);
        if (!(codec = tdav_session_av_get_best_neg_codec(base))) {
            TSK_DEBUG_ERROR("No codec matched");
            tsk_safeobj_unlock(base);
            return -2;
        }
        video->encoder.codec = (tmedia_codec_s*)tsk_object_ref(TSK_OBJECT(codec));
        tsk_safeobj_unlock(base);
    }
    
    if (base->rtp_manager) {
        tsk_size_t out_size = 0;
        tmedia_codec_t* codec_encoder = tsk_null;
        
        if (!base->rtp_manager->is_started) {
            TSK_DEBUG_ERROR("Not started");
            goto bail;
        }
        
        // take a reference to the encoder to make sure it'll not be destroyed while we're using it
        codec_encoder = (tmedia_codec_t*)tsk_object_ref(video->encoder.codec);
        if (!codec_encoder) {
            TSK_DEBUG_ERROR("The encoder is null");
            goto bail;
        }
        
        if (!video->started)
            goto bail;

        // Encode data
        tsk_mutex_lock(video->encoder.h_mutex);
        if (codec_encoder->opened) { // stop() function locks the encoder mutex before changing "started"
            /* producer supports yuv42p */
            out_size = codec_encoder->plugin->encode_new(codec_encoder, buffer, size, &video->encoder.buffer, &video->encoder.buffer_size, timestamp, video_id, force_key_frame, callback_data);
        }
        tsk_mutex_unlock(video->encoder.h_mutex);

       if(out_size > 1)
           tdav_session_video_encoder_cb((uint8_t*)video->encoder.buffer, out_size, timestamp, video_id, callback_data);
       else if(out_size == 1){
            TSK_DEBUG_ERROR("The encoder swtich sw!!");
           ret = 1; //-> swcode
       }
        
    bail:
        TSK_OBJECT_SAFE_FREE(codec_encoder);
    } else {
        TSK_DEBUG_ERROR("Invalid parameter");
        ret = -1;
    }
    
    return ret;
}

// Producer callback (From the producer to the network) => encode data before send()
static int tdav_session_video_producer_enc_cb(const void* callback_data, const void* buffer, tsk_size_t size)
{
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;
    tdav_session_av_t* base = (tdav_session_av_t*)callback_data;
    tsk_size_t yuv420p_size = 0 ;
    uint32_t duration;
    uint64_t now;
    int ret = 0;
    int i;
    h264_nal_type_t h264_nal_type;
    tsk_bool_t bIsIFrame = tsk_false;
    
    if(!base){
        TSK_DEBUG_ERROR("Null session");
        return 0;
    }
    
    // do nothing if session is held
    // when the session is held the end user will get feedback he also has possibilities to put the consumer and producer on pause
    if (TMEDIA_SESSION(base)->lo_held) {
        return 0;
    }
    
    // do nothing if not started yet
    if (!video->started) {
        TSK_DEBUG_INFO("Video session not started yet");
        return 0;
    }
    
    // get best negotiated codec if not already done
    // the encoder codec could be null when session is renegotiated without re-starting (e.g. hold/resume)
    if (!video->encoder.codec) {
        const tmedia_codec_t* codec;
        tsk_safeobj_lock(base);
        if (!(codec = tdav_session_av_get_best_neg_codec(base))) {
            TSK_DEBUG_ERROR("No codec matched");
            tsk_safeobj_unlock(base);
            return -2;
        }
        video->encoder.codec = (tmedia_codec_s*)tsk_object_ref(TSK_OBJECT(codec));
        tsk_safeobj_unlock(base);
    }
    
    if (base->rtp_manager) {
        //static int __rotation_counter = 0;
        /* encode */
        tsk_size_t out_size = 0;
        tmedia_codec_t* codec_encoder = tsk_null;
        
        if (!base->rtp_manager->is_started) {
            TSK_DEBUG_ERROR("Not started");
            goto bail;
        }
        
        // take a reference to the encoder to make sure it'll not be destroyed while we're using it
        codec_encoder = (tmedia_codec_t*)tsk_object_ref(video->encoder.codec);
        if (!codec_encoder) {
            TSK_DEBUG_ERROR("The encoder is null");
            goto bail;
        }
        
#define PRODUCER_OUTPUT_FIXSIZE (base->producer->video.chroma != tmedia_chroma_mjpeg) // whether the output data has a fixed size/length
#define PRODUCER_OUTPUT_RAW (base->producer->encoder.codec_id == tmedia_codec_id_none) // Otherwise, frames from the producer are already encoded
#define PRODUCER_SIZE_CHANGED ((video->conv.producerWidth && video->conv.producerWidth != base->producer->video.width) || (video->conv.producerHeight && video->conv.producerHeight != base->producer->video.height) \
|| (video->conv.xProducerSize && (video->conv.xProducerSize != size && PRODUCER_OUTPUT_FIXSIZE)))
#define ENCODED_NEED_FLIP (TMEDIA_CODEC_VIDEO(codec_encoder)->out.flip)
#define ENCODED_NEED_RESIZE (base->producer->video.width != TMEDIA_CODEC_VIDEO(codec_encoder)->out.width || base->producer->video.height != TMEDIA_CODEC_VIDEO(codec_encoder)->out.height)
#define PRODUCED_FRAME_NEED_ROTATION (base->producer->video.rotation != 0)
#define PRODUCED_FRAME_NEED_MIRROR (base->producer->video.mirror != tsk_false)
#define PRODUCED_FRAME_NEED_CHROMA_CONVERSION (base->producer->video.chroma != TMEDIA_CODEC_VIDEO(codec_encoder)->out.chroma)
        // Video codecs only accept YUV420P buffers ==> do conversion if needed or producer doesn't have the right size
        if (PRODUCER_OUTPUT_RAW && (PRODUCED_FRAME_NEED_CHROMA_CONVERSION || PRODUCER_SIZE_CHANGED || ENCODED_NEED_FLIP || ENCODED_NEED_RESIZE ||PRODUCED_FRAME_NEED_ROTATION || PRODUCED_FRAME_NEED_MIRROR)) {
            // Create video converter if not already done or producer size have changed
            if(!video->conv.toYUV420 || PRODUCER_SIZE_CHANGED){
                TSK_OBJECT_SAFE_FREE(video->conv.toYUV420);
                video->conv.producerWidth = base->producer->video.width;
                video->conv.producerHeight = base->producer->video.height;
                video->conv.xProducerSize = size;
                
                TSK_DEBUG_INFO("producer size = (%d, %d)", (int)base->producer->video.width, (int)base->producer->video.height);
                if (!(video->conv.toYUV420 = tmedia_converter_video_create(base->producer->video.width, base->producer->video.height, base->producer->video.chroma, TMEDIA_CODEC_VIDEO(codec_encoder)->out.width, TMEDIA_CODEC_VIDEO(codec_encoder)->out.height,
                                                                           TMEDIA_CODEC_VIDEO(codec_encoder)->out.chroma))){
                    TSK_DEBUG_ERROR("Failed to create video converter");
                    ret = -5;
                    goto bail;
                }
                // restore/set rotation scaling info because producer size could change
                tmedia_converter_video_set_scale_rotated_frames(video->conv.toYUV420, video->encoder.scale_rotated_frames);
            }
        }
        
        if(video->conv.toYUV420){
            video->encoder.scale_rotated_frames = video->conv.toYUV420->scale_rotated_frames;
            // check if rotation have changed and alert the codec
            // we avoid scalling the frame after rotation because it's CPU intensive and keeping the image ratio is difficult
            // it's up to the encoder to swap (w,h) and to track the rotation value
            if(video->encoder.rotation != base->producer->video.rotation){
                tmedia_param_t* param = tmedia_param_create(tmedia_pat_set,
                                                            tmedia_video,
                                                            tmedia_ppt_codec,
                                                            tmedia_pvt_int32,
                                                            "rotation",
                                                            (void*)&base->producer->video.rotation);
                if (!param) {
                    TSK_DEBUG_ERROR("Failed to create a media parameter");
                    return -1;
                }
                video->encoder.rotation = base->producer->video.rotation; // update rotation to avoid calling the function several times
                ret = tmedia_codec_set(codec_encoder, param);
                TSK_OBJECT_SAFE_FREE(param);
                // (ret != 0) -> not supported by the codec -> to be done by the converter
                video->encoder.scale_rotated_frames = (ret != 0);
            }
            
            // update one-shot parameters
            tmedia_converter_video_set(video->conv.toYUV420, base->producer->video.rotation, TMEDIA_CODEC_VIDEO(codec_encoder)->out.flip, base->producer->video.mirror, video->encoder.scale_rotated_frames);
            
            yuv420p_size = tmedia_converter_video_process(video->conv.toYUV420, buffer, size, &video->encoder.conv_buffer, &video->encoder.conv_buffer_size);
            if (!yuv420p_size || !video->encoder.conv_buffer) {
                TSK_DEBUG_ERROR("Failed to convert XXX buffer to YUV42P");
                ret = -6;
                goto bail;
            }
        }
        // 计算duration，用于计算timestamp
        now = tsk_time_now();
        duration = (uint32_t)(now - video->encoder.last_frame_time); // 单位ms
        video->encoder.last_frame_time = now;
        
        
        // Encode data
        tsk_mutex_lock(video->encoder.h_mutex);
        if (codec_encoder->opened) { // stop() function locks the encoder mutex before changing "started"
            if (video->encoder.conv_buffer && yuv420p_size) {
                /* producer doesn't support yuv42p */
                out_size = codec_encoder->plugin->encode_new(codec_encoder, video->encoder.conv_buffer, yuv420p_size, &video->encoder.buffer, &video->encoder.buffer_size, duration, 0, tsk_false, callback_data);
            }
            else {
                /* producer supports yuv42p */
                out_size = codec_encoder->plugin->encode_new(codec_encoder, buffer, size, &video->encoder.buffer, &video->encoder.buffer_size, duration, 0, tsk_false, callback_data);
            }
        }
        tsk_mutex_unlock(video->encoder.h_mutex);
        
        if (out_size) {
            // 计算duration，用于计算timestamp
            uint64_t now = tsk_time_now();
            uint32_t duration = (uint32_t)(now - video->encoder.last_frame_time); // 单位ms
            video->encoder.last_frame_time = now;
            // 第一个frame到来时，要用audio当前的timestamp初始化video的timestamp
            if (video->encoder.isFirstFrame == tsk_true) {
                base->rtp_manager->rtp.video_timestamp = TSK_MAX(base->rtp_manager->rtp.video_timestamp, base->rtp_manager->rtp.timestamp);
                duration = 0;
            }
            
            // 判断是否是IDR/SPS/PPS帧
            h264_nal_type = (h264_nal_type_t)(*((uint8_t*)video->encoder.buffer + H264_START_CODE_LEN) & 0x1f);
            bIsIFrame = (h264_nal_type == H264_NAL_SPS) ? tsk_true : tsk_false;
            
            int max_slice_size = Config_GetInt("MAX_MTU_SIZE", DEF_MAX_H264_STREAM_SLICE);

            // 考虑是否分块:
            // 如果size小于1024byte，就用单一NAL单元模式,需要进行timestamp更新
            // 如果size大于1024byte，就用切包处理，封装fu_header和fu_indicator, 第一个切包需要进行timestamp更新，
            if (out_size < max_slice_size + H264_START_CODE_LEN + H264_NALU_HEADER_LEN) { // 单包传输，不需要传前4个字节的开始码 [00 00 00 01]
                tdav_session_video_send_data(video,
                                             (const void*)((uint8_t*)video->encoder.buffer + H264_START_CODE_LEN),
                                             out_size - H264_START_CODE_LEN,
                                             duration * 48/* 90 KHz */,
                                             tsk_true,  // first packet
                                             tsk_true,  // marker
                                             1,         // len
                                             tsk_null,
                                             0,
                                             bIsIFrame,
                                             tsk_false,
                                             0);
            } else {
                int pos           = H264_START_CODE_LEN + H264_NALU_HEADER_LEN;
                int numOfFragment = (out_size - H264_START_CODE_LEN - H264_NALU_HEADER_LEN) / max_slice_size;
                int remain        = (out_size - H264_START_CODE_LEN - H264_NALU_HEADER_LEN) % max_slice_size;
                int fu_indicator  = (*((uint8_t*)video->encoder.buffer + pos - H264_NALU_HEADER_LEN) & 0x60) | 0x1c;
                int fu_header_first_fragment = (*((uint8_t*)video->encoder.buffer + pos - H264_NALU_HEADER_LEN) & 0x1f) | 0x80; // bit 7是起始fragment的标志位
                int fu_header_other_fragment = (*((uint8_t*)video->encoder.buffer + pos - H264_NALU_HEADER_LEN) & 0x1f);
                int fu_header_last_fragment  = (*((uint8_t*)video->encoder.buffer + pos - H264_NALU_HEADER_LEN) & 0x1f) | 0x40; // bit 6是最后一个fragment的标志位
                int packetNum = numOfFragment;
                if (remain) {
                    packetNum++;
                }
                for (i = 0; i < numOfFragment; i++) {
                    if (i == 0) {
                        *((uint8_t*)video->encoder.buffer + pos - 1) = fu_header_first_fragment;
                        *((uint8_t*)video->encoder.buffer + pos - 2) = fu_indicator;
                        tdav_session_video_send_data(video,
                                                     (const void*)((uint8_t*)video->encoder.buffer + pos - 2),
                                                     max_slice_size + 2,
                                                     duration * 48/* 90 KHz */,
                                                     tsk_true,  // first packet
                                                     tsk_false, // marker
                                                     packetNum, // len
                                                     tsk_null,
                                                     0,
                                                     bIsIFrame,
                                                     tsk_false,
                                                     0);
                    } else {
                        if (!remain && i == numOfFragment - 1)
                        {
                            *((uint8_t*)video->encoder.buffer + pos - 1) = fu_header_last_fragment;
                            *((uint8_t*)video->encoder.buffer + pos - 2) = fu_indicator;
                            tdav_session_video_send_data(video,
                                                         (const void*)((uint8_t*)video->encoder.buffer + pos - 2),
                                                         max_slice_size + 2,
                                                         duration * 48/* 90 KHz */,
                                                         tsk_false, // first packet
                                                         tsk_true,  // marker
                                                         packetNum, // len
                                                         tsk_null,
                                                         0,
                                                         bIsIFrame,
                                                         tsk_false,
                                                         0);
                        } else {
                            *((uint8_t*)video->encoder.buffer + pos - 1) = fu_header_other_fragment;
                            *((uint8_t*)video->encoder.buffer + pos - 2) = fu_indicator;
                            tdav_session_video_send_data(video,
                                                         (const void*)((uint8_t*)video->encoder.buffer + pos - 2),
                                                         max_slice_size + 2,
                                                         duration * 48/* 90 KHz */,
                                                         tsk_false, // first packet
                                                         tsk_false, // marker
                                                         packetNum, // len
                                                         tsk_null,
                                                         0,
                                                         bIsIFrame,
                                                         tsk_false,
                                                         0);
                        }
                    }
                    pos += max_slice_size;
                }
                if (remain) {
                    *((uint8_t*)video->encoder.buffer + pos - 2) = fu_indicator;
                    *((uint8_t*)video->encoder.buffer + pos - 1) = fu_header_last_fragment;
                    tdav_session_video_send_data(video,
                                                 (const void*)((uint8_t*)video->encoder.buffer + pos - 2),
                                                 remain + 2,
                                                 duration * 48/* 90 KHz */,
                                                 tsk_false, // first packet
                                                 tsk_true,  // marker
                                                 packetNum, // len
                                                 tsk_null,
                                                 0,
                                                 bIsIFrame,
                                                 tsk_false,
                                                 0);
                }
            }
            video->encoder.isFirstFrame = tsk_false;
        }
    bail:
        TSK_OBJECT_SAFE_FREE(codec_encoder);
    }
    else {
        TSK_DEBUG_ERROR("Invalid parameter");
        ret = -1;
    }
    
    return ret;
}

tsk_size_t tdav_session_video_send_data (tdav_session_video_t* video,
                                         const void *data,
                                         tsk_size_t size,
                                         uint64_t time,
                                         tsk_bool_t first_packet,
                                         tsk_bool_t marker,
                                         tsk_size_t len,
                                         const void *extension,
                                         tsk_size_t ext_size,
                                         tsk_bool_t iFrame,
                                         tsk_bool_t timeStampFromUser,
                                         tsk_size_t video_id)
{
    trtp_manager_s* rtp_manager = TDAV_SESSION_AV(video)->rtp_manager;
    trtp_rtp_packet_t* packet;
    
    if(TMEDIA_SESSION(video)->sessionid > 0) {
        tdav_rscode_t* rscode = tdav_session_video_select_rscode_by_sessionid(video, TMEDIA_SESSION(video)->sessionid,video_id, RSC_TYPE_ENCODE);
        if(!rscode) {
            tdav_rscode_t *rscode_new = tdav_rscode_create(TMEDIA_SESSION(video)->sessionid,video_id, RSC_TYPE_ENCODE, rtp_manager);
            rscode_new->uBusinessId = rtp_manager->uBusinessId;
            tdav_rscode_start(rscode_new);
            rscode = rscode_new;
            tsk_list_lock(video->rscode_list);
            tsk_list_push_back_data(video->rscode_list, (tsk_object_t**)&rscode_new);
            tsk_list_unlock(video->rscode_list);
        }
        
        if (timeStampFromUser) {
            packet = trtp_manager_create_packet_new(rtp_manager, data, size, extension, ext_size, time, iFrame, marker);
        } else {
            packet = trtp_manager_create_packet(rtp_manager, data, size, extension, ext_size, time, iFrame, marker);
        }
        
        if(!packet) {
            return -1;
        }
        
        trtp_rtp_header_t *rtp_header = packet->header;
        
        // video rtp packet csrc count is 4
        // csrc[0]: sessionId
        // csrc[1]: key frame flag
        // csrc[2]: first seq num + pkt count
        // csrc[3]: video_id

        //两路流的seq_num要分开
        if( video_id == TRTP_VIDEO_STREAM_TYPE_MAIN )
        {
            rtp_header->seq_num = ++rtp_manager->rtp.video_segment_seq_num;
            if (first_packet) {
                rtp_manager->rtp.video_first_packet_seq_num = rtp_header->seq_num;
            }
            
            rtp_header->csrc[2] = (rtp_manager->rtp.video_first_packet_seq_num << 16) | ((uint16_t)len);
        }
        else if (video_id == TRTP_VIDEO_STREAM_TYPE_SHARE) {
            rtp_header->seq_num = ++rtp_manager->rtp.video_segment_seq_num_share;
            if (first_packet) {
                rtp_manager->rtp.video_first_packet_seq_num_share = rtp_header->seq_num;
            }
            
            rtp_header->csrc[2] = (rtp_manager->rtp.video_first_packet_seq_num_share << 16) | ((uint16_t)len);
        } else {
            rtp_header->seq_num = ++rtp_manager->rtp.video_segment_seq_num_second;
            if (first_packet) {
                rtp_manager->rtp.video_first_packet_seq_num_second = rtp_header->seq_num;
            }

            rtp_header->csrc[2] = (rtp_manager->rtp.video_first_packet_seq_num_second << 16) | ((uint16_t)len);
        }
        
        rtp_header->csrc_count++;

        rtp_header->csrc[3] = video_id;
        rtp_header->csrc_count++;
       
        
        tdav_rscode_push_rtp_packet(rscode, (trtp_rtp_packet_t *)packet);
        TSK_OBJECT_SAFE_FREE(packet);
       // TSK_DEBUG_INFO("video stream type:%d, time:%lld, size:%d\n", video_id, time, size);
    }
    
    return 0;
}

#if 0 // 废弃代码
// Handle captured data taken from the frame list (From the producer to the network). Will encode() data before sending
static int tdav_session_video_producer_handle_captured_data(const tmedia_session_t *self, const void *buffer, tsk_size_t size)
{
    tdav_session_video_t* video = (tdav_session_video_t*)self;
    tdav_session_av_t* base = (tdav_session_av_t*)self;
    tsk_size_t yuv420p_size = 0;
    int ret = 0;
    
    if(!base){
        TSK_DEBUG_ERROR("Null session");
        return 0;
    }
    
    // do nothing if session is held
    // when the session is held the end user will get feedback he also has possibilities to put the consumer and producer on pause
    if (TMEDIA_SESSION(base)->lo_held) {
        return 0;
    }
    
    // do nothing if not started yet
    if (!video->started) {
        TSK_DEBUG_INFO("Video session not started yet");
        return 0;
    }
    
    // get best negotiated codec if not already done
    // the encoder codec could be null when session is renegotiated without re-starting (e.g. hold/resume)
    if (!video->encoder.codec) {
        const tmedia_codec_t* codec;
        tsk_safeobj_lock(base);
        if (!(codec = tdav_session_av_get_best_neg_codec(base))) {
            TSK_DEBUG_ERROR("No codec matched");
            tsk_safeobj_unlock(base);
            return -2;
        }
        video->encoder.codec = (tmedia_codec_s*)tsk_object_ref(TSK_OBJECT(codec));
        tsk_safeobj_unlock(base);
    }
    
    if (base->rtp_manager) {
        //static int __rotation_counter = 0;
        /* encode */
        tsk_size_t out_size = 0;
        tmedia_codec_t* codec_encoder = tsk_null;
        
        if (!base->rtp_manager->is_started) {
            TSK_DEBUG_ERROR("Not started");
            goto bail;
        }
        
        // take a reference to the encoder to make sure it'll not be destroyed while we're using it
        codec_encoder = (tmedia_codec_t*)tsk_object_ref(video->encoder.codec);
        if (!codec_encoder) {
            TSK_DEBUG_ERROR("The encoder is null");
            goto bail;
        }
        
#define PRODUCER_OUTPUT_FIXSIZE (base->producer->video.chroma != tmedia_chroma_mjpeg) // whether the output data has a fixed size/length
#define PRODUCER_OUTPUT_RAW (base->producer->encoder.codec_id == tmedia_codec_id_none) // Otherwise, frames from the producer are already encoded
#define PRODUCER_SIZE_CHANGED ((video->conv.producerWidth && video->conv.producerWidth != base->producer->video.width) || (video->conv.producerHeight && video->conv.producerHeight != base->producer->video.height) \
|| (video->conv.xProducerSize && (video->conv.xProducerSize != size && PRODUCER_OUTPUT_FIXSIZE)))
#define ENCODED_NEED_FLIP (TMEDIA_CODEC_VIDEO(codec_encoder)->out.flip)
#define ENCODED_NEED_RESIZE (base->producer->video.width != TMEDIA_CODEC_VIDEO(codec_encoder)->out.width || base->producer->video.height != TMEDIA_CODEC_VIDEO(codec_encoder)->out.height)
#define PRODUCED_FRAME_NEED_ROTATION (base->producer->video.rotation != 0)
#define PRODUCED_FRAME_NEED_MIRROR (base->producer->video.mirror != tsk_false)
#define PRODUCED_FRAME_NEED_CHROMA_CONVERSION (base->producer->video.chroma != TMEDIA_CODEC_VIDEO(codec_encoder)->out.chroma)
        // Video codecs only accept YUV420P buffers ==> do conversion if needed or producer doesn't have the right size
        if (PRODUCER_OUTPUT_RAW && (PRODUCED_FRAME_NEED_CHROMA_CONVERSION || PRODUCER_SIZE_CHANGED || ENCODED_NEED_FLIP || ENCODED_NEED_RESIZE ||PRODUCED_FRAME_NEED_ROTATION || PRODUCED_FRAME_NEED_MIRROR)) {
            // Create video converter if not already done or producer size have changed
            if(!video->conv.toYUV420 || PRODUCER_SIZE_CHANGED){
                TSK_OBJECT_SAFE_FREE(video->conv.toYUV420);
                video->conv.producerWidth = base->producer->video.width;
                video->conv.producerHeight = base->producer->video.height;
                video->conv.xProducerSize = size;
                
                TSK_DEBUG_INFO("producer size = (%d, %d)", (int)base->producer->video.width, (int)base->producer->video.height);
                if (!(video->conv.toYUV420 = tmedia_converter_video_create(base->producer->video.width, base->producer->video.height, base->producer->video.chroma, TMEDIA_CODEC_VIDEO(codec_encoder)->out.width, TMEDIA_CODEC_VIDEO(codec_encoder)->out.height,
                                                                           TMEDIA_CODEC_VIDEO(codec_encoder)->out.chroma))){
                    TSK_DEBUG_ERROR("Failed to create video converter");
                    ret = -5;
                    goto bail;
                }
                // restore/set rotation scaling info because producer size could change
                tmedia_converter_video_set_scale_rotated_frames(video->conv.toYUV420, video->encoder.scale_rotated_frames);
            }
        }
        
        if(video->conv.toYUV420){
            video->encoder.scale_rotated_frames = video->conv.toYUV420->scale_rotated_frames;
            // check if rotation have changed and alert the codec
            // we avoid scalling the frame after rotation because it's CPU intensive and keeping the image ratio is difficult
            // it's up to the encoder to swap (w,h) and to track the rotation value
            if(video->encoder.rotation != base->producer->video.rotation){
                tmedia_param_t* param = tmedia_param_create(tmedia_pat_set,
                                                            tmedia_video,
                                                            tmedia_ppt_codec,
                                                            tmedia_pvt_int32,
                                                            "rotation",
                                                            (void*)&base->producer->video.rotation);
                if (!param) {
                    TSK_DEBUG_ERROR("Failed to create a media parameter");
                    return -1;
                }
                video->encoder.rotation = base->producer->video.rotation; // update rotation to avoid calling the function several times
                ret = tmedia_codec_set(codec_encoder, param);
                TSK_OBJECT_SAFE_FREE(param);
                // (ret != 0) -> not supported by the codec -> to be done by the converter
                video->encoder.scale_rotated_frames = (ret != 0);
            }
            
            // update one-shot parameters
            tmedia_converter_video_set(video->conv.toYUV420, base->producer->video.rotation, TMEDIA_CODEC_VIDEO(codec_encoder)->out.flip, base->producer->video.mirror, video->encoder.scale_rotated_frames);
            
            yuv420p_size = tmedia_converter_video_process(video->conv.toYUV420, buffer, size, &video->encoder.conv_buffer, &video->encoder.conv_buffer_size);
            if (!yuv420p_size || !video->encoder.conv_buffer) {
                TSK_DEBUG_ERROR("Failed to convert XXX buffer to YUV42P");
                ret = -6;
                goto bail;
            }
        }
        
        // Encode data
        tsk_mutex_lock(video->encoder.h_mutex);
        if (codec_encoder->opened) { // stop() function locks the encoder mutex before changing "started"
            if (video->encoder.conv_buffer && yuv420p_size) {
                /* producer doesn't support yuv42p */
                out_size = codec_encoder->plugin->encode_new(codec_encoder, video->encoder.conv_buffer, yuv420p_size, &video->encoder.buffer, &video->encoder.buffer_size, 0, 0, tsk_false, self);
            }
            else {
                /* producer supports yuv42p */
                out_size = codec_encoder->plugin->encode_new(codec_encoder, buffer, size, &video->encoder.buffer, &video->encoder.buffer_size, 0, 0, tsk_false, self);
            }
        }
        tsk_mutex_unlock(video->encoder.h_mutex);
        
        
        if (out_size) {
            /* Never called, see tdav_session_video_raw_cb() */
            tsk_mutex_lock(base->rtp_manager_mutex);
            trtp_manager_send_rtp(base->rtp_manager, video->encoder.buffer, out_size, 6006, tsk_true, tsk_true);
            
			AddVideoCode((int)out_size, TMEDIA_SESSION(video)->sessionid);
            
            tsk_mutex_unlock(base->rtp_manager_mutex);
        }
    bail:
        TSK_OBJECT_SAFE_FREE(codec_encoder);
    }
    else {
        TSK_DEBUG_ERROR("Invalid parameter");
        ret = -1;
    }
    
    return ret;
}
#endif

static int  tdav_session_video_nack_check_cb(const void* callback_data, const trtp_rtp_packet_t* packet) {
    int ret = 0;
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;

    if (!video || !packet || !packet->header) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    // nack处理，对接收到的数据包做seq和timestamp检查
    uint32_t nack = (tmedia_get_video_nack_flag() &  (Config_GetInt("dummy_rtt",0) < 90 ? tdav_session_video_nack_half : tdav_session_video_nack_none));
    if (video->last_nack_flag != nack) {
        video->last_nack_flag = nack;
        int redundancy = 0;
        tdav_video_jb_packet_nack_check(video->jb, const_cast<trtp_rtp_packet_t*>(packet), &redundancy, 1);
    }

    //nack功能关闭后再打开时需要重置nack_check状态参数
    if (nack && video->jb) {
        int redundancy = 0; // 1:重复包，2:重传包
        tdav_video_jb_packet_nack_check(video->jb, const_cast<trtp_rtp_packet_t*>(packet), &redundancy, 0);
        if (redundancy) {
            // TSK_DEBUG_INFO("check packet duplicate seq:%u", (trtp_rtp_packet_t *)packet->header->seq_num);
            return redundancy;
        }
    }

    return ret;
}

// RTP callback (Network -> Decoder -> Consumer)
static int tdav_session_video_rtp_cb(const void* callback_data, const trtp_rtp_packet_t* packet)
{
    int ret = 0;
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;
    
    if (!video || !packet || !packet->header) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    // AddVideoCode( packet->payload.size,  packet->header->session_id );
    
    int session_id = -1;
    if(packet->header->csrc_count > 0) {
        session_id = (int32_t)packet->header->csrc[0];
    }
    
    TMEDIA_I_AM_ACTIVE(2000, "Video rtp cb comes, sessionId=%d", session_id);
    
    if(session_id > 0) {
		uint8_t* pGrounNAP = (uint8_t*)(&packet->header->csrc[2]);
		uint16_t iNAP = pGrounNAP[2] << 8 | pGrounNAP[3];

		uint32_t iCheckCodeLen = 1;
		if (packet->header->csrc_count >= 6){
			iCheckCodeLen = packet->header->csrc[5];
		}
        
        int iVideoID = 0;
        if (packet->header->csrc_count >= 5) {
            iVideoID = packet->header->csrc[4];
        } else {
            iVideoID = packet->header->csrc[3];
        }
        
        if (TRTP_VIDEO_STREAM_TYPE_SHARE == iVideoID) {
            AddVideoShareCode( packet->payload.size,  packet->header->session_id );
        } else {
            AddVideoCode( packet->payload.size,  packet->header->session_id );
        }

        // TSK_DEBUG_ERROR("tdav_session_video_rtp_cb sessionId[%d] seq[%d] videoid[%d]", session_id, packet->header->seq_num, iVideoID);
        tdav_rscode_t* rscode = tdav_session_video_select_rscode_by_sessionid(video, session_id,iVideoID, RSC_TYPE_DECODE);
        if (!rscode) {
            TSK_DEBUG_INFO("create rscode thread sessionid:%d, videoID:%d", session_id, iVideoID);
            tdav_rscode_t *rscode_new = tdav_rscode_create(session_id,iVideoID, RSC_TYPE_DECODE,  tsk_null);
            tdav_rscode_start(rscode_new);

            rscode = rscode_new;
            tsk_list_lock(video->rscode_list);
            tsk_list_push_back_data(video->rscode_list, (tsk_object_t**)&rscode_new);
            tsk_list_unlock(video->rscode_list);
        }

//        TSK_DEBUG_INFO("before rscode:videoid:(%d), seq_num:(%d), size:(%d)", packet->header->csrc[4], (int)packet->header->seq_num, packet->payload.size );
        tdav_rscode_push_rtp_packet(rscode, (trtp_rtp_packet_t *)packet);

        trtp_rtp_packet_t* pkt = tsk_null;
        tsk_list_item_t* item = tsk_null;
        while (tsk_null != (item = tdav_rscode_pop_rtp_packet(rscode))) {
            pkt = (trtp_rtp_packet_t*)item->data;

            // TODO:目前没有使用header extension, 暂时mask
            // tdav_session_video_decode_rtp_header_ext(video, pkt);
            
            // set video_id after rscode decode
            pkt->header->video_id = pkt->header->csrc[3];
			ret = video->jb ?
				tdav_video_jb_put(video, video->jb, (trtp_rtp_packet_t*)pkt, iNAP, iCheckCodeLen,rscode)
            : _tdav_session_video_decode(video, pkt);
            
            TSK_OBJECT_SAFE_FREE(item);
        }
    }
    return 0;
}


static int tdav_session_video_rtcp_cb(const void* callback_data, const trtp_rtcp_packet_t* packet)
{
    TMEDIA_I_AM_ACTIVE(100, "tdav_session_video_rtcp_cb is alive!");

    int ret = 0;
    const trtp_rtcp_report_psfb_t* psfb;
    const trtp_rtcp_report_rtpfb_t* rtpfb;
    const trtp_rtcp_rblocks_L_t* blocks = tsk_null;
    
    tdav_session_video_t* video = (tdav_session_video_t*)callback_data;
    tdav_session_av_t* base = (tdav_session_av_t*)callback_data;
    tmedia_session_t* session = (tmedia_session_t*)callback_data;
    
    //第一个block永远是SR，而且必须是，否则无法计算RTT
    uint32_t sr_ssrc = 0;
    if(packet->header->type == trtp_rtcp_packet_type_sr){
        const trtp_rtcp_report_sr_t* sr = (const trtp_rtcp_report_sr_t*)packet;
        sr_ssrc = sr->ssrc;
        if (base->rtp_manager && base->rtp_manager->sessionid != sr_ssrc &&
            video->session_map && video->session_map->find(sr_ssrc) == video->session_map->end()) {
            //新用户进入,重编I帧
            _tdav_session_video_remote_requested_idr(video, sr_ssrc);
            video->session_map->insert(sr_ssrc);
        }
       // TSK_DEBUG_INFO("rtcp cb rr sessionid:%d", sr_ssrc);
    }
    
    int video_up_lossrate = 0, audio_up_lossrate = 0;
    int video_dn_lossrate = 0, audio_dn_lossrate = 0;
    int audio_up_lossrate_max = 0;
    int audio_dn_lossrate_max = 0;

    int data_rtt_max = 0;
    if((blocks = (packet->header->type == trtp_rtcp_packet_type_rr) ? ((const trtp_rtcp_report_rr_t*)packet)->blocks :
        (packet->header->type == trtp_rtcp_packet_type_sr ? ((const trtp_rtcp_report_sr_t*)packet)->blocks : tsk_null))) {
        const tsk_list_item_t* item;
        const trtp_rtcp_rblock_t* block;
        int tmp_video_up_lossrate = 0, tmp_video_dn_lossrate = 0;
        
        Config_SetInt("data_rtt", 0);
        int max_rtt_limit = Config_GetInt("max_rtt", 1000);

        tsk_list_foreach(item, blocks) {
            if(!(block = (const trtp_rtcp_rblock_t*)item->data)) {
                continue;
            }
            if(base->rtp_manager->rtp.ssrc.local == block->ssrc) {
                // TSK_DEBUG_INFO("RTCP pkt loss fraction=%d, congestion_ctrl_enabled=%d", block->audio_fraction, (int)base->congestion_ctrl_enabled);

                struct trtp_rtcp_stat_qos_s stat_qos;
                memset(&stat_qos, 0, sizeof(struct trtp_rtcp_stat_qos_s));

                ret = trtp_rtcp_session_get_source_qos(base->rtp_manager->rtcp.session, sr_ssrc, block, &stat_qos);
                TMEDIA_I_AM_ACTIVE(3, "RTCP QOS - ssrc: %u ppl: up[a:%.1lf%%, v:%.1lf%%] dn[a:%.1lf%%, v:%.1lf%%] ab[%.1lfkbps] vb[%.1lfkbps][%u, %u] vsb[%.1lfkbps] jitter: [a: %u, v:%u] rtt=%d, stat_rtt=[%u, %u, %u]"
                    , sr_ssrc, 100*block->audio_fraction/256.0, 100*block->video_fraction/256.0, 100*stat_qos.a_down_fraction/256.0
                    , 100*stat_qos.v_down_fraction/256.0, 8*stat_qos.a_down_bitrate/1000.0, 8*stat_qos.v_down_bitrate/1000.0
                    , stat_qos.v_main_count, stat_qos.v_minor_count, 8*stat_qos.v_down_share_bitrate/1000.0 
                    , stat_qos.a_jitter, stat_qos.v_jitter, stat_qos.rtt, stat_qos.avg_rtt, stat_qos.min_rtt, stat_qos.max_rtt);

                if (stat_qos.rtt >= max_rtt_limit) {
                    video_up_lossrate = 51;
                }
                int real_up_loss = (int)(100 * block->video_fraction / 256.0);
                video_up_lossrate = (real_up_loss > video_up_lossrate) ? real_up_loss : video_up_lossrate;
                if (tmp_video_up_lossrate < video_up_lossrate) {
                    tmp_video_up_lossrate = video_up_lossrate;
                }
                setVideoUpPacketLossRtcp((int)(1000*block->video_fraction/256.0), sr_ssrc);

                video_dn_lossrate = (int)(100 * stat_qos.v_down_fraction / 256.0);
                if (tmp_video_dn_lossrate < video_dn_lossrate) {
                    tmp_video_dn_lossrate = video_dn_lossrate;
                }
                setVideoDnPacketLossRtcp((int)(1000*stat_qos.v_down_fraction/256.0), sr_ssrc);

                audio_up_lossrate = (int)(100*block->audio_fraction/256.0);
                if (audio_up_lossrate_max < audio_up_lossrate) {
                    audio_up_lossrate_max = audio_up_lossrate;
                }
                setAudioUpPacketLossRtcp((int)(1000*block->audio_fraction/256.0), sr_ssrc);

                audio_dn_lossrate = (int)(100*stat_qos.a_down_fraction/256.0);
                if (audio_dn_lossrate_max < audio_dn_lossrate) {
                    audio_dn_lossrate_max = audio_dn_lossrate;
                }
                setAudioDnPacketLossRtcp((int)(1000*stat_qos.a_down_fraction/256.0), sr_ssrc);

                data_rtt_max = stat_qos.rtt;
                if (Config_GetInt("data_rtt", 0) < data_rtt_max) {
                    Config_SetInt("data_rtt", data_rtt_max );
                }

                setAudioPacketDelayRtcp(stat_qos.rtt, sr_ssrc);
                setVideoPacketDelayRtcp(stat_qos.rtt, sr_ssrc);

                // Global packet loss estimation
                if (base->congestion_ctrl_enabled) {
                    float q2;
                    q2 = block->audio_fraction == 0 ? 1.f : ((float)block->audio_fraction / 256.f);
                    tsk_mutex_lock(video->h_mutex_qos);
                    session->qos_metrics.q2 = (session->qos_metrics.q2 + q2) / (video->q2_n++ ? 2.f : 1.f);
                    tsk_mutex_unlock(video->h_mutex_qos);
                }
                break;
            }
        }
        Config_SetInt("video_up_lossRate", tmp_video_up_lossrate < 3 ? Config_GetInt("video_up_lossRate",0) * 2 / 3 : tmp_video_up_lossrate );
        Config_SetInt("video_dn_lossrate", tmp_video_dn_lossrate < 3 ? Config_GetInt("video_dn_lossrate",0) * 2 / 3 : tmp_video_dn_lossrate );
    }

    if (audio_up_lossrate_max != video->lastUpAudioLoss_max) {
        // for audio ppl
        if (video->staticsQosCb) {
            video->staticsQosCb(0, 0, 0, audio_up_lossrate_max);
        }
        video->lastUpAudioLoss_max = audio_up_lossrate_max < 3 ? Config_GetInt("audio_up_lossRate",0) * 2 / 3  : audio_up_lossrate_max;
        Config_SetInt("audio_up_lossRate", video->lastUpAudioLoss_max);
        
    }

    if (audio_dn_lossrate_max != video->lastDnAudioLoss_max) {
        video->lastDnAudioLoss_max = audio_dn_lossrate_max;
        Config_SetInt("audio_dn_lossRate", audio_dn_lossrate_max);
    }

    int i = 0;
    while((psfb = (const trtp_rtcp_report_psfb_t*)trtp_rtcp_packet_get_at(packet, trtp_rtcp_packet_type_psfb, i++))) {
    switch(psfb->fci_type) {
        case trtp_rtcp_psfb_fci_type_fir: {
            for (int i = 0; i < psfb->fir.count; i++) {
                if (psfb->fir.ssrc[i] == base->rtp_manager->sessionid) {
                      TSK_DEBUG_INFO("Receiving RTCP-FIR (%u, %u)", ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_sender, psfb->fir.ssrc[i]);
                      _tdav_session_video_remote_requested_idr(video, ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media);

                }
            }
        }
        break;
        case trtp_rtcp_psfb_fci_type_pli: {
    
        }
            break;
        default:
            break;
        }
    }
    
    #if 0
    i = 0;
    while((psfb = (const trtp_rtcp_report_psfb_t*)trtp_rtcp_packet_get_at(packet, trtp_rtcp_packet_type_psfb, i++))) {
        switch(psfb->fci_type) {
            case trtp_rtcp_psfb_fci_type_fir: {
                TSK_DEBUG_INFO("Receiving RTCP-FIR (%u)", ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media);
                _tdav_session_video_remote_requested_idr(video, ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media);
                break;
            }
            case trtp_rtcp_psfb_fci_type_pli: {
                uint64_t now;
                TSK_DEBUG_INFO("Receiving RTCP-PLI (%u)", ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media);
                now = tsk_time_now();
                // more than one PLI in 500ms ?
                // "if" removed because PLI really means codec prediction chain is broken
                /*if((now - video->avpf.last_pli_time) < 500)*/{
                    _tdav_session_video_remote_requested_idr(video, ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media);
                }
                video->avpf.last_pli_time = now;
                break;
            }
            case trtp_rtcp_psfb_fci_type_afb: {
                if (psfb->afb.type == trtp_rtcp_psfb_afb_type_remb) {
                    uint64_t bw_up_reported_kpbs = ((psfb->afb.remb.mantissa << psfb->afb.remb.exp) >> 10);
                    TSK_DEBUG_INFO("Receiving RTCP-AFB-REMB (%u), exp=%u, mantissa=%u, bandwidth=%llukbps", ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media, psfb->afb.remb.exp, psfb->afb.remb.mantissa, bw_up_reported_kpbs);
                    if (base->congestion_ctrl_enabled) {
                        if (session->qos_metrics.bw_up_est_kbps != 0) {
                            float q3 = bw_up_reported_kpbs / (float)session->qos_metrics.bw_up_est_kbps;
                            q3 = TSK_CLAMP(0.f, q3, 1.f);
                            TSK_DEBUG_INFO("bw_up_estimated_kbps=%u, bw_up_reported_kpbs=%llu, q3=%f", session->qos_metrics.bw_up_est_kbps, bw_up_reported_kpbs, q3);
                            tsk_mutex_lock(video->h_mutex_qos);
                            session->qos_metrics.q3 = (session->qos_metrics.q3 + q3) / (video->q3_n++ ? 2.f : 1.f);
                            tsk_mutex_unlock(video->h_mutex_qos);
                        }
                    }
                }
                else if (psfb->afb.type == trtp_rtcp_psfb_afb_type_jcng) {
                    float jcng_q = ((float)psfb->afb.jcng.q / 255.f);
                    TSK_DEBUG_INFO("Receiving RTCP-AFB-JCNG (%u), q_recv=%u, q_dec=%f", ((const trtp_rtcp_report_fb_t*)psfb)->ssrc_media, psfb->afb.jcng.q, jcng_q);
                    if (base->congestion_ctrl_enabled) {
                        tsk_mutex_lock(video->h_mutex_qos);
                        session->qos_metrics.q5 = (session->qos_metrics.q5 + jcng_q) / (video->q5_n++ ? 2.f : 1.f);
                        tsk_mutex_unlock(video->h_mutex_qos);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    
    i = 0;
    while((rtpfb = (const trtp_rtcp_report_rtpfb_t*)trtp_rtcp_packet_get_at(packet, trtp_rtcp_packet_type_rtpfb, i++))) {
        switch(rtpfb->fci_type) {
            default:
                break;
            case trtp_rtcp_rtpfb_fci_type_nack: {
                if(rtpfb->nack.blp && rtpfb->nack.pid) {
                    tsk_size_t i;
                    int32_t j;
                    uint16_t pid, blp;
                    uint32_t r = 0; // the number of recoverable packets (lost but recovered using a NACK requests)
                    uint32_t u = 0; // the number of unrecoverable packets (lost but not recovered using NACK requests)
                    const tsk_list_item_t* item;
                    const trtp_rtp_packet_t* pkt_rtp;
                    for(i = 0; i < rtpfb->nack.count; ++i) {
                        static const int32_t __Pow2[16] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000 };
                        int32_t blp_count;
                        blp = rtpfb->nack.blp[i];
                        blp_count = blp ? 16 : 0;
                        
                        for(j = -1/*Packet ID (PID)*/; j < blp_count; ++j) {
                            if(j == -1 || (blp & __Pow2[j])) {
                                pid = (rtpfb->nack.pid[i] + (j + 1));
                                tsk_list_lock(video->avpf.packets);
                                tsk_list_foreach(item, video->avpf.packets) {
                                    if(!(pkt_rtp = (const trtp_rtp_packet_t *)item->data)) {
                                        continue;
                                    }
                                    
                                    // Very Important: the seq_nums are not consecutive because of wrapping.
                                    // For example, '65533, 65534, 65535, 0, 1' is a valid sequences which means we have to check all packets (probaly need somthing smarter)
                                    if(pkt_rtp->header->seq_num == pid) {
                                        ++r;
                                        TSK_DEBUG_INFO("NACK Found, pid=%d, blp=%u, r=%u", pid, blp, r);
                                        trtp_manager_send_rtp_packet(base->rtp_manager, pkt_rtp, tsk_true);
                                        break;
                                    }
                                    if(item == video->avpf.packets->tail) {
                                        // should never be called unless the tail is too small
                                        int32_t old_max = (int32_t)video->avpf.max;
                                        int32_t len_drop = (pkt_rtp->header->seq_num - pid);
                                        ++u;
                                        video->avpf.max = TSK_CLAMP((int32_t)tmedia_defaults_get_avpf_tail_min(), (old_max + len_drop), (int32_t)tmedia_defaults_get_avpf_tail_max());
                                        TSK_DEBUG_INFO("**NACK requesting dropped frames. List=[%d-%d], requested=%d, List.Max=%d, List.Count=%d, u=%u. RTT is probably too high.",
                                                       ((const trtp_rtp_packet_t*)TSK_LIST_FIRST_DATA(video->avpf.packets))->header->seq_num,
                                                       ((const trtp_rtp_packet_t*)TSK_LIST_LAST_DATA(video->avpf.packets))->header->seq_num,
                                                       pid,
                                                       (int)video->avpf.max,
                                                       (int)video->avpf.count,
                                                       (unsigned)u);
                                        // FIR not really requested but needed
                                        /*_tdav_session_video_remote_requested_idr(video, ((const trtp_rtcp_report_fb_t*)rtpfb)->ssrc_media);
                                         tsk_list_clear_items(video->avpf.packets);
                                         video->avpf.count = 0;*/
                                    } // if(last_item)
                                }// foreach(pkt)
                                tsk_list_unlock(video->avpf.packets);
                            }// if(BLP is set)
                        }// foreach(BIT in BLP)
                    }// foreach(nack)
                    if (base->congestion_ctrl_enabled) {
                        // Compute q1
                        if (r || u) {
                            float q1 = 1.f - (((r * 0.2f) + (u * 0.8f)) / (r + u));
                            tsk_mutex_lock(video->h_mutex_qos);
                            session->qos_metrics.q1 = (session->qos_metrics.q1 + q1) / (video->q1_n++ ? 2.f : 1.f);
                            tsk_mutex_unlock(video->h_mutex_qos);
                            TSK_DEBUG_INFO("RTCP-NACK: r=%u, u=%u, q1=%f", r, u, q1);
                        }
                    }
                }// if(nack-blp and nack-pid are set)
                break;
            }// case
        }// switch
    }// while(rtcp-pkt)
    #endif
    
    return ret;
}

static int tdav_session_switch_video_source_cb(const void* callback_data, int32_t sessionId, int32_t source_type)
{
    // call back for video switch source
    tdav_session_video_t* self = (tdav_session_video_t*)callback_data;

    if (self->staticsQosCb) {
        // TSK_DEBUG_INFO("tdav_session_switch_video_source_cb sessionId[%d], type[%d]", sessionId, source_type);
        self->staticsQosCb(1, sessionId, source_type, 0);
    }
    
    return 0;
}


static int tdav_session_video_encode_adjust_cb(const void* callback_data, int width, int height, int bitrate, int fps) 
{
    tdav_session_video_t* self = (tdav_session_video_t*)callback_data;

    if (self->videoAdjustCb) {
        //TSK_DEBUG_INFO("tdav_session_video_encode_adjust_cb w[%d], h[%d], bitrate[%d], fps[%d]", width, height, bitrate, fps);
        self->videoAdjustCb(width, height, bitrate, fps);
    }

    return 0;
}

// 后面要将上面两个回调集成到这个回调中，现在没时间弄
static int tdav_session_video_event_cb(const void* callback_data, int type, void* data, int error)
{
    tdav_session_video_t* self = (tdav_session_video_t*)callback_data;
    if (self->videoTransRouteCb) {
        //TSK_DEBUG_INFO("tdav_session_video_event_cb type[%d]", type);
        self->videoTransRouteCb(type, data, error);
    }

    return 0;
}

static int _tdav_session_video_set_defaults(tdav_session_video_t* self)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    self->jb_enabled = tmedia_defaults_get_videojb_enabled();
    self->zero_artifacts = tmedia_defaults_get_video_zeroartifacts_enabled();
    self->avpf.max = tmedia_defaults_get_avpf_tail_min();
    self->encoder.pkt_loss_level = tdav_session_video_pkt_loss_level_low;
    self->encoder.pkt_loss_prob_bad = 0; // honor first report
    self->encoder.pkt_loss_prob_good = TDAV_SESSION_VIDEO_PKT_LOSS_PROB_GOOD;
    self->encoder.last_frame_time = 0;
    
    // reset rotation info (MUST for reINVITE when mobile device in portrait[90 degrees])
    self->encoder.rotation = 0;
    
    TSK_DEBUG_INFO("Video 'zero-artifacts' option = %s", self->zero_artifacts  ? "yes" : "no");
    
    return 0;
}

// From jitter buffer to codec
static int _tdav_session_video_jb_cb(const tdav_video_jb_cb_data_xt* data)
{
    tdav_session_video_t* video = (tdav_session_video_t*)data->usr_data;
    tdav_session_av_t* base = (tdav_session_av_t*)data->usr_data;
    tmedia_session_t* session = (tmedia_session_t*)data->usr_data;
    
    switch(data->type){
    
        case tdav_video_jb_cb_data_type_rtp:
        {
            return _tdav_session_video_decode(video, data->rtp.pkt);
        }
        case tdav_video_jb_cb_data_type_tmfr:
        {
            if(base->rtp_manager && base->rtp_manager->is_started){
                base->time_last_frame_loss_report = tsk_time_now();
                _tdav_session_video_local_request_idr(session, "TMFR", data->ssrc);
                TSK_DEBUG_INFO("drop video request fri frame, ssrc:%d, sessonid:%d", data->ssrc, data->avpf.sessionid);
            }
        }
        default:
		break;
        case tdav_video_jb_cb_data_type_fl:
        {
            //TSK_DEBUG_INFO("vidoe jb cb datal lost gop :%d\n", data->fl.goptime);
            if (data->fl.count == data->fl.seq_num) {
                
                if (!data->fl.goptime) {
                    ((tdav_video_jb_cb_data_xt*)data)->fl.goptime = tsk_time_now();
                }
                if (tsk_time_now() - data->fl.goptime > TDAV_SESSION_VIDEO_LOST_PACKET_TIMEOUT) {
                    //回调
                    if (video->videoRuntimeEventCb) {
                        video->videoRuntimeEventCb(tdav_session_video_network_bad, data->fl.sessionid, false, true);
                    }
                    ((tdav_video_jb_cb_data_xt*)data)->fl.goptime = -1;
                }
                
            }
//            base->time_last_frame_loss_report = tsk_time_now();
//            if(data->fl.count > TDAV_SESSION_VIDEO_PKT_LOSS_MAX_COUNT_TO_REQUEST_FIR){
//                _tdav_session_video_local_request_idr(session, "TMFR", data->ssrc);
//            }
//            else {
//                if (base->avpf_mode_neg || base->is_fb_nack_neg) { // AVPF?
//                    // Send RTCP-NACK
//                    tsk_size_t i, j, k;
//                    uint16_t seq_nums[16];
//                    for(i = 0; i < data->fl.count; i+=16){
//                        for(j = 0, k = i; j < 16 && k < data->fl.count; ++j, ++k){
//                            seq_nums[j] = (uint16_t)(data->fl.seq_num + i + j);
//                            TSK_DEBUG_INFO("Request re-send(%u)", seq_nums[j]);
//                        }
//                        trtp_manager_signal_pkt_loss(base->rtp_manager, data->ssrc, seq_nums, j);
//                    }
//                }
//            }
            
            break;
        }
        case tdav_video_jb_cb_data_type_fps_changed:
        {
            if(base->congestion_ctrl_enabled){
                video->fps_changed = tsk_true;
                if(video->decoder.codec){
                    TSK_DEBUG_INFO("Congestion control enabled and fps updated from %u to %u", data->fps.old_fps, data->fps.new_fps);
                    TMEDIA_CODEC_VIDEO(video->decoder.codec)->in.fps = data->fps.new_fps;
                }
            }
            break;
        }
    }
    
    return 0;
}

// 构造nack请求报文兵发送给server
static int _tdav_sesiion_video_nack_cb(const trtp_rtcp_source_seq_map* data, void* usr_data) {
    // tdav_session_video_t* self = (tdav_session_video_t*)usr_data;
     tdav_session_av_t* base = (tdav_session_av_t*)usr_data;
    
    // sanity check
    if (!data || data->empty()) {
        // no video packets need to retransmit
        TSK_DEBUG_ERROR("nack callback: invalid parameters");
        return -1;
    }

    if (data->size() > 0) {
        // TSK_DEBUG_ERROR("send nack, lost count:%d", data->size());
        //build nack pkt and send to mcu
        trtp_rtcp_session_signal_pkt_loss(base->rtp_manager->rtcp.session, const_cast<trtp_rtcp_source_seq_map*>(data));
    }

    return 0;
}

int _tdav_session_video_open_decoder(tdav_session_video_t* self, uint8_t payload_type)
{
    int ret = 0;
    
    if ((self->decoder.codec_payload_type != payload_type) || !self->decoder.codec) {
        tsk_istr_t format;
        TSK_OBJECT_SAFE_FREE(self->decoder.codec);
        tsk_itoa(payload_type, &format);
        if (!(self->decoder.codec = tmedia_codec_find_by_format(TMEDIA_SESSION(self)->neg_codecs, format)) || !self->decoder.codec->plugin || (self->decoder.codec->plugin->decode && self->decoder.codec->plugin->decode_new)) {
            TSK_DEBUG_ERROR("%s is not a valid payload for this session", format);
            ret = -2;
            goto bail;
        }
        self->decoder.codec_payload_type = payload_type;
        self->decoder.codec_decoded_frames_count = 0; // because we switched the codecs
    }
    // Open codec if not already done
    if (!TMEDIA_CODEC(self->decoder.codec)->opened){
        if ((ret = tmedia_codec_open(self->decoder.codec, self))) {
            TSK_DEBUG_ERROR("Failed to open [%s] codec", self->decoder.codec->plugin->desc);
            goto bail;
        }
        self->decoder.codec_decoded_frames_count = 0; // because first time to use
        
    }
    //set callback by bao 2017-09-10
    //decoder
    tmedia_param_s* media_param;
    media_param = tmedia_param_create (tmedia_pat_set, tmedia_video, tmedia_ppt_session, tmedia_pvt_pvoid, "session_decode_callback",
                                       (void *)tdav_webrtc_decode_h264_callback);
    if (media_param)
    {
        tmedia_codec_set (TMEDIA_CODEC (self->decoder.codec), media_param);
        TSK_OBJECT_SAFE_FREE (media_param);
    }
    
    // for h264 callback before decode
    tmedia_param_t* prevideo_need_decode_and_render_param;
    prevideo_need_decode_and_render_param = tmedia_param_create(tmedia_pat_set,
                                                                tmedia_video,
                                                                tmedia_ppt_codec,
                                                                tmedia_pvt_int32,
                                                TMEDIA_PARAM_KEY_VIDEO_PREDECODE_NEED_DECODE_AND_RENDER,
                                                (void*)(&self->decoder.predecode_need_decode_and_render));
    if (!prevideo_need_decode_and_render_param) {
        TSK_DEBUG_ERROR("Failed to create a media parameter to set videoPreDecode need_decode_and_render:%d", self->decoder.predecode_need_decode_and_render);
    } else {
        tmedia_codec_set(TMEDIA_CODEC (self->decoder.codec), prevideo_need_decode_and_render_param);
        TSK_OBJECT_SAFE_FREE (prevideo_need_decode_and_render_param);
    }
    
    tmedia_param_t* prevideo_callback_param;
    prevideo_callback_param = tmedia_param_create(tmedia_pat_set,
                                                tmedia_video,
                                                tmedia_ppt_codec,
                                                tmedia_pvt_pvoid,
                                                TMEDIA_PARAM_KEY_VIDEO_PREDECODE_CB,
                                                (void*)(self->decoder.predecodeCb));
    if (!prevideo_callback_param) {
        TSK_DEBUG_ERROR("Failed to create a media parameter to set videoPreDecode callback:%p", self->decoder.predecodeCb);
    } else {
        tmedia_codec_set(TMEDIA_CODEC (self->decoder.codec), prevideo_callback_param);
        TSK_OBJECT_SAFE_FREE (prevideo_callback_param);
    }
    
    // for report decode param 
    tmedia_param_t* decode_callback_param;
    decode_callback_param = tmedia_param_create(tmedia_pat_set,
                                                tmedia_video,
                                                tmedia_ppt_codec,
                                                tmedia_pvt_pvoid,
                                                TMEDIA_PARAM_KEY_VIDEO_DECODE_PARAM_CB,
                                                (void*)(self->videoDecodeParamCb));
    if (!decode_callback_param) {
        TSK_DEBUG_ERROR("Failed to create a media parameter to set decode param callback:%p", self->videoDecodeParamCb);
    } else {
        tmedia_codec_set(TMEDIA_CODEC (self->decoder.codec), decode_callback_param);
        TSK_OBJECT_SAFE_FREE (decode_callback_param);
    }

bail:
    return ret;
}

int _tdav_session_video_create_new_decoded_inst(tdav_session_video_t* self, int32_t sessionId)
{
    int ret = 0;
    std::pair<decoder_consumer_map_t::iterator, bool> insertRet;
    
    decoder_consumer_manager_t* inst = new decoder_consumer_manager_t;
    inst->decoder_fps = TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.fps;
    inst->decoder_no_frame_cnt = 0;
    inst->decoder_last_frame_cnt = 0;
    inst->decoder_frame_cnt = 0;
    inst->decoder_first_put_frame = tsk_true;
    inst->decoder_started = tsk_false;
    inst->decoder_lst_cnt = 0;
    inst->decoder_reported_no_frame = tsk_false;
    inst->consumer_thread_running = tsk_false;
    inst->video_session = self;
    inst->decoder_session_id = sessionId;
    inst->consumer_thread_sema = tsk_null;
    inst->decoder_check_yuv_act = tsk_false;
    inst->decoder_codec_check_enable = tsk_true;
    inst->decoder_codec_check_enable = tsk_true;
    inst->decoder_codec_check_cnt = 0;
    inst->decoder_check_yuv_cnt = 0;
    inst->decoder_check_yuv_cont = 0;
    inst->decoder_check_yuv_unit = 0;
    inst->decoder_codec_input_time = 0;
    inst->decoder_codec_frame_cnt = 0;
    if (!(inst->decoder_frame_lst = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create list for consumer instance");
        if (inst) {
            delete inst;
            inst = tsk_null;
        }
        return -1;
    }
    
    insertRet = self->video_consumer_map->insert(decoder_consumer_map_t::value_type(sessionId, inst));
    return ret;
}

int _tdav_consumer_thread_start(decoder_consumer_manager_t *self)
{
    int ret = 0;
    
    if (!self ) {
        TSK_DEBUG_ERROR("Invalid parameter(%d)", self);
        return -1;
    }
    
    if (!self->video_session) {
        TSK_DEBUG_ERROR("Invalid parameter session (%d)", self->video_session);
        return -1;
    }
    
    if (self->consumer_thread_running) {
        return 0;
    }

	/*
    if (0 != pthread_mutex_init(&self->consumer_thread_mutex, NULL)) {
        TSK_DEBUG_ERROR("Consumer thread mutex init failed");
        return -1;
    }
    if (0 != pthread_cond_init(&self->consumer_thread_cond, NULL)) {
        TSK_DEBUG_ERROR("Consumer thread cond init failed");
        return -1;
    }
    */
    /*
	self->consumer_thread_cond = tsk_cond_create(false, false);
	if (self->consumer_thread_cond == NULL){
		return -1;
	}
     */
    self->consumer_thread_running = tsk_true;
    self->consumer_thread_sema = tsk_semaphore_create();
    if (!self->consumer_thread_sema) {
        TSK_DEBUG_ERROR("Consumer thread sema create failed");
        return -1;
    }
    ret = tsk_thread_create(&self->consumer_thread_handle, &tdav_video_consumer_thread, (void*)self);
    //ret = pthread_create(&self->consumer_thread_handle, NULL, &tdav_video_consumer_thread, (void*)self);
    if (0 != ret) {
        TSK_DEBUG_ERROR("Consumer thread create failed");
        return -1;
    }
    return 0;
}

int _tdav_session_video_put_decoded_frame(tdav_session_video_t* self, const void* buffer, tsk_size_t size, trtp_rtp_header_t* rtp_hdr, bool isTexture)
{
    int ret = 0;
    uint64_t now, duration;
    decoder_consumer_manager_t* video_decoded_inst = tsk_null;
    
    if (!self || !buffer || !rtp_hdr || (!size && !isTexture)) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    decoder_consumer_map_t::iterator it = self->video_consumer_map->find(rtp_hdr->session_id);
    if (it == self->video_consumer_map->end()) {
        /*
        // Create the new instance
        if (_tdav_session_video_create_new_decoded_inst(self, rtp_hdr->session_id) != 0) {
            TSK_DEBUG_ERROR("Failed to create video decoded output list for session:%d", rtp_hdr->session_id);
            return -2;
        }
        it = self->video_consumer_map->find(rtp_hdr->session_id);
        if (it == self->video_consumer_map->end()) {
            TSK_DEBUG_ERROR("impossible");
            return -3;
        }
        
        video_decoded_inst = it->second;
        // Create the thread reference to "video_decoded_inst"
        _tdav_consumer_thread_start(video_decoded_inst);
        
        TSK_DEBUG_INFO("Sucessfully created video decoded output list for session:%d", rtp_hdr->session_id);
         */
        return -1;
    }
    
    video_decoded_inst = it->second;
    
    // Refresh the fps
    if (!video_decoded_inst->decoder_first_put_frame) {

        duration = (rtp_hdr->timestamp - video_decoded_inst->decoder_last_frame_timeStamp);
        
        // Clamp
        duration = (int16_t)(TSK_CLAMP(20, duration, 1000));
        video_decoded_inst->decoder_fps = (uint16_t)(0.4f * 1000.0f / duration + 0.6f * video_decoded_inst->decoder_fps); // Average fps
        //TSK_DEBUG_INFO("PUT->decoder fps:%d, duration:%lld, ts:%u, last_ts:%u"
        //    , video_decoded_inst->decoder_fps, duration, rtp_hdr->timestamp, video_decoded_inst->decoder_last_frame_timeStamp);

        video_decoded_inst->decoder_last_frame_timeStamp = rtp_hdr->timestamp;
    } else {
        video_decoded_inst->decoder_first_put_frame = tsk_false;
        video_decoded_inst->decoder_last_frame_timeStamp = rtp_hdr->timestamp;
    }
    
    if (video_decoded_inst) {
        if (!isTexture) {
            decoder_frame_t* frame = (decoder_frame_t*)tsk_object_new(tdav_session_video_decoded_frame_def_t, (uint32_t)size);
            memcpy(frame->_buffer, buffer, frame->_size);                 // copy data
            memcpy(frame->_rtp_hdr, rtp_hdr, sizeof(trtp_rtp_header_t));  // copy header

            tsk_list_lock(video_decoded_inst->decoder_frame_lst);
            // Check if the list is full
            if (video_decoded_inst->decoder_lst_cnt >= MAX_VIDEO_CONSUMER_FRAME_NUM) {
                TSK_DEBUG_WARN("PUT->Check the list is full, pop the 1st item and drop, sessionId=%d, cnt=%d, decode_fps:%u"
                            , rtp_hdr->session_id, video_decoded_inst->decoder_lst_cnt, video_decoded_inst->decoder_fps);
                tsk_list_remove_first_item(video_decoded_inst->decoder_frame_lst);
                video_decoded_inst->decoder_lst_cnt--;
            }
            tsk_list_push_back_data(video_decoded_inst->decoder_frame_lst, (void**)&frame);
            video_decoded_inst->decoder_lst_cnt++;
            tsk_list_unlock(video_decoded_inst->decoder_frame_lst);
            //TSK_DEBUG_INFO("[paul debug] list length=%d, sessionId=%d", video_decoded_inst->decoder_lst_cnt, rtp_hdr->session_id);
            tsk_semaphore_increment(video_decoded_inst->consumer_thread_sema);
            video_decoded_inst->decoder_frame_cnt++;
        }
        else{
            tdav_session_av_t* base = (tdav_session_av_t*)self;
            tmedia_consumer_consume(base->consumer, buffer, size, rtp_hdr);
            video_decoded_inst->decoder_frame_cnt++;
        }
        
    }
    
    return ret;
}

static void* TSK_STDCALL tdav_video_consumer_thread(void *param)
{
    decoder_consumer_manager_t* video_decoded_inst = (decoder_consumer_manager_t*)param;
    tdav_session_video_t *video = TDAV_SESSION_VIDEO(video_decoded_inst->video_session);
    tdav_session_av_t* base = (tdav_session_av_t*)video;
    tsk_list_item_t* item = tsk_null;
    decoder_frame_t* frame = tsk_null;
    struct timeval  curTimeVal;
   // struct timespec timeoutSpec;
    long waitTimeUs = 0;
    float fineTuneTime = 0.0f;
    int16_t fineTuneLstLen = 0;
    uint64_t renderDelayMs = 0;
    uint64_t startRenderTimeMs = 0;
    uint64_t cachedCountBeforeStart = 0;

    TSK_DEBUG_INFO("tdav_video_consumer_thread - START, sessionId=%d", video_decoded_inst->decoder_session_id);
    
    while (video_decoded_inst->consumer_thread_running)
    {
        // reset item
        item = tsk_null;
        tsk_list_lock(video_decoded_inst->decoder_frame_lst);
        cachedCountBeforeStart = tsk_list_count_all(video_decoded_inst->decoder_frame_lst);
        tsk_list_unlock(video_decoded_inst->decoder_frame_lst);
		if (cachedCountBeforeStart > 0){
			cachedCountBeforeStart--;
		}
		else{
			if (0 != tsk_semaphore_decrement(video_decoded_inst->consumer_thread_sema)) {
				TSK_DEBUG_ERROR("Fatal error: consumer_thread_sema failed");
				break;
			}
		}
        
        if (!video_decoded_inst->consumer_thread_running)
        {
            TSK_DEBUG_INFO("WARNING:video producer thread stop");
            break;
        }
        
        // Check the list count
        if (video_decoded_inst->decoder_lst_cnt > 0/*2*/) { // At least buffer 3 frames can consume(render)
            video_decoded_inst->decoder_started = tsk_true;
        }else{
            video_decoded_inst->decoder_started = tsk_false;
        }

        if (video_decoded_inst->decoder_started) {
            // wait here, remember NOT lock the list, 8.0ms is the frame consume time
			// 這個綫程不做平滑計算和等待
            // waitTimeUs = (long)(1000.0f / video_decoded_inst->decoder_fps - renderDelayMs/*8.0*/ + fineTuneTime);
            // waitTimeUs = TSK_CLAMP(2, waitTimeUs, 100);
			// cond_wait_ret = tsk_cond_wait(video_decoded_inst->consumer_thread_cond, -1);

			
            // Consume the video frame
            //TSK_DEBUG_INFO("GET->list length=%d(%d), sessionId=%d", tsk_list_count(video_decoded_inst->decoder_frame_lst, tsk_null, tsk_null), video_decoded_inst->decoder_lst_cnt, video_decoded_inst->decoder_session_id);
            tsk_list_lock(video_decoded_inst->decoder_frame_lst);
            item = tsk_list_pop_first_item(video_decoded_inst->decoder_frame_lst);
            tsk_list_unlock(video_decoded_inst->decoder_frame_lst);
            if (item) {
                frame = (decoder_frame_t*)item->data;
                if (frame) {
					TMEDIA_I_AM_ACTIVE(200, "Video consumer thread consume one frame, size=%d", frame->_size);
                    //startRenderTimeMs = tsk_time_now();
                    tmedia_consumer_consume(base->consumer, frame->_buffer, frame->_size, frame->_rtp_hdr);
                   //renderDelayMs = TSK_CLAMP(2, (tsk_time_now() - startRenderTimeMs), 100);
                }
                video_decoded_inst->decoder_lst_cnt--;
            } else {
                TSK_DEBUG_WARN("GET->Check the list is empty, sessionId=%d", video_decoded_inst->decoder_session_id);
            }
			/*
            // Check is still nearly full or empty
            fineTuneLstLen = abs(video_decoded_inst->decoder_lst_cnt - MAX_VIDEO_CONSUMER_FRAME_NUM / 2);
            if (fineTuneLstLen >= 3) {
                fineTuneTime = 9.0f;
            } else if (fineTuneLstLen >= 2) {
                fineTuneTime = 6.0f;
            } else {
                fineTuneTime = 0.0f;
            }
            
            if (video_decoded_inst->decoder_lst_cnt > MAX_VIDEO_CONSUMER_FRAME_NUM / 2) {
                fineTuneTime = -fineTuneTime;
            }
			*/
        }
        
        if (item) {
            tsk_object_unref(item); // unref the item, then it will unref the item->data by itself
        }
    }
    
    /* signal waiting threads that this consumer thread is about to terminate */
    // tsk_cond_set(video_decoded_inst->consumer_thread_exit_cond);
/*
#ifndef TSK_UNDER_WINDOWS
    // free video_decoded_inst
    if (video_decoded_inst) {
        TSK_DEBUG_INFO("destroy consumer_thread_cond");
        //tsk_thread_destroy(&video_decoded_inst->consumer_thread_handle);
        tsk_cond_destroy(&video_decoded_inst->consumer_thread_cond);
        
        tsk_semaphore_destroy(&video_decoded_inst->consumer_thread_sema);
    }
    TSK_SAFE_FREE(video_decoded_inst);
#endif
 */

    TSK_DEBUG_INFO("tdav_video_consumer_thread - EXIT");
    
    return tsk_null;
}

static void* TSK_STDCALL tdav_video_monitor_thread(void *param)
{
    tdav_session_video_t *video = (tdav_session_video_t *)param;
  //  struct timeval curTimeVal;
   // struct timespec timeoutSpec;
	const int32_t interval = 100; // 100ms
	uint32_t videoNoFrameTimeout = tmedia_defaults_get_video_noframe_monitor_timeout();
	int32_t waitCnt = videoNoFrameTimeout / interval;
	decoder_consumer_map_t::iterator it;
	decoder_consumer_manager_t *video_decoded_inst;
    int32_t sessionId;
    
    TSK_DEBUG_INFO("tdav_video_monitor_thread - START");
    
    while (video->videoMonitorThreadStart) {
        // wait here
//        pthread_mutex_lock(&video->videoMonitorThreadMutex);
//        if (video->videoMonitorThreadStart) {
//            gettimeofday(&curTimeVal, NULL);
//            timeoutSpec.tv_sec  = curTimeVal.tv_sec + ((curTimeVal.tv_usec + interval * 1000) / 1000000);
//            timeoutSpec.tv_nsec = ((curTimeVal.tv_usec + interval * 1000) % 1000000) * 1000;
//            pthread_cond_timedwait(&video->videoMonitorThreadCond, &video->videoMonitorThreadMutex, &timeoutSpec);
//        }
//        pthread_mutex_unlock(&video->videoMonitorThreadMutex);
        tsk_cond_wait(video->videoMonitorThreadCond, interval);
        
        if (video->video_consumer_map->empty()) {
            continue;
        }
        for (it = video->video_consumer_map->begin(); it != video->video_consumer_map->end(); it++) {
            sessionId = it->first;
            video_decoded_inst = it->second;
			if (video_decoded_inst == nullptr)
			{
				continue;
			}
            if (sessionId != 0) {
                //TSK_DEBUG_INFO("[paul debug] level 3(%d)(%d)(%d)", video_decoded_inst->decoder_frame_cnt, video_decoded_inst->decoder_last_frame_cnt, video_decoded_inst->decoder_no_frame_cnt);
                // Check the last frame cnt == frame cnt
                if (video_decoded_inst->decoder_frame_cnt == video_decoded_inst->decoder_last_frame_cnt) {
                    video_decoded_inst->decoder_no_frame_cnt++;
                } else {
                    video_decoded_inst->decoder_last_frame_cnt = video_decoded_inst->decoder_frame_cnt;
                    video_decoded_inst->decoder_reported_no_frame = tsk_false;
                    video_decoded_inst->decoder_no_frame_cnt = 0;
                }
                
                if ((video_decoded_inst->decoder_no_frame_cnt >= waitCnt) && (!video_decoded_inst->decoder_reported_no_frame)) { // defaults 4s
                    TSK_DEBUG_INFO("No decoded frame comes(sessionId=%d)", sessionId);
                    video_decoded_inst->decoder_reported_no_frame = tsk_true;
                    if (video->videoRuntimeEventCb) {
                        video->videoRuntimeEventCb(tdav_session_video_shutdown, sessionId, false, false);
                        // delete_video_jb_session(video->jb, sessionId);
                    }
                }
            }
       
            //yuv检测
            if (video_decoded_inst->decoder_check_yuv_enable &&
                !video_decoded_inst->decoder_codec_check_enable &&
                video_decoded_inst->decoder_check_yuv_cnt < TDAV_SESSION_VIDEO_CHECK_TOTAL) {
               if (++video_decoded_inst->decoder_check_yuv_unit ==  TDAV_SESSION_VIDEO_CHECK_UNIT/interval) {
                   video_decoded_inst->decoder_check_yuv_act = tsk_true;
                   video_decoded_inst->decoder_check_yuv_unit = 0;
                   video_decoded_inst->decoder_check_yuv_cnt++;
                  }
            }
            
            //解码超时检测
            if (video_decoded_inst->decoder_codec_check_enable &&
                video_decoded_inst->decoder_codec_input_time != 0 &&
                tsk_time_now() - video_decoded_inst->decoder_codec_input_time > TDAV_SESSION_VIDEO_DECODER_TIMEOUT) {
                if (video_decoded_inst->decoder_codec_check_cnt >=3) {
                    //回调
                    if (video->videoRuntimeEventCb) {
                        video->videoRuntimeEventCb(tdav_session_video_decoder_error, sessionId, false, true);
                    }
                    video_decoded_inst->decoder_codec_check_enable = tsk_false;
                }
                video_decoded_inst->decoder_codec_check_cnt++;
            }
            
            //网络丢包检测
            
        }
    }
    TSK_DEBUG_INFO("tdav_video_monitor_thread - EXIT");
    return tsk_null;
}


static int  _tdav_session_video_decode_cb(const uint8_t * buff, int bufflen, int64_t timestamp, tdav_session_video_t * self, trtp_rtp_header_t * header, bool isTexture, int video_id)
{
    int ret = 0;
    tdav_session_av_t* base = (tdav_session_av_t*)self;
    tsk_size_t out_size = 0, _size = 0;
    const void* _buffer;
    decoder_consumer_map_t::iterator iter;
    // check
    if(!self->started || (bufflen < 2 && !isTexture) || !buff){
        return 0;
    }
    // check if stream is corrupted
    // the above decoding process is required in order to reset stream corruption status when IDR frame is decoded
    //    if(self->zero_artifacts && self->decoder.stream_corrupted && (__codecs_supporting_zero_artifacts & self->decoder.codec->id)){
    //        TSK_DEBUG_INFO("Do not render video frame because stream is corrupted and 'zero-artifacts' is enabled. Last seqnum=%u", video->decoder.last_seqnum);
    //        if(video->decoder.stream_corrupted && (tsk_time_now() - video->decoder.stream_corrupted_since) > TDAV_SESSION_VIDEO_AVPF_FIR_REQUEST_INTERVAL_MIN){
    //            TSK_DEBUG_INFO("Sending FIR to request IDR because frame corrupted since %llu...", video->decoder.stream_corrupted_since);
    //            _tdav_session_video_local_request_idr(TMEDIA_SESSION(video), "ZERO_ART_CORRUPTED", packet->header->ssrc);
    //        }
    //        goto bail;
    //    }
    
    // important: do not override the display size (used by the end-user) unless requested
    if(base->consumer->video.display.auto_resize){
        base->consumer->video.display.width = TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.width;//decoded width
        base->consumer->video.display.height = TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.height;//decoded height
    }
    
    // Convert decoded data to the consumer chroma and size
#define CONSUMER_NEED_DECODER (base->consumer->decoder.codec_id == tmedia_codec_id_none) // Otherwise, the consumer requires encoded frames
#define CONSUMER_IN_N_DISPLAY_MISMATCH        (!base->consumer->video.display.auto_resize && (base->consumer->video.in.width != base->consumer->video.display.width || base->consumer->video.in.height != base->consumer->video.display.height))
#define CONSUMER_DISPLAY_N_CODEC_MISMATCH        (base->consumer->video.display.width != TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.width || base->consumer->video.display.height != TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.height)
#define CONSUMER_DISPLAY_N_CONVERTER_MISMATCH    ( (self->conv.fromYUV420 && self->conv.fromYUV420->dstWidth != base->consumer->video.display.width) || (self->conv.fromYUV420 && self->conv.fromYUV420->dstHeight != base->consumer->video.display.height) )
#define CONSUMER_CHROMA_MISMATCH    (base->consumer->video.display.chroma != TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.chroma)
#define DECODED_NEED_FLIP    (TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.flip)
    
    if(CONSUMER_NEED_DECODER && (CONSUMER_CHROMA_MISMATCH || CONSUMER_DISPLAY_N_CODEC_MISMATCH || CONSUMER_IN_N_DISPLAY_MISMATCH || CONSUMER_DISPLAY_N_CONVERTER_MISMATCH || DECODED_NEED_FLIP)){
        
        // Create video converter if not already done
        if(!self->conv.fromYUV420 || CONSUMER_DISPLAY_N_CONVERTER_MISMATCH){
            TSK_OBJECT_SAFE_FREE(self->conv.fromYUV420);
            
            // create converter
            if(!(self->conv.fromYUV420 = tmedia_converter_video_create(TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.width, TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.height, TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.chroma, base->consumer->video.display.width, base->consumer->video.display.height,
                                                                       base->consumer->video.display.chroma))){
                TSK_DEBUG_ERROR("Failed to create video converter");
                ret = -3;
                goto bail;
            }
        }
    }
    
    // update consumer size using the codec decoded values
    // must be done here to avoid fooling "CONSUMER_IN_N_DISPLAY_MISMATCH" unless "auto_resize" option is enabled
    base->consumer->video.in.width = TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.width;//decoded width
    base->consumer->video.in.height = TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.height;//decoded height
    
    if(self->conv.fromYUV420){
        // update one-shot parameters
        tmedia_converter_video_set_flip(self->conv.fromYUV420, TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.flip);
        // convert data to the consumer's chroma
        out_size = tmedia_converter_video_process(self->conv.fromYUV420, buff, bufflen, &self->decoder.conv_buffer, &self->decoder.conv_buffer_size);
        if(!out_size || !self->decoder.conv_buffer){
            TSK_DEBUG_ERROR("Failed to convert YUV420 buffer to consumer's chroma");
            ret = -4;
            goto bail;
        }
        
        _buffer = self->decoder.conv_buffer;
        _size = out_size;
    }
    else{
        _buffer = buff;
        _size = bufflen;
    }
    
    // congetion control
    // send RTCP-REMB if:
    //  - congestion control is enabled and
    //  - fps changed or
    //    - first frame or
    //  - approximately every 1 seconds (1 = 1 * 1)
    if (base->congestion_ctrl_enabled && base->rtp_manager && (self->fps_changed || self->decoder.codec_decoded_frames_count == 0 || ((self->decoder.codec_decoded_frames_count % (TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.fps * 1)) == 0))){
        int32_t bandwidth_max_upload_kbps = base->bandwidth_max_upload_kbps;
        int32_t bandwidth_max_download_kbps = base->bandwidth_max_download_kbps; // user-defined (guard), INT_MAX if not defined
// bandwidth already computed in start() but the decoded video size was not correct and based on the SDP negotiation
bandwidth_max_download_kbps = TSK_MIN(
	tmedia_get_video_bandwidth_kbps_2(TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.width, TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.height, TMEDIA_CODEC_VIDEO(self->decoder.codec)->in.fps),
	bandwidth_max_download_kbps);
if (self->encoder.codec) {
	bandwidth_max_upload_kbps = TSK_MIN(
		tmedia_get_video_bandwidth_kbps_2(TMEDIA_CODEC_VIDEO(self->encoder.codec)->out.width, TMEDIA_CODEC_VIDEO(self->encoder.codec)->out.height, TMEDIA_CODEC_VIDEO(self->encoder.codec)->out.fps),
		bandwidth_max_upload_kbps);
}

self->fps_changed = tsk_false; // reset
TSK_DEBUG_INFO("video with congestion control enabled: max_bw_up(unused)=%d kpbs, max_bw_down=%d kpbs", bandwidth_max_upload_kbps, bandwidth_max_download_kbps);
ret = trtp_manager_set_app_bandwidth_max(base->rtp_manager, bandwidth_max_upload_kbps/* unused */, bandwidth_max_download_kbps);
	}

	// increace frame count and consume decoded video
	++self->decoder.codec_decoded_frames_count;
	//ret = tmedia_consumer_consume(base->consumer, _buffer, _size, packet->header);
	ret = _tdav_session_video_put_decoded_frame(self, _buffer, _size, header, isTexture);
	//yuv 异常检测
	iter = self->video_consumer_map->find(header->session_id);
	if (iter != self->video_consumer_map->end()) {
		decoder_consumer_manager_t * video_decoded_inst = iter->second;
		if (video_decoded_inst->decoder_codec_check_enable){
			video_decoded_inst->decoder_codec_input_time = 0;
			if (video_decoded_inst->decoder_codec_frame_cnt++ > 20)
				video_decoded_inst->decoder_codec_check_enable = tsk_false;
		}
		if (video_decoded_inst && video_decoded_inst->decoder_check_yuv_act && !isTexture)
		{
			tdav_session_video_check_unusual_t result = tdav_session_video_yuv_check((uint8_t*)_buffer, header->video_display_width, header->video_display_height);
			if (result != tdav_session_video_normal)
			{
				if (++video_decoded_inst->decoder_check_yuv_cont == TDAV_SESSION_VIDEO_CHECK_CONT) {

					//回调异常
					if (self->videoRuntimeEventCb) {
						self->videoRuntimeEventCb(result, header->session_id, false, true);
					}
					video_decoded_inst->decoder_check_yuv_cont = 0;
					video_decoded_inst->decoder_check_yuv_enable = tsk_false;

				}

			}
			else{
				video_decoded_inst->decoder_check_yuv_cont = 0;
				video_decoded_inst->decoder_check_yuv_enable = tsk_false;
			}
			video_decoded_inst->decoder_check_yuv_act = tsk_false;
		}

	}

bail:

	return ret;
}



bool parse_video_size(uint8_t* buff, int buffLen, uint32_t* width, uint32_t* height){

	int len = 0;
	for (int i = 0; i < buffLen - 4; ++i) {
		if (buff[i] == 0 &&
			buff[i + 1] == 0 &&
			buff[i + 2] == 0 &&
			buff[i + 3] == 1 &&
			(buff[i + 4] & 0x1f) == 8) {
			len = i;
			break;
		}
	}
	if (!len)
		return false;
	uint8_t* sps = &buff[1];
	uint32_t sps_len = len - 1;
	if (sps_len <= 0)
	{
		printf("h264 parse sps len == 0 !\n");
		return false;
	}
	rtc::Optional<youme::webrtc::SpsParser::SpsState> spsState = youme::webrtc::SpsParser::ParseSps(sps, sps_len);
	if (spsState->width == 0 || spsState->height == 0) {
		return false;
	}
	*width = spsState->width;// - (spsState->frame_crop_left_offset + spsState->frame_crop_right_offset)*2;
	*height = spsState->height;// - (spsState->frame_crop_top_offset + spsState->frame_crop_bottom_offset)*2;
	return true;
}

static int _tdav_session_video_decode(tdav_session_video_t* self, const trtp_rtp_packet_t* packet)
{
	tdav_session_av_t* base = (tdav_session_av_t*)self;
	static const tmedia_codec_id_t __codecs_supporting_zero_artifacts = (tmedia_codec_id_t)(tmedia_codec_id_vp8 | tmedia_codec_id_h264_bp | tmedia_codec_id_h264_mp | tmedia_codec_id_h263);
	int ret = 0;
	if (!self || !packet || !packet->header){
		TSK_DEBUG_ERROR("Invalid parameter: self=%d, packet=%d", self, packet);
		return -1;
	}

	tsk_safeobj_lock(base);
	if (self->started && base->consumer && base->consumer->is_started) {
		tsk_size_t out_size, _size;
		const void* _buffer;
		tdav_session_video_t* video = (tdav_session_video_t*)base;

		// Find the codec to use to decode the RTP payload
		if (!self->decoder.codec || self->decoder.codec_payload_type != packet->header->payload_type){
			if ((ret = _tdav_session_video_open_decoder(self, packet->header->payload_type))){
				TSK_DEBUG_ERROR("_tdav_session_video_decode open video decoder fail ret[%d] sessionid[%d] videoid[%d]"
					, ret, packet->header->session_id, packet->header->video_id);
				goto bail;
			}
		}

		// check whether bypassing is enabled (e.g. rtcweb breaker ON and media coder OFF)
		if (TMEDIA_SESSION(self)->bypass_decoding){
			// set codec id for internal use (useful to find codec with dynamic payload type)
			TRTP_RTP_HEADER(packet->header)->codec_id = self->decoder.codec->id;
			// consume the frame
			ret = tmedia_consumer_consume(base->consumer, (packet->payload.data ? packet->payload.data : packet->payload.data_const), packet->payload.size, packet->header);
			goto bail;
		}

		
		if (video->video_status_map )
		{
			auto status_iter = video->video_status_map->find(packet->header->session_id);
			if (status_iter == video->video_status_map->end())
			{
				status_iter = video->video_status_map->insert(std::make_pair(packet->header->session_id, false)).first;
			}
			if (!status_iter->second){
				const uint8_t* pay_ptr = tsk_null;
				tsk_size_t pay_size = 0;
				tsk_bool_t append_scp, end_of_unit;
				if ((!tdav_codec_h264_get_pay((packet->payload.data ? packet->payload.data : packet->payload.data_const), packet->payload.size, (const void**)&pay_ptr, &pay_size, &append_scp, &end_of_unit))
					&& pay_ptr && pay_size) {
					if ((pay_ptr[0] & 0x1F) == 0x7){

						uint32_t width = 0;
						uint32_t height = 0;
						if (parse_video_size((uint8_t*)pay_ptr, pay_size, &width, &height)){
							struct video_on_info
							{
								int session_id;
								uint32_t width;
								uint32_t height;
								int video_id;
							};
							TSK_DEBUG_INFO("video_on session:%d, width:%d, height:%d, video_id:%d", packet->header->session_id, width, height, packet->header->video_id); 
							video_on_info info = { packet->header->session_id, width, height, packet->header->video_id };
							tdav_session_video_event_cb(video, TRTP_ENGINE_EVENT_TYPE_VIDEO_ON, &info, 0);
							status_iter->second = true;
						}
					}
				}
			}
			
		}

        
        // Check if stream is corrupted or not
        //        if(video->decoder.last_seqnum && (video->decoder.last_seqnum + 1) != packet->header->seq_num){
        //            TSK_DEBUG_WARN("/!\\Video stream corrupted because of packet loss [%u - %u]. Pause rendering if 'zero_artifacts' (supported = %s, enabled = %s).",
        //                           video->decoder.last_seqnum,
        //                           packet->header->seq_num,
        //                           (__codecs_supporting_zero_artifacts & self->decoder.codec->id) ? "yes" : "no",
        //                           self->zero_artifacts ? "yes" : "no"
        //                           );
        //            if(!video->decoder.stream_corrupted){ // do not do the job twice
        //                if(self->zero_artifacts && (__codecs_supporting_zero_artifacts & self->decoder.codec->id)){
        //                    // request IDR now and every time after 'TDAV_SESSION_VIDEO_AVPF_FIR_REQUEST_INTERVAL_MIN' ellapsed
        //                    // 'zero-artifacts' not enabled then, we'll request IDR when decoding fails
        //                    TSK_DEBUG_INFO("Sending FIR to request IDR...");
        //                    _tdav_session_video_local_request_idr(TMEDIA_SESSION(video), "ZERO_ART_CORRUPTED", packet->header->ssrc);
        //                }
        //                // value will be updated when we decode an IDR frame
        //                video->decoder.stream_corrupted = tsk_true;
        //                video->decoder.stream_corrupted_since = tsk_time_now();
        //            }
        //            // will be used as guard to avoid redering corrupted IDR
        //            video->decoder.last_corrupted_timestamp = packet->header->timestamp;
        //        }
        video->decoder.last_seqnum = packet->header->seq_num; // update last seqnum

       
        // Decode data
        out_size = self->decoder.codec->plugin->decode_new(
                                                       self->decoder.codec,
                                                       (packet->payload.data ? packet->payload.data : packet->payload.data_const), packet->payload.size,
                                                       &self->decoder.buffer, &self->decoder.buffer_size,
                                                       packet->header,
                                                       self);
        if(out_size > 0 && self->started)
        {
            decoder_consumer_map_t::iterator iter;
            iter = self->video_consumer_map->find(packet->header->session_id);
            if (iter == self->video_consumer_map->end()) {
                if (_tdav_session_video_create_new_decoded_inst(self, packet->header->session_id) != 0) {
                    TSK_DEBUG_ERROR("Failed to create video decoded output list for session:%d", packet->header->session_id);
                    ret = -2;
                    goto bail;
                    //return -2;
                }
                iter = self->video_consumer_map->find(packet->header->session_id);
                if (iter == self->video_consumer_map->end()) {
                    TSK_DEBUG_ERROR("impossible");
                    ret = -3;
                    goto bail;
                    //return -3;
                }
                
                // Create the thread reference to "video_decoded_inst"
                _tdav_consumer_thread_start(iter->second);
                TSK_DEBUG_INFO("Sucessfully created video decoded output list for session:%d", packet->header->session_id);
                
            }
            if(iter->second->decoder_codec_check_enable)
                if(!iter->second->decoder_codec_input_time)
                    iter->second->decoder_codec_input_time = tsk_time_now();
             ret =  _tdav_session_video_decode_cb((const uint8_t*)self->decoder.buffer, out_size, packet->header->timestamp, self, packet->header, false, 0);
        }
        
        
    }
    else if (!base->consumer || !base->consumer->is_started) {
        TSK_DEBUG_INFO("Consumer not started (is_null=%d)", !base->consumer);
    }
    
bail:
    tsk_safeobj_unlock(base);
    
    return ret;
}

/* ============ Plugin interface ================= */

static int tdav_session_video_set(tmedia_session_t* self, const tmedia_param_t* param)
{
    int ret = 0;
    tdav_session_video_t* video;
    tdav_session_av_t* base;
    
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    // try with the base class to see if this option is supported or not
    if (tdav_session_av_set(TDAV_SESSION_AV(self), param) == tsk_true) {
        return 0;
    }
    
    video = (tdav_session_video_t*)self;
    base = (tdav_session_av_t*)self;
    
    if(param->plugin_type == tmedia_ppt_codec){
        tsk_mutex_lock(video->encoder.h_mutex);
        ret = tmedia_codec_set((tmedia_codec_t*)video->encoder.codec, param);
        tsk_mutex_unlock(video->encoder.h_mutex);
    }
    else if(param->plugin_type == tmedia_ppt_consumer){
        if(!base->consumer){
            TSK_DEBUG_ERROR("No consumer associated to this session");
            return -1;
        }
        if(param->value_type == tmedia_pvt_int32){
            if(tsk_striequals(param->key, "flip")){
                tsk_list_item_t* item;
                tsk_bool_t flip = (tsk_bool_t)TSK_TO_INT32((uint8_t*)param->value);
                tmedia_codecs_L_t *codecs = (tmedia_codecs_L_t*)tsk_object_ref(self->codecs);
                tsk_list_foreach(item, codecs){
                    TMEDIA_CODEC_VIDEO(item->data)->in.flip = flip;
                }
                tsk_object_unref(codecs);
            }
        }
        ret = tmedia_consumer_set_param(base->consumer, param);
    }
    else if(param->plugin_type == tmedia_ppt_producer){
        if(!base->producer){
            TSK_DEBUG_ERROR("No producer associated to this session");
            return -1;
        }
        if(param->value_type == tmedia_pvt_int32){
            if(tsk_striequals(param->key, "flip")){
                tsk_list_item_t* item;
                tsk_bool_t flip = (tsk_bool_t)TSK_TO_INT32((uint8_t*)param->value);
                tmedia_codecs_L_t *codecs = (tmedia_codecs_L_t*)tsk_object_ref(self->codecs);
                tsk_list_foreach(item, codecs){
                    TMEDIA_CODEC_VIDEO(item->data)->out.flip = flip;
                }
                tsk_object_unref(codecs);
            }
        }
        ret = tmedia_producer_set(base->producer, param);
    }
    else{
        if (param->value_type == tmedia_pvt_int32){
            if (tsk_striequals(param->key, "bandwidth-level")){
                tsk_list_item_t* item;
                self->bl = (tmedia_bandwidth_level_t)TSK_TO_INT32((uint8_t*)param->value);
                self->codecs = (tmedia_codecs_L_t*)tsk_object_ref(self->codecs);
                tsk_list_foreach(item, self->codecs){
                    ((tmedia_codec_t*)item->data)->bl = self->bl;
                }
                tsk_object_unref(self->codecs);
            } else if(tsk_striequals(param->key, TMEDIA_PARAM_KEY_SCREEN_ORIENTATION)) {
                base->screenOrientation = TSK_TO_INT32((uint8_t*)param->value);
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_NEED_DECODE_AND_RENDER)) {
                video->decoder.predecode_need_decode_and_render = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                TSK_DEBUG_INFO("Set videoPreDecode need_decode_and_render:%d", video->decoder.predecode_need_decode_and_render);
            }
        } else if (param->value_type == tmedia_pvt_pvoid) {
            if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_RUNTIME_EVENT_CB)) {
                video->videoRuntimeEventCb = (videoRuntimeEventCb_t)param->value;
            } else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_STATICS_QOS_CB)) {
                video->staticsQosCb = (staticsQoseCb_t)param->value;
            } else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_CB)) {
                video->decoder.predecodeCb = (predecodeCb_t)param->value;
                TSK_DEBUG_INFO("Set videoPreDecode callback:%p", video->decoder.predecodeCb);
            } else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_ENCODE_ADJUST_CB)) {
                video->videoAdjustCb = (videoEncodeAdjustCb_t)param->value;
            } else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_EVENT_CB)) {
                video->videoTransRouteCb = (videoTransRouteCb_t)param->value;
            } else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_DECODE_PARAM_CB)) {
                video->videoDecodeParamCb = (videoDecodeParamCb_t)param->value;
                TSK_DEBUG_INFO("Set decodeParam callback:%p", video->videoDecodeParamCb);
            }
        } 
    }
    
    return ret;
}

// static tdav_session_av_qos_stat_t* tdav_session_video_get_avqos_stat(tdav_session_video_t* self)
// {
//     if (!self) {
//         TSK_DEBUG_ERROR("Invalid parameter");
//         return -1;
//     }

//     return 0;
// }

static int tdav_session_video_get(tmedia_session_t* self, tmedia_param_t* param)
{
    if (!self || !param) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    tdav_session_av_t* base = (tdav_session_av_t*)self;

    // try with the base class to see if this option is supported or not
    if (tdav_session_av_get(TDAV_SESSION_AV(self), param) == tsk_true) {
        return 0;
    }
    else {
        if (param->plugin_type == tmedia_ppt_session) {
            if (param->value_type == tmedia_pvt_pobject) {
                if (tsk_striequals(param->key, "codec-encoder")) {
                    *((tsk_object_t**)param->value) = tsk_object_ref(TDAV_SESSION_VIDEO(self)->encoder.codec); // up to the caller to release the object
                    return 0;
                } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_AV_QOS_STAT)) {
                    
                    tdav_session_av_qos_stat_t* pavQosStat = new tdav_session_av_qos_stat_t;
                    
                    pavQosStat->audioUpLossCnt = TDAV_SESSION_VIDEO(self)->avQosStat.audioUpLossCnt;
                    pavQosStat->audioUpTotalCnt = TDAV_SESSION_VIDEO(self)->avQosStat.audioUpTotalCnt;
                    pavQosStat->videoUpLossCnt = TDAV_SESSION_VIDEO(self)->avQosStat.videoUpLossCnt;
                    pavQosStat->videoUpTotalCnt = TDAV_SESSION_VIDEO(self)->avQosStat.videoUpTotalCnt;

                    pavQosStat->audioDnLossrate = TDAV_SESSION_VIDEO(self)->avQosStat.audioDnLossrate;
                    pavQosStat->audioRtt = TDAV_SESSION_VIDEO(self)->avQosStat.audioRtt;
                    pavQosStat->videoDnLossrate = TDAV_SESSION_VIDEO(self)->avQosStat.videoDnLossrate;
                    pavQosStat->videoRtt = TDAV_SESSION_VIDEO(self)->avQosStat.videoRtt;

                    pavQosStat->auidoUpbitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.auidoUpbitrate * 8 / 1000);
                    pavQosStat->auidoDnbitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.auidoDnbitrate * 8 / 1000);

                    pavQosStat->videoUpCamerabitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.videoUpCamerabitrate * 8 / 1000);
                    pavQosStat->videoUpSharebitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.videoUpSharebitrate * 8 / 1000);
                    pavQosStat->videoDnCamerabitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.videoDnCamerabitrate * 8 / 1000);
                    pavQosStat->videoDnSharebitrate = (TDAV_SESSION_VIDEO(self)->avQosStat.videoDnSharebitrate * 8 / 1000);
                    pavQosStat->videoUpStream = TDAV_SESSION_VIDEO(self)->avQosStat.videoUpStream;

                    pavQosStat->videoDnFps = TDAV_SESSION_VIDEO(self)->avQosStat.videoDnFps;

                    trtp_manager_reset_report_stat(base->rtp_manager);
                    memset(&(TDAV_SESSION_VIDEO(self)->avQosStat), 0, sizeof(tdav_session_av_qos_stat_t));
                    *((tsk_object_t**)param->value) = pavQosStat;

                    return 0;
                }

            }
        }
    }
    
    TSK_DEBUG_WARN("This session doesn't support get(%s)", param->key);
    return -2;
}

static int tdav_session_video_prepare(tmedia_session_t* self)
{
    tdav_session_av_t* base = (tdav_session_av_t*)(self);
    // tdav_session_video_t* video = (tdav_session_video_t*)self;
    int ret = 0;
    
    // 获取audio初始化后的rtp manager对象指针，video和audio使用同个rtp manager对象
    base->rtp_manager = tdav_session_get_rtp_manager();
    
    if(base->rtp_manager){
        // 使用rtp video回调设置接口
        ret = trtp_manager_set_rtp_video_callback(base->rtp_manager, tdav_session_video_rtp_cb, base);
        ret = trtp_manager_set_rtcp_callback(base->rtp_manager, tdav_session_video_rtcp_cb, base);
        ret = trtp_manager_set_switch_video_callback(base->rtp_manager, tdav_session_switch_video_source_cb, base);
        ret = trtp_manager_set_video_adjust_callback(base->rtp_manager, tdav_session_video_encode_adjust_cb, base);
        ret = trtp_manager_set_video_route_callback(base->rtp_manager, tdav_session_video_event_cb, base);
        ret = trtp_manager_set_video_nack_check_callback(base->rtp_manager, tdav_session_video_nack_check_cb, base);

        // 设置rtp manager playloadtype, 使用H264_MP
        trtp_manager_set_video_payload_type(base->rtp_manager, atoi(TMEDIA_CODEC_FORMAT_H264_MP));
    }
    
    return ret;
}

static int tdav_session_video_start(tmedia_session_t* self)
{
    int ret;
    tdav_session_video_t* video;
    const tmedia_codec_t* codec;
    tdav_session_av_t* base;
    
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    video = (tdav_session_video_t*)self;
    base = (tdav_session_av_t*)self;
    
    if (video->started) {
        TSK_DEBUG_INFO("Video session already started");
        return 0;
    }
    
    // ENCODER codec
    if (!(codec = tdav_session_av_get_best_neg_codec(base))) {
        TSK_DEBUG_ERROR("No codec matched");
        return -2;
    }
    tsk_mutex_lock(video->encoder.h_mutex);
    TSK_OBJECT_SAFE_FREE(video->encoder.codec);
    video->encoder.codec = (tmedia_codec_s*)tsk_object_ref((tsk_object_t*)codec);

    // initialize the encoder using user-defined values
    if ((ret = tdav_session_av_init_encoder(base, video->encoder.codec))) {
        tsk_mutex_unlock(video->encoder.h_mutex);
        TSK_DEBUG_ERROR("Failed to initialize the encoder [%s] codec", video->encoder.codec->plugin->desc);
        return ret;
    }
    if (!TMEDIA_CODEC(video->encoder.codec)->opened) {
        if((ret = tmedia_codec_open(video->encoder.codec, self))){
            tsk_mutex_unlock(video->encoder.h_mutex);
            TSK_DEBUG_ERROR("Failed to open [%s] codec", video->encoder.codec->plugin->desc);
            return ret;
        }
    }
    tsk_mutex_unlock(video->encoder.h_mutex);
    
    if (video->jb) {
        if ((ret = tdav_video_jb_start(video->jb))) {
            TSK_DEBUG_ERROR("Failed to start jitter buffer");
            return ret;
        }
    }
    
    // 使用video session av start接口
    if ((ret = tdav_session_av_start_video(base, video->encoder.codec))) {
        TSK_DEBUG_ERROR("tdav_session_av_start(video) failed");
        return ret;
    }
    
    // 这里启动了关键的标志 video->started
    video->started = tsk_true;
    
    
    
    video->videoMonitorThreadCond = tsk_cond_create(false, false);
    if(video->videoMonitorThreadCond == NULL){
        return -1;
    }
    video->videoMonitorThreadStart = tsk_true;
    if(0 != tsk_thread_create(&video->videoMonitorThread,&tdav_video_monitor_thread, (void*)video)){
        return -1;
    }
    
//    if (0 != pthread_mutex_init(&video->videoMonitorThreadMutex, NULL)) {
//        return -1;
//    }
//    if (0 != pthread_cond_init(&video->videoMonitorThreadCond, NULL)) {
//        return -1;
//    }
//    video->videoMonitorThreadStart = tsk_true;
//    if (0 != pthread_create(&video->videoMonitorThread, NULL, &tdav_video_monitor_thread, (void*)video)) {
//        return -1;
//    }

    //encoder
    tmedia_param_t * media_param = tmedia_param_create (tmedia_pat_set, tmedia_video, tmedia_ppt_session, tmedia_pvt_pvoid, "session_encode_callback",
                                                        (void *)tdav_webrtc_encode_h264_callback);
    if (media_param)
    {
        tmedia_codec_set (TMEDIA_CODEC (video->encoder.codec), media_param);
        TSK_OBJECT_SAFE_FREE (media_param);
    }
    
    if(base->rtp_manager)
        base->rtp_manager->rtp.is_video_pause = false;
    
    return ret;
}

static int tdav_session_video_stop(tmedia_session_t* self)
{
    int ret = 0;
    tdav_session_video_t* video = (tdav_session_video_t*)self;
    tdav_session_av_t* base = (tdav_session_av_t*)self;
    
    if (video->started == tsk_false) {
        return 0;
    }
    
    TSK_DEBUG_INFO("@@ tdav_session_video_stop");
    
    // stop check av qos stat
    xt_timer_mgr_global_cancel(video->avQosStatTimer);

#if ANDROID
    // stop frame check
    xt_timer_mgr_global_cancel(video->check_timer_id);
    if (video->last_frame_buf) {
        TSK_FREE(video->last_frame_buf);
        video->last_frame_buf = tsk_null;
    }
    video->last_check_timestamp = 0;
#endif

    // must be here to make sure no other thread will lock the encoder once we have done it
    // tsk_mutex_lock(video->encoder.h_mutex); // encoder thread will check "started" var right after the lock is passed
    video->started = tsk_false;
    // tsk_mutex_unlock(video->encoder.h_mutex);
    // at this step we're sure that encode() will no longer be called which means we can safely close the codec
    //pause callback rtp data
    tsk_safeobj_lock(base);
    if(base->rtp_manager)
       base->rtp_manager->rtp.is_video_pause = true;
    
    ret = tdav_session_av_stop(base);
    
    if (video->jb) {
        ret = tdav_video_jb_stop(video->jb);
    }
    
    tdav_session_video_stop_rscode(video);
    
    // clear AVPF packets and wait for the dtor() before destroying the list
    tsk_list_lock(video->avpf.packets);
    tsk_list_clear_items(video->avpf.packets);
    tsk_list_unlock(video->avpf.packets);

    // tdav_session_av_stop() : stop producer and consumer, close encoder and all other codecs, stop rtpManager...
    // no need to lock the encoder to avoid using a closed codec (see above)
    // no need to lock the decoder as the rtpManager will be stop before closing the codec
    // lock-free stop() may avoid deadlock issue (cannot reproduce it myself) on Hovis
   // ret = tdav_session_av_stop(base);
    tsk_mutex_lock(video->encoder.h_mutex);
    TSK_OBJECT_SAFE_FREE(video->encoder.codec);
    tsk_mutex_unlock(video->encoder.h_mutex);
    TSK_OBJECT_SAFE_FREE(video->decoder.codec);

    //tdav_session_video_stop_rscode(video);
    // reset default values to make sure next start will be called with right defaults
    // do not call this function in start to avoid overriding values defined between prepare() and start()
    _tdav_session_video_set_defaults(video);
    tsk_safeobj_unlock(base);

    TSK_DEBUG_INFO("== tdav_session_video_stop");
    return ret;
}

static int tdav_session_video_clear(tmedia_session_t* self, int sessionId)
{
    TSK_DEBUG_INFO("tdav_session_video_clear Enter");
    decoder_consumer_map_t::iterator it;
    decoder_consumer_manager_t* video_decoded_inst = nullptr;
    tdav_session_video_t* video = (tdav_session_video_t*)self;
    tdav_session_av_t* base = (tdav_session_av_t*)self; //not use
    int ret = 0;

    if (video->started == tsk_false) {
        TSK_DEBUG_ERROR("tdav_session_video_clear video state invalid!");
        return 0;
    }

    // tsk_safeobj_lock(base);
    // close decoder with select sessionId
    tmedia_param_t* param = tmedia_param_create(tmedia_pat_set,
                                                tmedia_video,
                                                tmedia_ppt_codec,
                                                tmedia_pvt_int32,
                                                "close_decoder",
                                                (void*)&sessionId);
    if (!param) {
        TSK_DEBUG_ERROR("Failed to create a media parameter to close decoder sessionid:%d", sessionId);
    } else {
        TSK_DEBUG_INFO("close video decoder sessionId:%d", sessionId);
        tmedia_codec_set(TMEDIA_CODEC (video->decoder.codec), param);
        TSK_OBJECT_SAFE_FREE (param);
    }

    // delete video jb (include video and share)
    delete_video_jb_session(video->jb, sessionId);    
    delete_video_jb_session(video->jb, -1*sessionId);
    
    // delete video rscode thread
    do {
        tdav_rscode_t* rscode = nullptr;
        if (!video->rscode_list || 0 == tsk_list_count_all(video->rscode_list)) {
            break;
        }

        tsk_list_item_t *item = tsk_null;
        tsk_list_lock(video->rscode_list);
        item = video->rscode_list->head;
        //for(item = list ? list->head : tsk_null; item; item = item->next)
            
        while (item){
            if((TDAV_RSCODE(item->data)->session_id == sessionId || abs(TDAV_RSCODE(item->data)->session_id) == sessionId) 
               && (TDAV_RSCODE(item->data)->rs_type == RSC_TYPE_DECODE)) 
            {
                tsk_list_pop_item_by_data(video->rscode_list, item->data);
                rscode = TDAV_RSCODE(item->data);
                if (rscode) {
                    tdav_rscode_stop(rscode);
                    TSK_DEBUG_INFO("remove rscode from video rscode_list sessionId:%d, videoid:%d", sessionId, TDAV_RSCODE(item->data)->iVideoID);
                }
                
                TSK_OBJECT_SAFE_FREE(rscode);
                tsk_list_item_t *tmp = item;
                item = tmp->next;
                tmp->data = nullptr;
                TSK_OBJECT_SAFE_FREE(tmp);
                //item = video->rscode_list->head;

                if (!video->rscode_list->head) break;
                continue;
            }else{
                item = item->next;
            }
        }
        tsk_list_unlock(video->rscode_list);
    } while (0);
    

	if (video->video_status_map && !video->video_status_map->empty()){
		auto iter = video->video_status_map->find(sessionId);
		if (iter != video->video_status_map->end())
			video->video_status_map->erase(iter);
	}

    
    // stop consumer thread
    if (NULL != video->video_consumer_map && video->video_consumer_map->size() > 0) {
        TSK_DEBUG_INFO("start to stop consumer thread");
        //for (it = video->video_consumer_map->begin(); it != video->video_consumer_map->end(); it++) 
        {
            it = video->video_consumer_map->find(sessionId);
            if (it != video->video_consumer_map->end()) {
                TSK_DEBUG_INFO("find consumer thread sessionId:%d, stop it!", sessionId);
                video_decoded_inst = it->second;
                video->video_consumer_map->erase(it);
                
                if (video_decoded_inst && video_decoded_inst->decoder_frame_lst) {
                    tsk_list_clear_items(video_decoded_inst->decoder_frame_lst);
                    TSK_OBJECT_SAFE_FREE(video_decoded_inst->decoder_frame_lst);
                }

                if (video_decoded_inst) {
                    video_decoded_inst->consumer_thread_running = tsk_false;
                    // Increase the sema to avoid dead wait
                    if (tsk_semaphore_increment(video_decoded_inst->consumer_thread_sema)) {
                        // if send sem fail, try again
                        TSK_DEBUG_WARN("tdav_session_video_dtor send semaphore failed, try again!");
                        tsk_semaphore_increment(video_decoded_inst->consumer_thread_sema);
                    }

                    // Send signal to stop thread
                    //tsk_cond_set(video_decoded_inst->consumer_thread_cond);
                    
                    //#ifdef TSK_UNDER_WINDOWS
                    tsk_thread_join(&video_decoded_inst->consumer_thread_handle);
                    tsk_thread_destroy(&video_decoded_inst->consumer_thread_handle);
                    //tsk_cond_destroy(&video_decoded_inst->consumer_thread_cond);
                    
                    tsk_semaphore_destroy(&video_decoded_inst->consumer_thread_sema);
                    //#else
                    //tsk_thread_detach(&video_decoded_inst->consumer_thread_handle);
                    //#endif
                }

                TSK_SAFE_FREE(video_decoded_inst);
            }
        }
        TSK_DEBUG_INFO("end to stop consumer thread");
    }
    if (video->session_map) {
       video->session_map->erase(sessionId);
    }
    //tsk_safeobj_unlock(base);

    TSK_DEBUG_INFO("tdav_session_video_clear end");
    return ret;
}
static int tdav_session_video_pause(tmedia_session_t* self)
{
    return tdav_session_av_pause(TDAV_SESSION_AV(self));
}

static const tsdp_header_M_t* tdav_session_video_get_lo(tmedia_session_t* self)
{
    tsk_bool_t updated = tsk_false;
    const tsdp_header_M_t* ret;
    tdav_session_av_t* base = TDAV_SESSION_AV(self);
    
    if(!(ret = tdav_session_av_get_lo(base, &updated))){
        TSK_DEBUG_ERROR("tdav_session_av_get_lo(video) failed");
        return tsk_null;
    }
    
    if(updated){
        // set callbacks
        _tdav_session_video_set_callbacks(self);
    }
    
    return ret;
}

static int tdav_session_video_set_ro(tmedia_session_t* self, const tsdp_header_M_t* m)
{
    int ret;
    tsk_bool_t updated = tsk_false;
    tdav_session_av_t* base = TDAV_SESSION_AV(self);
    
    if((ret = tdav_session_av_set_ro(base, m, &updated))){
        TSK_DEBUG_ERROR("tdav_session_av_set_ro(video) failed");
        return ret;
    }
    
    // Check if "RTCP-NACK" and "RTC-FIR" are supported
    {
        const tmedia_codec_t* codec;
        base->is_fb_fir_neg = base->is_fb_nack_neg = base->is_fb_googremb_neg = tsk_false;
        if ((codec = tdav_session_av_get_best_neg_codec(base))) {
            // a=rtcp-fb:* ccm fir
            // a=rtcp-fb:* nack
            // a=rtcp-fb:* goog-remb
            char attr_fir[256], attr_nack[256], attr_goog_remb[256];
            int index = 0;
            const tsdp_header_A_t* A;
            
            sprintf(attr_fir, "%s ccm fir", codec->neg_format);
            sprintf(attr_nack, "%s nack", codec->neg_format);
            sprintf(attr_goog_remb, "%s goog-remb", codec->neg_format);
            
            while ((A = tsdp_header_M_findA_at(m, "rtcp-fb", index++))) {
                if (!base->is_fb_fir_neg) {
                    base->is_fb_fir_neg = (tsk_striequals(A->value, "* ccm fir") || tsk_striequals(A->value, attr_fir));
                }
                if (!base->is_fb_nack_neg) {
                    base->is_fb_nack_neg = (tsk_striequals(A->value, "* nack") || tsk_striequals(A->value, attr_nack));
                }
                if (!base->is_fb_googremb_neg) {
                    base->is_fb_googremb_neg = (tsk_striequals(A->value, "* goog-remb") || tsk_striequals(A->value, attr_goog_remb));
                }
            }
            base->is_fb_fir_neg = true;
        }
    }
    
    if (updated) {
        // set callbacks
        ret = _tdav_session_video_set_callbacks(self);
    }
    
    return ret;
}

// Plugin interface: callback from the end-user to set rtcp event callback (should be called only when encoding is bypassed)
static int tdav_session_video_rtcp_set_onevent_cbfn(tmedia_session_t* self, const void* context, tmedia_session_rtcp_onevent_cb_f func)
{
    tdav_session_video_t* video;
    tdav_session_av_t* base;
    
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    video = (tdav_session_video_t*)self;
    base = (tdav_session_av_t*)self;
    
    tsk_safeobj_lock(base);
    video->cb_rtcpevent.context = context;
    video->cb_rtcpevent.func = func;
    tsk_safeobj_unlock(base);
    
    return 0;
}

// Plugin interface: called by the end-user to send rtcp event (should be called only when encoding is bypassed)
static int tdav_session_video_rtcp_send_event(tmedia_session_t* self, tmedia_rtcp_event_type_t event_type, uint32_t ssrc_media)
{
    tdav_session_video_t* video;
    tdav_session_av_t* base;
    int ret = -1;
    
    if(!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    base = (tdav_session_av_t*)self;
    
    tsk_safeobj_lock(base);
    switch(event_type){
        case tmedia_rtcp_event_type_fir:
        {
            if(base->rtp_manager && base->rtp_manager->is_started){
                if(!ssrc_media){ // when called from C++/Java/C# bindings "ssrc_media" is a default parameter with value=0
                    ssrc_media = base->rtp_manager->rtp.ssrc.remote;
                }
                TSK_DEBUG_INFO("Send FIR(%u)", ssrc_media);
                _tdav_session_video_local_request_idr(self, "CALLBACK", ssrc_media);
            }
            break;
        }
    }
    tsk_safeobj_unlock(base);
    
    return ret;
}

// Plugin interface: called by the end-user to recv rtcp event
static int tdav_session_video_rtcp_recv_event(tmedia_session_t* self, tmedia_rtcp_event_type_t event_type, uint32_t ssrc_media)
{
    tdav_session_video_t* video;
    tdav_session_av_t* base;
    int ret = -1;
    
    if (!self){
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    video = (tdav_session_video_t*)self;
    base = (tdav_session_av_t*)self;
    
    tsk_safeobj_lock(base);
    switch(event_type){
        case tmedia_rtcp_event_type_fir:
        {
            _tdav_session_video_remote_requested_idr(video, ssrc_media);
            ret = 0;
            break;
        }
    }
    tsk_safeobj_unlock(base);
    
    return ret;
}

static int _tdav_session_video_set_callbacks(tmedia_session_t* self)
{
    if(self){
        tsk_list_item_t* item;
        tsk_list_foreach(item, TMEDIA_SESSION(self)->neg_codecs){
            // set codec callbacks
            tmedia_codec_video_set_enc_callback(TMEDIA_CODEC_VIDEO(item->data), tdav_session_video_raw_cb, self);
            tmedia_codec_video_set_dec_callback(TMEDIA_CODEC_VIDEO(item->data), tdav_session_video_decode_cb, self);
            /*
             // video RED功能暂时不需要
             // set RED callback: redundant data to decode and send to the consumer
             if(TMEDIA_CODEC(item->data)->plugin == tdav_codec_red_plugin_def_t){
             tdav_codec_red_set_callback((struct tdav_codec_red_s *)(item->data), tdav_session_video_rtp_cb, self);
             }
             */
        }
    }
    return 0;
}

// 定时从rtp/rtcp中获取qos，暂存便于上层提取上报
static int tdav_codec_avqos_stat_timer_callback_f (const void *arg, long timerid) {
    tdav_session_av_t* base = (tdav_session_av_t*)arg;
    tdav_session_video_t *self = (tdav_session_video_t *)arg;
    if (!self || !base) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    int ret = 0;
    // 半程丢包统计
    trtp_send_report_stat_t report_stat;
    if (base->rtp_manager) {
        ret = trtp_manager_get_report_stat(base->rtp_manager, &report_stat);
        if (!ret) {
            self->avQosStat.audioUpLossCnt = report_stat.audio_send_loss_count;
            self->avQosStat.audioUpTotalCnt = report_stat.audio_send_total_count;

            self->avQosStat.videoUpLossCnt = report_stat.video_send_loss_count;
            self->avQosStat.videoUpTotalCnt = report_stat.video_send_total_count;
        }
    }
    
    // 全程统计 && 上下行带宽统计
    struct trtp_rtcp_stat_qos_s stat_qos;
    if (base->rtp_manager && base->rtp_manager->rtcp.session) {
        ret = trtp_rtcp_session_get_session_qos(base->rtp_manager->rtcp.session, &stat_qos);
        if (!ret) {
            self->avQosStat.audioDnLossrate = (self->avQosStat.audioDnLossrate < stat_qos.a_down_fraction) \
                                                ? stat_qos.a_down_fraction : self->avQosStat.audioDnLossrate;

            self->avQosStat.audioRtt = (self->avQosStat.audioRtt < stat_qos.rtt) \
                                    ? stat_qos.rtt : self->avQosStat.audioRtt;

            self->avQosStat.videoDnLossrate = (self->avQosStat.videoDnLossrate < stat_qos.v_down_fraction) \
                                    ? stat_qos.v_down_fraction : self->avQosStat.videoDnLossrate;

            self->avQosStat.videoRtt = self->avQosStat.audioRtt;

            self->avQosStat.auidoUpbitrate = (self->avQosStat.auidoUpbitrate < stat_qos.a_up_bitrate) \
                                    ? stat_qos.a_up_bitrate : self->avQosStat.auidoUpbitrate;

            self->avQosStat.auidoDnbitrate = (self->avQosStat.auidoDnbitrate < stat_qos.a_down_bitrate) \
                                    ? stat_qos.a_down_bitrate : self->avQosStat.auidoDnbitrate;
        
            self->avQosStat.videoUpCamerabitrate = (self->avQosStat.videoUpCamerabitrate < stat_qos.v_up_camera_bitrate) \
                                    ? stat_qos.v_up_camera_bitrate : self->avQosStat.videoUpCamerabitrate;
            self->avQosStat.videoUpSharebitrate = (self->avQosStat.videoUpSharebitrate < stat_qos.v_up_share_bitrate) \
                                    ? stat_qos.v_up_share_bitrate : self->avQosStat.videoUpSharebitrate;

            self->avQosStat.videoDnCamerabitrate = (self->avQosStat.videoDnCamerabitrate < stat_qos.v_down_bitrate) \
                                    ? stat_qos.v_down_bitrate : self->avQosStat.videoDnCamerabitrate;
            self->avQosStat.videoDnSharebitrate = (self->avQosStat.videoDnSharebitrate < stat_qos.v_down_share_bitrate) \
                                    ? stat_qos.v_down_share_bitrate : self->avQosStat.videoDnSharebitrate;

            self->avQosStat.videoUpStream = stat_qos.v_up_stream;
        }
    }

    return 0;
}

static int tdav_codec_check_timer_callback_f (const void *arg, long timerid) {
    tdav_session_video_t *p_self = (tdav_session_video_t *)arg;
    static uint32_t refresh_counter = 0;
    if (!p_self || !p_self->last_frame_buf) {
        // TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    uint64_t now_time = tsk_gettimeofday_ms();
    int64_t diff_time = now_time - p_self->last_check_timestamp;
    if(Config_GetInt("STOP_SHARE_INPUT", 0) != 0)
    {
        if (p_self->last_frame_buf) {
            TSK_FREE(p_self->last_frame_buf);
            p_self->last_frame_buf = tsk_null;
        }
        return 0;
    }

    if (diff_time >= p_self->refresh_timeout && ++refresh_counter % p_self->refresh_limit == 0 ) {
        refresh_counter = 0;
        uint64_t timestamp = p_self->last_frame_timestamp + diff_time;
        TSK_DEBUG_INFO("chek frame send last frame to remote, timeout:%lld ts:%llu, size:%d", diff_time, timestamp, p_self->last_frame_size);
        tdav_session_video_producer_enc_cb_new(p_self, p_self->last_frame_buf, p_self->last_frame_size, timestamp, 2, p_self->last_frame_fmt, tsk_true);
    }

    return 0;
}

static int _tdav_session_video_init(tdav_session_video_t *p_self, tmedia_type_t e_media_type)
{
    int ret;
    tdav_session_av_t *p_base = TDAV_SESSION_AV(p_self);
    if (!p_self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    /* init() base */
    if ((ret = tdav_session_av_init(p_base, e_media_type)) != 0) {
        TSK_DEBUG_ERROR("tdav_session_av_init(video) failed");
        return ret;
    }
    
    /* init() self */
    _tdav_session_video_set_defaults(p_self);
    if (!p_self->encoder.h_mutex && !(p_self->encoder.h_mutex = tsk_mutex_create())) {
        TSK_DEBUG_ERROR("Failed to create encode mutex");
        return -4;
    }
    if (!p_self->avpf.packets && !(p_self->avpf.packets = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create list");
        return -2;
    }

    // nack 开关
    p_self->last_nack_flag = tmedia_get_video_nack_flag() & tdav_session_video_nack_half;

    if (p_self->jb_enabled) {
        if (!p_self->jb && !(p_self->jb = tdav_video_jb_create())) {
            TSK_DEBUG_ERROR("Failed to create jitter buffer");
            return -3;
        }
        tdav_video_jb_set_callback(p_self->jb, _tdav_session_video_jb_cb, p_self);

        // 注册nack回调
        tdav_video_jb_set_video_nack_callback(p_self->jb, _tdav_sesiion_video_nack_cb, p_self);
    }
    
    if (p_base->producer) {
        tmedia_producer_set_enc_callback(p_base->producer, tdav_session_video_producer_enc_cb, p_self);
        tmedia_producer_set_raw_callback(p_base->producer, tdav_session_video_raw_cb, p_self);
        tmedia_producer_set_enc_callback_new(p_base->producer, tdav_session_video_producer_enc_cb_new, p_self);
    }
    
    p_self->encoder.isFirstFrame = tsk_true;
    
    if(!(p_self->rscode_list = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create rscode list");
        return -5;
    }

	p_self->video_status_map = new video_status_map_t;
    
    p_self->video_consumer_map = new decoder_consumer_map_t;
    p_self->video_consumer_map->clear();
    
    p_self->session_map = new std::set<int>;
    
    
    p_self->stat.time_base_ms = tsk_gettimeofday_ms();
    p_self->stat.send_timestamp_period_ms = tmedia_defaults_get_video_packet_stat_report_period_ms();
    
    if (!p_self->h_mutex_qos && !(p_self->h_mutex_qos = tsk_mutex_create())) {
        TSK_DEBUG_ERROR("Failed to create qos mutex");
        return -6;
    }

    p_self->lastUpAudioLoss_max = -1;
    p_self->lastDnAudioLoss_max = -1;

    // start avQosStat timer, shedule period: 1000ms
    p_self->avQosStatTimer = xt_timer_mgr_global_schedule_loop(TDAV_VIDEO_QOS_STAT_CHECK_TIMEOUT, tdav_codec_avqos_stat_timer_callback_f, p_self);

#if ANDROID
    // start frame check timer, shedule period: 100ms
    p_self->refresh_timeout = tmedia_get_max_video_refresh_timeout(); //no data input check timeout
    p_self->refresh_limit = Config_GetInt("VIDEO_FRAME_LIMIT", 2);
    p_self->refresh_copy_interval = Config_GetInt("SHARE_COPY_INTERVAL", 50);
    if(p_self->refresh_timeout > 0) {
        p_self->check_timer_id = xt_timer_mgr_global_schedule_loop(TDAV_VIDEO_FRAME_CHECK_TIMEOUT, tdav_codec_check_timer_callback_f, p_self);
    }
    
    p_self->last_check_timestamp = tsk_gettimeofday_ms();
    p_self->last_frame_size = 0;
#endif

    return 0;
}

tdav_rscode_t* tdav_session_video_select_rscode_by_sessionid (tdav_session_video_t* self, int session_id,int iVideoID, RscType type) {
    tsk_list_item_t *item = tsk_null;
    tdav_rscode_t *rscode = tsk_null;
    
    if(!self->rscode_list) {
        TSK_DEBUG_ERROR("*** rscode list is null ***");
        return tsk_null;
    }
    
    tsk_list_lock(self->rscode_list);
    tsk_list_foreach(item, self->rscode_list) {
        if((TDAV_RSCODE(item->data)->session_id == session_id) && (TDAV_RSCODE(item->data)->rs_type == type) && (TDAV_RSCODE(item->data)->iVideoID == iVideoID)) {
            rscode = TDAV_RSCODE(item->data);
            break;
        }
    }
    tsk_list_unlock(self->rscode_list);
    
    return rscode;
}

int tdav_session_video_stop_rscode (tdav_session_video_t* self) {
    tsk_list_item_t *item = tsk_null;
    
    if(!self->rscode_list) {
        TSK_DEBUG_ERROR("*** rscode list is null ***");
        return -1;
    }
    
    tsk_list_lock(self->rscode_list);
    tsk_list_foreach(item, self->rscode_list) {
        tdav_rscode_stop(TDAV_RSCODE(item->data));
    }
    tsk_list_unlock(self->rscode_list);
    
    return 0;
}

//=================================================================================================
//	Session Video Plugin object definition
//
/* constructor */
static tsk_object_t* tdav_session_video_ctor(tsk_object_t * self, va_list * app)
{
    tdav_session_video_t *video = (tdav_session_video_t*)self;
    if(video){
        if (_tdav_session_video_init(video, tmedia_video)) {
            return tsk_null;
        }
    }
    return self;
}
/* destructor */
static tsk_object_t* tdav_session_video_dtor(tsk_object_t * self)
{ 
    tdav_session_video_t *video = (tdav_session_video_t*)self;
    decoder_consumer_map_t::iterator it;
    decoder_consumer_manager_t* video_decoded_inst;
    
    TSK_DEBUG_INFO("*** tdav_session_video_t destroyed ***");
    if(video){
        tdav_session_video_stop((tmedia_session_t*)video);
        // deinit self (rtp manager should be destroyed after the producer)
        TSK_OBJECT_SAFE_FREE(video->conv.toYUV420);
        TSK_OBJECT_SAFE_FREE(video->conv.fromYUV420);
        
        TSK_FREE(video->encoder.buffer);
        TSK_FREE(video->encoder.conv_buffer);
        TSK_FREE(video->decoder.buffer);
        TSK_FREE(video->decoder.conv_buffer);
        TSK_FREE(video->encoder.rtpExtBuffer);
        
        TSK_OBJECT_SAFE_FREE(video->encoder.codec);
        TSK_OBJECT_SAFE_FREE(video->decoder.codec);
        
        
        
        TSK_OBJECT_SAFE_FREE(video->avpf.packets);
        
        TSK_OBJECT_SAFE_FREE(video->jb);
        
        TSK_OBJECT_SAFE_FREE(video->rscode_list);
        
        // Must close videoMonitorThread first
        tsk_cond_set(video->videoMonitorThreadCond);
//        pthread_mutex_lock(&video->videoMonitorThreadMutex);
//        pthread_cond_signal(&video->videoMonitorThreadCond);
//        pthread_mutex_unlock(&video->videoMonitorThreadMutex);
        video->videoMonitorThreadStart = tsk_false;
        tsk_thread_join(&video->videoMonitorThread);
        tsk_thread_destroy(&video->videoMonitorThread);
        tsk_cond_destroy(&video->videoMonitorThreadCond);
//        pthread_join(video->videoMonitorThread, NULL);
//        pthread_cond_destroy(&video->videoMonitorThreadCond);
//        pthread_mutex_destroy(&video->videoMonitorThreadMutex);

        if (NULL != video->video_consumer_map) {
            for (it = video->video_consumer_map->begin(); it != video->video_consumer_map->end(); it++) {
                video_decoded_inst = it->second;
                if (video_decoded_inst && video_decoded_inst->decoder_frame_lst) {
                    tsk_list_clear_items(video_decoded_inst->decoder_frame_lst);
                    TSK_OBJECT_SAFE_FREE(video_decoded_inst->decoder_frame_lst);
                }
                if (video_decoded_inst) {
                    video_decoded_inst->consumer_thread_running = tsk_false;
                    // Increase the sema to avoid dead wait
                    if (tsk_semaphore_increment(video_decoded_inst->consumer_thread_sema)) {
                        // if send sem fail, try again
                        TSK_DEBUG_WARN("tdav_session_video_dtor send semaphore failed, try again!");
                        tsk_semaphore_increment(video_decoded_inst->consumer_thread_sema);
                    }

                    // Send signal to stop thread
                    //tsk_cond_set(video_decoded_inst->consumer_thread_cond);
                    
                    //#ifdef TSK_UNDER_WINDOWS
                    tsk_thread_join(&video_decoded_inst->consumer_thread_handle);
                    tsk_thread_destroy(&video_decoded_inst->consumer_thread_handle);
                    //tsk_cond_destroy(&video_decoded_inst->consumer_thread_cond);
                    
                    tsk_semaphore_destroy(&video_decoded_inst->consumer_thread_sema);
                    //#else
                    //tsk_thread_detach(&video_decoded_inst->consumer_thread_handle);
                    //#endif
                }

                //#ifdef TSK_UNDER_WINDOWS
                TSK_SAFE_FREE(video_decoded_inst);
                //#endif
            }
            video->video_consumer_map->clear();
            delete video->video_consumer_map;
            video->video_consumer_map = NULL;
        }

		if (NULL != video->video_status_map){
			delete video->video_status_map;
			video->video_status_map = NULL;
		}
        
        if (video->session_map) {
            delete video->session_map;
            video->session_map = NULL;
        }
        
        if(video->encoder.h_mutex){
            tsk_mutex_destroy(&(video->encoder.h_mutex));
            video->encoder.h_mutex = NULL;
        }
        
        if (video->h_mutex_qos) {
            tsk_mutex_destroy(&video->h_mutex_qos);
        }
        
        /* deinit() base */
        tdav_session_av_deinit(TDAV_SESSION_AV(video));
        
        TSK_DEBUG_INFO("*** Video session destroyed ***");
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t tdav_session_video_def_s = 
{
    sizeof(tdav_session_video_t),
    tdav_session_video_ctor,
    tdav_session_video_dtor,
    tmedia_session_cmp,
};
/* plugin definition*/
static const tmedia_session_plugin_def_t tdav_session_video_plugin_def_s = 
{
    &tdav_session_video_def_s,
    
    tmedia_video,
    "video",
    
    tdav_session_video_set,
    tdav_session_video_get,
    tdav_session_video_prepare,
    tdav_session_video_start,
    tdav_session_video_pause,
    tdav_session_video_stop,
    tdav_session_video_clear,

    /* Audio part */
    { tsk_null },
    
    tdav_session_video_get_lo,
    tdav_session_video_set_ro,
    
    /* T.140 */
    { tsk_null },
    
    // RTCP功能暂时不需要
    /*
     {
     tdav_session_video_rtcp_set_onevent_cbfn,
     tdav_session_video_rtcp_send_event,
     tdav_session_video_rtcp_recv_event
     }
     */
};
const tmedia_session_plugin_def_t *tdav_session_video_plugin_def_t = &tdav_session_video_plugin_def_s;

//=================================================================================================
//	Session BfcpVideo Plugin object definition
//
/* constructor */
static tsk_object_t* tdav_session_bfcpvideo_ctor(tsk_object_t * self, va_list * app)
{
    tdav_session_video_t *video = (tdav_session_video_t*)self;
    if(video){
        if (_tdav_session_video_init(video, tmedia_bfcp_video)) {
            return tsk_null;
        }
    }
    return self;
}
/* object definition */
static const tsk_object_def_t tdav_session_bfcpvideo_def_s = 
{
    sizeof(tdav_session_video_t),
    tdav_session_bfcpvideo_ctor,
    tdav_session_video_dtor,
    tmedia_session_cmp,
};
static const tmedia_session_plugin_def_t tdav_session_bfcpvideo_plugin_def_s = 
{
    &tdav_session_bfcpvideo_def_s,
    
    tmedia_bfcp_video,
    "video",
    
    tdav_session_video_set,
    tdav_session_video_get,
    tdav_session_video_prepare,
    tdav_session_video_start,
    tdav_session_video_pause,
    tdav_session_video_stop,
    tsk_null,

    /* Audio part */
    { tsk_null },
    
    tdav_session_video_get_lo,
    tdav_session_video_set_ro,
    
    /* T.140 */
    { tsk_null },
    
    // RTCP功能暂时不需要
    /*
     {
     tdav_session_video_rtcp_set_onevent_cbfn,
     tdav_session_video_rtcp_send_event,
     tdav_session_video_rtcp_recv_event
     }
     */
};
const tmedia_session_plugin_def_t *tdav_session_bfcpvideo_plugin_def_t = &tdav_session_bfcpvideo_plugin_def_s;

//=================================================================================================
// video decoded frame object definition
//
static tsk_object_t *tdav_session_video_decoded_frame_ctor (tsk_object_t *self, va_list *app)
{
    decoder_frame_t *frame = (decoder_frame_t *)self;
    uint32_t frame_size = va_arg(*app, uint32_t);
    if (frame)
    {
        if (frame_size > 0) {
            frame->_buffer  = tsk_calloc(frame_size, 1);
            frame->_size    = frame_size;
            frame->_rtp_hdr = (trtp_rtp_header_t *)tsk_calloc(sizeof(trtp_rtp_header_t), 1);
        }
    }
    return self;
}

static tsk_object_t *tdav_session_video_decoded_frame_dtor (tsk_object_t *self)
{
    decoder_frame_t *frame = (decoder_frame_t *)self;
    if (frame)
    {
        if ((frame->_size > 0) && (frame->_buffer)) {
            tsk_free(&frame->_buffer);
        }
        if (frame->_rtp_hdr) {
            tsk_free((void**)&frame->_rtp_hdr);
        }
    }
    return self;
}

static int tdav_session_video_decoded_frame_cmp (const tsk_object_t *_e1, const tsk_object_t *_e2)
{
    int ret;
    tsk_subsat_int32_ptr (_e1, _e2, &ret);
    return ret;
}

static const tsk_object_def_t tdav_session_video_decoded_frame_def_s = {
    sizeof (decoder_frame_t),
    tdav_session_video_decoded_frame_ctor,
    tdav_session_video_decoded_frame_dtor,
    tdav_session_video_decoded_frame_cmp
};
const tsk_object_def_t *tdav_session_video_decoded_frame_def_t = &tdav_session_video_decoded_frame_def_s;



//硬编解码回调处理
/*================================= hw code callback =================================*/
static void tdav_webrtc_decode_h264_callback(const unsigned char * buff, int bufflen, int64_t timestamp, const void* session, void* header, tsk_bool_t isTexture, int video_id)
{
    _tdav_session_video_decode_cb(buff, bufflen, timestamp, (tdav_session_video_t*)session, (trtp_rtp_header_t*)header, isTexture, video_id);
}

static void tdav_webrtc_encode_h264_callback(const unsigned char * buff, int bufflen, int64_t timestamp, int video_id, const void* session)
{
    tdav_session_video_encoder_cb(buff, bufflen, timestamp, video_id, session);
}

//视频检测
static  tdav_session_video_check_unusual_t tdav_session_video_yuv_check(uint8_t* yuv_data, int width, int height)
{
    if(!yuv_data || width<16 || height<16)
        return tdav_session_video_data_error;
    
    const int border_size = 6;
    uint64_t total_y = 0;
    uint64_t total_uv = 0;
    uint8_t * y = yuv_data;
    uint8_t * u = y + width*height;
    uint8_t * v = u + width*height/4;
    
    int size = height*width/4;
    for (int i = 0; i < size && !total_y; ++i) {
        total_y += *y++;
        total_y += *y++;
        total_y += *y++;
        total_y += *y++;
        total_uv += *u++ + *v++;
    }
    
    if(total_y == 0){
        if (total_uv == 0)
            return tdav_session_video_green_full;
        else if(total_uv == 0x80*2*size)
            return tdav_session_video_black_full;
        
    }
    
    y = yuv_data;
    u = y + width*height;
    v = u + width*height/4;
    uint64_t total_left_y = 0;
    uint64_t total_left_uv = 0;
    uint64_t total_right_y = 0;
    uint64_t total_right_uv = 0;
    int stride_y = width;
    int stride_uv = width/2;
    int i,b;
    for (i = 0; i < height/2 && (!total_left_y || !total_right_y); ++i) {
        
        for (b = 0; b < border_size/2; ++b) {
            total_left_y += *(y + stride_y*i + b*2) ;
            total_left_y += *(y + stride_y*i + b*2+1);
            total_right_y += *(y + stride_y*i + (stride_y - b*2 - 1));
            total_right_y += *(y + stride_y*i + (stride_y - b*2 - 2));
            total_left_uv += *(u + stride_uv*i + b) + *(v + stride_uv*i + b) ;
            total_right_uv += *(u + stride_uv*i + stride_uv - b - 1) + *(v + stride_uv*i + b + stride_uv - b - 1) ;
        }
    }
    if (total_left_y + total_left_uv == 0 || total_right_y + total_right_uv== 0) {
        return tdav_session_video_green_border;
    }
    if ((total_left_y == 0 && total_left_uv == 0x80*height/2*border_size/2) ||
        (total_right_y == 0 &&  total_right_uv == 0x80*height/2*border_size/2)) {
        return tdav_session_video_black_border;
    }
    
    return tdav_session_video_normal;
}

static tsk_bool_t tdav_session_save_file(uint8_t* data, uint32_t data_len)
{
    if (!data || !data_len) {
        return tsk_false;
    }
    const char* filename = "";
    FILE * pf = fopen(filename, "wb");
    if (pf) {
        fwrite(data, data_len, 1, pf);
        fclose(pf);
        return tsk_true;
    }
    return tsk_false;
}

//////////////////extension///////////////////////////////////////////////////////////////////////////////////
// Return the exact size of the encoded rtp header extension.
static tsk_size_t tdav_session_video_encode_rtp_header_ext(tdav_session_video_t *video, void* payload, tsk_size_t payload_size)
{
    tsk_size_t out_size = 0;
    VideoRtpHeaderExt_t rtpHeaderExt = { 0 };
    tsk_bool_t bIsExistExtension = tsk_false;
    uint64_t curTimeMs = tsk_gettimeofday_ms();
    
    if (!video || !video->encoder.codec) {
        return 0;
    }
    
    // Init the non-zero and non-null fields
    
    // Send my local timestamp with the RTP packet. The receiver will bounce it back, and I will
    // know the time cost of the round trip.
    
    if (video->stat.may_send_timestamp
        && (video->stat.send_timestamp_period_ms > 0)
        && ((curTimeMs - video->stat.last_send_timestamp_time_ms) >= video->stat.send_timestamp_period_ms) ) {
        
        rtpHeaderExt.hasTimestampOriginal = 1;
        rtpHeaderExt.timestampOriginal = (uint32_t)(tsk_gettimeofday_ms() - video->stat.time_base_ms);
        bIsExistExtension = tsk_true;
       
        //todopinky,先保证每次都发
        video->stat.last_send_timestamp_time_ms = curTimeMs;
        video->stat.may_send_timestamp = tsk_false;
        //TSK_DEBUG_INFO(">>>> sending timestamp:%u", rtpHeaderExt.timestampOriginal);
    }
    
    // Bounce the timestamp back to the sender.
    // Check first if there's data to be bounced without locking, so that we don't need to lock for
    // every packet even there's no data to be bounced.
    if (video->stat.timestamp_num > 0) {
        tsk_safeobj_lock(&(video->stat));
        // check again after locking
        if (video->stat.timestamp_num > 0) {
            int32_t offset = 0;
            int32_t i;
            for (i = 0; i < video->stat.timestamp_num; i++) {
                // duration between receiving and sending the timestamp
                uint32_t timestampDeltaMs = curTimeMs - video->stat.timestamps[i].timeWhenRecvMs;
                //TSK_DEBUG_INFO(">>>> bounce session:%d, timestamp:%u, timestampDeltaMs:%u", audio->stat.timestamps[i].sessionId, audio->stat.timestamps[i].timestampMs, timestampDeltaMs);
                
                // Encode the bounced timestamps
                *(int32_t*)&video->stat.timestampBouncedBuf[offset] = htonl(video->stat.timestamps[i].sessionId);
                offset += sizeof(int32_t);
                *(uint32_t*)&video->stat.timestampBouncedBuf[offset] = htonl(video->stat.timestamps[i].timestampMs + timestampDeltaMs);
                offset += sizeof(uint32_t);
                bIsExistExtension = tsk_true;
            }
            rtpHeaderExt.pTimestampBouncedData = video->stat.timestampBouncedBuf;
            rtpHeaderExt.timestampBouncedSize = video->stat.timestamp_num * BOUNCED_TIMESTAMP_INFO_SIZE;
            video->stat.timestamp_num = 0;
            bIsExistExtension = tsk_true;
        }
        tsk_safeobj_unlock(&(video->stat));
    }
    // Finally, encode the rtp header extension if necessay.
    if (bIsExistExtension) {
        out_size = tdav_codec_video_rtp_extension_encode(video, &rtpHeaderExt, &video->encoder.rtpExtBuffer, &video->encoder.rtpExtBufferSize);
    }
    
    return out_size;
}

// Decode the header extension, which may contains the bandwidth control data, FEC data, VAD result, etc.
static int tdav_session_video_decode_rtp_header_ext(tdav_session_video_t *video, const struct trtp_rtp_packet_s *packet)
{
    int ret = -1;
    VideoRtpHeaderExt_t rtpHeaderExt;
    int i;
    
    if (!video || !packet) {
        return ret;
    }
    
    // Since we received a packet, we may send a timestamp(for calculating network delay) in the next pakcet.
    video->stat.may_send_timestamp = tsk_true;
    
    if (packet->header->extension && (packet->extension.data || packet->extension.data_const) && (packet->extension.size > 0)) {
        uint8_t* ext_data = (uint8_t*)(packet->extension.data_const ? packet->extension.data_const : packet->extension.data);
        ret = tdav_codec_video_rtp_extension_decode(video, ext_data, packet->extension.size, &rtpHeaderExt);
    }
    
    // If no header extension, or failed to decode the header extension, just return.
    if (0 != ret) {
        return ret;
    }
    
    // If there's a timestamp from the sender, save it and bounce it back with the next packet.
    if (rtpHeaderExt.hasTimestampOriginal) {
        //TSK_DEBUG_INFO("@@>>  got original sessionid:%d, timestamp:%u", packet->header->session_id, rtpHeaderExt.timestampOriginal);
        
        tsk_safeobj_lock(&(video->stat));
        // Check if a timestamp from the same sessionID is waiting for being bounced back.
        // If so, just replace it with the new one.
        for (i = 0; i < video->stat.timestamp_num; i++) {
            if (video->stat.timestamps[i].sessionId == packet->header->session_id) {
                break;
            }
        }
        
        if (i < video->stat.timestamp_num) {
            video->stat.timestamps[i].timeWhenRecvMs = packet->header->receive_timestamp_ms;
            video->stat.timestamps[i].timestampMs = rtpHeaderExt.timestampOriginal;
        } else if (video->stat.timestamp_num < MAX_TIMESTAMP_NUM) {
            video->stat.timestamps[video->stat.timestamp_num].timeWhenRecvMs = packet->header->receive_timestamp_ms;
            video->stat.timestamps[video->stat.timestamp_num].sessionId = packet->header->session_id;
            video->stat.timestamps[video->stat.timestamp_num].timestampMs = rtpHeaderExt.timestampOriginal;
            video->stat.timestamp_num++;
        }
        tsk_safeobj_unlock(&(video->stat));
    }
    
    packet->header->networkDelayMs = 0;
    if ((rtpHeaderExt.timestampBouncedSize > 0) && rtpHeaderExt.pTimestampBouncedData) {
        uint32_t timestampBouncedNum = rtpHeaderExt.timestampBouncedSize / (sizeof(int32_t) + sizeof(uint32_t));
        int32_t  sessionIdBounced = 0;
        uint32_t timestampBounced = 0;
        uint32_t curRelTimeMs = (uint32_t)(tsk_gettimeofday_ms() - video->stat.time_base_ms);
        uint32_t offset = 0;
        
        for (i = 0; i < timestampBouncedNum; i++) {
            sessionIdBounced = ntohl(*(int32_t*)&rtpHeaderExt.pTimestampBouncedData[offset]);
            offset += sizeof(int32_t);
            timestampBounced = ntohl(*(uint32_t*)&rtpHeaderExt.pTimestampBouncedData[offset]);
            offset += sizeof(uint32_t);
            //TSK_DEBUG_INFO("@@>>  got bounced sessionid:%d, region:%u, timestamp:%u", sessionIdBounced, regionCodeBounced, timestampBounced);
            // If this timestamp is for me, we can stop now
            if (sessionIdBounced == packet->header->my_session_id) {
                break;
            }
        }
        
        if (i < timestampBouncedNum) {
            packet->header->networkDelayMs = (curRelTimeMs - timestampBounced) / 2;
            
            AddVideoPacketDelay( packet->header->networkDelayMs, packet->header->session_id );
        }
    }
    
    return 0;
}





/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
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
/**@file trtp_rtcp_session.c
 * @brief RTCP session.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *

 */
#include "tinyrtp/rtcp/trtp_rtcp_session.h"
#include "tinyrtp/rtcp/trtp_rtcp_packet.h"
#include "tinyrtp/rtcp/trtp_rtcp_header.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_rr.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_sr.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_sdes.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_bye.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_fb.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"

// #include "ice/tnet_ice_ctx.h"
//#include "turn/tnet_turn_session.h"
#include "tnet_transport.h"

#include "tnet_utils.h"

#include "tsk_string.h"
#include "tsk_md5.h"
#include "tsk_list.h"
#include "tsk_time.h"
#include "XTimerCWrapper.h"
#include "tsk_safeobj.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tmedia_utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> /* INT_MAX */

#include "tinymedia/tmedia_defaults.h"
#include "XConfigCWrapper.hpp"

#ifdef _MSC_VER
static double drand48()
{
    return (((double)rand()) / RAND_MAX);
}
static void srand48(long sv)
{
    srand((unsigned int) sv);
}
#endif

#define RTCP_BW			(160 * 50) // FIXME: default bandwidth (octet/second)
#define AUDIO_CODEC_RATE		48000		// FIXME
#define VIDEO_CODEC_RATE        90000
#define RTP_SEQ_MOD		(1 << 16)
#define MAX_DROPOUT		3000
#define MAX_MISORDER	100
#define MIN_SEQUENTIAL	2
#define RTCP_NUMBER_OF_SR 60

#define DEF_MAX_REORDERING_THRESHOLD 50

typedef double time_tp;
typedef void* packet_;

typedef enum event_ {
    EVENT_BYE,
    EVENT_REPORT,
    EVENT_RTP
}
event_;

typedef enum PacketType_ {
    PACKET_RTCP_REPORT,
    PACKET_BYE,
    PACKET_RTP,
}
PacketType_;

#define TypeOfEvent(e)	(e)

typedef enum PacketMediaType_ {
    PACKET_MEIDA_TYPE_UNKNOW = 0,
    PACKET_MEIDA_TYPE_AUDIO,
    PACKET_MEIDA_TYPE_VIDEO,
}
PacketMediaType_;

#define TRTP_RTCP_SOURCE(self)	((trtp_rtcp_source_t*)self)

typedef struct trtp_rtcp_source_s {
    TSK_DECLARE_OBJECT;

    uint32_t ssrc;			        /* source's ssrc */
    int video_id;              /* source video id: main or minor */
    int last_video_id;         /* last source video id */

    // audio
    uint16_t audio_max_seq;         /* highest seq. number seen */
    uint32_t audio_cycles;          /* shifted count of seq. number cycles */
    uint32_t audio_base_seq;        /* base seq number */
    uint32_t audio_bad_seq;         /* last 'bad' seq number + 1 */
    uint32_t audio_probation;       /* sequ. packets till source is valid */
    uint32_t audio_received;        /* packets received */
    uint32_t audio_expected_prior;  /* packet expected at last interval */
    uint32_t audio_received_prior;  /* packet received at last interval */
    uint32_t audio_jitter_q4;            /* audio estimated jitter */
    uint32_t audio_max_jitter_q4;        /* aidio estimated max jitter*/
    uint16_t audio_received_max_seq;        /* the maximum sequence number of the received consecutive audio packets*/
    int64_t  audio_last_received_time_ms;   /*time of the last received audio packet*/
    int64_t  audio_last_received_timestamp; /*timestamp of the last received audio packet*/
    uint32_t audio_transmitted;            /*received packet num*/
    uint32_t audio_retransmitted;          /*received retransmitted packet num*/

    uint32_t audio_base_ts;	        /* base timestamp */
    uint32_t audio_max_ts;	        /* highest timestamp number seen */
    uint32_t audio_rate;		    /* codec sampling rate */

    // video
    uint16_t video_max_seq;         /* highest seq. number seen */
    uint32_t video_cycles;          /* shifted count of seq. number cycles */
    uint32_t video_base_seq;        /* base seq number */
    uint32_t video_bad_seq;         /* last 'bad' seq number + 1 */
    uint32_t video_probation;       /* sequ. packets till source is valid */
    uint32_t video_received;        /* packets received */
    uint32_t video_expected_prior;  /* packet expected at last interval */
    uint32_t video_received_prior;  /* packet received at last interval */
    uint16_t video_received_max_seq;/* the maximum sequence number of the received consecutive video packets*/
    int64_t  video_last_received_time_ms;/*time of the last received video packet*/
    int64_t  video_last_received_timestamp;/*timestamp of the last received video packet*/
    uint32_t video_transmitted;            /*received packet num*/
    uint32_t video_retransmitted;          /*received retransmitted packet num*/
    uint32_t video_jitter_q4;            /* estimated jitter */
    uint32_t video_max_jitter_q4;        /* video estimated max jitter*/

    uint32_t video_base_ts;         /* base timestamp */
    uint32_t video_max_ts;          /* highest timestamp number seen */
    uint32_t video_rate;            /* codec sampling rate */

    uint32_t ntp_msw;  /* last received NTP timestamp from RTCP sender */
    uint32_t ntp_lsw;  /* last received NTP timestamp from RTCP sender */
    uint64_t dlsr;    /* delay since last SR */
    
    //RTT statistics
    int64_t rtt;                    /* last rtt */
    int64_t min_rtt;
    int64_t max_rtt;
    int64_t avg_rtt;
    uint32_t num_average_calcs;

    // downlink ppl -- audio
    uint32_t audio_fraction;        /* audio last fraction */
    uint32_t audio_max_fraction;    /* audio max fraction */

    // downlink ppl -- video
    uint32_t video_fraction;        /* video last fraction */
    uint32_t video_max_fraction;    /* video max fraction */

    uint32_t video_source_type;         /* 0:main, 1:minor */
    uint32_t video_last_source_type;    /* 0:main, 1:minor */
    uint32_t video_main_stream_count;   /* sender send main stream pkt count */
    uint32_t video_minor_stream_count;  /* sender send minor stream pkt count */
    int32_t  video_last_lossrate;   

    uint32_t video_down_loss_cnt;    /* count of down loss > 10%*/

    // audio bitrate recv stat
    uint32_t audio_recv_stat_bytes;
    uint32_t audio_recv_instant_bitrate;
    int64_t  audio_recv_last_stat_timestamp;

    // video bitrate recv stat  
    uint32_t video_recv_stat_bytes;
    uint32_t video_recv_instant_bitrate;
    int64_t  video_recv_last_stat_timestamp;

    // video(share) bitrate recv stat
    uint32_t video_share_recv_stat_bytes;
    uint32_t video_share_recv_instant_bitrate;
    int64_t  video_share_recv_last_stat_timestamp;
}

trtp_rtcp_source_t;

typedef tsk_list_t trtp_rtcp_sources_L_t; /**< List of @ref trtp_rtcp_header_t elements */

static tsk_object_t* trtp_rtcp_source_ctor(tsk_object_t * self, va_list * app)
{
    trtp_rtcp_source_t *source = (trtp_rtcp_source_t *)self;
    if(source) {
    }
    return self;
}
static tsk_object_t* trtp_rtcp_source_dtor(tsk_object_t * self)
{
    trtp_rtcp_source_t *source = (trtp_rtcp_source_t *)self;
    if(source) {
    }
    return self;
}
static const tsk_object_def_t trtp_rtcp_source_def_s = {
    sizeof(trtp_rtcp_source_t),
    trtp_rtcp_source_ctor,
    trtp_rtcp_source_dtor,
    tsk_null,
};
const tsk_object_def_t *trtp_rtcp_source_def_t = &trtp_rtcp_source_def_s;

static int _trtp_rtcp_source_init_seq(trtp_rtcp_source_t* self, uint32_t media_type, uint16_t seq, uint32_t ts);
static tsk_bool_t _trtp_rtcp_source_update_seq(trtp_rtcp_source_t* self, uint32_t media_type, uint16_t seq, uint32_t ts, uint32_t interval, int video_id);

static int _trtp_rtcp_check_packet_type(const trtp_rtp_packet_t* pkt)
{
    if (!pkt) {
        return -1;
    }

    if (pkt->header->payload_type == 111) {
        return PACKET_MEIDA_TYPE_AUDIO;   // audio
    } else if (pkt->header->payload_type == 105) {
        return PACKET_MEIDA_TYPE_VIDEO;   // video
    } else {
        return -2;
    }
}

static int __pred_find_source_by_ssrc(const tsk_list_item_t *item, const void *pssrc)
{
    if(item && item->data) {
        trtp_rtcp_source_t *source = (trtp_rtcp_source_t *)item->data;
        return source->ssrc - *((uint32_t*)pssrc);
    }
    return -1;
}

static trtp_rtcp_source_t* _trtp_rtcp_source_create(uint32_t ssrc, uint16_t seq, uint32_t ts)
{
    trtp_rtcp_source_t* source = nullptr;
    if(!(source = (trtp_rtcp_source_t *)tsk_object_new(trtp_rtcp_source_def_t))) {
        TSK_DEBUG_ERROR("Failed to create source object");
        return tsk_null;
    }

    _trtp_rtcp_source_init_seq(source, PACKET_MEIDA_TYPE_UNKNOW, seq, ts);
    source->ssrc = ssrc;

    source->video_id = -1;
    source->last_video_id = -1;

    source->audio_max_seq = seq - 1;
    source->audio_probation = MIN_SEQUENTIAL;
    source->audio_rate = AUDIO_CODEC_RATE;//FIXME

    source->video_max_seq = seq - 1;
    source->video_probation = MIN_SEQUENTIAL;
    source->video_rate = VIDEO_CODEC_RATE;//FIXME
    
    source->rtt = 0;
    source->min_rtt = 0;
    source->max_rtt = 0;
    source->avg_rtt = 0;
    source->num_average_calcs = 0;

    source->video_source_type = 0; // default main stream
    source->video_last_source_type = 0;
    source->video_main_stream_count  = 0;
    source->video_minor_stream_count = 0;

    source->video_last_lossrate = 0;
    source->video_down_loss_cnt = 0;
    
    // bitrate statistics for recv video
    source->video_recv_stat_bytes = 0;
    source->video_recv_instant_bitrate = 0;
    source->video_recv_last_stat_timestamp = 0;

    // bitrate statistics for recv share video
    source->video_share_recv_stat_bytes = 0;
    source->video_share_recv_instant_bitrate = 0;
    source->video_share_recv_last_stat_timestamp = 0;

    // bitrate statistics for recv audio
    source->audio_recv_stat_bytes = 0;
    source->audio_recv_instant_bitrate = 0;
    source->audio_recv_last_stat_timestamp = 0;

    return source;
}

static int _trtp_rtcp_source_init_seq(trtp_rtcp_source_t* self, uint32_t media_type, uint16_t seq, uint32_t ts)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if (PACKET_MEIDA_TYPE_AUDIO == media_type || PACKET_MEIDA_TYPE_UNKNOW == media_type) {
        self->audio_base_seq = seq;
        self->audio_max_seq = seq;
        self->audio_bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
        self->audio_cycles = 0;
        self->audio_received = 0;
        self->audio_received_prior = 0;
        self->audio_expected_prior = 0;
        self->audio_base_ts = ts;
        self->audio_max_ts = ts;
        self->audio_received_max_seq = 0;
        self->audio_last_received_time_ms = 0;
        self->audio_last_received_timestamp = 0;
        self->audio_transmitted = 0;
        self->audio_retransmitted = 0;
        self->audio_jitter_q4 = 0;
        self->audio_max_jitter_q4 = 0;
    }
    
    if (PACKET_MEIDA_TYPE_VIDEO == media_type || PACKET_MEIDA_TYPE_UNKNOW == media_type){
        self->video_base_seq = seq;
        self->video_max_seq = seq;
        self->video_bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
        self->video_cycles = 0;
        self->video_received = 0;
        self->video_received_prior = 0;
        self->video_expected_prior = 0;
        self->video_base_ts = ts;
        self->video_max_ts = ts;
        self->video_received_max_seq = -1;
        self->video_last_received_time_ms = 0;
        self->video_last_received_timestamp = 0;
        self->video_transmitted = 0;
        self->video_retransmitted = 0;
        self->video_jitter_q4 = 0;
        self->video_max_jitter_q4 = 0;
    }

    return 0;
}

static tsk_bool_t _trtp_rtcp_source_update_seq(trtp_rtcp_source_t* self, uint32_t media_type, uint16_t seq, uint32_t ts, uint32_t interval, int video_id)
{
    uint16_t udelta;
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return tsk_false;
    }

    if (PACKET_MEIDA_TYPE_AUDIO == media_type) {
        udelta = seq - self->audio_max_seq;

        /*
        * Source is not valid until MIN_SEQUENTIAL packets with
        * sequential sequence numbers have been received.
        */
        if (self->audio_probation) {
            /* packet is in sequence */
            if (seq == self->audio_max_seq + interval) {
                self->audio_probation--;
                self->audio_max_seq = seq;
                self->audio_max_ts = ts;
                if (self->audio_probation == 0) {
                    _trtp_rtcp_source_init_seq(self, PACKET_MEIDA_TYPE_AUDIO, seq, ts);
                    self->audio_received++;
                    return tsk_true;
                }
            }
            else {
                self->audio_probation = MIN_SEQUENTIAL - 1;
                self->audio_max_seq = seq;
                self->audio_max_ts = ts;
            }
            return tsk_false;
        }
        else if (udelta < MAX_DROPOUT) {
            /* in order, with permissible gap */
            if (seq < self->audio_max_seq) {
                /*
                * Sequence number wrapped - count another 64K cycle.
                */
                self->audio_cycles += RTP_SEQ_MOD;
            }
            self->audio_max_seq = seq;
            self->audio_max_ts = ts;
        }
        else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
            /* the sequence number made a very large jump */
            if (seq == self->audio_bad_seq) {
                /*
                * Two sequential packets -- assume that the other side
                * restarted without telling us so just re-sync
                * (i.e., pretend this was the first packet).
                */
                _trtp_rtcp_source_init_seq(self, PACKET_MEIDA_TYPE_AUDIO, seq, ts);
            }
            else {
                self->audio_bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
                return tsk_false;
            }
        }
        else {
            /* duplicate or reordered packet */
        }
        self->audio_received++;
    } else { // video info
        if (self->video_share_recv_instant_bitrate > 0 && (0 == video_id || 1 == video_id)) {
            // camera和share流同时存在时，计算共享桌面丢包率
            TMEDIA_I_AM_ACTIVE(1000, "stat lossrate share stream priority when recv share and camera together");
            return tsk_true;
        }
        
        udelta = seq - self->video_max_seq;
        
        /*
         * Source is not valid until MIN_SEQUENTIAL packets with
         * sequential sequence numbers have been received.
         */
        if (self->video_probation) {
            /* packet is in sequence */
            if (seq == self->video_max_seq + 1) {
                self->video_probation--;
                self->video_max_seq = seq;
                self->video_max_ts = ts;
                if (self->video_probation == 0) {
                    _trtp_rtcp_source_init_seq(self, PACKET_MEIDA_TYPE_VIDEO, seq, ts);
                    self->video_received++;
                    return tsk_true;
                }
            }
            else {
                self->video_probation = MIN_SEQUENTIAL - 1;
                self->video_max_seq = seq;
                self->video_max_ts = ts;
            }
            return tsk_false;
        }
        else if (udelta < MAX_DROPOUT) {
            /* in order, with permissible gap */
            if (seq < self->video_max_seq) {
                /*
                 * Sequence number wrapped - count another 64K cycle.
                 */
                self->video_cycles += RTP_SEQ_MOD;
            }
            self->video_max_seq = seq;
            self->video_max_ts = ts;
        }
        else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
            /* the sequence number made a very large jump */
            if (seq == self->video_bad_seq) {
                /*
                 * Two sequential packets -- assume that the other side
                 * restarted without telling us so just re-sync
                 * (i.e., pretend this was the first packet).
                 */
                _trtp_rtcp_source_init_seq(self, PACKET_MEIDA_TYPE_VIDEO, seq, ts);
            }
            else {
                self->video_bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
                return tsk_false;
            }
        }
        else {
            /* duplicate or reordered packet */
        }
        self->video_received++;
    }
    return tsk_true;
}

static tsk_bool_t _trtp_rtcp_source_is_probed(const trtp_rtcp_source_t* self)
{
    return (self && self->audio_probation == 0 && self->video_probation == 0);
}

typedef time_tp (*tc_f)();
static time_tp _trtp_rtcp_session_tc()
{
    return (time_tp)tsk_time_now();
}

typedef struct trtp_rtcp_session_s {
    TSK_DECLARE_OBJECT;

    tsk_bool_t is_started;
    tnet_fd_t local_fd;
    struct tnet_transport_s* transport; // not starter -> do not stop
    const struct sockaddr * remote_addr;

    tsk_bool_t is_using_p2p;    // 当前是否p2p路径传输， 默认为false
    tnet_fd_t local_fd_p2p;
    struct tnet_transport_s* transport_p2p;
    const struct sockaddr * remote_addr_p2p;

    // struct tnet_ice_ctx_s* ice_ctx; // not starter -> do not stop
    // tsk_bool_t is_ice_turn_active;

    const void* callback_data;
    trtp_rtcp_cb_f callback;

    // for switch video source(main/minor)
    const void* switch_cb_data;
    video_switch_cb_f switch_cb;

    int32_t app_bw_max_upload; // application specific (kbps)
    int32_t app_bw_max_download; // application specific (kbps)
    float app_jcng_q; // application specific for jitter buffer congestion estimation (quality in [0, 1])

    struct {
//        xt_timer_manager_handle_t* handle_global;
        xt_timer_id_t id_report;
        xt_timer_id_t id_bye;
    } timer;

    trtp_rtcp_source_t* source_local; /**< local source */
    trtp_rtcp_report_sdes_t* sdes;
    uint64_t time_start; /**< Start time in millis (NOT in NTP unit yet) */

    // <RTCP-FB>
    uint8_t fir_seqnr;
    // </RTCP-FB>

    // <sender>
    char* cname;
    uint32_t audio_packets_count;
    uint32_t audio_octets_count;
    uint32_t main_video_packets_count;
    uint32_t main_video_octets_count;
    uint32_t minor_video_packets_count;
    uint32_t minor_video_octets_count;
    uint32_t share_video_packets_count;
    uint32_t share_video_octets_count;

    int video_pkt_send_stat;

    uint32_t audio_send_instant_bitrate;    // 音频即时码率
    uint32_t video_send_camera_instant_bitrate;    // 视频即时码率（包括大小流）
    uint32_t video_send_share_instant_bitrate;     // 视频即时码率（仅包括share）
    uint32_t video_send_stream_stat;
    // </sender>

    // <others>
    time_tp tp; /**< the last time an RTCP packet was transmitted; */
    tc_f tc; /**< the current time */
    time_tp tn; /**< the next scheduled transmission time of an RTCP packet */
    int32_t pmembers; /**< the estimated number of session members at the time tn was last recomputed */
    int32_t members; /**< the most current estimate for the number of session members */
    int32_t senders; /**< the most current estimate for the number of senders in the session */
    double rtcp_bw; /**< The target RTCP bandwidth, i.e., the total bandwidth
      that will be used for RTCP packets by all members of this session,
      in octets per second.  This will be a specified fraction of the
      "session bandwidth" parameter supplied to the application at
      startup*/
    tsk_bool_t we_sent; /**< Flag that is true if the application has sent data since the 2nd previous RTCP report was transmitted */
    double avg_rtcp_size; /**< The average compound RTCP packet size, in octets,
      over all RTCP packets sent and received by this participant.  The
      size includes lower-layer transport and network protocol headers
      (e.g., UDP and IP) as explained in Section 6.2*/
    tsk_bool_t initial; /**< Flag that is true if the application has not yet sent an RTCP packet */
    // </others>

    trtp_rtcp_sources_L_t *sources;
    int source_count;

    TSK_DECLARE_SAFEOBJ;

// #if HAVE_SRTP
//     struct {
//         const srtp_t* session;
//     } srtp;
// #endif
    
   
}
trtp_rtcp_session_t;


static tsk_object_t* trtp_rtcp_session_ctor(tsk_object_t * self, va_list * app)
{
    trtp_rtcp_session_t *session = (trtp_rtcp_session_t *)self;
    if(session) {
        session->app_bw_max_upload = INT_MAX; // INT_MAX or <=0 means undefined
        session->app_bw_max_download = INT_MAX; // INT_MAX or <=0 means undefined
        session->sources = tsk_list_create();
        session->timer.id_report = XT_INVALID_TIMER_ID;
        session->timer.id_bye = XT_INVALID_TIMER_ID;
        session->tc = _trtp_rtcp_session_tc;
        session->source_count = 0;

        // get a handle for the global timer manager
        //session->timer.handle_global = xt_timer_manager_create();
        tsk_safeobj_init(session);
    }
    return self;
}
static tsk_object_t* trtp_rtcp_session_dtor(tsk_object_t * self)
{
    trtp_rtcp_session_t *session = (trtp_rtcp_session_t *)self;
    if(session) {
        trtp_rtcp_session_stop(session);
        tsk_list_item_t* item;
        if (tsk_list_count_all(session->sources) > 0) {
            tsk_list_foreach(item, session->sources) {
                TSK_OBJECT_SAFE_FREE(item->data);
            }
            tsk_list_clear_items(session->sources);
        }
        
        TSK_OBJECT_SAFE_FREE(session->sources);
        TSK_OBJECT_SAFE_FREE(session->source_local);
        TSK_OBJECT_SAFE_FREE(session->sdes);
        //TSK_OBJECT_SAFE_FREE(session->ice_ctx); // not starter -> do not stop
        TSK_FREE(session->cname);
        TSK_OBJECT_SAFE_FREE(session->transport); // not starter -> do not stop
        // release the handle for the global timer manager
        // xt_timer_manager_destroy(&session->timer.handle_global);

        tsk_safeobj_deinit(session);
    }
    return self;
}
static const tsk_object_def_t trtp_rtcp_session_def_s = {
    sizeof(trtp_rtcp_session_t),
    trtp_rtcp_session_ctor,
    trtp_rtcp_session_dtor,
    tsk_null,
};

const tsk_object_def_t *trtp_rtcp_session_def_t = &trtp_rtcp_session_def_s;
static tsk_bool_t _trtp_rtcp_session_have_source(trtp_rtcp_session_t* self, uint32_t ssrc);
static trtp_rtcp_source_t* _trtp_rtcp_session_find_source(trtp_rtcp_session_t* self, uint32_t ssrc);
static trtp_rtcp_source_t* _trtp_rtcp_session_find_or_add_source(trtp_rtcp_session_t* self, uint32_t ssrc, uint16_t seq_if_add, uint32_t ts_id_add);
static int _trtp_rtcp_session_add_source(trtp_rtcp_session_t* self, trtp_rtcp_source_t* source);
static int _trtp_rtcp_session_add_source_2(trtp_rtcp_session_t* self, uint32_t ssrc, uint16_t seq, uint32_t ts, tsk_bool_t *added);
static int _trtp_rtcp_session_remove_source(trtp_rtcp_session_t* self, uint32_t ssrc, tsk_bool_t *removed);
static tsk_size_t _trtp_rtcp_session_send_pkt(trtp_rtcp_session_t* self, trtp_rtcp_packet_t* pkt);
static tsk_size_t _trtp_rtcp_session_send_raw(trtp_rtcp_session_t* self, const void* data, tsk_size_t size);
static int _trtp_rtcp_session_timer_callback(const void* arg, xt_timer_id_t timer_id);

static void Schedule(trtp_rtcp_session_t* session, double tn, event_ e);
static void OnReceive(trtp_rtcp_session_t* session, const packet_ p, event_ e, tsk_size_t ReceivedPacketSize);
static void OnExpire(trtp_rtcp_session_t* session, event_ e);
static void SendBYEPacket(trtp_rtcp_session_t* session, event_ e);
static tsk_size_t SendRTCPReport(trtp_rtcp_session_t* session, event_ e);

trtp_rtcp_session_t* trtp_rtcp_session_create(uint32_t ssrc, const char* cname)
{
    trtp_rtcp_session_t* session;

    if(!(session = (trtp_rtcp_session_t *)tsk_object_new(trtp_rtcp_session_def_t))) {
        TSK_DEBUG_ERROR("Failed to create new session object");
        return tsk_null;
    }

    // RFC 3550 - 6.3.2 Initialization
    if(!(session->source_local = _trtp_rtcp_source_create(ssrc, 0, 0))) {
        TSK_DEBUG_ERROR("Failed to create new local source");
        TSK_OBJECT_SAFE_FREE(session);
        goto bail;
    }
    //_trtp_rtcp_session_add_source(session, session->source_local);
    session->initial = tsk_true;
    session->we_sent = tsk_false;
    session->senders = 1;
    session->members = 1;
    session->rtcp_bw = RTCP_BW;//FIXME: as parameter from the code, Also added possiblities to update this value
    session->cname = tsk_strdup(cname);

    // session->is_using_p2p = tsk_false;;

    session->audio_send_instant_bitrate;
    session->video_send_camera_instant_bitrate;
    session->video_send_share_instant_bitrate;
    session->video_send_stream_stat = 0;
    
bail:
    return session;
}

/*
struct trtp_rtcp_session_s* trtp_rtcp_session_create_2(struct tnet_ice_ctx_s* ice_ctx, uint32_t ssrc, const char* cname)
{
    struct trtp_rtcp_session_s* session = trtp_rtcp_session_create(ssrc, cname);
    if (session) {
        if ((session->ice_ctx = tsk_object_ref(ice_ctx))) {
            session->is_ice_turn_active = tnet_ice_ctx_is_turn_rtcp_active(session->ice_ctx);
        }
    }
    return session;
}*/

int trtp_rtcp_session_set_callback(trtp_rtcp_session_t* self, trtp_rtcp_cb_f callback, const void* callback_data)
{
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    tsk_safeobj_lock(self);
    self->callback = callback;
    self->callback_data = callback_data;
    tsk_safeobj_unlock(self);
    return 0;
}

int trtp_rtcp_session_set_video_swicth_callback(struct trtp_rtcp_session_s* self, video_switch_cb_f callback, const void* callback_data)
{
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    tsk_safeobj_lock(self);
    self->switch_cb = callback;
    self->switch_cb_data = callback_data;
    tsk_safeobj_unlock(self);
    return 0;
}

//#if HAVE_SRTP
//int trtp_rtcp_session_set_srtp_sess(trtp_rtcp_session_t* self, const srtp_t* session)
//{
//    if(!self) {
//        TSK_DEBUG_ERROR("Invalid parameter");
//        return -1;
//    }
//
//    tsk_safeobj_lock(self);
//    self->srtp.session = session;
//    tsk_safeobj_unlock(self);
//
//    return 0;
//}
//#endif

int trtp_rtcp_session_set_app_bw_and_jcng(trtp_rtcp_session_t* self, int32_t bw_upload_kbps, int32_t bw_download_kbps, float jcng_q)
{
    trtp_rtcp_report_rr_t* rr;
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock(self);

    self->app_bw_max_upload = bw_upload_kbps;
    self->app_bw_max_download = bw_download_kbps;
    self->app_jcng_q = jcng_q;

    if(self->is_started && self->source_local) { // INT_MAX or <=0 means undefined
        tsk_list_item_t* item;
        uint32_t media_ssrc_list[256] = {0};
        uint32_t media_ssrc_list_count = 0;
        // retrieve sources as array
        tsk_list_foreach(item, self->sources) {
            if(!item->data) {
                continue;
            }
            if((media_ssrc_list_count + 1) < sizeof(media_ssrc_list)/sizeof(media_ssrc_list[0])) {
                media_ssrc_list[media_ssrc_list_count++] = TRTP_RTCP_SOURCE(item->data)->ssrc;
            }
        }
        // create RTCP-RR packet and send it over the network
        if (media_ssrc_list_count > 0 && (rr = trtp_rtcp_report_rr_create_2(self->source_local->ssrc))) {
            // RTCP-REMB
            if (self->app_bw_max_download > 0 && self->app_bw_max_download != INT_MAX) {
                // app_bw_max_download unit is kbps while create_afb_remb() expect bps
                trtp_rtcp_report_psfb_t* psfb_afb_remb = trtp_rtcp_report_psfb_create_afb_remb(self->source_local->ssrc/*sender SSRC*/, media_ssrc_list, media_ssrc_list_count, (self->app_bw_max_download << 10));
                if (psfb_afb_remb) {
                    TSK_DEBUG_INFO("Packing RTCP-AFB-REMB (bw_dwn=%d kbps) for outgoing RTCP-RR", self->app_bw_max_download);
                    trtp_rtcp_packet_add_packet((trtp_rtcp_packet_t*)rr, (trtp_rtcp_packet_t*)psfb_afb_remb, tsk_false);
                    TSK_OBJECT_SAFE_FREE(psfb_afb_remb);
                }
            }
            // RTCP-JCNG
            if (self->app_jcng_q > 0.f && self->app_jcng_q <= 1.f) {
                // app_bw_max_download unit is kbps while create_afb_remb() expect bps
                trtp_rtcp_report_psfb_t* psfb_afb_jcng = trtp_rtcp_report_psfb_create_afb_jcng(self->source_local->ssrc/*sender SSRC*/, media_ssrc_list, media_ssrc_list_count, self->app_jcng_q);
                if (psfb_afb_jcng) {
                    TSK_DEBUG_INFO("Packing RTCP-AFB-JCNG (q=%f) for outgoing RTCP-RR", self->app_jcng_q);
                    trtp_rtcp_packet_add_packet((trtp_rtcp_packet_t*)rr, (trtp_rtcp_packet_t*)psfb_afb_jcng, tsk_false);
                    TSK_OBJECT_SAFE_FREE(psfb_afb_jcng);
                }
            }
            // send the RR message
            _trtp_rtcp_session_send_pkt(self, (trtp_rtcp_packet_t*)rr);
            TSK_OBJECT_SAFE_FREE(rr);
        }
    }

    tsk_safeobj_unlock(self);

    return 0;
}

int trtp_rtcp_session_start(trtp_rtcp_session_t* self, tnet_fd_t local_fd, const struct sockaddr * remote_addr)
{
    int ret = 0;

    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(self->is_started) {
        TSK_DEBUG_WARN("Already started");
        return 0;
    }

    // start global timer manager
//    if((ret = xt_timer_manager_start(self->timer.handle_global))) {
//        TSK_DEBUG_ERROR("Failed to start timer");
//        return ret;
//    }

    self->local_fd = local_fd;
    self->remote_addr = remote_addr;

    // Send Initial RR (mandatory)
    Schedule(self, 1000, EVENT_REPORT);

    // set start time
    self->time_start = tsk_time_now();

    self->is_started = tsk_true;

    return ret;
}

int trtp_rtcp_session_stop(trtp_rtcp_session_t* self)
{
    int ret = 0;

    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    if(self->is_started) {
        // send BYE synchronous way
        SendBYEPacket(self, EVENT_REPORT);

        // this is a global timer shared by many components -> stopping it won't remove
        // all scheduled items as it could continue running if still used
        tsk_safeobj_lock(self); // must
        if(XT_TIMER_ID_IS_VALID(self->timer.id_bye)) {
            // xt_timer_manager_cancel(self->timer.handle_global, self->timer.id_bye);
            xt_timer_mgr_global_cancel(self->timer.id_bye);
            self->timer.id_bye = XT_INVALID_TIMER_ID;
        }
        if(XT_TIMER_ID_IS_VALID(self->timer.id_report)) {
            // xt_timer_manager_cancel(self->timer.handle_global, self->timer.id_report);
            xt_timer_mgr_global_cancel(self->timer.id_report);
            self->timer.id_report = XT_INVALID_TIMER_ID;
        }
        tsk_safeobj_unlock(self);
        self->is_started = tsk_false;
    }

    return ret;
}

int trtp_rtcp_session_process_rtp_out(trtp_rtcp_session_t* self, const trtp_rtp_packet_t* packet_rtp, tsk_size_t size)
{
    int ret = 0;

    if(!self || !packet_rtp || !packet_rtp->header) {
        TSK_DEBUG_ERROR("RTP out: Invalid parameter");
        return -1;
    }

    if(!self->is_started) {
        TSK_DEBUG_ERROR("RTP out:Not started");
        return -2;
    }

    tsk_safeobj_lock(self);

    // create local source if not already done
    // first destroy it if the ssrc don't match
    packet_rtp->header->ssrc = packet_rtp->header->csrc[0]; // change rtp pkt ssrc to sessionid
    if(self->source_local && self->source_local->ssrc != packet_rtp->header->ssrc) {
        tsk_bool_t removed = tsk_false;
        // local ssrc has changed: sould never happen ...but who know?
        // remove the source
        TSK_DEBUG_WARN("Not expected to be called");
        _trtp_rtcp_session_remove_source(self, self->source_local->ssrc, &removed);
        TSK_OBJECT_SAFE_FREE(self->source_local);
        TSK_OBJECT_SAFE_FREE(self->sdes);

        self->audio_packets_count = 0;  // audio stat
        self->audio_octets_count = 0;
        self->main_video_packets_count = 0;  // main video stat
        self->main_video_octets_count = 0;
        self->minor_video_packets_count = 0;  // minor video stat
        self->minor_video_octets_count = 0;
        self->share_video_packets_count = 0;
        self->share_video_octets_count = 0;

        self->video_pkt_send_stat = 0;  //初始状态没有发送视频
        if(removed) {
            --self->senders;
            --self->members;
        }
    }
    
    // check meida type: audio or video
    int media_type = 0; // type 0:audio 1:video
    int payload_type = packet_rtp->header->payload_type;
    if (payload_type == 105) {  // todo: payload set by rtrp_manager
        media_type = 1;
    }
    
    if(!self->source_local) {
        if(!(self->source_local = _trtp_rtcp_source_create(packet_rtp->header->ssrc, packet_rtp->header->seq_num, packet_rtp->header->timestamp))) {
            TSK_DEBUG_ERROR("Failed to create new local source");
        }
        // add the source (refresh the number of senders, ...)
        _trtp_rtcp_session_add_source(self, self->source_local);
        // 'members' and 'senders' were already initialized in the constructor
    }

    if(!self->we_sent) {
        self->we_sent = tsk_true;
    }

    if (media_type) {
        int video_id = 0;
        if (packet_rtp->header->csrc_count >= 5) {
            video_id = packet_rtp->header->csrc[4];
        } else {
            video_id = packet_rtp->header->csrc[3];
        }
        
        if (1 == video_id) {
            ++self->minor_video_packets_count;
            self->minor_video_octets_count += (uint32_t)size;
            AddVideoCode( size,  packet_rtp->header->session_id );
        } else if (0 == video_id) {
            ++self->main_video_packets_count;
            self->main_video_octets_count += (uint32_t)size;
            AddVideoCode( size,  packet_rtp->header->session_id );
        } else if (2 == video_id) {
            ++self->share_video_packets_count;
            self->share_video_octets_count += (uint32_t)size;
            AddVideoShareCode( size,  packet_rtp->header->session_id );
        }

    } else {
        ++self->audio_packets_count;
        self->audio_octets_count += (uint32_t)size;

        AddAudioCode( size,  packet_rtp->header->csrc[0] );
    }

    tsk_safeobj_unlock(self);

    return ret;
}

int trtp_rtcp_session_is_newer_sequence_number(uint16_t sequence_number,
                          uint16_t prev_sequence_number){
    // Distinguish between elements that are exactly 0x8000 apart.
    // If s1>s2 and |s1-s2| = 0x8000: IsNewer(s1,s2)=true, IsNewer(s2,s1)=false
    // rather than having IsNewer(s1,s2) = IsNewer(s2,s1) = false.
    if ((uint16_t)(sequence_number - prev_sequence_number) == 0x8000) {
        return (sequence_number > prev_sequence_number) ? 1: 0;
    }
    if (sequence_number != prev_sequence_number && (uint16_t)(sequence_number - prev_sequence_number) < 0x8000) {
        return 1;
    } else {
        return 0;
    }
}


int trtp_rtcp_session_is_packet_in_order(trtp_rtcp_source_t* source, int media_type, uint16_t seq){
    
    // First packet is always in order.
    if (PACKET_MEIDA_TYPE_VIDEO == media_type) {    // video
        if(source->video_last_received_time_ms == 0)
            return 1;
        
        if(trtp_rtcp_session_is_newer_sequence_number(seq, source->video_received_max_seq)){
            return 1;
        }else{
            // If we have a restart of the remote side this packet is still in order.
            return !trtp_rtcp_session_is_newer_sequence_number(seq, source->video_received_max_seq - DEF_MAX_REORDERING_THRESHOLD);
        }        
    } else {   // audio
        if(source->audio_last_received_time_ms == 0)
            return 1;
        
        if(trtp_rtcp_session_is_newer_sequence_number(seq, source->audio_received_max_seq)){
            return 1;
        }else{
            // If we have a restart of the remote side this packet is still in order.
            return !trtp_rtcp_session_is_newer_sequence_number(seq, source->audio_received_max_seq - DEF_MAX_REORDERING_THRESHOLD);
        }   
    }
}

int trtp_rtcp_session_is_packet_retransmitted(trtp_rtcp_source_t* source, int media_type, trtp_rtp_header_t* header,int64_t min_rtt) {    
    // 由于video rtp传输timestamp就是以ms为单位, audio rtp传输timestamp是采样率为单位
    unsigned int frequency_khz = 1;//(uint32_t)(90000 / 1000);  // video freq is 90K
    int64_t time_diff_ms = 0, max_delay_ms = 0;
    uint32_t rtp_time_stamp_diff_ms = 0;
    uint32_t curr_jitter_q4 = 0;

    if (PACKET_MEIDA_TYPE_VIDEO == media_type) {
        frequency_khz = 1;
        time_diff_ms = tsk_time_now() - source->video_last_received_time_ms;
        rtp_time_stamp_diff_ms = (header->timestamp - source->video_last_received_timestamp)/frequency_khz;
        curr_jitter_q4 = source->video_jitter_q4; 
    } else {
        frequency_khz = (uint32_t)(source->audio_rate / 1000);
        time_diff_ms = tsk_time_now() - source->audio_last_received_time_ms;
        rtp_time_stamp_diff_ms = (header->timestamp - source->audio_last_received_timestamp)/frequency_khz;
        curr_jitter_q4 = source->audio_jitter_q4;
    }

    if(min_rtt == 0){
        float jitter_std = sqrt(static_cast<float>(curr_jitter_q4 >> 4));
        max_delay_ms = (int64_t)(2 * jitter_std);
    
        if(max_delay_ms == 0)
            max_delay_ms =1;
    }
    else{
        max_delay_ms = (min_rtt / 3) + 1;
    }
   
     return time_diff_ms > rtp_time_stamp_diff_ms + max_delay_ms;
}

void trtp_rtcp_session_qos(trtp_rtcp_source_t* source, int media_type, trtp_rtp_header_t* header, int video_id) {

    if (PACKET_MEIDA_TYPE_VIDEO == media_type && source->video_share_recv_instant_bitrate > 0 && (0 == video_id || 1 == video_id)) {
        // camera和share流同时存在时，计算共享流的qos
        TMEDIA_I_AM_ACTIVE(1000, "stat jitter share stream priority when recv share and camera together");
        return;
    }

    int retransmitted = 0;
    int in_order = trtp_rtcp_session_is_packet_in_order(source, media_type, header->seq_num);
    if (!in_order) {
        retransmitted = trtp_rtcp_session_is_packet_retransmitted(source, media_type, header, source->min_rtt);
    }

    int64_t packet_arrival_ms = tsk_time_now();
    if (PACKET_MEIDA_TYPE_VIDEO == media_type) {    // video jitter
        source->video_transmitted++;
        if(!in_order && retransmitted)
            source->video_retransmitted++;
        
        if(in_order){
            source->video_received_max_seq = header->seq_num;
            
            if(header->timestamp != source->video_last_received_timestamp && source->video_transmitted - source->video_retransmitted > 1){
                // trtp_rtcp_session_calculate_jitter(source,packet_rtp->header, packet_arrival_ms);
                int64_t time_diff_samples = (packet_arrival_ms - source->video_last_received_time_ms) -
                                                (header->timestamp - source->video_last_received_timestamp);
                
                time_diff_samples = abs(time_diff_samples);
                
                if (time_diff_samples < 1000) {
                    int32_t jitter_diff_q4 = (time_diff_samples << 4) - source->video_jitter_q4;
                    source->video_jitter_q4 += ((jitter_diff_q4 + 8) >> 4);
                }
            }
            source->video_last_received_timestamp = header->timestamp;
            source->video_last_received_time_ms = packet_arrival_ms;
        }
    } else {    // audio jitter
        source->audio_transmitted++;
        if(!in_order && retransmitted)
            source->audio_retransmitted++;
        
        if(in_order){
            source->audio_received_max_seq = header->seq_num;
            
            if(header->timestamp != source->audio_last_received_timestamp && source->audio_transmitted - source->audio_retransmitted > 1){
                // 由于audio rtp传输timestamp是采样率为单位, 所以时间戳需要转换为ms
                uint32_t frequency_khz = (uint32_t)(source->audio_rate / 1000);
                int64_t time_diff_samples = (packet_arrival_ms - source->audio_last_received_time_ms) -
                                                (header->timestamp - source->audio_last_received_timestamp)/frequency_khz;
                
                time_diff_samples = abs(time_diff_samples);
                if (time_diff_samples < 1000) {
                    int32_t jitter_diff_q4 = (time_diff_samples << 4) - source->audio_jitter_q4;
                    source->audio_jitter_q4 += ((jitter_diff_q4 + 8) >> 4);
                }
            }
            source->audio_last_received_timestamp = header->timestamp;
            source->audio_last_received_time_ms = packet_arrival_ms;
        }
    }
}

int trtp_rtcp_session_process_rtp_in(trtp_rtcp_session_t* self, const trtp_rtp_packet_t* packet_rtp, tsk_size_t size)
{
    int ret = 0;
    trtp_rtcp_source_t* source;

    if(!self || !packet_rtp || !packet_rtp->header) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    if(!self->is_started) {
        TSK_DEBUG_INFO("RTCP session not started");
        return -2;
    }
    
    // set sessionid as ssrc by mark
    if (packet_rtp->header->csrc_count > 0) {
        packet_rtp->header->ssrc = packet_rtp->header->csrc[0];
    } else {
        // packet which from webrtc, do nothing
    }
    
    // check media type
    int media = _trtp_rtcp_check_packet_type(packet_rtp);
    
    tsk_safeobj_lock(self);
    OnReceive(self, (const packet_)packet_rtp, EVENT_RTP, size);
    if((source = _trtp_rtcp_session_find_source(self, packet_rtp->header->ssrc))) {
        int interval = 1;
        // rscode打开的情况下 间隔为1
        if (PACKET_MEIDA_TYPE_AUDIO == media && packet_rtp->header->csrc_count < 5) {
            if (packet_rtp->header->marker) {
                interval = 2;
            }
        } 

        // for video, 检查video_id, 如果videoid发生变化，则reset统计数据
        int curr_video_id = 0;
        if (PACKET_MEIDA_TYPE_VIDEO == media) {
            if (packet_rtp->header->csrc_count >= 5) {
                // rscode open
                curr_video_id = packet_rtp->header->csrc[4];
            } else {
                curr_video_id = packet_rtp->header->csrc[3];
            }
            source->video_id = curr_video_id;

            // 远端开启共享流发送时，不处理
            if (!source->video_share_recv_instant_bitrate && -1 == source->last_video_id ) {
                source->last_video_id = curr_video_id;
            }
        }

        if(_trtp_rtcp_source_update_seq(source, media, packet_rtp->header->seq_num, packet_rtp->header->timestamp, interval, curr_video_id)) {
            // RFC 3550 A.8 Estimating the Interarrival Jitter
            if (PACKET_MEIDA_TYPE_AUDIO == media) {   // audio
                trtp_rtcp_session_qos(source, PACKET_MEIDA_TYPE_AUDIO, packet_rtp->header, curr_video_id);

                // audio recv stat
                {
                    if (!source->audio_recv_stat_bytes) {
                        source->audio_recv_last_stat_timestamp = tsk_time_now();
                    }
                    source->audio_recv_stat_bytes += size;

                    AddAudioCode( size,  packet_rtp->header->session_id );
                    AddAudioPacket(packet_rtp->header->seq_num, packet_rtp->header->session_id);
                }
            } else { // video
                trtp_rtcp_session_qos(source, PACKET_MEIDA_TYPE_VIDEO, packet_rtp->header, curr_video_id);

                if (!curr_video_id && TRTP_VIDEO_STREAM_TYPE_MINOR == source->video_source_type) {
                    // current source type is minor, but recv main stream pkt
                    source->video_source_type = TRTP_VIDEO_STREAM_TYPE_MAIN;
                } else if (TRTP_VIDEO_STREAM_TYPE_MINOR == curr_video_id && TRTP_VIDEO_STREAM_TYPE_MAIN == source->video_source_type) {
                    // current source type is main, but recv minor stream pkt
                    source->video_source_type = TRTP_VIDEO_STREAM_TYPE_MINOR;
                }

                // video recv stat (camera and share)
                if (curr_video_id != TRTP_VIDEO_STREAM_TYPE_SHARE) {
                    if (!source->video_recv_stat_bytes) {
                        source->video_recv_last_stat_timestamp = tsk_time_now();
                    }

                    source->video_recv_stat_bytes += size;
                    AddVideoCode( size,  packet_rtp->header->session_id );
                } else {
                    if (!source->video_share_recv_stat_bytes) {
                        source->video_share_recv_last_stat_timestamp = tsk_time_now();
                    }

                    source->video_share_recv_stat_bytes += size;
                    AddVideoShareCode( size,  packet_rtp->header->session_id );
                }
            }
        }
        TSK_OBJECT_SAFE_FREE(source);
    }

    tsk_safeobj_unlock(self);

    return ret;
}

int trtp_rtcp_session_process_rtcp_in(trtp_rtcp_session_t* self, const void* buffer, tsk_size_t size)
{
    int ret = 0;
    trtp_rtcp_packet_t* packet_rtcp = tsk_null;

    if(!self || !buffer || size < TRTP_RTCP_HEADER_SIZE) {
        TSK_DEBUG_ERROR("Invalid parameter to recv rtcp packet");
        return -1;
    }

    if(!self->is_started) {
        TSK_DEBUG_ERROR("rtcp session not started");
        return -2;
    }
    
    uint8_t pt = (trtp_rtcp_packet_type_t)((const uint8_t*)buffer)[1];
    if (pt < trtp_rtcp_packet_type_sr || pt > trtp_rtcp_packet_type_psfb) {
        TMEDIA_I_AM_ACTIVE(100, "type:%d, size:%u, not invalid rtcp pkt, drop it!", (int)pt, size);
        return 0;
    }

    int source_type = -1;
    int source_ssrc = 0;
    // derialize the RTCP packet for processing
    packet_rtcp = trtp_rtcp_packet_deserialize(buffer, size);
    if(packet_rtcp) {
        tsk_safeobj_lock(self);
        OnReceive(self,
                  (const packet_)packet_rtcp,
                  (packet_rtcp->header->type == trtp_rtcp_packet_type_bye) ? EVENT_BYE : EVENT_REPORT,
                  size);
        if(packet_rtcp->header->type == trtp_rtcp_packet_type_sr) {
            trtp_rtcp_source_t* source;

            const trtp_rtcp_report_sr_t* sr = (const trtp_rtcp_report_sr_t*)packet_rtcp;
            if((source = _trtp_rtcp_session_find_source(self, sr->ssrc))) {
                source->ntp_lsw = sr->sender_info.ntp_lsw;
                source->ntp_msw = sr->sender_info.ntp_msw;
                source->dlsr = tsk_time_now();
                source->video_main_stream_count  = sr->sender_info.video_sender_pcount;
                source->video_minor_stream_count = sr->sender_info.minor_video_sender_pcount;

                // TMEDIA_I_AM_ACTIVE(3, "trtp_rtcp_session_process_rtcp_in ssrc[%d] main[%d] minor[%d]"
                //                     , source->ssrc , source->video_main_stream_count
                //                     , source->video_minor_stream_count);

                if (source->video_main_stream_count || source->video_minor_stream_count) {
                    if ((!source->video_main_stream_count && (0 == source->video_source_type))
                        || (!source->video_minor_stream_count && (1 == source->video_source_type))) {
                        
                        // for server 0: main stream, 1: minor stream
                        source_type = (source->video_source_type == 0) ? 1 : 0;
                        source_ssrc = source->ssrc;
                    }
                }

                TSK_OBJECT_SAFE_FREE(source);
            }
        }
        tsk_safeobj_unlock(self); // must be before callback()

        int mode = tmedia_defaults_get_video_net_adjustmode();

        if (!mode && self->switch_cb && source_type >= 0) {
            self->switch_cb(self->switch_cb_data, source_ssrc, source_type);
        }
        
        if(self->callback) {
            ret = self->callback(self->callback_data, packet_rtcp);
        }
        TSK_OBJECT_SAFE_FREE(packet_rtcp);
    }
    
    return ret;
}

int trtp_rtcp_session_signal_pkt_loss(trtp_rtcp_session_t* self, trtp_rtcp_source_seq_map * source_seqs)
{
    //trtp_rtcp_report_rr_t* rr;
    if(!self || !self->source_local || !source_seqs) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(!self->is_started) {
        TSK_DEBUG_ERROR("Not started");
        return -1;
    }

    tsk_safeobj_lock(self);

    trtp_rtcp_report_rtpfb_t* rtpfb;
    if((rtpfb = trtp_rtcp_report_rtpfb_build_nack(self->source_local->ssrc, source_seqs))) {
        _trtp_rtcp_session_send_pkt(self,(trtp_rtcp_packet_t*)rtpfb);
        TSK_OBJECT_SAFE_FREE(rtpfb);
    }

    tsk_safeobj_unlock(self);

    return 0;
}

// send nack pkt for request full intra frame
int trtp_rtcp_session_signal_pkt_loss_plus(trtp_rtcp_session_t* self, uint32_t ssrc_media,const uint32_t* time_stamp,tsk_size_t count)
{
#if 0
    //trtp_rtcp_report_rr_t* rr;
    if(!self || !self->source_local) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(!self->is_started) {
        TSK_DEBUG_ERROR("Not started");
        return -1;
    }
    
    tsk_safeobj_lock(self);
    
    //if((rr = trtp_rtcp_report_rr_create_2(self->source_local->ssrc))) {
        trtp_rtcp_report_rtpfb_t* rtpfb;
        if((rtpfb = trtp_rtcp_report_rtpfb_create_nack_plus(self->source_local->ssrc, ssrc_media,time_stamp,count))) {
            //trtp_rtcp_packet_add_packet((trtp_rtcp_packet_t*)rr,  (trtp_rtcp_packet_t*)rtpfb, tsk_false);
            //_trtp_rtcp_session_send_pkt(self, (trtp_rtcp_packet_t*)rr);
            _trtp_rtcp_session_send_pkt(self, (trtp_rtcp_packet_t*)rtpfb);
            TSK_OBJECT_SAFE_FREE(rtpfb);
        }
        //TSK_OBJECT_SAFE_FREE(rr);
    //}
    
    tsk_safeobj_unlock(self);
#endif
    return 0;
}

// Frame corrupted means the prediction chain is broken -> Send FIR
int trtp_rtcp_session_signal_frame_corrupted(trtp_rtcp_session_t* self, uint32_t ssrc_media)
{
    trtp_rtcp_report_rr_t* rr;
    if(!self || !self->source_local) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    if(!self->is_started) {
        TSK_DEBUG_ERROR("Not started");
        return -1;
    }

    tsk_safeobj_lock(self);

    if((rr = trtp_rtcp_report_rr_create_2(self->source_local->ssrc))) {
        trtp_rtcp_report_psfb_t* psfb_fir = trtp_rtcp_report_psfb_create_fir(self->fir_seqnr++, self->source_local->ssrc, ssrc_media);
        if(psfb_fir) {
            trtp_rtcp_packet_add_packet((trtp_rtcp_packet_t*)rr, (trtp_rtcp_packet_t*)psfb_fir, tsk_false);
            _trtp_rtcp_session_send_pkt(self, (trtp_rtcp_packet_t*)rr);
            TSK_OBJECT_SAFE_FREE(psfb_fir);
        }
        TSK_OBJECT_SAFE_FREE(rr);
    }

    tsk_safeobj_unlock(self);
    return 0;
}

// for now send just a FIR
int trtp_rtcp_session_signal_jb_error(struct trtp_rtcp_session_s* self, uint32_t ssrc_media)
{
    return trtp_rtcp_session_signal_frame_corrupted(self, ssrc_media);
}

tnet_fd_t trtp_rtcp_session_get_local_fd(const struct trtp_rtcp_session_s* self)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return TNET_INVALID_FD;
    }
    return self->local_fd;
}

int trtp_rtcp_session_set_local_fd(struct trtp_rtcp_session_s* self, tnet_fd_t local_fd)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    self->local_fd = local_fd;
    return 0;
}

int trtp_rtcp_session_set_net_transport(struct trtp_rtcp_session_s* self, struct tnet_transport_s* transport)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    TSK_OBJECT_SAFE_FREE(self->transport);
    self->transport = (struct tnet_transport_s*)tsk_object_ref(transport);
    return 0;
}

int trtp_rtcp_session_set_p2p_transport(struct trtp_rtcp_session_s* self, struct tnet_transport_s* transport, tnet_fd_t local_fd, const struct sockaddr* remote_addr)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    TSK_OBJECT_SAFE_FREE(self->transport_p2p);
    self->transport_p2p = (struct tnet_transport_s*)tsk_object_ref(transport);
    self->local_fd_p2p = local_fd;
    self->remote_addr_p2p = remote_addr;
    return 0;
}

void trtp_rtcp_session_enable_p2p_transport(struct trtp_rtcp_session_s* self, tsk_bool_t enable)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return;
    }

    self->is_using_p2p = enable;
}

int trtp_rtcp_session_remove_member(struct trtp_rtcp_session_s* self, uint32_t ssrc)
{
    tsk_bool_t removed= tsk_false;
    _trtp_rtcp_session_remove_source(self, ssrc, &removed);

    return removed;
}

static tsk_bool_t _trtp_rtcp_session_have_source(trtp_rtcp_session_t* self, uint32_t ssrc)
{
    tsk_list_item_t* item;
    tsk_list_foreach(item, self->sources) {
        if(TRTP_RTCP_SOURCE(item->data)->ssrc == ssrc) {
            return tsk_true;
        }
    }
    return tsk_false;
}

// find source by ssrc
// the caller must release the returned object
static trtp_rtcp_source_t* _trtp_rtcp_session_find_source(trtp_rtcp_session_t* self, uint32_t ssrc)
{
    tsk_list_item_t* item;
    tsk_list_foreach(item, self->sources) {
        if(TRTP_RTCP_SOURCE(item->data)->ssrc == ssrc) {
            return (trtp_rtcp_source_t*)tsk_object_ref(item->data);
        }
    }
    return tsk_null;
}

// find or add source by ssrc
// the caller must release the returned object
/** unused, comment by mark
static trtp_rtcp_source_t* _trtp_rtcp_session_find_or_add_source(trtp_rtcp_session_t* self, uint32_t ssrc, uint16_t seq_if_add, uint32_t ts_id_add)
{
    trtp_rtcp_source_t* source;
    if((source = _trtp_rtcp_session_find_source(self, ssrc))) {
        return source;
    }

    if((source = _trtp_rtcp_source_create(ssrc, seq_if_add, ts_id_add))) {
        if((_trtp_rtcp_session_add_source(self, source)) != 0) {
            TSK_DEBUG_ERROR("Failed to add source");
            TSK_OBJECT_SAFE_FREE(source);
            return tsk_null;
        }
        return tsk_object_ref(source);
    }
    return tsk_null;
} **/

int _trtp_rtcp_session_add_source(trtp_rtcp_session_t* self, trtp_rtcp_source_t* source)
{
    if(!self || !source) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    tsk_list_lock(self->sources);
    source = (trtp_rtcp_source_t*)tsk_object_ref(source);
    tsk_list_push_back_data(self->sources, (void**)&source);
    tsk_list_unlock(self->sources);

    return 0;
}

// adds a source if doesn't exist
static int _trtp_rtcp_session_add_source_2(trtp_rtcp_session_t* self, uint32_t ssrc, uint16_t seq, uint32_t ts, tsk_bool_t *added)
{
    int ret = 0;
    tsk_list_item_t* item;
    trtp_rtcp_source_t* source;

    tsk_list_lock(self->sources);
    tsk_list_foreach(item, self->sources) {
        if(TRTP_RTCP_SOURCE(item->data)->ssrc == ssrc) {
            tsk_list_unlock(self->sources);
            *added = tsk_false;
            return 0;
        }
    }

    tsk_list_unlock(self->sources);

    if((source = _trtp_rtcp_source_create(ssrc, seq, ts))) {
        ret = _trtp_rtcp_session_add_source(self, source);
    }

    TSK_OBJECT_SAFE_FREE(source);

    *added = tsk_true;
    return ret;
}

int _trtp_rtcp_session_remove_source(trtp_rtcp_session_t* self, uint32_t ssrc, tsk_bool_t *removed)
{
    *removed = tsk_false;
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    tsk_list_lock(self->sources);
    if((*removed = tsk_list_remove_item_by_pred(self->sources, __pred_find_source_by_ssrc, &ssrc)) == tsk_true) {
        // ...
    }
    tsk_list_unlock(self->sources);
    return 0;
}

static tsk_size_t _trtp_rtcp_session_send_pkt(trtp_rtcp_session_t* self, trtp_rtcp_packet_t* pkt)
{
    tsk_size_t ret = 0;
    tsk_size_t __num_bytes_pad = 0;
    tsk_buffer_t* buffer;

    if(!self->remote_addr || self->local_fd <= 0) {
        TSK_DEBUG_ERROR("Invalid network settings");
        return 0;
    }

// #if HAVE_SRTP
//     if(self->srtp.session) {
//         __num_bytes_pad = (SRTP_MAX_TRAILER_LEN + 0x4);
//     }
// #endif
    
    if((buffer = trtp_rtcp_packet_serialize(pkt, __num_bytes_pad))) {
        void* data = buffer->data;
        int size = (int)buffer->size;
//#if HAVE_SRTP
//        if(self->srtp.session) {
//            if(srtp_protect_rtcp(((srtp_t)*self->srtp.session), data, &size) != err_status_ok) {
//                TSK_DEBUG_ERROR("srtp_protect_rtcp() failed");
//            }
//        }
//#endif
        ret = _trtp_rtcp_session_send_raw(self, data, size);
        TSK_OBJECT_SAFE_FREE(buffer);
    }

    return ret;
}

static tsk_size_t _trtp_rtcp_session_send_raw(trtp_rtcp_session_t* self, const void* data, tsk_size_t size)
{
    tsk_size_t ret = 0;
    if (!self || !data || !size) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }
    // p2p used 
    if (self->is_using_p2p) {
        ret = tnet_transport_sendto(self->transport_p2p, self->local_fd_p2p, self->remote_addr_p2p, data, size);
    }
    else {
        ret = self->transport
              ? tnet_transport_sendto(self->transport, self->local_fd, self->remote_addr, data, size)
              : tnet_sockfd_sendto(self->local_fd, self->remote_addr, data, size);
    }
    return ret;
}

static int _trtp_rtcp_session_timer_callback(const void* arg, xt_timer_id_t timer_id) {
    trtp_rtcp_session_t* session = (trtp_rtcp_session_t*)arg;
    tsk_safeobj_lock(session); // must
        if(session->timer.id_bye == timer_id) {
            //OnExpire(session, EVENT_BYE);
            SendBYEPacket(session, EVENT_BYE);
        }
        else if(session->timer.id_report == timer_id) {
            //OnExpire(session, EVENT_REPORT);
            tsk_size_t SentPacketSize = SendRTCPReport(session, EVENT_REPORT);
            session->avg_rtcp_size = (1./16.)*SentPacketSize + (15./16.)*(session->avg_rtcp_size);
//            session->tp = tc;
            session->initial = tsk_false;
        }
        tsk_safeobj_unlock(session);
        return 0;
    
}

//static int _trtp_rtcp_session_timer_callback(const void* arg, xt_timer_id_t timer_id)
//{
//    trtp_rtcp_session_t* session = (trtp_rtcp_session_t*)arg;
//    tsk_safeobj_lock(session); // must
//    if(session->timer.id_bye == timer_id) {
//        session->timer.id_bye = XT_INVALID_TIMER_ID;
//        OnExpire(session, EVENT_BYE);
//    }
//    else if(session->timer.id_report == timer_id) {
//        session->timer.id_report = XT_INVALID_TIMER_ID;
//        OnExpire(session, EVENT_REPORT);
//    }
//    tsk_safeobj_unlock(session);
//    return 0;
//}

static tsk_bool_t IsRtpPacket(const packet_ p)
{
    return (TSK_OBJECT_HEADER(p)->__def__ == trtp_rtp_packet_def_t);
}

static PacketType_ PacketType(const packet_ p)
{
    if(IsRtpPacket(p)) {
        return PACKET_RTP;
    }
    else {
        switch(((const trtp_rtcp_packet_t*)p)->header->type) {
        case trtp_rtcp_packet_type_bye:
            return PACKET_BYE;
        default:
            return PACKET_RTCP_REPORT;
        }
    }
}

// Returns true if the packet is from a member or not
// also checks all csrc
static tsk_bool_t NewMember(trtp_rtcp_session_t* session, const packet_ p)
{
    uint32_t ssrc = 0;
    if(IsRtpPacket(p)) {
        const trtp_rtp_packet_t* packet_rtp = (const trtp_rtp_packet_t*)p;
//        tsk_size_t i;
        /** comment by mark
        for(i = 0; i < packet_rtp->header->csrc_count && i < sizeof(packet_rtp->header->csrc)/sizeof(packet_rtp->header->csrc[0]); ++i) {
            if(!(_trtp_rtcp_session_have_source(session, packet_rtp->header->csrc[i]))) {
                return tsk_false;
            }
        }**/
        
        if(packet_rtp->header->csrc_count > 0 && !(_trtp_rtcp_session_have_source(session, packet_rtp->header->csrc[0]))) {
            ssrc = packet_rtp->header->ssrc;
        } else {
            return tsk_false;
        }
    }
    else {
        switch(((const trtp_rtcp_packet_t*)p)->header->type) {
        case trtp_rtcp_packet_type_rr:
            ssrc = ((const trtp_rtcp_report_rr_t*)p)->ssrc;
            break;
        case trtp_rtcp_packet_type_sr:
            ssrc = ((const trtp_rtcp_report_sr_t*)p)->ssrc;
            break;
        case trtp_rtcp_packet_type_bye: {
            tsk_size_t i;
            const trtp_rtcp_report_bye_t* bye = (const trtp_rtcp_report_bye_t*)p;
            for(i = 0; i < TRTP_RTCP_PACKET(bye)->header->rc; ++i) {
                if(!_trtp_rtcp_session_have_source(session, bye->ssrc_list[i])) {
                    return tsk_false;
                }
            }
            return tsk_true;
        }
        default:
            return tsk_false;
        }
    }

    return !_trtp_rtcp_session_have_source(session, ssrc);
}

#define NewSender(session, p) NewMember((session), (p))

static tsk_size_t AddMemberUsingRTCPPacket(trtp_rtcp_session_t* session, const trtp_rtcp_packet_t* p, tsk_bool_t sender)
{
    trtp_rtcp_packets_L_t* packets = tsk_null;
    trtp_rtcp_rblocks_L_t* blocks = tsk_null;
    tsk_bool_t added = tsk_false;
    tsk_size_t count = 0;

    switch (p->header->type) {
        case trtp_rtcp_packet_type_rr: {
            const trtp_rtcp_report_rr_t* rr = (const trtp_rtcp_report_rr_t*)p;
            _trtp_rtcp_session_add_source_2(session, ((const trtp_rtcp_report_rr_t*)p)->ssrc, 0, 0, &added);
            if(added) {
                ++count;
            }

            packets = rr->packets;
            blocks = rr->blocks;
            break;
        }
        case trtp_rtcp_packet_type_sr: {
            const trtp_rtcp_report_sr_t* sr = (const trtp_rtcp_report_sr_t*)p;
            _trtp_rtcp_session_add_source_2(session, ((const trtp_rtcp_report_sr_t*)p)->ssrc, 0, 0, &added);
            if(added) {
                ++count;
            }
            packets = sr->packets;
            blocks = sr->blocks;
            break;
        }
        default: {
            break;
        }
    }

    // no use, commenrt by mark
    // if (!sender) {
    //     if (packets) {
    //         const tsk_list_item_t *item;
    //         tsk_list_foreach(item, packets) {
    //             AddMemberUsingRTCPPacket(session, (const trtp_rtcp_packet_t*)item->data, sender);
    //         }
    //     }
    //     if (blocks) {
    //         const tsk_list_item_t *item;
    //         tsk_list_foreach(item, blocks) {
    //             _trtp_rtcp_session_add_source_2(session, TRTP_RTCP_RBLOCK(item->data)->ssrc, PACKET_MEIDA_TYPE_UNKNOW, 0, 0, &added);
    //             if(added) {
    //                 ++count;
    //             }
    //         }
    //     }
    // }

    return count;
}

static tsk_size_t AddMember_(trtp_rtcp_session_t* session, const packet_ p, tsk_bool_t sender)
{
    tsk_size_t count = 0;
    if (IsRtpPacket(p)) {
        const trtp_rtp_packet_t* packet_rtp = (const trtp_rtp_packet_t*)p;
        tsk_size_t i;
        tsk_bool_t added = tsk_false;
        //int32_t media = _trtp_rtcp_check_packet_type(packet_rtp);
        _trtp_rtcp_session_add_source_2(session, packet_rtp->header->ssrc, packet_rtp->header->seq_num, packet_rtp->header->timestamp, &added);
        if (added) {
            ++count;
        }
        
        /** no use, comment by mark
        for (i = 0; i < packet_rtp->header->csrc_count && i < sizeof(packet_rtp->header->csrc)/sizeof(packet_rtp->header->csrc[0]); ++i) {
            _trtp_rtcp_session_add_source_2(session, packet_rtp->header->csrc[i], 0, 0, &added);
            if(added) {
                ++count;
            }
        }**/
    }
    else {
        count += AddMemberUsingRTCPPacket(session, (const trtp_rtcp_packet_t*) p, sender);
    }
    return count;
}

#define AddMember(session, p)	AddMember_((session), (p), tsk_false)
#define AddSender(session, p)	AddMember_((session), (p), tsk_true)


static tsk_size_t RemoveMemberUsingRTCPPacket(trtp_rtcp_session_t* session, const trtp_rtcp_packet_t* p)
{
    trtp_rtcp_packets_L_t* packets = tsk_null;
    trtp_rtcp_rblocks_L_t* blocks = tsk_null;
    tsk_bool_t removed = tsk_false;
    tsk_size_t count = 0;

    switch(p->header->type) {
    case trtp_rtcp_packet_type_rr: {
        const trtp_rtcp_report_rr_t* rr = (const trtp_rtcp_report_rr_t*)p;
        _trtp_rtcp_session_remove_source(session, ((const trtp_rtcp_report_rr_t*)p)->ssrc, &removed);
        if(removed) {
            ++count;
        }

        packets = rr->packets;
        blocks = rr->blocks;
        break;
    }
    case trtp_rtcp_packet_type_sr: {
        const trtp_rtcp_report_sr_t* sr = (const trtp_rtcp_report_sr_t*)p;
        _trtp_rtcp_session_remove_source(session, ((const trtp_rtcp_report_sr_t*)p)->ssrc, &removed);
        if(removed) {
            ++count;
        }
        packets = sr->packets;
        blocks = sr->blocks;
        break;
    }
    default: {
        break;
    }
    }

    if(packets) {
        const tsk_list_item_t *item;
        tsk_list_foreach(item, packets) {
            RemoveMemberUsingRTCPPacket(session, (const trtp_rtcp_packet_t*)item->data);
        }
    }
    if(blocks) {
        const tsk_list_item_t *item;
        tsk_list_foreach(item, blocks) {
            _trtp_rtcp_session_remove_source(session, TRTP_RTCP_RBLOCK(item->data)->ssrc, &removed);
            if(removed) {
                ++count;
            }
        }
    }


    return count;
}

static tsk_size_t RemoveMember(trtp_rtcp_session_t* session, const packet_ p)
{
    tsk_size_t count = 0;
    if(IsRtpPacket(p)) {
        const trtp_rtp_packet_t* packet_rtp = (const trtp_rtp_packet_t*)p;
        tsk_size_t i;
        tsk_bool_t removed = tsk_false;
        _trtp_rtcp_session_remove_source(session, packet_rtp->header->ssrc, &removed);
        if(removed) {
            ++count;
        }
        for(i = 0; i < packet_rtp->header->csrc_count && i < sizeof(packet_rtp->header->csrc)/sizeof(packet_rtp->header->csrc[0]); ++i) {
            _trtp_rtcp_session_remove_source(session, packet_rtp->header->csrc[i], &removed);
            if(removed) {
                ++count;
            }
        }
    }
    else {
        count += RemoveMemberUsingRTCPPacket(session, (const trtp_rtcp_packet_t*) p);
    }
    return count;
}

#define RemoveSender(session, p) RemoveMember((session), (p))

// Sends BYE in synchronous mode
static void SendBYEPacket(trtp_rtcp_session_t* session, event_ e)
{
    trtp_rtcp_report_bye_t* bye;
    tsk_size_t __num_bytes_pad = 0;

    if(!session->remote_addr || session->local_fd <= 0) {
        TSK_DEBUG_ERROR("Invalid network settings");
        return;
    }

    tsk_safeobj_lock(session);

// #if HAVE_SRTP
//     if(session->srtp.session) {
//         __num_bytes_pad = (SRTP_MAX_TRAILER_LEN + 0x4);
//     }
// #endif

    if(session->source_local && (bye = trtp_rtcp_report_bye_create_2(session->source_local->ssrc))) {
        tsk_buffer_t* buffer;
        // serialize and send the packet
        if((buffer = trtp_rtcp_packet_serialize((const trtp_rtcp_packet_t*)bye, __num_bytes_pad))) {
            void* data = buffer->data;
            int size = (int)buffer->size;
// #if HAVE_SRTP
//             if(session->srtp.session) {
//                 if(srtp_protect_rtcp(((srtp_t)*session->srtp.session), data, &size) != err_status_ok) {
//                     TSK_DEBUG_ERROR("srtp_protect_rtcp() failed");
//                 }
//             }
// #endif
            _trtp_rtcp_session_send_raw(session, data, size);
            TSK_OBJECT_SAFE_FREE(buffer);
        }
        TSK_OBJECT_SAFE_FREE(bye);
    }

    tsk_safeobj_unlock(session);
}

// returns sent packet size
static tsk_size_t SendRTCPReport(trtp_rtcp_session_t* session, event_ e)
{
    tsk_size_t ret = 0;
    int video_down_lossrate = 0;
    int video_receive_member = 0;
    tsk_bool_t need_rtcp = tsk_true;
    //一个rtcp中包含多少个成员信息
    int rtcp_member_count = 0;

    uint32_t max_rtcp_member = tmedia_defaults_get_rtcp_memeber_count();
    tsk_safeobj_lock(session);
    
    //超过限定人数，不发送rtcp
    video_receive_member = tsk_list_count_all(session->sources);
    if (video_receive_member >= max_rtcp_member) {
        //tsk_safeobj_unlock(session);
        //return 0;
        need_rtcp = tsk_false;
    }
    else
    {
        need_rtcp = tsk_true;
    }
    
    //本端有发送音视频流，需要发送rtcp
    if (tsk_false == need_rtcp
        && (session->audio_packets_count > 0
        || session->main_video_packets_count > 0
        || session->minor_video_packets_count > 0
        || session->share_video_packets_count > 0)) {
        TMEDIA_I_AM_ACTIVE(10, "need to send rtcp source when has %d members", video_receive_member);
        need_rtcp = tsk_true;
    }
    
    if(tsk_false == need_rtcp)
    {
        TMEDIA_I_AM_ACTIVE(10, "send rtcp source size:%d, too large to send rtcp pkt", video_receive_member);
        tsk_safeobj_unlock(session);
        return 0;
    }

    if(session->initial) {
        // Send Receiver report (manadatory to be the first on)
        trtp_rtcp_report_rr_t* rr = trtp_rtcp_report_rr_create_2(session->source_local->ssrc);
        if(rr) {
            // serialize and send the packet
            ret = _trtp_rtcp_session_send_pkt(session, (trtp_rtcp_packet_t*)rr);
            TSK_OBJECT_SAFE_FREE(rr);
        }
    } else {
        trtp_rtcp_report_sr_t* sr = trtp_rtcp_report_sr_create_null();
        uint32_t media_ssrc_list[16] = {0};
        uint32_t media_ssrc_list_count = 0;
        if(sr) {
            uint64_t time_now = tsk_time_now();
            uint64_t ntp_now = time_now;    // 目前video rtp的时间戳采用ms单位，所以无需转换
            
            trtp_rtcp_rblock_t* rblock;
            trtp_rtcp_source_t* source;
            tsk_list_item_t *item;
            tsk_bool_t packet_lost = tsk_false;

            if (session->main_video_packets_count || session->minor_video_packets_count) {
                session->video_pkt_send_stat = 1;
            } else {
                session->video_pkt_send_stat = 0; 
            }

            // sender info
            sr->ssrc = session->source_local->ssrc;
            sr->sender_info.ntp_msw = ((ntp_now >> 16) & 0xFFFFFFFF); // 发送时间取48位，msw存储高32位，lsw存储低16位，兼容之前的ntp逻辑
            sr->sender_info.ntp_lsw = ((ntp_now << 16) & 0xFFFFFFFF); // 使用高16位
            sr->sender_info.audio_sender_pcount = session->audio_packets_count;
            sr->sender_info.audio_sender_ocount = session->audio_octets_count;
            sr->sender_info.minor_video_sender_pcount = session->minor_video_packets_count;
            sr->sender_info.minor_video_sender_ocount = session->minor_video_octets_count;
            sr->sender_info.video_sender_pcount = session->main_video_packets_count;
            sr->sender_info.video_sender_ocount = session->main_video_octets_count;

            // 存储音视频即时发送码率
            session->audio_send_instant_bitrate = session->audio_octets_count;
            session->video_send_camera_instant_bitrate = session->main_video_octets_count + session->minor_video_octets_count;
            session->video_send_share_instant_bitrate = session->share_video_octets_count;

            // 记录当前发送流种类
            session->video_send_stream_stat = 0;
            session->video_send_stream_stat = (session->main_video_packets_count > 0) ? (session->video_send_stream_stat | 0x1) : session->video_send_stream_stat;
            session->video_send_stream_stat = (session->minor_video_packets_count > 0) ? (session->video_send_stream_stat | 0x2) : session->video_send_stream_stat;
            session->video_send_stream_stat = (session->share_video_packets_count > 0) ? (session->video_send_stream_stat | 0x4) : session->video_send_stream_stat;
                      
            {	/* rtp_timestamp */
                struct timeval tv;
                uint64_t rtp_timestamp = (time_now - session->time_start) * (session->source_local->audio_rate / 1000); // notice by mark
                tv.tv_sec = (long)(rtp_timestamp / 1000);
                tv.tv_usec = (long)(rtp_timestamp - ((rtp_timestamp / 1000) * 1000)) * 1000;
                sr->sender_info.rtp_timestamp = (uint32_t)tsk_time_get_ms(&tv);

            }

            TMEDIA_I_AM_ACTIVE(10, "send rtcp source:%d, main[%d][%.1fkbps] minor[%d][%.1fkbps] share[%d][%.1fkbps] audio[%d][%.1fkbps]"
                , video_receive_member, session->main_video_packets_count, 8*session->main_video_octets_count/1000.0
                , session->minor_video_packets_count, 8*session->minor_video_packets_count/1000.0
                , session->share_video_packets_count, 8*session->share_video_octets_count/1000.0
                , session->audio_packets_count, 8*session->audio_octets_count/1000.0);

            session->audio_packets_count   = 0;
            session->audio_octets_count    = 0;            
            session->main_video_packets_count   = 0;
            session->main_video_octets_count    = 0;
            session->minor_video_packets_count  = 0;
            session->minor_video_octets_count   = 0;
            session->share_video_packets_count  = 0;
            session->share_video_octets_count   = 0;

            // report blocks
            tsk_list_foreach(item, session->sources) {
                if(!(source = (trtp_rtcp_source_t*)item->data)) {
                    continue;
                }
                
                //rtcp只携带配置的最大用户数信息
                ++rtcp_member_count;
                
                if(rtcp_member_count >= max_rtcp_member)
                {
                    continue;
                }
                
                if((rblock = trtp_rtcp_rblock_create_null())) {
                    int32_t expected = 0, expected_interval = 0, received_interval = 0, lost_interval = 0;

                    rblock->ssrc = source->ssrc;
                    // RFC 3550 - A.3 Determining Number of Packets Expected and Lost
                    // rblock audio info
                    expected = (source->audio_cycles + source->audio_max_seq) - source->audio_base_seq + 1;
                    expected_interval = expected - source->audio_expected_prior;
                    source->audio_expected_prior = expected;
                    received_interval = source->audio_received - source->audio_received_prior;
                    source->audio_received_prior = source->audio_received;
                    lost_interval = expected_interval - received_interval;
                    if (expected_interval == 0 || lost_interval <= 0) {
                        rblock->audio_fraction = 0;
                    }
                    else {
                        rblock->audio_fraction = (lost_interval << 8) / expected_interval;
                    }

                    rblock->audio_cumulative_no_lost = ((expected - source->audio_received));
                 
                    if(!packet_lost && rblock->audio_fraction) {
                        packet_lost = tsk_true;
                    }

                    rblock->audio_last_seq = ((source->audio_cycles & 0xFFFF) << 16) | source->audio_max_seq;
                    rblock->audio_jitter = (source->audio_jitter_q4 >> 4);
    
                    // rblock video info
                    expected = 0, expected_interval = 0, received_interval = 0, lost_interval = 0;
                    expected = (source->video_cycles + source->video_max_seq) - source->video_base_seq + 1;
                    expected_interval = expected - source->video_expected_prior;
                    source->video_expected_prior = expected;
                    received_interval = source->video_received - source->video_received_prior;
                    source->video_received_prior = source->video_received;
                    lost_interval = expected_interval - received_interval;
                    if (expected_interval == 0 || lost_interval <= 0) {
                        rblock->video_fraction = 0;
                    }
                    else {
                        rblock->video_fraction = (lost_interval << 8) / expected_interval;
                    }

                    rblock->video_cumulative_no_lost = ((expected - source->video_received));
                   
                    if(!packet_lost && rblock->video_fraction) {
                        packet_lost = tsk_true;
                    }
                    
                    rblock->video_last_seq = ((source->video_cycles & 0xFFFF) << 16) | source->video_max_seq;
                    rblock->video_jitter = (source->video_jitter_q4 >> 4);
                    
                    // last sr
                    rblock->lsr = ((source->ntp_msw & 0xFFFF) << 16) | ((source->ntp_lsw & 0xFFFF0000) >> 16);
                    if(source->dlsr) {
                        rblock->dlsr = (uint32_t)(((time_now - source->dlsr) * 65536) / 1000); // in units of 1/65536 seconds
                    }

                    // audio downlink qos
                    rblock->audio_fraction = ((rblock->audio_fraction < 256) ? rblock->audio_fraction : 0);
                    source->audio_fraction = rblock->audio_fraction;
                    if (source->audio_max_fraction < rblock->audio_fraction) {
                        source->audio_max_fraction = rblock->audio_fraction;
                    }

                    if (source->audio_max_jitter_q4 < source->audio_jitter_q4) {
                        source->audio_max_jitter_q4 = source->audio_jitter_q4;
                    }                    

                    // video downlink qos
                    rblock->video_fraction = ((rblock->video_fraction < 256) ? rblock->video_fraction : 0);
                    source->video_fraction = rblock->video_fraction;
                    if (source->video_max_fraction < rblock->video_fraction) {
                        source->video_max_fraction = rblock->video_fraction;
                    }

                    if (source->video_max_jitter_q4 < source->video_jitter_q4) {
                        source->video_max_jitter_q4 = source->video_jitter_q4;
                    }

                    // 大小流切换时，下行丢包率统计异常，pass
                    // 远端开启共享流发送时，不处理
                    if (!source->video_share_recv_instant_bitrate && (source->video_id != source->last_video_id)) {
                        source->video_fraction = 0;
                        source->video_max_fraction = 0;

                        source->last_video_id = source->video_id;
                        rblock->video_fraction = 0;
                    }

                    // TSK_DEBUG_INFO("RTCP Receiver: sessionid[%u], a_loss=%.1lf%%, v_loss=%.1lf%%, a_jitter=%d, v_jitter=%d", 
                    //     source->ssrc, 100*rblock->audio_fraction/256.0, 100*rblock->video_fraction/256.0, rblock->audio_jitter,rblock->video_jitter);
                    
                    video_down_lossrate = 100*rblock->video_fraction/256.0;

                    trtp_rtcp_report_sr_add_block(sr, rblock);
                    TSK_OBJECT_SAFE_FREE(rblock);

                    // 统计连续丢包率情况是否切换大小流接收
                    int32_t source_type = -1;
                    if (video_down_lossrate >= 10) {
                        source->video_down_loss_cnt++;
                    } else {
                        source->video_down_loss_cnt = 0;
                    }

                    // 连续3个周期丢包率大于10%则切换到小流
                    // 切换大小流之前要确认对方相应的大小流正在发送数据
                    if (source->video_down_loss_cnt >= 3 && 0 == source->video_source_type
                        && source->video_minor_stream_count > 0) {
                        // switch to minor
                        source_type = 1;
                    } else if (!source->video_down_loss_cnt && 1 == source->video_source_type
                        && source->video_main_stream_count > 0 && !source->video_last_lossrate) {
                        // switch to main
                        source_type = 0;
                    }

                    int mode = tmedia_defaults_get_video_net_adjustmode();

                    // 大小流接收调整模式
                    //  mode  0:自动调整; 1:上层手动调整
                    if (session->switch_cb && !mode) {

                        // 自动调整模式下连续3个tick丢包10%以上切换到小流，并且在网络恢复时切换回大流
                        // 手动模式下不自动调整大小流接收
                        if (1 == source_type || (0 == source->video_last_source_type && 0 == source_type)) {
                            session->switch_cb(session->switch_cb_data, source->ssrc, source_type);
                            source->video_source_type = source_type;
                        }
                    }
                    
                    source->video_last_lossrate = video_down_lossrate;

                    // trtp_rtcp_report_sr_add_block(sr, rblock);
                    // TSK_OBJECT_SAFE_FREE(rblock);
                }

                if((media_ssrc_list_count + 1) < sizeof(media_ssrc_list)/sizeof(media_ssrc_list[0])) {
                    media_ssrc_list[media_ssrc_list_count++] = source->ssrc;
                }
            }

            // // If at least one member has a packet loss rate of 0
            // if (!video_receive_member) {
            //     Config_SetInt("video_down_loss_all", 1);
            // }
            //comment by mark, no use now
            // if(media_ssrc_list_count > 0) {
            //     // draft-alvestrand-rmcat-remb-02
            //     if(session->app_bw_max_download > 0 && session->app_bw_max_download != INT_MAX) { // INT_MAX or <=0 means undefined
            //         // app_bw_max_download unit is kbps while create_afb_remb() expect bps
            //         trtp_rtcp_report_psfb_t* psfb_afb_remb = trtp_rtcp_report_psfb_create_afb_remb(session->source_local->ssrc/*sender SSRC*/, media_ssrc_list, media_ssrc_list_count, (session->app_bw_max_download * 1024));
            //         if(psfb_afb_remb) {
            //             TSK_DEBUG_INFO("Packing RTCP-AFB-REMB (bw_dwn=%d kbps) for outgoing RTCP-SR", session->app_bw_max_download);
            //             trtp_rtcp_packet_add_packet((trtp_rtcp_packet_t*)sr, (trtp_rtcp_packet_t*)psfb_afb_remb, tsk_false);
            //             TSK_OBJECT_SAFE_FREE(psfb_afb_remb);
            //         }
            //     }
            // }

            // serialize and send the packet
            ret = _trtp_rtcp_session_send_pkt(session, (trtp_rtcp_packet_t*)sr);
            TSK_OBJECT_SAFE_FREE(sr);
        }
    }

    tsk_safeobj_unlock(session);
    return ret;
}

static void Schedule(trtp_rtcp_session_t* session, double tn, event_ e)
{
    tsk_safeobj_lock(session); // must
    switch(e) {
    case EVENT_BYE:
        if(!XT_TIMER_ID_IS_VALID(session->timer.id_bye)) {
            session->timer.id_bye = xt_timer_mgr_global_schedule_loop((uint64_t)tn, _trtp_rtcp_session_timer_callback, session);
        }
        break;
    case EVENT_REPORT:
        if(!XT_TIMER_ID_IS_VALID(session->timer.id_report)) {
            session->timer.id_report = xt_timer_mgr_global_schedule_loop((uint64_t)tn, _trtp_rtcp_session_timer_callback, session);
        }
        break;
    default:
        TSK_DEBUG_ERROR("Unexpected code called");
        break;
    }
    tsk_safeobj_unlock(session);
}

#define Reschedule(session, tn, e) Schedule((session), (tn), (e))

static double rtcp_interval(int32_t members,
                            int32_t senders,
                            double rtcp_bw,
                            int32_t we_sent,
                            double avg_rtcp_size,
                            tsk_bool_t initial)
{
    /*
    * Minimum average time between RTCP packets from this site (in
    * seconds).  This time prevents the reports from `clumping' when
    * sessions are small and the law of large numbers isn't helping
    * to smooth out the traffic.  It also keeps the report interval
    * from becoming ridiculously small during transient outages like
    * a network partition.
    */
#define RTCP_MIN_TIME 5.
    /*
    * Fraction of the RTCP bandwidth to be shared among active
    * senders.  (This fraction was chosen so that in a typical
    * session with one or two active senders, the computed report
    * time would be roughly equal to the minimum report time so that
    * we don't unnecessarily slow down receiver reports.)  The
    * receiver fraction must be 1 - the sender fraction.
    */
#define RTCP_SENDER_BW_FRACTION 0.25
#define RTCP_RCVR_BW_FRACTION (1 - RTCP_SENDER_BW_FRACTION)
    /*
    * To compensate for "timer reconsideration" converging to a
    * value below the intended average.
    */
#define COMPENSATION  (2.71828 - 1.5)

    double t;                   /* interval */
    double rtcp_min_time = RTCP_MIN_TIME;
    int n;                      /* no. of members for computation */

    /*
    * Very first call at application start-up uses half the min
    * delay for quicker notification while still allowing some time
    * before reporting for randomization and to learn about other
    * sources so the report interval will converge to the correct
    * interval more quickly.
    */
    if (initial) {
        rtcp_min_time /= 2;
    }
    /*
    * Dedicate a fraction of the RTCP bandwidth to senders unless
    * the number of senders is large enough that their share is
    * more than that fraction.
    */
    n = members;
    if (senders <= members * RTCP_SENDER_BW_FRACTION) {
        if (we_sent) {
            rtcp_bw *= RTCP_SENDER_BW_FRACTION;
            n = senders;
        }
        else {
            rtcp_bw *= RTCP_RCVR_BW_FRACTION;
            n -= senders;
        }
    }

    /*
    * The effective number of sites times the average packet size is
    * the total number of octets sent when each site sends a report.
    * Dividing this by the effective bandwidth gives the time
    * interval over which those packets must be sent in order to
    * meet the bandwidth target, with a minimum enforced.  In that
    * time interval we send one report so this time is also our
    * average time between reports.
    */
    t = avg_rtcp_size * n / rtcp_bw;
    if (t < rtcp_min_time) {
        t = rtcp_min_time;
    }

    /*
    * To avoid traffic bursts from unintended synchronization with
    * other sites, we then pick our actual next report interval as a
    * random number uniformly distributed between 0.5*t and 1.5*t.
    */
    t = t * (drand48() + 0.5);
    t = t / COMPENSATION;

    //return (t * 1000);

    return 1000;   // set rtcp interval to 1s by mark
}


static void OnExpire(trtp_rtcp_session_t* session, event_ e)
{
    /* This function is responsible for deciding whether to send an
    * RTCP report or BYE packet now, or to reschedule transmission.
    * It is also responsible for updating the pmembers, initial, tp,
    * and avg_rtcp_size state variables.  This function should be
    * called upon expiration of the event timer used by Schedule().
    */

    double t;     /* Interval */
    double tn;    /* Next transmit time */
    double tc;

    /* In the case of a BYE, we use "timer reconsideration" to
    * reschedule the transmission of the BYE if necessary */

    if (TypeOfEvent(e) == EVENT_BYE) {
        t = rtcp_interval(session->members,
                          session->senders,
                          session->rtcp_bw,
                          session->we_sent,
                          session->avg_rtcp_size,
                          session->initial);
        tn = session->tp + t;
        if (tn <= session->tc()) {
            SendBYEPacket(session, e);
#if 0
            exit(1);
#endif
        }
        else {
#if 0
            Schedule(session, tn, e);
#else
            Schedule(session, 0, e);
#endif
        }

    }
    else if (TypeOfEvent(e) == EVENT_REPORT) {
        t =  rtcp_interval(session->members,
                          session->senders,
                          session->rtcp_bw,
                          session->we_sent,
                          session->avg_rtcp_size,
                          session->initial);
        tn = session->tp + t;
        if (tn <= (tc = session->tc())) {
            tsk_size_t SentPacketSize = SendRTCPReport(session, e);
            session->avg_rtcp_size = (1./16.)*SentPacketSize + (15./16.)*(session->avg_rtcp_size);
            session->tp = tc;

            /* We must redraw the interval.  Don't reuse the
            one computed above, since its not actually
            distributed the same, as we are conditioned
            on it being small enough to cause a packet to
            be sent */

            t = rtcp_interval(session->members,
                              session->senders,
                              session->rtcp_bw,
                              session->we_sent,
                              session->avg_rtcp_size,
                              session->initial);
#if 0
            Schedule(session, t+tc, e);
#else
            Schedule(session, t, e);
#endif
            session->initial = tsk_false;
        }
        else {
#if 0
            Schedule(session, tn, e);
#else
            Schedule(session, 0, e);
#endif
        }
        session->pmembers = session->members;
    }
}

static void OnReceive(trtp_rtcp_session_t* session, const packet_ p, event_ e, tsk_size_t ReceivedPacketSize)
{
    /* What we do depends on whether we have left the group, and are
    * waiting to send a BYE (TypeOfEvent(e) == EVENT_BYE) or an RTCP
    * report.  p represents the packet that was just received.  */

    if (PacketType(p) == PACKET_RTCP_REPORT) {
        if (NewMember(session, p) && (TypeOfEvent(e) == EVENT_REPORT)) {
            tsk_size_t count = AddSender(session, p);
            session->senders += (int32_t)count;
            session->members += (int32_t)count;
        }
        session->avg_rtcp_size = (1./16.)*ReceivedPacketSize + (15./16.)*(session->avg_rtcp_size);
    }
    else if (PacketType(p) == PACKET_RTP) {

        if (NewSender(session, p)) {
            tsk_size_t count = AddSender(session, p);
            session->senders += (int32_t)count;
            session->members += (int32_t)count;
        }
    }
    else if (PacketType(p) == PACKET_BYE) {
        session->avg_rtcp_size = (1./16.)*ReceivedPacketSize + (15./16.)*(session->avg_rtcp_size);

        if (TypeOfEvent(e) == EVENT_REPORT) {
            double tc = session->tc();
            tsk_size_t count = RemoveMember(session, p);
            session->senders -= (int32_t)count;
            session->members -= (int32_t)count;
#if 0
            if (NewSender(session, p) == tsk_false) {
                RemoveSender(p);
                session->senders -= 1;
            }
            if (NewMember(session, p) == tsk_false) {
                RemoveMember(p);
                session->members -= 1;
            }
#endif

            if (session->members < session->pmembers && session->pmembers) {
                session->tn = (time_tp)(tc +
                                        (((double) session->members)/(session->pmembers))*(session->tn - tc));
                session->tp = (time_tp)(tc -
                                        (((double) session->members)/(session->pmembers))*(tc - session->tp));

                /* Reschedule the next report for time tn */

                Reschedule(session, session->tn, e);
                session->pmembers = session->members;
            }

        }
        else if (TypeOfEvent(e) == EVENT_BYE) {
            session->members += 1;
        }
    }
}

int trtp_rtcp_session_get_session_qos(struct trtp_rtcp_session_s* self, struct trtp_rtcp_stat_qos_s* stat_qos)
{
    // sanity check
    if (!self || !stat_qos) {
        TSK_DEBUG_ERROR("input parameter sanity check invalid when get rtt");
        return -1;
    }

    int ret = 0;
    trtp_rtcp_source_t* source = tsk_null;
    uint32_t rtt = 0, a_down_fraction = 0, v_down_fraction = 0; 
    uint32_t a_down_bitrate = 0, v_down_bitrate = 0, v_down_share_bitrate = 0;
    
    tsk_list_item_t* item;
    tsk_list_foreach(item, self->sources) {
        source = TRTP_RTCP_SOURCE(item->data);

        // 多路下行丢包率和RTT取最大值
        rtt = (rtt < source->rtt) ?  source->rtt : rtt;
        a_down_fraction = (a_down_fraction < source->audio_fraction) ?  source->audio_fraction : a_down_fraction;
        v_down_fraction = (v_down_fraction < source->video_fraction) ?  source->video_fraction : v_down_fraction;

        // 多路下行带宽统计取和
        a_down_bitrate += source->audio_recv_instant_bitrate;
        v_down_bitrate += source->video_recv_instant_bitrate;
        v_down_share_bitrate += source->video_share_recv_instant_bitrate;
    }

    // rtt && lossrate
    stat_qos->rtt = rtt;
    stat_qos->a_down_fraction = a_down_fraction;
    stat_qos->v_down_fraction = v_down_fraction;

    // 下行带宽
    stat_qos->a_down_bitrate = a_down_bitrate;
    stat_qos->v_down_bitrate = v_down_bitrate;
    stat_qos->v_down_share_bitrate = v_down_share_bitrate;

    // 上行带宽
    stat_qos->a_up_bitrate = self->audio_send_instant_bitrate;
    stat_qos->v_up_camera_bitrate = self->video_send_camera_instant_bitrate;
    stat_qos->v_up_share_bitrate = self->video_send_share_instant_bitrate;

    stat_qos->v_up_stream = self->video_send_stream_stat;
    return ret;
}

int trtp_rtcp_session_get_source_qos(struct trtp_rtcp_session_s* self, uint32_t ssrc, const trtp_rtcp_rblock_t* block, struct trtp_rtcp_stat_qos_s* stat_qos) {
    int index = 0;
    int rtt = 0;
    uint32_t last_sr_send_time = 0;

    // sanity check
    if (!self || !block || !stat_qos) {
        TSK_DEBUG_ERROR("input parameter sanity check invalid when get rtt");
        return -1;
    }
    
    int ret = 0;
    uint64_t now_timestamp = tsk_time_now();
    tsk_safeobj_lock(self);
    trtp_rtcp_source_t* source = _trtp_rtcp_session_find_source(self, ssrc);

    do {
        if (block->lsr == 0) {
            TMEDIA_I_AM_ACTIVE_ERROR(5, "lsr is zero or send sr count is zero, remote ssrc:%u", ssrc);
            ret = -2;
            break;
        }

        if (!source) {
            TMEDIA_I_AM_ACTIVE_ERROR(5, "can not find source which ssrc: %u", ssrc);
            ret =  -3;
            break;
        }

        last_sr_send_time = block->lsr;
        if (!last_sr_send_time) {
            TMEDIA_I_AM_ACTIVE_ERROR(5, "can not find last sr send time, remote ssrc:%u", ssrc);
            ret = -4;
            break;
        }

        // calculate remote process time to estimate rtt
        uint32_t d = ((uint64_t)(block->dlsr) * 1000 / 65536);
        uint32_t time_now = (uint32_t)now_timestamp;
        rtt = time_now - d - last_sr_send_time;

        if (rtt > source->max_rtt) {
            source->max_rtt = rtt;
        }

        // calculate min, max, avg rtt
        if (source->min_rtt == 0) {
            source->min_rtt = rtt;
        } else if (rtt < source->min_rtt) {
            source->min_rtt = rtt;
        }
        source->rtt = rtt;
        if (source->num_average_calcs != 0) {
            float ac = (float) source->num_average_calcs;
            float new_average = ((ac / (ac + 1)) * source->avg_rtt) + ((1 / (ac + 1)) * source->rtt);
            source->avg_rtt = (int64_t) (new_average + 0.5f);
        } else {
            source->avg_rtt = rtt;
        }
        source->num_average_calcs++;

        // rtt
        stat_qos->rtt = source->rtt;
        stat_qos->avg_rtt = source->avg_rtt;
        stat_qos->min_rtt = source->min_rtt;
        stat_qos->max_rtt = source->max_rtt;

        //  下行丢包率、jitter统计
        stat_qos->a_down_fraction = (source->audio_fraction < 256 ? source->audio_fraction : 0);
        stat_qos->a_down_max_fraction = (source->audio_max_fraction < 256 ? source->audio_max_fraction : 0);
        stat_qos->a_jitter = (source->audio_jitter_q4 >> 4);
        stat_qos->a_max_jitter = (source->audio_max_jitter_q4 >> 4);

        stat_qos->v_down_fraction = (source->video_fraction < 256 ? source->video_fraction : 0);
        stat_qos->v_down_max_fraction = (source->video_max_fraction < 256 ? source->video_max_fraction : 0);
        stat_qos->v_jitter = (source->video_jitter_q4 >> 4);
        stat_qos->v_max_jitter = (source->video_max_jitter_q4 >> 4);
    } while (0);
    
    // 统计接收码率
    int32_t tick_interval = now_timestamp - source->video_recv_last_stat_timestamp;
    if (source->video_recv_last_stat_timestamp && tick_interval) {
        source->video_recv_instant_bitrate = source->video_recv_stat_bytes * 1000 / tick_interval;
        source->video_recv_stat_bytes = 0;
    }

    int32_t tick_interval_share = now_timestamp - source->video_share_recv_last_stat_timestamp;
    if (source->video_share_recv_last_stat_timestamp && tick_interval_share) {
        source->video_share_recv_instant_bitrate = source->video_share_recv_stat_bytes * 1000 / tick_interval_share;
        source->video_share_recv_stat_bytes = 0;
    }

    int32_t tick_interval_audio = now_timestamp - source->audio_recv_last_stat_timestamp;
    if (source->audio_recv_last_stat_timestamp && tick_interval_audio) {
        source->audio_recv_instant_bitrate = source->audio_recv_stat_bytes * 1000 / tick_interval_audio;
        source->audio_recv_stat_bytes = 0;
    }

    // 下行带宽统计
    stat_qos->v_main_count = source->video_main_stream_count;
    stat_qos->v_minor_count = source->video_minor_stream_count;
    stat_qos->a_down_bitrate = source->audio_recv_instant_bitrate;
    stat_qos->v_down_bitrate = source->video_recv_instant_bitrate;
    stat_qos->v_down_share_bitrate = source->video_share_recv_instant_bitrate;

    tsk_object_unref(source);
    tsk_safeobj_unlock(self);

    return ret;
}

int trtp_rtcp_session_get_source_info(struct trtp_rtcp_session_s* self, uint32_t ssrc, uint32_t* sendstatus, uint32_t* source_type) {
    int ret = 0;

    // sanity check
    if (!self || !sendstatus || !source_type) {
        TSK_DEBUG_ERROR("input parameter sanity check invalid when get source info");
        return -1;
    }

    tsk_safeobj_lock(self);
    trtp_rtcp_source_t* source = _trtp_rtcp_session_find_source(self, ssrc);
    do {
        *sendstatus = 0;
        *source_type = 0;

        if (!source) {
            TSK_DEBUG_ERROR("can not find source which ssrc: %u", ssrc);
            ret = -2;
            break;
        }

        if (source->video_main_stream_count > 0) {
            *sendstatus |= 0x1;
        }

        if (source->video_minor_stream_count > 0) {
            *sendstatus |= 0x2;
        }

        // 如果没有收到rtcp则认为远端同时发送大小流
        if (*sendstatus == 0) {
            *sendstatus = 0x3;
        }

        *source_type = source->video_source_type;

        TSK_DEBUG_INFO("trtp_rtcp_session_get_source_info ssrc:%u, sendstatus:0x%x, source:%u", ssrc, *sendstatus, *source_type);
    } while (0);
    tsk_object_unref(source);
    tsk_safeobj_unlock(self);

    return ret;
}

int trtp_rtcp_session_set_source_type(struct trtp_rtcp_session_s* self, uint32_t ssrc, uint32_t source_type) {
    int ret = 0;

    // sanity check
    if (!self) {
        TSK_DEBUG_ERROR("input parameter sanity check invalid when set source type");
        return -1;
    }

    tsk_safeobj_lock(self);
    do {
        trtp_rtcp_source_t* source = _trtp_rtcp_session_find_source(self, ssrc);
        if (!source) {
            TSK_DEBUG_ERROR("can not find source which ssrc: %u", ssrc);
            ret = -2;
            break;
        }

        if (0 == source_type || 1 == source_type) {
            TSK_DEBUG_INFO("trtp_rtcp_session_set_source_type ssrc:%u, source:%u", ssrc, source_type);
            source->video_source_type = source_type;
            source->video_last_source_type = source->video_source_type;
        } else {
            TSK_DEBUG_ERROR("input source type[%u] invalid", source_type);
        }

    } while (0);

    tsk_safeobj_unlock(self);

    return ret;
}

int trtp_rtcp_session_get_video_sendstat(struct trtp_rtcp_session_s* self) {

    if (!self) {
        return 1;   // 获取失败时默认视频发送
    }
    return self->video_pkt_send_stat;
}

int trtp_rtcp_session_get_source_count(struct trtp_rtcp_session_s* self) {

    if (!self || !self->sources) {
        TSK_DEBUG_ERROR("invalid parameter");
        return -1;
    }

    return tsk_list_count_all(self->sources);
}

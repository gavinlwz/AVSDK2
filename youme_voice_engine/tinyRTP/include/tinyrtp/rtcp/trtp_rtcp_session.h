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
/**@file trtp_rtcp_session.h
 * @brief RTCP session.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *
 */
#ifndef TINYMEDIA_RTCP_SESSION_H
#define TINYMEDIA_RTCP_SESSION_H

#include "tinyrtp_config.h"
#include "trtp_rtcp_rblock.h"
//#include "tinyrtp/trtp_srtp.h"

#include "tnet_types.h"

#include "tsk_common.h"

TRTP_BEGIN_DECLS

struct trtp_rtcp_packet_s;
struct trtp_rtp_packet_s;
struct tnet_ice_ctx_s;
struct tnet_transport_s;

typedef struct trtp_rtcp_stat_qos_s {
    // 端到端全程RTT
    uint32_t rtt;       // realtime rtt, 针对session, 则是取下行多路中最大的rtt
    uint32_t avg_rtt;
    uint32_t min_rtt;
    uint32_t max_rtt;

    // 下行统计
    uint32_t a_down_fraction;       // 针对source是当前端到端实时音频loss，针对session则是下行多路中的平均值
    uint32_t a_down_max_fraction;
    uint32_t a_jitter;
    uint32_t a_max_jitter;
    
    uint32_t v_down_fraction;       // 针对source是当前端到端实时视频loss，针对session则是下行多路中的平均值
    uint32_t v_down_max_fraction;
    uint32_t v_jitter;
    uint32_t v_max_jitter;
    
    uint32_t v_main_count;          // 该source在当前rtcp周期内发送main视频包数量
    uint32_t v_minor_count;         // 该source在当前rtcp周期内发送ninor视频包数量

    uint32_t a_down_bitrate;        // 针对source是当前端到端实时音频下行带宽，针对session则是下行多路中的音频带宽之和
    uint32_t v_down_bitrate;        // 针对source是当前端到端实时camera视频下行带宽，针对session则是下行多路中的camera视频带宽之和
    uint32_t v_down_share_bitrate;  // 针对source是当前端到端实时share视频下行带宽，针对session则是下行多路中的share视频带宽之和·

    // 上行统计
    uint32_t a_up_bitrate;
    uint32_t v_up_camera_bitrate;
    uint32_t v_up_share_bitrate;

    uint32_t v_up_stream;   // 视频上行流统计，bit0:大流，bit1:小流，bit2:share
} trtp_rtcp_stat_qos_t;

typedef int (*trtp_rtcp_cb_f)(const void* callback_data, const struct trtp_rtcp_packet_s* packet);
typedef int (*trtp_rtcp_jb_cb_f)(const void* callback_data, const struct video_jitter_nack_s* nack);
typedef int (*video_switch_cb_f)(const void* callback_data, int32_t sessionId, int32_t source_type);

struct trtp_rtcp_session_s* trtp_rtcp_session_create(uint32_t ssrc, const char* cname);
struct trtp_rtcp_session_s* trtp_rtcp_session_create_2(struct tnet_ice_ctx_s* ice_ctx, uint32_t ssrc, const char* cname);
int trtp_rtcp_session_set_callback(struct trtp_rtcp_session_s* self, trtp_rtcp_cb_f callback, const void* callback_data);
int trtp_rtcp_session_set_video_swicth_callback(struct trtp_rtcp_session_s* self, video_switch_cb_f callback, const void* callback_data);

int trtp_rtcp_session_set_app_bw_and_jcng(struct trtp_rtcp_session_s* self, int32_t bw_upload_kbps, int32_t bw_download_kbps, float jcng_q);
int trtp_rtcp_session_start(struct trtp_rtcp_session_s* self, tnet_fd_t local_fd, const struct sockaddr* remote_addr);
int trtp_rtcp_session_stop(struct trtp_rtcp_session_s* self);
int trtp_rtcp_session_process_rtp_out(struct trtp_rtcp_session_s* self, const struct trtp_rtp_packet_s* packet_rtp, tsk_size_t size);
int trtp_rtcp_session_process_rtp_in(struct trtp_rtcp_session_s* self, const struct trtp_rtp_packet_s* packet_rtp, tsk_size_t size);
int trtp_rtcp_session_process_rtcp_in(struct trtp_rtcp_session_s* self, const void* buffer, tsk_size_t size);
int trtp_rtcp_session_signal_pkt_loss_plus(struct trtp_rtcp_session_s* self, uint32_t ssrc_media,const uint32_t* time_stamp,tsk_size_t count);
int trtp_rtcp_session_signal_frame_corrupted(struct trtp_rtcp_session_s* self, uint32_t ssrc_media);
int trtp_rtcp_session_signal_jb_error(struct trtp_rtcp_session_s* self, uint32_t ssrc_media);

tnet_fd_t trtp_rtcp_session_get_local_fd(const struct trtp_rtcp_session_s* self);
int trtp_rtcp_session_set_local_fd(struct trtp_rtcp_session_s* self, tnet_fd_t local_fd);
int trtp_rtcp_session_set_net_transport(struct trtp_rtcp_session_s* self, struct tnet_transport_s* transport);
int trtp_rtcp_session_set_p2p_transport(struct trtp_rtcp_session_s* self, struct tnet_transport_s* transport, tnet_fd_t local_fd, const struct sockaddr* remote_addr);
void trtp_rtcp_session_enable_p2p_transport(struct trtp_rtcp_session_s* self, tsk_bool_t enable);

int trtp_rtcp_session_get_session_qos(struct trtp_rtcp_session_s* self, struct trtp_rtcp_stat_qos_s* stat_qos);
int trtp_rtcp_session_get_source_qos(struct trtp_rtcp_session_s* self, uint32_t ssrc, const trtp_rtcp_rblock_t* block, struct trtp_rtcp_stat_qos_s* stat_qos);
int trtp_rtcp_session_remove_member(struct trtp_rtcp_session_s* self, uint32_t ssrc);

int trtp_rtcp_session_get_source_info(struct trtp_rtcp_session_s* self, uint32_t ssrc, uint32_t* sendstatus, uint32_t* source_type);
int trtp_rtcp_session_set_source_type(struct trtp_rtcp_session_s* self, uint32_t ssrc, uint32_t source_type);

int trtp_rtcp_session_get_video_sendstat(struct trtp_rtcp_session_s* self);

int trtp_rtcp_session_get_source_count(struct trtp_rtcp_session_s* self);
TRTP_END_DECLS

#endif /* TINYMEDIA_RTCP_SESSION_H */

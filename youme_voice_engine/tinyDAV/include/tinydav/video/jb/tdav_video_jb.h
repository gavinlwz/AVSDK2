/*
* Copyright (C) 2012 Doubango Telecom <http://www.doubango.org>
*
* Contact: Mamadou Diop <diopmamadou(at)doubango(DOT)org>
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

/**@file tdav_video_jb.h
 * @brief Video Jitter Buffer
 *
 * @author Mamadou Diop <diopmamadou(at)doubango(DOT)org>
 */
#ifndef TINYDAV_VIDEO_JB_H
#define TINYDAV_VIDEO_JB_H

#include "tinydav_config.h"
#include "tinydav/video/tdav_session_video.h"
#include "tinydav/video/jb/youme_video_jitter.h"

#include "tinymedia/tmedia_jitterbuffer.h"
#include "tinyrtp/rtcp/trtp_rtcp_report_fb.h"

#include "tsk_buffer.h"
#include "tsk_time.h"
#include "tsk_list.h"
#include "tsk_safeobj.h"

#include <list>
#include <map>

TDAV_BEGIN_DECLS

typedef enum tdav_video_jb_cb_data_type_e
{
	tdav_video_jb_cb_data_type_rtp,
	tdav_video_jb_cb_data_type_fl, // frame lost
	tdav_video_jb_cb_data_type_tmfr, // too many frames removed
	tdav_video_jb_cb_data_type_fdd, // average frame decoding duration
	tdav_video_jb_cb_data_type_fps_changed, // fps changed, detection done using the timestamp
}
tdav_video_jb_cb_data_type_t;

typedef struct tdav_video_jb_cb_data_xs
{
	tdav_video_jb_cb_data_type_t type;
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
tdav_video_jb_cb_data_xt;

typedef int (*tdav_video_jb_cb_f)(const tdav_video_jb_cb_data_xt* data);
typedef int (*tdav_video_nack_cb_f)(const trtp_rtcp_source_seq_map* data, void* usr_data);

int tdav_video_jb_set_callback(struct tdav_video_jb_s* self, tdav_video_jb_cb_f callback, void* usr_data);
int tdav_video_jb_set_video_nack_callback(struct tdav_video_jb_s* self, tdav_video_nack_cb_f callback, void* usr_data);

struct tdav_video_jb_s* tdav_video_jb_create();
int tdav_video_jb_start(struct tdav_video_jb_s* self);
int tdav_video_jb_put(void* video, struct tdav_video_jb_s* self, struct trtp_rtp_packet_s* rtp_pkt, uint16_t iNAP, uint32_t iCheckCodeLen,tdav_rscode_t *rscode);
int tdav_video_jb_stop(struct tdav_video_jb_s* self);
int delete_video_jb_session(struct tdav_video_jb_s *jb, int32_t sessionId) ;

// nack process function
void tdav_video_jb_packet_nack_check(struct tdav_video_jb_s* self, trtp_rtp_packet_t* rtp_pkt, int *redundancy, int reset);

TDAV_END_DECLS

#endif /* TINYDAV_VIDEO_JB_H */

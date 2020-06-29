/*
* Copyright (C) 2010-2011 Mamadou Diop.
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

/**@file tdav_session_video.h
 * @brief Video Session plugin.
 *
 * @author Mamadou Diop <diopmamadou(at)doubango.org>
 *

 */
#ifndef TINYDAV_SESSION_VIDEO_H
#define TINYDAV_SESSION_VIDEO_H

#include "tinydav_config.h"
#include "tinydav/tdav_session_av.h"

#include "tsk_thread.h"
#include "tsk_list.h"
#include "tsk_semaphore.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinydav/common/tdav_rscode.h"


TDAV_BEGIN_DECLS

#define MAX_VIDEO_BUF_FRAME_NUM (10)
#define MAX_VIDEO_FRAME_SIZE    (3110400)    // 1080p, 1920 * 1080 * 3/2
#define MAX_VIDEO_TEMP_BUF_SIZE (64 * 1024)  // 64K
#define DEF_MAX_H264_STREAM_SLICE    (1150)//(1320)    // 1 MTU size
#define MAX_H264_STREAM_SLICE_MERGE_SIZE (50) //切割后小于50字节的包直接合并
#define H264_START_CODE_LEN      (4)    // 有问题！！ start code len有可能为3字节
#define H264_NALU_HEADER_LEN     (1)
//#define RS_CODE_DATA_LEN         (MAX_H264_STREAM_SLICE + 2)

typedef struct tdav_session_video_frame_buffer_s
{
    TSK_DECLARE_OBJECT;
    
    void *data;
    uint32_t size;
    int32_t fps;
    int32_t width;
    int32_t height;
} tdav_session_video_frame_buffer_t;



typedef enum tdav_session_video_pkt_loss_level_e
{
	tdav_session_video_pkt_loss_level_low,
	tdav_session_video_pkt_loss_level_medium,
	tdav_session_video_pkt_loss_level_high,
}
tdav_session_video_pkt_loss_level_t;

typedef enum tdav_session_video_nack_flag_s {
    tdav_session_video_nack_none    = 0,
    tdav_session_video_nack_half    = 0x1,
    tdav_session_video_nack_fir     = 0x2,
} tdav_session_video_nack_flag_t;

//视频运行时事件
typedef enum tdav_session_video_check_unusual_e{
    tdav_session_video_normal = 0,
    tdav_session_video_shutdown = 1,
    tdav_session_video_data_error,
    tdav_session_video_network_bad,
    tdav_session_video_black_full,
    tdav_session_video_green_full,
    tdav_session_video_black_border,
    tdav_session_video_green_border,
    tdav_session_video_blurred_screen,
    tdav_session_video_encoder_error,
    tdav_session_video_decoder_error,
}tdav_session_video_check_unusual_t;

TINYDAV_GEXTERN const tmedia_session_plugin_def_t *tdav_session_video_plugin_def_t;
TINYDAV_GEXTERN const tmedia_session_plugin_def_t *tdav_session_bfcpvideo_plugin_def_t;

typedef void (*videoRuntimeEventCb_t)(tdav_session_video_check_unusual_t type, int32_t sessionId, tsk_boolean_t needUploadPic, tsk_boolean_t needReport);
typedef void (*staticsQoseCb_t)(int media_type, int32_t sessionId, int source_type, int audio_ppl);
typedef void (*predecodeCb_t)(void* buffer, tsk_size_t size, int32_t sessionId, int64_t timestamp);
typedef void (*videoEncodeAdjustCb_t)(int width, int32_t height, int bitrate, int fps);
typedef void (*videoTransRouteCb_t)(int type, void *data, int error);
typedef void (*videoDecodeParamCb_t)(int32_t sessionId, int32_t width, int32_t height);

TDAV_END_DECLS

#endif /* TINYDAV_SESSION_VIDEO_H */

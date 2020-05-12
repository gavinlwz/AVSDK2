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
#include "tinyrtp/rtcp/trtp_rtcp_rblock.h"

#include "tnet_endianness.h"
#include "tsk_debug.h"

/* 6.4.1 report block info
  for youme, seperate audio and video qos counts

	   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report |                 SSRC_n (SSRC of first source)                 |
block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Audio  | fraction lost |       cumulative number of packets lost       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           extended highest sequence number received           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      interarrival jitter                      |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Video  | fraction lost |       cumulative number of packets lost       |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |           extended highest sequence number received           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                      interarrival jitter                      |       
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
common |                         last SR (LSR)                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                   delay since last SR (DLSR)                  |
       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

SSRC_n (4 bytes)
    (ssrc of report block) 该block对应的源标识符，对应sessionID
    注意这里对标准进行了扩展，分别统计了audio和视频部分，用于后续的带宽检测

fraction lost (1 byte)
    上次 SR 或 RR 发送之后，从 SSRC_n 源的 RTP 报文丢失的部分，丢包率以定点表示。
    具体计算为：ppl = fraction lost / 256;

cumulative number of packets lost (3 bytes)
    从接收rtp开始，SSRC_n 源的 RTP 报文丢失的数量。定义为期望的减去实际接收的

extended highest sequence number received (4 bytes)
    低16位包含从源 SSRC_n 接收到的 RTP 数据报文中最高的序列值和 16 位的扩展序列值

interarrival jitter (4 bytes)
    关于 RTP 数据报文 interarrival 时间的统计方差的估值，以 timestamp 单元来估值，表现为无符号整数

LSR (4 bytes)
    最近的从SSRC_n接收到的SR报文NTPtimestamp中间32bit。如果尚未收到SR，则LSR置0

DLSR (4 bytes)
    延迟定义为从接收到从源 SSRC_n 来的上一个 SR 到发送本接收报告块的间隔，表示为 1/65536 秒一个单元。
    如果尚未收到 SR，DLSR 域设置为零

*/

static tsk_object_t* trtp_rtcp_rblock_ctor(tsk_object_t * self, va_list * app)
{
    trtp_rtcp_rblock_t *block = (trtp_rtcp_rblock_t *)self;
    if(block) {
        // do nothing
    }
    return self;
}

static tsk_object_t* trtp_rtcp_rblock_dtor(tsk_object_t * self)
{
    trtp_rtcp_rblock_t *block = (trtp_rtcp_rblock_t *)self;
    if(block) {
        // do nothing
    }

    return self;
}

static const tsk_object_def_t trtp_rtcp_rblock_def_s = {
    sizeof(trtp_rtcp_rblock_t),
    trtp_rtcp_rblock_ctor,
    trtp_rtcp_rblock_dtor,
    tsk_null,
};

const tsk_object_def_t *trtp_rtcp_rblock_def_t = &trtp_rtcp_rblock_def_s;

trtp_rtcp_rblock_t* trtp_rtcp_rblock_create_null()
{
    return (trtp_rtcp_rblock_t*)tsk_object_new(trtp_rtcp_rblock_def_t);
}

trtp_rtcp_rblock_t* trtp_rtcp_rblock_deserialize(const void* data, tsk_size_t size)
{
    trtp_rtcp_rblock_t* rblock = tsk_null;
    const uint8_t* pdata = (const uint8_t*)data;
    if(!data || size < TRTP_RTCP_RBLOCK_SIZE) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return tsk_null;
    }
    
    int offset = 0;
    if((rblock = trtp_rtcp_rblock_create_null())) {
        rblock->ssrc = (uint32_t)tnet_ntohl_2(pdata);
        offset += 4;  // ssrc is 4bytes

        // rblock audio qos
        rblock->audio_fraction = pdata[offset];
        offset += 1;  // ssrc is 4bytes  

        rblock->audio_cumulative_no_lost = (tnet_ntohl_2(&pdata[5]) >> 8) & 0xFFFFFF;

        rblock->audio_last_seq = (uint32_t)tnet_ntohl_2(&pdata[8]);
        rblock->audio_jitter = (uint32_t)tnet_ntohl_2(&pdata[12]);

        //rblock video qos
        rblock->video_fraction = pdata[16];
        rblock->video_cumulative_no_lost = (tnet_ntohl_2(&pdata[17]) >> 8) & 0xFFFFFF;
        rblock->video_last_seq = (uint32_t)tnet_ntohl_2(&pdata[20]);
        rblock->video_jitter = (uint32_t)tnet_ntohl_2(&pdata[24]);     

        // rblock last sr  
        rblock->lsr = (uint32_t)tnet_ntohl_2(&pdata[28]);
        rblock->dlsr = (uint32_t)tnet_ntohl_2(&pdata[32]);
    }
    else {
        TSK_DEBUG_ERROR("Failed to create report block object");
    }

    return rblock;
}

int trtp_rtcp_rblock_deserialize_list(const void* data, tsk_size_t _size, trtp_rtcp_rblocks_L_t* dest_list)
{
    int32_t size = (int32_t)_size;
    const uint8_t* pdata = (const uint8_t*)data;
    trtp_rtcp_rblock_t* rblock;

    if(!data || !size || !dest_list) {
        TSK_DEBUG_ERROR("Invalid parameter to deserialize report block list");
        return -1;
    }

    while(size >= TRTP_RTCP_RBLOCK_SIZE) {
        if((rblock = trtp_rtcp_rblock_deserialize(pdata, size))) {
            tsk_list_push_back_data(dest_list, (void**)&rblock);
        }
        if((size -= TRTP_RTCP_RBLOCK_SIZE) > 0) {
            pdata += TRTP_RTCP_RBLOCK_SIZE;
        }
    }
    return 0;
}

// todo:待RTCP打通后，这块需要优化，将序列化的共有部分提取出公共函数
int trtp_rtcp_rblock_serialize_to(const trtp_rtcp_rblock_t* self, void* data, tsk_size_t size)
{
    uint8_t* pdata = (uint8_t*)data;
    if(!self || !data || size < TRTP_RTCP_RBLOCK_SIZE) {
        TSK_DEBUG_ERROR("Invalid parameter to serialize to report block");
        return -1;
    }

    // rblock ssrc
    pdata[0] = self->ssrc >> 24;
    pdata[1] = (self->ssrc >> 16) & 0xFF;
    pdata[2] = (self->ssrc >> 8) & 0xFF;
    pdata[3] = (self->ssrc & 0xFF);

    // audio qos
    pdata[4] = self->audio_fraction;
    pdata[5] = (self->audio_cumulative_no_lost >> 16) & 0xFF;
    pdata[6] = (self->audio_cumulative_no_lost >> 8) & 0xFF;
    pdata[7] = (self->audio_cumulative_no_lost & 0xFF);
    pdata[8] = self->audio_last_seq >> 24;
    pdata[9] = (self->audio_last_seq >> 16) & 0xFF;
    pdata[10] = (self->audio_last_seq >> 8) & 0xFF;
    pdata[11] = (self->audio_last_seq & 0xFF);
    pdata[12] = self->audio_jitter >> 24;
    pdata[13] = (self->audio_jitter >> 16) & 0xFF;
    pdata[14] = (self->audio_jitter >> 8) & 0xFF;
    pdata[15] = (self->audio_jitter & 0xFF);

    // video qso
    pdata[16] = self->video_fraction;
    pdata[17] = (self->video_cumulative_no_lost >> 16) & 0xFF;
    pdata[18] = (self->video_cumulative_no_lost >> 8) & 0xFF;
    pdata[19] = (self->video_cumulative_no_lost & 0xFF);
    pdata[20] = self->video_last_seq >> 24;
    pdata[21] = (self->video_last_seq >> 16) & 0xFF;
    pdata[22] = (self->video_last_seq >> 8) & 0xFF;
    pdata[23] = (self->video_last_seq & 0xFF);
    pdata[24] = self->video_jitter >> 24;
    pdata[25] = (self->video_jitter >> 16) & 0xFF;
    pdata[26] = (self->video_jitter >> 8) & 0xFF;
    pdata[27] = (self->video_jitter & 0xFF);

    // last sr
    pdata[28] = self->lsr >> 24;
    pdata[29] = (self->lsr >> 16) & 0xFF;
    pdata[30] = (self->lsr >> 8) & 0xFF;
    pdata[31] = (self->lsr & 0xFF);
    pdata[32] = self->dlsr >> 24;
    pdata[33] = (self->dlsr >> 16) & 0xFF;
    pdata[34] = (self->dlsr >> 8) & 0xFF;
    pdata[35] = (self->dlsr & 0xFF);

    return 0;
}

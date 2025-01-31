﻿/*
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
#include "tinyrtp/rtcp/trtp_rtcp_report_sr.h"
#include "tinyrtp/rtcp/trtp_rtcp_header.h"

#include "tnet_endianness.h"

#include "tsk_debug.h"

#define TRTP_RTCP_PACKET_SR_MIN_SIZE (TRTP_RTCP_HEADER_SIZE + 4 + TRTP_RTCP_REPORT_SR_SENDINFO_LEN)   // no rblock

// RFC 3550 6.4.1 SR: Sender Report RTCP Packet
// for youme, seperate audio and video send counts
/***
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        header |V=2|P|    RC   |   PT=SR=200   |             length     |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                   SSRC of sender (sessionId)                  |
        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
sender  |              NTP timestamp, most significant word             |
info    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             NTP timestamp, least significant word             |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         RTP timestamp                         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                   sender's audio packet count                 |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                   sender's audio octet count                  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                sender's video packet count (main)             |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                sender's video octet count (main)              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                sender's video packet count (minor)            |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                sender's video octet count (minor)             |
        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
report  |                 SSRC_n (SSRC of first source)                 |
block   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Audio   | fraction lost |       cumulative number of packets lost       |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |           extended highest sequence number received           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                      interarrival jitter                      |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
Video   | fraction lost |       cumulative number of packets lost       |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |           extended highest sequence number received           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                      interarrival jitter                      |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         last SR (LSR)                         |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                   delay since last SR (DLSR)                  |
        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
        
 SSRC of sender (sessionId, 4 bytes) 
    本端ssrc，对应sessionID

 NTP timestamp（8 bytes）
    当本报告发送时，标识 wall clock time(即CLOCK_REALTIME)，这样就可以和timestamp 联起来使用，
    而 timestamp 是从接收报告中得到的，用来估量 (measure)那些接收者的 round-trip 传播

 RTP timestamp (4 bytes)
    与 NTP timestamp 保持一致，在数据报文中与 RTP timestamp 有相同的单元和相同的随机偏移

 NOTICE：
    为了减少RTCP的包量，会将多个类型的RTCP复合在一起发送，所以在SR报文中也会有携带report block部分
 **/

static tsk_object_t* trtp_rtcp_report_sr_ctor(tsk_object_t * self, va_list * app)
{
    trtp_rtcp_report_sr_t *sr = (trtp_rtcp_report_sr_t *)self;
    if(sr) {
        sr->blocks = tsk_list_create();
        sr->packets = tsk_list_create();
    }
    return self;
}

static tsk_object_t* trtp_rtcp_report_sr_dtor(tsk_object_t * self)
{
    trtp_rtcp_report_sr_t *sr = (trtp_rtcp_report_sr_t *)self;
    if(sr) {
        // deinit self
        TSK_OBJECT_SAFE_FREE(sr->blocks);
        TSK_OBJECT_SAFE_FREE(sr->packets);
        // deinit base
        trtp_rtcp_packet_deinit(TRTP_RTCP_PACKET(sr));
    }

    return self;
}

static const tsk_object_def_t trtp_rtcp_report_sr_def_s = {
    sizeof(trtp_rtcp_report_sr_t),
    trtp_rtcp_report_sr_ctor,
    trtp_rtcp_report_sr_dtor,
    tsk_null,
};
const tsk_object_def_t *trtp_rtcp_report_sr_def_t = &trtp_rtcp_report_sr_def_s;

trtp_rtcp_report_sr_t* trtp_rtcp_report_sr_create_null()
{
    trtp_rtcp_report_sr_t* sr;
    if((sr = (trtp_rtcp_report_sr_t*)tsk_object_new(trtp_rtcp_report_sr_def_t))) {
        trtp_rtcp_packet_init(TRTP_RTCP_PACKET(sr), TRTP_RTCP_HEADER_VERSION_DEFAULT, 0, 0, trtp_rtcp_packet_type_sr, (TRTP_RTCP_HEADER_SIZE  + 4 + TRTP_RTCP_REPORT_SR_SENDINFO_LEN));
    }
    return sr;
}

trtp_rtcp_report_sr_t* trtp_rtcp_report_sr_create(struct trtp_rtcp_header_s* header)
{
    trtp_rtcp_report_sr_t* sr;
    if((sr = (trtp_rtcp_report_sr_t*)tsk_object_new(trtp_rtcp_report_sr_def_t))) {
        TRTP_RTCP_PACKET(sr)->header = (struct trtp_rtcp_header_s *)tsk_object_ref(header);
    }
    return sr;
}


trtp_rtcp_report_sr_t* trtp_rtcp_report_sr_deserialize(const void* data, tsk_size_t _size)
{
    trtp_rtcp_report_sr_t* sr = tsk_null;
    trtp_rtcp_header_t* header = tsk_null;
    const uint8_t* pdata = (const uint8_t*)data;
    int32_t size = (int32_t)_size;

    if(!data || size < TRTP_RTCP_PACKET_SR_MIN_SIZE) {
        TSK_DEBUG_ERROR("Invalid parameter to deserialize sr");
        return tsk_null;
    }

    if(!(header = trtp_rtcp_header_deserialize(pdata, size))) {
        TSK_DEBUG_ERROR("Failed to deserialize the rtcp header");
        goto bail;
    }
    if(header->length_in_bytes < TRTP_RTCP_PACKET_SR_MIN_SIZE) {
        TSK_DEBUG_ERROR("Too short for rtcp sr pkt");
        goto bail;
    }

    if(!(sr = trtp_rtcp_report_sr_create(header))) {
        TSK_DEBUG_ERROR("Failed to create sr object");
        goto bail;
    }

    sr->ssrc = (uint32_t)tnet_ntohl_2(&pdata[4]);
    pdata += (TRTP_RTCP_HEADER_SIZE + 4);
    size -= (TRTP_RTCP_HEADER_SIZE + 4);

    // sender info -- youme format is 28bytes
    sr->sender_info.ntp_msw = (uint32_t)tnet_ntohl_2(&pdata[0]);
    sr->sender_info.ntp_lsw = (uint32_t)tnet_ntohl_2(&pdata[4]);
    sr->sender_info.rtp_timestamp = (uint32_t)tnet_ntohl_2(&pdata[8]);
    sr->sender_info.audio_sender_pcount = (uint32_t)tnet_ntohl_2(&pdata[12]);
    sr->sender_info.audio_sender_ocount = (uint32_t)tnet_ntohl_2(&pdata[16]);
    sr->sender_info.video_sender_pcount = (uint32_t)tnet_ntohl_2(&pdata[20]);
    sr->sender_info.video_sender_ocount = (uint32_t)tnet_ntohl_2(&pdata[24]);

    //minor video info
    sr->sender_info.minor_video_sender_pcount = (uint32_t)tnet_ntohl_2(&pdata[28]);
    sr->sender_info.minor_video_sender_ocount = (uint32_t)tnet_ntohl_2(&pdata[32]);

    pdata += TRTP_RTCP_REPORT_SR_SENDINFO_LEN;  // send info length is 36
    size -= TRTP_RTCP_REPORT_SR_SENDINFO_LEN;

    // Blocks
    if(header->rc > 0) {
        tsk_size_t i = 0;
        trtp_rtcp_rblock_t* rblock;

        while(i++ < header->rc && size >= TRTP_RTCP_RBLOCK_SIZE) {
            if((rblock = trtp_rtcp_rblock_deserialize(pdata, size))) {
                tsk_list_push_back_data(sr->blocks, (void**)&rblock);
            }
            pdata += TRTP_RTCP_RBLOCK_SIZE;
            size -= TRTP_RTCP_RBLOCK_SIZE;
        }
    }

    // Other Packets
    while(size > TRTP_RTCP_HEADER_SIZE) {
        trtp_rtcp_packet_t* packet;

        if((packet = trtp_rtcp_packet_deserialize(pdata, size))) {
            if((size -= packet->header->length_in_bytes) > 0) {
                pdata += packet->header->length_in_bytes;
            }
            tsk_list_push_back_data(sr->packets, (void**)&packet);
            continue;
        }
        break;
    }

bail:
    TSK_OBJECT_SAFE_FREE(header);
    return sr;
}

int trtp_rtcp_report_sr_serialize_to(const trtp_rtcp_report_sr_t* self, void* data, tsk_size_t size)
{
    int ret;
    const tsk_list_item_t* item;
    uint8_t* pdata = (uint8_t*)data;

    if(!self || !data || size < trtp_rtcp_report_sr_get_size(self)) {
        TSK_DEBUG_ERROR("Invalid parameter to serialize sr pkt");
        return -1;
    }
    if((ret = trtp_rtcp_header_serialize_to(TRTP_RTCP_PACKET(self)->header, pdata, size))) {
        TSK_DEBUG_ERROR("Failed to serialize the rtcp header");
        return ret;
    }

    // rtcp common header (ver, csrc count, payload type, length)
    pdata[TRTP_RTCP_HEADER_SIZE] = self->ssrc >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 1] = (self->ssrc >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 2] = (self->ssrc >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 3] = (self->ssrc & 0xFF);
    
    // ntp ts -- 8bytes
    pdata[TRTP_RTCP_HEADER_SIZE + 4] = self->sender_info.ntp_msw >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 5] = (self->sender_info.ntp_msw >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 6] = (self->sender_info.ntp_msw >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 7] = self->sender_info.ntp_msw & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 8] = self->sender_info.ntp_lsw >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 9] = (self->sender_info.ntp_lsw >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 10] = (self->sender_info.ntp_lsw >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 11] = self->sender_info.ntp_lsw & 0xFF;
    
    // rtp ts -- 4bytes
    pdata[TRTP_RTCP_HEADER_SIZE + 12] = self->sender_info.rtp_timestamp >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 13] = (self->sender_info.rtp_timestamp >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 14] = (self->sender_info.rtp_timestamp >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 15] = self->sender_info.rtp_timestamp & 0xFF;
    
    // send audio pkt counts and octet counts -- 8bytes
    pdata[TRTP_RTCP_HEADER_SIZE + 16] = self->sender_info.audio_sender_pcount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 17] = (self->sender_info.audio_sender_pcount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 18] = (self->sender_info.audio_sender_pcount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 19] = self->sender_info.audio_sender_pcount & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 20] = self->sender_info.audio_sender_ocount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 21] = (self->sender_info.audio_sender_ocount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 22] = (self->sender_info.audio_sender_ocount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 23] = self->sender_info.audio_sender_ocount & 0xFF;

    // send video pkt counts and octet counts -- 8bytes
    pdata[TRTP_RTCP_HEADER_SIZE + 24] = self->sender_info.video_sender_pcount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 25] = (self->sender_info.video_sender_pcount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 26] = (self->sender_info.video_sender_pcount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 27] = self->sender_info.video_sender_pcount & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 28] = self->sender_info.video_sender_ocount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 29] = (self->sender_info.video_sender_ocount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 30] = (self->sender_info.video_sender_ocount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 31] = self->sender_info.video_sender_ocount & 0xFF;

    // send minor video pkt counts and octet counts -- 8bytes
    pdata[TRTP_RTCP_HEADER_SIZE + 32] = self->sender_info.minor_video_sender_pcount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 33] = (self->sender_info.minor_video_sender_pcount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 34] = (self->sender_info.minor_video_sender_pcount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 35] = self->sender_info.minor_video_sender_pcount & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 36] = self->sender_info.minor_video_sender_ocount >> 24;
    pdata[TRTP_RTCP_HEADER_SIZE + 37] = (self->sender_info.minor_video_sender_ocount >> 16) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 38] = (self->sender_info.minor_video_sender_ocount >> 8) & 0xFF;
    pdata[TRTP_RTCP_HEADER_SIZE + 39] = self->sender_info.minor_video_sender_ocount & 0xFF;

    pdata += (TRTP_RTCP_HEADER_SIZE + 4 + TRTP_RTCP_REPORT_SR_SENDINFO_LEN);
    size -= (TRTP_RTCP_HEADER_SIZE + 4 + TRTP_RTCP_REPORT_SR_SENDINFO_LEN);

    if(TRTP_RTCP_PACKET(self)->header->rc > 0) {
        tsk_list_foreach(item, self->blocks) {
            if(!item->data) {
                continue;
            }
            if((ret = trtp_rtcp_rblock_serialize_to(TRTP_RTCP_RBLOCK(item->data), pdata, size))) {
                TSK_DEBUG_ERROR("Failed to serialize the rblock");
                goto bail;
            }
            pdata += TRTP_RTCP_RBLOCK_SIZE;
            size -= TRTP_RTCP_RBLOCK_SIZE;
        }
    }

    tsk_list_foreach(item, self->packets) {
        if(!item->data) {
            continue;
        }
        if((ret = trtp_rtcp_packet_serialize_to(TRTP_RTCP_PACKET(item->data), pdata, size))) {
            TSK_DEBUG_ERROR("Failed to serialize packet");
            goto bail;
        }
        pdata += TRTP_RTCP_PACKET(item->data)->header->length_in_bytes;
        size -= TRTP_RTCP_PACKET(item->data)->header->length_in_bytes;
    }

bail:
    return ret;
}

int trtp_rtcp_report_sr_add_block(trtp_rtcp_report_sr_t* self, trtp_rtcp_rblock_t* rblock)
{
    if(!self || !TRTP_RTCP_PACKET(self)->header || !rblock) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    rblock = (trtp_rtcp_rblock_t*)tsk_object_ref(rblock);
    tsk_list_push_back_data(self->blocks, (void**)&rblock);
    ++TRTP_RTCP_PACKET(self)->header->rc;
    TRTP_RTCP_PACKET(self)->header->length_in_bytes += TRTP_RTCP_RBLOCK_SIZE;
    TRTP_RTCP_PACKET(self)->header->length_in_words_minus1 = ((TRTP_RTCP_PACKET(self)->header->length_in_bytes >> 2) - 1) +
            ((TRTP_RTCP_PACKET(self)->header->length_in_bytes & 0x03) ? 1 : 0);
    return 0;
}

tsk_size_t trtp_rtcp_report_sr_get_size(const trtp_rtcp_report_sr_t* self)
{
    tsk_size_t size;
    const tsk_list_item_t* item;

    if(!self || !TRTP_RTCP_PACKET(self)->header) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }

    size = TRTP_RTCP_PACKET(self)->header->length_in_bytes;
//    if(TRTP_RTCP_PACKET(self)->header->rc > 0){
//        tsk_list_foreach(item, self->blocks){
//            if(item->data){
//                size += TRTP_RTCP_RBLOCK_SIZE;
//            }
//        }
//    }
    tsk_list_foreach(item, self->packets) {
        if(item->data && TRTP_RTCP_PACKET(item->data)->header) {
            size += TRTP_RTCP_PACKET(item->data)->header->length_in_bytes;
        }
    }

    return size;
}


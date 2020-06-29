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
#ifndef TINYRTP_RTCP_REPORT_FB_H
#define TINYRTP_RTCP_REPORT_FB_H

#include "tinyrtp_config.h"
#include "tinyrtp/rtcp/trtp_rtcp_packet.h"
#include <map>
#include <list>
#include <vector>

#define TRTP_RTCP_REPORT_FB(self) ((trtp_rtcp_report_fb_t*)(self))
#define TRTP_DECLARE_RTCP_FB_PACKET trtp_rtcp_report_fb_t __packet_fb__

typedef struct trtp_rtcp_source_nack_s {
	uint32_t sessionid;
	uint16_t videoid;
	std::list<uint16_t>* nack_list;

	trtp_rtcp_source_nack_s() {
		nack_list = nullptr;
	}

    ~trtp_rtcp_source_nack_s() {
		if (nack_list) {
			delete[] nack_list;
			nack_list = nullptr;
		}
    }
} trtp_rtcp_source_nack_t;

// typedef std::map< uint32_t, std::list<uint16_t> > trtp_rtcp_source_seq_map;
typedef std::list< trtp_rtcp_source_nack_t* > trtp_rtcp_source_seq_map;

// RFC 4585 6.1. Common Packet Format for Feedback Messages
typedef struct trtp_rtcp_report_fb_s {
    TRTP_DECLARE_RTCP_PACKET;

    uint32_t ssrc_sender; /* SSRC of packet sender */
    uint32_t ssrc_media; /* SSRC of media source */
}
trtp_rtcp_report_fb_t;

typedef enum trtp_rtcp_rtpfb_fci_type_e {
    trtp_rtcp_rtpfb_fci_type_nack = 1, // RFC 4585
    trtp_rtcp_rtpfb_fci_type_nack_plus = 2, // self-defined
    trtp_rtcp_rtpfb_fci_type_tmmbn = 4, // RFC 5104
}
trtp_rtcp_rtpfb_fci_type_t;

typedef struct trtp_rtcp_report_source_s {
    uint32_t source_size;   // 当前source对应nack body的长度
    uint32_t ssrc_media;
    tsk_size_t count;
    uint16_t* pid; // 16 bits
    uint16_t* blp; // 16 bits
} trtp_rtcp_report_source_t;

// Transport layer FB message
typedef struct trtp_rtcp_report_rtpfb_s {
    TRTP_DECLARE_RTCP_FB_PACKET;

    trtp_rtcp_rtpfb_fci_type_t fci_type;
//    union {
        std::vector<trtp_rtcp_report_source_t*> *v_nack;

//        struct {
//            tsk_size_t count; // not part of the specification
//            uint32_t* ssrc; // 32 bits
//            uint8_t* MxTBR_Exp; // 6 bits
//            uint32_t* MxTBR_Mantissa; // 17 bits
//            uint16_t* MeasuredOverhead; // 9 bits
//        } tmmbn;
//    };
}
trtp_rtcp_report_rtpfb_t;

trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_create_null();
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_create(struct trtp_rtcp_header_s* header);
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_create_2(trtp_rtcp_rtpfb_fci_type_t fci_type, uint32_t ssrc_sender, uint32_t ssrc_media_src);
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_create_nack(uint32_t ssrc_sender, uint32_t ssrc_media_src, const uint16_t* seq_nums, tsk_size_t count);
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_create_nack_plus(uint32_t ssrc_sender, uint32_t ssrc_media_src,const uint32_t* timestamp,tsk_size_t count);
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_build_nack(uint32_t ssrc_sender, trtp_rtcp_source_seq_map * source_seqs);
trtp_rtcp_report_rtpfb_t* trtp_rtcp_report_rtpfb_deserialize(const void* data, tsk_size_t size);
int trtp_rtcp_report_rtpfb_serialize_to(const trtp_rtcp_report_rtpfb_t* self, void* data, tsk_size_t size);
tsk_size_t trtp_rtcp_report_rtpfb_get_size(const trtp_rtcp_report_rtpfb_t* self);


typedef enum trtp_rtcp_psfb_fci_type_e {
    trtp_rtcp_psfb_fci_type_pli = 1, /* rfc 4585: Picture Loss Indication (PLI) */
    trtp_rtcp_psfb_fci_type_sli = 2, /* rfc 4585: Slice Loss Indication (SLI) */
    trtp_rtcp_psfb_fci_type_rpsi = 3, /* rfc 4585: Reference Picture Selection Indication (RPSI) */
    trtp_rtcp_psfb_fci_type_fir = 4, /* rfc 5104: Full Intra Request (FIR) Command*/
    trtp_rtcp_psfb_fci_type_afb = 15, /* rfc 4585: Application layer FB (AFB) message */
}
trtp_rtcp_psfb_fci_type_t;

/* rfc 4585: Application layer FB (AFB) message */
typedef enum trtp_rtcp_psfb_afb_type_e {
    trtp_rtcp_psfb_afb_type_none,
    trtp_rtcp_psfb_afb_type_remb, // draft-alvestrand-rmcat-remb-02
    trtp_rtcp_psfb_afb_type_jcng, // doubango congestion control estimation
}
trtp_rtcp_psfb_afb_type_t;

// Payload-specific FB message
typedef struct trtp_rtcp_report_psfb_s {
    TRTP_DECLARE_RTCP_FB_PACKET;

    trtp_rtcp_psfb_fci_type_t fci_type;
    union {
        // struct{ //rfc 4585: 6.3.1.2
        // } pli;
        struct { // rfc 4585: 6.3.2.2
            uint16_t* first;
            uint16_t* number;
            uint8_t* pic_id;
        } sli;
        struct { // rfc 4585: 6.3.3.2
            unsigned pb:8;
            unsigned pt:7;
            uint8_t* bytes;
        } rpsi;
        struct { // rfc 5104: 4.3.1.1
            tsk_size_t count;
            uint32_t* ssrc; // 32 bits
            uint8_t* seq_num; // 8 bits
        } fir;
        struct { // rfc 4585: 6.4
            trtp_rtcp_psfb_afb_type_t type;
            union {
                struct { // draft-alvestrand-rmcat-remb-02: 2.2
                    // MxTBR = mantissa * 2^exp = (mantissa << exp) bps
                    uint8_t num_ssrc;
                    uint8_t exp; // 6bits
                    uint32_t mantissa; // 18bits
                    uint32_t* ssrc_feedbacks; // 'num_ssrc'nth SSRC entries
                } remb;
                struct { // jitter buffer congestion estimation: doubango
                    uint8_t num_ssrc;
                    uint8_t q; // 8bits
                    uint32_t* ssrc_feedbacks; // 'num_ssrc'nth SSRC entries
                } jcng;
                struct {
                    uint8_t* bytes; // store bytes to allow reconstruction
                } none; // unknown type
            };
        } afb;
    };
}
trtp_rtcp_report_psfb_t;

trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_null();
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create(struct trtp_rtcp_header_s* header);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_2(trtp_rtcp_psfb_fci_type_t fci_type, uint32_t ssrc_sender, uint32_t ssrc_media_src);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_pli(uint32_t ssrc_sender, uint32_t ssrc_media_src);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_fir(uint8_t seq_num, uint32_t ssrc_sender, uint32_t ssrc_media_src);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_afb_remb(uint32_t ssrc_sender, const uint32_t* ssrc_media_src_list, uint32_t ssrc_media_src_list_count, uint32_t bitrate/*in bps*/);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_create_afb_jcng(uint32_t ssrc_sender, const uint32_t* ssrc_media_src_list, uint32_t ssrc_media_src_list_count, float jcng_q/*in quality metric*/);
trtp_rtcp_report_psfb_t* trtp_rtcp_report_psfb_deserialize(const void* data, tsk_size_t size);
int trtp_rtcp_report_psfb_serialize_to(const trtp_rtcp_report_psfb_t* self, void* data, tsk_size_t size);
tsk_size_t trtp_rtcp_report_psfb_get_size(const trtp_rtcp_report_psfb_t* self);

#endif /* TINYRTP_RTCP_REPORT_FB_H */

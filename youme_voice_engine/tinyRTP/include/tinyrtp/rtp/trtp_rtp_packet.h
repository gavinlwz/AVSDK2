/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/
/**@file trtp_rtp_packet.h
 * @brief RTP packet.
 *
*
 *
 */
#ifndef TINYRTP_RTP_PACKET_H
#define TINYRTP_RTP_PACKET_H

#include "tinyrtp_config.h"

#include "tinyrtp/rtp/trtp_rtp_header.h"

#include "tsk_object.h"

#define RTP_EXTENSION_MAGIC_NUM 0xFE12          //魔法数字定义为0xFE12
#define RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE 2   //rtp扩展头Magic Num，占16bit
#define RTP_EXTENSION_LEN_PROFILE 2             //扩展头长度所占16bit
#define RTP_EXTENSION_TYPE_PROFILE 2            //内容类型占16bit
#define BUSINESS_ID_LEN_PROFILE 2               //内容长度所占16bit

TRTP_BEGIN_DECLS

enum RTP_EXTENSION_TYPE {
    BUSINESSID = 1
};

typedef struct trtp_rtp_packet_s
{
	TSK_DECLARE_OBJECT;

	trtp_rtp_header_t* header;

	struct{
		void* data;
		const void* data_const; // never free()d. an alternative to "data"
		tsk_size_t size;
	} payload;
	
	/* extension header as per RFC 3550 section 5.3.1 */
	struct{
		void* data;
        const void* data_const; // never free()d. an alternative to "data"
		tsk_size_t size; /* contains the first two 16-bit fields */
	} extension;
    
    struct trtp_rtp_packet_s* prev;
    struct trtp_rtp_packet_s* next;
}
trtp_rtp_packet_t;
typedef tsk_list_t trtp_rtp_packets_L_t;


TINYRTP_API trtp_rtp_packet_t* trtp_rtp_packet_copy(const trtp_rtp_packet_t *self);
TINYRTP_API trtp_rtp_packet_t* trtp_rtp_packet_create_null();
TINYRTP_API trtp_rtp_packet_t* trtp_rtp_packet_create(uint32_t ssrc, uint16_t seq_num, uint64_t timestamp, uint8_t payload_type, tsk_bool_t marker);
TINYRTP_API trtp_rtp_packet_t* trtp_rtp_packet_create_2(const trtp_rtp_header_t* header);
TINYRTP_API tsk_size_t trtp_rtp_packet_guess_serialbuff_size(const trtp_rtp_packet_t *self);
TINYRTP_API tsk_size_t trtp_rtp_packet_serialize_to(const trtp_rtp_packet_t *self, void* buffer, tsk_size_t size);
TINYRTP_API tsk_buffer_t* trtp_rtp_packet_serialize(const trtp_rtp_packet_t *self, tsk_size_t num_bytes_pad);
TINYRTP_API trtp_rtp_packet_t* trtp_rtp_packet_deserialize(const void *data, tsk_size_t size);


TINYRTP_GEXTERN const tsk_object_def_t *trtp_rtp_packet_def_t;

TRTP_END_DECLS

#endif /* TINYRTP_RTP_PACKET_H */

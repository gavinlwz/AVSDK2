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
/**@file trtp_rtp_header.h
 * @brief RTP header.
 *
*
 *
 */
#ifndef TINYRTP_RTP_HEADER_H
#define TINYRTP_RTP_HEADER_H

#include "tinyrtp_config.h"
#include "tinymedia/tmedia_codec.h"
#include "tsk_buffer.h"
#include "tinydav/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.h"

TRTP_BEGIN_DECLS


//#ifdef YOUME_FOR_QINIU
#define TRTP_RTP_HEADER_MIN_SIZE 20
//#else
//#define TRTP_RTP_HEADER_MIN_SIZE 12
//#endif

#define TRTP_RTP_HEADER(self)	((trtp_rtp_header_t*)(self))


// For RTP sequence number handling

// The max RTP sequence number
#define MAX_RTP_SEQ_NO       (65535)
#define HALF_MAX_RTP_SEQ_NO  (MAX_RTP_SEQ_NO / 2)

// Check if the RTP sequence number wrap around
#define IS_RTP_SEQ_NO_WRAP_AROUND(prevSeqNo, curSeqNo) \
    ( (((int32_t)curSeqNo - (int32_t)prevSeqNo) > HALF_MAX_RTP_SEQ_NO) \
    || (((int32_t)prevSeqNo - (int32_t)curSeqNo) > HALF_MAX_RTP_SEQ_NO) )

// Check if the current RTP sequence number is incremental to the previous one
#define IS_RTP_SEQ_NO_INC(prevSeqNo, curSeqNo) \
    ( ((curSeqNo > prevSeqNo) && ((curSeqNo - prevSeqNo) <  HALF_MAX_RTP_SEQ_NO)) \
    || ((curSeqNo < prevSeqNo) && ((prevSeqNo - curSeqNo) >  HALF_MAX_RTP_SEQ_NO)) )

// Calculate the difference between 2 RTP sequence numbers
#define DIFF_RTP_SEQ_NO(prevSeqNo, curSeqNo) \
    ( IS_RTP_SEQ_NO_WRAP_AROUND(prevSeqNo, curSeqNo) \
    ? ( (curSeqNo < prevSeqNo) ? (int32_t)(MAX_RTP_SEQ_NO - prevSeqNo + curSeqNo) : -(int32_t)(MAX_RTP_SEQ_NO - curSeqNo + prevSeqNo) ) \
    : ((int32_t)curSeqNo - (int32_t)prevSeqNo) \
    )

// The max RTP timestamp
#define MAX_RTP_TS       ((int64_t)1 << 32)
#define HALF_MAX_RTP_TS  ((int64_t)MAX_RTP_TS / 2)

// Check if the RTP timestamp wrap around
#define IS_RTP_TS_WRAP_AROUND(prevTs, curTs) \
    ( (((int64_t)curTs - (int64_t)prevTs) > HALF_MAX_RTP_TS) \
    || (((int64_t)prevTs - (int64_t)curTs) > HALF_MAX_RTP_TS) )

// Calculate the difference between 2 RTP timestamp
#define DIFF_RTP_TS(prevTs, curTs) \
    ( IS_RTP_TS_WRAP_AROUND(prevTs, curTs) \
    ? ( (curTs < prevTs) ? (int64_t)(MAX_RTP_TS - prevTs + curTs) : -(int64_t)(MAX_RTP_TS - curTs + prevTs) ) \
    : ((int64_t)curTs - (int64_t)prevTs) \
    )

// Definitions of "bit-wise or" packet flags, such as "if this packet is decoded from FEC".
#define RTP_PACKET_FLAG_SET(x, flag)       (x |= flag)
#define RTP_PACKET_FLAG_CLEAR(x, flag)     (x &= ~(flag))
#define RTP_PACKET_FLAG_CONTAINS(x, flag)  ((x & flag) ? 1 : 0)
#define RTP_PACKET_FLAG_FEC                (0x1)
#define RTP_PACKET_FLAG_PLC                (0x2)

typedef enum bkaud_source_e
{
    BKAUD_SOURCE_NULL = 0,
    BKAUD_SOURCE_GAME,
    BKAUD_SOURCE_MICBYPASS,
    BKAUD_SOURCE_PCMCALLBACK,
    BKAUD_SOURCE_MAX = 10,
} bkaud_source_t;

typedef struct trtp_rtp_header_s
{
	TSK_DECLARE_OBJECT;
	/* RFC 3550 section 5.1 - RTP Fixed Header Fields
		0                   1                   2                   3
		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |                           timestamp                           |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |           synchronization source (SSRC) identifier            |
	   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	   |            contributing source (CSRC) identifiers             |
	   |                             最大15个。具体个数保存在CC 里面   |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |				            payload		         			   |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	   |															   |
	   |                                                               |  
	   +-+-+-+-+-+-+-+-+-       8字节时间戳    +-+-+-+-+-+-+-+-+-+-+-+-+
	   |															   |
	   |                                                               |
	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+




	*/
	unsigned version:2;
	unsigned padding:1;
	unsigned extension:1;
	unsigned csrc_count:4;
	unsigned marker:1; //音频用来标识是否合包
	unsigned payload_type:7;
	uint16_t seq_num;
    uint32_t timestamp;
	uint32_t ssrc;
	uint32_t csrc[15]; //RScode pack 头 [0] session_id  [1] 是否是rscode 冗余包  [2]rscode 包的组id(头两字节)+ N(后两个字节)   [3]一组Rscode 包中 的序号 0--2*NPAR [4] videoid [5]冗余包数量
    //视频 数据头 [0] session_id [1] video frametype [2] framecount+frameserial [3] videoid
	// for internal use
	enum tmedia_codec_id_e codec_id;
    uint32_t flag; // "bit-wise or" of packet attributes, such as "if this packet is decoded from FEC"
    // The session id was embedded in the first csrc. So we can actually get the session id with the csrc defined above.
    // But to make things clear, and to hide the details from outside the RTP module, we define it explictly here.
    int32_t  session_id;
    // My own session id. For handling some session specific information such as bandwidth control data.
    int32_t  my_session_id;
    // The |receive_timestamp| is an indication of the time when the packet was received,
    // and was measured with the same tick rate as the RTP timestamp of the current payload.
    uint32_t receive_timestamp;
    // |receive_timestamp_ms| is measured in milli-second
    uint64_t receive_timestamp_ms;
    // Frequency for the clock taken as the timestamp base for the current payload type.
    uint32_t  payload_type_frequency;
    
    // Bandwidth control data for the purposes similar to RTCP:
    // 1. Bandwidth control data received from the sender(client/server).
    BandwidthCtrlMsg_t bc_received;
    // 2. Bandwidth control data calculated from my statistics, and are ready to be sent to the far end.
    BandwidthCtrlMsg_t bc_to_send;
    
    // Network delay in ms, 0 if no network delay info is available
    uint32_t networkDelayMs;
    
    enum bkaud_source_e bkaud_source;
    
    tsk_bool_t is_last_pkts;
    int32_t video_display_width;
    int32_t video_display_height;
//#ifdef YOUME_FOR_QINIU
    uint64_t timestampl;
//#endif

    //坑爹啊。这里需要保存这些参数，当包累积过多的时候，需要丢掉或者强制不丢。这些参数在RSCODE 编码的时候要带上
    //但是不需要 发送给对方。
    uint8_t video_id;
    uint8_t frameType; //I 帧还是P 帧率
    uint16_t frameSerial;
    uint16_t frameCount;
    uint16_t packetGroup;
}
trtp_rtp_header_t;

TINYRTP_API trtp_rtp_header_t* trtp_rtp_header_create_null();



TINYRTP_API trtp_rtp_header_t* trtp_rtp_header_create(uint32_t ssrc, uint16_t seq_num, uint64_t timestamp, uint8_t payload_type, tsk_bool_t marker);
TINYRTP_API tsk_size_t trtp_rtp_header_guess_serialbuff_size(const trtp_rtp_header_t *self);
TINYRTP_API tsk_size_t trtp_rtp_header_serialize_to(const trtp_rtp_header_t *self, void *buffer, tsk_size_t size);
TINYRTP_API tsk_buffer_t* trtp_rtp_header_serialize(const trtp_rtp_header_t *self);
TINYRTP_API trtp_rtp_header_t* trtp_rtp_header_deserialize(const void *data, tsk_size_t size);



TINYRTP_GEXTERN const tsk_object_def_t *trtp_rtp_header_def_t;

TRTP_END_DECLS

#endif /* TINYRTP_RTP_HEADER_H */

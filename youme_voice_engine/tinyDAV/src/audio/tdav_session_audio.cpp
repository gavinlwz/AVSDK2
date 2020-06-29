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

/**@file tdav_session_audio.c
* @brief Audio Session plugin.
*
 *
* @contributors: See $(YOUME_HOME)\contributors.txt
*/
#include <math.h>
#include "tinydav/audio/tdav_session_audio.h"

//#include "tinydav/codecs/dtmf/tdav_codec_dtmf.h"
#include "tinydav/audio/tdav_consumer_audio.h"

#include "tinymedia/tmedia_consumer.h"
#include "tinymedia/tmedia_defaults.h"
#include "tinymedia/tmedia_denoise.h"
#include "tinymedia/tmedia_jitterbuffer.h"
#include "tinymedia/tmedia_producer.h"
#include "tinymedia/tmedia_codec.h"

#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinyrtp/trtp_manager.h"
#include "tinydav/codecs/rtp_extension/tdav_codec_rtp_extension.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "tinydav/common/tdav_audio_rscode.h"
#include "tsk_debug.h"
#include "tsk_memory.h"
#include <math.h>
#include "XOsWrapper.h"

#include "XConfigCWrapper.hpp"

#ifdef OS_LINUX
#include <sys/prctl.h>
#endif

#define TDAV_AUDIO_OPUS_DTX_MAX_PEROID_NUM 25 // 开启DTX后，发送单个静音包周期最大为500ms

#define TDAV_AUDIO_RESAMPLER_DEFAULT_QUALITY 5
#define MICAUD_BUFFER_SZ 1024*8

#define TDAV_AUDIO_PCM_FILE_MIX_MIC        1
#define TDAV_AUDIO_PCM_FILE_MIX_SPEAKER    2
#define TDAV_AUDIO_PCM_FILE_SEND_PCM       3

static int _tdav_session_audio_dtmfe_timercb (const void *arg, xt_timer_id_t timer_id);
static struct tdav_session_audio_dtmfe_s *
_tdav_session_audio_dtmfe_create (const tdav_session_audio_t *session, uint8_t event, uint16_t duration, uint32_t seq, uint32_t timestamp, uint8_t format, tsk_bool_t M, tsk_bool_t E);

static int tdav_session_audio_producer_handle_recorded_data (const tmedia_session_t *self, const void *buffer, tsk_size_t size);
static void* tdav_session_audio_producer_thread(void* param);
static void tdav_session_audio_apply_bandwidth_ctrl(tdav_session_audio_t *audio);
static tsk_size_t tdav_session_audio_encode_rtp_header_ext(tdav_session_audio_t *audio, void* payload, tsk_size_t payload_size, tsk_bool_t silence_or_noise);
static int tdav_session_audio_decode_rtp_header_ext(tdav_session_audio_t *audio, const struct trtp_rtp_packet_s *packet);
static int tadv_session_audio_get_mixaud(tdav_session_audio_t *audio, void* destBuf, tsk_size_t mixSize, long* timeStamp);
static int32_t tadv_session_audio_calc_power_db(const uint8_t* buffer, tsk_size_t size);
static void tdav_session_audio_handle_mic_level_callback(tdav_session_audio_t* audio, const uint8_t* buffer, tsk_size_t size);


static tdav_audio_rscode_t* tdav_session_audio_select_rscode_by_sessionid(tdav_session_audio_t* self, int session_id, RscAudioType type) {
	tsk_list_item_t *item = tsk_null;
	tdav_audio_rscode_t *rscode = tsk_null;

	if (!self->rscode_list) {
		TSK_DEBUG_ERROR("*** rscode list is null ***");
		return tsk_null;
	}

	tsk_list_lock(self->rscode_list);
	tsk_list_foreach(item, self->rscode_list) {
		if ((TDAV_AUDIO_RSCODE(item->data)->session_id == session_id) && (TDAV_AUDIO_RSCODE(item->data)->rs_type == type)) {
			rscode = TDAV_AUDIO_RSCODE(item->data);
			break;
		}
	}
	tsk_list_unlock(self->rscode_list);

	return rscode;
}

static void tdav_session_audio_send_combine_packet(tdav_session_audio_t* audio, trtp_rtp_packet_t* packet)
{
    int offset;
    
	if (audio->iCombineCount == 1)
	{
		//一个包就直接放到rs队列
		tdav_session_av_t *base = (tdav_session_av_t *)audio;
        trtp_manager_t *self = base->rtp_manager;
		tdav_audio_rscode_t* rscode = tdav_session_audio_select_rscode_by_sessionid(audio, base->rtp_manager->sessionid, RSC_TYPE_ENCODE);
		if (!rscode) {
			tdav_audio_rscode_t *rscode_new = tdav_audio_rscode_create(audio, base->rtp_manager->sessionid, RSC_TYPE_ENCODE, base->rtp_manager);
			rscode_new->uBusinessId = self->uBusinessId;
            tdav_audio_rscode_start(rscode_new);
			rscode = rscode_new;
			tsk_list_lock(audio->rscode_list);
			tsk_list_push_back_data(audio->rscode_list, (tsk_object_t**)&rscode_new);
			tsk_list_unlock(audio->rscode_list);
		}

        if (self->uBusinessId != 0) {
            /*  | extension magic num（2byte） |    extension lenth（2byte）          |
                |    data type（2byte）        |    data size（2byte）                |
                |  data(以4字节对齐，数据不是4的整数倍，在data高位第一个4字节前面补0x00)      |
            */
            // BusinessID is put into rtp extension
            // 16bit magic num + 16bit extension lenth + 16bit type lenth + 16bit business id len + business id size
            
           // if (packet->header->extension == 0) {
                packet->extension.size = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE+RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
                packet->extension.data = (unsigned char *)malloc(packet->extension.size);
                memset(packet->extension.data, 0, packet->extension.size);

                //往rtp head extension中写入魔法数字
                offset = 0;
                uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
                memcpy((unsigned char *)packet->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
                offset+=RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

                //往rtp head extension中写入扩展内容长度
                uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
                uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
                memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
                offset+=RTP_EXTENSION_LEN_PROFILE;

                //往rtp head extension中写入内容数据类型
                uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
                memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
                offset+=RTP_EXTENSION_TYPE_PROFILE;

                //往rtp head extension中写入内容数据长度
                uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
                memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
                offset+=BUSINESS_ID_LEN_PROFILE;

                //往rtp head extension中写入内容数据
                uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
                memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
                offset += sizeof(uint32_t);
                uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
                memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
                
                packet->header->extension = 1;
            //}
        }

		tdav_audio_rscode_push_rtp_packet(rscode, packet);
		return ;
	}

	tsk_list_lock(audio->combinePacket_list);
	trtp_rtp_packet_t* pTmpPacket = (trtp_rtp_packet_t*)tsk_object_ref(packet);
	tsk_list_push_back_data(audio->combinePacket_list, (tsk_object_t**)&pTmpPacket);
	
	//不够合包的个数就返回
	tsk_size_t totalPktSize = tsk_list_count_all(audio->combinePacket_list);
	if (totalPktSize < audio->iCombineCount)
	{
		tsk_list_unlock(audio->combinePacket_list);
		return;
	}
	//取出包,放入一个临时队列
	tsk_list_t* needSendLists = tsk_list_create();
	for (int i = 0; i < audio->iCombineCount;i++)
	{
        tsk_list_item_t* item = tsk_list_pop_first_item(audio->combinePacket_list);
		tsk_object_t* tmp = tsk_object_ref(item->data);
		tsk_list_push_back_data(needSendLists, &tmp);
        TSK_OBJECT_SAFE_FREE(item);
	}
	tsk_list_unlock(audio->combinePacket_list);

	//并且合成一个rtp 包。 构造一个rtp头，然后将 音频包组合到一个buffer 里面，放到payload。
	//组合的时候 2 字节的packet 长度，后面是数据，依次搞下去
	trtp_rtp_packet_t* pCombinePacket = trtp_rtp_packet_copy(packet); //拷贝一个rtp 头
	pCombinePacket->header->marker = 1;
	pCombinePacket->header->extension = 0;
    pCombinePacket->payload.size      = 0;
	//遍历一下 临时列表，计算payload 的总长度
	tsk_list_item_t* item = tsk_null;
	tsk_list_foreach(item, needSendLists) {
		trtp_rtp_packet_t* tmp = (trtp_rtp_packet_t*)item->data;

		pCombinePacket->payload.size += 2;//两字节长度
		pCombinePacket->payload.size += trtp_rtp_packet_guess_serialbuff_size(tmp);
	}
	//申请空间
	pCombinePacket->payload.data = tsk_malloc(pCombinePacket->payload.size);
	int iOffset = 0;
	tsk_list_foreach(item, needSendLists) {
		trtp_rtp_packet_t* tmp = (trtp_rtp_packet_t*)item->data;		
		uint16_t pkgSize = (uint16_t)trtp_rtp_packet_guess_serialbuff_size(tmp);
		uint16_t netSize = tnet_htons(pkgSize);
		//复制长度
		memcpy((char*)pCombinePacket->payload.data + iOffset, &netSize, sizeof(uint16_t));
		iOffset += sizeof(uint16_t);
		//拷贝数据
		trtp_rtp_packet_serialize_to(tmp, (char*)pCombinePacket->payload.data + iOffset, pkgSize);
		iOffset += pkgSize;
	}

	tdav_session_av_t *base = (tdav_session_av_t *)audio;
    trtp_manager_t *self = base->rtp_manager;
	tdav_audio_rscode_t* rscode = tdav_session_audio_select_rscode_by_sessionid(audio, base->rtp_manager->sessionid, RSC_TYPE_ENCODE);
	if (!rscode) {
		tdav_audio_rscode_t *rscode_new = tdav_audio_rscode_create(audio, base->rtp_manager->sessionid, RSC_TYPE_ENCODE, base->rtp_manager);
		rscode_new->uBusinessId = self->uBusinessId;
        tdav_audio_rscode_start(rscode_new);
		rscode = rscode_new;
		tsk_list_lock(audio->rscode_list);
		tsk_list_push_back_data(audio->rscode_list, (tsk_object_t**)&rscode_new);
		tsk_list_unlock(audio->rscode_list);
	}

    if (self->uBusinessId != 0) {
        /*  | extension magic num（2byte） |    extension lenth（2byte）          |
            |    data type（2byte）        |    data size（2byte）                |
            |  data(以4字节对齐，数据不是4的整数倍，在data高位第一个4字节前面补0x00)      |
        */
        // BusinessID is put into rtp extension
        // 16bit magic num + 16bit extension lenth + 16bit type lenth + 16bit business id len + business id size
        pCombinePacket->extension.size = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE+RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
        pCombinePacket->extension.data = (unsigned char *)malloc(pCombinePacket->extension.size);
        memset(pCombinePacket->extension.data, 0, pCombinePacket->extension.size);

        //往rtp head extension中写入魔法数字
        offset = 0;
        uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
        offset+=RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

        //往rtp head extension中写入扩展内容长度
        uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
        uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
        offset+=RTP_EXTENSION_LEN_PROFILE;

        //往rtp head extension中写入内容数据类型
        uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
        offset+=RTP_EXTENSION_TYPE_PROFILE;

        //往rtp head extension中写入内容数据长度
        uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
        offset+=BUSINESS_ID_LEN_PROFILE;

        //往rtp head extension中写入内容数据
        uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
        memcpy((unsigned char *)pCombinePacket->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
        
        pCombinePacket->header->extension = 1;
    }

	tdav_audio_rscode_push_rtp_packet(rscode, pCombinePacket);
	tsk_object_unref(pCombinePacket);
	TSK_OBJECT_SAFE_FREE(needSendLists);
}

static void tdav_session_audio_open_pcm_files(tdav_session_audio_t *audio, int type)
{
    static char dumpPcmPath[1024] = "";
    const char *pBaseDir = NULL;
    int baseLen = 0;
    
    pBaseDir = tmedia_defaults_get_app_document_path();
    
    if (NULL == pBaseDir) {
        return;
    }
    
    strncpy(dumpPcmPath, pBaseDir, sizeof(dumpPcmPath) - 1);
    baseLen = strlen(dumpPcmPath) + 1; // plus the trailing '\0'
    
    switch (type) {
        case TDAV_AUDIO_PCM_FILE_MIX_MIC:
            strncat(dumpPcmPath, "/dump_mix_mic.pcm", sizeof(dumpPcmPath) - baseLen);
            if (audio->pcm_file.mic) {
                fclose(audio->pcm_file.mic);
            }
            audio->pcm_file.mic = fopen (dumpPcmPath, "wb");
            audio->pcm_file.mic_size = 0;
            break;
        case TDAV_AUDIO_PCM_FILE_MIX_SPEAKER:
            strncat(dumpPcmPath, "/dump_mix_speaker.pcm", sizeof(dumpPcmPath) - baseLen);
            if (audio->pcm_file.speaker) {
                fclose(audio->pcm_file.speaker);
            }
            audio->pcm_file.speaker = fopen (dumpPcmPath, "wb");
            audio->pcm_file.speaker_size = 0;
            break;
        case TDAV_AUDIO_PCM_FILE_SEND_PCM:
            strncat(dumpPcmPath, "/dump_mix_send.pcm", sizeof(dumpPcmPath) - baseLen);
            if (audio->pcm_file.send_pcm) {
                fclose(audio->pcm_file.send_pcm);
            }
            audio->pcm_file.send_pcm = fopen (dumpPcmPath, "wb");
            audio->pcm_file.send_pcm_size = 0;
            break;
        default:
            return;
    }
}

static void tdav_session_audio_close_pcm_files(tdav_session_audio_t *audio)
{
    if (audio->pcm_file.mic) {
        fclose (audio->pcm_file.mic);
        audio->pcm_file.mic = NULL;
        audio->pcm_file.mic_size = 0;
    }
    if (audio->pcm_file.speaker) {
        fclose (audio->pcm_file.speaker);
        audio->pcm_file.speaker = NULL;
        audio->pcm_file.speaker_size = 0;
    }
    if (audio->pcm_file.send_pcm) {
        fclose (audio->pcm_file.send_pcm);
        audio->pcm_file.send_pcm = NULL;
        audio->pcm_file.send_pcm_size = 0;
    }
}

/* DTMF event object */
typedef struct tdav_session_audio_dtmfe_s
{
    TSK_DECLARE_OBJECT;

    xt_timer_id_t timer_id;
    trtp_rtp_packet_t *packet;

    const tdav_session_audio_t *session;
} tdav_session_audio_dtmfe_t;
extern const tsk_object_def_t *tdav_session_audio_dtmfe_def_t;

// RTP/RTCP callback (From the network to the consumer)
static int tdav_session_audio_consume_cb(tdav_session_audio_t *audio, const struct trtp_rtp_packet_s *packet)
{
	tmedia_codec_t *codec = tsk_null;
	tdav_session_av_t *base = (tdav_session_av_t *)audio;
	int ret = -1;

	if (!audio || !packet || !packet->header)
	{
		TSK_DEBUG_ERROR("Invalid parameter");
		goto bail;
	}

	if (audio->is_started && base->consumer && base->consumer->is_started)
	{
		//tsk_size_t out_size = 0;
		//uint16_t curSeqNo = 0;

		// Find the codec to use to decode the RTP payload
		if (!audio->decoder.codec || audio->decoder.payload_type != packet->header->payload_type)
		{
			tsk_istr_t format;
			TSK_OBJECT_SAFE_FREE(audio->decoder.codec);
			tsk_itoa(packet->header->payload_type, &format);
			if (!(audio->decoder.codec = tmedia_codec_find_by_format(TMEDIA_SESSION(audio)->neg_codecs, format)) || !audio->decoder.codec->plugin || (!audio->decoder.codec->plugin->decode && !audio->decoder.codec->plugin->decode_new))
			{
				TSK_DEBUG_ERROR("%s is not a valid payload for this session", format);
				ret = -2;
				goto bail;
			}
			audio->decoder.payload_type = packet->header->payload_type;
		}
		// ref() the codec to be able to use it short time after stop(SAFE_FREE(codec))
		if (!(codec = (tmedia_codec_t *)tsk_object_ref(TSK_OBJECT(audio->decoder.codec))))
		{
			TSK_DEBUG_ERROR("Failed to get decoder codec");
			goto bail;
		}

		// If NetEq is enabled, decoding will be done inside NetEq
		//if (!tmedia_defaults_get_neteq_applied()) {
		//    // Open codec if not already done
		//    if (!TMEDIA_CODEC (codec)->opened)
		//    {
		//        tsk_safeobj_lock (base);
		//        if ((ret = tmedia_codec_open (codec)))
		//        {
		//            tsk_safeobj_unlock (base);
		//            TSK_DEBUG_ERROR ("Failed to open [%s] codec", codec->plugin->desc);
		//            TSK_OBJECT_SAFE_FREE (audio->decoder.codec);
		//            goto bail;
		//        }
		//        tsk_safeobj_unlock (base);
		//    }
		//}

		// Calculate |receive_timestamp| from |receive_timestamp_ms|.
		// We cannot determine the clock rate in the RTP module, so we delay the calculation here.
		packet->header->receive_timestamp = (uint32_t)TMEDIA_CODEC_MS_TO_RTP_TICK_AUDIO_DECODING(audio->decoder.codec, packet->header->receive_timestamp_ms);
		packet->header->payload_type_frequency = (uint32_t)TMEDIA_CODEC_RTP_CLOCK_RATE_AUDIO_DECODING(audio->decoder.codec);

		// Decode the RTP header extension
		tdav_session_audio_decode_rtp_header_ext(audio, packet);

		// If NetEq is enabled, decoding will be done inside NetEq
		//if (tmedia_defaults_get_neteq_applied()) {
		tmedia_consumer_consume(base->consumer, packet->payload.data, packet->payload.size, packet->header);
		//} else if (atoi(TMEDIA_CODEC_FORMAT_OPUS) == packet->header->payload_type) {
		//    // Start decoding the FEC/PLC and the current packet
		//    curSeqNo = packet->header->seq_num;
		//    do
		//    {
		//        // Decode data
		//        // On return, if FEC/PLC applied, the seq_num and is_fec_plc will be changed accordingly.
		//        packet->header->seq_num = curSeqNo;
		//        RTP_PACKET_FLAG_CLEAR(packet->header->flag, RTP_PACKET_FLAG_FEC);
		//        RTP_PACKET_FLAG_CLEAR(packet->header->flag, RTP_PACKET_FLAG_PLC);
		//        out_size = codec->plugin->decode (codec, packet->payload.data, packet->payload.size, &audio->decoder.buffer, &audio->decoder.buffer_size, packet->header);

		//        if ((out_size > 0) && audio->is_started)
		//        {
		//                    if (NULL != audio->m_pReceiveFile)
		//                    {
		//                        fwrite (audio->decoder.buffer, 1, out_size, audio->m_pReceiveFile);
		//                    }
		// consume the frame
		//            tmedia_consumer_consume (base->consumer, audio->decoder.buffer, out_size, packet->header);
		//        }
		//    } while (RTP_PACKET_FLAG_CONTAINS(packet->header->flag, RTP_PACKET_FLAG_FEC)
		//             || RTP_PACKET_FLAG_CONTAINS(packet->header->flag, RTP_PACKET_FLAG_PLC));
		//}

		// Save the bandwidth control data returned from the consumer for the uses of the encoders
		if (packet->header->bc_to_send.isValid || packet->header->bc_received.isValid) {
			tsk_safeobj_lock(&(audio->bc));
			if (packet->header->bc_to_send.isValid) {
				audio->bc.to_send = packet->header->bc_to_send;
				packet->header->bc_to_send.isValid = 0;
			}
			if (packet->header->bc_received.isValid) {
				audio->bc.received = packet->header->bc_received;
				packet->header->bc_received.isValid = 0;
			}
			tsk_safeobj_unlock(&(audio->bc));
		}
	}
	else
	{
#ifdef MAC_OS
#else
		// When failed to init OpenSL, and in faked recording mode, this will print too much 
		//TSK_DEBUG_INFO ("Session audio not ready");
#endif
	}

	// everything is ok
	ret = 0;

bail:
	tsk_object_unref(TSK_OBJECT(codec));
	return ret;
}


int tdav_session_audio_rscode_cb(tdav_session_audio_t *audio, const struct trtp_rtp_packet_s *packet)
{
	if (packet->header->marker == 0)
	{
		//直接consume
		tdav_session_audio_consume_cb(audio, packet);
		return 0;
	}
	uint64_t curTimeMs = tsk_gettimeofday_ms();
	//此处需要解包，因为对方发过来的会有合包
	int iOffset = 0;

	while (iOffset < packet->payload.size)
	{
		uint16_t pkgSize = 0;
		memcpy(&pkgSize, (char*)packet->payload.data + iOffset, sizeof(pkgSize));
		pkgSize = tnet_ntohs(pkgSize);
		iOffset += sizeof(pkgSize);
		//序列化一个rtp 包出来
		trtp_rtp_packet_t* tmp = trtp_rtp_packet_deserialize((char*)packet->payload.data + iOffset, pkgSize);
        //可能为null导致crash
        if(!tmp)
            break;
		tmp->header->my_session_id = packet->header->my_session_id;
		tmp->header->session_id = packet->header->session_id;
		tmp->header->receive_timestamp_ms = curTimeMs++;

		iOffset += pkgSize;
		tdav_session_audio_consume_cb(audio, tmp);
		tsk_object_unref(tmp);
	}
	return 0;
}
// RTP/RTCP callback (From the network to the consumer)
static int tdav_session_audio_rtp_cb (const void *callback_data, const struct trtp_rtp_packet_s *packet)
{
    tdav_session_audio_t *audio = (tdav_session_audio_t *)callback_data;
    //tdav_session_av_t *base = (tdav_session_av_t *)callback_data;
    int ret = -1;
    
    if (!audio || !packet || !packet->header)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
   
	int session_id = -1;
	if (packet->header->csrc_count > 0) {
		session_id = (int32_t)packet->header->csrc[0];
	}
	tdav_audio_rscode_t* rscode = tdav_session_audio_select_rscode_by_sessionid(audio, session_id, RSC_TYPE_DECODE);
	if (!rscode) {
		tdav_audio_rscode_t *rscode_new = tdav_audio_rscode_create(audio, session_id, RSC_TYPE_DECODE, tsk_null);
		tdav_audio_rscode_start(rscode_new);
		rscode = rscode_new;
		tsk_list_lock(audio->rscode_list);
		tsk_list_push_back_data(audio->rscode_list, (tsk_object_t**)&rscode_new);
		tsk_list_unlock(audio->rscode_list);
	}
	tdav_audio_rscode_push_rtp_packet(rscode, (trtp_rtp_packet_t *)packet);
    return ret;
}


//#define USE_PCM_TEST_DATA
#if defined(USE_PCM_TEST_DATA)
    #define USE_PCM_TEST_DATA_1
    uint32_t pcm_test_pos = 0;
    #if defined(USE_PCM_TEST_DATA_1)
        #include "pcm_test1.h"
        uint8_t* pcm_test_data = pcm_test1_data;
        uint32_t pcm_test_size = sizeof(pcm_test1_data);
    #elif defined(USE_PCM_TEST_DATA_2)
        #include "pcm_test2.h"
        uint8_t* pcm_test_data = pcm_test2_data;
        uint32_t pcm_test_size = sizeof(pcm_test2_data);
    #endif
#endif //defined(USE_PCM_TEST_DATA)

// Producer callback (From the producer to the network). Will encode() data before sending
static int tdav_session_audio_producer_enc_cb (const void *callback_data, const void *buffer, tsk_size_t size)
{
    int ret = 0;

#if defined(USE_PCM_TEST_DATA)
    if (size > (pcm_test_size - pcm_test_pos)) {
        pcm_test_pos = 0;
    }
    memcpy((void*)buffer, &pcm_test_data[pcm_test_pos], size);
    pcm_test_pos += size;
#endif // defined(USE_PCM_TEST_DATA)

    tdav_session_audio_t *audio = (tdav_session_audio_t *)callback_data;
    tdav_session_av_t *base = (tdav_session_av_t *)callback_data;
    
    if (!audio)
    {
        TSK_DEBUG_ERROR ("Null session");
        return 0;
    }

    // do nothing if session is held
    // when the session is held the end user will get feedback he also has possibilities to put the
    // consumer and producer on pause
    if (TMEDIA_SESSION (audio)->lo_held)
    {
        return 0;
    }
    
    if ((NULL == audio->encoder.free_frame_buf_lst)
        || (NULL == audio->encoder.filled_frame_buf_lst)
        || (NULL == audio->encoder.filled_frame_sema))
    {
        return tdav_session_audio_producer_handle_recorded_data ((const tmedia_session_t*)callback_data, buffer, size);
    }

    if (audio->is_started && base->rtp_manager && base->rtp_manager->is_started)
    {
        tsk_list_item_t *free_item;
        size_t item_count;
        
        if (size > MAX_FRAME_SIZE)
        {
            TSK_DEBUG_WARN("Recorded frame size(%lu) exceeds the max(%d)", size, MAX_FRAME_SIZE);
            size = MAX_FRAME_SIZE;
        }

#ifdef OS_LINUX
        prctl(PR_SET_NAME, "audio_p_enc");
#endif
        
        tsk_list_lock (audio->encoder.free_frame_buf_lst);
        free_item = tsk_list_pop_first_item (audio->encoder.free_frame_buf_lst);
        tsk_list_unlock (audio->encoder.free_frame_buf_lst);
        
        if (NULL == free_item)
        {
            tsk_list_lock(audio->encoder.filled_frame_buf_lst);
            item_count = tsk_list_count (audio->encoder.filled_frame_buf_lst, tsk_null, tsk_null);
            tsk_list_unlock(audio->encoder.filled_frame_buf_lst);

            if (audio->recOverflowCount < MAX_BUF_FRAME_NUM) {
                TSK_DEBUG_INFO("No free item available, MAX_BUF_FRAME_NUM:%d, item_count:%d", MAX_BUF_FRAME_NUM, (int)item_count);
            }
            audio->recOverflowCount++;
            
            if (item_count < MAX_BUF_FRAME_NUM)
            {
                free_item = tsk_list_item_create();
                if (free_item)
                {
                    free_item->data = tsk_object_new (tdav_session_audio_frame_buffer_def_t, MAX_FRAME_SIZE);
                }
            } else {
                // When comes here, it means the recording module was producing too fast(e.g. due to some bugs),
                // we need to sleep for a while to avoid exhaust the CPU.
		        //TSK_DEBUG_INFO("moon XSleep item_count=%d\n", item_count);
                //printf("**   XSleep  tdav_session_audio_producer_enc_cb ** filled_frame_buf_lst item_count=%d  %lld\n",item_count,tsk_gettimeofday_ms());
#ifdef ARMV7
                XSleep(5);
#else
                XSleep(40);
#endif
            }
        }
        
        if (free_item)
        {
            if (free_item->data)
            {
                tdav_session_audio_frame_buffer_t* frame_buf = (tdav_session_audio_frame_buffer_t*)free_item->data;
                memcpy(frame_buf->pcm_data, buffer, size);
                frame_buf->pcm_size = size;
            }
            tsk_list_lock(audio->encoder.filled_frame_buf_lst);
            tsk_list_push_item (audio->encoder.filled_frame_buf_lst, &free_item, tsk_true);
            tsk_list_unlock(audio->encoder.filled_frame_buf_lst);
            tsk_semaphore_increment(audio->encoder.filled_frame_sema);
            
        }
    }

    return ret;
}

static int32_t tadv_session_audio_calc_power_db(const uint8_t* buffer, tsk_size_t size)
{
    uint32_t pcm_avg = 0;
    uint32_t pcm_sum = 0;
    uint32_t pcm_count = 0;
    int32_t  power_db = 0;
    int32_t  index;

    if (!buffer || size <= 1) {
        return 0;
    }
    
    size -= 1;
    for (index = 0; index < size; index += 2) {
        pcm_sum += abs((int16_t)(buffer[index] | buffer[index+1] << 8));
        pcm_count++;
    }
    
    if (pcm_count > 0) {
        pcm_avg = pcm_sum / pcm_count;
    }
    
    power_db = (int32_t)(20 * log10((double)pcm_avg / 32767));
    return power_db;
}

static void tdav_session_audio_handle_mic_level_callback(tdav_session_audio_t* audio, const uint8_t* buffer, tsk_size_t size)
{
    // If the audio level less than this value, we take it as silence.
#define SILENCE_DB  (60)
    
    int32_t level = 0;

    if (!audio || !buffer || size <= 1) {
    	return;
    }
    
    if (audio->isMicMute) {
        if (audio->mic_level.callback && (audio->mic_level.last_level != 0)) {
            audio->mic_level.last_level = 0;
            audio->mic_level.callback(audio->mic_level.last_level);
        }
        return;
    }
    
    level = tadv_session_audio_calc_power_db(buffer, size) + SILENCE_DB;
    if (level < 0) {
        level = 0;
    }
    if (level > SILENCE_DB) {
        level = SILENCE_DB;
    }
    level = (int32_t)(level * audio->mic_level.max_level / SILENCE_DB);
    //char str[20];
    //memset (str, '1', sizeof(str));
    //str[level] = '\0';
    //TSK_DEBUG_INFO("%s", str);
    if (audio->mic_level.callback && (level != audio->mic_level.last_level)) {
        audio->mic_level.last_level = level;
        audio->mic_level.callback(audio->mic_level.last_level);
    }
}

// Handle recorded data taken from the frame list (From the producer to the network). Will encode() data before sending
static int tdav_session_audio_producer_handle_recorded_data (const tmedia_session_t *self, const void *buffer, tsk_size_t size)
{
    int ret = 0;
    
    tdav_session_audio_t *audio = (tdav_session_audio_t *)self;
    tdav_session_av_t *base = (tdav_session_av_t *)self;
    long timeStamp = 0;
    if (!audio)
    {
        TSK_DEBUG_ERROR ("Null session");
        return 0;
    }
    // do nothing if session is held
    // when the session is held the end user will get feedback he also has possibilities to put the
    // consumer and producer on pause
    if (TMEDIA_SESSION (audio)->lo_held)
    {
        return 0;
    }

    // get best negotiated codec if not already done
    // the encoder codec could be null when session is renegotiated without re-starting (e.g.
    // hold/resume)
    if (!audio->encoder.codec)
    {
        const tmedia_codec_t *codec;
        tsk_safeobj_lock (base);
        if (!(codec = tdav_session_av_get_best_neg_codec (base)))
        {
            TSK_DEBUG_ERROR ("No codec matched");
            tsk_safeobj_unlock (base);
            return -2;
        }
        audio->encoder.codec = (tmedia_codec_t *)tsk_object_ref (TSK_OBJECT (codec));
        tsk_safeobj_unlock (base);
    }

    if (audio->is_started && base->rtp_manager && base->rtp_manager->is_started)
    {
        /* encode */
        tsk_size_t out_size = 0;
        int iMicMixAudExist = 0;
        tsk_bool_t silence_or_noise = tsk_false;

        // Open codec if not already done
        if (!audio->encoder.codec->opened)
        {
            tsk_safeobj_lock (base);
            if ((ret = tmedia_codec_open (audio->encoder.codec, self)))
            {
                tsk_safeobj_unlock (base);
                TSK_DEBUG_ERROR ("Failed to open [%s] codec", audio->encoder.codec->plugin->desc);
                return -4;
            }
            tsk_safeobj_unlock (base);
        }

        // 如果不调用，将收不到udp数据
        // 无论是否有语音数据，始终每2s发送一个dummy包
        tsk_mutex_lock(base->rtp_manager_mutex);
        trtp_manager_send_rtp_dummy(base->rtp_manager);
        tsk_mutex_unlock(base->rtp_manager_mutex);

        if (tmedia_defaults_get_external_input_mode()) {
            if (tsk_list_count_all(audio->mic_mix_aud.mixaud_filled_lst) < 1) {
                //TSK_DEBUG_ERROR("no output data, wait a tick");
                return 0;
            } else {
                // Handle mix audio to encoder
                iMicMixAudExist = tadv_session_audio_get_mixaud(audio, (void *)buffer, size, &timeStamp);
                if (iMicMixAudExist <= 0) {
                    TSK_DEBUG_ERROR ("Failed to get mix data, %d", iMicMixAudExist);
                    if (tmedia_defaults_get_external_input_mode()) {
                        return 0;
                    }
                }
            }
        }
        // If it's not normal audio data(e.g. a silence frame for setting up the connection with the server)
        // then no preprocessing is needed.
        //
        // Denoise (VAD, AGC, Noise suppression, ...)
        // Must be done after resampling
        #if !defined(USE_PCM_TEST_DATA)
        if (audio->denoise)
        {
            ret = tmedia_denoise_process_record (TMEDIA_DENOISE (audio->denoise), (void *)buffer, (uint32_t)size, &silence_or_noise);
        }
        #endif // !defined(USE_PCM_TEST_DATA)
        
        if (audio->micVolumeGain != 1.0f) {
            tdav_codec_apply_volume_gain(audio->micVolumeGain, (void*)buffer, size, 2);
        }

#if 0
        if (audio->micVolumeGain != 0.0f && iMicMixAudExist && audio->resample) {
            tdav_audio_unit_t* audio_unit = tdav_audio_unit_create(size);
            memcpy(audio_unit->data, (void *)buffer, size);
            audio_unit->timestamp = 0;
            audio_unit->sessionid = 0;
            tdav_audio_resample_push(audio->resample, audio_unit);
            TSK_OBJECT_SAFE_FREE(audio_unit);
        }
#endif
        
        if (audio->mic_level.callback && audio->mic_level.max_level > 0) {
            tdav_session_audio_handle_mic_level_callback(audio, (uint8_t*)buffer, size);
        }
        
        // Apply the bandwidth control data before encoding.
        tdav_session_audio_apply_bandwidth_ctrl(audio);
        
        if(audio->isRecordCallbackOn && audio->pcmCallback){
           tdav_session_audio_frame_buffer_t* frame_buf_cb_record =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, size);
           if( frame_buf_cb_record )
           {
               memcpy(frame_buf_cb_record->pcm_data, (void *)buffer, size );
               frame_buf_cb_record->pcm_size = size;
               frame_buf_cb_record->channel_num = 1;
               frame_buf_cb_record->sample_rate = audio->encoder.codec->in.rate;
               frame_buf_cb_record->is_float = 0;
               frame_buf_cb_record->is_interleaved = 1;
               frame_buf_cb_record->bytes_per_sample = 2;
               audio->pcmCallback(frame_buf_cb_record,0x2);// Pcbf_record
               tsk_object_unref(frame_buf_cb_record);
           }
       }

        if (!tmedia_defaults_get_external_input_mode()) {
            // Handle mix audio to encoder
            iMicMixAudExist = tadv_session_audio_get_mixaud(audio, (void *)buffer, size, &timeStamp);
        }
        
        //dump pcm before encode and send
        if (audio->pcm_file.send_pcm) {
            if (audio->pcm_file.send_pcm_size > audio->pcm_file.max_size) {
                tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_SEND_PCM );
            }
            if (audio->pcm_file.send_pcm) {
                fwrite (buffer, 1, size, audio->pcm_file.send_pcm);
                audio->pcm_file.send_pcm_size += size;
            }
        }

        // Encode data
        if ((audio->isMicMute) && (iMicMixAudExist == 0)) {
            // do nothing
        } else {
            if ((audio->encoder.codec = (tmedia_codec_t *)tsk_object_ref (audio->encoder.codec)))
            {
                tsk_bool_t need_send_rtp = tsk_true;
                out_size = audio->encoder.codec->plugin->encode(audio->encoder.codec, buffer, size, &audio->encoder.buffer, &audio->encoder.buffer_size);
                if (out_size <= 1) {
                    audio->encoder.codec_dtx_current_num++;
                    if(audio->encoder.codec_dtx_current_num == 1){
                        need_send_rtp = true;
                    }else if(audio->encoder.codec_dtx_current_num % audio->encoder.codec_dtx_peroid_num == 0) {
                        audio->encoder.codec_dtx_current_num = 0;
                        need_send_rtp = true;
                    }else {
                        need_send_rtp = false;
                    }
                }else {
                    audio->encoder.codec_dtx_current_num = 0;
                    need_send_rtp = true;
                }
                if (need_send_rtp && out_size)
                {
                    uint32_t duration = TMEDIA_CODEC_FRAME_DURATION_AUDIO_ENCODING (audio->encoder.codec);
                    tsk_size_t ext_size = tdav_session_audio_encode_rtp_header_ext(audio, audio->encoder.buffer, out_size, silence_or_noise);
                    tsk_mutex_lock(base->rtp_manager_mutex);
                   /* if (ext_size > 0) {
                        trtp_manager_send_rtp_with_extension (base->rtp_manager, audio->encoder.buffer, out_size, duration,
                                                              tsk_false, tsk_true, audio->encoder.rtpExtBuffer, ext_size);
                    } else {
                        trtp_manager_send_rtp_with_extension (base->rtp_manager, audio->encoder.buffer, out_size, duration,
                                                              tsk_false, tsk_true, tsk_null, 0);
                    }*/
					
					//引用计数1
					trtp_rtp_packet_t* packet =trtp_manager_gen_rtp_with_extension(base->rtp_manager, audio->encoder.buffer, out_size, duration,
						tsk_false, tsk_true, audio->encoder.rtpExtBuffer, ext_size);
					//先把rtp 包放进缓存队列
					tdav_session_audio_send_combine_packet(audio, packet);									
					TSK_OBJECT_SAFE_FREE(packet);

                    // AddAudioCode( (int)out_size , TMEDIA_SESSION(audio)->sessionid );
                    
                    tsk_mutex_unlock(base->rtp_manager_mutex);
                }
                tsk_object_unref (audio->encoder.codec);
            }
            else
            {
                TSK_DEBUG_WARN ("No encoder");
            }
        }
    }

done:
    return ret;
}

static void* tdav_session_audio_producer_thread(void* param)
{
    tdav_session_audio_t *audio = TDAV_SESSION_AUDIO (param);
    tsk_list_item_t *filled_item;
    uint32_t packet_count = 0;

    if ( (NULL == audio) || (tsk_false == audio->is_started)
        || (NULL == audio->encoder.free_frame_buf_lst)
        || (NULL == audio->encoder.filled_frame_buf_lst)
        || (NULL == audio->encoder.filled_frame_sema))
    {
        TSK_DEBUG_ERROR("Producer thread failed to start");
        return NULL;
    }
    
    TSK_DEBUG_INFO("Producer thread starts");
#ifdef OS_LINUX
    prctl(PR_SET_NAME, "audio_producer");
#endif
    
    while (audio->is_started) {
        if (0 != tsk_semaphore_decrement(audio->encoder.filled_frame_sema)) {
            TSK_DEBUG_ERROR("Fatal error: filled_frame_sema failed");
            break;
        }
        // When successfully got one semaphore item, need to check the status first,
        // because this item may be set by the stop() function, and it's only for exiting,
        // not means there's a item in the filled_list.
        if (!audio->is_started) {
            break;
        }
        tsk_list_lock(audio->encoder.filled_frame_buf_lst);
        filled_item = tsk_list_pop_first_item (audio->encoder.filled_frame_buf_lst);
        tsk_list_unlock(audio->encoder.filled_frame_buf_lst);

        if (NULL == filled_item) {
            TSK_DEBUG_ERROR("Fatal error: got an empty recorded item");
        } else {
            // Check the status again. This can make the stopping faster.
        	if (audio->is_started) {
                tdav_session_audio_frame_buffer_t* frame_buf = (tdav_session_audio_frame_buffer_t*)filled_item->data;
                tdav_session_audio_producer_handle_recorded_data ((tmedia_session_t*)audio, frame_buf->pcm_data, frame_buf->pcm_size);

                if ( ((packet_count < 1000) && ((packet_count % 200) == 0))
                    || ((packet_count >= 1000) && ((packet_count % 1000) == 0)) ) {
                    if (frame_buf->pcm_size >= 16) {
                        uint8_t *tmpBuf = (uint8_t*)frame_buf->pcm_data;
                        TSK_DEBUG_INFO("Mic: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                                       tmpBuf[0], tmpBuf[1], tmpBuf[2], tmpBuf[3], tmpBuf[4], tmpBuf[5], tmpBuf[6], tmpBuf[7],
                                       tmpBuf[8], tmpBuf[9], tmpBuf[10], tmpBuf[11], tmpBuf[12], tmpBuf[13], tmpBuf[14], tmpBuf[15]);
                    }
                }
                packet_count++;
            }

            // Even if it's going to stop, the item should be pushed to the list, otherwise there will be memory leak.
            tsk_list_lock (audio->encoder.free_frame_buf_lst);
            tsk_list_push_item (audio->encoder.free_frame_buf_lst, &filled_item, tsk_true);
            tsk_list_unlock (audio->encoder.free_frame_buf_lst);
        }
    };
    
    TSK_DEBUG_INFO("Producer thread stopped");
    
    return NULL;
}

static void tdav_session_audio_apply_bandwidth_ctrl(tdav_session_audio_t *audio)
{
    BandwidthCtrlMsg_t bc_received = { 0 };
    
    if (!audio || !audio->encoder.codec) {
        return;
    }
    // Check the validity before locking. This can reduce the times of locking.
    // Even if the validity changes just after checking, the worst case is missing a valid feedback data.
    // Since the feedback data is just something about statistics, so it doen't really matter.
    bc_received.isValid = 0;
    if (audio->bc.received.isValid) {
        tsk_safeobj_lock(&(audio->bc));
        bc_received = audio->bc.received;
        audio->bc.received.isValid = 0;
        tsk_safeobj_unlock(&(audio->bc));
    }
    
    if (bc_received.isValid) {
        if (BWCtrlMsgServerCommand == bc_received.msgType) {
            audio->encoder.codec->packet_loss_perc = bc_received.msg.server.redunantPercent;
        } else if (BWCtrlMsgClientReport == bc_received.msgType) {
            audio->encoder.codec->packet_loss_perc = bc_received.msg.client.packetLossRateQ8 * 100 / 255;
        }
    }
}

//将外部(CYouMeVoiceEngine::mixAudioTrack)提供的音频数据，混合到麦克风的音轨里面。
static void tdav_session_audio_put_mixaud(tdav_session_audio_t *audio, void *frame)
{
    tsk_list_item_t *free_item;
    //size_t  item_count;
    //static uint32_t u4MixaudPutCnt = 0;
    if (!audio || !frame) {
        return;
    }
    if (!audio->is_started) {
        return;
    }
    if (!audio->mic_mix_aud.mixaud_free_lst || !audio->mic_mix_aud.mixaud_filled_lst || !audio->mic_mix_aud.mixaud_sema_lst) {
        TSK_DEBUG_ERROR("Mic mix audio related items ISNT be initialized!!");
        return;
    }
    
    bool bTimeout = false;
#ifdef MAC_OS
    
    //为了处理mac下拔掉麦克风，引起信号不发送，而停止线程取不到锁而被卡死的问题，增加了信号超时
    //又为了信号来的不均匀，而不想把超时时间设置得太长，所以增加了超时计数
    if( !tmedia_defaults_get_external_input_mode() )
    {
        //超时计数，为了处理
        for( int i = 0 ; i < 2; i++ )
        {
            int ret = tsk_semaphore_decrement_time(audio->mic_mix_aud.mixaud_sema_lst, 50 );
            if( ret != 0 )
            {
//                audio->mic_mix_aud.mixau_timeout_count++;
                bTimeout= true;
                break;
            }
            else{
                if( audio->mic_mix_aud.mixau_timeout_count != 0 )
                {
                    audio->mic_mix_aud.mixau_timeout_count -= 1;
                    continue;
                }
                else{
                    bTimeout = false;
                    break;
                    
                }
            }
        }
    }
    
#else
    if (!tmedia_defaults_get_external_input_mode() && (0 != tsk_semaphore_decrement_time(audio->mic_mix_aud.mixaud_sema_lst,45))) {
        // External mode should't block/pass external input
        TSK_DEBUG_ERROR("Fatal error: filled_frame_sema failed");
        return;
    }
#endif
    
	// Should check the status just after being unblocked from the semaphore. 
	// Because the stop function will increment the semaphore by 1 when it's
	// going to stop, if at this moment the semaphore is blocking, it will be
	// unblocked immediately. If we don't check the flag in this case, we will
	// miss the last chance to exit.
    if (!audio->is_started) {
        return;
    }
    
    tsk_list_lock(audio->mic_mix_aud.mixaud_free_lst);
    free_item = tsk_list_pop_first_item (audio->mic_mix_aud.mixaud_free_lst);
    tsk_list_unlock(audio->mic_mix_aud.mixaud_free_lst);
    
#ifdef ARMV7
    if (NULL == free_item)
    {
       tsk_list_lock(audio->mic_mix_aud.mixaud_filled_lst);
       tsk_size_t item_count = tsk_list_count (audio->mic_mix_aud.mixaud_filled_lst, tsk_null, tsk_null);
       tsk_list_unlock(audio->mic_mix_aud.mixaud_filled_lst);
       TSK_DEBUG_INFO("No free item available, MAX_BUF_FRAME_NUM:%d, item_count:%d", MAX_MIXAUD_ITEM_NUM, item_count);
       if (item_count < MAX_BKAUD_ITEM_NUM)
       {
           free_item = tsk_list_item_create();
           if (free_item)
           {
              free_item->data = tsk_object_new (tdav_session_audio_frame_buffer_def_t, MAX_FRAME_SIZE);
           }
       }
    }
#endif

    if (free_item)
    {
        if (free_item->data)
        {
            tdav_session_audio_frame_buffer_t* item = (tdav_session_audio_frame_buffer_t*)free_item->data;
            tdav_session_audio_frame_buffer_t* frame_buf = (tdav_session_audio_frame_buffer_t*)frame;
            memcpy(item->pcm_data, frame_buf->pcm_data, frame_buf->pcm_size);
            frame_buf->channel_num = (frame_buf->channel_num == 0) ? 1 : frame_buf->channel_num;
            item->bytes_per_sample = frame_buf->bytes_per_sample;
            item->channel_num      = frame_buf->channel_num;
            item->sample_rate      = frame_buf->sample_rate;
            item->pcm_size         = frame_buf->pcm_size;
            
            if ((audio->mic_mix_aud.mixaud_rate != frame_buf->sample_rate) || (audio->mic_mix_aud.mixaud_resampler_1 == NULL)) {
                tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_1);
#ifdef USE_WEBRTC_RESAMPLE
                if (audio->mic_mix_aud.mixaud_resampler_1 == NULL)
                {
                    audio->mic_mix_aud.mixaud_resampler_1 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
                }
                youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_1);
                resampler->InitializeIfNeeded(frame_buf->sample_rate, audio->encoder.codec->in.rate, frame_buf->channel_num);
#else
                speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_1);
                audio->mic_mix_aud.mixaud_resampler_1 = speex_resampler_init(frame_buf->channel_num, frame_buf->sample_rate, audio->encoder.codec->in.rate, 3, NULL);
#endif
                tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_1);
            }
            
            if ((audio->mic_mix_aud.mixaud_rate != frame_buf->sample_rate) || (audio->mic_mix_aud.mixaud_resampler_2 == NULL)) {
                tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_2);
#ifdef USE_WEBRTC_RESAMPLE
                if (audio->mic_mix_aud.mixaud_resampler_2 == NULL)
                {
                    audio->mic_mix_aud.mixaud_resampler_2 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
                }
                youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_2);
                resampler->InitializeIfNeeded(frame_buf->sample_rate, tmedia_defaults_get_playback_sample_rate(), frame_buf->channel_num);
#else
                speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_2);
                audio->mic_mix_aud.mixaud_resampler_2 = speex_resampler_init(frame_buf->channel_num, frame_buf->sample_rate, tmedia_defaults_get_playback_sample_rate(), 3, NULL);
#endif
                tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_2);
            }
            audio->mic_mix_aud.mixaud_rate = frame_buf->sample_rate;
        }
        else
        {
            TSK_DEBUG_INFO("*** moon free_item->data  is null ***  %lld",tsk_gettimeofday_ms());
        }
        
        tsk_list_lock(audio->mic_mix_aud.mixaud_filled_lst);
        tsk_list_push_item(audio->mic_mix_aud.mixaud_filled_lst, &free_item, tsk_true);
        tsk_list_unlock(audio->mic_mix_aud.mixaud_filled_lst);
        
        //if timeout happen ,only really put frame buf into list ,we add timeout_count
        if( bTimeout )
        {
            audio->mic_mix_aud.mixau_timeout_count++;
        }
    }
    else
    {
        //printf("***  free_item  is null **** %lld\n",tsk_gettimeofday_ms());
	    //TSK_DEBUG_INFO("***  moon free_item  is null ***");
    }
}

static int tdav_session_audio_resample( uint32_t& inSample, uint32_t& outSample,
                                       void* resample,  tsk_mutex_handle_t*  resample_mutex, void* fromBuff, void* toBuff, tsk_size_t outBufSize )
{
    if( toBuff == nullptr )
    {
        return 0;
    }
    
    if ( resample && fromBuff != nullptr ) {
        tsk_mutex_lock( resample_mutex );
#ifdef USE_WEBRTC_RESAMPLE
        youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>( resample );
        int src_nb_samples_per_process = ((resampler->GetSrcSampleRateHz() * 10) / 1000);
        int dst_nb_samples_per_process = ((resampler->GetDstSampleRateHz() * 10) / 1000);
        for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
            resampler->Resample((const int16_t*)fromBuff + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)toBuff + i * dst_nb_samples_per_process, 0);
        }
#else
        speex_resampler_process_int( resampler, 0, (const spx_int16_t*)fromBuff, &inSample, (spx_int16_t*)toBuff, &outSample);
#endif
        tsk_mutex_unlock( resample_mutex );
        return 1;
    } else {
        memset( toBuff, 0, outBufSize);
        return 0;
    }
}

static int tadv_session_audio_get_mixaud_buffer( tdav_session_audio_t *audio, int mixSize, long* timeStamp )
{
    static uint32_t u4MixaudGetCnt = 0;
    uint32_t readSize   = 0;
    uint32_t remainSize = 0;
    uint32_t spkSize    = 0;
    uint32_t preSampleRate = 0;
    //非0表示：外部(CYouMeVoiceEngine::mixAudioTrack)提供的了音频数据，需要混合到麦克风的音轨里面
    int iMicMixAudExist = 0;
    uint32_t newReadSize = 0;
    tsk_list_item_t *first_item = NULL;
    tdav_session_audio_frame_buffer_t* frame_buf = NULL;
    
    first_item = audio->mic_mix_aud.mixaud_lstitem_backup;
    if (!first_item) {
        tsk_list_lock(audio->mic_mix_aud.mixaud_filled_lst);
        first_item = tsk_list_pop_first_item(audio->mic_mix_aud.mixaud_filled_lst); // Pop item condition 1, First time pop
        tsk_list_unlock(audio->mic_mix_aud.mixaud_filled_lst);
        if (first_item) {
            frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
        } else {
            if (++u4MixaudGetCnt % 400 == 0) {
                TSK_DEBUG_INFO("Mic mix audio is NOT enough: level 1");
            }
            memset(audio->mic_mix_aud.mixaud_tempbuf1, 0, readSize);
            memset(audio->mic_mix_aud.mixaud_delayproc->pDelayProcBuf, 0, MAX_DELAY_MS * (audio->encoder.codec->in.rate / 1000) * sizeof(int16_t));
        }
    }
    else{
        frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
    }

    if (first_item && frame_buf && frame_buf->pcm_data) {
        // Re-calculate the read size
        readSize = mixSize * frame_buf->sample_rate / audio->encoder.codec->in.rate;
        preSampleRate = frame_buf->sample_rate;
        if (frame_buf->pcm_size >= readSize) {
            // Means the remain size is still enough
            memcpy((char*)audio->mic_mix_aud.mixaud_tempbuf1, (char*)frame_buf->pcm_data + audio->mic_mix_aud.mixaud_filled_position, readSize);
            frame_buf->pcm_size -= readSize;
            audio->mic_mix_aud.mixaud_filled_position += readSize;
            iMicMixAudExist = 1;
            *timeStamp = frame_buf->timeStamp;
        } else {
            // 外部输入检测buffer数据是否足够，不够则等下一tick处理
            if (tmedia_defaults_get_external_input_mode()) {
                tsk_list_item_t *item = NULL;
                int bfferSize = frame_buf->pcm_size;    // first_item buffer size 
                int bufferEnough = 0;
                tsk_list_foreach (item, audio->mic_mix_aud.mixaud_filled_lst) {
                    bfferSize += ((tdav_session_audio_frame_buffer_t*)item->data)->pcm_size;
                    if (bfferSize >= readSize) {
                        bufferEnough = 1;
                        break;
                    }
                }

                if (!bufferEnough) {
                    // buffer is not enough, wait for next tick to process
                    audio->mic_mix_aud.mixaud_lstitem_backup = first_item;
                    return 0;
                }
            }

            memcpy((char*)audio->mic_mix_aud.mixaud_tempbuf1, (char*)frame_buf->pcm_data + audio->mic_mix_aud.mixaud_filled_position, frame_buf->pcm_size);
            audio->mic_mix_aud.mixaud_filled_position = 0;

            // Means the remain size is NOT enough
            int writtenSize = frame_buf->pcm_size;
            remainSize = readSize - frame_buf->pcm_size;

            tsk_list_lock(audio->mic_mix_aud.mixaud_free_lst);
            tsk_list_push_item(audio->mic_mix_aud.mixaud_free_lst, &first_item, tsk_true);
            tsk_list_unlock(audio->mic_mix_aud.mixaud_free_lst);
            if (!tmedia_defaults_get_external_input_mode()) {
                // External mode should't block/pass external input
                tsk_semaphore_increment(audio->mic_mix_aud.mixaud_sema_lst);
            }
            
            while (remainSize > 0) {
                tsk_list_lock(audio->mic_mix_aud.mixaud_filled_lst);
                first_item = tsk_list_pop_first_item(audio->mic_mix_aud.mixaud_filled_lst); // Pop item condition 2, buffer wrapping
                tsk_list_unlock(audio->mic_mix_aud.mixaud_filled_lst);
                
                if (first_item) {
                    frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
                    if (preSampleRate == frame_buf->sample_rate) {
                        // newReadSize means the read size in the secode packet
                        if (frame_buf->pcm_size <= remainSize) {
                            // Normally this case is impossible, but here it is the error handler
                            // TSK_DEBUG_WARN("Mic mix audio size in one packet is too small:%d %d %d",remainSize,audio->mic_mix_aud.mixaud_filled_position,frame_buf->pcm_size);
                            //TMEDIA_I_AM_ACTIVE(200,"Mic mix audio size in one packet is too small:%d %d %d",remainSize, audio->mic_mix_aud.mixaud_filled_position, frame_buf->pcm_size);
                            memcpy((char*)audio->mic_mix_aud.mixaud_tempbuf1 + writtenSize, (char*)frame_buf->pcm_data + audio->mic_mix_aud.mixaud_filled_position, frame_buf->pcm_size);
                            // memset((char*)audio->mic_mix_aud.mixaud_tempbuf1 + writtenSize + frame_buf->pcm_size, 0,  (newReadSize - frame_buf->pcm_size));
                            audio->mic_mix_aud.mixaud_filled_position = 0;
                            remainSize -= frame_buf->pcm_size;
                            writtenSize += frame_buf->pcm_size;
                            frame_buf->pcm_size = 0;
                        } else {
                            memcpy((char*)audio->mic_mix_aud.mixaud_tempbuf1 + writtenSize, (char*)frame_buf->pcm_data + audio->mic_mix_aud.mixaud_filled_position, remainSize);
                            audio->mic_mix_aud.mixaud_filled_position += remainSize;
                            frame_buf->pcm_size -= remainSize;
                            writtenSize = 0;
                            remainSize = 0;
                        }

                        if (0 == frame_buf->pcm_size) {
                            tsk_list_lock(audio->mic_mix_aud.mixaud_free_lst);
                            tsk_list_push_item(audio->mic_mix_aud.mixaud_free_lst, &first_item, tsk_true);
                            tsk_list_unlock(audio->mic_mix_aud.mixaud_free_lst);
                            first_item = NULL;
                        }

                        if (0 == remainSize) {
                            iMicMixAudExist = 1;
                            *timeStamp = frame_buf->timeStamp;
                            break;
                        }
                    }
                }
                else {
                    if (++u4MixaudGetCnt % 400 == 0) {
                        TSK_DEBUG_INFO("Mic mix audio is NOT enough: level 2");
                    }
                    // memset(audio->mic_mix_aud.mixaud_tempbuf1, 0, readSize);
                    iMicMixAudExist = 0;
                    break;
                }
            }
        }
    }
    
    audio->mic_mix_aud.mixaud_lstitem_backup = first_item;
    
    return iMicMixAudExist;
}

void tdav_session_audio_mic_to_spk( tdav_session_audio_t* audio , int spkSize,  bkaud_source_t type )
{
    static uint32_t u4MixaudGetCnt = 0;
    tdav_session_av_t *base   = (tdav_session_av_t *)audio;
    trtp_rtp_header_t *header = trtp_rtp_header_create_null();
    
    tdav_session_audio_frame_buffer_t *mic_to_spk_item = (tdav_session_audio_frame_buffer_t*)tsk_malloc(sizeof(tdav_session_audio_frame_buffer_t));
    mic_to_spk_item->bytes_per_sample = sizeof(int16_t);
    mic_to_spk_item->channel_num = 1;
    mic_to_spk_item->is_float = 0;
    if( type == BKAUD_SOURCE_MICBYPASS )
    {
        mic_to_spk_item->pcm_data = audio->mic_mix_aud.mixaud_spkoutbuf;
    }
    else {
        mic_to_spk_item->pcm_data = audio->mic_mix_aud.mixaud_pcm_cb_buff;
    }
    
    mic_to_spk_item->pcm_size = spkSize;
    mic_to_spk_item->sample_rate = tmedia_defaults_get_playback_sample_rate ();
    if (++u4MixaudGetCnt % 600 == 0) {
        TSK_DEBUG_INFO("Put mic mix audio item: ChannelNum=%d, Samplerate=%d, PcmSize=%d", mic_to_spk_item->channel_num, mic_to_spk_item->sample_rate, mic_to_spk_item->pcm_size);
    }
    
    header->bkaud_source = type;
    tmedia_consumer_consume (base->consumer, mic_to_spk_item, sizeof(mic_to_spk_item), (tsk_object_t *)header);
    tsk_object_delete(header);
    TSK_SAFE_FREE(mic_to_spk_item);
}

static int tadv_session_audio_get_mixaud(tdav_session_audio_t *audio, void* destBuf, tsk_size_t mixSize, long* timeStamp)
{
    uint32_t readSize   = 0;
    //uint32_t remainSize = 0;
    uint32_t spkSize    = 0;
    uint32_t inSample, outSample;
    //tsk_list_item_t *first_item = NULL;
    //tdav_session_audio_frame_buffer_t* frame_buf = NULL;
    static uint32_t u4MixaudGetCnt = 0;
    int iMicMixAudExist = 0;
    tsk_boolean_t bNeedRecordPcmCallback = tsk_false;   //录音数据是否要给出pcm回调，如果要的话，就得传给speaker
    int cnt = 0;
    //uint32_t preSampleRate = 0;
    //uint32_t newReadSize = 0;
    
    tdav_session_av_t *base = (tdav_session_av_t *)audio;

    if ((!audio) || (!destBuf) || (!mixSize)) {
        return -1;
    }
    
    if (!audio->mic_mix_aud.mixaud_filled_lst || !audio->mic_mix_aud.mixaud_free_lst || !audio->mic_mix_aud.mixaud_sema_lst) {
        TSK_DEBUG_ERROR("Mic mix audio related items ISNT be initialized!!");
        return -1;
    }
    
    if (!audio->mic_mix_aud.mixaud_rate) {
        if (++u4MixaudGetCnt % 400 == 0) {
            TSK_DEBUG_INFO("Mic mix audio sample rate = 0!");
        }
        return -1;
    }
    
    //检查是否需要把录音和背景音乐都混到远端音频一起处理,目前需要的场景为：录屏/Pcm回调/会议纪要
    //if( audio->isSaveScreen || audio->isTranscriberOn || ( audio->isPcmCallback && (audio->isRecordCallbackOn || audio->isMixCallbackOn ) ) )
    if( audio->isSaveScreen || audio->isTranscriberOn ||  audio->isMixCallbackOn )
    {
        bNeedRecordPcmCallback = tsk_true;
    }
    
    //prepare buffer for background music
    readSize = mixSize * audio->mic_mix_aud.mixaud_rate / audio->encoder.codec->in.rate;
    spkSize  = mixSize * tmedia_defaults_get_playback_sample_rate () / audio->encoder.codec->in.rate;
    if (readSize > MAX_TEMP_BUF_BYTESZ) {
        audio->mic_mix_aud.mixaud_tempbuf1 = tsk_realloc(audio->mic_mix_aud.mixaud_tempbuf1, readSize);
    }
    if (mixSize > MAX_TEMP_BUF_BYTESZ) {
        audio->mic_mix_aud.mixaud_tempbuf2 = tsk_realloc(audio->mic_mix_aud.mixaud_tempbuf2, mixSize);
    }
    if (spkSize > MAX_TEMP_BUF_BYTESZ) {
        audio->mic_mix_aud.mixaud_tempbuf3 = tsk_realloc(audio->mic_mix_aud.mixaud_tempbuf3, spkSize);
        audio->mic_mix_aud.mixaud_tempbuf4 = tsk_realloc(audio->mic_mix_aud.mixaud_tempbuf4, spkSize);
    }
    
    //获取背景音乐mix
    iMicMixAudExist = tadv_session_audio_get_mixaud_buffer( audio, mixSize, timeStamp );
    if (!iMicMixAudExist) {
        return iMicMixAudExist;
    }
    // audio->mic_mix_aud.mixaud_tempbuf1 : Pure music data buffer(48K/44.1K etc) base on the samplerate of the music
    // audio->mic_mix_aud.mixaud_tempbuf2 : Music date buffer(48K/44.1K etc) resample to 16K for RTP send
    // audio->mic_mix_aud.mixaud_tempbuf3 : Music data buffer(48K/44.1K etc) resample to speaker output samplerate(48K/44.1K/16K etc)
    // destBuf                            : Pure voice data buffer(16K)
    // audio->mic_mix_aud.mixaud_tempbuf4 : Pure voice data buffer(16K) resample to speaker output samplerate(48K/44.1K/16K etc)

    // For RTP path
    // Background music transfer to match encoder input sample rate
    inSample  = readSize / sizeof(int16_t);
    outSample = mixSize / sizeof(int16_t);
    if( iMicMixAudExist )
    {
        tdav_session_audio_resample( inSample, outSample,  audio->mic_mix_aud.mixaud_resampler_1,
                                    audio->mic_mix_aud.mixaud_resampler_mutex_1,
                                    audio->mic_mix_aud.mixaud_tempbuf1, audio->mic_mix_aud.mixaud_tempbuf2, mixSize );
    }
    
    // For bypass speaker path
    // Background music transfer to match output sample rate
    inSample  = readSize / sizeof(int16_t);
    outSample = spkSize / sizeof(int16_t);
    if( (iMicMixAudExist) && (tmedia_defaults_get_external_input_mode() || audio->bgmBypassToSpk || bNeedRecordPcmCallback) )
    {
        tdav_session_audio_resample( inSample, outSample,  audio->mic_mix_aud.mixaud_resampler_2,
                                    audio->mic_mix_aud.mixaud_resampler_mutex_2,
                                    audio->mic_mix_aud.mixaud_tempbuf1, audio->mic_mix_aud.mixaud_tempbuf3, spkSize );
    }
    
    // Pure voice
    // For bypass speaker path, pure voice transfer to match output sample rate
    if (audio->mic_mix_aud.mixaud_resampler_3 == NULL) {
        tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_3);
#ifdef USE_WEBRTC_RESAMPLE
        audio->mic_mix_aud.mixaud_resampler_3 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
        youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_3);
        resampler->InitializeIfNeeded(audio->encoder.codec->in.rate, tmedia_defaults_get_playback_sample_rate (), 1);
#else
        speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_3);
        audio->mic_mix_aud.mixaud_resampler_3 = speex_resampler_init(1, audio->encoder.codec->in.rate, tmedia_defaults_get_playback_sample_rate (), 3, NULL);
#endif
        tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_3);
    } else {
        inSample  = mixSize / sizeof(int16_t);
        outSample = spkSize / sizeof(int16_t);
        if (( (audio->micBypassToSpk) && (audio->isHeadsetPlugin) ) || bNeedRecordPcmCallback ) {
            int ret = tdav_session_audio_resample( inSample, outSample,  audio->mic_mix_aud.mixaud_resampler_3,
                                                  audio->mic_mix_aud.mixaud_resampler_mutex_3,
                                                  destBuf, audio->mic_mix_aud.mixaud_tempbuf4, spkSize );
            
            if( ret != 0 ) //TODO: pcm callback 、屏幕录制、语音识别不能不降低音量
            {
                // Special processing, for monitor mode, -12dB gain apply to voice track track to avoid howling in some smartphone.
                while (cnt < outSample) {
                    *((int16_t *)audio->mic_mix_aud.mixaud_tempbuf4 + cnt) >>= 2;
                    cnt++;
                }
            }
        } else {
            memset(audio->mic_mix_aud.mixaud_tempbuf4, 0, spkSize);
        }
    }
    
    if (spkSize > MAX_TEMP_BUF_BYTESZ) {
        audio->mic_mix_aud.mixaud_spkoutbuf = tsk_realloc(audio->mic_mix_aud.mixaud_spkoutbuf, spkSize);
        audio->mic_mix_aud.mixaud_pcm_cb_buff = tsk_realloc(audio->mic_mix_aud.mixaud_pcm_cb_buff, spkSize);
    }
    // Mix audio here
    if (iMicMixAudExist) {
        // Bypass speaker path

        if (audio->micBypassToSpk && audio->bgmBypassToSpk && audio->isHeadsetPlugin ) {
            tdav_codec_mixer_normal((int16_t *)audio->mic_mix_aud.mixaud_tempbuf3, (int16_t *)audio->mic_mix_aud.mixaud_tempbuf4, (int16_t *)audio->mic_mix_aud.mixaud_spkoutbuf, spkSize / sizeof(int16_t));
        } else if (audio->micBypassToSpk && audio->isHeadsetPlugin && audio->mic_mix_aud.mixaud_tempbuf4) {
            memcpy(audio->mic_mix_aud.mixaud_spkoutbuf, audio->mic_mix_aud.mixaud_tempbuf4, spkSize);
        } else if (audio->bgmBypassToSpk && audio->isHeadsetPlugin && audio->mic_mix_aud.mixaud_tempbuf3) {
            memcpy(audio->mic_mix_aud.mixaud_spkoutbuf, audio->mic_mix_aud.mixaud_tempbuf3, spkSize);
        } else{
            memset(audio->mic_mix_aud.mixaud_spkoutbuf, 0, spkSize);
        }
        
        //pcm callback总是同时需要背景音乐和mic录音的
        if( bNeedRecordPcmCallback )
        {
            tdav_codec_mixer_normal((int16_t *)audio->mic_mix_aud.mixaud_tempbuf3, (int16_t *)audio->mic_mix_aud.mixaud_tempbuf4, (int16_t *)audio->mic_mix_aud.mixaud_pcm_cb_buff, spkSize / sizeof(int16_t));
        }
        
        // Rtp path
        // 七牛需求：这里的mix audio被用来传输语音数据，所以不能有delay
        tdav_codec_mixer_normal((int16_t *)destBuf, (int16_t *)audio->mic_mix_aud.mixaud_tempbuf2, (int16_t *)destBuf, mixSize / sizeof(int16_t));
    } else {

        // No need to think about the RTP path(delay), since there is no MIC mix audio
        // Bypass speaker path
        if ( audio->micBypassToSpk && audio->isHeadsetPlugin) {
            memcpy(audio->mic_mix_aud.mixaud_spkoutbuf, audio->mic_mix_aud.mixaud_tempbuf4, spkSize);
        }else {
            memset(audio->mic_mix_aud.mixaud_spkoutbuf, 0, spkSize);
        }
        
        if( bNeedRecordPcmCallback )
        {
            memcpy(audio->mic_mix_aud.mixaud_pcm_cb_buff, audio->mic_mix_aud.mixaud_tempbuf4, spkSize);
        }
        else{
            memset(audio->mic_mix_aud.mixaud_pcm_cb_buff, 0, spkSize);
        }
    }
    
    // 外部输入模式下，只有consumer启动，bypass数据才有效
    if (base->consumer && base->consumer->is_started && ((iMicMixAudExist != 0) 
        || tmedia_defaults_get_external_input_mode() 
        || ((audio->micBypassToSpk || audio->bgmBypassToSpk) && audio->isHeadsetPlugin))) {
        tdav_session_audio_mic_to_spk( audio, spkSize, BKAUD_SOURCE_MICBYPASS );
    }
    
    if( bNeedRecordPcmCallback )
    {
        tdav_session_audio_mic_to_spk( audio, spkSize, BKAUD_SOURCE_PCMCALLBACK );
    }

    return iMicMixAudExist;
}



// Return the exact size of the encoded rtp header extension.
static tsk_size_t tdav_session_audio_encode_rtp_header_ext(tdav_session_audio_t *audio, void* payload, tsk_size_t payload_size, tsk_bool_t silence_or_noise)
{
    tsk_size_t out_size = 0;
    BandwidthCtrlMsg_t bc_to_send = { 0 };
    RtpHeaderExt_t rtpHeaderExt = { 0 };
    tsk_bool_t bIsExistExtension = tsk_false;
    uint64_t curTimeMs = tsk_gettimeofday_ms();
    
    if (!audio || !audio->encoder.codec) {
        return 0;
    }

    // Init the non-zero and non-null fields
    rtpHeaderExt.fecType = RTP_HEADER_EXT_FEC_TYPE_NONE;
    
    // Check the validity before locking. This can reduce the times of locking.
    // Even if the validity changes just after checking, the worst case is missing a valid feedback data.
    // Since the feedback data is just something about statistics, so it doen't really matter.
    bc_to_send.isValid = 0;
    if (audio->bc.to_send.isValid) {
        tsk_safeobj_lock(&(audio->bc));
        bc_to_send = audio->bc.to_send;
        audio->bc.to_send.isValid = 0;
        tsk_safeobj_unlock(&(audio->bc));
    }

    if (bc_to_send.isValid) {
        out_size = tdav_codec_bandwidth_ctrl_encode(&bc_to_send, &audio->encoder.bwCtrlBuffer, &audio->encoder.bwCtrlBufferSize);
        if (out_size > 0) {
            rtpHeaderExt.pBandwidthCtrlData = (uint8_t*)audio->encoder.bwCtrlBuffer;
            rtpHeaderExt.bandwidthCtrlSize = out_size;
        }
        if (out_size > 0) {
            bIsExistExtension = tsk_true;
        }
    }
    
    if ((audio->vad.callback) && (audio->vad.len_in_frame_num > 0) && (audio->vad.silence_percent > 0)) {
        tsk_bool_t cur_frame_silence = tsk_false;
        audio->vad.total_frame_count++;
        if (silence_or_noise) {
            audio->vad.silence_frame_count++;
        }
        if (audio->vad.total_frame_count >= audio->vad.len_in_frame_num) {
            // for debug
            if (audio->vad.group_count < 100) {
                audio->vad.group_count++;
                if ((audio->vad.group_count % 5) == 0) {
                    TSK_DEBUG_INFO("VAD total_frame_count:%d, silence_frame_count:%d", audio->vad.total_frame_count, audio->vad.silence_frame_count);
                }
            }
    
            if (audio->vad.silence_frame_count > (audio->vad.total_frame_count * audio->vad.silence_percent / 100)) {
                cur_frame_silence = tsk_true;
            } else {
                cur_frame_silence = tsk_false;
            }
            //TSK_DEBUG_INFO("^^^^^^^ silence:%d, total_frame:%d, silence_frame:%d", cur_frame_silence, audio->vad.total_frame_count, audio->vad.silence_frame_count);
            if (audio->vad.pre_frame_silence != cur_frame_silence) {
                audio->vad.pre_frame_silence = cur_frame_silence;
                rtpHeaderExt.hasVoiceSilence = 1;
                rtpHeaderExt.voiceSilence = cur_frame_silence ? 1 : 0;
                bIsExistExtension = tsk_true;
                
                // for debug
                if (audio->vad.status_sent_count < 3) {
                    TSK_DEBUG_INFO("VAD sending silence status:%d", audio->vad.pre_frame_silence);
                    audio->vad.status_sent_count++;
                }
            }
            audio->vad.total_frame_count = 0;
            audio->vad.silence_frame_count = 0;
        }
    }
    
    // Send my local timestamp with the RTP packet. The receiver will bounce it back, and I will
    // know the time cost of the round trip.
    if (audio->stat.may_send_timestamp
        && (audio->stat.send_timestamp_period_ms > 0)
        && ((curTimeMs - audio->stat.last_send_timestamp_time_ms) >= audio->stat.send_timestamp_period_ms) ) {
        
        rtpHeaderExt.hasTimestampOriginal = 1;
        rtpHeaderExt.timestampOriginal = (uint32_t)(tsk_gettimeofday_ms() - audio->stat.time_base_ms);
        bIsExistExtension = tsk_true;
        audio->stat.last_send_timestamp_time_ms = curTimeMs;
        audio->stat.may_send_timestamp = tsk_false;
        //TSK_DEBUG_INFO(">>>> sending timestamp:%u", rtpHeaderExt.timestampOriginal);
    }
    
    // Bounce the timestamp back to the sender.
    // Check first if there's data to be bounced without locking, so that we don't need to lock for
    // every packet even there's no data to be bounced.
    if (audio->stat.timestamp_num > 0) {
        tsk_safeobj_lock(&(audio->stat));
        // check again after locking
        if (audio->stat.timestamp_num > 0) {
            int32_t offset = 0;
            int32_t i;
            for (i = 0; i < audio->stat.timestamp_num; i++) {
                 // duration between receiving and sending the timestamp
                uint32_t timestampDeltaMs = curTimeMs - audio->stat.timestamps[i].timeWhenRecvMs;
                //TSK_DEBUG_INFO(">>>> bounce session:%d, timestamp:%u, timestampDeltaMs:%u", audio->stat.timestamps[i].sessionId, audio->stat.timestamps[i].timestampMs, timestampDeltaMs);
                
                // Encode the bounced timestamps
                *(int32_t*)&audio->stat.timestampBouncedBuf[offset] = htonl(audio->stat.timestamps[i].sessionId);
                offset += sizeof(int32_t);
                *(uint32_t*)&audio->stat.timestampBouncedBuf[offset] = htonl(audio->stat.timestamps[i].timestampMs + timestampDeltaMs);
                offset += sizeof(uint32_t);
                bIsExistExtension = tsk_true;
            }
            rtpHeaderExt.pTimestampBouncedData = audio->stat.timestampBouncedBuf;
            rtpHeaderExt.timestampBouncedSize = audio->stat.timestamp_num * BOUNCED_TIMESTAMP_INFO_SIZE;
            audio->stat.timestamp_num = 0;
            bIsExistExtension = tsk_true;
        }
        tsk_safeobj_unlock(&(audio->stat));
    }
    // Finally, encode the rtp header extension if necessay.
    if (bIsExistExtension) {
        out_size = tdav_codec_rtp_extension_encode(audio, &rtpHeaderExt, &audio->encoder.rtpExtBuffer, &audio->encoder.rtpExtBufferSize);
    }

    return out_size;
}

// Decode the header extension, which may contains the bandwidth control data, FEC data, VAD result, etc.
static int tdav_session_audio_decode_rtp_header_ext(tdav_session_audio_t *audio, const struct trtp_rtp_packet_s *packet)
{
    int ret = -1;
    RtpHeaderExt_t rtpHeaderExt;
    int i;

    if (!audio || !packet) {
        return ret;
    }
    
    // Since we received a packet, we may send a timestamp(for calculating network delay) in the next pakcet.
    audio->stat.may_send_timestamp = tsk_true;

    packet->header->bc_received.isValid = 0;
    packet->header->bc_to_send.isValid = 0;
    if (packet->header->extension && (packet->extension.data || packet->extension.data_const) && (packet->extension.size > 0)) {
        uint8_t* ext_data = (uint8_t*)(packet->extension.data_const ? packet->extension.data_const : packet->extension.data);
        ret = tdav_codec_rtp_extension_decode(audio, ext_data, packet->extension.size, &rtpHeaderExt);
    }

    // If no header extension, or failed to decode the header extension, just return.
    if (0 != ret) {
        return ret;
    }
    
    if (rtpHeaderExt.pBandwidthCtrlData && (rtpHeaderExt.bandwidthCtrlSize > 0)) {
        ret = tdav_codec_bandwidth_ctrl_decode(rtpHeaderExt.pBandwidthCtrlData, rtpHeaderExt.bandwidthCtrlSize, &packet->header->bc_received);
        packet->header->bc_received.isValid = (0 == ret);
    }
            
    if ((RTP_HEADER_EXT_FEC_TYPE_NONE != rtpHeaderExt.fecType) && rtpHeaderExt.pFecData && (rtpHeaderExt.fecSize > 0)) {
        // TODO: parse fec data
    }

    if (rtpHeaderExt.hasVoiceSilence) {
        if (audio->vad.callback) {
            audio->vad.callback(packet->header->session_id, rtpHeaderExt.voiceSilence);
            // for debug
            if (audio->vad.status_recv_count < 3) {
                TSK_DEBUG_INFO("VAD receive silence status:%d", rtpHeaderExt.voiceSilence);
                audio->vad.status_recv_count++;
            }
        }
    }
    
    // If there's a timestamp from the sender, save it and bounce it back with the next packet.
    if (rtpHeaderExt.hasTimestampOriginal) {
        //TSK_DEBUG_INFO("@@>>  got original sessionid:%d, timestamp:%u", packet->header->session_id, rtpHeaderExt.timestampOriginal);

        tsk_safeobj_lock(&(audio->stat));
        // Check if a timestamp from the same sessionID is waiting for being bounced back.
        // If so, just replace it with the new one.
        for (i = 0; i < audio->stat.timestamp_num; i++) {
            if (audio->stat.timestamps[i].sessionId == packet->header->session_id) {
                break;
            }
        }

        if (i < audio->stat.timestamp_num) {
            audio->stat.timestamps[i].timeWhenRecvMs = packet->header->receive_timestamp_ms;
            audio->stat.timestamps[i].timestampMs = rtpHeaderExt.timestampOriginal;
        } else if (audio->stat.timestamp_num < MAX_TIMESTAMP_NUM) {
            audio->stat.timestamps[audio->stat.timestamp_num].timeWhenRecvMs = packet->header->receive_timestamp_ms;
            audio->stat.timestamps[audio->stat.timestamp_num].sessionId = packet->header->session_id;
            audio->stat.timestamps[audio->stat.timestamp_num].timestampMs = rtpHeaderExt.timestampOriginal;
            audio->stat.timestamp_num++;
        }
        tsk_safeobj_unlock(&(audio->stat));
    }
    
    packet->header->networkDelayMs = 0;
    if ((rtpHeaderExt.timestampBouncedSize > 0) && rtpHeaderExt.pTimestampBouncedData) {
        uint32_t timestampBouncedNum = rtpHeaderExt.timestampBouncedSize / (sizeof(int32_t) + sizeof(uint32_t));
        int32_t  sessionIdBounced = 0;
        uint32_t timestampBounced = 0;
        uint32_t curRelTimeMs = (uint32_t)(tsk_gettimeofday_ms() - audio->stat.time_base_ms);
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
        }
    }
    
    return 0;
}



/* ============ Plugin interface ================= */

static int tdav_session_audio_set (tmedia_session_t *self, const tmedia_param_t *param)
{
    int ret = 0;
    tdav_session_audio_t *audio;
    tdav_session_av_t *base;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (tdav_session_av_set (TDAV_SESSION_AV (self), param) == tsk_true)
    {
        return 0;
    }

    audio = (tdav_session_audio_t *)self;
    base  = (tdav_session_av_t *)self;

    if (param->plugin_type == tmedia_ppt_consumer)
    {
        TSK_DEBUG_ERROR ("Not expected consumer_set(%s)", param->key);
    }
    else if (param->plugin_type == tmedia_ppt_producer)
    {
        TSK_DEBUG_ERROR ("Not expected producer_set(%s)", param->key);
    }
    else
    {
        if (param->value_type == tmedia_pvt_pobject)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_SPEAKER)) {
                tdav_session_audio_frame_buffer_t *frame_buf = (tdav_session_audio_frame_buffer_t *)param->value;
                if (frame_buf) {
                    if (audio->pcm_file.speaker) {
                        if (audio->pcm_file.speaker_size > audio->pcm_file.max_size) {
                            tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_MIX_SPEAKER);
                        }
                        if (audio->pcm_file.speaker) {
                            fwrite (frame_buf->pcm_data, 1, frame_buf->pcm_size, audio->pcm_file.speaker);
                            audio->pcm_file.speaker_size += frame_buf->pcm_size;
                        }
                    }
                    //TSK_DEBUG_INFO("addr:%d,%d, size:%d, samplerate:%d, ch:%d, float:%d",
                    //               frame_buf->pcm_data, *(int16_t*)(frame_buf->pcm_data), frame_buf->pcm_size, frame_buf->sample_rate, frame_buf->channel_num, frame_buf->is_float);
                    trtp_rtp_header_t *header = trtp_rtp_header_create_null();
                    header->bkaud_source = BKAUD_SOURCE_GAME;
                    tmedia_consumer_consume (base->consumer, frame_buf, sizeof(frame_buf), (tsk_object_t *)header);
                    tsk_object_delete(header);
                }
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_MICPHONE)) {
                tdav_session_audio_frame_buffer_t *frame_buf = (tdav_session_audio_frame_buffer_t *)param->value;
                if (frame_buf) {
                    // dump extern input audio to file
                    if (audio->pcm_file.mic) {
                        if (audio->pcm_file.mic_size > audio->pcm_file.max_size) {
                            tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_MIX_MIC);
                        }
                        if (audio->pcm_file.mic) {
                            fwrite (frame_buf->pcm_data, 1, frame_buf->pcm_size, audio->pcm_file.mic);
                            audio->pcm_file.mic_size += frame_buf->pcm_size;
                        }
                    }

                    // ===== Pre-processing start ===== //
                    int16_t i2SampleNumTotal = frame_buf->pcm_size / frame_buf->bytes_per_sample;
                    int16_t i2SampleNumPerCh = i2SampleNumTotal / frame_buf->channel_num;
                    int16_t *pi2Buf1 = (int16_t *)tsk_malloc(i2SampleNumTotal * sizeof(int16_t));
                    int16_t *pi2Buf2 = (int16_t *)tsk_malloc(i2SampleNumTotal * sizeof(int16_t));
                    // Float transfer
                    tdav_codec_float_to_int16 (frame_buf->pcm_data, pi2Buf1, &(frame_buf->bytes_per_sample), &(frame_buf->pcm_size), frame_buf->is_float);
                    
                    // 2 channel & interleaved, downmix process
                    tdav_codec_multichannel_interleaved_to_mono (pi2Buf1, pi2Buf2, &(frame_buf->channel_num), &(frame_buf->pcm_size), frame_buf->is_interleaved);
                    
                    // Volume here, use i2SampleNumPerCh samples to do fade if volume change(smooth gain)
                    int32_t i4CurVol  = (int32_t)((int64_t)(audio->mixAudioVol) * 0x7fffffff / 100);
                    tdav_codec_volume_smooth_gain (pi2Buf2, i2SampleNumPerCh, i2SampleNumPerCh, i4CurVol, &audio->mixLastVol);
                    memcpy(frame_buf->pcm_data, pi2Buf2, frame_buf->pcm_size);
                    
                    TSK_SAFE_FREE(pi2Buf1);
                    TSK_SAFE_FREE(pi2Buf2);
                    // ===== Pre-processing end ===== //
                    
                    if (frame_buf->channel_num == 1) { // Currently only support 1 channel processing
                        tdav_session_audio_put_mixaud(audio, frame_buf);
                    } else {
                        // Implement here
                        TSK_DEBUG_WARN("Currently ONLY support MONO background audio!");
                    }
                }
            }
            return 0;
        }
        else if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_AEC_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->aec_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                    TSK_DEBUG_INFO("set aec_enabled:%d", audio->denoise->aec_enabled);
                }
            }
            else if (tsk_striequals (param->key, "echo-tail"))
            {
                if (audio->denoise)
                {
                    return tmedia_denoise_set (audio->denoise, param);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_AGC_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->agc_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                    TSK_DEBUG_INFO("set agc_enabled:%d", audio->denoise->agc_enabled);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VAD_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->vad_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                    TSK_DEBUG_INFO("set vad_enabled:%d", audio->denoise->vad_enabled);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_NS_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->noise_supp_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                    TSK_DEBUG_INFO("set noise_supp_enabled:%d", audio->denoise->noise_supp_enabled);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_HEADSET_PLUGIN))
            {
                if (audio->denoise)
                {
                    audio->denoise->headset_plugin = (TSK_TO_INT32 ((uint8_t *)param->value) != 0 ? tsk_true : tsk_false);
                    TSK_DEBUG_INFO("set headset_plugin:%d", audio->denoise->headset_plugin);
                }
                audio->isHeadsetPlugin = (TSK_TO_INT32 ((uint8_t *)param->value) != 0 ? tsk_true : tsk_false);
                if (audio->isHeadsetPlugin)
                {
                    TSK_DEBUG_INFO("set headset_plugin:%d, will disable voice bypass to speaker path if this function is enabled!!", audio->isHeadsetPlugin);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SOUND_TOUCH_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->soundtouch_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0 ? tsk_true : tsk_false);
                    TSK_DEBUG_INFO("set soundtouch_enabled:%d", audio->denoise->soundtouch_enabled);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SOUND_TOUCH_TEMPO))
            {
                if (audio->denoise)
                {
                    float fTempo = ((float)TSK_TO_INT32 ((uint8_t *)param->value)) / 100.0f;

                    const float EPSINON = 0.00001;
                    if (fabsf (fTempo - audio->denoise->soundtouch_tempo) > EPSINON)
                    {
                        audio->denoise->soundtouch_tempo = fTempo;
                        audio->denoise->soundtouch_paraChanged = tsk_true;
                    }
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SOUND_TOUCH_RATE))
            {
                if (audio->denoise)
                {
                    float fRate = ((float)TSK_TO_INT32 ((uint8_t *)param->value)) / 100.0f;

                    const float EPSINON = 0.00001;
                    if (fabsf (fRate - audio->denoise->soundtouch_rate) > EPSINON)
                    {
                        audio->denoise->soundtouch_rate = fRate;
                        audio->denoise->soundtouch_paraChanged = tsk_true;
                    }
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SOUND_TOUCH_PITCH))
            {
                if (audio->denoise)
                {
                    float fPitch = ((float)TSK_TO_INT32 ((uint8_t *)param->value)) / 100.0f;

                    const float EPSINON = 0.00001;
                    if (fabsf (fPitch - audio->denoise->soundtouch_pitch) > EPSINON)
                    {
                        audio->denoise->soundtouch_pitch = fPitch;
                        audio->denoise->soundtouch_paraChanged = tsk_true;
                    }
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_ENABLED))
            {
                int32_t i4Paras  = TSK_TO_INT32 ((uint8_t *)param->value);
                uint8_t u1PathId = (i4Paras >> 8) & 0xff;
                uint8_t u1Enable = i4Paras & 0xff;
                if ((AVSessionBackgroundMusicMode_t)u1PathId == AVSESSION_BGM_TO_SPEAKER) {
                    tmedia_consumer_set_param (base->consumer, param);
                } else {
//                    audio->mic_mix_aud.mixaud_enable = u1Enable ? 1 : 0;
//                    if (audio->mic_mix_aud.mixaud_enable) {
//                        audio->mic_mix_aud.mixaud_rate = 0; // Make sure the resampler can be initialized
//                        audio->mic_mix_aud.mixaud_filled_position = 0;
//                        if (audio->mic_mix_aud.mixaud_sema_lst) {
//                            tsk_semaphore_destroy(&audio->mic_mix_aud.mixaud_sema_lst);
//                            audio->mic_mix_aud.mixaud_sema_lst = tsk_semaphore_create_2(MAX_BKAUD_ITEM_NUM);
//                        }
//                    } else {
//                        // Semaphore MUST +1 to avoid APPs wait release always
//                        tsk_semaphore_increment(audio->mic_mix_aud.mixaud_sema_lst);
//                    }
//                    tmedia_consumer_set_param (base->consumer, param);
                }
                TSK_DEBUG_INFO("Set mixAudioEnabled: (PathId: %d)(Enable: %d)", u1PathId, u1Enable);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME))
            {
                int32_t i4Paras  = TSK_TO_INT32 ((uint8_t *)param->value);
                uint8_t u1PathId = (i4Paras >> 8) & 0xff;
                uint8_t u1Volume = i4Paras & 0xff;
                if ((AVSessionBackgroundMusicMode_t)u1PathId == AVSESSION_BGM_TO_SPEAKER) {
                    tmedia_consumer_set_param (base->consumer, param);
                } else {
                    audio->mixAudioVol = u1Volume;
                }
                
                TSK_DEBUG_INFO("Set mixAudioVol:%d", audio->mixAudioVol);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MICROPHONE_MUTE))
            {
                audio->isMicMute = (tsk_bool_t)(TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("Set mic mute:%d", audio->isMicMute);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SPEAKER_MUTE))
            {
                audio->isSpeakerMute = (tsk_bool_t)(TSK_TO_INT32 ((uint8_t *)param->value));
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_BYPASS_TO_SPEAKER))
            {
                audio->micBypassToSpk = (tsk_bool_t)(*((int32_t *)param->value) != 0);
                TSK_DEBUG_INFO("Set mic bypass to speaker:%d", audio->micBypassToSpk);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_BGM_BYPASS_TO_SPEAKER))
            {
                audio->bgmBypassToSpk = (tsk_bool_t)(*((int32_t *)param->value) != 0);
                TSK_DEBUG_INFO("Set bgm bypass to speaker:%d", audio->bgmBypassToSpk);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_FLAG))
            {
                int flag = *((int32_t *)param->value);
                audio->isRemoteCallbackOn = (tsk_bool_t)( (flag & 0x1) != 0 );
                audio->isRecordCallbackOn = (tsk_bool_t)( (flag & 0x2) != 0 );
                audio->isMixCallbackOn = (tsk_bool_t)( (flag & 0x4) != 0 );
                TSK_DEBUG_INFO("Set pcmCallback flag:%d, remote:%d, record:%d, mix:%d", flag,
                               audio->isRemoteCallbackOn,audio->isRecordCallbackOn ,audio->isMixCallbackOn );
                tmedia_consumer_set_param (base->consumer, param);
            }            
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_CHANNEL))
            {
                int nPCMCallbackChannel = *((int32_t *)param->value);
                audio->nPCMCallbackChannel = nPCMCallbackChannel;
                TSK_DEBUG_INFO("Set Output Channel:%d", nPCMCallbackChannel);
                tmedia_consumer_set_param (base->consumer, param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_SAMPLE_RATE))
            {
                int nPCMCallbackSampleRate = *((int32_t *)param->value);
                audio->nPCMCallbackSampleRate = nPCMCallbackSampleRate;
                TSK_DEBUG_INFO("Set Output Sample Rate:%d", nPCMCallbackSampleRate);
                tmedia_consumer_set_param (base->consumer, param);
            }
            else if( tsk_striequals( param->key, TMEDIA_PARAM_KEY_TRANSCRIBER_ENABLED ))
            {
                audio->isTranscriberOn = (*((int32_t *)param->value)) != 0 ;
                TSK_DEBUG_INFO( "Set transcriber enable:%d", audio->isTranscriberOn );
                tmedia_consumer_set_param (base->consumer, param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_BGM_DELAY))
            {
                tsk_mutex_lock(audio->mic_mix_aud.mixaud_delayproc->pDelayMutex);
                audio->mic_mix_aud.mixaud_delayproc->i2SetDelay = (int16_t)(TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("Set mix audio delay:%d", audio->mic_mix_aud.mixaud_delayproc->i2SetDelay);
                tsk_mutex_unlock(audio->mic_mix_aud.mixaud_delayproc->pDelayMutex);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_REVERB_ENABLED))
            {
                if (audio->denoise)
                {
                    audio->denoise->reverb_filter_enabled_ap = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                    TSK_DEBUG_INFO("set reverb enabled:%d", audio->denoise->reverb_filter_enabled_ap);
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RECORDING_TIME_MS)) {
                if (base->rtp_manager) {
                    trtp_manager_set_recording_time_ms(base->rtp_manager, TSK_TO_UINT32 ((uint8_t *)param->value),
                                                       48000); // hard code to 48KHz, since the audio codec may not be available at this time
                                                       //(uint32_t)TMEDIA_CODEC_RTP_CLOCK_RATE_AUDIO_DECODING (audio->decoder.codec));
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PLAYING_TIME_MS)) {
                if (base->rtp_manager) {
                    trtp_manager_set_playing_time_ms(base->rtp_manager, TSK_TO_UINT32 ((uint8_t *)param->value),
                                                     48000); // hard code to 48KHz, since the audio codec may not be available at this time
                                                     //(uint32_t)TMEDIA_CODEC_RTP_CLOCK_RATE_AUDIO_DECODING (audio->decoder.codec));
                }
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MAX_MIC_LEVEL_CALLBACK))
            {
                audio->mic_level.max_level = TSK_TO_INT32 ((uint8_t *)param->value);
                TSK_DEBUG_INFO("set max mic level:%d", audio->mic_level.max_level);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MAX_FAREND_VOICE_LEVEL))
            {
                tmedia_consumer_set_param (base->consumer, param);
            }else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_INBAND_FEC_ENABLE))
            {
                tmedia_codec_set(TDAV_SESSION_AUDIO (self)->encoder.codec, param);
            }else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_SET_PACKET_LOSS_PERC))
            {
                tmedia_codec_set(TDAV_SESSION_AUDIO (self)->encoder.codec, param);
            }else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_SET_BITRATE))
            {
                tmedia_codec_set(TDAV_SESSION_AUDIO (self)->encoder.codec, param);
            }
            else if( tsk_striequals (param->key, TMEDIA_PARAM_KEY_SAVE_SCREEN) )
            {
                audio->isSaveScreen = (TSK_TO_INT32( (uint8_t *)param->value )) != 0 ;
                TSK_DEBUG_INFO("Set SaveScreen :%d", audio->isSaveScreen);
                tmedia_consumer_set_param (base->consumer, param);
            }
        } else if (param->value_type == tmedia_pvt_pvoid) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VAD_CALLBACK)) {
                audio->vad.callback = (VadCallback_t)param->value;
                TSK_DEBUG_INFO("set vadCallback:%p", audio->vad.callback);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK)) {
                if (tsk_null != param->value) {
                    audio->isPcmCallback = tsk_true;
                } else {
                    audio->isPcmCallback = tsk_false;
                }
                audio->pcmCallback = (PcmCallback_t)param->value;
                TSK_DEBUG_INFO("set pcmCallback flag:%d", audio->isPcmCallback);
                tmedia_consumer_set_param (base->consumer, param);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIC_LEVEL_CALLBACK)) {
                audio->mic_level.callback = (MicLevelCallback_t)param->value;
                TSK_DEBUG_INFO("set micLevelCallback:%p", audio->mic_level.callback);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_FAREND_VOICE_LEVEL_CALLBACK)) {
                tmedia_consumer_set_param (base->consumer, param);
            }
            
        }
    }

    return ret;
}

static int tdav_session_audio_get (tmedia_session_t *self, tmedia_param_t *param)
{
    tdav_session_audio_t *audio = nullptr;
    tdav_session_av_t* base = nullptr;

    if (!self || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    audio = (tdav_session_audio_t*)self;
    base = (tdav_session_av_t *)self;
    
    // try with the base class to see if this option is supported or not
    if (tdav_session_av_get (TDAV_SESSION_AV (self), param) == tsk_true)
    {
        return 0;
    }
    else
    {
        // the codec information is held by the session even if the user is authorized to request it
        // for the consumer/producer
        if (param->value_type == tmedia_pvt_pobject)
        {
            if (param->plugin_type == tmedia_ppt_consumer)
            {
                TSK_DEBUG_ERROR ("Not implemented");
                return -4;
            }
            else if (param->plugin_type == tmedia_ppt_producer)
            {
                if (tsk_striequals ("codec", param->key))
                {
                    const tmedia_codec_t *codec;
                    if (!(codec = TDAV_SESSION_AUDIO (self)->encoder.codec))
                    {
                        codec = tdav_session_av_get_best_neg_codec ((const tdav_session_av_t *)self); // up to the caller to release the object
                    }
                    *((tsk_object_t **)param->value) = tsk_object_ref (TSK_OBJECT (codec));
                    return 0;
                }
            }
            else if (param->plugin_type == tmedia_ppt_session)
            {
                if (tsk_striequals (param->key, "codec-encoder"))
                {
                    *((tsk_object_t **)param->value) = tsk_object_ref (TDAV_SESSION_AUDIO (self)->encoder.codec); // up to the caller to release the object
                    return 0;
                } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PACKET_STAT)) {
                    param->plugin_type = tmedia_ppt_jitterbuffer;
                    return tmedia_consumer_get_param (base->consumer, param);
                }
            }
        } 
        else if( param->value_type == tmedia_pvt_int32 )
        {
            if( tsk_striequals( param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_EFFECT_FREE_BUFF_COUNT ))
            {
                // tsk_list_lock( audio->mic_effect_mix_aud.mixaud_free_lst );
                // int free_buf_count = tsk_list_count_all( audio->mic_effect_mix_aud.mixaud_free_lst );
                // tsk_list_unlock( audio->mic_effect_mix_aud.mixaud_free_lst );
                // ((int32_t *)param->value)[0] = free_buf_count;
                // return tsk_true;
            }
            else if( tsk_striequals( param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_FREE_BUFF_COUNT ))
            {
                tsk_list_lock( audio->mic_mix_aud.mixaud_free_lst );
                int free_buf_count = tsk_list_count_all( audio->mic_mix_aud.mixaud_free_lst );
                tsk_list_unlock( audio->mic_mix_aud.mixaud_free_lst );
                ((int32_t *)param->value)[0] = free_buf_count;
                return tsk_true;
            }
        }
    }

    TSK_DEBUG_WARN ("This session doesn't support get(%s)", param->key);
    return -2;
}

static int tdav_session_audio_prepare (tmedia_session_t *self)
{
    tdav_session_av_t *base = (tdav_session_av_t *)(self);
    int ret;

    if ((ret = tdav_session_av_prepare (base)))
    {
        TSK_DEBUG_ERROR ("tdav_session_av_prepare(audio) failed");
        return ret;
    }

    if (base->rtp_manager)
    {
        ret = trtp_manager_set_rtp_callback (base->rtp_manager, tdav_session_audio_rtp_cb, base);
    }

    return ret;
}
#ifdef MAC_OS
#include <mach-o/dyld.h>
char *GetExeDir ()
{
    char buf[0];
    uint32_t size = 0;
    int res = _NSGetExecutablePath (buf, &size);

    char *path = (char *)malloc (size + 1);
    path[size] = 0;
    res = _NSGetExecutablePath (path, &size);

    char *p = strrchr (path, '/');
    *p = 0;
    return path;
}
#endif

static int tdav_session_audio_start (tmedia_session_t *self)
{
     TSK_DEBUG_INFO("Enter");
    int ret;
    tdav_session_audio_t *audio;
    const tmedia_codec_t *codec;
    tdav_session_av_t *base;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    audio = (tdav_session_audio_t *)self;
    base = (tdav_session_av_t *)self;

    if (audio->is_started)
    {
        TSK_DEBUG_INFO ("Audio session already started");
        return 0;
    }
	if (!(audio->rscode_list = tsk_list_create())) {
		TSK_DEBUG_ERROR("Failed to create rscode list");
		return -5;
	}
    if (!(codec = tdav_session_av_get_best_neg_codec (base)))
    {
        TSK_DEBUG_ERROR ("No codec matched");
        return -2;
    }

    TSK_OBJECT_SAFE_FREE (audio->encoder.codec);
    audio->encoder.codec = (tmedia_codec_t *)tsk_object_ref ((tsk_object_t *)codec);

    if ((ret = tdav_session_av_start (base, codec)))
    {
#ifdef MAC_OS
        //如果voip启动失败了，强制设置为hal模式，重启一次
        if( base->useHalMode == tsk_false )
        {
            TSK_DEBUG_ERROR ("tdav_session_av_start(audio Voip) failed, use HAL restart");
            tdav_session_av_stop(base);
            base->forceUseHalMode = tsk_true;
            
            ret = tdav_session_av_start( base, codec );
        }
#endif

        if( ret )
        {
            TSK_DEBUG_ERROR ("tdav_session_av_start(audio) failed");
            return ret;
        }
    } else {
#ifdef MAC_OS
    	// mac use audiounit(voip) aec
    	audio->denoise->aec_enabled = tsk_false;
#endif    	
    	TSK_DEBUG_INFO("tdav_session_av_start success");
    }

    if (audio->pcm_file.max_size > 0)
    {
        TSK_DEBUG_INFO("Start session audio mix dumping pcm, max_size:%u", audio->pcm_file.max_size);
        
        tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_MIX_MIC);
        tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_MIX_SPEAKER);
        tdav_session_audio_open_pcm_files(audio, TDAV_AUDIO_PCM_FILE_SEND_PCM );
    }
    
    if (base->rtp_manager)
    {
        /* Denoise (AEC, Noise Suppression, AGC)
        * tmedia_denoise_process_record() is called after resampling and before encoding which means
        * sampling rate is equal to codec's rate
        * tmedia_denoise_echo_playback() is called before playback which means sampling rate is
        * equal to consumer's rate
        */
        if (audio->denoise)
        {
            if( base->useHalMode == tsk_true )
            {
                audio->denoise->aec_enabled = tsk_true;
            }
            
            uint32_t record_frame_size_samples = TMEDIA_CODEC_PCM_FRAME_SIZE_AUDIO_ENCODING (audio->encoder.codec);
            uint32_t record_sampling_rate = TMEDIA_CODEC_RATE_ENCODING (audio->encoder.codec);
            uint32_t record_channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING (audio->encoder.codec);

            //uint32_t playback_frame_size_samples = (base->consumer && base->consumer->audio.ptime && base->consumer->audio.out.rate && base->consumer->audio.out.channels) ?
            //                                       ((base->consumer->audio.ptime * base->consumer->audio.out.rate) / 1000) * base->consumer->audio.out.channels :
            //                                       TMEDIA_CODEC_PCM_FRAME_SIZE_AUDIO_DECODING (audio->encoder.codec);
            //uint32_t playback_sampling_rate = (base->consumer && base->consumer->audio.out.rate) ? base->consumer->audio.out.rate : TMEDIA_CODEC_RATE_DECODING (audio->encoder.codec);
            uint32_t playback_frame_size_samples = TMEDIA_CODEC_PCM_FRAME_SIZE_AUDIO_DECODING (audio->encoder.codec);
            uint32_t playback_sampling_rate = TMEDIA_CODEC_RATE_DECODING (audio->encoder.codec);
            uint32_t playback_channels = (base->consumer && base->consumer->audio.out.channels) ? base->consumer->audio.out.channels : TMEDIA_CODEC_CHANNELS_AUDIO_DECODING (audio->encoder.codec);
            
            // Use the same parameters with playback frame
            uint32_t purevoice_frame_size_samples = playback_frame_size_samples;
            uint32_t purevoice_sampling_rate      = playback_sampling_rate;
            uint32_t purevoice_channels           = playback_channels;
            TSK_DEBUG_INFO ("Audio denoiser to be opened(record_frame_size_samples=%u, "
                            "record_sampling_rate=%u, record_channels=%u, "
                            "playback_frame_size_samples=%u, playback_sampling_rate=%u, "
                            "playback_channels=%u)",
                            record_frame_size_samples, record_sampling_rate, record_channels, playback_frame_size_samples, playback_sampling_rate, playback_channels);

            //if (record_sampling_rate == 16000) {
                // close()
                tmedia_denoise_close (audio->denoise);
                // open() with new values
                tmedia_denoise_open (audio->denoise,
                                     record_frame_size_samples, record_sampling_rate, TSK_CLAMP (1, record_channels, 2),
                                     playback_frame_size_samples, playback_sampling_rate, TSK_CLAMP (1, playback_channels, 2),
                                     purevoice_frame_size_samples, purevoice_sampling_rate, TSK_CLAMP (1, purevoice_channels, 2));
            //}
        }
    }

    audio->is_started = (ret == 0);
    
    if (audio->is_started
        && audio->encoder.free_frame_buf_lst
        && audio->encoder.filled_frame_buf_lst
        && audio->encoder.filled_frame_sema)
    {
        ret = tsk_thread_create (&(audio->encoder.producer_thread_handle),
                                 tdav_session_audio_producer_thread,
                                 (void*)self);
        if (0 != ret)
        {
            TSK_DEBUG_ERROR ("failed to create producer thread");
        }
    }

//    if (1)
//    {
//        audio->m_pReceiveFile = fopen ("/sdcard/recv.pcm", "wb");
//        audio->m_pOpusFile = fopen ("/sdcard/opus.data", "wb");
//    }
    
    if(base->rtp_manager)
        base->rtp_manager->rtp.is_audio_pause = false;
    
    TSK_DEBUG_INFO("is audio pause  : false ");

#ifdef MAC_OS
    /*
    char *path = GetExeDir ();
    char szPathName[1024];
    strcpy (szPathName, path);
    free (path);
    strcat (szPathName, "/opus.data");
    audio->m_pOpusEncodedFile = fopen (szPathName, "rb");
     */
#endif
 TSK_DEBUG_INFO("Leave");
    return ret;
}

static int tdav_session_audio_stop (tmedia_session_t *self)
{

    TSK_DEBUG_INFO("Enter");
    tdav_session_audio_t *audio = TDAV_SESSION_AUDIO (self);
    if (audio->is_started == tsk_false)
    {
        return 0;
    }
 
    audio->is_started = tsk_false;
    tdav_session_av_t *base = TDAV_SESSION_AV (self);
    //pause callback rtp data
    if(base->rtp_manager)
       base->rtp_manager->rtp.is_audio_pause = true;
    int ret = tdav_session_av_stop (base);
    
	if (audio->rscode_list != nullptr)
	{
		tsk_list_item_t *item = tsk_null;
		tsk_list_lock(audio->rscode_list);
		tsk_list_foreach(item, audio->rscode_list) {
			tdav_audio_rscode_stop(TDAV_AUDIO_RSCODE(item->data));
		}
		tsk_list_unlock(audio->rscode_list);
	}
	
    // The producer thread should be stopped before calling tdav_session_av_stop.
    // Because the producer thread is part of the producer, and the producer should
    // be stopped before stopping the rtp_manager(which is done in tdav_session_av_stop)
    // Otherwise, the producer thread may try to access the rtp_manager while the rtp_manager
    // has been stopped.
    //audio->is_started = tsk_false;
    if (audio->encoder.producer_thread_handle)
    {
        // Add one more item to the semaphore after audio->is_started was set to false.
        // Make sure the thread can always exit.
        tsk_semaphore_increment(audio->encoder.filled_frame_sema);
        
        // Audio producer log out, MUST increase the mixaud sema, to avoid dead lock
        tsk_semaphore_increment(audio->mic_mix_aud.mixaud_sema_lst);
        
        TSK_DEBUG_INFO("Start to join the producer thread");
        tsk_thread_join(&audio->encoder.producer_thread_handle);
        TSK_DEBUG_INFO("Successfully joined the producer thread");
    }
    
//    tdav_session_av_t *base = TDAV_SESSION_AV (self);
//    int ret = tdav_session_av_stop (base);
    
    if (audio->encoder.free_frame_buf_lst) {
        tsk_list_lock(audio->encoder.free_frame_buf_lst);
        tsk_list_clear_items(audio->encoder.free_frame_buf_lst);
        tsk_list_unlock(audio->encoder.free_frame_buf_lst);
    }
    if (audio->encoder.filled_frame_buf_lst) {
        tsk_list_lock(audio->encoder.filled_frame_buf_lst);
        tsk_list_clear_items(audio->encoder.filled_frame_buf_lst);
        tsk_list_unlock(audio->encoder.filled_frame_buf_lst);
    }
    if (audio->encoder.filled_frame_sema) {
        // No cross-platform API for reset the semaphore, so use this way.
        tsk_semaphore_destroy(&audio->encoder.filled_frame_sema);
        audio->encoder.filled_frame_sema = tsk_semaphore_create_2(0);
    }
    TSK_OBJECT_SAFE_FREE (audio->encoder.codec);
    TSK_OBJECT_SAFE_FREE (audio->decoder.codec);

    // close the jitter buffer and denoiser to be sure it will be reopened and reinitialized if
    // reINVITE or UPDATE
    // this is a "must" when the initial and updated sessions use codecs with different rate
    if (audio->jitterbuffer && audio->jitterbuffer->opened)
    {
        ret = tmedia_jitterbuffer_close (audio->jitterbuffer);
    }
    if (audio->denoise && audio->denoise->opened)
    {
        ret = tmedia_denoise_close (audio->denoise);
    }
//    if (NULL != audio->m_pReceiveFile)
//    {
//        fclose (audio->m_pReceiveFile);
//        audio->m_pReceiveFile = NULL;
//    }
    tdav_session_audio_close_pcm_files(audio);
    return ret;
}

static int tdav_session_audio_clear (tmedia_session_t* self, int sessionId)
{
	int ret = 0;
    TSK_DEBUG_INFO("tdav_session_audio_clear Enter");
    tdav_session_audio_t *audio = TDAV_SESSION_AUDIO (self);
    if (audio->is_started == tsk_false)
    {
        return 0;
    }
 	
 	// close rscode decode thread
    tdav_audio_rscode_t* rscode = tdav_session_audio_select_rscode_by_sessionid(audio, sessionId, RSC_TYPE_DECODE);
	if (rscode) {
		tdav_audio_rscode_stop(rscode);
		tsk_list_remove_item_by_data(audio->rscode_list, (tsk_object_t *)rscode);
	}
	
    return ret;
}

static int tdav_session_audio_send_dtmf (tmedia_session_t *self, uint8_t event)
{
    tdav_session_audio_t *audio;
    tdav_session_av_t *base;
    tmedia_codec_t *codec;
    int rate = 8000, ptime = 20;
    uint16_t duration;
    tdav_session_audio_dtmfe_t *dtmfe, *copy;
    int format = 101;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    audio = (tdav_session_audio_t *)self;
    base = (tdav_session_av_t *)self;

    // Find the DTMF codec to use to use the RTP payload
    if ((codec = tmedia_codec_find_by_format (TMEDIA_SESSION (audio)->codecs, TMEDIA_CODEC_FORMAT_DTMF)))
    {
        rate = (int)codec->out.rate;
        format = atoi (codec->neg_format ? codec->neg_format : codec->format);
        TSK_OBJECT_SAFE_FREE (codec);
    }

    /* do we have an RTP manager? */
    if (!base->rtp_manager)
    {
        TSK_DEBUG_ERROR ("No RTP manager associated to this session");
        return -2;
    }

    /* Create Events list */
    if (!audio->dtmf_events)
    {
        audio->dtmf_events = tsk_list_create ();
    }

    /* Start the timer manager */
    if (!audio->timer.started)
    {
        audio->timer.started = tsk_true;
    }


    /*	RFC 4733 - 5.  Examples

    +-------+-----------+------+--------+------+--------+--------+------+
    |  Time | Event     |   M  |  Time- |  Seq |  Event |  Dura- |   E  |
    |  (ms) |           |  bit |  stamp |   No |  Code  |   tion |  bit |
    +-------+-----------+------+--------+------+--------+--------+------+
    |     0 | "9"       |      |        |      |        |        |      |
    |       | starts    |      |        |      |        |        |      |
    |    50 | RTP       |  "1" |      0 |    1 |    9   |    400 |  "0" |
    |       | packet 1  |      |        |      |        |        |      |
    |       | sent      |      |        |      |        |        |      |
    |   100 | RTP       |  "0" |      0 |    2 |    9   |    800 |  "0" |
    |       | packet 2  |      |        |      |        |        |      |
    |       | sent      |      |        |      |        |        |      |
    |   150 | RTP       |  "0" |      0 |    3 |    9   |   1200 |  "0" |
    |       | packet 3  |      |        |      |        |        |      |
    |       | sent      |      |        |      |        |        |      |
    |   200 | RTP       |  "0" |      0 |    4 |    9   |   1600 |  "0" |
    |       | packet 4  |      |        |      |        |        |      |
    |       | sent      |      |        |      |        |        |      |
    |   200 | "9" ends  |      |        |      |        |        |      |
    |   250 | RTP       |  "0" |      0 |    5 |    9   |   1600 |  "1" |
    |       | packet 4  |      |        |      |        |        |      |
    |       | first     |      |        |      |        |        |      |
    |       | retrans-  |      |        |      |        |        |      |
    |       | mission   |      |        |      |        |        |      |
    |   300 | RTP       |  "0" |      0 |    6 |    9   |   1600 |  "1" |
    |       | packet 4  |      |        |      |        |        |      |
    |       | second    |      |        |      |        |        |      |
    |       | retrans-  |      |        |      |        |        |      |
    |       | mission   |      |        |      |        |        |      |
    =====================================================================
    |   880 | First "1" |      |        |      |        |        |      |
    |       | starts    |      |        |      |        |        |      |
    |   930 | RTP       |  "1" |   7040 |    7 |    1   |    400 |  "0" |
    |       | packet 5  |      |        |      |        |        |      |
    |       | sent      |      |        |      |        |        |      |
    */

    // ref()(thread safeness)
    audio = (tdav_session_audio_t *)tsk_object_ref (audio);

    // says we're sending DTMF digits to avoid mixing with audio (SRTP won't let this happen because
    // of senquence numbers)
    // flag will be turned OFF when the list is empty
    audio->is_sending_dtmf_events = tsk_true;

    duration = TMEDIA_CODEC_PCM_FRAME_SIZE_AUDIO_ENCODING (audio->encoder.codec);

    // lock() list
    tsk_list_lock (audio->dtmf_events);

    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 1, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_true, tsk_false);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 0, _tdav_session_audio_dtmfe_timercb, copy);
    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 2, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_false, tsk_false);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 1, _tdav_session_audio_dtmfe_timercb, copy);
    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 3, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_false, tsk_false);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 2, _tdav_session_audio_dtmfe_timercb, copy);
    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 4, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_false, tsk_false);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 3, _tdav_session_audio_dtmfe_timercb, copy);

    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 4, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_false, tsk_true);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 4, _tdav_session_audio_dtmfe_timercb, copy);
    copy = dtmfe = _tdav_session_audio_dtmfe_create (audio, event, duration * 4, ++base->rtp_manager->rtp.seq_num, base->rtp_manager->rtp.timestamp, (uint8_t)format, tsk_false, tsk_true);
    tsk_list_push_back_data (audio->dtmf_events, (void **)&dtmfe);
    xt_timer_mgr_global_schedule (ptime * 5, _tdav_session_audio_dtmfe_timercb, copy);

    // unlock() list
    tsk_list_unlock (audio->dtmf_events);

    // increment timestamp
    base->rtp_manager->rtp.timestamp += duration;

    // unref()(thread safeness)
    audio = (tdav_session_audio_t *)tsk_object_unref (audio);

    return 0;
}

static int tdav_session_audio_pause (tmedia_session_t *self)
{
    return tdav_session_av_pause (TDAV_SESSION_AV (self));
}

static const tsdp_header_M_t *tdav_session_audio_get_lo (tmedia_session_t *self)
{
    tsk_bool_t updated = tsk_false;
    const tsdp_header_M_t *ret;
    tdav_session_av_t *base = TDAV_SESSION_AV (self);


    if (!(ret = tdav_session_av_get_lo (base, &updated)))
    {
        TSK_DEBUG_ERROR ("tdav_session_av_get_lo(audio) failed");
        return tsk_null;
    }

    if (updated)
    {
        tsk_safeobj_lock (base);
        TSK_OBJECT_SAFE_FREE (TDAV_SESSION_AUDIO (self)->encoder.codec);
        tsk_safeobj_unlock (base);
    }

    return ret;
}

static int tdav_session_audio_set_ro (tmedia_session_t *self, const tsdp_header_M_t *m)
{
    int ret;
    tsk_bool_t updated = tsk_false;
    tdav_session_av_t *base = TDAV_SESSION_AV (self);

    if ((ret = tdav_session_av_set_ro (base, m, &updated)))
    {
        TSK_DEBUG_ERROR ("tdav_session_av_set_ro(audio) failed");
        return ret;
    }

    if (updated)
    {
        tsk_safeobj_lock (base);
        // reset audio jitter buffer (new Offer probably comes with new seq_nums or timestamps)
        if (base->consumer)
        {
            ret = tdav_consumer_audio_reset (TDAV_CONSUMER_AUDIO (base->consumer));
        }
        // destroy encoder to force requesting new one
        TSK_OBJECT_SAFE_FREE (TDAV_SESSION_AUDIO (self)->encoder.codec);
        tsk_safeobj_unlock (base);
    }

    return ret;
}


/* Internal function used to create new DTMF event */
static tdav_session_audio_dtmfe_t *_tdav_session_audio_dtmfe_create (const tdav_session_audio_t *session, uint8_t event, uint16_t duration, uint32_t seq, uint32_t timestamp, uint8_t format, tsk_bool_t M, tsk_bool_t E)
{
    tdav_session_audio_dtmfe_t *dtmfe;
    const tdav_session_av_t *base = (const tdav_session_av_t *)session;
    static uint8_t volume = 10;
    static uint32_t ssrc = 0x5234A8;

    uint8_t pay[4] = { 0 };

    /* RFC 4733 - 2.3.  Payload Format
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |     event     |E|R| volume    |          duration             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    if (!(dtmfe = (tdav_session_audio_dtmfe_t *)tsk_object_new (tdav_session_audio_dtmfe_def_t)))
    {
        TSK_DEBUG_ERROR ("Failed to create new DTMF event");
        return tsk_null;
    }
    dtmfe->session = session;

    if (!(dtmfe->packet = trtp_rtp_packet_create ((session && base->rtp_manager) ? base->rtp_manager->rtp.ssrc.local : ssrc, seq, timestamp, format, M)))
    {
        TSK_DEBUG_ERROR ("Failed to create DTMF RTP packet");
        TSK_OBJECT_SAFE_FREE (dtmfe);
        return tsk_null;
    }

    pay[0] = event;
    pay[1] |= ((E << 7) | (volume & 0x3F));
    pay[2] = (duration >> 8);
    pay[3] = (duration & 0xFF);

    /* set data */
    if ((dtmfe->packet->payload.data = tsk_calloc (sizeof (pay), sizeof (uint8_t))))
    {
        memcpy (dtmfe->packet->payload.data, pay, sizeof (pay));
        dtmfe->packet->payload.size = sizeof (pay);
    }

    return dtmfe;
}

static int _tdav_session_audio_dtmfe_timercb (const void *arg, xt_timer_id_t timer_id)
{
    tdav_session_audio_dtmfe_t *dtmfe = (tdav_session_audio_dtmfe_t *)arg;
    tdav_session_audio_t *audio;

    if (!dtmfe || !dtmfe->session || !dtmfe->session->dtmf_events)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* Send the data */
    TSK_DEBUG_INFO ("Sending DTMF event...");
    trtp_manager_send_rtp_packet (TDAV_SESSION_AV (dtmfe->session)->rtp_manager, dtmfe->packet, tsk_false);


    audio = (tdav_session_audio_t *)tsk_object_ref (TSK_OBJECT (dtmfe->session));
    tsk_list_lock (audio->dtmf_events);
    /* Remove and delete the event from the queue */
    tsk_list_remove_item_by_data (audio->dtmf_events, dtmfe);
    /* Check if there are pending events */
    audio->is_sending_dtmf_events = !TSK_LIST_IS_EMPTY (audio->dtmf_events);
    tsk_list_unlock (audio->dtmf_events);
    tsk_object_unref (audio);

    return 0;
}


//=================================================================================================
//	Session Audio Plugin object definition
//
/* constructor */
static tsk_object_t *tdav_session_audio_ctor (tsk_object_t *self, va_list *app)
{
    tdav_session_audio_t *audio = (tdav_session_audio_t *)self;
    tsk_list_item_t *free_item;
    int LoopCnt;
    uint16_t recordSampleRate = 0;
    uint16_t playbackSampleRate = 0;

    if (audio)
    {
		audio->rscode_list = nullptr;
		audio->iCombineCount = Config_GetInt("AUDIO_COM_COUNT", 1);
		audio->combinePacket_list = tsk_list_create();
        int ret;
        tdav_session_av_t *base = TDAV_SESSION_AV (self);

        /* init() base */
        if ((ret = tdav_session_av_init (base, tmedia_audio)) != 0)
        {
            TSK_DEBUG_ERROR ("tdav_session_av_init(audio) failed");
            return tsk_null;
        }
        audio->mixAudioVol = 100;
        audio->mixLastVol = 0;
        audio->isMicMute = tsk_true;
        audio->isSpeakerMute = tsk_true;
        audio->micBypassToSpk = tsk_false;
        audio->bgmBypassToSpk = tsk_false;
        audio->isHeadsetPlugin = tsk_false;
        audio->isPcmCallback = tsk_false;
        audio->isRemoteCallbackOn = tsk_false;
        audio->isRecordCallbackOn = tsk_false;
        audio->isMixCallbackOn = tsk_false;
        audio->isSaveScreen = tsk_false;
        audio->isTranscriberOn = tsk_false;
        audio->vad.len_in_frame_num = tmedia_defaults_get_vad_len_ms() / 20;
        audio->vad.silence_percent = tmedia_defaults_get_vad_silence_percent();
        audio->vad.pre_frame_silence = tsk_true;
        audio->vad.total_frame_count = 0;
        audio->vad.silence_frame_count = 0;
        audio->vad.callback = tsk_null;
        audio->recOverflowCount = 0;
        audio->stat.time_base_ms = tsk_gettimeofday_ms();
        audio->stat.send_timestamp_period_ms = tmedia_defaults_get_packet_stat_report_period_ms();
        audio->micVolumeGain = (float)tmedia_defaults_get_mic_volume_gain() / 100.0f;
        
        /* pcm dumpping files */
        audio->pcm_file.max_size = tmedia_defaults_get_pcm_file_size_kb() * 1024;
        audio->pcm_file.mic = NULL;
        audio->pcm_file.mic_size = 0;
        audio->pcm_file.speaker = NULL;
        audio->pcm_file.speaker_size = 0;
        audio->pcm_file.send_pcm = NULL;
        audio->pcm_file.send_pcm_size = 0;
        
        recordSampleRate = tmedia_defaults_get_record_sample_rate();      // Max support record sample rate;
        playbackSampleRate = tmedia_defaults_get_playback_sample_rate();  // Max support playback sample rate;

        /* init() self */
        tsk_safeobj_init(&(audio->bc));
        tsk_safeobj_init(&(audio->stat));

        if (base->producer)
        {
            audio->encoder.free_frame_buf_lst = tsk_list_create();
            audio->encoder.filled_frame_buf_lst = tsk_list_create();
            audio->encoder.filled_frame_sema = tsk_semaphore_create_2(0);
            audio->encoder.codec_dtx_peroid_num = tmedia_defaults_get_opus_dtx_peroid() / 20;
            if (audio->encoder.codec_dtx_peroid_num > TDAV_AUDIO_OPUS_DTX_MAX_PEROID_NUM) {
                audio->encoder.codec_dtx_peroid_num = TDAV_AUDIO_OPUS_DTX_MAX_PEROID_NUM;
            }
            audio->encoder.codec_dtx_current_num = 0;
            tmedia_producer_set_enc_callback (base->producer, tdav_session_audio_producer_enc_cb, audio);
            
            audio->mic_mix_aud.mixaud_enable = tsk_false;
            audio->mic_mix_aud.mixaud_rate   = 44100;
            audio->mic_mix_aud.mixaud_filled_position = 0;
            audio->mic_mix_aud.mixaud_lstitem_backup  = NULL;
            audio->mic_mix_aud.mixaud_filled_lst = tsk_list_create();
            audio->mic_mix_aud.mixaud_free_lst   = tsk_list_create();
            audio->mic_mix_aud.mixaud_sema_lst   = tsk_semaphore_create_2(MAX_MIXAUD_ITEM_NUM);
            audio->mic_mix_aud.mixau_timeout_count = 0;
            
#ifdef USE_WEBRTC_RESAMPLE
            audio->mic_mix_aud.mixaud_resampler_1 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
            (static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_1))->InitializeIfNeeded(audio->mic_mix_aud.mixaud_rate, recordSampleRate, 1);
            audio->mic_mix_aud.mixaud_resampler_2 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
            (static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_2))->InitializeIfNeeded(audio->mic_mix_aud.mixaud_rate, tmedia_defaults_get_playback_sample_rate(), 1);
            audio->mic_mix_aud.mixaud_resampler_3 = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
            (static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_3))->InitializeIfNeeded(recordSampleRate, playbackSampleRate, 1);
#else
            
			audio->mic_mix_aud.mixaud_resampler_1 = speex_resampler_init(1, audio->mic_mix_aud.mixaud_rate, recordSampleRate, 3, NULL); // fake resampler handler
            audio->mic_mix_aud.mixaud_resampler_2 = speex_resampler_init(1, audio->mic_mix_aud.mixaud_rate, tmedia_defaults_get_playback_sample_rate(), 3, NULL); // fake resampler handler
            audio->mic_mix_aud.mixaud_resampler_3 = speex_resampler_init(1, recordSampleRate, playbackSampleRate, 3, NULL); // fake resampler handler
#endif
            audio->mic_mix_aud.mixaud_resampler_mutex_1 = tsk_mutex_create_2(tsk_false);
            
            audio->mic_mix_aud.mixaud_resampler_mutex_2 = tsk_mutex_create_2(tsk_false);
            
            audio->mic_mix_aud.mixaud_resampler_mutex_3 = tsk_mutex_create_2(tsk_false);
            
            audio->mic_mix_aud.mixaud_tempbuf1   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_tempbuf2   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_tempbuf3   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_tempbuf4   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_spkoutbuf  = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_pcm_cb_buff = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_tempbuf1, 0, MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_tempbuf2, 0, MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_tempbuf3, 0, MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_tempbuf4, 0, MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_spkoutbuf, 0, MAX_TEMP_BUF_BYTESZ);
            memset(audio->mic_mix_aud.mixaud_pcm_cb_buff, 0, MAX_TEMP_BUF_BYTESZ);
            
            // Fill free list, 增加MAX_MIXAUD_ITEM_TIMEOUT_NUM  个，用于超时处理
            for (LoopCnt = 0; LoopCnt < MAX_MIXAUD_ITEM_NUM + MAX_MIXAUD_ITEM_TIMEOUT_NUM ; LoopCnt++) {
                free_item = tsk_list_item_create();
                free_item->data = tsk_object_new (tdav_session_audio_frame_buffer_def_t, MAX_FRAME_SIZE);
                tsk_list_lock(audio->mic_mix_aud.mixaud_free_lst);
                tsk_list_push_item(audio->mic_mix_aud.mixaud_free_lst, &free_item, tsk_true);
                tsk_list_unlock(audio->mic_mix_aud.mixaud_free_lst);
            }
            
            audio->mic_mix_aud.mixaud_delayproc = (mic_mix_aud_delay_t *)tsk_malloc(sizeof(mic_mix_aud_delay_t));
            audio->mic_mix_aud.mixaud_delayproc->i2SetDelay      = tmedia_defaults_get_external_input_mode() ? 0 : tmedia_defaults_get_backgroundmusic_delay();// 外部输入下不产生bgm那一路的延迟
            audio->mic_mix_aud.mixaud_delayproc->i2ReadBlk       = 0;
            audio->mic_mix_aud.mixaud_delayproc->i2WriteBlk      = 0;
            audio->mic_mix_aud.mixaud_delayproc->i2TimePerBlk    = (int16_t)tmedia_defaults_get_audio_ptime();
            audio->mic_mix_aud.mixaud_delayproc->pDelayProcBuf   = tsk_malloc(MAX_DELAY_MS * (recordSampleRate / 1000) * sizeof(int16_t)); // 540/20*(16000/1000*20)*2
            audio->mic_mix_aud.mixaud_delayproc->pDelayOutputBuf = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_delayproc->i2MaxBlkNum     = MAX_DELAY_MS / audio->mic_mix_aud.mixaud_delayproc->i2TimePerBlk - 1;
            memset(audio->mic_mix_aud.mixaud_delayproc->pDelayProcBuf, 0, MAX_DELAY_MS * (recordSampleRate / 1000) * sizeof(int16_t));
            memset(audio->mic_mix_aud.mixaud_delayproc->pDelayOutputBuf, 0, MAX_TEMP_BUF_BYTESZ);
            audio->mic_mix_aud.mixaud_delayproc->pDelayMutex = tsk_mutex_create_2(tsk_false);
        }
        
        if (base->consumer)
        {
            // It's important to create the denoiser and jitter buffer here as dynamic plugins (from
            // shared libs) don't have access to the registry
            if (!(audio->denoise = tmedia_denoise_create ()))
            {
                TSK_DEBUG_WARN ("No Audio denoiser found");
            }
            else
            {
                // IMPORTANT: This means that the consumer must be child of "tdav_consumer_audio_t"
                // object
                tdav_consumer_audio_set_denoise (TDAV_CONSUMER_AUDIO (base->consumer), audio->denoise);
            }

            if (!(audio->jitterbuffer = tmedia_jitterbuffer_create (tmedia_audio)))
            {
                TSK_DEBUG_ERROR ("Failed to create jitter buffer");
            }
            else
            {
                ret = tmedia_jitterbuffer_init (TMEDIA_JITTER_BUFFER (audio->jitterbuffer));
                tdav_consumer_audio_set_jitterbuffer (TDAV_CONSUMER_AUDIO (base->consumer), audio->jitterbuffer);
            }
        }
#ifdef ANDROID
        audio->resample = tdav_audio_resample_create(0, tsk_false, 1, recordSampleRate, 44100);
#else
        audio->resample = tdav_audio_resample_create(0, tsk_false, 1, recordSampleRate, 48000);
#endif
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_session_audio_dtor (tsk_object_t *self)
{
    tdav_session_audio_t *audio = (tdav_session_audio_t *)self;
    
    TSK_DEBUG_INFO ("*** tdav_session_audio_t destroyed ***");
    if (audio)
    {

		TSK_OBJECT_SAFE_FREE(audio->combinePacket_list);

		TSK_OBJECT_SAFE_FREE(audio->rscode_list);
        tdav_session_audio_stop ((tmedia_session_t *)audio);
        // Do it in this order (deinit self first)

        /* Timer manager */
        if (audio->timer.started)
        {
            if (audio->dtmf_events)
            {
                /* Cancel all events */
                tsk_list_item_t *item;
                tsk_list_foreach (item, audio->dtmf_events)
                {
                    xt_timer_mgr_global_cancel (((tdav_session_audio_dtmfe_t *)item->data)->timer_id);
                }
            }
        }

        /* CleanUp the DTMF events */
        TSK_OBJECT_SAFE_FREE (audio->dtmf_events);

        TSK_OBJECT_SAFE_FREE (audio->denoise);
        TSK_OBJECT_SAFE_FREE (audio->jitterbuffer);

        TSK_OBJECT_SAFE_FREE (audio->encoder.codec);
        TSK_FREE (audio->encoder.buffer);
        TSK_FREE (audio->encoder.rtpExtBuffer);
        TSK_FREE (audio->encoder.bwCtrlBuffer);
        TSK_OBJECT_SAFE_FREE (audio->decoder.codec);
        TSK_FREE (audio->decoder.buffer);

        // free resamplers
        TSK_FREE (audio->encoder.resampler.buffer);
        TSK_OBJECT_SAFE_FREE (audio->encoder.resampler.instance);
        TSK_FREE (audio->decoder.resampler.buffer);
        TSK_OBJECT_SAFE_FREE (audio->decoder.resampler.instance);
        
        // free recording frame buffer list
        if (audio->encoder.free_frame_buf_lst) {
            tsk_list_clear_items (audio->encoder.free_frame_buf_lst);
            TSK_OBJECT_SAFE_FREE (audio->encoder.free_frame_buf_lst);
        }
        if (audio->encoder.filled_frame_buf_lst) {
            tsk_list_clear_items (audio->encoder.filled_frame_buf_lst);
            TSK_OBJECT_SAFE_FREE (audio->encoder.filled_frame_buf_lst);
        }
        if (audio->encoder.filled_frame_sema) {
            tsk_semaphore_destroy(&audio->encoder.filled_frame_sema);
        }
        
        if (audio->mic_mix_aud.mixaud_sema_lst) {
            tsk_semaphore_destroy(&audio->mic_mix_aud.mixaud_sema_lst);
        }
        audio->mic_mix_aud.mixau_timeout_count = 0;
        if (audio->mic_mix_aud.mixaud_free_lst) {
            tsk_list_clear_items(audio->mic_mix_aud.mixaud_free_lst);
            TSK_OBJECT_SAFE_FREE(audio->mic_mix_aud.mixaud_free_lst);
        }
        
        if( audio->mic_mix_aud.mixaud_lstitem_backup )
        {
            TSK_OBJECT_SAFE_FREE( audio->mic_mix_aud.mixaud_lstitem_backup );
        }
        if (audio->mic_mix_aud.mixaud_filled_lst) {
            tsk_list_clear_items(audio->mic_mix_aud.mixaud_filled_lst);
            TSK_OBJECT_SAFE_FREE(audio->mic_mix_aud.mixaud_filled_lst);
        }
        if (audio->mic_mix_aud.mixaud_resampler_1) {
            tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_1);
#ifdef USE_WEBRTC_RESAMPLE
            delete static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_1);
            audio->mic_mix_aud.mixaud_resampler_1 = nullptr;
#else
            speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_1);
            audio->mic_mix_aud.mixaud_resampler_1 = tsk_null;
#endif
            tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_1);
        }
        if (audio->mic_mix_aud.mixaud_resampler_mutex_1) {
            tsk_mutex_destroy(&(audio->mic_mix_aud.mixaud_resampler_mutex_1));
        }
        if (audio->mic_mix_aud.mixaud_resampler_2) {
            tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_2);
#ifdef USE_WEBRTC_RESAMPLE
            delete static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_2);
            audio->mic_mix_aud.mixaud_resampler_2 = nullptr;
#else
            speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_2);
            audio->mic_mix_aud.mixaud_resampler_2 = tsk_null;
#endif
            tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_2);
        }
        if (audio->mic_mix_aud.mixaud_resampler_mutex_2) {
            tsk_mutex_destroy(&(audio->mic_mix_aud.mixaud_resampler_mutex_2));
        }
        if (audio->mic_mix_aud.mixaud_resampler_3) {
            tsk_mutex_lock(audio->mic_mix_aud.mixaud_resampler_mutex_3);
#ifdef USE_WEBRTC_RESAMPLE
            delete static_cast<youme::webrtc::PushResampler<int16_t> *>(audio->mic_mix_aud.mixaud_resampler_3);
            audio->mic_mix_aud.mixaud_resampler_3 = nullptr;
#else
            speex_resampler_destroy(audio->mic_mix_aud.mixaud_resampler_3);
            audio->mic_mix_aud.mixaud_resampler_3 = tsk_null;
#endif
            tsk_mutex_unlock(audio->mic_mix_aud.mixaud_resampler_mutex_3);
        }
        if (audio->mic_mix_aud.mixaud_resampler_mutex_3) {
            tsk_mutex_destroy(&(audio->mic_mix_aud.mixaud_resampler_mutex_3));
        }
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_tempbuf1);
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_tempbuf2);
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_tempbuf3);
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_tempbuf4);
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_spkoutbuf);
        TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_pcm_cb_buff);
        
        if (audio->mic_mix_aud.mixaud_delayproc) {
            if (audio->mic_mix_aud.mixaud_delayproc->pDelayOutputBuf) {
                TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_delayproc->pDelayOutputBuf);
            }
            if (audio->mic_mix_aud.mixaud_delayproc->pDelayProcBuf) {
                TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_delayproc->pDelayProcBuf);
            }
            tsk_mutex_destroy(&(audio->mic_mix_aud.mixaud_delayproc->pDelayMutex));
            TSK_SAFE_FREE(audio->mic_mix_aud.mixaud_delayproc);
        }
        
        if (audio->resample) {
            TSK_OBJECT_SAFE_FREE(audio->resample);
        }
        
        tsk_safeobj_deinit(&(audio->bc));
        tsk_safeobj_deinit(&(audio->stat));
        
        /* deinit base */
        tdav_session_av_deinit (TDAV_SESSION_AV (self));
        
        TSK_DEBUG_INFO ("*** Audio session destroyed ***");
    }

    return self;
}
/* object definition */
static const tsk_object_def_t tdav_session_audio_def_s = {
    sizeof (tdav_session_audio_t), tdav_session_audio_ctor, tdav_session_audio_dtor, tmedia_session_cmp,
};
/* plugin definition*/
static const tmedia_session_plugin_def_t tdav_session_audio_plugin_def_s = { &tdav_session_audio_def_s,

                                                                             tmedia_audio,
                                                                             "audio",

                                                                             tdav_session_audio_set,
                                                                             tdav_session_audio_get,
                                                                             tdav_session_audio_prepare,
                                                                             tdav_session_audio_start,
                                                                             tdav_session_audio_pause,
                                                                             tdav_session_audio_stop,
                                                                             tdav_session_audio_clear,

                                                                             { tdav_session_audio_send_dtmf },

                                                                             tdav_session_audio_get_lo,
                                                                             tdav_session_audio_set_ro };
const tmedia_session_plugin_def_t *tdav_session_audio_plugin_def_t = &tdav_session_audio_plugin_def_s;
static const tmedia_session_plugin_def_t tdav_session_bfcpaudio_plugin_def_s = { &tdav_session_audio_def_s,

                                                                                 tmedia_bfcp_audio,
                                                                                 "audio",

                                                                                 tdav_session_audio_set,
                                                                                 tdav_session_audio_get,
                                                                                 tdav_session_audio_prepare,
                                                                                 tdav_session_audio_start,
                                                                                 tdav_session_audio_pause,
                                                                                 tdav_session_audio_stop,
                                                                                 tsk_null,

                                                                                 /* Audio part */
                                                                                 { tdav_session_audio_send_dtmf },

                                                                                 tdav_session_audio_get_lo,
                                                                                 tdav_session_audio_set_ro };
const tmedia_session_plugin_def_t *tdav_session_bfcpaudio_plugin_def_t = &tdav_session_bfcpaudio_plugin_def_s;


//=================================================================================================
//	DTMF event object definition
//
static tsk_object_t *tdav_session_audio_dtmfe_ctor (tsk_object_t *self, va_list *app)
{
    tdav_session_audio_dtmfe_t *event = (tdav_session_audio_dtmfe_t *)self;
    if (event)
    {
        event->timer_id = XT_INVALID_TIMER_ID;
    }
    return self;
}

static tsk_object_t *tdav_session_audio_dtmfe_dtor (tsk_object_t *self)
{
    tdav_session_audio_dtmfe_t *event = (tdav_session_audio_dtmfe_t *)self;
    if (event)
    {
        TSK_OBJECT_SAFE_FREE (event->packet);
    }

    return self;
}

static int tdav_session_audio_dtmfe_cmp (const tsk_object_t *_e1, const tsk_object_t *_e2)
{
    int ret;
    tsk_subsat_int32_ptr (_e1, _e2, &ret);
    return ret;
}

static const tsk_object_def_t tdav_session_audio_dtmfe_def_s = {
    sizeof (tdav_session_audio_dtmfe_t), tdav_session_audio_dtmfe_ctor, tdav_session_audio_dtmfe_dtor, tdav_session_audio_dtmfe_cmp,
};
const tsk_object_def_t *tdav_session_audio_dtmfe_def_t = &tdav_session_audio_dtmfe_def_s;


//=================================================================================================
//	audio frame buffer object definition
//
static tsk_object_t *tdav_session_audio_frame_buffer_ctor (tsk_object_t *self, va_list *app)
{
    tdav_session_audio_frame_buffer_t *frame_buf = (tdav_session_audio_frame_buffer_t *)self;
    uint32_t alloc_size = va_arg(*app, uint32_t);
    if (frame_buf)
    {
        if (alloc_size > 0) {
            frame_buf->pcm_data = tsk_calloc(alloc_size, 1);
            frame_buf->alloc_size = alloc_size;
        }
    }
    return self;
}

static tsk_object_t *tdav_session_audio_frame_buffer_dtor (tsk_object_t *self)
{
    tdav_session_audio_frame_buffer_t *frame_buf = (tdav_session_audio_frame_buffer_t *)self;
    if (frame_buf)
    {
        if ((frame_buf->alloc_size > 0) && (frame_buf->pcm_data)) {
            tsk_free(&frame_buf->pcm_data);
        }
    }
    return self;
}

static int tdav_session_audio_frame_buffer_cmp (const tsk_object_t *_e1, const tsk_object_t *_e2)
{
    int ret;
    tsk_subsat_int32_ptr (_e1, _e2, &ret);
    return ret;
}

static const tsk_object_def_t tdav_session_audio_frame_buffer_def_s = {
    sizeof (tdav_session_audio_frame_buffer_t),
    tdav_session_audio_frame_buffer_ctor,
    tdav_session_audio_frame_buffer_dtor,
    tdav_session_audio_frame_buffer_cmp
};
const tsk_object_def_t *tdav_session_audio_frame_buffer_def_t = &tdav_session_audio_frame_buffer_def_s;


//=================================================================================================
//	audio vad result object definition
//
static tsk_object_t *tdav_session_audio_vad_result_ctor (tsk_object_t *self, va_list *app)
{
    tdav_session_audio_vad_result_t *vad_item = (tdav_session_audio_vad_result_t *)self;
    if (vad_item)
    {
        vad_item->i4SessionId = 0;
        vad_item->u1VadRes = 0;
    }
    return self;
}

static tsk_object_t *tdav_session_audio_vad_result_dtor (tsk_object_t *self)
{
    tdav_session_audio_vad_result_t *vad_item = (tdav_session_audio_vad_result_t *)self;
    if (vad_item)
    {
        vad_item->i4SessionId = 0;
        vad_item->u1VadRes = 0;
    }
    return self;
}

static int tdav_session_audio_vad_result_cmp (const tsk_object_t *_e1, const tsk_object_t *_e2)
{
    int ret;
    tsk_subsat_int32_ptr (_e1, _e2, &ret);
    return ret;
}

static const tsk_object_def_t tdav_session_audio_vad_result_def_s = {
    sizeof (tdav_session_audio_vad_result_t),
    tdav_session_audio_vad_result_ctor,
    tdav_session_audio_vad_result_dtor,
    tdav_session_audio_vad_result_cmp
};
const tsk_object_def_t *tdav_session_audio_vad_result_def_t = &tdav_session_audio_vad_result_def_s;

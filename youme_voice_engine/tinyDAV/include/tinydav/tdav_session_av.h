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

/**@file tdav_session_av.h
 * @brief Audio/Video/T.140 base Session plugin
 */

#ifndef TINYDAV_SESSION_AV_H
#define TINYDAV_SESSION_AV_H

#include "tinydav_config.h"

#include "tinymedia/tmedia_session.h"
#include "tsk_safeobj.h"

TDAV_BEGIN_DECLS

#define TDAV_SESSION_AV(self) ((tdav_session_av_t*)(self))

typedef struct tdav_session_av_s
{
	TMEDIA_DECLARE_SESSION;

	tsk_bool_t use_ipv6;

	enum tmedia_type_e media_type;
	enum tmedia_profile_e media_profile;
	enum tmedia_mode_e avpf_mode_set;
	enum tmedia_mode_e avpf_mode_neg;
	tsk_bool_t is_fb_fir_neg; // a=rtcp-fb:* ccm fir
	tsk_bool_t is_fb_nack_neg; // a=rtcp-fb:* nack
	tsk_bool_t is_fb_googremb_neg; // a=rtcp-fb:* goog-remb
	//tsk_bool_t use_srtp;
	tsk_bool_t is_webrtc2sip_mode_enabled;
	uint32_t rtp_ssrc;

    uint64_t time_last_frame_loss_report; // from jb
	int32_t bandwidth_max_upload_kbps;
	int32_t bandwidth_max_download_kbps;
	int32_t fps;
	tsk_bool_t congestion_ctrl_enabled;
	tmedia_pref_video_size_t pref_size; // output

	/* sdp capabilities (RFC 5939) */
	struct tdav_sdp_caps_s* sdp_caps;


	
	char* local_ip;
	char* remote_ip;
	uint16_t remote_port;
	struct tsdp_message_s* remote_sdp;
	struct tsdp_message_s* local_sdp;

	struct trtp_manager_s* rtp_manager;
    tsk_mutex_handle_t   * rtp_manager_mutex;

	struct tmedia_consumer_s* consumer;
	struct tmedia_producer_s* producer;
    tsk_mutex_handle_t* producer_mutex;
    
    tsk_bool_t useHalMode;
    tsk_bool_t forceUseHalMode;     //如果用voip启动失败了，那强制用hal，知道session重新创建


	struct{
		char* reason;
		tsk_bool_t is_fatal;
		void* tid[1];
	} last_error;
	
	// codec's payload type mapping used when bypassing is enabled
	struct{
		int8_t local;
		int8_t remote;
		int8_t neg;
	} pt_map;
    
    int32_t screenOrientation;

	TSK_DECLARE_SAFEOBJ;
}
tdav_session_av_t;

#define TDAV_DECLARE_SESSION_AV tdav_session_av_t __session_av__

int tdav_session_av_init(tdav_session_av_t* self, tmedia_type_t media_type);
tsk_bool_t tdav_session_av_set(tdav_session_av_t* self, const struct tmedia_param_s* param);
tsk_bool_t tdav_session_av_get(tdav_session_av_t* self, struct tmedia_param_s* param);
int tdav_session_av_prepare(tdav_session_av_t* self);
int tdav_session_av_init_encoder(tdav_session_av_t* self, struct tmedia_codec_s* encoder);
int tdav_session_av_start(tdav_session_av_t* self, const struct tmedia_codec_s* best_codec);

// video av start接口，不改动rtp manager对象，新增接口避免改动audio逻辑
int tdav_session_av_start_video(tdav_session_av_t* self, const struct tmedia_codec_s* best_codec);

int tdav_session_av_stop(tdav_session_av_t* self);
int tdav_session_av_pause(tdav_session_av_t* self);
const tsdp_header_M_t* tdav_session_av_get_lo(tdav_session_av_t* self, tsk_bool_t *updated);
int tdav_session_av_set_ro(tdav_session_av_t* self, const struct tsdp_header_M_s* m, tsk_bool_t *updated);
const tmedia_codec_t* tdav_session_av_get_best_neg_codec(const tdav_session_av_t* self);
int tdav_session_av_deinit(tdav_session_av_t* self);

// video和audio使用同个rtp manager对象，由audio session初始化rtp manager对象后set&get，以方便video模块调用
void tdav_session_set_rtp_manager(struct trtp_manager_s* rtp_manager);
struct trtp_manager_s* tdav_session_get_rtp_manager();

///////////////////////////////////////////////////////////////////////////////////////
// For getting statistics data
typedef struct tdav_session_av_stat_item_s
{
    int32_t   sessionId;
    uint16_t  packetLossRate10000th;
    uint32_t  avgPacketTimeMs;
    uint32_t  networkDelayMs;
} tdav_session_av_stat_item_t;

typedef struct tdav_session_av_stat_info_s
{
    TSK_DECLARE_OBJECT;
    
    uint32_t numOfItems;
    tdav_session_av_stat_item_t *statItems;
} tdav_session_av_stat_info_t;
extern const tsk_object_def_t *tdav_session_av_stat_info_def_t;


typedef struct tdav_session_av_qos_stat_s
{
	uint32_t audioUpLossCnt; 	// 半程统计
	uint32_t audioUpTotalCnt; 	// 半程统计
	uint32_t videoUpLossCnt; 	// 半程统计
	uint32_t videoUpTotalCnt; 	// 半程统计

	uint32_t audioDnLossrate;	// 全程统计，多路rtcp取最大值
	uint32_t audioRtt;			// 全程统计，多路rtt取最大值
	uint32_t videoDnLossrate;	// 全程统计，多路rtcp取最大值
	uint32_t videoRtt;			// 全程统计，多路rtt取最大值

	uint32_t auidoUpbitrate;	// 上报周期内取最大值
	uint32_t auidoDnbitrate;	// 上报周期内取最大值，下行为多路下行之和

	uint32_t videoUpCamerabitrate;	// 上报周期内取最大值，上行camera流带宽（包括大小流）
	uint32_t videoUpSharebitrate;	// 上报周期内取最大值，上行share流带宽（仅包括share流）
	uint32_t videoDnCamerabitrate;	// 上报周期内取最大值，下行为多路下行之和，包括视频大小流
	uint32_t videoDnSharebitrate;	// 上报周期内取最大值，下行为多路下行之和，仅包括share流

	uint32_t videoUpStream;		// 视频上行流统计，bit0:大流，bit1:小流，bit2:share
	uint32_t videoDnFps;		// 视频解码后FPS

} tdav_session_av_qos_stat_t;

TDAV_END_DECLS

#endif

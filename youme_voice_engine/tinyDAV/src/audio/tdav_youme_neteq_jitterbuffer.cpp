//
//  tdav_youme_neteq_jitterbuffer.cpp
//  youme_voice_engine
//
//  Created by peter on 6/27/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "webrtc/modules/audio_coding/neteq/include/neteq.h"
#include "webrtc/common_types.h"
#include "webrtc/common_audio/resampler/include/push_resampler.h"
#include "tinyrtp/rtp/trtp_rtp_header.h"
#include "tinymedia/tmedia_defaults.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include "tinymedia/tmedia_codec.h"
#include <map>
#include <math.h>
#include "stdinc.h"

#include "version.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "tinydav/codecs/mixer/speex_resampler.h"
#include "tinydav/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.h"
#include "tinydav/audio/tdav_youme_neteq_jitterbuffer.h"
#include "webrtc/common_audio/ring_buffer.h" // Use ring buffer open source function
#include "tinydav/codecs/h264/tdav_codec_h264.h"
#include "tdav_audio_unit.h"
#include "tmedia_utils.h"
#include "NgnVideoManager.hpp"
#include "YouMeEngineManagerForQiniu.hpp"
#include "XConfigCWrapper.hpp"

#include "webrtc/modules/utility/include/audio_frame_operations.h"

#if FFMPEG_SUPPORT
#include "YMVideoRecorderManager.hpp"
#endif

#include "YMTranscriberManager.hpp"

using namespace youme;

enum PcmCallbackFlag
{
    Pcbf_remote = 0x1,
    Pcbf_record = 0x2,
    Pcbf_mix = 0x4,
};

typedef struct packet_stat_s
{
    uint32_t packet_count;
    uint32_t total_time_ms;
    uint32_t max_packet_gap_ms;
    uint32_t gap_40ms_count;
    uint32_t gap_100ms_count;
    uint32_t duplicate_count;
    uint32_t disorder_count;
    uint32_t max_disorder;
    uint32_t loss_count;
    uint32_t max_loss;
} packet_stat_t;

typedef struct jb_manager_s
{
    webrtc::NetEq *neteq;
    uint64_t last_pkt_ts;      // Timestamp when receiving the last packet.
    uint32_t rtp_ts_mult;
    bool     first_packet;
    uint32_t last_rtp_ts_ori;
    uint32_t last_rtp_ts;
    uint32_t last_recv_ts_ori;
    uint32_t last_recv_ts;
    int32_t  session_id;
    BandwidthCtrlMsg_t bc_received; // the last feedback received from the far end
    uint64_t           last_time_send_bc;
    
    packet_stat_t prev_stat;
    packet_stat_t cur_stat;
    uint32_t stat_first_seq_num;
    uint32_t stat_last_seq_num;
    uint32_t stat_total_time_ms_since_last_log;
    uint32_t stat_network_delay_ms;
    tsk_bool_t stat_available;
} jb_manager_t;
typedef std::map<int32_t, jb_manager_t*>  jb_manager_map_t;

typedef struct bkaud_manager_s
{
    tsk_list_t             *bkaud_free_lst;
    tsk_list_t             *bkaud_filled_lst;
    tsk_semaphore_handle_t *bkaud_sema_lst;
#ifdef USE_WEBRTC_RESAMPLE
    youme::webrtc::PushResampler<int16_t> *bkaud_resampler;
#else
    SpeexResamplerState    *bkaud_resampler;
#endif
    tsk_mutex_handle_t     *bkaud_resampler_mutex;
    tsk_list_item_t        *bkaud_lstitem_backup;
    
    uint32_t               bkaud_filled_postition;
    uint32_t               bkaudrate;
    uint32_t               bkaud_vol;
    int32_t                bkaud_last_vol;
    
    void                   *bkaud_tempbuf1;
    void                   *bkaud_tempbuf2;
} bkaud_manager_t;

typedef struct micaud_manager_s
{
    RingBuffer             *micaud_ringbuf;
    tsk_mutex_handle_t     *micaud_ringbuf_mutex;
    uint32_t               micaudrate;
    
    void                   *micaud_tempbuf1;    //mic送过来的原始数据(包括外部输入)
    void                   *micaud_tempbuf2;    //重采样为播放采样率的数据
    
#ifdef USE_WEBRTC_RESAMPLE
    youme::webrtc::PushResampler<int16_t> *micaud_resampler;
#else
    SpeexResamplerState    *micaud_resampler;
#endif
    tsk_mutex_handle_t     *micaud_resampler_mutex;
    
    // Inidcate the usage of the current mic data: for monitoring, or for callback, or both
    tsk_bool_t is_for_callback_only;
} micaud_manager_t;

typedef  struct transcriber_manager_s
{
#ifdef USE_WEBRTC_RESAMPLE
    youme::webrtc::PushResampler<int16_t> *record_resampler;
    youme::webrtc::PushResampler<int16_t> *remote_resampler;
#else
    SpeexResamplerState    *record_resampler;
    SpeexResamplerState    *remote_resampler;
#endif
    
    tsk_mutex_handle_t     *record_resampler_mutex;
    tsk_mutex_handle_t     *remote_resampler_mutex;
    
    void                *voice_tempbuf_record;
    void                *voice_tempbuf_remote;
}transcriber_manager_t;

typedef struct voice_manager_s
{
#ifdef USE_WEBRTC_RESAMPLE
    youme::webrtc::PushResampler<int16_t> *voice_resampler;
    youme::webrtc::PushResampler<int16_t> *voice_resampler_pcmcb;
    //外部接口设置下来的重采样器
    youme::webrtc::PushResampler<int16_t> *voice_resampler_pcmcbbyuser;     //remote
    youme::webrtc::PushResampler<int16_t> *voice_resampler_pcmcb_record;    //record
    youme::webrtc::PushResampler<int16_t> *voice_resampler_pcmcb_mix;       //mix
#else
    SpeexResamplerState *voice_resampler;
    SpeexResamplerState *voice_resampler_pcmcb;
    SpeexResamplerState *voice_resampler_pcmcbbyuser;
    SpeexResamplerState *voice_resampler_pcmcb_record;
    SpeexResamplerState *voice_resampler_pcmcb_mix;
#endif
    tsk_mutex_handle_t  *voice_resampler_pcmcb_mutex;
    tsk_mutex_handle_t  *voice_resampler_pcmcbbyuser_mutex;
    tsk_mutex_handle_t  *voice_resampler_pcmcb_record_mutex;
    tsk_mutex_handle_t  *voice_resampler_pcmcb_mix_mutex;
    void                *voice_tempbuf1;
    void                *voice_tempbuf2;
    void                *voice_tempbuf3;        // input and remote mix data
} voice_manager_t;

/** youme JitterBuffer*/
typedef struct tdav_youme_jitterBuffer_s
{
    TMEDIA_DECLARE_JITTER_BUFFER;
    
    uint32_t frame_duration;
    uint32_t in_rate;
    uint32_t out_rate;
    uint32_t mixed_callback_rate;
    uint32_t channels;
    uint32_t frame_sample_num;
    jb_manager_map_t* jb_mgr_map;
    AudioMixMeta     *mix_meta;
    int32_t          mix_meta_num;
    uint32_t         max_jb_num;

    //背景声音(BKAUD_SOURCE_GAME)，不是背景音乐bgm
    bkaud_manager_t     *bkaud_magr_t;
    //打开了耳返(BKAUD_SOURCE_MICBYPASS)
    micaud_manager_t    *micaud_magr_t;
    //设置了pcm callback(BKAUD_SOURCE_PCMCALLBACK)
    micaud_manager_t    *micaud_callback_magr_t;
    voice_manager_t     *voice_magr_t;
    
    transcriber_manager_s * transcriber_mgr_t;
    
    PcmCallback_t pcm_callback;
    tsk_bool_t isRemoteCallbackOn;  // 远端数据是否回调
    tsk_bool_t isRecordCallbackOn;  // 录音数据是否回调
    tsk_bool_t isMixCallbackOn;     // 远端和录音的混音是否回调
    
    int32_t nPCMCallbackChannel;
    int32_t nPCMCallbackSampleRate;

    tsk_boolean_t isSaveScreen;
    
    tsk_bool_t isTranscriberOn;
    
    uint64_t  last_time_calc_recv_bc; // last time calculate the received bandwidth control data and output to the codec
    
    // for debug only
    uint32_t  packet_count;
    
    // for statistics
    uint32_t stat_log_period_ms;
    uint32_t stat_calc_period_ms;
    
    tsk_bool_t running;
    tsk_thread_handle_t *thread_handle;
    tsk_list_t *decoder_ouput;
    tsk_list_t *audio_ouput;
    
    tsk_list_t *audio_resample_list;
    
    struct
    {
        int32_t max_level;
        voiceLevelCb_t callback;
        int32_t last_level;
        int32_t count;
    } farend_voice_level;

    struct
    {
        uint32_t max_size;
        FILE* remote_cb;
        uint32_t remote_cb_size;
        FILE* mix_cb;
        uint32_t mix_cb_size;
        FILE* file_record_cb;
        uint32_t record_cb_size;
    } pcm_file;
} tdav_youme_neteq_jitterbuffer_t;

#define TDAV_JITTERBUFFER_PCM_FILE_REMOTE_CB        1
#define TDAV_JITTERBUFFER_PCM_FILE_MIX_CB           2
#define TDAV_JITTERBUFFER_PCM_FILE_RECORD_CB        3

typedef enum JitterBufferBackgroundMusicMode {
    JITTERBUF_BGM_TO_SPEAKER = 0,  ///< 背景音乐混合到输出语音，并通过扬声器播出
    JITTERBUF_BGM_TO_MIC = 1,  ///<　背景音乐混合到麦克风语音，并一起发送给接收端
    JITTERBUF_BGM_TO_MIC_AND_BYPASS_TO_SPEAKER = 2  ///< 背景音乐混合到麦克风语音，并一起发送给接收端，同时输出到扬声器播出
} JitterBufferBackgroundMusicMode_t;

// Max number of sessions allowed to exist at the same time.
// This used to protect from the case that some clients join in and exit from the conversation frequently in very short time.
// Consider the CPU load, 16 is actually a quite large number.
//#define MAX_SESSION_NUM          (16)
// If no data from a client for more than this time, we will remove it from the jb_manager map.
// If it sends data again later, a new map will be created.
#define SESSION_EXPIRE_TIME_MS   (30*1000) //30 seconds

// If no data from a client for more than this time, and MaxJiterBufferNum is reached,
// its jitter buffer can be removed, can a new jitter buffer can be created for a new session.
#define SESSION_NO_DATA_TIME_MS  (2*1000) // 2 seconds

//
// Internal function prototypes.
//
static void calc_received_bandwidth_ctrl_data (tdav_youme_neteq_jitterbuffer_t *jb, BandwidthCtrlMsg_t *pMsg);
static void handle_bandwidth_ctrl_data(tdav_youme_neteq_jitterbuffer_t *jb, jb_manager_t* jb_mgr, trtp_rtp_header_t *rtp_hdr);
static bool check_and_remove_expired_session(tdav_youme_neteq_jitterbuffer_t *jb);
static int  create_neteq_jitterbuffer_for_new_session (tdav_youme_neteq_jitterbuffer_t *jb, int32_t session_id);
static void free_jb_manager(jb_manager_t **jb_mgr);
static void dump_jb_manager_map(tdav_youme_neteq_jitterbuffer_t *jb);
static int tdav_youme_neteq_jitterbuffer_mix_all_remote_pcm_data( tdav_youme_neteq_jitterbuffer_t *jb, tsk_size_t out_size , int nNeteqJbNum, tsk_size_t *voice_data_size);


//向录写管理器输入音频数据
static int tdav_youme_neteq_jitterbuffer_input_audio_pcm_to_transcriber( tdav_youme_neteq_jitterbuffer_t *jb, void *audio_data , tsk_size_t audio_size, int samplerate , int sessionId );
static void notify_pcm_data( tdav_youme_neteq_jitterbuffer_t *jb, tdav_session_audio_frame_buffer_t* frame_buf_cb, int flag  );

static int tdav_youme_neteq_jitterbuffer_resample( uint32_t& inSample, uint32_t& outSample,
                                                  void* resample,  tsk_mutex_handle_t*  resample_mutex, void* fromBuff, void* toBuff, tsk_size_t outBufSize );

#define MICAUD_BUFFER_SZ (1024*10)

static void tdav_youme_neteq_jitterbuffer_open_pcm_files(tdav_youme_neteq_jitterbuffer_t *jb, int type)
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
        case TDAV_JITTERBUFFER_PCM_FILE_REMOTE_CB:
            strncat(dumpPcmPath, "/dump_remote_cb.pcm", sizeof(dumpPcmPath) - baseLen);
            if (jb->pcm_file.remote_cb) {
                fclose(jb->pcm_file.remote_cb);
            }
            jb->pcm_file.remote_cb = fopen (dumpPcmPath, "wb");
            jb->pcm_file.remote_cb_size = 0;
            break;
        case TDAV_JITTERBUFFER_PCM_FILE_MIX_CB:
            strncat(dumpPcmPath, "/dump_mix_cb.pcm", sizeof(dumpPcmPath) - baseLen);
            if (jb->pcm_file.mix_cb) {
                fclose(jb->pcm_file.mix_cb);
            }
            jb->pcm_file.mix_cb = fopen (dumpPcmPath, "wb");
            jb->pcm_file.mix_cb_size = 0;
            break;
        case TDAV_JITTERBUFFER_PCM_FILE_RECORD_CB:
            strncat(dumpPcmPath, "/dump_record_cb.pcm", sizeof(dumpPcmPath) - baseLen);
            if (jb->pcm_file.file_record_cb) {
                fclose(jb->pcm_file.file_record_cb);
            }
            jb->pcm_file.file_record_cb = fopen (dumpPcmPath, "wb");
            jb->pcm_file.record_cb_size = 0;
            break;
        default:
            return;
    }
}

static void tdav_youme_neteq_jitterbuffer_close_pcm_files(tdav_youme_neteq_jitterbuffer_t *jb)
{
    if (jb->pcm_file.remote_cb) {
        fclose (jb->pcm_file.remote_cb);
        jb->pcm_file.remote_cb = NULL;
        jb->pcm_file.remote_cb_size = 0;
    }
    if (jb->pcm_file.mix_cb) {
        fclose (jb->pcm_file.mix_cb);
        jb->pcm_file.mix_cb = NULL;
        jb->pcm_file.mix_cb_size = 0;
    }
    if (jb->pcm_file.file_record_cb) {
        fclose (jb->pcm_file.file_record_cb);
        jb->pcm_file.file_record_cb = NULL;
        jb->pcm_file.record_cb_size = 0;
    }
}

static int tdav_youme_neteq_jitterbuffer_set_param (tmedia_jitterbuffer_t *self, const tmedia_param_t *param)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    
    if (!self || !param) {
        TSK_DEBUG_ERROR ("JitterBuffer set error!");
        return -1;
    }
    
    if (param->plugin_type == tmedia_ppt_session) {
        if (param->value_type == tmedia_pvt_int32) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_ENABLED)) {
                int32_t i4Paras  = TSK_TO_INT32 ((uint8_t *)param->value);
                uint8_t u1PathId = (i4Paras >> 8) & 0xff;
                if (u1PathId == JITTERBUF_BGM_TO_SPEAKER) {
                } else if (u1PathId == JITTERBUF_BGM_TO_MIC_AND_BYPASS_TO_SPEAKER) {
                }
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME)) {
                jb->bkaud_magr_t->bkaud_vol = (TSK_TO_INT32 ((uint8_t *)param->value)) & 0xff;
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_MAX_FAREND_VOICE_LEVEL)) {
                jb->farend_voice_level.max_level = TSK_TO_INT32 ((uint8_t *)param->value);
                TSK_DEBUG_INFO("Set farend voice max level=%d", jb->farend_voice_level.max_level);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_FLAG))
            {
                int flag = *((int32_t *)param->value);
                jb->isRemoteCallbackOn = (tsk_bool_t)( (flag & 0x1) != 0 );
                jb->isRecordCallbackOn = (tsk_bool_t)( (flag & 0x2) != 0 );
                jb->isMixCallbackOn = (tsk_bool_t)( (flag & 0x4) != 0 );
                TSK_DEBUG_INFO("Set pcmCallback flag:%d, remote:%d, record:%d, mix:%d", flag,
                               jb->isRemoteCallbackOn,jb->isRecordCallbackOn ,jb->isMixCallbackOn );
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_CHANNEL))
            {
                int nPCMCallbackChannel = *((int32_t *)param->value);
                jb->nPCMCallbackChannel = nPCMCallbackChannel;
                TSK_DEBUG_INFO("Set pcm callback Channel:%d", nPCMCallbackChannel);
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK_SAMPLE_RATE))
            {
                int nPCMCallbackSampleRate = *((int32_t *)param->value);
                jb->nPCMCallbackSampleRate = nPCMCallbackSampleRate;
                TSK_DEBUG_INFO("Set pcm callback Sample Rate:%d", nPCMCallbackSampleRate);
            }
            else if(tsk_striequals (param->key, TMEDIA_PARAM_KEY_SAVE_SCREEN)) {
                jb->isSaveScreen = (*((int32_t *)param->value)) != 0 ;
                TSK_DEBUG_INFO("Set SaveScreen :%d", jb->isSaveScreen);
            }
            else if( tsk_striequals( param->key, TMEDIA_PARAM_KEY_TRANSCRIBER_ENABLED ))
            {
                jb->isTranscriberOn = (*((int32_t *)param->value)) != 0 ;
                TSK_DEBUG_INFO("Set transcriber :%d", jb->isTranscriberOn);
            }
        }  else if (param->value_type == tmedia_pvt_pvoid) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PCM_CALLBACK)) {
                jb->pcm_callback = (PcmCallback_t)param->value;
                TSK_DEBUG_INFO("set pcmCallback:%p", jb->pcm_callback);
            } else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_FAREND_VOICE_LEVEL_CALLBACK)) {
                jb->farend_voice_level.callback = (voiceLevelCb_t)param->value;
                TSK_DEBUG_INFO("Set farend voice level cb=0x%x", param->value);
            }
            
        }
    }
    return 0;
}

static int tdav_youme_neteq_jitterbuffer_get_param (tmedia_jitterbuffer_t *self, tmedia_param_t *param)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    
    if (!self || !param) {
        TSK_DEBUG_ERROR ("JitterBuffer set error!");
        return -1;
    }
    
    if (param->value_type == tmedia_pvt_pobject) {
        if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_PACKET_STAT)) {
            tdav_session_av_stat_info_t *stat_info = (tdav_session_av_stat_info_t *)tsk_object_new(tdav_session_av_stat_info_def_t, (uint32_t) jb->jb_mgr_map->size());
            if (stat_info) {
                uint64_t curTime = tsk_gettimeofday_ms();
                jb_manager_map_t::iterator it;
                for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
                    jb_manager_t* jb_mgr = it->second;
                    if (jb_mgr) {
                        // If there's packet arriving in 1 second, we can consider this session as active
                        if (jb_mgr->stat_available && (curTime - jb_mgr->last_pkt_ts) < 1000) {
                            stat_info->statItems[stat_info->numOfItems].sessionId = jb_mgr->session_id;
                            stat_info->statItems[stat_info->numOfItems].packetLossRate10000th =
                                (jb_mgr->prev_stat.loss_count * 10000) / (jb_mgr->prev_stat.packet_count + jb_mgr->prev_stat.loss_count);
                            stat_info->statItems[stat_info->numOfItems].avgPacketTimeMs = jb_mgr->prev_stat.total_time_ms / jb_mgr->prev_stat.packet_count;
                            stat_info->statItems[stat_info->numOfItems].networkDelayMs = jb_mgr->stat_network_delay_ms;
                            stat_info->numOfItems++;
                        }
                    }
                }
                if (stat_info->numOfItems > 0) {
                    *(tdav_session_av_stat_info_t**)param->value = stat_info;
                } else {
                    TSK_OBJECT_SAFE_FREE(stat_info);
                    *(tdav_session_av_stat_info_t**)param->value = tsk_null;
                }
                return 0;
            }
        }
    }
    
    return -1;
}

static void dump_jb_manager_map(tdav_youme_neteq_jitterbuffer_t *jb)
{
    jb_manager_map_t::iterator it;
    uint64_t cur_time = tsk_gettimeofday_ms();
    TSK_DEBUG_INFO("Dump jb managers:");
    for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
        jb_manager_t* jb_mgr = (jb_manager_t*)it->second;
        TSK_DEBUG_INFO("session:%d, timediff:%u", jb_mgr->session_id, (int)(cur_time - jb_mgr->last_pkt_ts));
    }
}


//
// Calculate a proper bandwidth control data from all clients' feed back statistics in a past period of time.
//
static void calc_received_bandwidth_ctrl_data (tdav_youme_neteq_jitterbuffer_t *jb, BandwidthCtrlMsg_t *pMsg)
{
    pMsg->msgType = BWCtrlMsgClientReport;
    tdav_codec_bandwidth_ctrl_clear_msg(pMsg);
    pMsg->msg.client.packetLossRateQ8 = 0;

    jb_manager_map_t::iterator it;
    for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
        if (it->second) {
            jb_manager_t *jb_mgr = it->second;
            if (jb_mgr->bc_received.isValid
                && (BWCtrlMsgClientReport == jb_mgr->bc_received.msgType)) {
                if (jb_mgr->bc_received.msg.client.packetLossRateQ8 > pMsg->msg.client.packetLossRateQ8) {
                    pMsg->msg.client.packetLossRateQ8 = jb_mgr->bc_received.msg.client.packetLossRateQ8;
                }
                jb_mgr->bc_received.isValid = 0;
                pMsg->isValid = 1;
            }
        }
    }
}

//
// Handle received bandwidth control data, and prepare statistics data for bandwidth control purpose.
//
static void handle_bandwidth_ctrl_data(tdav_youme_neteq_jitterbuffer_t *jb, jb_manager_t* jb_mgr, trtp_rtp_header_t *rtp_hdr)
{
    if (!jb || !jb_mgr || !rtp_hdr) {
        return;
    }

    uint64_t current_time = rtp_hdr->receive_timestamp_ms;
    int feedback_period = tmedia_defaults_get_rtp_feedback_period();

    // Store the received bandwidth control data feed back from the clients, and calculate a proper
    // control data to instructing the encoder (for controlling redundancy, bandwidth, etc.)
    if (rtp_hdr->bc_received.isValid
        && (BWCtrlMsgClientReport == rtp_hdr->bc_received.msgType)
        && (rtp_hdr->bc_received.msg.client.forSessionId == rtp_hdr->my_session_id)) {
        
        jb_mgr->bc_received = rtp_hdr->bc_received;
        rtp_hdr->bc_received.isValid = 0; // set it to be invalid since we already take it
        // If it's the first received bandwidth control data, get the time
        if (0 == jb->last_time_calc_recv_bc) {
            TSK_DEBUG_INFO("First got bc data, from session:%d, for session:%d, loss_rate:%d.%d%%",
                           jb_mgr->session_id, jb_mgr->bc_received.msg.client.forSessionId,
                           jb_mgr->bc_received.msg.client.packetLossRateQ8 * 100 / 255,
                           (jb_mgr->bc_received.msg.client.packetLossRateQ8 * 10000 / 255) % 100);
            jb->last_time_calc_recv_bc = current_time;
        }
        //TSK_DEBUG_INFO("Got bc data, from session:%d, for session:%d, loss_rate:%u",
        //               jb_mgr->session_id, jb_mgr->bc_received.msg.client.forSessionId, jb_mgr->bc_received.msg.client.packetLossRateQ8);

        // If time's up, calculate the received statistics to get a proper bandwith control data to instructing the encoder.
        // The calculation period is twice as the feedback period, so that we can accumulate enough data from diffrent clients.
        if ((jb->last_time_calc_recv_bc > 0)
            && (feedback_period > 0)
            && ((current_time - jb->last_time_calc_recv_bc) >= (feedback_period * 2))
            ) {
            jb->last_time_calc_recv_bc = current_time;
            calc_received_bandwidth_ctrl_data(jb, &(rtp_hdr->bc_received));
        }
    } else if (rtp_hdr->bc_received.isValid
               && (BWCtrlMsgServerCommand == rtp_hdr->bc_received.msgType)) {
        // apply the server command that eligible for the jitter buffer and pass it on (do NOT set it to be invalid) for further handling.
    } else {
        rtp_hdr->bc_received.isValid = 0;
    }
    
    // Prepare the statistics for the client who is sending me data.
    if ((feedback_period > 0)
        && ((current_time - jb_mgr->last_time_send_bc) >= feedback_period) ) {
        
        webrtc::RtcpStatistics stats;
        jb_mgr->neteq->GetRtcpStatistics(&stats);
        jb_mgr->last_time_send_bc = current_time;
        
        rtp_hdr->bc_to_send.msgType = BWCtrlMsgClientReport;
        tdav_codec_bandwidth_ctrl_clear_msg(&(rtp_hdr->bc_to_send));
        rtp_hdr->bc_to_send.isValid = 1;
        rtp_hdr->bc_to_send.msg.client.forSessionId = jb_mgr->session_id;
        rtp_hdr->bc_to_send.msg.client.packetLossRateQ8 = (int32_t)stats.fraction_lost;
        //TSK_DEBUG_INFO("Preapared for session:%d, my loss rate:%d", jb_mgr->session_id, rtp_hdr->bc_to_send.msg.client.packetLossRateQ8);
    } else {
        rtp_hdr->bc_to_send.isValid = 0;
    }
}

//
// Free resources for a jb manager.
//
static void free_jb_manager(jb_manager_t **jb_mgr)
{
    if (jb_mgr && (*jb_mgr)) {
        TSK_DEBUG_INFO("Removing jb manager for session:%d", (*jb_mgr)->session_id);
        if (NULL != (*jb_mgr)->neteq) {
            delete (*jb_mgr)->neteq;
            (*jb_mgr)->neteq = NULL;
        }
        delete (*jb_mgr);
        (*jb_mgr) = NULL;
    }
}

//
// This function is intended to be called before creating a new session.
//
static bool check_and_remove_expired_session(tdav_youme_neteq_jitterbuffer_t *jb, uint64_t currentTime)
{
    jb_manager_map_t::iterator it;
    jb_manager_map_t::iterator *pItToErase = new(std::nothrow) jb_manager_map_t::iterator[jb->max_jb_num];
    if (NULL == pItToErase) {
        return false;
    }
    
    bool ret = true;
    uint32_t sessionToEraseNum = 0;
    jb_manager_map_t::iterator itLeastUsed = jb->jb_mgr_map->end();
    uint64_t timeLeastUsed = 0;
    
    for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
        jb_manager_t *jb_mgr = it->second;
        if (jb_mgr) {
            uint64_t timeDiff = currentTime - jb_mgr->last_pkt_ts;
            if (timeDiff > SESSION_EXPIRE_TIME_MS) {
                pItToErase[sessionToEraseNum] = it;
                sessionToEraseNum++;
            } else if (timeDiff > timeLeastUsed) {
                itLeastUsed = it;
                timeLeastUsed = timeDiff;
            }
        }
        // protecting from out of bound
        if (sessionToEraseNum >= jb->max_jb_num) {
            break;
        }
    }
    
    if (sessionToEraseNum > 0) {
        // Erase all the expired sessions.
        for (int32_t i = 0; i < sessionToEraseNum; i++) {
            free_jb_manager(&(pItToErase[i]->second));
            jb->jb_mgr_map->erase(pItToErase[i]);
        }
    } else if (jb->jb_mgr_map->size() >= jb->max_jb_num) {
        // Max session number reached, erase the least recently used one if it's not sending data
        // in time of SESSION_NO_DATA_TIME_MS.
        if ((timeLeastUsed >= SESSION_NO_DATA_TIME_MS) && (itLeastUsed != jb->jb_mgr_map->end())) {
            free_jb_manager(&(itLeastUsed->second));
            jb->jb_mgr_map->erase(itLeastUsed);
        } else {
            // Max jitter buffer number is reached and there's no inactive session, we should
            // refuse the new session to join in.
            // Do not print for this error. Otherwise, it may print for every packet.
            ret = false;
        }
    }
    
    delete[] pItToErase;
    pItToErase = NULL;
    return ret;
}

static int create_neteq_jitterbuffer_for_new_session (tdav_youme_neteq_jitterbuffer_t *jb, int32_t session_id, uint64_t curTimeMs)
{
    // Removed expired sessions and make sure the session number doesn't exceeds the limit.
    if (check_and_remove_expired_session(jb, curTimeMs) == false) {
        // Do not print for this error. Otherwise, it may print for every packet.
        return -1;
    }
    
    //////////////////////////////////////////////////////////////////////////
    std::pair< jb_manager_map_t::iterator, bool > insertRet;
    webrtc::NetEq::Config config;
    AudioMixMeta *new_mix_meta = NULL;
    jb_manager_t *jb_mgr = NULL;
    size_t session_num = jb->jb_mgr_map->size();

    /////////////////////////////////////////////////////////////////////////
    // Expand the mixer metadata array
    if (jb->mix_meta_num < (session_num + 1)) {
        new_mix_meta = new AudioMixMeta[session_num + 1];
        if (NULL == new_mix_meta) {
            goto bail;
        }
        // copy the old data and create a new pcm buffer
        if (NULL != jb->mix_meta) {
            memcpy(new_mix_meta, jb->mix_meta, sizeof(AudioMixMeta) * session_num);
        }
        new_mix_meta[session_num].bufferPtr = new int16_t[jb->frame_sample_num];
        if (NULL == new_mix_meta[session_num].bufferPtr) {
            goto bail;
        }
        // replace the the mixer metadata array with the new one
        delete[] jb->mix_meta;
        jb->mix_meta     = new_mix_meta;
        jb->mix_meta_num = session_num + 1;
        new_mix_meta     = NULL;
    }

    /////////////////////////////////////////////////////////////////////////
    // Create a new jitter buffer manager
    jb_mgr = new jb_manager_t;
    if (NULL == jb_mgr) {
        goto bail;
    }
    memset(jb_mgr, 0, sizeof(jb_manager_t));
    
    config.sample_rate_hz             = tmedia_defaults_get_record_sample_rate();
    config.enable_audio_classifier    = false;
    config.enable_post_decode_vad     = false;
    config.max_delay_ms               = tmedia_defaults_get_max_neteq_delay_ms();
    config.max_packets_in_buffer      = config.max_delay_ms / jb->frame_duration;
    config.background_noise_mode      = webrtc::NetEq::kBgnOff;
    //config.playout_mode               = webrtc::kPlayoutOn;
    config.playout_mode               = webrtc::kPlayoutGameVoice;
    config.enable_fast_accelerate     = false;

    jb_mgr->neteq = webrtc::NetEq::Create(config);
    if (NULL == jb_mgr->neteq) {
        goto bail;
    }
    jb_mgr->last_pkt_ts      = curTimeMs;
    jb_mgr->rtp_ts_mult      = 48000 / tmedia_defaults_get_record_sample_rate();
    jb_mgr->first_packet     = true;
    jb_mgr->session_id       = session_id;

    jb_mgr->bc_received.msgType = BWCtrlMsgClientReport;
    tdav_codec_bandwidth_ctrl_clear_msg(&(jb_mgr->bc_received));
    jb_mgr->last_time_send_bc = curTimeMs;
    jb_mgr->neteq->SetMinimumDelay(Config_GetUInt("MIN_NETEQ_DELAY_MS", 100));
	insertRet = jb->jb_mgr_map->insert(jb_manager_map_t::value_type(session_id, jb_mgr));
    if (!insertRet.second) {
        goto bail;
    }
    
    // NetEq initializaton
    jb_mgr->neteq->RegisterPayloadType(webrtc::NetEqDecoder::kDecoderOpus, atoi(TMEDIA_CODEC_FORMAT_OPUS), tmedia_defaults_get_record_sample_rate());
    TSK_DEBUG_INFO ("== Created a new neteq jb for session:%d", session_id);
    dump_jb_manager_map(jb);

    return 0;
bail:
    TSK_DEBUG_ERROR ("Failed to create a new neteq jitter buffer for session:%d", session_id);
    if (NULL != jb_mgr) {
        if (NULL != jb_mgr->neteq) {
            delete jb_mgr->neteq;
            jb_mgr->neteq = NULL;
        }
        delete jb_mgr;
        jb_mgr = NULL;
    }
    
    if (NULL != new_mix_meta) {
        if (NULL != new_mix_meta[session_num].bufferPtr) {
            delete[] new_mix_meta[session_num].bufferPtr;
            new_mix_meta[session_num].bufferPtr = NULL;
        }
        delete[] new_mix_meta;
        new_mix_meta = NULL;
    }
    return -1;
}

static void* TSK_STDCALL tdav_neteq_thread_func(void *arg) {
    tdav_youme_neteq_jitterbuffer_t *self = (tdav_youme_neteq_jitterbuffer_t *)arg;
    TSK_DEBUG_INFO("neteq thread enters");
    
    while(1) {
        if(!self->running) {
            break;
        } else {
            //没有取到包
            //TSK_DEBUG_INFO("neteq thread is running");
            tsk_list_item_t *item = tsk_null;
            //发送端直接从 list 读数据
            tsk_list_lock(self->decoder_ouput);
            item = tsk_list_pop_first_item(self->decoder_ouput);
            tsk_list_unlock(self->decoder_ouput);
            
            if(item) {
                tdav_audio_unit_t* audio_unit = TDAV_AUDIO_UNIT(item->data);
                //TMEDIA_DUMP_TO_FILE("/pcm_data.bin", audio_unit->data, audio_unit->size);
                std::string userId = CVideoChannelManager::getInstance()->getUserId(audio_unit->sessionid);
                YouMeEngineManagerForQiniu::getInstance()->onAudioFrameCallback(userId, audio_unit->data, audio_unit->size, audio_unit->timestamp);
                TSK_OBJECT_SAFE_FREE(item);
            } else {
                tsk_thread_sleep(20);
            }
        }
    }
    
    TSK_DEBUG_INFO("neteq thread exits");
    return tsk_null;
}

#ifdef USE_WEBRTC_RESAMPLE
void tdav_youme_neteq_jitterbuffer_init_resampler( youme::webrtc::PushResampler<int16_t> ** presampler, int channels, int in_rate, int out_rate )
{
    *presampler = new youme::webrtc::PushResampler<int16_t>();
    (*presampler)->InitializeIfNeeded(in_rate, out_rate, channels);
}
#else
void tdav_youme_neteq_jitterbuffer_init_resampler( SpeexResamplerState ** presampler, int channels, int in_rate, int out_rate )
{
    *presampler = speex_resampler_init(channels, in_rate, out_rate, 3, NULL);
}
#endif

#ifdef USE_WEBRTC_RESAMPLE
void tdav_youme_neteq_jitterbuffer_delete_resampler( youme::webrtc::PushResampler<int16_t> ** presampler )
{
    delete *presampler;
    *presampler = nullptr;
}
#else
void tdav_youme_neteq_jitterbuffer_delete_resampler( SpeexResamplerState ** presampler )
{
    speex_resampler_destroy( *presampler );
    *presampler = tsk_null;
}
#endif


static int tdav_youme_neteq_jitterbuffer_open (tmedia_jitterbuffer_t *self, uint32_t frame_duration, uint32_t in_rate, uint32_t out_rate, uint32_t channels)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    tsk_list_item_t *free_item;
    int LoopCnt;
    
    TSK_DEBUG_INFO ("Open youme neteq jb (ptime=%u, in_rate=%u, out_rate=%u)", frame_duration, in_rate, out_rate);
    
    /*
    if(!jb->thread_handle) {
        jb->running = tsk_true;
        int ret = tsk_thread_create(&jb->thread_handle, tdav_neteq_thread_func, jb);
        if((0 != ret) && (!jb->thread_handle)) {
            TSK_DEBUG_ERROR("Failed to create neteq thread");
            return -1;
        }
        ret = tsk_thread_set_priority(jb->thread_handle, TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }
     */
    
    jb->frame_duration       = frame_duration;
    jb->in_rate              = in_rate;
    jb->out_rate             = out_rate;
    jb->mixed_callback_rate  = tmedia_defaults_get_mixed_callback_samplerate();//tmedia_defaults_get_std_sample_rate(jb->nPCMCallbackSampleRate, 48000);
    jb->channels             = channels;
    jb->frame_sample_num     = frame_duration * in_rate * channels / 1000;
    jb->mix_meta             = NULL;
    jb->mix_meta_num         = 0;
    jb->jb_mgr_map->clear();
    jb->last_time_calc_recv_bc = 0; // 0 means there's no received bandwidth control data yet
    jb->voice_magr_t->voice_resampler = nullptr;
    jb->voice_magr_t->voice_resampler_pcmcb = nullptr;  
    jb->voice_magr_t->voice_resampler_pcmcbbyuser = nullptr;
    jb->voice_magr_t->voice_resampler_pcmcb_record = nullptr;
    jb->voice_magr_t->voice_resampler_pcmcb_mix = nullptr;
    jb->transcriber_mgr_t->record_resampler = nullptr;
    jb->transcriber_mgr_t->remote_resampler = nullptr;   
    jb->bkaud_magr_t->bkaud_resampler = nullptr;
    jb->packet_count         = 0;

    // initial resample handler of voice
#ifdef USE_WEBRTC_RESAMPLE
    jb->voice_magr_t->voice_resampler = new youme::webrtc::PushResampler<int16_t>();
    jb->voice_magr_t->voice_resampler->InitializeIfNeeded(in_rate, out_rate, channels);
    
    jb->voice_magr_t->voice_resampler_pcmcb = new youme::webrtc::PushResampler<int16_t>();
    jb->voice_magr_t->voice_resampler_pcmcb->InitializeIfNeeded(out_rate, jb->mixed_callback_rate, channels);
#else
    jb->voice_magr_t->voice_resampler  = speex_resampler_init(channels, in_rate, out_rate, 3, NULL);
    jb->voice_magr_t->voice_resampler_pcmcb = speex_resampler_init(channels, out_rate, jb->mixed_callback_rate, 3, NULL);
#endif
    
    tdav_youme_neteq_jitterbuffer_init_resampler( &(jb->transcriber_mgr_t->record_resampler), channels,  jb->out_rate, YMTranscriberManager::getInstance()->getSupportSampleRate()  );
    tdav_youme_neteq_jitterbuffer_init_resampler( &(jb->transcriber_mgr_t->remote_resampler), channels, jb->in_rate, YMTranscriberManager::getInstance()->getSupportSampleRate()  );
    jb->transcriber_mgr_t->record_resampler_mutex = tsk_mutex_create_2( tsk_false );
    jb->transcriber_mgr_t->remote_resampler_mutex = tsk_mutex_create_2( tsk_false );
    jb->transcriber_mgr_t->voice_tempbuf_record   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    jb->transcriber_mgr_t->voice_tempbuf_remote   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    
    jb->voice_magr_t->voice_resampler_pcmcb_mutex = tsk_mutex_create_2(tsk_false);
    jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex = tsk_mutex_create_2(tsk_false);
    jb->voice_magr_t->voice_resampler_pcmcb_record_mutex = tsk_mutex_create_2(tsk_false);
    jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex = tsk_mutex_create_2(tsk_false);

    jb->voice_magr_t->voice_tempbuf1   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    jb->voice_magr_t->voice_tempbuf2   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    jb->voice_magr_t->voice_tempbuf3   = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    memset(jb->voice_magr_t->voice_tempbuf1, 0, MAX_TEMP_BUF_BYTESZ);
    memset(jb->voice_magr_t->voice_tempbuf2, 0, MAX_TEMP_BUF_BYTESZ);
    memset(jb->voice_magr_t->voice_tempbuf3, 0, MAX_TEMP_BUF_BYTESZ);
    
    // initial background audio related items
    jb->bkaud_magr_t->bkaud_vol              = 100;
    jb->bkaud_magr_t->bkaud_last_vol         = 0;
    jb->bkaud_magr_t->bkaud_sema_lst         = tsk_semaphore_create_2(MAX_BKAUD_ITEM_NUM);
    jb->bkaud_magr_t->bkaud_free_lst         = tsk_list_create();
    jb->bkaud_magr_t->bkaud_filled_lst       = tsk_list_create();
    jb->bkaud_magr_t->bkaud_filled_postition = 0;
    jb->bkaud_magr_t->bkaud_lstitem_backup   = NULL;
    jb->bkaud_magr_t->bkaudrate              = 44100;
#ifdef USE_WEBRTC_RESAMPLE
    jb->bkaud_magr_t->bkaud_resampler = new youme::webrtc::PushResampler<int16_t>(); // fake resampler handler;
    jb->bkaud_magr_t->bkaud_resampler->InitializeIfNeeded(jb->bkaud_magr_t->bkaudrate, jb->out_rate, 1);
    
#else
    jb->bkaud_magr_t->bkaud_resampler        = speex_resampler_init(1, jb->bkaud_magr_t->bkaudrate, jb->out_rate, 3, NULL); // fake resampler handler;
#endif
    jb->bkaud_magr_t->bkaud_resampler_mutex  = tsk_mutex_create_2(tsk_false);
    jb->bkaud_magr_t->bkaud_tempbuf1         = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    jb->bkaud_magr_t->bkaud_tempbuf2         = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    memset(jb->bkaud_magr_t->bkaud_tempbuf1, 0, MAX_TEMP_BUF_BYTESZ);
    memset(jb->bkaud_magr_t->bkaud_tempbuf2, 0, MAX_TEMP_BUF_BYTESZ);
    
    for (LoopCnt = 0; LoopCnt < MAX_BKAUD_ITEM_NUM; LoopCnt++) {
        free_item = tsk_list_item_create();
        free_item->data = tsk_object_new (tdav_session_audio_frame_buffer_def_t, MAX_FRAME_SIZE);
        tsk_list_lock(jb->bkaud_magr_t->bkaud_free_lst);
        tsk_list_push_item(jb->bkaud_magr_t->bkaud_free_lst, &free_item, tsk_true);
        tsk_list_unlock(jb->bkaud_magr_t->bkaud_free_lst);
    }
    
    jb->micaud_magr_t->micaudrate            = 16000;
    jb->micaud_magr_t->micaud_ringbuf        = WebRtc_youme_CreateBuffer(MICAUD_BUFFER_SZ, sizeof(int16_t));
    jb->micaud_magr_t->micaud_ringbuf_mutex  = tsk_mutex_create_2(tsk_false);
    jb->micaud_magr_t->micaud_tempbuf1       = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    jb->micaud_magr_t->micaud_tempbuf2       = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    memset(jb->micaud_magr_t->micaud_tempbuf1, 0, MAX_TEMP_BUF_BYTESZ);
    memset(jb->micaud_magr_t->micaud_tempbuf2, 0, MAX_TEMP_BUF_BYTESZ);
    jb->micaud_magr_t->micaud_resampler = nullptr;
    
    jb->micaud_magr_t->is_for_callback_only  = tsk_false;
    
    jb->micaud_callback_magr_t->micaudrate            = 16000;
    jb->micaud_callback_magr_t->micaud_ringbuf        = WebRtc_youme_CreateBuffer(MICAUD_BUFFER_SZ, sizeof(int16_t));
    jb->micaud_callback_magr_t->micaud_ringbuf_mutex  = tsk_mutex_create_2(tsk_false);
    jb->micaud_callback_magr_t->micaud_tempbuf1       = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    memset(jb->micaud_callback_magr_t->micaud_tempbuf1, 0, MAX_TEMP_BUF_BYTESZ);
    jb->micaud_callback_magr_t->micaud_tempbuf2       = tsk_malloc(MAX_TEMP_BUF_BYTESZ);
    memset(jb->micaud_callback_magr_t->micaud_tempbuf2, 0, MAX_TEMP_BUF_BYTESZ);
    jb->micaud_callback_magr_t->micaud_resampler = nullptr;
    
    /* pcm dumpping files */
    jb->pcm_file.max_size = tmedia_defaults_get_pcm_file_size_kb() * 1024;
    jb->pcm_file.remote_cb = NULL;
    jb->pcm_file.remote_cb_size = 0;
    jb->pcm_file.mix_cb = NULL;
    jb->pcm_file.mix_cb_size = 0;
    jb->pcm_file.file_record_cb = NULL;
    jb->pcm_file.record_cb_size = 0;

    if (jb->pcm_file.max_size > 0)
    {
        TSK_DEBUG_INFO("Start jitterbuffer mix dumping pcm, max_size:%u", jb->pcm_file.max_size);
        
        tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_REMOTE_CB);
        tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_MIX_CB);
        tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_RECORD_CB);
    }
    return 0;
}

static int tdav_youme_neteq_jitterbuffer_tick (tmedia_jitterbuffer_t *self)
{
    // TSK_DEBUG_ERROR ("Not implemented");
    return 0;
}

static int tdav_youme_neteq_jitterbuffer_put_bkaudio (tmedia_jitterbuffer_t *self, void *data, tsk_object_t *proto_hdr)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    tdav_session_audio_frame_buffer_t* base = (tdav_session_audio_frame_buffer_t*)data;
    trtp_rtp_header_t* header = TRTP_RTP_HEADER (proto_hdr);
    
    tsk_list_item_t *free_item;
    size_t  item_count;
    int16_t i2SampleNumTotal, i2SampleNumPerCh;
    
    if (header->bkaud_source == BKAUD_SOURCE_GAME) {
        // ===== Pre-processing start ===== //
        i2SampleNumTotal = base->pcm_size / base->bytes_per_sample;
        i2SampleNumPerCh = i2SampleNumTotal / base->channel_num;
        int16_t *pi2Buf1 = new int16_t[i2SampleNumTotal];
        int16_t *pi2Buf2 = new int16_t[i2SampleNumTotal];
        // Float transfer
        tdav_codec_float_to_int16 (base->pcm_data, pi2Buf1, &(base->bytes_per_sample), &(base->pcm_size), base->is_float);
        
        // 2 channel & interleaved, downmix process
        tdav_codec_multichannel_interleaved_to_mono (pi2Buf1, pi2Buf2, &(base->channel_num), &(base->pcm_size), base->is_interleaved);
        
        // Volume here, use 50 samples to do fade if volume change(smooth gain)
        int32_t i4CurVol  = (int32_t)((int64_t)(jb->bkaud_magr_t->bkaud_vol) * 0x7fffffff / 100);
        tdav_codec_volume_smooth_gain (pi2Buf2, i2SampleNumPerCh, i2SampleNumPerCh, i4CurVol, &jb->bkaud_magr_t->bkaud_last_vol);
        memcpy(base->pcm_data, pi2Buf2,base->pcm_size);
        
        TSK_SAFE_FREE(pi2Buf1);
        TSK_SAFE_FREE(pi2Buf2);
        // ===== Pre-processing end ===== //
        
        if (base->channel_num == 1) { // Currently only support 1 channel processing
            if (!jb->bkaud_magr_t->bkaud_free_lst || !jb->bkaud_magr_t->bkaud_filled_lst || !jb->bkaud_magr_t->bkaud_sema_lst) {
                TSK_DEBUG_ERROR("Background audio related items ISNT be initialized!!");
                return -1;
            }
            
            if (0 != tsk_semaphore_decrement(jb->bkaud_magr_t->bkaud_sema_lst)) {
                TSK_DEBUG_ERROR("Fatal error: filled_frame_sema failed");
                return -1;
            }
                
            tsk_list_lock(jb->bkaud_magr_t->bkaud_free_lst);
            free_item = tsk_list_pop_first_item (jb->bkaud_magr_t->bkaud_free_lst);
            tsk_list_unlock(jb->bkaud_magr_t->bkaud_free_lst);
                
            if (free_item)
            {
                if (free_item->data)
                {
                    tdav_session_audio_frame_buffer_t* frame_buf = (tdav_session_audio_frame_buffer_t*)free_item->data;
                    base->channel_num = (base->channel_num == 0) ? 1 : base->channel_num;
                    memcpy(frame_buf->pcm_data, base->pcm_data, base->pcm_size);
                    frame_buf->bytes_per_sample = base->bytes_per_sample;
                    frame_buf->channel_num      = base->channel_num;
                    frame_buf->sample_rate      = base->sample_rate;
                    frame_buf->pcm_size         = base->pcm_size;
                    
                    if ((jb->bkaud_magr_t->bkaudrate != base->sample_rate) || (jb->bkaud_magr_t->bkaud_resampler != nullptr)) {
                        tsk_mutex_lock(jb->bkaud_magr_t->bkaud_resampler_mutex);
#ifdef USE_WEBRTC_RESAMPLE
                        jb->bkaud_magr_t->bkaud_resampler->InitializeIfNeeded(jb->bkaud_magr_t->bkaudrate, jb->out_rate, frame_buf->channel_num);
#else
                        speex_resampler_destroy(jb->bkaud_magr_t->bkaud_resampler);
                        jb->bkaud_magr_t->bkaud_resampler = speex_resampler_init(frame_buf->channel_num, jb->bkaud_magr_t->bkaudrate, jb->out_rate, 3, NULL);
#endif
                        tsk_mutex_unlock(jb->bkaud_magr_t->bkaud_resampler_mutex);
                    }
                    jb->bkaud_magr_t->bkaudrate = base->sample_rate;
                }
                tsk_list_lock(jb->bkaud_magr_t->bkaud_filled_lst);
                tsk_list_push_item (jb->bkaud_magr_t->bkaud_filled_lst, &free_item, tsk_true);
                tsk_list_unlock(jb->bkaud_magr_t->bkaud_filled_lst);
            }
        } else {
            // Implement here
            TSK_DEBUG_WARN("Currently ONLY support MONO background audio!");
        }
    } else if (header->bkaud_source == BKAUD_SOURCE_MICBYPASS) {
        //打开了mic耳返
        if (base->channel_num == 1) { // Currently only support 1 channel processing
            if (jb->micaud_magr_t->micaudrate != base->sample_rate) {
                jb->micaud_magr_t->micaudrate = base->sample_rate;
            }
            
            if (WebRtc_youme_available_write(jb->micaud_magr_t->micaud_ringbuf) >= (base->pcm_size / sizeof(int16_t))) {
                // Means can write buffer
                tsk_mutex_lock(jb->micaud_magr_t->micaud_ringbuf_mutex);
                WebRtc_youme_WriteBuffer(jb->micaud_magr_t->micaud_ringbuf, base->pcm_data, base->pcm_size / sizeof(int16_t));
                tsk_mutex_unlock(jb->micaud_magr_t->micaud_ringbuf_mutex);
            } else {
                TMEDIA_I_AM_ACTIVE(100, "Mic audio ring buffer is full, will drop audio!! available_write:%d, pcmSize:%d", WebRtc_youme_available_write(jb->micaud_magr_t->micaud_ringbuf), base->pcm_size / sizeof(int16_t));
            }
        }
    }
    else if(  header->bkaud_source == BKAUD_SOURCE_PCMCALLBACK )
    {
        //打开了pcm callback，record cb或者mix cb
        if (base->channel_num == 1) { // Currently only support 1 channel processing
            if (jb->micaud_callback_magr_t->micaudrate != base->sample_rate) {
                jb->micaud_callback_magr_t->micaudrate = base->sample_rate;
            }
            
            if (WebRtc_youme_available_write(jb->micaud_callback_magr_t->micaud_ringbuf) >= (base->pcm_size / sizeof(int16_t))) {
                // Means can write buffer
                tsk_mutex_lock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
                WebRtc_youme_WriteBuffer(jb->micaud_callback_magr_t->micaud_ringbuf, base->pcm_data, base->pcm_size / sizeof(int16_t));
                tsk_mutex_unlock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
            } else {
                TSK_DEBUG_WARN("Mic pcm callback audio ring buffer is full, will drop audio!! available_write:%d, pcmSize:%d", WebRtc_youme_available_write(jb->micaud_callback_magr_t->micaud_ringbuf), base->pcm_size / sizeof(int16_t));
            }
        }
    }
    
    if (header->bkaud_source == BKAUD_SOURCE_PCMCALLBACK) {
        jb->micaud_magr_t->is_for_callback_only = tsk_true;
    } else {
        jb->micaud_magr_t->is_for_callback_only = tsk_false;
    }

    return 0;
}

static int tdav_youme_neteq_jitterbuffer_put (tmedia_jitterbuffer_t *self, void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{

    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    trtp_rtp_header_t *rtp_hdr = NULL;
    
    if (!data || !data_size || !proto_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    rtp_hdr = TRTP_RTP_HEADER (proto_hdr);
    uint64_t cur_time_ms = rtp_hdr->receive_timestamp_ms;
	jb_manager_map_t::iterator it = jb->jb_mgr_map->find(rtp_hdr->session_id);
	if (it == jb->jb_mgr_map->end()) {
        if (create_neteq_jitterbuffer_for_new_session(jb, rtp_hdr->session_id, cur_time_ms) != 0) {
            // Do not print for this error. Otherwise, it may print for every packet.
            return -1;
        }
		it = jb->jb_mgr_map->find(rtp_hdr->session_id);
		if (it == jb->jb_mgr_map->end()) {
            TSK_DEBUG_ERROR ("impossible");
            return -1;
        }
    }
    
    jb_manager_t* jb_mgr = it->second;
    if (NULL == jb_mgr->neteq)
    {
        TSK_DEBUG_ERROR ("impossible");
        return -1;
    }
    
    webrtc::WebRtcRTPHeader webrtc_rtp_header;
    rtc::ArrayView<const uint8_t> payload((uint8_t*)data, (size_t)data_size);
    // Convert the clock rate for the timestamp.
    // NetEq assume all RTP timestamp clock rates same as the sampling frequency, but for opus,
    // the clock rate is always 48KHz per the spec. The original WebRtc NetEq works around this
    // by fixing the sampling frequency at 48KHz for the internal decoder. But it's not suitable
    // for our current architecture, so we make a conversion here.
    // TODO: check if it's better to always output at the sampling frequency 48KHz.
    if (jb_mgr->first_packet || (jb_mgr->rtp_ts_mult <= 1)) {
        jb_mgr->last_rtp_ts = rtp_hdr->timestamp;
    } else {
        int32_t diff = (int32_t)DIFF_RTP_TS(jb_mgr->last_rtp_ts_ori, rtp_hdr->timestamp);
        // If the difference exceedes 1 minute, there must be a reset from the sender
        if (abs(diff) > (60 * 48000)) {
            jb_mgr->last_rtp_ts = rtp_hdr->timestamp;
        } else {
            jb_mgr->last_rtp_ts += (uint32_t)(diff / (int32_t)jb_mgr->rtp_ts_mult);
        }
    }
    jb_mgr->last_rtp_ts_ori = rtp_hdr->timestamp;
    
    if (jb_mgr->first_packet || (jb_mgr->rtp_ts_mult <= 1)) {
        jb_mgr->last_recv_ts     = rtp_hdr->receive_timestamp;
    } else {
        int32_t diff = DIFF_RTP_TS(jb_mgr->last_recv_ts_ori, rtp_hdr->receive_timestamp);
        jb_mgr->last_recv_ts += (uint32_t)(diff / (int32_t)jb_mgr->rtp_ts_mult);
    }
    jb_mgr->last_recv_ts_ori = rtp_hdr->receive_timestamp;

    // RTPHeader has a constructor to clean the memory, no memset needed
    webrtc_rtp_header.header.markerBit                   = rtp_hdr->marker;
    webrtc_rtp_header.header.payloadType                 = rtp_hdr->payload_type;
    webrtc_rtp_header.header.sequenceNumber              = rtp_hdr->seq_num;
    webrtc_rtp_header.header.timestamp                   = jb_mgr->last_rtp_ts;
    webrtc_rtp_header.header.ssrc                        = rtp_hdr->ssrc;
    webrtc_rtp_header.header.headerLength                = 32 * 3; // The fixed RTP header without CSRC and extenstion
    webrtc_rtp_header.header.payload_type_frequency      = rtp_hdr->payload_type_frequency;
    webrtc_rtp_header.frameType                          = webrtc::kAudioFrameSpeech;
    webrtc_rtp_header.type.Audio.numEnergy               = 0;
    webrtc_rtp_header.type.Audio.isCNG                   = false;
    webrtc_rtp_header.type.Audio.channel                 = 1;
    memset(webrtc_rtp_header.type.Audio.arrOfEnergy, 0, sizeof(webrtc_rtp_header.type.Audio.arrOfEnergy));
    
    if (jb_mgr->neteq->InsertPacket(webrtc_rtp_header, payload, jb_mgr->last_recv_ts) != 0) {
        TSK_DEBUG_ERROR ("Failed to insert packet into NetEq");
        return -1;
    }
    
    if (rtp_hdr->networkDelayMs > 0) {
        jb_mgr->stat_network_delay_ms = rtp_hdr->networkDelayMs;
    }

    if (jb->stat_calc_period_ms <= 0) {
        // No statistics is needed. Do nothing for now.
    } else if (jb_mgr->first_packet) {
        jb_mgr->stat_first_seq_num = rtp_hdr->seq_num;
        jb_mgr->stat_last_seq_num = rtp_hdr->seq_num;
    } else {
        uint32_t packet_gap_ms = (uint32_t)(cur_time_ms - jb_mgr->last_pkt_ts);
        jb_mgr->cur_stat.packet_count++;
        jb_mgr->cur_stat.total_time_ms += packet_gap_ms;
        if (packet_gap_ms > jb_mgr->cur_stat.max_packet_gap_ms) {
            jb_mgr->cur_stat.max_packet_gap_ms = packet_gap_ms;
        }
        if (packet_gap_ms > 100) {
            jb_mgr->cur_stat.gap_100ms_count++;
        } else if (packet_gap_ms > 40) {
            jb_mgr->cur_stat.gap_40ms_count++;
        }
        
        int32_t seqNumDiffLast = DIFF_RTP_SEQ_NO(jb_mgr->stat_last_seq_num, rtp_hdr->seq_num);
        if (seqNumDiffLast < 0) {
            // Disordered packet. Keep stat_last_seq_num unchanged.
            int32_t seqNumDiffFirst = DIFF_RTP_SEQ_NO(jb_mgr->stat_first_seq_num, rtp_hdr->seq_num);
            jb_mgr->cur_stat.disorder_count++;
            if ((-seqNumDiffLast) > jb_mgr->cur_stat.max_disorder) {
                jb_mgr->cur_stat.max_disorder = (-seqNumDiffLast);
            }
            // If the disordered packet is lost one within this stat period, take it into account.
            if ((seqNumDiffFirst > 0) && (jb_mgr->cur_stat.loss_count > 0)) {
                jb_mgr->cur_stat.loss_count--;
            }
        } else if (0 == seqNumDiffLast) {
            // Duplicated packet. Keep stat_last_seq_num unchanged.
            jb_mgr->cur_stat.duplicate_count++;
        } else if (seqNumDiffLast > 1) {
            // Lost packet.
            int32_t packetLossCount = seqNumDiffLast - 1;
            jb_mgr->cur_stat.loss_count += packetLossCount;
            if (packetLossCount > jb_mgr->cur_stat.max_loss) {
                jb_mgr->cur_stat.max_loss = packetLossCount;
            }
            jb_mgr->stat_last_seq_num = rtp_hdr->seq_num;
        } else {
            // Normal
            jb_mgr->stat_last_seq_num = rtp_hdr->seq_num;
        }
        
        if (jb_mgr->cur_stat.total_time_ms >= jb->stat_calc_period_ms) {
            jb_mgr->prev_stat = jb_mgr->cur_stat;
            jb_mgr->stat_available = tsk_true;
            memset(&(jb_mgr->cur_stat), 0, sizeof(packet_stat_t));
            jb_mgr->stat_first_seq_num = rtp_hdr->seq_num;
            jb_mgr->stat_last_seq_num  = rtp_hdr->seq_num;
            
            jb_mgr->stat_total_time_ms_since_last_log += jb_mgr->prev_stat.total_time_ms;
            if (jb_mgr->stat_total_time_ms_since_last_log >= jb->stat_log_period_ms) {
            webrtc::RtcpStatistics stats;
            jb_mgr->neteq->GetRtcpStatistics(&stats);
                TSK_DEBUG_INFO ("Session(%d) TimeMs Total(%u)Avg(%u)Max(%u)40ms(%u)100ms(%u), Packet Count(%u) NetEqLossRate(%u) Loss(%u)Rate(%u%%)Max(%u) Disorder(%u)Rate(%u%%)Max(%u) Dup(%u)",
                                jb_mgr->session_id,
                                jb_mgr->prev_stat.total_time_ms,
                                jb_mgr->prev_stat.total_time_ms / jb_mgr->prev_stat.packet_count,
                                jb_mgr->prev_stat.max_packet_gap_ms,
                                jb_mgr->prev_stat.gap_40ms_count,
                                jb_mgr->prev_stat.gap_100ms_count,
                                jb_mgr->prev_stat.packet_count,
                            (stats.fraction_lost * 100) / 255,
                                jb_mgr->prev_stat.loss_count,
                                (jb_mgr->prev_stat.loss_count * 100) / (jb_mgr->prev_stat.packet_count + jb_mgr->prev_stat.loss_count),
                                jb_mgr->prev_stat.max_loss,
                                jb_mgr->prev_stat.disorder_count,
                                (jb_mgr->prev_stat.disorder_count * 100) / (jb_mgr->prev_stat.packet_count + jb_mgr->prev_stat.loss_count),
                                jb_mgr->prev_stat.max_disorder,
                                jb_mgr->prev_stat.duplicate_count);

                jb_mgr->stat_total_time_ms_since_last_log = 0;
            }
        }
    }
    
    jb_mgr->first_packet = false;
    jb_mgr->last_pkt_ts = cur_time_ms;
    
    handle_bandwidth_ctrl_data(jb, jb_mgr, rtp_hdr);

    return 0;
}

static tsk_size_t tdav_youme_neteq_jitterbuffer_get_micaud (tmedia_jitterbuffer_t *self, void *in_data, void *out_data, tsk_size_t out_size)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    tsk_size_t readSize  = (jb->out_rate == 0) ? 0 : out_size * jb->micaud_magr_t->micaudrate / jb->out_rate;
    static uint32_t u4MicaudEmptyCnt = 0;
    
    //read data
    if (WebRtc_youme_available_read(jb->micaud_magr_t->micaud_ringbuf) >= (readSize / sizeof(int16_t))) {
        // Means can read buffer
        tsk_mutex_lock(jb->micaud_magr_t->micaud_ringbuf_mutex);
        WebRtc_youme_ReadBuffer(jb->micaud_magr_t->micaud_ringbuf, NULL, in_data, readSize / sizeof(int16_t));
        tsk_mutex_unlock(jb->micaud_magr_t->micaud_ringbuf_mutex);
    } else {
        if (++u4MicaudEmptyCnt % 400 == 0) {
            TSK_DEBUG_INFO("(Mic+Audio mute in anchor mode)Mic audio ring buffer is empty, audio underrun!");
        }
        
        memset( out_data, 0, out_size );
        return 0;
    }
    
    //resample to play samplerate
    int in_rate = jb->micaud_magr_t->micaudrate;
    int out_rate = jb->out_rate;
    if( in_rate != out_rate )
    {
        if( jb->micaud_magr_t->micaud_resampler == nullptr )
        {
            
#ifdef USE_WEBRTC_RESAMPLE
            jb->micaud_magr_t->micaud_resampler = new youme::webrtc::PushResampler<int16_t>();
            jb->micaud_magr_t->micaud_resampler->InitializeIfNeeded(in_rate, out_rate, 1);
#else
            jb->micaud_magr_t->micaud_resampler  = speex_resampler_init(channels, in_rate, out_rate, 3, NULL);
#endif
        }
        
        if (jb->micaud_magr_t->micaud_resampler != nullptr) {
            uint32_t inSample  = readSize / sizeof(int16_t);
            uint32_t outSample = out_size / sizeof(int16_t);
            tdav_youme_neteq_jitterbuffer_resample( inSample, outSample, jb->micaud_magr_t->micaud_resampler, jb->micaud_magr_t->micaud_resampler_mutex,
                                                   in_data, out_data, out_size );
        }
    }
    else{
        memcpy( out_data, in_data, out_size );
    }

    return out_size;
}

static tsk_size_t tdav_youme_neteq_jitterbuffer_get_micaud_callback (tmedia_jitterbuffer_t *self, void *in_data, void *out_data, tsk_size_t out_size)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    tsk_size_t readSize  = (jb->out_rate == 0) ? 0 : out_size * jb->micaud_callback_magr_t->micaudrate / jb->out_rate;
    static uint32_t u4MicaudEmptyCnt = 0;
    
    if (WebRtc_youme_available_read(jb->micaud_callback_magr_t->micaud_ringbuf) >= (readSize / sizeof(int16_t))) {
        // Means can read buffer
        tsk_mutex_lock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
        WebRtc_youme_ReadBuffer(jb->micaud_callback_magr_t->micaud_ringbuf, NULL, in_data, readSize / sizeof(int16_t));
        tsk_mutex_unlock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
    } else {
        if (++u4MicaudEmptyCnt % 400 == 0) {
            TSK_DEBUG_INFO("(Mic+Audio mute in anchor mode)Mic audio ring buffer is empty, audio underrun!");
        }
        
        memset( out_data, 0, out_size );
        return 0;
    }
    
    //resample to play samplerate
    int in_rate = jb->micaud_callback_magr_t->micaudrate;
    int out_rate = jb->out_rate;
    if( in_rate != out_rate )
    {
        if( jb->micaud_callback_magr_t->micaud_resampler == nullptr )
        {
            
#ifdef USE_WEBRTC_RESAMPLE
            jb->micaud_callback_magr_t->micaud_resampler = new youme::webrtc::PushResampler<int16_t>();
            jb->micaud_callback_magr_t->micaud_resampler->InitializeIfNeeded(in_rate, out_rate, 1);
#else
            jb->micaud_callback_magr_t->micaud_resampler  = speex_resampler_init(channels, in_rate, out_rate, 3, NULL);
#endif
        }
        
        if (jb->micaud_callback_magr_t->micaud_resampler != nullptr) {
            uint32_t inSample  = readSize / sizeof(int16_t);
            uint32_t outSample = out_size / sizeof(int16_t);
            tdav_youme_neteq_jitterbuffer_resample( inSample, outSample, jb->micaud_callback_magr_t->micaud_resampler, jb->micaud_callback_magr_t->micaud_resampler_mutex,
                                                   in_data, out_data, out_size );
        }
    }
    else{
        memcpy( out_data, in_data, out_size );
    }
    
    return out_size;
}

static int tdav_youme_neteq_jitterbuffer_resample( uint32_t& inSample, uint32_t& outSample,
                                       void* resample,  tsk_mutex_handle_t*  resample_mutex, void* fromBuff, void* toBuff, tsk_size_t outBufSize )
{
    if( toBuff == nullptr )
    {
        return 0;
    }
    
    if ( resample && fromBuff != nullptr ) {
        if( resample_mutex )
        {
            tsk_mutex_lock( resample_mutex );
        }
        
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
        if( resample_mutex )
        {
            tsk_mutex_unlock( resample_mutex );
        }
        
        return 1;
    } else {
        memset( toBuff, 0, outBufSize);
        return 0;
    }
}

static tsk_size_t tdav_youme_neteq_jitterbuffer_get_bkaud (tmedia_jitterbuffer_t *self, void *in_data, void *out_data, tsk_size_t out_size)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    static uint32_t u4BkaudEmptyCnt = 0;

    tsk_list_item_t *first_item = NULL;
    tdav_session_audio_frame_buffer_t* frame_buf = NULL;
    uint32_t inSample, outSample;
    uint32_t remainSize = 0;
    tsk_size_t readSize = (jb->out_rate == 0) ? 0 : out_size * jb->bkaud_magr_t->bkaudrate / jb->out_rate;
    tsk_size_t outSize  = out_size;
    
    if ((!self) || (!in_data) ||(!out_data) || (!out_size)) {
        return 0;
    }
    if (!jb->bkaud_magr_t->bkaud_free_lst || !jb->bkaud_magr_t->bkaud_filled_lst || !jb->bkaud_magr_t->bkaud_sema_lst) {
        TSK_DEBUG_ERROR("Background related items ISNT be initialized!!");
        return 0;
    }
    
    if (!jb->bkaud_magr_t->bkaudrate) {
        if (++u4BkaudEmptyCnt % 400 == 0) {
            TSK_DEBUG_INFO("Background audio sample rate = 0!");
        }
        return 0;
    }
    
    first_item = jb->bkaud_magr_t->bkaud_lstitem_backup;
    if (first_item) {
        frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
    }
    
    if (!first_item) {
        tsk_list_lock(jb->bkaud_magr_t->bkaud_filled_lst);
        first_item = tsk_list_pop_first_item(jb->bkaud_magr_t->bkaud_filled_lst);
        tsk_list_unlock(jb->bkaud_magr_t->bkaud_filled_lst);
        if (first_item) {
            frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
        } else {
            if (++u4BkaudEmptyCnt % 400 == 0) {
                TSK_DEBUG_INFO("Background audio is NOT enough: level 1");
            }
            return 0;
        }
    }
    
    if (first_item && frame_buf && frame_buf->pcm_data) {
        if (frame_buf->pcm_size >= readSize) {
            // Means the remain size is still enough
            memcpy((char*)in_data, (char*)frame_buf->pcm_data + jb->bkaud_magr_t->bkaud_filled_postition, readSize);
            frame_buf->pcm_size -= readSize;
            jb->bkaud_magr_t->bkaud_filled_postition += readSize;
        } else {
            // Means the remain size is NOT enough
            remainSize = frame_buf->pcm_size;
            memcpy((char*)in_data, (char*)frame_buf->pcm_data + jb->bkaud_magr_t->bkaud_filled_postition, frame_buf->pcm_size);
            jb->bkaud_magr_t->bkaud_filled_postition = 0;
            tsk_list_lock(jb->bkaud_magr_t->bkaud_free_lst);
            tsk_list_push_item(jb->bkaud_magr_t->bkaud_free_lst, &first_item, tsk_true);
            tsk_list_unlock(jb->bkaud_magr_t->bkaud_free_lst);
            tsk_semaphore_increment(jb->bkaud_magr_t->bkaud_sema_lst);
            
            tsk_list_lock(jb->bkaud_magr_t->bkaud_filled_lst);
            first_item = tsk_list_pop_first_item(jb->bkaud_magr_t->bkaud_filled_lst);
            tsk_list_unlock(jb->bkaud_magr_t->bkaud_filled_lst);
            if (first_item) {
                frame_buf = (tdav_session_audio_frame_buffer_t*)first_item->data;
                memcpy((char*)in_data + remainSize, (char*)frame_buf->pcm_data + jb->bkaud_magr_t->bkaud_filled_postition, readSize - remainSize);
                jb->bkaud_magr_t->bkaud_filled_postition += readSize - remainSize;
                frame_buf->pcm_size -= readSize - remainSize;
            } else {
                if (++u4BkaudEmptyCnt % 400 == 0) {
                    TSK_DEBUG_INFO("Background audio is NOT enough: level 2");
                }
                return 0;
            }
        }
    }
    jb->bkaud_magr_t->bkaud_lstitem_backup = first_item;
    if (jb->bkaud_magr_t->bkaud_resampler != nullptr) {
        inSample  = readSize / sizeof(int16_t);
        outSample = outSize / sizeof(int16_t);
        tdav_youme_neteq_jitterbuffer_resample( inSample, outSample, jb->bkaud_magr_t->bkaud_resampler, jb->bkaud_magr_t->bkaud_resampler_mutex,
                                               in_data, out_data, out_size );
    }
    return outSize;
}

static int32_t tdav_youme_neteq_jitterbuffer_calc_power_db(const uint8_t* buffer, tsk_size_t size)
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

static void tdav_youme_neteq_jitterbuffer_handle_farend_voice_level_callback(tdav_youme_neteq_jitterbuffer_t *jb, uint32_t sessionId, void* buffer, uint32_t size)
{
    // If the audio level less than this value, we take it as silence.
#define SILENCE_DB  (60)

    if (!jb || !buffer || size <= 0) {
        return;
    }
    if (jb->farend_voice_level.count++ % 10 == 0) {
        int32_t level = 0;
        level = tdav_youme_neteq_jitterbuffer_calc_power_db((const uint8_t *) buffer, (tsk_size_t) size) + SILENCE_DB;
        if (level < 0) {
            level = 0;
        }
        if (level > SILENCE_DB) {
            level = SILENCE_DB;
        }
        level = (int32_t) (level * jb->farend_voice_level.max_level / SILENCE_DB);

        if (jb->farend_voice_level.callback && (level != jb->farend_voice_level.last_level)) {
//            if (jb->farend_voice_level.count++ % 10 == 0) {
                jb->farend_voice_level.last_level = level;
                jb->farend_voice_level.callback(jb->farend_voice_level.last_level, sessionId);
//            }
        }
    }
}

tdav_audio_resample_t* tdav_audio_resample_find(tdav_youme_neteq_jitterbuffer_t *self, uint32_t sessionid, tsk_boolean_t isMixing) {
    tsk_list_item_t *item = tsk_null;
    tdav_audio_resample_t *resample = tsk_null;
    
    if(!self->audio_resample_list) {
        TSK_DEBUG_ERROR("*** audio_resample_list is null ***");
        return tsk_null;
    }
    
    tsk_list_lock(self->audio_resample_list);
    tsk_list_foreach(item, self->audio_resample_list) {
        if((TDAV_AUDIO_RESAMPLE(item->data)->sessionid == sessionid) && (TDAV_AUDIO_RESAMPLE(item->data)->isMixing == isMixing)) {
            resample = TDAV_AUDIO_RESAMPLE(item->data);
            break;
        }
    }
    tsk_list_unlock(self->audio_resample_list);
    
    return resample;
}

//初始化pcm回调的重采样器
static void tdav_youme_init_pcmcbresample(tmedia_jitterbuffer_t *self)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    if(NULL==jb->voice_magr_t->voice_resampler_pcmcbbyuser)
    {
    #ifdef USE_WEBRTC_RESAMPLE
        jb->voice_magr_t->voice_resampler_pcmcbbyuser = new youme::webrtc::PushResampler<int16_t>();
        jb->voice_magr_t->voice_resampler_pcmcbbyuser->InitializeIfNeeded(jb->out_rate , jb->nPCMCallbackSampleRate, jb->channels);
    #else
        jb->voice_magr_t->voice_resampler_pcmcbbyuser = speex_resampler_init(channels, out_rate, jb->nPCMCallbackSampleRate, 3, NULL);
    #endif
    }
}

//初始化pcm回调的重采样器
static void tdav_youme_init_pcmcbresample_record(tmedia_jitterbuffer_t *self)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    if(NULL==jb->voice_magr_t->voice_resampler_pcmcb_record)
    {
    #ifdef USE_WEBRTC_RESAMPLE
        jb->voice_magr_t->voice_resampler_pcmcb_record = new youme::webrtc::PushResampler<int16_t>();
        jb->voice_magr_t->voice_resampler_pcmcb_record->InitializeIfNeeded(jb->out_rate , jb->nPCMCallbackSampleRate, jb->channels);
    #else
        jb->voice_magr_t->voice_resampler_pcmcb_record = speex_resampler_init(channels, out_rate, jb->nPCMCallbackSampleRate, 3, NULL);
    #endif
    }
}

//初始化pcm回调的重采样器
static void tdav_youme_init_pcmcbresample_mix(tmedia_jitterbuffer_t *self)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    if(NULL==jb->voice_magr_t->voice_resampler_pcmcb_mix)
    {
    #ifdef USE_WEBRTC_RESAMPLE
        jb->voice_magr_t->voice_resampler_pcmcb_mix = new youme::webrtc::PushResampler<int16_t>();
        jb->voice_magr_t->voice_resampler_pcmcb_mix->InitializeIfNeeded(jb->out_rate , jb->nPCMCallbackSampleRate, jb->channels);
    #else
        jb->voice_magr_t->voice_resampler_pcmcb_mix = speex_resampler_init(channels, out_rate, jb->nPCMCallbackSampleRate, 3, NULL);
    #endif
    }
}

//out_data数据用于播放
//voice_data数据用于回声消除
static tsk_size_t tdav_youme_neteq_jitterbuffer_get (tmedia_jitterbuffer_t *self, void *out_data, void *voice_data, tsk_size_t out_size)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    
    int ret = 0;
    tsk_size_t ret_size = 0;
    jb_manager_map_t::iterator it;
    static int nNeteqJbNum = 0;
    tsk_size_t readBkSz    = 0;
    tsk_size_t outBkSz     = 0;
    tsk_size_t readMicSz   = 0;
    uint32_t inSample = 0 , outSample = 0 ;
    //存在外部输入的音频数据
    tsk_bool_t bIsExistBkaud  = tsk_false;
    //打开了耳返
    tsk_bool_t bIsExistMicaud = tsk_false;
    //设置了pcmcallback
    tsk_bool_t bIsExistMicaudCb = tsk_false;

    // Must initialize here! 送给speaker的Mic和background混合数据 
    //jb->bkaud_magr_t->bkaud_tempbuf2现在是空数据，只是先用pBkMicAudio指向这块buffer
    void *pBkMicAudio = jb->bkaud_magr_t->bkaud_tempbuf2; 

    //送给回声消除的bk,mic混合数据。因为callback数据一定包含 mic和bgm，所以如果有callback，用callback数据送给回声消除
    void *pBkMicAudioCb = jb->micaud_callback_magr_t->micaud_tempbuf2; 

    tdav_session_audio_frame_buffer_t * frame_buf_cb_remote = tsk_null; // PCM frame buffer for callback
    tdav_session_audio_frame_buffer_t * frame_buf_cb_record = tsk_null;
    tdav_session_audio_frame_buffer_t *frame_buf_cb_mix = tsk_null; // PCM frame buffer for callback
    tdav_session_audio_frame_buffer_t * frame_buf_cb_mix_resample = tsk_null;
    
    //tdav_youme_init_pcmcbresample(self);
    
    if (!out_data || !out_size || !voice_data)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    
    if (jb->bkaud_magr_t->bkaudrate > 48000) {
        TSK_DEBUG_ERROR ("Invalid background audio sample rate");
        return 0;
    }
    
    #if FFMPEG_SUPPORT
    YMVideoRecorderManager::getInstance()->setAudioInfo( jb->out_rate );
    #endif
    
    jb->packet_count++;

    readBkSz = (jb->out_rate == 0) ? 0 : out_size * jb->bkaud_magr_t->bkaudrate / jb->out_rate;
    outBkSz  = out_size;
    if (readBkSz > MAX_TEMP_BUF_BYTESZ) {
        jb->bkaud_magr_t->bkaud_tempbuf1 = tsk_realloc(jb->bkaud_magr_t->bkaud_tempbuf1, readBkSz);
    }
    if (outBkSz > MAX_TEMP_BUF_BYTESZ) {
        jb->bkaud_magr_t->bkaud_tempbuf2 = tsk_realloc(jb->bkaud_magr_t->bkaud_tempbuf2, outBkSz);
    }
    
    // 获取背景声音(BKAUD_SOURCE_GAME)，放在jb->bkaud_magr_t->bkaud_tempbuf1
    // 然后进行重采样，放在jb->bkaud_magr_t->bkaud_tempbuf2
    if (tdav_youme_neteq_jitterbuffer_get_bkaud(self, jb->bkaud_magr_t->bkaud_tempbuf1, jb->bkaud_magr_t->bkaud_tempbuf2, out_size)) {
        bIsExistBkaud = tsk_true;
        pBkMicAudio = jb->bkaud_magr_t->bkaud_tempbuf2;
    } else {
        memset(jb->bkaud_magr_t->bkaud_tempbuf2, 0, outBkSz);
    }
    
    readMicSz = out_size;
    if (readMicSz > MAX_TEMP_BUF_BYTESZ) {
        jb->micaud_magr_t->micaud_tempbuf1 = tsk_realloc(jb->micaud_magr_t->micaud_tempbuf1, readMicSz);
        jb->micaud_callback_magr_t->micaud_tempbuf1  = tsk_realloc(jb->micaud_callback_magr_t->micaud_tempbuf1, readMicSz);
    }
    
    if( out_size > MAX_TEMP_BUF_BYTESZ )
    {
        jb->micaud_magr_t->micaud_tempbuf2 = tsk_realloc(jb->micaud_magr_t->micaud_tempbuf1, out_size);
        jb->micaud_callback_magr_t->micaud_tempbuf2  = tsk_realloc(jb->micaud_callback_magr_t->micaud_tempbuf1, out_size);
    }
    
    // 耳返/外部输入模式来的数据
    //麦克风的声音直接mix到这里，要在这里完成转频到playback samplerate
    if (tdav_youme_neteq_jitterbuffer_get_micaud(self, jb->micaud_magr_t->micaud_tempbuf1, jb->micaud_magr_t->micaud_tempbuf2, out_size)) { // out_size == readMicSz
        bIsExistMicaud = tsk_true;
        pBkMicAudio = jb->micaud_magr_t->micaud_tempbuf2;
    } else {
        memset(jb->micaud_magr_t->micaud_tempbuf2, 0, out_size);
    }
    
    //录像/callback回调来的数据，同时包含mic和bg数据。
    if (tdav_youme_neteq_jitterbuffer_get_micaud_callback(self, jb->micaud_callback_magr_t->micaud_tempbuf1, jb->micaud_callback_magr_t->micaud_tempbuf2, out_size)) { // out_size == readMicSz
        bIsExistMicaudCb = tsk_true;
        pBkMicAudioCb = jb->micaud_callback_magr_t->micaud_tempbuf2;
    } else {
        memset(jb->micaud_callback_magr_t->micaud_tempbuf2, 0, out_size);
    }
    
    // Level 1 mix audio
    // Mix background audio & mic audio here(playback samplerate mix)
    if (bIsExistBkaud && bIsExistMicaud) {
        tdav_codec_mixer_normal((int16_t*)jb->bkaud_magr_t->bkaud_tempbuf2, (int16_t*)jb->micaud_magr_t->micaud_tempbuf2, (int16_t*)jb->bkaud_magr_t->bkaud_tempbuf2, outBkSz / sizeof(int16_t));
        pBkMicAudio = jb->bkaud_magr_t->bkaud_tempbuf2;
    }

    // No audio data coming in yet.
    if (jb->jb_mgr_map->empty()) {
        if ((jb->packet_count % 1000) == 0) {
            TSK_DEBUG_INFO("Not received any audio data, packet_count:%u", jb->packet_count);
        }
        
        if (bIsExistBkaud || bIsExistMicaud) {
            if(tmedia_defaults_get_external_input_mode()){
                //#ifdef YOUME_FOR_QINIU
                memset(out_data, 0, out_size);
                inSample = out_size / sizeof(int16_t);
                outSample = inSample * jb->mixed_callback_rate / jb->out_rate;
                tdav_youme_neteq_jitterbuffer_resample( inSample, outSample, jb->voice_magr_t->voice_resampler_pcmcb, jb->voice_magr_t->voice_resampler_pcmcb_mutex,
                                                       pBkMicAudio, jb->voice_magr_t->voice_tempbuf3,  outSample * sizeof( int16_t ) );
                //TMEDIA_DUMP_TO_FILE("/pcm_data_dump_1.bin", jb->voice_magr_t->voice_tempbuf3, outSample * sizeof(int16_t));
                YouMeEngineManagerForQiniu::getInstance()->onAudioFrameMixedCallback(jb->voice_magr_t->voice_tempbuf3, outSample * sizeof(int16_t), 0);
            }else {
                //#else
                memcpy(out_data, pBkMicAudio, out_size);
                //#endif
            }
        }
        else{
            memset(out_data, 0, out_size);
        }
        
        memset(voice_data, 0, out_size * jb->in_rate / jb->out_rate);
        
        if( (jb->pcm_callback && jb->isMixCallbackOn ) || jb->isSaveScreen  )
        {
            frame_buf_cb_mix =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, out_size);
            frame_buf_cb_mix->pcm_size = out_size;
            //这里使用out_rate的采样率，可能需要进行转换啊。
            memcpy(frame_buf_cb_mix->pcm_data, jb->micaud_callback_magr_t->micaud_tempbuf1, out_size );
			frame_buf_cb_mix->pcm_size = out_size;
        }
        
        //这里没有任何远端数据
        if( jb->pcm_callback && jb->isRemoteCallbackOn )
        {
            frame_buf_cb_remote =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, out_size);
        }
        
        //if( ( jb->pcm_callback && jb->isRecordCallbackOn ) || jb->isTranscriberOn )
        if(jb->isTranscriberOn)
        {
            frame_buf_cb_record =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, out_size);
            if( frame_buf_cb_record )
            {
                memcpy(frame_buf_cb_record->pcm_data, jb->micaud_callback_magr_t->micaud_tempbuf1, out_size );
				frame_buf_cb_record->pcm_size = out_size;
            }
        }
    }
    else{
        // Get audio data from NetEq for each session,合成的数据放到jb->voice_magr_t->voice_tempbuf1中
        tsk_size_t voice_data_size = 0;
        outSample = tdav_youme_neteq_jitterbuffer_mix_all_remote_pcm_data( jb , out_size, nNeteqJbNum, &voice_data_size);
        if( outSample == 0 )  //返回0表示出错了
        {
            return 0;
        }
    
        if (NULL != voice_data) {
            memcpy(voice_data, jb->voice_magr_t->voice_tempbuf1, voice_data_size);
        }
        
        if ( (jb->pcm_callback && jb->isMixCallbackOn) || jb->isSaveScreen || tmedia_defaults_get_external_input_mode() ) {
            frame_buf_cb_mix =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample * sizeof(int16_t));
            tdav_codec_mixer_normal((int16_t*)jb->voice_magr_t->voice_tempbuf2, (int16_t*)pBkMicAudioCb, (int16_t*)frame_buf_cb_mix->pcm_data, outSample);
            frame_buf_cb_mix->pcm_size = out_size;
        }
        
        if( jb->pcm_callback && jb->isRemoteCallbackOn)
        {
            //frame_buf_cb_remote =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample * sizeof(int16_t));
            //if( frame_buf_cb_remote )
            {
                // frame_buf_cb_remote =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample * sizeof(int16_t));
                // if( frame_buf_cb_remote )
                // {
                //     memcpy(frame_buf_cb_remote->pcm_data, jb->voice_magr_t->voice_tempbuf1, outSample * sizeof(int16_t) );
                // }

                tdav_youme_init_pcmcbresample(self);

                uint32_t outSample2 = outSample;

                // 重采样demo，供参考
                inSample  = outSample2;
                outSample2 = outSample * jb->nPCMCallbackSampleRate / jb->out_rate;//outSample * jb->mixed_callback_rate / jb->out_rate;
                frame_buf_cb_remote =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample2 * sizeof(int16_t) * jb->nPCMCallbackChannel);
                tdav_youme_neteq_jitterbuffer_resample( inSample, outSample2, jb->voice_magr_t->voice_resampler_pcmcbbyuser, jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex,
                                         jb->voice_magr_t->voice_tempbuf1, frame_buf_cb_remote->pcm_data,  outSample2 * sizeof( int16_t ) );

                //单通道转双通道
                if(1 == jb->channels && 2 == jb->nPCMCallbackChannel)
                {
                    int16_t data_copy[youme::webrtc::AudioFrame::kMaxDataSizeSamples];
                    memcpy(data_copy, frame_buf_cb_remote->pcm_data,sizeof(int16_t) * outSample2);
                    youme::webrtc::AudioFrameOperations::MonoToStereo(data_copy, (size_t)outSample2, (int16_t*)frame_buf_cb_remote->pcm_data);
                }

                int dump_size = outSample2 * sizeof(int16_t) * jb->nPCMCallbackChannel;           
                frame_buf_cb_remote->pcm_size = dump_size;

                // dump file
                if (jb->pcm_file.remote_cb) {
                    if (jb->pcm_file.remote_cb_size > jb->pcm_file.max_size) {
                        tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_REMOTE_CB);
                    }
                    if (jb->pcm_file.remote_cb) {
                        fwrite (frame_buf_cb_remote->pcm_data, 1, dump_size, jb->pcm_file.remote_cb);
                        jb->pcm_file.remote_cb_size += dump_size;
                    }
                }
            }
        }
        
        //if( ( jb->pcm_callback && jb->isRecordCallbackOn ) || jb->isTranscriberOn)
        if(jb->isTranscriberOn)
        {
            frame_buf_cb_record =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample * sizeof(int16_t));
            if( frame_buf_cb_record )
            {

                memcpy(frame_buf_cb_record->pcm_data, jb->micaud_callback_magr_t->micaud_tempbuf1, outSample * sizeof(int16_t) );
				frame_buf_cb_record->pcm_size = outSample * sizeof(int16_t);
            }
        }

        if(jb->isMixCallbackOn)
        {
            tdav_youme_init_pcmcbresample_mix(self);
            uint32_t outSample2 = outSample;

            // 重采样,mix call back
            inSample  = outSample2;
            outSample2 = outSample * jb->nPCMCallbackSampleRate / jb->out_rate;//outSample * jb->mixed_callback_rate / jb->out_rate;
            frame_buf_cb_mix_resample =  (tdav_session_audio_frame_buffer_t *)tsk_object_new(tdav_session_audio_frame_buffer_def_t, outSample2 * sizeof(int16_t) * jb->nPCMCallbackChannel);
            if (frame_buf_cb_mix && frame_buf_cb_mix_resample) {
                tdav_youme_neteq_jitterbuffer_resample( inSample, outSample2, jb->voice_magr_t->voice_resampler_pcmcb_mix, jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex,
                                        frame_buf_cb_mix->pcm_data, frame_buf_cb_mix_resample->pcm_data,  outSample2 * sizeof( int16_t ) );
            }

            //单通道转双通道
            if(1==jb->channels && 2==jb->nPCMCallbackChannel)
            {
                int16_t data_copy[youme::webrtc::AudioFrame::kMaxDataSizeSamples];
                memcpy(data_copy, frame_buf_cb_mix_resample->pcm_data,sizeof(int16_t) * outSample2);
                youme::webrtc::AudioFrameOperations::MonoToStereo(data_copy, (size_t)outSample2, (int16_t*)frame_buf_cb_mix_resample->pcm_data);
            }

            int dump_size = outSample2 * sizeof(int16_t) * jb->nPCMCallbackChannel;           

            frame_buf_cb_mix_resample->pcm_size = dump_size;

            if (jb->pcm_file.mix_cb) {
                if (jb->pcm_file.mix_cb_size > jb->pcm_file.max_size) {
                    //重新打开，相当于清空重写
                        tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_MIX_CB);
                }
                if (jb->pcm_file.mix_cb) {
                    fwrite (frame_buf_cb_mix_resample->pcm_data, 1, dump_size, jb->pcm_file.mix_cb);
                    jb->pcm_file.mix_cb_size += dump_size;
                }
            }
        }
        
        // // Level 3 mix audio(playback sample rate mix)
        // Mix voice data and background audio data together
        if( bIsExistMicaud )
        {
            if( tmedia_defaults_get_external_input_mode() )
            {
                memcpy(out_data, jb->voice_magr_t->voice_tempbuf2, outSample * sizeof(int16_t));
            }
            else//耳返
            {
                tdav_codec_mixer_normal((int16_t*)jb->voice_magr_t->voice_tempbuf2, (int16_t*)pBkMicAudio, (int16_t*)out_data, outSample);
            }
        }
        else{
            memcpy(out_data, jb->voice_magr_t->voice_tempbuf2, outSample * sizeof(int16_t));
        }

        
        if(tmedia_defaults_get_external_input_mode()) {
            //#ifdef YOUME_FOR_QINIU
            inSample  = outSample;
            outSample = outSample * jb->mixed_callback_rate / jb->out_rate;
            tdav_youme_neteq_jitterbuffer_resample( inSample, outSample, jb->voice_magr_t->voice_resampler_pcmcb, jb->voice_magr_t->voice_resampler_pcmcb_mutex,
                                                   frame_buf_cb_mix->pcm_data, jb->voice_magr_t->voice_tempbuf3,  outSample * sizeof( int16_t ) );
            //这里要mix远端数据和input的mic数据
            YouMeEngineManagerForQiniu::getInstance()->onAudioFrameMixedCallback(jb->voice_magr_t->voice_tempbuf3, outSample * sizeof(int16_t), 0);
            //TMEDIA_DUMP_TO_FILE("/pcm_data_dump.bin", jb->voice_magr_t->voice_tempbuf3, outSample * sizeof(int16_t));
            //#endif
            
            // int dump_size = outSample * sizeof(int16_t);
            // if (jb->pcm_file.mix_cb) {
            //     if (jb->pcm_file.mix_cb_size > jb->pcm_file.max_size) {
            //         tdav_youme_neteq_jitterbuffer_open_pcm_files(jb, TDAV_JITTERBUFFER_PCM_FILE_MIX_CB);
            //     }
            //     if (jb->pcm_file.mix_cb) {
            //         fwrite (jb->voice_magr_t->voice_tempbuf3, 1, dump_size, jb->pcm_file.mix_cb);
            //         jb->pcm_file.mix_cb_size += dump_size;
            //     }
            // }
        }
        
        if ((++nNeteqJbNum) % 800 == 0) {
            TSK_DEBUG_INFO("ret_size:(%d),out_size:(%d),readBk_size:(%d),readMic_size:(%d),in_rate:(%d),out_rate:(%d),bkaud_rate:(%d),micaud_rate:(%d)", ret_size, out_size, readBkSz, readMicSz, jb->in_rate, jb->out_rate, jb->bkaud_magr_t->bkaudrate, jb->micaud_magr_t->micaudrate);
        }
        
        /*
         tdav_audio_resample_t* resample = tdav_audio_resample_find(jb, 0, tsk_true);
         if(!resample) {
         tdav_audio_resample_t* resample_new = tdav_audio_resample_create(0, tsk_true, 1, 16000, 48000);
         resample = resample_new;
         tsk_list_push_back_data(jb->audio_resample_list, (tsk_object_t**)&resample_new);
         }
         
         tdav_audio_unit_t* audio_unit = tdav_audio_unit_create(out_size);
         memcpy(audio_unit->data, (void*)(out_data), out_size);
         audio_unit->timestamp = 0;
         audio_unit->sessionid = 0;
         tdav_audio_resample_push(resample, audio_unit);
         TSK_OBJECT_SAFE_FREE(audio_unit);
         */
        //YouMeEngineManagerForQiniu::getInstance()->onAudioFrameMixedCallback(out_data, out_size, 0);
    }
    
    if( jb->pcm_callback  )
    {
        if( jb->isRemoteCallbackOn )
        {
            notify_pcm_data( jb, frame_buf_cb_remote,  Pcbf_remote );
        }
        /*
        if( jb->isRecordCallbackOn )
        {
            notify_pcm_data( jb,  frame_buf_cb_record, Pcbf_record );
        }
        */
        if( jb->isMixCallbackOn )
        {
            notify_pcm_data( jb,  frame_buf_cb_mix, Pcbf_mix);
        }

    }
    
    if( jb->isSaveScreen && frame_buf_cb_mix )
    {
#if FFMPEG_SUPPORT
        YMVideoRecorderManager::getInstance()->inputAudioData( (uint8_t*)frame_buf_cb_mix->pcm_data, frame_buf_cb_mix->pcm_size , tsk_time_now());
#endif
    }
    
    if( jb->isTranscriberOn && frame_buf_cb_record)
    {
        //local record的sessionId暂时用0表示吧
        tdav_youme_neteq_jitterbuffer_input_audio_pcm_to_transcriber(jb, (uint8_t*)frame_buf_cb_record->pcm_data , frame_buf_cb_record->pcm_size , jb->out_rate  , 0 );
    }
    
    if( frame_buf_cb_remote )
    {
        tsk_object_unref(frame_buf_cb_remote);
    }
    if( frame_buf_cb_record )
    {
        tsk_object_unref(frame_buf_cb_record);
    }
    if( frame_buf_cb_mix )
    {
        tsk_object_unref(frame_buf_cb_mix);
    }
	if (frame_buf_cb_mix_resample)
	{
		tsk_object_unref(frame_buf_cb_mix_resample);
	}
    return out_size;
}

static int tdav_youme_neteq_jitterbuffer_input_audio_pcm_to_transcriber( tdav_youme_neteq_jitterbuffer_t *jb, void *audio_data , tsk_size_t audio_size, int samplerate , int sessionId )
{
    int needSampleRate = YMTranscriberManager::getInstance()->getSupportSampleRate() ;
    if( samplerate == needSampleRate )
    {
        YMTranscriberManager::getInstance()->inputAudioData( sessionId, (uint8_t*)audio_data,  audio_size );
    }
    else{
        uint32_t inSample = audio_size / sizeof( int16_t );
        uint32_t outSample = inSample * needSampleRate / samplerate ;
        //需要重采样
        if( sessionId == 0 )
        {
            tdav_youme_neteq_jitterbuffer_resample( inSample, outSample , jb->transcriber_mgr_t->record_resampler, jb->transcriber_mgr_t->record_resampler_mutex,
                                                   audio_data,  jb->transcriber_mgr_t->voice_tempbuf_record, (tsk_size_t)MAX_TEMP_BUF_BYTESZ);
            
            YMTranscriberManager::getInstance()->inputAudioData( sessionId,  (uint8_t*)jb->transcriber_mgr_t->voice_tempbuf_record, outSample*sizeof(int16_t ));
        }
        else{
            tdav_youme_neteq_jitterbuffer_resample( inSample, outSample , jb->transcriber_mgr_t->remote_resampler, jb->transcriber_mgr_t->remote_resampler_mutex,
                                                   audio_data,  jb->transcriber_mgr_t->voice_tempbuf_remote, (tsk_size_t)MAX_TEMP_BUF_BYTESZ);
            
            YMTranscriberManager::getInstance()->inputAudioData( sessionId,  (uint8_t*)jb->transcriber_mgr_t->voice_tempbuf_remote, outSample*sizeof(int16_t ));
        }
    }
    
    return 0;
}

static int tdav_youme_neteq_jitterbuffer_mix_all_remote_pcm_data( tdav_youme_neteq_jitterbuffer_t *jb, tsk_size_t out_size , int nNeteqJbNum, tsk_size_t *voice_data_size)
{
    int ret = 0;
    tsk_size_t ret_size;
    jb_manager_map_t::iterator it;
    
    // Get audio data from NetEq for each session
    int max_sample_num = 0;
    int audio_track_num = 0;
    for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
        jb_manager_t *jb_mgr = it->second;
        if (!jb_mgr || !jb_mgr->neteq || !jb->mix_meta[audio_track_num].bufferPtr) {
            TSK_DEBUG_ERROR("Invalid parameter");
            return 0;
        }
        
        uint32_t total_samples_got = 0;
        bool ignore_cur_track = false;
        
        do {
            size_t samples_per_channel = 0;
            int num_channels = 0;
            ret = jb_mgr->neteq->GetAudio (jb->frame_sample_num - total_samples_got,
                                           &jb->mix_meta[audio_track_num].bufferPtr[total_samples_got],
                                           &samples_per_channel,
                                           &num_channels,
                                           NULL);
            if (webrtc::NetEq::kOK != ret) {
                ignore_cur_track = true;
                break;
            }
            // This should never happen. But if it unfortunately happens, we should not continue.
            if (num_channels != jb->channels) {
                TSK_DEBUG_ERROR ("Fatal error: NetEq output channel number(%d) doesn't match the preset one(%d)", samples_per_channel, jb->channels);
                return 0;
            }
            total_samples_got += (samples_per_channel * num_channels);
        } while (total_samples_got < jb->frame_sample_num);
        
        if ((jb->packet_count % 1000) == 0) {
            TSK_DEBUG_INFO("session(%d) last_packet_time(%ums) output_avail:%d, jbNum:%d, maxJbNum:%d",
                           jb_mgr->session_id, (uint32_t)(tsk_gettimeofday_ms() - jb_mgr->last_pkt_ts), !ignore_cur_track,
                           jb->jb_mgr_map->size(), jb->max_jb_num);
        }
        // If no data available for this session, just continue to read for the next session
        if (ignore_cur_track) {
            continue;
        }
        
        
        jb->mix_meta[audio_track_num].sampleCount = total_samples_got;
        if (total_samples_got > max_sample_num) {
            max_sample_num = total_samples_got;
        }
        
        if (jb->farend_voice_level.callback && jb->farend_voice_level.max_level > 0) {
            tdav_youme_neteq_jitterbuffer_handle_farend_voice_level_callback(jb,
                                                                             jb_mgr->session_id,
                                                                             (void*)(jb->mix_meta[audio_track_num].bufferPtr),
                                                                             total_samples_got * sizeof(int16_t));
        }
        
        if( jb->isTranscriberOn )
        {
            tdav_youme_neteq_jitterbuffer_input_audio_pcm_to_transcriber(jb, (void*)(jb->mix_meta[audio_track_num].bufferPtr), total_samples_got * sizeof(int16_t) , jb->in_rate , jb_mgr->session_id );
        }
#if 0
        tdav_audio_resample_t* resample = tdav_audio_resample_find(jb, jb_mgr->session_id, tsk_false);
        if(!resample) {
#ifdef ANDROID
            tdav_audio_resample_t* resample_new = tdav_audio_resample_create(jb_mgr->session_id, tsk_false, 1, jb->in_rate, 44100);
#else
            tdav_audio_resample_t* resample_new = tdav_audio_resample_create(jb_mgr->session_id, tsk_false, 1, jb->in_rate, 48000);
#endif
            resample = resample_new;
            tsk_list_push_back_data(jb->audio_resample_list, (tsk_object_t**)&resample_new);
        }
        
        tdav_audio_unit_t* audio_unit = tdav_audio_unit_create(out_size);
        memcpy(audio_unit->data, (void*)(jb->mix_meta[audio_track_num].bufferPtr), audio_unit->size);
        //TMEDIA_DUMP_TO_FILE("/pcm_data_16.bin", audio_unit->data, audio_unit->size);
        audio_unit->timestamp = 0;
        audio_unit->sessionid = jb_mgr->session_id;
        tdav_audio_resample_push(resample, audio_unit);
        TSK_OBJECT_SAFE_FREE(audio_unit);
#endif
        
        audio_track_num++;
    }
    
    // 最大48000 samplerate @ 20ms
    if (max_sample_num > 960) {
        max_sample_num = 960;
    }
    ret_size = max_sample_num * sizeof(int16_t);
    
    if (ret_size > (out_size * jb->in_rate / jb->out_rate)) {
        ret_size = out_size * jb->in_rate / jb->out_rate;
    }
    
    // Mix voice data for all the sessions together
    if (ret_size > MAX_TEMP_BUF_BYTESZ) {
        jb->voice_magr_t->voice_tempbuf1 = tsk_realloc(jb->voice_magr_t->voice_tempbuf1, ret_size);
    }
    if (ret_size * jb->out_rate / jb->in_rate > MAX_TEMP_BUF_BYTESZ) {
        jb->voice_magr_t->voice_tempbuf2 = tsk_realloc(jb->voice_magr_t->voice_tempbuf2, ret_size * jb->out_rate / jb->in_rate);
    }
    
    // // Level 2 mix audio(record samplerate mix)
    if (audio_track_num > 1) {
        tdav_codec_mixer_mix16(jb->mix_meta, audio_track_num, (int16_t*)jb->voice_magr_t->voice_tempbuf1, max_sample_num);
    } else if (ret_size > 0) {
        memcpy(jb->voice_magr_t->voice_tempbuf1, jb->mix_meta[0].bufferPtr, ret_size);
    }
    
    if (!ret_size) { // Special case, there is a(some) unavaliable packet get make the ret_size = 0
        if ((nNeteqJbNum) % 800 == 0) {
            TSK_DEBUG_WARN("Special unavailable packet make ret_size=0!");
        }
        ret_size = out_size * jb->in_rate / jb->out_rate;
        memset(jb->voice_magr_t->voice_tempbuf1, 0, ret_size);
    }
    
    *voice_data_size = ret_size;
    // 转频，将record samplerate 转成 playback samplerate
    uint32_t inSample  = ret_size / sizeof(int16_t);
    uint32_t outSample = ret_size * jb->out_rate / jb->in_rate / sizeof(int16_t);
    if (jb->voice_magr_t->voice_resampler) {
#ifdef USE_WEBRTC_RESAMPLE
        int src_nb_samples_per_process = ((jb->voice_magr_t->voice_resampler->GetSrcSampleRateHz() * 10) / 1000);
        int dst_nb_samples_per_process = ((jb->voice_magr_t->voice_resampler->GetDstSampleRateHz() * 10) / 1000);
        for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
            jb->voice_magr_t->voice_resampler->Resample((const int16_t*)jb->voice_magr_t->voice_tempbuf1 + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)jb->voice_magr_t->voice_tempbuf2 + i * dst_nb_samples_per_process, 0);
        }
#else
        speex_resampler_process_int(jb->voice_magr_t->voice_resampler, 0, (const spx_int16_t*)jb->voice_magr_t->voice_tempbuf1, (spx_uint32_t*)&inSample, (spx_int16_t*)jb->voice_magr_t->voice_tempbuf2, (spx_uint32_t*)&outSample);
#endif
    } else {
        TSK_DEBUG_ERROR("Voice resampler handler ISNT be initialized!");
    }
    
    return outSample;
}

static void notify_pcm_data( tdav_youme_neteq_jitterbuffer_t *jb, tdav_session_audio_frame_buffer_t* frame_buf_cb, int flag   )
{
    if( frame_buf_cb && jb )
    {
        // 48000 20mms stereo(16bits):3840 bytes
        // 数据长度为0，或者超过最大值则不回调上层
        if(!frame_buf_cb->pcm_size || frame_buf_cb->pcm_size > 4000)
        {
            return ;
        }

        frame_buf_cb->channel_num = jb->nPCMCallbackChannel;
        frame_buf_cb->sample_rate = jb->nPCMCallbackSampleRate;
        frame_buf_cb->is_float = 0;
        frame_buf_cb->is_interleaved = 1;
        frame_buf_cb->bytes_per_sample = 2 * jb->nPCMCallbackChannel;
        
        if (jb->pcm_callback) {
            jb->pcm_callback(frame_buf_cb, flag );
        }
    }
}

static int tdav_youme_neteq_jitterbuffer_reset (tmedia_jitterbuffer_t *self)
{
    //tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    return 0;
}

static int tdav_youme_neteq_jitterbuffer_close (tmedia_jitterbuffer_t *self)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    
    /*
    if(jb->running) {
        jb->running = tsk_false;
        tsk_thread_join(&jb->thread_handle);
    }
     */
    tdav_youme_neteq_jitterbuffer_close_pcm_files(jb);

    if (NULL != jb->mix_meta) {
        for (uint32_t i = 0; i < jb->mix_meta_num; i++) {
            if (NULL != jb->mix_meta[i].bufferPtr) {
                delete[] jb->mix_meta[i].bufferPtr;
                jb->mix_meta[i].bufferPtr = NULL;
            }
        }
        delete[] jb->mix_meta;
        jb->mix_meta = NULL;
    }
    
    jb_manager_map_t::iterator it;
	for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
        if (it->second) {
            jb_manager_t *jb_mgr = it->second;
            if (NULL != jb_mgr->neteq) {
                delete jb_mgr->neteq;
                jb_mgr->neteq = NULL;
            }
            delete it->second;
            it->second = NULL;
        }
    }
	jb->jb_mgr_map->clear();
    
    // Destroy handler of resampler here
    if (jb->voice_magr_t->voice_resampler) {
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->voice_magr_t->voice_resampler;
        jb->voice_magr_t->voice_resampler = nullptr;
#else
        speex_resampler_destroy(jb->voice_magr_t->voice_resampler);
        jb->voice_magr_t->voice_resampler = tsk_null;
#endif
    }
    if (jb->voice_magr_t->voice_resampler_pcmcb) {
        tsk_mutex_lock(jb->voice_magr_t->voice_resampler_pcmcb_mutex);
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->voice_magr_t->voice_resampler_pcmcb;
        jb->voice_magr_t->voice_resampler_pcmcb = nullptr;
#else
        speex_resampler_destroy(jb->voice_magr_t->voice_resampler_pcmcb);
        jb->voice_magr_t->voice_resampler_pcmcb = tsk_null;
#endif
        tsk_mutex_unlock(jb->voice_magr_t->voice_resampler_pcmcb_mutex);
    }
    if (jb->voice_magr_t->voice_resampler_pcmcb_mutex) {
        tsk_mutex_destroy(&(jb->voice_magr_t->voice_resampler_pcmcb_mutex));
    }

//remote
    if (jb->voice_magr_t->voice_resampler_pcmcbbyuser) {
        tsk_mutex_lock(jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex);
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->voice_magr_t->voice_resampler_pcmcbbyuser;
        jb->voice_magr_t->voice_resampler_pcmcbbyuser = nullptr;
#else
        speex_resampler_destroy(jb->voice_magr_t->voice_resampler_pcmcbbyuser);
        jb->voice_magr_t->voice_resampler_pcmcbbyuser = tsk_null;
#endif
        tsk_mutex_unlock(jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex);
    }
    if (jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex) {
        tsk_mutex_destroy(&(jb->voice_magr_t->voice_resampler_pcmcbbyuser_mutex));
    }
    //record
        if (jb->voice_magr_t->voice_resampler_pcmcb_record) {
        tsk_mutex_lock(jb->voice_magr_t->voice_resampler_pcmcb_record_mutex);
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->voice_magr_t->voice_resampler_pcmcb_record;
        jb->voice_magr_t->voice_resampler_pcmcb_record = nullptr;
#else
        speex_resampler_destroy(jb->voice_magr_t->voice_resampler_pcmcb_record);
        jb->voice_magr_t->voice_resampler_pcmcb_record = tsk_null;
#endif
        tsk_mutex_unlock(jb->voice_magr_t->voice_resampler_pcmcb_record_mutex);
    }
    if (jb->voice_magr_t->voice_resampler_pcmcb_record_mutex) {
        tsk_mutex_destroy(&(jb->voice_magr_t->voice_resampler_pcmcb_record_mutex));
    }
    //mix
        //record
    if (jb->voice_magr_t->voice_resampler_pcmcb_mix) {
        tsk_mutex_lock(jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex);
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->voice_magr_t->voice_resampler_pcmcb_mix;
        jb->voice_magr_t->voice_resampler_pcmcb_mix = nullptr;
#else
        speex_resampler_destroy(jb->voice_magr_t->voice_resampler_pcmcb_mix);
        jb->voice_magr_t->voice_resampler_pcmcb_mix = tsk_null;
#endif
        tsk_mutex_unlock(jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex);
    }
    if (jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex) {
        tsk_mutex_destroy(&(jb->voice_magr_t->voice_resampler_pcmcb_mix_mutex));
    }
    
    if( jb->transcriber_mgr_t->record_resampler )
    {
        tsk_mutex_lock(jb->transcriber_mgr_t->record_resampler_mutex);
        tdav_youme_neteq_jitterbuffer_delete_resampler( &(jb->transcriber_mgr_t->record_resampler));
        tsk_mutex_unlock(jb->transcriber_mgr_t->record_resampler_mutex);
    }
    if( jb->transcriber_mgr_t->record_resampler_mutex )
    {
        tsk_mutex_destroy(&(jb->transcriber_mgr_t->record_resampler_mutex));
    }
    
    if( jb->transcriber_mgr_t->remote_resampler )
    {
        tsk_mutex_lock(jb->transcriber_mgr_t->remote_resampler_mutex);
        tdav_youme_neteq_jitterbuffer_delete_resampler( &(jb->transcriber_mgr_t->remote_resampler));
        tsk_mutex_unlock(jb->transcriber_mgr_t->remote_resampler_mutex);
    }
    if( jb->transcriber_mgr_t->remote_resampler_mutex )
    {
        tsk_mutex_destroy(&(jb->transcriber_mgr_t->remote_resampler_mutex));
    }
    
    TSK_SAFE_FREE(jb->transcriber_mgr_t->voice_tempbuf_record);
    TSK_SAFE_FREE(jb->transcriber_mgr_t->voice_tempbuf_remote);
    
    
    if (jb->bkaud_magr_t->bkaud_filled_lst) {
        tsk_list_lock(jb->bkaud_magr_t->bkaud_filled_lst);
        tsk_list_clear_items(jb->bkaud_magr_t->bkaud_filled_lst);
        tsk_list_unlock(jb->bkaud_magr_t->bkaud_filled_lst);
        TSK_OBJECT_SAFE_FREE(jb->bkaud_magr_t->bkaud_filled_lst);
    }
    if (jb->bkaud_magr_t->bkaud_free_lst) {
        tsk_list_lock(jb->bkaud_magr_t->bkaud_free_lst);
        tsk_list_clear_items(jb->bkaud_magr_t->bkaud_free_lst);
        tsk_list_unlock(jb->bkaud_magr_t->bkaud_free_lst);
        TSK_OBJECT_SAFE_FREE(jb->bkaud_magr_t->bkaud_free_lst);
    }
    if (jb->bkaud_magr_t->bkaud_sema_lst) {
        tsk_semaphore_destroy(&jb->bkaud_magr_t->bkaud_sema_lst);
    }
    if (jb->bkaud_magr_t->bkaud_resampler) {
        tsk_mutex_lock(jb->bkaud_magr_t->bkaud_resampler_mutex);
#ifdef USE_WEBRTC_RESAMPLE
        delete jb->bkaud_magr_t->bkaud_resampler;
        jb->bkaud_magr_t->bkaud_resampler = nullptr;
#else
        speex_resampler_destroy(jb->bkaud_magr_t->bkaud_resampler);
        jb->bkaud_magr_t->bkaud_resampler = tsk_null;
#endif
        tsk_mutex_unlock(jb->bkaud_magr_t->bkaud_resampler_mutex);
    }
    if (jb->bkaud_magr_t->bkaud_resampler_mutex) {
        tsk_mutex_destroy(&(jb->bkaud_magr_t->bkaud_resampler_mutex));
    }
    
    tsk_mutex_lock(jb->micaud_magr_t->micaud_ringbuf_mutex);
    WebRtc_youme_FreeBuffer(jb->micaud_magr_t->micaud_ringbuf);
    jb->micaud_magr_t->micaud_ringbuf = tsk_null;
    tsk_mutex_unlock(jb->micaud_magr_t->micaud_ringbuf_mutex);
    tsk_mutex_destroy(&(jb->micaud_magr_t->micaud_ringbuf_mutex));
    jb->micaud_magr_t->micaud_ringbuf_mutex = tsk_null;
    
    tsk_mutex_lock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
    WebRtc_youme_FreeBuffer(jb->micaud_callback_magr_t->micaud_ringbuf);
    jb->micaud_callback_magr_t->micaud_ringbuf = tsk_null;
    tsk_mutex_unlock(jb->micaud_callback_magr_t->micaud_ringbuf_mutex);
    tsk_mutex_destroy(&(jb->micaud_callback_magr_t->micaud_ringbuf_mutex));
    jb->micaud_callback_magr_t->micaud_ringbuf_mutex = tsk_null;
    
    TSK_SAFE_FREE(jb->bkaud_magr_t->bkaud_tempbuf1);
    TSK_SAFE_FREE(jb->bkaud_magr_t->bkaud_tempbuf2);
    TSK_SAFE_FREE(jb->micaud_magr_t->micaud_tempbuf1);
    jb->micaud_magr_t->micaud_tempbuf1 = tsk_null;
    TSK_SAFE_FREE(jb->micaud_magr_t->micaud_tempbuf2);
    jb->micaud_magr_t->micaud_tempbuf2 = tsk_null;
    TSK_SAFE_FREE(jb->micaud_callback_magr_t->micaud_tempbuf1);
    jb->micaud_callback_magr_t->micaud_tempbuf1 = tsk_null;
    TSK_SAFE_FREE(jb->micaud_callback_magr_t->micaud_tempbuf2);
    jb->micaud_callback_magr_t->micaud_tempbuf2 = tsk_null;
    TSK_SAFE_FREE(jb->voice_magr_t->voice_tempbuf1);
    TSK_SAFE_FREE(jb->voice_magr_t->voice_tempbuf2);
    TSK_SAFE_FREE(jb->voice_magr_t->voice_tempbuf3);
    
    jb->farend_voice_level.callback = tsk_null;
    jb->farend_voice_level.count = 0;
    jb->farend_voice_level.last_level = 0;
    jb->farend_voice_level.max_level = 0;
    return 0;
}

//
//	youme jitterbufferr Plugin definition
//

/* constructor */
static tsk_object_t *tdav_youme_neteq_jitterbuffer_ctor (tsk_object_t *self, va_list *app)
{
    tdav_youme_neteq_jitterbuffer_t *jitterbuffer = (tdav_youme_neteq_jitterbuffer_t *)self;
    TSK_DEBUG_INFO ("Create youme neteq jitter buffer");
    
    if (jitterbuffer)
    {
        /* init base */
        tmedia_jitterbuffer_init (TMEDIA_JITTER_BUFFER (jitterbuffer));
        /* init self */
		jitterbuffer->jb_mgr_map = new jb_manager_map_t;
        jitterbuffer->bkaud_magr_t = (bkaud_manager_t*)tsk_malloc(sizeof(bkaud_manager_t));
        jitterbuffer->micaud_magr_t = (micaud_manager_t*)tsk_malloc(sizeof(micaud_manager_t));
        jitterbuffer->micaud_callback_magr_t = (micaud_manager_t*)tsk_malloc(sizeof(micaud_manager_t));
        jitterbuffer->transcriber_mgr_t = (transcriber_manager_t*)tsk_malloc( sizeof(transcriber_manager_t));
        jitterbuffer->voice_magr_t = (voice_manager_t*)tsk_malloc(sizeof(voice_manager_t));
        jitterbuffer->max_jb_num = tmedia_defaults_get_max_jitter_buffer_num();
        jitterbuffer->stat_calc_period_ms = tmedia_defaults_get_packet_stat_report_period_ms();
        jitterbuffer->stat_log_period_ms = tmedia_defaults_get_packet_stat_log_period_ms();
        jitterbuffer->mixed_callback_rate  = tmedia_defaults_get_mixed_callback_samplerate();
        jitterbuffer->nPCMCallbackSampleRate = 48000;
        jitterbuffer->nPCMCallbackChannel = 1;
        if (jitterbuffer->stat_calc_period_ms <= 0) {
            // If no packet statistics report is required, we may need to calculate the statistics for logging.
            jitterbuffer->stat_calc_period_ms = jitterbuffer->stat_log_period_ms;
        }
        
        jitterbuffer->isRemoteCallbackOn = false;  // 远端数据是否回调
        jitterbuffer->isRecordCallbackOn = false;  // 录音数据是否回调
        jitterbuffer->isMixCallbackOn = false;     // 远端和录音的混音是否回调
        
        jitterbuffer->isSaveScreen = false;
        jitterbuffer->isTranscriberOn = false;
        
        jitterbuffer->micaud_magr_t->micaud_ringbuf = tsk_null;
        jitterbuffer->micaud_magr_t->micaud_ringbuf_mutex = tsk_null;
        
        jitterbuffer->micaud_callback_magr_t->micaud_ringbuf = tsk_null;
        jitterbuffer->micaud_callback_magr_t->micaud_ringbuf_mutex = tsk_null;
        
        jitterbuffer->micaud_magr_t->micaud_tempbuf1 = tsk_null;
        jitterbuffer->micaud_magr_t->micaud_tempbuf2 = tsk_null;
        jitterbuffer->micaud_callback_magr_t->micaud_tempbuf1 = tsk_null;
        jitterbuffer->micaud_callback_magr_t->micaud_tempbuf2 = tsk_null;
        
        jitterbuffer->farend_voice_level.callback = tsk_null;
        jitterbuffer->farend_voice_level.count = 0;
        jitterbuffer->farend_voice_level.last_level = 0;
        jitterbuffer->farend_voice_level.max_level = 0;
    }
    else{
        return tsk_null;
    }
    
    if(!(jitterbuffer->audio_ouput = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create audio output list.");
        return tsk_null;
    }
    
    if(!(jitterbuffer->decoder_ouput = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create decoder output list.");
        return tsk_null;
    }
    
    if(!(jitterbuffer->audio_resample_list = tsk_list_create())) {
        TSK_DEBUG_ERROR("Failed to create audio_resample_list list.");
        return tsk_null;
    }
    
    return self;
}
/* destructor */
static tsk_object_t *tdav_youme_neteq_jitterbuffer_dtor (tsk_object_t *self)
{
    tdav_youme_neteq_jitterbuffer_t *jb = (tdav_youme_neteq_jitterbuffer_t *)self;
    if (jb)
    {
        /* deinit base */
        tmedia_jitterbuffer_deinit (TMEDIA_JITTER_BUFFER (jb));
        /* deinit self */
        if (NULL != jb->mix_meta) {
            for (uint32_t i = 0; i < jb->mix_meta_num; i++) {
                if (NULL != jb->mix_meta[i].bufferPtr) {
                    delete[] jb->mix_meta[i].bufferPtr;
                    jb->mix_meta[i].bufferPtr = NULL;
                }
            }
            delete[] jb->mix_meta;
            jb->mix_meta = NULL;
        }
        
		if (NULL != jb->jb_mgr_map) {
			jb_manager_map_t::iterator it;
			for (it = jb->jb_mgr_map->begin(); it != jb->jb_mgr_map->end(); it++) {
				free_jb_manager(&(it->second));
			}
			jb->jb_mgr_map->clear();
			delete jb->jb_mgr_map;
			jb->jb_mgr_map = NULL;
		}
    }
    else{
        return self;
    }
    
    
    TSK_OBJECT_SAFE_FREE(jb->audio_ouput);
    TSK_OBJECT_SAFE_FREE(jb->decoder_ouput);
    TSK_OBJECT_SAFE_FREE(jb->audio_resample_list);
    
    TSK_SAFE_FREE(jb->bkaud_magr_t);
    TSK_SAFE_FREE(jb->micaud_magr_t);
    TSK_SAFE_FREE(jb->micaud_callback_magr_t);
    TSK_SAFE_FREE(jb->voice_magr_t);
    TSK_SAFE_FREE(jb->transcriber_mgr_t);
    
    return self;
}
/* object definition */
static const tsk_object_def_t tdav_youme_neteq_jitterbuffer_def_s = {
    sizeof (tdav_youme_neteq_jitterbuffer_t),
    tdav_youme_neteq_jitterbuffer_ctor,
    tdav_youme_neteq_jitterbuffer_dtor,
    tsk_null,
};
/* plugin definition*/
static const tmedia_jitterbuffer_plugin_def_t tdav_youme_neteq_jitterbuffer_plugin_def_s = {
    &tdav_youme_neteq_jitterbuffer_def_s,
    tmedia_audio,
    "Youme neteq jitter buffer",
    
    tdav_youme_neteq_jitterbuffer_set_param,
    tdav_youme_neteq_jitterbuffer_get_param,
    tdav_youme_neteq_jitterbuffer_open,
    tdav_youme_neteq_jitterbuffer_tick,
    tdav_youme_neteq_jitterbuffer_put,
    tdav_youme_neteq_jitterbuffer_get,
    tdav_youme_neteq_jitterbuffer_reset,
    tdav_youme_neteq_jitterbuffer_close,
    tdav_youme_neteq_jitterbuffer_put_bkaudio,
};
const tmedia_jitterbuffer_plugin_def_t *tdav_youme_neteq_jitterbuffer_plugin_def_t = &tdav_youme_neteq_jitterbuffer_plugin_def_s;

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

/**@file tdav_session_audio.h
 * @brief Audio Session plugin.
 *
*
 *
 */
#ifndef TINYDAV_SESSION_AUDIO_H
#define TINYDAV_SESSION_AUDIO_H

#include "tinydav_config.h"

#include "tinydav/tdav_session_av.h"
#include "XTimerCWrapper.h"
#include "tsk_thread.h"
#include "tsk_semaphore.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinydav/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.h"
#include "webrtc/common_audio/ring_buffer.h"      // Use ring buffer open source function
#include "tinydav/codecs/mixer/speex_resampler.h" // Use speex open source function
#include "tdav_audio_unit.h"

TDAV_BEGIN_DECLS

typedef tsk_list_t tdav_session_audio_dtmfe_L_t;

#ifndef OS_LINUX
#define MAX_BUF_FRAME_NUM    (20)
#define MAX_BKAUD_ITEM_NUM    (10)
#define MAX_MIXAUD_ITEM_NUM   (20)
#define MAX_MIXAUD_ITEM_TIMEOUT_NUM  (5)
#define MAX_FRAME_SIZE       (5760) // 48KHz, 60ms: 480 * 6 * 2
#define MAX_TEMP_BUF_BYTESZ (1920)
#define MAX_DELAY_MS        (540)
#else
#define MAX_BUF_FRAME_NUM    (20)
#define MAX_BKAUD_ITEM_NUM    (10)
#define MAX_MIXAUD_ITEM_NUM   (10)
#define MAX_MIXAUD_ITEM_TIMEOUT_NUM  (5)
#define MAX_FRAME_SIZE       (1920) // 8KHz, 120ms: 80 * 12 * 2
#define MAX_TEMP_BUF_BYTESZ (1920)
#define MAX_DELAY_MS        (540)

#endif

typedef enum AVSessionBackgroundMusicMode {
    AVSESSION_BGM_TO_SPEAKER = 0,
    AVSESSION_BGM_TO_MIC = 1
} AVSessionBackgroundMusicMode_t;

typedef struct tdav_session_audio_frame_buffer_s
{
    TSK_DECLARE_OBJECT;
    
    uint8_t  bytes_per_sample;
    uint8_t  channel_num;
    uint8_t  is_float;
    uint8_t  is_interleaved;
    void  *pcm_data;
    uint32_t alloc_size;
    uint32_t pcm_size;
    uint32_t sample_rate;
    uint64_t     timeStamp;
    
} tdav_session_audio_frame_buffer_t;
extern const tsk_object_def_t *tdav_session_audio_frame_buffer_def_t;

typedef struct tdav_session_audio_vad_result_s
{
    TSK_DECLARE_OBJECT;
    
    uint8_t u1VadRes;
    int32_t i4SessionId;
} tdav_session_audio_vad_result_t;
extern const tsk_object_def_t *tdav_session_audio_vad_result_def_t;

typedef struct mic_mix_aud_delay_s
{
    int16_t i2SetDelay;
    int16_t i2TimePerBlk;
    int16_t i2ReadBlk;
    int16_t i2WriteBlk;
    int16_t i2MaxBlkNum;
    void    *pDelayProcBuf;
    void    *pDelayOutputBuf;
    tsk_mutex_handle_t *pDelayMutex;
} mic_mix_aud_delay_t;

// This timestamp info is for statistics purpose. One client sends a timestamp
// to the receivers, and the receivers bounce it back. To limit the network
// traffic, we only bounce MAX_TIMESTAMP_NUM timestamps on every packet. If there
// are more timestamps received during sending two packets, they will be ignored.
typedef struct stat_timestamp_info_s
{
    uint64_t timeWhenRecvMs; // local time when we receive this timestamp
    uint32_t timestampMs;
    int32_t  sessionId;
} stat_timestamp_info_t;
#define MAX_TIMESTAMP_NUM    (5)
// SessionID(int32_t) + RegionCode(uint8_t) + Timestamp(uint32_t)
#define BOUNCED_TIMESTAMP_INFO_SIZE    (sizeof(int32_t) + sizeof(uint8_t) + sizeof(uint32_t))

typedef void (*VadCallback_t)(int32_t sessionId, tsk_bool_t isSilence);
typedef void (*PcmCallback_t)(tdav_session_audio_frame_buffer_t* frameBuffer, int flag );
typedef void (*MicLevelCallback_t)(int32_t level);


typedef struct tdav_session_audio_s
{
    TDAV_DECLARE_SESSION_AV;

    tsk_bool_t is_started;

    struct
    {
        unsigned started : 1;
    } timer;

    struct
    {
        uint32_t payload_type;
        struct tmedia_codec_s *codec;

        void *buffer;
        tsk_size_t buffer_size;

        void *bwCtrlBuffer; // bandwidth control data buffer
        tsk_size_t bwCtrlBufferSize;

        void *rtpExtBuffer;
        tsk_size_t rtpExtBufferSize;

        int32_t codec_dtx_peroid_num;
        int32_t codec_dtx_current_num;

        struct
        {
            void *buffer;
            tsk_size_t buffer_size;
            struct tmedia_resampler_s *instance;
        } resampler;
        tsk_list_t *free_frame_buf_lst;
        tsk_list_t *filled_frame_buf_lst;
        tsk_semaphore_handle_t *filled_frame_sema;
        tsk_thread_handle_t *producer_thread_handle;
    } encoder;

    struct
    {
        uint32_t payload_type;
        struct tmedia_codec_s *codec;

        void *buffer;
        tsk_size_t buffer_size;

        struct
        {
            void *buffer;
            tsk_size_t buffer_size;
            struct tmedia_resampler_s *instance;
        } resampler;
    } decoder;
    
    struct
    {
        uint32_t               mixaud_enable;
        uint32_t               mixaud_rate;
        uint32_t               mixaud_filled_position;
        tsk_list_item_t        *mixaud_lstitem_backup;
#ifdef USE_WEBRTC_RESAMPLE
        void                    *mixaud_resampler_1;
        void                    *mixaud_resampler_2;
        void                    *mixaud_resampler_3;
#else
        SpeexResamplerState     *mixaud_resampler_1;
        SpeexResamplerState     *mixaud_resampler_2;
        SpeexResamplerState     *mixaud_resampler_3;
#endif
        tsk_mutex_handle_t     *mixaud_resampler_mutex_1;
        tsk_mutex_handle_t     *mixaud_resampler_mutex_2;
        tsk_mutex_handle_t     *mixaud_resampler_mutex_3;
        tsk_list_t             *mixaud_free_lst;
        tsk_list_t             *mixaud_filled_lst;
        tsk_semaphore_handle_t *mixaud_sema_lst;
        
        void                   *mixaud_tempbuf1;
        void                   *mixaud_tempbuf2;
        void                   *mixaud_tempbuf3;
        void                   *mixaud_tempbuf4;
        void                   *mixaud_spkoutbuf;
        void                   *mixaud_pcm_cb_buff;
        mic_mix_aud_delay_t    *mixaud_delayproc;
        
        uint32_t mixau_timeout_count;
    } mic_mix_aud;
    
    struct
    {
        BandwidthCtrlMsg_t received;
        BandwidthCtrlMsg_t to_send;
        TSK_DECLARE_SAFEOBJ;
    } bc;

    struct
    {
        // the time when the audio session was initialized
        uint64_t time_base_ms;
        // the last time when we sent a timestamp
        uint64_t last_send_timestamp_time_ms;
        uint32_t send_timestamp_period_ms;
        // If we didn't got any packet from others, it doesn't make sense to send a timestamp,
        // because we will not receive any feedback.
        tsk_bool_t may_send_timestamp;
        // A buffer for holding timestamps from the senders, these timestamps will be bounced back
        // to the senders later.
        stat_timestamp_info_t timestamps[MAX_TIMESTAMP_NUM];
        // Number of timestamps held in the array "timestamps" above
        uint32_t timestamp_num;
        // A buffer for encoding the timestamps into the RTP headder extension
        uint8_t  timestampBouncedBuf[BOUNCED_TIMESTAMP_INFO_SIZE * MAX_TIMESTAMP_NUM];
        TSK_DECLARE_SAFEOBJ;
    } stat;

    struct tmedia_denoise_s *denoise;
    struct tmedia_jitterbuffer_s *jitterbuffer;
    int32_t mixAudioVol; // Volume (range: 0-100) of the audio track that's going to be mixed into the speaker/microphone.
    int32_t mixLastVol;
    tsk_bool_t isMicMute;
    tsk_bool_t isSpeakerMute;
    float      micVolumeGain;
    tsk_bool_t micBypassToSpk;  // Bypass microphone audio to the speaker output
    tsk_bool_t bgmBypassToSpk;  // Bypass backgroundmusic audio to the speaker output
    tsk_bool_t isHeadsetPlugin; // Headset plugin flag
    tsk_bool_t isPcmCallback; // Mix the microphone input with speaker output and callback the mixed data to the user
    int32_t recOverflowCount;
    
    tsk_bool_t isRemoteCallbackOn;  // 远端数据是否回调
    tsk_bool_t isRecordCallbackOn;  // 录音数据是否回调
    tsk_bool_t isMixCallbackOn;     // 远端和录音的混音是否回调
    PcmCallback_t pcmCallback;      // pcm回调，用于本地mic录音 pcm回调
    
    int32_t nPCMCallbackChannel;
    int32_t nPCMCallbackSampleRate;
    
    tsk_bool_t isSaveScreen;    //录屏的情况下，要保存录音数据和远端数据
    tsk_bool_t isTranscriberOn; //是否开启会议记录
    
    
    tdav_session_audio_dtmfe_L_t *dtmf_events;
    tsk_bool_t is_sending_dtmf_events;
//    FILE *m_pReceiveFile;
//    FILE* m_pOpusFile;
#ifdef MAC_OS
    FILE* m_pOpusEncodedFile;
#endif
    
    struct
    {
        uint32_t   len_in_frame_num;
        uint32_t   silence_percent;
        tsk_bool_t pre_frame_silence;
        int32_t    total_frame_count;
        int32_t    silence_frame_count;
        VadCallback_t callback;
        
        // for debug
        int32_t  group_count;
        int32_t  status_sent_count;
        int32_t  status_recv_count;
    } vad;
    
    struct
    {
        int32_t max_level;
        MicLevelCallback_t callback;
        int32_t last_level;
    } mic_level;
    
    struct
    {
        uint32_t max_size;
        FILE* mic;
        uint32_t mic_size;
        FILE* speaker;
        uint32_t speaker_size;
        FILE* send_pcm;
        uint32_t send_pcm_size;
    } pcm_file;
    
    tdav_audio_resample_t* resample;
	//RSCODE 
	tsk_list_t* rscode_list;

	//合包个数
	int iCombineCount;
	tsk_list_t* combinePacket_list;

} tdav_session_audio_t;

#define TDAV_SESSION_AUDIO(self) ((tdav_session_audio_t *)(self))


int tdav_session_audio_rscode_cb(tdav_session_audio_t *audio, const struct trtp_rtp_packet_s *packet);

TINYDAV_GEXTERN const tmedia_session_plugin_def_t *tdav_session_audio_plugin_def_t;
TINYDAV_GEXTERN const tmedia_session_plugin_def_t *tdav_session_bfcpaudio_plugin_def_t;

TDAV_END_DECLS

#endif /* TINYDAV_SESSION_AUDIO_H */

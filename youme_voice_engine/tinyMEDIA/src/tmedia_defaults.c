
#include "tinymedia/tmedia_defaults.h"
#include "tinyrtp/rtp/trtp_rtp_header.h"
#include "tsk_string.h"
#include "tsk_debug.h"
#include <limits.h>

// 这些全局变量为所有会话共享，可以使用"session_set()"族函数设置特定会话的参数

static tmedia_profile_t __profile = tmedia_profile_default;

static tmedia_bandwidth_level_t __bl = tmedia_bl_unrestricted;

//是否启用拥塞控制
static tsk_bool_t __congestion_ctrl_enabled = tsk_false;

//video相关参数
static int32_t __video_fps = 15; // allowed values: ]0 - 120]
static int32_t __video_fps_child = 15; // allowed values: ]0 - 120]
static int32_t __video_fps_share = 15; // allowed values: ]0 - 120]
static int32_t __video_motion_rank = 2; // allowed values: 1(low), 2(medium) or 4(high)
static int32_t __bw_video_up_max_kbps = INT_MAX; // <= 0: unrestricted, Unit: kbps
static int32_t __bw_video_down_max_kbps = INT_MAX; // <= 0: unrestricted, Unit: kbps
//static tmedia_pref_video_size_t __pref_video_size = tmedia_pref_video_size_cif; // 352 x 288: Android, iOS, WP7
//static tmedia_pref_video_size_t __pref_video_size = tmedia_pref_video_size_qcif; // 176 x 144: Android, iOS, WP7
static tmedia_pref_video_size_t __pref_video_size = tmedia_pref_video_size_sqvga; // 160 x 120: Android, iOS, WP7
//static tmedia_pref_video_size_t __pref_video_size = tmedia_pref_video_size_qvga; // 320 x 240

// disable
static int32_t __jb_margin_ms = -1;

// -1: disable 4: default for speex
static int32_t __jb_max_late_rate_percent = -1;

//声学回声消除相关参数
static uint32_t __echo_tail = 100;

static uint32_t __echo_skew = 0;

static tsk_bool_t __aec_enabled = tsk_true;

// AEC option, 0: AECM, 1: AEC
static uint32_t __aec_option = 0;

//自动增益控制相关参数
static tsk_bool_t __agc_enabled = tsk_true;

static float __agc_level = 8000.0f;

static int32_t __agc_target_level = 12;

static int32_t __agc_compress_gain = 18;

//自动静音检测相关参数
static tsk_bool_t __vad_enabled = tsk_true;

//噪音消除相关参数
static tsk_bool_t __noise_supp_enabled = tsk_true;

//啸叫消除相关参数
static tsk_bool_t __howling_supp_enabled = tsk_false;

//Reverb Enable
static tsk_bool_t __reverb_filter_enabled = tsk_false;

static tsk_bool_t __100rel_enabled = tsk_false;

static int32_t __sx = -1;

static int32_t __sy = -1;

static int32_t __audio_producer_gain = 0;

static int32_t __audio_consumer_gain = 0;

static tsk_bool_t __audio_faked_consumer = tsk_false;

static int32_t __audio_channels_playback = 1;

static int32_t __audio_channels_record = 1;

static int32_t __audio_ptime = 20;

// RTP相关参数
// RTP端口起始值
static uint16_t __rtp_port_range_start = 1024;

// RTP端口终止值
static uint16_t __rtp_port_range_stop = 65535;

// This option is force symetric RTP for remote size. Local: always ON
static tsk_bool_t __rtp_symetric_enabled = tsk_false;

//媒体类型
static tmedia_type_t __media_type = tmedia_audio;

static int32_t __volume = 100;

// pref. camera(index=1) and sound card(index=0) friendly names (e.g. Logitech HD Pro Webcam C920).
static char *__producer_friendly_name[3] = { tsk_null /*audio*/, tsk_null /*video*/, tsk_null /*bfcpvideo*/ };

// Session Timers: 0: disabled
static int32_t __inv_session_expires = 0;

static char *__inv_session_refresher = tsk_null;




static tsk_bool_t __bypass_encoding_enabled = tsk_false;

static tsk_bool_t __bypass_decoding_enabled = tsk_false;

static tsk_bool_t __videojb_enabled = tsk_true;
static tsk_bool_t __video_zeroartifacts_enabled = tsk_false; // Requires from remote parties to support AVPF (RTCP-FIR/NACK/PLI)
// Network buffer size used for RTP (SO_RCVBUF, SO_SNDBUF)
static tsk_size_t __rtpbuff_size = 0x200000;

// Min size for tail used to honor RTCP-NACK requests
static tsk_size_t __avpf_tail_min = 20;

// Max size for tail used to honor RTCP-NACK requests
static tsk_size_t __avpf_tail_max = 160;

// Whether to use AVPF instead of AVP or negotiate. FIXME
static tmedia_mode_t __avpf_mode = tmedia_mode_optional;

// Microphone record sample rate
static int32_t __record_sample_rate = 16000;
static int __record_sample_rate_setted = 0;

// Playback sample rate
static int32_t __playback_sample_rate = 16000;
static int __playback_sample_rate_setted = 0;

// Mixed Callback sample rate
#ifdef ANDROID
static int32_t __mixed_callback_sample_rate = 44100;
#else
static int32_t __mixed_callback_sample_rate = 48000;
#endif

// If enable opus in-band FEC
static tsk_bool_t __opus_inband_fec_enabled = tsk_false;

// If enable opus out-band FEC
static tsk_bool_t __opus_outband_fec_enabled = tsk_false;

static int32_t __opus_packet_loss_perc = 10;

static int        __opus_dtx_peroid = 100;
static int        __opus_complexity = 5;
static tsk_bool_t __opus_encoder_vbr_enabled = tsk_true;
static int        __opus_encoder_max_bandwidth = 1105;
static int        __opus_encoder_bitrate = 25000;
static int        __rtp_feedback_period = 0;

static char *__ssl_certs_priv_path = tsk_null;

static char *__ssl_certs_pub_path = tsk_null;

static char *__ssl_certs_ca_path = tsk_null;

static tsk_bool_t __ssl_certs_verify = tsk_false;

// Maximum number of FDs this process is allowed to open. Zero to disable.
static tsk_size_t __max_fds = 0;

// If it's ture, the recording will not be actually initialized, instead, a "faked" recording
// thread will be created and keep outputting silence frames. 
static tsk_bool_t __android_faked_recording = tsk_false;

// iOS specific configurations
static tsk_bool_t __ios_no_permission_no_rec = tsk_true;
static tsk_bool_t __ios_airplay_no_rec = tsk_true;
static tsk_bool_t __ios_always_play_category = tsk_true;
static tsk_bool_t __ios_need_set_mode = tsk_true;
static uint32_t   __ios_recovery_delay_from_call = 0;

// File path for the application specific path, this is OS dependent.
// Typically we write log files into this path.
static char __app_document_path[1024] = "";

static uint32_t   __pcm_file_size_kb = 0;
static uint32_t   __h264_file_size_kb = 0;
static uint32_t   __vad_len_ms = 1000;
static uint32_t   __vad_silence_percent = 20;
static tsk_bool_t __comm_mode_enabled = tsk_true;
static tsk_bool_t __android_audio_path_under_opensles = tsk_true;

static uint32_t   __background_music_delay = 0;
static uint32_t   __max_jitter_buffer_num = 10;
static uint32_t   __max_neteq_delay_ms = 1000;
static uint32_t   __packet_stat_log_period_ms = 0;
static uint32_t   __packet_stat_report_period_ms = 0;
static uint32_t   __video_packet_stat_report_period_ms = 0;
static uint32_t   __mic_volume_gain = 100;
static uint32_t   __spkout_fadein_time_ms = 6000;

static int        __camera_rotation = 90;

// 默认本地渲染分辨率为720p（外部输入版本时会调用setVideoNetResolution接口重置）
#ifdef _WIN32
static int __video_camera_width =  640;
static int __video_camera_height =  480;
#else
static int __video_camera_width = 720;
static int __video_camera_height = 1280;
#endif

static int __video_video_size_set = 0;
static int __video_video_code_width = 320;
static int __video_video_code_height = 240;

static int __video_video_size_set_child = 0;
static int __video_video_code_width_child = 0;
static int __video_video_code_height_child = 0;

static int __video_video_size_set_share = 0;
static int __video_video_code_width_share = 0;
static int __video_video_code_height_share = 0;

static tsk_bool_t __video_hwcodec_enabled = tsk_true;
static int    __video_encoder_bitrate = 0;
static int    __video_current_bitrate = 0;

static int    __video_encoder_bitrate_child = 0;
static int    __video_current_bitrate_child = 0;

static int    __video_encoder_bitrate_share = 0;

//默认不采用quality模式
static int    __video_encoder_quality_max = -1;
static int    __video_encoder_quality_min = -1;

static int    __video_encoder_quality_max_child = -1;
static int    __video_encoder_quality_min_child = -1;

static int    __video_encoder_quality_max_share = -1;
static int    __video_encoder_quality_min_share = -1;

static uint32_t __video_noframe_monitor_timeout = 4000; // 视频无渲染帧超时回调时间

static tsk_bool_t __external_input_mode = tsk_false;

#if WIN32
static int __record_device_id = -1 ;
#else
static int __record_device_id = 0;
#endif
static void* __video_egl_context = tsk_null;

static void* __video_egl_share_context = tsk_null;

static tsk_bool_t __video_frame_raw_cb_enabeld = tsk_false;     // for video encode and decode

static tsk_bool_t __video_decode_raw_cb_enabeld = tsk_false;    // only for video decode

static int __video_share_type = 0;

static int __video_net_adjust_mode = 0;

static int __video_capture_sampling_mode = 1;

static int __sfu_max_video_encoder_bitrate_level = 100;
static int __sfu_min_video_encoder_bitrate_level = 50;

static int __p2p_max_video_encoder_bitrate_level = 120;

static uint32_t   __video_default_bitrate_multiple = 5;
static uint32_t   __video_refresh_timeout = 500;

static int __video_encode_adjust_mode = 0; // 0: inter adjust, 1: extern adjust

static uint32_t __max_rtcp_member_count = 10;

static uint32_t __video_nack_flag = 0;  // nack 开关，按bit位设置，bit 0: 接收端半程nack; bit 1: 全程FIR

int tmedia_defaults_set_profile (tmedia_profile_t profile)
{
    __profile = profile;
    return 0;
}
tmedia_profile_t tmedia_defaults_get_profile ()
{
    return __profile;
}

// @deprecated
int tmedia_defaults_set_bl (tmedia_bandwidth_level_t bl)
{
    __bl = bl;
    return 0;
}
// @deprecated
tmedia_bandwidth_level_t tmedia_defaults_get_bl ()
{
    return __bl;
}

int tmedia_defaults_set_congestion_ctrl_enabled (tsk_bool_t enabled)
{
    __congestion_ctrl_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_congestion_ctrl_enabled ()
{
    return __congestion_ctrl_enabled;
}

// video接口
int tmedia_defaults_set_videojb_enabled(tsk_bool_t enabled){
    __videojb_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_videojb_enabled(){
    return __videojb_enabled;
}

int tmedia_defaults_set_video_zeroartifacts_enabled(tsk_bool_t enabled){
    __video_zeroartifacts_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_video_zeroartifacts_enabled(){
    return __video_zeroartifacts_enabled;
}

int tmedia_defaults_set_video_fps(int32_t video_fps){
    if(video_fps > 0 && video_fps <= 120){
        __video_fps = video_fps;
        return 0;
    }
    TSK_DEBUG_ERROR("%d not valid for video fps", video_fps);
    return -1;
}
int32_t tmedia_defaults_get_video_fps(){
    return __video_fps;
}


int tmedia_defaults_set_video_fps_child(int32_t video_fps){
    if(video_fps > 0 && video_fps <= 120){
        __video_fps_child = video_fps;
        return 0;
    }
    TSK_DEBUG_ERROR("%d not valid for video fps", video_fps);
    return -1;
}
int32_t tmedia_defaults_get_video_fps_child(){
    return __video_fps_child;
}

int tmedia_defaults_set_video_fps_share(int32_t video_fps){
    if(video_fps > 0 && video_fps <= 120){
        __video_fps_share = video_fps;
        return 0;
    }
    TSK_DEBUG_ERROR("%d not valid for video fps", video_fps);
    return -1;
}

int32_t tmedia_defaults_get_video_fps_share(){
    return __video_fps_share;
}

int tmedia_defaults_set_video_motion_rank(int32_t video_motion_rank){
    switch(video_motion_rank){
        case 1/*low*/: case 2/*medium*/: case 4/*high*/: __video_motion_rank = video_motion_rank; return 0;
        default: TSK_DEBUG_ERROR("%d not valid for video motion rank. Must be 1, 2 or 4", video_motion_rank); return -1;
    }
}
int32_t tmedia_defaults_get_video_motion_rank(){
    return __video_motion_rank;
}

int tmedia_defaults_set_bandwidth_video_upload_max(int32_t bw_video_up_max_kbps){
    __bw_video_up_max_kbps = bw_video_up_max_kbps > 0 ? bw_video_up_max_kbps : INT_MAX;
    return 0;
}
int32_t tmedia_defaults_get_bandwidth_video_upload_max(){
    return __bw_video_up_max_kbps;
}

int tmedia_defaults_set_bandwidth_video_download_max(int32_t bw_video_down_max_kbps){
    __bw_video_down_max_kbps = bw_video_down_max_kbps > 0 ? bw_video_down_max_kbps : INT_MAX;
    return 0;
}
int32_t tmedia_defaults_get_bandwidth_video_download_max(){
    return __bw_video_down_max_kbps;
}

int tmedia_defaults_set_pref_video_size(int pref_video_size){
    __pref_video_size = (tmedia_pref_video_size_t)pref_video_size;
    return 0;
}

int tmedia_defaults_set_jb_margin (int32_t jb_margin_ms)
{
    __jb_margin_ms = jb_margin_ms;
    return __jb_margin_ms;
}

int32_t tmedia_defaults_get_jb_margin ()
{
    return __jb_margin_ms;
}

int tmedia_defaults_set_jb_max_late_rate (int32_t jb_max_late_rate_percent)
{
    __jb_max_late_rate_percent = jb_max_late_rate_percent;
    return 0;
}

int32_t tmedia_defaults_get_jb_max_late_rate ()
{
    return __jb_max_late_rate_percent;
}

int tmedia_defaults_set_echo_tail (uint32_t echo_tail)
{
    __echo_tail = echo_tail;
    return 0;
}

int tmedia_defaults_set_echo_skew (uint32_t echo_skew)
{
    __echo_skew = echo_skew;
    return 0;
}

uint32_t tmedia_defaults_get_echo_tail ()
{
    return __echo_tail;
}

uint32_t tmedia_defaults_get_echo_skew ()
{
    return __echo_skew;
}

int tmedia_defaults_set_aec_enabled (tsk_bool_t aec_enabled)
{
    __aec_enabled = aec_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_aec_enabled ()
{
    return __aec_enabled;
}

int tmedia_defaults_set_aec_option (uint32_t aec_option)
{
    __aec_option = aec_option;
    return 0;
}

uint32_t tmedia_defaults_get_aec_option ()
{
    return __aec_option;
}

int tmedia_defaults_set_agc_enabled (tsk_bool_t agc_enabled)
{
    __agc_enabled = agc_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_agc_enabled ()
{
    return __agc_enabled;
}

int tmedia_defaults_set_agc_level (float agc_level)
{
    __agc_level = agc_level;
    return 0;
}

float tmedia_defaults_get_agc_level ()
{
    return __agc_level;
}

int tmedia_defaults_set_agc_target_level (int32_t agc_target_level)
{
    __agc_target_level = agc_target_level;
    return 0;
}

int tmedia_defaults_get_agc_target_level ()
{
    return __agc_target_level;
}

int tmedia_defaults_set_agc_compress_gain (int32_t agc_compress_gain)
{
    __agc_compress_gain = agc_compress_gain;
    return 0;
}

int tmedia_defaults_get_agc_compress_gain ()
{
    return __agc_compress_gain;
}

int tmedia_defaults_set_vad_enabled (tsk_bool_t vad_enabled)
{
    __vad_enabled = vad_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_vad_enabled ()
{
    return __vad_enabled;
}

int tmedia_defaults_set_noise_supp_enabled (tsk_bool_t noise_supp_enabled)
{
    __noise_supp_enabled = noise_supp_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_noise_supp_enabled ()
{
    return __noise_supp_enabled;
}

int tmedia_defaults_set_howling_supp_enabled (tsk_bool_t howling_supp_enabled)
{
    __howling_supp_enabled = howling_supp_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_howling_supp_enabled ()
{
    return __howling_supp_enabled;
}

int tmedia_defaults_set_reverb_filter_enabled (tsk_bool_t reverb_filter_enabled)
{
    __reverb_filter_enabled = reverb_filter_enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_reverb_filter_enabled ()
{
    return __reverb_filter_enabled;
}

int tmedia_defaults_set_100rel_enabled (tsk_bool_t _100rel_enabled)
{
    __100rel_enabled = _100rel_enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_100rel_enabled ()
{
    return __100rel_enabled;
}

int tmedia_defaults_set_screen_size (int32_t sx, int32_t sy)
{
    __sx = sx;
    __sy = sy;
    return 0;
}

int32_t tmedia_defaults_get_screen_x ()
{
    return __sx;
}

int32_t tmedia_defaults_get_screen_y ()
{
    return __sy;
}


int tmedia_defaults_set_audio_ptime (int32_t audio_ptime)
{
    if (audio_ptime > 0)
    {
        __audio_ptime = audio_ptime;
        return 0;
    }
    TSK_DEBUG_ERROR ("Invalid parameter");
    return -1;
}
int32_t tmedia_defaults_get_audio_ptime ()
{
    return __audio_ptime;
}
int tmedia_defaults_set_audio_channels (int32_t channels_playback, int32_t channels_record)
{
    if (channels_playback != 1 && channels_playback != 2)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (channels_record != 1 && channels_record != 2)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    __audio_channels_playback = channels_playback;
    __audio_channels_record = channels_record;
    return 0;
}
int32_t tmedia_defaults_get_audio_channels_playback ()
{
    return __audio_channels_playback;
}
int32_t tmedia_defaults_get_audio_channels_record ()
{
    return __audio_channels_record;
}

int tmedia_defaults_set_audio_gain (int32_t audio_producer_gain, int32_t audio_consumer_gain)
{
    __audio_producer_gain = audio_producer_gain;
    __audio_consumer_gain = audio_consumer_gain;
    return 0;
}

int32_t tmedia_defaults_get_audio_producer_gain ()
{
    return __audio_producer_gain;
}

int32_t tmedia_defaults_get_audio_consumer_gain ()
{
    return __audio_consumer_gain;
}

int tmedia_defaults_set_audio_faked_consumer (tsk_boolean_t audio_faked_consumer)
{
    __audio_faked_consumer = audio_faked_consumer;
    return 0;
}

tsk_bool_t tmedia_defaults_get_audio_faked_consumer ()
{
    return __audio_faked_consumer;
}

uint16_t tmedia_defaults_get_rtp_port_range_start ()
{
    return __rtp_port_range_start;
}

uint16_t tmedia_defaults_get_rtp_port_range_stop ()
{
    return __rtp_port_range_stop;
}
int tmedia_defaults_set_rtp_port_range (uint16_t start, uint16_t stop)
{
    if (start < 1024 || stop < 1024 || start >= stop)
    {
        TSK_DEBUG_ERROR ("Invalid parameter: (%u < 1024 || %u < 1024 || %u >= %u)", start, stop, start, stop);
        return -1;
    }
    __rtp_port_range_start = start;
    __rtp_port_range_stop = stop;
    return 0;
}

int tmedia_defaults_set_rtp_symetric_enabled (tsk_bool_t enabled)
{
    __rtp_symetric_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_rtp_symetric_enabled ()
{
    return __rtp_symetric_enabled;
}

tmedia_type_t tmedia_defaults_get_media_type ()
{
    return __media_type;
}

int tmedia_defaults_set_media_type (tmedia_type_t media_type)
{
    __media_type = media_type;
    return 0;
}

int tmedia_defaults_set_volume (int32_t volume)
{
    __volume = TSK_CLAMP (0, volume, 100);
    return 0;
}
int32_t tmedia_defaults_get_volume ()
{
    return __volume;
}

int tmedia_producer_set_friendly_name (tmedia_type_t media_type, const char *friendly_name)
{
    if (media_type != tmedia_audio && media_type != tmedia_video && media_type != tmedia_bfcp_video)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    tsk_strupdate (&__producer_friendly_name[(media_type == tmedia_audio) ? 0 : (media_type == tmedia_bfcp_video ? 2 : 1)], friendly_name);
    return 0;
}
const char *tmedia_producer_get_friendly_name (tmedia_type_t media_type)
{
    if (media_type != tmedia_audio && media_type != tmedia_video && media_type != tmedia_bfcp_video)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }
    return __producer_friendly_name[(media_type == tmedia_audio) ? 0 : (media_type == tmedia_bfcp_video ? 2 : 1)];
}

int32_t tmedia_defaults_get_inv_session_expires ()
{
    return __inv_session_expires;
}
int tmedia_defaults_set_inv_session_expires (int32_t timeout)
{
    if (timeout >= 0)
    {
        __inv_session_expires = timeout;
        return 0;
    }
    TSK_DEBUG_ERROR ("Invalid parameter");
    return -1;
}

const char *tmedia_defaults_get_inv_session_refresher ()
{
    return __inv_session_refresher;
}
int tmedia_defaults_set_inv_session_refresher (const char *refresher)
{
    tsk_strupdate (&__inv_session_refresher, refresher);
    return 0;
}


int tmedia_defaults_set_bypass_encoding (tsk_bool_t enabled)
{
    __bypass_encoding_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_bypass_encoding ()
{
    return __bypass_encoding_enabled;
}

int tmedia_defaults_set_bypass_decoding (tsk_bool_t enabled)
{
    __bypass_decoding_enabled = enabled;
    return 0;
}
tsk_bool_t tmedia_defaults_get_bypass_decoding ()
{
    return __bypass_decoding_enabled;
}

int tmedia_defaults_set_rtpbuff_size (tsk_size_t rtpbuff_size)
{
    __rtpbuff_size = rtpbuff_size;
    return 0;
}
tsk_size_t tmedia_defaults_get_rtpbuff_size ()
{
    return __rtpbuff_size;
}

int tmedia_defaults_set_avpf_tail (tsk_size_t tail_min, tsk_size_t tail_max)
{
    __avpf_tail_min = tail_min;
    __avpf_tail_max = tail_max;
    return 0;
}
tsk_size_t tmedia_defaults_get_avpf_tail_min ()
{
    return __avpf_tail_min;
}
tsk_size_t tmedia_defaults_get_avpf_tail_max ()
{
    return __avpf_tail_max;
}

int tmedia_defaults_set_avpf_mode (enum tmedia_mode_e mode)
{
    __avpf_mode = mode;
    return 0;
}
enum tmedia_mode_e tmedia_defaults_get_avpf_mode ()
{
    return __avpf_mode;
}

int tmedia_defaults_set_record_sample_rate (int32_t record_sample_rate,  int forceSet)
{
    if( forceSet == 1 || __record_sample_rate_setted == 0 )
    {
        if( forceSet  == 1 )
        {
            __record_sample_rate_setted = 1;
        }
        
        switch (record_sample_rate)
        {
            case 8000:
            case 12000:
            case 16000:
            case 24000:
            case 32000:
            case 44100:
            case 48000:
                __record_sample_rate = record_sample_rate;
                break;
            default:
                TSK_DEBUG_ERROR ("%u not valid for record_sample_rate", record_sample_rate);
                return -1;
        }
    }
    
#ifdef MAC_OS
    //macos 播放与采集，必须使用同样的采样率
    tmedia_defaults_set_playback_sample_rate( record_sample_rate, forceSet );
#endif
    
    return 0;
}
int32_t tmedia_defaults_get_record_sample_rate ()
{
    return __record_sample_rate;
}

int tmedia_defaults_set_playback_sample_rate (int32_t playback_sample_rate,  int forceSet )
{
#ifdef MAC_OS
    //macos 播放与录音，必须使用同样的采样率,如果设置的playback_sample_rate与录音采样率不一样，就不设置
    if( playback_sample_rate != tmedia_defaults_get_record_sample_rate() )
    {
        return 0;
    }
#endif
    
    if( forceSet == 1 || __playback_sample_rate_setted == 0 )
    {
        if( forceSet == 1 )
        {
            __playback_sample_rate_setted = 1;
        }
        
        switch (playback_sample_rate)
        {
            case 8000:
            case 12000:
            case 16000:
            case 24000:
            case 32000:
            case 44100:
            case 48000:
                __playback_sample_rate = playback_sample_rate;
                return 0;
            default:
                TSK_DEBUG_ERROR ("%u not valid for playback_sample_rate", playback_sample_rate);
                return -1;
        }
    }
    
    return 0;
}
int32_t tmedia_defaults_get_playback_sample_rate ()
{
    return __playback_sample_rate;
}

int tmedia_defaults_set_mixed_callback_samplerate(int32_t mixedCallbackSamplerate) {
    __mixed_callback_sample_rate = mixedCallbackSamplerate;
    return 0;
}

int32_t tmedia_defaults_get_mixed_callback_samplerate() {
    return __mixed_callback_sample_rate;
}

int tmedia_defaults_set_opus_inband_fec_enabled (tsk_bool_t enabled)
{
    __opus_inband_fec_enabled = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_opus_inband_fec_enabled ()
{
    return __opus_inband_fec_enabled;
}

int tmedia_defaults_set_opus_outband_fec_enabled (tsk_bool_t enabled)
{
    __opus_outband_fec_enabled = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_opus_outband_fec_enabled ()
{
    return __opus_outband_fec_enabled;
}

int tmedia_defaults_set_opus_packet_loss_perc(int packet_loss_perc)
{
    __opus_packet_loss_perc = packet_loss_perc;
    return 0;
}

int tmedia_defaults_get_opus_packet_loss_perc()
{
    return __opus_packet_loss_perc;
}

int tmedia_defaults_set_opus_dtx_peroid(int32_t peroid)
{
    __opus_dtx_peroid = peroid;
    return 0;
}

int tmedia_defaults_get_opus_dtx_peroid()
{
    return __opus_dtx_peroid;
}

int tmedia_defaults_set_opus_encoder_complexity(int complexity)
{
    __opus_complexity = complexity;
    return 0;
}

int tmedia_defaults_get_opus_encoder_complexity()
{
    return __opus_complexity;
}

int tmedia_defaults_set_opus_encoder_vbr_enabled(tsk_bool_t enabled)
{
    __opus_encoder_vbr_enabled = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_opus_encoder_vbr_enabled()
{
    return __opus_encoder_vbr_enabled;
}

int tmedia_defaults_set_opus_encoder_max_bandwidth(int bandwidth)
{
    __opus_encoder_max_bandwidth = bandwidth;
    return 0;
}

int tmedia_defaults_get_opus_encoder_max_bandwidth()
{
    return __opus_encoder_max_bandwidth;
}

int tmedia_defaults_set_opus_encoder_bitrate(int bitrate)
{
    __opus_encoder_bitrate = bitrate;
    return 0;
}

int tmedia_defaults_get_opus_encoder_bitrate()
{
    return __opus_encoder_bitrate;
}

int tmedia_defaults_set_rtp_feedback_period(int period)
{
    __rtp_feedback_period = period;
    return 0;
}

int tmedia_defaults_get_rtp_feedback_period()
{
    return __rtp_feedback_period;
}

int tmedia_defaults_set_ssl_certs (const char *priv_path, const char *pub_path, const char *ca_path, tsk_bool_t verify)
{
    tsk_strupdate (&__ssl_certs_priv_path, priv_path);
    tsk_strupdate (&__ssl_certs_pub_path, pub_path);
    tsk_strupdate (&__ssl_certs_ca_path, ca_path);
    __ssl_certs_verify = verify;
    return 0;
}
int tmedia_defaults_get_ssl_certs (const char **priv_path, const char **pub_path, const char **ca_path, tsk_bool_t *verify)
{
    if (priv_path)
        *priv_path = __ssl_certs_priv_path;
    if (pub_path)
        *pub_path = __ssl_certs_pub_path;
    if (ca_path)
        *ca_path = __ssl_certs_ca_path;
    if (verify)
        *verify = __ssl_certs_verify;
    return 0;
}

int tmedia_defaults_set_max_fds (int32_t max_fds)
{
    if (max_fds > 0 && max_fds < 0xFFFF)
    {
        __max_fds = (tsk_size_t)max_fds;
        return 0;
    }
    return -1;
}
tsk_size_t tmedia_defaults_get_max_fds ()
{
    return __max_fds;
}

int tmedia_defaults_set_android_faked_recording (tsk_bool_t fakedRecording)
{
    __android_faked_recording = fakedRecording;
    return 0;
}

tsk_bool_t tmedia_defaults_get_android_faked_recording ()
{
    return __android_faked_recording;
}

int tmedia_defaults_set_ios_no_permission_no_rec (tsk_bool_t enabled)
{
    __ios_no_permission_no_rec = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_ios_no_permission_no_rec ()
{
    return __ios_no_permission_no_rec;
}

int tmedia_defaults_set_ios_airplay_no_rec (tsk_bool_t enabled)
{
    __ios_airplay_no_rec = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_ios_airplay_no_rec ()
{
    return __ios_airplay_no_rec;
}

int tmedia_defaults_set_ios_always_play_category (tsk_bool_t enabled)
{
    __ios_always_play_category = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_ios_always_play_category ()
{
    return __ios_always_play_category;
}

int tmedia_defaults_set_ios_need_set_mode (tsk_bool_t enabled)
{
    __ios_need_set_mode = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_ios_need_set_mode ()
{
    return __ios_need_set_mode;
}

int tmedia_defaults_set_app_document_path (const char* path)
{
    strncpy(__app_document_path, path, sizeof(__app_document_path));
    return 0;
}

int tmedia_defaults_set_ios_recovery_delay_from_call(uint32_t iosRecoveryDelayFromCall)
{
    __ios_recovery_delay_from_call = iosRecoveryDelayFromCall;
    return 0;
}

uint32_t tmedia_defaults_get_ios_recovery_delay_from_call()
{
    return __ios_recovery_delay_from_call;
}

const char* tmedia_defaults_get_app_document_path ()
{
    return __app_document_path;
}

int tmedia_defaults_set_pcm_file_size_kb (uint32_t size_kb)
{
    __pcm_file_size_kb = size_kb;
    return 0;
}

uint32_t tmedia_defaults_get_pcm_file_size_kb ()
{
    return __pcm_file_size_kb;
}

int tmedia_defaults_set_h264_file_size_kb (uint32_t size_kb)
{
    __h264_file_size_kb = size_kb;
    return 0;
}

uint32_t tmedia_defaults_get_h264_file_size_kb ()
{
    return __h264_file_size_kb;
}

int tmedia_defaults_set_vad_len_ms (uint32_t vad_len_ms)
{
    __vad_len_ms = vad_len_ms;
    return 0;
}

uint32_t tmedia_defaults_get_vad_len_ms ()
{
    return __vad_len_ms;
}

int tmedia_defaults_set_vad_silence_percent (uint32_t vad_silence_percent)
{
    __vad_silence_percent = vad_silence_percent;
    return 0;
}

uint32_t tmedia_defaults_get_vad_silence_percent ()
{
    return __vad_silence_percent;
}

int tmedia_defaults_set_comm_mode_enabled (tsk_bool_t enabled)
{
    __comm_mode_enabled = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_comm_mode_enabled ()
{
    return __comm_mode_enabled;
}

int tmedia_defaults_set_android_opensles_enabled (tsk_bool_t enabled)
{
    __android_audio_path_under_opensles = enabled;
    return 0;
}

tsk_bool_t tmedia_defaults_get_android_opensles_enabled()
{
    return __android_audio_path_under_opensles;
}

int tmedia_defaults_set_backgroundmusic_delay (uint32_t delay)
{
    __background_music_delay = delay;
    return 0;
}

uint32_t tmedia_defaults_get_backgroundmusic_delay ()
{
    return __background_music_delay;
}

int tmedia_defaults_set_max_jitter_buffer_num (uint32_t maxJbNum)
{
    if ((maxJbNum < 1) || (maxJbNum > 100)) {
        maxJbNum = 100;
    }
    __max_jitter_buffer_num = maxJbNum;
    return 0;
}

uint32_t tmedia_defaults_get_max_jitter_buffer_num()
{
    return __max_jitter_buffer_num;
}

int tmedia_defaults_set_max_neteq_delay_ms (uint32_t maxDelayMs)
{
    if ((maxDelayMs < 100) || (maxDelayMs > 10000)) {
        maxDelayMs = 1000;
    }
    __max_neteq_delay_ms = maxDelayMs;
    return 0;
}

uint32_t tmedia_defaults_get_max_neteq_delay_ms()
{
    return __max_neteq_delay_ms;
}

int tmedia_defaults_set_packet_stat_log_period_ms (uint32_t periodMs)
{
    __packet_stat_log_period_ms = periodMs;
    return 0;
}

uint32_t tmedia_defaults_get_packet_stat_log_period_ms()
{
    return __packet_stat_log_period_ms;
}

int tmedia_defaults_set_packet_stat_report_period_ms (uint32_t periodMs)
{
    __packet_stat_report_period_ms = periodMs;
    return 0;
}

uint32_t tmedia_defaults_get_packet_stat_report_period_ms()
{
    return __packet_stat_report_period_ms;
}

int tmedia_defaults_set_video_packet_stat_report_period_ms (uint32_t periodMs)
{
    __video_packet_stat_report_period_ms = periodMs;
    return 0;
}

uint32_t tmedia_defaults_get_video_packet_stat_report_period_ms()
{
    return __video_packet_stat_report_period_ms;
}

int tmedia_defaults_set_mic_volume_gain (uint32_t gain)
{
    __mic_volume_gain = gain;
    return 0;
}

uint32_t tmedia_defaults_get_mic_volume_gain()
{
    return __mic_volume_gain;
}

int tmedia_defaults_set_camera_rotation (int camRotation)
{
    __camera_rotation = camRotation;
    return 0;
}

int tmedia_defaults_get_camera_rotation ()
{
    return __camera_rotation;
}

void tmedia_defaults_set_camera_size(unsigned width, unsigned height)
{
    __video_camera_width = width;
    __video_camera_height = height;
}

void tmedia_defaults_get_camera_size( unsigned* width, unsigned* height)
{
    *width = __video_camera_width;
    *height = __video_camera_height;
}

void tmedia_defaults_set_video_size(unsigned width, unsigned height, int forceSet)
{
    if( forceSet == 1 || __video_video_size_set == 0 )
    {
        __video_video_code_width = width;
        __video_video_code_height = height;
        
        if( forceSet == 1 )
        {
            __video_video_size_set = 1;
        }
    }
}

void tmedia_defaults_get_video_size( unsigned* width, unsigned* height)
{
    *width = __video_video_code_width;
    *height = __video_video_code_height;
}

void tmedia_defaults_set_video_size_child(unsigned width, unsigned height, int forceSet)
{
    if( forceSet == 1 || __video_video_size_set_child == 0 )
    {
        __video_video_code_width_child = width;
        __video_video_code_height_child = height;
        
        if( forceSet == 1 )
        {
            __video_video_size_set_child = 1;
        }
    }
}

void tmedia_defaults_get_video_size_child( unsigned* width, unsigned* height)
{
    *width = __video_video_code_width_child;
    *height = __video_video_code_height_child;
}

void tmedia_defaults_set_video_size_share(unsigned width, unsigned height, int forceSet)
{
    if( forceSet == 1 || __video_video_size_set_share == 0 )
    {
        __video_video_code_width_share = width;
        __video_video_code_height_share = height;
        
        if( forceSet == 1 )
        {
            __video_video_size_set_share = 1;
        }
    }
}

void tmedia_defaults_get_video_size_share( unsigned* width, unsigned* height)
{
    *width = __video_video_code_width_share;
    *height = __video_video_code_height_share;
}

void tmedia_defaults_set_video_net_adjustmode(int mode)
{
    __video_net_adjust_mode = mode;
}

int tmedia_defaults_get_video_net_adjustmode()
{
    return __video_net_adjust_mode;
}

void tmedia_defaults_set_video_codec_bitrate(unsigned bitrate)
{
    __video_encoder_bitrate = bitrate;
}

unsigned tmedia_defaults_get_video_codec_bitrate()
{
    return __video_encoder_bitrate;
}

void tmedia_defaults_set_video_codec_bitrate_child(unsigned bitrate)
{
    __video_encoder_bitrate_child = bitrate;
}

unsigned tmedia_defaults_get_video_codec_bitrate_child()
{
    return __video_encoder_bitrate_child;
}

void tmedia_defaults_set_video_codec_bitrate_share(unsigned bitrate)
{
    __video_encoder_bitrate_share = bitrate;
}

unsigned tmedia_defaults_get_video_codec_bitrate_share()
{
    return __video_encoder_bitrate_share;
}


void tmedia_defaults_set_video_codec_quality_max(unsigned quality_max)
{
    __video_encoder_quality_max = quality_max;
}

unsigned tmedia_defaults_get_video_codec_quality_max()
{
    return __video_encoder_quality_max;
}

void tmedia_defaults_set_video_codec_quality_min(unsigned quality_min)
{
    __video_encoder_quality_min = quality_min;
}

unsigned tmedia_defaults_get_video_codec_quality_min()
{
    return __video_encoder_quality_min;
}

void tmedia_defaults_set_video_codec_quality_max_child(unsigned quality_max)
{
    __video_encoder_quality_max_child = quality_max;
}

unsigned tmedia_defaults_get_video_codec_quality_max_child()
{
    return __video_encoder_quality_max_child;
}

void tmedia_defaults_set_video_codec_quality_min_child(unsigned quality_min)
{
    __video_encoder_quality_min_child = quality_min;
}

unsigned tmedia_defaults_get_video_codec_quality_min_child()
{
    return __video_encoder_quality_min_child;
}

void tmedia_defaults_set_video_codec_quality_max_share(unsigned quality_max)
{
    __video_encoder_quality_max_share = quality_max;
}

unsigned tmedia_defaults_get_video_codec_quality_max_share()
{
    return __video_encoder_quality_max_share;
}

void tmedia_defaults_set_video_codec_quality_min_share(unsigned quality_min)
{
    __video_encoder_quality_min_share = quality_min;
}

unsigned tmedia_defaults_get_video_codec_quality_min_share()
{
    return __video_encoder_quality_min_share;
}

void tmedia_defaults_set_video_hwcodec_enabled(tsk_bool_t enabled)
{
    __video_hwcodec_enabled = enabled;
}

tsk_bool_t tmedia_defaults_get_video_hwcodec_enabled()
{
    return __video_hwcodec_enabled;
}

void tmedia_defaults_set_video_noframe_monitor_timeout(uint32_t timeout)
{
    __video_noframe_monitor_timeout = timeout;
}

uint32_t tmedia_defaults_get_video_noframe_monitor_timeout()
{
    return __video_noframe_monitor_timeout;
}

void tmedia_set_video_current_bitrate(int bitrate)
{
    __video_current_bitrate = bitrate;
}

int tmedia_get_video_current_bitrate()
{
    if(!__video_current_bitrate)
    {
        unsigned width, height;
        tmedia_defaults_get_video_size(&width, &height);
        float factor = 1.0f;
        if (__video_encoder_quality_max != -1 && __video_encoder_quality_min != -1) {
            factor = 0.75f;
        } 
        return (factor * tmedia_defaults_size_to_bitrate(width, height));
    }
    return __video_current_bitrate;
}

int tmedia_get_video_current_bitrate_child()
{
    if(!__video_current_bitrate_child)
    {
        unsigned width, height;
        tmedia_defaults_get_video_size_child(&width, &height);
        float factor = 1.0f;
        if (__video_encoder_quality_max_child != -1 && __video_encoder_quality_min_child != -1) {
            factor = 0.75f;
        } 

        return (factor * tmedia_defaults_size_to_bitrate(width, height));
    }
    return __video_current_bitrate_child;
}

int tmedia_defaults_size_to_bitrate(unsigned width, unsigned height)
{
    int v = 0;
    int g = width * height;
    

    if (g < 80 * 60) {
        v = g*3*3/1000;
    }
    else if(g >= 80*60 && g < 128*96)
    {
        v = 120;
    }
    else if(g >= 128*96 && g < 160*120)
    {
        v = 160;
    }
    else if(g >= 160*120 && g < 176*144)
    {
        v = 180;
    }
    else if(g >= 176*144 && g < 176*240)
    {
        v = 240;
    }
    else if(g >= 176*240 && g < 176*320)
    {
        v = 260;
    }
    else if(g >= 176*320 && g < 320*240)
    {
        v = 320;
    }
    else if(g >= 320*240 && g < 352*288)
    {
        v = 360;
    }
    else if(g >= 352*288 && g < 480*320)
    {
        v = 400;
    }
    else if(g >= 480*320 && g < 640*480)
    {
        v = 500;
    }
    else if(g >= 640*480 && g < 800*480)
    {
        v = 800;
    }
    else if(g >= 800*480 && g < 704*576)
    {
        v = 1000;
    }
    else if(g >= 704*576 && g < 852*480)
    {
        v = 1100;
    }
    else if(g >= 852*480 && g < 800*600)
    {
        v = 1200;
    }
    else if(g >= 800*600 && g < 720*960)
    {
        v = 1300;
    }
    else if(g >= 720*960 && g < 1024*768)
    {
        v = 1400;
    }
    else if(g >= 1024*768 && g < 1280*720)
    {
        v = 1600;
    }
    else if(g >= 1280*720 && g < 1408*1152)
    {
        v = 1800;
    }
    else if(g >= 1408*1152 && g < 1920*1080)
    {
        v = 2000;
    }
    else if(g >= 1920*1080 && g < 2560*1440)
    {
        v = 4000;
    }
    else if(g >= 2560*1440 && g < 3840*2160)
    {
        v = 6000;
    }
    else
    {
        v = g*2/1000;
    }
    //if(__video_encoder_bitrate && __video_encoder_bitrate < v*2)
    //    v = __video_encoder_bitrate;

    return v;
}

// 设置是否由外部输入音视频Frame
void tmedia_defaults_set_external_input_mode ( tsk_bool_t enabled )
{
    __external_input_mode = enabled;
}

tsk_bool_t tmedia_defaults_get_external_input_mode ( )
{
    return __external_input_mode;
}

void tmedia_defaults_set_video_adjust_mode(int mode)
{
    __video_encode_adjust_mode = mode;
}

int tmedia_defaults_get_video_adjust_mode()
{
    return __video_encode_adjust_mode;
}

int tmedia_defaults_set_spkout_fade_time (uint32_t timeMs)
{
    __spkout_fadein_time_ms = timeMs;
    return 0;
}

uint32_t tmedia_defaults_get_spkout_fade_time()
{
    return __spkout_fadein_time_ms;
}

void tmedia_set_video_egl_context(void* eglContext)
{
    __video_egl_context = eglContext;
}

void* tmedia_get_video_egl_context()
{
    return __video_egl_context;
}

void tmedia_set_video_egl_share_context(void* eglContext)
{
    __video_egl_share_context = eglContext;
}

void* tmedia_get_video_egl_share_context()
{
    return __video_egl_share_context;
}

void tmedia_defaults_set_video_frame_raw_cb_enabled(tsk_bool_t enabled) {
    __video_frame_raw_cb_enabeld = enabled;
}

tsk_bool_t tmedia_defaults_get_video_frame_raw_cb_enabled() {
    return  __video_frame_raw_cb_enabeld;
}

void tmedia_defaults_set_video_decode_raw_cb_enabled(tsk_bool_t enabled) {
    __video_decode_raw_cb_enabeld = enabled;
}

tsk_bool_t tmedia_defaults_get_video_decode_raw_cb_enabled() {
    return  __video_decode_raw_cb_enabeld;
}

void tmedia_set_record_device( int deviceID )
{
    __record_device_id = deviceID;
}

int  tmedia_get_record_device()
{
    return __record_device_id;
}

void tmedia_set_video_share_type(int businessType)
{
    __video_share_type = businessType;
}

int tmedia_get_video_share_type()
{
#ifdef ANDROID
    return 0xffff;
#else
    return __video_share_type;
#endif
}

// for win32
void tmedia_defaults_set_video_sampling_mode(int mode)
{
    __video_capture_sampling_mode = mode;
}

int tmedia_defaults_get_video_sampling_mode()
{
    return __video_capture_sampling_mode;
}

void tmedia_set_max_video_bitrate_level_for_sfu(int bitrate_level)
{
    __sfu_max_video_encoder_bitrate_level = bitrate_level;
}

int tmedia_get_max_video_bitrate_level_for_sfu()
{
    return __sfu_max_video_encoder_bitrate_level;
}

void tmedia_set_min_video_bitrate_level_for_sfu(int bitrate_level)
{
    __sfu_min_video_encoder_bitrate_level = bitrate_level;
}

int tmedia_get_min_video_bitrate_level_for_sfu()
{
    return __sfu_min_video_encoder_bitrate_level;
}

void tmedia_set_max_video_bitrate_level_for_p2p(int bitrate_level)
{
    __p2p_max_video_encoder_bitrate_level = bitrate_level;
}

int tmedia_get_max_video_bitrate_level_for_p2p()
{
    return __p2p_max_video_encoder_bitrate_level;
}

void tmedia_defaults_set_rtcp_memeber_count(uint32_t count)
{
    __max_rtcp_member_count = count;
}

uint32_t tmedia_defaults_get_rtcp_memeber_count()
{
    return __max_rtcp_member_count;
}

void tmedia_set_max_video_bitrate_multiple(uint32_t multiple)
{
    __video_default_bitrate_multiple = multiple;
}

uint32_t tmedia_get_max_video_bitrate_multiple()
{
    return __video_default_bitrate_multiple;
}

void tmedia_set_max_video_refresh_timeout(uint32_t timeout)
{
    __video_refresh_timeout = timeout;
}

uint32_t tmedia_get_max_video_refresh_timeout()
{
    return __video_refresh_timeout;
}

void tmedia_set_video_nack_flag(uint32_t nack)
{
    __video_nack_flag = nack;
}

uint32_t tmedia_get_video_nack_flag()
{
    return __video_nack_flag;
}
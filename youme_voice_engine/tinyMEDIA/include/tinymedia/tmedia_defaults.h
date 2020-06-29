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
#ifndef TINYMEDIA_DEFAULTS_H
#define TINYMEDIA_DEFAULTS_H

#include "tinymedia_config.h"

#include "tmedia_common.h"

TMEDIA_BEGIN_DECLS


TINYMEDIA_API int tmedia_defaults_set_profile(tmedia_profile_t profile);
TINYMEDIA_API tmedia_profile_t tmedia_defaults_get_profile();
TINYMEDIA_API int tmedia_defaults_set_bl(tmedia_bandwidth_level_t bl); // @deprecated
TINYMEDIA_API tmedia_bandwidth_level_t tmedia_defaults_get_bl(); // @deprecated
TINYMEDIA_API int tmedia_defaults_set_congestion_ctrl_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_congestion_ctrl_enabled();
TINYMEDIA_API int tmedia_defaults_set_video_fps(int32_t video_fps);
TINYMEDIA_API int32_t tmedia_defaults_get_video_fps();
TINYMEDIA_API int tmedia_defaults_set_video_fps_child(int32_t video_fps);
TINYMEDIA_API int32_t tmedia_defaults_get_video_fps_child();
TINYMEDIA_API int tmedia_defaults_set_video_fps_share(int32_t video_fps);
TINYMEDIA_API int32_t tmedia_defaults_get_video_fps_share();

TINYMEDIA_API int tmedia_defaults_set_video_motion_rank(int32_t video_motion_rank);
TINYMEDIA_API int32_t tmedia_defaults_get_video_motion_rank();
TINYMEDIA_API int tmedia_defaults_set_bandwidth_video_upload_max(int32_t bw_video_up_max_kbps);
TINYMEDIA_API int32_t tmedia_defaults_get_bandwidth_video_upload_max();
TINYMEDIA_API int tmedia_defaults_set_bandwidth_video_download_max(int32_t bw_video_down_max_kbps);
TINYMEDIA_API int32_t tmedia_defaults_get_bandwidth_video_download_max();
TINYMEDIA_API int tmedia_defaults_set_pref_video_size(int pref_video_size);
TINYMEDIA_API int tmedia_defaults_set_jb_margin(int32_t jb_margin_ms);
TINYMEDIA_API int32_t tmedia_defaults_get_jb_margin();
TINYMEDIA_API int tmedia_defaults_set_jb_max_late_rate(int32_t jb_max_late_rate_percent);
TINYMEDIA_API int32_t tmedia_defaults_get_jb_max_late_rate();
TINYMEDIA_API int tmedia_defaults_set_echo_tail(uint32_t echo_tail);
TINYMEDIA_API int tmedia_defaults_set_echo_skew(uint32_t echo_skew);
TINYMEDIA_API uint32_t tmedia_defaults_get_echo_tail();
TINYMEDIA_API uint32_t tmedia_defaults_get_echo_skew();
TINYMEDIA_API int tmedia_defaults_set_aec_enabled(tsk_bool_t aec_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_aec_enabled();
TINYMEDIA_API int tmedia_defaults_set_aec_option(uint32_t aec_option);
TINYMEDIA_API uint32_t tmedia_defaults_get_aec_option();
TINYMEDIA_API int tmedia_defaults_set_agc_enabled(tsk_bool_t agc_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_agc_enabled();
TINYMEDIA_API int tmedia_defaults_set_agc_level(float agc_level);
TINYMEDIA_API float tmedia_defaults_get_agc_level();
TINYMEDIA_API int tmedia_defaults_set_agc_target_level(int agc_target_level);
TINYMEDIA_API int tmedia_defaults_get_agc_target_level();
TINYMEDIA_API int tmedia_defaults_set_agc_compress_gain(int agc_compress_gain);
TINYMEDIA_API int tmedia_defaults_get_agc_compress_gain();
TINYMEDIA_API int tmedia_defaults_set_vad_enabled(tsk_bool_t vad_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_vad_enabled();
TINYMEDIA_API int tmedia_defaults_set_noise_supp_enabled(tsk_bool_t noise_supp_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_noise_supp_enabled();
TINYMEDIA_API int tmedia_defaults_set_howling_supp_enabled(tsk_bool_t howling_supp_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_howling_supp_enabled();
TINYMEDIA_API int tmedia_defaults_set_reverb_filter_enabled(tsk_bool_t reverb_filter_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_reverb_filter_enabled();

TINYMEDIA_API int tmedia_defaults_set_100rel_enabled(tsk_bool_t _100rel_enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_100rel_enabled();
TINYMEDIA_API int tmedia_defaults_set_screen_size(int32_t sx, int32_t sy);
TINYMEDIA_API int32_t tmedia_defaults_get_screen_x();
TINYMEDIA_API int32_t tmedia_defaults_get_screen_y();
TINYMEDIA_API int tmedia_defaults_set_audio_ptime(int32_t audio_ptime);
TINYMEDIA_API int32_t tmedia_defaults_get_audio_ptime();
TINYMEDIA_API int tmedia_defaults_set_audio_channels(int32_t channels_playback, int32_t channels_record);
TINYMEDIA_API int32_t tmedia_defaults_get_audio_channels_playback();
TINYMEDIA_API int32_t tmedia_defaults_get_audio_channels_record();
TINYMEDIA_API int tmedia_defaults_set_audio_gain(int32_t audio_producer_gain, int32_t audio_consumer_gain);
TINYMEDIA_API int32_t tmedia_defaults_get_audio_producer_gain();
TINYMEDIA_API int32_t tmedia_defaults_get_audio_consumer_gain();
TINYMEDIA_API int tmedia_defaults_set_audio_faked_consumer(tsk_bool_t audio_faked_consumer);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_audio_faked_consumer();
TINYMEDIA_API uint16_t tmedia_defaults_get_rtp_port_range_start();
TINYMEDIA_API uint16_t tmedia_defaults_get_rtp_port_range_stop();
TINYMEDIA_API int tmedia_defaults_set_rtp_port_range(uint16_t start, uint16_t stop);
TINYMEDIA_API int tmedia_defaults_set_rtp_symetric_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_rtp_symetric_enabled();
TINYMEDIA_API tmedia_type_t tmedia_defaults_get_media_type();
TINYMEDIA_API int tmedia_defaults_set_media_type(tmedia_type_t media_type);
TINYMEDIA_API int tmedia_defaults_set_volume(int32_t volume);
TINYMEDIA_API int32_t tmedia_defaults_get_volume();
TINYMEDIA_API int tmedia_producer_set_friendly_name(tmedia_type_t media_type, const char* friendly_name);
TINYMEDIA_API const char* tmedia_producer_get_friendly_name(tmedia_type_t media_type);
TINYMEDIA_API int32_t tmedia_defaults_get_inv_session_expires();
TINYMEDIA_API int tmedia_defaults_set_inv_session_expires(int32_t timeout);
TINYMEDIA_API const char* tmedia_defaults_get_inv_session_refresher();
TINYMEDIA_API int tmedia_defaults_set_inv_session_refresher(const char* refresher);TINYMEDIA_API int tmedia_defaults_set_bypass_encoding(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_bypass_encoding();
TINYMEDIA_API int tmedia_defaults_set_bypass_decoding(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_bypass_decoding();
TINYMEDIA_API int tmedia_defaults_set_videojb_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_videojb_enabled();
TINYMEDIA_API int tmedia_defaults_set_video_zeroartifacts_enabled(tsk_bool_t enabled);TINYMEDIA_API int tmedia_defaults_set_rtpbuff_size(tsk_size_t rtpbuff_size);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_video_zeroartifacts_enabled();
TINYMEDIA_API tsk_size_t tmedia_defaults_get_rtpbuff_size();
TINYMEDIA_API int tmedia_defaults_set_avpf_tail(tsk_size_t tail_min, tsk_size_t tail_max);
TINYMEDIA_API int tmedia_defaults_set_avpf_mode(enum tmedia_mode_e mode);
TINYMEDIA_API enum tmedia_mode_e tmedia_defaults_get_avpf_mode();
TINYMEDIA_API tsk_size_t tmedia_defaults_get_avpf_tail_min();
TINYMEDIA_API tsk_size_t tmedia_defaults_get_avpf_tail_max();
TINYMEDIA_API int tmedia_defaults_set_record_sample_rate(int32_t record_sample_rate,  int forceSet);
TINYMEDIA_API int32_t tmedia_defaults_get_record_sample_rate();
TINYMEDIA_API int tmedia_defaults_set_playback_sample_rate(int32_t playback_sample_rate,  int forceSet);
TINYMEDIA_API int32_t tmedia_defaults_get_playback_sample_rate();
TINYMEDIA_API int tmedia_defaults_set_mixed_callback_samplerate(int32_t mixedCallbackSamplerate);
TINYMEDIA_API int32_t tmedia_defaults_get_mixed_callback_samplerate();

TINYMEDIA_API int tmedia_defaults_set_opus_inband_fec_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_opus_inband_fec_enabled();
TINYMEDIA_API int tmedia_defaults_set_opus_outband_fec_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_opus_outband_fec_enabled();
TINYMEDIA_API int tmedia_defaults_set_opus_packet_loss_perc(int packet_loss_perc);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_opus_packet_loss_perc();
TINYMEDIA_API int tmedia_defaults_set_opus_dtx_peroid(int32_t peroid);
TINYMEDIA_API int tmedia_defaults_get_opus_dtx_peroid();
TINYMEDIA_API int tmedia_defaults_set_opus_encoder_complexity(int complexity);
TINYMEDIA_API int tmedia_defaults_get_opus_encoder_complexity();
TINYMEDIA_API int tmedia_defaults_set_opus_encoder_vbr_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_opus_encoder_vbr_enabled();
TINYMEDIA_API int tmedia_defaults_set_opus_encoder_max_bandwidth(int bandwidth);
TINYMEDIA_API int tmedia_defaults_get_opus_encoder_max_bandwidth();
TINYMEDIA_API int tmedia_defaults_set_opus_encoder_bitrate(int bitrate);
TINYMEDIA_API int tmedia_defaults_get_opus_encoder_bitrate();
TINYMEDIA_API int tmedia_defaults_set_rtp_feedback_period(int period);
TINYMEDIA_API int tmedia_defaults_get_rtp_feedback_period();
TINYMEDIA_API int tmedia_defaults_set_neteq_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_neteq_enabled();
TINYMEDIA_API int tmedia_defaults_set_neteq_applied(tsk_bool_t applied);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_neteq_applied();

TINYMEDIA_API int tmedia_defaults_set_ssl_certs(const char* priv_path, const char* pub_path, const char* ca_path, tsk_bool_t verify);
TINYMEDIA_API int tmedia_defaults_get_ssl_certs(const char** priv_path, const char** pub_path, const char** ca_path, tsk_bool_t *verify);
TINYMEDIA_API int tmedia_defaults_set_max_fds(int32_t max_fds);
TINYMEDIA_API tsk_size_t tmedia_defaults_get_max_fds();

TINYMEDIA_API int tmedia_defaults_set_android_faked_recording (tsk_bool_t fakedRecording);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_android_faked_recording ();
TINYMEDIA_API int tmedia_defaults_set_ios_no_permission_no_rec (tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_ios_no_permission_no_rec();
TINYMEDIA_API int tmedia_defaults_set_ios_airplay_no_rec(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_ios_airplay_no_rec();
TINYMEDIA_API int tmedia_defaults_set_ios_always_play_category(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_ios_always_play_category();
TINYMEDIA_API int tmedia_defaults_set_ios_need_set_mode(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_ios_need_set_mode();
TINYMEDIA_API int tmedia_defaults_set_ios_recovery_delay_from_call(uint32_t iosRecoveryDelayFromCall);
TINYMEDIA_API uint32_t tmedia_defaults_get_ios_recovery_delay_from_call();

TINYMEDIA_API int tmedia_defaults_set_app_document_path (const char* path);
TINYMEDIA_API const char* tmedia_defaults_get_app_document_path ();
TINYMEDIA_API int tmedia_defaults_set_pcm_file_size_kb (uint32_t size_kb);
TINYMEDIA_API uint32_t tmedia_defaults_get_pcm_file_size_kb ();
TINYMEDIA_API int tmedia_defaults_set_h264_file_size_kb (uint32_t size_kb);
TINYMEDIA_API uint32_t tmedia_defaults_get_h264_file_size_kb ();
TINYMEDIA_API int tmedia_defaults_set_vad_len_ms (uint32_t vad_len_ms);
TINYMEDIA_API uint32_t tmedia_defaults_get_vad_len_ms ();
TINYMEDIA_API int tmedia_defaults_set_vad_silence_percent (uint32_t vad_silence_percent);
TINYMEDIA_API uint32_t tmedia_defaults_get_vad_silence_percent ();
TINYMEDIA_API int tmedia_defaults_set_comm_mode_enabled (tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_comm_mode_enabled ();
TINYMEDIA_API int tmedia_defaults_set_android_opensles_enabled (tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_android_opensles_enabled ();

TINYMEDIA_API int tmedia_defaults_set_backgroundmusic_delay (uint32_t delay);
TINYMEDIA_API uint32_t tmedia_defaults_get_backgroundmusic_delay ();

TINYMEDIA_API int tmedia_defaults_set_max_jitter_buffer_num (uint32_t maxJbNum);
TINYMEDIA_API uint32_t tmedia_defaults_get_max_jitter_buffer_num();
TINYMEDIA_API int tmedia_defaults_set_max_neteq_delay_ms (uint32_t maxDelayMs);
TINYMEDIA_API uint32_t tmedia_defaults_get_max_neteq_delay_ms();
TINYMEDIA_API int tmedia_defaults_set_packet_stat_log_period_ms (uint32_t periodMs);
TINYMEDIA_API uint32_t tmedia_defaults_get_packet_stat_log_period_ms();
TINYMEDIA_API uint32_t tmedia_defaults_get_video_packet_stat_report_period_ms();
TINYMEDIA_API int tmedia_defaults_set_packet_stat_report_period_ms (uint32_t periodMs);
TINYMEDIA_API int tmedia_defaults_set_video_packet_stat_report_period_ms (uint32_t periodMs);
TINYMEDIA_API uint32_t tmedia_defaults_get_packet_stat_report_period_ms();
TINYMEDIA_API int tmedia_defaults_set_mic_volume_gain (uint32_t gain);
TINYMEDIA_API uint32_t tmedia_defaults_get_mic_volume_gain();
TINYMEDIA_API int tmedia_defaults_set_spkout_fade_time (uint32_t timeMs);
TINYMEDIA_API uint32_t tmedia_defaults_get_spkout_fade_time();

TINYMEDIA_API int tmedia_defaults_set_camera_rotation (int camRotation);
TINYMEDIA_API int tmedia_defaults_get_camera_rotation ();

TINYMEDIA_API void tmedia_defaults_set_camera_size(unsigned width, unsigned height);
TINYMEDIA_API void tmedia_defaults_get_camera_size( unsigned* width, unsigned* height);

TINYMEDIA_API void tmedia_defaults_set_video_size(unsigned width, unsigned height, int forceSet);
TINYMEDIA_API void tmedia_defaults_get_video_size( unsigned* width, unsigned* height);
TINYMEDIA_API void tmedia_defaults_set_video_size_child(unsigned width, unsigned height, int forceSet);
TINYMEDIA_API void tmedia_defaults_get_video_size_child( unsigned* width, unsigned* height);
TINYMEDIA_API void tmedia_defaults_set_video_size_share(unsigned width, unsigned height, int forceSet);
TINYMEDIA_API void tmedia_defaults_get_video_size_share( unsigned* width, unsigned* height);

TINYMEDIA_API void tmedia_defaults_set_video_net_adjustmode(int mode);
TINYMEDIA_API int tmedia_defaults_get_video_net_adjustmode();

TINYMEDIA_API void tmedia_defaults_set_video_codec_bitrate(unsigned bitrate);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_bitrate();
TINYMEDIA_API void tmedia_defaults_set_video_codec_bitrate_child(unsigned bitrate);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_bitrate_child();
TINYMEDIA_API void tmedia_defaults_set_video_codec_bitrate_share(unsigned bitrate);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_bitrate_share();

TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_max(unsigned quality_max);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_max();
TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_min(unsigned quality_min);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_min();

TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_max_child(unsigned quality_max);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_max_child();
TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_min_child(unsigned quality_min);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_min_child();

TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_max_share(unsigned quality_max);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_max_share();
TINYMEDIA_API void tmedia_defaults_set_video_codec_quality_min_share(unsigned quality_min);
TINYMEDIA_API unsigned tmedia_defaults_get_video_codec_quality_min_share();

TINYMEDIA_API void tmedia_defaults_set_video_hwcodec_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_video_hwcodec_enabled();
TINYMEDIA_API int tmedia_defaults_size_to_bitrate(unsigned width, unsigned height);
TINYMEDIA_API uint32_t tmedia_defaults_get_video_noframe_monitor_timeout();
TINYMEDIA_API void tmedia_defaults_set_video_noframe_monitor_timeout(uint32_t timeout);

TINYMEDIA_API void tmedia_set_video_current_bitrate(int bitrate);
TINYMEDIA_API int tmedia_get_video_current_bitrate();

TINYMEDIA_API int tmedia_get_video_current_bitrate_child();

TINYMEDIA_API void tmedia_defaults_set_external_input_mode(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_external_input_mode();
TINYMEDIA_API void tmedia_defaults_set_video_adjust_mode(int mode);
TINYMEDIA_API int tmedia_defaults_get_video_adjust_mode();

TINYMEDIA_API void tmedia_set_video_egl_context(void* eglContext);
TINYMEDIA_API void* tmedia_get_video_egl_context();
TINYMEDIA_API void tmedia_set_video_egl_share_context(void* eglContext);
TINYMEDIA_API void* tmedia_get_video_egl_share_context();

TINYMEDIA_API void tmedia_defaults_set_video_frame_raw_cb_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_video_frame_raw_cb_enabled();

TINYMEDIA_API void tmedia_defaults_set_video_decode_raw_cb_enabled(tsk_bool_t enabled);
TINYMEDIA_API tsk_bool_t tmedia_defaults_get_video_decode_raw_cb_enabled();

TINYMEDIA_API void tmedia_set_record_device( int deviceID );
TINYMEDIA_API int  tmedia_get_record_device();

TINYMEDIA_API int tmedia_get_video_share_type();
TINYMEDIA_API void tmedia_set_video_share_type(int businessType);

// for win32
TINYMEDIA_API void tmedia_defaults_set_video_sampling_mode(int mode);
TINYMEDIA_API int tmedia_defaults_get_video_sampling_mode();

TINYMEDIA_API void tmedia_set_max_video_bitrate_level_for_sfu(int bitrate_level);
TINYMEDIA_API int tmedia_get_max_video_bitrate_level_for_sfu();

TINYMEDIA_API void tmedia_set_min_video_bitrate_level_for_sfu(int bitrate_level);
TINYMEDIA_API int tmedia_get_min_video_bitrate_level_for_sfu();

TINYMEDIA_API void tmedia_set_max_video_bitrate_level_for_p2p(int bitrate_level);
TINYMEDIA_API int tmedia_get_max_video_bitrate_level_for_p2p();

TINYMEDIA_API void tmedia_defaults_set_rtcp_memeber_count(uint32_t count);
TINYMEDIA_API uint32_t tmedia_defaults_get_rtcp_memeber_count();

TINYMEDIA_API void tmedia_set_max_video_bitrate_multiple(uint32_t multiple);
TINYMEDIA_API uint32_t tmedia_get_max_video_bitrate_multiple();

TINYMEDIA_API void tmedia_set_max_video_refresh_timeout(uint32_t timeout);
TINYMEDIA_API uint32_t tmedia_get_max_video_refresh_timeout();

TINYMEDIA_API void tmedia_set_video_nack_flag(uint32_t nack);
TINYMEDIA_API uint32_t tmedia_get_video_nack_flag();

TMEDIA_END_DECLS

#endif /* TINYMEDIA_DEFAULTS_H */

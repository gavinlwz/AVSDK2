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

/**@file tmedia_params.h
 * @brief Media parameters used to configure any session or plugin.
 *
 *
 *

 */
#ifndef TINYMEDIA_PARAMS_H
#define TINYMEDIA_PARAMS_H

#include "tinymedia_config.h"

#include "tinymedia/tmedia_common.h"

#include "tsk_list.h"

TMEDIA_BEGIN_DECLS

#define TMEDIA_PARAM(self) ((tmedia_param_t*)(self))

typedef enum tmedia_param_access_type_e
{
	tmedia_pat_get,
	tmedia_pat_set
}
tmedia_param_access_type_t;

typedef enum tmedia_param_plugin_type_e
{
	tmedia_ppt_consumer,
	tmedia_ppt_producer,
	tmedia_ppt_codec,
	tmedia_ppt_session,
	tmedia_ppt_manager,
    tmedia_ppt_jitterbuffer,
}
tmedia_param_plugin_type_t;

typedef enum tmedia_param_value_type_e
{
	tmedia_pvt_int32,
	tmedia_pvt_bool = tmedia_pvt_int32,
	tmedia_pvt_pobject,
	tmedia_pvt_pchar,
	tmedia_pvt_int64,
    tmedia_pvt_pvoid,
}
tmedia_param_value_type_t;

#define TMEDIA_PARAM_VALUE_TYPE_IS_PTR(self) ((self) == tmedia_pvt_pobject || (self) == tmedia_pvt_pchar || (self) == tmedia_pvt_pvoid)

typedef struct tmedia_param_s
{
	TSK_DECLARE_OBJECT;

	tmedia_param_access_type_t access_type;
	tmedia_type_t media_type;
	tmedia_param_plugin_type_t plugin_type;
	tmedia_param_value_type_t value_type;
	
	char* key;
	/* Because setting parameters could be deferred
	* ==> MUST copy the value for later use.
	* e.g. TMEDIA_SESSION_MANAGER_SET_INT32("width", 1234); 1234 will be lost when we exit the block code
	*/
	void* value;
}
tmedia_param_t;

typedef tsk_list_t tmedia_params_L_t; /**< List of @ref tsk_param_t elements. */

#define tmedia_params_create() tsk_list_create()

TINYMEDIA_API tmedia_param_t* tmedia_param_create(tmedia_param_access_type_t access_type, 
									tmedia_type_t media_type, 
									tmedia_param_plugin_type_t plugin_type, 
									tmedia_param_value_type_t value_type,
									const char* key,
									void* value);
#define tmedia_param_create_get(media_type, plugin_type, value_type, key, value) tmedia_param_create(tmedia_pat_get, (media_type), (plugin_type), (value_type), (key), (value))
#define tmedia_param_create_get_session(media_type, value_type, key, value) tmedia_param_create_get((media_type), tmedia_ppt_session, (value_type), (key), (value))
#define tmedia_param_create_get_codec(media_type, value_type, key, value) tmedia_param_create_get((media_type), tmedia_ppt_codec, (value_type), (key), (value))
#define tmedia_param_create_set(media_type, plugin_type, value_type, key, value) tmedia_param_create(tmedia_pat_set, (media_type), (plugin_type), (value_type), (value))

TINYMEDIA_API tmedia_params_L_t* tmedia_params_create_2(va_list *app);

TINYMEDIA_API int tmedia_params_add_param(tmedia_params_L_t **self, 
							tmedia_param_access_type_t access_type, 
							tmedia_type_t media_type, 
							tmedia_param_plugin_type_t plugin_type, 
							tmedia_param_value_type_t value_type,
							const char* key,
							void* value);

TINYMEDIA_GEXTERN const tsk_object_def_t *tmedia_param_def_t;

/* Definitions of parameter keys */
#define TMEDIA_PARAM_KEY_RECORDING_ERROR                   "recording_error"
#define TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA             "recording_error_extra"
#define TMEDIA_PARAM_KEY_MICROPHONE_MUTE                   "microphone_mute"
#define TMEDIA_PARAM_KEY_SPEAKER_MUTE                      "speaker_mute"
#define TMEDIA_PARAM_KEY_HEADSET_PLUGIN                    "headset_plugin"
#define TMEDIA_PARAM_KEY_MIC_VOLUME                        "mic_volume"
#define TMEDIA_PARAM_KEY_SPEAKER_VOLUME                    "speaker_volume"
#define TMEDIA_PARAM_KEY_SMART_SETTING                     "smart_setting"
#define TMEDIA_PARAM_KEY_AGC_ENABLED                       "agc_enabled"
#define TMEDIA_PARAM_KEY_AEC_ENABLED                       "aec_enabled"
#define TMEDIA_PARAM_KEY_NS_ENABLED                        "ns_enabled"
#define TMEDIA_PARAM_KEY_VAD_ENABLED                       "vad_enabled"
#define TMEDIA_PARAM_KEY_SOUND_TOUCH_ENABLED               "sound_touch_enabled"
#define TMEDIA_PARAM_KEY_SOUND_TOUCH_TEMPO                 "sound_touch_tempo"
#define TMEDIA_PARAM_KEY_SOUND_TOUCH_RATE                  "sound_touch_rate"
#define TMEDIA_PARAM_KEY_SOUND_TOUCH_PITCH                 "sound_touch_pitch"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_SPEAKER           "mix_audio_track_speaker"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_MICPHONE          "mix_audio_track_micphone"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_ENABLED                 "mix_audio_track_enabled"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME                  "mix_audio_track_volume"
#define TMEDIA_PARAM_KEY_MIC_BYPASS_TO_SPEAKER             "mic_bypass_to_speaker"
#define TMEDIA_PARAM_KEY_BGM_BYPASS_TO_SPEAKER             "bgm_bypass_to_speaker"
#define TMEDIA_PARAM_KEY_BGM_DELAY                         "background_music_delay"
#define TMEDIA_PARAM_KEY_RTP_TIMESTAMP                     "rtp_timestamp"
#define TMEDIA_PARAM_REVERB_ENABLED                        "reverb_enabled"
#define TMEDIA_PARAM_KEY_PACKET_STAT                       "packet_stat"
#define TMEDIA_PARAM_KEY_AV_QOS_STAT                       "av_qos_stat"
#define TMEDIA_PARAM_KEY_VAD_CALLBACK                      "vad_callback"
#define TMEDIA_PARAM_KEY_RECORDING_TIME_MS                 "recording_time_ms"
#define TMEDIA_PARAM_KEY_PLAYING_TIME_MS                   "playing_time_ms"
#define TMEDIA_PARAM_KEY_PCM_CALLBACK                      "pcm_callback"
#define TMEDIA_PARAM_KEY_PCM_CALLBACK_FLAG                 "pcm_callback_flag"
#define TMEDIA_PARAM_KEY_PCM_CALLBACK_CHANNEL              "pcm_callback_channel"
#define TMEDIA_PARAM_KEY_PCM_CALLBACK_SAMPLE_RATE          "pcm_callback_rate"
#define TMEDIA_PARAM_KEY_TRANSCRIBER_ENABLED                "transcriber_enabled"

#define TMEDIA_PARAM_KEY_SAVE_SCREEN                       "save_screen"
#define TMEDIA_PARAM_KEY_MIC_LEVEL_CALLBACK                "mic_level_callback"
#define TMEDIA_PARAM_KEY_MAX_MIC_LEVEL_CALLBACK            "max_mic_level_callback"
#define TMEDIA_PARAM_KEY_FAREND_VOICE_LEVEL_CALLBACK       "farend_voice_level_callback"
#define TMEDIA_PARAM_KEY_MAX_FAREND_VOICE_LEVEL            "max_farend_voice_level"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_TRACK_EFFECT            "mix_audio_track_effect"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_VOLUME_EFFECT           "mix_audio_track_volume_effect"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_EFFECT_FREE_BUFF_COUNT  "mix_audio_track_effect_free_buff_count"
#define TMEDIA_PARAM_KEY_MIX_AUDIO_FREE_BUFF_COUNT         "mix_audio_track_free_buff_count"

// Opus
#define TMEDIA_PARAM_KEY_OPUS_INBAND_FEC_ENABLE            "opus_inband_fec_enable"
#define TMEDIA_PARAM_KEY_OPUS_SET_PACKET_LOSS_PERC         "opus_set_packet_loss_perc"
#define TMEDIA_PARAM_KEY_OPUS_SET_BITRATE         		   "opus_set_bitrate"

// Video Part
#define TMEDIA_PARAM_KEY_VIDEO_RENDER_CB                   "video_render_cb"                     
#define TMEDIA_PARAM_KEY_SESSION_ID                        "session_id"
#define TMEDIA_PARAM_KEY_VIDEO_RUNTIME_EVENT_CB            "video_runtime_event_cb"
#define TMEDIA_PARAM_KEY_NEED_OPEN_CAMERA                  "need_open_camera"
#define TMEDIA_PARAM_KEY_VIDEO_WIDTH                       "video_width"
#define TMEDIA_PARAM_KEY_VIDEO_HEIGHT                      "video_height"
#define TMEDIA_PARAM_KEY_SCREEN_ORIENTATION                "screen_orientation"
#define TMEDIA_PARAM_KEY_STATICS_QOS_CB            		   "statics_qos_cb"
#define TMEDIA_PARAM_KEY_VIDEO_PREDECODE_CB                "video_predecode_cb"
#define TMEDIA_PARAM_KEY_VIDEO_PREDECODE_NEED_DECODE_AND_RENDER  "video_predecode_need_decode_and_render"
#define TMEDIA_PARAM_KEY_VIDEO_ENCODE_ADJUST_CB            "video_encode_adjust_cb"
#define TMEDIA_PARAM_KEY_VIDEO_DECODE_PARAM_CB            "video_decode_param_cb"
#define TMEDIA_PARAM_KEY_VIDEO_EVENT_CB            		   "video_event_cb"
TMEDIA_END_DECLS

#endif /* TINYMEDIA_PARAMS_H */

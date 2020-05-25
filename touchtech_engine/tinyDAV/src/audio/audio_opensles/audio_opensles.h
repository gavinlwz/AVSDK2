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
#ifndef _YOUME_AUDIO_OPENSLES_H
#define _YOUME_AUDIO_OPENSLES_H

#include "tinysak_config.h"

#include "audio_opensles_config.h"

#include "tsk_plugin.h"

AUDIO_OPENSLES_BEGIN_DECLS

typedef void audio_opensles_instance_handle_t;

AUDIO_OPENSLES_API int __plugin_get_def_count ();
AUDIO_OPENSLES_API tsk_plugin_def_type_t __plugin_get_def_type_at (int index);
AUDIO_OPENSLES_API tsk_plugin_def_media_type_t __plugin_get_def_media_type_at (int index);
AUDIO_OPENSLES_API tsk_plugin_def_ptr_const_t __plugin_get_def_at (int index);

audio_opensles_instance_handle_t *audio_opensles_instance_create (uint64_t session_id);
int audio_opensles_instance_prepare_consumer (audio_opensles_instance_handle_t *self, struct tmedia_consumer_s **consumer);
int audio_opensles_instance_prepare_producer (audio_opensles_instance_handle_t *_self, struct tmedia_producer_s **producer);
int audio_opensles_instance_start_consumer (audio_opensles_instance_handle_t *self);
int audio_opensles_instance_start_producer (audio_opensles_instance_handle_t *self);
int audio_opensles_instance_stop_consumer (audio_opensles_instance_handle_t *self);
int audio_opensles_instance_stop_producer (audio_opensles_instance_handle_t *self);
int audio_opensles_instance_set_speakerOn (audio_opensles_instance_handle_t *self, tsk_bool_t speakerOn);
int audio_opensles_instance_set_microphone_volume (audio_opensles_instance_handle_t *self, int32_t volume);
int audio_opensles_instance_get_recording_error (audio_opensles_instance_handle_t *_self, int32_t *pErrCode);
int audio_opensles_instance_get_recording_error_extra (audio_opensles_instance_handle_t *_self, int32_t *pErrExtra);
int audio_opensles_instance_destroy (audio_opensles_instance_handle_t **self);

AUDIO_OPENSLES_END_DECLS

#endif /* _YOUME_AUDIO_OPENSLES_H */

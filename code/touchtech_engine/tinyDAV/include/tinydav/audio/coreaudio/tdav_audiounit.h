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
#ifndef TDAV_AUDIO_UNIT_H
#define TDAV_AUDIO_UNIT_H

#include "tinydav_config.h"

#ifdef __APPLE__

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include "tsk_object.h"

TDAV_BEGIN_DECLS


#if TARGET_OS_MAC
#	if AUDIO_UNIT_VERSION < 1060
#		define AudioComponent Component
#		define AudioComponentInstance ComponentInstance
#		define AudioComponentDescription ComponentDescription
#		define AudioComponentFindNext FindNextComponent
#		define AudioComponentInstanceNew OpenAComponent
#		define AudioComponentInstanceDispose CloseComponent
#	endif
#endif /* TARGET_OS_MAC */

typedef void* tdav_audiounit_handle_t;

tdav_audiounit_handle_t* tdav_audiounit_handle_create(uint64_t session_id, tsk_boolean_t bUseHalMode);
AudioComponentInstance tdav_audiounit_handle_get_instance(tdav_audiounit_handle_t* self);
int tdav_audiounit_handle_signal_consumer_prepared(tdav_audiounit_handle_t* self);
int tdav_audiounit_handle_signal_producer_prepared(tdav_audiounit_handle_t* self);
int tdav_audiounit_handle_start(tdav_audiounit_handle_t* self);
uint32_t tdav_audiounit_handle_get_frame_duration(tdav_audiounit_handle_t* self);
int tdav_audiounit_handle_configure(tdav_audiounit_handle_t* self, tsk_bool_t consumer, uint32_t ptime, AudioStreamBasicDescription* audioFormat);
int tdav_audiounit_handle_mute(tdav_audiounit_handle_t* self, tsk_bool_t mute);
int tdav_audiounit_handle_interrupt(tdav_audiounit_handle_t* self, tsk_bool_t interrupt);
int tdav_audiounit_handle_stop(tdav_audiounit_handle_t* self);
int tdav_audiounit_handle_destroy(tdav_audiounit_handle_t** self);

TDAV_END_DECLS

#endif /* __APPLE__ */

#endif /* TDAV_AUDIO_UNIT_H */

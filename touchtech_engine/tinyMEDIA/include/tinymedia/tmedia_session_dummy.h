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

#ifndef TINYMEDIA_SESSION_DUMMY_H
#define TINYMEDIA_SESSION_DUMMY_H

#include "tinymedia_config.h"

#include "tmedia_session.h"

#include "tsk_object.h"

TMEDIA_BEGIN_DECLS

/** Dummy Audio session */
typedef struct tmedia_session_daudio_s
{
	TMEDIA_DECLARE_SESSION_AUDIO;
	uint16_t local_port;
	uint16_t remote_port;
}
tmedia_session_daudio_t;

TINYMEDIA_GEXTERN const tmedia_session_plugin_def_t *tmedia_session_daudio_plugin_def_t;


TMEDIA_END_DECLS

#endif /* TINYMEDIA_SESSION_DUMMY_H */


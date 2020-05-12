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

/**@file tmedia_session_dummy.c
 * @brief Dummy sessions used for test only.
 *
 *
 *

 */
#include "tinymedia/tmedia_session_dummy.h"

#include "tsk_memory.h"
#include "tsk_debug.h"

/* ============ Audio Session ================= */

int tmedia_session_daudio_set(tmedia_session_t* self, const tmedia_param_t* param)
{
	tmedia_session_daudio_t* daudio;

	daudio = (tmedia_session_daudio_t*)self;

	return 0;
}

int tmedia_session_daudio_get(tmedia_session_t* self, tmedia_param_t* param)
{
	return 0;
}

int tmedia_session_daudio_prepare(tmedia_session_t* self)
{
	tmedia_session_daudio_t* daudio;

	daudio = (tmedia_session_daudio_t*)self;

	/* set local port */
	daudio->local_port = rand() ^ rand();

	return 0;
}

int tmedia_session_daudio_start(tmedia_session_t* self)
{
	return 0;
}

int tmedia_session_daudio_stop(tmedia_session_t* self)
{
	tmedia_session_daudio_t* daudio;

	daudio = (tmedia_session_daudio_t*)self;

	/* very important */
	daudio->local_port = 0;

	return 0;
}

int tmedia_session_daudio_send_dtmf(tmedia_session_t* self, uint8_t event)
{
	return 0;
}

int tmedia_session_daudio_pause(tmedia_session_t* self)
{
	return 0;
}

const tsdp_header_M_t* tmedia_session_daudio_get_lo(tmedia_session_t* self)
{
	tmedia_session_daudio_t* daudio;
	tsk_bool_t changed = tsk_false;

	if(!self || !self->plugin){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	daudio = (tmedia_session_daudio_t*)self;

	if(self->ro_changed && self->M.lo){
		/* Codecs */
		tsdp_header_A_removeAll_by_field(self->M.lo->Attributes, "fmtp");
		tsdp_header_A_removeAll_by_field(self->M.lo->Attributes, "rtpmap");
		tsk_list_clear_items(self->M.lo->FMTs);
		
		/* QoS */
		tsdp_header_A_removeAll_by_field(self->M.lo->Attributes, "curr");
		tsdp_header_A_removeAll_by_field(self->M.lo->Attributes, "des");
		tsdp_header_A_removeAll_by_field(self->M.lo->Attributes, "conf");
	}

	changed = (self->ro_changed || !self->M.lo);
	
	if(!self->M.lo && !(self->M.lo = tsdp_header_M_create(self->plugin->media, daudio->local_port, "RTP/AVP"))){
		TSK_DEBUG_ERROR("Failed to create lo");
		return tsk_null;
	}

	if(changed){
		/* from codecs to sdp */
		tmedia_codec_to_sdp(self->neg_codecs ? self->neg_codecs : self->codecs, self->M.lo);
	}
	

	return self->M.lo;
}

int tmedia_session_daudio_set_ro(tmedia_session_t* self, const tsdp_header_M_t* m)
{
	tmedia_codecs_L_t* neg_codecs;

	if((neg_codecs = tmedia_session_match_codec(self, m))){
		/* update negociated codecs */
		TSK_OBJECT_SAFE_FREE(self->neg_codecs);
		self->neg_codecs = neg_codecs;
		/* update remote offer */
		TSK_OBJECT_SAFE_FREE(self->M.ro);
		self->M.ro = tsk_object_ref((void*)m);
		
		return 0;
	}
	return -1;
}

//=================================================================================================
//	Dummy Audio session object definition
//
/* constructor */
static tsk_object_t* tmedia_session_daudio_ctor(tsk_object_t * self, va_list * app)
{
	tmedia_session_daudio_t *session = self;
	if(session){
		/* init base: called by tmedia_session_create() */
		/* init self */
	}
	return self;
}
/* destructor */
static tsk_object_t* tmedia_session_daudio_dtor(tsk_object_t * self)
{ 
	tmedia_session_daudio_t *session = self;
	if(session){
		/* deinit base */
		tmedia_session_deinit(self);
		/* deinit self */
	}

	return self;
}
/* object definition */
static const tsk_object_def_t tmedia_session_daudio_def_s = 
{
	sizeof(tmedia_session_daudio_t),
	tmedia_session_daudio_ctor, 
	tmedia_session_daudio_dtor,
	tmedia_session_cmp, 
};
/* plugin definition*/
static const tmedia_session_plugin_def_t tmedia_session_daudio_plugin_def_s = 
{
	&tmedia_session_daudio_def_s,
	
	tmedia_audio,
	"audio",
	
	tmedia_session_daudio_set,
	tmedia_session_daudio_get,
	tmedia_session_daudio_prepare,
	tmedia_session_daudio_start,
	tmedia_session_daudio_pause,
	tmedia_session_daudio_stop,
	tsk_null,
	
	/* Audio part */
	{ tsk_null },

	tmedia_session_daudio_get_lo,
	tmedia_session_daudio_set_ro
};
const tmedia_session_plugin_def_t *tmedia_session_daudio_plugin_def_t = &tmedia_session_daudio_plugin_def_s;



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

/**@file tdav_codec_t140.c
 * @brief T140 codec implementation (RFC 4103)
 */
#include "tinydav/codecs/t140/tdav_codec_t140.h"

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_debug.h"

// RFC 4103 - 6.  Parameter for Character Transmission Rate
#define TDAV_CODEC_T140_CPS	30

static int tdav_codec_t140_set(tmedia_codec_t* self, const struct tmedia_param_s* param)
{
	return 0;
}

static int tdav_codec_t140_open(tmedia_codec_t* self)
{
	return 0;
}

static int tdav_codec_t140_close(tmedia_codec_t* self)
{
	return 0;
}


static tsk_size_t tdav_codec_t140_encode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size)
{
	tsk_size_t out_size = in_size;
	
	if(!self || !in_data || !in_size || !out_data){
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}
	
	if(*out_max_size <out_size){
		if(!(*out_data = tsk_realloc(*out_data, out_size))){
			TSK_DEBUG_ERROR("Failed to allocate new buffer");
			*out_max_size = 0;
			return 0;
		}
		*out_max_size = out_size;
	}
	memcpy(*out_data, in_data, out_size);
	
	return out_size;
}

static tsk_size_t tdav_codec_t140_decode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size, const tsk_object_t* proto_hdr,tsk_bool_t* bPLC)
{
	tsk_size_t out_size = in_size;

	if(!self || !in_data || !in_size || !out_data){
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}

	/* allocate new buffer */
	if(*out_max_size < out_size){
		if(!(*out_data = tsk_realloc(*out_data, out_size))){
			TSK_DEBUG_ERROR("Failed to allocate new buffer");
			*out_max_size = 0;
			return 0;
		}
		*out_max_size = out_size;
	}
	memcpy(*out_data, in_data, out_size);
	
	return out_size;
}

static tsk_bool_t tdav_codec_t140_sdp_att_match(const tmedia_codec_t* self, const char* att_name, const char* att_value)
{
	return tsk_true;
}

static char* tdav_codec_t140_sdp_att_get(const tmedia_codec_t* self, const char* att_name)
{
	if(!self || !att_name){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	if(tsk_striequals("fmtp", att_name)){
		char* fmtp = tsk_null;
		tsk_sprintf(&fmtp, "cps=%d", TDAV_CODEC_T140_CPS);
		return fmtp;
	}
	return tsk_null;
}


//
//	G.711u Plugin definition
//

/* constructor */
static tsk_object_t* tdav_codec_t140_ctor(tsk_object_t * self, va_list * app)
{
	tdav_codec_t140_t *t140 = self;
	if(t140){
		/* init base: called by tmedia_codec_create() */
		/* init self */
	}
	return self;
}
/* destructor */
static tsk_object_t* tdav_codec_t140_dtor(tsk_object_t * self)
{ 
	tdav_codec_t140_t *t140 = self;
	if(t140){
		/* deinit base */
		/* deinit self */
	}

	return self;
}
/* object definition */
static const tsk_object_def_t tdav_codec_t140_def_s = 
{
	sizeof(tdav_codec_t140_t),
	tdav_codec_t140_ctor, 
	tdav_codec_t140_dtor,
	tmedia_codec_cmp, 
};
/* plugin definition*/
static const tmedia_codec_plugin_def_t tdav_codec_t140_plugin_def_s = 
{
	&tdav_codec_t140_def_s,
	
	tmedia_t140,
	tmedia_codec_id_t140,
	"t140",
	"T140 codec (From tinyDAV)",
	TMEDIA_CODEC_FORMAT_T140,
	tsk_true,
	1000, // rate
	
	/* audio */
	{0},
	
	/* video */
	{0},
	
	tdav_codec_t140_set,
	tdav_codec_t140_open,
	tdav_codec_t140_close,
	tdav_codec_t140_encode,
	tdav_codec_t140_decode,
	tdav_codec_t140_sdp_att_match,
	tdav_codec_t140_sdp_att_get
};
const tmedia_codec_plugin_def_t *tdav_codec_t140_plugin_def_t = &tdav_codec_t140_plugin_def_s;

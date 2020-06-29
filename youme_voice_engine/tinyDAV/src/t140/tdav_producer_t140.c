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

/**@file tdav_producer_t140.c
 * @brief Producer for T140 protocol (RFC 4103)
 */
#include "tinydav/t140/tdav_producer_t140.h"

#include "tsk_debug.h"

static int tdav_producer_t140_set(tmedia_producer_t* self, const tmedia_param_t* param)
{
	return 0;
}

static int tdav_producer_t140_prepare(tmedia_producer_t* self, const tmedia_codec_t* param)
{
	return 0;
}

static int tdav_producer_t140_start(tmedia_producer_t* self)
{
	return 0;
}

static int tdav_producer_t140_pause(tmedia_producer_t* self)
{
	return 0;
}

static int tdav_producer_t140_stop(tmedia_producer_t* self)
{
	return 0;
}

int tdav_producer_send_data(tdav_producer_t140_t* self, enum tmedia_t140_data_type_e data_type, const void* data_ptr, unsigned data_size)
{
	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	if(TMEDIA_PRODUCER(self)->enc_cb.callback){
		if(data_type != tmedia_t140_data_type_utf8){
			// build data
			tsk_size_t cmd_size = 0, i;
			int32_t cmd_val = (int32_t)data_type;
			if(data_ptr || data_size){
				TSK_DEBUG_WARN("Data not expected for commands");
			}
			// TODO: use ASM POPCNT
			for(i = 0; i < 32; i+= 8){
				if(((cmd_val >> i) & 0xFF)){
					++cmd_size;
				}
			}
			if(cmd_size){
				TMEDIA_PRODUCER(self)->enc_cb.callback(TMEDIA_PRODUCER(self)->enc_cb.callback_data, &cmd_val, cmd_size);
			}
		}
		else{
			TMEDIA_PRODUCER(self)->enc_cb.callback(TMEDIA_PRODUCER(self)->enc_cb.callback_data, data_ptr, data_size);
		}
	}
	return 0;
}

//
//	T.140 producer object definition
//
/* constructor */
static tsk_object_t* tdav_producer_t140_ctor(tsk_object_t * self, va_list * app)
{
	tdav_producer_t140_t *producer = self;
	if(producer){
		/* init base */
		
		/* init self */
	}
	return self;
}
/* destructor */
static tsk_object_t* tdav_producer_t140_dtor(tsk_object_t * self)
{ 
	tdav_producer_t140_t *producer = self;
	if(producer){

		/* stop */
		if(producer->started){
			tdav_producer_t140_stop(self);
		}

		/* deinit base */
		
		/* deinit self */
		
	}

	return self;
}
/* object definition */
static const tsk_object_def_t tdav_producer_t140_def_s = 
{
	sizeof(tdav_producer_t140_t),
	tdav_producer_t140_ctor, 
	tdav_producer_t140_dtor,
	tsk_null, 
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t tdav_producer_t140_plugin_def_s = 
{
	&tdav_producer_t140_def_s,
	
	tmedia_t140,
	"T.140 producer",
	
	tdav_producer_t140_set,
	tdav_producer_t140_prepare,
	tdav_producer_t140_start,
	tdav_producer_t140_pause,
	tdav_producer_t140_stop
};
const tmedia_producer_plugin_def_t *tdav_producer_t140_plugin_def_t = &tdav_producer_t140_plugin_def_s;
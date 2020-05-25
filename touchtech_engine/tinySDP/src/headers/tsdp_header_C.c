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


/**@file tsdp_header_C.c
 * @brief "c=" header (Connection Data).
 */
#include "tinysdp/headers/tsdp_header_C.h"

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"

#include <string.h>

/***********************************
*	Ragel state machine.
*/

/* #line 66 "./ragel/tsdp_parser_header_C.rl" */



tsdp_header_C_t* tsdp_header_c_create(const char* nettype, const char* addrtype, const char* addr)
{
	return tsk_object_new(TSDP_HEADER_C_VA_ARGS(nettype, addrtype, addr));
}

tsdp_header_C_t* tsdp_header_c_create_null()
{
	return tsdp_header_c_create(tsk_null, tsk_null, tsk_null);
}




tsdp_header_C_t *tsdp_header_C_parse(const char *data)
{
	tsdp_header_C_t *hdr_C = tsdp_header_c_create_null();
    int len =strlen(data);
    hdr_C->addr = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_C->addr, data, len);
    
    len = strlen("IP4");
    hdr_C->addrtype = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_C->addrtype, "IP4", len);
    
    len = strlen("IN");
    hdr_C->nettype = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_C->nettype, "IN", len);
	return hdr_C;
}







//========================================================
//	E header object definition
//

static tsk_object_t* tsdp_header_C_ctor(tsk_object_t *self, va_list * app)
{
	tsdp_header_C_t *C = self;
	if(C){
		TSDP_HEADER(C)->type = tsdp_htype_C;
		TSDP_HEADER(C)->rank = TSDP_HTYPE_C_RANK;
		
		C->nettype = tsk_strdup(va_arg(*app, const char*));
		C->addrtype = tsk_strdup(va_arg(*app, const char*));
		C->addr = tsk_strdup(va_arg(*app, const char*));
	}
	else{
		TSK_DEBUG_ERROR("Failed to create new C header.");
	}
	return self;
}

static tsk_object_t* tsdp_header_C_dtor(tsk_object_t *self)
{
	tsdp_header_C_t *C = self;
	if(C){
		TSK_FREE(C->nettype);
		TSK_FREE(C->addrtype);
		TSK_FREE(C->addr);
	}
	else{
		TSK_DEBUG_ERROR("Null PC header.");
	}

	return self;
}


static const tsk_object_def_t tsdp_header_C_def_s = 
{
	sizeof(tsdp_header_C_t),
	tsdp_header_C_ctor,
	tsdp_header_C_dtor,
	tsk_null
};

const tsk_object_def_t *tsdp_header_C_def_t = &tsdp_header_C_def_s;

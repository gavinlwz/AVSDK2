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

/**@file tsdp_header_O.c
 * @brief SDP "o=" header (Origin).
 */
#include "tinysdp/headers/tsdp_header_O.h"

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"

#include <string.h>

/***********************************
*	Ragel state machine.
*/

/* #line 80 "./ragel/tsdp_parser_header_O.rl" */


tsdp_header_O_t* tsdp_header_O_create(const char* username, uint32_t sess_id, uint32_t sess_version, const char* nettype, const char* addrtype, const char* addr)
{
    return tsk_object_new(TSDP_HEADER_O_VA_ARGS( nettype, addrtype, addr));
}


tsdp_header_O_t *tsdp_header_O_parse(const char *pszServerIP)
{
    tsdp_header_O_t *hdr_O = tsdp_header_O_create(tsk_null, 0, 0, tsk_null, tsk_null, tsk_null);
	   
    int len =strlen(pszServerIP);
    hdr_O->addr = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_O->addr, pszServerIP, len);
    
    len = strlen("IP4");
    hdr_O->addrtype = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_O->addrtype, "IP4", len);
    
    len = strlen("IN");
    hdr_O->nettype = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_O->nettype, "IN", len);
	return hdr_O;
}







//========================================================
//	O header object definition
//

static tsk_object_t* tsdp_header_O_ctor(tsk_object_t *self, va_list * app)
{
	tsdp_header_O_t *O = self;
	if(O){
		TSDP_HEADER(O)->type = tsdp_htype_O;
        TSDP_HEADER(O)->rank = TSDP_HTYPE_O_RANK;
		
		
		O->nettype = tsk_strdup(va_arg(*app, const char*));
		O->addrtype = tsk_strdup(va_arg(*app, const char*));
		O->addr = tsk_strdup(va_arg(*app, const char*));
	}
	else{
		TSK_DEBUG_ERROR("Failed to create new O header.");
	}
	return self;
}

static tsk_object_t* tsdp_header_O_dtor(tsk_object_t *self)
{
	tsdp_header_O_t *O = self;
	if(O){
		TSK_FREE(O->nettype);
		TSK_FREE(O->addrtype);
		TSK_FREE(O->addr);
	}
	else{
		TSK_DEBUG_ERROR("Null O header.");
	}

	return self;
}


static const tsk_object_def_t tsdp_header_O_def_s = 
{
	sizeof(tsdp_header_O_t),
	tsdp_header_O_ctor,
	tsdp_header_O_dtor,
	tsk_null
};

const tsk_object_def_t *tsdp_header_O_def_t = &tsdp_header_O_def_s;

﻿/*******************************************************************
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

/**@file tsdp_message.c
 * @brief SDP message.
 */

#include "tinysdp/tsdp_message.h"

#include "tinysdp/headers/tsdp_header_O.h"

#include "tsk_debug.h"

#define TSDP_LINE_S_VALUE_DEFAULT "-"	/* as per RFC 3264 subclause 5 */



/*== Predicate function to find tsdp_header_t object by name. */
static int __pred_find_header_by_name(const tsk_list_item_t *item, const void *name)
{
	if(item && item->data && name){
		tsdp_header_t *header = item->data;
		return tsdp_header_get_nameex(header) - *((const char*)name);
	}
	return -1;
}

/*== Predicate function to find media object by name. */
static int __pred_find_media_by_name(const tsk_list_item_t *item, const void *name)
{
	if(item && item->data && name){
		tsdp_header_t *header = item->data;
		if(header->type == tsdp_htype_M){
			return tsk_stricmp(((tsdp_header_M_t*)header)->media, (const char*)name);
		}
	}
	return -1;
}

tsdp_message_t* tsdp_message_create()
{
	return tsk_object_new(tsdp_message_def_t);
}

/*== Add headers/fmt to the media line */
int __add_headers(tsdp_header_M_t* m, va_list *ap)
{
	const tsk_object_def_t* objdef;
	tsdp_header_t *header;
	tsdp_fmt_t* fmt;
	
	if(!m){
		return -1;
	}
	
	while((objdef = va_arg(*ap, const tsk_object_def_t*))){
		if(objdef == tsdp_fmt_def_t){
			if((fmt = tsk_object_new_2(objdef, ap))){
				tsk_list_push_back_data(m->FMTs, (void**)&fmt);
			}
		}
		else{
			if((header = tsk_object_new_2(objdef, ap))){
				tsdp_header_M_add(m, header);
				TSK_OBJECT_SAFE_FREE(header);
			}
		}
	}
	return 0;
}

int tsdp_message_add_header(tsdp_message_t *self, const tsdp_header_t *hdr)
{
	if(self && hdr){
		tsdp_header_t *header = tsk_object_ref((void*)hdr);
		tsk_list_push_ascending_data(self->headers, (void**)&header); // Very important: Headers MUST appear in a fixed order (see ranks def).

		return 0;
	}
	return -1;
}

int tsdp_message_add_headers(tsdp_message_t *self, ...)
{
	const tsk_object_def_t* objdef;
	tsdp_header_t *header;
	va_list ap;

	if(!self){
		return -1;
	}

	va_start(ap, self);
	while((objdef = va_arg(ap, const tsk_object_def_t*))){
		if((header = tsk_object_new_2(objdef, &ap))){
			tsdp_message_add_header(self, header);
			TSK_OBJECT_SAFE_FREE(header);
		}
	}
	va_end(ap);

	return 0;
}

const tsdp_header_t *tsdp_message_get_headerAt(const tsdp_message_t *self, tsdp_header_type_t type, tsk_size_t index)
{
	tsk_size_t pos = 0;
	const tsk_list_item_t *item;
	const tsdp_header_t *hdr;

	if(!self || !self->headers){
		return tsk_null;
	}

	tsk_list_foreach(item, self->headers){
		hdr = item->data;
		if(hdr->type == type){
			if(pos++ >= index){
				return hdr;
			}
		}
	}
	
	return tsk_null;
}

const tsdp_header_t *tsdp_message_get_header(const tsdp_message_t *self, tsdp_header_type_t type)
{
	return tsdp_message_get_headerAt(self, type, 0);
}

const tsdp_header_A_t* tsdp_message_get_headerA_at(const tsdp_message_t* self, const char* field, tsk_size_t index)
{

	tsk_size_t pos = 0;
	const tsk_list_item_t *item;
	const tsdp_header_t *hdr;
	const tsdp_header_A_t *hdrA;

	if(!self || !self->headers){
		return tsk_null;
	}

	tsk_list_foreach(item, self->headers){
		hdr = item->data;
		if((hdr->type == tsdp_htype_A) && (hdrA = (const tsdp_header_A_t *)hdr) && (tsk_striequals(hdrA->field, field))){
			if(pos++ >= index){
				return hdrA;
			}
		}
	}
	
	return tsk_null;
}

const tsdp_header_A_t* tsdp_message_get_headerA(const tsdp_message_t* self, const char* field)
{
	return tsdp_message_get_headerA_at(self, field, 0);
}

const tsdp_header_t *tsdp_message_get_headerByName(const tsdp_message_t *self, char name)
{
	if(self){
		return tsk_list_find_object_by_pred(self->headers, __pred_find_header_by_name, &name);
	}
	else{
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}
}


tsdp_message_t* tsdp_message_create_empty(const char* addr, tsk_bool_t ipv6, uint32_t version)
{
	tsdp_message_t* ret = 0;

	if(!(ret = tsdp_message_create())){
		return tsk_null;
	}

	/*	RFC 3264 - 5 Generating the Initial Offer
		The numeric value of the session id and version in the o line MUST be 
		representable with a 64 bit signed integer.  The initial value of the version MUST be less than
	   (2**62)-1, to avoid rollovers.
	*/
	TSDP_MESSAGE_ADD_HEADER(ret, TSDP_HEADER_O_VA_ARGS(
        "IN",
		(ipv6 ? "IP6" : "IP4"),
		addr));

	return ret;
}

int tsdp_message_add_media(tsdp_message_t *self, const char* media, uint32_t port, const char* proto, ...)
{
	va_list ap;
	int ret;
		
	va_start(ap, proto);
	ret = tsdp_message_add_media_2(self, media, port, proto, &ap);
	va_end(ap);

	return ret;
}

int tsdp_message_add_media_2(tsdp_message_t *self, const char* media, uint32_t port, const char* proto, va_list *ap)
{
	int ret = -1;
	tsdp_header_M_t* m;

	if(!self){
		return -1;
	}

	if((m = tsdp_header_M_create(media, port, proto))){
		__add_headers(m, ap);
		
		ret = tsdp_message_add_header(self, TSDP_HEADER(m));
		TSK_OBJECT_SAFE_FREE(m);
	}
	
	return ret;
}

int tsdp_message_remove_media(tsdp_message_t *self, const char* media)
{
	if(!self || !media){
		return 0;
	}

	tsk_list_remove_item_by_pred(self->headers, __pred_find_media_by_name, media);
	return 0;
}

const tsdp_header_M_t* tsdp_message_find_media(const tsdp_message_t *self, const char* media)
{
	if(self && media){
		const tsk_list_item_t* item;
		if((item = tsk_list_find_item_by_pred(self->headers, __pred_find_media_by_name, media))){
			return TSDP_HEADER_M(item->data);
		}
	}	
	return tsk_null;
}



/* ================= 3GPP TS 34.610 :: Communication HOLD (HOLD) using IP Multimedia (IM) Core ================*/
int tsdp_message_hold(tsdp_message_t* self, const char* media)
{
	tsdp_header_M_t* M;
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	// 3GPP TS 34.610-900 - 4.5.2.1	Actions at the invoking UE
	if((item = tsk_list_find_item_by_pred(self->headers, __pred_find_media_by_name, media))){
		M = TSDP_HEADER_M(item->data);
		tsdp_header_M_hold(M, tsk_true);
	}

	return 0;
}

int tsdp_message_resume(tsdp_message_t* self, const char* media)
{
	tsdp_header_M_t* M;
	const tsk_list_item_t* item;

	if(!self){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	// 3GPP TS 34.610-900 - 4.5.2.1	Actions at the invoking UE
	if((item = tsk_list_find_item_by_pred(self->headers, __pred_find_media_by_name, media))){
		M = TSDP_HEADER_M(item->data);
		tsdp_header_M_resume(M, tsk_true);
	}

	return 0;
}

tsk_bool_t tsdp_message_is_ice_enabled(const tsdp_message_t *self, tsk_size_t media_index)
{
	if (self) {
		const tsdp_header_A_t *A;
		const tsdp_header_M_t *M;
		tsk_bool_t have_ufrag = tsk_false, have_pwd = tsk_false, have_candidates = tsk_false;
		tsk_size_t index0 = 0;

		// session level attributes
		if ((A = tsdp_message_get_headerA(self, "ice-ufrag"))) {
			have_ufrag = tsk_true;
		}
		if ((A = tsdp_message_get_headerA(self, "ice-pwd"))) {
			have_pwd = tsk_true;
		}
		while ((M = (const tsdp_header_M_t*)tsdp_message_get_headerAt(self, tsdp_htype_M, index0))) {
			if (index0 == media_index) {
				if ((A = tsdp_header_M_findA(M, "ice-ufrag"))) {
					have_ufrag = tsk_true;
				}
				if ((A = tsdp_header_M_findA(M, "ice-pwd"))) {
					have_pwd = tsk_true;
				}
				have_candidates = (tsdp_header_M_findA_at(M, "candidate", 0) != tsk_null);
				return have_ufrag && have_pwd && have_candidates;
			}
			++index0;
		}
	}
	return tsk_false;
}

tsk_bool_t tsdp_message_is_ice_restart(const tsdp_message_t *self, tsk_size_t media_index)
{
	// https://tools.ietf.org/html/rfc5245#section-9.1.1.1
	if (self) {
		const tsdp_header_C_t *C;
		const tsdp_header_M_t *M;
		tsk_size_t index0 = 0;

		// Session level
		if ((C = (const tsdp_header_C_t*)tsdp_message_get_header(self, tsdp_htype_C)) && C->addr){
			if (tsk_striequals("0.0.0.0", C->addr)) {
				return tsk_true;
			}
		}
		// Media level
		while ((M = (const tsdp_header_M_t*)tsdp_message_get_headerAt(self, tsdp_htype_M, index0))) {
			if (index0 == media_index) {
				return (M->C && M->C->addr && tsk_striequals("0.0.0.0", M->C->addr));
			}
			++index0;
		}
	}
	return tsk_false;
}













//=================================================================================================
//	SDP object definition
//
static void* tsdp_message_ctor(void * self, va_list * app)
{
	tsdp_message_t *message = self;
	if(message){
		message->headers = tsk_list_create();
	}
	return self;
}

static void* tsdp_message_dtor(void * self)
{ 
	tsdp_message_t *message = self;
	if(message){
		TSK_OBJECT_SAFE_FREE(message->headers);
	}
	return self;
}

static const tsk_object_def_t tsdp_message_def_s = 
{
	sizeof(tsdp_message_t),
	tsdp_message_ctor, 
	tsdp_message_dtor,
	tsk_null, 
};
const tsk_object_def_t *tsdp_message_def_t = &tsdp_message_def_s;

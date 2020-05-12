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

/**@file tsdp_header_A.c
 * @brief SDP "a=" header (Attributes).
 * 
 */
#include "tinysdp/headers/tsdp_header_A.h"
#include "tmedia_codec.h"
#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"

#include <string.h>

/***********************************
*	Ragel state machine.
*/

/* #line 62 "./ragel/tsdp_parser_header_A.rl" */




tsdp_header_A_t* tsdp_header_A_create(const char* field, const char* value)
{
        return tsk_object_new(TSDP_HEADER_A_VA_ARGS(field, value));
}

tsdp_header_A_t* tsdp_header_A_create_null()
{
        return tsdp_header_A_create(tsk_null, tsk_null);
}


tsdp_header_A_t *tsdp_header_A_parse(int iSampleRate,int iChannels)
{
    tsdp_header_A_t *hdr_A = tsdp_header_A_create_null();
   
    int len =strlen("fmtp");
    hdr_A->field = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_A->field, "fmtp", len);
    
    len = strlen("111 maxplaybackrate=48000; sprop-maxcapturerate=48000; stereo=0; sprop-stereo=0; useinbandfec=0; usedtx=0");
    hdr_A->value = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_A->value, "111 maxplaybackrate=48000; sprop-maxcapturerate=48000; stereo=0; sprop-stereo=0; useinbandfec=0; usedtx=0", len);
    
	return hdr_A;
}


int tsdp_header_A_removeAll_by_field(tsdp_headers_A_L_t *attributes, const char* field)
{
        tsk_list_item_t* item;
        const tsdp_header_A_t* A;

        if(!attributes || !field){
                TSK_DEBUG_ERROR("Invalid parameter");
                return -1;
        }

again:
        tsk_list_foreach(item, attributes){
                if(!(A = item->data) || TSDP_HEADER(A)->type != tsdp_htype_A){
                        continue;
                }
                if(tsk_striequals(field, A->field)){
                        tsk_list_remove_item(attributes, item);
                        goto again;
                }
        }

        return 0;
}

int tsdp_header_A_removeAll_by_fields(tsdp_headers_A_L_t *attributes, const char** fields, tsk_size_t fields_count)
{
        tsk_size_t i;

        if(!attributes || !fields){
                TSK_DEBUG_ERROR("Invalid parameter");
                return -1;
        }

        for(i = 0; i < fields_count; ++i){
                if(!fields[i]){
                        continue;
                }
                tsdp_header_A_removeAll_by_field(attributes, fields[i]);
        }
        return 0;
}




//========================================================
//      A header object definition
//

static tsk_object_t* tsdp_header_A_ctor(tsk_object_t *self, va_list * app)
{
        tsdp_header_A_t *A = self;
        if(A)
        {
                TSDP_HEADER(A)->type = tsdp_htype_A;
                TSDP_HEADER(A)->rank = TSDP_HTYPE_A_RANK;
                
                A->field = tsk_strdup(va_arg(*app, const char*));
                A->value = tsk_strdup(va_arg(*app, const char*));
        }
        else{
                TSK_DEBUG_ERROR("Failed to create new A header.");
        }
        return self;
}

static tsk_object_t* tsdp_header_A_dtor(tsk_object_t *self)
{
        tsdp_header_A_t *A = self;
        if(A){
                TSK_FREE(A->field);
                TSK_FREE(A->value);
        }
        else{
                TSK_DEBUG_ERROR("Null A header.");
        }

        return self;
}


static const tsk_object_def_t tsdp_header_A_def_s = 
{
        sizeof(tsdp_header_A_t),
        tsdp_header_A_ctor,
        tsdp_header_A_dtor,
        tsk_null
};

const tsk_object_def_t *tsdp_header_A_def_t = &tsdp_header_A_def_s;

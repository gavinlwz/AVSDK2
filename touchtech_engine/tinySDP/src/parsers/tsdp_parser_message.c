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
/**@file tsdp_machine_message.rl
 * @brief Ragel file.
 */
#include "tinysdp/parsers/tsdp_parser_message.h"

#include "tinysdp/headers/tsdp_header_A.h"
#include "tinysdp/headers/tsdp_header_C.h"
#include "tinysdp/headers/tsdp_header_M.h"
#include "tinysdp/headers/tsdp_header_O.h"

#include "tsk_debug.h"


/* #line 238 "./ragel/tsdp_parser_message.rl" */


TSK_RAGEL_DISABLE_WARNINGS_BEGIN()
/* Ragel data */

/* #line 243 "./ragel/tsdp_parser_message.rl" */
TSK_RAGEL_DISABLE_WARNINGS_END()

tsdp_message_t* tsdp_message_parse(const char*  pszServerIP , int iRtpPort,int iSampleRate)
{
	tsdp_message_t* sdp_msg = tsdp_message_create();
	tsdp_header_t *header = tsk_null;
	tsdp_header_M_t *hdr_M = tsk_null;

    if((header = (tsdp_header_t*)tsdp_header_O_parse(pszServerIP)))
    {
        tsdp_message_add_header(sdp_msg, header);
        tsk_object_unref(header);
    }
   
   
    if((hdr_M = tsdp_header_M_parse(iRtpPort, "video"))){
        tsdp_message_add_header(sdp_msg, TSDP_HEADER(hdr_M));
        hdr_M = tsk_object_unref(hdr_M);
    }
  
    if((hdr_M = tsdp_header_M_parse(iRtpPort, "audio"))){
        tsdp_message_add_header(sdp_msg, TSDP_HEADER(hdr_M));
        hdr_M = tsk_object_unref(hdr_M);
    }
  
    
    if(hdr_M && !hdr_M->C){
        hdr_M->C = tsdp_header_C_parse(pszServerIP);
    }
    else if((header = (tsdp_header_t*)tsdp_header_C_parse(pszServerIP))){
        tsdp_message_add_header(sdp_msg, header);
        tsk_object_unref(header);
    }
    
    if(hdr_M){
        if(!hdr_M->Attributes){
            hdr_M->Attributes = tsk_list_create();
        }
        if((header = (tsdp_header_t*)tsdp_header_A_parse(iSampleRate,2))){
            tsk_list_push_back_data(hdr_M->Attributes, (void**)&header);
        }
    }
    else if((header = (tsdp_header_t*)tsdp_header_A_parse(iSampleRate,2))){
        tsdp_message_add_header(sdp_msg, header);
        tsk_object_unref(header);
    }
	return sdp_msg;
}

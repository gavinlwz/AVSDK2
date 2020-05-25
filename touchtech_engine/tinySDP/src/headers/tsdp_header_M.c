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


/**@file tsdp_header_M.c
 * @brief SDP "m=" header (Media Descriptions).
 *
 */
#include "tinysdp/headers/tsdp_header_M.h"

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "tmedia_codec.h"
#include <string.h>

/***********************************
*	Ragel state machine.
*/

/* #line 78 "./ragel/tsdp_parser_header_M.rl" */


tsdp_header_M_t *tsdp_header_M_create (const char *media, uint32_t port, const char *proto)
{
    return tsk_object_new (TSDP_HEADER_M_VA_ARGS (media, port, proto));
}

tsdp_header_M_t *tsdp_header_M_create_null ()
{
    return tsdp_header_M_create (tsk_null, 0, tsk_null);
}

tsdp_fmt_t *tsdp_fmt_create (const char *fmt)
{
    return tsk_object_new (TSDP_FMT_VA_ARGS (fmt));
}

tsdp_header_M_t *tsdp_header_M_parse (int iRtpPort, char* type_name)
{
    tsdp_header_M_t *hdr_M = tsdp_header_M_create_null ();
    int len =strlen(type_name);
    hdr_M->media = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_M->media, type_name, len);
    hdr_M->port = iRtpPort;
    
    
    len =strlen("RTP/AVP");
    hdr_M->proto = (char*)tsk_calloc(len+1, sizeof(char));
    memcpy(hdr_M->proto, "RTP/AVP", len);
    
    tsk_string_t *string = tsk_string_create(tsk_null);
    if (tsk_striequals(type_name, "audio")) {
        len = strlen(TMEDIA_CODEC_FORMAT_OPUS);
        string->value = (char*)tsk_calloc(len+1, sizeof(char)), memcpy(string->value, TMEDIA_CODEC_FORMAT_OPUS, len);
    } else if (tsk_striequals(type_name, "video")) {
        len = strlen(TMEDIA_CODEC_FORMAT_H264_MP);
        string->value = (char*)tsk_calloc(len+1, sizeof(char)), memcpy(string->value, TMEDIA_CODEC_FORMAT_H264_MP, len);
    }
    
    if(!hdr_M->FMTs)
    {
        hdr_M->FMTs = tsk_list_create();
    }
    tsk_list_push_back_data(hdr_M->FMTs, ((void**) &string)); 
    
    
    
    return hdr_M;
}


// for A headers, use "tsdp_header_A_removeAll_by_field()"
int tsdp_header_M_remove (tsdp_header_M_t *self, tsdp_header_type_t type)
{
    switch (type)
    {
    case tsdp_htype_C:
    {
        TSK_OBJECT_SAFE_FREE (self->C);
        break;
    }
   
    default:
        break;
    }
    return 0;
}

int tsdp_header_M_add (tsdp_header_M_t *self, const tsdp_header_t *header)
{
    if (!self || !header)
    {
        return -1;
    }

    switch (header->type)
    {
   
    case tsdp_htype_C:
    {
        TSK_OBJECT_SAFE_FREE (self->C);
        self->C = tsk_object_ref ((void *)header);
        break;
    }
       
    case tsdp_htype_A:
    {
        tsdp_header_t *A = tsk_object_ref ((void *)header);
        if (!self->Attributes)
        {
            self->Attributes = tsk_list_create ();
        }
        tsk_list_push_back_data (self->Attributes, (void **)&A);
        break;
    }
    default:
        break;
    }

    return 0;
}

int tsdp_header_M_add_headers (tsdp_header_M_t *self, ...)
{
    const tsk_object_def_t *objdef;
    tsdp_header_t *header;
    tsdp_fmt_t *fmt;
    va_list ap;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    va_start (ap, self);
    while ((objdef = va_arg (ap, const tsk_object_def_t *)))
    {
        if (objdef == tsdp_fmt_def_t)
        {
            if ((fmt = tsk_object_new_2 (objdef, &ap)))
            {
                tsk_list_push_back_data (self->FMTs, (void **)&fmt);
            }
        }
        else
        {
            if ((header = tsk_object_new_2 (objdef, &ap)))
            {
                tsdp_header_M_add (self, header);
                TSK_OBJECT_SAFE_FREE (header);
            }
        }
    }
    va_end (ap);

    return 0;
}

int tsdp_header_M_add_headers_2 (tsdp_header_M_t *self, const tsdp_headers_L_t *headers)
{
    const tsk_list_item_t *item;

    if (!self || !headers)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    tsk_list_foreach (item, headers)
    {
        tsdp_header_M_add (self, item->data);
    }

    return 0;
}

int tsdp_header_M_add_fmt (tsdp_header_M_t *self, const char *fmt)
{
    tsdp_fmt_t *_fmt;
    if (!self || tsk_strnullORempty (fmt))
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if ((_fmt = tsdp_fmt_create (fmt)))
    {
        tsk_list_push_back_data (self->FMTs, (void **)&_fmt);
        return 0;
    }
    else
    {
        TSK_DEBUG_ERROR ("Failed to create fmt object");
        return -2;
    }
}

int tsdp_header_M_remove_fmt (tsdp_header_M_t *self, const char *fmt)
{
    const tsk_list_item_t *itemM;
    const tsdp_fmt_t *_fmt;
    char *fmt_plus_space = tsk_null;
    tsk_size_t fmt_plus_space_len;
    if (!self || tsk_strnullORempty (fmt))
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    tsk_sprintf (&fmt_plus_space, "%s ", fmt);
    if ((fmt_plus_space_len = tsk_strlen (fmt_plus_space)))
    {
        tsk_list_foreach (itemM, self->FMTs)
        {
            if (!(_fmt = (const tsdp_fmt_t *)itemM->data))
            {
                continue;
            }
            if (tsk_striequals (_fmt->value, fmt))
            {
                // remove all A headers using this attribute
                const tsdp_header_A_t *A;
                const tsk_list_item_t *itemA;
            removeAttributes:
                tsk_list_foreach (itemA, self->Attributes)
                {
                    if (!(A = (const tsdp_header_A_t *)itemA->data))
                    {
                        continue;
                    }
                    if (tsk_strindexOf (A->value, fmt_plus_space_len, fmt_plus_space) == 0)
                    {
                        // Guard to be sure we know what to remove. For example:
                        // tsdp_header_M_remove_fmt(self, 0) would remove both
                        // a=rtpmap:0 PCMU/8000/1
                        // a=crypto:0 AES_CM_128_HMAC_SHA1_32 inline:Gi8s25tDKDnd/xORJ/ZtRWWC1drVbax5Ve4ftCWd
                        // and cause issue 115: https://code.google.com/p/webrtc2sip/issues/detail?id=115
                        if (!tsk_striequals (A->field, "crypto"))
                        {
                            tsk_list_remove_item (self->Attributes, (tsk_list_item_t *)itemA);
                            goto removeAttributes;
                        }
                    }
                }
                tsk_list_remove_item (self->FMTs, (tsk_list_item_t *)itemM);
                break;
            }
        }
    }
    TSK_FREE (fmt_plus_space);
    return 0;
}

tsk_bool_t tsdp_header_M_have_fmt (const tsdp_header_M_t *self, const char *fmt)
{
    if (self && !tsk_strnullORempty (fmt))
    {
        const tsk_list_item_t *item;
        const tsdp_fmt_t *_fmt;
        tsk_list_foreach (item, self->FMTs)
        {
            if (!(_fmt = (const tsdp_fmt_t *)item->data))
            {
                continue;
            }
            if (tsk_striequals (_fmt->value, fmt))
            {
                return tsk_true;
            }
        }
    }

    return tsk_false;
}

const tsdp_header_A_t *tsdp_header_M_findA_at (const tsdp_header_M_t *self, const char *field, tsk_size_t index)
{
    const tsk_list_item_t *item;
    tsk_size_t pos = 0;
    const tsdp_header_A_t *A;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    tsk_list_foreach (item, self->Attributes)
    {
        if (!(A = item->data))
        {
            continue;
        }

        if (tsk_strequals (A->field, field))
        {
            if (pos++ >= index)
            {
                return A;
            }
        }
    }

    return tsk_null;
}

const tsdp_header_A_t *tsdp_header_M_findA (const tsdp_header_M_t *self, const char *field)
{
    return tsdp_header_M_findA_at (self, field, 0);
}

char *tsdp_header_M_getAValue (const tsdp_header_M_t *self, const char *field, const char *fmt)
{
    char *value = tsk_null; /* e.g. AMR-WB/16000 */
    tsk_size_t i = 0, fmt_len, A_len;
    int indexof;
    const tsdp_header_A_t *A;

    fmt_len = tsk_strlen (fmt);
    if (!self || !fmt_len || fmt_len > 3 /*'0-255' or '*'*/)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    /* find "a=rtpmap" */
    while ((A = tsdp_header_M_findA_at (self, field, i++)))
    {
        /* A->value would be: "98 AMR-WB/16000" */
        if ((A_len = tsk_strlen (A->value)) < (fmt_len + 1 /*space*/))
        {
            continue;
        }
        if ((indexof = tsk_strindexOf (A->value, A_len, fmt)) == 0 && (A->value[fmt_len] == ' '))
        {
            value = tsk_strndup (&A->value[fmt_len + 1], (A_len - (fmt_len + 1)));
            break;
        }
    }
    return value;
}

/* as per 3GPP TS 34.610 */
int tsdp_header_M_hold (tsdp_header_M_t *self, tsk_bool_t local)
{
    const tsdp_header_A_t *a;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if ((a = tsdp_header_M_findA (self, local ? "recvonly" : "sendonly")))
    {
        // an "inactive" SDP attribute if the stream was previously set to "recvonly" media stream
        tsk_strupdate (&(TSDP_HEADER_A (a)->field), local ? "inactive" : "recvonly");
    }
    else if ((a = tsdp_header_M_findA (self, "sendrecv")))
    {
        // a "sendonly" SDP attribute if the stream was previously set to "sendrecv" media stream
        tsk_strupdate (&(TSDP_HEADER_A (a)->field), local ? "sendonly" : "recvonly");
    }
    else
    {
        // default value is sendrecv. hold on default --> sendonly
        if (!(a = tsdp_header_M_findA (self, local ? "sendonly" : "recvonly")) && !(a = tsdp_header_M_findA (self, "inactive")))
        {
            tsdp_header_A_t *newA;
            if ((newA = tsdp_header_A_create (local ? "sendonly" : "recvonly", tsk_null)))
            {
                tsdp_header_M_add (self, TSDP_HEADER_CONST (newA));
                TSK_OBJECT_SAFE_FREE (newA);
            }
        }
    }
    return 0;
}

/* as per 3GPP TS 34.610 */
int tsdp_header_M_set_holdresume_att (tsdp_header_M_t *self, tsk_bool_t lo_held, tsk_bool_t ro_held)
{
    const tsdp_header_A_t *A;
    static const char *hold_resume_atts[2][2] = {
        { "sendrecv", "recvonly" }, { "sendonly", "inactive" },
    };
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if ((A = tsdp_header_M_findA (self, "sendrecv")) || (A = tsdp_header_M_findA (self, "sendonly")) || (A = tsdp_header_M_findA (self, "recvonly")) || (A = tsdp_header_M_findA (self, "inactive")))
    {
        tsk_strupdate (&(TSDP_HEADER_A (A)->field), hold_resume_atts[lo_held & 1][ro_held & 1]);
    }
    else
    {
        tsdp_header_A_t *newA;
        if ((newA = tsdp_header_A_create (hold_resume_atts[lo_held & 1][ro_held & 1], tsk_null)))
        {
            tsdp_header_M_add (self, TSDP_HEADER_CONST (newA));
            TSK_OBJECT_SAFE_FREE (newA);
        }
    }

    return 0;
}

const char *tsdp_header_M_get_holdresume_att (const tsdp_header_M_t *self)
{
    static const char *hold_resume_atts[4] = { "sendrecv" /*first because most likely to be present*/, "recvonly", "sendonly", "inactive" };
    static tsk_size_t hold_resume_atts_count = sizeof (hold_resume_atts) / sizeof (hold_resume_atts[0]);
    tsk_size_t i;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return hold_resume_atts[0];
    }
    for (i = 0; i < hold_resume_atts_count; ++i)
    {
        if (tsdp_header_M_findA (self, hold_resume_atts[i]))
        {
            return hold_resume_atts[i];
        }
    }
    return hold_resume_atts[0];
}

tsk_bool_t tsdp_header_M_is_held (const tsdp_header_M_t *self, tsk_bool_t local)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }

    /* both cases */
    if (tsdp_header_M_findA (self, "inactive"))
    {
        return tsk_true;
    }

    if (local)
    {
        return tsdp_header_M_findA (self, "recvonly") ? tsk_true : tsk_false;
    }
    else
    {
        return tsdp_header_M_findA (self, "sendonly") ? tsk_true : tsk_false;
    }
}

/* as per 3GPP TS 34.610 */
int tsdp_header_M_resume (tsdp_header_M_t *self, tsk_bool_t local)
{
    const tsdp_header_A_t *a;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if ((a = tsdp_header_M_findA (self, "inactive")))
    {
        // a "recvonly" SDP attribute if the stream was previously an inactive media stream
        tsk_strupdate (&(TSDP_HEADER_A (a)->field), local ? "recvonly" : "sendonly");
    }
    else if ((a = tsdp_header_M_findA (self, local ? "sendonly" : "recvonly")))
    {
        // a "sendrecv" SDP attribute if the stream was previously a sendonly media stream, or the attribute may be omitted, since sendrecv is the default
        tsk_strupdate (&(TSDP_HEADER_A (a)->field), "sendrecv");
    }
    return 0;
}

tsk_bool_t tsdp_header_M_is_ice_enabled (const tsdp_header_M_t *self)
{
    if (self)
    {
        const tsdp_header_A_t *A;
        tsk_bool_t have_ufrag = tsk_false, have_pwd = tsk_false, have_candidates = tsk_false;

        if ((A = tsdp_header_M_findA (self, "ice-ufrag")))
        {
            have_ufrag = tsk_true;
        }
        if ((A = tsdp_header_M_findA (self, "ice-pwd")))
        {
            have_pwd = tsk_true;
        }
        have_candidates = (tsdp_header_M_findA_at (self, "candidate", 0) != tsk_null);
        return have_ufrag && have_pwd && have_candidates;
    }
    return tsk_false;
}

tsk_bool_t tsdp_header_M_is_ice_restart (const tsdp_header_M_t *self)
{
    // https://tools.ietf.org/html/rfc5245#section-9.1.1.1
    if (self)
    {
        return (self->C && self->C->addr && tsk_striequals ("0.0.0.0", self->C->addr));
    }
    return tsk_false;
}

int tsdp_header_M_diff (const tsdp_header_M_t *M_old, const tsdp_header_M_t *M_new, tsdp_header_M_diff_t *_diff)
{
    tsdp_header_M_diff_t diff = tsdp_header_M_diff_none;
    const tsdp_header_A_t *A0, *A1;
    int ret = 0;
    tsk_size_t index;
    if (!M_old || !_diff)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (!M_new || !tsk_striequals (M_new->media, M_old->media))
    {
        // media lines must be at the same index
        // (M1 == null) means media lines are not at the same index or new one have been added/removed
        diff |= tsdp_header_M_diff_index;
    }

    // hold/resume
    if (M_new && !tsk_striequals (tsdp_header_M_get_holdresume_att (M_old), tsdp_header_M_get_holdresume_att (M_new)))
    {
        diff |= tsdp_header_M_diff_hold_resume;
    }

    // dtls fingerprint
    A0 = tsdp_header_M_findA_at (M_old, "fingerprint", 0);
    A1 = M_new ? tsdp_header_M_findA_at (M_new, "fingerprint", 0) : tsk_null;
    if (A0 && A1 && !tsk_striequals (A0->value, A1->value))
    {
        diff |= tsdp_header_M_diff_dtls_fingerprint;
    }

    // sdes cryptos
    index = 0;
    do
    {
        A0 = tsdp_header_M_findA_at (M_old, "crypto", index);
        A1 = M_new ? tsdp_header_M_findA_at (M_new, "crypto", index) : tsk_null;
        if (A0 && A1)
        {
            if (!tsk_striequals (A0->value, A1->value))
            {
                diff |= tsdp_header_M_diff_sdes_crypto;
            }
        }
        else if (index == 0 && (A0 || A1))
        { // (A1 && !AO) means "more" crypto lines, otherwise "less". In all cases if the first matched we're ok
            diff |= tsdp_header_M_diff_sdes_crypto;
        }
        ++index;
    } while ((A0 && A1) && ((diff & tsdp_header_M_diff_sdes_crypto) != tsdp_header_M_diff_sdes_crypto));

    // media lines
    if ((diff & tsdp_header_M_diff_index) != tsdp_header_M_diff_index)
    {
        char *M0ProtoF = tsk_null; // old proto with "F" at the end, do nothing if we transit from AVP to AVPF because it means nego. succeeded
        tsk_strcat_2 (&M0ProtoF, "%sF", M_old->proto);
        if (
        // (M1 == null) means media lines are not at the same index or new one have been added/removed
        (!M_new)
        // same media (e.g. audio)
        ||
        !tsk_striequals (M_new->media, M_old->media)
        // same protos (e.g. SRTP)
        ||
        !(tsk_striequals (M_new->proto, M_old->proto) || tsk_striequals (M_new->proto, M0ProtoF)))
        {
            diff |= tsdp_header_M_diff_index;
        }
        TSK_FREE (M0ProtoF);
    }

    // codecs
    if (M_new && (diff & tsdp_header_M_diff_index) != tsdp_header_M_diff_index)
    { // make sure it's same media at same index
        if (tsk_list_count_all (M_old->FMTs) == tsk_list_count_all (M_new->FMTs))
        {
            if ((diff & tsdp_header_M_diff_index) != tsdp_header_M_diff_index)
            { // make sure it's same media at same index
                const tsk_list_item_t *codec_item0;
                const tsdp_fmt_t *codec_fmt1;
                tsk_size_t codec_index0 = 0;
                tsk_list_foreach (codec_item0, M_old->FMTs)
                {
                    codec_fmt1 = (const tsdp_fmt_t *)tsk_list_find_object_by_pred_at_index (M_new->FMTs, tsk_null, tsk_null, codec_index0++);
                    if (!codec_fmt1 || !tsk_striequals (codec_fmt1->value, ((const tsdp_fmt_t *)codec_item0->data)->value))
                    {
                        diff |= tsdp_header_M_diff_codecs;
                        break;
                    }
                }
            }
        }
        else
        {
            diff |= tsdp_header_M_diff_codecs;
        }
    }

    // network ports
    if ((M_new ? M_new->port : 0) != (M_old->port))
    {
        diff |= tsdp_header_M_diff_network_info;
    }

    if (M_new)
    {
        if ((diff & tsdp_header_M_diff_network_info) != tsdp_header_M_diff_network_info)
        {
            // Connection informations must be both "null" or "not-null"
            if (!((M_old->C && M_new->C) || (!M_old->C && !M_new->C)))
            {
                diff |= tsdp_header_M_diff_network_info;
            }
            else if (M_old->C)
            {
                if (!tsk_strequals (M_new->C->addr, M_old->C->addr) || !tsk_strequals (M_new->C->nettype, M_old->C->nettype) || !tsk_strequals (M_new->C->addrtype, M_old->C->addrtype))
                {
                    diff |= tsdp_header_M_diff_network_info;
                }
            }
        }
    }

    // media type
    if (M_new)
    {
        if (!tsk_striequals (M_new->media, M_old->media))
        {
            diff |= tsdp_header_M_diff_media_type;
        }
        else
        {
            if (tsk_striequals (M_new->media, "audio") || tsk_striequals (M_new->media, "video"))
            {
                const char *content0, *content1;
                // check if it's BFCP audio/video session
                // content attribute described in http://tools.ietf.org/html/rfc4796
                A0 = tsdp_header_M_findA (M_old, "content");
                A1 = tsdp_header_M_findA (M_new, "content");
                content0 = A0 ? A0->value : "main";
                content1 = A1 ? A1->value : "main";
                if (!tsk_striequals (content0, content1))
                {
                    diff |= tsdp_header_M_diff_media_type;
                }
            }
        }
    }

    // ice
    if (M_new)
    {
        if (tsdp_header_M_is_ice_enabled (M_new))
        {
            diff |= tsdp_header_M_diff_ice_enabled;
        }
        if (tsdp_header_M_is_ice_restart (M_new))
        {
            diff |= tsdp_header_M_diff_ice_restart;
        }
    }

    *_diff = diff;
    return ret;
}

//
// int tsdp_header_M_set(tsdp_header_M_t* self, ...)
//{
//	int ret = -1;
//	va_list params;
//	int type;
//
//	va_start(params, self);
//
//	if(!m){
//		goto bail;
//	}
//
//	while((type=va_arg(params, int))){
//		switch(type){
//			case 0x01: /* FMT */
//			{
//				tsk_string_t* fmt = tsk_string_create(va_arg(values, const char *));
//				if(fmt){
//					tsk_list_push_back_data(sefl->FMTs, (void**)&fmt);
//				}
//				break;
//			}
//			case 0x02: /* A */
//			{
//				tsdp_header_A_t* A = tsdp_header_A_create(va_arg(values, const char *), va_arg(values, const char *));
//				if(A){
//					if(!M->Attributes){
//						M->Attributes = tsk_list_create();
//					}
//					tsk_list_push_back_data(M->Attributes, (void**)&A);
//				}
//				break;
//			}
//		}
//	}
//
// bail:
//	va_end(params);
//	return ret;
//}


//========================================================
//	M header object definition
//

static tsk_object_t *tsdp_header_M_ctor (tsk_object_t *self, va_list *app)
{
    tsdp_header_M_t *M = self;
    if (M)
    {
        TSDP_HEADER (M)->type = tsdp_htype_M;
        TSDP_HEADER (M)->rank = TSDP_HTYPE_M_RANK;

        M->FMTs = tsk_list_create (); // Because there is at least one fmt.

        M->media = tsk_strdup (va_arg (*app, const char *));
        M->port = va_arg (*app, uint32_t);
        M->proto = tsk_strdup (va_arg (*app, const char *));
    }
    else
    {
        TSK_DEBUG_ERROR ("Failed to create new M header.");
    }
    return self;
}

static tsk_object_t *tsdp_header_M_dtor (tsk_object_t *self)
{
    tsdp_header_M_t *M = self;
    if (M)
    {
        TSK_FREE (M->media);
        TSK_FREE (M->proto);
        TSK_OBJECT_SAFE_FREE (M->FMTs);

                TSK_OBJECT_SAFE_FREE (M->C);
        TSK_OBJECT_SAFE_FREE (M->Attributes);
    }
    else
    {
        TSK_DEBUG_ERROR ("Null M header.");
    }

    return self;
}


static const tsk_object_def_t tsdp_header_M_def_s = { sizeof (tsdp_header_M_t), tsdp_header_M_ctor, tsdp_header_M_dtor,tsk_null };

const tsk_object_def_t *tsdp_header_M_def_t = &tsdp_header_M_def_s;

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
/**@file tsdp.h
 * @brief SDP (RFC 4566) implementations with both MMTel and PoC extensions.
 *
 *
 *

 */
#ifndef TINYSDP_TSDP_H
#define TINYSDP_TSDP_H

#include "tinysdp_config.h"

#include "tinysdp/tsdp_message.h"

TSDP_BEGIN_DECLS

#define TSDP_CTX_CREATE()	tsk_object_new(tsdp_ctx_def_t)

typedef void tsdp_ctx_handle_t;

TINYSDP_API const tsdp_message_t* tsdp_ctx_local_get_sdp(const tsdp_ctx_handle_t* self);

TINYSDP_API int tsdp_ctx_local_add_headers(tsdp_ctx_handle_t* self, ...);
TINYSDP_API int tsdp_ctx_local_add_media(tsdp_ctx_handle_t* self, const tsdp_header_M_t* media);
TINYSDP_API int tsdp_ctx_local_add_media_2(tsdp_ctx_handle_t* self, const char* media, uint32_t port, const char* proto, ...);

TINYSDP_API const tsdp_message_t* tsdp_ctx_remote_get_sdp(const tsdp_ctx_handle_t* self);

TINYSDP_API const tsdp_message_t* tsdp_ctx_negotiated_get_sdp(const tsdp_ctx_handle_t* self);


// 3GPP TS 24.610 Communication HOLD
TINYSDP_API int tsdp_ctx_hold(tsdp_ctx_handle_t* self, const char* media);
TINYSDP_API int tsdp_ctx_resume(tsdp_ctx_handle_t* self, const char* media);

TINYSDP_GEXTERN const tsk_object_def_t *tsdp_ctx_def_t;

TSDP_END_DECLS


#endif /* TINYSDP_TSDP_H */

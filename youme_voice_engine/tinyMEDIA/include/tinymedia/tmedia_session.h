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
#ifndef TINYMEDIA_SESSION_H
#define TINYMEDIA_SESSION_H

#include "tinymedia_config.h"

#include "tinymedia/tmedia_codec.h"
#include "tinymedia/tmedia_params.h"

#include "tinysdp/tsdp_message.h"

#include "tnet_types.h"

#include "tsk_list.h"
#include "tsk_debug.h"
#include "tsk_safeobj.h"

TMEDIA_BEGIN_DECLS

struct tmedia_session_s;

// rfc5168 (XML Schema for Media Control) commands
typedef enum tmedia_session_rfc5168_cmd_e
{
    tmedia_session_rfc5168_cmd_picture_fast_update,
} tmedia_session_rfc5168_cmd_t;
// BFCP (rfc4582) events
typedef enum tmedia_session_bfcp_evt_type_e
{
    tmedia_session_bfcp_evt_type_err,          // Global error
    tmedia_session_bfcp_evt_type_flreq_status, // FloorRequestStatus
} tmedia_session_bfcp_evt_type_t;

typedef struct tmedia_session_bfcp_evt_xs
{
    tmedia_session_bfcp_evt_type_t type;
    const char *reason;
    union
    {
        struct
        {
            int code;
        } err;
        struct
        {
            uint16_t status;
        } flreq;
    };
} tmedia_session_bfcp_evt_xt;

#define TMEDIA_SESSION(self) ((tmedia_session_t *)(self))
#define TMEDIA_SESSION_AUDIO(self) ((tmedia_session_audio_t *)(self))
#define TMEDIA_SESSION_VIDEO(self) ((tmedia_session_video_t *)(self))

typedef int (*tmedia_session_t140_ondata_cb_f)(const void *usrdata,
                                               tmedia_t140_data_type_t data_type,
                                               const void *data_ptr,
                                               unsigned data_size);
typedef int (*tmedia_session_rtcp_onevent_cb_f)(const void *usrdata,
                                                tmedia_rtcp_event_type_t event_type,
                                                uint32_t ssrc_media);
typedef int (*tmedia_session_onerror_cb_f)(const void *usrdata,
                                           const struct tmedia_session_s *session,
                                           const char *reason,
                                           tsk_bool_t is_fatal);

typedef int (*tmedia_session_bfcp_cb_f)(const void *usrdata,
                                        const struct tmedia_session_s *session,
                                        const tmedia_session_bfcp_evt_xt *evt);

typedef int (*tmedia_session_rfc5168_cb_f)(const void* usrdata, const struct tmedia_session_s* session, const char* reason, enum tmedia_session_rfc5168_cmd_e command);

/**Max number of plugins (session types) we can create */
#define TMED_SESSION_MAX_PLUGINS 0x0F

/** Base objct used for all media sessions */
typedef struct tmedia_session_s
{
    TSK_DECLARE_OBJECT;

    //! unique id. If you want to modifiy this field then you must use @ref
    //tmedia_session_get_unique_id()
    uint64_t id;
    //! session type
    tmedia_type_t type;
    //! list of codec ids used as filter on the enabled codecs
    tmedia_codec_id_t codecs_allowed;
    //! list of codecs managed by this session (enabled)
    tmedia_codecs_L_t *codecs;
    //! negociated codec
    tmedia_codecs_L_t *neg_codecs;
    //! whether the ro have been prepared (up to the manager to update the value)
    tsk_bool_t ro_changed;
    //! whether the session have been initialized (up to the manager to update the value)
    tsk_bool_t initialized;
    //! whether the session have been prepared (up to the manager to update the value)
    tsk_bool_t prepared;
    //! whether the session is localy held
    tsk_bool_t lo_held;
    //! whether the session is remotely held
    tsk_bool_t ro_held;
    //! bandwidth level
    tmedia_bandwidth_level_t bl;
    //! error callback function: not part of the plugin (likes .t140 or .rtcp) because it's not part
    //of the API
    struct
    {
        tmedia_session_onerror_cb_f fun;
        const void *usrdata;
    } onerror_cb;

	struct {
		tmedia_session_rfc5168_cb_f fun;
		const void* usrdata;
	} rfc5168_cb;
	
    //! BFCP (rfc4582)
    struct
    {
        tmedia_session_bfcp_cb_f fun;
        const void *usrdata;
    } bfcp_cb;

    tsk_bool_t bypass_encoding;
    tsk_bool_t bypass_decoding;

    struct {
        float q1; /**< 2.1.Packet loss recovery */
        float q2; /**< 2.2.Global packet loss estimation */
        float q3; /**< 2.3.Receiver Estimated Maximum Bitrate */
        float q4; /**< 2.4.Latency estimation */
        float q5; /**< 2.5. Jitter buffer congestion estimation */
        float qvag;
        unsigned bw_up_est_kbps;
        unsigned bw_down_est_kbps;
        unsigned video_out_width;
        unsigned video_out_height;
        unsigned video_in_width;
        unsigned video_in_height;
        unsigned video_in_avg_fps;
        unsigned video_dec_avg_time;
        unsigned video_enc_avg_time;
        uint64_t last_update_time;
    } qos_metrics;
        
    struct
    {
        tsdp_header_M_t *lo;
        tsdp_header_M_t *ro;
    } M;
    int32_t sessionid;
    //! plugin used to create the session
    const struct tmedia_session_plugin_def_s *plugin;
} tmedia_session_t;

/** Virtual table used to define a session plugin */
typedef struct tmedia_session_plugin_def_s
{
    //! object definition used to create an instance of the session
    const tsk_object_def_t *objdef;

    //! the type of the session
    tmedia_type_t type;
    //! the media name. e.g. "audio", "video", "message", "image" etc.
    const char *media;

    int (*set)(tmedia_session_t *, const tmedia_param_t *);
    int (*get)(tmedia_session_t *, tmedia_param_t *);
    int (*prepare)(tmedia_session_t *);
    int (*start)(tmedia_session_t *);
    int (*pause)(tmedia_session_t *);
    int (*stop)(tmedia_session_t *);
    int (*clear)(tmedia_session_t *, int);

    struct
    { /* Special case */
        int (*send_dtmf)(tmedia_session_t *, uint8_t);
    } audio;

    const tsdp_header_M_t *(*get_local_offer)(tmedia_session_t *);
    /* return zero if can handle the ro and non-zero otherwise */
    int (*set_remote_offer)(tmedia_session_t *, const tsdp_header_M_t *);

    struct
    { /* Special case */
        int (*set_ondata_cbfn)(tmedia_session_t *, const void *usrdata, tmedia_session_t140_ondata_cb_f func);
        int (*send_data)(tmedia_session_t *, enum tmedia_t140_data_type_e data_type, const void *data_ptr, unsigned data_size);
    } t140;
} tmedia_session_plugin_def_t;

TINYMEDIA_API uint64_t tmedia_session_get_unique_id ();
TINYMEDIA_API int tmedia_session_init (tmedia_session_t *self, tmedia_type_t type);
TINYMEDIA_API int tmedia_session_set (tmedia_session_t *self, ...);
TINYMEDIA_API tsk_bool_t tmedia_session_set_2 (tmedia_session_t *self, const tmedia_param_t *param);
TINYMEDIA_API int tmedia_session_get (tmedia_session_t *self, tmedia_param_t *param);
TINYMEDIA_API int tmedia_session_cmp (const tsk_object_t *sess1, const tsk_object_t *sess2);
TINYMEDIA_API int tmedia_session_plugin_register (const tmedia_session_plugin_def_t *plugin);
TINYMEDIA_API const tmedia_session_plugin_def_t *tmedia_session_plugin_find_by_media (const char *media);
TINYMEDIA_API int tmedia_session_plugin_unregister (const tmedia_session_plugin_def_t *plugin);
TINYMEDIA_API tmedia_session_t *tmedia_session_create (tmedia_type_t type);
TINYMEDIA_API tmedia_codecs_L_t *tmedia_session_match_codec (tmedia_session_t *self, const tsdp_header_M_t *M);
TINYMEDIA_API int
tmedia_session_set_onerror_cbfn (tmedia_session_t *self, const void *usrdata, tmedia_session_onerror_cb_f fun);

TINYMEDIA_API int tmedia_session_set_rfc5168_cbfn(tmedia_session_t* self, const void* usrdata, tmedia_session_rfc5168_cb_f fun);

TINYMEDIA_API int
tmedia_session_set_bfcp_cbfn (tmedia_session_t *self, const void *usrdata, tmedia_session_bfcp_cb_f fun);
TINYMEDIA_API int tmedia_session_deinit (tmedia_session_t *self);
typedef tsk_list_t tmedia_sessions_L_t; /**< List of @ref tmedia_session_t objects */
#define TMEDIA_DECLARE_SESSION tmedia_session_t __session__

/** Audio Session */
typedef struct tmedia_session_audio_s
{
    TMEDIA_DECLARE_SESSION;
} tmedia_session_audio_t;
#define tmedia_session_audio_init(self) tmedia_session_init (TMEDIA_SESSION (self), tmedia_audio)
TINYMEDIA_API int tmedia_session_audio_send_dtmf (tmedia_session_audio_t *self, uint8_t event);
#define tmedia_session_audio_deinit(self) tmedia_session_deinit (TMEDIA_SESSION (self))
#define tmedia_session_audio_create() tmedia_session_create (tmedia_audio)
#define TMEDIA_DECLARE_SESSION_AUDIO tmedia_session_audio_t __session_audio__

/** Video Session */
typedef struct tmedia_session_video_s
{
    TMEDIA_DECLARE_SESSION;
} tmedia_session_video_t;
#define tmedia_session_video_init(self) tmedia_session_init (TMEDIA_SESSION (self), tmedia_video)
#define tmedia_session_video_deinit(self) tmedia_session_deinit (TMEDIA_SESSION (self))
#define tmedia_session_video_create() tmedia_session_create (tmedia_video)
#define TMEDIA_DECLARE_SESSION_VIDEO tmedia_session_video_t __session_video__
/** BFCP Session */
struct tmedia_session_bfcp_s;
struct tbfcp_event_s;
typedef struct tmedia_session_bfcp_s
{
    TMEDIA_DECLARE_SESSION;

    struct
    {
        // use "struct tbfcp_event_s" instead of "tbfcp_event_t" to avoid linking aginst tinyBFCP
        int (*fun)(const struct tbfcp_event_s *event);
        const void *data;
    } callback;

    // int (* share_file) (struct tmedia_session_bfcp_s*, const char* path, va_list *app);
    // int (* share_screen) (struct tmedia_session_bfcp_s*, const tmedia_params_L_t *params);
} tmedia_session_bfcp_t;
#define tmedia_session_bfcp_init(self) tmedia_session_init (TMEDIA_SESSION (self), tmedia_bfcp)
#define tmedia_session_bfcp_deinit(self) tmedia_session_deinit (TMEDIA_SESSION (self))
#define tmedia_session_bfcp_create() tmedia_session_create (tmedia_bfcp)
#define TMEDIA_DECLARE_SESSION_BFCP tmedia_session_bfcp_t __session_bfcp__

/** T.140 session */
int tmedia_session_t140_set_ondata_cbfn (tmedia_session_t *self, const void *context, tmedia_session_t140_ondata_cb_f func);
int tmedia_session_t140_send_data (tmedia_session_t *self,
                                   enum tmedia_t140_data_type_e data_type,
                                   const void *data_ptr,
                                   unsigned data_size);

typedef void (*SessionStartFailCallback_t)( tmedia_type_t type);

/** Session manager */
typedef struct tmedia_session_mgr_s
{
    TSK_DECLARE_OBJECT;

    //! whether we are the offerer or not
    tsk_bool_t offerer;
    //! local IP address or FQDN
    char *addr;
    //! public IP address or FQDN
    char *public_addr;
    //! whether the @a addr is IPv6 or not (useful when @addr is a FQDN)
    tsk_bool_t ipv6;

    struct
    {
        uint32_t lo_ver;
        tsdp_message_t *lo;

        int32_t ro_ver;
        tsdp_message_t *ro;
    } sdp;

    tsk_bool_t started;
    tsk_bool_t ro_provisional;
    //! session type
    tmedia_type_t type;
    //! bandwidth level
    tmedia_bandwidth_level_t bl;

    /* session error callback */
    struct
    {
        tmedia_session_onerror_cb_f fun;
        const void *usrdata;
    } onerror_cb;

	/* rfc5168 callback */
	struct {
		tmedia_session_rfc5168_cb_f fun;
		const void* usrdata;
	} rfc5168_cb;
    //! List of all sessions
    tmedia_sessions_L_t *sessions;

    //! User's parameters used to confugure plugins
    tmedia_params_L_t *params;
    
    SessionStartFailCallback_t session_start_fail_callback;
    
    TSK_DECLARE_SAFEOBJ;
} tmedia_session_mgr_t;

typedef enum tmedia_session_param_type_e
{
    tmedia_sptype_null = 0,

    tmedia_sptype_set,
    tmedia_sptype_get
} tmedia_session_param_type_t;

#define TMEDIA_SESSION_SET_PARAM(MEDIA_TYPE_ENUM, PLUGIN_TYPE_ENUM, VALUE_TYPE_ENUM, KEY_STR, VALUE) \
    tmedia_sptype_set, (tmedia_type_t)MEDIA_TYPE_ENUM, (tmedia_param_plugin_type_t)PLUGIN_TYPE_ENUM, \
    (tmedia_param_value_type_t)VALUE_TYPE_ENUM, (const char *)KEY_STR,                               \
    TMEDIA_PARAM_VALUE_TYPE_IS_PTR (VALUE_TYPE_ENUM) ? (void *)VALUE : (void *) & VALUE
#define TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, PLUGIN_TYPE_ENUM, VALUE_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    tmedia_sptype_get, (tmedia_type_t)MEDIA_TYPE_ENUM, (tmedia_param_plugin_type_t)PLUGIN_TYPE_ENUM,     \
    (tmedia_param_value_type_t)VALUE_TYPE_ENUM, (const char *)KEY_STR, (void *)VALUE_PTR
/* Manager */
#define TMEDIA_SESSION_MANAGER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_PARAM (tmedia_none, tmedia_ppt_manager, tmedia_pvt_int32, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_MANAGER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (tmedia_none, tmedia_ppt_manager, tmedia_pvt_pobject, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_MANAGER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_PARAM (tmedia_none, tmedia_ppt_manager, tmedia_pvt_pchar, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_MANAGER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_PARAM (tmedia_none, tmedia_ppt_manager, tmedia_pvt_int64, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_MANAGER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_PARAM (tmedia_none, tmedia_ppt_manager, tmedia_pvt_int32, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_MANAGER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PARAM(tmedia_none, tmedia_ppt_manager, tmedia_pvt_pobject, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_MANAGER_GET_STR(KEY_STR, VALUE_PSTR)	TMEDIA_SESSION_GET_PARAM(tmedia_none,
//tmedia_ppt_manager, tmedia_pvt_pchar, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_MANAGER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_PARAM(tmedia_none, tmedia_ppt_manager, tmedia_pvt_int64, KEY_STR, VALUE_PINT64)
/* ANY Session */
#define TMEDIA_SESSION_SET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_int32, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_SET_POBJECT(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pobject, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_SET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pchar, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_SET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_int64, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_SET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pvoid, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_GET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_int32, KEY_STR, VALUE_PINT32)
#define TMEDIA_SESSION_GET_POBJECT(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PPOBJECT) \
    TMEDIA_SESSION_GET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pobject, KEY_STR, VALUE_PPOBJECT)
//#define TMEDIA_SESSION_GET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pobject, KEY_STR,
//VALUE_PPTR)
//#define TMEDIA_SESSION_GET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_pchar, KEY_STR,
//VALUE_PSTR)
#define TMEDIA_SESSION_GET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT64) \
    TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_session, tmedia_pvt_int64, KEY_STR, VALUE_PINT64)
/* AUDIO Session */
#define TMEDIA_SESSION_AUDIO_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_INT32 (tmedia_audio, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_AUDIO_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_POBJECT (tmedia_audio, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_AUDIO_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_STR (tmedia_audio, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_AUDIO_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_INT64 (tmedia_audio, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_AUDIO_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_INT32 (tmedia_audio, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_AUDIO_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PVOID(tmedia_audio, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_AUDIO_GET_STR(KEY_STR, VALUE_PSTR)	TMEDIA_SESSION_GET_STR(tmedia_audio,
//KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_AUDIO_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_INT64(tmedia_audio, KEY_STR, VALUE_PINT64)
/* VIDEO Session */
#define TMEDIA_SESSION_VIDEO_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_INT32 (tmedia_video, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_VIDEO_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_POBJECT (tmedia_video, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_VIDEO_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_STR (tmedia_video, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_VIDEO_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_INT64 (tmedia_video, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_VIDEO_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_INT32 (tmedia_video, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_VIDEO_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PVOID(tmedia_video, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_VIDEO_GET_STR(KEY_STR, VALUE_PSTR)	TMEDIA_SESSION_GET_STR(tmedia_video,
//KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_VIDEO_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_INT64(tmedia_video, KEY_STR, VALUE_PINT64)
/* MSRP Session */
#define TMEDIA_SESSION_MSRP_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_INT32 (tmedia_msrp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_MSRP_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_POBJECT (tmedia_msrp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_MSRP_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_STR (tmedia_msrp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_MSRP_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_INT64 (tmedia_msrp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_MSRP_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_INT32 (tmedia_msrp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_MSRP_GET_PVOID(KEY_STR, VALUE_PPTR)	TMEDIA_SESSION_GET_PVOID(tmedia_msrp,
//KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_MSRP_GET_STR(KEY_STR, VALUE_PSTR)	TMEDIA_SESSION_GET_STR(tmedia_msrp,
//KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_MSRP_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_INT64(tmedia_msrp, KEY_STR, VALUE_PINT64)
/* BFCP Session */
#define TMEDIA_SESSION_BFCP_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_INT32 (tmedia_bfcp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_BFCP_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_POBJECT (tmedia_bfcp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_BFCP_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_STR (tmedia_bfcp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_BFCP_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_INT64 (tmedia_bfcp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_BFCP_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_INT32 (tmedia_bfcp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_BFCP_GET_PVOID(KEY_STR, VALUE_PPTR)	TMEDIA_SESSION_GET_PVOID(tmedia_bfcp,
//KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_BFCP_GET_STR(KEY_STR, VALUE_PSTR)	TMEDIA_SESSION_GET_STR(tmedia_bfcp,
//KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_BFCP_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_INT64(tmedia_bfcp, KEY_STR, VALUE_PINT64)
/* ANY Consumer */
#define TMEDIA_SESSION_CONSUMER_SET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_int32, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_CONSUMER_SET_POBJECT(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_pobject, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_CONSUMER_SET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_pchar, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_CONSUMER_SET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_int64, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_CONSUMER_SET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_pvoid, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_CONSUMER_GET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_int32, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_CONSUMER_GET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_pobject, KEY_STR,
//VALUE_PPTR)
//#define TMEDIA_SESSION_CONSUMER_GET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_pchar, KEY_STR,
//VALUE_PSTR)
//#define TMEDIA_SESSION_CONSUMER_GET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_consumer, tmedia_pvt_int64, KEY_STR,
//VALUE_PINT64)
/* AUDIO Consumer */
#define TMEDIA_SESSION_AUDIO_CONSUMER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_CONSUMER_SET_INT32 (tmedia_audio, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_AUDIO_CONSUMER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_CONSUMER_SET_POBJECT (tmedia_audio, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_AUDIO_CONSUMER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_CONSUMER_SET_STR (tmedia_audio, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_AUDIO_CONSUMER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_CONSUMER_SET_INT64 (tmedia_audio, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_AUDIO_CONSUMER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_CONSUMER_GET_INT32 (tmedia_audio, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_AUDIO_CONSUMER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_CONSUMER_GET_PVOID(tmedia_audio, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_AUDIO_CONSUMER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_CONSUMER_GET_STR(tmedia_audio, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_AUDIO_CONSUMER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_CONSUMER_GET_INT64(tmedia_audio, KEY_STR, VALUE_PINT64)
/* VIDEO Consumer */
#define TMEDIA_SESSION_VIDEO_CONSUMER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_CONSUMER_SET_INT32 (tmedia_video, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_VIDEO_CONSUMER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_CONSUMER_SET_POBJECT (tmedia_video, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_VIDEO_CONSUMER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_CONSUMER_SET_STR (tmedia_video, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_VIDEO_CONSUMER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_CONSUMER_SET_INT64 (tmedia_video, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_VIDEO_CONSUMER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_CONSUMER_GET_INT32 (tmedia_video, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_VIDEO_CONSUMER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_CONSUMER_GET_PVOID(tmedia_video, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_VIDEO_CONSUMER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_CONSUMER_GET_STR(tmedia_video, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_VIDEO_CONSUMER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_CONSUMER_GET_INT64(tmedia_video, KEY_STR, VALUE_PINT64)
/* MSRP Consumer */
#define TMEDIA_SESSION_MSRP_CONSUMER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_CONSUMER_SET_INT32 (tmedia_msrp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_MSRP_CONSUMER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_CONSUMER_SET_POBJECT (tmedia_msrp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_MSRP_CONSUMER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_CONSUMER_SET_STR (tmedia_msrp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_MSRP_CONSUMER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_CONSUMER_SET_INT64 (tmedia_msrp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_MSRP_CONSUMER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_CONSUMER_GET_INT32 (tmedia_msrp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_MSRP_CONSUMER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_CONSUMER_GET_PVOID(tmedia_msrp, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_MSRP_CONSUMER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_CONSUMER_GET_STR(tmedia_msrp, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_MSRP_CONSUMER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_CONSUMER_GET_INT64(tmedia_msrp, KEY_STR, VALUE_PINT64)
/* BFCP Consumer */
#define TMEDIA_SESSION_BFCP_CONSUMER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_CONSUMER_SET_INT32 (tmedia_bfcp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_BFCP_CONSUMER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_CONSUMER_SET_POBJECT (tmedia_bfcp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_BFCP_CONSUMER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_CONSUMER_SET_STR (tmedia_bfcp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_BFCP_CONSUMER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_CONSUMER_SET_INT64 (tmedia_bfcp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_BFCP_CONSUMER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_CONSUMER_GET_INT32 (tmedia_bfcp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_BFCP_CONSUMER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_CONSUMER_GET_PVOID(tmedia_bfcp, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_BFCP_CONSUMER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_CONSUMER_GET_STR(tmedia_bfcp, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_BFCP_CONSUMER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_CONSUMER_GET_INT64(tmedia_bfcp, KEY_STR, VALUE_PINT64)

/* ANY Producer */
#define TMEDIA_SESSION_PRODUCER_SET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_int32, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_PRODUCER_SET_POBJECT(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pobject, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_PRODUCER_SET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pchar, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_PRODUCER_SET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_int64, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_PRODUCER_SET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_SET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pvoid, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_PRODUCER_GET_INT32(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_GET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_int32, KEY_STR, VALUE_PINT32)
#define TMEDIA_SESSION_PRODUCER_GET_INT64(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PINT64) \
    TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_int64, KEY_STR, VALUE_PINT64)
#define TMEDIA_SESSION_PRODUCER_GET_POBJECT(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_GET_PARAM (MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pobject, KEY_STR, VALUE_PTR)
//#define TMEDIA_SESSION_PRODUCER_GET_PVOID(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pobject, KEY_STR,
//VALUE_PPTR)
//#define TMEDIA_SESSION_PRODUCER_GET_STR(MEDIA_TYPE_ENUM, KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_GET_PARAM(MEDIA_TYPE_ENUM, tmedia_ppt_producer, tmedia_pvt_pchar, KEY_STR,
//VALUE_PSTR)
/* AUDIO Producer */
#define TMEDIA_SESSION_AUDIO_PRODUCER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_PRODUCER_SET_INT32 (tmedia_audio, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_AUDIO_PRODUCER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_PRODUCER_SET_POBJECT (tmedia_audio, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_AUDIO_PRODUCER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_AUDIO_PRODUCER_SET_STR (tmedia_audio, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_AUDIO_PRODUCER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_PRODUCER_SET_INT64 (tmedia_audio, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_AUDIO_PRODUCER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_PRODUCER_GET_INT32 (tmedia_audio, KEY_STR, VALUE_PINT32)
#define TMEDIA_SESSION_AUDIO_PRODUCER_GET_INT64(KEY_STR, VALUE_PINT64) \
    TMEDIA_SESSION_PRODUCER_GET_INT64(tmedia_audio, KEY_STR, VALUE_PINT64)
//#define TMEDIA_SESSION_AUDIO_PRODUCER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_PRODUCER_GET_PVOID(tmedia_audio, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_AUDIO_PRODUCER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_AUDIO_PRODUCER_GET_STR(tmedia_audio, KEY_STR, VALUE_PSTR)
/* Video Producer */
#define TMEDIA_SESSION_VIDEO_PRODUCER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_PRODUCER_SET_INT32 (tmedia_video, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_VIDEO_PRODUCER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_PRODUCER_SET_POBJECT (tmedia_video, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_VIDEO_PRODUCER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_AUDIO_PRODUCER_SET_STR (tmedia_video, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_VIDEO_PRODUCER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_PRODUCER_SET_INT64 (tmedia_video, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_VIDEO_PRODUCER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_PRODUCER_GET_INT32 (tmedia_video, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_VIDEO_PRODUCER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_PRODUCER_GET_PVOID(tmedia_video, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_VIDEO_PRODUCER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_AUDIO_PRODUCER_GET_STR(tmedia_video, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_VIDEO_PRODUCER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_PRODUCER_GET_INT64(tmedia_video, KEY_STR, VALUE_PINT64)
/* MSRP Producer */
#define TMEDIA_SESSION_MSRP_PRODUCER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_PRODUCER_SET_INT32 (tmedia_msrp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_MSRP_PRODUCER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_PRODUCER_SET_POBJECT (tmedia_msrp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_MSRP_PRODUCER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_AUDIO_PRODUCER_SET_STR (tmedia_msrp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_MSRP_PRODUCER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_PRODUCER_SET_INT64 (tmedia_msrp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_MSRP_PRODUCER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_PRODUCER_GET_INT32 (tmedia_msrp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_PRODUCER_GET_PVOID(tmedia_msrp, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_AUDIO_PRODUCER_GET_STR(tmedia_msrp, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_PRODUCER_GET_INT64(tmedia_msrp, KEY_STR, VALUE_PINT64)
/* BFCP Producer */
#define TMEDIA_SESSION_BFCP_PRODUCER_SET_INT32(KEY_STR, VALUE_INT32) \
    TMEDIA_SESSION_PRODUCER_SET_INT32 (tmedia_bfcp, KEY_STR, VALUE_INT32)
#define TMEDIA_SESSION_BFCP_PRODUCER_SET_POBJECT(KEY_STR, VALUE_PTR) \
    TMEDIA_SESSION_PRODUCER_SET_POBJECT (tmedia_bfcp, KEY_STR, VALUE_PTR)
#define TMEDIA_SESSION_BFCP_PRODUCER_SET_STR(KEY_STR, VALUE_STR) \
    TMEDIA_SESSION_AUDIO_PRODUCER_SET_STR (tmedia_bfcp, KEY_STR, VALUE_STR)
#define TMEDIA_SESSION_BFCP_PRODUCER_SET_INT64(KEY_STR, VALUE_INT64) \
    TMEDIA_SESSION_PRODUCER_SET_INT64 (tmedia_bfcp, KEY_STR, VALUE_INT64)
#define TMEDIA_SESSION_BFCP_PRODUCER_GET_INT32(KEY_STR, VALUE_PINT32) \
    TMEDIA_SESSION_PRODUCER_GET_INT32 (tmedia_bfcp, KEY_STR, VALUE_PINT32)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_PVOID(KEY_STR, VALUE_PPTR)
//TMEDIA_SESSION_PRODUCER_GET_PVOID(tmedia_bfcp, KEY_STR, VALUE_PPTR)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_STR(KEY_STR, VALUE_PSTR)
//TMEDIA_SESSION_AUDIO_PRODUCER_GET_STR(tmedia_bfcp, KEY_STR, VALUE_PSTR)
//#define TMEDIA_SESSION_MSRP_PRODUCER_GET_INT64(KEY_STR, VALUE_PINT64)
//TMEDIA_SESSION_PRODUCER_GET_INT64(tmedia_bfcp, KEY_STR, VALUE_PINT64)


#define TMEDIA_SESSION_SET_NULL() tmedia_sptype_null
#define TMEDIA_SESSION_GET_NULL() tmedia_sptype_null

//#define TMEDIA_SESSION_SET_REMOTE_IP(IP_STR)				tmedia_sptype_remote_ip, (const char*)
//IP_STR
//#define TMEDIA_SESSION_SET_LOCAL_IP(IP_STR, IPv6_BOOL)		tmedia_sptype_local_ip, (const char*)
//IP_STR, (tsk_bool_t)IPv6_BOOL
//#define TMEDIA_SESSION_SET_RTCP(ENABLED_BOOL)				tmedia_sptype_set_rtcp,
//(tsk_bool_t)ENABLED_BOOL
//#define TMEDIA_SESSION_SET_QOS(TYPE_ENUM, STRENGTH_ENUM)	tmedia_sptype_qos,
//(tmedia_qos_stype_t)TYPE_ENUM, (tmedia_qos_strength_t)STRENGTH_ENUM


TINYMEDIA_API tmedia_session_mgr_t *
tmedia_session_mgr_create (tmedia_type_t type, const char *addr, tsk_bool_t ipv6, tsk_bool_t offerer);

TINYMEDIA_API void tmedia_session_mgr_cleanup(tmedia_session_mgr_t *self);

TINYMEDIA_API int tmedia_session_mgr_set_codecs_supported (tmedia_session_mgr_t *self,
                                                           tmedia_codec_id_t codecs_supported);
TINYMEDIA_API tmedia_session_t *tmedia_session_mgr_find (tmedia_session_mgr_t *self, tmedia_type_t type);

TINYMEDIA_API int tmedia_session_mgr_start (tmedia_session_mgr_t *self );
TINYMEDIA_API int tmedia_session_mgr_restart( tmedia_session_mgr_t *self , tmedia_type_t type );
TINYMEDIA_API int tmedia_session_mgr_set (tmedia_session_mgr_t *self, ...);
TINYMEDIA_API int tmedia_session_mgr_set_2 (tmedia_session_mgr_t *self, va_list *app);
TINYMEDIA_API int tmedia_session_mgr_set_3 (tmedia_session_mgr_t *self, const tmedia_params_L_t *params);
TINYMEDIA_API int tmedia_session_mgr_get (tmedia_session_mgr_t *self, ...);
TINYMEDIA_API int tmedia_session_mgr_stop (tmedia_session_mgr_t *self);
TINYMEDIA_API int tmedia_session_mgr_clear (tmedia_session_mgr_t *self, int iServerSessionID, tmedia_type_t type);

TINYMEDIA_API const tsdp_message_t *tmedia_session_mgr_get_lo (tmedia_session_mgr_t *self,int iSessionID);
TINYMEDIA_API int
tmedia_session_mgr_set_ro (tmedia_session_mgr_t *self, const tsdp_message_t *sdp, tmedia_ro_type_t ro_type,int iSessionID);
TINYMEDIA_API const tsdp_message_t *tmedia_session_mgr_get_ro (tmedia_session_mgr_t *self);

TINYMEDIA_API tsk_bool_t
tmedia_session_mgr_is_held (tmedia_session_mgr_t *self, tmedia_type_t type, tsk_bool_t local);
TINYMEDIA_API tsk_bool_t tmedia_session_mgr_canresume (tmedia_session_mgr_t *self);
TINYMEDIA_API tsk_bool_t tmedia_session_mgr_has_active_session (tmedia_session_mgr_t *self);
TINYMEDIA_API int tmedia_session_mgr_send_dtmf (tmedia_session_mgr_t *self, uint8_t event);
TINYMEDIA_API int tmedia_session_mgr_set_t140_ondata_cbfn (tmedia_session_mgr_t *self,
                                                           const void *context,
                                                           tmedia_session_t140_ondata_cb_f fun);
TINYMEDIA_API int tmedia_session_mgr_send_t140_data (tmedia_session_mgr_t *self,
                                                     enum tmedia_t140_data_type_e data_type,
                                                     const void *data_ptr,
                                                     unsigned data_size);

TINYMEDIA_API void tmedia_session_set_session_start_fail_callback( tmedia_session_mgr_t * self,
                                                                  SessionStartFailCallback_t  callback );


TINYMEDIA_API int tmedia_session_mgr_set_onerror_cbfn (tmedia_session_mgr_t *self,
                                                       const void *usrdata,
                                                       tmedia_session_onerror_cb_f fun);

TINYMEDIA_API int tmedia_session_mgr_set_bfcp_cbfn (tmedia_session_mgr_t *self,
                                                    const void *usrdata,
                                                    tmedia_session_bfcp_cb_f fun);

TINYMEDIA_GEXTERN const tsk_object_def_t *tmedia_session_mgr_def_t;

TMEDIA_END_DECLS

#endif /* TINYMEDIA_SESSION_H */

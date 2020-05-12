

#include "tinydav/tdav_session_av.h"


#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinyrtp/trtp_manager.h"


#include "tinymedia/tmedia_consumer.h"
#include "tinymedia/tmedia_defaults.h"
#include "tinymedia/tmedia_producer.h"

#ifdef MAC_OS
#include "tinydav/tdav_apple.h"
#endif


#include <ctype.h>  /* isspace */
#include <limits.h> /* INT_MAX */
#include <math.h>   /* log10 */

// rtp manager对象，同一进程使用同个对象
static struct trtp_manager_s *g_rtp_manager = NULL;

#if !defined(TDAV_DFAULT_FP_HASH)
#define TDAV_DFAULT_FP_HASH tnet_dtls_hash_type_sha256
#endif /* TDAV_DFAULT_FP_HASH */
#if !defined(TDAV_FIXME_MEDIA_LEVEL_DTLS_ATT)
#define TDAV_FIXME_MEDIA_LEVEL_DTLS_ATT 0
#endif /* TDAV_FIXME_MEDIA_LEVEL_DTLS_ATT */

// rfc5763 - The endpoint MUST NOT use the connection attribute defined in [RFC4145].
#if !defined(TDAV_DTLS_CONNECTION_ATT)
#define TDAV_DTLS_CONNECTION_ATT 0
#endif


#define SDP_CAPS_COUNT_MAX 0x1F
#define SDP_DECLARE_TAG int32_t tag // [1 - *]
#define SDP_TAG(self) ((self) ? *((int32_t *)(self)) : 0)

typedef enum RTP_PROFILE_E {
    RTP_PROFILE_NONE = 0x00,

    RTP_PROFILE_AVP = (1 << 0),
    RTP_PROFILE_AVPF = (1 << 1),

    RTP_PROFILE_SECURE = (1 << 2),
    RTP_PROFILE_SECURE_SDES = (RTP_PROFILE_SECURE | (1 << 3)),
    RTP_PROFILE_SECURE_DTLS = (RTP_PROFILE_SECURE | (1 << 4)),

    RTP_PROFILE_SAVP = (RTP_PROFILE_AVP | RTP_PROFILE_SECURE_SDES),
    RTP_PROFILE_SAVPF = (RTP_PROFILE_AVPF | RTP_PROFILE_SECURE_SDES),

    RTP_PROFILE_UDP_TLS_RTP_SAVP = (RTP_PROFILE_AVP | RTP_PROFILE_SECURE_DTLS),
    RTP_PROFILE_UDP_TLS_RTP_SAVPF = (RTP_PROFILE_AVPF | RTP_PROFILE_SECURE_DTLS)
} RTP_PROFILE_T;

typedef struct RTP_PROFILE_XS
{
    enum RTP_PROFILE_E type;
    const char *name;
} RTP_PROFILE_XT;

static const RTP_PROFILE_XT RTP_PROFILES[] = {
    { RTP_PROFILE_AVP, "RTP/AVP" },
    { RTP_PROFILE_AVPF, "RTP/AVPF" },
    { RTP_PROFILE_SAVP, "RTP/SAVP" },
    { RTP_PROFILE_SAVPF, "RTP/SAVPF" },
    { RTP_PROFILE_UDP_TLS_RTP_SAVP, "UDP/TLS/RTP/SAVP" },
    { RTP_PROFILE_UDP_TLS_RTP_SAVPF, "UDP/TLS/RTP/SAVPF" },
};
#define RTP_PROFILES_COUNT (sizeof (RTP_PROFILES) / sizeof (RTP_PROFILES[0]))

typedef struct sdp_acap_xs
{
    SDP_DECLARE_TAG;
    unsigned optional : 1; // "e.g. [2]"
    unsigned or : 1;       // "e.g.|2"
    const char *value;
} sdp_acap_xt;
typedef sdp_acap_xt sdp_acaps_xt[SDP_CAPS_COUNT_MAX];

typedef struct sdp_tcap_xs
{
    SDP_DECLARE_TAG;
    RTP_PROFILE_T profile;
} sdp_tcap_xt;
typedef sdp_tcap_xt sdp_tcaps_xt[SDP_CAPS_COUNT_MAX];

typedef struct sdp_pcfg_xs
{
    SDP_DECLARE_TAG;
    sdp_tcap_xt tcap;
    sdp_acaps_xt acaps;
} sdp_pcfg_xt;
typedef sdp_pcfg_xt sdp_acfg_xt;
typedef sdp_pcfg_xt sdp_pcfgs_xt[SDP_CAPS_COUNT_MAX];
typedef tsk_object_t sdp_headerM_Or_Message; /* tsdp_header_M_t or tsdp_message_t */

#define _sdp_reset(self) \
    if ((self))          \
        memset ((self), 0, sizeof (*(self)));
#define _sdp_pcfgs_reset(self) _sdp_reset ((self))
#define _sdp_acfgs_reset(self) _sdp_reset ((self))
#define _sdp_pcfg_reset(self) _sdp_reset ((self))
#define _sdp_acfg_reset(self) _sdp_reset ((self))
#define _sdp_tcaps_reset(self) _sdp_reset ((self))
#define _sdp_acaps_reset(self) _sdp_reset ((self))
#define _sdp_integer_length(self) ((self) ? ((int32_t)log10 (abs (self)) + 1) : 1)
#define _sdp_str_index_of(str, sub_str) tsk_strindexOf ((str), tsk_strlen ((str)), sub_str)
#define _sdp_str_starts_with(str, sub_str) (_sdp_str_index_of ((str), (sub_str)) == 0)
#define _sdp_str_contains(str, sub_str) (_sdp_str_index_of ((str), (sub_str)) != -1)
#define _SDP_DECLARE_INDEX_OF(name)                                                                     \
    static int32_t _sdp_##name##s_indexof (sdp_##name##_xt (*name##s)[SDP_CAPS_COUNT_MAX], int32_t tag) \
    {                                                                                                   \
        if (name##s)                                                                                    \
        {                                                                                               \
            int32_t i;                                                                                  \
            for (i = 0; i < SDP_CAPS_COUNT_MAX; ++i)                                                    \
            {                                                                                           \
                if ((*name##s)[i].tag == tag)                                                           \
                {                                                                                       \
                    return i;                                                                           \
                }                                                                                       \
            }                                                                                           \
        }                                                                                               \
        return -1;                                                                                      \
    }

typedef struct tdav_sdp_caps_s
{
    TSK_DECLARE_OBJECT;

    sdp_pcfgs_xt local;
    sdp_pcfgs_xt remote;
    sdp_acfg_xt acfg;
} tdav_sdp_caps_t;

static tdav_sdp_caps_t *tdav_sdp_caps_create ();

static const tsdp_header_A_t *_sdp_findA_at (const sdp_headerM_Or_Message *sdp, const char *field, tsk_size_t index);
static int _sdp_add_headerA (sdp_headerM_Or_Message *sdp, const char *field, const char *value);
static RTP_PROFILE_T _sdp_profile_from_string (const char *profile);
static int32_t _sdp_acaps_indexof (sdp_acap_xt (*acaps)[SDP_CAPS_COUNT_MAX], int32_t tag);

static int _sdp_acaps_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_acap_xt (*acaps)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset);
static int32_t _sdp_tcaps_indexof (sdp_tcap_xt (*tcaps)[SDP_CAPS_COUNT_MAX], int32_t tag);
static int _sdp_tcaps_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_tcap_xt (*tcaps)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset);
static int _sdp_acfg_to_sdp (sdp_headerM_Or_Message *sdp, const sdp_acfg_xt *acfg);
static int _sdp_pcfgs_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_acap_xt (*acaps)[SDP_CAPS_COUNT_MAX], sdp_tcap_xt (*tcaps)[SDP_CAPS_COUNT_MAX], sdp_pcfg_xt (*pcfgs)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset);

int tdav_session_av_init (tdav_session_av_t *self, tmedia_type_t media_type)
{
    uint64_t session_id;
    tmedia_session_t *base = TMEDIA_SESSION (self);

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!base->initialized)
    {
        int ret = tmedia_session_init (base, media_type);
        if (ret != 0)
        {
            return ret;
        }
    }

    /* base::init(): called by tmedia_session_create() */

    self->media_type = media_type;
    self->media_profile = tmedia_defaults_get_profile ();
   
    self->avpf_mode_set = self->avpf_mode_neg = tmedia_defaults_get_avpf_mode ();
    self->fps = -1;                                           // use what is negotiated by
    self->bandwidth_max_upload_kbps =INT_MAX;
    self->bandwidth_max_download_kbps =INT_MAX;
  
    self->congestion_ctrl_enabled = tmedia_defaults_get_congestion_ctrl_enabled ();                                                                         // whether to enable
                                                                                                                                                            // draft-alvestrand-rtcweb-congestion-03 and
                                                                                                                                                            // draft-alvestrand-rmcat-remb-01


    tsk_safeobj_init (self);

    // session id
    if (!(session_id = TMEDIA_SESSION (self)->id))
    { // set the session id if not already done
        TMEDIA_SESSION (self)->id = session_id = tmedia_session_get_unique_id ();
    }
    // consumer
    TSK_OBJECT_SAFE_FREE (self->consumer);

    TSK_DEBUG_INFO ("media_type = %d", self->media_type);
    if (!(self->consumer = tmedia_consumer_create ((self->media_type & tmedia_video || (self->media_type & tmedia_bfcp_video) == tmedia_bfcp_video) ? tmedia_video : tmedia_audio, session_id)))
    { // get an audio (or video) consumer and ignore "bfcp" part
        TSK_DEBUG_ERROR ("Failed to create consumer for media type = %d", self->media_type);
    }
    // producer
    TSK_OBJECT_SAFE_FREE (self->producer);
    if (!(self->producer = tmedia_producer_create (self->media_type, session_id)))
    {
        TSK_DEBUG_ERROR ("Failed to create producer for media type = %d", self->media_type);
    }
    


    // sdp caps
    TSK_OBJECT_SAFE_FREE (self->sdp_caps);
    if (!(self->sdp_caps = tdav_sdp_caps_create ()))
    {
        TSK_DEBUG_ERROR ("Failed to create SDP caps");
        return -1;
    }

    // pt mapping (when bypassing is enabled)
    self->pt_map.local = self->pt_map.remote = self->pt_map.neg = -1;

    return 0;
}

tsk_bool_t tdav_session_av_set (tdav_session_av_t *self, const tmedia_param_t *param)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }

    // try with base class first
    if (tmedia_session_set_2 (TMEDIA_SESSION (self), param))
    {
        return tsk_true;
    }

    if (param->plugin_type == tmedia_ppt_consumer && self->consumer)
    {
        return (tmedia_consumer_set_param (self->consumer, param) == 0);
    }
    else if (param->plugin_type == tmedia_ppt_producer && self->producer)
    {
        return (tmedia_producer_set (self->producer, param) == 0);
    }
    else if (param->plugin_type == tmedia_ppt_session)
    {
        if (param->value_type == tmedia_pvt_pchar)
        {
            if (tsk_striequals (param->key, "remote-ip"))
            {
                if (param->value)
                {
                    tsk_strupdate (&self->remote_ip, (const char *)param->value);
                    return tsk_true;
                }
            }
            else if (tsk_striequals (param->key, "local-ip"))
            {
                tsk_strupdate (&self->local_ip, (const char *)param->value);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "local-ipver"))
            {
                self->use_ipv6 = tsk_striequals (param->value, "ipv6");
                return tsk_true;
            }
        }
        else if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, "srtp-mode"))
            {

                TSK_DEBUG_INFO ("'srtp-mode' param ignored beacuse SRTP not enabled. Please "
                                "rebuild the source code with this option.");

                return tsk_true;
            }
            else if (tsk_striequals (param->key, "rtp-ssrc"))
            {
                self->rtp_ssrc = *((uint32_t *)param->value);
                if (self->rtp_manager && self->rtp_ssrc)
                {
                    self->rtp_manager->rtp.ssrc.local = self->rtp_ssrc;
                }
                return tsk_true;
            }
           
            else if (tsk_striequals (param->key, "avpf-mode"))
            {
                self->avpf_mode_set = (tmedia_mode_t)TSK_TO_INT32 ((uint8_t *)param->value);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "webrtc2sip-mode-enabled"))
            {
                self->is_webrtc2sip_mode_enabled = (TSK_TO_INT32 ((uint8_t *)param->value) != 0);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "bandwidth-max-upload"))
            {
                self->bandwidth_max_upload_kbps = TSK_TO_INT32 ((uint8_t *)param->value);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "bandwidth-max-download"))
            {
                self->bandwidth_max_download_kbps = TSK_TO_INT32 ((uint8_t *)param->value);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "fps"))
            {
                self->fps = TSK_TO_INT32 ((uint8_t *)param->value);
                return tsk_true;
            }
           
        }
        else if (param->value_type == tmedia_pvt_pobject)
        {
            if (tsk_striequals (param->key, "remote-sdp-message"))
            {
                TSK_OBJECT_SAFE_FREE (self->remote_sdp);
                self->remote_sdp = (struct tsdp_message_s *)tsk_object_ref (param->value);
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "local-sdp-message"))
            {
                TSK_OBJECT_SAFE_FREE (self->local_sdp);
                self->local_sdp = (struct tsdp_message_s *)tsk_object_ref (param->value);
                return tsk_true;
            }
        }
    }

    return tsk_false;
}

tsk_bool_t tdav_session_av_get (tdav_session_av_t *self, tmedia_param_t *param)
{
    if (!self || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }

    if (param->plugin_type == tmedia_ppt_producer && self->producer)
    {
        return (tmedia_producer_get (self->producer, param) == 0);
    }
    else if (param->plugin_type == tmedia_ppt_session)
    {
        if (param->value_type == tmedia_pvt_int32)
        {
            if (tsk_striequals (param->key, "codecs-negotiated"))
            { // negotiated codecs
                tmedia_codecs_L_t *neg_codecs = tsk_object_ref (TMEDIA_SESSION (self)->neg_codecs);
                if (neg_codecs)
                {
                    const tsk_list_item_t *item;
                    tsk_list_foreach (item, neg_codecs)
                    {
                        ((int32_t *)param->value)[0] |= TMEDIA_CODEC (item->data)->id;
                    }
                    TSK_OBJECT_SAFE_FREE (neg_codecs);
                }
                return tsk_true;
            }
            else if (tsk_striequals (param->key, "srtp-enabled"))
            {
                ((int8_t *)param->value)[0] = 0;
                TSK_DEBUG_INFO ("Ignoring parameter 'srtp-enabled' because SRTP not supported. "
                                "Please rebuild the source code with this option enabled.");
                return tsk_true;
            }
            else if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_RTP_TIMESTAMP))
            {
                if (self->rtp_manager) {
                    ((int32_t *)param->value)[0] = (int32_t)trtp_manager_get_current_timestamp(self->rtp_manager);
                } else {
                    ((int32_t *)param->value)[0] = 0;
                }
                return tsk_true;
            }
        }
        else if (param->value_type == tmedia_pvt_pobject)
        {
            if (tsk_striequals (param->key, "producer"))
            {
                *((tsk_object_t **)param->value) = tsk_object_ref (self->producer); // up to the caller to release the object
                return tsk_true;
            }
        }
    }

    return tsk_false;
}

int tdav_session_av_init_encoder (tdav_session_av_t *self, struct tmedia_codec_s *encoder)
{
    if (!self || !encoder)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    encoder->bandwidth_max_upload = self->bandwidth_max_upload_kbps;
    encoder->bandwidth_max_download = self->bandwidth_max_download_kbps;
    return 0;
}

int tdav_session_av_prepare (tdav_session_av_t *self)
{
    int ret = 0;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    /* set local port */
    if (!self->rtp_manager)
    {
        self->rtp_manager =
        trtp_manager_create (self->local_ip, self->use_ipv6,self->__session__.sessionid);
    }
    if (self->rtp_manager)
    {
        // Port range
        if ((ret = trtp_manager_set_port_range (self->rtp_manager, tmedia_defaults_get_rtp_port_range_start (), tmedia_defaults_get_rtp_port_range_stop ())))
        {
            return ret;
        }
        if ((ret = trtp_manager_prepare (self->rtp_manager)))
        {
            return ret;
        }
        
        if (self->rtp_manager->sessionid)
        {
            self->rtp_manager->rtp.ssrc.local = self->rtp_manager->sessionid;
        }
        
        // 初始化完rtp manager后更新全局manager对象，保证同一进程使用同个manager对象
        tdav_session_set_rtp_manager(self->rtp_manager);
        
        // 初始化rtp manager mutex
        self->rtp_manager_mutex = tsk_mutex_create_2(tsk_false);
        self->producer_mutex = tsk_mutex_create();
    }

    return ret;
}

int tdav_session_av_start (tdav_session_av_t *self, const tmedia_codec_t *best_codec)
{
    if (!self || !best_codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
#ifdef MAC_OS
    if( tdav_mac_has_both_audio_device() == 0  || self->forceUseHalMode == tsk_true )
    {
        TSK_DEBUG_INFO(" Use Hal ");
        self->producer->audio.useHalMode = tsk_true;
        self->consumer->audio.useHalMode = tsk_true;
        self->useHalMode = tsk_true;
    }
    else{
        self->producer->audio.useHalMode = tsk_false;
        self->consumer->audio.useHalMode = tsk_false;
        self->useHalMode = tsk_false;
        TSK_DEBUG_INFO(" Use VOIP ");
    }
    
#endif

    if (self->rtp_manager)
    {
        int ret;
        tmedia_param_t *media_param = tsk_null;
        static const int32_t __ByPassIsYes = 1;
        static const int32_t __ByPassIsNo = 0;

        /* RTP/RTCP manager: use latest information. */


        // network information will be updated when the RTP manager starts if ICE is enabled
        ret = trtp_manager_set_rtp_remote (self->rtp_manager, self->remote_ip, self->remote_port);
        ret = trtp_manager_set_payload_type (self->rtp_manager, best_codec->neg_format ? atoi (best_codec->neg_format) : atoi (best_codec->format));
        {
            int32_t bandwidth_max_upload_kbps = self->bandwidth_max_upload_kbps;
            int32_t bandwidth_max_download_kbps = self->bandwidth_max_download_kbps;
            TSK_DEBUG_INFO ("max_bw_up=%d kpbs, max_bw_down=%d kpbs, congestion_ctrl_enabled=%d, media_type=%d", bandwidth_max_upload_kbps, bandwidth_max_download_kbps, self->congestion_ctrl_enabled,
                            self->media_type);
            // forward up/down bandwidth info to rctp session (used in RTCP-REMB)
            trtp_manager_set_app_bandwidth_max (self->rtp_manager, bandwidth_max_upload_kbps, bandwidth_max_download_kbps);
        }

        // because of AudioUnit under iOS => prepare both consumer and producer then start() at the
        // same time
        /* prepare consumer and producer */
        // Producer could output encoded frames:
        //	- On WP8 with built-in H.264 encoder
        //	- When Intel Quick Sync is used for encoding and added on the same Topology as the
        // producer (camera MFMediaSource)
        if (self->producer)
        {
            if ((ret = tmedia_producer_prepare (self->producer, best_codec)) == 0)
            {
                media_param = tmedia_param_create (tmedia_pat_set, best_codec->type, tmedia_ppt_codec, tmedia_pvt_int32, "bypass-encoding",
                                                   (void *)(self->producer->encoder.codec_id == best_codec->id ? &__ByPassIsYes : &__ByPassIsNo));
                if (media_param)
                {
                    tmedia_codec_set (TMEDIA_CODEC (best_codec), media_param);
                    TSK_OBJECT_SAFE_FREE (media_param);
                }
            }
        }
        // Consumer could accept encoded frames as input:
        //	- On WP8 with built-in H.264 decoder
        //	- When IMFTransform decoder is used for decoding and added on the same Topology as the
        // consumer (EVR)
        if (self->consumer)
        {
            if ((ret = tmedia_consumer_prepare (self->consumer, best_codec)) == 0)
            {
                media_param = tmedia_param_create (tmedia_pat_set, best_codec->type, tmedia_ppt_codec, tmedia_pvt_int32, "bypass-decoding",
                                                   (void *)(self->consumer->decoder.codec_id == best_codec->id ? &__ByPassIsYes : &__ByPassIsNo));
                if (media_param)
                {
                    tmedia_codec_set (TMEDIA_CODEC (best_codec), media_param);
                    TSK_OBJECT_SAFE_FREE (media_param);
                }
            }
        }

        // Start RTP manager
        ret = trtp_manager_start (self->rtp_manager);

        tsk_safeobj_lock (self);
        if (self->consumer && !self->consumer->is_started)
            ret = tmedia_consumer_start (self->consumer);
        if (self->producer && !self->producer->is_started)
        {
            ret = tmedia_producer_start (self->producer);
        }
        tsk_safeobj_unlock (self);

        return ret;
    }
    else
    {
        TSK_DEBUG_ERROR ("Invalid RTP/RTCP manager");
        return -3;
    }

    return 0;
}

int tdav_session_av_start_video (tdav_session_av_t *self, const tmedia_codec_t *best_codec)
{
    TSK_DEBUG_INFO ("@@ tdav_session_av_start_video");
    if (!self || !best_codec)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    // 这里video接口不改动rtp manager内部逻辑
    if (self->rtp_manager)
    {
        int ret = -1;
        tmedia_param_t *media_param = tsk_null;
        static const int32_t __ByPassIsYes = 1;
        static const int32_t __ByPassIsNo = 0;
        
        // because of AudioUnit under iOS => prepare both consumer and producer then start() at the
        // same time
        /* prepare consumer and producer */
        // Producer could output encoded frames:
        //	- On WP8 with built-in H.264 encoder
        //	- When Intel Quick Sync is used for encoding and added on the same Topology as the
        // producer (camera MFMediaSource)
        if (self->producer)
        {
            if ((ret = tmedia_producer_prepare (self->producer, best_codec)) == 0)
            {
                media_param = tmedia_param_create (tmedia_pat_set, best_codec->type, tmedia_ppt_codec, tmedia_pvt_int32, "bypass-encoding",
                                                   (void *)(self->producer->encoder.codec_id == best_codec->id ? &__ByPassIsYes : &__ByPassIsNo));
                if (media_param)
                {
                    tmedia_codec_set (TMEDIA_CODEC (best_codec), media_param);
                    TSK_OBJECT_SAFE_FREE (media_param);
                }
                
                media_param = tmedia_param_create (tmedia_pat_set, best_codec->type, tmedia_ppt_codec, tmedia_pvt_int32, "rotation",
                                                   (void *)&self->screenOrientation);
                if (media_param)
                {
                    tmedia_codec_set (TMEDIA_CODEC (best_codec), media_param);
                    TSK_OBJECT_SAFE_FREE (media_param);
                }
            }
        }
        // Consumer could accept encoded frames as input:
        //	- On WP8 with built-in H.264 decoder
        //	- When IMFTransform decoder is used for decoding and added on the same Topology as the
        // consumer (EVR)
        if (self->consumer)
        {
            if ((ret = tmedia_consumer_prepare (self->consumer, best_codec)) == 0)
            {
                media_param = tmedia_param_create (tmedia_pat_set, best_codec->type, tmedia_ppt_codec, tmedia_pvt_int32, "bypass-decoding",
                                                   (void *)(self->consumer->decoder.codec_id == best_codec->id ? &__ByPassIsYes : &__ByPassIsNo));
                if (media_param)
                {
                    tmedia_codec_set (TMEDIA_CODEC (best_codec), media_param);
                    TSK_OBJECT_SAFE_FREE (media_param);
                }
            }
        }

        tsk_safeobj_lock (self);
        if (self->consumer && !self->consumer->is_started)
            ret = tmedia_consumer_start (self->consumer);
        if (self->producer && !self->producer->is_started)
        {
            TSK_DEBUG_INFO ("tdav_session_av_start_video start producer");
            ret = tmedia_producer_start (self->producer);
        }
        tsk_safeobj_unlock (self);
        
        return ret;
    }
    else
    {
        TSK_DEBUG_ERROR ("Invalid RTP/RTCP manager");
        
        return -3;
    }
    
    return 0;
}

int tdav_session_av_stop (tdav_session_av_t *self)
{
    TSK_DEBUG_INFO("Enter");
    tmedia_codec_t *codec;
    tsk_list_item_t *item;
    int ret = 0;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* stop Producer */
    if (self->producer)
    {
        tsk_mutex_lock(self->producer_mutex);
        ret = tmedia_producer_stop (self->producer);
        tsk_mutex_unlock(self->producer_mutex);
    }

    /* stop RTP/RTCP manager */
    if (self->rtp_manager)
    {
        int iCurCount = trtp_manager_subPlugRefCount(self->rtp_manager);
        if (0 == iCurCount) {
           trtp_manager_stop (self->rtp_manager);
        }
        
    }

    /* stop Consumer (after RTP manager to silently discard in coming packets) */
    if (self->consumer)
    {
        ret = tmedia_consumer_stop (self->consumer);
    }

    /* close codecs to force open() for next start (e.g SIP UPDATE with SDP) */
    if (TMEDIA_SESSION (self)->neg_codecs)
    {
        tsk_list_foreach (item, TMEDIA_SESSION (self)->neg_codecs)
        {
            if (!(codec = TMEDIA_CODEC (item->data)))
            {
                continue;
            }
            ret = tmedia_codec_close (codec);
        }
    }
    TSK_DEBUG_INFO("Leave");
    return ret;
}

int tdav_session_av_pause (tdav_session_av_t *self)
{
    int ret = 0;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* Consumer */
    if (self->consumer)
    {
        ret = tmedia_consumer_pause (self->consumer);
    }
    /* Producer */
    if (self->producer)
    {
        ret = tmedia_producer_pause (self->producer);
    }

    return ret;
}

const tsdp_header_M_t *tdav_session_av_get_lo (tdav_session_av_t *self, tsk_bool_t *updated)
{
    tmedia_session_t *base = TMEDIA_SESSION (self);
    tsk_bool_t is_first_media;

    if (!base || !base->plugin || !updated)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    *updated = tsk_false;

    if (!self->rtp_manager || !self->rtp_manager->transport)
    {
        if (self->rtp_manager && (!self->rtp_manager->transport))
        { // reINVITE or UPDATE (manager was destroyed when stoppped)
            if (trtp_manager_prepare (self->rtp_manager))
            {
                TSK_DEBUG_ERROR ("Failed to prepare transport");
                return tsk_null;
            }
        }
        else
        {
            TSK_DEBUG_ERROR ("RTP/RTCP manager in invalid");
            return tsk_null;
        }
    }

    // only first media will add session-level attributes (e.g. DTLS setup and fingerprint)
    if ((is_first_media = !!self->local_sdp))
    {
        const tsdp_header_M_t *firstM = (const tsdp_header_M_t *)tsdp_message_get_headerAt (self->local_sdp, tsdp_htype_M, 0);
        if (!(is_first_media = !firstM))
        {
            is_first_media = tsk_striequals (TMEDIA_SESSION (self)->plugin->media, firstM->media);
        }
    }

    if (base->ro_changed && base->M.lo)
    {
        static const char *__fields[] = { /* Codecs */
                                          "fmtp", "rtpmap", "imageattr",
                                          /* QoS */
                                          "curr", "des", "conf",
                                          /* SRTP */
                                          "crypto",
                                          /* DTLS */
                                          "setup", "fingerprint",
                                          /* ICE */
                                          "candidate", "ice-ufrag", "ice-pwd",
                                          /* SDPCapNeg */
                                          "tcap", "acap", "pcfg",
                                          /* Others */
                                          "mid", "rtcp-mux", "ssrc"
        };
        // remove media-level attributes
        tsdp_header_A_removeAll_by_fields (base->M.lo->Attributes, __fields, sizeof (__fields) / sizeof (__fields[0]));
        tsk_list_clear_items (base->M.lo->FMTs);
        // remove session-level attributes
        if (is_first_media)
        {
            // headers: contains all kind of headers but this is a smart function :)
            tsdp_header_A_removeAll_by_fields ((tsdp_headers_A_L_t *)self->local_sdp->headers, __fields, sizeof (__fields) / sizeof (__fields[0]));
        }
    }

    *updated = (base->ro_changed || !base->M.lo);

    if (!base->M.lo)
    {
        if ((base->M.lo = tsdp_header_M_create (base->plugin->media, self->rtp_manager->rtp.public_port, "RTP/AVP")))
        {
           
            /* 3GPP TS 24.229 - 6.1.1 General
             In order to support accurate bandwidth calculations, the UE may include the "a=ptime"
             attribute for all "audio" media
             lines as described in RFC 4566 [39]. If a UE receives an "audio" media line with
             "a=ptime" specified, the UE should
             transmit at the specified packetization rate. If a UE receives an "audio" media line
             which does not have "a=ptime"
             specified or the UE does not support the "a=ptime" attribute, the UE should transmit at
             the default codec packetization
             rate as defined in RFC 3551 [55A]. The UE will transmit consistent with the resources
             available from the network.

             For "video" and "audio" media types that utilize the RTP/RTCP, the UE shall specify the
             proposed bandwidth for each
             media stream utilizing the "b=" media descriptor and the "AS" bandwidth modifier in the
             SDP.

             The UE shall include the MIME subtype "telephone-event" in the "m=" media descriptor in
             the SDP for audio media
             flows that support both audio codec and DTMF payloads in RTP packets as described in
             RFC 4733 [23].
             */
            if (self->media_type & tmedia_audio)
            {
                tsk_istr_t ptime;
                tsk_itoa (tmedia_defaults_get_audio_ptime (), &ptime);
                tsdp_header_M_add_headers (base->M.lo,
                                           /* rfc3551 section 4.5 says the default ptime is 20 */
                                           TSDP_HEADER_A_VA_ARGS ("ptime", ptime),
                                           // TSDP_HEADER_A_VA_ARGS("minptime", "1"),
                                           // TSDP_HEADER_A_VA_ARGS("maxptime", "255"),
                                           // TSDP_HEADER_A_VA_ARGS("silenceSupp", "off - - - -"),
                                           tsk_null);
                // the "telephone-event" fmt/rtpmap is added below
            }
            else if ((self->media_type & tmedia_video || (self->media_type & tmedia_bfcp_video) == tmedia_bfcp_video))
            {
                tsk_istr_t session_id;
                // https://code.google.com/p/webrtc2sip/issues/detail?id=81
                // goog-remb:
                // https://groups.google.com/group/discuss-webrtc/browse_thread/thread/c61ad3487e2acd52
                // rfc5104 - 7.1.  Extension of the rtcp-fb Attribute
                tsdp_header_M_add_headers (base->M.lo, TSDP_HEADER_A_VA_ARGS ("rtcp-fb", "* ccm fir"), TSDP_HEADER_A_VA_ARGS ("rtcp-fb", "* nack"), TSDP_HEADER_A_VA_ARGS ("rtcp-fb", "* goog-remb"), tsk_null);
                // https://tools.ietf.org/html/rfc4574
                // http://tools.ietf.org/html/rfc4796
                tsk_itoa (base->id, &session_id);
                tsdp_header_M_add_headers (base->M.lo, TSDP_HEADER_A_VA_ARGS ("label", session_id), TSDP_HEADER_A_VA_ARGS ("content", (self->media_type & tmedia_bfcp) ? "slides" : "main"), tsk_null);
                // http://tools.ietf.org/html/rfc3556
                // https://tools.ietf.org/html/rfc3890
            }
        }
        else
        {
            TSK_DEBUG_ERROR ("Failed to create lo");
            return tsk_null;
        }
    }

    if (*updated)
    {
        tmedia_codecs_L_t *neg_codecs = tsk_null;

        if (base->M.ro)
        {
            TSK_OBJECT_SAFE_FREE (base->neg_codecs);
            /* update negociated codecs */
            if ((neg_codecs = tmedia_session_match_codec (base, base->M.ro)))
            {
                base->neg_codecs = neg_codecs;
            }
            /* from codecs to sdp */
            if (TSK_LIST_IS_EMPTY (base->neg_codecs))
            {
                base->M.lo->port = 0; /* Keep the RTP transport and reuse it when we receive a
                                         reINVITE or UPDATE request */
                // To reject an offered stream, the port number in the corresponding stream in the
                // answer
                // MUST be set to zero.  Any media formats listed are ignored.  AT LEAST ONE MUST BE
                // PRESENT, AS SPECIFIED BY SDP.
                tsk_strupdate (&base->M.lo->proto, base->M.ro->proto);
                if (base->M.ro->FMTs)
                {
                    tsk_list_pushback_list (base->M.lo->FMTs, base->M.ro->FMTs);
                }
                TSK_DEBUG_INFO ("No codec matching for media type = %d", (int32_t)self->media_type);
                goto DONE;
            }
            else
            {
                tmedia_codec_to_sdp (base->neg_codecs, base->M.lo);
            }
        }
        else
        {
            /* from codecs to sdp */
            tmedia_codec_to_sdp (base->codecs, base->M.lo);
        }


        /* RFC 5939: acfg */
        if (self->sdp_caps->acfg.tag > 0)
        {
            _sdp_acfg_to_sdp (base->M.lo, &self->sdp_caps->acfg);
        }


        tsdp_header_M_set_holdresume_att (base->M.lo, base->lo_held, base->ro_held);

        /* Update Proto*/
        tsk_strupdate (&base->M.lo->proto,((self->avpf_mode_neg == tmedia_mode_mandatory) ? "RTP/AVPF" : "RTP/AVP"));

        
            if (base->M.lo->C)
            {
                tsk_strupdate (&base->M.lo->C->addr, self->rtp_manager->rtp.public_ip);
                tsk_strupdate (&base->M.lo->C->addrtype, (self->use_ipv6 ? "IP6" : "IP4"));
            }
            base->M.lo->port = self->rtp_manager->rtp.public_port;
        

        if (self->media_type & tmedia_audio)
        {
            ///* 3GPP TS 24.229 - 6.1.1 General
            //	The UE shall include the MIME subtype "telephone-event" in the "m=" media descriptor
            // in the SDP for audio media
            //	flows that support both audio codec and DTMF payloads in RTP packets as described in
            // RFC 4733 [23].
            //*/
            // tsdp_header_M_add_fmt(base->M.lo, TMEDIA_CODEC_FORMAT_DTMF);
            // tsdp_header_M_add_headers(base->M.lo,
            //			TSDP_HEADER_A_VA_ARGS("fmtp", TMEDIA_CODEC_FORMAT_DTMF" 0-15"),
            //		tsk_null);
            // tsdp_header_M_add_headers(base->M.lo,
            //			TSDP_HEADER_A_VA_ARGS("rtpmap", TMEDIA_CODEC_FORMAT_DTMF"
            // telephone-event/8000"),
            //		tsk_null);
        }
    DONE:;
    } // end-of-if(*updated)

    return base->M.lo;
}

int tdav_session_av_set_ro (tdav_session_av_t *self, const struct tsdp_header_M_s *m, tsk_bool_t *updated)
{
    tmedia_codecs_L_t *neg_codecs;
    tmedia_session_t *base = TMEDIA_SESSION (self);
    RTP_PROFILE_T profile_remote;
    int32_t acfg_idx = -1;

    if (!base || !m || !updated)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (!self->rtp_manager)
    {
        TSK_DEBUG_ERROR ("RTP manager is null. Did you forget to prepare the session?");
        return -1;
    }

    /* update remote offer */
    TSK_OBJECT_SAFE_FREE (base->M.ro);
    base->M.ro = tsk_object_ref ((void *)m);

    *updated = tsk_false;

    // check if the RTP profile from remote party is supported or not
    if ((profile_remote = _sdp_profile_from_string (m->proto)) == RTP_PROFILE_NONE)
    {
        TSK_DEBUG_ERROR ("%s not supported as RTP profile", m->proto);
        return -2;
    }
   
   

    if (base->M.lo)
    {
        if ((neg_codecs = tmedia_session_match_codec (base, m)))
        {
            /* update negociated codecs */
            TSK_OBJECT_SAFE_FREE (base->neg_codecs);
            base->neg_codecs = neg_codecs;
            *updated = tsk_true;
        }
        else
        {
            TSK_DEBUG_ERROR ("Codecs mismatch");
            return -1;
        }
    }

    /* AVPF */
    if (self->avpf_mode_set == tmedia_mode_optional && self->avpf_mode_neg != tmedia_mode_mandatory)
    {
        self->avpf_mode_neg = _sdp_str_contains (base->M.ro->proto, "AVPF") ? tmedia_mode_mandatory : tmedia_mode_none;
    }

    /* RFC 5939 - Session Description Protocol (SDP) Capability Negotiation */
    {
        sdp_acaps_xt acaps;
        sdp_tcaps_xt tcaps;

        _sdp_acfg_reset (&self->sdp_caps->acfg);

        _sdp_acaps_reset (&acaps);
        _sdp_tcaps_reset (&tcaps);
        _sdp_pcfgs_reset (&self->sdp_caps->remote);

        // session-level attributes
        if (self->remote_sdp)
        {
            _sdp_pcfgs_from_sdp (self->remote_sdp, &acaps, &tcaps, &self->sdp_caps->remote, tsk_false);
        }
        // media-level attributes
        _sdp_pcfgs_from_sdp (base->M.ro, &acaps, &tcaps, &self->sdp_caps->remote, tsk_false);
    }

    /* get connection associated to this media line
     * If the connnection is global, then the manager will call tdav_session_audio_set() */
    if (m->C && m->C->addr)
    {
        tsk_strupdate (&self->remote_ip, m->C->addr);
        self->use_ipv6 = tsk_striequals (m->C->addrtype, "IP6");
    }
    /* set remote port */
    self->remote_port = m->port;

   

    /* Remote SSRC */
    {
        // will be also updated based on received RTP packets
        const tsdp_header_A_t *ssrcA = tsdp_header_M_findA (m, "ssrc");
        if (ssrcA && ssrcA->value)
        {
            if (sscanf (ssrcA->value, "%u %*s", &self->rtp_manager->rtp.ssrc.remote) != EOF)
            {
                TSK_DEBUG_INFO ("Remote SSRC = %u", self->rtp_manager->rtp.ssrc.remote);
            }
        }
    }

    // set actual config
    if (acfg_idx == -1)
    {
        // none matched (means SRTP negotiation failed or not enabled -> try to negotiate AVP(F))
        int32_t i;
        for (i = 0; (i < SDP_CAPS_COUNT_MAX && self->sdp_caps->remote[i].tag > 0); ++i)
        {
            if (self->sdp_caps->remote[i].tcap.tag > 0)
            {
                if ((self->sdp_caps->remote[i].tcap.profile & RTP_PROFILE_AVPF) == RTP_PROFILE_AVPF)
                {
                    acfg_idx = i;
                    break;
                }
            }
        }
    }
    if (acfg_idx != -1)
    {
        self->sdp_caps->acfg = self->sdp_caps->remote[acfg_idx];
        if (self->avpf_mode_set == tmedia_mode_optional && self->avpf_mode_neg != tmedia_mode_mandatory)
        {
            self->avpf_mode_neg = ((self->sdp_caps->acfg.tcap.profile & RTP_PROFILE_AVPF) == RTP_PROFILE_AVPF) ? tmedia_mode_mandatory : tmedia_mode_none;
        }
    }

    return 0;
}

const tmedia_codec_t *tdav_session_av_get_best_neg_codec (const tdav_session_av_t *self)
{
    const tsk_list_item_t *item;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    tsk_list_foreach (item, TMEDIA_SESSION (self)->neg_codecs)
    {
        // exclude DTMF, RED and ULPFEC
        if (TMEDIA_CODEC (item->data)->plugin &&
            (TMEDIA_CODEC (item->data)->plugin->encode && TMEDIA_CODEC (item->data)->plugin->decode) ||
            (TMEDIA_CODEC (item->data)->plugin->encode_new && TMEDIA_CODEC (item->data)->plugin->decode_new))
        {
            return TMEDIA_CODEC (item->data);
        }
    }
    return tsk_null;
}

int tdav_session_av_deinit (tdav_session_av_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    /* deinit self (rtp manager should be destroyed after the producer) */
    TSK_OBJECT_SAFE_FREE (self->consumer);
    TSK_OBJECT_SAFE_FREE (self->producer);
    TSK_OBJECT_SAFE_FREE (self->sdp_caps);
    TSK_OBJECT_SAFE_FREE (self->remote_sdp);
    TSK_OBJECT_SAFE_FREE (self->local_sdp);
    TSK_FREE (self->remote_ip);
    TSK_FREE (self->local_ip);
    if (self->media_type == tmedia_audio) {
        TSK_OBJECT_SAFE_FREE (self->rtp_manager);
    } else if(self->media_type == tmedia_video) {
        self->rtp_manager = tsk_null;
    }
    
    if (self->rtp_manager_mutex) {
        tsk_mutex_destroy(&(self->rtp_manager_mutex));
        self->rtp_manager_mutex = tsk_null;
    }
    
    if (self->producer_mutex) {
        tsk_mutex_destroy(&(self->producer_mutex));
        self->producer_mutex = tsk_null;
    }

    /* NAT Traversal context */
  

    /* Last error */
    if (self->last_error.tid[0])
    {
        tsk_thread_join (self->last_error.tid);
    }
    TSK_FREE (self->last_error.reason);

    tsk_safeobj_deinit (self);

    /* deinit base */
    tmedia_session_deinit (TMEDIA_SESSION (self));

    return 0;
}

// rtp manager对象set&get
void tdav_session_set_rtp_manager(struct trtp_manager_s* rtp_manager)
{
    g_rtp_manager = rtp_manager;
}

struct trtp_manager_s *tdav_session_get_rtp_manager()
{
    if (NULL == g_rtp_manager)
    {
        return NULL;
    }
    
    return g_rtp_manager;
}


static const tsdp_header_A_t *_sdp_findA_at (const sdp_headerM_Or_Message *sdp, const char *field, tsk_size_t index)
{
    if (sdp)
    {
        if (TSK_OBJECT_HEADER (sdp)->__def__ == tsdp_message_def_t)
        {
            return tsdp_message_get_headerA_at ((const tsdp_message_t *)sdp, field, index);
        }
        else if (TSK_OBJECT_HEADER (sdp)->__def__ == tsdp_header_M_def_t)
        {
            return tsdp_header_M_findA_at ((const tsdp_header_M_t *)sdp, field, index);
        }
    }

    TSK_DEBUG_ERROR ("Invalid parameter");
    return tsk_null;
}

static int _sdp_add_headerA (sdp_headerM_Or_Message *sdp, const char *field, const char *value)
{
    if (sdp && field)
    {
        if (TSK_OBJECT_HEADER (sdp)->__def__ == tsdp_message_def_t)
        {
            return tsdp_message_add_headers ((tsdp_message_t *)sdp, TSDP_HEADER_A_VA_ARGS (field, value), tsk_null);
        }
        else if (TSK_OBJECT_HEADER (sdp)->__def__ == tsdp_header_M_def_t)
        {
            return tsdp_header_M_add_headers ((tsdp_header_M_t *)sdp, TSDP_HEADER_A_VA_ARGS (field, value), tsk_null);
        }
    }

    TSK_DEBUG_ERROR ("Invalid parameter");
    return -1;
}

static RTP_PROFILE_T _sdp_profile_from_string (const char *profile)
{
    int32_t i;
    for (i = 0; i < RTP_PROFILES_COUNT; ++i)
    {
        if (tsk_striequals (RTP_PROFILES[i].name, profile))
        {
            return RTP_PROFILES[i].type;
        }
    }
    return RTP_PROFILE_NONE;
}


_SDP_DECLARE_INDEX_OF (acap);


static int _sdp_acaps_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_acap_xt (*acaps)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset)
{
    tsk_size_t acaps_count, acaps_idx;
    const tsdp_header_A_t *A;
    int32_t tag, index, size;

    if (!sdp || !acaps)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }


    if (reset)
    {
        _sdp_acaps_reset (acaps);
        acaps_count = 0;
    }
    else
    {
        if ((acaps_count = _sdp_acaps_indexof (acaps, 0)) == -1)
        {
            TSK_DEBUG_ERROR ("No room to append items");
            return -1;
        }
    }

    acaps_idx = 0;
    while ((A = _sdp_findA_at (sdp, "acap", acaps_idx++)))
    {
        if (!(size = (int32_t)tsk_strlen (A->value)))
        {
            goto next;
        }
        if (sscanf (A->value, "%d", &tag) == EOF)
        {
            TSK_DEBUG_ERROR ("sscanf(%s) failed", A->value);
            break;
        }
        if (tag <= 0 || (tag + 1) > SDP_CAPS_COUNT_MAX)
        {
            TSK_DEBUG_WARN ("Ignoring tag with value = %d", tag);
            goto next;
        }

        index = _sdp_integer_length (tag) + 1; /*SPACE*/
        if (index >= size)
        {
            TSK_DEBUG_WARN ("a=%s is empty", A->value);
            goto next;
        }

        (*acaps)[acaps_count].tag = tag;
        (*acaps)[acaps_count].value = &A->value[index];
    next:
        if (++acaps_count >= SDP_CAPS_COUNT_MAX)
        {
            break;
        }
    }

    return 0;
}

_SDP_DECLARE_INDEX_OF (tcap);

static int _sdp_tcaps_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_tcap_xt (*tcaps)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset)
{
    int32_t tcaps_count, tcaps_idx, profiles_count;
    const tsdp_header_A_t *A;
    int32_t tag, index, size, tag_fake;
    char tcap[256];

    if (!sdp || !tcaps)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if (reset)
    {
        _sdp_tcaps_reset (tcaps);
        tcaps_count = 0;
    }
    else
    {
        if ((tcaps_count = _sdp_tcaps_indexof (tcaps, 0)) == -1)
        {
            TSK_DEBUG_ERROR ("No room to append items");
            return -1;
        }
    }

    profiles_count = 0;
    index = 0;
    tcaps_idx = 0;
    while ((A = _sdp_findA_at (sdp, "tcap", tcaps_idx++)))
    {
        if (!(size = (int32_t)tsk_strlen (A->value)))
        {
            goto next;
        }
        if (sscanf (&A->value[index], "%d", &tag) == EOF || (_sdp_integer_length (tag) + 1 >= size))
        {
            TSK_DEBUG_ERROR ("sscanf(%s) failed", A->value);
            break;
        }
        if (tag <= 0 || (tag + 1) > SDP_CAPS_COUNT_MAX)
        {
            TSK_DEBUG_WARN ("Ignoring tag with value = %d", tag);
            goto next;
        }

        index += _sdp_integer_length (tag) + 1 /*SPACE*/;

        profiles_count = 0;
        tag_fake = tag;
        while (sscanf (&A->value[index], "%255s", tcap) != EOF)
        {
            if (tag_fake < SDP_CAPS_COUNT_MAX)
            {
                (*tcaps)[tcaps_count + profiles_count].tag = tag_fake;
                (*tcaps)[tcaps_count + profiles_count].profile = _sdp_profile_from_string (tcap); // split profiles
            }
            if ((index += (int32_t)tsk_strlen (tcap) + 1 /*SPACE*/) >= size)
            {
                break;
            }
            ++tag_fake;
            ++profiles_count;
        }
    next:
        if (++tcaps_count >= SDP_CAPS_COUNT_MAX)
        {
            break;
        }
    }

    return 0;
}

static int _sdp_acfg_to_sdp (sdp_headerM_Or_Message *sdp, const sdp_acfg_xt *acfg)
{
    int32_t i_a_caps;
    char *acfg_str = tsk_null;

    if (!sdp || !acfg || acfg->tag <= 0)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    // acfg: tag
    tsk_strcat_2 (&acfg_str, "%d", acfg->tag);
    // acfg: t=
    if (acfg_str && acfg->tcap.tag > 0)
    {
        tsk_strcat_2 (&acfg_str, " t=%d", acfg->tcap.tag);
    }
    // acfg: a=
    for (i_a_caps = 0; acfg_str && i_a_caps < SDP_CAPS_COUNT_MAX; ++i_a_caps)
    {
        if (acfg->acaps[i_a_caps].tag <= 0)
        {
            break;
        }
        if (i_a_caps == 0)
        {
            tsk_strcat_2 (&acfg_str, " a=%d", acfg->acaps[i_a_caps].tag);
        }
        else
        {
            tsk_strcat_2 (&acfg_str, "%s%s%d%s", // e.g. |2 or ,6 or ,[2]
                          acfg->acaps[i_a_caps].or ? "|" : ",", acfg->acaps[i_a_caps].optional ? "[" : "", acfg->acaps[i_a_caps].tag, acfg->acaps[i_a_caps].optional ? "]" : "");
        }
    }

    // a=acfg:
    if (acfg_str)
    {
        _sdp_add_headerA (sdp, "acfg", acfg_str);
        TSK_FREE (acfg_str);
    }

    return 0;
}

_SDP_DECLARE_INDEX_OF (pcfg);

static int _sdp_pcfgs_from_sdp (const sdp_headerM_Or_Message *sdp, sdp_acap_xt (*acaps)[SDP_CAPS_COUNT_MAX], sdp_tcap_xt (*tcaps)[SDP_CAPS_COUNT_MAX], sdp_pcfg_xt (*pcfgs)[SDP_CAPS_COUNT_MAX], tsk_bool_t reset)
{
    tsk_size_t pcfgs_count, pcfgs_idx;
    const tsdp_header_A_t *A;
    int32_t tag, index = 0, size, t, a_tag, indexof;
    sdp_tcap_xt *tcap_curr;
    int ret;
    char pcfg[256], a[256];

    if (!sdp || !acaps || !tcaps || !pcfgs)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    if ((ret = _sdp_tcaps_from_sdp (sdp, tcaps, reset)))
    {
        return ret;
    }
    if ((ret = _sdp_acaps_from_sdp (sdp, acaps, reset)))
    {
        return ret;
    }

    if (reset)
    {
        _sdp_pcfgs_reset (pcfgs);
        pcfgs_count = 0;
    }
    else
    {
        if ((pcfgs_count = _sdp_pcfgs_indexof (pcfgs, 0)) == -1)
        {
            TSK_DEBUG_ERROR ("No room to append items");
            return -1;
        }
    }

    pcfgs_idx = 0;
    tcap_curr = tsk_null;
    while ((A = _sdp_findA_at (sdp, "pcfg", pcfgs_idx++)))
    {
        if (!(size = (int32_t)tsk_strlen (A->value)))
        {
            goto next_A;
        }
        if (sscanf (A->value, "%d", &tag) == EOF || (_sdp_integer_length (tag) + 1 >= size))
        {
            TSK_DEBUG_ERROR ("sscanf(%s) failed", A->value);
            break;
        }
        if (tag <= 0 || (tag + 1) > SDP_CAPS_COUNT_MAX)
        {
            TSK_DEBUG_WARN ("Ignoring tag with value = %d", tag);
            goto next_A;
        }

        (*pcfgs)[pcfgs_count].tag = tag;

        index = _sdp_integer_length (tag) + 1 /*SPACE*/;

        while (sscanf (&A->value[index], "%255s", pcfg) != EOF)
        {
            if (_sdp_str_starts_with (&A->value[index], "t=") && sscanf (pcfg, "t=%d", &t) != EOF)
            {
                if (t <= 0 || t + 1 >= SDP_CAPS_COUNT_MAX)
                {
                    TSK_DEBUG_ERROR ("t = %d ignored", t);
                    goto next_pcfg;
                }
                // tcap is something like a=tcap:1 RTP/SAVPF RTP/SAVP RTP/AVPF
                // tcap [2] is "RTP/SAVP" -> not indexed by tag
                tcap_curr = &(*pcfgs)[pcfgs_count].tcap;
                if ((indexof = _sdp_tcaps_indexof (tcaps, t)) == -1)
                {
                    TSK_DEBUG_ERROR ("Failed to find 'tcap' with tag=%d", t);
                    goto next_pcfg;
                }
                *tcap_curr = (*tcaps)[indexof];
            }
            else
            {
                if (_sdp_str_starts_with (&A->value[index], "a=") && sscanf (pcfg, "a=%255s", a) != EOF)
                {
                    char a_copy[sizeof (a)], *pch, *saveptr;
                    tsk_size_t pcfg_acfgs_count = 0;
                    sdp_acap_xt *acap;
                    memcpy (a_copy, a, sizeof (a));

                    pch = tsk_strtok_r (a, ",[]|", &saveptr);
                    while (pch)
                    {
                        a_tag = atoi (pch);
                        if (a_tag <= 0 || a_tag + 1 >= SDP_CAPS_COUNT_MAX)
                        {
                            TSK_DEBUG_ERROR ("a = %d ignored", a_tag);
                            goto next_a;
                        }
                        if ((indexof = _sdp_acaps_indexof (acaps, a_tag)) == -1)
                        {
                            TSK_DEBUG_ERROR ("Failed to find 'acap' with tag=%d", a_tag);
                            goto next_a;
                        }
                        acap = &(*pcfgs)[pcfgs_count].acaps[pcfg_acfgs_count++];
                        *acap = (*acaps)[indexof];
                        acap->optional = (pch != a && a_copy[pch - a - 1] == '[') ? 1 : 0;
                        acap->or = (pch != a && a_copy[pch - a - 1] == '|') ? 1 : 0;
                    next_a:
                        pch = tsk_strtok_r (tsk_null, ",[]|", &saveptr);
                    }
                }
                tcap_curr = tsk_null;
            }
        next_pcfg:
            if ((index += (int32_t)tsk_strlen (pcfg) + 1 /*SPACE*/) >= size)
            {
                break;
            }
        }
    next_A:
        if (++pcfgs_count >= SDP_CAPS_COUNT_MAX)
        {
            break;
        }
    }

    return ret;
}


static tsk_object_t *tdav_sdp_caps_ctor (tsk_object_t *self, va_list *app)
{
    tdav_sdp_caps_t *caps = self;
    if (caps)
    {
    }
    return self;
}
static tsk_object_t *tdav_sdp_caps_dtor (tsk_object_t *self)
{
    tdav_sdp_caps_t *caps = self;
    if (caps)
    {
    }
    return self;
}
static const tsk_object_def_t tdav_sdp_caps_def_s = {
    sizeof (tdav_sdp_caps_t), tdav_sdp_caps_ctor, tdav_sdp_caps_dtor, tsk_null,
};

static tdav_sdp_caps_t *tdav_sdp_caps_create ()
{
    return tsk_object_new (&tdav_sdp_caps_def_s);
}

//=================================================================================================
//	audio/video statistics information object definition
//
static tsk_object_t *tdav_session_av_stat_info_ctor (tsk_object_t *self, va_list *app)
{
    tdav_session_av_stat_info_t *stat_info = self;
    uint32_t maxNumOfItems = va_arg(*app, uint32_t);
    if (stat_info)
    {
        stat_info->numOfItems = 0;
        stat_info->statItems = tsk_calloc(maxNumOfItems, sizeof(tdav_session_av_stat_item_t));
    }
    return self;
}

static tsk_object_t *tdav_session_av_stat_info_dtor (tsk_object_t *self)
{
    tdav_session_av_stat_info_t *stat_info = self;
    if (stat_info)
    {
        // free resources here
        tsk_free((void**)&(stat_info->statItems));
    }
    return self;
}

static int tdav_session_av_stat_info_cmp (const tsk_object_t *_e1, const tsk_object_t *_e2)
{
    int ret;
    tsk_subsat_int32_ptr (_e1, _e2, &ret);
    return ret;
}

static const tsk_object_def_t tdav_session_av_stat_info_def_s = {
    sizeof (tdav_session_av_stat_info_t),
    tdav_session_av_stat_info_ctor,
    tdav_session_av_stat_info_dtor,
    tdav_session_av_stat_info_cmp
};
const tsk_object_def_t *tdav_session_av_stat_info_def_t = &tdav_session_av_stat_info_def_s;

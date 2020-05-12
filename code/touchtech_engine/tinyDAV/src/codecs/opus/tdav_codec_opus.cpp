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

/**@file tdav_codec_opus.c
 * @brief OPUS audio codec.
 * SDP: http://tools.ietf.org/html/draft-spittka-payload-rtp-opus-03
 */
#include "tinydav/codecs/opus/tdav_codec_opus.h"

#if HAVE_LIBOPUS

#include "opus.h"

#include "tinymedia/tmedia_defaults.h"

#include "tinyrtp/rtp/trtp_rtp_packet.h"

#include "tsk_params.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include <map>

#if !defined(TDAV_OPUS_MAX_FRAME_SIZE_IN_SAMPLES)
#define TDAV_OPUS_MAX_FRAME_SIZE_IN_SAMPLES (5760) /* 120ms@48kHz */
#endif
#if !defined(TDAV_OPUS_MAX_FRAME_SIZE_IN_BYTES)
#define TDAV_OPUS_MAX_FRAME_SIZE_IN_BYTES (TDAV_OPUS_MAX_FRAME_SIZE_IN_SAMPLES << 1) /* 120ms@48kHz */
#endif

typedef struct opus_decoder_manager_s
{
    youme::OpusDecoder *inst;
    opus_int16 buff[TDAV_OPUS_MAX_FRAME_SIZE_IN_SAMPLES];
    int32_t    last_seq;
    uint64_t   last_pkt_ts;
} opus_decoder_manager_t;

typedef std::map<int32_t, opus_decoder_manager_t*>  opus_decoder_manager_map_t;

typedef struct tdav_codec_opus_s
{
    TMEDIA_DECLARE_CODEC_AUDIO;

    struct
    {
        youme::OpusEncoder *inst;
        tsk_bool_t fec_enabled;
        tsk_bool_t dtx_enabled;
        int packet_loss_perc;
    } encoder;

    struct
    {
        opus_decoder_manager_map_t* dec_map;//巨坑。 mallco 申请的内存，class 对象的虚函数表是空
        tsk_bool_t fec_enabled;
        tsk_bool_t dtx_enabled;
        opus_int32 rate;
        opus_int32 channels;
    } decoder;
} tdav_codec_opus_t;


static tsk_bool_t _tdav_codec_opus_rate_is_valid (const int32_t rate)
{
    switch (rate)
    {
    case 8000:
    case 12000:
    case 16000:
    case 24000:
    case 48000:
        return tsk_true;
    default:
        return tsk_false;
    }
}

int tdav_codec_opus_set(tmedia_codec_t *_self, const struct tmedia_param_s * param)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)_self;
    if (!opus || !param)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    if (NULL == opus->encoder.inst) {
        TSK_DEBUG_ERROR ("encoder inst is null!!");
        return -1;
    }
    
    if (param->value_type == tmedia_pvt_int32)
    {
        if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_INBAND_FEC_ENABLE))
        {
            int32_t inband_fec_enable = *((int32_t *)param->value);
            opus->encoder.fec_enabled = inband_fec_enable;
            opus_encoder_ctl (opus->encoder.inst, OPUS_SET_INBAND_FEC (inband_fec_enable));
            TSK_DEBUG_INFO ("[OPUS] Set inbandfec:%d", inband_fec_enable);
            return 0;
        }else if(tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_SET_PACKET_LOSS_PERC)) {
            int32_t packet_loss_perc = *((int32_t *)param->value);
            _self->packet_loss_perc = packet_loss_perc;
            opus->encoder.packet_loss_perc = packet_loss_perc;
            opus_encoder_ctl (opus->encoder.inst, OPUS_SET_PACKET_LOSS_PERC (packet_loss_perc));
            TSK_DEBUG_INFO ("[OPUS] Set packet loss perc:%d", packet_loss_perc);
            return 0;
        }else if(tsk_striequals (param->key, TMEDIA_PARAM_KEY_OPUS_SET_BITRATE)) {
            int32_t bitrate = *((int32_t *)param->value);
            opus_encoder_ctl (opus->encoder.inst, OPUS_SET_BITRATE (bitrate));
            TSK_DEBUG_INFO ("[OPUS] Set bitrate:%d", bitrate);
            return 0;
        }
    }
    return -1;
}

static int tdav_codec_opus_open (tmedia_codec_t *self, const void* context)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)self;
    int opus_err;

    if (!opus)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    // Initialize the decoder
    /*
    if (NULL == opus->decoder.inst)
    {
        TSK_DEBUG_INFO ("[OPUS] Open decoder: rate=%d, channels=%d", (int)self->in.rate, (int)TMEDIA_CODEC_AUDIO (self)->in.channels);
        opus->decoder.inst = youme::opus_decoder_create ((opus_int32)self->in.rate, (int)TMEDIA_CODEC_AUDIO (self)->in.channels, &opus_err);
        if (NULL == opus->decoder.inst || opus_err != OPUS_OK)
        {
            TSK_DEBUG_ERROR ("Failed to create Opus decoder(rate=%d, channels=%d) instance with error code=%d.", (int)self->in.rate, (int)TMEDIA_CODEC_AUDIO (self)->in.channels, opus_err);
            return -2;
        }
    }
    opus->decoder.last_seq = -1;
    */
    opus->decoder.rate = (opus_int32)self->out.rate;
    opus->decoder.channels = (opus_int32)TMEDIA_CODEC_AUDIO (self)->in.channels;
    TSK_DEBUG_INFO ("[OPUS] Open decoder: rate=%d, channels=%d", opus->decoder.rate, opus->decoder.channels);

    // Initialize the encoder
    if (NULL == opus->encoder.inst)
    {
        TSK_DEBUG_INFO ("[OPUS] Open encoder: rate=%d, channels=%d", (int)self->in.rate, (int)TMEDIA_CODEC_AUDIO (self)->out.channels);
        opus->encoder.inst = youme::opus_encoder_create ((opus_int32)self->in.rate, (int)TMEDIA_CODEC_AUDIO (self)->out.channels, OPUS_APPLICATION_VOIP, &opus_err);
        if (NULL == opus->encoder.inst || opus_err != OPUS_OK)
        {
            TSK_DEBUG_ERROR ("Failed to create Opus decoder(rate=%d, channels=%d) instance with error code=%d.", (int)self->out.rate, (int)TMEDIA_CODEC_AUDIO (self)->out.channels, opus_err);
            return -2;
        }
    }
    
    tsk_bool_t inBandFecEnabled = tmedia_defaults_get_opus_inband_fec_enabled();
    tsk_bool_t outBandFecEnabled = tmedia_defaults_get_opus_outband_fec_enabled();
    tsk_bool_t dtxEnabled = tmedia_defaults_get_opus_dtx_peroid() > 0;
    tsk_bool_t vbrEnabled = tmedia_defaults_get_opus_encoder_vbr_enabled();
    int complexity = tmedia_defaults_get_opus_encoder_complexity();
    int maxBandwidth = tmedia_defaults_get_opus_encoder_max_bandwidth();
    int bitrate = tmedia_defaults_get_opus_encoder_bitrate();
    TSK_DEBUG_INFO ("Opus encoder: inBandFecEnabled(%d),outBandFecEnabled(%d),dtxEnabled(%d),vbrEnabled(%d),complexity(%d),maxBandwidth(%d),bitrate(%d)",
                    inBandFecEnabled, outBandFecEnabled, dtxEnabled, vbrEnabled, complexity, maxBandwidth, bitrate);

    self->packet_loss_perc = tmedia_defaults_get_opus_packet_loss_perc();
    opus->encoder.packet_loss_perc = tmedia_defaults_get_opus_packet_loss_perc();
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_SIGNAL (OPUS_SIGNAL_VOICE));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_INBAND_FEC (inBandFecEnabled));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_PACKET_LOSS_PERC (opus->encoder.packet_loss_perc));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_DTX (dtxEnabled));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_COMPLEXITY (complexity));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_VBR (vbrEnabled));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_MAX_BANDWIDTH (maxBandwidth));
    opus_encoder_ctl (opus->encoder.inst, OPUS_SET_BITRATE (bitrate));

    return 0;
}

static int tdav_codec_opus_close (tmedia_codec_t *self)
{
    /* resources will be freed by the dctor() */
    return 0;
}

static tsk_size_t tdav_codec_opus_encode (tmedia_codec_t *self, const void *in_data, tsk_size_t in_size, void **out_data, tsk_size_t *out_max_size)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)self;
    opus_int32 ret;

    if (!self || !in_data || !in_size || !out_data)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    if (!opus->encoder.inst)
    {
        TSK_DEBUG_ERROR ("Encoder not ready");
        return 0;
    }

    // we're sure that the output (encoded) size cannot be higher than the input (raw)
    if (*out_max_size < in_size)
    {
        if (!(*out_data = tsk_realloc (*out_data, in_size)))
        {
            TSK_DEBUG_ERROR ("Failed to allocate buffer with size = %zu", in_size);
            *out_max_size = 0;
            return 0;
        }
        *out_max_size = in_size;
    }
    
//    if (opus->encoder.fec_enabled
//        && (opus->encoder.packet_loss_perc != self->packet_loss_perc))
//    {
//        opus->encoder.packet_loss_perc = self->packet_loss_perc;
//        opus_encoder_ctl (opus->encoder.inst, OPUS_SET_PACKET_LOSS_PERC (opus->encoder.packet_loss_perc));
//        TSK_DEBUG_INFO ("----- [OPUS] Apply packet-loss-rate(%d) to opus encoder self :%p", self->packet_loss_perc, self);
//    }

    ret = opus_encode (opus->encoder.inst, (const opus_int16 *)in_data, (int)(in_size >> 1), (unsigned char *)*out_data, (opus_int32)*out_max_size);

    if (ret < 0)
    {
        TSK_DEBUG_ERROR ("opus_encode() failed with error code = %d", ret);
        return 0;
    }

    return (tsk_size_t)ret;
}

static int tdav_codec_opus_creat_new_decoder (tdav_codec_opus_t *opus_codec, int32_t session_id)
{
    int opus_err = OPUS_OK;
    std::pair< opus_decoder_manager_map_t::iterator, bool > insertRet;
    opus_decoder_manager_t *dec_mgr = new opus_decoder_manager_t;
    if (NULL == dec_mgr) {
        goto bail;
    }
    
    memset(dec_mgr, 0, sizeof(opus_decoder_manager_t));
    dec_mgr->inst = youme::opus_decoder_create (opus_codec->decoder.rate, opus_codec->decoder.channels, &opus_err);
    if ((NULL == dec_mgr->inst) || (opus_err != OPUS_OK)) {
        TSK_DEBUG_ERROR ("Failed to create Opus decoder for session(%d) with error code=%d.", session_id, opus_err);
        goto bail;
    }
    dec_mgr->last_seq = -1;
    dec_mgr->last_pkt_ts = tsk_gettimeofday_ms();
   
	insertRet = opus_codec->decoder.dec_map->insert(opus_decoder_manager_map_t::value_type(session_id, dec_mgr));
    if (!insertRet.second) {
        goto bail;
    }
    
    return 0;
bail:
    if (NULL != dec_mgr) {
        if (NULL != dec_mgr->inst) {
            opus_decoder_destroy(dec_mgr->inst);
            dec_mgr->inst = tsk_null;
        }
        delete dec_mgr;
        dec_mgr = NULL;
    }
    return -1;
}


static tsk_size_t tdav_codec_opus_decode (tmedia_codec_t *self, const void *in_data, tsk_size_t in_size, void **out_data, tsk_size_t *out_max_size, tsk_object_t *proto_hdr)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)self;
    int frame_size;
    trtp_rtp_header_t *rtp_hdr = (trtp_rtp_header_t *)proto_hdr;
    int32_t rtp_seq_no_diff = 0;

    if (!self || !in_data || !in_size || !out_data)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
	opus_decoder_manager_map_t &dec_map = *(opus->decoder.dec_map);
    opus_decoder_manager_map_t::iterator it = dec_map.find(rtp_hdr->session_id);
    if (it == dec_map.end()) {
        if (tdav_codec_opus_creat_new_decoder(opus, rtp_hdr->session_id) != 0) {
            TSK_DEBUG_ERROR ("[OPUS] Failed to create opus decoder for session(%d)", rtp_hdr->session_id);
            return 0;
        }
        it = dec_map.find(rtp_hdr->session_id);
        if (it == dec_map.end()) {
            TSK_DEBUG_ERROR ("impossible");
            return 0;
        }
        TSK_DEBUG_INFO ("[OPUS] Successfully created opus decoder for session(%d)", rtp_hdr->session_id);
    }
    opus_decoder_manager_t* dec_mgr = it->second;

    if (!dec_mgr->inst)
    {
        TSK_DEBUG_ERROR ("Decoder not ready");
        return 0;
    }
    
    if (dec_mgr->last_seq == rtp_hdr->seq_num)
    {
        TSK_DEBUG_INFO ("[Opus] Packet duplicated, seq_num=%d", rtp_hdr->seq_num);
        return 0;
    }
    
    rtp_seq_no_diff = DIFF_RTP_SEQ_NO(dec_mgr->last_seq, rtp_hdr->seq_num);
    
    // If there's packet loss, try to do FEC/PLC
    if(opus->decoder.fec_enabled && (dec_mgr->last_seq >= 0) && (rtp_seq_no_diff > 1)) {
        int req_frame_size = TMEDIA_CODEC_PCM_FRAME_SIZE_AUDIO_DECODING(opus);
        int LBRRFlag = 0;
        frame_size = opus_decode(dec_mgr->inst, (const unsigned char *)in_data, (opus_int32)in_size, dec_mgr->buff, req_frame_size, 1);
        dec_mgr->last_seq++;
        rtp_hdr->seq_num = dec_mgr->last_seq;
        if (opus_decoder_ctl(dec_mgr->inst, OPUS_GET_LBRR_FLAG(&LBRRFlag)) != OPUS_OK) {
            LBRRFlag = 0;
        }
        if (LBRRFlag) {
            RTP_PACKET_FLAG_SET(rtp_hdr->flag, RTP_PACKET_FLAG_FEC);
        } else {
            RTP_PACKET_FLAG_SET(rtp_hdr->flag, RTP_PACKET_FLAG_PLC);
        }
        //TSK_DEBUG_INFO("[Opus] Packet loss, seq_num=%d, fec:%d, plc:%d",
        //               opus->decoder.last_seq,
        //               RTP_PACKET_FLAG_CONTAINS(rtp_hdr->flag, RTP_PACKET_FLAG_FEC) ? 1 : 0,
        //               RTP_PACKET_FLAG_CONTAINS(rtp_hdr->flag, RTP_PACKET_FLAG_PLC) ? 1 : 0);
    } else {
        frame_size = opus_decode (dec_mgr->inst, (const unsigned char *)in_data, (opus_int32)in_size, dec_mgr->buff, TDAV_OPUS_MAX_FRAME_SIZE_IN_SAMPLES, 0);
        if ((dec_mgr->last_seq < 0) || (rtp_seq_no_diff > 0)) {
            dec_mgr->last_seq = rtp_hdr->seq_num;
        }
    }
    
    if (frame_size > 0)
    {
        tsk_size_t frame_size_inbytes = (frame_size << 1);
        if (*out_max_size < frame_size_inbytes)
        {
            if (!(*out_data = tsk_realloc (*out_data, frame_size_inbytes)))
            {
                TSK_DEBUG_ERROR ("Failed to allocate new buffer");
                *out_max_size = 0;
                return 0;
            }
            *out_max_size = frame_size_inbytes;
        }
        memcpy (*out_data, dec_mgr->buff, frame_size_inbytes);
        return frame_size_inbytes;
    }
    else
    {
        TSK_DEBUG_INFO ("Failed to opus_decode,ret=%d", frame_size);
        return 0;
    }
}

static tsk_bool_t tdav_codec_opus_sdp_att_match (const tmedia_codec_t *codec, const char *att_name, const char *att_value)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)codec;

    if (!opus)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }

    TSK_DEBUG_INFO ("[OPUS] Trying to match [%s:%s]", att_name, att_value);

    if (tsk_striequals (att_name, "fmtp"))
    {
        int val_int;
        tsk_params_L_t *params;
        /* e.g. FIXME */
        if ((params = tsk_params_fromstring (att_value, ";", tsk_true)))
        {
            tsk_bool_t ret = tsk_false;
            /* === maxplaybackrate ===*/
            if ((val_int = tsk_params_get_param_value_as_int (params, "maxplaybackrate")) != -1)
            {
                if (!_tdav_codec_opus_rate_is_valid (val_int))
                {
                    TSK_DEBUG_ERROR ("[OPUS] %d not valid as maxplaybackrate value", val_int);
                    goto done;
                }
                //TMEDIA_CODEC (opus)->out.rate = TSK_MIN ((int32_t)TMEDIA_CODEC (opus)->out.rate, val_int);
                //TMEDIA_CODEC_AUDIO (opus)->out.timestamp_multiplier = tmedia_codec_audio_get_timestamp_multiplier (codec->id, codec->out.rate);
            }
            /* === sprop-maxcapturerate ===*/
            if ((val_int = tsk_params_get_param_value_as_int (params, "sprop-maxcapturerate")) != -1)
            {
                if (!_tdav_codec_opus_rate_is_valid (val_int))
                {
                    TSK_DEBUG_ERROR ("[OPUS] %d not valid as sprop-maxcapturerate value", val_int);
                    goto done;
                }
                //TMEDIA_CODEC (opus)->in.rate = TSK_MIN ((int32_t)TMEDIA_CODEC (opus)->in.rate, val_int);
                //TMEDIA_CODEC_AUDIO (opus)->in.timestamp_multiplier = tmedia_codec_audio_get_timestamp_multiplier (codec->id, codec->in.rate);
            }
            ret = tsk_true;
        done:
            TSK_OBJECT_SAFE_FREE (params);
            return ret;
        }
    }

    return tsk_true;
}

static char *tdav_codec_opus_sdp_att_get (const tmedia_codec_t *codec, const char *att_name)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)codec;

    if (!opus)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    if (tsk_striequals (att_name, "fmtp"))
    {
        char *fmtp = tsk_null;
        tsk_sprintf (&fmtp, "maxplaybackrate=%d; sprop-maxcapturerate=%d; stereo=%d; "
                            "sprop-stereo=%d; useinbandfec=%d; usedtx=%d",
                     TMEDIA_CODEC (opus)->in.rate, TMEDIA_CODEC (opus)->out.rate, (TMEDIA_CODEC_AUDIO (opus)->in.channels == 2) ? 1 : 0, (TMEDIA_CODEC_AUDIO (opus)->out.channels == 2) ? 1 : 0,
                     opus->decoder.fec_enabled ? 1 : 0, opus->decoder.dtx_enabled ? 1 : 0);
        return fmtp;
    }

    return tsk_null;
}

//
//	OPUS Plugin definition
//

/* constructor */
static tsk_object_t *tdav_codec_opus_ctor (tsk_object_t *self, va_list *app)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)self;
    if (opus)
    {
        /* init base: called by tmedia_codec_create() */
        /* init self */
        TMEDIA_CODEC (opus)->in.rate = tmedia_defaults_get_record_sample_rate();
        TMEDIA_CODEC (opus)->out.rate = tmedia_defaults_get_record_sample_rate();
        TMEDIA_CODEC_AUDIO (opus)->in.channels = 1;
        TMEDIA_CODEC_AUDIO (opus)->out.channels = 1;
		opus->decoder.dec_map = new opus_decoder_manager_map_t;
		opus->decoder.dec_map->clear();
        opus->decoder.fec_enabled = tmedia_defaults_get_opus_inband_fec_enabled();
        opus->decoder.dtx_enabled = tmedia_defaults_get_opus_dtx_peroid() > 0;
        opus->encoder.fec_enabled = tmedia_defaults_get_opus_inband_fec_enabled();
        opus->encoder.dtx_enabled = tmedia_defaults_get_opus_dtx_peroid() > 0;
    }
    return self;
}
/* destructor */
static tsk_object_t *tdav_codec_opus_dtor (tsk_object_t *self)
{
    tdav_codec_opus_t *opus = (tdav_codec_opus_t *)self;
    if (opus)
    {
        /* deinit base */
        tmedia_codec_audio_deinit (opus);
        /* deinit self */
        opus_decoder_manager_map_t::iterator it;
		for (it = opus->decoder.dec_map->begin(); it != opus->decoder.dec_map->end(); it++) {
            opus_decoder_manager_t *dec_mgr = it->second;
            if (dec_mgr) {
                if (dec_mgr->inst) {
                    opus_decoder_destroy(dec_mgr->inst);
                    dec_mgr->inst = tsk_null;
                }
                delete dec_mgr;
                dec_mgr = NULL;
            }
        }
		opus->decoder.dec_map->clear();
		delete opus->decoder.dec_map;
        if (opus->encoder.inst)
        {
            opus_encoder_destroy (opus->encoder.inst), opus->encoder.inst = tsk_null;
        }
    }

    return self;
}
/* object definition */
static const tsk_object_def_t tdav_codec_opus_def_s = {
    sizeof (tdav_codec_opus_t),
    tdav_codec_opus_ctor,
    tdav_codec_opus_dtor,
    tmedia_codec_cmp,
};

/* plugin definition*/
static const tmedia_codec_plugin_def_t tdav_codec_opus_plugin_def_s = { &tdav_codec_opus_def_s,

                                                                        tmedia_audio,
                                                                        tmedia_codec_id_opus,
                                                                        "opus",
                                                                        "opus Codec",
                                                                        TMEDIA_CODEC_FORMAT_OPUS,
                                                                        tsk_true,
                                                                        48000, // this is the default sample rate

                                                                        {
                                                                        /* audio */
                                                                        2, // channels
                                                                        0  // ptime @deprecated
                                                                        },
    
                                                                        /* video */
                                                                        {0},

                                                                        tdav_codec_opus_set, // set()
                                                                        tdav_codec_opus_open,
                                                                        tdav_codec_opus_close,
                                                                        tsk_null,
                                                                        tsk_null,
                                                                        tdav_codec_opus_encode,
                                                                        tdav_codec_opus_decode,
                                                                        tdav_codec_opus_sdp_att_match,
                                                                        tdav_codec_opus_sdp_att_get };
const tmedia_codec_plugin_def_t *tdav_codec_opus_plugin_def_t = &tdav_codec_opus_plugin_def_s;


#endif /* HAVE_LIBOPUS */

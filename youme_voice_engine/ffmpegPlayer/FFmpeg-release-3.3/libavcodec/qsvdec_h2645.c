﻿/*
 * Intel MediaSDK QSV based H.264 / HEVC decoder
 *
 * copyright (c) 2013 Luca Barbato
 * copyright (c) 2015 Anton Khirnov
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include <stdint.h>
#include <string.h>

#include <mfx/mfxvideo.h>

#include "libavutil/common.h"
#include "libavutil/fifo.h"
#include "libavutil/opt.h"

#include "avcodec.h"
#include "internal.h"
#include "qsv_internal.h"
#include "qsvdec.h"
#include "qsv.h"

enum LoadPlugin {
    LOAD_PLUGIN_NONE,
    LOAD_PLUGIN_HEVC_SW,
    LOAD_PLUGIN_HEVC_HW,
};

typedef struct QSVH2645Context {
    AVClass *class;
    QSVContext qsv;

    int load_plugin;

    // the filter for converting to Annex B
    AVBSFContext *bsf;

    AVFifoBuffer *packet_fifo;

    AVPacket pkt_filtered;
} QSVH2645Context;

static void qsv_clear_buffers(QSVH2645Context *s)
{
    AVPacket pkt;
    while (av_fifo_size(s->packet_fifo) >= sizeof(pkt)) {
        av_fifo_generic_read(s->packet_fifo, &pkt, sizeof(pkt), NULL);
        av_packet_unref(&pkt);
    }

    av_bsf_free(&s->bsf);

    av_packet_unref(&s->pkt_filtered);
}

static av_cold int qsv_decode_close(AVCodecContext *avctx)
{
    QSVH2645Context *s = avctx->priv_data;

    ff_qsv_decode_close(&s->qsv);

    qsv_clear_buffers(s);

    av_fifo_free(s->packet_fifo);

    return 0;
}

static av_cold int qsv_decode_init(AVCodecContext *avctx)
{
    QSVH2645Context *s = avctx->priv_data;
    int ret;

    if (avctx->codec_id == AV_CODEC_ID_HEVC && s->load_plugin != LOAD_PLUGIN_NONE) {
        static const char *uid_hevcdec_sw = "15dd936825ad475ea34e35f3f54217a6";
        static const char *uid_hevcdec_hw = "33a61c0b4c27454ca8d85dde757c6f8e";

        if (s->qsv.load_plugins[0]) {
            av_log(avctx, AV_LOG_WARNING,
                   "load_plugins is not empty, but load_plugin is not set to 'none'."
                   "The load_plugin value will be ignored.\n");
        } else {
            av_freep(&s->qsv.load_plugins);

            if (s->load_plugin == LOAD_PLUGIN_HEVC_SW)
                s->qsv.load_plugins = av_strdup(uid_hevcdec_sw);
            else
                s->qsv.load_plugins = av_strdup(uid_hevcdec_hw);
            if (!s->qsv.load_plugins)
                return AVERROR(ENOMEM);
        }
    }

    s->packet_fifo = av_fifo_alloc(sizeof(AVPacket));
    if (!s->packet_fifo) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    return 0;
fail:
    qsv_decode_close(avctx);
    return ret;
}

static int qsv_init_bsf(AVCodecContext *avctx, QSVH2645Context *s)
{
    const char *filter_name = avctx->codec_id == AV_CODEC_ID_HEVC ?
                              "hevc_mp4toannexb" : "h264_mp4toannexb";
    const AVBitStreamFilter *filter;
    int ret;

    if (s->bsf)
        return 0;

    filter = av_bsf_get_by_name(filter_name);
    if (!filter)
        return AVERROR_BUG;

    ret = av_bsf_alloc(filter, &s->bsf);
    if (ret < 0)
        return ret;

    ret = avcodec_parameters_from_context(s->bsf->par_in, avctx);
    if (ret < 0)
        return ret;

    s->bsf->time_base_in = avctx->time_base;

    ret = av_bsf_init(s->bsf);
    if (ret < 0)
        return ret;

    return ret;
}

static int qsv_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame, AVPacket *avpkt)
{
    QSVH2645Context *s = avctx->priv_data;
    AVFrame *frame    = data;
    int ret;

    /* make sure the bitstream filter is initialized */
    ret = qsv_init_bsf(avctx, s);
    if (ret < 0)
        return ret;

    /* buffer the input packet */
    if (avpkt->size) {
        AVPacket input_ref = { 0 };

        if (av_fifo_space(s->packet_fifo) < sizeof(input_ref)) {
            ret = av_fifo_realloc2(s->packet_fifo,
                                   av_fifo_size(s->packet_fifo) + sizeof(input_ref));
            if (ret < 0)
                return ret;
        }

        ret = av_packet_ref(&input_ref, avpkt);
        if (ret < 0)
            return ret;
        av_fifo_generic_write(s->packet_fifo, &input_ref, sizeof(input_ref), NULL);
    }

    /* process buffered data */
    while (!*got_frame) {
        /* prepare the input data -- convert to Annex B if needed */
        if (s->pkt_filtered.size <= 0) {
            AVPacket input_ref;

            /* no more data */
            if (av_fifo_size(s->packet_fifo) < sizeof(AVPacket))
                return avpkt->size ? avpkt->size : ff_qsv_process_data(avctx, &s->qsv, frame, got_frame, avpkt);

            av_packet_unref(&s->pkt_filtered);

            av_fifo_generic_read(s->packet_fifo, &input_ref, sizeof(input_ref), NULL);
            ret = av_bsf_send_packet(s->bsf, &input_ref);
            if (ret < 0) {
                av_packet_unref(&input_ref);
                return ret;
            }

            ret = av_bsf_receive_packet(s->bsf, &s->pkt_filtered);
            if (ret < 0)
                av_packet_move_ref(&s->pkt_filtered, &input_ref);
            else
                av_packet_unref(&input_ref);
        }

        ret = ff_qsv_process_data(avctx, &s->qsv, frame, got_frame, &s->pkt_filtered);
        if (ret < 0)
            return ret;

        s->pkt_filtered.size -= ret;
        s->pkt_filtered.data += ret;
    }

    return avpkt->size;
}

static void qsv_decode_flush(AVCodecContext *avctx)
{
    QSVH2645Context *s = avctx->priv_data;

    qsv_clear_buffers(s);
    ff_qsv_decode_flush(avctx, &s->qsv);
}

#define OFFSET(x) offsetof(QSVH2645Context, x)
#define VD AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM

#if CONFIG_HEVC_QSV_DECODER
AVHWAccel ff_hevc_qsv_hwaccel = {
    .name           = "hevc_qsv",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_HEVC,
    .pix_fmt        = AV_PIX_FMT_QSV,
};

static const AVOption hevc_options[] = {
    { "async_depth", "Internal parallelization depth, the higher the value the higher the latency.", OFFSET(qsv.async_depth), AV_OPT_TYPE_INT, { .i64 = ASYNC_DEPTH_DEFAULT }, 0, INT_MAX, VD },

    { "load_plugin", "A user plugin to load in an internal session", OFFSET(load_plugin), AV_OPT_TYPE_INT, { .i64 = LOAD_PLUGIN_HEVC_SW }, LOAD_PLUGIN_NONE, LOAD_PLUGIN_HEVC_HW, VD, "load_plugin" },
    { "none",     NULL, 0, AV_OPT_TYPE_CONST, { .i64 = LOAD_PLUGIN_NONE },    0, 0, VD, "load_plugin" },
    { "hevc_sw",  NULL, 0, AV_OPT_TYPE_CONST, { .i64 = LOAD_PLUGIN_HEVC_SW }, 0, 0, VD, "load_plugin" },
    { "hevc_hw",  NULL, 0, AV_OPT_TYPE_CONST, { .i64 = LOAD_PLUGIN_HEVC_HW }, 0, 0, VD, "load_plugin" },

    { "load_plugins", "A :-separate list of hexadecimal plugin UIDs to load in an internal session",
        OFFSET(qsv.load_plugins), AV_OPT_TYPE_STRING, { .str = "" }, 0, 0, VD },
    { NULL },
};

static const AVClass hevc_class = {
    .class_name = "hevc_qsv",
    .item_name  = av_default_item_name,
    .option     = hevc_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_hevc_qsv_decoder = {
    .name           = "hevc_qsv",
    .long_name      = NULL_IF_CONFIG_SMALL("HEVC (Intel Quick Sync Video acceleration)"),
    .priv_data_size = sizeof(QSVH2645Context),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_HEVC,
    .init           = qsv_decode_init,
    .decode         = qsv_decode_frame,
    .flush          = qsv_decode_flush,
    .close          = qsv_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1 | AV_CODEC_CAP_AVOID_PROBING,
    .priv_class     = &hevc_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_P010,
                                                    AV_PIX_FMT_QSV,
                                                    AV_PIX_FMT_NONE },
};
#endif

#if CONFIG_H264_QSV_DECODER
AVHWAccel ff_h264_qsv_hwaccel = {
    .name           = "h264_qsv",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .pix_fmt        = AV_PIX_FMT_QSV,
};

static const AVOption options[] = {
    { "async_depth", "Internal parallelization depth, the higher the value the higher the latency.", OFFSET(qsv.async_depth), AV_OPT_TYPE_INT, { .i64 = ASYNC_DEPTH_DEFAULT }, 0, INT_MAX, VD },
    { NULL },
};

static const AVClass class = {
    .class_name = "h264_qsv",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_h264_qsv_decoder = {
    .name           = "h264_qsv",
    .long_name      = NULL_IF_CONFIG_SMALL("H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (Intel Quick Sync Video acceleration)"),
    .priv_data_size = sizeof(QSVH2645Context),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_H264,
    .init           = qsv_decode_init,
    .decode         = qsv_decode_frame,
    .flush          = qsv_decode_flush,
    .close          = qsv_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1 | AV_CODEC_CAP_AVOID_PROBING,
    .priv_class     = &class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_P010,
                                                    AV_PIX_FMT_QSV,
                                                    AV_PIX_FMT_NONE },
};
#endif

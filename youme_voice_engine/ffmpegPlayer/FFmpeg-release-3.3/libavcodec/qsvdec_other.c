﻿/*
 * Intel MediaSDK QSV based MPEG-2, VC-1 and VP8 decoders
 *
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

typedef struct QSVOtherContext {
    AVClass *class;
    QSVContext qsv;

    AVFifoBuffer *packet_fifo;

    AVPacket input_ref;
} QSVOtherContext;

static void qsv_clear_buffers(QSVOtherContext *s)
{
    AVPacket pkt;
    while (av_fifo_size(s->packet_fifo) >= sizeof(pkt)) {
        av_fifo_generic_read(s->packet_fifo, &pkt, sizeof(pkt), NULL);
        av_packet_unref(&pkt);
    }

    av_packet_unref(&s->input_ref);
}

static av_cold int qsv_decode_close(AVCodecContext *avctx)
{
    QSVOtherContext *s = avctx->priv_data;

    ff_qsv_decode_close(&s->qsv);

    qsv_clear_buffers(s);

    av_fifo_free(s->packet_fifo);

    return 0;
}

static av_cold int qsv_decode_init(AVCodecContext *avctx)
{
    QSVOtherContext *s = avctx->priv_data;
    int ret;

#if CONFIG_VP8_QSV_DECODER
    if (avctx->codec_id == AV_CODEC_ID_VP8) {
        static const char *uid_vp8dec_hw = "f622394d8d87452f878c51f2fc9b4131";

        av_freep(&s->qsv.load_plugins);
        s->qsv.load_plugins = av_strdup(uid_vp8dec_hw);
        if (!s->qsv.load_plugins)
            return AVERROR(ENOMEM);
    }
#endif

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

static int qsv_decode_frame(AVCodecContext *avctx, void *data,
                            int *got_frame, AVPacket *avpkt)
{
    QSVOtherContext *s = avctx->priv_data;
    AVFrame *frame    = data;
    int ret;

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
        if (s->input_ref.size <= 0) {
            /* no more data */
            if (av_fifo_size(s->packet_fifo) < sizeof(AVPacket))
                return avpkt->size ? avpkt->size : ff_qsv_process_data(avctx, &s->qsv, frame, got_frame, avpkt);

            av_packet_unref(&s->input_ref);
            av_fifo_generic_read(s->packet_fifo, &s->input_ref, sizeof(s->input_ref), NULL);
        }

        ret = ff_qsv_process_data(avctx, &s->qsv, frame, got_frame, &s->input_ref);
        if (ret < 0)
            return ret;

        s->input_ref.size -= ret;
        s->input_ref.data += ret;
    }

    return avpkt->size;
}

static void qsv_decode_flush(AVCodecContext *avctx)
{
    QSVOtherContext *s = avctx->priv_data;

    qsv_clear_buffers(s);
    ff_qsv_decode_flush(avctx, &s->qsv);
}

#define OFFSET(x) offsetof(QSVOtherContext, x)
#define VD AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    { "async_depth", "Internal parallelization depth, the higher the value the higher the latency.", OFFSET(qsv.async_depth), AV_OPT_TYPE_INT, { .i64 = ASYNC_DEPTH_DEFAULT }, 0, INT_MAX, VD },
    { NULL },
};

#if CONFIG_MPEG2_QSV_HWACCEL
AVHWAccel ff_mpeg2_qsv_hwaccel = {
    .name           = "mpeg2_qsv",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .pix_fmt        = AV_PIX_FMT_QSV,
};
#endif

#if CONFIG_MPEG2_QSV_DECODER
static const AVClass mpeg2_qsv_class = {
    .class_name = "mpeg2_qsv",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_mpeg2_qsv_decoder = {
    .name           = "mpeg2_qsv",
    .long_name      = NULL_IF_CONFIG_SMALL("MPEG-2 video (Intel Quick Sync Video acceleration)"),
    .priv_data_size = sizeof(QSVOtherContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPEG2VIDEO,
    .init           = qsv_decode_init,
    .decode         = qsv_decode_frame,
    .flush          = qsv_decode_flush,
    .close          = qsv_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1 | AV_CODEC_CAP_AVOID_PROBING,
    .priv_class     = &mpeg2_qsv_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_QSV,
                                                    AV_PIX_FMT_NONE },
};
#endif

#if CONFIG_VC1_QSV_HWACCEL
AVHWAccel ff_vc1_qsv_hwaccel = {
    .name           = "vc1_qsv",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_VC1,
    .pix_fmt        = AV_PIX_FMT_QSV,
};
#endif

#if CONFIG_VC1_QSV_DECODER
static const AVClass vc1_qsv_class = {
    .class_name = "vc1_qsv",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_vc1_qsv_decoder = {
    .name           = "vc1_qsv",
    .long_name      = NULL_IF_CONFIG_SMALL("VC-1 video (Intel Quick Sync Video acceleration)"),
    .priv_data_size = sizeof(QSVOtherContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_VC1,
    .init           = qsv_decode_init,
    .decode         = qsv_decode_frame,
    .flush          = qsv_decode_flush,
    .close          = qsv_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1 | AV_CODEC_CAP_AVOID_PROBING,
    .priv_class     = &vc1_qsv_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_QSV,
                                                    AV_PIX_FMT_NONE },
};
#endif

#if CONFIG_VP8_QSV_HWACCEL
AVHWAccel ff_vp8_qsv_hwaccel = {
    .name           = "vp8_qsv",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_VP8,
    .pix_fmt        = AV_PIX_FMT_QSV,
};
#endif

#if CONFIG_VP8_QSV_DECODER
static const AVClass vp8_qsv_class = {
    .class_name = "vp8_qsv",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_vp8_qsv_decoder = {
    .name           = "vp8_qsv",
    .long_name      = NULL_IF_CONFIG_SMALL("VP8 video (Intel Quick Sync Video acceleration)"),
    .priv_data_size = sizeof(QSVOtherContext),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_VP8,
    .init           = qsv_decode_init,
    .decode         = qsv_decode_frame,
    .flush          = qsv_decode_flush,
    .close          = qsv_decode_close,
    .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_DR1 | AV_CODEC_CAP_AVOID_PROBING,
    .priv_class     = &vp8_qsv_class,
    .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_NV12,
                                                    AV_PIX_FMT_QSV,
                                                    AV_PIX_FMT_NONE },
};
#endif

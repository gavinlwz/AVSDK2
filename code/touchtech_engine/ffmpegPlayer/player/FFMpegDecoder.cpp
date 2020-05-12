//
//  FFMpegAudioDecoder.cpp
//  youme_voice_engine
//
//  Created by peter on 8/13/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "version.h"
#if FFMPEG_SUPPORT

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "FFMpegDecoder.h"

static char gLogBuf[2048] = "";
static void _log_delegate(void *context, int level, const char *fmt, va_list ap)
{
    int ret = vsnprintf(gLogBuf, sizeof(gLogBuf), fmt, ap);
    if (ret > 0) {
        TSK_DEBUG_INFO("%s", gLogBuf);
    }
}

static int _interrupt_func(void *context)
{
//    if (static_cast<CFFmpegDemuxer *>(pContext)->isAboutQuit()) {
//        return 1;
//    } else {
//        return 0;
//    }
    return 0;
}

static void _msg_delegate(int nMsgType, int nSubType, const char* pucMsgBuf, int nBufSize, void *pContext)
{
//    if (pContext) {
//        static_cast<CFFmpegDemuxer *>(pContext)->notifyMsg(nMsgType, nSubType, pucMsgBuf, nBufSize);
//    }
}

CFFMpegDecoder::CFFMpegDecoder() :
  m_pFmtContext(NULL)
, m_pCodecContext(NULL)
, m_pAvFrame(NULL)
, m_nStreamIndex(0)
, m_state(DecoderClosed)
{
    
}

CFFMpegDecoder::~CFFMpegDecoder()
{
    if (DecoderOpened == m_state) {
        TSK_DEBUG_WARN("The decoder is still opened, try to close it");
        close();
    }
}


bool CFFMpegDecoder::open(const char* pFilePath)
{
    int ret = 0;
    
    m_state = DecoderOpening;
    
    // register all formats and codecs
    av_register_all();
    
    //avformat_network_init();
    
   av_log_set_callback(_log_delegate);
   av_log_set_level(99);
    
    m_pFmtContext = avformat_alloc_context();
    if (NULL == m_pFmtContext) {
        TSK_DEBUG_ERROR("Failed to avformat_alloc_context");
        goto bail;
    }
    TSK_DEBUG_INFO("avformat_alloc_context OK");
    
    m_pFmtContext->interrupt_callback.callback	= _interrupt_func;
    m_pFmtContext->interrupt_callback.opaque	= this;
    //av_msg_set_callback(_msg_delegate);
    
    // Open input file, and allocate format context
    ret = avformat_open_input(&m_pFmtContext, pFilePath, NULL, NULL);
    if (0 != ret) {
        TSK_DEBUG_ERROR("avformat_open_input failed, ret:%d", ret);
        goto bail;
    }
    
    // retrieve stream information
    ret = avformat_find_stream_info(m_pFmtContext, NULL);
    if (0 != ret) {
        TSK_DEBUG_ERROR("avformat_find_stream_info failed, ret:%d", ret);
        goto bail;
    }
    
    m_pCodecContext = avcodec_alloc_context3(NULL);
    if (NULL == m_pCodecContext) {
        TSK_DEBUG_ERROR("avcodec_alloc_context3 failed, ret:%d", ret);
        goto bail;
    }

    // find the first audio stream and create a AVCodecContext for it
    for (int i = 0; i < m_pFmtContext->nb_streams; i++) {
        AVCodecContext* pCodecContext = m_pFmtContext->streams[i]->codec;
        m_nStreamIndex = i;
        if ((AVMEDIA_TYPE_AUDIO == pCodecContext->codec_type) || (AVMEDIA_TYPE_VIDEO == pCodecContext->codec_type)) { // A/V case
            TSK_DEBUG_INFO("AVMediaType is AUDIO or VIDEO!");
            AVCodec *pCodec = NULL;
            ret = avcodec_copy_context(m_pCodecContext, pCodecContext);
            if (0 != ret) {
                TSK_DEBUG_ERROR("avcodec_copy_context failed");
                goto bail;
            }
            pCodec = avcodec_find_decoder(m_pCodecContext->codec_id);
            if (NULL == pCodec) {
                TSK_DEBUG_ERROR("avcodec_find_decoder failed");
                goto bail;
            }
            ret = avcodec_open2(m_pCodecContext, pCodec, NULL);
            if (0 != ret) {
                TSK_DEBUG_ERROR("avcodec_open2 failed, ret:%d", ret);
                goto bail;
            }
            break;
        } else {
            TSK_DEBUG_INFO("Ignore AVMediaType:%d", pCodecContext->codec_type);
        }
    }

    m_state = DecoderOpened;
    TSK_DEBUG_INFO("Successfully open the file:%s", pFilePath);
    return true;
bail:
    
    m_state = DecoderClosed;
    return false;
}

void CFFMpegDecoder::close()
{
    if (DecoderOpened != m_state) {
        TSK_DEBUG_WARN("Ffmpeg decoder was not opened yet");
        return;
    }
    
    m_state = DecoderClosing;

    if (m_pCodecContext) {
        avcodec_close(m_pCodecContext);
        av_free(m_pCodecContext);
        m_pCodecContext = NULL;
    }
    
    if (m_pFmtContext) {
        avformat_close_input(&m_pFmtContext);
    }
    
    if (m_pAvFrame) {
        av_frame_free(&m_pAvFrame);
    }

    m_state = DecoderClosed;
    TSK_DEBUG_INFO("CFFMpegDecoder closed");
}

// return the actual size of the output data
// return 0 if the current packet is not the desired one, the caller can try the next frame.
// return -1 if reaches the end of file or there's any error occured.
int32_t CFFMpegDecoder::getNextFrame(void** ppOutBuf, uint32_t *pMaxSize, AudioMeta_t *pAudioMeta, VideoMeta_s *pVideoMeta)
{
    int ret = 0, i;
    AVPacket avPacket;
    int got_frame = 0;
    uint32_t outSize = 0;
    
    if (!ppOutBuf || !pMaxSize) {
        TSK_DEBUG_ERROR("Illegal parameters");
        return -1;
    }

    if ((DecoderOpened != m_state) || !m_pFmtContext || !m_pCodecContext) {
        TSK_DEBUG_ERROR("Illegal state");
        return -1;
    }
    
    av_init_packet(&avPacket);
    ret = av_read_frame(m_pFmtContext, &avPacket);
    if (0 !=  ret) {
        TSK_DEBUG_INFO("av_read_frame: no more data");
        ret = -2;
        goto bail;
    }
    if (avPacket.stream_index != m_nStreamIndex) {
        ret = 0;
        goto bail;
    }

    if (!m_pAvFrame) {
        m_pAvFrame = av_frame_alloc();
    }
    
    if (!m_pAvFrame) {
        TSK_DEBUG_ERROR("failed to allocate AVFrame");
        ret = -1;
        goto bail;
    }
    
    if (AVMEDIA_TYPE_AUDIO == m_pCodecContext->codec_type) {
        ret = avcodec_decode_audio4(m_pCodecContext, m_pAvFrame, &got_frame, &avPacket);
        if ((ret < 0) || !got_frame) {
            TSK_DEBUG_ERROR("failed to decoder audio packet, ret:%d", ret);
        
            if (AVERROR_INVALIDDATA == ret) {
                ret = -2;
            } else {
                ret = -1;
            }
            goto bail;
        }
    
        pAudioMeta->channels       = m_pAvFrame->channels;
        pAudioMeta->sampleRate     = m_pAvFrame->sample_rate;
        // check for supported format
        if ( ((AV_SAMPLE_FMT_S16 != m_pAvFrame->format) && (AV_SAMPLE_FMT_S16P != m_pAvFrame->format))
            || (m_pAvFrame->channels > 2)) {
            TSK_DEBUG_ERROR("Unsupported audio format:%d, channels:%d",
                        m_pAvFrame->format,
                        m_pAvFrame->channels);
            ret = -1;
            goto bail;
        }
    
        if (AV_SAMPLE_FMT_S16 == m_pAvFrame->format) {
            outSize = m_pAvFrame->linesize[0];
        } else if (AV_SAMPLE_FMT_S16P == m_pAvFrame->format) {
            outSize = m_pAvFrame->linesize[0] * m_pAvFrame->channels;
        }

        // Allocate or enlarge the output buffer if necessary
        if ((NULL == *ppOutBuf) || (outSize > *pMaxSize)) {
            void *pNewBuf = tsk_realloc(*ppOutBuf, outSize);
            if (NULL == pNewBuf) {
                TSK_DEBUG_ERROR ("Failed to realloc size:%u", outSize);
                ret = -1;
                goto bail;
            }
            *ppOutBuf = pNewBuf;
            *pMaxSize = outSize;
        }
    
        // Copy the audio data to the output buffer
        if (AV_SAMPLE_FMT_S16 == m_pAvFrame->format) {
            memcpy(*ppOutBuf, m_pAvFrame->extended_data[0], outSize);
            pAudioMeta->isInterleaved = 1;
        } else if (AV_SAMPLE_FMT_S16P == m_pAvFrame->format) {
            for (int i = 0; i < m_pAvFrame->channels; i++) {
                memcpy((uint8_t*)(*ppOutBuf) + (i * m_pAvFrame->linesize[0]), m_pAvFrame->extended_data[i], m_pAvFrame->linesize[0]);
            }
            pAudioMeta->isInterleaved = 0;
        }
    } else if (AVMEDIA_TYPE_VIDEO == m_pCodecContext->codec_type) {
        int widthUV, heightUV;
        int index = 0;
        
        if (AV_CODEC_ID_MJPEG == m_pCodecContext->codec_id)
            return 0;

        ret = avcodec_decode_video2(m_pCodecContext, m_pAvFrame, &got_frame, &avPacket);
        if (ret < 0) {
            TSK_DEBUG_ERROR("failed to decoder video packet");
            ret = -1;
            goto bail;
        }
        
        int width  = m_pCodecContext->width;
        int height = m_pCodecContext->height;
        pVideoMeta->i4PicWidth  = width;
        pVideoMeta->i4PicHeight = height;
        pVideoMeta->i4PixelFormat = m_pCodecContext->pix_fmt;
        pVideoMeta->i4CodedPicOrder = m_pAvFrame->coded_picture_number;
        pVideoMeta->i4DisplayPicOrder = m_pAvFrame->display_picture_number;
        pVideoMeta->i8Pts = m_pAvFrame->pts;
        if ((AV_PIX_FMT_YUV444P != pVideoMeta->i4PixelFormat) && (AV_PIX_FMT_YUV420P != pVideoMeta->i4PixelFormat)) {
            TSK_DEBUG_ERROR("Unsupported video pixel format:%d", m_pCodecContext->pix_fmt);
            ret = -1;
            goto bail;
        }
        if (AV_PIX_FMT_YUV444P == pVideoMeta->i4PixelFormat) {
            outSize  = width * height * 3;
            widthUV  = width;
            heightUV = height;
        } else if (AV_PIX_FMT_YUV420P == pVideoMeta->i4PixelFormat) {
            outSize  = width * height * 3 / 2;
            widthUV  = width / 2;
            heightUV = height / 2;
        }
        
        // Allocate or enlarge the output buffer if necessary
        if ((NULL == *ppOutBuf) || (outSize > *pMaxSize)) {
            void *pNewBuf = tsk_realloc(*ppOutBuf, outSize);
            if (NULL == pNewBuf) {
                TSK_DEBUG_ERROR ("Failed to realloc size:%u", outSize);
                ret = -1;
                goto bail;
            }
            *ppOutBuf = pNewBuf;
            *pMaxSize = outSize;
        }
        
        // Copy the video data to the output buffer
        if (got_frame) {
            for (i = 0; i < height; i++) { // Y
                memcpy((uint8_t*)(*ppOutBuf) + index, m_pAvFrame->data[0] + i * m_pAvFrame->linesize[0], width);
                index += width;
            }
            for (i = 0; i < heightUV; i++) { // U
                memcpy((uint8_t*)(*ppOutBuf) + index, m_pAvFrame->data[1] + i * m_pAvFrame->linesize[1], widthUV);
                index += widthUV;
            }
            for (i = 0; i < heightUV; i++) { // V
                memcpy((uint8_t*)(*ppOutBuf) + index, m_pAvFrame->data[2] + i * m_pAvFrame->linesize[2], widthUV);
                index += widthUV;
            }
        } else {
            outSize = 0;
        }
    }
    
    av_free_packet(&avPacket);
    return outSize;
    
bail:
    av_free_packet(&avPacket);
    return ret;
}

#endif //FFMPEG_SUPPORT


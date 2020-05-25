//
//  YMVideoRecorder.cpp
//  VideoRecord
//
//  Created by pinky on 2019/5/15.
//  Copyright © 2019 youme. All rights reserved.
//

#if !defined(OS_LINUX)
#include "YMVideoRecorder.hpp"
#include "tsk_debug.h"

#define RECORD_AUDIO 1

namespace  {
    int H264_NAL_TYPE_SPS = 7;
}

YMVideoRecorder::YMVideoRecorder()
{
    m_inputAudioInfo.sampleRate = 16000;
    m_inputAudioInfo.sampleFmt = AV_SAMPLE_FMT_S16;
    m_inputAudioInfo.channel_layout = AV_CH_LAYOUT_MONO;
    m_inputAudioInfo.channels = 1;
    
    m_encodeAudioInfo.sampleRate = 44100;
    m_encodeAudioInfo.sampleFmt = AV_SAMPLE_FMT_S16;
    m_encodeAudioInfo.channel_layout = AV_CH_LAYOUT_MONO;
    m_encodeAudioInfo.channels = 1;
    
    m_encodeBitrate = 32000;
    
    m_c = nullptr;
    m_ofmt_c = nullptr;
    m_audioFrame = nullptr;
    
    m_audioBuffMaxLen = 1024 * sizeof(int16_t) * 5;
    m_audioBuff = new uint8_t[m_audioBuffMaxLen];
    
    m_audioResampleBuff = nullptr;
    m_audioResampleBuffSize = 0;
    audio_resampler = nullptr;
    
    m_sps = nullptr;
    m_sps_size = 0;
    m_pps = nullptr;
    m_pps_size = 0;
   
    reset();
}

void YMVideoRecorder::reset()
{
    m_bRecording = false;
    m_bRecordInited = false;
    m_bRecordInitFailed = false;
    m_timestampStart = 0;
    m_bVideoInfoSetted = false;
    m_bAudioInfoSetted = false;
    
    memset( m_audioBuff,  0,  m_audioBuffMaxLen );
    m_audioBuffCurLen = 0;
    
    if( m_sps )
    {
        delete m_sps;
        m_sps = nullptr;
        m_sps_size = 0;
    }
    
    if( m_pps )
    {
        delete m_pps;
        m_pps = nullptr;
        m_pps_size = 0;
    }
}

YMVideoRecorder::~YMVideoRecorder()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if( m_audioBuff )
    {
        delete [] m_audioBuff;
        m_audioBuff = nullptr;
        m_audioBuffMaxLen = 0;
        m_audioBuffCurLen = 0;
    }
}

void YMVideoRecorder::setVideoInfo( int width, int height, int fps, int birrate, int timebaseden )
{
//    printf("Pinky:setVideoInfo:%d, %d, %d, %d, %d\n", width, height, fps, birrate, timebaseden  );
    std::lock_guard<std::mutex> lock(m_mutex);
    if( m_bVideoInfoSetted )
    {
        return;
    }
    
    m_videoInfo.width = width;
    m_videoInfo.height  = height;
    m_videoInfo.fps = fps;
    m_videoInfo.birrate = birrate;
    m_videoInfo.timebaseden = timebaseden;
    
    m_bVideoInfoSetted = true;
}

void YMVideoRecorder::setInputAudioInfo( int sampleRate )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if( m_bAudioInfoSetted )
    {
        return ;
    }
    
//    printf("Pinky:setInputAudioInfo:%d\n",sampleRate );
    m_inputAudioInfo.sampleRate = sampleRate;

    m_bAudioInfoSetted = true;
}

void YMVideoRecorder::setFilePath( std::string filePath )
{
    m_filePath = filePath;
}

int YMVideoRecorder::startRecord( )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    reset();
    
    m_bRecording = true;
    
    return 0;
}

void YMVideoRecorder::logFFmpegError( std::string opMsg, int err )
{
    char buff[1024] = {0};
    av_strerror(err, buff, 1024);
    TSK_DEBUG_INFO("%s err:%s", opMsg.c_str(), buff );
}
 
int YMVideoRecorder::innerInit()
{
    if( m_bRecordInited )
    {
        return 0;
    }
    
    if( m_bRecordInitFailed )
    {
        return 0;
    }
    
    av_register_all();
    avcodec_register_all();
    
    int ret = 0;
    
    //4(version+avc profile+avc compatibility+avc level) + 1(naluLengthSizeMinusOne) + 1(number of sps nalu) + 2(sps size) + sps data + 1(number of pps nalu) + 2(pps size) + pps data
    int avcc_length = 4 + 1 + 1 + 2 + (m_sps_size-4) + 1 + 2 + (m_pps_size-4);
    uint8_t *avcc = new uint8_t[avcc_length];
    int offset = 0;
    
    //初始化编码器
#if RECORD_AUDIO
    AVCodec* codec = avcodec_find_encoder( AV_CODEC_ID_AAC );
    if( !codec )
    {
        TSK_DEBUG_INFO("avcodec_find_encoder error");
        goto failed;
    }
    
    m_c =  avcodec_alloc_context3( codec );
    if( !m_c )
    {
        TSK_DEBUG_INFO("avcodec_alloc_context3 error");
        goto failed;
    }
    
    m_c->bit_rate = m_encodeBitrate;
    m_c->sample_rate = m_encodeAudioInfo.sampleRate;
    m_c->sample_fmt = m_encodeAudioInfo.sampleFmt;
    m_c->channel_layout = m_encodeAudioInfo.channel_layout;
    m_c->channels = m_encodeAudioInfo.channels;
    m_c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    
    ret = avcodec_open2( m_c , codec,  nullptr );
    if( ret < 0 )
    {
        logFFmpegError("avcodec_open2", ret );
        goto failed;
    }
    TSK_DEBUG_INFO("avcodec_open2 success");

//    //初始化重采样：
//    m_swr = swr_alloc_set_opts( m_swr,
//                               m_encodeAudioInfo.channel_layout, m_encodeAudioInfo.sampleFmt, m_encodeAudioInfo.sampleRate,
//                               m_inputAudioInfo.channel_layout, m_inputAudioInfo.sampleFmt, m_inputAudioInfo.sampleRate,
//                               0,0);
//    if( m_swr )
//    {
//        printf("swr_alloc_set_opts error\n");
//        goto failed;
//    }
    
    //初始化重采样
    if( m_inputAudioInfo.sampleRate != m_encodeAudioInfo.sampleRate )
    {
#ifdef USE_WEBRTC_RESAMPLE
        if ( audio_resampler  == NULL)
        {
            audio_resampler = static_cast<void*>(new youme::webrtc::PushResampler<int16_t>());
        }
        youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>(audio_resampler);
        resampler->InitializeIfNeeded(m_inputAudioInfo.sampleRate, m_encodeAudioInfo.sampleRate, 1);
#else
        speex_resampler_destroy( audio_resampler );
        audio_resampler = speex_resampler_init(1,  m_inputAudioInfo.sampleRate, m_encodeAudioInfo.sampleRate, 3, NULL);
#endif
    }
#endif
    
    ret = avformat_alloc_output_context2( &m_ofmt_c,  nullptr,  nullptr,  m_filePath.c_str() );
    if( !m_ofmt_c )
    {
        logFFmpegError("avformat_alloc_output_context2", ret );
        goto failed;
    }
    
#if RECORD_AUDIO
    //添加音频流
    m_audioStream = avformat_new_stream(m_ofmt_c, codec);
    m_audioStream->codecpar->codec_tag = m_c->codec_tag;
    avcodec_parameters_from_context(m_audioStream->codecpar, m_c);
#endif
    
    //添加视频流
    m_videoStream = avformat_new_stream( m_ofmt_c,  nullptr );
    if( !m_videoStream )
    {
       TSK_DEBUG_INFO("alloc video out stream fail");
       goto failed;
    }
    
    m_videoStream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    /*此处,需指定编码后的H264数据的分辨率、帧率及码率*/
    m_videoStream->codec->codec_id = AV_CODEC_ID_H264;
    m_videoStream->codec->bit_rate = m_videoInfo.birrate;
    m_videoStream->codec->width = m_videoInfo.width;
    m_videoStream->codec->height = m_videoInfo.height;
    m_videoStream->codec->time_base.num = 1;
    m_videoStream->codec->time_base.den = m_videoInfo.fps;
    
    //add avcc versino number
    avcc[0] = 0x01;
    offset += 1;
    
    //add sps avc profile,compatibility,level
    memcpy(avcc+offset, m_sps+5, 3);
    offset += 3;
    
    //add avcc NaluLengthMinusSizeOne
    avcc[offset] = 0xff;
    offset += 1;
    
    //add number of sps nalu
    avcc[offset] = 0xe1;
    offset += 1;
    
    //add avcc sps size and sps data
    avcc[offset] = ((m_sps_size-4) & 0xff00) >> 8;
    avcc[offset+1] = (m_sps_size-4) & 0x00ff;
    offset += 2;
    memcpy(avcc+offset, m_sps+4, m_sps_size-4);
    offset += m_sps_size-4;
    
    //add number of pps nalu
    avcc[offset] = 0x01;
    offset += 1;
    
    //add avcc pps size and pps data
    avcc[offset] = ((m_pps_size-4) & 0xff00) >> 8;
    avcc[offset+1] = (m_pps_size-4) & 0x00ff;
    offset += 2;
    memcpy(avcc+offset, m_pps+4, m_pps_size-4);
    
    m_videoStream->codec->extradata = avcc;
    m_videoStream->codec->extradata_size = avcc_length;
    
    //打开输出
    ret = avio_open( &m_ofmt_c->pb, m_filePath.c_str(), AVIO_FLAG_WRITE );
    if( ret < 0 )
    {
        logFFmpegError("avio_open", ret );
        goto failed;
    }
    
    //写文件头
    ret = avformat_write_header( m_ofmt_c ,NULL);
    if( ret < 0 )
    {
        logFFmpegError("avformat_write_header", ret );
        goto failed;
    }
    
#if RECORD_AUDIO
    //准备编码用的AVFrame
    m_audioFrame = av_frame_alloc();
    m_audioFrame->format = m_encodeAudioInfo.sampleFmt;
    m_audioFrame->channel_layout = m_encodeAudioInfo.channel_layout;
    m_audioFrame->channels = m_encodeAudioInfo.channels;
    m_audioFrame->nb_samples = m_c->frame_size;
    ret = av_frame_get_buffer(m_audioFrame,0);
    if( ret < 0 )
    {
        logFFmpegError("av_frame_get_buffer", ret );
        
        goto failed;
    }
    
    m_audioFrame->pts = 0;
    m_audioFrame->pkt_dts = m_audioFrame->pkt_pts = 0;
    
    av_init_packet( & m_audioPkt );
#endif
    
    av_init_packet( & m_videoPkt );
    
    m_bRecordInited = true;
    
    return 0;
failed:
//    if( m_swr )
//    {
//        swr_free( &m_swr );
//        m_swr = nullptr;
//    }
    m_bRecordInitFailed = true;
    //如果失败了才需要清理，不然就应该在stop的时候清理
    if( m_c )
    {
        avcodec_free_context( &m_c );
        m_c = nullptr;
    }
    
    if( m_ofmt_c )
    {
        avio_close( m_ofmt_c->pb );
        avformat_free_context( m_ofmt_c );
        m_ofmt_c = nullptr;
    }
    
    if( m_audioFrame )
    {
        av_frame_free( &m_audioFrame );
    }
    
    if ( audio_resampler ) {
#ifdef USE_WEBRTC_RESAMPLE
        delete static_cast<youme::webrtc::PushResampler<int16_t> *>(audio_resampler);
        audio_resampler = nullptr;
#else
        speex_resampler_destroy(audio_resampler);
        audio_resampler= nullptr;
#endif
    }
    
    return ret;
    
}
void YMVideoRecorder::stopRecord()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if( m_bRecording == false )
    {
        return ;
    }
    
    m_bRecording = false;
    
    if( m_bRecordInited == true )
    {
        int ret = av_write_trailer( m_ofmt_c );
        if( ret < 0 )
        {
            logFFmpegError( "av_write_trailer", ret );
        }
        
        if( m_c )
        {
            avcodec_free_context( &m_c );
            m_c = nullptr;
        }
        
        if( m_ofmt_c )
        {
            avio_close( m_ofmt_c->pb );
            avformat_free_context( m_ofmt_c );
            m_ofmt_c = nullptr;
        }
        
        if( m_audioFrame )
        {
            av_frame_free( &m_audioFrame );
        }
        
        if ( audio_resampler ) {
#ifdef USE_WEBRTC_RESAMPLE
            delete static_cast<youme::webrtc::PushResampler<int16_t> *>(audio_resampler);
            audio_resampler = nullptr;
#else
            speex_resampler_destroy(audio_resampler);
            audio_resampler= nullptr;
#endif
        }
        
        
        m_bRecordInited = false;
    }
    
    return ;
}

void YMVideoRecorder::inputAudioData( uint8_t* data, int data_size, int timestamp )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if( !m_bRecording )
    {
        return ;
    }
    
    //video的基本信息都还没设置就来数据了，怎么那么快
    if( !m_bVideoInfoSetted || !m_bAudioInfoSetted )
    {
        return ;
    }
    
    if( !m_bRecordInited )
    {
        innerInit();
    }
    
    if( !m_bRecordInited )
    {
        return ;
    }
    
    //视频数据必须一开始就有，所以等视频有了，才放音频进来
    if( m_timestampStart == 0 )
    {
        return ;
    }
    
//    printf("Pinky:inputAudioData:%d, inner:%d\n", timestamp, timestamp - m_timestampStart );
    
    uint8_t* resampleData = nullptr;
    int resampleSize = 0;
    
    if ( audio_resampler ) {
        uint32_t inSample = data_size /sizeof( int16_t );
        uint32_t outSize = data_size * m_encodeAudioInfo.sampleRate / m_inputAudioInfo.sampleRate;
        uint32_t outSample = outSize / sizeof( int16_t );
        
        if( m_audioResampleBuffSize < outSize )
        {
            if( m_audioResampleBuff )
            {
                delete [] m_audioResampleBuff;
            }
            
            m_audioResampleBuffSize = outSize;
            m_audioResampleBuff = new uint8_t[m_audioResampleBuffSize];
        }
#ifdef USE_WEBRTC_RESAMPLE
        youme::webrtc::PushResampler<int16_t> * resampler = static_cast<youme::webrtc::PushResampler<int16_t> *>( audio_resampler );
        int src_nb_samples_per_process = ((resampler->GetSrcSampleRateHz() * 10) / 1000);
        int dst_nb_samples_per_process = ((resampler->GetDstSampleRateHz() * 10) / 1000);
        for (int i = 0; i * src_nb_samples_per_process < inSample; i++) {
            resampler->Resample((const int16_t*)data + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)m_audioResampleBuff + i * dst_nb_samples_per_process, 0);
        }
#else
        speex_resampler_process_int(audio_resampler, 0, (const spx_int16_t*)data, &inSample, (spx_int16_t*)m_audioResampleBuff, &outSample);
#endif
        
        resampleData = m_audioResampleBuff;
        resampleSize = outSize;
    }
    else{
        resampleData = data;
        resampleSize = data_size;
    }
    
    //todo,这里还可以优化
    //先要进入重采样
    if( m_audioBuffCurLen + resampleSize > m_audioBuffMaxLen )
    {
        //放不下了，丢弃掉
        return;
    }
    
    memcpy( m_audioBuff + m_audioBuffCurLen, resampleData ,  resampleSize );
    m_audioBuffCurLen += resampleSize;
    int frameBuffLen = m_audioFrame->nb_samples * sizeof( int16_t);
    if( m_audioBuffCurLen > frameBuffLen )
    {
        memcpy( m_audioFrame->data[0], m_audioBuff, frameBuffLen );
        m_audioFrame->linesize[0] = frameBuffLen;
        
        m_audioFrame->pts += m_audioFrame->nb_samples;
        m_audioFrame->pkt_pts = m_audioFrame->pts;
        m_audioFrame->pkt_dts = m_audioFrame->pts;
        
        encodeAudioData( m_audioFrame );
        
        memmove(m_audioBuff,  m_audioBuff + frameBuffLen,  frameBuffLen );
        m_audioBuffCurLen -= frameBuffLen;
    }
    
}

void YMVideoRecorder::setSpsAndPps(uint8_t* sps, int sps_size , uint8_t* pps, int pps_size )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    //只需要设置一次就可以了
    if( m_sps || m_pps )
    {
        return ;
    }
    
    m_sps_size = sps_size;
    m_sps = new uint8_t[m_sps_size];
    memcpy( m_sps,  sps,  sps_size);
    
    m_pps_size = pps_size;
    m_pps = new uint8_t[m_pps_size];
    memcpy( m_pps,  pps,  pps_size);
}

void YMVideoRecorder::inputVideoData( uint8_t* in_data, int in_data_size, int timestamp, int nalu_type  )
{
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if( !m_bRecording )
    {
        return ;
    }
    
    //video的基本信息都还没设置就来数据了，怎么那么快
    if( !m_bVideoInfoSetted || !m_bAudioInfoSetted )
    {
        return ;
    }
    
    bool bFirstFrame = false;
    
    //只有第一个sps来了，才开始录像
    if( nalu_type == H264_NAL_TYPE_SPS && m_timestampStart == 0 )
    {
        m_timestampStart = timestamp;
        bFirstFrame = true;
    }
    
    //
    if( m_timestampStart == 0 )
    {
        return ;
    }
    
    if( !m_bRecordInited  )
    {
        innerInit();
    }
    
    if( !m_bRecordInited )
    {
        return ;
    }
    
    uint8_t* tmpbuff = in_data;
    int tmpbuff_size = in_data_size;
    
    //如果是第一帧，需要把sps和pps拼接进去
    if( bFirstFrame && m_sps && m_pps )
    {
        int totalLen = m_sps_size + m_pps_size + in_data_size;
        uint8_t* buff = new uint8_t[ totalLen ];
        
        memcpy( buff , m_sps , m_sps_size );
        memcpy( buff + m_sps_size, m_pps,  m_pps_size );
        memcpy( buff + m_sps_size + m_pps_size ,  in_data,  in_data_size );
        
        tmpbuff = buff;
        tmpbuff_size = totalLen;
    }

    int innerTimestamp = timestamp - m_timestampStart;
//    printf("Pinky:inputVideoData: time:%d,  inner:%d, nalType:%d, size:%d\n", timestamp, innerTimestamp, nalu_type, in_data_size  );
    
    m_videoPkt.data = tmpbuff;
    m_videoPkt.size = tmpbuff_size;
    
    m_videoPkt.pts = innerTimestamp;
    m_videoPkt.dts = innerTimestamp;
    m_videoPkt.duration = 0;
    
    m_videoPkt.stream_index = m_videoStream->index;
    AVRational code_time_base;
    code_time_base.num = 1;
    code_time_base.den = m_videoInfo.timebaseden;
    av_packet_rescale_ts( &m_videoPkt, code_time_base , m_videoStream->time_base);
//    printf("Pinky:inputVideoData:write video pts:%d, frame :%fs, duration:%fs\n",   m_videoPkt.pts, m_videoPkt.pts * av_q2d(m_videoStream->time_base ), m_videoPkt.duration* av_q2d(m_videoStream->time_base ) );
//    av_bitstream_filter_filter(h264bsfc, m_videoStream->codec, NULL, &m_videoPkt.data, &m_videoPkt.size, m_videoPkt.data, m_videoPkt.size, 0);
    int ret = av_interleaved_write_frame( m_ofmt_c , &m_videoPkt );
    if( ret < 0 )
    {
        TSK_DEBUG_INFO("write error!");
    }
    
    av_packet_unref( &m_videoPkt );
    if( tmpbuff != in_data )
    {
        delete tmpbuff;
        tmpbuff = nullptr;
    }
}

int YMVideoRecorder::encodeAudioData( AVFrame* frame )
{
    int ret = avcodec_send_frame( m_c,  frame );
    if( ret != 0 )
    {
        return ret;
    }
    
    while( ret >= 0 )
    {
        ret = avcodec_receive_packet(m_c, &m_audioPkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            TSK_DEBUG_ERROR("Error encoding audio frame");
            break; ;
        }
        
        m_audioPkt.stream_index = m_audioStream->index;
        av_packet_rescale_ts( &m_audioPkt,  m_c->time_base,  m_audioStream->time_base );
        // TSK_DEBUG_INFO("write audio frame :%fs, duration:%fs",  m_audioPkt.pts * av_q2d(m_audioStream->time_base ), m_audioPkt.duration* av_q2d(m_audioStream->time_base ) );
        ret = av_interleaved_write_frame( m_ofmt_c ,&m_audioPkt);
        avio_flush( m_ofmt_c->pb );
        if( ret < 0 )
        {
            TSK_DEBUG_INFO("write frame error:%d", ret );
        }
    }
    
    return 0;
}
#endif

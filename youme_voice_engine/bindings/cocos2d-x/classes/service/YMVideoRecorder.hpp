//
//  YMVideoRecorder.hpp
//  VideoRecord
//
//  Created by pinky on 2019/5/15.
//  Copyright © 2019 youme. All rights reserved.
//

#ifndef YMVideoRecorder_hpp
#define YMVideoRecorder_hpp

#include <stdio.h>
#include <string>

extern "C"
{
#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

#include <mutex>

#include "tinydav/codecs/mixer/speex_resampler.h" // Use speex open source function

struct AudioInfo
{
    int sampleRate;
    AVSampleFormat sampleFmt;
    uint64_t channel_layout;
    int channels;
};

struct VideoInfo
{
    int birrate;
    int width;
    int height;
    int fps;
    int timebaseden;
};

class YMVideoRecorder
{
public:
    YMVideoRecorder();
    ~YMVideoRecorder();
    
public:
    void setFilePath( std::string filePath );
    //设置初始化的信息
    void setVideoInfo( int width, int height, int fps, int birrate, int timebaseden );
    void setInputAudioInfo( int sampleRate );
    
    //输出mp4把，不能随便指定输出格式，因为输出格式不一定只是所有的编码
    int startRecord( );
    void stopRecord();
    
    void inputAudioData( uint8_t* data, int data_size, int timestamp );
    //只支持h264
    void setSpsAndPps( uint8_t* sps, int sps_size, uint8_t* pps, int pps_size );
    void inputVideoData( uint8_t* data, int data_size, int timestamp, int nalu_type  );
 
private:
    //初始化ffmpeg需要的任何东西，并写文件头。在第一个数据输入的时候处理。
    int innerInit();
    //编码的一帧和输入的一个buff是不一样长的，重采样后，先放入一个buff吧。
//    void resampleAudioData( uint8_t* data, int data_size );
    //重采样好进行buff，时间戳就得重新算了吧
    int encodeAudioData( AVFrame* frame  );
    
    void reset();
    
    void logFFmpegError( std::string opMsg, int err );
private:
    AudioInfo m_inputAudioInfo;
    AudioInfo m_encodeAudioInfo;
    VideoInfo m_videoInfo;
    int64_t  m_encodeBitrate;
    
    AVCodecContext* m_c;
    AVFormatContext* m_ofmt_c;
//    SwrContext* m_swr;
    AVStream* m_audioStream;
    AVStream* m_videoStream;

    
    //音频buff，因为编码的长度和一次输入的长度不一致
    uint8_t* m_audioBuff;
    int m_audioBuffCurLen;
    int m_audioBuffMaxLen;
    
    uint8_t* m_audioResampleBuff;
    int m_audioResampleBuffSize;
    
    AVFrame *m_audioFrame ;
    AVPacket m_audioPkt;
    AVPacket m_videoPkt;
    
    bool m_bRecording;
    
    bool m_bVideoInfoSetted = false;
    bool m_bAudioInfoSetted = false;

    
    std::mutex m_mutex;
    bool m_bRecordInited;
    bool m_bRecordInitFailed;
    
    std::string m_filePath;
    
    int m_timestampStart;
    
    uint8_t* m_sps;
    int m_sps_size;
    uint8_t* m_pps;
    int m_pps_size;
    
#ifdef USE_WEBRTC_RESAMPLE
    void                    *audio_resampler;
#else
    SpeexResamplerState     *audio_resampler;
#endif
};

#endif /* YMVideoRecorder_hpp */

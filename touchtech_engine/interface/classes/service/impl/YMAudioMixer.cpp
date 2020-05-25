#include "YMAudioMixer.hpp"
#include <sstream>
#include <memory>
#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <YouMeCommon/TimeUtil.h>

#include "tmedia_utils.h"
#include "tinymedia/tmedia_defaults.h"
#include "tsk_debug.h"
#include "XConfigCWrapper.hpp"

static int YMAUDIO_MIXER_RINGBUFFER_MAX_TIME_MS = 1200;


// 论文:
// http://wenku.baidu.com/view/884624d97f1922791688e85b.html

typedef struct
{
    int Rsh;
    int K;
} TABLE;

// N取2的整数次幂. N越大, 高强度信号越失真. 推荐取 8或16
#define ITEMS 5
const static TABLE table[ITEMS] = {// N=8
    {3, 0},
    {6, 28672},
    {9, 32256},
    {12, 32704},
    {15, 32760}
};
/*
 const static TABLE table[ITEMS] = {// N=16
 {3, 0},
 {6, 30720},
 {9, 32640},
 {12, 32760},
 {15, 32767}
 };
 */
#define MIN(m, n) ((m < n)?(m):(n))
#define MAX(m, n) ((m > n)?(m):(n))
#define ABS(n) ((n < 0)?(-n):(n))
#define Q 16
#define MAXS16 (0x7FFF)
#define SGN(n) ((n < 0) ? (-1) : (1))

/**
 * short* in     Sample 输入流数组
 * int in_tracks 输入流音轨数 (Sample输入流数组长度)
 * return        混音后的结果数值, short 型
 */
static inline int16_t ymaudio_mixer_do_mix(int16_t * in, size_t in_tracks)
{
    long total = 0;
    long n, c, d;
    for (; in_tracks;) {
        total += in[--in_tracks];
    }
    n = MIN((ABS(total) >> (Q - 1)), ITEMS - 1);
    c = ABS(total) & MAXS16;
    d = (c << 2) + (c << 1) + c;
    return SGN(total) * ((d >> table[n].Rsh) + table[n].K);
}

typedef struct audioMixMeta {
    int16_t * bufferPtr;
    uint32_t  sampleCount;
} AudioMixMeta;

void ymaudio_mixer_codec_mixer_mix16(const std::vector<AudioMixMeta> &pInputPtr, const size_t nTrackCountTotal, int16_t* pOutBufferPtr, const size_t nMixSampleCountMax)
{
    size_t nPos = 0, nTrackIdx = 0, nTrackCountToMix = 0;
    
    int16_t* pInBufferPtr = (int16_t*)malloc( sizeof(int16_t) * nTrackCountTotal);
    AudioMixMeta meta;
    for (;nPos < nMixSampleCountMax; nPos++) {
        nTrackIdx = 0;
        nTrackCountToMix = 0;
        for (;nTrackIdx < nTrackCountTotal; nTrackIdx++ ) {
            meta = pInputPtr[nTrackIdx];
            if (nPos >= meta.sampleCount) {
                continue;
            }
            pInBufferPtr[nTrackCountToMix++] = meta.bufferPtr[nPos];
        }
        pOutBufferPtr[nPos] = ymaudio_mixer_do_mix(pInBufferPtr, nTrackCountToMix);
    }
    
    free((void*) pInBufferPtr);
}


static inline uint16_t ymaudio_mixer_bswap_16(uint16_t x)
{
    return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

static inline void ymaudio_mixer_multichannel_interleaved_to_mono (void *pInput, void* pOutput, int* pu1ChNum, int* pu4TotalSz, bool bInterleaved)
{
    int16_t *pi2Buf1 = (int16_t *)pInput;
    int16_t *pi2Buf2 = (int16_t *)pOutput;
    int16_t i2SamplePerCh = *pu4TotalSz / sizeof(int16_t) / *pu1ChNum;
    
    if (!pInput || !pOutput || !(*pu1ChNum)) {
        return;
    }
    
    if ((*pu1ChNum == 2) && (bInterleaved)) {
        for (int i = 0; i < i2SamplePerCh; i++) {
            pi2Buf2[i] = pi2Buf1[i * (*pu1ChNum)]; // left channel
            pi2Buf2[i + i2SamplePerCh] = pi2Buf1[i * (*pu1ChNum) + 1]; // right channel
        }
    } else {
        memcpy(pi2Buf2, pi2Buf1, *pu4TotalSz);
    }
    
    // Downmix to MONO here, fix me!!
    if (*pu1ChNum == 2) {
        for (int i = 0; i < i2SamplePerCh; i++) {
            pi2Buf2[i] = (pi2Buf2[i] >> 1) + (pi2Buf2[i + i2SamplePerCh] >> 1);
        }
        *pu1ChNum /= 2;
        *pu4TotalSz /= 2;
    }
}

YMAudioMixer::YMAudioMixer()
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    mMixer_start_buffering_time_ms = Config_GetInt("MIXER_START_BUFFERING_TIME_MS", 60);
    mMixer_end_closing_time_ms = Config_GetInt("MIXER_END_CLOSING_TIME_MS", 200);
    
    if (tmedia_defaults_get_pcm_file_size_kb() > 0) {
        openPcmDumpFile();
    }
}

YMAudioMixer::~YMAudioMixer()
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if (mMixThread.joinable()) {
        if (std::this_thread::get_id() != mMixThread.get_id()) {
            m_isLooping = false;
            TSK_DEBUG_INFO("Start joining thread");
            mMixThread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            mMixThread.detach();
        }
    }
    
    std::unique_lock<std::mutex> dataLock(mDataMutex);
    
    // 释放重采样Map的所有buffer
    auto it_resampler = mInputAudioResamplerMap.begin();
    for (; it_resampler != mInputAudioResamplerMap.end();)
    {
        if(NULL != it_resampler->second) {
            delete it_resampler->second;
            it_resampler->second = NULL;
        }
        it_resampler = mInputAudioResamplerMap.erase(it_resampler);
    }
    // 释放输入Map的所有buffer
    auto it = mInputAudioMap.begin();
    for (; it != mInputAudioMap.end();)
    {
        if(NULL != it->second.second) {
            WebRtc_youme_FreeBuffer(it->second.second);
            it->second.second = NULL;
        }
        it = mInputAudioMap.erase(it);
    }
    
    // 释放processTickMap
    auto processTickIt = mProcessTickCountMap.begin();
    for (; processTickIt != mProcessTickCountMap.end();)
    {
        processTickIt = mProcessTickCountMap.erase(processTickIt);
    }
    
    closePcmDumpFile();
}

void YMAudioMixer::openPcmDumpFile () {
    char dumpPcmPath[1024] = "";
    const char *pBaseDir = NULL;
    int baseLen = 0;
    
    pBaseDir = tmedia_defaults_get_app_document_path();
    
    if (NULL == pBaseDir) {
        return;
    }
    
    strncpy(dumpPcmPath, pBaseDir, sizeof(dumpPcmPath) - 1);
    baseLen = strlen(dumpPcmPath) + 1; // plus the trailing '\0'
    
    strncat(dumpPcmPath, "/dump_audio_mixer_mixed_data.pcm", sizeof(dumpPcmPath) - baseLen);
    if (mMixedDataDumpFile) {
        fclose(mMixedDataDumpFile);
    }
    mMixedDataDumpFile = fopen (dumpPcmPath, "wb");
    mMixedDataDumpSize = 0;
}

void YMAudioMixer::closePcmDumpFile()
{
    if (mMixedDataDumpFile) {
        fclose (mMixedDataDumpFile);
        mMixedDataDumpFile = NULL;
        mMixedDataDumpSize = 0;
    }
}

int YMAudioMixer::setMixOutputInfo(YMAudioFrameInfo info)
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if(!info.isSignedInteger) {
        //todu 暂时只支持int16_t
        return -1;
    }
    if(info.isFloat) {
        //todu 暂时只支持int16_t
        return -1;
    }
    if(info.bytesPerFrame != 2) {
        //todu 暂时只支持int16_t
        return -1;
    }
    if(info.channels != 1) {
        // 输入支持单声道和双声道，输出暂时只支持单声道
        return -1;
    }
    
    switch (info.sampleRate) {
        case 8000:
        case 16000:
        case 24000:
        case 22050:
        case 32000:
        case 44100:
        case 48000:
            break;
        default:
            return -1;
    }
    mOutputInfo = info;
    return 0;
}

int YMAudioMixer::setMixedCallback(YMAudioMixerCallback callback)
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if (NULL == callback) {
        return -1;
    }
    mCallback = callback;
    return 0;
}

int YMAudioMixer::setOutputIntervalMs(int interval)
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if(interval != 20)
        return -1;
    mInterval = interval;
    return 0;
}

size_t YMAudioMixer::inputAudioFrameForMix(void* data, size_t length, int streamId, YMAudioFrameInfo info)
{
    // std::unique_lock<std::mutex> stateLock(mStateMutex);
    
    if (!m_isLooping) {
        return -1;
    }
    
    if(!info.isSignedInteger) {
        //todu 暂时只支持int16_t
        return -1;
    }
    if(info.isFloat) {
        //todu 暂时只支持int16_t
        return -1;
    }

    if(info.channels != 1 && info.channels != 2) {
        // 输入支持单声道和双声道，输出暂时只支持单声道
        return -1;
    }
    
    if(info.bytesPerFrame/info.channels != 2) {
        //todu 暂时只支持int16_t
        return -1;
    }

    switch (info.sampleRate) {
        case 8000:
        case 16000:
        case 24000:
        case 22050:
        case 32000:
        case 44100:
        case 48000:
            break;
        default:
            return -1;
    }
    
    std::unique_lock<std::mutex> dataLock(mDataMutex);
    size_t input_frames = length/info.bytesPerFrame;

    RingBuffer* ringbuffer = NULL;
    auto it = mInputAudioMap.find(streamId);
    if (it == mInputAudioMap.end()) {
        size_t ringbuffer_max_bytes = YMAUDIO_MIXER_RINGBUFFER_MAX_TIME_MS * info.sampleRate * info.bytesPerFrame / 1000;
        size_t ringbuffer_max_frames = ringbuffer_max_bytes / info.bytesPerFrame;

        ringbuffer = WebRtc_youme_CreateBuffer(ringbuffer_max_frames, info.bytesPerFrame);
        WebRtc_youme_InitBuffer(ringbuffer);
        mInputAudioMap[streamId] = std::make_pair(info, ringbuffer);
        mProcessTickCountMap[streamId] = std::make_pair<int, int>(mMixer_start_buffering_time_ms/mInterval, 0);
    }else {
        ringbuffer = it->second.second;
    }
    
    if(WebRtc_youme_available_write(ringbuffer) < input_frames){
        TSK_DEBUG_WARN("== inputAudioFrame id:%d ringbuffer full, drop it!!", streamId);
        return -1;
    }
    
    size_t written_frames = WebRtc_youme_WriteBuffer(ringbuffer, data, input_frames);

    return written_frames;
}

void YMAudioMixer::startThread()
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if (m_isLooping) {
        TSK_DEBUG_INFO("YMAudioMixer already started");
        return;
    }
    m_isLooping = true;
    mMixThread = std::thread(&YMAudioMixer::audioMixThreadFunc, this);
}

void YMAudioMixer::stopThread()
{
    std::unique_lock<std::mutex> stateLock(mStateMutex);
    if (!m_isLooping) {
        TSK_DEBUG_INFO("YMAudioMixer already stopped");
        return;
    }
    if (mMixThread.joinable()) {
        if (std::this_thread::get_id() != mMixThread.get_id()) {
            m_isLooping = false;
            TSK_DEBUG_INFO("Start joining thread");
            mMixThread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            mMixThread.detach();
        }
    }
}


void YMAudioMixer::getStreamData(int streamId, void** outdata, YMAudioFrameInfo inInfo, RingBuffer* ringbuffer)
{
    size_t process_output_frames = mInterval * mOutputInfo.sampleRate / 1000;
    size_t process_input_frames = mInterval * inInfo.sampleRate / 1000;
    size_t read_available_frames = WebRtc_youme_available_read(ringbuffer);
    int read_available_time_ms = 1000*read_available_frames/inInfo.sampleRate;
    auto processTickIt = mProcessTickCountMap.find(streamId);
    if (processTickIt == mProcessTickCountMap.end()) {
        TSK_DEBUG_ERROR("can not find tick count for:%d", streamId);
        *outdata = NULL;
        return;
    }
    if(read_available_time_ms < mInterval) {
//        TSK_DEBUG_INFO("--- read_available_time_ms:%d, mInterval:%d", read_available_time_ms, mInterval);
        if(processTickIt->second.first == 0) {
            if(processTickIt->second.second >= mMixer_end_closing_time_ms/mInterval) {
                // 数据量不足已超过mMixer_end_closing_time_ms/mInterval个tick，重置为初始状态，下一次输入还要等mMixer_start_buffering_time_ms/mInterval个tick
                TSK_DEBUG_INFO("---- streamId[%d] data lack too long, reset state", streamId);
                processTickIt->second.first = mMixer_start_buffering_time_ms/mInterval;
                processTickIt->second.second = 0;
                *outdata = NULL;
                return;
            }
//            TSK_DEBUG_INFO("---- streamId[%d] data lack tick:[%d][%d]", streamId, processTickIt->second.first, processTickIt->second.second);
            ++(processTickIt->second.second);
        }
        *outdata = NULL;
    }else {
        if(processTickIt->second.first > 0 && processTickIt->second.second == 0) {
            // 初始状态下要先等待mMixer_start_buffering_time_ms/mInterval个tick再取数据
            --(processTickIt->second.first);
//            TSK_DEBUG_ERROR("---- streamId[%d] start buffering tick:[%d][%d]", streamId, processTickIt->second.first, processTickIt->second.second);
            *outdata = NULL;
            return;
        }
//        TSK_DEBUG_INFO("---- streamId[%d] runing tick:[%d][%d]", streamId, processTickIt->second.first, processTickIt->second.second);
        int input_data_size = process_input_frames * inInfo.bytesPerFrame;
        void* input_data = malloc(process_input_frames * inInfo.bytesPerFrame);
        void* temp_data = malloc(process_input_frames * inInfo.bytesPerFrame);
        void* output_data = malloc(process_output_frames * mOutputInfo.bytesPerFrame);
        WebRtc_youme_ReadBuffer(ringbuffer, NULL, input_data, process_input_frames);

        // 大小端转换
        if(inInfo.isBigEndian != mOutputInfo.isBigEndian) {
            for (int i = 0; i < process_input_frames * inInfo.channels; ++i) {
                *((int16_t*)input_data + i) = ymaudio_mixer_bswap_16(*((int16_t*)input_data + i));
            }
        }
        
        // 单双声道转换,目前输出仅支持单声道
        if(inInfo.channels == 2 && mOutputInfo.channels == 1) {
            ymaudio_mixer_multichannel_interleaved_to_mono(input_data, temp_data, &(inInfo.channels), &input_data_size, !inInfo.isNonInterleaved);
            inInfo.bytesPerFrame = inInfo.bytesPerFrame / 2;
            // inInfo.channels = 1;    // 输出数据为单声道
            memcpy(input_data, temp_data, input_data_size);
        }

        // 重采样
        if(inInfo.sampleRate != mOutputInfo.sampleRate) {
            if(mInputAudioResamplerMap[streamId] == NULL) {
                mInputAudioResamplerMap[streamId] = new youme::webrtc::PushResampler<int16_t>();
                mInputAudioResamplerMap[streamId]->InitializeIfNeeded(inInfo.sampleRate, mOutputInfo.sampleRate, mOutputInfo.channels);
            }
            int src_nb_samples_per_process = ((mInputAudioResamplerMap[streamId]->GetSrcSampleRateHz() * 10) / 1000);
            int dst_nb_samples_per_process = ((mInputAudioResamplerMap[streamId]->GetDstSampleRateHz() * 10) / 1000);
            for (int i = 0; i * src_nb_samples_per_process < process_input_frames * inInfo.channels; i++) {
                mInputAudioResamplerMap[streamId]->Resample((const int16_t*)input_data + i * src_nb_samples_per_process, src_nb_samples_per_process, (int16_t*)output_data + i * dst_nb_samples_per_process, 0);
            }
        }else {
            memcpy(output_data, input_data, input_data_size);
        }

        free(input_data);
        free(temp_data);
        input_data = NULL;
        temp_data = NULL;
        *outdata = output_data;
    }
}

void YMAudioMixer::audioMixThreadFunc()
{
    uint64_t last_t = youmecommon::CTimeUtil::GetTimeOfDay_MS();
    uint64_t curr_t = youmecommon::CTimeUtil::GetTimeOfDay_MS();
    uint64_t start_t = youmecommon::CTimeUtil::GetTimeOfDay_MS();
    int time_count = 1;
    
    while (m_isLooping) {
        
        uint64_t right_time = start_t + time_count * mInterval;
        curr_t = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (curr_t < right_time) {
            std::this_thread::sleep_for(std::chrono::milliseconds(right_time - curr_t));
        }
        last_t = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        
        // TSK_DEBUG_INFO("--- thread sleep_time:%llu cost_time:%llu, count:%d, average sleep :%f", right_time - curr_t, last_t - start_t, time_count, (double)(last_t - start_t)/ time_count );
        time_count++;
        
        size_t process_output_frames = mInterval * mOutputInfo.sampleRate / 1000;
        std::vector<AudioMixMeta> input_mix_arrays;
        {
            std::unique_lock<std::mutex> dataLock(mDataMutex);
            for (auto it = mInputAudioMap.begin(); it != mInputAudioMap.end();) {
                void* mix_in_data = NULL;
                int streamId = it->first;
                getStreamData(streamId, &mix_in_data, it->second.first, it->second.second);
                if (NULL == mix_in_data) {
                    // release resampler
//                    TSK_DEBUG_INFO("== audioMixThreadFunc streamId:%d not enough data", streamId);
                }else {
                    AudioMixMeta mixMeta;
                    mixMeta.bufferPtr = (int16_t*)mix_in_data;
                    mixMeta.sampleCount = (uint32_t)mInterval * mOutputInfo.sampleRate / 1000;
                    input_mix_arrays.push_back(mixMeta);
                }
                ++it;
            }
            void* mix_out_data = malloc(process_output_frames * mOutputInfo.bytesPerFrame);
            memset(mix_out_data, 0, process_output_frames * mOutputInfo.bytesPerFrame);
            if(input_mix_arrays.size() == 0) {
                //dump pcm before encode and send
                if (mMixedDataDumpFile) {
                    if (mMixedDataDumpSize > tmedia_defaults_get_pcm_file_size_kb() * 1024) {
                        openPcmDumpFile();
                    }
                    if (mMixedDataDumpFile) {
                        fwrite (mix_out_data, 1, process_output_frames * mOutputInfo.bytesPerFrame, mMixedDataDumpFile);
                        mMixedDataDumpSize += process_output_frames * mOutputInfo.bytesPerFrame;
                    }
                }
                free(mix_out_data);
                mix_out_data = NULL;
                continue;
            }
            if (input_mix_arrays.size() == 1) {
                memcpy(mix_out_data, input_mix_arrays[0].bufferPtr, process_output_frames * mOutputInfo.bytesPerFrame);
            }else {
                ymaudio_mixer_codec_mixer_mix16(input_mix_arrays, input_mix_arrays.size(), (int16_t*)mix_out_data, process_output_frames);
            }
            
            if(mCallback != NULL) {
                mOutputInfo.timestamp = last_t;
                mCallback(mix_out_data, process_output_frames * mOutputInfo.bytesPerFrame, mOutputInfo);
            }
            
            //dump pcm before encode and send
            if (mMixedDataDumpFile) {
                if (mMixedDataDumpSize > tmedia_defaults_get_pcm_file_size_kb() * 1024) {
                    openPcmDumpFile();
                }
                if (mMixedDataDumpFile) {
                    fwrite (mix_out_data, 1, process_output_frames * mOutputInfo.bytesPerFrame, mMixedDataDumpFile);
                    mMixedDataDumpSize += process_output_frames * mOutputInfo.bytesPerFrame;
                }
            }
            
            for (int i = 0; i < input_mix_arrays.size(); i++) {
                delete input_mix_arrays[i].bufferPtr;
            }
            input_mix_arrays.clear();
            free(mix_out_data);
            mix_out_data = NULL;
        }
    }
    
}


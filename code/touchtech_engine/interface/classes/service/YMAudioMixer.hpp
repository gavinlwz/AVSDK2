#ifndef YMAudioMixer_hpp
#define YMAudioMixer_hpp

#include "YMAudioMixer.hpp"
#include "YouMeConstDefine.h"

#include <stdio.h>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <thread>

#include "webrtc/common_audio/ring_buffer.h"
#include "webrtc/common_audio/resampler/include/push_resampler.h"


typedef void (*YMAudioMixerCallback) (void* data, size_t length, YMAudioFrameInfo info);

class YMAudioMixer
{
public:
    YMAudioMixer();
    ~YMAudioMixer();
    
    void openPcmDumpFile();
    void closePcmDumpFile();
    
    int setMixOutputInfo(YMAudioFrameInfo info);
    int setMixedCallback(YMAudioMixerCallback callback);
    int setOutputIntervalMs(int interval);
//    void setStartBufferTimeMs(int startTime);
    size_t inputAudioFrameForMix(void* data, size_t length, int streamId, YMAudioFrameInfo info);
    
    void startThread();
    void stopThread();
    
private:
    void getStreamData(int stream, void** indata, YMAudioFrameInfo inInfo, RingBuffer* ringbuffer);
    void audioMixThreadFunc();
    
    YMAudioMixerCallback mCallback;
    YMAudioFrameInfo mOutputInfo;
    int mInterval = 20;
    int mMixer_start_buffering_time_ms = 100;
    int mMixer_end_closing_time_ms = 200;
    std::thread mMixThread;
    std::mutex mDataMutex;
    std::mutex mStateMutex;
    std::map<int, std::pair<YMAudioFrameInfo, RingBuffer*>> mInputAudioMap;
    std::map<int, youme::webrtc::PushResampler<int16_t>*> mInputAudioResamplerMap;
    std::map<int, std::pair<int, int>> mProcessTickCountMap;
    FILE* mMixedDataDumpFile = NULL;
    uint32_t mMixedDataDumpSize = 0;
    bool m_isLooping = false;
};

#endif /* YMAudioMixer_hpp */

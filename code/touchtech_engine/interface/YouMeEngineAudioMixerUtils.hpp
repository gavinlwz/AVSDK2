//
//  YouMeEngineAudioMixerUtils.hpp
//  youme_voice_engine
//
//  Created by youmi on 2017/12/5.
//  Copyright Â© 2017å¹´ Youme. All rights reserved.
//

#ifndef YouMeEngineAudioMixerUtils_hpp
#define YouMeEngineAudioMixerUtils_hpp

#include <stdio.h>

#include <thread>
#include <deque>
#include <mutex>
#include <XCondWait.h>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include "webrtc/common_audio/resampler/include/push_resampler.h"

class AudioMixerTrack {
    
public:
    AudioMixerTrack(int size);
    AudioMixerTrack(void* data, int size, int channels, uint64_t timestamp);
    AudioMixerTrack(int size, int channels, uint64_t timestamp);
    virtual ~AudioMixerTrack();
    
public:
    int channels;
    void* data;
    int size;
    uint64_t timestamp;
};

class AudioMixerFifo {
    
public:
    AudioMixerFifo(std::string indexId, int samplerate, int channels);
    virtual ~AudioMixerFifo();
    
    int write(void* data, int size);
    int read(void* data, int size);
    int getBlankSize();
    int getDataSize();
    void clean();
public:
    std::string indexId;
    int samplerate;
    int channels;
    void* data;
    int size;
    int rp;
    int wp;
    
    std::recursive_mutex mutex;
};

class YouMeEngineAudioMixerUtils {
    
private:
    static YouMeEngineAudioMixerUtils *mPInstance;
    static std::mutex mInstanceMutex;
    std::thread m_thread;
    bool m_isLooping = false;
    int m_audioTrackSamplerate = 44100;
    youme::webrtc::PushResampler<int16_t>* m_resampler;
    
    std::list<std::shared_ptr<AudioMixerFifo>> audioFifoList;
    std::recursive_mutex audioFifoListMutex;

    YouMeEngineAudioMixerUtils();
    virtual ~YouMeEngineAudioMixerUtils();
    
    std::shared_ptr<AudioMixerFifo> getAudioMixerFifo(std::string indexId);
    void applyVolumeGain(void* pBuf, int nBytes, int nBytesPerSample, float volumeGain);
    float m_inputVolumeGain;
    float m_trackVolumeGain;
    // Audio mixing thread related
    int addNewAudioMixerFifo(std::string indexId, int samplerate, int channels);

    std::shared_ptr<AudioMixerTrack> mixAudio(std::list<std::shared_ptr<AudioMixerTrack>> audioTrackList);
    
public:
    static YouMeEngineAudioMixerUtils* getInstance(void);
    static void destroy();
    
    YouMeErrorCode setAudioMixerTrackSamplerate(int sampleRate);
    YouMeErrorCode setAudioMixerTrackVolume(int volume);
    YouMeErrorCode setAudioMixerInputVolume(int volume);
    YouMeErrorCode pushAudioMixerTrack(void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, bool bFloat, uint64_t timestamp);
    YouMeErrorCode inputAudioToMix(std::string indexId, void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, bool bFloat, uint64_t timestamp);
};
#endif /* YouMeEngineAudioMixerUtils_hpp */

/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:paul
 *  日期:2016.9.19
 *  说明:对外发布接口
 ******************************************************************/
#ifndef _YOUME_AUDIO_ANDROID_IMPL_H
#define _YOUME_AUDIO_ANDROID_IMPL_H

#include "audio_android_producer.h"
#include "audio_android_consumer.h"

class AudioAndroidDeviceCallbackImpl
{
    public:
    AudioAndroidDeviceCallbackImpl ();
    ~AudioAndroidDeviceCallbackImpl ();

    int32_t RecordedDataIsAvailable (const void *audioSamples, const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec);
    int32_t NeedMorePlayData (const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec, void *audioSamples, uint32_t &nSamplesOut);
    
    inline void SetConsumer (const struct audio_consumer_android_s *pConsumer)
    {
        m_pConsumer = pConsumer;
    }
    inline void SetProducer (const struct audio_producer_android_s *pProducer)
    {
        m_pProducer = pProducer;
    }

    private:
    const struct audio_producer_android_s *m_pProducer; // must be const and must not take reference
    const struct audio_consumer_android_s *m_pConsumer; // mut be const and must not take reference
};

#endif /* _YOUME_AUDIO_OPENSLES_SLDEVICE_IMPL_H */

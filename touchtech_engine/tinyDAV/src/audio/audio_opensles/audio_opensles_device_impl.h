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
#ifndef _YOUME_AUDIO_OPENSLES_SLDEVICE_IMPL_H
#define _YOUME_AUDIO_OPENSLES_SLDEVICE_IMPL_H

#include "audio_opensles_config.h"
#include "audio_opensles_device.h"

class SLAudioDeviceCallbackImpl : public SLAudioDeviceCallback
{
    public:
    SLAudioDeviceCallbackImpl ();
    virtual ~SLAudioDeviceCallbackImpl ();

    virtual int32_t RecordedDataIsAvailable (const void *audioSamples, const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec);

    virtual int32_t NeedMorePlayData (const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec, void *audioSamples, uint32_t &nSamplesOut);

    inline void SetConsumer (const struct audio_consumer_opensles_s *pConsumer)
    {
        m_pConsumer = pConsumer;
    }
    inline void SetProducer (const struct audio_producer_opensles_s *pProducer)
    {
        m_pProducer = pProducer;
    }

    private:
    const struct audio_consumer_opensles_s *m_pConsumer; // mut be const and must not take reference
    const struct audio_producer_opensles_s *m_pProducer; // mut be const and must not take reference
};

#endif /* _YOUME_AUDIO_OPENSLES_SLDEVICE_IMPL_H */

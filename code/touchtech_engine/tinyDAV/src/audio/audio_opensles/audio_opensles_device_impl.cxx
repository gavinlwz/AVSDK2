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
#include "audio_opensles_device_impl.h"

#include "audio_opensles_producer.h"
#include "audio_opensles_consumer.h"

#include "NgnConfigurationEntry.h"
#include "NgnMemoryConfiguration.hpp"


SLAudioDeviceCallbackImpl::SLAudioDeviceCallbackImpl () : SLAudioDeviceCallback ()
{
}

SLAudioDeviceCallbackImpl::~SLAudioDeviceCallbackImpl ()
{
}

int32_t SLAudioDeviceCallbackImpl::RecordedDataIsAvailable (const void *audioSamples, const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec)
{
    if (!m_pProducer)
    {
        TSK_DEBUG_WARN ("No wrapped producer");
        return 0;
    }
        
    return audio_producer_opensles_handle_data_10ms (m_pProducer, audioSamples, nSamples, nBytesPerSample, samplesPerSec, nChannels);
}


int32_t SLAudioDeviceCallbackImpl::NeedMorePlayData (const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec, void *audioSamples, uint32_t &nSamplesOut)
{
    if (!m_pConsumer)
    {
        TSK_DEBUG_WARN ("No wrapped consumer");
        return 0;
    }
    nSamplesOut = audio_consumer_opensles_get_data_10ms (m_pConsumer, audioSamples, nSamples, nBytesPerSample, nChannels, samplesPerSec);
    
    if (nSamplesOut >= 0)
    {
        return 0;
    }
   
    return -1;
}
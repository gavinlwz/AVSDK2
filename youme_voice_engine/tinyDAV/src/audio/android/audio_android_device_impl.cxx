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
#include "audio_android_device_impl.h"

AudioAndroidDeviceCallbackImpl::AudioAndroidDeviceCallbackImpl ()
{
}

AudioAndroidDeviceCallbackImpl::~AudioAndroidDeviceCallbackImpl ()
{
}

int32_t AudioAndroidDeviceCallbackImpl::RecordedDataIsAvailable (const void *audioSamples, const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec)
{
    if (!m_pProducer)
    {
        TSK_DEBUG_WARN ("No wrapped producer");
        return 0;
    }
        
    return audio_producer_android_handle_data_20ms (m_pProducer, audioSamples, nSamples, nBytesPerSample, samplesPerSec, nChannels);
}

int32_t AudioAndroidDeviceCallbackImpl::NeedMorePlayData (const uint32_t nSamples, const uint8_t nBytesPerSample, const uint8_t nChannels, const uint32_t samplesPerSec, void *audioSamples, uint32_t &nSamplesOut)
{
    if (!m_pConsumer)
    {
        TSK_DEBUG_WARN ("No wrapped consumer");
        return 0;
    }
    nSamplesOut = audio_consumer_android_get_data_20ms (m_pConsumer, audioSamples, nSamples, nBytesPerSample, nChannels, samplesPerSec);
    
    if (nSamplesOut >= 0)
    {
        return 0;
    }
    
    return -1;
}
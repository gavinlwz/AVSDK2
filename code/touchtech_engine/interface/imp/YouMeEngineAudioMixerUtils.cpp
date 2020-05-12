//
//  YouMeEngineAudioMixerUtils.cpp
//  youme_voice_engine
//
//  Created by youmi on 2017/12/5.
//  Copyright Â© 2017å¹´ Youme. All rights reserved.
//

#include "tsk_debug.h"

#include "tmedia_utils.h"
#include "NgnTalkManager.hpp"
#include <StringUtil.hpp>
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "YouMeEngineAudioMixerUtils.hpp"

#define    auido_clear_index            "0"

/**********************************************
 *
 * AudioMixerTrack implementation
 *
 **********************************************/
AudioMixerTrack::AudioMixerTrack(int size) {
    this->size = size;
    this->data = malloc(this->size);
    memset(this->data, 0, this->size);
}

AudioMixerTrack::AudioMixerTrack(void* data, int size, int channels, uint64_t timestamp) {
    this->channels = channels;
    this->size = size;
    this->data = malloc(this->size);
    memcpy(this->data, data, this->size);
    this->timestamp = timestamp;
}

AudioMixerTrack::AudioMixerTrack(int size, int channels, uint64_t timestamp) {
    this->channels = channels;
    this->size = size;
    this->data = malloc(this->size);
    memset(this->data, 0, this->size);
    this->timestamp = timestamp;
}

AudioMixerTrack::~AudioMixerTrack() {
    if(this->data) {
        free(this->data);
    }
}

/**********************************************
 *
 * AudioMixerFifo implementation
 *
 **********************************************/
AudioMixerFifo::AudioMixerFifo(std::string indexId, int samplerate, int channels) {
    // create a fifo with 640k
    this->indexId = indexId;
    this->samplerate = samplerate;
    this->channels = channels;
    this->size = 640*1024;
    this->data = malloc(this->size);
    this->rp = 0;
    this->wp = 0;
}

AudioMixerFifo::~AudioMixerFifo() {
    if(this->data) {
        free(this->data);
    }
}

int AudioMixerFifo::write(void* data, int size) {
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    
    int avail = getBlankSize();
    int copySize = 0;
    if(avail > size) {
        copySize = size;
    } else {
        copySize = avail;
    }
    
    if(wp >= rp) {
        int tempSize = this->size - wp;
        if(tempSize >= copySize) {
            memcpy((void*)((uint8_t*)this->data + wp), data, copySize);
        } else {
            memcpy((void*)((uint8_t*)this->data + wp), data, tempSize);
            memcpy(this->data, (void*)((uint8_t*)data + tempSize), copySize-tempSize);
        }
        wp += copySize;
        wp %= this->size;
    } else {
        memcpy((void*)((uint8_t*)this->data + wp), data, copySize);
        wp += copySize;
    }
    
    return copySize;
}

int AudioMixerFifo::read(void* data, int size) {
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    int avail = getDataSize();
    
    int copySize = 0;
    if(avail > size) {
        copySize = size;
    } else {
        copySize = avail;
    }
    
    if(wp > rp) {
        memcpy(data, (void*)((uint8_t*)this->data + rp), copySize);
        rp += copySize;
    } else {
        int tempSize = this->size - rp;
        if(tempSize >= copySize) {
            memcpy(data, (void*)((uint8_t*)this->data + rp), copySize);
        } else {
            memcpy(data, (void*)((uint8_t*)this->data + rp), tempSize);
            memcpy((void*)((uint8_t*)data + tempSize), this->data, copySize - tempSize);
        }
        rp += copySize;
        rp %= this->size;
    }
    
    return copySize;
}

int AudioMixerFifo::getBlankSize() {
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    int avail = 0;
    if(rp == wp) {
        avail = this->size;
    } else if(rp > wp) {
        avail = rp - wp;
    } else if(rp < wp) {
        avail = this->size - wp + rp;
    }
    
    return avail - 16;
}

int AudioMixerFifo::getDataSize() {
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    int avail = 0;
    if(rp == wp) {
        avail = 0;
    } else if(rp > wp) {
        avail = this->size - rp + wp;
    } else if(rp < wp) {
        avail = wp - rp;
    }
    
    return avail;
}

void AudioMixerFifo::clean()
{
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    rp = wp = 0;
}

/**********************************************
 *
 * YouMeEngineAudioMixerUtils implementation
 *
 **********************************************/

YouMeEngineAudioMixerUtils *YouMeEngineAudioMixerUtils::mPInstance = NULL;
std::mutex YouMeEngineAudioMixerUtils::mInstanceMutex;

YouMeEngineAudioMixerUtils::YouMeEngineAudioMixerUtils() {
    m_resampler = new youme::webrtc::PushResampler<int16_t>();
    m_inputVolumeGain = 1.0f;
    m_trackVolumeGain = 1.0f;
}

YouMeEngineAudioMixerUtils::~YouMeEngineAudioMixerUtils() {
}

YouMeEngineAudioMixerUtils* YouMeEngineAudioMixerUtils::getInstance(void)
{
    if (NULL == mPInstance)
    {
        std::unique_lock<std::mutex> lck (mInstanceMutex);
        if (NULL == mPInstance)
        {
            mPInstance = new YouMeEngineAudioMixerUtils ();
        }
    }
    return mPInstance;
}

void YouMeEngineAudioMixerUtils::destroy ()
{
    TSK_DEBUG_INFO ("@@ destroy");
    if (mPInstance != nullptr) {
        std::unique_lock<std::mutex> lck (mInstanceMutex);
        delete mPInstance;
        mPInstance = NULL;
        TSK_DEBUG_INFO ("== destroy");
    }
}

std::shared_ptr<AudioMixerTrack> YouMeEngineAudioMixerUtils::mixAudio(std::list<std::shared_ptr<AudioMixerTrack>> audioTrackList) {
    int trackCount = audioTrackList.size();
    if(trackCount <= 0) {
        return NULL;
    }
    
    std::list<std::shared_ptr<AudioMixerTrack>>::iterator itor;
    itor = audioTrackList.begin();
    
    int size = (*itor)->size;
    int samples = size / sizeof(int16_t);
    int16_t* pInBufferPtr = (int16_t*)malloc(sizeof(int16_t) * trackCount);
    
    std::shared_ptr<AudioMixerTrack> audioTrackOut = std::shared_ptr<AudioMixerTrack>(new AudioMixerTrack(size));
    
    for(int i = 0; i < samples; i++) {
        int nTrackIdx = 0;
        itor = audioTrackList.begin();
        while(itor != audioTrackList.end()) {
            int16_t* inTrackData = (int16_t*)((*itor)->data);
            pInBufferPtr[nTrackIdx++] = inTrackData[i];
            itor++;
        }
        
        int16_t* outTrackData = (int16_t*)(audioTrackOut->data);
        outTrackData[i] = do_mixing(pInBufferPtr, trackCount);
    }
    
    if(pInBufferPtr) {
        free(pInBufferPtr);
    }
    
    return audioTrackOut;
}

YouMeErrorCode YouMeEngineAudioMixerUtils::setAudioMixerTrackSamplerate(int sampleRate) {
    switch (sampleRate) {
        case SAMPLE_RATE_8:
        case SAMPLE_RATE_16:
        case SAMPLE_RATE_24:
        case SAMPLE_RATE_32:
        case SAMPLE_RATE_44:
        case SAMPLE_RATE_48:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
            break;
    }
    m_audioTrackSamplerate = sampleRate;
    return YOUME_SUCCESS;
}

YouMeErrorCode YouMeEngineAudioMixerUtils::pushAudioMixerTrack(void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, bool bFloat, uint64_t timestamp) {
    //TSK_DEBUG_INFO("pushAudioMixerTrack pBuf:%p, nSizeInByte:%d, nChannelNUm:%d, nSampleRate:%d, nBytesPerSample:%d, bFloat:%d", pBuf, nSizeInByte);
    if(pBuf == NULL || nSizeInByte <= 0 || (nChannelNUm != 1 && nChannelNUm != 2) ) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    switch (nBytesPerSample) {
        case 1:
        case 2:
        case 4:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
            break;
    }
    switch (nSampleRate) {
        case SAMPLE_RATE_8:
        case SAMPLE_RATE_16:
        case SAMPLE_RATE_24:
        case SAMPLE_RATE_32:
        case SAMPLE_RATE_44:
        case SAMPLE_RATE_48:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
            break;
    }
    applyVolumeGain(pBuf, nSizeInByte, nBytesPerSample, m_trackVolumeGain);
    std::shared_ptr<AudioMixerTrack> audioTrack = std::shared_ptr<AudioMixerTrack>(new AudioMixerTrack(pBuf, nSizeInByte, nChannelNUm, timestamp));
    std::list<std::shared_ptr<AudioMixerTrack>> audioTrackList;
    audioTrackList.push_back(std::shared_ptr<AudioMixerTrack>(audioTrack));
    // mix audio
    std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
    std::list<std::shared_ptr<AudioMixerFifo>>::iterator itor;
    itor = audioFifoList.begin();
    while(itor != audioFifoList.end()) {
        std::shared_ptr<AudioMixerTrack> audioTrack_temp = std::shared_ptr<AudioMixerTrack>(new AudioMixerTrack(nSizeInByte));
        int readSize = (*itor)->read(audioTrack_temp->data, audioTrack_temp->size);
        
        audioTrack_temp->size = readSize;
        audioTrackList.push_back(audioTrack_temp);
        itor++;
    }
    
    //TMEDIA_DUMP_TO_FILE("/pcm_data_48_input.pcm", audioTrack->data, audioTrack->size);
    std::shared_ptr<AudioMixerTrack> audioTrackOut = mixAudio(audioTrackList);
    //TMEDIA_DUMP_TO_FILE("/pcm_data_48_output.pcm", audioTrackOut->data, audioTrackOut->size);
    
    memcpy(pBuf, audioTrackOut->data, nSizeInByte);
    audioTrackList.clear();
    return YOUME_SUCCESS;
}

std::shared_ptr<AudioMixerFifo> YouMeEngineAudioMixerUtils::getAudioMixerFifo(std::string indexId) {
    std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
    std::shared_ptr<AudioMixerFifo> audioFifo = NULL;
    std::list<std::shared_ptr<AudioMixerFifo>>::iterator itor;
    itor = audioFifoList.begin();
    while(itor != audioFifoList.end()) {
        if((*itor)->indexId == indexId) {
            audioFifo = (*itor);
            break;
        }
        itor++;
    }
    return audioFifo;
}

int YouMeEngineAudioMixerUtils::addNewAudioMixerFifo(std::string indexId, int samplerate, int channels) {
    std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
    TSK_DEBUG_INFO("addAudioMixerFifo(indexId:%s, samplerate:%d, channels:%d)", indexId.c_str(), samplerate, channels);
    
    //add local user clean cache buffer
    if (auido_clear_index == indexId)
    {
        std::list<std::shared_ptr<AudioMixerFifo>>::iterator itor;
        itor = audioFifoList.begin();
        while(itor != audioFifoList.end()) {
            (*itor)->clean();
            itor++;
        }
    }
    
    if(NULL == getAudioMixerFifo(indexId)) {
        std::shared_ptr<AudioMixerFifo> audioFifo = std::shared_ptr<AudioMixerFifo>(new AudioMixerFifo(indexId, samplerate, channels));
        audioFifoList.push_back(audioFifo);
    }
    
    return 0;
}

void YouMeEngineAudioMixerUtils::applyVolumeGain(void* pBuf, int nBytes, int nBytesPerSample, float volumeGain)
{
    if ((volumeGain < 0.0f) || (1.0f == volumeGain)) {
        return;
    }
    
    if (2 == nBytesPerSample) {
        uint8_t *pData = (uint8_t*)pBuf;
        int pcmMin = - (1 << 15);
        int pcmMax = (1 << 15) - 1;
        int i=0;
        for (i = 0; i < nBytes - 1; i += 2) {
            short oldValue = 0;
            short newValue;
            int   tempValue;
            
            oldValue |= ((pData[i]) & 0xFF);
            oldValue |= ((pData[i + 1] << 8) & 0xFF00);
            if (oldValue >= 0) {
                tempValue = (int)(oldValue * volumeGain + 0.5f);
            } else {
                tempValue = (int)(oldValue * volumeGain - 0.5f);
            }
            
            if(tempValue > pcmMax) {
                newValue = (short)pcmMax;
            } else if(tempValue < pcmMin) {
                newValue = (short)pcmMin;
            } else {
                newValue = (short)tempValue;
            }
            pData[i] = (uint8_t) (newValue & 0xFF);
            pData[i + 1] = (uint8_t) ((newValue & 0xFF00) >> 8);
        }
    } else if (1 == nBytesPerSample) {
        int8_t *pData = (int8_t*)pBuf;
        int pcmMin = - (1 << 7);
        int pcmMax = (1 << 7) - 1;
        int i = 0;
        for (i = 0; i < nBytes; i++) {
            int tempValue;
            if (pData[i] >= 0) {
                tempValue= (int)(pData[i] * volumeGain + 0.5f);
            } else {
                tempValue= (int)(pData[i] * volumeGain - 0.5f);
            }
            
            if(tempValue > pcmMax) {
                pData[i] = (int8_t)pcmMax;
            } else if(tempValue < pcmMin) {
                pData[i]  = (int8_t)pcmMin;
            } else {
                pData[i]  = (int8_t)tempValue;
            }
        }
    }
}

YouMeErrorCode YouMeEngineAudioMixerUtils::setAudioMixerTrackVolume(int volume) {
    if(volume > 500 || volume < 0) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    m_trackVolumeGain = (float)volume/100;
    return YOUME_SUCCESS;
}

YouMeErrorCode YouMeEngineAudioMixerUtils::setAudioMixerInputVolume(int volume) {
    if(volume > 500 || volume < 0) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    m_inputVolumeGain = (float)volume/100;
    return YOUME_SUCCESS;
}

YouMeErrorCode YouMeEngineAudioMixerUtils::inputAudioToMix(std::string indexId, void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, bool bFloat, uint64_t timestamp) {
    //TSK_DEBUG_INFO("inputAudioToMix indexId:%s, pBuf:%p, nSizeInByte:%d, nChannelNUm:%d, nSampleRate:%d, nBytesPerSample:%d, bFloat:%d", indexId.c_str(), pBuf, nSizeInByte);
    if(indexId.empty() || pBuf == NULL || nSizeInByte <= 0 || (nChannelNUm != 1 && nChannelNUm != 2) ) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    switch (nBytesPerSample) {
        case 1:
        case 2:
        case 4:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
            break;
    }
    switch (nSampleRate) {
        case SAMPLE_RATE_8:
        case SAMPLE_RATE_16:
        case SAMPLE_RATE_24:
        case SAMPLE_RATE_32:
        case SAMPLE_RATE_44:
        case SAMPLE_RATE_48:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
            break;
    }
    if(!indexId.empty()) {
        // ===== Pre-processing start ===== //
        void *pcm_data = (void *)tsk_malloc(nSizeInByte);
        memcpy(pcm_data, pBuf, nSizeInByte);
        uint8_t bytes_per_sample = (uint8_t)nBytesPerSample;
        uint8_t channel_num = (uint8_t)nChannelNUm;
        uint8_t is_float = bFloat ? 1 : 0;
        uint32_t pcm_size = nSizeInByte;
        uint32_t sample_rate = nSampleRate;
        
        int32_t i2SampleNumTotal = pcm_size / bytes_per_sample;
        int32_t i2SampleNumPerCh = i2SampleNumTotal / channel_num;
        int16_t *pi2Buf1 = (int16_t *)tsk_malloc(pcm_size);
        // Float transfer
        tdav_codec_float_to_int16 (pcm_data, pi2Buf1, &(bytes_per_sample), &(pcm_size), is_float);
        
        if (nChannelNUm == 2) {
            //TSK_DEBUG_INFO("need 2 channel to 1 ");
            for (size_t i = 0; i < i2SampleNumPerCh; i++) {
                pi2Buf1[i] = (pi2Buf1[2 * i] >> 1) + (pi2Buf1[2 * i + 1] >> 1) ;
            }
            pcm_size = i2SampleNumPerCh * sizeof(int16_t);
            int16_t *pcm_temp_data = (int16_t *)tsk_realloc(pcm_data, pcm_size);
            if (tsk_null != pcm_temp_data) {
                pcm_data = pcm_temp_data;
            }
        }

        memcpy(pcm_data, pi2Buf1, pcm_size);
        
        TSK_SAFE_FREE(pi2Buf1);
        // ===== Pre-processing end ===== //
        
        int16_t* outData = NULL;
        int outLength = 0;
        bool needRelease = false;
        if(nSampleRate != m_audioTrackSamplerate && m_resampler != nullptr) {
            int inLength = pcm_size;
            int16_t* inData = (int16_t*)pcm_data;
            
            outLength = pcm_size * (float)m_audioTrackSamplerate / sample_rate;
            // webrtc resample maybe crash when buffer size don't match sample
            while(1) {
                float temp = (float)outLength * sample_rate / m_audioTrackSamplerate;
                if (temp - (int)temp == 0){
                    break;
                }
                outLength++;
            }

            outData = (int16_t*)tsk_malloc(outLength);
            memset(outData, 0, outLength);
            uint32_t in_samples  = inLength / sizeof(int16_t);
            //uint32_t out_samples = outLength / sizeof(int16_t);
            
            //TSK_DEBUG_INFO("inLength:%d, outLength:%d, in_samples:%d", inLength, outLength, in_samples);
            
            m_resampler->InitializeIfNeeded(sample_rate, m_audioTrackSamplerate, 1);
            
            int src_nb_samples_per_process = ((m_resampler->GetSrcSampleRateHz() * 10) / 1000);
            int dst_nb_samples_per_process = ((m_resampler->GetDstSampleRateHz() * 10) / 1000);
            
            for (int i = 0; i * src_nb_samples_per_process < in_samples; i++) {
                int process_sample = src_nb_samples_per_process<in_samples-i*src_nb_samples_per_process?src_nb_samples_per_process:in_samples-i*src_nb_samples_per_process;
                //TSK_DEBUG_INFO("for loop process_sample:%d,  i:%d, i * src_nb_samples_per_process:%d", process_sample, i, i * src_nb_samples_per_process);
                int result = m_resampler->Resample((const int16_t*)inData + i * src_nb_samples_per_process, process_sample, (int16_t*)outData + i * dst_nb_samples_per_process, 0);
                //TSK_DEBUG_INFO("for loop result:%d", result);
            }
            needRelease = true;
        }else {
            outData = (int16_t*)pcm_data;
            outLength = pcm_size;
        }
        
        applyVolumeGain(outData, outLength, bytes_per_sample, m_inputVolumeGain);
        
        std::shared_ptr<AudioMixerFifo> audioFifo = YouMeEngineAudioMixerUtils::getInstance()->getAudioMixerFifo(indexId);
        if(NULL == audioFifo) {
            addNewAudioMixerFifo(indexId, sample_rate, 1);
        }
        
        audioFifo = YouMeEngineAudioMixerUtils::getInstance()->getAudioMixerFifo(indexId);
        
        int writeSize = audioFifo->write(outData, outLength);
        
        if(needRelease) {
            TSK_FREE(outData);
        }
        
        TSK_FREE(pcm_data);
    }
    return YOUME_SUCCESS;
}

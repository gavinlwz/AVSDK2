//
//  YouMeEngineManagerForQiniu.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/8/4.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <StringUtil.hpp>
#include "tsk_debug.h"
#include "../java/jni/AudioMgr.h"
#include "tmedia_utils.h"
#include "YouMeEngineManagerForQiniuCWrapper.h"
#include "NgnVideoManager.hpp"
#include "NgnTalkManager.hpp"
#include "ImageUtils.hpp"
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "YouMeEngineManagerForQiniu.hpp"
#include "YouMeVideoMixerAdapter.h"
#include "TLSMemory.h"

#define    auido_local_userid            "0"
#define    video_recv_cache_size          3

using namespace youme_voice_engine;

/**********************************************
 *
 * Frame implementation
 *
 **********************************************/
Frame::Frame(int width, int height, int format) {
    this->len = width * height * 3 / 2;
    //this->data = malloc(this->len);
    this->data = (void*)CTLSMemory::GetInstance()->Allocate(len, this->iSlot);
    this->width = width;
    this->height = height;
    this->format = format;
}

Frame::Frame(void* data, int len, int width, int height, int format, int mirror, uint64_t timestamp) {
    //this->data = malloc(len);
    this->data = (void*)CTLSMemory::GetInstance()->Allocate(len, this->iSlot);
    memcpy(this->data, data, len);
    this->len = len;
    this->width = width;
    this->height = height;
    this->format = format;
    this->timestamp = timestamp;
    this->mirror = mirror;
}

Frame::~Frame() {
    if(this->data)
        CTLSMemory::GetInstance()->Free(this->iSlot);
    //free(this->data);
}

void Frame::resize(int width, int height) {
}

//y->0  uv->128
void Frame::clear()
{
    if(!data || !width || !height)
        return;
    
    memset((char*)data, 0, width*height);
    memset((char*)data+width*height, 128, width*height/2);
}

/**********************************************
 *
 * AudioTrack implementation
 *
 **********************************************/
AudioTrack::AudioTrack(int size) {
    this->size = size;
    this->data = malloc(this->size);
    memset(this->data, 0, this->size);
}

AudioTrack::AudioTrack(void* data, int size, int samplerate, int channels, uint64_t timestamp) {
    this->samplerate = samplerate;
    this->channels = channels;
    this->size = size;
    this->data = malloc(this->size);
    memcpy(this->data, data, this->size);
    this->timestamp = timestamp;
}

AudioTrack::AudioTrack(int size, int samplerate, int channels, uint64_t timestamp) {
    this->samplerate = samplerate;
    this->channels = channels;
    this->size = size;
    this->data = malloc(this->size);
    memset(this->data, 0, this->size);
    this->timestamp = timestamp;
}

AudioTrack::~AudioTrack() {
    if(this->data) {
        free(this->data);
    }
}

/**********************************************
 *
 * AudioFifo implementation
 *
 **********************************************/
AudioFifo::AudioFifo(std::string userId, int samplerate, int channels) {
    // create a fifo with 64k
    this->userId = userId;
    this->samplerate = samplerate;
    this->channels = channels;
    this->size = 64*1024;
    this->data = malloc(this->size);
    this->rp = 0;
    this->wp = 0;
}

AudioFifo::~AudioFifo() {
    if(this->data) {
        free(this->data);
    }
}

int AudioFifo::write(void* data, int size) {
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

int AudioFifo::read(void* data, int size) {
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

int AudioFifo::getBlankSize() {
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

int AudioFifo::getDataSize() {
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

void AudioFifo::clean()
{
    std::lock_guard<std::recursive_mutex> stateLock(this->mutex);
    rp = wp = 0;
}

/**********************************************
 *
 * MixingInfo implementation
 *
 **********************************************/
MixingInfo::MixingInfo(std::string userId, int x, int y, int z, int width, int height) {
    this->userId = userId;
    this->x = x;
    this->y = y;
    this->z = z;
    this->width = width;
    this->height = height;
    this->isMainVideo = true;
    this->freezedFrame = NULL;
}

MixingInfo::~MixingInfo() {
    frameList.clear();
}

//重载<
bool MixingInfo::operator<(const MixingInfo& mixingInfo)
{
    if(this->z<mixingInfo.z)
    {
        return true;
    }

    return false;
}

void MixingInfo::pushFrame(std::shared_ptr<Frame> frame) {
    std::lock_guard<std::recursive_mutex> stateLock(this->frameListMutex);
    if(this->frameList.size() >= 10){//最多允许积累10个数据
        this->frameList.clear();
    }
    this->frameList.push_back(frame);
}

std::shared_ptr<Frame> MixingInfo::popFrame() {
    std::lock_guard<std::recursive_mutex> stateLock(this->frameListMutex);
    if(this->frameList.size() > 0) {
        std::shared_ptr<Frame> frame = this->frameList.front();
        this->freezedFrame = frame;
        this->frameList.pop_front();
        return frame;
    } else {
        return NULL;
    }
}

std::shared_ptr<Frame> MixingInfo::popFrame(uint64_t timestamp, long delta) {
    std::lock_guard<std::recursive_mutex> stateLock(this->frameListMutex);
    long minTs = timestamp - delta;
    long maxTs = timestamp + delta;
    
    std::shared_ptr<Frame> frame = NULL;
    std::list<std::shared_ptr<Frame>>::iterator itor;
    itor = frameList.begin();
    while(itor != frameList.end()) {
        if((*itor)->timestamp < minTs) {
            frameList.erase(itor);
            continue;
        } else if(((*itor)->timestamp >= minTs) && ((*itor)->timestamp <= maxTs)) {
            frame = (*itor);
            this->freezedFrame = frame;
            break;
        } else {
            break;
        }
    }
    return frame;
}

void MixingInfo::clean()
{
    std::lock_guard<std::recursive_mutex> stateLock(this->frameListMutex);
    frameList.clear();
}



/**********************************************
 *
 * YouMeEngineManagerForQiniu implementation
 *
 **********************************************/

std::recursive_mutex* mutex = new std::recursive_mutex();
YouMeEngineManagerForQiniu* instance = NULL;

YouMeEngineManagerForQiniu::YouMeEngineManagerForQiniu() {
    m_localVideoNoNull = false;
    //mMixedFrame = new Frame(320, 480, 0);
    mMixedFrame = NULL;
    m_mixing = false;
    m_isWaitMixing = false;
    m_lastTimestamp = 0;
    m_frameCount = 0;
    startThread();
    startAudioMixingThread();
}

YouMeEngineManagerForQiniu::~YouMeEngineManagerForQiniu() {
    stopThread();
    stopAudioMixingThread();
    if(mMixedFrame)
       delete mMixedFrame;
    instance = nullptr;
}

YouMeEngineManagerForQiniu* YouMeEngineManagerForQiniu::getInstance(void)
{
    if (instance == NULL) {
        std::lock_guard<std::recursive_mutex> stateLock(*mutex);
        if(instance == NULL)
           instance = new (std::nothrow) YouMeEngineManagerForQiniu();
    }
    return instance;
}

void YouMeEngineManagerForQiniu::frameRender(std::string userId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void * buf, uint64_t timestamp) {

    if(!m_localVideoNoNull)
        return;
    std::shared_ptr<MixingInfo> mixingInfo = getMixingInfo(userId);
    if(NULL != mixingInfo) {
        if (mixingInfo->frameList.size() >= video_recv_cache_size) {
            return;
        }
        std::shared_ptr<Frame> frame = std::shared_ptr<Frame>(new Frame((void*)buf, nBufSize, nWidth, nHeight, 0, 0, timestamp));
        mixingInfo->pushFrame(frame);
    }
}

//void YouMeEngineManagerForQiniu::onVideoFrameCallback(std::string userId, const void * data, int len, int width, int height, int fmt, uint64_t timestamp) {
//#ifdef ANDROID
//    JNI_onVideoFrameCallbackID(userId, data, len, width, height, fmt, timestamp);
//#else
//    if(videoFrameCallback) {
//        videoFrameCallback->onVideoFrameCallback(userId, (void*)data, len, width, height, fmt, timestamp);
//    }
//#endif
//}
//
//void YouMeEngineManagerForQiniu::onVideoFrameMixedCallback(const void * data, int len, int width, int height, int fmt, uint64_t timestamp) {
//#ifdef ANDROID
//    JNI_onVideoFrameMixedCallbackID(data, len, width, height, fmt, timestamp);
//#else
//    if(videoFrameCallback) {
//        videoFrameCallback->onVideoFrameMixedCallback((void*)data, len, width, height, fmt, timestamp);
//    }
//#endif
//}

void YouMeEngineManagerForQiniu::onAudioFrameCallback(std::string userId, void* data, int len, uint64_t timestamp) {
#ifdef ANDROID
    JNI_onAudioFrameCallbackID(userId, data, len, timestamp);
#else
    if(audioFrameCallback) {
        audioFrameCallback->onAudioFrameCallback(userId.c_str(), data, len, timestamp);
    }
#endif
}

void YouMeEngineManagerForQiniu::onAudioFrameMixedCallback(void* data, int len, uint64_t timestamp) {
#ifdef ANDROID
    JNI_onAudioFrameMixedCallbackID(data, len, timestamp);
#else
    if(audioFrameCallback) {
        audioFrameCallback->onAudioFrameMixedCallback(data, len, timestamp);
    }
#endif
    //TSK_DEBUG_INFO("YouMeEngineManagerForQiniu: onAudioFrameMixedCallback timestamp:%lld\n",timestamp);
}


void YouMeEngineManagerForQiniu::startThread() {
    m_isLooping = true;
    m_thread = std::thread(&YouMeEngineManagerForQiniu::threadFunc, this);
}

void YouMeEngineManagerForQiniu::stopThread() {
    if (m_thread.joinable()) {
        if (std::this_thread::get_id() != m_thread.get_id()) {
            m_isLooping = false;
            //mixing_condWait.SetSignal();
            m_msgQueueMutex.lock();
            m_msgQueueCond.notify_all();
            m_msgQueueMutex.unlock();
            TSK_DEBUG_INFO("Start joining thread");
            m_thread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            m_thread.detach();
        }
        ClearMessageQueue();
    }
}

void YouMeEngineManagerForQiniu::ClearMessageQueue() {
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
    std::deque<Frame*>::iterator it;
    while (!m_msgQueue.empty()) {
        Frame* pMsg = m_msgQueue.front();
        m_msgQueue.pop_front();
        if (pMsg) {
            delete pMsg;
            pMsg = NULL;
        }
    }
}

void YouMeEngineManagerForQiniu::startAudioMixingThread() {
    m_isLooping_audio_mixing = true;
    m_thread_audio_mixing = std::thread(&YouMeEngineManagerForQiniu::audioMixingThreadFunc, this);
}

void YouMeEngineManagerForQiniu::stopAudioMixingThread() {
    if (m_thread_audio_mixing.joinable()) {
        if (std::this_thread::get_id() != m_thread_audio_mixing.get_id()) {
            m_isLooping_audio_mixing = false;
            //mixing_condWait.SetSignal();
            m_msgQueueMutex_audio_mixing.lock();
            m_msgQueueCond_audio_mixing.notify_all();
            m_msgQueueMutex_audio_mixing.unlock();
            TSK_DEBUG_INFO("Start joining thread");
            m_thread_audio_mixing.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            m_thread_audio_mixing.detach();
        }
        ClearAudioMixingMessageQueue();
    }
}

void YouMeEngineManagerForQiniu::ClearAudioMixingMessageQueue() {
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex_audio_mixing);
    std::deque<Frame*>::iterator it;
    while (!m_msgQueue_audio_mixing.empty()) {
    }
}

#if 0
void YouMeEngineManagerForQiniu::mixVideo(Frame* input, int x, int y, int width, int height) {
    Image* image = new Image(input->width, input->height, input->data);
    Image* src = ImageUtils::centerScale(image, width, height);
    
    x = (x >> 2) << 2;
    y = (y >> 2) << 2;
    
    Frame* dest = this->mMixedFrame;
    
    uint8_t* ydest = (uint8_t*)dest->data;
    uint8_t* udest = ydest + dest->width * dest->height;
    uint8_t* vdest = ydest + dest->width * dest->height * 5 / 4;
    
    uint8_t* ysrc = (uint8_t*)src->data;;
    uint8_t* usrc = ysrc + src->width * src->height;;
    uint8_t* vsrc = ysrc + src->width * src->height * 5 / 4;;
    
    // yplane
    for(int row = 0; row < src->height; row++) {
        for(int column = 0; column < src->width; column++) {
            if(((row + y) >= 0) && ((row + y) < dest->height) && ((column + x) >= 0) && ((column + x) < dest->width)) {
                ydest[(row + y) * dest->width + (column + x)] = ysrc[row * src->width + column];
            }
        }
    }

    // uplane
    for(int row = 0; row < src->height/2; row++) {
        for(int column = 0; column < src->width/2; column++) {
            if(((row + y/2) >= 0) && ((row + y/2) < dest->height/2) && ((column + x/2) >= 0) && ((column + x/2) < dest->width/2)) {
                udest[(row + y/2) * dest->width/2 + (column + x/2)] = usrc[row * src->width/2 + column];
            }
        }
    }

    // vplane
    for(int row = 0; row < src->height/2; row++) {
        for(int column = 0; column < src->width/2; column++) {
            if(((row + y/2) >= 0) && ((row + y/2) < dest->height/2) && ((column + x/2) >= 0) && ((column + x/2) < dest->width/2)) {
                vdest[(row + y/2) * dest->width/2 + (column + x/2)] = vsrc[row * src->width/2 + column];
            }
        }
    }
    
    delete image;
    delete src;
}
#else


void YouMeEngineManagerForQiniu::mixVideo(Frame* input, int x, int y, int width, int height) {
    
    Frame* dest = this->mMixedFrame;
    if (x == 0 &&
        y == 0 &&
        width == input->width &&
        height == input->height &&
        width == dest->width &&
        height == dest->height) {
        memcpy(dest->data, input->data, width*height*3/2);
        return;
    }
    Image* src = new Image(width, height);
    if(width != input->width || height != input->height){
        ImageUtils::centerScale_new((char *)input->data, input->width, input->height, (char *)src->data, width, height);
    }else{
        memcpy(src->data, input->data, width*height*3/2);
    }
//    if (x == 0 && y == 0 && width == dest->width && height == dest->height) {
//        memcpy(dest->data, src->data, width*height*3/2);
//        delete src;
//        return;
//    }
    // 当分辨率为非被四整除偶数时，画面布局会出现黑边
    x = (x >> 1) << 1;
    y = (y >> 1) << 1;
    
    int src_left = 0, src_top = 0, src_w = width, src_h = height;
    int dest_left = x, dest_top = y;
    int dest_stride_y = dest->width;
    int dest_stride_uv = dest->width/2;
    int src_stride_y = width;
    int src_stride_uv = width/2;
    if (x < 0) {
        src_left = -x;
        src_w = width - src_left;
        dest_left = 0;
        
    }
    if (y < 0) {
        src_top = -y;
        src_h = height - src_top;
        dest_top = 0;
    }
    
    if(dest_left > dest->width || dest_top > dest->height) {
        delete src;
        return;
    }  
    
    if (dest_left + src_w > dest->width) {
        src_w = dest->width - dest_left;
    }
    if (dest_top + src_h > dest->height) {
        src_h = dest->height - dest_top;
    }
    
    uint8_t* ydest = (uint8_t*)dest->data;
    uint8_t* udest = ydest + dest->width * dest->height;
    uint8_t* vdest = ydest + dest->width * dest->height * 5 / 4;
    
    uint8_t* ysrc = (uint8_t*)src->data;;
    uint8_t* usrc = ysrc + src->width * src->height;;
    uint8_t* vsrc = ysrc + src->width * src->height * 5 / 4;
    
    int dest_row = dest_top;
    // yplane
    for(int row = src_top; row < src_h; ++row, ++dest_row) {
        memcpy(ydest + dest_row*dest_stride_y + dest_left, ysrc + row*src_stride_y + src_left, src_w);
    }
    
    dest_row = dest_top/2;
    // uplane、vplane
    for(int row = src_top/2; row < src_h/2; ++row, ++dest_row) {
        memcpy(udest + dest_row*dest_stride_uv + dest_left/2, usrc + row*src_stride_uv + src_left/2, src_w/2);
        memcpy(vdest + dest_row*dest_stride_uv + dest_left/2, vsrc + row*src_stride_uv + src_left/2, src_w/2);
    }
    delete src;
}

#endif

std::shared_ptr<MixingInfo> YouMeEngineManagerForQiniu::getMixingInfo(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    std::shared_ptr<MixingInfo> mixingInfo = NULL;
    std::list<std::shared_ptr<MixingInfo>>::iterator itor;
    itor = mixingInfoList.begin();
    while(itor != mixingInfoList.end()) {
        if((*itor)->userId == userId) {
            mixingInfo = (*itor);
            break;
        }
        itor++;
    }
    
    return mixingInfo;
}


void YouMeEngineManagerForQiniu::threadFunc() {
    TSK_DEBUG_INFO("YouMeEngineManagerForQiniu::threadFunc() thread enters.");
    
    while (m_isLooping) {

        std::unique_lock<std::mutex> queueLock(m_msgQueueMutex);
        while (m_isLooping && m_msgQueue.empty()) {
            m_msgQueueCond.wait(queueLock);
        }
        
        if (!m_isLooping) {
            break;
        }

        Frame* frame = m_msgQueue.front();
        m_msgQueue.pop_front();
        queueLock.unlock();
        std::shared_ptr<MixingInfo> mixingInfo = NULL;
        // handle frame
        // get main mixingInfo
        bool added = false;
        if (!mMixedFrame || (!m_mixSizeOutSet && (frame->width != mMixedFrame->width || frame->height != mMixedFrame->height))) {
            setMixVideoSizeNew(frame->width, frame->height);
            added = true;
        }
        if(added || !needMixing(CNgnTalkManager::getInstance()->m_strUserID)){
            addMixOverlayVideo(CNgnTalkManager::getInstance()->m_strUserID, 0, 0, 0, frame->width, frame->height);
        }
        isWaitMixing();
        {
            m_mixing = true;
            std::lock_guard<std::recursive_mutex> stateLock(*mutex);
            //----------
            if(mixingInfoList.size() == 0)
                m_mixing = false;
        }
        if(mMixedFrame)
           mMixedFrame->clear();
        else{
           m_mixing = false;
           continue;
        }
        std::list<std::shared_ptr<MixingInfo>>::iterator itor;
        itor = mixingInfoList.begin();
        while(itor != mixingInfoList.end()) {
            mixingInfo = (*itor);
            if(mixingInfo->userId == CNgnTalkManager::getInstance()->m_strUserID) {
                // local video
                if (frame->mirror == YOUME_VIDEO_MIRROR_MODE_ENABLED || frame->mirror == YOUME_VIDEO_MIRROR_MODE_NEAR) {
                    ICameraManager::getInstance()->mirror((uint8_t*)frame->data, frame->width, frame->height);
                }
                mixVideo(frame, mixingInfo->x, mixingInfo->y, mixingInfo->width, mixingInfo->height);
            } else {
                // remote video
                {
                    uint64_t pts;
                    std::shared_ptr<Frame> frame1 = mixingInfo->popFrame();
                    if (NULL != frame1) {
                        pts = frame1->timestamp;
                        mixVideo(frame1.get(), mixingInfo->x, mixingInfo->y, mixingInfo->width, mixingInfo->height);
                    } else if (NULL != mixingInfo->freezedFrame) {
                        pts = mixingInfo->freezedFrame.get()->timestamp;
                        mixVideo(mixingInfo->freezedFrame.get(), mixingInfo->x, mixingInfo->y, mixingInfo->width, mixingInfo->height);
                    }
                }
            }
            itor++;
        }
        YouMeVideoMixerAdapter::getInstance()->pushVideoFrameMixedCallback(mMixedFrame->data, mMixedFrame->len, mMixedFrame->width, mMixedFrame->height, VIDEO_FMT_YUV420P, frame->timestamp);
        
        delete frame;
        frame = NULL;
        m_mixing = false;
        
    }
    TSK_DEBUG_INFO("YouMeEngineManagerForQiniu::threadFunc() thread exits");
}

std::shared_ptr<AudioTrack> YouMeEngineManagerForQiniu::mixAudio(std::list<std::shared_ptr<AudioTrack>> audioTrackList) {
    int trackCount = audioTrackList.size();
    if(trackCount <= 0) {
        return NULL;
    }
    
    std::list<std::shared_ptr<AudioTrack>>::iterator itor;
    itor = audioTrackList.begin();
    
    int size = (*itor)->size;
    int samples = size / sizeof(int16_t);
    int16_t* pInBufferPtr = (int16_t*)malloc(sizeof(int16_t) * trackCount);
    
    std::shared_ptr<AudioTrack> audioTrackOut = std::shared_ptr<AudioTrack>(new AudioTrack(size));
    
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
     
    
    //TMEDIA_DUMP_TO_FILE("/pcm_data_48_output_1.bin", audioTrackOut->data, audioTrackOut->size);
    
    return audioTrackOut;
}

void YouMeEngineManagerForQiniu::audioMixingThreadFunc() {
    TSK_DEBUG_INFO("YouMeEngineManagerForQiniu::audioMixingThreadFunc() thread enters.");
    
    while (m_isLooping_audio_mixing) {
        //TSK_DEBUG_INFO("YouMeEngineManagerForQiniu::audioMixingThreadFunc() thread is running.");
        
        std::unique_lock<std::mutex> queueLock(m_msgQueueMutex_audio_mixing);
        while (m_isLooping_audio_mixing && m_msgQueue_audio_mixing.empty()) {
            m_msgQueueCond_audio_mixing.wait(queueLock);
        }
        
        if (!m_isLooping_audio_mixing) {
            break;
        }
        
        AudioTrack* audioTrack = m_msgQueue_audio_mixing.front();
        m_msgQueue_audio_mixing.pop_front();
        queueLock.unlock();
        
        int size = audioTrack->size;
        std::list<std::shared_ptr<AudioTrack>> audioTrackList;
        audioTrackList.push_back(std::shared_ptr<AudioTrack>(audioTrack));
        // mix audio
        std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
        std::list<std::shared_ptr<AudioFifo>>::iterator itor;
        itor = audioFifoList.begin();
        while(itor != audioFifoList.end()) {
            std::shared_ptr<AudioTrack> audioTrack_temp = std::shared_ptr<AudioTrack>(new AudioTrack(size));
            int readSize = (*itor)->read(audioTrack_temp->data, audioTrack_temp->size);
            
            //TMEDIA_DUMP_TO_FILE("/pcm_data_48_read.bin", audioTrack_temp->data, audioTrack_temp->size);
            //readSize != size ??
            audioTrack_temp->size = readSize;
            audioTrackList.push_back(audioTrack_temp);
            itor++;
        }
        
        
        //TMEDIA_DUMP_TO_FILE("/pcm_data_48_input.bin", audioTrack->data, audioTrack->size);
        std::shared_ptr<AudioTrack> audioTrackOut = mixAudio(audioTrackList);
        //TMEDIA_DUMP_TO_FILE("/pcm_data_48_output.bin", audioTrackOut->data, audioTrackOut->size);
        
        /// process audio data: mix them sample by sample
        onAudioFrameMixedCallback(audioTrackOut->data, audioTrackOut->size, audioTrack->timestamp);
        
        audioTrackList.clear();
        
        //delete audioTrack;
        //audioTrack = NULL;
    }
    
    TSK_DEBUG_INFO("YouMeEngineManagerForQiniu::audioMixingThreadFunc() thread exits");
}

void YouMeEngineManagerForQiniu::setPreviewFps(uint32_t fps) {
    m_previewFps = fps;
}

bool YouMeEngineManagerForQiniu::dropFrame(){
    bool isDrop = true;
    uint64_t newTime = tsk_time_now();
    
    if ((newTime - m_lastTimestamp + 5)/(1000.0f/m_previewFps) >= m_frameCount && m_frameCount < m_previewFps)
    {
        m_frameCount++;
        isDrop = false;
    }
    if (newTime - m_lastTimestamp >= 1000)
    {
        m_lastTimestamp = newTime;
        m_frameCount = 0;
    }
    return isDrop;
}

void YouMeEngineManagerForQiniu::pushFrame(Frame* frame) {
#if defined(WIN32) || defined(ANDROID)
    if(dropFrame()) {
        TMEDIA_I_AM_ACTIVE(150,"@@ local video drop pts:%lld", frame->timestamp);
        delete frame;
        return;
    }
#endif

    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
    if(m_msgQueue.size() >= video_recv_cache_size){//最多允许积累3个数据
        TSK_DEBUG_INFO("pushFrame cleared");
        std::deque<Frame*>::iterator it = m_msgQueue.begin();
        for(it = m_msgQueue.begin();
            it != m_msgQueue.end();
            it++)
        {
            delete (*it);
        }
        m_msgQueue.clear();
    }
 
    m_msgQueue.push_back(frame);
    m_msgQueueCond.notify_one();
    if(!m_localVideoNoNull)
        m_localVideoNoNull = true;
    //TSK_DEBUG_INFO("qiniu video local ts:%lld, delay:%d(ms)", frame->timestamp, m_msgQueue.size()*66);
}
void  YouMeEngineManagerForQiniu::setMixVideoSizeNew(int width, int height){
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->mMixedFrame == NULL) {
        mMixedFrame = new Frame(width, height, 0);
    } else {
        if((this->mMixedFrame->width != width) || (this->mMixedFrame->height != height)) {
            delete mMixedFrame;
            mMixedFrame = new Frame(width, height, 0);
        }
    }
}
void YouMeEngineManagerForQiniu::setMixVideoSize(int width, int height) {
    TSK_DEBUG_INFO("setMixVideoSize,w:%d h:%d",width,height);
    setMixVideoSizeNew(width, height);
    m_mixSizeOutSet = true;
    TSK_DEBUG_INFO("Leave setMixVideoSize");
   
}

void YouMeEngineManagerForQiniu::addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height) {
    waitMixing();
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    TSK_DEBUG_INFO("addMixOverlayVideo,x:%d y:%d z:%d w:%d h:%d u:%s",x,y,z,width,height,userId.c_str());

    std::list<std::shared_ptr<MixingInfo>>::iterator itor;

    itor = mixingInfoList.begin();
    while(itor != mixingInfoList.end()) {
        if((*itor)->userId == userId) {
            mixingInfoList.erase(itor);
            break;
        }
        itor++;
    }
    
    std::shared_ptr<MixingInfo> mixingInfo = std::shared_ptr<MixingInfo>(new MixingInfo(userId.c_str(), x, y, z, width, height));
//    std::shared_ptr<Frame> frame = std::shared_ptr<Frame>(new Frame(width, height, 0));
//    frame->black();
//    mixingInfo->freezedFrame = frame;
    
    if(mixingInfoList.size() == 0) {
        mixingInfoList.push_back(mixingInfo);
    } else {
        for(itor = mixingInfoList.begin(); ; itor++) {
            if(itor == mixingInfoList.end()) {
                mixingInfoList.push_back(mixingInfo);
                break;
            } else if(mixingInfo->z < (*itor)->z) {
                mixingInfoList.insert(itor, mixingInfo);
                break;
            }
        }
    }
    

    TSK_DEBUG_INFO("Leave addMixOverlayVideo");
    
}

void YouMeEngineManagerForQiniu::removeMixOverlayVideo(std::string userId) {
    waitMixing();
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    TSK_DEBUG_INFO("Leave removeMixOverlayVideo:%s",userId.c_str());
    std::list<std::shared_ptr<MixingInfo>>::iterator itor;
    itor = mixingInfoList.begin();
    while(itor != mixingInfoList.end()) {
        if((*itor)->userId == userId) {
            mixingInfoList.erase(itor);
            break;
        }
        itor++;
    }
    if (CNgnTalkManager::getInstance()->m_strUserID == userId) {
        m_localVideoNoNull = false;
    }
    TSK_DEBUG_INFO("Leave removeMixOverlayVideo");
}

void  YouMeEngineManagerForQiniu::hangupMixOverlayVideo(std::string userId){
    waitMixing();
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    TSK_DEBUG_INFO("Leave hangupMixOverlayVideo:%s",userId.c_str());
    std::list<std::shared_ptr<MixingInfo>>::iterator itor;
    itor = mixingInfoList.begin();
    while(itor != mixingInfoList.end()) {
        if((*itor)->userId == userId) {
            (*itor)->clean();
             //重构freezedFrame
            std::shared_ptr<Frame> frame = std::shared_ptr<Frame>(new Frame((*itor)->width, (*itor)->height, 0));
            frame.get()->clear();
            (*itor)->freezedFrame = frame;
            break;
        }
        itor++;
    }

    TSK_DEBUG_INFO("Leave hangupMixOverlayVideo");
   
}

void  YouMeEngineManagerForQiniu::removeAllOverlayVideo(){
    waitMixing();
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    mixingInfoList.clear();
    m_msgQueue.clear();
    audioFifoList.clear();
    m_msgQueue_audio_mixing.clear();
    if(mMixedFrame){
        delete mMixedFrame;
        mMixedFrame = NULL;
    }
    m_lastTimestamp = 0;
    m_frameCount = 0;
    m_mixSizeOutSet = false;
    
}

std::string YouMeEngineManagerForQiniu::toString() {
    return "";
}

void YouMeEngineManagerForQiniu::setAudioFrameCallback(IYouMeAudioFrameCallback* audioFrameCallback) {
    this->audioFrameCallback = audioFrameCallback;
}

bool YouMeEngineManagerForQiniu::needMixing(std::string userId) {
    //std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(NULL != getMixingInfo(userId)) {
        return true;
    } else {
        return false;
    }
}

void YouMeEngineManagerForQiniu::pushAudioTrack(AudioTrack* audioTrack) {
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex_audio_mixing);
    if(m_msgQueue_audio_mixing.size() >= 30){//最多允许积累30个数据
        TSK_DEBUG_INFO("pushAudioTrack cleared");
        std::deque<AudioTrack*>::iterator it = m_msgQueue_audio_mixing.begin();
        for(it = m_msgQueue_audio_mixing.begin();
            it != m_msgQueue_audio_mixing.end();
            it++)
        {
            delete (*it);
        }
        m_msgQueue_audio_mixing.clear();
    }
    m_msgQueue_audio_mixing.push_back(audioTrack);
    m_msgQueueCond_audio_mixing.notify_one();
}

std::shared_ptr<AudioFifo> YouMeEngineManagerForQiniu::getAudioFifo(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
    std::shared_ptr<AudioFifo> audioFifo = NULL;
    std::list<std::shared_ptr<AudioFifo>>::iterator itor;
    itor = audioFifoList.begin();
    while(itor != audioFifoList.end()) {
        if((*itor)->userId == userId) {
            audioFifo = (*itor);
            break;
        }
        itor++;
    }
    return audioFifo;
}

int YouMeEngineManagerForQiniu::addAudioFifo(std::string userId, int samplerate, int channels) {
    std::lock_guard<std::recursive_mutex> stateLock(audioFifoListMutex);
    TSK_DEBUG_INFO("addAudioFifo(userId:%s, samplerate:%d, channels:%d)", userId.c_str(), samplerate, channels);
    
    //add local user clean cache buffer
    if (auido_local_userid == userId)
    {
        std::list<std::shared_ptr<AudioFifo>>::iterator itor;
        itor = audioFifoList.begin();
        while(itor != audioFifoList.end()) {
            (*itor)->clean();
            itor++;
        }
    }
    
    if(NULL == getAudioFifo(userId)) {
        std::shared_ptr<AudioFifo> audioFifo = std::shared_ptr<AudioFifo>(new AudioFifo(userId, samplerate, channels));
        audioFifoList.push_back(audioFifo);
    }
    
   
    return 0;
}

//去掉合流线程锁，防止所有远端视频都在等待合流，影响渲染
bool  YouMeEngineManagerForQiniu::waitMixing(){
    m_isWaitMixing = true;
    while (m_mixing) {
        tsk_thread_sleep(5);
    }
    m_isWaitMixing = false;
    return true;
}

bool  YouMeEngineManagerForQiniu::isWaitMixing(){
    while (m_isWaitMixing) {
        tsk_thread_sleep(5);
    }
    return true;
}

// wrapper function
#ifdef __cplusplus
extern "C" {
#endif
    void YM_Qiniu_onAudioFrameCallback(int sessionId, void* data, int len, uint64_t timestamp) {
        //std::string userId = CVideoChannelManager::getInstance()->getUserId(sessionId);
        std::string userId = CStringUtilT<char>::to_string(sessionId);
        //YouMeEngineManagerForQiniu::getInstance()->onAudioFrameCallback(userId, data, len, timestamp);
        
        if(!userId.empty()) {
            std::shared_ptr<AudioFifo> audioFifo = YouMeEngineManagerForQiniu::getInstance()->getAudioFifo(userId);
            if(NULL == audioFifo) {
#ifdef ANDROID
                YouMeEngineManagerForQiniu::getInstance()->addAudioFifo(userId, 44100, 1);
#else
                YouMeEngineManagerForQiniu::getInstance()->addAudioFifo(userId, 48000, 1);
#endif
            }
            
            audioFifo = YouMeEngineManagerForQiniu::getInstance()->getAudioFifo(userId);
   
            int writeSize = audioFifo->write(data, len);
           
            if (auido_local_userid == userId) {
                //YouMeEngineManagerForQiniu::getInstance()->m_msgQueueCond_audio_mixing.notify_one();
                //TSK_DEBUG_INFO("qiniu audio recv len:%d, ts:%lld, buffer delay:%d(ms)\n", len, timestamp, \
                               audioFifo->getDataSize()*1000/(audioFifo->samplerate*audioFifo->channels*2));
            }
            
            //TMEDIA_DUMP_TO_FILE("/pcm_data_48_write.bin", data, writeSize);
        }
    }
    
    void YM_Qiniu_onAudioFrameMixedCallback(void* data, int len, uint64_t timestamp) {
        //YouMeEngineManagerForQiniu::getInstance()->onAudioFrameMixedCallback(data, len, timestamp);
    }
#ifdef __cplusplus
}
#endif

//
//  YouMeEngineVideoCodec.cpp
//  youme_voice_engine
//
//  Created by bhb on 2017/11/17.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "YouMeEngineVideoCodec.hpp"
#include "../../tinyMEDIA/include/tinymedia.h"
#include "YouMeVideoMixerAdapter.h"
#include "ICameraManager.hpp"
#include "YouMeConstDefine.h"

static YouMeEngineVideoCodec* instanceYouMeEngineVideoCodec = NULL;
YouMeEngineVideoCodec::YouMeEngineVideoCodec()
{
    m_isLooping = false;
    m_isQiniuFrame = false;
    m_lastTime = 0;
    m_frameCount = 0;
    m_cameraPreviewCallback = NULL;
    startThread();
}

YouMeEngineVideoCodec::~YouMeEngineVideoCodec()
{
    stopThread();
    ClearMessageQueue();
}

YouMeEngineVideoCodec * YouMeEngineVideoCodec::getInstance()
{
    if (!instanceYouMeEngineVideoCodec) {
        instanceYouMeEngineVideoCodec = new YouMeEngineVideoCodec();
    }
    return instanceYouMeEngineVideoCodec;
}

void YouMeEngineVideoCodec::unInstance()
{
    if(instanceYouMeEngineVideoCodec)
    {
        delete instanceYouMeEngineVideoCodec;
        instanceYouMeEngineVideoCodec = NULL;
    }
}

void YouMeEngineVideoCodec::registerCodecCallback(CameraPreviewCallback *cb)
{
    m_cameraPreviewCallback = cb;
}

void YouMeEngineVideoCodec::unregisterCodecCallback()
{
    m_cameraPreviewCallback = NULL;
}

void YouMeEngineVideoCodec::pushFrame(FrameImage* frame, bool isQiniu)
{
    // To ensure local preview fluency, 
    // local preview does not require frame drawing
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
    if (m_msgQueue.size() > 2) { //最多允许积累3个数据
		int s = m_msgQueue.size();
        FrameImage* oldFrame = m_msgQueue.front();
        m_msgQueue.pop_front();
        delete oldFrame;
    }
    m_isQiniuFrame = isQiniu;
    m_msgQueue.push_back(frame);
    m_msgQueueCond.notify_one();

#if 0    
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
    
    uint64_t newTime = tsk_time_now();
    int fps = tmedia_defaults_get_video_fps();

    if ( ((newTime - m_lastTime) + 10 >= (1000.0f/fps)*m_frameCount && m_frameCount < fps)) {
        
        if(m_msgQueue.size() > 3){//最多允许积累3个数据
            FrameImage* oldFrame = m_msgQueue.front();
            m_msgQueue.pop_front();
            delete oldFrame;
        }
        m_isQiniuFrame = isQiniu;
        m_msgQueue.push_back(frame);
        m_msgQueueCond.notify_one();
        m_frameCount++;
    }
    else
    {
        delete frame;
    }
    
    if (newTime - m_lastTime >= 1000) {
        m_lastTime = newTime;
        m_frameCount = 0;
    }
#endif    
    
}

void YouMeEngineVideoCodec::pushFrameNew(FrameImage* frame){
    
	std::lock_guard<std::mutex> queueLockNew(m_msgQueueMutexNew);
	if (m_msgQueueNew.size() > 3) { //最多允许积累3个数据
		int s = m_msgQueueNew.size();
		FrameImage* oldFrame = m_msgQueueNew.front();
		TSK_DEBUG_ERROR("msgqueue delete frame ts[%llu]", oldFrame->timestamp);
		m_msgQueueNew.pop_front();
        delete oldFrame;
    }
	m_msgQueueNew.push_back(frame);
	m_msgQueueCondNew.notify_one();
}

void YouMeEngineVideoCodec::startThread()
{
    m_isLooping = true;
    m_thread = std::thread(&YouMeEngineVideoCodec::threadFunc, this);
	m_threadNew = std::thread(&YouMeEngineVideoCodec::threadFuncNew, this);
}

void YouMeEngineVideoCodec::stopThread()
{
    if (m_thread.joinable()) {
        if (std::this_thread::get_id() != m_thread.get_id()) {
            m_isLooping = false;
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

	if (m_threadNew.joinable()) {
		if (std::this_thread::get_id() != m_threadNew.get_id()) {
			m_isLooping = false;
			m_msgQueueMutexNew.lock();
			m_msgQueueCondNew.notify_all();
			m_msgQueueMutexNew.unlock();
			TSK_DEBUG_INFO("Start joining m_threadNew");
			m_threadNew.join();
			TSK_DEBUG_INFO("Joining m_threadNew OK");
		}
		else {
			m_threadNew.detach();
		}
		ClearMessageQueueNew();
	}
}

void YouMeEngineVideoCodec::threadFunc()
{
    while (m_isLooping) {
        
        std::unique_lock<std::mutex> queueLock(m_msgQueueMutex);
        while (m_isLooping && m_msgQueue.empty()) {
            m_msgQueueCond.wait(queueLock);
        }
        
        if (!m_isLooping) {
            break;
        }
        
        FrameImage* frame = m_msgQueue.front();
        m_msgQueue.pop_front();
        queueLock.unlock();
        
        if(m_cameraPreviewCallback){
            if(frame->mirror == YOUME_VIDEO_MIRROR_MODE_ENABLED ||
               frame->mirror == YOUME_VIDEO_MIRROR_MODE_FAR)
                ICameraManager::getInstance()->mirror((uint8_t*)frame->data, frame->width, frame->height);
            if(frame->double_stream){
                YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameNew(frame->data, frame->len, frame->width, frame->height, frame->videoid, frame->timestamp, frame->fmt);
                //m_cameraPreviewCallback->onPreviewFrameNew(frame->data, frame->width*frame->height*3/2, frame->width, frame->height, frame->videoid, frame->timestamp);

            }else{
                YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrame(frame->data, frame->len, frame->width, frame->height, frame->timestamp, frame->fmt);
                //m_cameraPreviewCallback->onPreviewFrame(frame->data, frame->width*frame->height*3/2, frame->width, frame->height, frame->timestamp);
            }
            
        }
        delete frame;
        
    }
     TSK_DEBUG_INFO("YouMeEngineVideoCodec::threadFunc() thread exits");
}

void YouMeEngineVideoCodec::threadFuncNew()
{
	//pushFrameNew 的数据消费线程，目前是对应屏幕共享的数据流
	while (m_isLooping) {

		std::unique_lock<std::mutex> queueLockNew(m_msgQueueMutexNew);
		while (m_isLooping && m_msgQueueNew.empty()) {
			m_msgQueueCondNew.wait(queueLockNew);
		}

		if (!m_isLooping) {
			break;
		}

		FrameImage* frame = m_msgQueueNew.front();
		m_msgQueueNew.pop_front();
		queueLockNew.unlock();

		if (m_cameraPreviewCallback){
			if (frame->mirror == YOUME_VIDEO_MIRROR_MODE_ENABLED ||
				frame->mirror == YOUME_VIDEO_MIRROR_MODE_FAR)
				ICameraManager::getInstance()->mirror((uint8_t*)frame->data, frame->width, frame->height);
			if (frame->double_stream){
				YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameNew(frame->data, frame->len, frame->width, frame->height, frame->videoid, frame->timestamp, frame->fmt);
			}
			else{
				YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrame(frame->data, frame->len, frame->width, frame->height, frame->timestamp, frame->fmt);
			}

		}
		delete frame;

	}
	TSK_DEBUG_INFO("YouMeEngineVideoCodec::threadFuncNew() thread exits");
}

void YouMeEngineVideoCodec::ClearMessageQueue()
{
    std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
    std::deque<FrameImage*>::iterator it;
    while (!m_msgQueue.empty()) {
        FrameImage* pMsg = m_msgQueue.front();
        m_msgQueue.pop_front();
        if (pMsg) {
            delete pMsg;
            pMsg = NULL;
        }
    }
}

void YouMeEngineVideoCodec::ClearMessageQueueNew()
{
	std::lock_guard<std::mutex> queueLockNew(m_msgQueueMutexNew);
	std::deque<FrameImage*>::iterator it;
	while (!m_msgQueueNew.empty()) {
		FrameImage* pMsg = m_msgQueueNew.front();
		m_msgQueueNew.pop_front();
		if (pMsg) {
			delete pMsg;
			pMsg = NULL;
		}
	}
}


//
//  VideoRenderInfo.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/7/7.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <StringUtil.hpp>
#include "tsk_debug.h"
#include "YouMeVoiceEngine.h"
#include "NgnTalkManager.hpp"
#include "VideoRenderInfo.hpp"


CVideoRenderInfo::CVideoRenderInfo(std::string userId, int renderId) {
    this->userId = userId;
    this->renderId = renderId;
    if(renderId < 0) {
        this->state = (YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER);
    } else {
        this->state = (YOUME_CHANNEL_RENDER_STATE_NORMAL);
    }
    //不需要render绑定了，为什么要开个线程呢？
   // startThread();
}

CVideoRenderInfo::CVideoRenderInfo(std::string userId) {
    this->userId = userId;
    this->state = (YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER);
   // startThread();
}

CVideoRenderInfo::~CVideoRenderInfo() {
   // stopThread();
}

void CVideoRenderInfo::startThread() {
    m_isLooping = true;
    m_thread = std::thread(&CVideoRenderInfo::threadFunc, this);
}

void CVideoRenderInfo::stopThread() {
    if (m_thread.joinable()) {
        if (std::this_thread::get_id() != m_thread.get_id()) {
            m_isLooping = false;
            video_render_condWait.SetSignal();
            TSK_DEBUG_INFO("Start joining thread");
            m_thread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            m_thread.detach();
        }
    }
}

void CVideoRenderInfo::threadFunc() {
    TSK_DEBUG_INFO("CVideoRenderInfo::threadFunc() thread enters. userId:%s", userId.c_str());
    
    while (m_isLooping) {
        youmecommon::WaitResult result;
        if((state & YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER) != 0) {
//            CYouMeVoiceEngine::getInstance()->OnReciveOtherVideoDataOnNotify(userId, CNgnTalkManager::getInstance()->m_strTalkRoomID);
            result = video_render_condWait.WaitTime(1000);
            TSK_DEBUG_INFO("CVideoRenderInfo::threadFunc() thread is running. result:%d", result);
        } else {
            video_render_condWait.Wait();
            TSK_DEBUG_INFO("CVideoRenderInfo::threadFunc() thread is running.");
        }
    }
    
    TSK_DEBUG_INFO("CVideoRenderInfo::threadFunc() thread exits");
}

std::string CVideoRenderInfo::toString() {
    return CStringUtilT<char>::formatString("CVideoRenderInfo{userId:%s, renderId:%d, state:0x%08x}", userId.c_str(), renderId, state);
}

int CVideoRenderInfo::setRenderId(int renderId) {
    this->renderId = renderId;
    if(renderId < 0) {
        this->state |= (YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER);
    } else {
        this->state &= (~YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER);
    }
    
    video_render_condWait.SetSignal();
    return 0;
}

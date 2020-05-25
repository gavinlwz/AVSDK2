//
//  VideoUserInfo.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/7/7.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <StringUtil.hpp>
#include "tsk_debug.h"
#include "YouMeVoiceEngine.h"
#include "VideoUserInfo.hpp"
#include "XOsWrapper.h"

/***
 * CVideoUserInfo construct function
 */
CVideoUserInfo::CVideoUserInfo(int sessionId) {
    this->sessionId = sessionId;
    this->state = (YOUME_CHANNEL_USER_STATE_UNKNOW_USER);
    startThread();
}

CVideoUserInfo::CVideoUserInfo(int sessionId, std::string userId) {
    this->sessionId = sessionId;
    this->userId = userId;
    if(userId.empty()) {
        this->state = (YOUME_CHANNEL_USER_STATE_UNKNOW_USER);
        startThread();
    } else {
        this->state = (YOUME_CHANNEL_USER_STATE_NORMAL);
    }
}

CVideoUserInfo::CVideoUserInfo(std::string userId) {
    startThread();
}

CVideoUserInfo::~CVideoUserInfo() {
    stopThread();
}

void CVideoUserInfo::startThread() {
    m_isLooping = true;
    m_thread = std::thread(&CVideoUserInfo::threadFunc, this);
}

void CVideoUserInfo::stopThread() {
    if (m_thread.joinable()) {
        if (std::this_thread::get_id() != m_thread.get_id()) {
            m_isLooping = false;
            video_user_condWait.SetSignal();
            TSK_DEBUG_INFO("Start joining thread");
            m_thread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            m_thread.detach();
        }
    }
}

void CVideoUserInfo::threadFunc() {
    //TSK_DEBUG_INFO("CVideoUserInfo::threadFunc() session:%d thread enters", this->sessionId);
    
    while (m_isLooping) {
        youmecommon::WaitResult result;
        if( m_isPause )
        {
            XSleep(20);
            continue;
        }
        
        if((state & YOUME_CHANNEL_USER_STATE_UNKNOW_USER) != 0) {
            if( sessionId >= 0 )
            {
                CYouMeVoiceEngine::getInstance()->sendSessionUserIdMapRequest(sessionId);
            }
            else{
                //负数的session代表share流的
                CYouMeVoiceEngine::getInstance()->sendSessionUserIdMapRequest( -sessionId );
            }
            
            result = video_user_condWait.WaitTime(5000);
            TSK_DEBUG_INFO("CVideoUserInfo::threadFunc() session:%d thread is running. result:%d", this->sessionId, result);
        } else {
             //需要继续等待？？
            //video_user_condWait.Wait();
            // TSK_DEBUG_INFO("CVideoUserInfo::threadFunc() session:%d thread is running.", this->sessionId);
            break;
        }
    }
    
    TSK_DEBUG_INFO("CVideoUserInfo::threadFunc() session:%d thread exits", this->sessionId);
}

std::string CVideoUserInfo::toString() {
    return CStringUtilT<char>::formatString("CVideoUserInfo{userId:%s, sessionId:%d, state:0x%08x}", userId.c_str(), sessionId, state);
}

int CVideoUserInfo::setUserId(std::string userId) {
    this->userId = userId;
    if(userId.empty()) {
        this->state |= (YOUME_CHANNEL_USER_STATE_UNKNOW_USER);
    } else {
        this->state &= (~YOUME_CHANNEL_USER_STATE_UNKNOW_USER);
        video_user_condWait.SetSignal();
    }
    
    return 0;
}

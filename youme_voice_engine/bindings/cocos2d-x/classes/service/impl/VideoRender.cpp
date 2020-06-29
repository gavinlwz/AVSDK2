//
//  VideoRender.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/6/8.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <StringUtil.hpp>
#include "tsk_debug.h"
#include "VideoRender.hpp"

VideoRender::VideoRender(std::string userId, int renderId, int sessionId) {
    this->userId = userId;
    this->renderId = renderId;
    this->sessionId = sessionId;
}

std::string VideoRender::toString() {
    return CStringUtilT<char>::formatString("{userId:%s, renderId:%d, sessionId:%d}", userId.c_str(), renderId, sessionId);
    //TSK_DEBUG_INFO("{userId:%s, renderId:%d, sessionId:%d}", userId.c_str(), renderId, sessionId);
}


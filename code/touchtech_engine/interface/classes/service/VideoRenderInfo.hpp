//
//  VideoRenderInfo.hpp
//  youme_voice_engine
//
//  Created by mac on 2017/7/7.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef VideoRenderInfo_hpp
#define VideoRenderInfo_hpp

#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <XCondWait.h>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>

#define YOUME_CHANNEL_RENDER_STATE_NORMAL (0)
#define YOUME_CHANNEL_RENDER_STATE_UNKNOW_RENDER (1 << 0)
#define YOUME_CHANNEL_RENDER_STATE_NONE (-1)


/****
 * 用户信息
 *
 *
 */
class CVideoRenderInfo {
public:
    std::string userId;
    int renderId = -1;
    int sessionId = -1;
    int state = YOUME_CHANNEL_RENDER_STATE_NONE;
    
    std::thread m_thread;
    bool m_isLooping = false;
    std::recursive_mutex video_render_mutex;
    youmecommon::CXCondWait video_render_condWait;
    
    bool m_isFristVideo = true;
    
public:
    CVideoRenderInfo(std::string userId);
    CVideoRenderInfo(std::string userId, int renderId);
    ~CVideoRenderInfo();
    void startThread();
    void stopThread();
    void threadFunc();
    std::string toString();
    int setRenderId(int renderId);
};

#endif /* VideoRenderInfo_hpp */

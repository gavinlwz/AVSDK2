//
//  VideoUserInfo.hpp
//  youme_voice_engine
//
//  Created by mac on 2017/7/7.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef VideoUserInfo_hpp
#define VideoUserInfo_hpp

#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <XCondWait.h>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>


#define YOUME_CHANNEL_USER_STATE_NORMAL (0)
#define YOUME_CHANNEL_USER_STATE_UNKNOW_USER (1 << 0)
#define YOUME_CHANNEL_USER_STATE_NONE (-1)


/****
 * 用户信息
 *
 *
 */
class CVideoUserInfo {
public:
    int sessionId;
    std::string userId;
    int state = YOUME_CHANNEL_USER_STATE_NONE;
    
    std::thread m_thread;
    bool m_isLooping = false;
    std::recursive_mutex video_user_mutex;
    youmecommon::CXCondWait video_user_condWait;
    
    bool m_isPause = false;
    bool m_isVideo = false;
    int m_initWidth = 0;
    int m_initHeight = 0;
    
public:
    CVideoUserInfo(int sessionId);
    CVideoUserInfo(int sessionId, std::string userId);
    CVideoUserInfo(std::string userId);
    ~CVideoUserInfo();
    
    void startThread();
    void stopThread();
    void threadFunc();
    std::string toString();
    
    int setUserId(std::string userId);
    /*
    int setRenderId(int renderId);
    int getRenderId();
     */
};

#endif /* VideoUserInfo_hpp */

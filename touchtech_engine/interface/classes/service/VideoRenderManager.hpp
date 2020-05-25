//
//  VideoRenderManager.hpp
//  youme_voice_engine
//
//  Created by mac on 2017/6/8.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef VideoRenderManager_hpp
#define VideoRenderManager_hpp

#include "VideoRender.hpp"
#include <string>
#include <mutex>
#include <list>
#include <memory>

class VideoRenderManager {
private:
    VideoRenderManager();
    virtual ~VideoRenderManager();

private:
    static VideoRenderManager* instance;
    std::list<std::shared_ptr<VideoRender>> videoRenders;

public:
    static VideoRenderManager* getInstance(void);
    bool isVideoRenderExist(std::string userId);
    bool isVideoRenderExist(int renderId);
    bool isVideoRenderExist(std::string userId, int sessionId);
    int createVideoRender(std::string userId);
    int deleteVideoRender(int renderId);
    int insertVideoRender(std::string userId, int sessionId, int renderId);
    std::shared_ptr<VideoRender> getVideoRender(int sessionId);
    std::string getUserIdBySessionId(int sessionId);
    void pairSessionId(int sessionId, std::string & userId);
    void toString();
};

#endif /* VideoRenderManager_hpp */

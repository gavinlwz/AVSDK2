//
//  VideoRenderManager.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/6/8.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "VideoRenderManager.hpp"

std::recursive_mutex* video_render_manager_mutex = new std::recursive_mutex();
int gRenderId = 0;
VideoRenderManager* VideoRenderManager::instance = NULL;

VideoRenderManager::VideoRenderManager() {
    
}

VideoRenderManager::~VideoRenderManager() {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    videoRenders.clear();
}

VideoRenderManager* VideoRenderManager::getInstance(void)
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    if (instance == NULL) {
        instance = new (std::nothrow) VideoRenderManager();
    }
    
    return instance;
}

bool VideoRenderManager::isVideoRenderExist(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while (itor != videoRenders.end()) {
        if((*itor)->userId == userId) {
            return true;
        }
        itor++;
    }
    return false;
}

bool VideoRenderManager::isVideoRenderExist(int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while (itor != videoRenders.end()) {
        if((*itor)->renderId == renderId) {
            return true;
        }
        itor++;
    }
    return false;
}

bool VideoRenderManager::isVideoRenderExist(std::string userId, int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while (itor != videoRenders.end()) {
        if(((*itor)->userId == userId) && ((*itor)->sessionId == sessionId)) {
            return true;
        }
        itor++;
    }
    return false;
}

int VideoRenderManager::createVideoRender(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::shared_ptr<VideoRender> videoRender(new VideoRender(userId, gRenderId++, -1));
    videoRenders.push_back(videoRender);
    return videoRender->renderId;
}

int VideoRenderManager::deleteVideoRender(int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while(itor != videoRenders.end()) {
        if((*itor)->renderId == renderId) {
            videoRenders.erase(itor);
            break;
        }
        itor++;
    }

    return 0;
}

int VideoRenderManager::insertVideoRender(std::string userId, int sessionId, int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::shared_ptr<VideoRender> videoRender(new VideoRender(userId, sessionId, renderId));
    videoRenders.push_back(videoRender);
    return 0;
}

std::shared_ptr<VideoRender> VideoRenderManager::getVideoRender(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while(itor != videoRenders.end()) {
        if((*itor)->sessionId == sessionId) {
            return *itor;
        }
        itor++;
    }
    
    return NULL;
}

/***
 * 根据sessionId获取userId
 */
std::string VideoRenderManager::getUserIdBySessionId(int sessionId)
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while(itor != videoRenders.end()) {
        if((*itor)->sessionId == sessionId) {
            return (*itor)->userId;
        }
        itor++;
    }
    return "This_is_NULL";
}

void VideoRenderManager::pairSessionId(int sessionId, std::string & userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    itor = videoRenders.begin();
    while(itor != videoRenders.end()) {
        if((*itor)->userId == userId) {
            (*itor)->sessionId = sessionId;
        }
        itor++;
    }
}

void VideoRenderManager::toString() {
    std::lock_guard<std::recursive_mutex> stateLock(*video_render_manager_mutex);
    std::list<std::shared_ptr<VideoRender>>::iterator itor;
    for(itor = videoRenders.begin(); itor != videoRenders.end(); itor++) {
        (*itor)->toString();
    }
}

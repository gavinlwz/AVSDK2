//
//  NgnVideoManager.cpp
//  youme_voice_engine
//
//  Created by fire on 2017/4/7.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <StringUtil.hpp>
#include "YouMeVoiceEngine.h"
#include "NgnVideoManager.hpp"
#include "NgnTalkManager.hpp"


CVideoManager * CVideoManager::gVideoManager = NULL;
static int gRenderId = 0;


CVideoManager::CVideoManager()
{
    
}

CVideoManager::~CVideoManager()
{
    
}

CVideoManager * CVideoManager::getInstance(void)
{
    if (gVideoManager == NULL) {
        gVideoManager = new (std::nothrow) CVideoManager();
        
        gVideoManager->init(); /* init */
        gRenderId = 0;
    }
    
    return gVideoManager;
}

void CVideoManager::destory(void)
{
    if (gVideoManager != NULL) {
        delete gVideoManager;
        
        gVideoManager = NULL;
        gRenderId = 0;
    }
}

bool CVideoManager::init(void)
{
    mRenders.clear();
    return true;
}

/***
 * 创建RenderId
 * return : -1 -> failed,  other value -> renderId
 *
 */
int CVideoManager::createRender(const std::string & userId)
{
    std::lock_guard<std::recursive_mutex> stateLock(managerMutex);
    return CVideoChannelManager::getInstance()->createRender(userId);
}

/***
 * 删除renderId 
 */
void CVideoManager::deleteRender(int renderId)
{
    std::lock_guard<std::recursive_mutex> stateLock(managerMutex);
    std::map<int, RenderInfo>::iterator iter = mRenders.find(renderId);
    if (iter != mRenders.end()) {
        mRenders.erase(iter);
    }

    CVideoChannelManager::getInstance()->deleteRender(renderId);
}

/***
 * 根据sessionId获取renderId 
 */
int CVideoManager::getRenderId(int sessionId)
{
    return CVideoChannelManager::getInstance()->getRenderId(sessionId);
}

/***
 * 根据sessionId获取userId
 */
std::string CVideoManager::getUserId(int sessionId)
{
    return CVideoChannelManager::getInstance()->getUserId(sessionId);
}

std::recursive_mutex* video_channel_manager_mutex = new std::recursive_mutex();
CVideoChannelManager* CVideoChannelManager::instance = NULL;

CVideoChannelManager::CVideoChannelManager() {
    
}

CVideoChannelManager::~CVideoChannelManager() {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    userList.clear();
    renderList.clear();
}

CVideoChannelManager* CVideoChannelManager::getInstance(void)
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    if (instance == NULL) {
        instance = new (std::nothrow) CVideoChannelManager();
    }
    
    return instance;
}

int CVideoChannelManager::createRender(const std::string & userId)
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@createRender. userId:%s", userId.c_str());
    
    std::shared_ptr<CVideoRenderInfo> renderInfo = getRenderInfo(userId);
    if(NULL == renderInfo) {
        renderInfo = std::shared_ptr<CVideoRenderInfo>(new CVideoRenderInfo(userId, gRenderId++));
        renderList.push_back(renderInfo);
    } else {
        renderInfo->setRenderId(gRenderId++);
    }

    TSK_DEBUG_INFO("==createRender. userId:%s renderId:%d", userId.c_str(), renderInfo->renderId);
    return renderInfo->renderId;
}

int CVideoChannelManager::deleteRender(int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@deleteRender. renderId:%d", renderId);
    
    std::string userId;
    std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor;
    itor = renderList.begin();
    while(itor != renderList.end()) {
        if((*itor)->renderId == renderId) {
            userId = (*itor)->userId;
            renderList.erase(itor);
            break;
        }
        itor++;
    }
    
    deleteUser(userId);
    
    TSK_DEBUG_INFO("==deleteRender. renderId:%d", renderId);
    return 0;
}

int CVideoChannelManager::deleteRender(const std::string & userId){
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@deleteRender. userId:%s", userId.c_str());
    
    std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor;
    itor = renderList.begin();
    while(itor != renderList.end()) {
        if((*itor)->userId == userId) {
            renderList.erase(itor);
            break;
        }
        itor++;
    }
    
    //deleteUser(userId);
    
    TSK_DEBUG_INFO("==deleteRender. userId:%s", userId.c_str());
    return 0;
}

int CVideoChannelManager::createUser(int sessionId, std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@createUser. sessionId:%d", sessionId);
    
    std::shared_ptr<CVideoUserInfo> userInfo = getUserInfo(sessionId);
    if(NULL == userInfo) {
        userInfo = std::shared_ptr<CVideoUserInfo>(new CVideoUserInfo(sessionId, userId));
        userList.push_back(userInfo);
    } else {
        userInfo->setUserId(userId);
    }
    
    TSK_DEBUG_INFO("==createUser. sessionId:%d", sessionId );
    return 0;
}

int CVideoChannelManager::createUser(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@createUser sessionId:%d", sessionId);
    
    std::shared_ptr<CVideoUserInfo> userInfo = getUserInfo(sessionId);
    if(NULL == userInfo) {
        userInfo = std::shared_ptr<CVideoUserInfo>(new CVideoUserInfo(sessionId));
        userList.push_back(userInfo);
    }
    
    TSK_DEBUG_INFO("==createUser sessionId:%d", sessionId);
    return 0;
}

int CVideoChannelManager::getRenderId(int sessionId)
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    int renderId = -1;
    std::shared_ptr<CVideoUserInfo> userInfo = getUserInfo(sessionId);
    if(NULL != userInfo) {
        std::shared_ptr<CVideoRenderInfo> renderInfo = getRenderInfo(userInfo->userId);
        if(NULL != renderInfo) {
            renderId = renderInfo->renderId;
        }
    }
    
    return renderId;
}

int CVideoChannelManager::insertUser(int sessionId, std::string userId,int videoId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    TSK_DEBUG_INFO("## insertUser session:%d userId:%s", sessionId, userId.c_str());
    std::shared_ptr<CVideoUserInfo> userInfo = getUserInfo(sessionId);
    if(NULL == userInfo) {
        userInfo = std::shared_ptr<CVideoUserInfo>(new CVideoUserInfo(sessionId, userId));
        userList.push_back(userInfo);
    } else {
        userInfo->setUserId(userId);
    }

    if(!userInfo->userId.empty()) {
        std::shared_ptr<CVideoRenderInfo> renderInfo = getRenderInfo(userInfo->userId);
        if(NULL == renderInfo) {
            renderInfo = std::shared_ptr<CVideoRenderInfo>(new CVideoRenderInfo(userInfo->userId));
            renderInfo->sessionId = sessionId;
            renderList.push_back(renderInfo);
            //VIDEO_ON 通知
            if(userInfo->m_isVideo){
               renderInfo->m_isFristVideo = false;
               CYouMeVoiceEngine::getInstance()->OnReciveOtherVideoDataOnNotify(userId, CNgnTalkManager::getInstance()->m_strTalkRoomID, videoId ,userInfo->m_initWidth, userInfo->m_initHeight);
            }
        }
        else{
            renderInfo->sessionId = sessionId;
        }
    }
    TSK_DEBUG_INFO("== insertUser session:%d userId:%s", sessionId, userId.c_str());
    
    return 0;
}

int CVideoChannelManager::deleteUser(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->userId == userId) {
            userList.erase(itor);
            break;
        }
        itor++;
    }
    return 0;
}

int CVideoChannelManager::deleteUser(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->sessionId == sessionId) {
            userList.erase(itor);
            break;
        }
        itor++;
    }
    return 0;
}

std::string CVideoChannelManager::getUserId(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);

    std::string userId;
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->sessionId == sessionId) {
            userId = (*itor)->userId;
        }
        itor++;
    }
    return userId;
}

int CVideoChannelManager::getSessionId(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);

    int sessionId = -1;
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->userId == userId) {
            sessionId = (*itor)->sessionId;
        }
        itor++;
    }
    return sessionId;
}

int CVideoChannelManager::setRenderId(std::string userId, int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    std::shared_ptr<CVideoRenderInfo> renderInfo = getRenderInfo(userId);
    if(NULL != renderInfo) {
        renderInfo->setRenderId(renderId);
    }
    return 0;
}

std::shared_ptr<CVideoUserInfo> CVideoChannelManager::getUserInfo(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::shared_ptr<CVideoUserInfo> userInfo = NULL;
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->sessionId == sessionId) {
            userInfo = (*itor);
        }
        itor++;
    }
    return userInfo;
}

std::shared_ptr<CVideoUserInfo> CVideoChannelManager::getUserInfo(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::shared_ptr<CVideoUserInfo> userInfo = NULL;
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->userId == userId) {
            userInfo = (*itor);
        }
        itor++;
    }
    return userInfo;
}

std::shared_ptr<CVideoRenderInfo> CVideoChannelManager::getRenderInfo(int renderId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::shared_ptr<CVideoRenderInfo> renderInfo = NULL;
    std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor;
    itor = renderList.begin();
    while(itor != renderList.end()) {
        if((*itor)->renderId == renderId) {
            renderInfo = (*itor);
        }
        itor++;
    }
    return renderInfo;
}

std::shared_ptr<CVideoRenderInfo> CVideoChannelManager::getRenderInfo(std::string userId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::shared_ptr<CVideoRenderInfo> renderInfo = NULL;
    std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor;
    itor = renderList.begin();
    while(itor != renderList.end()) {
        if((*itor)->userId == userId) {
            renderInfo = (*itor);
        }
        itor++;
    }
    return renderInfo;
}

std::shared_ptr<CVideoRenderInfo> CVideoChannelManager::getSessionToRenderInfo(int sessionId){
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    
    std::shared_ptr<CVideoRenderInfo> renderInfo = NULL;
    std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor;
    itor = renderList.begin();
    while(itor != renderList.end()) {
        if((*itor)->sessionId == sessionId) {
            renderInfo = (*itor);
        }
        itor++;
    }
    return renderInfo;
}


int CVideoChannelManager::getUserState(int sessionId) {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);

    int state = -1;
    std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
    itor = userList.begin();
    while(itor != userList.end()) {
        if((*itor)->sessionId == sessionId) {
            state = (*itor)->state;
        }
        itor++;
    }
    return state;
}

int CVideoChannelManager::clear() {
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    TSK_DEBUG_INFO("@@clear");
    
    renderList.clear();
    userList.clear();
    
    TSK_DEBUG_INFO("==clear");
    return 0;
}

void CVideoChannelManager::pause()
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    auto itor = userList.begin();
    itor = userList.begin();
    for( ; itor != userList.end(); ++itor )
    {
        (*itor)->m_isPause = true;
    }
}

void CVideoChannelManager::resume()
{
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);
    auto itor = userList.begin();
    itor = userList.begin();
    for( ; itor != userList.end(); ++itor )
    {
        (*itor)->m_isPause = false;
    }
}

void CVideoChannelManager::toString() {
    static int i = 0;
    std::lock_guard<std::recursive_mutex> stateLock(*video_channel_manager_mutex);

    if(i%100 == 0) {
        std::list<std::shared_ptr<CVideoUserInfo>>::iterator itor;
        itor = userList.begin();
        while(itor != userList.end()) {
           // TSK_DEBUG_INFO("%s", (*itor)->toString().c_str());
            itor++;
        }
        
        std::list<std::shared_ptr<CVideoRenderInfo>>::iterator itor1;
        itor1 = renderList.begin();
        while(itor1 != renderList.end()) {
           // TSK_DEBUG_INFO("%s", (*itor1)->toString().c_str());
            itor1++;
        }
    }
    i++;
}

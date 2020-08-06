//
//  NgnVideoManager.hpp
//  youme_voice_engine
//
//  Created by fire on 2017/4/7.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef NgnVideoManager_hpp
#define NgnVideoManager_hpp

#include <string>
#include <mutex>
#include <map>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include "VideoRenderManager.hpp"
#include "VideoUserInfo.hpp"
#include "VideoRenderInfo.hpp"

/****
 * 视频管理
 * 
 * 目前音视频包携带的用户信息只有sessionId，而用户创建UI元素的时候是不知道他人的sessionId的，
 * 所以在视频包数据过来之后，通过发送sessionId数据到服务器，获取对应的userId，来进行用户界面的UI元素与音视频包关联，
 * 只有关联成功后才讲视频包传到UI进行渲染。
 *
 */

class CVideoManager {
private:
    CVideoManager();
    virtual ~CVideoManager();

private:
    typedef struct _RenderInfo {
        std::string userId;
        int renderId;
        int sessionId;
        
        _RenderInfo() :
            userId(""),
            renderId(0),
            sessionId(0)
        {
        }
    } RenderInfo;
    
private:
    static CVideoManager * gVideoManager;
    std::recursive_mutex managerMutex;
    
    std::map<int, RenderInfo> mRenders;     /* <userId, RenderInfo> */

public:
    static CVideoManager * getInstance(void);
    static void destory(void);
    
    int  createRender(const std::string & userId);  /* 创建RenderId */
    void deleteRender(int renderId);                /* 删除renderId */
    int  getRenderId(int sessionId);                /* 根据sessionId获取renderId */
    std::string getUserId(int sessionId);          /* 根据sessionId获取userId*/

private:
    bool init(void);
};

/****
 * 房间用户信息列表
 *
 *
 */
class CVideoChannelManager {
private:
    CVideoChannelManager();
    virtual ~CVideoChannelManager();

private:
    static CVideoChannelManager* instance;
    
public:
    static CVideoChannelManager* getInstance();
    std::list<std::shared_ptr<CVideoUserInfo>> userList;
    std::list<std::shared_ptr<CVideoRenderInfo>> renderList;
    
    int createRender(const std::string & userId);
    int deleteRender(int renderId);
    int deleteRender(const std::string & userId);
    int createUser(int sessionId, std::string userId);
    int createUser(int sessionId);
    int getRenderId(int sessionId);                /* 根据sessionId获取renderId */
    int insertUser(int sessionId, std::string userId = "", int videoId=0);
    int deleteUser(std::string userId);
    int deleteUser(int sessionId);
    std::string getUserId(int sessionId);
    int getSessionId(std::string userId);
    int setRenderId(std::string userId, int renderId);
    std::shared_ptr<CVideoUserInfo> getUserInfo(int sessionId);
    std::shared_ptr<CVideoUserInfo> getUserInfo(std::string userId);
    std::shared_ptr<CVideoRenderInfo> getRenderInfo(std::string userId);
    std::shared_ptr<CVideoRenderInfo> getRenderInfo(int renderId);
    std::shared_ptr<CVideoRenderInfo> getSessionToRenderInfo(int sessionId);
    int getUserState(int sessionId);
    int clear();
    
    //重连的情况下，要暂停ID查询
    void pause();
    void resume();
    
    void toString();
public:
    
};

#endif /* NgnVideoManager_hpp */

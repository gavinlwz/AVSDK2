//
//  NgnTalkManager.hpp
//  youme_voice_engine
//
//  Created by fire on 2016/11/9.
//  Copyright © 2016年 Youme. All rights reserved.
//

#ifndef NgnTalkManager_hpp
#define NgnTalkManager_hpp

#include <string>
#include <mutex>
#include <map>
#include "YouMeConstDefine.h"


enum YOUME_CHANNEL_STATE {
    YOUME_CHANNEL_NONE = 0,             /* 初始状态 */
    YOUME_CHANNEL_CONNECTING,           /* 连接中 */
    YOUME_CHANNEL_CONNECTED,            /* 已连接（会话中） */
    YOUME_CHANNEL_TERMWAITING,          /* 退出中 */
    YOUME_CHANNEL_TERMINATED            /* 已退出 */
};

/**
 * 单个房间信息
 */
class CChannelInfo
{
public:
    CChannelInfo() { }
    ~CChannelInfo() { }

public:
    bool switchState(YOUME_CHANNEL_STATE oldState, YOUME_CHANNEL_STATE newState);
    YOUME_CHANNEL_STATE getState();
    void setState(YOUME_CHANNEL_STATE state);
    

public:
    std::string m_strRoomID;                                /* 房间id */
	YouMeUserRole_t  m_eUserRole = YOUME_USER_LISTENER;       /* 用户角色（默认听众） */
    YOUME_CHANNEL_STATE m_eStatus = YOUME_CHANNEL_NONE;     /* 房间状态（初始值） */
};

/**
 * 房间管理
 */
class CNgnTalkManager
{
private:
    CNgnTalkManager();
    ~CNgnTalkManager();
    
public:
    static CNgnTalkManager * getInstance();
	static void destory();

    std::map<std::string, CChannelInfo> * getAllChannel();		/* 获取所有频道信息 */
	CChannelInfo * getChannel(const std::string & roomID);		/* 获取单个频道信息 */
	void setChannel(CChannelInfo & channnel);					/* 设置频道 */
	void removeChannel(std::string & roomID);					/* 删除频道 */

	std::string genServerRoomID(const std::string & strRoomID); /** client -> server */
	std::string getClientRoomID(const std::string & strRoomID); /** server -> client */

public:
    std::string mcu_ip;                                 /* mcu ip */
    int mcu_tcpport = 0;                                /* tcp port */
    int mcu_udpport = 0;                                /* udp port */
    
	std::string mAppKey;								/* appkey */
    std::string m_strUserID;                            /* 用户id */
    std::string m_strTalkRoomID;                        /* 当前会话的房间id */
    int m_iSessionID = 0;                               /* sessionid */

    int m_iRoomNumber;									/* 当前加入房间数量 */
    bool m_bReConnect;                                  /* 是否是重连 */


protected:
	static CNgnTalkManager * m_pTalkManager;
	static std::mutex m_talkMutex;

private:
    std::map<std::string, CChannelInfo> m_mChannels;    /* 房间列表 */
};

#endif /* NgnTalkManager_hpp */

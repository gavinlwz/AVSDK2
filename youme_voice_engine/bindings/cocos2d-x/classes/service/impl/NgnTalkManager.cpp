//
//  NgnTalkManager.cpp
//  youme_voice_engine
//
//  Created by fire on 2016/11/9.
//  Copyright © 2016年 Youme. All rights reserved.
//

#include "NgnTalkManager.hpp"
#include "tsk_debug.h"



CNgnTalkManager * CNgnTalkManager::m_pTalkManager = NULL;
std::mutex CNgnTalkManager::m_talkMutex;


/***
 * 交换房间状态
 */
bool CChannelInfo::switchState(YOUME_CHANNEL_STATE oldState, YOUME_CHANNEL_STATE newState)
{
    if (this->m_eStatus != oldState) {
        return false;
    }
    
    this->m_eStatus = newState;
    return true;
}

/**
 * 获取当前房间状态
 */
YOUME_CHANNEL_STATE CChannelInfo::getState()
{
    return this->m_eStatus;
}

/**
 * 设置当前房间状态
 */
void CChannelInfo::setState(YOUME_CHANNEL_STATE state)
{
    this->m_eStatus = state;
}


/****************************** CNgnTalkManager **************************************/

CNgnTalkManager::CNgnTalkManager()
{
    TSK_DEBUG_INFO ("===========  CNgnTalkManager  ===========");
	m_mChannels.clear();
    mcu_ip.clear();
    mcu_tcpport = 0;
    mcu_udpport = 0;
    m_strUserID.clear();
    m_strTalkRoomID.clear();
    m_iSessionID = 0;
    m_iRoomNumber = 0;
    m_bReConnect = false;
}

CNgnTalkManager::~CNgnTalkManager()
{
    TSK_DEBUG_INFO ("===========  ~CNgnTalkManager  ===========");
	m_mChannels.clear();
    mcu_ip.clear();
    mcu_tcpport = 0;
    mcu_udpport = 0;
    m_strUserID.clear();
    m_strTalkRoomID.clear();
    m_iSessionID = 0;
    m_iRoomNumber = 0;
    m_bReConnect = false;
}

CNgnTalkManager * CNgnTalkManager::getInstance()
{
    std::lock_guard<std::mutex> lock(m_talkMutex);
    if (m_pTalkManager == NULL) {
        m_pTalkManager = new CNgnTalkManager();
    }
    return m_pTalkManager;
}

void CNgnTalkManager::destory()
{
    std::lock_guard<std::mutex> lock(m_talkMutex);
    if (m_pTalkManager != NULL) {
        delete m_pTalkManager;
    }
    m_pTalkManager = NULL;
}


/***
 * 获取频道信息
 */
CChannelInfo * CNgnTalkManager::getChannel(const std::string & roomID)
{
    std::map<std::string, CChannelInfo>::iterator it = m_mChannels.find(roomID);
    if (it != m_mChannels.end()) {
        return &(it->second);
    }
    return NULL;
}


/***
 * 获取频道列表信息
 */
std::map<std::string, CChannelInfo> * CNgnTalkManager::getAllChannel()
{
    return &m_mChannels;
}

/***
 * 设置频道信息
 */
void CNgnTalkManager::setChannel(CChannelInfo & channnel)
{
	std::lock_guard<std::mutex> lock(m_talkMutex);
	m_mChannels.insert(std::map<std::string, CChannelInfo>::value_type(channnel.m_strRoomID, channnel));
    m_iRoomNumber++;
}

/***
 * 删除频道信息
 */
void CNgnTalkManager::removeChannel(std::string & roomID)
{
	std::lock_guard<std::mutex> lock(m_talkMutex);
	std::map<std::string, CChannelInfo>::iterator it = m_mChannels.find(roomID);
	if (it != m_mChannels.end()) {
		m_iRoomNumber--;
		m_mChannels.erase(it);
	}
}


/**
* 生成服务器端用的roomid
*/
std::string CNgnTalkManager::genServerRoomID(const std::string & strRoomID)
{
	return mAppKey + strRoomID;
}

/**
* 还原客户端roomid
*/
std::string CNgnTalkManager::getClientRoomID(const std::string & strRoomID)
{
	if (strRoomID.length() > mAppKey.length()) { /* 为了更安全，如果服务器下发的roomid错误的时候 */
		return strRoomID.substr(mAppKey.length());
	}
	return strRoomID;
}

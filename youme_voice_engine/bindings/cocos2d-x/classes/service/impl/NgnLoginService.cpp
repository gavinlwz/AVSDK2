//
//  NgnLoginService.cpp
//  youme_voice_engine
//
//  Created by wanglei on 15/11/13.
//  Copyright © 2015年 youme. All rights reserved.
//


#include "tsk_debug.h"
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include "NgnConfigurationEntry.h"
#include "tsk_time.h"
#include "NgnLoginService.hpp"
#include "YouMeConstDefine.h"
#include "XDNSParse.h"

#include "tsk_debug.h"
#include "SyncUDP.h"

#include "tsk_uuid.h"
#include "serverlogin.pb.h"
#include "YoumeRunningState.pb.h"
#include "MonitoringCenter.h"
#include "NgnMemoryConfiguration.hpp"
#include "XOsWrapper.h"
#include "XConfigCWrapper.hpp"

#include "ReportService.h"
#include "AVStatistic.h"
#include "NgnApplication.h"
#include "YouMeCommon/json/json.h"
#include "YouMeVoiceEngine.h"

extern YOUME_RTC_SERVER_REGION g_serverRegionId;
extern std::string g_extServerRegionName;

#define INTERNET_MTU        (500) /* 数据接收长度 */
#define RTY_TIMES           (3) /* 重试次数 */
#define DEFAULT_UDP_PORT    (5576)  /* 默认聊天UDP端口 */

extern int g_serverMode;
extern std::string g_serverIp;
extern int g_serverPort;

#include <iostream>
using namespace std;


NgnLoginService::NgnLoginService ()
{
}

NgnLoginService::~NgnLoginService ()
{
    TSK_DEBUG_INFO("~NgnLoginService Enter");
    InterUninit();
    TSK_DEBUG_INFO("~NgnLoginService Leave");
}

/***
 * 关闭数据发送喝和接收3个线程（含心跳）
 */
void NgnLoginService::InterUninit()
{
    TSK_DEBUG_INFO("InterUninit Enter");
    
    this->m_loginCondWait.SetSignal();

    if (NULL != this->mRecvThreadHandle) {
        this->m_bRecvThreadExit = true;
        tsk_thread_join(&this->mRecvThreadHandle);
    }

    if (NULL != this->mSendHeartThreadHandle) {
        this->m_heartCondWait.SetSignal();
        tsk_thread_join(&this->mSendHeartThreadHandle);
        this->m_heartCondWait.Reset();
    }
    
    if (NULL != this->mSendDataThreadHandle) {
        this->m_bSendThreadExit = true;
        this->m_sendListSema.Increment();
        tsk_thread_join(&this->mSendDataThreadHandle);
    }
    
    {
        TSK_DEBUG_INFO("m_sendPacketList clear");
        std::lock_guard<std::mutex> lock(this->m_sendListMutex);
        this->m_sendPacketList.clear();
    }
    TSK_DEBUG_INFO("signal UnInit");
    this->m_syncTcp.UnInit();
    
    TSK_DEBUG_INFO("InterUninit Leave");
}

/***
 * 初始化数据发送和接收3个线程（含心跳）
 */
bool NgnLoginService::InterInitHeartSocket()
{
    TSK_DEBUG_INFO("InterInitHeartSocket Enter");
    
    this->m_bRecvThreadExit = false;
    this->m_bSendThreadExit = false;
    this->m_bSocketError = false;
    this->m_iReadErrCount = 0;
    this->m_heartCondWait.Reset();

    assert(this->mRecvThreadHandle == NULL);
    assert(this->mSendHeartThreadHandle == NULL);
    assert(this->mSendDataThreadHandle == NULL);
    
    tsk_thread_create(&this->mRecvThreadHandle, RecvTCPThread, this); // 启动一个线程用来接收
    tsk_thread_create(&this->mSendDataThreadHandle, SendTCPThread, this); // 启动一个线程用来发送数据
    tsk_thread_create(&this->mSendHeartThreadHandle, SendHeartThread, this); // 启动一个线程用来发送心跳

    TSK_DEBUG_INFO("InterInitHeartSocket Leave");
    
    return true;
}


/***
 * 发送消息队列中数据（含心跳）
 */
TSK_STDCALL void * NgnLoginService::SendTCPThread (void * arg){
    NgnLoginService * pThis = (NgnLoginService *)arg;
    
    while (true) {
        pThis->m_sendListSema.Decrement();
        
        if (pThis->m_bSocketError) {
            TSK_DEBUG_ERROR("####Login service @send data@ find socket is broken, exit");
            break;
        }
        
        if (pThis->m_bSendThreadExit) {
            TSK_DEBUG_INFO("####Login service SendTCPThread exit");
            break;
        }
        
        {
            youmecommon::CXSharedArray<char> pPacket;
            std::lock_guard<std::mutex> lock(pThis->m_sendListMutex);
            std::list< YOUMETALKTCPPacketInfo >::iterator iter;
            TSK_DEBUG_INFO("Login service message queue size: [ %lu ]", pThis->m_sendPacketList.size());
            
            for (iter = pThis->m_sendPacketList.begin(); iter != pThis->m_sendPacketList.end(); ) {
                if(pThis->m_bSendThreadExit) {
                    TSK_DEBUG_ERROR("####Login service SendTCPThread exit2");
                    break;
                }
                
                pPacket = iter->pPacket;
                
                if (pPacket.Get() == NULL) {
                    TSK_DEBUG_ERROR("####Login service tcp packet is null");
                    continue;
                }
                
                int sendedLen = pThis->m_syncTcp.SendData((const char*)pPacket.Get(), pPacket.GetBufferLen());
                
                if (sendedLen != pPacket.GetBufferLen() ) {
                    pThis->m_bSocketError = true;
                    TSK_DEBUG_ERROR("####Login service tcp send fail, len:%d success len:%d", pPacket.GetBufferLen(), sendedLen);
                    break;
                }
                
                TSK_DEBUG_INFO("Login service send message command type: [ %d ]", iter->commandType);
                
                pThis->m_sendPacketList.erase(iter++);
            }
        }
        
        if (pThis->m_bSendThreadExit) {
            TSK_DEBUG_ERROR("####Login service SendTCPThread exit3");
            break;
        }
    }
    
    TSK_DEBUG_INFO("Login service @send data@ thread quit!");
    
    return NULL;
}

/***
 * 添加消息到消息队列
 */
void NgnLoginService::AddTCPQueue(YouMeProtocol::MSG_TYPE commandType,const char* buffer, int iBufferSize){
    YOUMETALKTCPPacketInfo packet;
    packet.commandType = commandType;
    packet.pPacket = youmecommon::CXSharedArray<char>((int)iBufferSize);
    memcpy(packet.pPacket.Get(), buffer , iBufferSize);
    
    {   // 先加入到 map 中，然后发送
        std::lock_guard<std::mutex> lock(m_sendListMutex);
        m_sendPacketList.push_back(packet);
    }
    m_sendListSema.Increment();
}


/***
 * 发送心跳线程
 */
TSK_STDCALL void *NgnLoginService::SendHeartThread (void *arg)
{
    NgnLoginService * pThis = (NgnLoginService *)arg;
    pThis->m_iLostHeartPacket = 0;
    
    int iTimeout = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::HEART_TIMEOUT, NgnConfigurationEntry::DEFAULT_HEART_TIMEOUT);
    int iTimeoutCount =CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::HEART_TIMEOUT_COUNT, NgnConfigurationEntry::DEFAULT_TIMEOUT_COUNT);
    
    YouMeProtocol::YouMeVoice_Command_Heart heartProtocol;
    heartProtocol.set_allocated_head (CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_Heart));
    heartProtocol.set_sessionid (pThis->m_iSessionID);
    TSK_DEBUG_INFO("Login service m_iSessionID: %d", pThis->m_iSessionID);
    std::string strSendData;
    heartProtocol.SerializeToString (&strSendData);

    while (true)
    {
		if ((pThis->m_bSocketError) || (pThis->m_iLostHeartPacket >= iTimeoutCount)) { // 网络断开 或者 连续丢包达到一定的个数了，要退出了
			TSK_DEBUG_ERROR("####Login service @send heart@ socket is broken %d or lost max heart %d, reconnect!", pThis->m_bSocketError, pThis->m_iLostHeartPacket);
            
            {
                /* 心跳丢包重连 数据上报 */
                ReportService * report = ReportService::getInstance();
                youmeRTC::ReportChannel heart;
				heart.operate_type = REPORT_CHANNEL_LOSTHEART;
                std::string strShortRoomID;
                CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( pThis->m_strRoomID, strShortRoomID );
                heart.roomid = strShortRoomID;
                heart.sessionid = pThis->m_iSessionID;
                heart.lost_heart = pThis->m_iLostHeartPacket;
                heart.heart_interval = iTimeout;
                heart.heart_count = iTimeoutCount;
                heart.on_disconnect = 1;
				heart.sdk_version = SDK_NUMBER;
                heart.platform = NgnApplication::getInstance()->getPlatform();
                heart.canal_id = NgnApplication::getInstance()->getCanalID();
                heart.package_name = NgnApplication::getInstance()->getPackageName();
                report->report(heart);
            }

            if (pThis->m_pCallback) {
                pThis->m_pCallback->OnDisconnect();
            }

            break;
        }
        
        pThis->m_iLostHeartPacket++;
        pThis->AddTCPQueue(YouMeProtocol::MSG_Heart , strSendData.c_str() , strSendData.length());
        
        if (youmecommon::WaitResult_Timeout != pThis->m_heartCondWait.WaitTime(iTimeout * 1000)) {
            break;
        }
        TSK_DEBUG_INFO("Login service lost heart count: [ %d ]", pThis->m_iLostHeartPacket);
    }
    
    TSK_DEBUG_INFO("Login service @send heart@ thread quit!");
    return NULL;
}

/***
 * 接收服务器数据线程（含心跳）
 */
TSK_STDCALL void *NgnLoginService::RecvTCPThread (void *arg)
{
    NgnLoginService* pThis = (NgnLoginService*)arg;
    
    TSK_DEBUG_INFO("RecvTCPThread start");
    static int count = 0;
//    TSK_DEBUG_INFO("pThis->m_syncTcp:%d",pThis->m_syncTcp);
    while (!pThis->m_bRecvThreadExit)
    {       
		if (pThis->m_bSocketError) {
			TSK_DEBUG_ERROR("####Login service @recv data@ find socket is broken, exit!");
			break;
		}

        int ret = pThis->m_syncTcp.CheckRecv(0, 500 * 1000);
        
        if (pThis->m_bRecvThreadExit) {
            TSK_DEBUG_INFO("####Login service RecvTCPThread m_bRecvThreadExit, exit");
            break;
        }
        
        if (-1 == ret) { // socket 错误直接退出，可以通过关闭socket 来达到退出目的
            TSK_DEBUG_ERROR("####Login service RecvTCPThread recv data fail, exit");
            pThis->m_bSocketError = true;
            break;
        }
        
        if (0 == ret) { // 超时了，continue一下
            // TSK_DEBUG_INFO("Login service recv message timeout, wait!");
            continue;
        }
        
        pThis->DealRead(); // 其他情况表示已经触发了可用的条件
    }
    
    TSK_DEBUG_INFO("Login service @recv data@ thread quit!");
    return NULL;
}


/***
 * 读取数据
 */
void NgnLoginService::DealRead()
{
    youmecommon::CXSharedArray<char> buffer;
    int iRecvLen = m_syncTcp.RecvData(buffer);
    
    if (iRecvLen <= 0) {
        this->m_iReadErrCount++;
        TSK_DEBUG_ERROR("####Login service DealRead: failed to read tcp packet:%d", iRecvLen);
        
        if (this->m_iReadErrCount == 2) {
            this->m_bSocketError = true;
            TSK_DEBUG_ERROR("####Login service DealRead: ReadErrorCount:%d, reconnect", this->m_iReadErrCount);
            
            if (this->m_pCallback) {
                this->m_pCallback->OnDisconnect();
            }
        }
        
        return;
    }
    
    this->m_iReadErrCount = 0;
    YouMeProtocol::ServerReqHead head; // 使用proto buf 解析数据
    
    if (!head.ParseFromArray(buffer.Get(), iRecvLen)) {
        TSK_DEBUG_ERROR("####Login service DealRead: protobuf parsing failed:%d",iRecvLen);
        return;
    }
    
    
    if (head.head().msgtype() == YouMeProtocol::MSG_Kickout) {
        TSK_DEBUG_ERROR("####Login service DealRead server kickout");
        
        ReportQuitData::getInstance()->m_kickout_count++;
        
        if (this->m_pCallback) {
            this->m_pCallback->OnDisconnect();
        }
    }
    
    else if (head.head().msgtype() == YouMeProtocol::MSG_Heart) {
        YouMeProtocol::YouMeVoice_Command_HeartResponse responseProtocol;
        if (responseProtocol.ParseFromArray (buffer.Get(), iRecvLen)) {
            if (responseProtocol.head ().code () == 0) {
                this->m_iLostHeartPacket = 0;
            }
        }
    }
    
	else if (head.head().msgtype() == YouMeProtocol::MSG_UploadLog) { // 服务器主动出发需要上传日志
		MonitoringCenter::getInstance()->UploadLog(UploadType_Notify, 0, false);
	}
    
    else if(head.head().msgtype() == YouMeProtocol::MSG_COMMON_STATUS_SERVER) {
        YouMeProtocol::YouMeVoice_Command_CommonStatusServer response;
        
        if (response.ParseFromArray(buffer.Get(), iRecvLen)) {
            std::string strUserID = response.userid();
            int iStatus = response.status();
            int eventType = response.eventtype();
            
            if (NULL != this->m_pCallback) {
                this->m_pCallback->OnCommonStatusEvent((STATUS_EVENT_TYPE_t)eventType, strUserID, iStatus);
            }
        }
    }
    
//    else if( head.head().msgtype() == YouMeProtocol::MSG_MIC_CTR_SERVER )
//    {
//        YouMeProtocol::YouMeVoice_Command_Recive_MicControl response;
//        if (response.ParseFromArray(buffer.Get(), iRecvLen))
//        {
//            std::string strUserID = response.userid();
//            int iStatus = response.status();
//            if (NULL != m_pCallback) {
//                m_pCallback->OnReciveMicphoneControllMsg (strUserID, iStatus);
//            }
//        }
//    }
//    else if( head.head().msgtype() == YouMeProtocol::MSG_SPEAKER_CTR_SERVER )
//    {
//        YouMeProtocol::YouMeVoice_Command_Recive_SpeakerControl response;
//        if (response.ParseFromArray(buffer.Get(), iRecvLen))
//        {
//            std::string strUserID = response.userid();
//            int iStatus = response.status();
//            if (NULL != m_pCallback) {
//                m_pCallback->OnReciveSpeakerControllMsg (strUserID, iStatus);
//            }
//        }
//    }
//    else if( head.head().msgtype() == YouMeProtocol::MSG_AVOID_MEMEBER_SERVER )
//    {
//        YouMeProtocol::YouMeVoice_Command_Recive_AvoidControl response;
//        if (response.ParseFromArray(buffer.Get(), iRecvLen))
//        {
//            std::string strUserID = response.userid();
//            int iStatus = response.status();
//            if (NULL != m_pCallback) {
//                m_pCallback->OnReciveAvoidControllMsg (strUserID, iStatus);
//            }
//        }
//    }
//
//    else if( head.head().msgtype() == YouMeProtocol::MSG_ROOM_MEMBERS_CHANGE ) {
//        YouMeProtocol::YouMeVoice_Command_Recive_Members response;
//        
//        if(response.ParseFromArray(buffer.Get(), iRecvLen)){
//            if (NULL != m_pCallback) {
//                std::string strUserIDs;
//                
//                for (int i=0; i<response.userid_size(); i++) {
//                    if(i!=0) strUserIDs.append(",");
//                    strUserIDs.append(response.userid(i));
//                }
//                
//                if (NULL != this->m_pCallback) {
//                    this->m_pCallback->OnReciveRoomMembersChangeNotify ( strUserIDs );
//                }
//            }
//            
//        }
//    }

	/** 加入频道 */
	else if (head.head().msgtype() == YouMeProtocol::MSG_JOIN_ROOM_SERVER) {
		YouMeProtocol::YouMeVoice_Command_JoinRoomResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)) {
            if (m_pCallback) {
                if (response.success()){
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_JOIN, NgnLoginServiceCallback::ROOM_EVENT_SUCCESS);
				} else {
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_JOIN, NgnLoginServiceCallback::ROOM_EVENT_FAILED);
				}
			}
		}
	}

	/** 离开频道 */
	else if (head.head().msgtype() == YouMeProtocol::MSG_LEAVE_ROOM_SERVER) {
		YouMeProtocol::YouMeVoice_Command_LeaveRoomResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
            if (m_pCallback) {
                if (response.success()) {
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_LEAVE, NgnLoginServiceCallback::ROOM_EVENT_SUCCESS);
                } else {
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_LEAVE, NgnLoginServiceCallback::ROOM_EVENT_FAILED);
				}
			}
		}
	}

	/** 切换频道 */
	else if (head.head().msgtype() == YouMeProtocol::MSG_SPEAK_TO_ROOM_SERVER) {
		YouMeProtocol::YouMeVoice_Command_SpeakToRoomResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)) {
            if (m_pCallback) {
                if (response.success()) {
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_SPEAK_TO, NgnLoginServiceCallback::ROOM_EVENT_SUCCESS);
                } else {
                    m_pCallback->OnRoomEvent(response.roomid(), NgnLoginServiceCallback::ROOM_EVENT_SPEAK_TO, NgnLoginServiceCallback::ROOM_EVENT_FAILED);
				}
            }
		}
	}
    
    /** get userID from sessionID */
    else if( head.head().msgtype() == YouMeProtocol::MSG_SESSIONID_TO_USERID ) {
        TSK_DEBUG_INFO ("@@ --%s() MsgType:MSG_SESSIONID_TO_USERID", __FUNCTION__);
        YouMeProtocol::YouMeVoice_Command_Session2UserIdResponse response;
        YouMeProtocol::YouMeVoice_Command_Session2UserIdResponse::Speaker speakerInfo;
        if(response.ParseFromArray(buffer.Get(), iRecvLen)){
            int speakerNum = response.result_size();
            if ((speakerNum > 0) && (speakerNum < 100) && m_pCallback) {
                SessionUserIdPairVector_t idPairVector;
                idPairVector.reserve(speakerNum);
                for (int i = 0; i < speakerNum; i++) {
                    SessionUserIdPair_t idPair;
                    idPair.sessionId = response.result(i).sessionid();
                    idPair.userId = response.result(i).userid();
                    idPairVector.push_back(idPair);
                }
                m_pCallback->OnReceiveSessionUserIdPair(idPairVector);
            }
        }
    }
	else if(  head.head().msgtype() == YouMeProtocol::MSG_GET_USER_LIST_SERVER ){
        YouMeProtocol::YouMeVoice_Command_ChannelUserList_Response response;
        if( response.ParseFromArray( buffer.Get(), iRecvLen )){
            std::string strRoomID = response.req_channel_id();
            
			if (response.userid_size() <= 0)
			{
				TSK_DEBUG_WARN("@@ --%s() MsgType:MSG_GET_USER_LIST_SERVER[Warning: response.userid_size():%d]", __FUNCTION__, response.userid_size());
			}
              
			{
                std::list<MemberChangeInner> listMemberChange;
				listMemberChange.clear();
                for( int i = 0 ;i < response.userid_size(); i++ ){
                    MemberChangeInner memChange;
                    
                    memChange.userID = response.userid( i );
                    memChange.sessionId = 0;
                    memChange.isJoin = true;
                    
                    listMemberChange.push_back( memChange );
                }
                
                m_pCallback->OnMemberChange(strRoomID, listMemberChange,  false );
            }

            //用户请求房间内用户列表时，同时将userid和sessionid映射关系保存在本地
            {
                SessionUserIdPairVector_t idPairVector;
                idPairVector.reserve(response.userinfo_size());
                for( int i = 0 ;i < response.userinfo_size(); i++ ){
                    SessionUserIdPair_t idPair;
                    idPair.sessionId = response.userinfo( i ).sessionid();
                    idPair.userId = response.userinfo( i ).userid();
                    idPairVector.push_back(idPair);
                }
                m_pCallback->OnReceiveSessionUserIdPair(idPairVector);
            }
            
            if( response.next_index() != 0 ){
                YouMeProtocol::YouMeVoice_Command_ChannelUserList_Request  getReq;
                getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_GET_USER_LIST));
                getReq.set_sessionid(m_iSessionID);
                getReq.set_channel_id( strRoomID );
                getReq.set_start_index( response.next_index() );
                getReq.set_req_count( response.left_count() );
                
                std::string strReqData;
                getReq.SerializeToString(&strReqData);
                
                AddTCPQueue(YouMeProtocol::MSG_GET_USER_LIST, strReqData.c_str(), strReqData.length());
            }
        }
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_USER_JOIN_LEAVE_NOTIFY ){
        YouMeProtocol::YouMeVoice_Command_ChannelUserJoinLeaveNotify notify;
        if( notify.ParseFromArray( buffer.Get(), iRecvLen )){
            std::string strRoomID = notify.channel_id();
            std::list<MemberChangeInner> listMemberChange;
            
            for( int i = 0; i < notify.user_list_size(); i++ ){
                MemberChangeInner memChange;
                
                memChange.userID = notify.user_list( i ).userid();
                memChange.sessionId = notify.user_list(i).sessionid();
                memChange.isJoin = notify.user_list( i ).flag() == 1 ? true : false ;
                
                listMemberChange.push_back( memChange );
             
                if(  memChange.isJoin == false ){
                    AVStatistic::getInstance()->NotifyVideoStat( memChange.userID,  false );
                    
                    if (m_pCallback) {
                        m_pCallback->OnOtherAudioInputStatusChgNfy(memChange.userID, 0);
                        m_pCallback->OnOtherVideoInputStatusChgNfy(memChange.userID, 0, true ,0);
                    }
                }
            }
            if (m_pCallback) {
                m_pCallback->OnMemberChange( strRoomID, listMemberChange, true );
            }
        }
    }
	else if (head.head().msgtype() == YouMeProtocol::MSG_FIGHT_4_MIC_INIT){
		YouMeProtocol::YouMeVoice_Command_Fight4MicInitResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), response.room_id(), response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_FIGHT_4_MIC_DEINIT){
		YouMeProtocol::YouMeVoice_Command_Fight4MicDeinitResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), response.room_id(), response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_FIGHT_4_MIC){
		YouMeProtocol::YouMeVoice_Command_Fight4MicResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), response.mic_enable_flag(), response.talk_time(), response.error_code(), response.room_id(), response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_RELEASE_MIC){
		YouMeProtocol::YouMeVoice_Command_ReleaseMicResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), response.room_id(), response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_FIGHT_4_MIC_NOTIFY){
		YouMeProtocol::YouMeVoice_Command_Fight4MicNotifyRequest notify;
		if (notify.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnGrabMicNotify(notify.mode(), notify.event_type(), notify.mic_right(), notify.mic_enable_flag(), notify.is_mic_flag(), 
					notify.talk_time(), notify.room_id(), notify.user_id(), notify.json_str());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_INVITE){
		YouMeProtocol::YouMeVoice_Command_InviteResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), "", response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_ACCEPT){
		YouMeProtocol::YouMeVoice_Command_AcceptResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, response.talk_time_out(), response.error_code(), "", response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_INVITE_NOTIFY){
		YouMeProtocol::YouMeVoice_Command_InviteNotifyRequest notify;
		if (notify.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnInviteMicNotify(notify.mode(), notify.event_type(), notify.error_code(), notify.talk_time_out(), "", notify.from_user_id(), notify.to_user_id(), notify.json_str());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_INVITE_BYE){
		YouMeProtocol::YouMeVoice_Command_InviteByeResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), "", response.session_id());
			}
		}
	}
	else if (head.head().msgtype() == YouMeProtocol::MSG_INVITE_INIT)
	{
		YouMeProtocol::YouMeVoice_Command_InviteInitResponse response;
		if (response.ParseFromArray(buffer.Get(), iRecvLen)){
			if (m_pCallback) {
				m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, response.error_code(), "", response.session_id());
			}
		}
	}
    
    /* 屏蔽别人视频，自己的response */
    else if ( head.head().msgtype() == YouMeProtocol::MSG_MASK_VIDEO_BY_USERID ) {
        YouMeProtocol::YouMeVoice_Command_MaskVideoByUserIdResponse response;
        if (response.ParseFromArray(buffer.Get(), iRecvLen)) {
            //int sessionId = response.sessionid();
            if (m_pCallback) { // TODO
            }
        }
    }
    
    /* 被屏蔽者收到的通知 */
    else if ( head.head().msgtype() == YouMeProtocol::MSG_MASK_VIDEO_BY_USERID_NFY ) {
        YouMeProtocol::YouMeVoice_Command_MaskVideoNotifyRequest response;
        if(response.ParseFromArray(buffer.Get(), iRecvLen)) {
            std::string selfUserId = response.to_user_id();
            std::string fromUserId = response.from_user_id();
            int mask = response.status();
            if (m_pCallback) {
                m_pCallback->OnMaskVideoByUserIdNfy(selfUserId, fromUserId, mask);
            }
        }
    }
    
    /* 开关摄像头，自己的response */
//    else if ( head.head().msgtype() == YouMeProtocol::MSG_CAMERA_STATUS ) {
//    }
    
    /* 开关摄像头，别人收到的response */
//    else if ( head.head().msgtype() == YouMeProtocol::MSG_CAMERA_STATUS_NFY ) {
//        YouMeProtocol::YouMeVoice_Command_CameraNotifyRequest response;
//        if(response.ParseFromArray(buffer.Get(), iRecvLen)) {
//            std::string camChgUserId = response.user_id();
//            int cameraStatus = response.status();
//            if (m_pCallback) {
//                m_pCallback->OnWhoseCamStatusChgNfy(camChgUserId, cameraStatus);
//            }
//        }
//    }
	else if( head.head().msgtype() == YouMeProtocol::MSG_SEND_MESSAGE_SERVER ){
        YouMeProtocol::YouMeVoice_Command_SendMessageResponse response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent( head.head().msgtype(), 0, response.msg_id(), head.head().code(), response.to_channel_id(),  response.sessionid() );
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_MESSAGE_NOTIFY ){
        YouMeProtocol::YouMeVoice_Command_MessageNotify response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent( head.head().msgtype(), 0, response.msg_id(), 0, response.to_channel_id(),  response.from_sessionid() , response.msg_content() );
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_KICK_USRE_ACK )
    {
        YouMeProtocol::YouMeVoice_Command_KickingResponse response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent( head.head().msgtype(), 0, 0, head.head().code(), response.channel_id(),  response.sessionid() , response.user_id() );
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_KICK_USRE_NOTIFY )
    {
        YouMeProtocol::YouMeVoice_Command_KickingNotify response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                int kickReason = 0;
                if( response.kick_code() == YouMeProtocol::KICK_BY_ADMIN )
                {
                    kickReason = YOUME_KICK_ADMIN;
                    
                }
                else if( response.kick_code() == YouMeProtocol::KICK_BY_RELOGIN )
                {
                    kickReason = YOUME_KICK_RELOGIN;
                }
                
                stringstream ss;
                ss<<response.src_user_id()<<","<<kickReason<<","<<response.kick_time()<<endl;
                m_pCallback->OnKickFromChannel( response.channel_id() , ss.str() );
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_KICK_OTHER_NOTIFY )
    {
        YouMeProtocol::YouMeVoice_Command_KickingNotify response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback )
            {
                m_pCallback->OnCommonEvent( YouMeProtocol::MSG_KICK_OTHER_NOTIFY, 0, 0, head.head().code(), response.channel_id(),  0 , response.kicked_user_id() );
        
            }
        }
    }
    /* 收到别人的音视频输入状态改变 */
    else if ( head.head().msgtype() == YouMeProtocol::MSG_MEDIA_AVINPUT_STAT_NOTIFY ) {
        YouMeProtocol::YouMeVoice_Command_AVInput_Notify response;
        if(response.ParseFromArray(buffer.Get(), iRecvLen)) {
            std::string inputChgUserId = response.userid();
            int mediaType = response.media_type();
            int op = response.op();
            if (m_pCallback) {
                switch (mediaType) {
                    case 1:
                        m_pCallback->OnOtherAudioInputStatusChgNfy(inputChgUserId, op);
                        break;
                    case 2:
                        m_pCallback->OnOtherVideoInputStatusChgNfy(inputChgUserId, op, false, 0 );
                        break;
                    case 3:
                        m_pCallback->OnOtheShareInputStatusChgNfy(inputChgUserId, op);
                    default:
                        break;
                }
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_MEDIA_AVINPUT_STAT_ACK ) {
        // nothing to do
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_QUERY_USER_VIDEO_INFO_ACK ) {
        YouMeProtocol::YouMeVoice_Command_Query_User_Video_Info_Rsp response;
        if(response.ParseFromArray(buffer.Get(), iRecvLen)) {
            youmecommon::Value root;
            for (int i = 0; i < response.user_video_info_list_size(); ++i) {
                YouMeProtocol::YouMeVoice_UserVedioInfo uservideoinfo =  response.user_video_info_list(i);
                youmecommon::Value videoVaule;
                std::string userid = uservideoinfo.other_userid();;
                videoVaule["userid"] = youmecommon::Value(userid);
                for (int n = 0; n < uservideoinfo.video_info_list_size(); ++n) {
                    int type = uservideoinfo.video_info_list(n).resolution_type();
                    videoVaule["resolution"].append(youmecommon::Value(type));
                }
                root["uservideoinfo"].append(videoVaule);
            }
            if( m_pCallback ){
                string tmpstr = root.toStyledString();
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), "",  response.sessionid(), tmpstr);
            }
        }
    }
    else if ( head.head().msgtype() == YouMeProtocol::MSG_SET_RECV_VIDEO_INFO_ACK ) {
        YouMeProtocol::YouMeVoice_Command_Set_User_Video_Info_Rsp response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), "",  response.sessionid(), "");
            }
        }
    }
    else if (head.head().msgtype() == YouMeProtocol::MSG_SET_RECV_VIDEO_INFO_NOTIFY) {
        YouMeProtocol::YouMeVoice_Command_Set_User_Video_Info_Notify notify;
        if (notify.ParseFromArray(buffer.Get(), iRecvLen)) {
            YouMeProtocol::YouMeVoice_Video_info videoInfo = notify.video_info();

            SetUserVideoInfoNotify notifyInfo;
            notifyInfo.usrId = notify.set_user_id();
            notifyInfo.sessionId = notify.set_sessionid();
            notifyInfo.videoType = videoInfo.resolution_type();


            if( m_pCallback ){
                m_pCallback->OnUserVideoInfoNotify(notifyInfo);
            }
        }
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_BUSS_REPORT_ACK )
    {
        //do nothing
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_SET_PUSH_SINGLE_ACK )
    {
        YouMeProtocol::YouMeVoice_Video_SetPushSingle_Rsp response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), response.channelid(),  response.sessionid(), "");
            }
        }
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_REMOVE_PUSH_SINGLE_ACK )
    {
        //do nothing
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_SET_PUSH_MIX_ACK )
    {
        YouMeProtocol::YouMeVoice_Video_SetPushMix_Rsp response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), response.channelid(),  response.sessionid(), "");
            }
        }
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_CLEAR_PUSH_MIX_ACK )
    {
        //do nothing
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_REMOVE_PUSH_MIX_USER )
    {
        YouMeProtocol::YouMeVoice_Video_AddPushMixUser_Rsp response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), response.channelid(),  response.sessionid(), response.userid());
            }
        }
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_REMOVE_PUSH_MIX_USER_ACK )
    {
        //do nothing
    }
    else if( head.head().msgtype() == YouMeProtocol::MSG_OTHER_SET_PUSH_MIX )
    {
        YouMeProtocol::YouMeVoice_Video_OtherSetPushMix_Notify response;
        if( response.ParseFromArray(buffer.Get(), iRecvLen) ){
            if( m_pCallback ){
                m_pCallback->OnCommonEvent(head.head().msgtype(), 0, 0, head.head().code(), response.channelid(),  0, response.userid());
            }
        }
    }
    else {
        assert(false);
    }
}

void NgnLoginService::SetToken( const string& strToken, uint32_t timeStamp){
    std::lock_guard<std::mutex> lock(this->m_tokenMutex);
    m_strToken = strToken;
    m_timeStamp = timeStamp;
}

/***
 * 重新登录
 */
YouMeErrorCode NgnLoginService::ReLoginServerSync(const string & strUserID,
                                                  YouMeUserRole_t eUserRole,
                                                  const string & strRedirectServer,
                                                  int iRedirectServerPort,
                                                  const string & strRoomID,
                                                  NgnLoginServiceCallback * pCallback,
                                                  std::string & strServerIP,
                                                  int & iRTPUdpPort,
                                                  int & iSessionID,
                                                  int & iServerPort,
                                                  uint64_t & uBusinessID,
                                                  bool bVideoAutoReceive)
{
    /** 
     * 目前来看处理是一样的，后续如有额外处理再增加。
     * (PS: 之前的ReLogin只是比Login少了两次Report，其它的都一样)
     */
    TSK_DEBUG_INFO("======== ReLogin ========");
    return LoginServerSync(strUserID, eUserRole, strRedirectServer, iRedirectServerPort, strRoomID, pCallback, strServerIP, iRTPUdpPort, iSessionID, iServerPort, uBusinessID, bVideoAutoReceive);
}


/***
 * 登录
 *
 */
YouMeErrorCode NgnLoginService::LoginServerSync (const string & strUserID,
                                                 YouMeUserRole_t eUserRole,
                                                 const string &strRedirectServer,
                                                 int iRedirectServerPort,
                                                 const string & strRoomID,
                                                 NgnLoginServiceCallback * pCallback,
                                                 std::string & strServerIP,
                                                 int & iRTPUdpPort,
                                                 int & iSessionID,
                                                 int & iServerPort,
                                                 uint64_t & uBusinessID,
                                                 bool bVideoAutoReceive)
{
    this->m_strUserID = strUserID;
    this->m_strRoomID = strRoomID;
    this->m_loginCondWait.Reset();
    this->m_pCallback = pCallback;
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    strServerIP = "";
    iServerPort = DEFAULT_UDP_PORT;
    iSessionID = 0;
    
    TSK_DEBUG_INFO("======== Login ========");
    
    if (SERVER_MODE_FIXED_IP_MCU == g_serverMode) {
        strServerIP = g_serverIp;
        iServerPort = g_serverPort;
    } else {
        errCode = RedirectToMcuServer(strRedirectServer, iRedirectServerPort, strRoomID, strServerIP, iServerPort);
        if (YOUME_SUCCESS != errCode) {
            return errCode;
        }
    }
    
    if (strServerIP.empty()) { // 没有获取到服务器IP地址，回调失败
        return YOUME_ERROR_NETWORK_ERROR;
    }

    errCode = LoginToMcuServer(strUserID, strRoomID, eUserRole, strServerIP, iServerPort, iRTPUdpPort, iSessionID, uBusinessID, bVideoAutoReceive);

   if (YOUME_SUCCESS != errCode) {
        return errCode;
    }
    
    if (0 == iSessionID) { // 没有获取到session
        return YOUME_ERROR_NETWORK_ERROR;
    }
    
    this->m_iSessionID = iSessionID;
    this->m_iServerPort = iServerPort;
    this->m_strServerIP = strServerIP;

    InterInitHeartSocket();
    TSK_DEBUG_INFO ("YOUME_EVENT_JOIN_OK");
    
    return YOUME_SUCCESS;
}

/***
 * 获取聊天室服务器ip, port
 * 参数：
 *  [in ] strRedirectServer     - 重定向服务器域名
 *  [in ] iRedirectServerPort   - 重定向服务器端口
 *  [in ] strRoomID             - 房间ID
 *  [out] strServerIP           - 聊天室服务器IP
 *  [out] iServerPort           - 聊天室服务器端口
 *
 * 返回值：
 *  YouMeErrorCode:非0 失败, 0 成功<服务器响应成功，未必真的取到服务器ip和端口，也可能是错误码>
 */
YouMeErrorCode NgnLoginService::RedirectToMcuServer(const string & strRedirectServer,
                                       const int iRedirectServerPort,
                                       const string & strRoomID,
                                       string & strServerIP,
                                       int & iServerPort)
{
    YouMeErrorCode errorCode = YOUME_SUCCESS;
    XString strRedirectIP;
    std::vector<XString> ipList;
    int iStatus = -1000;
    uint64_t ulStart = 0;
    int iRery = 0;
    YouMeProtocol::Bridge getBridgeProto;
    std::string strProbufData;
    
    ReportQuitData::getInstance()->m_redirect_count++;

    
    TSK_DEBUG_INFO ("#### (TCP Redirect)Parsing redirect server:%s, port:%d", strRedirectServer.c_str(), iRedirectServerPort);
    ulStart = tsk_time_now();
    if(!youmecommon::CXDNSParse::IsIPAddress(LocalToXString(strRedirectServer))) {
        youmecommon::CXDNSParse::ParseDomain2(LocalToXString(strRedirectServer), ipList);
    }else{
        ipList.push_back(UTF8TOXString(strRedirectServer));
    }
    ReportQuitData::getInstance()->m_dns_count++;
    
    //上报域名解析
    {
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportDNSParse dns;
        dns.parse_addr = strRedirectServer;
        dns.parse_usetime = (tsk_time_now() - ulStart);
        dns.parse_result = 0;
        for (int i = 0; i < ipList.size(); i++) {
            dns.parse_iplist.append( XStringToUTF8(ipList[i]) ).append(";");  // ip列表
        }
        dns.sdk_version = SDK_NUMBER;
        dns.platform = NgnApplication::getInstance()->getPlatform();
        dns.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report( dns );
    }
    
    if (!ipList.empty()) {
        strRedirectIP = ipList[0];
        for (int i = 0; i < ipList.size(); i++) {
            TSK_DEBUG_INFO ("#### redirect server ip[%d]:%s", i, XStringToLocal(ipList[i]).c_str());
        }
    } else {
        TSK_DEBUG_ERROR ("#### Parsing redirect server ip failed! ");
        errorCode = YOUME_ERROR_NETWORK_ERROR; // 解析ip失败
        goto redirect_mcu_end;
    }
    
    ulStart = tsk_time_now(); //重定向开始上报
    
    // 重定向获取
    getBridgeProto.set_bridgeid (strRoomID);
    getBridgeProto.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_Redirect));
    getBridgeProto.set_mode(CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_MCU_MODE, (int)2));
    getBridgeProto.set_area(g_extServerRegionName);
    getBridgeProto.SerializeToString (&strProbufData);
    
    m_RedirectTcp.UnInit();
	if (!m_RedirectTcp.Init(XStringToLocal(strRedirectIP), iRedirectServerPort, 25)) {
        TSK_DEBUG_ERROR ("#### Redirect server socket init failed!");
        errorCode = YOUME_ERROR_NETWORK_ERROR;
        goto redirect_mcu_end;
    }
    
    if (!m_RedirectTcp.Connect(10)) {
        if (m_bAborted) {
            TSK_DEBUG_INFO ("==Redirect aborted");
            errorCode = YOUME_ERROR_USER_ABORT;
        } else {
        TSK_DEBUG_ERROR ("Failed to connect to the redirect server");
        errorCode = YOUME_ERROR_NETWORK_ERROR;
        }
        goto redirect_mcu_end;
    }
    
    for (iRery = 0; iRery < RTY_TIMES ; iRery++)
    {
        if (m_bAborted) {
            TSK_DEBUG_INFO ("==Redirect aborted");
            errorCode = YOUME_ERROR_USER_ABORT;
            goto redirect_mcu_end;
        }
            
        int sentBytes = m_RedirectTcp.SendData(strProbufData.c_str (), strProbufData.length());
        if (strProbufData.length() != sentBytes) {
            TSK_DEBUG_ERROR ("Failed to send data to the redirect server");
            continue;
        }
            
        youmecommon::CXSharedArray<char> recvBuffer;
        int recvNum = m_RedirectTcp.RecvData (recvBuffer);
        if (recvNum <= 0) {
            TSK_DEBUG_ERROR ("#### RecvData failed, retry [ %d ]", iRery + 1 );
            continue;
        } else {
            YouMeProtocol::Serveraddr responseProtocol;
                
            if (!responseProtocol.ParseFromArray (recvBuffer.Get(), recvNum)) {
                TSK_DEBUG_ERROR ("#### Redirect server protocol parse failed retry [ %d ]", iRery + 1);
                continue;
            }
                
            iStatus = responseProtocol.status();
                
            if (0 == iStatus) {
                strServerIP = responseProtocol.ip();
                iServerPort = responseProtocol.port();
            } else {
                if (1 == iStatus) {
                    continue;
                }
            }
            TSK_DEBUG_INFO ("#### Redirected to MCU server:%s, port:%d", responseProtocol.ip ().c_str (), responseProtocol.port ());
            break;
        }
    }
    

    
    if (iRery == RTY_TIMES) { // 超过最大重试次数
        TSK_DEBUG_ERROR ("#### Redirect server retry over max times, quit!");
        errorCode = YOUME_ERROR_NETWORK_ERROR;
    }

redirect_mcu_end:
    m_RedirectTcp.UnInit();
    {
        // 新的数据上报
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportRedirect redirect;
        redirect.redirect_server = XStringToLocal(strRedirectIP);
        redirect.redirect_port = iRedirectServerPort;
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        redirect.roomid = strShortRoomID;
        redirect.retry_times = iRery;
        redirect.redirect_usetime = (tsk_time_now() - ulStart);
        redirect.redirect_result = -errorCode;
        redirect.redirect_status = iStatus;
        redirect.mcu_server = strServerIP;
        redirect.mcu_port = iServerPort;
		redirect.sdk_version = SDK_NUMBER;
        redirect.server_region = g_serverRegionId;
        redirect.platform = NgnApplication::getInstance()->getPlatform();
        redirect.canal_id = NgnApplication::getInstance()->getCanalID();
        redirect.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(redirect);
    }
    
    return errorCode;
}


/***
 * 获取聊天室会话session_id, udp_port
 * 参数：
 *  [in ] strUserID     - 用户ID
 *  [in ] strRoomID     - 房间ID
 *  [in ] strServerIP   - 聊天服务器IP
 *  [in ] iServerPort   - 聊天服务器端口
 *  [out] iRTPUdpPort   - 聊天服务器会话UDP端口
 *  [out] iSessionID    - 聊天服务器会话ID
 *  [out] uBusinessID   - 聊天服务器房间业务ID
 * 返回值：
 *  -1 失败, 0 成功<服务器响应成功，未必真的取到sessionid，也可能是错误码>
 */
YouMeErrorCode NgnLoginService::LoginToMcuServer(const string & strUserID,
                                        const string & strRoomID,
                                        YouMeUserRole_t eUserRole, 
                                        const string & strServerIP,
                                        const int & iServerPort,
                                        int & iRTPUdpPort,
                                        int & iSessionID,
                                        uint64_t & uBusinessID,
                                        bool bVideoAutoReceive)
{
    YouMeErrorCode code = YOUME_SUCCESS;
    int iStatus = -1000;
    uint64_t ulStart = 0;
    int sendRet = 0;
    int recvNum = 0;
 
    ReportQuitData::getInstance()->m_login_count++;
    ulStart = tsk_time_now();
   
	TSK_DEBUG_INFO("==================== Start login to MCU server ==========================");

    // 登陆到服务器，使用loginserver 协议
    YouMeProtocol::YouMeVoice_Command_JoinConference joinProtocol;
    joinProtocol.set_allocated_head (CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_JoinConference));
    joinProtocol.set_roomid(strRoomID);
    // 带上这些额外的参数
    CNgnMemoryConfiguration * pConfig = CNgnMemoryConfiguration::getInstance();
    joinProtocol.set_enabledtx(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_DTX_PEROID, NgnConfigurationEntry::DEFAULT_OPUS_DTX_PEROID) > 0);
    joinProtocol.set_enablevbr(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_VBR, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_VBR));
    joinProtocol.set_encodecomplex(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_COMPLEXITY, NgnConfigurationEntry::DEFAULT_OPUS_COMPLEXITY));
    joinProtocol.set_interbandfec(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_INBANDFEC, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_INBANDFEC));
    joinProtocol.set_outbandfec(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_ENABLE_OUTBANDFEC, NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_OUTBANDFEC));
    joinProtocol.set_maxbandlimited(pConfig->GetConfiguration(NgnConfigurationEntry::OPUS_MAX_BANDWIDTH, NgnConfigurationEntry::DEFAULT_OPUS_MAX_BANDWIDTH));
    joinProtocol.set_userid(strUserID);
    joinProtocol.set_needuserlist( false );     //这个字段已经无用了
    joinProtocol.set_av_notify( true ); // 是否接收房间内其它用户已经开启音视频的通知（视频版本）
    joinProtocol.set_auto_recive( bVideoAutoReceive );      // 进入房间是否接收其它用户视频流

    YouMeProtocol::YouMeUserRole role = YouMeProtocol::YOUME_USER_NONE;
    switch (eUserRole) {
        case YOUME_USER_NONE:
            role = YouMeProtocol::YOUME_USER_NONE;
            break;
        case YOUME_USER_TALKER_FREE:
            role = YouMeProtocol::YOUME_USER_TALKER_FREE;
            break;
        case YOUME_USER_TALKER_ON_DEMAND:
            role = YouMeProtocol::YOUME_USER_TALKER_ON_DEMAND;
            break;
        case YOUME_USER_LISTENER:
            role = YouMeProtocol::YOUME_USER_LISTENER;
            break;
        case YOUME_USER_COMMANDER:
            role = YouMeProtocol::YOUME_USER_COMMANDER;
            break;
        case YOUME_USER_HOST:
            role = YouMeProtocol::YOUME_USER_HOST;
            break;
        case YOUME_USER_GUSET:
            role = YouMeProtocol::YOUME_USER_GUSET;
            break;
        default:
            role = YouMeProtocol::YOUME_USER_NONE;
            break;
    }
    joinProtocol.set_user_role(role);

    //token非空才表示有设置，需要验证{
    {
        std::lock_guard<std::mutex> lock(this->m_tokenMutex);
        if( !m_strToken.empty() ){
            joinProtocol.set_token( m_strToken );
            if (m_timeStamp != 0) {
                joinProtocol.set_timestamp(m_timeStamp);
            }
        }
    }

    std::string strSerialData;
    joinProtocol.SerializeToString(&strSerialData);
    
    // 发送协议数据
    int iRetry = 0;
    
    for (iRetry = 0; iRetry < RTY_TIMES ; iRetry++)
    {
        if (m_bAborted) {
            TSK_DEBUG_INFO ("==LoginMcu aborted");
            code = YOUME_ERROR_USER_ABORT;
            goto login_mcu_end;
        }
        
        m_syncTcp.UnInit();
        if (!m_syncTcp.Init(strServerIP, iServerPort, 25)) {
            TSK_DEBUG_ERROR ("#### Login to MCU server init socket fail %d times: %s:%d", iRetry + 1, strServerIP.c_str(), iServerPort);
            m_loginCondWait.WaitTime(3 * 1000);
            continue;
        }
        
        TSK_DEBUG_INFO ("#### Start login to MCU server %s:%d", strServerIP.c_str(), iServerPort);
        
        if (!m_syncTcp.Connect(25)) {
            if (m_bAborted) {
                TSK_DEBUG_INFO ("==LoginMcu aborted");
                code = YOUME_ERROR_USER_ABORT;
                goto login_mcu_end;
            } else {
            TSK_DEBUG_ERROR ("#### MCU server login fail %d times: %s:%d", iRetry + 1, strServerIP.c_str(), iServerPort);
                code = YOUME_ERROR_NETWORK_ERROR;
                m_loginCondWait.WaitTime(2 * 1000);
            continue;
        }
        }
        
        iRTPUdpPort = 0;
        iSessionID = 0;
        
        sendRet = m_syncTcp.SendData(strSerialData.c_str(), strSerialData.length());
        
        if (sendRet <= 0) {
            TSK_DEBUG_WARN ("#### MCU server send data error %d times: %s:%d", iRetry + 1, strServerIP.c_str(), iServerPort);
            m_loginCondWait.WaitTime(2 * 1000);
            continue;
        }
        
        youmecommon::CXSharedArray<char> recvBuffer;
        recvNum = m_syncTcp.RecvData (recvBuffer);
        
        if (recvNum <= 0) {
            TSK_DEBUG_WARN ("#### MCU server recv data error %d times: %s:%d", iRetry + 1, strServerIP.c_str(), iServerPort);
            m_loginCondWait.WaitTime(2 * 1000);
            continue;
        } else {
            YouMeProtocol::YouMeVoice_Command_JoinConferenceResponse responseProtocol;
            if (!responseProtocol.ParseFromArray (recvBuffer.Get(), recvNum)) {
                TSK_DEBUG_ERROR ("#### MCU server protocol parse failed, retry! ");
                continue;
            }
            
            iStatus = responseProtocol.head().code();
            
            if (0 == iStatus) {
                iRTPUdpPort = responseProtocol.udpport();
                iSessionID = responseProtocol.sessionid();
                
                int iMediaCtlPort = responseProtocol.media_ctl_port();
                Config_SetInt( CONFIG_MediaCtlPort, iMediaCtlPort);
                
                int speakerNum = responseProtocol.speaker_list_size();
                if ((speakerNum > 0) && (speakerNum < 100) && m_pCallback) {
                    SessionUserIdPairVector_t idPairVector;
                    idPairVector.reserve(speakerNum);
                    for (int i = 0; i < speakerNum; i++) {
                        SessionUserIdPair_t idPair;
                        idPair.sessionId = responseProtocol.speaker_list(i).sessionid();
                        idPair.userId = responseProtocol.speaker_list(i).userid();
                        idPairVector.push_back(idPair);
                    }
                    m_pCallback->OnReceiveSessionUserIdPair(idPairVector);
                }

                uBusinessID = responseProtocol.business_id();
                
                TSK_DEBUG_INFO ("#### Login to MCU server OK. RTP UDP port:%d, ctrl UDP port:%d sessionID:%d, uBusinessID=%llu", iRTPUdpPort, iMediaCtlPort, iSessionID, uBusinessID);
            }
            else{
                if( iStatus == 1001 ){
                    code = YOUME_ERROR_TOKEN_ERROR;
                }
                else if( iStatus == 1007 )
                {
                    code = YOUME_ERROR_BE_KICK;
                }
                else{
                    code = YOUME_ERROR_NETWORK_ERROR;
                }
                
                TSK_DEBUG_INFO ("#### Login to MCU server Failed. token error");
            }

            break;
        }
    }
    
    if (iRetry == RTY_TIMES) { // 超过最大重试次数
        TSK_DEBUG_ERROR ("#### MCU server retry over max times, quit!");
        code = YOUME_ERROR_NETWORK_ERROR;
    }
 
login_mcu_end:
    {
        /** 新的登录数据上报 */
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportLogin login;
        
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        
        login.roomid = strShortRoomID;
        login.mcu_server = strServerIP;
        login.mcu_port = iServerPort;
        login.retry_times = iRetry;
        login.login_usetime = tsk_time_now() - ulStart;
        login.login_result = -(int)code;
        login.login_status = iStatus;
        login.mcu_sessionid = iSessionID;
        login.mcu_udpport = iRTPUdpPort;
		login.sdk_version = SDK_NUMBER;
        login.server_region = g_serverRegionId;
        login.platform = NgnApplication::getInstance()->getPlatform();
        login.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report(login);
    }
    
    return code;
}

/***
* 加入频道
*/
YouMeErrorCode NgnLoginService::JoinChannel(int sessionID, const std::string strRoomID, NgnLoginServiceCallback * pCallback)
{
	// 加入多房间 ，使用joinroom 协议
	YouMeProtocol::YouMeVoice_Command_JoinRoomRequest joinRoom;
	joinRoom.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_JOIN_ROOM));
	joinRoom.set_sessionid(sessionID);
	joinRoom.set_roomid(strRoomID);
    joinRoom.set_av_notify(true); // 是否接收房间内其它用户已经开启音视频的通知（视频版本）
	std::string strSerialData;
	joinRoom.SerializeToString(&strSerialData);

	// 发送协议数据
	AddTCPQueue(YouMeProtocol::MSG_JOIN_ROOM, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

/**
* 切换频道
*/
YouMeErrorCode NgnLoginService::SpeakToChannel(int sessionID, const std::string strRoomID, uint32_t timestamp)
{
	// 加入多房间 ，使用joinroom 协议
	YouMeProtocol::YouMeVoice_Command_SpeakToRoomRequest speak;
	speak.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_SPEAK_TO_ROOM));
	speak.set_sessionid(sessionID);
	speak.set_roomid(strRoomID);
	speak.set_timestamp(timestamp);
	std::string strSerialData;
	speak.SerializeToString(&strSerialData);

	// 发送协议数据
	AddTCPQueue(YouMeProtocol::MSG_SPEAK_TO_ROOM, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}


/**
* 离开频道
*/
YouMeErrorCode NgnLoginService::LeaveChannel(int sessionID, const std::string strRoomID)
{
	// 加入多房间 ，使用joinroom 协议
	YouMeProtocol::YouMeVoice_Command_LeaveRoomRequest leave;
	leave.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_LEAVE_ROOM));
	leave.set_sessionid(sessionID);
	leave.set_roomid(strRoomID);
	std::string strSerialData;
	leave.SerializeToString(&strSerialData);

	// 发送协议数据
	AddTCPQueue(YouMeProtocol::MSG_LEAVE_ROOM, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

//
// 屏蔽视频
//
YouMeErrorCode NgnLoginService::maskVideoByUserIdRequest(const std::string userId, int sessionId, int mask)
{
    YouMeProtocol::YouMeVoice_Command_MaskVideoByUserIdRequest request;
    request.set_user_id(userId);
    request.set_sessionid(sessionId);
    request.set_status(mask);
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_MASK_VIDEO_BY_USERID));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_MASK_VIDEO_BY_USERID, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

//
// 摄像头状态转换
//
//YouMeErrorCode NgnLoginService::cameraStatusChgReport(int sessionId, int cameraStatus)
//{
//    YouMeProtocol::YouMeVoice_Command_CameraStatusChangeRequest request;
//    request.set_sessionid(sessionId);
//    request.set_status(cameraStatus);
//    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_CAMERA_STATUS));
//    std::string strSerialData;
//    request.SerializeToString(&strSerialData);
//
//    // 发送协议数据
//    AddTCPQueue(YouMeProtocol::MSG_CAMERA_STATUS, strSerialData.c_str(), strSerialData.length());
//    return YOUME_SUCCESS;
//}

//
// 共享视频输入状态变化
//
YouMeErrorCode NgnLoginService::shareInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus)
{
    YouMeProtocol::YouMeVoice_Command_AVInput_Status_Req request;
    request.set_sessionid(sessionId);
    request.set_userid(strUserID);
    request.set_media_type(3);//share input
    request.set_op(inputStatus);
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_MEDIA_AVINPUT_STAT));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_MEDIA_AVINPUT_STAT, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

//
// 视频输入状态变化
//
YouMeErrorCode NgnLoginService::videoInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus)
{
    YouMeProtocol::YouMeVoice_Command_AVInput_Status_Req request;
    request.set_sessionid(sessionId);
    request.set_userid(strUserID);
    request.set_media_type(2);//video
    request.set_op(inputStatus);
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_MEDIA_AVINPUT_STAT));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_MEDIA_AVINPUT_STAT, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

//
// 音频输入状态变化
//
YouMeErrorCode NgnLoginService::audioInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus)
{
    YouMeProtocol::YouMeVoice_Command_AVInput_Status_Req request;
    request.set_sessionid(sessionId);
    request.set_userid(strUserID);
    request.set_media_type(1);//audio
    request.set_op(inputStatus);
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_MEDIA_AVINPUT_STAT));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_MEDIA_AVINPUT_STAT, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

//
// 查询用户传输视频信息
//
YouMeErrorCode NgnLoginService::queryUserVideoInfoReport(int mySessionId, std::vector<std::string>& toSeesionId)
{
    YouMeProtocol::YouMeVoice_Command_Query_User_Video_Info_Req request;
    request.set_sessionid(mySessionId);
    for (int i = 0; i < toSeesionId.size(); ++i) {
        std::string userid = toSeesionId[i];
        request.add_other_userid_list(userid);
       // request.add_other_userid_list(toSeesionId[i]);
    }
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_QUERY_USER_VIDEO_INFO));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_QUERY_USER_VIDEO_INFO, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}


//
// 设置用户接收视频分辨率
//
YouMeErrorCode NgnLoginService::setUserRecvResolutionReport(int mySessionId, std::vector<IYouMeVoiceEngine::userVideoInfo>& videoInfo)
{
    YouMeProtocol::YouMeVoice_Command_Set_User_Video_Info_Req request;
    request.set_sessionid(mySessionId);

    for (int i = 0; i < videoInfo.size(); ++i) {
        YouMeProtocol::YouMeVoice_UserVedioInfo* userInfo =  request.add_user_video_info_list();
        std::string userid = videoInfo[i].userId;
        userInfo->set_other_userid(userid);
        userInfo->set_other_sessionid(0);
        YouMeProtocol::YouMeVoice_Video_info* videoList = userInfo->add_video_info_list();
        videoList->set_resolution_type(videoInfo[i].resolutionType);
    }

    
    request.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_SET_RECV_VIDEO_INFO));
    std::string strSerialData;
    request.SerializeToString(&strSerialData);
    
    // 发送协议数据
    AddTCPQueue(YouMeProtocol::MSG_SET_RECV_VIDEO_INFO, strSerialData.c_str(), strSerialData.length());
    return YOUME_SUCCESS;
}

/***
 * 停止登录
 */
void NgnLoginService::StopLogin()
{
    Abort();
    InterUninit();
    Reset();
}


void NgnLoginService::Reset()
{
    this->m_bAborted = false;
    this->m_loginCondWait.Reset();
    this->m_heartCondWait.Reset(); // 设置退出心跳线程
    this->m_syncTcp.Reset();
    this->m_RedirectTcp.Reset();
    
}

void NgnLoginService::Abort()
{
    this->m_bAborted = true;
    this->m_loginCondWait.SetSignal();
    this->m_heartCondWait.SetSignal(); // 设置退出心跳线程
    this->m_bRecvThreadExit = true; // 设置退出接收线程
    this->m_bSendThreadExit = true; // 设置退出发送线程, 必须先设退出标识，再激活semaphore, 保证一定不会卡死在semaphore上
    this->m_sendListSema.Increment();
    this->m_syncTcp.Abort();
    this->m_RedirectTcp.Abort();
}


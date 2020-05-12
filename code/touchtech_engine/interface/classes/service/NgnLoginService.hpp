//
//  NgnLoginService.hpp
//  youme_voice_engine
//
//  Created by wanglei on 15/11/13.
//  Copyright © 2015年 youme. All rights reserved.
//

#ifndef NgnLoginService_hpp
#define NgnLoginService_hpp
#include "tsk_thread.h"
#include "YouMeCommon/XCondWait.h"
#include <mutex>
#include <string>
#include "YouMeConstDefine.h"
#include "YouMeInternalDef.h"
#include "YouMeCommon/XCondWait.h"
#include "SyncTCPTalk.h"
#include <map>
#include "YouMeCommon/XSemaphore.h"
#include "ProtocolBufferHelp.h"
#include <list>
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#endif

#include "IYouMeEventCallback.h"
#include "IYouMeVoiceEngine.h"
using namespace std;

struct YOUMETALKTCPPacketInfo
{
    //包数据
    youmecommon::CXSharedArray<char> pPacket;
    YouMeProtocol::MSG_TYPE commandType;
};

typedef struct SessionUserIdPair_s
{
    int32_t sessionId;
    std::string userId;
} SessionUserIdPair_t;

typedef std::vector<SessionUserIdPair_t> SessionUserIdPairVector_t;

struct MemberChangeInner{
    std::string userID;
    int32_t     sessionId;
    bool isJoin;
};

struct SetUserVideoInfoNotify {
    std::string usrId;
    int32_t     sessionId;
    int         videoType;  // 0:main stream; 1:minor stream
};

class NgnLoginServiceCallback
{
public:
    enum RoomEventType {
        ROOM_EVENT_JOIN,
        ROOM_EVENT_LEAVE,
        ROOM_EVENT_SPEAK_TO
    };
    
    enum RoomEventResult {
        ROOM_EVENT_SUCCESS,
        ROOM_EVENT_FAILED
    };

	enum GrabMicState {
		GRAB_MIC_NONE = 0,
		GRAB_MIC_START = 1,
		GRAB_MIC_STOP = 2,
		GRAB_MIC_GET = 3,
		GRAB_MIC_FREE = 4,
	};

    virtual void OnDisconnect() = 0;
    virtual void OnKickFromChannel( const std::string& strRoomID, const std::string& strParam ) = 0;
    virtual void OnCommonStatusEvent(STATUS_EVENT_TYPE_t eventType, const std::string&strUserID,int iStauts) =0;

    virtual void OnRoomEvent(const std::string& strRoomIDFull, RoomEventType eventType, RoomEventResult result) = 0;
//    virtual void OnReciveMicphoneControllMsg(const std::string&strUserID,int iStauts)=0;
//    //接收成员扬声器状态控制指令
//    virtual void OnReciveSpeakerControllMsg(const std::string&strUserID,int iStauts)=0;
//    //接收成员列表变化通知
//    virtual void OnReciveRoomMembersChangeNotify(const std::string &strUserIDs)=0;
//    //接收语音消息被屏蔽一路的通知
//    virtual void OnReciveAvoidControllMsg(const std::string&strUserID,int iStauts)=0;
    virtual void OnReceiveSessionUserIdPair(const SessionUserIdPairVector_t &idPairVector) = 0;
 	virtual void OnMemberChange( const std::string& strRoomID, std::list<MemberChangeInner>& listMemberChange, bool bUpdate ) = 0 ;
    virtual void OnUserVideoInfoNotify(const struct SetUserVideoInfoNotify &info) = 0;
    // 接收到被别人屏蔽的通知
    virtual void OnMaskVideoByUserIdNfy(const std::string &selfUserId, const std::string &framUserId, int mask) = 0;
    // 接收到别人的摄像头状态改变通知
    //virtual void OnWhoseCamStatusChgNfy(const std::string &camChgUserId, int cameraStatus) = 0;
    // 接收到别人的视频输入状态改变通知
    virtual void OnOtherVideoInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus, bool bLeave ,int videoId ) = 0;
    // 接收到别人的音频输入状态改变通知
    virtual void OnOtherAudioInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus) = 0;
    // 接收到别人的共享视频输入状态改变通知
    virtual void OnOtheShareInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus) = 0;    
	// 抢麦通知事件
	virtual void OnGrabMicNotify(int mode, int ntype, int getMicFlag, int autoOpenMicFlag, int hasMicFlag, int talkTime, const std::string& strRoomID, const std::string strUserID, const std::string& strContent) = 0;

	// 连麦通知事件
	virtual void OnInviteMicNotify(int mode, int ntype, int errCode, int talkTime, const std::string& strRoomID, const std::string& fromUserID, const std::string& toUserID, const std::string& strContent) = 0;

	// 消息通用回复事件
    virtual void OnCommonEvent( int nMsgId, int wparam, int lparam, int nErrCode, const std::string strRoomId, int nSessionId , std::string strParam = "" ) = 0;
};

class NgnLoginService
{
public:
    NgnLoginService ();
    ~NgnLoginService ();

public:
   
    YouMeErrorCode LoginServerSync (const string& strUserID, YouMeUserRole_t eUserRole, const string &directServerAddr, int iPort,const string &strRoomID,
                                    NgnLoginServiceCallback *pCallback,std::string& strServerIP,
                                    int &iRTPUdpPort, int& iSessionID, int& iServerPort, uint64_t &uBusinessID, bool bVideoAutoReceive);
    
    //同步登陆，不开线程
    YouMeErrorCode ReLoginServerSync(const string& strUserID, YouMeUserRole_t eUserRole, const string &directServerAddr, int iPort,const string &strRoomID,
                                     NgnLoginServiceCallback *pCallback,std::string& strServer,
                                     int &iRtpPort, int& iSessionID, int& iUDPPort, uint64_t &uBusinessID, bool bVideoAutoReceive);
    
    //设置安全验证的token和用户加入房间的时间
    void SetToken( const string& strToken, uint32_t timeStamp);
    
	// 加入频道
	YouMeErrorCode JoinChannel(int sessionID, const std::string strRoomID, NgnLoginServiceCallback * pCallback);
	
	// 切换频道
	YouMeErrorCode SpeakToChannel(int sessionID, const std::string strRoomID, uint32_t timestamp);

	// 离开频道
	YouMeErrorCode LeaveChannel(int sessionID, const std::string strRoomID);

	// 离开会议
	YouMeErrorCode LeaveConference(int sessionID, const std::string strRoomID);
    
    // 屏蔽指定session ID的视频
    YouMeErrorCode maskVideoByUserIdRequest(const std::string userId, int sessionId, int mask);
    
    // 摄像头开关状态改变
    //YouMeErrorCode cameraStatusChgReport(int sessionId, int cameraStatus);
    
    //共享视频输入状态变化
    YouMeErrorCode shareInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus);
    
    // 视频输入状态变化
    YouMeErrorCode videoInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus);
    
    // 音频输入状态变化
    YouMeErrorCode audioInputStatusChgReport(const string & strUserID, int sessionId, int inputStatus);
    
    //查询用户传输视频信息
    YouMeErrorCode queryUserVideoInfoReport(int mySessionId, std::vector<std::string> &toSeesionId);
    
    
    // 设置用户接收视频分辨率
    YouMeErrorCode setUserRecvResolutionReport(int mySessionId, std::vector<IYouMeVoiceEngine::userVideoInfo> &videoInfo);

    // 设置退出信号，关闭socket等，并等待操作真正结束。
    void StopLogin();
    // CSyncTCP* GetTcpClient();
    
    // 设置退出信号，关闭socket等，但不等待操作真正结束。
    void Reset();
    void Abort();
    
    void AddTCPQueue(YouMeProtocol::MSG_TYPE commandType,const char* buffer, int iBufferSize);
    
    
private:
    static TSK_STDCALL void *RecvTCPThread (void *arg);
    static TSK_STDCALL void *SendHeartThread (void *arg);
    static TSK_STDCALL void *SendTCPThread (void *arg);
private:
    void DealRead();
    bool InterInitHeartSocket();
    void InterUninit();
    
    // 重定向到MCU服务器（即语音服务器）
    YouMeErrorCode RedirectToMcuServer(const string & strRedirectServer,
                          const int iRedirectServerPort,
                          const string & strRoomID,
                          string & strServerIP,
                          int & iServerPort);
    YouMeErrorCode RedirectToMcuServerUdp(const string & strRedirectServer,
                          const int iRedirectServerPort,
                          const string & strProbufData,
                          string & strServerIP,
                          int & iServerPort);
    
    YouMeErrorCode RedirectToMcuServerTcp(const string & strRedirectServer,
                            const int iRedirectServerPort,
                            const string & strProbufData,
                            string & strServerIP,
                            int & iServerPort);
    
    // 登录到MCU服务器（即语音服务器）
    YouMeErrorCode LoginToMcuServer(const string & strUserID,
                           const string & strRoomID,
                           YouMeUserRole_t eUserRole,
                           const string & strServerIP,
                           const int & iServerPort,
                           int & iRTPUdpPort,
                           int & iSessionID,
                           uint64_t & uBusinessID,
                           bool LoginToMcuServer);
    
    NgnLoginServiceCallback *m_pCallback = NULL;
    tsk_thread_handle_t *mRecvThreadHandle = NULL;
    bool m_bRecvThreadExit = false;
    tsk_thread_handle_t *mSendHeartThreadHandle = NULL;
    tsk_thread_handle_t *mSendDataThreadHandle = NULL;
    youmecommon::CXCondWait m_loginCondWait;
    
    //一个用来发送/接收的TCP socket
    youmertc::CSyncTCP m_syncTcp;
    youmertc::CSyncTCP m_RedirectTcp;
    youmecommon::CXCondWait m_heartCondWait;
    //心跳包连续丢的个数
    int m_iLostHeartPacket =0;
    //保存服务器的一些信息
    int m_iSessionID = 0;
    std::string m_strUserID;
    std::string m_strRoomID;
    
    std::mutex  m_tokenMutex;
    std::string m_strToken;
    uint32_t m_timeStamp = 0;
    
    std::string m_strServerIP;
    int m_iServerPort;
    
    // TCP message queue and handler thread
    youmecommon::CXSemaphore m_sendListSema;
    bool m_bSendThreadExit = false;
    std::mutex m_sendListMutex;
    std::list < YOUMETALKTCPPacketInfo > m_sendPacketList;
    
    int m_iReadErrCount = 0;
    bool m_bSocketError = false;
    bool m_bAborted = false;
};


#endif /* NgnLoginService_hpp */

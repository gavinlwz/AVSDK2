//  YouMeVoiceEngine.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/9/6.
//  Copyright (c) 2015年 youme. All rights reserved.
//


#include "version.h"
#include "NgnConfigurationEntry.h"
#include "YouMeCommon/utf8/checked.h"   //这个文件必须在NgnEngine.h,common库的某个文件有点问题
#include "NgnEngine.h"
#include "ProtocolBufferHelp.h"
#include "SDKValidateTalk.h"
#include "SyncUDP.h"
#include "XTimer.h"
#include "stdlib.h"
#include "tsk_debug.h"
#include "tsk_thread.h"
#include "MonitoringCenter.h"
#include "tinydav/audio/tdav_youme_neteq_jitterbuffer.h"
#include "tinydav/video/tdav_session_video.h"
#include "tinyrtp/trtp_manager.h"
#include "tmedia_utils.h"
#include "tinymedia/tmedia_defaults.h"
#include "XOsWrapper.h"



#include "YouMeCommon/json/json.h"
#include "YouMeCommon/CryptUtil.h"
#include "YouMeCommon/DownloadUploadManager.h"
#include "YouMeCommon/XFile.h"
#include "NgnTalkManager.hpp"


#include "YouMeCommon/TranslateUtil.h"
#include "YouMeCommon/StringUtil.hpp"
//#include "YouMeCommon/utf8.h"
#if FFMPEG_SUPPORT
#include "YMVideoRecorderManager.hpp"
#endif
#include "XConfigCWrapper.hpp"
#include "CustomInputManager.h"
#include "AVStatistic.h"

#include "YMTranscriberManager.hpp"

#if TARGET_OS_OSX
#include "AudioDeviceMgr_OSX.h"
#endif

#include <algorithm>



#if FFMPEG_SUPPORT
#include "IFFMpegDecoder.h"
#if defined (__cplusplus)
extern "C" {
#include "libx264/x264.h"
#if ANDROID
extern void JNI_Resume_Audio_Record ();
extern void JNI_Pause_Audio_Record ();
extern int  JNI_Is_Wired_HeadsetOn ();
#endif
};
#endif
#endif //FFMPEG_SUPPORT

#include <mutex>

#ifdef WIN32
#include <windows.h> /* PathRemoveFileSpec */
#include <wtypes.h>
#include "MMNotificationClient.h"
#include "../win/CameraManager.h"
#include "libscreen/libscreen.h"
#include "../win/AudioDeviceMgr_Win.h"
#elif ANDROID
#include "AudioMgr.h"
extern void UpdateAndroid(const std::string& strURL,const std::string& strMD5);
extern void TriggerNetChange();
#else
#include "tdav_apple.h"
#endif

#if defined(ANDROID) || defined(__APPLE__)
#include <signal.h>
#endif

#include "NgnVideoManager.hpp"
#include "ReportService.h"
#include "ICameraManager.hpp"

#include "YouMeVoiceEngine.h"


#if defined(__APPLE__)
#include "AVRuntimeMonitor.h"
#endif


//#ifdef YOUME_FOR_QINIU
#include "YouMeEngineManagerForQiniu.hpp"
//#endif
#include "YouMeVideoMixerAdapter.h"

#ifdef TARGET_OS_OSX
#include "av_capture.h"
#endif

#ifdef OS_LINUX
#include <sys/prctl.h>
#endif

#ifdef WIN32
#include "tinyDAV/tdav_win32.h"
#endif

extern int g_serverMode;
extern std::string g_serverIp;
extern int g_serverPort;
YOUME_RTC_SERVER_REGION g_serverRegionId = RTC_CN_SERVER;
std::string g_extServerRegionName = "";

unsigned int CYouMeVoiceEngine::m_iTranslateRequestID = 0;


int CYouMeVoiceEngine::s_iSerial = 0;
std::mutex CYouMeVoiceEngine::m_serialMutex;

std::mutex CYouMeVoiceEngine::m_mutexOtherResolution;
std::map<int, CYouMeVoiceEngine::InnerSize> CYouMeVoiceEngine::m_mapOtherResolution;
CTranslateUtil* g_ymvideo_pTranslateUtil = NULL;



/**
 * 新的重试机制
 * 理论上，应用不主动退出就一直重连，重连机制如下：
 *
 * 设置500次，理论重试时间约 500 * 35秒  18000秒 = 300分钟
 * 如果近5个小时都连接不上，也没必要无限重连了。
 *
 *****************************************
 * times |  base |  rand | time (msec)
 * ------|-------|-------|----------------
 *   0   |   0   |   1   |     0 -  1000
 *   1   |   1   |   1   |  1000 -  2000
 *   2   |   2   |   1   |  2000 -  3000
 *   3   |   3   |   2   |  3000 -  5000
 *   4   |   5   |   2   |  5000 -  7000
 *   5   |   7   |   3   |  7000 - 10000
 *   6   |   10  |   5   | 10000 - 15000
 *   7   |   15  |   5   | 15000 - 20000
 *   8   |   20  |   5   | 20000 - 25000
 *   9   |   25  |   5   | 25000 - 30000
 *  10   |   30  |   5   | 30000 - 35000
 *11-MAX |   30  |   5   | 30000 - 35000
 *****************************************
 *
 *
 */
#define RECONNECT_RETRY_MAX       (500)

#define DEFAULT_MODE 3
#define DEFAULT_WINDOW_ID 0

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  \
do{						\
if (p)				\
{					\
delete (p);		\
(p) = NULL;		\
}					\
} while (0)

#endif

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
class CRoomManager
{
public:
    enum ROOM_STATE_e {
        ROOM_STATE_INVALID = 0,
        ROOM_STATE_DISCONNECTED,
        ROOM_STATE_CONNECTING,
        ROOM_STATE_RECONNECTING,
        ROOM_STATE_CONNECTED,
        ROOM_STATE_DISCONNECTING
    };
    
    typedef struct GrabMicInfo_s
    {
        int maxAllowCount;
        int maxTalkTime;
        int voteTime;
        int mode;
    }GrabMicInfo_t;
    
    typedef struct InviteMicInfo_s
    {
        int waitTimeout;
        int maxTalkTime;
        bool bStateBroadcast;
    }InviteMicInfo_t;
    
    typedef struct RoomInfo_s
    {
        std::string idFull;
        ROOM_STATE_e state;
        uint64_t joinRoomTime;
        GrabMicInfo_t grabmicInfo;
        InviteMicInfo_t invitemicInfo;
    } RoomInfo_t;
    
    typedef std::map<std::string, RoomInfo_t> RoomMap_t;
    
    CRoomManager()
    {
        _cleanStates();
    }
    ~CRoomManager()
    {
        
    }
    
    static const char* stateToString(ROOM_STATE_e state)
    {
        switch (state) {
            case ROOM_STATE_INVALID:
                return "Invalid";
            case ROOM_STATE_DISCONNECTED:
                return "Disconnected";
            case ROOM_STATE_CONNECTING:
                return "Connecting";
            case ROOM_STATE_RECONNECTING:
                return "Reconnecting";
            case ROOM_STATE_CONNECTED:
                return "Connected";
            case ROOM_STATE_DISCONNECTING:
                return "Disconnecting";
            default:
                return "Unknown";
        }
    }
    
    bool addRoom(const std::string& strRoomId, RoomInfo_t& roomInfo)
    {
        lock_guard<std::mutex> lock(m_mtx);
        std::pair<RoomMap_t::iterator,bool> retPair = m_roomMap.insert(RoomMap_t::value_type(strRoomId, roomInfo));
        if (false == retPair.second) {
            TSK_DEBUG_ERROR("Failed to insert to map for room#%s", strRoomId.c_str());
            return false;
        }
        return true;
    }
    
    bool removeRoom(const std::string& strRoomId)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        if (it != m_roomMap.end()) {
            m_roomMap.erase(it);
            if (strRoomId.compare(m_speakToRoomId) == 0) {
                // If there's only one room left, the server will mark it as "speakTo", otherwise the "speakToRoom"
                // will be undefined.
                if (m_roomMap.size() == 1) {
                    m_speakToRoomId = m_roomMap.begin()->first;
                    TSK_DEBUG_INFO("speakToRoomId:%s was automatically switched to:%s", strRoomId.c_str(), m_speakToRoomId.c_str());
                } else {
                    _cleanStates();
                    TSK_DEBUG_INFO("speakToRoomId:%s was removed, now becomes null", strRoomId.c_str());
                }
            }
            return true;
        } else {
            TSK_DEBUG_ERROR("Cannot find room#%s", strRoomId.c_str());
            return false;
        }
    }
    
    void removeAllRooms()
    {
        lock_guard<std::mutex> lock(m_mtx);
        m_roomMap.clear();
        _cleanStates();
    }
    
    bool setRoomState(const std::string& strRoomId, ROOM_STATE_e state)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        if (it != m_roomMap.end()) {
            it->second.state = state;
            return true;
        } else {
            TSK_DEBUG_ERROR("Cannot find room#%s", strRoomId.c_str());
            return false;
        }
    }
    
    bool setCurrentRoomState(ROOM_STATE_e state)
    {
        lock_guard<std::mutex> lock(m_mtx);
        if (m_iterator != m_roomMap.end()) {
            m_iterator->second.state = state;
            return true;
        } else {
            return false;
        }
    }
    
    ROOM_STATE_e getRoomState(const std::string& strRoomId)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        if (it != m_roomMap.end()) {
            return it->second.state;
        } else {
            TSK_DEBUG_INFO("Cannot find room#%s", strRoomId.c_str());
            return ROOM_STATE_INVALID;
        }
    }
    
    ROOM_STATE_e getCurrentRoomState(){
        lock_guard<std::mutex> lock(m_mtx);
        if (m_iterator != m_roomMap.end()) {
            return m_iterator->second.state;
        }
        else {
            return ROOM_STATE_INVALID;
        }
    }
    
    bool getRoomInfo(const std::string& strRoomId, RoomInfo_t &roomInfo)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        if (it != m_roomMap.end()) {
            roomInfo = it->second;
            return true;
        } else {
            TSK_DEBUG_INFO("Cannot find room#%s", strRoomId.c_str());
            return false;
        }
    }
    
    RoomInfo_t* findRoomInfo(const std::string& strRoomId)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        if (it != m_roomMap.end()) {
            return &(it->second);
        }
        else {
            TSK_DEBUG_INFO("Cannot find room#%s", strRoomId.c_str());
            return nullptr;
        }
    }
    
    bool getFirstRoomInfo(RoomInfo_t &roomInfo)
    {
        lock_guard<std::mutex> lock(m_mtx);
        m_iterator = m_roomMap.begin();
        if (m_iterator != m_roomMap.end()) {
            roomInfo = m_iterator->second;
            return true;
        } else {
            return false;
        }
    }
    
    bool getNextRoomInfo(RoomInfo_t &roomInfo)
    {
        lock_guard<std::mutex> lock(m_mtx);
        if (m_iterator == m_roomMap.end()) {
            return false;
        }
        m_iterator++;
        if (m_iterator == m_roomMap.end()) {
            return false;
        }
        
        roomInfo = m_iterator->second;
        return true;
    }
    
    
    int32_t getRoomCount()
    {
        lock_guard<std::mutex> lock(m_mtx);
        return (int32_t)m_roomMap.size();
    }
    
    void setSpeakToRoom(const std::string& strRoomId)
    {
        m_speakToRoomId = strRoomId;
    }
    
    std::string getSpeakToRoomId()
    {
        return m_speakToRoomId;
    }
    
    bool isInRoom(const std::string& strRoomId)
    {
        lock_guard<std::mutex> lock(m_mtx);
        RoomMap_t::iterator it = m_roomMap.find(strRoomId);
        return it != m_roomMap.end();
    }
    
private:
    void _cleanStates()
    {
        m_speakToRoomId = "";
        m_iterator = m_roomMap.end();
    }
private:
    RoomMap_t m_roomMap;
    std::string m_speakToRoomId;
    RoomMap_t::iterator m_iterator;
    std::mutex m_mtx;
};

struct QueryHttpInfo{
    int requestID;
    std::string strCommand;
    std::string strQueryBody;
};


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
class CMessageBlock
{
public:
	enum MsgType
	{
		// API message types
		MsgApiInit,
		MsgApiSetServerRegion,
		MsgApiJoinConfSingle,
		MsgApiJoinConfMulti,
		MsgApiSpeakToConference,
		MsgApiLeaveConfMulti,
		MsgApiLeaveConfAll,
		MsgApiRoomEvent,
		MsgApiReconnect,
		MsgApiSetMicMute,
		MsgApiSetSpeakerMute,
		MsgApiSetVolume,
		MsgApiCheckRecordingError,
		MsgApiSetOthersMicOn,
		MsgApiSetOthersSpeakerOn,
		MsgApiSetOthersMuteToMe,
		MsgApiSetAecEnabled,
		MsgApiSetAnsEnabled,
		MsgApiSetAgcEnabled,
		MsgApiSetVadEnabled,
		MsgApiSetSoundTouchEnabled,
		MsgApiSetSoundTouchTempo,
		MsgApiSetSoundTouchRate,
		MsgApiSetSoundTouchPitch,
		MsgApiPlayBgm,
		MsgApiPauseBgm,
		MsgApiStopBgm,
		MsgApiSetBgmVolume,
		MsgApiSetMicAndBgmBypassToSpeaker,
		MsgApiSetReverbEnabled,
		MsgApiSetVadCallbackEnabled,
		MsgApiSetBgmDelay,
		MsgApiOnHeadsetPlugin,
		MsgApiPauseConf,
		MsgApiResumeConf,
		MsgApiOnReceiveSessionUserIdPair,
		MsgApiPacketStatReport,
		MsgApiSetRecordingTimeMs,
		MsgApiSetPlayingTimeMs,
		MsgApiSetMicLevelCallback,
		MsgApiSetFarendVoiceLevelCallback,
		MsgApiSetReleaseMicWhenMute,
		MsgApiOpenVideoEncoder,
		MsgApiSetVideoRenderCbEnabled,
		MsgApiMaskVideoByUserId,
		//        MsgApiStartCapture,
		//        MsgApiStopCapture,
		MsgApiSetVideoRuntimeEventCb,
		MsgApiQueryHttpInfo,         //http接口向服务器查询数据
		MsgApiGetChannelUserList,       //查询房间用户列表
		MsgApiSetAutoSendStatus,
		MsgApiSetSaveScreenEnabled,


		MsgApiSendMessage,
		MsgApiSetOutputToSpeaker,
		MsgApiKickOther,
		MsgApiBeKick,
		MsgApiStartPush,
		MsgApiStopPush,
		MsgApiSetPushMix,
		MsgApiClearPushMix,
		MsgApiAddPushMixUser,
		MsgApiRemovePushMixUser,
		MsgApiSetRecordDevice,

		// Callback message types
		MsgCbEvent,
		MsgCbVad,
		MsgCbPcm,
		// MsgCbCallEvent,
		// MsgCbCallCommonStatus,
		// MsgCbCallMemberChange,
		MsgCbHttpInfo,          //http接口向服务器查询数据的返回结果
		MsgCbMemChange,

		MsgCbSentenceBegin,
		MsgCbSentenceEnd,
		MsgCbSentenceChanged,
        
        
        // Worker message types
        MsgWorkerCheckRecordingError,
        
        // PCM callback message
        MsgApiSetPcmCallback,
        MsgApiSetTranscriberCallback,
        
        MsgApiStartGrabMic,
        MsgApiStopGrabMic,
        MsgApiReqGrabMic,
        MsgApiFreeGrabMic,
        MsgApiReqInviteMic,
        MsgApiRspInviteMic,
        MsgApiStopInviteMic,
        MsgApiInitInviteMic,
        MsgCbBroadcast,
        MsgCbAVStatistic,
        MsgApiVideoInputStatus,
        MsgApiAudioInputStatus,
        MsgApiQueryUsersVideoInfo,
        MsgApiSetUsersVideoInfo,
        MsgApiShareInputStatus,
        MsgApiSetStaticsQosCb,
        MsgSetOpusPacketLossPerc,
        MsgSetOpusBitrate,
        MsgApiAVQosStatReport,
    };
    
    MsgType m_msgType;
    union {
        bool      bTrue;
        uint32_t  u32Value;
        int32_t   i32Value;
        float     fValue;
        void*     pVoid;
        
        struct {
            YOUME_RTC_SERVER_REGION regionId;
            string* pStrRegionName;
            bool bAppend;
        } apiSetServerRegion;
        
        struct {
            string* pStrUserID;
            string* pStrRoomID;
            YouMeUserRole_t eUserRole;
            bool needMic;
            bool videoAutoRecv;
        } apiJoinConf;
        
        struct {
            string* pStrUserID;
            string* pStrRoomID;
            YouMeUserRole_t eUserRole;
            bool videoAutoRecv;
        } apiJoinConfMulti;
        
        struct {
            string* pStrRoomID;
        } apiSpeakToConference;
        
        struct {
            string *pStrRoomID;
        } apiLeaveConf;
        
        struct {
            string *pStrRoomID;
            NgnLoginServiceCallback::RoomEventType eventType;
            NgnLoginServiceCallback::RoomEventResult result;
        } apiRoomEvent;
        
        struct {
            string *pStrUserID;
            bool   bTrue;
        } apiSetUserBool;
        
        struct {
            string *pStrFilePath;
            bool bRepeat;
        } apiPlayBgm;
        
        struct {
            uint32_t volume;
        } apiSetBgmVolume;
        
        struct {
            bool bMicEnable;
            bool bBgmEnable;
        } apiSetMicAndBgmBypassToSpeaker;

        struct {
            SessionUserIdPairVector_t *pIdPairVector;
        } apiOnReceiveSessionUserIdPair;
        
        struct {
            string *pQueryCommond;
            string *pQueryBody;
            int requestID;
        } apiQueryHttpInfo;
        
        struct {
            string* pStrRoomID;
            int maxCount;
            bool bNotifyMemChange;
        } apiGetUserList;
        
        struct{
            string* roomID;
            int32_t maxCount;
            int32_t maxTime;
            int32_t mode;
            uint32_t voteTime;
            string *pJsonData;
        }apiStartGrabMic;
        
        struct {
            string* roomID;
            string *pJsonData;
        }apiStopGrabMic;
        
        struct {
            string* roomID;
            int32_t score;
            bool bAutoOpen;
            string *pJsonData;
        }apiReqGrabMic;
        
        struct {
            string* roomID;
        }apiFreeGrabMic;
        
        struct {
            string* roomID;
            string *userID;
            int32_t waitTime;
            int32_t maxTime;
            bool bBroadcast;
            string *pJsonData;
        }apiReqInviteMic;
        
        struct {
            string* roomID;
            string *userID;
            bool bAccept;
            string *pJsonData;
        }apiRspInviteMic;
        
        struct {
            string* roomID;
        }apiStopInviteMic;
        
        struct
        {
            string* roomID;
            int32_t waitTime;
            int32_t talkTime;
        }apiInitInviteMic;
        
        struct
        {
            int requestID;
            string* roomID;
            string* content;
            string* toUserID;
        }apiSendMessage;
        
        struct
        {
            string* roomID;
            string* userID;
            int32_t lastTime;
        }apiKickOther;
        
        struct {
            string *roomID;
            string *param;
        } apiBeKick;
        
        struct
        {
            string* url;
        }apiStartPush;
        
        struct
        {
            string* url;
            int32_t width;
            int32_t height;
        }apiSetMixPush;
        
        
        struct
        {
            string* userID;
            int32_t x;
            int32_t y;
            int32_t z;
            int32_t width;
            int32_t height;
        }apiAddMixPushUser;
        
        struct
        {
            string* userID;
        }apiRemoveMixPushUser;
        
        struct {
            YouMeEvent event;
            YouMeErrorCode error;
            std::string * room;
            std::string * param;
        } cbEvent;
        
        struct {
            int32_t sessionId;
            bool bSilence;
        } cbVad;
        
        struct {
            tdav_session_audio_frame_buffer_t* frameBuffer;
            int callbackFlag;
        } cbPcm;
        
        struct {
            string *pStrFilePath;
        } apiOpenVideoEncoder;
        
        struct {
            string *pStrUserId;
            int mask;
        } apiMaskVideoByUserId;
        
        struct {
            int cameraStatus;
        }apiCamStatusChange;
        
        //struct {
        //    YouMeEvent       event;
        //   YouMeErrorCode   errCode;
        //	std::string      *pStrRoomID;
        //} cbCallEvent;
        
        //struct {
        //    YouMeEvent          event;
        //    int                 status;
        //    string              *pStrUserID;
        //} cbCallCommonStatus;
        
        //struct {
        //    string              *pStrUserIDs;
        //	std::string         *pStrRoomID;
        //} cbCallMemberChange;
        
        struct {
            int requestID;
            string *pQuery;
            string *pJsonData;
            YouMeErrorCode_t errCode;
        } cbHttpInfo;
        
        struct {
            string* roomID;
            std::list<MemberChangeInner>* pListMemChagne;
            bool bUpdate;
        } cbMemChange;
        
        struct
        {
            YouMeBroadcast bc;
            string* roomID;
            string* param1;
            string* param2;
            string* pJsonData;
        } cbBroadcast;
        
        struct {
            YouMeAVStatisticType  type;
            std::string * userID ;
            int value;
        } cbAVStatistic;

		struct {
			int sessionId;
			int sentenceIndex;
			int sentenceTime;
		} cbSentenceBegin;

		struct {
			int sessionId;
			int sentenceIndex;
			std::string* result;
			int sentenceTime;
		} cbSentenceEnd;
        
        struct {
            int inputStatus;
            YouMeErrorCode errorCode;
        } apiVideoInputStatusChange;
        
        struct {
            int inputStatus;
        }apiAudioInputStatusChange;

        struct {
            int inputStatus;
        }apiShareInputStatusChange;

        struct{
            std::vector<std::string>* userList;
        }apiQueryUsersVideoInfo;
        
        struct{
            std::vector<IYouMeVoiceEngine::userVideoInfo>* videoInfoList;
        }apiSetUsersVideoInfo;
        
        struct{
            std::string* deviceUid;
        }apiSetDevice;
        
        struct
        {
            void* pcmCallback;
            int pcmCallbackFlag;
        }apiSetPcmCallback;

        
        struct
        {
            void* transcriberCallback;
            bool enable;
        }apiSetTranscriberCallback;
        
    } m_param;
    
    CMessageBlock(MsgType msgType)
    {
        m_msgType = msgType;
        
        // New objects if necessary
        switch (msgType) {
            case MsgApiSetServerRegion:
                m_param.apiSetServerRegion.pStrRegionName = new(nothrow) string();
                break;
            case MsgApiJoinConfSingle:
            case MsgApiJoinConfMulti:
                m_param.apiJoinConf.pStrRoomID = new(nothrow) string();
                m_param.apiJoinConf.pStrUserID = new(nothrow) string();
                break;
            case MsgApiSpeakToConference:
                m_param.apiSpeakToConference.pStrRoomID = new (nothrow)string();
                break;
            case MsgApiLeaveConfMulti:
                m_param.apiLeaveConf.pStrRoomID = new(nothrow) string();
                break;
            case MsgApiRoomEvent:
                m_param.apiRoomEvent.pStrRoomID = new(nothrow) string();
                break;
            case MsgApiSetOthersMicOn:
            case MsgApiSetOthersSpeakerOn:
            case MsgApiSetOthersMuteToMe:
                m_param.apiSetUserBool.pStrUserID = new(nothrow) string();
                break;
            case MsgApiPlayBgm:
                m_param.apiPlayBgm.pStrFilePath = new(nothrow) string();
                break;
            case MsgApiOnReceiveSessionUserIdPair:
                m_param.apiOnReceiveSessionUserIdPair.pIdPairVector = new(nothrow) SessionUserIdPairVector_t;
                break;
            case MsgApiQueryHttpInfo:
                m_param.apiQueryHttpInfo.pQueryCommond = new(nothrow) string();
                m_param.apiQueryHttpInfo.pQueryBody = new(nothrow) string();
                break;
            case MsgApiGetChannelUserList:
                m_param.apiGetUserList.pStrRoomID = new( nothrow) string();
                break;
            case MsgApiStartGrabMic:
                m_param.apiStartGrabMic.roomID = new(nothrow)string();
                m_param.apiStartGrabMic.pJsonData = new(nothrow)string();
                break;
            case MsgApiStopGrabMic:
                m_param.apiStopGrabMic.roomID = new(nothrow)string();
                m_param.apiStopGrabMic.pJsonData = new(nothrow)string();
                break;
            case MsgApiReqGrabMic:
                m_param.apiReqGrabMic.roomID = new(nothrow)string();
                m_param.apiReqGrabMic.pJsonData = new(nothrow)string();
                break;
            case MsgApiFreeGrabMic:
                m_param.apiFreeGrabMic.roomID = new( nothrow) string();
                break;
            case MsgApiReqInviteMic:
                m_param.apiReqInviteMic.roomID = new(nothrow)string();
                m_param.apiReqInviteMic.userID = new(nothrow)string();
                m_param.apiReqInviteMic.pJsonData = new(nothrow)string();
                break;
            case MsgApiRspInviteMic:
                m_param.apiRspInviteMic.roomID = new(nothrow)string();
                m_param.apiRspInviteMic.userID = new(nothrow)string();
                m_param.apiRspInviteMic.pJsonData = new(nothrow)string();
                break;
            case MsgApiStopInviteMic:
                m_param.apiStopInviteMic.roomID = new(nothrow)string();
                break;
            case MsgApiInitInviteMic:
                m_param.apiInitInviteMic.roomID = new(nothrow)string();
                break;
            case MsgApiSendMessage:
                m_param.apiSendMessage.roomID = new ( nothrow )string ();
                m_param.apiSendMessage.content = new ( nothrow )string();
                m_param.apiSendMessage.toUserID = new ( nothrow )string();
                break;
            case MsgApiKickOther:
                m_param.apiKickOther.roomID = new (nothrow )string();
                m_param.apiKickOther.userID = new (nothrow ) string();
                break;
            case MsgApiBeKick:
                m_param.apiBeKick.roomID = new (nothrow )string();
                m_param.apiBeKick.param = new (nothrow ) string();
                break;
            case MsgApiStartPush:
                m_param.apiStartPush.url = new (nothrow) string();
                break;
            case MsgApiSetPushMix:
                m_param.apiSetMixPush.url = new (nothrow) string();
                break;
            case MsgApiAddPushMixUser:
                m_param.apiAddMixPushUser.userID = new (nothrow ) string();
                break;
            case MsgApiRemovePushMixUser:
                m_param.apiRemoveMixPushUser.userID = new (nothrow ) string();
                break;
            case MsgCbEvent:
                m_param.cbEvent.room = new (nothrow)string();
                m_param.cbEvent.param = new (nothrow)string();
                break;
            case MsgCbPcm:
                m_param.cbPcm.frameBuffer = NULL;
                break;
                /***
                 case MsgCbCallCommonStatus:
                 m_param.cbCallCommonStatus.pStrUserID = new(nothrow) string();
                 break;
                 case MsgCbCallMemberChange:
                 m_param.cbCallMemberChange.pStrUserIDs = new(nothrow) string();
                 m_param.cbCallMemberChange.pStrRoomID = new (nothrow) string();
                 break;
                 **/
            case MsgCbHttpInfo:
                m_param.cbHttpInfo.pJsonData = new(nothrow) string();
                m_param.cbHttpInfo.pQuery = new(nothrow) string();
                break;
            case MsgCbMemChange:
                m_param.cbMemChange.roomID = new(nothrow) string();
                m_param.cbMemChange.pListMemChagne = new( nothrow) std::list<MemberChangeInner>();
                break;
            case MsgCbBroadcast:
                m_param.cbBroadcast.roomID = new(nothrow)string();
                m_param.cbBroadcast.param1 = new(nothrow)string();
                m_param.cbBroadcast.param2 = new(nothrow)string();
                m_param.cbBroadcast.pJsonData = new(nothrow)string();
                break;
			case MsgCbSentenceEnd:
			case MsgCbSentenceChanged:
				m_param.cbSentenceEnd.result = new (nothrow)string();
				break;
            case MsgApiOpenVideoEncoder:
                m_param.apiOpenVideoEncoder.pStrFilePath = new(nothrow) string();
                break;
            case MsgApiMaskVideoByUserId:
                m_param.apiMaskVideoByUserId.pStrUserId = new (nothrow) string();
                break;
            case MsgCbAVStatistic:
                m_param.cbAVStatistic.userID = new(nothrow) string();
                break;
            case MsgApiQueryUsersVideoInfo:
                m_param.apiQueryUsersVideoInfo.userList = new(nothrow) std::vector<std::string>();
                break;
            case MsgApiSetUsersVideoInfo:
                m_param.apiSetUsersVideoInfo.videoInfoList = new(nothrow) std::vector<IYouMeVoiceEngine::userVideoInfo>();
                break;
            case MsgApiSetRecordDevice:
                m_param.apiSetDevice.deviceUid = new(nothrow) string();
                break;
            default:
                break;
        }
    }
    
    ~CMessageBlock()
    {
        // Delete objects
        switch (m_msgType) {
            case MsgApiSetServerRegion:
                if (m_param.apiSetServerRegion.pStrRegionName) {
                    delete m_param.apiSetServerRegion.pStrRegionName;
                    m_param.apiSetServerRegion.pStrRegionName = NULL;
                }
                break;
            case MsgApiJoinConfSingle:
            case MsgApiJoinConfMulti:
                if (m_param.apiJoinConf.pStrRoomID) {
                    delete m_param.apiJoinConf.pStrRoomID;
                    m_param.apiJoinConf.pStrRoomID = NULL;
                }
                if (m_param.apiJoinConf.pStrUserID) {
                    delete m_param.apiJoinConf.pStrUserID;
                    m_param.apiJoinConf.pStrUserID = NULL;
                }
                break;
            case MsgApiSpeakToConference:
                if (m_param.apiSpeakToConference.pStrRoomID != NULL) {
                    delete m_param.apiSpeakToConference.pStrRoomID;
                    m_param.apiSpeakToConference.pStrRoomID = NULL;
                }
                break;
            case MsgApiLeaveConfMulti:
                if (m_param.apiLeaveConf.pStrRoomID) {
                    delete m_param.apiLeaveConf.pStrRoomID;
                    m_param.apiLeaveConf.pStrRoomID = NULL;
                }
                break;
            case MsgApiRoomEvent:
                if (m_param.apiRoomEvent.pStrRoomID) {
                    delete m_param.apiRoomEvent.pStrRoomID;
                    m_param.apiRoomEvent.pStrRoomID = NULL;
                }
                break;
            case MsgApiSetOthersMicOn:
            case MsgApiSetOthersSpeakerOn:
            case MsgApiSetOthersMuteToMe:
                if (m_param.apiSetUserBool.pStrUserID) {
                    delete m_param.apiSetUserBool.pStrUserID;
                    m_param.apiSetUserBool.pStrUserID = NULL;
                }
                break;
            case MsgApiPlayBgm:
                if (m_param.apiPlayBgm.pStrFilePath) {
                    delete m_param.apiPlayBgm.pStrFilePath;
                    m_param.apiPlayBgm.pStrFilePath = NULL;
                }
                break;
            case MsgApiOnReceiveSessionUserIdPair:
                if (m_param.apiOnReceiveSessionUserIdPair.pIdPairVector) {
                    delete m_param.apiOnReceiveSessionUserIdPair.pIdPairVector;
                    m_param.apiOnReceiveSessionUserIdPair.pIdPairVector = NULL;
                }
                break;
            case MsgApiQueryHttpInfo:
                if( m_param.apiQueryHttpInfo.pQueryCommond){
                    delete  m_param.apiQueryHttpInfo.pQueryCommond ;
                    m_param.apiQueryHttpInfo.pQueryCommond = NULL;
                }
                if( m_param.apiQueryHttpInfo.pQueryBody){
                    delete  m_param.apiQueryHttpInfo.pQueryBody ;
                    m_param.apiQueryHttpInfo.pQueryBody = NULL;
                }
                break;
            case MsgApiGetChannelUserList:
                if ( m_param.apiGetUserList.pStrRoomID ){
                    delete m_param.apiGetUserList.pStrRoomID ;
                    m_param.apiGetUserList.pStrRoomID  = NULL;
                }
                break;
            case MsgApiStartGrabMic:
                SAFE_DELETE(m_param.apiStartGrabMic.roomID);
                SAFE_DELETE(m_param.apiStartGrabMic.pJsonData);
                break;
            case MsgApiStopGrabMic:
                SAFE_DELETE(m_param.apiStopGrabMic.roomID);
                SAFE_DELETE(m_param.apiStopGrabMic.pJsonData);
                break;
            case MsgApiReqGrabMic:
                SAFE_DELETE(m_param.apiReqGrabMic.roomID);
                SAFE_DELETE(m_param.apiReqGrabMic.pJsonData);
                break;
            case MsgApiFreeGrabMic:
                SAFE_DELETE(m_param.apiFreeGrabMic.roomID);
                break;
            case MsgApiReqInviteMic:
                SAFE_DELETE(m_param.apiReqInviteMic.roomID);
                SAFE_DELETE(m_param.apiReqInviteMic.userID);
                SAFE_DELETE(m_param.apiReqInviteMic.pJsonData);
                break;
            case MsgApiRspInviteMic:
                SAFE_DELETE(m_param.apiRspInviteMic.roomID);
                SAFE_DELETE(m_param.apiRspInviteMic.userID);
                SAFE_DELETE(m_param.apiRspInviteMic.pJsonData);
                break;
            case MsgApiStopInviteMic:
                SAFE_DELETE(m_param.apiStopInviteMic.roomID);
                break;
            case MsgApiInitInviteMic:
                SAFE_DELETE(m_param.apiInitInviteMic.roomID);
                break;
            case MsgApiSendMessage:
                SAFE_DELETE( m_param.apiSendMessage.roomID );
                SAFE_DELETE( m_param.apiSendMessage.content );
                SAFE_DELETE( m_param.apiSendMessage.toUserID );
                break;
            case MsgApiKickOther:
                SAFE_DELETE( m_param.apiKickOther.roomID );
                SAFE_DELETE( m_param.apiKickOther.userID );
                break;
            case MsgApiBeKick:
                SAFE_DELETE( m_param.apiBeKick.roomID );
                SAFE_DELETE( m_param.apiBeKick.param );
                break;
            case MsgApiStartPush:
                SAFE_DELETE( m_param.apiStartPush.url );
                break;
            case MsgApiSetPushMix:
                SAFE_DELETE( m_param.apiSetMixPush.url );
                break;
            case MsgApiAddPushMixUser:
                SAFE_DELETE( m_param.apiAddMixPushUser.userID );
                break;
            case MsgApiRemovePushMixUser:
                SAFE_DELETE( m_param.apiRemoveMixPushUser.userID );
                break;
            case MsgCbEvent:
                SAFE_DELETE(m_param.cbEvent.room);
                SAFE_DELETE(m_param.cbEvent.param);
                break;
            case MsgCbPcm:
                if (m_param.cbPcm.frameBuffer) {
                    tsk_object_unref(m_param.cbPcm.frameBuffer);
                    m_param.cbPcm.frameBuffer = NULL;
                }
                break;
                /***
                 case MsgCbCallCommonStatus:
                 if (m_param.cbCallCommonStatus.pStrUserID) {
                 delete m_param.cbCallCommonStatus.pStrUserID;
                 m_param.cbCallCommonStatus.pStrUserID = NULL;
                 }
                 break;
                 case MsgCbCallMemberChange:
                 if (m_param.cbCallMemberChange.pStrUserIDs) {
                 delete m_param.cbCallMemberChange.pStrUserIDs;
                 m_param.cbCallMemberChange.pStrUserIDs = NULL;
                 }
                 if (m_param.cbCallMemberChange.pStrRoomID != NULL) {
                 delete m_param.cbCallMemberChange.pStrRoomID;
                 m_param.cbCallMemberChange.pStrRoomID = NULL;
                 }
                 break;
                 ***/
            case MsgCbHttpInfo:
                if( m_param.cbHttpInfo.pJsonData){
                    delete  m_param.cbHttpInfo.pJsonData ;
                    m_param.cbHttpInfo.pJsonData = NULL;
                }
                
                if( m_param.cbHttpInfo.pQuery){
                    delete  m_param.cbHttpInfo.pQuery ;
                    m_param.cbHttpInfo.pQuery = NULL;
                }
                break;
            case MsgCbMemChange:
                if( m_param.cbMemChange.roomID ){
                    delete  m_param.cbMemChange.roomID;
                    m_param.cbMemChange.roomID = NULL;
                }
                
                if( m_param.cbMemChange.pListMemChagne != NULL ){
                    delete m_param.cbMemChange.pListMemChagne;
                    m_param.cbMemChange.pListMemChagne = NULL;
                }
                break;
            case MsgCbBroadcast:
                SAFE_DELETE(m_param.cbBroadcast.roomID);
                SAFE_DELETE(m_param.cbBroadcast.param1);
                SAFE_DELETE(m_param.cbBroadcast.param2);
                SAFE_DELETE(m_param.cbBroadcast.pJsonData);
                break;
			case MsgCbSentenceEnd:
			case MsgCbSentenceChanged:
				SAFE_DELETE(m_param.cbSentenceEnd.result);
				break;
            case MsgApiOpenVideoEncoder:
                if (m_param.apiOpenVideoEncoder.pStrFilePath) {
                    delete m_param.apiOpenVideoEncoder.pStrFilePath;
                    m_param.apiOpenVideoEncoder.pStrFilePath = NULL;
                }
                break;
            case MsgApiMaskVideoByUserId:
                if (m_param.apiMaskVideoByUserId.pStrUserId) {
                    delete m_param.apiMaskVideoByUserId.pStrUserId;
                    m_param.apiMaskVideoByUserId.pStrUserId = NULL;
                }
                break;
            case MsgCbAVStatistic:
                SAFE_DELETE(m_param.cbAVStatistic.userID );
                break;
            case MsgApiQueryUsersVideoInfo:
                if(m_param.apiQueryUsersVideoInfo.userList){
                    delete  m_param.apiQueryUsersVideoInfo.userList;
                    m_param.apiQueryUsersVideoInfo.userList = NULL;
                }
                break;
            case MsgApiSetUsersVideoInfo:
                if(m_param.apiSetUsersVideoInfo.videoInfoList){
                    delete m_param.apiSetUsersVideoInfo.videoInfoList;
                    m_param.apiSetUsersVideoInfo.videoInfoList = NULL;
                }
                
                break;
            case MsgApiSetRecordDevice:
                if( m_param.apiSetDevice.deviceUid )
                {
                    SAFE_DELETE( m_param.apiSetDevice.deviceUid );
                }
                break;
            default:
                break;
        }
    }
};

class CMessageLoop
{
public:
    typedef void (*handler_t)(void* pContext, CMessageBlock* pMsg);
    
    CMessageLoop(handler_t handler, void* pContext, const char* pName) {
        m_handler = handler;
        m_pContext = pContext;
        m_strName = pName;
    }
    
    ~CMessageLoop() {
        Stop();
    }
    
    void Start() {
        Stop();
        m_msgQueue.clear();
        m_isLooping = true;
        m_thread = std::thread(&CMessageLoop::ThreadFunc, this);
    }
    
    void Stop() {
        if (m_thread.joinable()) {
            if (std::this_thread::get_id() != m_thread.get_id()) {
                m_isLooping = false;
                m_msgQueueMutex.lock();
                m_msgQueueCond.notify_all();
                m_msgQueueMutex.unlock();
                TSK_DEBUG_INFO("Start joining %s thread", m_strName.c_str());
                m_thread.join();
                TSK_DEBUG_INFO("Joining %s thread OK", m_strName.c_str());
            } else {
                m_thread.detach();
            }
            ClearMessageQueue();
        }
    }
    
    void SendMessage(CMessageBlock* pMsg) {
        std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
        m_msgQueue.push_back(pMsg);
        m_msgQueueCond.notify_one();
    }
    
    void ClearMessageQueue() {
        std::lock_guard<std::mutex> queueLock(m_msgQueueMutex);
        std::deque<CMessageBlock*>::iterator it;
        while (!m_msgQueue.empty()) {
            CMessageBlock* pMsg = m_msgQueue.front();
            m_msgQueue.pop_front();
            if (pMsg) {
                delete pMsg;
                pMsg = NULL;
            }
        }
    }
    
private:
    void ThreadFunc() {
        TSK_DEBUG_INFO("Enter %s thread", m_strName.c_str());
        
        while (m_isLooping) {
            std::unique_lock<std::mutex> queueLock(m_msgQueueMutex);
            while (m_isLooping && m_msgQueue.empty()) {
                m_msgQueueCond.wait(queueLock);
            }
            if (!m_isLooping) {
                break;
            }
            CMessageBlock* pMsg = m_msgQueue.front();
            m_msgQueue.pop_front();
            queueLock.unlock();
            
            if (!pMsg) {
                continue;
            }
            
            if (m_handler) {
                m_handler(m_pContext, pMsg);
            }
            
            delete pMsg;
            pMsg = NULL;
        }
        
        TSK_DEBUG_INFO("Leave %s thread", m_strName.c_str());
    }
    
    
private:
    handler_t m_handler;
    void* m_pContext;
    
    std::thread m_thread;
    std::deque<CMessageBlock*> m_msgQueue;
    std::mutex m_msgQueueMutex;
    std::condition_variable m_msgQueueCond;
    bool m_isLooping;
    std::string m_strName;
};

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
CYouMeVoiceEngine *CYouMeVoiceEngine::mPInstance = NULL;
mutex CYouMeVoiceEngine::mInstanceMutex;
CYouMeVoiceEngine::CYouMeVoiceEngine ()
:m_bTranslateThreadExit(false)
{
#ifdef ANDROID
    m_inputSampleRate = SAMPLE_RATE_44; //android 默认
#else
    m_inputSampleRate = SAMPLE_RATE_48; //ios默认
#endif
    
#ifdef WIN32
	tdav_win32_init();
#endif
    m_isInRoom = false;
    initLangCodeMap();
}

CYouMeVoiceEngine::~CYouMeVoiceEngine ()
{
    if (m_pMainMsgLoop) {
        m_pMainMsgLoop->Stop();
        delete m_pMainMsgLoop;
        m_pMainMsgLoop = NULL;
    }
    
    if (m_pCbMsgLoop) {
        m_pCbMsgLoop->Stop();
        delete m_pCbMsgLoop;
        m_pCbMsgLoop = NULL;
    }
    
    if (m_pWorkerMsgLoop) {
        m_pWorkerMsgLoop->Stop();
        delete m_pWorkerMsgLoop;
        m_pWorkerMsgLoop = NULL;
    }
    
    if (m_pRoomMgr) {
        delete m_pRoomMgr;
        m_pRoomMgr = NULL;
    }
    
    SAFE_DELETE(m_RoomPropMgr);
#ifdef WIN32
	tdav_win32_deinit();
#endif
}
CYouMeVoiceEngine *CYouMeVoiceEngine::getInstance ()
{
    if (NULL == mPInstance)
    {
        std::unique_lock<std::mutex> lck (mInstanceMutex);
        if (NULL == mPInstance)
        {
            mPInstance = new CYouMeVoiceEngine ();
        }
    }
    return mPInstance;
}


//
// static
//
void CYouMeVoiceEngine::MainMessgeHandler(void* pContext, CMessageBlock* pMsg)
{
    if (!pContext || !pMsg) {
        return;
    }
    
    CYouMeVoiceEngine *pThis = (CYouMeVoiceEngine*)pContext;
    switch (pMsg->m_msgType) {
        case CMessageBlock::MsgApiInit:
            pThis->doInit();
            break;
        case CMessageBlock::MsgApiSetServerRegion:
            pThis->doSetServerRegion(pMsg->m_param.apiSetServerRegion.regionId,
                                     pMsg->m_param.apiSetServerRegion.bAppend);
            break;
        case CMessageBlock::MsgApiLeaveConfMulti:
            pThis->doLeaveConferenceMulti(*(pMsg->m_param.apiLeaveConf.pStrRoomID));
            break;
        case CMessageBlock::MsgApiLeaveConfAll:
            pThis->doLeaveConferenceAll(pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiJoinConfSingle:
            pThis->doJoinConferenceSingle(*(pMsg->m_param.apiJoinConf.pStrUserID),
                                          *(pMsg->m_param.apiJoinConf.pStrRoomID),
                                          pMsg->m_param.apiJoinConf.eUserRole,
                                          pMsg->m_param.apiJoinConf.needMic,
                                          pMsg->m_param.apiJoinConf.videoAutoRecv);
            break;
        case CMessageBlock::MsgApiJoinConfMulti:
            pThis->doJoinConferenceMulti(*(pMsg->m_param.apiJoinConf.pStrUserID),
                                         *(pMsg->m_param.apiJoinConf.pStrRoomID),
                                         pMsg->m_param.apiJoinConf.eUserRole);
            break;
        case CMessageBlock::MsgApiRoomEvent:
            pThis->doOnRoomEvent(*(pMsg->m_param.apiRoomEvent.pStrRoomID),
                                 pMsg->m_param.apiRoomEvent.eventType,
                                 pMsg->m_param.apiRoomEvent.result);
            break;
        case CMessageBlock::MsgApiSpeakToConference:
            pThis->doSpeakToConference(*(pMsg->m_param.apiSpeakToConference.pStrRoomID));
            break;
        case CMessageBlock::MsgApiReconnect:
            pThis->doReconnect();
            break;
        case CMessageBlock::MsgApiSetMicMute:
            // Reset the AVSessionMgr if the mic need to be released when mute.
            if (!MediaSessionMgr::defaultsGetExternalInputMode()){
                //#ifndef YOUME_FOR_QINIU
                if ((pThis->m_bReleaseMicWhenMute || pThis->m_bCurrentSilentMic) && pThis->NeedMic() && pThis->m_avSessionMgr) {
                    pThis->stopAvSessionManager();
                    pThis->startAvSessionManager(pMsg->m_param.bTrue ? false : true, pThis->m_bOutputToSpeaker, pMsg->m_param.bTrue ? false : true);
                }
                //#endif
            }
            pThis->m_bMicMute = pMsg->m_param.bTrue;
            pThis->applyMicMute(pMsg->m_param.bTrue);
#if defined(ANDROID)
            JNI_Set_Mic_isMute(pMsg->m_param.bTrue ? 1 : 0);
#endif
            break;
        case CMessageBlock::MsgApiSetSpeakerMute:
            pThis->m_bSpeakerMute = pMsg->m_param.bTrue;
            pThis->applySpeakerMute(pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiSetAutoSendStatus:
            pThis->doSetAutoSend( pMsg->m_param.bTrue );
            break;
        case CMessageBlock::MsgApiSetVolume:
            pThis->applyVolume(pMsg->m_param.u32Value);
            break;
        case CMessageBlock::MsgApiCheckRecordingError:
            pThis->checkRecoringError();
            break;
        case CMessageBlock::MsgApiSetOthersMicOn:
            pThis->sendEventToServer(YouMeProtocol::MIC_CTR_STATUS,
                                     pMsg->m_param.apiSetUserBool.bTrue,
                                     *(pMsg->m_param.apiSetUserBool.pStrUserID));
            break;
        case CMessageBlock::MsgApiSetOthersSpeakerOn:
            pThis->sendEventToServer(YouMeProtocol::SPEAKER_CTR_STATUS,
                                     pMsg->m_param.apiSetUserBool.bTrue,
                                     *(pMsg->m_param.apiSetUserBool.pStrUserID));
            break;
        case CMessageBlock::MsgApiSetOthersMuteToMe:
            pThis->sendEventToServer(YouMeProtocol::AVOID_STATUS,
                                     pMsg->m_param.apiSetUserBool.bTrue,
                                     *(pMsg->m_param.apiSetUserBool.pStrUserID));
            break;
        case CMessageBlock::MsgApiSetAecEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setAECEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetAnsEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setNSEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetAgcEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setAGCEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetVadEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setVADEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetSoundTouchEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setSoundtouchEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetSoundTouchTempo:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setSoundtouchTempo(pMsg->m_param.fValue);
            }
            break;
        case CMessageBlock::MsgApiSetSoundTouchRate:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setSoundtouchRate(pMsg->m_param.fValue);
            }
            break;
        case CMessageBlock::MsgApiSetSoundTouchPitch:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setSoundtouchPitch(pMsg->m_param.fValue);
            }
            break;
        case CMessageBlock::MsgApiPlayBgm:
            pThis->doPlayBackgroundMusic((*pMsg->m_param.apiPlayBgm.pStrFilePath),
                                         pMsg->m_param.apiPlayBgm.bRepeat);
            break;
        case CMessageBlock::MsgApiPauseBgm:
            pThis->doPauseBackgroundMusic(pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiStopBgm:
            pThis->doStopBackgroundMusic();
            break;
        case CMessageBlock::MsgApiSetMicAndBgmBypassToSpeaker:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setMicBypassToSpeaker(pMsg->m_param.apiSetMicAndBgmBypassToSpeaker.bMicEnable);
                pThis->m_avSessionMgr->setBgmBypassToSpeaker(pMsg->m_param.apiSetMicAndBgmBypassToSpeaker.bBgmEnable);
            }
            break;
        case CMessageBlock::MsgApiSetReverbEnabled:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setReverbEnabled(pMsg->m_param.bTrue);
            }
            break;
        case CMessageBlock::MsgApiSetVadCallbackEnabled:
            if (pThis->m_avSessionMgr) {
                if (pMsg->m_param.bTrue) {
                    pThis->m_avSessionMgr->setVadCallback((void*)vadCallback);
                } else {
                    pThis->m_avSessionMgr->setVadCallback(NULL);
                }
            }
            break;
        case CMessageBlock::MsgApiSetPcmCallback:
            pThis->doSetPcmCallback( (IYouMePcmCallback*)pMsg->m_param.apiSetPcmCallback.pcmCallback, pMsg->m_param.apiSetPcmCallback.pcmCallbackFlag );
            break;
        case CMessageBlock::MsgApiSetTranscriberCallback:
            pThis->doSetTranscriberCallback( (IYouMeTranscriberCallback*)pMsg->m_param.apiSetTranscriberCallback.transcriberCallback, pMsg->m_param.apiSetTranscriberCallback.enable );
            break;
        case CMessageBlock::MsgApiSetBgmVolume:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setMixAudioTrackVolume((uint8_t)YOUME_RTC_BGM_TO_MIC,
                                                              (uint8_t)(pMsg->m_param.apiSetBgmVolume.volume));
            }
            break;
        case CMessageBlock::MsgApiSetBgmDelay:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setBackgroundMusicDelay(pMsg->m_param.u32Value);
            }
            break;
        case CMessageBlock::MsgApiSetSaveScreenEnabled:
            if( pThis->m_avSessionMgr ){
                pThis->m_avSessionMgr->setSaveScreenEnabled( pMsg->m_param.bTrue );
            }
            break;
        case CMessageBlock::MsgApiOnHeadsetPlugin:
        {
            bool bPlugin = pMsg->m_param.i32Value == 0 ? false : true;
            // Reset the AVSessionMgr and exit comm mode when headset plugin.
            if (pThis->m_bExitCommModeWhenHeadsetPlugin && (pThis->m_bHeadsetPlugin != bPlugin) && pThis->NeedMic() && pThis->m_avSessionMgr) {
                pThis->m_bHeadsetPlugin = bPlugin;
                pThis->stopAvSessionManager();
                pThis->startAvSessionManager(pThis->m_bMicMute ? false : true, pThis->m_bOutputToSpeaker, true);
            } else {
                pThis->m_bHeadsetPlugin = bPlugin;
                if (pThis->m_avSessionMgr) {
                    pThis->m_avSessionMgr->onHeadsetPlugin((int)pMsg->m_param.i32Value);
                }
            }
        }
            break;
        case CMessageBlock::MsgApiPauseConf:
            pThis->doPauseConference((bool)pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiResumeConf:
            pThis->doResumeConference((bool)pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiOnReceiveSessionUserIdPair:
            pThis->doOnReceiveSessionUserIdPair(*(pMsg->m_param.apiOnReceiveSessionUserIdPair.pIdPairVector));
            break;
        case CMessageBlock::MsgApiPacketStatReport:
            pThis->doPacketStatReport();
            break;
        case CMessageBlock::MsgApiAVQosStatReport:
            pThis->doAVQosStatReport();
            break;
        case CMessageBlock::MsgCbVad:
            pThis->doNotifyVadStatus(pMsg->m_param.cbVad.sessionId, pMsg->m_param.cbVad.bSilence);
            break;
        case CMessageBlock::MsgApiSetRecordingTimeMs:
            pThis->m_nRecordingTimeMs = (int64_t)pMsg->m_param.u32Value;
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setRecordingTimeMs(pMsg->m_param.u32Value);
            }
            break;
        case CMessageBlock::MsgApiSetPlayingTimeMs:
            pThis->m_nPlayingTimeMs = (int64_t)pMsg->m_param.u32Value;
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setPlayingTimeMs(pMsg->m_param.u32Value);
            }
            break;
        case CMessageBlock::MsgApiSetMicLevelCallback:
            if (pThis->m_avSessionMgr) {
                if (pMsg->m_param.i32Value > 0) {
                    pThis->m_avSessionMgr->setMicLevelCallback((void*)micLevelCallback);
                } else {
                    pThis->m_avSessionMgr->setMicLevelCallback(NULL);
                }
                pThis->m_avSessionMgr->setMaxMicLevelCallback(pMsg->m_param.i32Value);
            }
            break;
        case CMessageBlock::MsgApiSetFarendVoiceLevelCallback:
            if (pThis->m_avSessionMgr) {
                if (pMsg->m_param.i32Value > 0) {
                    pThis->m_avSessionMgr->setFarendVoiceLevelCallback((void*)farendVoiceLevelCallback);
                } else {
                    pThis->m_avSessionMgr->setFarendVoiceLevelCallback(NULL);
                }
                pThis->m_avSessionMgr->setMaxFarendVoiceLevel(pMsg->m_param.i32Value);
            }
            break;
        case CMessageBlock::MsgApiSetReleaseMicWhenMute:
            pThis->m_bReleaseMicWhenMute = pMsg->m_param.bTrue;
            if (pThis->NeedMic() && pThis->m_avSessionMgr) {
                pThis->stopAvSessionManager();
                if (pThis->m_bReleaseMicWhenMute) {
                    pThis->startAvSessionManager(pThis->m_bMicMute ? false : true, pThis->m_bOutputToSpeaker,false);
                } else {
                    pThis->startAvSessionManager(true, pThis->m_bOutputToSpeaker,true);
                }
            }
            break;
        case CMessageBlock::MsgApiStartGrabMic:
            pThis->doStartGrabMicAction(*(pMsg->m_param.apiStartGrabMic.roomID),
                                        pMsg->m_param.apiStartGrabMic.maxCount, pMsg->m_param.apiStartGrabMic.maxTime, pMsg->m_param.apiStartGrabMic.mode, pMsg->m_param.apiStartGrabMic.voteTime,
                                        *(pMsg->m_param.apiStartGrabMic.pJsonData));
            break;
        case CMessageBlock::MsgApiStopGrabMic:
            pThis->doStopGrabMicAction(*(pMsg->m_param.apiStopGrabMic.roomID), *(pMsg->m_param.apiStopGrabMic.pJsonData));
            break;
        case CMessageBlock::MsgApiReqGrabMic:
            pThis->doRequestGrabMic(*(pMsg->m_param.apiReqGrabMic.roomID), pMsg->m_param.apiReqGrabMic.score, pMsg->m_param.apiReqGrabMic.bAutoOpen,
                                    *(pMsg->m_param.apiReqGrabMic.pJsonData));
            break;
        case CMessageBlock::MsgApiFreeGrabMic:
            pThis->doFreeGrabMic(*(pMsg->m_param.apiFreeGrabMic.roomID));
            break;
        case CMessageBlock::MsgApiReqInviteMic:
            pThis->doRequestInviteMic(*(pMsg->m_param.apiReqInviteMic.roomID), *(pMsg->m_param.apiReqInviteMic.userID),
                                      pMsg->m_param.apiReqInviteMic.waitTime, pMsg->m_param.apiReqInviteMic.maxTime, pMsg->m_param.apiReqInviteMic.bBroadcast,
                                      *(pMsg->m_param.apiReqInviteMic.pJsonData));
            break;
        case CMessageBlock::MsgApiRspInviteMic:
            pThis->doResponseInviteMic(*(pMsg->m_param.apiRspInviteMic.roomID), *(pMsg->m_param.apiRspInviteMic.userID), pMsg->m_param.apiRspInviteMic.bAccept,
                                       *(pMsg->m_param.apiRspInviteMic.pJsonData));
            break;
        case CMessageBlock::MsgApiStopInviteMic:
            pThis->doStopInviteMic(*(pMsg->m_param.apiStopInviteMic.roomID));
            break;
        case CMessageBlock::MsgApiInitInviteMic:
            pThis->doInitInviteMic(*(pMsg->m_param.apiInitInviteMic.roomID), pMsg->m_param.apiInitInviteMic.waitTime, pMsg->m_param.apiInitInviteMic.talkTime);
            break;
        case CMessageBlock::MsgApiSendMessage:
            pThis->doSendMessage( pMsg->m_param.apiSendMessage.requestID, *(pMsg->m_param.apiSendMessage.roomID), *(pMsg->m_param.apiSendMessage.content), *(pMsg->m_param.apiSendMessage.toUserID));
            break;
        case CMessageBlock::MsgApiKickOther:
            pThis->doKickOther( *(pMsg->m_param.apiKickOther.roomID), *(pMsg->m_param.apiKickOther.userID) , pMsg->m_param.apiKickOther.lastTime );
            break;
        case CMessageBlock::MsgApiBeKick:
            pThis->doBeKickFromChannel( *(pMsg->m_param.apiBeKick.roomID  ) ,   *(pMsg->m_param.apiBeKick.param ));
            break;
        case CMessageBlock::MsgApiStartPush:
            pThis->doStartPush( pThis->mRoomID, pThis->mStrUserID, *(pMsg->m_param.apiStartPush.url));
            break;
        case CMessageBlock::MsgApiStopPush:
            pThis->doStopPush( pThis->mRoomID, pThis->mStrUserID);
            break;
        case CMessageBlock::MsgApiSetPushMix:
            pThis->doSetPushMix( pThis->mRoomID, pThis->mStrUserID,  *(pMsg->m_param.apiSetMixPush.url),
                                pMsg->m_param.apiSetMixPush.width , pMsg->m_param.apiSetMixPush.height );
            break;
        case CMessageBlock::MsgApiClearPushMix:
            pThis->doClearPushMix( pThis->mRoomID );
            break;
        case CMessageBlock::MsgApiAddPushMixUser:
            pThis->doAddPushMixUser( pThis->mRoomID, *(pMsg->m_param.apiAddMixPushUser.userID),
                                     pMsg->m_param.apiAddMixPushUser.x,  pMsg->m_param.apiAddMixPushUser.y,  pMsg->m_param.apiAddMixPushUser.z,
                                    pMsg->m_param.apiAddMixPushUser.width, pMsg->m_param.apiAddMixPushUser.width);
            break;
        case CMessageBlock::MsgApiRemovePushMixUser:
            pThis->doRemovePushMixUser( pThis->mRoomID, *(pMsg->m_param.apiRemoveMixPushUser.userID) );
            break;
        case CMessageBlock::MsgApiSetOutputToSpeaker:
            pThis->doSetOutputToSpeaker(pMsg->m_param.bTrue);
            break;
        case CMessageBlock::MsgApiOpenVideoEncoder:
            pThis->doOpenVideoEncoder(*pMsg->m_param.apiOpenVideoEncoder.pStrFilePath);
            break;
        case CMessageBlock::MsgApiSetVideoRenderCbEnabled:
            if (pThis->m_avSessionMgr) {
                if (pMsg->m_param.bTrue) {
                    pThis->m_avSessionMgr->setVideoRenderCb((void*)videoRenderCb);
                } else {
                    pThis->m_avSessionMgr->setVideoRenderCb(NULL);
                }
            }
            break;
        case CMessageBlock::MsgApiMaskVideoByUserId:
            pThis->doMaskVideoByUserId(*pMsg->m_param.apiMaskVideoByUserId.pStrUserId, pMsg->m_param.apiMaskVideoByUserId.mask);
            break;
//        case CMessageBlock::MsgApiStartCapture:
//        case CMessageBlock::MsgApiStopCapture:
//            pThis->doCamStatusChgReport(pMsg->m_param.apiCamStatusChange.cameraStatus);
//            break;
        case CMessageBlock::MsgApiSetVideoRuntimeEventCb:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setVideoRuntimeEventCb((void*)videoRuntimeEventCb);
            }
            break;
        case CMessageBlock::MsgApiSetStaticsQosCb:
            if (pThis->m_avSessionMgr) {
                pThis->m_avSessionMgr->setStaticsQosCb((void*)onStaticsQosCb);
            }
            break;
        case CMessageBlock::MsgSetOpusPacketLossPerc:
            if (pThis->m_avSessionMgr) {
                int packetLossPerc = pMsg->m_param.i32Value;
                pThis->m_avSessionMgr->setOpusPacketLossPerc(packetLossPerc);
            }
            break;
        case CMessageBlock::MsgSetOpusBitrate:
            if (pThis->m_avSessionMgr) {
                int bitrate = pMsg->m_param.i32Value;
                pThis->m_avSessionMgr->setOpusBitrate(bitrate);
            }
            break;
        case CMessageBlock::MsgApiQueryHttpInfo:
            pThis->doQueryHttpInfo( pMsg->m_param.apiQueryHttpInfo.requestID, *(pMsg->m_param.apiQueryHttpInfo.pQueryCommond), *(pMsg->m_param.apiQueryHttpInfo.pQueryBody) );
            break;
        case CMessageBlock::MsgApiGetChannelUserList:
            pThis->doGetChannelUserList( *(pMsg->m_param.apiGetUserList.pStrRoomID),
                                        pMsg->m_param.apiGetUserList.maxCount,
                                        pMsg->m_param.apiGetUserList.bNotifyMemChange );
            break;
        case CMessageBlock::MsgApiVideoInputStatus:
            pThis->doVideoInputStatusChgReport(pMsg->m_param.apiVideoInputStatusChange.inputStatus, pMsg->m_param.apiVideoInputStatusChange.errorCode);
            break;
        case CMessageBlock::MsgApiShareInputStatus:
            pThis->doShareInputStatusChgReport(pMsg->m_param.apiAudioInputStatusChange.inputStatus);
            break;    
        case CMessageBlock::MsgApiAudioInputStatus:
            pThis->doAudioInputStatusChgReport(pMsg->m_param.apiAudioInputStatusChange.inputStatus);
            break;
        case CMessageBlock::MsgApiQueryUsersVideoInfo:
            pThis->doQueryUsersVideoInfo(pMsg->m_param.apiQueryUsersVideoInfo.userList);
            break;
        case CMessageBlock::MsgApiSetUsersVideoInfo:
            pThis->doSetUsersVideoInfo(pMsg->m_param.apiSetUsersVideoInfo.videoInfoList);
            break;
        case CMessageBlock::MsgApiSetRecordDevice:
            pThis->doSetRecordDevice( *(pMsg->m_param.apiSetDevice.deviceUid) );
            break;
        default:
            TSK_DEBUG_ERROR("Unknown main msg type:%d", pMsg->m_msgType);
            break;
    }
}

//
// static
//
void CYouMeVoiceEngine::CbMessgeHandler(void* pContext, CMessageBlock* pMsg)
{
    if (!pContext || !pMsg) {
        return;
    }
    
    CYouMeVoiceEngine *pThis = (CYouMeVoiceEngine*)pContext;
    
    if (NULL == pThis->mPEventCallback) {
        TSK_DEBUG_ERROR("callback pointers are null");
        return;
    }
    
    switch (pMsg->m_msgType) {
        case CMessageBlock::MsgCbEvent:
            if ((YOUME_EVENT_OTHERS_VOICE_ON != pMsg->m_param.cbEvent.event)
                && (YOUME_EVENT_OTHERS_VOICE_OFF != pMsg->m_param.cbEvent.event)
                && (YOUME_EVENT_MY_MIC_LEVEL != pMsg->m_param.cbEvent.event)
                && (YOUME_EVENT_FAREND_VOICE_LEVEL != pMsg->m_param.cbEvent.event)) {
                TSK_DEBUG_INFO("Send Event callback, event(%d):%s, errCode:%d, room:%s, param:%s",
                               pMsg->m_param.cbEvent.event,
                               eventToString(pMsg->m_param.cbEvent.event),
                               pMsg->m_param.cbEvent.error,
                               pMsg->m_param.cbEvent.room->c_str(),
                               pMsg->m_param.cbEvent.param->c_str());
            }
            
            pThis->mPEventCallback->onEvent(pMsg->m_param.cbEvent.event,
                                            pMsg->m_param.cbEvent.error,
                                            pMsg->m_param.cbEvent.room->c_str(),
                                            pMsg->m_param.cbEvent.param->c_str());
            break;
            /***
             case CMessageBlock::MsgCbCallEvent:
             TSK_DEBUG_INFO("Send CallEvent callback, event:%s, errCode:%d, roomid:%s",
             callEventToString(pMsg->m_param.cbCallEvent.event), pMsg->m_param.cbCallEvent.errCode, pMsg->m_param.cbCallEvent.pStrRoomID->c_str());
             pThis->mPEventCallback->onEvent (pMsg->m_param.cbCallEvent.event, pMsg->m_param.cbCallEvent.errCode,
             *(pMsg->m_param.cbCallEvent.pStrRoomID));
             break;
             case CMessageBlock::MsgCbCallCommonStatus:
             TSK_DEBUG_INFO("Send CommonStatus callback, event:%d, status:%d, userID:%s",
             pMsg->m_param.cbCallCommonStatus.event,
             pMsg->m_param.cbCallCommonStatus.status,
             pMsg->m_param.cbCallCommonStatus.pStrUserID->c_str());
             pThis->mPEventCallback->onEvent(pMsg->m_param.cbCallCommonStatus.event,
             *(pMsg->m_param.cbCallCommonStatus.pStrUserID),
             pMsg->m_param.cbCallCommonStatus.status);
             break;
             case CMessageBlock::MsgCbCallMemberChange:
             TSK_DEBUG_INFO("Send MemberChange callback, userIDs:%s", pMsg->m_param.cbCallMemberChange.pStrUserIDs->c_str());
             pThis->mPEventCallback->onEvent(*(pMsg->m_param.cbCallMemberChange.pStrUserIDs),
             *(pMsg->m_param.cbCallMemberChange.pStrRoomID));
             break;
             ***/
        case CMessageBlock::MsgCbHttpInfo:
            TSK_DEBUG_INFO("Send QueryHttpInfo callback:err:%d, query:%s, result:%s",pMsg->m_param.cbHttpInfo.errCode,  pMsg->m_param.cbHttpInfo.pQuery->c_str(),
                           pMsg->m_param.cbHttpInfo.pJsonData->c_str() );
            if( pThis->mYouMeApiCallback ){
                pThis->mYouMeApiCallback->onRequestRestAPI( pMsg->m_param.cbHttpInfo.requestID,
                                                           pMsg->m_param.cbHttpInfo.errCode, pMsg->m_param.cbHttpInfo.pQuery->c_str(), pMsg->m_param.cbHttpInfo.pJsonData->c_str() ) ;
            }
            break;
        case CMessageBlock::MsgCbMemChange:
            TSK_DEBUG_INFO("Send MemberChange callback" );
            if( pThis->mMemChangeCallback ){
                youmecommon::Value jsonRoot;
                jsonRoot["type"] = youmecommon::Value(2); //CALLBACK_TYPE_MEM_CHANGE , c interface需要靠这个字段区分命令字
                jsonRoot["channelid"] = youmecommon::Value( pMsg->m_param.cbMemChange.roomID->c_str());
                jsonRoot["isUpdate"]  = youmecommon::Value( pMsg->m_param.cbMemChange.bUpdate );
                
                for( auto iter = pMsg->m_param.cbMemChange.pListMemChagne->begin(); iter != pMsg->m_param.cbMemChange.pListMemChagne->end(); ++iter )
                {
                    youmecommon::Value jsonMemChange;
                    jsonMemChange["userid"] = youmecommon::Value( iter->userID.c_str() );
                    jsonMemChange["isJoin"] = youmecommon::Value( iter->isJoin );
                    jsonRoot["memchange"].append(jsonMemChange);
                    
                    //单个用户回调通知
                    pThis->mMemChangeCallback->onMemberChange( pMsg->m_param.cbMemChange.roomID->c_str(),
                                                              iter->userID.c_str() ,iter->isJoin ,
                                                              pMsg->m_param.cbMemChange.bUpdate );
                }
                TSK_DEBUG_INFO("Send MemberChange callback %s",XStringToUTF8(jsonRoot.toSimpleString()).c_str());
                pThis->mMemChangeCallback->onMemberChange( pMsg->m_param.cbMemChange.roomID->c_str(),
                                                          XStringToUTF8(jsonRoot.toSimpleString()).c_str(), pMsg->m_param.cbMemChange.bUpdate);
                
            }
            break;
        case CMessageBlock::MsgCbBroadcast:
            TSK_DEBUG_INFO("Send ChannelBroadcast callback:");
            if (pThis->mNotifyCallback){
                pThis->mNotifyCallback->onBroadcast(pMsg->m_param.cbBroadcast.bc,
                                                    pMsg->m_param.cbBroadcast.roomID->c_str(),
                                                    pMsg->m_param.cbBroadcast.param1->c_str(),
                                                    pMsg->m_param.cbBroadcast.param2->c_str(),
                                                    pMsg->m_param.cbBroadcast.pJsonData->c_str());
            }
            break;
        case CMessageBlock::MsgCbAVStatistic:
            if( pThis->mAVStatistcCallback )
            {
                pThis->mAVStatistcCallback->onAVStatistic( pMsg->m_param.cbAVStatistic.type,
                                                          pMsg->m_param.cbAVStatistic.userID->c_str(),
                                                          pMsg->m_param.cbAVStatistic.value);
            }
            break;
		case CMessageBlock::MsgCbSentenceBegin:
			if (pThis->mPTranscriberCallback)
			{
				int sessionId = pMsg->m_param.cbSentenceBegin.sessionId;
				std::string userid = pThis->getUserNameBySessionId(sessionId);
				if (sessionId == 0)
				{
					userid = pThis->mStrUserID;
				}
				pThis->mPTranscriberCallback->onSentenceBegin(userid, pMsg->m_param.cbSentenceBegin.sentenceIndex);
			}
			break;
		case CMessageBlock::MsgCbSentenceChanged:
			if (pThis->mPTranscriberCallback)
			{
				int sessionId = pMsg->m_param.cbSentenceEnd.sessionId;
				std::string userid = pThis->getUserNameBySessionId(sessionId);
				if (sessionId == 0)
				{
					userid = pThis->mStrUserID;
				}
				pThis->mPTranscriberCallback->onSentenceChanged(userid, pMsg->m_param.cbSentenceEnd.sentenceIndex,*(pMsg->m_param.cbSentenceEnd.result));
			}
			break;
		case CMessageBlock::MsgCbSentenceEnd:
			if (pThis->mPTranscriberCallback)
			{
				int sessionId = pMsg->m_param.cbSentenceEnd.sessionId;
				std::string userid = pThis->getUserNameBySessionId(sessionId);
				if (sessionId == 0)
				{
					userid = pThis->mStrUserID;
				}
				pThis->mPTranscriberCallback->onSentenceEnd(userid, pMsg->m_param.cbSentenceEnd.sentenceIndex, *(pMsg->m_param.cbSentenceEnd.result));
			}
			break;
        default:
            TSK_DEBUG_ERROR("Unknown callback msg type:%d", pMsg->m_msgType);
    }
}

//
// Worker thread for some time consuming work.
//
void CYouMeVoiceEngine::WorkerMessgeHandler(void* pContext, CMessageBlock* pMsg)
{
    if (!pContext || !pMsg) {
        return;
    }
    CYouMeVoiceEngine *pThis = (CYouMeVoiceEngine*)pContext;
    
    switch (pMsg->m_msgType) {
        case CMessageBlock::MsgWorkerCheckRecordingError:
            pThis->triggerCheckRecordingError();
            break;
        default:
            TSK_DEBUG_ERROR("Unknown worker msg type:%d", pMsg->m_msgType);
    }
}

//
// Pcm callback thread for sending PCM data to the user for some special purposes.
//
void CYouMeVoiceEngine::PcmCallbackHandler(void* pContext, CMessageBlock* pMsg)
{
    if (!pContext || !pMsg) {
        return;
    }
    CYouMeVoiceEngine *pThis = (CYouMeVoiceEngine*)pContext;
    switch (pMsg->m_msgType) {
        case CMessageBlock::MsgCbPcm:
        {
            tdav_session_audio_frame_buffer_t* frameBuffer = pMsg->m_param.cbPcm.frameBuffer;
            if (pThis->mPPcmCallback && frameBuffer) {
                switch( pMsg->m_param.cbPcm.callbackFlag )
                {
                    case PcmCallbackFlag_Remote:
                        pThis->mPPcmCallback->onPcmDataRemote(frameBuffer->channel_num, frameBuffer->sample_rate, frameBuffer->bytes_per_sample,
                                                              frameBuffer->pcm_data, frameBuffer->pcm_size);
                        break;
                    case PcmCallbackFlag_Record:
                        pThis->mPPcmCallback->onPcmDataRecord(frameBuffer->channel_num, frameBuffer->sample_rate, frameBuffer->bytes_per_sample,
                                                              frameBuffer->pcm_data, frameBuffer->pcm_size);
                        break;
                    case PcmCallbackFlag_Mix:
                        pThis->mPPcmCallback->onPcmDataMix(frameBuffer->channel_num, frameBuffer->sample_rate, frameBuffer->bytes_per_sample,
                                                           frameBuffer->pcm_data, frameBuffer->pcm_size);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        default:
            TSK_DEBUG_ERROR("Unknown PCM callback msg type:%d", pMsg->m_msgType);
    }
}

//
// Set the current state to the specified one
//
void CYouMeVoiceEngine::setState (ConferenceState state)
{
    lock_guard<std::recursive_mutex> stateLock(mStateMutex);
    mState = state;
    TSK_DEBUG_INFO ("-- mState:%s", stateToString(mState));
}

bool CYouMeVoiceEngine::isStateInitialized()
{
    if ((STATE_INITIALIZED == mState) && !mIsAboutToUninit) {
        return true;
    } else {
        return false;
    }
}

const char* CYouMeVoiceEngine::stateToString(ConferenceState state)
{
    switch (state) {
        case STATE_INITIALIZING:
            return "Initializing";
        case STATE_INIT_FAILED:
            return "InitFailed";
        case STATE_INITIALIZED:
            return "Initialized";
        case STATE_UNINITIALIZED:
            return "Uninitialized";
        default:
            return "Unknown";
    }
}

const char* CYouMeVoiceEngine::eventToString(YouMeEvent event)
{
    switch (event) {
        case YOUME_EVENT_INIT_OK:
            return "INIT_OK";
        case YOUME_EVENT_INIT_FAILED:
            return "INIT_FAILED";
        case YOUME_EVENT_JOIN_OK:
            return "JOIN_OK";
        case YOUME_EVENT_JOIN_FAILED:
            return "JOIN_FAILED";
        case YOUME_EVENT_LEAVED_ONE:
            return "LEAVED_ONE";
        case YOUME_EVENT_LEAVED_ALL:
            return "LEAVED_ALL";
        case YOUME_EVENT_PAUSED:
            return "PAUSED";
        case YOUME_EVENT_RESUMED:
            return "RESUMED";
        case YOUME_EVENT_SPEAK_SUCCESS:
            return "SPEAK_SUCCESS";
        case YOUME_EVENT_SPEAK_FAILED:
            return "SPEAK_FAILED";
        case YOUME_EVENT_RECONNECTING:
            return "RECONNECTING";
        case YOUME_EVENT_RECONNECTED:
            return "RECONNECTED";
        case YOUME_EVENT_REC_PERMISSION_STATUS:
            return "REC_PERMISSION_STATUS";
        case YOUME_EVENT_BGM_STOPPED:
            return "BGM_STOPPED";
        case YOUME_EVENT_BGM_FAILED:
            return "BGM_FAILED";
//        case YOUME_EVENT_MEMBER_CHANGE:
//            return "YOUME_EVENT_MEMBER_CHANGE";
            
        case YOUME_EVENT_OTHERS_MIC_ON:
            return "OTHERS_MIC_ON";
        case YOUME_EVENT_OTHERS_MIC_OFF:
            return "OTHERS_MIC_OFF";
        case YOUME_EVENT_OTHERS_SPEAKER_ON:
            return "OTHERS_SPEAKER_ON";
        case YOUME_EVENT_OTHERS_SPEAKER_OFF:
            return "OTHERS_SPEAKER_OFF";
        case YOUME_EVENT_OTHERS_VOICE_ON:
            return "OTHERS_VOICE_ON";
        case YOUME_EVENT_OTHERS_VOICE_OFF:
            return "OTHERS_VOICE_OFF";
        case YOUME_EVENT_MY_MIC_LEVEL:
            return "MIC_LEVEL";
            
        case YOUME_EVENT_MIC_CTR_ON:
            return "MIC_CTR_ON";
        case YOUME_EVENT_MIC_CTR_OFF:
            return "MIC_CTR_OFF";
        case YOUME_EVENT_SPEAKER_CTR_ON:
            return "SPEAKER_CTR_ON";
        case YOUME_EVENT_SPEAKER_CTR_OFF:
            return "SPEAKER_CTR_OFF";
            
        case YOUME_EVENT_LISTEN_OTHER_ON:
            return "LISTEN_OTHER_ON";
        case YOUME_EVENT_LISTEN_OTHER_OFF:
            return "LISTEN_OTHER_OFF";

        case YOUME_EVENT_LOCAL_MIC_ON:
            return "LOCAL_MIC_ON";
        case YOUME_EVENT_LOCAL_MIC_OFF:
            return "LOCAL_MIC_OFF";
        case YOUME_EVENT_LOCAL_SPEAKER_ON:
            return "LOCAL_SPEAKER_ON";
        case YOUME_EVENT_LOCAL_SPEAKER_OFF:
            return "LOCAL_SPEAKER_OFF";

        case YOUME_EVENT_GRABMIC_START_OK:
            return "GRABMIC_START_OK";
        case YOUME_EVENT_GRABMIC_START_FAILED:
            return "GRABMIC_START_FAILED";
        case YOUME_EVENT_GRABMIC_STOP_OK:
            return "GRABMIC_STOP_OK";
        case YOUME_EVENT_GRABMIC_STOP_FAILED:
            return "GRABMIC_STOP_FAILED";
        case YOUME_EVENT_GRABMIC_REQUEST_OK:
            return "GRABMIC_REQUEST_OK";
        case YOUME_EVENT_GRABMIC_REQUEST_FAILED:
            return "GRABMIC_REQUEST_FAILED";
        case YOUME_EVENT_GRABMIC_REQUEST_WAIT:
            return "GRABMIC_REQUEST_WAIT";
        case YOUME_EVENT_GRABMIC_RELEASE_OK:
            return "GRABMIC_RELEASE_OK";
        case YOUME_EVENT_GRABMIC_RELEASE_FAILED:
            return "GRABMIC_RELEASE_FAILED";
        case YOUME_EVENT_GRABMIC_ENDMIC:
            return "GRABMIC_ENDMIC";

        case YOUME_EVENT_GRABMIC_NOTIFY_START:
            return "GRABMIC_NOTIFY_START";
        case YOUME_EVENT_GRABMIC_NOTIFY_STOP:
            return "GRABMIC_NOTIFY_STOP";
        case YOUME_EVENT_GRABMIC_NOTIFY_HASMIC:
            return "GRABMIC_NOTIFY_HASMIC";
        case YOUME_EVENT_GRABMIC_NOTIFY_NOMIC:
            return "GRABMIC_NOTIFY_NOMIC";

        case YOUME_EVENT_INVITEMIC_SETOPT_OK:
            return "INVITEMIC_SETOPT_OK";
        case YOUME_EVENT_INVITEMIC_SETOPT_FAILED:
            return "INVITEMIC_SETOPT_FAILED";
        case YOUME_EVENT_INVITEMIC_REQUEST_OK:
            return "INVITEMIC_REQUEST_OK";
        case YOUME_EVENT_INVITEMIC_REQUEST_FAILED:
            return "INVITEMIC_REQUEST_FAILED";
        case YOUME_EVENT_INVITEMIC_RESPONSE_OK:
            return "INVITEMIC_RESPONSE_OK";
        case YOUME_EVENT_INVITEMIC_RESPONSE_FAILED:
            return "INVITEMIC_RESPONSE_FAILED";
        case YOUME_EVENT_INVITEMIC_STOP_OK:
            return "INVITEMIC_STOP_OK";
        case YOUME_EVENT_INVITEMIC_STOP_FAILED:
            return "INVITEMIC_STOP_FAILED";

        case YOUME_EVENT_INVITEMIC_CAN_TALK:
            return "INVITEMIC_CAN_TALK";
        case YOUME_EVENT_INVITEMIC_CANNOT_TALK:
            return "INVITEMIC_CANNOT_TALK";

        case YOUME_EVENT_INVITEMIC_NOTIFY_CALL:
            return "INVITEMIC_NOTIFY_CALL";
        case YOUME_EVENT_INVITEMIC_NOTIFY_ANSWER:
            return "INVITEMIC_NOTIFY_ANSWER";
        case YOUME_EVENT_INVITEMIC_NOTIFY_CANCEL:
            return "INVITEMIC_NOTIFY_CANCEL";
            
        case YOUME_EVENT_SEND_MESSAGE_RESULT:
            return "SEND_MESSAGE_RESULT";
        case YOUME_EVENT_MESSAGE_NOTIFY:
            return "MESSAGE_NOTIFY";
            
        case YOUME_EVENT_KICK_RESULT:
            return "KICK_RESULT";
        case YOUME_EVENT_KICK_NOTIFY:
            return "KICK_NOTIFY";
            
        case YOUME_EVENT_FAREND_VOICE_LEVEL:
            return "FAREND_VOICE_LEVEL";
            
        case YOUME_EVENT_OTHERS_BE_KICKED:
            return "OTHERS_BE_KICKED";

        case YOUME_EVENT_OTHERS_VIDEO_ON:
            return "OTHERS_VIDEO_ON";
//        case YOUME_EVENT_OTHERS_VIDEO_OFF:
//            return "OTHERS_VIDEO_OFF";
//        case YOUME_EVENT_OTHERS_CAMERA_PAUSE:
//            return "OTHERS_CAMERA_PAUSE";
//        case YOUME_EVENT_OTHERS_CAMERA_RESUME:
//            return "OTHERS_CAMERA_RESUME";
        case YOUME_EVENT_MASK_VIDEO_BY_OTHER_USER:
            return "MASK_VIDEO_BY_OTHERS";
        case YOUME_EVENT_RESUME_VIDEO_BY_OTHER_USER:
            return "RESUME_VIDEO_BY_OTHERS";
        case YOUME_EVENT_MASK_VIDEO_FOR_USER:
            return "MASK_VIDEO_FOR_OTHERS";
        case YOUME_EVENT_RESUME_VIDEO_FOR_USER:
            return "RESUME_VIDEO_FOR_OTHERS";
        case YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN:
            return "OTHERS_VIDEO_SHUT_DOWN";
        case YOUME_EVENT_OTHERS_VIDEO_INPUT_START:
            return "OTHERS_VIDEO_INPUT_START";
        case YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP:
            return "OTHERS_VIDEO_INPUT_STOP";

        case YOUME_EVENT_LOCAL_SHARE_INPUT_START:
            return "LOCAL_SHARE_INPUT_START";
        case YOUME_EVENT_LOCAL_SHARE_INPUT_STOP:
            return "LOCAL_SHARE_INPUT_STOP";
        case YOUME_EVENT_OTHERS_SHARE_INPUT_START:
            return "OTHERS_SHARE_INPUT_START";
        case YOUME_EVENT_OTHERS_SHARE_INPUT_STOP:
            return "OTHERS_SHARE_INPUT_STOP";

        case YOUME_EVENT_MEDIA_DATA_ROAD_PASS:
            return "MEDIA_DATA_ROAD_PASS";
        case YOUME_EVENT_MEDIA_DATA_ROAD_BLOCK:
            return "MEDIA_DATA_ROAD_BLOCK";
            
        case YOUME_EVENT_QUERY_USERS_VIDEO_INFO:
            return "QUERY_USERS_VIDEO_INFO";
        case YOUME_EVENT_SET_USERS_VIDEO_INFO:
            return "SET_USERS_VIDEO_INFO";
        case YOUME_EVENT_SET_USERS_VIDEO_INFO_NOTIFY:
            return "SET_USERS_VIDEO_INFO_NOTIFY";   
        case YOUME_EVENT_LOCAL_VIDEO_INPUT_START:
            return "LOCAL_VIDEO_INPUT_START";
        case YOUME_EVENT_LOCAL_VIDEO_INPUT_STOP:
            return "LOCAL_VIDEO_INPUT_STOP";
            
        case YOUME_EVENT_OTHERS_DATA_ERROR:
            return "OTHERS_DATA_ERROR";
        case YOUME_EVENT_OTHERS_NETWORK_BAD:
            return "OTHERS_NETWORK_BAD";
        case YOUME_EVENT_OTHERS_BLACK_FULL:
            return "OTHERS_BLACK_FULL";
        case YOUME_EVENT_OTHERS_GREEN_FULL:
            return "OTHERS_GREEN_FULL";
        case YOUME_EVENT_OTHERS_BLACK_BORDER:
            return "OTHERS_BLACK_BORDER";
        case YOUME_EVENT_OTHERS_GREEN_BORDER:
            return "OTHERS_GREEN_BORDER";
        case YOUME_EVENT_OTHERS_BLURRED_SCREEN:
            return "OTHERS_BLURRED_SCREEN";
        case YOUME_EVENT_OTHERS_ENCODER_ERROR:
            return "OTHERS_ENCODER_ERROR";
        case YOUME_EVENT_OTHERS_DECODER_ERROR:
            return "OTHERS_DECODER_ERROR";
            
        case YOUME_EVENT_CAMERA_DEVICE_CONNECT:
            return "CAMERA_DEVICE_CONNECT";
        case YOUME_EVENT_CAMERA_DEVICE_DISCONNECT:
            return "CAMERA_DEVICE_DISCONNECT";
        
        case YOUME_EVENT_RECOGNIZE_MODULE_INIT_START:
            return "RECOGNIZE_MODULE_INIT_START";
        case YOUME_EVENT_RECOGNIZE_MODULE_INIT_END:
            return "RECOGNIZE_MODULE_INIT_END";
        case YOUME_EVENT_RECOGNIZE_MODULE_UNINIT:
            return "RECOGNIZE_MODULE_UNINIT";
        case YOUME_EVENT_RECOGNIZE_MODULE_ERR:
            return "RECOGNIZE_MODULE_ERR";
			
		case YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT:
            return "VIDEO_ENCODE_PARAM_REPORT";

        case YOUME_EVENT_RTP_ROUTE_P2P:
            return "YOUME_EVENT_RTP_ROUTE_P2P";
        case YOUME_EVENT_RTP_ROUTE_SEREVER:
            return "YOUME_EVENT_RTP_ROUTE_SEREVER";
        case YOUME_EVENT_RTP_ROUTE_CHANGE_TO_SERVER:
            return "YOUME_EVENT_RTP_ROUTE_CHANGE_TO_SERVER";
        case YOUME_EVENT_EOF:
            return "EOF";
            
        default:
            return "Unknown";
    }
}

bool IsValidChar(char c)
{
    if (((c >= '0') && (c <='9'))||
        ((c >= 'A') && (c <= 'Z'))||
        ((c >= 'a') && (c <= 'z'))||
        (c == '_')||
        (c == '+')||
        (c == '-')||
        (c == '=')||
        (c == '/'))
    {
        return true;
    }
    return  false;
}

void CYouMeVoiceEngine::removeAppKeyFromRoomId(const std::string& strRoomIdFull, std::string& strRoomIdShort)
{
    // If the full room ID is not starting with the app key, it's not actually a full one.
    if (strRoomIdFull.find(mJoinAppKey) == 0) {
        strRoomIdShort = strRoomIdFull.substr(mJoinAppKey.length());
    } else {
        strRoomIdShort = strRoomIdFull;
    }
}
void CYouMeVoiceEngine::NotifyCustomData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSendSessionId )
{
	if (nullptr != m_pCustomDataCallback)
	{
        std::string fromUserId = getUserNameBySessionId(uSendSessionId);
        TSK_DEBUG_INFO("receive custom message from %s", fromUserId.c_str());
		m_pCustomDataCallback->OnCustomDataNotify(pData, iDataLen, uTimeSpan);
	}
}
///////////声网兼容接口//////////////////////////////////////////////
int CYouMeVoiceEngine::enableLocalVideoRender(bool enabled)
{
    TSK_DEBUG_INFO ("@@ enableLocalVideoRender %d",enabled);
    return YouMeVideoMixerAdapter::getInstance()->enableLocalVideoRender( enabled );
}

int CYouMeVoiceEngine::enableLocalVideoSend(bool enabled)
{
    TSK_DEBUG_INFO ("@@ enableLocalVideoSend %d",enabled);
    return YouMeVideoMixerAdapter::getInstance()->enableLocalVideoSend( enabled );
}

int CYouMeVoiceEngine::muteAllRemoteVideoStreams(bool mute)
{
    TSK_DEBUG_INFO ("@@ muteAllRemoteVideoStreams %d",mute);
    return YouMeVideoMixerAdapter::getInstance()->muteAllRemoteVideoStreams( mute );
}

int CYouMeVoiceEngine::setDefaultMuteAllRemoteVideoStreams(bool mute)
{
    TSK_DEBUG_INFO ("@@ setDefaultMuteAllRemoteVideoStreams %d",mute);
    return YouMeVideoMixerAdapter::getInstance()->setDefaultMuteAllRemoteVideoStreams( mute );
}

int CYouMeVoiceEngine::muteRemoteVideoStream(std::string& uid, bool mute)
{
    TSK_DEBUG_INFO ("@@ muteRemoteVideoStream %s, %d",uid.c_str() ,mute );
    return YouMeVideoMixerAdapter::getInstance()->muteRemoteVideoStream( uid, mute  );
}

#if TARGET_OS_IPHONE || defined(ANDROID)
bool CYouMeVoiceEngine::isCameraZoomSupported()
{
    TSK_DEBUG_INFO ("@@ isCameraZoomSupported");
    return ICameraManager::getInstance()->isCameraZoomSupported();
}
float CYouMeVoiceEngine::setCameraZoomFactor(float zoomFactor)
{
    TSK_DEBUG_INFO ("@@ setCameraZoomFactor");
    return ICameraManager::getInstance()->setCameraZoomFactor( zoomFactor);
}
bool CYouMeVoiceEngine::isCameraFocusPositionInPreviewSupported()
{
    TSK_DEBUG_INFO ("@@ isCameraFocusPositionInPreviewSupported");
    return ICameraManager::getInstance()->isCameraFocusPositionInPreviewSupported();
}
bool CYouMeVoiceEngine::setCameraFocusPositionInPreview( float x , float y )
{
    TSK_DEBUG_INFO ("@@ setCameraFocusPositionInPreview");
    return ICameraManager::getInstance()->setCameraFocusPositionInPreview( x, y );
}
bool CYouMeVoiceEngine::isCameraTorchSupported()
{
    TSK_DEBUG_INFO ("@@ isCameraTorchSupported");
    return ICameraManager::getInstance()->isCameraTorchSupported();
}
bool CYouMeVoiceEngine::setCameraTorchOn( bool isOn )
{
    TSK_DEBUG_INFO ("@@ setCameraTorchOn");
    return ICameraManager::getInstance()->setCameraTorchOn( isOn);
}
bool CYouMeVoiceEngine::isCameraAutoFocusFaceModeSupported()
{
    TSK_DEBUG_INFO ("@@ isCameraAutoFocusFaceModeSupported");
    return ICameraManager::getInstance()->isCameraAutoFocusFaceModeSupported();
}
bool CYouMeVoiceEngine::setCameraAutoFocusFaceModeEnabled( bool enable )
{
    TSK_DEBUG_INFO ("@@ setCameraAutoFocusFaceModeEnabled");
    return ICameraManager::getInstance()->setCameraAutoFocusFaceModeEnabled( enable );
}
#endif

void CYouMeVoiceEngine::setLocalVideoMirrorMode( YouMeVideoMirrorMode mode )
{
    TSK_DEBUG_INFO ("@@ setLocalVideoMirrorMode, mode:%d", mode);
    
    ICameraManager::getInstance()->setLocalVideoMirrorMode( mode );
}


/////////////////////////////////////////////////////////////////////////////////
// The following functions should only be called from the main loop thread.
void CYouMeVoiceEngine::applyMicMute(bool mute,bool notify)
{
    if (!NeedMic()) {
        TSK_DEBUG_INFO("mNeedMic && mInviteMic  is false, Force mic to mute");
        mute = true;
        m_bMicMute = mute;
    }
    
    if (m_avSessionMgr) {
        m_avSessionMgr->setMicrophoneMute(mute);
        if(notify) {
            if (NeedMic() && mAutoSendStatus) {
                sendEventToServer(YouMeProtocol::MIC_STATUS, !mute, mStrUserID);
            }
            
            doCallEvent(mute ? YOUME_EVENT_LOCAL_MIC_OFF : YOUME_EVENT_LOCAL_MIC_ON);
        }
    }
    else{
       if(notify)  doCallEvent(mute ? YOUME_EVENT_LOCAL_MIC_OFF : YOUME_EVENT_LOCAL_MIC_ON, YOUME_ERROR_UNKNOWN);
    }
}

void CYouMeVoiceEngine::applySpeakerMute(bool mute,bool notify)
{
    if (m_avSessionMgr) {
        if (mPPcmCallback && !mPcmOutputToSpeaker) {
            TSK_DEBUG_INFO("Already set PcmCallback and don't Ouput to speaker, Force speaker to mute");
            m_avSessionMgr->setSpeakerMute(true);
            return;
        }
        m_avSessionMgr->setSpeakerMute(mute);
        
         if(notify) {
            if (mAutoSendStatus) {
                sendEventToServer(YouMeProtocol::SPEAKER_STATUS, !mute, mStrUserID);
            }
            
            doCallEvent(mute ? YOUME_EVENT_LOCAL_SPEAKER_OFF : YOUME_EVENT_LOCAL_SPEAKER_ON);
         }
    }
    else{
        if(notify) doCallEvent(mute ? YOUME_EVENT_LOCAL_SPEAKER_OFF : YOUME_EVENT_LOCAL_SPEAKER_ON, YOUME_ERROR_UNKNOWN);
    }
}

void CYouMeVoiceEngine::applyVolume(uint32_t volume)
{
#ifdef ANDROID
    // Apply an extra gain on the output audio volume.
    // This is a workaround for some android phones that have very low volume when in communication mode.
    uint32_t nGain = (uint32_t)CNgnMemoryConfiguration::getInstance()->GetConfiguration( NgnConfigurationEntry::ANDROID_OUTPUT_VOLUME_GAIN,
                                                                                        NgnConfigurationEntry::DEFAULT_ANDROID_OUTPUT_VOLUME_GAIN);
    uint32_t finalVolume = TSK_CLAMP (0, volume * nGain /100, 1000);
#elif __APPLE__
    // This is a workaround for some iPhones to suppress howl.
    uint32_t nGain = (uint32_t)CNgnMemoryConfiguration::getInstance()->GetConfiguration( NgnConfigurationEntry::IOS_OUTPUT_VOLUME_GAIN,
                                                                                        NgnConfigurationEntry::DEFAULT_IOS_OUTPUT_VOLUME_GAIN);
    uint32_t finalVolume = TSK_CLAMP (0, volume * nGain /100, 1000);
#else
    uint32_t finalVolume = volume;
#endif
    
    if (m_avSessionMgr) {
        m_avSessionMgr->setVolume(finalVolume);
    }
    
}

void CYouMeVoiceEngine::sendEventToServer(YouMeProtocol::STATUS_EVENT_TYPE eventType, bool isOn, const std::string& strUserID)
{
    // m_avSessionMgr is not null means we are already in room
    // TODO: added state management in m_loginService
    if (m_avSessionMgr) {
        TSK_DEBUG_INFO ("SendMsg type %d to %s",eventType,strUserID.c_str());
        YouMeProtocol::YouMeVoice_Command_CommonStatus commonStatusEvent;
        commonStatusEvent.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_COMMON_STATUS));
        commonStatusEvent.set_eventtype(eventType);
        commonStatusEvent.set_userid(strUserID);
        commonStatusEvent.set_sessionid (mSessionID);
        commonStatusEvent.set_status(isOn?0:1);
        std::string strMicData;
        commonStatusEvent.SerializeToString(&strMicData);
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_COMMON_STATUS, strMicData.c_str(), strMicData.length());
    }
}

//
// On calling this function, session2UserIdReq should be populated with sessionIDs to be map
//
void CYouMeVoiceEngine::sendSessionUserIdMapRequest(YouMeProtocol::YouMeVoice_Command_Session2UserIdRequest &session2UserIdReq)
{
    TSK_DEBUG_INFO ("Send sessionToUserIdMap request");
    session2UserIdReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_SESSIONID_TO_USERID));
    std::string strRequest;
    session2UserIdReq.SerializeToString(&strRequest);
    m_loginService.AddTCPQueue(YouMeProtocol::MSG_SESSIONID_TO_USERID, strRequest.c_str(), strRequest.length());
}

//
// On calling this function, session2UserIdReq should be populated with sessionIDs to be map
//
YouMeErrorCode CYouMeVoiceEngine::sendSessionUserIdMapRequest(int sessionID)
{
    TSK_DEBUG_INFO ("--Send sessionToUserIdMap request sessionID:%d", sessionID);
    YouMeProtocol::YouMeVoice_Command_Session2UserIdRequest request;
    request.add_sessionid(sessionID);
    request.set_user_session( mSessionID );
    sendSessionUserIdMapRequest(request);
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::sendAVStaticReport( YouMeProtocol::YouMeVoice_Command_BussReport_Req&  request )
{
    std::string strRequest;
    request.SerializeToString(&strRequest);
    m_loginService.AddTCPQueue(YouMeProtocol::MSG_BUSS_REPORT, strRequest.c_str(), strRequest.length());
}

bool CYouMeVoiceEngine::sendCbMsgCallAVStatistic( YouMeAVStatisticType avType,  int32_t sessionId,  int value  )
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (isStateInitialized() && m_pCbMsgLoop) {
        auto userID = getUserNameBySessionId( sessionId );
        if( userID == "" )
        {
            sendSessionUserIdMapRequest(sessionId);            
            return false;
        }
        
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbAVStatistic);
        if (pMsg) {
            if (NULL == pMsg->m_param.cbAVStatistic.userID ) {
                delete pMsg;
                pMsg = NULL;
                return false;
            }
            
            pMsg->m_param.cbAVStatistic.type = avType;
            pMsg->m_param.cbAVStatistic.value = value;
            *(pMsg->m_param.cbAVStatistic.userID) = userID;
            m_pCbMsgLoop->SendMessage(pMsg);
            return true;
        }
    }
    
    TSK_DEBUG_ERROR("Failed to sendCbMsgCallAVStatistic, avType(%d), session:(%d), value:(%d)",
                    avType, sessionId, value);
    return false;
    
    
    
}

bool CYouMeVoiceEngine::sendCbMsgCallEvent(const YouMeEvent eventType, const YouMeErrorCode errCode,
                                           const std::string& strRoomID, const std::string& strParam)
{
    if( eventType == YOUME_EVENT_JOIN_OK ||
       eventType == YOUME_EVENT_LEAVED_ALL ||
       eventType == YOUME_EVENT_LEAVED_ONE )
    {
        m_isInRoom = m_pRoomMgr->getRoomCount() != 0 ;
    }
    
    
#ifdef YOUME_VIDEO
    //视频包，进入房间的时候，获取房间用户列表
    if( eventType == YOUME_EVENT_JOIN_OK || eventType == YOUME_EVENT_RECONNECTED )
    {
        getChannelUserList( strRoomID.c_str() , -1 , true );
        
        m_mapShutdownReason.clear();
    }
#endif
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    //这里处理视频 shutdown的原因记录
    {
        if( /*YOUME_EVENT_OTHERS_CAMERA_PAUSE == eventType ||*/
           YOUME_EVENT_MASK_VIDEO_FOR_USER == eventType  )
        {
            m_mapShutdownReason[ strParam  ] = eventType;
        }
        else if (/*YOUME_EVENT_OTHERS_CAMERA_RESUME == eventType ||*/
                 YOUME_EVENT_RESUME_VIDEO_FOR_USER == eventType   )
        {
            auto it = m_mapShutdownReason.find( strParam.c_str() );
            if( it != m_mapShutdownReason.end() )
            {
                if(/*( YOUME_EVENT_OTHERS_CAMERA_RESUME == eventType && YOUME_EVENT_OTHERS_CAMERA_PAUSE == it->second ) ||*/
                   ( YOUME_EVENT_RESUME_VIDEO_FOR_USER == eventType && YOUME_EVENT_MASK_VIDEO_FOR_USER == it->second ) )
                {
                    m_mapShutdownReason.erase( it );
                }
            }
        }
        
        if( YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN == eventType  )
        {
            //如果YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN有明确的已知原因，就不通知了，因为不是一场中断
            auto it = m_mapShutdownReason.find( strParam );
            if( it != m_mapShutdownReason.end() ){
                m_mapShutdownReason.erase( it );
                return true ;
            }
        }
    }
    
    if (isStateInitialized() && m_pCbMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbEvent);
        if (pMsg) {
            
            if (NULL == pMsg->m_param.cbEvent.room) {
                delete pMsg;
                pMsg = NULL;
                return false;
            }
            
            pMsg->m_param.cbEvent.event = eventType;
            pMsg->m_param.cbEvent.error = errCode;
            *(pMsg->m_param.cbEvent.room) = strRoomID;
            *(pMsg->m_param.cbEvent.param) = strParam;
            m_pCbMsgLoop->SendMessage(pMsg);
            return true;
        }
    }
    
    TSK_DEBUG_ERROR("Failed to send CalllEvent message, eventType(%d):%s, errCode:%d, state:%s",
                    eventType, eventToString(eventType), (int32_t)errCode, stateToString(mState));
    return false;
}

bool CYouMeVoiceEngine::sendCbMsgCallCommonStatus(YouMeEvent eventType, const string &strUserID, int status)
{
    return sendCbMsgCallEvent( eventType, YOUME_SUCCESS, "", strUserID);
}

bool CYouMeVoiceEngine::sendCbMsgCallBroadcastEvent(YouMeBroadcast bc, const std::string& roomID, const std::string& param1, const std::string& param2, const std::string& content)
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (isStateInitialized() && m_pCbMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgCbBroadcast);
        if (pMsg) {
            pMsg->m_param.cbBroadcast.bc = bc;
            *(pMsg->m_param.cbBroadcast.roomID) = roomID;
            *(pMsg->m_param.cbBroadcast.param1) = param1;
            *(pMsg->m_param.cbBroadcast.param2) = param2;
            *(pMsg->m_param.cbBroadcast.pJsonData) = content;
            m_pCbMsgLoop->SendMessage(pMsg);
            return true;
        }
    }
    
    TSK_DEBUG_ERROR("Failed to send sendCbMsgCallBroadcastEvent message, bctype:%d, roomID:%s, param1:%s param2:%s content:%s",
                    bc, roomID.c_str(), param1.c_str(), param2.c_str(), content.c_str());
    return false;
}

bool CYouMeVoiceEngine::sendCbMsgCallOtherVideoDataOn(const std::string & strUserIDs, const std::string & strRoomId, uint16_t width, uint16_t height)
{
    return sendCbMsgCallEvent( YOUME_EVENT_OTHERS_VIDEO_ON, (YouMeErrorCode)(((int32_t)width << 16) | height), strRoomId, strUserIDs );
}

//bool CYouMeVoiceEngine::sendCbMsgCallOtherVideoDataOff(const std::string & strUserIDs, const std::string & strRoomId)
//{
//    return sendCbMsgCallEvent( YOUME_EVENT_OTHERS_VIDEO_OFF, YOUME_SUCCESS, strRoomId, strUserIDs  );
//}

//bool CYouMeVoiceEngine::sendCbMsgCallOtherCameraStatus(const std::string & strUserIDs, const std::string & strRoomId, int status)
//{
//    YouMeEvent evt ;
//    if (!status) {
//        evt = YOUME_EVENT_OTHERS_CAMERA_PAUSE;
//        AVStatistic::getInstance()->NotifyVideoStat( strUserIDs,  false );
//    } else {
//        evt = YOUME_EVENT_OTHERS_CAMERA_RESUME;
//        AVStatistic::getInstance()->NotifyVideoStat( strUserIDs,  true );
//    }
//
//    return sendCbMsgCallEvent( evt, YOUME_SUCCESS, strRoomId, strUserIDs   );
//}

void CYouMeVoiceEngine::doInit()
{
    TSK_DEBUG_INFO ("$$ doInit");

    //先初始化数据上报通道
    ReportService::getInstance()->initDataChannel();

    //uint64_t ulStartTime = tsk_time_now();
    mRedirectServerInfoVec.clear();
    mCanRefreshRedirect = false;
    ReportQuitData::getInstance()->m_valid_count++;
    YouMeErrorCode yErrorcode = CSDKValidate::GetInstance ()->ServerLoginIn (false, mAppKeySuffix, mRedirectServerInfoVec, mCanRefreshRedirect);
    // if ((YOUME_SUCCESS == yErrorcode) && !(g_extServerRegionName.empty())) {
        // The redirect server list returned here is for the region speicifed in mServerRegionName.
        // Not for regions specified in mServerRegionNameMap.
        // mLastServerRegionNameMap.insert(ServerRegionNameMap_t::value_type(g_extServerRegionName, 1));
    // }
    
    // if ((yErrorcode != YOUME_SUCCESS) && (YOUME_ERROR_UNKNOWN != yErrorcode)){
        //触发日志上报
        //MonitoringCenter::getInstance()->UploadLog(UploadType_SDKValidFail,(int)yErrorcode);
        
    // }
#ifdef WIN32
    CMMNotificationClient::Init();
#endif //WIN32
    
    if (YOUME_SUCCESS != yErrorcode)
    {
        TSK_DEBUG_ERROR("SDK validate failed");
        goto bail_out;
    }
    else
    {
        //先判断后台是否关闭了功能
        bool bEnable = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::ENABLE, NgnConfigurationEntry::DEFAULT_ENABLE);
        if (bEnable) {
            //SDK 验证成功了才初始化别的
            CXTimer::GetInstance ()->Init ();
            
            mPNetworkService = mPNgnEngine->getNetworkService ();
            mPNetworkService->registerCallback (this);
            
            start ();
            
            //判断是否需要更新
#ifdef ANDROID
            if (CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::UPDATE, NgnConfigurationEntry::DEFAULT_UPDATE))
            {
                
                std::string strUrl =CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::UPDATE_URL,NgnConfigurationEntry::DEFAULT_UPDATE_URL);
                std::string strMD5 =CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::UPDATE_MD5,NgnConfigurationEntry::DEFAULT_UPDATE_MD5);
                TSK_DEBUG_INFO("Need to update:%s %s",strUrl.c_str(),strMD5.c_str());
                UpdateAndroid(strUrl,strMD5);
            }
#endif
        }
        else
        {
            TSK_DEBUG_INFO("################### Voice is disabled by the server config ##################");
            yErrorcode = YOUME_ERROR_SERVER_INVALID;
            goto bail_out;
        }
    }
 
    {
        // 新的数据上报 初始化成功
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportCommon init;
        init.common_type = REPORT_COMMON_INIT;
        init.result = 0;
        init.param = "";
        init.sdk_version = SDK_NUMBER;
        init.brand = NgnApplication::getInstance()->getBrand();
        init.model = NgnApplication::getInstance()->getModel();
        init.platform = NgnApplication::getInstance()->getPlatform();
        init.canal_id = NgnApplication::getInstance()->getCanalID();
        init.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(init);
    }
    tsk_set_log_maxFileSizeKb(CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::LOG_FILE_SIZE_KB, NgnConfigurationEntry::DEF_LOG_FILE_SIZE_KB));
    if (!tmedia_defaults_get_external_input_mode() && !m_bSetLogLevel)
    {
        // 外部输入模式下，客户需要在初始化之前setLogLevel这里不做处理, 否则使用服务器下发的配置
        tsk_set_log_maxLevelConsole(CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::LOG_LEVEL_CONSOLE, NgnConfigurationEntry::DEF_LOG_LEVEL_CONSOLE));
        tsk_set_log_maxLevelFile(CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::LOG_LEVEL_FILE, NgnConfigurationEntry::DEF_LOG_LEVEL_FILE));
    }
    
    // Init audio mode to "Play" to replace the default one "Ambient", so that the iPhone silence button
    // behaviors the same through all the game. Should be set after init, so that the server config can take effect.
    //
#if defined(__APPLE__)
//     if (!MediaSessionMgr::defaultsGetExternalInputMode()) {
        tdav_InitCategory(0, m_bOutputToSpeaker);
//     }
#endif

    
    //初始化翻译
    {
        int nTranslateSwitch = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::CONFIG_TRANSLATE_ENABLE, NgnConfigurationEntry::TRANSLATE_SWITCH_DEFAULT);
        int nTranslateMethod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_METHOD, NgnConfigurationEntry::TRANSLATE_METHOD_DEFAULT);
        if (nTranslateSwitch && g_ymvideo_pTranslateUtil != NULL)
        {
            std::string strHost = "";
            std::string strCgi = "";
            if (!nTranslateMethod)
            {
                strHost = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_HOST,
                                                                                   NgnConfigurationEntry::TRANSLATE_HOST_DEFAULT);
                strCgi = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_CGI,
                                                                                  NgnConfigurationEntry::TRANSLATE_CGI_PATH_DEFAULT);
                strCgi += "t?";
            }
            else
            {
                strHost = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_HOST_V2,
                                                                                   NgnConfigurationEntry::TRANSLATE_HOST_DEFAULT_V2);
                strCgi = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_CGI_V2,
                                                                                  NgnConfigurationEntry::TRANSLATE_CGI_PATH_DEFAULT_V2);
                strCgi += ("v2?key=");
                std::string strGoogleAPIKey = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_GOOGLE_APIKEY,
                                                                                                       NgnConfigurationEntry::TRANSLATE_GOOGLE_APIKEY_DEFAULT);
                strCgi += strGoogleAPIKey;
            }
            
            std::string strRegularPattern = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_REGULAR,
                                                                                                     NgnConfigurationEntry::TRANSLATE_REGULAR_DEFAULT);
            g_ymvideo_pTranslateUtil->Init( UTF8TOXString(strHost), UTF8TOXString(strCgi), UTF8TOXString(strRegularPattern));
        }
    }
    setState(STATE_INITIALIZED);
    
    if (mPEventCallback) {
        TSK_DEBUG_INFO("Call back for YOUME_EVENT_INIT_OK");
        mPEventCallback->onEvent(YOUME_EVENT_INIT_OK, YOUME_SUCCESS, "", "");
    }
    TSK_DEBUG_INFO ("== doInit");
    return;
    
bail_out:
    setState(STATE_INIT_FAILED);
    if (mPEventCallback) {
        TSK_DEBUG_INFO("Call back for YOUME_EVENT_INIT_FAILED");
        mPEventCallback->onEvent(YOUME_EVENT_INIT_FAILED, yErrorcode, "", "");
    }
    TSK_DEBUG_INFO ("== doInit failed");
}

void CYouMeVoiceEngine::doSetServerRegion(YOUME_RTC_SERVER_REGION serverRegionId, bool bAppend)
{
    std::string regionName;
    
    TSK_DEBUG_INFO ("$$ doSetServerRegion regionId:%d, bAppend:%d", serverRegionId, (int)bAppend);
    
    switch (serverRegionId) {
        case RTC_CN_SERVER:
            regionName = "cn";
            break;
        case RTC_US_SERVER:
            regionName = "us";
            break;
        case RTC_HK_SERVER:
            regionName = "hk";
            break;
        case RTC_SG_SERVER:
            regionName = "sg";
            break;
        case RTC_KR_SERVER:
            regionName = "kr";
            break;
        case RTC_AU_SERVER:
            regionName = "au";
            break;
        case RTC_DE_SERVER:
            regionName = "de";
            break;
        case RTC_BR_SERVER:
            regionName = "br";
            break;
        case RTC_IN_SERVER:
            regionName = "in";
            break;
        case RTC_JP_SERVER:
            regionName = "jp";
            break;
        case RTC_IE_SERVER:
            regionName = "ie";
            break;
        case RTC_USW_SERVER:
            regionName = "usw";
            break;
        case RTC_USM_SERVER:
            regionName = "usm";
            break;
        case RTC_CA_SERVER:
            regionName = "ca";
            break;
        case RTC_LON_SERVER:
            regionName = "lon";
            break;
        case RTC_FRA_SERVER:
            regionName = "fra";
            break;
        case RTC_DXB_SERVER:
            regionName = "dxb";
            break;
        case RTC_EXT_SERVER:
            // regionName = extServerRegionName.substr(0, 5);
            break;
        case RTC_DEFAULT_SERVER:
            TSK_DEBUG_INFO("Default region:%d", serverRegionId);
            regionName = "undefined";
            break;
        default:
            TSK_DEBUG_ERROR("Invalid region:%d", serverRegionId);
            return;
    }
    
    if (bAppend) {
        std::map<std::string, int>::iterator it = mServerRegionNameMap.find(regionName);
        if (it != mServerRegionNameMap.end()) {
            it->second++;
        } else {
            mServerRegionNameMap.insert(std::map<std::string, int>::value_type(regionName, 1));
        }
    } else {
        mServerRegionNameMap.clear();
        mServerRegionNameMap.insert(std::map<std::string, int>::value_type(regionName, 1));
        g_serverRegionId = serverRegionId;
    }
    
    TSK_DEBUG_INFO ("== doSetServerRegion");
}

YouMeErrorCode CYouMeVoiceEngine::setLocalConnectionInfo(std::string strLocalIP, int iLocalPort, std::string strRemoteIP, int iRemotePort, int iNetType)
{
    TSK_DEBUG_INFO("@ setLocalConnectionInfo local[%s:%d], remote[%s:%d], type:%d", strLocalIP.c_str(), iLocalPort, strRemoteIP.c_str(), iRemotePort, iNetType);
    if (strLocalIP.empty() || strRemoteIP.empty() || !iLocalPort || !iRemotePort || iNetType) {
        TSK_DEBUG_ERROR ("invalid param!");
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    Config_SetString( "p2p_local_ip", const_cast<char*>(strLocalIP.c_str()));
    Config_SetInt( "p2p_local_port", iLocalPort);
    Config_SetString( "p2p_remote_ip", const_cast<char*>(strRemoteIP.c_str()));
    Config_SetInt( "p2p_remote_port", iRemotePort);

    return YOUME_SUCCESS;
}


YouMeErrorCode CYouMeVoiceEngine::clearLocalConnectionInfo()
{
    TSK_DEBUG_INFO("@@ clearLocalConnectionInfo");

    Config_SetString( "p2p_local_ip", "");
    Config_SetInt( "p2p_local_port", 0);
    Config_SetString( "p2p_remote_ip", "");
    Config_SetInt( "p2p_remote_port", 0);

    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setRouteChangeFlag(int enable)
{
    TSK_DEBUG_INFO("@ setRouteChangeFlag enable[%d]", enable);
    Config_SetInt("route_change_flag", enable);
    
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::doJoinConferenceSingle(const std::string& strUserID, const string &strRoomID,
                                               YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv)
{
    if (m_pRoomMgr->getRoomCount() > 0) {
        doLeaveConferenceAllProxy();
    }
    doJoinConferenceFirst(strUserID, strRoomID, eUserRole, needMic, bVideoAutoRecv);
    
}

void CYouMeVoiceEngine::doJoinConferenceMulti(const std::string& strUserID, const string &strRoomID, YouMeUserRole_t eUserRole)
{
    if (m_pRoomMgr->getRoomCount() <= 0) {
        doJoinConferenceFirst(strUserID, strRoomID, eUserRole, true, true);
    } else {
        doJoinConferenceMore(strUserID, strRoomID, eUserRole);
    }
}


void CYouMeVoiceEngine::doJoinConferenceFirst(const std::string& strUserID, const string &strRoomID,
                                              YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv)
{
    TSK_DEBUG_INFO ("$$ doJoinConferenceFirst, roomID:%s", strRoomID.c_str());
    
    CRoomManager::RoomInfo_t roomInfo;
    roomInfo.idFull = ToYMRoomID( strRoomID );
    roomInfo.state = CRoomManager::ROOM_STATE_CONNECTING;
    roomInfo.joinRoomTime = tsk_time_now();
    if (!m_pRoomMgr->addRoom(strRoomID, roomInfo)) {
        sendCbMsgCallEvent(YOUME_EVENT_JOIN_FAILED, YOUME_ERROR_UNKNOWN, strRoomID, mStrUserID);
        TSK_DEBUG_ERROR ("== doJoinConferenceFirst, failed to add roomInfo");
        return;
    }
    
    // 外部采集模式下默认不启动mic
    if (MediaSessionMgr::defaultsGetExternalInputMode()){
        needMic = false;
    }
    
    mRoomID = strRoomID;
    mStrUserID = strUserID;
    mVideoAutoRecv = bVideoAutoRecv;
    mNeedMic = needMic;
    mInviteMicOK = false;
    // Reset channel properties
    m_bMicMute = true; // 默认进房间打开麦克风
    m_bSpeakerMute = true;
    m_bMicBypassToSpeaker = false;
    m_bBgmBypassToSpeaker = false;
    m_bReverbEnabled = false;
    m_nPlayingTimeMs = -1;
    m_nRecordingTimeMs = -1;
    m_externalInputStartTime = 0;
    m_externalVideoDuration = 0;
    m_nOpusPacketLossPerc = 10;
    m_nOpusPacketBitrate = 25000;
    m_bCurrentSilentMic = false;
    
    ReportQuitData::getInstance()->m_join_count++;
    
    {
        /* 加入会议数据上报 */
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportChannel join;
        join.operate_type = REPORT_CHANNEL_JOIN;
        join.api_from = 0;
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        join.roomid = strShortRoomID;
        join.need_userlist = false; //不需要这个字段了
        join.need_speaker = false; // obsolete, TODO: remove it
        join.auto_sendstatus = false;
        join.sdk_version = SDK_NUMBER;
        join.video_duration = (unsigned int)ICameraManager::getInstance()->resetDuration();
        join.platform = NgnApplication::getInstance()->getPlatform();
        join.canal_id = NgnApplication::getInstance()->getCanalID();
        join.user_role = eUserRole;
        join.need_mic = mNeedMic;
        join.auto_sendstatus = mAutoSendStatus;
        join.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(join);
    }
    
    // Upload log if required
    bool bUploadLog = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::UPLOAD_LOG, NgnConfigurationEntry::DEF_UPLOAD_LOG);
    string strUploadLogForUser = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::UPLOAD_LOG_FOR_USER, NgnConfigurationEntry::DEF_UPLOAD_LOG_FOR_USER);
    if (bUploadLog || (strUserID == strUploadLogForUser)) {
        TSK_DEBUG_INFO("#### Upload log due to server configuration");
        MonitoringCenter::getInstance()->UploadLog(UploadType_Notify, 0, false);
        CNgnMemoryConfiguration::getInstance()->SetConfiguration(NgnConfigurationEntry::UPLOAD_LOG, false);
        CNgnMemoryConfiguration::getInstance()->SetConfiguration(NgnConfigurationEntry::UPLOAD_LOG_FOR_USER, (string)"");
    }
    
    // Get the configurations from the validate server again.
    YouMeErrorCode errCode = loginToMcu(roomInfo.idFull, eUserRole, false, bVideoAutoRecv);
    if ((YOUME_SUCCESS != errCode) && (YOUME_ERROR_USER_ABORT != errCode)) {
        // Get the configurations/(redirect servers) from the validate server again.
        mRedirectServerInfoVec.clear();
        mCanRefreshRedirect = false;
        ReportQuitData::getInstance()->m_valid_count++;
        if (YOUME_SUCCESS == CSDKValidate::GetInstance()->ServerLoginIn(false, mAppKeySuffix, mRedirectServerInfoVec, mCanRefreshRedirect))
        {
            if (!g_extServerRegionName.empty()) {
                // The redirect server list returned here is for the region speicifed in mServerRegionName.
                // Not for regions specified in mServerRegionNameMap.
                mLastServerRegionNameMap.insert(ServerRegionNameMap_t::value_type(g_extServerRegionName, 1));
            }
            
            errCode = loginToMcu(roomInfo.idFull, eUserRole, false, bVideoAutoRecv);
        }
    }
    
    
    if (YOUME_SUCCESS == errCode) {
        bool needMic = NeedMic();
        if (m_bReleaseMicWhenMute && needMic) {
            needMic = m_bMicMute ? false : true;
        }
        
        errCode = startAvSessionManager(needMic, m_bOutputToSpeaker,true);
        if (YOUME_SUCCESS == errCode)
        {
            m_pRoomMgr->setRoomState(strRoomID, CRoomManager::ROOM_STATE_CONNECTED);
            m_pRoomMgr->setSpeakToRoom(strRoomID);
            sendCbMsgCallEvent(YOUME_EVENT_JOIN_OK, YOUME_SUCCESS, strRoomID, mStrUserID);
            
            if (mRole != mTmpRole && mTmpRole != YOUME_USER_NONE){
                mRole = mTmpRole;
                mTmpRole = YOUME_USER_NONE;
            }
            
            AVStatistic::getInstance()->StartThread();
            AVStatistic::getInstance()->NotifyRoomName( strRoomID , mSessionID, true   );

			if (mTranscriberEnabled)
			{
				YMTranscriberManager::getInstance()->Start();
			}

            if (m_avSessionMgr) {
                tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                if (session) {
                    trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 0);  

                    //在manager层保存businessId信息
                    trtp_manager_set_business_id(TDAV_SESSION_AV(session)->rtp_manager, mBusinessID);
                }
            }
        } else {
            m_pRoomMgr->removeRoom(strRoomID);
            sendCbMsgCallEvent(YOUME_EVENT_JOIN_FAILED, errCode, strRoomID, mStrUserID);
        }
    } else {
        m_pRoomMgr->removeRoom(strRoomID);
        sendCbMsgCallEvent(YOUME_EVENT_JOIN_FAILED, errCode, strRoomID, mStrUserID);
    }
    
    CNgnTalkManager::getInstance()->m_strUserID = mStrUserID;
    

   
    TSK_DEBUG_INFO ("== doJoinConferenceFirst");
}



void CYouMeVoiceEngine::doJoinConferenceMore(const std::string& strUserID, const string &strRoomID, YouMeUserRole_t eUserRole)
{
    TSK_DEBUG_INFO ("$$ doJoinConferenceMore, roomID:%s", strRoomID.c_str());
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    YouMeErrorCode tempErrCode = YOUME_SUCCESS;
    CRoomManager::RoomInfo_t roomInfo;
    
    ReportQuitData::getInstance()->m_join_count++;
    {
        /* 加入会议数据上报 */
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportChannel join;
        join.operate_type = REPORT_CHANNEL_JOIN;
        join.api_from = 1;
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        join.roomid = strShortRoomID;
        join.need_userlist = false; //不需要这个字段了

        join.need_speaker = false; // obsolete, TODO: remove it
        join.auto_sendstatus = false;
        join.sdk_version = SDK_NUMBER;
        join.video_duration = (unsigned int)ICameraManager::getInstance()->resetDuration();
        join.platform = NgnApplication::getInstance()->getPlatform();
        join.canal_id = NgnApplication::getInstance()->getCanalID();
        join.user_role = eUserRole;
        join.need_mic = mNeedMic;
        join.auto_sendstatus = mAutoSendStatus;
        join.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(join);
    }
    
    if (m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        if (CRoomManager::ROOM_STATE_CONNECTED == roomInfo.state) {
            if (mRole != mTmpRole && mTmpRole != YOUME_USER_NONE){
                mRole = mTmpRole;
                mTmpRole = YOUME_USER_NONE;
            }
            sendCbMsgCallEvent(YOUME_EVENT_JOIN_OK, YOUME_SUCCESS, strRoomID, mStrUserID);
            TSK_DEBUG_INFO ("== doJoinConferenceMore, already connected");
        } else if ((CRoomManager::ROOM_STATE_CONNECTING == roomInfo.state) || (CRoomManager::ROOM_STATE_RECONNECTING == roomInfo.state)) {
            TSK_DEBUG_INFO ("== doJoinConferenceMore, already connecting, do nothing");
        } else {
            // It's disconnecting
            m_loginService.JoinChannel(mSessionID, roomInfo.idFull , this);
            TSK_DEBUG_INFO ("== doJoinConferenceMore, already exist, state:%s", CRoomManager::stateToString(roomInfo.state));
        }
        return;
    }
    
    if (m_pRoomMgr->getRoomCount() >=
        CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::MAX_CHANNEL_NUM, NgnConfigurationEntry::DEF_MAX_CHANNEL_NUM)) {
        errCode = YOUME_ERROR_TOO_MANY_CHANNELS;
        goto bail;
    }
    
    roomInfo.idFull = ToYMRoomID( strRoomID );
    roomInfo.state = CRoomManager::ROOM_STATE_CONNECTING;
    roomInfo.joinRoomTime = tsk_time_now();
    if (!m_pRoomMgr->addRoom(strRoomID, roomInfo)) {
        errCode = YOUME_ERROR_UNKNOWN;
        goto bail;
    }
    
    tempErrCode = m_loginService.JoinChannel(mSessionID, roomInfo.idFull , this);
    if (tempErrCode != YOUME_SUCCESS) {
        m_pRoomMgr->removeRoom(strRoomID);
        errCode = YOUME_ERROR_NETWORK_ERROR;
        goto bail;
    }
    
    TSK_DEBUG_INFO ("== doJoinConferenceMore");
    return;
    
bail:
    sendCbMsgCallEvent(YOUME_EVENT_JOIN_FAILED, errCode, strRoomID, mStrUserID);
    TSK_DEBUG_INFO ("== doJoinConferenceMore failed");
    return;
}

void CYouMeVoiceEngine::doJoinConferenceMoreDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result)
{
    TSK_DEBUG_INFO ("$$ doJoinConferenceMoreDone, roomID:%s", strRoomID.c_str());
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        TSK_DEBUG_ERROR("== doJoinConferenceMoreDone failed to get room info");
        return;
    }
    
    if (CRoomManager::ROOM_STATE_RECONNECTING == roomInfo.state) {
        if (NgnLoginServiceCallback::ROOM_EVENT_SUCCESS == result) {
            m_pRoomMgr->setRoomState(strRoomID, CRoomManager::ROOM_STATE_CONNECTED);
        } else {
            // TODO: Added a reconnect mechanism here
        }
    } else {
        if (NgnLoginServiceCallback::ROOM_EVENT_SUCCESS == result) {
            if (mRole != mTmpRole && mTmpRole != YOUME_USER_NONE){
                mRole = mTmpRole;
                mTmpRole = YOUME_USER_NONE;
            }
            m_pRoomMgr->setRoomState(strRoomID, CRoomManager::ROOM_STATE_CONNECTED);
            sendCbMsgCallEvent(YOUME_EVENT_JOIN_OK, YOUME_SUCCESS, strRoomID, mStrUserID);
        } else {
            m_pRoomMgr->removeRoom(strRoomID);
            sendCbMsgCallEvent(YOUME_EVENT_JOIN_FAILED, YOUME_ERROR_NETWORK_ERROR, strRoomID, mStrUserID);
        }
    }
    
    TSK_DEBUG_INFO ("== doJoinConferenceMoreDone");
}

void CYouMeVoiceEngine::doSpeakToConference(const std::string& strRoomID)
{
    TSK_DEBUG_INFO ("$$ doSpeakToConference, roomID:%s", strRoomID.c_str());
    
    if (strRoomID.compare(m_pRoomMgr->getSpeakToRoomId()) == 0) {
        sendCbMsgCallEvent(YOUME_EVENT_SPEAK_SUCCESS, YOUME_SUCCESS, strRoomID, mStrUserID);
        TSK_DEBUG_INFO ("== doSpeakToConference, alredy speak to this room");
        return;
    }
    
    // Handle the special roomID that indicates speaking to all room
    if (strRoomID.compare("all") == 0) {
        TSK_DEBUG_INFO ("speak to all room");
        uint32_t timestamp = m_avSessionMgr->getRtpTimestamp();
        YouMeErrorCode tempErrCode = m_loginService.SpeakToChannel(mSessionID, strRoomID, timestamp);
        if (YOUME_SUCCESS != tempErrCode) {
            sendCbMsgCallEvent(YOUME_EVENT_SPEAK_FAILED, YOUME_ERROR_UNKNOWN, strRoomID, mStrUserID);
        }
        
        TSK_DEBUG_INFO ("== doSpeakToConference");
        return;
    }
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        sendCbMsgCallEvent(YOUME_EVENT_SPEAK_FAILED, YOUME_ERROR_CHANNEL_NOT_EXIST, strRoomID, mStrUserID);
        TSK_DEBUG_INFO ("== doSpeakToConference, room doesn't exist");
        return;
    }
    
    uint32_t timestamp = m_avSessionMgr->getRtpTimestamp();
    TSK_DEBUG_INFO ("speak to room since timestamp:%u", timestamp);
    YouMeErrorCode tempErrCode = m_loginService.SpeakToChannel(mSessionID, roomInfo.idFull, timestamp);
    if (YOUME_SUCCESS != tempErrCode) {
        sendCbMsgCallEvent(YOUME_EVENT_SPEAK_FAILED, YOUME_ERROR_UNKNOWN, strRoomID, mStrUserID);
    }
    
    TSK_DEBUG_INFO ("== doSpeakToConference");
}

void CYouMeVoiceEngine::doSpeakToConferenceDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result)
{
    TSK_DEBUG_INFO ("$$ doSpeakToConferenceDone, roomID:%s, result:%d", strRoomID.c_str(), (int)result);
    
    CRoomManager::RoomInfo_t roomInfo;
    if ((strRoomID.compare("all") != 0) && !m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        TSK_DEBUG_ERROR("== doSpeakToConferenceDone failed to get room info");
        return;
    }
    
    if (NgnLoginServiceCallback::ROOM_EVENT_SUCCESS == result) {
        m_pRoomMgr->setSpeakToRoom(strRoomID);
        sendCbMsgCallEvent(YOUME_EVENT_SPEAK_SUCCESS, YOUME_SUCCESS, strRoomID, mStrUserID);
    } else {
        sendCbMsgCallEvent(YOUME_EVENT_SPEAK_FAILED, YOUME_ERROR_UNKNOWN, strRoomID, mStrUserID);
    }
    
    TSK_DEBUG_INFO ("== doSpeakToConferenceDone");
}

YouMeErrorCode CYouMeVoiceEngine::startAvSessionManager(bool needMic, bool outputToSpeaker ,bool bJoinRoom)
{
    if (MediaSessionMgr::defaultsGetExternalInputMode()){
        //#ifdef YOUME_FOR_QINIU
        needMic = false;
        //#endif
    }
    TSK_DEBUG_INFO("$$ startAvSessionManager needMic:%d, outputToSpeaker:%d, bJoinRoom:%d", needMic, outputToSpeaker, bJoinRoom);

#ifdef MAC_OS
    //重置选定的mic的设备ID,一个设备的uid应该不会变，但audiodeviceid会变，所以记录uid比较好
    {
        int choosedMic = AudioDeviceMgr_OSX::getInstance()->getAudioDeviceID( m_strMicDeviceChoosed.c_str() );
        tmedia_set_record_device( choosedMic );
        tdav_mac_update_use_audio_device();
    }
#elif WIN32
	{
		int chooseMic = AudioDeviceMgr_Win::getInstance()->getDeviceIDByUID(m_strMicDeviceChoosed.c_str());
		tmedia_set_record_device(chooseMic);
	}
#endif
    
    
#if defined(__APPLE__)
    if (!MediaSessionMgr::defaultsGetExternalInputMode())
    {
        int result = hasHeadset ();
        m_bHeadsetPlugin =  result == 2 || result == 1 ? true : false;
        bool bCommModeEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
        if ((m_bExitCommModeWhenHeadsetPlugin && m_bHeadsetPlugin) || (!needMic)) {
            bCommModeEnabled = false;
            TSK_DEBUG_INFO("Disable CommMode, m_bExitCommModeWhenHeadsetPlugin:%d, m_bHeadsetPlugin:%d, bCommModeEnabled:%d, needMic:%d", m_bExitCommModeWhenHeadsetPlugin, m_bHeadsetPlugin, bCommModeEnabled, needMic);
        }
        MediaSessionMgr::defaultsSetCommModeEnabled(bCommModeEnabled);
        tdav_InitCategory(needMic ? 1 : 0, outputToSpeaker);
    }
    
#elif defined(ANDROID)
    bool bCommModeEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
    int eRecord_CommMode_Order = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE_ORDER, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE_ORDER);
    m_bHeadsetPlugin = JNI_Is_Wired_HeadsetOn() == 0 ? false : true;
    if ((m_bExitCommModeWhenHeadsetPlugin && m_bHeadsetPlugin) || !needMic) {
        bCommModeEnabled = false;
        eRecord_CommMode_Order = COMMMODE_NONE;
        TSK_DEBUG_INFO("Disable CommMode, m_bExitCommModeWhenHeadsetPlugin:%d, m_bHeadsetPlugin:%d, bCommModeEnabled:%d", m_bExitCommModeWhenHeadsetPlugin, m_bHeadsetPlugin, bCommModeEnabled);
    }

    MediaSessionMgr::defaultsSetAndroidFakedRecording(needMic ? false : true);
    // Only when the microphone and the speaker are needed at the same time, we need to enter the communication(VoIP) mode.
    if (needMic) {
        // Communication mode should be set before session init, so we set it by setting the default value.
        MediaSessionMgr::defaultsSetCommModeEnabled(bCommModeEnabled);
        if (eRecord_CommMode_Order == COMMMODE_BEFORE ||
            eRecord_CommMode_Order == COMMMODE_TWO_SIDES) {
            // Set comm mode before start record
            // Server configuration has highest priority.
            if (bCommModeEnabled && !mInCommMode) {
				TSK_DEBUG_INFO ("set Android communication mode to default(by server)");
                start_voice();
                mInCommMode = true;
            }
        }
    } else {
        if (MediaSessionMgr::defaultsGetExternalInputMode()){
            //#ifdef YOUME_FOR_QINIU
            TSK_DEBUG_INFO ("set Android communication mode to ON although QINIU no need mic");
            bool bCommModeEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
            MediaSessionMgr::defaultsSetCommModeEnabled(bCommModeEnabled);
        }else {
            //#else
            TSK_DEBUG_INFO ("set Android communication mode to OFF since no need mic");
            // Force communication mode to off if mic is not needed at the same time.
            MediaSessionMgr::defaultsSetCommModeEnabled(false);
            //#endif
        }
    }
#endif

    m_bCurrentSilentMic = !needMic;

    if (NULL != m_audioMixer) {
        m_audioMixer->stopThread();
        delete m_audioMixer;
        m_audioMixer = NULL;
    }

    m_audioMixer = new YMAudioMixer();
    m_audioMixer->setMixedCallback(audioMixCallback);
    YMAudioFrameInfo audioMixFrameInfo;
    audioMixFrameInfo.bytesPerFrame = 2;
    audioMixFrameInfo.channels = 1;
    audioMixFrameInfo.isBigEndian = false;
    audioMixFrameInfo.isFloat = false;
    audioMixFrameInfo.isNonInterleaved = false;
    audioMixFrameInfo.isSignedInteger = true;
    audioMixFrameInfo.sampleRate = m_inputSampleRate;
    audioMixFrameInfo.timestamp = 0;
    m_audioMixer->setMixOutputInfo(audioMixFrameInfo);
    m_audioMixer->setOutputIntervalMs(20);
    m_audioMixer->startThread();

    m_avSessionExit = true;
    m_avSessionMgrMutex.lock();
    if (NULL != m_avSessionMgr) {
        m_avSessionMgr->UnInit();
        delete m_avSessionMgr;
    }
    m_avSessionMgr = new CAVSessionMgr (mMcuAddr, mMcuRtpPort, mSessionID);
    bool bRet = m_avSessionMgr->Init ();
    m_avSessionMgr->setSessionId(mSessionID);
    m_avSessionMgr->setNeedOpenCamera(m_bCameraIsOpen);
    m_avSessionMgr->setSessionStartFailCallback((void*)onSessionStartFail);
    m_avSessionMgr->setStaticsQosCb((void*)onStaticsQosCb);
    m_avSessionMgr->setVideoEncodeAdjustCb((void*)onVideoEncodeAdjustCb);
    m_avSessionMgr->setVideoDecodeParamCb((void*)onVideoDecodeParamCb);
    m_avSessionMgr->setVideoEventCb((void*)onVideoEventCb);
    m_avSessionMgr->setVideoPreDecodeCallback(mPVideoPreDecodeCallback?(void*)videoPreDecodeCallback:NULL);
    m_avSessionMgr->setVideoPreDecodeNeedDecodeandRender(mPreVideoNeedDecodeandRender);
    switch(NgnApplication::getInstance()->getScreenOrientation()) {
        case portrait:
            m_avSessionMgr->setScreenOrientation(0);
            break;
        case upside_down:
            m_avSessionMgr->setScreenOrientation(180);
            break;
        case landscape_left:
            m_avSessionMgr->setScreenOrientation(90);
            break;
        case landscape_right:
            m_avSessionMgr->setScreenOrientation(270);
            break;
    }
    YouMeVideoMixerAdapter::getInstance()->initMixerAdapter(tmedia_defaults_get_video_hwcodec_enabled());
    m_avSessionMgr->startSession();
    m_avSessionMgrMutex.unlock();
    m_avSessionExit = false;
    if (bRet == true)
    {
        bool bCheckRecordingError = true;
#ifdef ANDROID
        // Only when the microphone and the speaker are needed at the same time, we need to enter the VoIP mode
        // and turn on AEC. For iPhone, the system AEC is enough, so do not set it.
        if (needMic) {
            bool bAecEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AEC, NgnConfigurationEntry::DEFAULT_GENERAL_AEC);
            m_avSessionMgr->setAECEnabled(bAecEnabled);
            
            if (eRecord_CommMode_Order == COMMMODE_AFTER ||
                eRecord_CommMode_Order == COMMMODE_TWO_SIDES) {
                // Set comm mode after start record
                // Server configuration has highest priority.
				TSK_DEBUG_INFO ("set Android communication mode to default(by server)");
                bool bCommModeEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
                if (bCommModeEnabled && !mInCommMode) {
                    start_voice();
                    mInCommMode = true;
                }
            }
        } else {
            if (MediaSessionMgr::defaultsGetExternalInputMode()){
                //#ifdef YOUME_FOR_QINIU
                bool bAecEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::GENERAL_AEC, NgnConfigurationEntry::DEFAULT_GENERAL_AEC);
                m_avSessionMgr->setAECEnabled(bAecEnabled);
                bool bCommModeEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::COMMUNICATION_MODE, NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE);
                if (bCommModeEnabled && !mInCommMode) {
                    start_voice();
                    mInCommMode = true;
                }
            }else {
                //#else
                // force AEC to false
                m_avSessionMgr->setAECEnabled(false);
                if (mInCommMode) {
                    stop_voice();
                    mInCommMode = false;
                }
                //#endif
            }
        }
        // Init audio settings in through the java APIs.
        init_audio_settings(outputToSpeaker);
        if (mNeedMic && bJoinRoom && JNI_startRequestPermissionForApi23()) {
            //bCheckRecordingError = false;
        }
#endif
        m_avSessionMgr->setMicBypassToSpeaker(m_bMicBypassToSpeaker);
        m_avSessionMgr->setBgmBypassToSpeaker(m_bBgmBypassToSpeaker);
        m_avSessionMgr->setReverbEnabled(m_bReverbEnabled);
        m_avSessionMgr->onHeadsetPlugin((int)m_bHeadsetPlugin);
        m_avSessionMgr->setMixAudioTrackVolume((uint8_t)YOUME_RTC_BGM_TO_MIC, (uint8_t)m_nMixToMicVol);
        m_avSessionMgr->setMixAudioTrackVolume((uint8_t)YOUME_RTC_BGM_TO_SPEAKER, (uint8_t)m_nMixToSpeakerVol);
        m_avSessionMgr->setVadCallback(m_bVadCallbackEnabled ? (void*)vadCallback : NULL);
        m_avSessionMgr->setPcmCallback(mPPcmCallback ? (void*)pcmCallback : NULL);
        m_avSessionMgr->setPcmCallbackFlag( mPcmCallbackFlag );
        m_avSessionMgr->setPCMCallbackChannel(m_nPCMCallbackChannel);
        m_avSessionMgr->setPCMCallbackSampleRate(m_nPCMCallbackSampleRate);
        m_avSessionMgr->setTranscriberEnable( mTranscriberEnabled );

        if (m_nRecordingTimeMs >= 0) {
            m_avSessionMgr->setRecordingTimeMs((uint32_t)m_nRecordingTimeMs);
        }
        if (m_nPlayingTimeMs >= 0) {
            m_avSessionMgr->setPlayingTimeMs((uint32_t)m_nPlayingTimeMs);
        }
        
        m_avSessionMgr->setSaveScreenEnabled( m_bSaveScreen );
        
        applyVolume(m_nVolume);
        applySpeakerMute(m_bSpeakerMute);
        applyMicMute(m_bMicMute);
        m_avSessionMgr->setMicLevelCallback(m_nMaxMicLevelCallback > 0 ? (void*)micLevelCallback : NULL);
        m_avSessionMgr->setMaxMicLevelCallback(m_nMaxMicLevelCallback);
        m_avSessionMgr->setFarendVoiceLevelCallback(m_nMaxFarendVoiceLevel > 0 ? (void*)farendVoiceLevelCallback : NULL);
        m_avSessionMgr->setMaxFarendVoiceLevel(m_nMaxFarendVoiceLevel);
        // startPacketStatReportThread();
        startAVQosStatReportThread();
        setVideoRuntimeEventCb();
        setStaticsQosCb();

#if defined(__APPLE__)
        uint32_t consumerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_CONSUMER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_CONSUMER_PEROID_MS);
        uint32_t producerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_PRODUCER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_PRODUCER_PEROID_MS);
        if (consumerPeriod > 0 || producerPeriod > 0) {
            AVRuntimeMonitor::getInstance()->Init();
        }
        if (consumerPeriod > 0) {
            AVRuntimeMonitor::getInstance()->StartConsumerMoniterThread();
        }
        if (producerPeriod > 0) {
            AVRuntimeMonitor::getInstance()->StartProducerMoniterThread();
        }
        #if TARGET_OS_IPHONE
        m_iosRetrans = Config_GetInt("IOS_REPLAYKIT_RETRANS", 1);  // 是否打开ios重发功能，0:关闭; 1:打开
        TSK_DEBUG_INFO ("set ios replay kit mode:%u", m_iosRetrans);
        bool enableCache = (m_iosRetrans == 1) ? true : false;
        ICameraManager::getInstance()->setCacheLastPixelEnable(enableCache);

        // 仅打开重发功能时创建重发包线程
        if (1 == m_iosRetrans) {
            startIosVideoThread();
        }
        
        #endif
#endif

        //设置后才能收到远端数据回调
        if(mPVideoCallback) {
            setVideoRenderCbEnable(true);
        }
        
        if (bCheckRecordingError && needMic) {
            checkRecoringError();
        }
        
        TSK_DEBUG_INFO("== startAvSessionManager OK");
        return YOUME_SUCCESS;
    }
    else
    {
#if defined(ANDROID)
        if (mInCommMode) {
            stop_voice ();
            mInCommMode = false;
        }
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
        tdav_RestoreCategory();
#endif //TARGET_OS_IPHONE
#endif
        
        TSK_DEBUG_INFO("== startAvSessionManager failed");
        return YOUME_ERROR_START_FAILED;
    }
}

YouMeErrorCode CYouMeVoiceEngine::restoreEngine() {
    TSK_DEBUG_INFO("$$ restoreEngine");
    
    if(mPVideoCallback) {
        setVideoRenderCbEnable(true);
    }
    //setVideoRenderCbEnable(true);
    TSK_DEBUG_INFO("== restoreEngine OK");
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::triggerCheckRecordingError()
{
    // Sleep for a while and then send a message to the main loop
    m_workerThreadCondWait.Reset();
    if (youmecommon::WaitResult_Timeout == m_workerThreadCondWait.WaitTime(200)) {
        // If timeout(not interrupted by the user), send a message to check for recording error.
        if (m_pMainMsgLoop) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiCheckRecordingError);
            if (pMsg) {
                m_pMainMsgLoop->SendMessage(pMsg);
            }
        }
    }
}

//
// 检查录音模块是否正常工作（比如：是否有录音权限，是否有音频数据输出）
// 因为确定录音模块是否正常工作在某些情况下需要做超时等待，所以作为一个独立事件通知，应用可以选择是否处理该事件
//
void CYouMeVoiceEngine::checkRecoringError()
{
    YouMeErrorCode recErrCode = YOUME_SUCCESS;
    int32_t        recErrExtra = 0;
    bool           bGetRecErrOK = false;
    
    if (m_avSessionMgr) {
        bGetRecErrOK = m_avSessionMgr->getRecordingError((int32_t*)&recErrCode, &recErrExtra);
        if (bGetRecErrOK && (YOUME_ERROR_REC_PERMISSION_UNDEFINED == recErrCode)) {
            // 对iOS, 当等待用户确认录音权限时，返回的录音权限是 undefine，在这种情况下，等待一段时间后继续检查，直到用户做出选择。
            if (m_pWorkerMsgLoop) {
                CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgWorkerCheckRecordingError);
                if (pMsg) {
                    m_pWorkerMsgLoop->SendMessage(pMsg);
                }
            }
        } else if (bGetRecErrOK && (YOUME_SUCCESS != recErrCode)) {
            // 向上层通知录音错误码
            TSK_DEBUG_INFO ("Call back recording error to app, errCode:%d, extra:%d", recErrCode, recErrExtra);
            sendCbMsgCallEvent(YOUME_EVENT_REC_PERMISSION_STATUS, recErrCode, "", mStrUserID);
        } else {
            TSK_DEBUG_INFO ("Call back recording success to app, bGetRecErrOK:%d, recErrCode:%d", bGetRecErrOK, recErrCode);
            sendCbMsgCallEvent(YOUME_EVENT_REC_PERMISSION_STATUS, recErrCode, "", mStrUserID);
        }
    }
}

void CYouMeVoiceEngine::doLeaveConferenceMulti(const string &strRoomID)
{
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, YOUME_ERROR_CHANNEL_NOT_EXIST, strRoomID, mStrUserID);
        TSK_DEBUG_INFO ("== doLeaveConferenceMulti, room not exist:%s", strRoomID.c_str());
        return;
    }
    
    if (m_pRoomMgr->getRoomCount() == 1) {
        doLeaveConferenceAllProxy();
        sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, YOUME_SUCCESS, strRoomID, mStrUserID);
    } else {
        // If it's going to leave the "speakTo" room, we should mute the microphone it's on.
        // Because after leaving, the "speakTo" room will be undefined, and if the microphone
        // is on, the user may speak to an unintended room.
        if ((m_pRoomMgr->getSpeakToRoomId() == strRoomID) && !m_bMicMute) {
            m_bMicMute = true; // 默认进房间关闭麦克风
            applyMicMute(m_bMicMute);
        }
        
        YouMeErrorCode errCode = m_loginService.LeaveChannel(mSessionID, roomInfo.idFull);
        if (YOUME_SUCCESS != errCode) {
            sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, errCode, strRoomID, mStrUserID);
        } else {
            m_pRoomMgr->setRoomState(strRoomID, CRoomManager::ROOM_STATE_DISCONNECTING);
        }
    }
}

void CYouMeVoiceEngine::doLeaveConferenceMultiDone(const string &strRoomID, NgnLoginServiceCallback::RoomEventResult result)
{
    TSK_DEBUG_INFO ("$$ doLeaveConferenceMultiDone, roomID:%s", strRoomID.c_str());
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
        TSK_DEBUG_ERROR("== doLeaveConferenceMultiDone failed to get room info");
        return;
    }
    
    if (NgnLoginServiceCallback::ROOM_EVENT_SUCCESS == result) {
        m_pRoomMgr->removeRoom(strRoomID);
        sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, YOUME_SUCCESS, strRoomID, mStrUserID);
    } else {
        m_pRoomMgr->setRoomState(strRoomID, CRoomManager::ROOM_STATE_CONNECTED);
        sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, YOUME_ERROR_NETWORK_ERROR, strRoomID, mStrUserID);
    }
    
    TSK_DEBUG_INFO ("== doLeaveConferenceMultiDone");
    
}

void CYouMeVoiceEngine::doBeKickFromChannel( const string& strRoomID, const string& param  ){
    TSK_DEBUG_INFO ("$$ doBeKickFromChannel, roomID:%s", strRoomID.c_str());
    
    {
        /* 被踢出房间 数据上报 */
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportChannel beKick;
        beKick.operate_type = REPORT_CHANNEL_KICKOUT;
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        beKick.roomid = strShortRoomID;
        beKick.sessionid = mSessionID;
        
        auto curRoomInfo = m_pRoomMgr->findRoomInfo(strRoomID);
        if ( curRoomInfo != nullptr) {
            beKick.rtc_usetime = (tsk_time_now() - curRoomInfo->joinRoomTime );
        } else {
            beKick.rtc_usetime = 0 ;
        }
        
        
        beKick.on_disconnect = 0;
        beKick.sdk_version = SDK_NUMBER;
        if( MediaSessionMgr::defaultsGetExternalInputMode() ){
            if( m_externalInputStartTime != 0 ){
                beKick.video_duration = m_externalVideoDuration + (tsk_gettimeofday_ms() - m_externalInputStartTime);
            }
            else{
                beKick.video_duration = m_externalVideoDuration;
            }
        }
        else{
            beKick.video_duration = (unsigned int)ICameraManager::getInstance()->getDuration() + video_duration;
        }
        beKick.platform = NgnApplication::getInstance()->getPlatform();
        beKick.canal_id = NgnApplication::getInstance()->getCanalID();
        beKick.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(beKick);
    }
    
    
    if( m_pRoomMgr->getRoomCount() == 1 )
    {
        doLeaveConferenceAllProxy( true );
    }
    else{
        if ((m_pRoomMgr->getSpeakToRoomId() == strRoomID) && !m_bMicMute) {
            m_bMicMute = true; // 默认进房间关闭麦克风
            applyMicMute(m_bMicMute);
        }
        
        m_pRoomMgr->removeRoom(strRoomID);
    }
    
    sendCbMsgCallEvent(YOUME_EVENT_KICK_NOTIFY, YOUME_SUCCESS, strRoomID,  param );
    
    
    TSK_DEBUG_INFO ("== doBeKickFromChannel");
}


void CYouMeVoiceEngine::doSetOutputToSpeaker(bool bOutputToSpeaker){
    TSK_DEBUG_INFO ("$$ doSetOutputToSpeaker");
    YouMeErrorCode ret = YOUME_SUCCESS;
    if (m_bOutputToSpeaker == bOutputToSpeaker) {
        TSK_DEBUG_INFO("== doSetOutputToSpeaker set the same value:%d", bOutputToSpeaker);
        sendCbMsgCallEvent(YOUME_EVENT_SWITCH_OUTPUT, YOUME_SUCCESS, "", "");
        return;
    }
    if ( m_avSessionMgr) {
        m_bOutputToSpeaker = bOutputToSpeaker;
#ifdef ANDROID
        init_audio_settings(m_bOutputToSpeaker?1:0);
#elif __APPLE__
#if TARGET_OS_IPHONE
        ret = (tdav_apple_setOutputoSpeaker(m_bOutputToSpeaker?1:0)) == 0 ? YOUME_SUCCESS : YOUME_ERROR_WRONG_STATE;
#else
        ret = YOUME_ERROR_API_NOT_SUPPORTED;
#endif
#else
        ret = YOUME_ERROR_API_NOT_SUPPORTED;
#endif
        switch (ret) {
            case YOUME_SUCCESS:
                TSK_DEBUG_INFO("== doSetOutputToSpeaker success!!");
                sendCbMsgCallEvent(YOUME_EVENT_SWITCH_OUTPUT, YOUME_SUCCESS, "", "");
                break;
            case YOUME_ERROR_API_NOT_SUPPORTED:
                TSK_DEBUG_INFO("== doSetOutputToSpeaker failed, not supported");
                sendCbMsgCallEvent(YOUME_EVENT_SWITCH_OUTPUT, YOUME_ERROR_API_NOT_SUPPORTED, "", "");
                break;
            case YOUME_ERROR_WRONG_STATE:
                TSK_DEBUG_INFO("== doSetOutputToSpeaker failed, wrong state");
                sendCbMsgCallEvent(YOUME_EVENT_SWITCH_OUTPUT, YOUME_ERROR_WRONG_STATE, "", "");
                break;
            default:
                break;
        }
    } else{
        TSK_DEBUG_INFO("== doSetOutputToSpeaker failed, wrong state");
        sendCbMsgCallEvent(YOUME_EVENT_SWITCH_OUTPUT, YOUME_ERROR_WRONG_STATE, "", "");
    }
}

void CYouMeVoiceEngine::doSetAutoSend( bool bAutoSend ){
    mAutoSendStatus = bAutoSend;
    
    if( m_avSessionMgr && mAutoSendStatus == true ){
        if( NeedMic() ){
            sendEventToServer(YouMeProtocol::MIC_STATUS, !isMicrophoneMute(), mStrUserID);
        }
        
        sendEventToServer(YouMeProtocol::SPEAKER_STATUS, !getSpeakerMute(), mStrUserID);
    }
}

void CYouMeVoiceEngine::doSetRecordDevice(  std::string& strDeviceUid )
{
    TSK_DEBUG_INFO ("$$ doSetRecordDevice, device:%s", strDeviceUid.c_str() );
    
    if( m_strMicDeviceChoosed.compare( strDeviceUid) == 0 )
        return ;
    
    m_strMicDeviceChoosed = strDeviceUid;

#ifdef MAC_OS
    tdav_mac_set_choose_device( strDeviceUid.c_str() );
#elif WIN32
	//todo
#endif
    
    restartAVSessionAudio();
    

    
    TSK_DEBUG_INFO ("$$ doSetRecordDevice");
}


void CYouMeVoiceEngine::doLeaveConferenceAll(bool needCallback)
{
    TSK_DEBUG_INFO ("$$ doLeaveConferenceAll");
    
    {
        //离开房间的时候，要把没上报的视频渲染分辨率上报了
        if( !m_mapCurOtherUserID.empty() && !m_mapOtherResolution.empty())
        {
            uint64_t curTimeMs = tsk_gettimeofday_ms();
            unsigned int duration = curTimeMs - m_LastCheckTimeMs;
            m_LastCheckTimeMs = curTimeMs;
            
            unsigned width, height;
            tmedia_defaults_get_video_size(&width, &height);
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportRecvVideoInfo videoSelfLeave;
            videoSelfLeave.roomid = mRoomID;
            videoSelfLeave.other_userid = "";
            videoSelfLeave.width  = width;
            videoSelfLeave.height = height;
            videoSelfLeave.video_render_duration = duration; //tsk_gettimeofday_ms();
            videoSelfLeave.report_type = REPORT_VIDEO_SELF_LEAVE_ROOM;
            videoSelfLeave.sdk_version = SDK_NUMBER;
            videoSelfLeave.platform    = NgnApplication::getInstance()->getPlatform();
            videoSelfLeave.num_camera_stream = m_NumOfCamera;
            videoSelfLeave.sum_resolution = calcSumResolution("");
            videoSelfLeave.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoSelfLeave);
        }
        
        std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
        m_NumOfCamera = 0;
        m_mapCurOtherUserID.clear();
        m_mapOtherResolution.clear();
        m_UserIdStartRenderMap.clear();
    }

	#if FFMPEG_SUPPORT
		YMVideoRecorderManager::getInstance()->stopRecordAll();
	#endif
    #ifdef TRANSSCRIBER
	YMTranscriberManager::getInstance()->Stop();
    #endif
	mTranscriberEnabled = false;

    if (m_bStartshare != false) {
        stopShareStream();
        m_bStartshare = false;
    }
    
    if (m_bSaveScreen != false) {
        stopSaveScreen();
        m_bSaveScreen = false;
    }

    if (m_pRoomMgr->getRoomCount() > 0) {
        doLeaveConferenceAllProxy();
    }
    if (needCallback) {
        sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ALL, YOUME_SUCCESS, "", mStrUserID);
    }
    m_isInRoom = false;
    mIsWaitingForLeaveAll = false; // Indicate any one who is waiting for LeaveAll that it's done.
    CSDKValidate::GetInstance()->Reset();
    m_loginService.Reset();
    m_workerThreadCondWait.Reset();
    m_reconnctWait.Reset();
    
    YouMeVideoMixerAdapter::getInstance()->leavedRoom();
    this->m_bInputVideoIsOpen = false;
    this->m_bInputShareIsOpen = false;
    TSK_DEBUG_INFO ("== doLeaveConferenceAll");
}

void CYouMeVoiceEngine::doLeaveConferenceAllProxy( bool bKick )
{
    TSK_DEBUG_INFO ("$$ doLeaveConferenceAllProxy");
    
    //
    // stop everything that may be in progress
    //
    CSDKValidate::GetInstance()->Abort();
    TSK_DEBUG_INFO ("stop SDKValidate OK");
    m_loginService.StopLogin();
    TSK_DEBUG_INFO ("stop m_loginService OK");
    doStopBackgroundMusic();
    
    if( m_bCameraIsOpen ){
        ICameraManager::getInstance()->stopCapture();
        m_bCameraIsOpen = false;
    }
    
    if(m_avSessionMgr) {
        tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
        video_duration = trtp_manager_get_video_duration(TDAV_SESSION_AV(session)->rtp_manager);

        // stop p2p check timer
        trtp_manager_stop_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
    }
    
    AVStatistic::getInstance()->StopThread();
    stopAvSessionManager ();
    
    CRoomManager::RoomInfo_t firstRoomInfo;
    if (!m_pRoomMgr->getFirstRoomInfo(firstRoomInfo)) {
        TSK_DEBUG_ERROR("Cannot find first room info");
        firstRoomInfo.idFull = "";
    }
    
    //不是被踢才需要发消息通知
    if( !bKick )
    {
        // Send message to the server to notify leaving the room
        do {
            youmertc::CSyncTCP m_client;
            TSK_DEBUG_INFO("Leaving for mcuAddr:%s, port:%u, sessionID:%d", mMcuAddr.c_str(), mMcuSignalPort, mSessionID);
            if (!m_client.Init(mMcuAddr, mMcuSignalPort, 2)) //2秒收发超时
            {
                TSK_DEBUG_ERROR ("init login socket fail");
                break;
            }
            TSK_DEBUG_INFO ("Connect the login server for leave with 2 sec timeout");
            if(!m_client.Connect(2)){
                TSK_DEBUG_ERROR ("connect login server fail");
                break;
            }
            
            YouMeProtocol::YouMeVoice_Command_LeaveConference leaveProtocol;
            leaveProtocol.set_allocated_head (CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_LeaveConference));
            leaveProtocol.set_roomid (firstRoomInfo.idFull);
            leaveProtocol.set_sessionid (mSessionID);
            
            std::string strSerialData;
            leaveProtocol.SerializeToString (&strSerialData);
            
            TSK_DEBUG_INFO ("Sending leaveProtocol...");
            int sendlen = m_client.SendData (strSerialData.c_str (), strSerialData.length ());
            if(sendlen != strSerialData.length ()){
                TSK_DEBUG_WARN ("Sending leaveProtocol failed");
                break;
            }
            TSK_DEBUG_INFO ("Sending leaveProtocol OK");
        } while(0);
    }
    
    CRoomManager::RoomInfo_t curRoomInfo;
    std::string roomIdShort = "";
    if (m_pRoomMgr->getFirstRoomInfo(curRoomInfo)) {
        do {
            removeAppKeyFromRoomId(curRoomInfo.idFull, roomIdShort);
            ReportLeaveConferenceFun(roomIdShort, curRoomInfo.joinRoomTime);
            if (!m_pRoomMgr->getNextRoomInfo(curRoomInfo)) {
                break;
            }
        } while (true);
    } else {
        // If leaveChannelAll is called while a joinConference is in progress, this may happen,
        // because joinConference may be interrupted and the roomInfo has been removed.
        ReportLeaveConferenceFun(roomIdShort, tsk_time_now());
    }
    
    resetConferenceProperties();
    
    m_pRoomMgr->removeAllRooms();
    CVideoChannelManager::getInstance()->clear();
    mIsReconnecting = false;
    
    mRole = YOUME_USER_NONE;
    
    TSK_DEBUG_INFO ("== doLeaveConferenceAllProxy");
}

void CYouMeVoiceEngine::doQueryHttpInfo( int requestID,  const std::string& strCommand, const std::string& strQueryBody ){
    TSK_DEBUG_INFO ("$$ doQueryHttpInfo, %d", requestID );
    QueryHttpInfo info;
    info.requestID = requestID;
    info.strCommand = strCommand;
    info.strQueryBody = strQueryBody;
    
    std::lock_guard<std::mutex> lock(m_queryHttpInfoLock);
    m_queryHttpList.push_back( info );
    m_httpSemaphore.Increment();
    TSK_DEBUG_INFO ("$$ doQueryHttpInfo end");
}

void CYouMeVoiceEngine::doGetChannelUserList( const std::string& strRoomID, int maxCount, bool bNotifyMemChange ){
    TSK_DEBUG_INFO ("$$ doGetChannelUserList" );
    
    if (m_avSessionMgr) {
        YouMeProtocol::YouMeVoice_Command_ChannelUserList_Request  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_GET_USER_LIST));
        getReq.set_sessionid( mSessionID);
        getReq.set_channel_id( strRoomID );
        getReq.set_start_index( 0 );
        getReq.set_req_count( maxCount );
        getReq.set_notify_flag( bNotifyMemChange?1:2 );
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_GET_USER_LIST, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO ("$$ doGetChannelUserList end");
}

void CYouMeVoiceEngine::resetConferenceProperties()
{
    mStrUserID = "";
    mRoomID = "";
    mNeedMic = false;
    mInviteMicOK = false;
    mAutoSendStatus = false;
    mSessionID = -1;
    mMcuAddr = "";
    mMcuRtpPort = -1;
    mMcuSignalPort = -1;
    mBusinessID = 0;

    {
        std::lock_guard< std::mutex > lock(m_mutexSessionMap);
        m_SessionUserIdMap.clear();
    }
   
    {
        std::lock_guard< std::mutex > lock(m_mutexVideoInfoMap);
        m_UserVideoInfoMap.clear();
    }

    {
        std::lock_guard< std::mutex > lock(m_mutexVideoInfoCache);
        m_UserVideoInfoList.clear();
    }
    
    m_nRecordingTimeMs = -1;
    m_nPlayingTimeMs = -1;
    CSDKValidate::GetInstance()->Reset();
    m_loginService.Reset();
    m_workerThreadCondWait.Reset();
    m_reconnctWait.Reset();
}

void CYouMeVoiceEngine::ReportLeaveConferenceFun(std::string& strRoomID, uint64_t joinRoomTime)
{
 
    
    ReportQuitData::getInstance()->m_leave_count++;
    {
        // 新的数据上报
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportChannel leave;
        leave.operate_type = REPORT_CHANNEL_LEAVE;
        std::string strShortRoomID;
        CYouMeVoiceEngine::getInstance()->removeAppKeyFromRoomId( strRoomID, strShortRoomID );
        leave.roomid = strShortRoomID;
        leave.rtc_usetime = (tsk_time_now() - joinRoomTime);
        leave.sdk_version = SDK_NUMBER;
        if( MediaSessionMgr::defaultsGetExternalInputMode() ){
            if( m_externalInputStartTime != 0 ){
                leave.video_duration = m_externalVideoDuration + (tsk_gettimeofday_ms() - m_externalInputStartTime);
            }
            else{
                leave.video_duration = m_externalVideoDuration;
            }
        }
        else{
            leave.video_duration = (unsigned int)ICameraManager::getInstance()->getDuration() + video_duration;
        }
        
        leave.platform = NgnApplication::getInstance()->getPlatform();
        leave.canal_id = NgnApplication::getInstance()->getCanalID();
        leave.package_name = NgnApplication::getInstance()->getPackageName();
        leave.sessionid = mSessionID;
        report->report(leave);
    }
}

void CYouMeVoiceEngine::stopAvSessionManager(bool isReconnect)
{
    TSK_DEBUG_INFO ("$$ stopAvSessionManager");
    
#if defined(__APPLE__)
    uint32_t consumerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_CONSUMER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_CONSUMER_PEROID_MS);
    uint32_t producerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_PRODUCER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_PRODUCER_PEROID_MS);
    if (consumerPeriod > 0) {
        AVRuntimeMonitor::getInstance()->StopConsumerMoniterThread();
    }
    if (producerPeriod > 0) {
        AVRuntimeMonitor::getInstance()->StopProducerMoniterThread();
    }
    if (consumerPeriod > 0 || producerPeriod > 0) {
        AVRuntimeMonitor::getInstance()->UnInit();
    }

    #if TARGET_OS_IPHONE
    stopIosVideoThread();
    #endif
#endif

    stopAVQosStatReportThread();
    // stopPacketStatReportThread();
    
    if (NULL == m_avSessionMgr)
    {
        TSK_DEBUG_ERROR ("== m_avSessionMgr is NULL!");
    }else {
        m_avSessionExit = true;
        m_avSessionMgrMutex.lock();
        if (m_avSessionMgr) {
            m_avSessionMgr->UnInit ();
            delete m_avSessionMgr;
            m_avSessionMgr = NULL;
        }
        m_avSessionMgrMutex.unlock();
        TSK_DEBUG_INFO ("delete avSessionMgr OK");
    }
    
    if (NULL == m_audioMixer)
    {
        TSK_DEBUG_ERROR ("== m_audioMixer is NULL!");
    }else {
        if (m_audioMixer) {
            m_audioMixer->stopThread();
            delete m_audioMixer;
            m_audioMixer = NULL;
        }
        TSK_DEBUG_INFO ("delete avSessionMgr OK");
    }
    

#if defined(ANDROID)
    JNI_stopRequestPermissionForApi23();
    if (mInCommMode) {
        stop_voice();
        mInCommMode = false;
    }
#elif defined(__APPLE__)
    if(!isReconnect) {
#if TARGET_OS_IPHONE
        tdav_RestoreCategory();
#endif //TARGET_OS_IPHONE
    }
#endif
    m_bCurrentSilentMic = false;
    TSK_DEBUG_INFO("== stopAvSessionManager OK");
}

bool CYouMeVoiceEngine::releaseMicSync(){
    TSK_DEBUG_INFO("$$ releaseMicSync");
    if (isStateInitialized() && NULL != m_avSessionMgr){
        preReleaseMicStatus = isMicrophoneMute();
#if defined(ANDROID)
        JNI_Pause_Audio_Record();
#else
        applyMicMute(true,false);
#endif
    }
    TSK_DEBUG_INFO ("== releaseMicSync OK");
    return true;
}

bool CYouMeVoiceEngine::resumeMicSync(){
    TSK_DEBUG_INFO("$$ resumeMicSync");
    if (isStateInitialized() && NULL != m_avSessionMgr){
#if defined(ANDROID)
        JNI_Resume_Audio_Record();
#else
        TSK_DEBUG_INFO ("== resumeMicSync to mute:%d",preReleaseMicStatus);
        applyMicMute(preReleaseMicStatus,false);
#endif
    }
    TSK_DEBUG_INFO ("== resumeMicSync OK");
    return true;
}

int  CYouMeVoiceEngine::getCameraCount()
{
    TSK_DEBUG_INFO ("@@ getCameraCount");
#if WIN32 || TARGET_OS_OSX
    return ICameraManager::getInstance()->getCameraCount();
#endif
    TSK_DEBUG_INFO ("== getCameraCount api only support WIN32 or OSX");
    return 0;
}

std::string CYouMeVoiceEngine::getCameraName(int cameraId)
{
    TSK_DEBUG_INFO ("@@ getCameraName for id:%d", cameraId);
#if WIN32 || TARGET_OS_OSX
    return ICameraManager::getInstance()->getCameraName(cameraId);
#endif
    TSK_DEBUG_INFO ("== getCameraName api only support WIN32 or OSX");
    return "";
}

YouMeErrorCode CYouMeVoiceEngine::setOpenCameraId(int cameraId)
{
    TSK_DEBUG_INFO ("@@ setOpenCameraId for id:%d", cameraId);
#if WIN32 || TARGET_OS_OSX
    return ICameraManager::getInstance()->setOpenCameraId(cameraId);
#endif
    TSK_DEBUG_INFO ("== setOpenCameraId api only support WIN32 or OSX");
    return YOUME_ERROR_API_NOT_SUPPORTED;
}


int CYouMeVoiceEngine::getRecordDeviceCount()
{
    TSK_DEBUG_INFO("@@ getRecordDeviceCount");
#if TARGET_OS_OSX
    AudioDeviceMgr_OSX::getInstance()->dumpInputDevicesInfo();
    return AudioDeviceMgr_OSX::getInstance()->getInputDeviceCount();
#elif WIN32
	return 	AudioDeviceMgr_Win::getInstance()->getAudioInputDeviceCount();
#endif
    TSK_DEBUG_INFO ("== getRecordDeviceCount api only support WIN32 or OSX");
    return 0;
}

bool CYouMeVoiceEngine::getRecordDeviceInfo(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceUId[MAX_DEVICE_ID_LENGTH])
{
    TSK_DEBUG_INFO("@@ getRecordDeviceInfo");
#if TARGET_OS_OSX
    return AudioDeviceMgr_OSX::getInstance()->getInputDevice( index, deviceName, deviceUId );
#elif WIN32
	return 	AudioDeviceMgr_Win::getInstance()->getAudioInputDevice( index, deviceName, deviceUId );
#endif
    TSK_DEBUG_INFO ("== getRecordDeviceInfo api only support WIN32 or OSX");
    return 0;
    
}

YouMeErrorCode CYouMeVoiceEngine::setRecordDevice( const char* deviceUId)
{
    TSK_DEBUG_INFO("@@ setRecordDevice,%s", deviceUId);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
#if TARGET_OS_OSX
    //如果设置了空，表示清空设置，系统内部使用默认麦克风
    std::string deviceuid = "";
    if( deviceUId != NULL && strlen( deviceUId ) != 0 )
    {
        //如果设置的设备uid不存在，就啥都不做
        int device = AudioDeviceMgr_OSX::getInstance()->getAudioDeviceID( deviceUId );
        if( device == 0 )
        {
            TSK_DEBUG_INFO("== setRecordDevice device not valid");
            return YOUME_ERROR_DEVICE_NOT_VALID;
        }
        else if( !( AudioDeviceMgr_OSX::getInstance()->isInputDevice( device ) ) )
        {
            TSK_DEBUG_INFO("== setRecordDevice device not input device");
            return YOUME_ERROR_DEVICE_NOT_VALID;
        }
        else{
            deviceuid = deviceUId;
        }
    }

	if (m_pMainMsgLoop) {
		CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetRecordDevice);
		if (pMsg) {
			*(pMsg->m_param.apiSetDevice.deviceUid) = deviceUId;
			m_pMainMsgLoop->SendMessage(pMsg);
			TSK_DEBUG_INFO("== setRecordDevice");
			return YOUME_SUCCESS;
		}
	}

	TSK_DEBUG_INFO("== setRecordDevice delayed");
	return YOUME_ERROR_MEMORY_OUT;
#elif WIN32

	std::string deviceuid = "";
	if (deviceUId != nullptr && strlen(deviceUId) != 0)
	{
		int deviceID = AudioDeviceMgr_Win::getInstance()->getDeviceIDByUID(deviceUId);
		if (deviceID == -1)
		{
			TSK_DEBUG_INFO("== setRecordDevice device not valid");
			return YOUME_ERROR_DEVICE_NOT_VALID;
		}
		else {
			deviceuid = deviceUId;
		}
	}
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetRecordDevice);
        if (pMsg) {
            *(pMsg->m_param.apiSetDevice.deviceUid) = deviceUId;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setRecordDevice");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setRecordDevice delayed");
    return YOUME_ERROR_MEMORY_OUT;
#endif
    TSK_DEBUG_INFO ("== setRecordDevice api only support WIN32 or OSX");
    return YOUME_ERROR_API_NOT_SUPPORTED;
}

void CYouMeVoiceEngine::setMixVideoSize(int width, int height){
    YouMeVideoMixerAdapter::getInstance()->setMixVideoSize(width, height);
}
void CYouMeVoiceEngine::addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height){
    YouMeVideoMixerAdapter::getInstance()->addMixOverlayVideo(userId, x, y, z, width, height);
}
void CYouMeVoiceEngine::removeMixOverlayVideo(std::string userId){
    YouMeVideoMixerAdapter::getInstance()->removeMixOverlayVideo(userId);
}
void CYouMeVoiceEngine::removeAllOverlayVideo(){
    YouMeVideoMixerAdapter::getInstance()->removeAllOverlayVideo();
}

YouMeErrorCode CYouMeVoiceEngine::setExternalFilterEnabled(bool enabled){
    if(YouMeVideoMixerAdapter::getInstance()->setExternalFilterEnabled(enabled))
        return YOUME_SUCCESS;
    return YOUME_ERROR_UNKNOWN;
}

tmedia_session_t* CYouMeVoiceEngine::getMediaSession(tmedia_type_t type) {
    if (!m_avSessionMgr) {
        TSK_DEBUG_ERROR("== m_avSessionMgr is NULL!");
        return NULL;
    }
    return m_avSessionMgr->getMediaSession(type);
}

void CYouMeVoiceEngine::restartAVSessionAudio( )
{
    //没在房间里就不要搞这些了
    if( !m_isInRoom )
    {
        return ;
    }
    
    TSK_DEBUG_INFO ("$$ restartAVSessionAudio " );
#if defined(__APPLE__)
    bool needMic = NeedMic();
    if (m_bReleaseMicWhenMute && needMic) {
        needMic = m_bMicMute ? false : true;
    }
    tdav_InitCategory(needMic ? 1 : 0, m_bOutputToSpeaker);
#endif
    
#ifdef MAC_OS
    //重置选定的mic的设备ID,一个设备的uid应该不会变，但audiodeviceid会变，所以记录uid比较好
    {
        int choosedMic = AudioDeviceMgr_OSX::getInstance()->getAudioDeviceID( m_strMicDeviceChoosed.c_str() );
        tmedia_set_record_device( choosedMic );
        tdav_mac_update_use_audio_device();
    }
#elif WIN32
	{
		int chooseMic = AudioDeviceMgr_Win::getInstance()->getDeviceIDByUID( m_strMicDeviceChoosed.c_str());
		tmedia_set_record_device(chooseMic);
	}
#endif
    
    pauseBackgroundMusic();
    std::lock_guard<std::recursive_mutex> sessionMgrLock(m_avSessionMgrMutex);

    bool ret = true;
    if( m_avSessionMgr )
    {
        ret = m_avSessionMgr->Restart( tmedia_audio );
    }
    
    if( ret )
    {
        resumeBackgroundMusic();
    }
}

YouMeErrorCode CYouMeVoiceEngine::loginToMcu(std::string &strRoomID, YouMeUserRole_t eUserRole, bool bRelogin, bool bVideoAutoRecv)
{
    if (mCanRefreshRedirect && (mLastServerRegionNameMap != mServerRegionNameMap)) {
        YouMeErrorCode errCode = CSDKValidate::GetInstance ()->GetRedirectList(mServerRegionNameMap, mRedirectServerInfoVec);
        if (YOUME_SUCCESS == errCode) {
            mLastServerRegionNameMap = mServerRegionNameMap;
        }
    }
    
    if ((SERVER_MODE_FIXED_IP_REDIRECT == g_serverMode) || (SERVER_MODE_FIXED_IP_MCU == g_serverMode)) {
        RedirectServerInfo_t redirectServerInfo;
        redirectServerInfo.host = g_serverIp;
        redirectServerInfo.port = g_serverPort;
        mRedirectServerInfoVec.clear();
        mRedirectServerInfoVec.push_back(redirectServerInfo);
    } else if (mRedirectServerInfoVec.size() <= 0) {
        // Using the redirect server IP/port returned by the legacy way.
        RedirectServerInfo_t redirectServerInfo;
        // Get the redirect server ip/port. These are returned from the validation server.
        redirectServerInfo.host = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_REDIRECT_ADD, NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_ADD);
        redirectServerInfo.port = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_REDIRECT_PORT, NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_PORT);
        mRedirectServerInfoVec.push_back(redirectServerInfo);
        TSK_DEBUG_INFO("No redirect server list, use the legacy redirect server %s:%d", redirectServerInfo.host.c_str(), redirectServerInfo.port);
    }
    
    string strMcuAddr;
    int iMcuRtpPort = 0;
    int iSessionID = 0;
    int iMcuSignalPort = 0;
    uint64_t uBusinessID = 0;
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    
    for (int i = 0; i < mRedirectServerInfoVec.size(); i++) {
        if (mRedirectServerInfoVec[i].host.empty() || (mRedirectServerInfoVec[i].port <= 0)) {
            continue;
        }
        
        TSK_DEBUG_INFO("Trying to login with redirect server %s:%d", mRedirectServerInfoVec[i].host.c_str(),  mRedirectServerInfoVec[i].port);
        if (bRelogin) {
            errCode = m_loginService.ReLoginServerSync (mStrUserID, eUserRole, mRedirectServerInfoVec[i].host, mRedirectServerInfoVec[i].port, strRoomID,
                                                        this, strMcuAddr, iMcuRtpPort, iSessionID, iMcuSignalPort, uBusinessID, bVideoAutoRecv);
        } else {
            errCode = m_loginService.LoginServerSync (mStrUserID, eUserRole, mRedirectServerInfoVec[i].host, mRedirectServerInfoVec[i].port, strRoomID,
                                                      this, strMcuAddr, iMcuRtpPort, iSessionID, iMcuSignalPort, uBusinessID, bVideoAutoRecv);
        }
        if (YOUME_SUCCESS == errCode) {
            break;
        }
    }
    
    if (YOUME_SUCCESS == errCode) {
        mSessionID = iSessionID;
        mMcuAddr = strMcuAddr;
        mMcuSignalPort = iMcuSignalPort;
        mMcuRtpPort = iMcuRtpPort;
        mBusinessID = uBusinessID;
        if(bRelogin && mSessionID == iSessionID && mMcuAddr == strMcuAddr
           && mMcuSignalPort == iMcuSignalPort && mMcuRtpPort == iMcuRtpPort )
        {
            //重连时MCU完全没有变化，不需要重启UDP模块
            bMcuChanged = false;
        }else{
            bMcuChanged = true;
        }
        AVStatistic::getInstance()->NotifyUserName( mSessionID, mStrUserID );
        #if FFMPEG_SUPPORT
        YMVideoRecorderManager::getInstance()->NotifyUserName( mSessionID, mStrUserID, true  );
        #endif
    }
    
    return errCode;
}

void CYouMeVoiceEngine::doReconnect()
{
    TSK_DEBUG_INFO ("$$ Enter doReconnect");
    
    if (m_pRoomMgr->getRoomCount() <= 0) {
        mIsReconnecting = false;
        TSK_DEBUG_INFO ("$$ Not in Room,abort doReconnect");
        return;
    }
    
    // We should connect to the speakToRoomId first, because on reconnect, the first connected
    // room will get the speaking right.
    CRoomManager::RoomInfo_t speakToRoomInfo;
    CRoomManager::RoomInfo_t curRoomInfo;
    
    std::vector<std::string> roomIdToDeleteVector;
    CRoomManager::RoomInfo_t tmpRoomInfo;
    bool leavingSpeakToRoom = false;
    
    std::string strSpeakToRoomId = m_pRoomMgr->getSpeakToRoomId();
    bool roomInfoOK = m_pRoomMgr->getRoomInfo(strSpeakToRoomId, speakToRoomInfo);
    if (!roomInfoOK) {
        //should not goto here:
        roomInfoOK = m_pRoomMgr->getFirstRoomInfo(speakToRoomInfo);
        if (roomInfoOK){
            strSpeakToRoomId = speakToRoomInfo.idFull.substr(mAppKey.length());
        }
    }
    if (!roomInfoOK) {
        mIsReconnecting = false;
        TSK_DEBUG_ERROR("Fatal error, cannot find info for speakToRoomId:%s", strSpeakToRoomId.c_str());
        return;
    }
    
    m_pRoomMgr->setRoomState(strSpeakToRoomId, CRoomManager::ROOM_STATE_RECONNECTING);
    sendCbMsgCallEvent(YOUME_EVENT_RECONNECTING, YOUME_SUCCESS, strSpeakToRoomId, mStrUserID);
    
    // stop send audio/video data
    if (m_avSessionMgr) {
        tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
        if (session) {
            trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 0); 

            // 停止p2p检测定时器
            trtp_manager_stop_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
        }
    }

    CVideoChannelManager::getInstance()->pause();
    
    // Stop everything
    m_loginService.StopLogin();
    TSK_DEBUG_INFO ("doReconnect: stop m_loginService ok");
    
    //#if defined(__APPLE__)
    //    uint32_t consumerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_CONSUMER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_CONSUMER_PEROID_MS);
    //    uint32_t producerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_PRODUCER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_PRODUCER_PEROID_MS);
    //    if (consumerPeriod > 0) {
    //        AVRuntimeMonitor::getInstance()->StopConsumerMoniterThread();
    //    }
    //    if (producerPeriod > 0) {
    //        AVRuntimeMonitor::getInstance()->StopProducerMoniterThread();
    //    }
    //    if (consumerPeriod > 0 || producerPeriod > 0) {
    //        AVRuntimeMonitor::getInstance()->UnInit();
    //    }
    //#endif
    //
    //    stopPacketStatReportThread();
    
    // If the AV session manager is active, we need to restore it after relogin.
    
    
    for (int i = 0; i < RECONNECT_RETRY_MAX; i++) { // 重连机制
        if (m_reconnctWait.WaitTime(0) != youmecommon::WaitResult_Timeout)
        {   /** 外面终止，不再重连 */
            TSK_DEBUG_INFO ("doReconnect retry i = %d, abort outside", i);
            m_reconnctWait.Reset();
            break;
        }
        
        uint32_t waitMsec = getReconnectWaitMsec(i);
        
        TSK_DEBUG_INFO ("doReconnect retry i = %d, wait = %d", i, waitMsec);
        
        if (m_reconnctWait.WaitTime(waitMsec) != youmecommon::WaitResult_Timeout)
        {   /** 等待过程中终止，不再重连 */
            TSK_DEBUG_INFO ("doReconnect retry i = %d, abort in wait", i);
            m_reconnctWait.Reset();
            break;
        }
        
        // Delete the rooms what user want to leave
        if (m_pRoomMgr->getFirstRoomInfo(tmpRoomInfo)) {
            do {
                if (tmpRoomInfo.state ==  CRoomManager::ROOM_STATE_DISCONNECTING) {
                    // if tmpRoomInfo is speakToRoom, reassign a new speakToRoom
                    if (tmpRoomInfo.idFull.compare(speakToRoomInfo.idFull) == 0) {
                        leavingSpeakToRoom = true;
                    }
                    roomIdToDeleteVector.push_back(tmpRoomInfo.idFull.substr(mJoinAppKey.length()));
                }
                if (!m_pRoomMgr->getNextRoomInfo(tmpRoomInfo)) {
                    break;
                }
            } while (true);
        }
        
        for (auto roomId : roomIdToDeleteVector)
        {
            m_pRoomMgr->removeRoom(roomId);
            sendCbMsgCallEvent(YOUME_EVENT_LEAVED_ONE, YOUME_SUCCESS, roomId, mStrUserID);
        }
        roomIdToDeleteVector.clear();
        
        if (leavingSpeakToRoom){
            CRoomManager::RoomInfo_t roomInfo;
            if (m_pRoomMgr->getFirstRoomInfo(roomInfo)){
                m_pRoomMgr->setSpeakToRoom(roomInfo.idFull.substr(mJoinAppKey.length()));
                speakToRoomInfo = roomInfo;
            }
        }
        
        // Get the configurations from the validate server again.
        YouMeErrorCode errCode = loginToMcu(speakToRoomInfo.idFull, mRole, true, mVideoAutoRecv);
        if ((YOUME_SUCCESS != errCode) && (YOUME_ERROR_USER_ABORT != errCode)) {
            // Get the configurations/(redirect servers) from the validate server again.
            mRedirectServerInfoVec.clear();
            mCanRefreshRedirect = false;
            ReportQuitData::getInstance()->m_valid_count++;
            if (YOUME_SUCCESS == CSDKValidate::GetInstance()->ServerLoginIn(true, mAppKeySuffix, mRedirectServerInfoVec, mCanRefreshRedirect))
            {
                if (!g_extServerRegionName.empty()) {
                    // The redirect server list returned here is for the region speicifed in mServerRegionName.
                    // Not for regions specified in mServerRegionNameMap.
                    mLastServerRegionNameMap.insert(ServerRegionNameMap_t::value_type(g_extServerRegionName, 1));
                }
                
                errCode = loginToMcu(speakToRoomInfo.idFull, mRole, true, mVideoAutoRecv);
            }
        }
        
        if (YOUME_SUCCESS == errCode)
        {
            bool isAvSessionMgrActive = false;
            m_avSessionMgrMutex.lock();
            //if (bMcuChanged && m_avSessionMgr) {
			if ( m_avSessionMgr) {
                isAvSessionMgrActive = true;
                //        m_avSessionMgr->UnInit ();
                //        delete m_avSessionMgr;
                //        m_avSessionMgr = NULL;
                stopAvSessionManager(true);
                TSK_DEBUG_INFO ("doReconnect: stop m_avSessionMgr ok");
            }
            m_avSessionMgrMutex.unlock();
            
            {
                /* 断线重连 数据上报 */
                ReportService * report = ReportService::getInstance();
                youmeRTC::ReportChannel disconnect;
                disconnect.operate_type = REPORT_CHANNEL_RECONNECT;
                disconnect.sessionid = mSessionID;
                disconnect.sdk_version = SDK_NUMBER;
                disconnect.platform = NgnApplication::getInstance()->getPlatform();
                disconnect.canal_id = NgnApplication::getInstance()->getCanalID();
                disconnect.package_name = NgnApplication::getInstance()->getPackageName();
                report->report(disconnect);
            }
            
            if (isAvSessionMgrActive) {
                bool needMic = NeedMic();
                if (m_bReleaseMicWhenMute && needMic) {
                    needMic = m_bMicMute ? false : true;
                }
                errCode = startAvSessionManager(needMic, m_bOutputToSpeaker,false);
            }

            if (YOUME_SUCCESS == errCode)
            {
                this->m_bInputVideoIsOpen = false;
                
                restoreEngine();
                #ifdef TRANSSCRIBER
				if (mTranscriberEnabled)
				{
					YMTranscriberManager::getInstance()->Start();
				}
                #endif
                // For multi-room, join other rooms by send message to the server
                if (ROOM_MODE_MULTI == m_roomMode) {
                    if (m_pRoomMgr->getFirstRoomInfo(curRoomInfo)) {
                        do {
                            // Only send message for rooms except the speakToRoomId
                            if (curRoomInfo.idFull.compare(speakToRoomInfo.idFull) == 0) {
                                m_pRoomMgr->setCurrentRoomState(CRoomManager::ROOM_STATE_CONNECTED);
                            } else {
                                if (m_loginService.JoinChannel(mSessionID, curRoomInfo.idFull, this) == YOUME_SUCCESS) {
                                    m_pRoomMgr->setCurrentRoomState(CRoomManager::ROOM_STATE_RECONNECTING);
                                }
                            }
                            
                            if (!m_pRoomMgr->getNextRoomInfo(curRoomInfo)) {
                                break;
                            }
                        } while (true);
                    }
                }else if (ROOM_MODE_SINGLE == m_roomMode) {
                    if (m_pRoomMgr->getFirstRoomInfo(curRoomInfo)) {
                        m_pRoomMgr->setCurrentRoomState(CRoomManager::ROOM_STATE_CONNECTED);
                    }
                }
                
                {
                    // send local user videoinfo to server
                    if (m_UserVideoInfoList.size() > 0) {
                        TSK_DEBUG_INFO ("send cache uservideoinfo to server again, list count:%d", m_UserVideoInfoList.size());

                        m_loginService.setUserRecvResolutionReport(mSessionID, m_UserVideoInfoList);
                    }

                    // check local video send status
                    doCheckVideoSendStatus();

                    // send local_video_input_start when reconnect success
                    doVideoInputStatusChgReport( m_bCameraIsOpen ? 1 : 0 , YOUME_SUCCESS);
                }
                
                // resume send audio/video data
                if (m_avSessionMgr) {
                    tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                    if (session) {
                        trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 1); 
                    }
                }

                mIsReconnecting = false;
                sendCbMsgCallEvent(YOUME_EVENT_RECONNECTED, YOUME_SUCCESS, strSpeakToRoomId, mStrUserID);
                CVideoChannelManager::getInstance()->resume();
                TSK_DEBUG_INFO ("== doReconnect OK");
                return;
            }
        }
    }
    
    // resume send audio/video data
    if (m_avSessionMgr) {
        tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
        if (session) {
            trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 1); 
        }
    }

    mIsReconnecting = false;
    sendCbMsgCallEvent(YOUME_EVENT_RECONNECTED, YOUME_ERROR_UNKNOWN, strSpeakToRoomId, mStrUserID);
    CVideoChannelManager::getInstance()->clear();
    TSK_DEBUG_INFO ("== doReconnect failed or interrupted");
}

void CYouMeVoiceEngine::doPauseConference(bool needCallback)
{
    TSK_DEBUG_INFO("$$ doPauseConference");
    if (m_pRoomMgr->getRoomCount() <= 0) {
        return;
    }

    if (NULL != m_avSessionMgr) {
        tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
        if (session) {
            trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 0); 

            // 停止p2p检测定时器
            trtp_manager_stop_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
        }

        stopAvSessionManager();
    }
    if (needCallback) {
        sendCbMsgCallEvent(YOUME_EVENT_PAUSED, YOUME_SUCCESS, "", mStrUserID);
    }
    
    if( this->m_bInputVideoIsOpen == true  )
    {
        m_loginService.videoInputStatusChgReport(mStrUserID, mSessionID,  false);
    }
}

void CYouMeVoiceEngine::doResumeConference(bool needCallback)
{
    TSK_DEBUG_INFO("$$ doResumeConference");
    if (m_pRoomMgr->getRoomCount() <= 0) {
        return;
    }
    
    if( this->m_bInputVideoIsOpen == true  )
    {
        m_loginService.videoInputStatusChgReport(mStrUserID, mSessionID,  true);
    }
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (NULL == m_avSessionMgr) {
        bool needMic = NeedMic();
        if (m_bReleaseMicWhenMute && needMic) {
            needMic = m_bMicMute ? false : true;
        }
        errCode = startAvSessionManager(needMic, m_bOutputToSpeaker,false);
        if (ROOM_MODE_MULTI == m_roomMode) {
            std::string strRoomID = m_pRoomMgr->getSpeakToRoomId();
            CRoomManager::RoomInfo_t roomInfo;
            if (!m_pRoomMgr->getRoomInfo(strRoomID, roomInfo)) {
                TSK_DEBUG_INFO ("== doResumeConference speak to room(%s) again, room doesn't exist", strRoomID.c_str());
                return;
            }
            uint32_t timestamp = m_avSessionMgr->getRtpTimestamp();
            TSK_DEBUG_INFO ("== doResumeConference speak to room(%s) again, since timestamp:%u", strRoomID.c_str(), timestamp);
            YouMeErrorCode tempErrCode = m_loginService.SpeakToChannel(mSessionID, roomInfo.idFull, timestamp);
            if (YOUME_SUCCESS != tempErrCode) {
                TSK_DEBUG_INFO ("== doResumeConference speak to room(%s) again,  since timestamp:%u failed", strRoomID.c_str(), timestamp);
            }
        }
        if (YOUME_SUCCESS == errCode)
        {
            restoreEngine();
        }
    }
    if (needCallback) {
        sendCbMsgCallEvent(YOUME_EVENT_RESUMED, errCode, "", mStrUserID);
    }
}

std::string CYouMeVoiceEngine::getUserNameBySessionId( int32_t sessionid )
{
    // 共享流的sessionid是对应user的sessionid取反
    int tempSessionId = abs(sessionid);
    if( tempSessionId == mSessionID ){
        return mStrUserID;
    }
 
    std::lock_guard< std::mutex > lock(m_mutexSessionMap);
    auto iter = m_SessionUserIdMap.find( tempSessionId );
    if( iter != m_SessionUserIdMap.end() )
    {
        return iter->second;
    }
    else
    {
        return "";
    }
}

int CYouMeVoiceEngine::getSessionIdByUserName( const std::string& userName )
{
    std::lock_guard< std::mutex > lock(m_mutexSessionMap);
    for( auto iter = m_SessionUserIdMap.begin(); iter != m_SessionUserIdMap.end(); ++iter ){
        if( iter->second == userName ){
            return iter->first;
        }
    }
    
    return 0;
}

int CYouMeVoiceEngine::getSelfSessionID()
{
    return mSessionID;
}


void CYouMeVoiceEngine::doOnReceiveSessionUserIdPair(SessionUserIdPairVector_t &idPairVector)
{
    for (int i = 0; i < idPairVector.size(); i++) {
        if( idPairVector[i].sessionId == mSessionID )
        {
            continue;
        }
        
        CVideoChannelManager::getInstance()->insertUser(idPairVector[i].sessionId, idPairVector[i].userId);
        AVStatistic::getInstance()->NotifyUserName(idPairVector[i].sessionId,  idPairVector[i].userId );
        #if FFMPEG_SUPPORT
        YMVideoRecorderManager::getInstance()->NotifyUserName( idPairVector[i].sessionId,  idPairVector[i].userId);
        #endif
        TSK_DEBUG_INFO("- session:%d, user:%s", idPairVector[i].sessionId, idPairVector[i].userId.c_str());
        
        {
            std::lock_guard< std::mutex > lock(m_mutexSessionMap);
            
            auto iter = m_SessionUserIdMap.begin();
            for (; iter != m_SessionUserIdMap.end(); ++iter) {
                if (iter->second == idPairVector[i].userId && (iter->first != idPairVector[i].sessionId)) {
                    
                    iter = m_SessionUserIdMap.erase(iter);
                    m_SessionUserIdMap.insert(SessionUserIdMap_t::value_type(idPairVector[i].sessionId, idPairVector[i].userId));
                    TSK_DEBUG_INFO ("doOnReceiveSessionUserIdPair 1st insert user:%s sessionid:%u from m_SessionUserIdMap", idPairVector[i].userId.c_str(), idPairVector[i].sessionId);
                    break;
                }
            }

            if (iter == m_SessionUserIdMap.end()) {
                m_SessionUserIdMap.insert(SessionUserIdMap_t::value_type(idPairVector[i].sessionId, idPairVector[i].userId));
                TSK_DEBUG_INFO ("doOnReceiveSessionUserIdPair 2nd insert user:%s sessionid:%u from m_SessionUserIdMap", idPairVector[i].userId.c_str(), idPairVector[i].sessionId);
            }

            // std::pair<SessionUserIdMap_t::iterator, bool> retPair = m_SessionUserIdMap.insert(SessionUserIdMap_t::value_type(idPairVector[i].sessionId, idPairVector[i].userId));
            // if (false == retPair.second) {
            //     TSK_DEBUG_ERROR("Failed to insert to map");
            // }
        }
    }
}

void CYouMeVoiceEngine::doNotifyVadStatus(int32_t sessionId, bool bSilence)
{
    std::string userName = getUserNameBySessionId( sessionId );

    if ( userName == "" ) {
        YouMeProtocol::YouMeVoice_Command_Session2UserIdRequest session2UserIdReq;
        session2UserIdReq.add_sessionid(sessionId);
        session2UserIdReq.set_user_session( mSessionID );
        sendSessionUserIdMapRequest(session2UserIdReq);
    } else {
        //SK_DEBUG_INFO("\n ------------------------------ vad sessionId:%d, bSilence:%d", sessionId, (int)bSilence);
        if (bSilence) {
            sendCbMsgCallEvent(YOUME_EVENT_OTHERS_VOICE_OFF, YOUME_SUCCESS, "", userName);
        } else {
            sendCbMsgCallEvent(YOUME_EVENT_OTHERS_VOICE_ON, YOUME_SUCCESS, "", userName);
        }
    }
}

void CYouMeVoiceEngine::vadCallback(int32_t sessionId, tsk_bool_t isSilence)
{
    //TSK_DEBUG_INFO("\n--------- get vad callback, session:%d, isSilence:%d\n", sessionId, isSilence);
    getInstance()->notifyVadStatus(sessionId, isSilence);
}

void CYouMeVoiceEngine::notifyVadStatus(int32_t sessionId, tsk_bool_t isSilence)
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbVad);
        if (pMsg) {
            pMsg->m_param.cbVad.sessionId = sessionId;
            pMsg->m_param.cbVad.bSilence = isSilence ? true : false;
            m_pMainMsgLoop->SendMessage(pMsg);
        }
    }
}

void CYouMeVoiceEngine::doSetPcmCallback(IYouMePcmCallback* callback, int flag )
{
    std::lock_guard<std::recursive_mutex> pcmCallbackLoopLock(m_pcmCallbackLoopMutex);
    // Destroy the current message loop to avoid memory violation.
    if (m_pPcmCallbackLoop) {
        m_pPcmCallbackLoop->Stop();
        delete m_pPcmCallbackLoop;
        m_pPcmCallbackLoop = NULL;
    }
    
    if (!m_pPcmCallbackLoop && callback) {
        m_pPcmCallbackLoop = new(nothrow) CMessageLoop(PcmCallbackHandler, this, "PcmCbMsg");
        if (m_pPcmCallbackLoop) {
            m_pPcmCallbackLoop->Start();
        }
    }
    
    mPPcmCallback = callback;
    mPcmCallbackFlag = flag;
    
    if (mPPcmCallback && m_avSessionMgr) {
        m_avSessionMgr->setPcmCallback((void*)pcmCallback);
        m_avSessionMgr->setPcmCallbackFlag( mPcmCallbackFlag );
        applySpeakerMute(m_bSpeakerMute);
    }
}

void CYouMeVoiceEngine::doSetTranscriberCallback(IYouMeTranscriberCallback* callback, bool enable )
{
    #ifdef TRANSSCRIBER
	if (enable)
	{
		//todopinky:临时在本地生成token
		YMTranscriberManager::getInstance()->setToken("");
		YMTranscriberManager::getInstance()->Start();
	}
	else {
		YMTranscriberManager::getInstance()->Stop();
	}
    if ( m_avSessionMgr) {
        m_avSessionMgr->setTranscriberEnable( enable );
    }
    #endif
}

void CYouMeVoiceEngine::notifySentenceBegin(int sessionId, int setenceIndex, int sentenceTime )
{
	if (mPTranscriberCallback)
	{
		CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbSentenceBegin );
		if (pMsg) {
			pMsg->m_param.cbSentenceBegin.sessionId = sessionId;
			pMsg->m_param.cbSentenceBegin.sentenceIndex = setenceIndex;
			pMsg->m_param.cbSentenceBegin.sentenceTime = sentenceTime;
			m_pCbMsgLoop->SendMessage(pMsg);
		}
	}
}

void  CYouMeVoiceEngine::notifySentenceEnd(int sessionId, int sentenceIndex, std::string result, int sentenceTime)
{
	if (mPTranscriberCallback)
	{
		CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbSentenceEnd);
		if (pMsg) {
			pMsg->m_param.cbSentenceEnd.sessionId = sessionId;
			pMsg->m_param.cbSentenceEnd.sentenceIndex = sentenceIndex;
			*(pMsg->m_param.cbSentenceEnd.result) = result;
			pMsg->m_param.cbSentenceEnd.sentenceTime = sentenceTime;
			m_pCbMsgLoop->SendMessage(pMsg);
		}
	}
}

void  CYouMeVoiceEngine::notifySentenceChanged(int sessionId, int sentenceIndex, std::string result, int sentenceTime)
{
	if (mPTranscriberCallback)
	{
		CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbSentenceChanged);
		if (pMsg) {
			pMsg->m_param.cbSentenceEnd.sessionId = sessionId;
			pMsg->m_param.cbSentenceEnd.sentenceIndex = sentenceIndex;
			*(pMsg->m_param.cbSentenceEnd.result) = result;
			pMsg->m_param.cbSentenceEnd.sentenceTime = sentenceTime;
			m_pCbMsgLoop->SendMessage(pMsg);
		}
	}
}

void CYouMeVoiceEngine::pcmCallback(tdav_session_audio_frame_buffer_t* frameBuffer, int flag)
{
    getInstance()->notifyPcmCallback(frameBuffer, flag );
}

void CYouMeVoiceEngine::notifyPcmCallback(tdav_session_audio_frame_buffer_t* frameBuffer, int flag )
{
    if (!frameBuffer) {
        return;
    }
    
    std::lock_guard<std::recursive_mutex> pcmCallbackLoopLock(m_pcmCallbackLoopMutex);
    
    if (m_pPcmCallbackLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbPcm);
        if (pMsg) {
            tsk_object_ref(frameBuffer);
            pMsg->m_param.cbPcm.frameBuffer = frameBuffer;
            pMsg->m_param.cbPcm.callbackFlag = flag;
            m_pPcmCallbackLoop->SendMessage(pMsg);
        }
    }
}

void CYouMeVoiceEngine::doPlayBackgroundMusic(const string &strFilePath, bool bRepeat)
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("$$ doPlayBackgroundMusic");
    
    m_bgmPauseMutex.lock();
    m_bBgmPaused = false;
    m_bgmPauseCond.notify_all();
    m_bgmPauseMutex.unlock();
    
    // Before creating a new thread, check if the previous thread was stopped.
    // If not, stop it first.
    if (m_playBgmThread.joinable()) {
        m_bBgmStarted = false;
        if (std::this_thread::get_id() != m_playBgmThread.get_id()) {
            TSK_DEBUG_INFO("Start to join the BGM thread");
            // The flag should be set before disabling mixing audio
            m_playBgmThread.join();
            TSK_DEBUG_INFO("Join the BGM thread OK");
        } else {
            m_playBgmThread.detach();
        }
    }
    
    m_bBgmStarted = true;
    mLastMusicPath = strFilePath;
    mbMusicRepeat = bRepeat;
    
    m_playBgmThread = std::thread(&CYouMeVoiceEngine::PlayBackgroundMusicThreadFunc, this, strFilePath, bRepeat);
    
    TSK_DEBUG_INFO("== doPlayBackgroundMusic");
#endif
}

void CYouMeVoiceEngine::doPauseBackgroundMusic(bool bPaused)
{
    //数据上报
    {
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportBackgroundMusic bg;
        if( bPaused )
        {
            bg.operate_type = BG_OP_PAUSE;
        }
        else{
            bg.operate_type = BG_OP_RESUME;
        }
        
        bg.sdk_version = SDK_NUMBER;
        bg.platform = NgnApplication::getInstance()->getPlatform();
        bg.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report(bg );
    }
    
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("$$ doPauseBackgroundMusic");
    
    m_bgmPauseMutex.lock();
    m_bBgmPaused = bPaused;
    m_bgmPauseCond.notify_all();
    m_bgmPauseMutex.unlock();
    
    TSK_DEBUG_INFO("== doPauseBackgroundMusic");
#endif
}

void CYouMeVoiceEngine::doStopBackgroundMusic()
{
    //数据上报
    {
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportBackgroundMusic bg;
        bg.operate_type = BG_OP_STOP;
        bg.sdk_version = SDK_NUMBER;
        bg.platform = NgnApplication::getInstance()->getPlatform();
        bg.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report(bg );
    }
    
    
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("$$ doStopBackgroundMusic");
    
    if (m_playBgmThread.joinable()) {
        m_bgmPauseMutex.lock();
        m_bBgmPaused = false;
        m_bgmPauseCond.notify_all();
        m_bgmPauseMutex.unlock();
        
        m_bBgmStarted = false;
        if (std::this_thread::get_id() != m_playBgmThread.get_id()) {
            TSK_DEBUG_INFO("Start to join the BGM thread");
            // The flag should be set before disabling mixing audio
            m_playBgmThread.join();
            TSK_DEBUG_INFO("Join the BGM thread OK");
        } else {
            m_playBgmThread.detach();
        }
    }
    
    TSK_DEBUG_INFO("== doStopBackgroundMusic");
#endif
}

void CYouMeVoiceEngine::PlayBackgroundMusicThreadFunc(const string strFilePath, bool bRepeat)
{
#if FFMPEG_SUPPORT
    
    TSK_DEBUG_INFO("$$ PlayBackgroundMusicThreadFunc:%s, bRepeat:%d", strFilePath.c_str(), bRepeat);
    
    void* pBuf = NULL;
    uint32_t maxSize = 0;
    AudioMeta_s audioMeta;
    YouMeErrorCode result;
    
    // 新的数据上报 初始化成功
    ReportService * report = ReportService::getInstance();
    youmeRTC::ReportBackgroundMusic bg;
    bg.operate_type = BG_OP_START;
    bg.file_path = strFilePath;
    bg.play_repeat = bRepeat == true ? 1 : 0 ;
    bg.sdk_version = SDK_NUMBER;
    bg.platform = NgnApplication::getInstance()->getPlatform();
    bg.canal_id = NgnApplication::getInstance()->getCanalID();
    
    
    IFFMpegDecoder* pAudioDec = IFFMpegDecoder::createInstance();
    if (!pAudioDec || !pAudioDec->open(strFilePath.c_str())) {
        TSK_DEBUG_ERROR("Failed open file");
        IFFMpegDecoder::destroyInstance(&pAudioDec);
        sendCbMsgCallEvent(YOUME_EVENT_BGM_FAILED, YOUME_SUCCESS, "", mStrUserID);
        
        bg.play_result = YOUME_EVENT_BGM_FAILED ;
        report->report(bg);
        
        return;
    }
    
    bg.play_result = 0 ;
    report->report(bg);
    
    
    while (m_bBgmStarted) {
        std::unique_lock<std::mutex> pauseLock(m_bgmPauseMutex);
        if (m_bBgmPaused) {
            m_bgmPauseCond.wait(pauseLock);
        }
        pauseLock.unlock();
        
        int freeCount = 0;
        {
            std::lock_guard<std::recursive_mutex> sessionMgrLock(m_avSessionMgrMutex);
            if( m_avSessionMgr )
            {
                freeCount = m_avSessionMgr->getMixAudioFreeBuffCount();
            }
        }
        
        if( freeCount <= 0 )
        {
            //判断是否满了，不满就取数据，满了就等待一会
            XSleep(10);
            continue;
        }

        int32_t outSize = pAudioDec->getNextFrame(&pBuf, &maxSize, &audioMeta, NULL);
        if (!m_bBgmStarted) {
            pAudioDec->close();
            break;
        }

        if (outSize > 0) {
            result = mixAudioTrack(pBuf, outSize, audioMeta.channels, audioMeta.sampleRate,
                                   2, 0, false, true, false, false);
            // If mixAudioTrack returned false, the session should be in an unstable status(e.g. reconnecting),
            // just sleep a while and try again later.
            if (YOUME_SUCCESS != result) {
                XSleep(10);
                continue;
            }
        } else if (0 == outSize) {
            continue;
        } else if (bRepeat && outSize == -2) {
            // reopen the file
            pAudioDec->close();
            if (!pAudioDec->open(strFilePath.c_str())) {
                TSK_DEBUG_ERROR("Failed open file");
                break;
            }
        } else {
            // Error or end of file
            break;
        }
    }
    // Release resources
    if (pBuf) {
        free(pBuf);
        pBuf = NULL;
    }
    pAudioDec->close();
    IFFMpegDecoder::destroyInstance(&pAudioDec);
    
    // If it's not stopped intentionally by the user, send a notification.
    if (m_bBgmStarted) {
        sendCbMsgCallEvent(YOUME_EVENT_BGM_STOPPED, YOUME_SUCCESS, "", mStrUserID);
    }
    
    TSK_DEBUG_INFO("== PlayBackgroundMusicThreadFunc:%s", strFilePath.c_str());
    
#endif //FFMPEG_SUPPORT
}

#if TARGET_OS_IPHONE
void CYouMeVoiceEngine::startIosVideoThread()
{
    m_refreshTimeout = tmedia_get_max_video_refresh_timeout(); //no data input check timeout

    TSK_DEBUG_INFO("$$ startIosVideoThread, refresh:%u", m_refreshTimeout);
    m_videoQueueStarted = true; 
    m_iosVideoThread = std::thread(&CYouMeVoiceEngine::processIosVideothreadFunc, this);
}


void CYouMeVoiceEngine::stopIosVideoThread()
{
    if (m_iosVideoThread.joinable()) {
        m_videoQueueStarted = false;
        m_videoQueueMutex.lock();
        m_videoQueueCond.notify_all();
        m_videoQueueMutex.unlock();
        
        if (std::this_thread::get_id() != m_iosVideoThread.get_id()) {
            TSK_DEBUG_INFO("Start joining thread");
            m_iosVideoThread.join();
            TSK_DEBUG_INFO("Joining thread OK");
        } else {
            m_iosVideoThread.detach();
        }

        {
            std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
            std::deque<FrameImage*>::iterator it;
            while (!m_videoQueue.empty()) {
                FrameImage* pMsg = m_videoQueue.front();
                m_videoQueue.pop_front();
                if (pMsg) {
                    ICameraManager::getInstance()->decreaseBufferRef(pMsg->data);
                    delete pMsg;
                    pMsg = NULL;
                }
            }
        }
    }
}

void CYouMeVoiceEngine::processIosVideothreadFunc()
{
    TSK_DEBUG_INFO("processIosVideothreadFunc() thread Enter");

    YouMeErrorCode ret = YOUME_SUCCESS;
    FrameImage* frame = NULL;
    int tempWidth = 0, tempHeight = 0;

    while (m_videoQueueStarted) {
        // 同步模式强制处理
        if (!m_refreshTimeout) {
            // 重刷时间为0, 则关闭重刷线程
            break;
        }

        {
            std::unique_lock<std::mutex> queueLock(m_videoQueueMutex);
            if (m_videoQueueCond.wait_for(queueLock, std::chrono::milliseconds(m_refreshTimeout)) == std::cv_status::timeout) {
                if (!m_videoQueueStarted) {
                    break;
                }

                if (m_videoQueue.empty()) {
                    continue;
                }
                
                frame = m_videoQueue.front();
                frame->timestamp += m_refreshTimeout;
                m_videoQueue.pop_front();
            } else {
                continue;
            }
        }

        // 重发数据不需要cache
        ICameraManager::getInstance()->setCacheLastPixelEnable(false);
        TSK_DEBUG_INFO("retry send last frame, ts:%llu", frame->timestamp);

        // for screen share, no need preview, renderflag:0
        ret = ICameraManager::getInstance()->videoDataOutputGLESForIOS(frame->data, frame->width, frame->height, frame->fmt, frame->rotate, 0, frame->timestamp, 0);

        {
            // 用于外部输入帧率低时，强刷最近的一帧数据给远端，时间戳会增加
            std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
            if (m_videoQueue.empty()) {
                m_videoQueue.push_back(frame);
                continue;
            }
        }
        
        ICameraManager::getInstance()->decreaseBufferRef(frame->data);
        delete frame;
        frame = NULL;
    }
    
    TSK_DEBUG_INFO("processIosVideothreadFunc() thread leave");
}
#endif

void CYouMeVoiceEngine::doOnRoomEvent(const std::string& strRoomID,
                                      NgnLoginServiceCallback::RoomEventType eventType,
                                      NgnLoginServiceCallback::RoomEventResult result)
{
    switch (eventType) {
        case NgnLoginServiceCallback::ROOM_EVENT_JOIN:
            doJoinConferenceMoreDone(strRoomID, result);
            break;
        case NgnLoginServiceCallback::ROOM_EVENT_LEAVE:
            doLeaveConferenceMultiDone(strRoomID, result);
            break;
        case NgnLoginServiceCallback::ROOM_EVENT_SPEAK_TO:
            doSpeakToConferenceDone(strRoomID, result);
            break;
        default:
            TSK_DEBUG_ERROR("Unkonw room event type:%d", eventType);
    }
}

/////////////////////////////////////////////
// For packet statistics report
void CYouMeVoiceEngine::doPacketStatReport()
{
    tdav_session_av_stat_info_t* stat_info = NULL;
    if (m_avSessionMgr) {
        stat_info = m_avSessionMgr->getPacketStat();
    }
    
    if (stat_info) {
        YouMeProtocol::YouMeVoice_Command_Session2UserIdRequest session2UserIdReq;
        session2UserIdReq.set_user_session( mSessionID );
        
        //TSK_DEBUG_INFO("==== itemNum:%d", stat_info->numOfItems);
        for (int i = 0; i < stat_info->numOfItems; i++) {
            std::string  other_userid = "";
            {
                int otherSession = stat_info->statItems[i].sessionId;
                std::lock_guard< std::mutex > lock(m_mutexSessionMap);
                SessionUserIdMap_t::iterator it = m_SessionUserIdMap.find(otherSession);
                if (it != m_SessionUserIdMap.end()){
                    other_userid = it->second ;
                }
            }
            
            if( other_userid != "" )
            {
                //新上报
                {
                    ReportService * report = ReportService::getInstance();
                    youmeRTC::ReportPacketStatOneUser packetStatOneUser;
                    packetStatOneUser.other_userid = other_userid;
                    packetStatOneUser.packetLossRate10000th = stat_info->statItems[i].packetLossRate10000th;
                    packetStatOneUser.networkDelayMs = stat_info->statItems[i].networkDelayMs;
                    packetStatOneUser.averagePacketTimeMs = stat_info->statItems[i].avgPacketTimeMs;
                    packetStatOneUser.serverRegion = g_serverRegionId;
                    packetStatOneUser.platform = NgnApplication::getInstance()->getPlatform();
                    packetStatOneUser.sdk_version = SDK_NUMBER;
                    
                    packetStatOneUser.canal_id = NgnApplication::getInstance()->getCanalID();
                    report->report(packetStatOneUser);
                }
                
            }
            else{
                //TSK_DEBUG_INFO("==== no userId map for session:%d", stat_info->statItems[i].sessionId);
                session2UserIdReq.add_sessionid(stat_info->statItems[i].sessionId);
            }
        }
        
        if (session2UserIdReq.sessionid_size() > 0) {
            sendSessionUserIdMapRequest(session2UserIdReq);
        }
        
        tsk_object_unref(stat_info);
    }
}

void CYouMeVoiceEngine::startPacketStatReportThread()
{
    uint32_t reportPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::PACKET_STAT_REPORT_PERIOD_MS, NgnConfigurationEntry::DEF_PACKET_STAT_REPORT_PERIOD_MS);
    if (reportPeriod <= 0) {
        return;
    }
    
    // Before creating a new thread, check if the previous thread was stopped. If not, stop it first.
    if (m_packetStatReportThread.joinable()) {
        stopPacketStatReportThread();
    }
    
    m_bPacketStatReportEnabled = true;
    m_packetStatReportThread = std::thread(&CYouMeVoiceEngine::packetStatReportThreadFunc, this, reportPeriod);
}

void CYouMeVoiceEngine::stopPacketStatReportThread()
{
    if (m_packetStatReportThread.joinable()) {
        m_bPacketStatReportEnabled = false;
        m_packetStatReportThreadCondWait.SetSignal();
        if (std::this_thread::get_id() != m_packetStatReportThread.get_id()) {
            TSK_DEBUG_INFO("Start to join the PacketStatReport thread");
            m_packetStatReportThread.join();
            TSK_DEBUG_INFO("Join the PacketStatReport thread OK");
        } else {
            m_packetStatReportThread.detach();
        }
    }
    
}

void CYouMeVoiceEngine::packetStatReportThreadFunc(uint32_t reportPeroidMs)
{
    TSK_DEBUG_INFO("$$ packetStatReportThreadFunc, reportPeroidMs:%u", reportPeroidMs);
    
    while (m_bPacketStatReportEnabled) {
        m_packetStatReportThreadCondWait.Reset();
        m_packetStatReportThreadCondWait.WaitTime(reportPeroidMs);
        // Check the flag again after waking up.
        if (!m_bPacketStatReportEnabled) {
            break;
        }
        mStateMutex.lock();
        if (isStateInitialized() && m_pMainMsgLoop) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiPacketStatReport);
            if (pMsg) {
                m_pMainMsgLoop->SendMessage(pMsg);
            }
        }
        mStateMutex.unlock();
    }
    
    TSK_DEBUG_INFO("== packetStatReportThreadFunc");
}

/////////////////////////////////////////////
// For audio/video qos report

void CYouMeVoiceEngine::doAVQosStatReport()
{
    TSK_DEBUG_INFO("@@ doAVQosStatReport");
    tdav_session_av_qos_stat_t* stat_info = NULL;
    if (m_avSessionMgr) {
        stat_info = m_avSessionMgr->getAVQosStat();
    }
    
    uint64_t curtime = tsk_gettimeofday_ms();
    if (stat_info) {
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportAVQosStatisticInfo  avQosStat;

        avQosStat.business_id = 0;              // not use so far
        avQosStat.sdk_version = SDK_NUMBER;
        avQosStat.platform = NgnApplication::getInstance()->getPlatform();
        avQosStat.canal_id = NgnApplication::getInstance()->getCanalID();
        avQosStat.user_type = 0;    // 仅上报本端

        // 第一次上报默认时间为
        if (!m_lastAVQosStatReportTime) {
            avQosStat.report_period = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AV_QOS_STAT_REPORT_PERIOD_MS, NgnConfigurationEntry::DEF_AV_QOS_STAT_REPORT_PERIOD_MS);
        } else {
            avQosStat.report_period = curtime - m_lastAVQosStatReportTime;
        }
        m_lastAVQosStatReportTime = curtime;

        avQosStat.join_sucess_count = 1;
        avQosStat.join_count = 1;

        avQosStat.stream_id_def = stat_info->videoUpStream;

        avQosStat.audio_up_lost_cnt = stat_info->audioUpLossCnt;
        avQosStat.audio_up_total_cnt = stat_info->audioUpTotalCnt;
        avQosStat.audio_dn_lossrate = stat_info->audioDnLossrate;
        avQosStat.audio_rtt = stat_info->audioRtt;

        avQosStat.video_up_lost_cnt = stat_info->videoUpLossCnt;
        avQosStat.video_up_total_cnt = stat_info->videoUpTotalCnt;
        avQosStat.video_dn_lossrate = stat_info->videoDnLossrate;
        avQosStat.video_rtt = stat_info->videoRtt;

        avQosStat.audio_up_bitrate = stat_info->auidoUpbitrate;
        avQosStat.audio_dn_bitrate = stat_info->auidoDnbitrate;
        avQosStat.video_up_camera_bitrate = stat_info->videoUpCamerabitrate;
        avQosStat.video_up_share_bitrate = stat_info->videoUpSharebitrate;
        avQosStat.video_dn_camera_bitrate = stat_info->videoDnCamerabitrate;
        avQosStat.video_dn_share_bitrate = stat_info->videoDnSharebitrate;

        avQosStat.video_up_fps = 0;
        avQosStat.video_dn_fps = 0;

        avQosStat.block_count = 0;
        avQosStat.block_time = 0;
             
        avQosStat.room_name = mRoomID;
        avQosStat.brand = NgnApplication::getInstance()->getBrand();
        avQosStat.model = NgnApplication::getInstance()->getModel();

        //  TSK_DEBUG_INFO("doAVQosStatReport period:%u, stream:%x audioup:%u-%u, videoup:%u-%u, rtt:%u, dn-loss:%u-%u, bitrate:up[%u-%u-%u]dn[%u-%u-%u]"
        //     , avQosStat.report_period, avQosStat.stream_id_def, avQosStat.audio_up_lost_cnt, avQosStat.audio_up_total_cnt
        //     , avQosStat.video_up_lost_cnt, avQosStat.video_up_total_cnt, avQosStat.audio_rtt, avQosStat.audio_dn_lossrate
        //     , avQosStat.video_dn_lossrate, avQosStat.audio_up_bitrate, avQosStat.video_up_camera_bitrate, avQosStat.video_up_share_bitrate
        //     , avQosStat.audio_dn_bitrate, avQosStat.video_dn_camera_bitrate, avQosStat.video_dn_share_bitrate);
        report->report(avQosStat);

        delete stat_info;
    }
}

void CYouMeVoiceEngine::startAVQosStatReportThread()
{
    uint32_t reportPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AV_QOS_STAT_REPORT_PERIOD_MS, NgnConfigurationEntry::DEF_AV_QOS_STAT_REPORT_PERIOD_MS);
    if (reportPeriod <= 0) {
        return;
    }
    
    // Before creating a new thread, check if the previous thread was stopped. If not, stop it first.
    if (m_avQosStatReportThread.joinable()) {
        stopAVQosStatReportThread();
    }
    
    m_bAVQosStatReportEnabled = true;
    m_avQosStatReportThread = std::thread(&CYouMeVoiceEngine::avQosStatReportThreadFunc, this, reportPeriod);
}

void CYouMeVoiceEngine::stopAVQosStatReportThread()
{
    if (m_avQosStatReportThread.joinable()) {
        m_bAVQosStatReportEnabled = false;
        m_avQosStatReportThreadCondWait.SetSignal();
        if (std::this_thread::get_id() != m_avQosStatReportThread.get_id()) {
            TSK_DEBUG_INFO("Start to join the avQosStatReport thread");
            m_avQosStatReportThread.join();
            TSK_DEBUG_INFO("Join the avQosStatReport thread OK");
        } else {
            m_avQosStatReportThread.detach();
        }
    }
    
}

void CYouMeVoiceEngine::avQosStatReportThreadFunc(uint32_t reportPeroidMs)
{
    TSK_DEBUG_INFO("$$ avQosStatReportThreadFunc, reportPeroidMs:%u", reportPeroidMs);
    
    while (m_bAVQosStatReportEnabled) {
        m_avQosStatReportThreadCondWait.Reset();
        m_avQosStatReportThreadCondWait.WaitTime(reportPeroidMs);
        // Check the flag again after waking up.
        if (!m_bAVQosStatReportEnabled) {
            break;
        }

        mStateMutex.lock();
        if (isStateInitialized() && m_pMainMsgLoop) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiAVQosStatReport);
            if (pMsg) {
                m_pMainMsgLoop->SendMessage(pMsg);
            }
        }
        mStateMutex.unlock();
    }
    
    TSK_DEBUG_INFO("== avQosStatReportThreadFunc");
}

void CYouMeVoiceEngine::micLevelCallback(int32_t micLevel)
{
    getInstance()->sendCbMsgCallEvent(YOUME_EVENT_MY_MIC_LEVEL, (YouMeErrorCode)micLevel, "", "");
}

void CYouMeVoiceEngine::onSessionStartFail(tmedia_type_t type )
{
    if( (type & tmedia_audio) != 0 )
    {
        getInstance()->sendCbMsgCallEvent( YOUME_EVENT_AUDIO_START_FAIL, YOUME_ERROR_UNKNOWN, "", "");
    }
    
}

void CYouMeVoiceEngine::farendVoiceLevelCallback(int32_t farendVoiceLevel, uint32_t sessionId)
{
    std::string userId;
    // Pair userId from sessionId
    userId = CVideoManager::getInstance()->getUserId(sessionId);
    if (userId != "") {
        getInstance()->sendCbMsgCallEvent(YOUME_EVENT_FAREND_VOICE_LEVEL, (YouMeErrorCode)farendVoiceLevel, "", userId);
    } else {
        TMEDIA_I_AM_ACTIVE(10, "Not match the userId from sessionId, request userid with sessionId");
        getInstance()->sendSessionUserIdMapRequest(sessionId);
    }
}

void CYouMeVoiceEngine::doOpenVideoEncoder(const string &strFilePath)
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("$$ doOpenVideoEncoder");
    
    //    const int picWidth     = 352;
    //    const int picHeight    = 288;
    //    const int frameSz      = picWidth * picHeight;
    //    const int frameNum     = 90;
    char dumpVideoPath[1024] = "";
    const char *pBaseDir   = NULL;
    int baseLen            = 0;
    
    pBaseDir = tmedia_defaults_get_app_document_path();
    if (NULL == pBaseDir) {
        return;
    }
    strncpy(dumpVideoPath, pBaseDir, sizeof(dumpVideoPath) - 1);
    baseLen = strlen(dumpVideoPath) + 1; // plus the trailing '\0'
    //strncat(dumpVideoPath, "/dump_h264.h264", sizeof(dumpVideoPath) - baseLen);
    strncat(dumpVideoPath, "/dump_yuv.yuv", sizeof(dumpVideoPath) - baseLen);
    
#if 0
    // x264 initial =======
    x264_t       *pX264Handle = NULL;
    x264_param_t *pX264Param = new x264_param_t;
    //x264_param_default(pX264Param);
    x264_param_default_preset(pX264Param, "veryfast", "zerolatency");
    pX264Param->i_threads = X264_SYNC_LOOKAHEAD_AUTO; // 取空缓冲区继续使用不死锁的保证
    // 视频选项
    pX264Param->i_width    = picWidth;
    pX264Param->i_height   = picHeight;
    pX264Param->i_frame_total = frameNum; // Total video frame;
    pX264Param->i_keyint_max  = 30/3;
    pX264Param->i_keyint_min  = 30/3;
    pX264Param->i_csp = X264_CSP_I420; // x
    // 流参数
    //pX264Param->i_bframe   = 5;// 如果用的zerolatency，这个值不能设或者设置成0，目的是不让encoder有缓冲帧
    //pX264Param->b_open_gop = 0;
    //pX264Param->i_bframe_pyramid  = 0;
    //pX264Param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
#if 1
    // 速率控制参数
    pX264Param->rc.i_bitrate = 1024 * 30;
    //pX264Param->rc.i_bitrate = 1024 * 10;
    pX264Param->i_fps_den    = 1;
    //pX264Param->i_fps_num    = 10;
    pX264Param->i_fps_num    = 30;
    pX264Param->i_timebase_den = pX264Param->i_fps_num;
    pX264Param->i_timebase_num = pX264Param->i_fps_den;
#endif
    // 设置profile，使用Baseline profile
    x264_param_apply_profile(pX264Param, x264_profile_names[1]);
    // 编码辅助变量
    int iNal  = 0;
    x264_nal_t *pNals = NULL;
    x264_picture_t *pPicIn  = new x264_picture_t;
    x264_picture_t *pPicOut = new x264_picture_t;
    x264_picture_init(pPicOut);
    x264_picture_alloc(pPicIn, X264_CSP_I420, pX264Param->i_width, pX264Param->i_height);
    pPicIn->img.i_csp = X264_CSP_I420;
    pPicIn->img.i_plane = 3;
    // 打开编码器句柄
    pX264Handle = x264_encoder_open(pX264Param);
    
    FILE *fpRead  = fopen(strFilePath.c_str(), "rb");
    FILE *fpWrite = fopen(dumpVideoPath, "wb");
    int i, k, ret;
    if ((fpRead != NULL) && (fpWrite != NULL)) {
        for (i = 0; i < frameNum; i++) {
            // 读文件
            fread(pPicIn->img.plane[0], sizeof(char), frameSz, fpRead);   // Y
            fread(pPicIn->img.plane[1], sizeof(char), frameSz/4, fpRead); // U
            fread(pPicIn->img.plane[2], sizeof(char), frameSz/4, fpRead); // U
            pPicIn->i_pts = i;
            // 编码
            ret = x264_encoder_encode(pX264Handle, &pNals, &iNal, pPicIn, pPicOut);
            if (ret < 0) {
                TSK_DEBUG_ERROR ("Encoder error!");
            } else if (ret == 0) {
                TSK_DEBUG_INFO ("Encoder buffer data!");
            } else {
                // 写文件
                for (k = 0; k < iNal; k++) {
                    fwrite(pNals[k].p_payload, 1, pNals[k].i_payload, fpWrite);
                }
            }
        }
        
        // Flush encoder
        i = 0;
        while (1) {
            ret = x264_encoder_encode(pX264Handle, &pNals, &iNal, NULL, pPicOut);
            if (ret == 0) {
                break;
            }
            TSK_DEBUG_INFO("Encoder flush 1 frame!");
            for (k = 0; k < iNal; k++) {
                fwrite(pNals[k].p_payload, 1, pNals[k].i_payload, fpWrite);
            }
            i++;
        }
        
        // 清除图像区域
        x264_picture_clean(pPicIn);
        x264_encoder_close(pX264Handle);
        pX264Handle = NULL;
        delete pPicIn;
        pPicIn = NULL;
        delete pPicOut;
        pPicOut = NULL;
        delete pX264Param;
        pX264Param = NULL;
        fclose(fpRead);
        fpRead = NULL;
        fclose(fpWrite);
        fpWrite = NULL;
    }
#else
    // 初始化文件读写
    FILE *fpWrite = fopen(dumpVideoPath, "wb");
    if (fpWrite == NULL) {
        return;
    }
    
    void* pBuf = NULL;
    uint32_t maxSize = 0;
    VideoMeta_s videoMeta;
    IFFMpegDecoder *pVideoDec = IFFMpegDecoder::createInstance();
    if (!pVideoDec || !pVideoDec->open(strFilePath.c_str())) {
        TSK_DEBUG_ERROR("Failed open file");
        IFFMpegDecoder::destroyInstance(&pVideoDec);
        return;
    }
    
    while (1) {
        int32_t outSize = pVideoDec->getNextFrame(&pBuf, &maxSize, NULL, &videoMeta);
        if (outSize > 0) { // Means decode OK
            fwrite(pBuf, 1, maxSize, fpWrite);
        } else if (outSize < 0){
            break;
        }
    }
    
    if (pBuf) {
        free(pBuf);
        pBuf = NULL;
    }
    pVideoDec->close();
    IFFMpegDecoder::destroyInstance(&pVideoDec);
    fclose(fpWrite);
    fpWrite = NULL;
#endif
    
    TSK_DEBUG_INFO("== doOpenVideoEncoder done!");
#endif
}

void CYouMeVoiceEngine::doMaskVideoByUserId(std::string &userId, int mask)
{
    if (mSessionID > 0) {
        TSK_DEBUG_INFO("$$ doMaskVideoByUserId: userId:%s, mysessionId:%d, mask:%d", userId.c_str(), mSessionID, mask);
        m_loginService.maskVideoByUserIdRequest(userId, mSessionID, mask);
    }
    TSK_DEBUG_INFO("== doMaskVideoByUserId done!");
}

//void CYouMeVoiceEngine::doCamStatusChgReport(int cameraStatus)
//{
//    if (mSessionID > 0) {
//        TSK_DEBUG_INFO("$$ doCamStatusChgReport:  mysessionId:%d, cameraStatus:%d", mSessionID, cameraStatus);
//        m_loginService.cameraStatusChgReport(mSessionID, cameraStatus);
//    }
//    TSK_DEBUG_INFO("== doCamStatusChgReport done!");
//}

void CYouMeVoiceEngine::doVideoInputStatusChgReport(int inputStatus, YouMeErrorCode errorCode)
{
    if (mSessionID > 0) {
        bool tempInputVideoStatus = (inputStatus ? true : false);
        if (tempInputVideoStatus == this->m_bInputVideoIsOpen) {
            TSK_DEBUG_INFO("doVideoInputStatusChgReport repeated inputstatus:%d", this->m_bInputVideoIsOpen);
            return;
        }

        TSK_DEBUG_INFO("$$ doVideoInputStatusChgReport:  myUserId:%s, mysessionId:%d, inputStatus:%d, errorCode:%d", mStrUserID.c_str(), mSessionID, inputStatus, errorCode);
        
        sendCbMsgCallEvent(inputStatus?YOUME_EVENT_LOCAL_VIDEO_INPUT_START:YOUME_EVENT_LOCAL_VIDEO_INPUT_STOP, errorCode, "", mStrUserID);
        
        if (YOUME_SUCCESS !=  errorCode && YOUME_ERROR_CAMERA_EXCEPTION !=  errorCode)
        {
            // input 开启/关闭 失败则不通知房间内其它人
            TSK_DEBUG_INFO("== doVideoInputStatusChgReport failed, errorCode:%d", errorCode);
            return;
        }
        
        //通知服务器，用户视频输入状态
        m_loginService.videoInputStatusChgReport(mStrUserID, mSessionID, inputStatus);

        this->m_bInputVideoIsOpen = inputStatus ? true : false;
        AVStatistic::getInstance()->NotifyVideoStat( mStrUserID,  m_bInputVideoIsOpen );
        
        if (this->m_bInputVideoIsOpen) {
            AVStatistic::getInstance()->NotifyStartVideo();
            m_externalInputStartTime = tsk_gettimeofday_ms();
            
            //report open camera event
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportVideoEvent videoEvent;
            videoEvent.sessionid = mSessionID;
            videoEvent.event = REPORT_VIDEO_OPEN_CAMERA;
            videoEvent.result = errorCode;
            videoEvent.sdk_version = SDK_NUMBER;
            videoEvent.platform = NgnApplication::getInstance()->getPlatform();
            videoEvent.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoEvent);
        } else {
            if( m_externalInputStartTime > 0 ){
                m_externalVideoDuration += tsk_gettimeofday_ms() - m_externalInputStartTime;
                m_externalInputStartTime = 0;
            }
            //report stop camera event
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportVideoEvent videoEvent;
            videoEvent.sessionid = mSessionID;
            videoEvent.event = REPORT_VIDEO_CLOSE_CAMERA;
            videoEvent.result = errorCode;
            videoEvent.sdk_version = SDK_NUMBER;
            videoEvent.platform = NgnApplication::getInstance()->getPlatform();
            videoEvent.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoEvent);
        }
    }
    TSK_DEBUG_INFO("== doVideoInputStatusChgReport done!");
}

void CYouMeVoiceEngine::doAudioInputStatusChgReport(int inputStatus)
{
    if (mSessionID > 0) {
        TSK_DEBUG_INFO("$$ doAudioInputStatusChgReport:  myUserId:%s, mysessionId:%d, inputStatus:%d", mStrUserID.c_str(), mSessionID, inputStatus);
        m_loginService.audioInputStatusChgReport(mStrUserID, mSessionID, inputStatus);
        this->m_bInputAudioIsOpen = inputStatus ? true : false;
    }
    TSK_DEBUG_INFO("== doAudioInputStatusChgReport done!");
}

void CYouMeVoiceEngine::doShareInputStatusChgReport(int inputStatus)
{
    if (mSessionID > 0) {
        TSK_DEBUG_INFO("$$ doShareInputStatusChgReport:  myUserId:%s, mysessionId:%d, inputStatus:%d", mStrUserID.c_str(), mSessionID, inputStatus);
        std::string share_uid = getShareUserName( mStrUserID );
        this->m_bInputShareIsOpen = inputStatus ? true : false;
        sendCbMsgCallEvent(inputStatus?YOUME_EVENT_LOCAL_SHARE_INPUT_START:YOUME_EVENT_LOCAL_SHARE_INPUT_STOP, YOUME_SUCCESS, "", share_uid);
        m_loginService.shareInputStatusChgReport(mStrUserID, mSessionID, inputStatus);
    }
    TSK_DEBUG_INFO("== doShareInputStatusChgReport done!");
}

//void CYouMeVoiceEngine::doAudioInputStatusChgReport(int inputStatus)
//{
//    if (mSessionID > 0) {
//        m_loginService.audioInputStatusChgReport(mStrUserID, mSessionID, inputStatus);
//        this->m_bInputAudioIsOpen = inputStatus ? true : false;
//    }
//
//}

YouMeErrorCode CYouMeVoiceEngine::doQueryUsersVideoInfo(std::vector<std::string>* userList)
{
    if(userList)
    {
        return  m_loginService.queryUserVideoInfoReport(mSessionID, *userList);
    }
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::doSetUsersVideoInfo(std::vector<IYouMeVoiceEngine::userVideoInfo>* videoInfoList)
{
    if(videoInfoList)
    {
        // if (!m_UserVideoInfoList) {
        //     m_UserVideoInfoList = new(nothrow) std::vector<IYouMeVoiceEngine::userVideoInfo>();
        // }

        // m_UserVideoInfoList->clear();

        for (auto iter = videoInfoList->begin(); iter != videoInfoList->end(); ++iter) {

            int sessionId = getSessionIdByUserName(iter->userId);

            if (m_avSessionMgr) {
                tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                if (session) {
                    trtp_manager_set_source_type(TDAV_SESSION_AV(session)->rtp_manager, sessionId, iter->resolutionType);
                }
            }
            
            // m_UserVideoInfoList->push_back(*iter);
        }

        return  m_loginService.setUserRecvResolutionReport(mSessionID, *videoInfoList);
    }
    return YOUME_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// The following functions should be called by the outside modules

void CYouMeVoiceEngine::OnDisconnect()
{
    TSK_DEBUG_INFO ("@@ Enter OnDisconnect");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO ("== OnDisconnect, not inited");
        return;
    }
    
    if (mIsReconnecting) {
        TSK_DEBUG_INFO ("== OnDisconnect, reconnecting is in progress");
        return;
    }
    mIsReconnecting = true;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiReconnect);
        if (pMsg) {
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== OnDisconnect");
            return;
        }
    }
    
    TSK_DEBUG_INFO("== OnDisconnect: failed to send message");
}

void CYouMeVoiceEngine::OnKickFromChannel( const std::string& strRoomID , const std::string& strParam)
{
    TSK_DEBUG_INFO ("@@ Enter OnKickFromChannel");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO ("== OnKickFromChannel, not inited");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiBeKick);
        if (pMsg) {
            if (nullptr == pMsg->m_param.apiBeKick.roomID || nullptr ==  pMsg->m_param.apiBeKick.param  ) {
                delete pMsg;
                pMsg = NULL;
                return ;
            }
            
            std::string roomid;
            removeAppKeyFromRoomId( strRoomID, roomid);
            *(pMsg->m_param.apiBeKick.roomID ) = roomid;
            *(pMsg->m_param.apiBeKick.param  ) = strParam;
            
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO ("@@ OnKickFromChannel");
            return ;
        }
        
    }
    
    TSK_DEBUG_INFO("== OnKickFromChannel: failed to send message");
}

void CYouMeVoiceEngine::destroy ()
{
    TSK_DEBUG_INFO ("@@ destroy");
    std::unique_lock<std::mutex> lck (mInstanceMutex);
	if (mPInstance) {
		delete mPInstance;
		mPInstance = NULL;
	}
    TSK_DEBUG_INFO ("== destroy");
}

bool CYouMeVoiceEngine::ToYMData(int msg, int msgex, int err, YouMeEvent* ym_evt, YouMeErrorCode* ym_err)
{
    if (err == 0){
        *ym_err = YouMeErrorCode::YOUME_SUCCESS;
    }
    else if( err == 1006 )
    {
        *ym_err = YouMeErrorCode::YOUME_ERROR_NOT_IN_CHANNEL;
        
    }
    else{
        *ym_err = YouMeErrorCode::YOUME_ERROR_UNKNOWN;
    }
    
    switch (msg)
    {
        case YouMeProtocol::MSG_FIGHT_4_MIC_INIT:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_START_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_START_FAILED;
            }
            break;
        case YouMeProtocol::MSG_FIGHT_4_MIC_DEINIT:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_STOP_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_STOP_FAILED;
            }
            break;
        case YouMeProtocol::MSG_FIGHT_4_MIC:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_FAILED;
                if (err == YouMeProtocol::FIGHT_MIC_ERROR_CODE::FIGHT_MIC_FULL)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_GRABMIC_FULL;
                else if (err == YouMeProtocol::FIGHT_MIC_ERROR_CODE::FIGHT_MIC_END)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_GRABMIC_HASEND;
                else if (err == YouMeProtocol::FIGHT_MIC_ERROR_CODE::FIGHT_MIC_QUEUE){
                    *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_WAIT;
                    *ym_err = YouMeErrorCode::YOUME_SUCCESS;
                }
            }
            break;
        case YouMeProtocol::MSG_RELEASE_MIC:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_RELEASE_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_RELEASE_FAILED;
            }
            break;
        case YouMeProtocol::MSG_FIGHT_4_MIC_NOTIFY:
            if (msgex == NgnLoginServiceCallback::GRAB_MIC_START){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_NOTIFY_START;
            }
            else if (msgex == NgnLoginServiceCallback::GRAB_MIC_STOP){
                *ym_evt = YouMeEvent::YOUME_EVENT_GRABMIC_NOTIFY_STOP;
            }
            break;
        case YouMeProtocol::MSG_INVITE:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_REQUEST_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_REQUEST_FAILED;
                if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_NOT_EXIST)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_NOUSER;
                else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_OFFLINE)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_OFFLINE;
                else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_REJECT)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_REJECT;
            }
            break;
        case YouMeProtocol::MSG_ACCEPT:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_RESPONSE_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_RESPONSE_FAILED;
                if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_NOT_EXIST)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_NOUSER;
                else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_OFFLINE)
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_OFFLINE;
                else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_REJECT){
                    *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_RESPONSE_OK;
                    *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_REJECT;
                }
            }
            break;
        case YouMeProtocol::MSG_INVITE_BYE:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_STOP_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_STOP_FAILED;
            }
            break;
        case YouMeProtocol::MSG_INVITE_NOTIFY:
            if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_NOT_EXIST)
                *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_NOUSER;
            else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_OFFLINE)
                *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_OFFLINE;
            else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_REJECT)
                *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_REJECT;
            else if (err == YouMeProtocol::INVITE_ERROR_CODE::INVITE_TIME_OUT)
                *ym_err = YouMeErrorCode::YOUME_ERROR_INVITEMIC_TIMEOUT;
            
            if (msgex == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_REQUEST){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_NOTIFY_CALL;
            }
            else if (msgex == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_RESPONSE){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_NOTIFY_ANSWER;
            }
            else if (msgex == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_CANCEL){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_NOTIFY_CANCEL;
            }
            break;
        case YouMeProtocol::MSG_INVITE_INIT:
            if (err == 0){
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_SETOPT_OK;
            }
            else{
                *ym_evt = YouMeEvent::YOUME_EVENT_INVITEMIC_SETOPT_FAILED;
            }
            break;
        case YouMeProtocol::MSG_SEND_MESSAGE_SERVER:
            *ym_evt = YouMeEvent::YOUME_EVENT_SEND_MESSAGE_RESULT;
            break;
        case YouMeProtocol::MSG_MESSAGE_NOTIFY:
            *ym_evt = YouMeEvent::YOUME_EVENT_MESSAGE_NOTIFY;
            if( err == 0 )
            {
                *ym_err = YouMeErrorCode::YOUME_SUCCESS;
            }
            else{
                *ym_err = YouMeErrorCode::YOUME_ERROR_SEND_MESSAGE_FAIL;
            }
            break;
        case YouMeProtocol::MSG_KICK_USRE_ACK:
            *ym_evt = YouMeEvent::YOUME_EVENT_KICK_RESULT;
            break;
        case YouMeProtocol::MSG_KICK_USRE_NOTIFY:
            *ym_evt = YouMeEvent::YOUME_EVENT_KICK_NOTIFY;
            break;
        case YouMeProtocol::MSG_KICK_OTHER_NOTIFY:
            *ym_evt = YouMeEvent::YOUME_EVENT_OTHERS_BE_KICKED;
            break;
        case YouMeProtocol::MSG_QUERY_USER_VIDEO_INFO_ACK:
            *ym_evt = YouMeEvent::YOUME_EVENT_QUERY_USERS_VIDEO_INFO;
            break;
        case YouMeProtocol::MSG_SET_RECV_VIDEO_INFO_ACK:
            *ym_evt = YouMeEvent::YOUME_EVENT_SET_USERS_VIDEO_INFO;
            break;
        case YouMeProtocol::MSG_SET_RECV_VIDEO_INFO_NOTIFY:
            *ym_evt = YouMeEvent::YOUME_EVENT_SET_USERS_VIDEO_INFO_NOTIFY;
            break;
        case YouMeProtocol::MSG_SET_PUSH_SINGLE_ACK:
            *ym_evt = YouMeEvent::YOUME_EVENT_START_PUSH;
            if( err == 0 )
            {
                *ym_err = YouMeErrorCode::YOUME_SUCCESS;
            }
            else{
                *ym_err = YouMeErrorCode::YOUME_ERROR_SET_PUSH_PARAM_FAIL;
            }
            break;
        case YouMeProtocol::MSG_SET_PUSH_MIX_ACK:
            *ym_evt = YouMeEvent::YOUME_EVENT_SET_PUSH_MIX;
            if( err == 0 )
            {
                *ym_err = YouMeErrorCode::YOUME_SUCCESS;
            }
            else{
                *ym_err = YouMeErrorCode::YOUME_ERROR_SET_PUSH_PARAM_FAIL;
            }
            break;
        case YouMeProtocol::MSG_ADD_PUSH_MIX_USER:
            *ym_evt = YouMeEvent::YOUME_EVENT_ADD_PUSH_MIX_USER;
            if( err == 0 )
            {
                *ym_err = YouMeErrorCode::YOUME_SUCCESS;
            }
            else{
                *ym_err = YouMeErrorCode::YOUME_ERROR_SET_PUSH_PARAM_FAIL;
            }
            break;
        case YouMeProtocol::MSG_OTHER_SET_PUSH_MIX:
            *ym_evt = YouMeEvent::YOUME_EVENT_OTHER_SET_PUSH_MIX;
            break;
        default:
            *ym_evt = YouMeEvent::YOUME_EVENT_EOF;
    }
    
    return *ym_evt != YouMeEvent::YOUME_EVENT_EOF;
}


std::string CYouMeVoiceEngine::ToYMRoomID(const std::string& room_id)
{
    return mJoinAppKey + room_id;
}

void CYouMeVoiceEngine::setToken( const char* pToken,  uint32_t timeStamp){
    if( pToken != NULL ){
        m_loginService.SetToken(pToken, timeStamp);
    }
    else{
        m_loginService.SetToken("", 0);
    }
}

YouMeErrorCode CYouMeVoiceEngine::setJoinChannelKey( const std::string &strAPPKey )
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if( !strAPPKey.empty())
    {
        TSK_DEBUG_INFO ("== setJoinChannelKey:%s",strAPPKey.c_str());
        mJoinAppKey = strAPPKey;
        
        NgnApplication::getInstance ()->setAppKey (strAPPKey);
    }
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::init (IYouMeEventCallback *pEventCallback, const std::string &strAPPKey, const std::string &strAPPSecret,
                                        YOUME_RTC_SERVER_REGION serverRegionId)
{
    YouMeErrorCode errCode = YOUME_SUCCESS;
    
    if (!pEventCallback || strAPPKey.empty() || strAPPSecret.empty()) {
        TSK_DEBUG_ERROR ("!! init: inavlid parameters");
        return YOUME_ERROR_INVALID_PARAM;
    }
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (mIsAboutToUninit || (STATE_INITIALIZING == mState) || (STATE_INITIALIZED == mState)) {
        TSK_DEBUG_ERROR ("!! init: wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }

    mState = STATE_INITIALIZING;
    mAppKey = strAPPKey;
    mAppSecret = strAPPSecret;

    if(MediaSessionMgr::defaultsGetExternalInputMode()){
#if defined(__APPLE__)
        m_bReleaseMicWhenMute = true;
#if TARGET_OS_IPHONE
        if (ICameraManager::getInstance ()->isUnderAppExtension()) {
            Config_SetInt("IOS_APP_EXTENSION", 1);
        }
#endif        
#else
        m_bReleaseMicWhenMute = false;
#endif
    }else {
        m_bReleaseMicWhenMute = false;
    }
    
    setJoinChannelKey( strAPPKey );
    mPEventCallback = pEventCallback;
    ICameraManager::getInstance()->registerYoumeEventCallback(pEventCallback);
    
    m_roomMode = ROOM_MODE_NONE;
    // Since the message loop has not been set up yet, we can call the "doXXXX" functions safely here.
    doSetServerRegion(serverRegionId, false);
    
    //验证key与进房间的key分离，域名前缀用验证key
    int appKeyLen = mAppKey.length();
    if (appKeyLen > 8) {
        mAppKeySuffix = mAppKey.substr(appKeyLen - 8);
    } else {
        mAppKeySuffix = mAppKey;
    }
    
    CSDKValidate::GetInstance ()->SetAppKeySuffix( mAppKeySuffix );
    
    mPNgnEngine = NgnEngine::getInstance ();
    mPNgnEngine->initialize ();
    TSK_DEBUG_INFO ("@@ init: %s ",strAPPKey.c_str()  );
    
    m_bExitHttpQuery = false;
#ifdef WIN32
    {   // Initial Ws2_32.dll
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        int err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0)
        {
            return YOUME_ERROR_NETWORK_ERROR;
        }
    }
#endif
    
#ifdef  ANDROID
    // TriggerNetChange();
#endif
    
    // TCP socket在服务器shutdown或其他意外发送了RST消息后，如果还强行去send, 或发SIGPIPE，导致进程退出.
    // 处理SIGPIPE信号可以优雅的处理这种情况。
#if defined(ANDROID) || defined(__APPLE__)
    {
        struct sigaction pipeAct;
        pipeAct.sa_handler = SIG_IGN;
        sigemptyset (&pipeAct.sa_mask);
        pipeAct.sa_flags = 0;
        int ret = sigaction(SIGPIPE, &pipeAct, NULL);
        if (0 != ret) {
            TSK_DEBUG_ERROR("Failed to register the handler for SIGPIPE");
        }
    }
#endif
    
    // Create the channel manager if it's not created yet
    if (m_pRoomMgr) {
        delete m_pRoomMgr;
    }
    m_pRoomMgr = new(nothrow) CRoomManager();
    if (NULL == m_pRoomMgr) {
        TSK_DEBUG_ERROR ("failed to create channel manager");
        errCode = YOUME_ERROR_MEMORY_OUT;
        goto bail_out;
    }
    
    SAFE_DELETE(m_RoomPropMgr);
    m_RoomPropMgr = new(nothrow)CRoomManager();
    if (NULL == m_RoomPropMgr) {
        TSK_DEBUG_ERROR("failed to create channel props manager");
        errCode = YOUME_ERROR_MEMORY_OUT;
        goto bail_out;
    }
    
    // Start message loop
    if (NULL == m_pMainMsgLoop) {
        m_pMainMsgLoop = new(nothrow) CMessageLoop(MainMessgeHandler, this, "MainMsg");
    }
    if (NULL == m_pMainMsgLoop) {
        TSK_DEBUG_ERROR ("failed to create main messsage loop");
        errCode = YOUME_ERROR_MEMORY_OUT;
        goto bail_out;
    }
    m_pMainMsgLoop->Start();
    
    if (NULL == m_pCbMsgLoop) {
        m_pCbMsgLoop = new(nothrow) CMessageLoop(CbMessgeHandler, this, "CbMsg");
    }
    if (NULL == m_pCbMsgLoop) {
        TSK_DEBUG_ERROR ("failed to create callback messsage loop");
        errCode = YOUME_ERROR_MEMORY_OUT;
        goto bail_out;
    }
    m_pCbMsgLoop->Start();
    
    if (NULL == m_pWorkerMsgLoop) {
        m_pWorkerMsgLoop = new(nothrow) CMessageLoop(WorkerMessgeHandler, this, "WorkerMsg");
    }
    if (NULL == m_pWorkerMsgLoop) {
        TSK_DEBUG_ERROR ("failed to create worker messsage loop");
        errCode = YOUME_ERROR_MEMORY_OUT;
        goto bail_out;
    }
    m_pWorkerMsgLoop->Start();
   
#if TARGET_OS_IPHONE
    if (!Config_GetInt ("IOS_APP_EXTENSION", 0)) {
#endif   
        g_ymvideo_pTranslateUtil = new CTranslateUtil();
        if( m_translateThread.joinable() ){
            m_bTranslateThreadExit = true;
            m_translateSemaphore.Increment();
            m_translateThread.join();
        }
        m_bTranslateThreadExit = false;
        m_translateThread = std::thread(&CYouMeVoiceEngine::TranslateThread, this);
#if TARGET_OS_IPHONE        
    }
#endif

    // Clear the PCM callback handler
    m_pcmCallbackLoopMutex.lock();
    if (m_pPcmCallbackLoop) {
        m_pPcmCallbackLoop->Stop();
        delete m_pPcmCallbackLoop;
        m_pPcmCallbackLoop = NULL;
    }
    m_pcmCallbackLoopMutex.unlock();
    
    //避免初始化失败后再次进入
    if( m_queryHttpThread.joinable() ){
        m_bExitHttpQuery = true;
        m_httpSemaphore.Increment();
        m_queryHttpThread.join();
    }
    m_queryHttpThread = std::thread(&CYouMeVoiceEngine::QueryHttpInfoThreadProc, this);
    
    MonitoringCenter::getInstance()->Init();

#if SDK_VALIDATE  
    CSDKValidate::GetInstance ()->Init ();
    CSDKValidate::GetInstance ()->SetPlatform (NgnApplication::getInstance ()->getPlatform ());
    CSDKValidate::GetInstance ()->SetPackageName (NgnApplication::getInstance ()->getPackageName ());
    CSDKValidate::GetInstance ()->SetAppKey (strAPPKey);
    if (!CSDKValidate::GetInstance ()->SetAppSecret (strAPPSecret))
    {
        TSK_DEBUG_ERROR ("SetAppSecret failed");
        errCode = YOUME_ERROR_INVALID_PARAM;
        goto bail_out;
    }
#endif
    m_ulInitStartTime = tsk_time_now();
    
    // Send a message to start the time consuming part of init
    {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiInit);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail_out;
        }
        m_pMainMsgLoop->SendMessage(pMsg);
    }
    
    TSK_DEBUG_INFO ("== init");
    
    return YOUME_SUCCESS;
    
bail_out:
    //    {
    //        // 新的数据上报 初始化失败
    //        ReportService * report = ReportService::getInstance();
    //        ReportCommon init;
    //        init.common_type = REPORT_COMMON_INIT;
    //        init.result = errCode;
    //		init.sdk_version = SDK_NUMBER;
    //        report->report(init);
    //    }
    
    setState(STATE_INIT_FAILED);
    TSK_DEBUG_INFO ("== init failed");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::unInit ()
{
    TSK_DEBUG_INFO ("@@ unInit");
    
    std::unique_lock<std::recursive_mutex> stateLock(mStateMutex);
    // If the unInit is in progress, just return.
    if (mIsAboutToUninit || (STATE_UNINITIALIZED == mState)) {
        TSK_DEBUG_ERROR ("== state:%s, mIsAboutToUninit:%d", stateToString(mState), (int)mIsAboutToUninit);
        return YOUME_ERROR_WRONG_STATE;
    }
    mIsAboutToUninit = true;
    TSK_DEBUG_INFO ("Is about to uninit...");
    stateLock.unlock();
    
    // Send message to leave the conference as soon as possible.
    mIsWaitingForLeaveAll = true;
    if (!leaveConfForUninit ())
    {
        mIsWaitingForLeaveAll = false;
    }

    CSDKValidate::GetInstance()->Abort();
    
    TSK_DEBUG_INFO ("Waiting for state to idle...");
    // If it's initializing when calling unInit(), it may becomes STATE_INIT_FAILED(if failed) or STATE_INITIALIZED
    // (if successful). If it's still in channel, waiting for mIsWaitingForLeaveAll to be false.
    for (int i = 0; i < 50; i++) {
        if ((STATE_INITIALIZING != mState) && !mIsWaitingForLeaveAll) {
            break;
        }
        XSleep(100);
    }
    TSK_DEBUG_INFO ("Waiting for state to idle OK");
    
    {
        // 新的数据上报
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportCommon uninit;
        uninit.common_type = REPORT_COMMON_UNINIT;
        uninit.result = 0;
        stringstream ss;
        ss<<(tsk_time_now() - m_ulInitStartTime);
        uninit.param = ss.str();
        uninit.sdk_version = SDK_NUMBER;
        uninit.brand = NgnApplication::getInstance()->getBrand();
        uninit.model = NgnApplication::getInstance()->getModel();
        uninit.platform = NgnApplication::getInstance()->getPlatform();
        uninit.canal_id = NgnApplication::getInstance()->getCanalID();
        uninit.package_name = NgnApplication::getInstance()->getPackageName();
        report->report(uninit);
    }
    
    {
        //上报退出时，本次相关的各种数据
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportQuit quit;
        quit.total_usetime = tsk_time_now() - m_ulInitStartTime;
        quit.dns_count = ReportQuitData::getInstance()->m_dns_count;
        quit.valid_count = ReportQuitData::getInstance()->m_valid_count;
        quit.redirect_count = ReportQuitData::getInstance()->m_redirect_count;
        quit.login_count = ReportQuitData::getInstance()->m_login_count;
        quit.join_count = ReportQuitData::getInstance()->m_join_count;
        quit.leave_count = ReportQuitData::getInstance()->m_leave_count;
        quit.kickout_count = ReportQuitData::getInstance()->m_kickout_count;
        quit.uploadlog_count = ReportQuitData::getInstance()->m_uploadlog_count;
        quit.sdk_version = SDK_NUMBER;
        quit.platform = NgnApplication::getInstance()->getPlatform();
        quit.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report( quit );
        
    }
    
    stop ();
    CXTimer::GetInstance ()->UnInit ();
    NgnEngine::getInstance()->stop();
    NgnEngine::destroy ();
    
    // finally stop message loop
    if (m_pMainMsgLoop) {
        m_pMainMsgLoop->Stop();
    }
    if (m_pCbMsgLoop) {
        m_pCbMsgLoop->Stop();
    }
    if (m_pWorkerMsgLoop) {
        m_pWorkerMsgLoop->Stop();
    }
    
    m_pcmCallbackLoopMutex.lock();
    if (m_pPcmCallbackLoop) {
        m_pPcmCallbackLoop->Stop();
        delete m_pPcmCallbackLoop;
        m_pPcmCallbackLoop = NULL;
    }
    m_pcmCallbackLoopMutex.unlock();
    
    if (m_pRoomMgr) {
        delete m_pRoomMgr;
        m_pRoomMgr = NULL;
    }
    
    SAFE_DELETE(m_RoomPropMgr);
    
    if( m_queryHttpThread.joinable() ){
        m_bExitHttpQuery = true;
        m_httpSemaphore.Increment();
        m_queryHttpThread.join();
    }
    
    if (m_translateThread.joinable())
    {
        m_bTranslateThreadExit = true;
        m_translateSemaphore.Increment();
        m_translateThread.join();
    }
    
    if( g_ymvideo_pTranslateUtil )
    {
        delete g_ymvideo_pTranslateUtil;
        g_ymvideo_pTranslateUtil = nullptr;
    }
    
    
    setState(STATE_UNINITIALIZED);
    mIsAboutToUninit = false;
    CSDKValidate::GetInstance()->Reset();
    TSK_DEBUG_INFO ("== unInit");
    return YOUME_SUCCESS;
}


YouMeErrorCode CYouMeVoiceEngine::requestRestApi(  const std::string& strCommand, const std::string& strQueryBody ,  int* requestID ){
    static atomic<int> nextRequestID(0);
    int curRequestID = nextRequestID;
    nextRequestID++;
    if( requestID != NULL ){
        *requestID = curRequestID;
    }
    
    TSK_DEBUG_INFO ("@@ requestRestApi id:%d, command:%s, body:%s", curRequestID, strCommand.c_str(), strQueryBody.c_str() );
    
    if ( strCommand.empty() || strQueryBody.empty() )
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiQueryHttpInfo);
        if (pMsg) {
            if (NULL == pMsg->m_param.apiQueryHttpInfo.pQueryCommond || NULL == pMsg->m_param.apiQueryHttpInfo.pQueryBody ) {
                delete pMsg;
                pMsg = NULL;
                return YOUME_ERROR_MEMORY_OUT;
            }
            
            *(pMsg->m_param.apiQueryHttpInfo.pQueryCommond) = strCommand;
            *(pMsg->m_param.apiQueryHttpInfo.pQueryBody) = strQueryBody;
            pMsg->m_param.apiQueryHttpInfo.requestID = curRequestID;
            
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO ("@@ requestRestApi");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO ("@@ requestRestApi failed");
    return YOUME_ERROR_MEMORY_OUT;
}

std::string CYouMeVoiceEngine::getHttpHost(){
    if( g_serverMode != SERVER_MODE_FORMAL ){
        return "https://test3api.youme.im";
    }
    else{
        switch (g_serverRegionId) {
            case RTC_CN_SERVER:
                return "https://api.youme.im";
                break;
            case RTC_SG_SERVER:
                return "https://sgapi.youme.im";
                break;
            case RTC_HK_SERVER:
                return "https://hkapi.youme.im";
                break;
            case RTC_US_SERVER:
            case RTC_USW_SERVER:
            case RTC_USM_SERVER:
                return "https://usapi.youme.im";
                break;
            default:
                return "https://api.youme.im";
                break;
        }
    }
}

void CYouMeVoiceEngine::setLogLevel( YOUME_LOG_LEVEL consoleLevel, YOUME_LOG_LEVEL fileLevel)
{
    m_bSetLogLevel = true;
    tsk_set_log_maxLevelConsole((int)consoleLevel);
    tsk_set_log_maxLevelFile((int)fileLevel);
    TSK_DEBUG_INFO("@@== setLogLevel consoleLevel:%d, fileLevel:%d", consoleLevel, fileLevel);
}

YouMeErrorCode CYouMeVoiceEngine::setExternalInputSampleRate( YOUME_SAMPLE_RATE inputSampleRate, YOUME_SAMPLE_RATE mixedCallbackSampleRate )
{
    TSK_DEBUG_INFO("@@ setExternalInputSampleRate inputSampleRate:%d, mixedCallbackSampleRate:%d", inputSampleRate, mixedCallbackSampleRate);
    
    if(!MediaSessionMgr::defaultsGetExternalInputMode()){
        TSK_DEBUG_ERROR("== setExternalInputSampleRate is not external input mode, return");
        return YOUME_ERROR_WRONG_STATE;
    }
    switch ( inputSampleRate) {
        case SAMPLE_RATE_8:
        case SAMPLE_RATE_16:
        case SAMPLE_RATE_24:
        case SAMPLE_RATE_32:
        case SAMPLE_RATE_44:
        case SAMPLE_RATE_48:
        {
            m_inputSampleRate = inputSampleRate;
        }
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
    }
    switch ( mixedCallbackSampleRate) {
        case SAMPLE_RATE_8:
        case SAMPLE_RATE_16:
        case SAMPLE_RATE_24:
        case SAMPLE_RATE_32:
        case SAMPLE_RATE_44:
        case SAMPLE_RATE_48:
        {
            m_mixedCallbackSampleRate = mixedCallbackSampleRate;
            MediaSessionMgr::defaultsSetMixedCallbackSamplerate(mixedCallbackSampleRate);
        }
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
    }
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoSmooth(int enable)
{
    TSK_DEBUG_INFO("@@== setVideoSmooth: %d", enable );
    if (enable < 0 || enable > 1) {
        TSK_DEBUG_INFO("@@ invalid param, enable:%d", enable );
        return YOUME_ERROR_INVALID_PARAM;
    }

    MediaSessionMgr::defaultsSetVideoNackFlag( enable );
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoNetAdjustmode(int mode)
{
    TSK_DEBUG_INFO("@@== setVideoNetAdjustmode: mode:%d", mode );
    if (mode < 0 || mode > 1) {
        TSK_DEBUG_INFO("@@ invalid param, mode:%d", mode );
        return YOUME_ERROR_INVALID_PARAM;
    }

    MediaSessionMgr::setVideoNetAdjustMode( mode );
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoNetResolution( int width, int height )
{
    TSK_DEBUG_INFO("@@ setVideoNetResolution: width:%d, height:%d", width, height );
    
    if (width <= 0 || height <= 0) {
        TSK_DEBUG_ERROR("== invalid param" );
        return YOUME_ERROR_INVALID_PARAM;
    }

    if( width % 2 != 0 )
    {
        width += 1;
    }
    
    if( height % 2 != 0 )
    {
        height += 1 ;
    }

    MediaSessionMgr::setVideoNetResolution( width, height );
    YouMeVideoMixerAdapter::getInstance()->setVideoNetResolutionWidth(width, height);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoNetResolutionForSecond( int width, int height )
{
    TSK_DEBUG_INFO("@@== setVideoNetResolutionForSecond: width:%d, height:%d", width, height );

    if (width <= 0 || height <= 0) {
        TSK_DEBUG_ERROR("== invalid param" );
        return YOUME_ERROR_INVALID_PARAM;
    }

    if( width % 2 != 0 )
    {
        width += 1;
    }
    
    if( height % 2 != 0 )
    {
        height += 1 ;
    }
    
    MediaSessionMgr::setVideoNetResolutionForSecond( width, height );
    YouMeVideoMixerAdapter::getInstance()->setVideoNetResolutionWidthForSecond(width, height);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::switchResolutionForLandscape()
{
	TSK_DEBUG_INFO("@@== switchResolutionForLandscape");
	lock_guard<recursive_mutex> stateLock(mStateMutex);
	if (!isStateInitialized()) {
		TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
		return YOUME_ERROR_WRONG_STATE;
	}
	uint32_t width = 0;
	uint32_t height = 0;
	MediaSessionMgr::getVideoNetResolution(width, height);

	uint32_t width2 = 0;
	uint32_t height2 = 0;
	MediaSessionMgr::getVideoNetResolutionForSecond(width2, height2);
    
    uint32_t widthLocal = 0;
    uint32_t heightLocal = 0;
    MediaSessionMgr::getVideoLocalResolution(widthLocal, heightLocal);
	
	if (width < height){
        setVideoLocalResolution(heightLocal, widthLocal);
		setVideoNetResolution(height, width);
		if(width2 > 0 && height2 >0) setVideoNetResolutionForSecond(height2, width2);
	}
	else
	{
		return YOUME_ERROR_NOT_PROCESS;
	}
	
	return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::resetResolutionForPortrait()
{
	TSK_DEBUG_INFO("@@== resetResolutionForPortrait");
	lock_guard<recursive_mutex> stateLock(mStateMutex);
	if (!isStateInitialized()) {
		TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
		return YOUME_ERROR_WRONG_STATE;
	}
	uint32_t width = 0;
	uint32_t height = 0;
	MediaSessionMgr::getVideoNetResolution(width, height);

	uint32_t width2 = 0;
	uint32_t height2 = 0;
	MediaSessionMgr::getVideoNetResolutionForSecond(width2, height2);
    
    uint32_t widthLocal = 0;
    uint32_t heightLocal = 0;
    MediaSessionMgr::getVideoLocalResolution(widthLocal, heightLocal);

	if (height < width){
        setVideoLocalResolution(heightLocal, widthLocal);
		setVideoNetResolution(height, width);
		if (width2 > 0 && height2 >0) setVideoNetResolutionForSecond(height2, width2);
	}
	else
	{
		return YOUME_ERROR_NOT_PROCESS;
	}

	return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::translateText(unsigned int* requestID, const char* text, YouMeLanguageCode destLangCode, YouMeLanguageCode srcLangCode)
{
    TSK_DEBUG_INFO("@@== translateText");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if( text == nullptr || std::string(text).empty() )
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    int nTranslateSwitch = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::CONFIG_TRANSLATE_ENABLE, NgnConfigurationEntry::TRANSLATE_SWITCH_DEFAULT);
    if (!nTranslateSwitch)
    {
        return YOUME_ERROR_API_NOT_ALLOWED;
    }
    
    std::map<YouMeLanguageCode, XString>::const_iterator itr = m_langCodeMap.find(destLangCode);
    if (itr == m_langCodeMap.end())
    {
        return YOUME_ERROR_NO_LANG_CODE;
    }
    
    std::map<YouMeLanguageCode, XString>::const_iterator itrSrc = m_langCodeMap.find(srcLangCode);
    if (itrSrc == m_langCodeMap.end())
    {
        return YOUME_ERROR_NO_LANG_CODE;
    }
    
    TranslateInfo info;
    info.srcLangCode = srcLangCode;
    info.destLangCode = destLangCode;
    info.text = text;
    info.requestID = ++m_iTranslateRequestID;
    if (requestID != NULL)
    {
        *requestID = info.requestID;
    }
    {
        std::lock_guard<std::mutex> lock(m_translateLock);
        m_translateInfoList.push_back(info);
    }
    m_translateSemaphore.Increment();
    
    return YOUME_SUCCESS;
    
}

void CYouMeVoiceEngine::TranslateThread()
{
    TSK_DEBUG_INFO("enter");
    while (m_translateSemaphore.Decrement())
    {
        if (m_bTranslateThreadExit)
        {
            break;
        }
        
        TranslateInfo info;
        {
            std::lock_guard<std::mutex> lock(m_translateLock);
            info = m_translateInfoList.front();
            m_translateInfoList.pop_front();
        }
        
//        TSK_DEBUG_INFO("try translate %s, from :%d, to :%d ", info.text.c_str(), info.destLangCode, info.srcLangCode );
        
        std::map<YouMeLanguageCode, XString>::const_iterator itrDestLang = m_langCodeMap.find(info.destLangCode);
        std::map<YouMeLanguageCode, XString>::const_iterator itrSrcLang = m_langCodeMap.find(info.srcLangCode);
        if (itrDestLang == m_langCodeMap.end() || itrSrcLang == m_langCodeMap.end())
        {
            continue;
        }
        
        
        XString strSrcLanguage;
        int nTranslateMethod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CONFIG_TRANSLATE_METHOD, NgnConfigurationEntry::TRANSLATE_METHOD_DEFAULT);
        XString strResult;
        if (!nTranslateMethod)
        {
            strResult = g_ymvideo_pTranslateUtil->Translate( UTF8TOXString(info.text), itrSrcLang->second, itrDestLang->second, strSrcLanguage);
        }
        else
        {
            strResult = g_ymvideo_pTranslateUtil->TranslateV2( UTF8TOXString(info.text), itrSrcLang->second, itrDestLang->second, strSrcLanguage);
        }
        
//        TSK_DEBUG_INFO("try translate nTranslateMethod:%d, result:%s", nTranslateMethod, XStringToUTF8(strResult).c_str() );
        
        if (m_pTranslateCallback)
        {
            std::transform(strSrcLanguage.begin(), strSrcLanguage.end(), strSrcLanguage.begin(), ::tolower);
            YouMeLanguageCode srcLanguageCode = info.srcLangCode;
            if (srcLanguageCode == LANG_AUTO)
            {
                for (std::map<YouMeLanguageCode, XString>::const_iterator itr = m_langCodeMap.begin(); itr != m_langCodeMap.end(); ++itr)
                {
                    if (itr->second == strSrcLanguage)
                    {
                        srcLanguageCode = itr->first;
                        break;
                    }
                }
            }
            m_pTranslateCallback->onTranslateTextComplete(strResult.empty() ? YOUME_ERROR_NETWORK_ERROR : YOUME_SUCCESS, info.requestID, XStringToUTF8(strResult), srcLanguageCode, info.destLangCode);
        }
        
        // count character
        XString strNewWord = CStringUtil::replace_text( UTF8TOXString(info.text), __XT("\r"), __XT(""));

        std::vector<unsigned long long> u16_cvt_str;
#ifdef WIN32
        for (int i = 0; i < strNewWord.size(); i++)
        {
            u16_cvt_str.push_back(strNewWord.at(i));
        }
#else
        ::youmecommon::utf8to16(strNewWord.begin(), strNewWord.end(), std::back_inserter(u16_cvt_str));
#endif // WIN32

        XUINT64 chaterCount = 0;
        if (strResult == __XT(""))
        {
            chaterCount = 1;
        }
        else
        {
            chaterCount = (XUINT64)u16_cvt_str.size();
        }

        ReportTranslateStatus(strResult.empty() ? 1 : 0, itrSrcLang->second, itrDestLang->second, chaterCount, nTranslateMethod);
    }
    TSK_DEBUG_INFO("leave");
}

void CYouMeVoiceEngine::ReportTranslateStatus(short status, const XString& srcLanguage, const XString& destLanguage, XUINT64 chaterCount, int translateMethod)
{
    ReportService * report = ReportService::getInstance();
    youmeRTC::ReportTranslateInfo reportData;

    reportData.SDKVersion = SDK_NUMBER;
    reportData.status = status;
    reportData.srcLanguage = XStringToUTF8(srcLanguage);
    reportData.destLanguage = XStringToUTF8(destLanguage);
    reportData.characterCount = chaterCount;
    reportData.translateVersion = translateMethod;
    
    report->report( reportData );
}

YouMeErrorCode CYouMeVoiceEngine::setVideoNetResolutionForShare( int width, int height )
{
    TSK_DEBUG_INFO("@@== setVideoNetResolutionForShare: width:%d, height:%d", width, height );

    if (width <= 0 || height <= 0) {
        TSK_DEBUG_ERROR("== invalid param" );
        return YOUME_ERROR_INVALID_PARAM;
    }
        
    if( width % 2 != 0 )
    {
        width += 1;
    }
    
    if( height % 2 != 0 )
    {
        height += 1 ;
    }
    
    shareWidth = width;
    shareHeight = height;
    MediaSessionMgr::setVideoNetResolutionForShare( width, height );
    // YouMeVideoMixerAdapter::getInstance()->setVideoNetResolutionWidthForSecond(width, height);

    #if TARGET_OS_IPHONE || TARGET_OS_OSX
    ICameraManager::getInstance()->setShareVideoEncResolution( width, height );
    #endif

    #ifdef ANDROID
        JNI_screenRecorderSetResolution(width, height);
    #endif

    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::setAVStatisticInterval( int interval  )
{
    TSK_DEBUG_INFO("@@== setAVStatisticInterval:%d", interval);
    AVStatistic::getInstance()->setInterval( interval );
}

void CYouMeVoiceEngine::setRestApiCallback(IRestApiCallback* cb ){
    TSK_DEBUG_INFO("@@== set cb :%p",cb);
    mYouMeApiCallback = cb;
}

void CYouMeVoiceEngine::setMemberChangeCallback( IYouMeMemberChangeCallback* cb ){
    TSK_DEBUG_INFO("@@== set cb :%p", cb);
    mMemChangeCallback = cb;
}

void CYouMeVoiceEngine::setAVStatisticCallback( IYouMeAVStatisticCallback* cb )
{
    TSK_DEBUG_INFO("@@== set cb :%p",cb);
    mAVStatistcCallback = cb;
}

void CYouMeVoiceEngine::setNotifyCallback(IYouMeChannelMsgCallback* cb)
{
    TSK_DEBUG_INFO("@@== set cb :%p",cb);
    mNotifyCallback = cb;
}

std::string CYouMeVoiceEngine::getQueryHttpUrl( std::string& strCommand ){
    std::stringstream ss;
    auto curtime = time(0);
    std::stringstream ssCheck;
    ssCheck<<mAppSecret<<curtime;
    
    XString checkSum = youmecommon::CCryptUtil::SHA1( UTF8TOXString(ssCheck.str()) );
    
    ss<< getHttpHost()<<"/";
    ss<<"v1/im/"<<strCommand<<"?";
    ss<<"appkey="<<mAppKey<<"&";
    if( mStrUserID.empty() ){
        ss<<"identifier="<<"unlogin"<<"&";
    }
    else{
        ss<<"identifier="<<mStrUserID<<"&";
    }
    ss<<"curtime="<<curtime<<"&";
    ss<<"checksum="<<XStringToUTF8(checkSum);
    
    return ss.str();
}

void CYouMeVoiceEngine::QueryHttpInfoThreadProc(){
    //todo:这里，没消息的时候要睡眠
    while( m_httpSemaphore.Decrement() ){
        
        if(m_bExitHttpQuery){
            break;
        }
        
        QueryHttpInfo queryInfo;
        {
            std::lock_guard<std::mutex> lock(m_queryHttpInfoLock);
            if( m_queryHttpList.empty() ){
                continue;
            }
            
            queryInfo = m_queryHttpList.front();
            m_queryHttpList.pop_front();
        }
        
        std::string strCommand = queryInfo.strCommand;
        std::string strBody = queryInfo.strQueryBody;
        std::string strResponse;    //服务器返回
        std::string strResult;      //查询的数据结果
        
        youmecommon::Value  jsonRootQuery;
        youmecommon::Value  jsonCommond = youmecommon::Value( strCommand.c_str() );
        youmecommon::Value  jsonBody = youmecommon::Value( strBody.c_str() );
        jsonRootQuery["command"] = jsonCommond;
        jsonRootQuery["query"] = jsonBody;
        std::string strQuery = XStringToUTF8(jsonRootQuery.toSimpleString() );
        
        std::map<std::string, std::string> mapHead;
        mapHead["Content-Type"] = "application/json";
        std::stringstream ss;
        ss<<strBody.length();
        mapHead["Content-Length"] = ss.str();
        std::string strUrl = getQueryHttpUrl( strCommand );
        
        YouMeErrorCode errorcode = YOUME_SUCCESS;
        
        bool bSuccess = CDownloadUploadManager::HttpRequest( UTF8TOXString(strUrl) , strBody, strResponse, true, 10, &mapHead);
        if(bSuccess){
            youmecommon::Value  jsonRoot;
            youmecommon::Reader reader;
            
            bool bRet = reader.parse( strResponse,  jsonRoot );
            
            if( bRet  &&
               jsonRoot.isMember("ActionStatus") &&
               jsonRoot.isMember("ErrorCode") &&
               jsonRoot.isMember("ErrorInfo") ){
                
                std::string strStatus = jsonRoot["ActionStatus"].asString();
                int err = jsonRoot["ErrorCode"].asInt();
                std::string errInfo = jsonRoot["ErrorInfo"].asString();
                strResult = XStringToUTF8(jsonRoot.toSimpleString());
                
                if( strStatus == "OK" && err == 0 ){
                    CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbHttpInfo);
                    if (pMsg) {
                        *(pMsg->m_param.cbHttpInfo.pJsonData)= strResult;
                        *(pMsg->m_param.cbHttpInfo.pQuery) = strQuery;
                        pMsg->m_param.cbHttpInfo.requestID = queryInfo.requestID;
                        pMsg->m_param.cbHttpInfo.errCode = YOUME_SUCCESS;
                        m_pCbMsgLoop->SendMessage(pMsg);
                    }
                }
                else{
                    errorcode = YOUME_ERROR_QUERY_RESTAPI_FAIL;
                    TSK_DEBUG_INFO ("requestRestApi,result err:%d,reason:%s", err, errInfo.c_str() );
                }
            }
            else{
                errorcode = YOUME_ERROR_QUERY_RESTAPI_FAIL;
                TSK_DEBUG_INFO ("requestRestApi,wrong json:%s", strResponse.c_str() );
            }
        }
        else{
            errorcode = YOUME_ERROR_QUERY_RESTAPI_FAIL;
            TSK_DEBUG_INFO ("requestRestApi,  failed" );
        }
        
        
        if( errorcode != 0 ){
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbHttpInfo);
            if (pMsg) {
                *(pMsg->m_param.cbHttpInfo.pJsonData)= strResult;
                *(pMsg->m_param.cbHttpInfo.pQuery) = strQuery;
                pMsg->m_param.cbHttpInfo.errCode = errorcode;
                pMsg->m_param.cbHttpInfo.requestID = queryInfo.requestID;
                m_pCbMsgLoop->SendMessage(pMsg);
            }
        }
    }
}
bool CYouMeVoiceEngine::leaveConfForUninit()
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    // Now we are in STATE_TERMINATING_LAST, no more message will be allowed to be sent into the main loop.
    // So clearing it will make sure the MsgApiLeaveConfAll will be handled immediately.
    if (m_pMainMsgLoop) {
        m_pMainMsgLoop->ClearMessageQueue();
    }
    
    CSDKValidate::GetInstance()->Abort();
    m_loginService.Abort();
    m_workerThreadCondWait.SetSignal();
    m_reconnctWait.SetSignal();
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiLeaveConfAll);
        if (NULL == pMsg) {
            goto bail;
        }
        TSK_DEBUG_INFO ("Sending message to leave conf for uninit");
        pMsg->m_param.bTrue = false; // Indicate we don't need a callback when it's done with MsgApiLeaveConfAll
        m_pMainMsgLoop->SendMessage(pMsg);
        return true;
    }
    
bail:
    TSK_DEBUG_ERROR("Failed to send message to leave conference");
    return false;
}

void CYouMeVoiceEngine::setServerRegion(YOUME_RTC_SERVER_REGION serverRegionId, const std::string &extServerRegionName, bool bAppend)
{
    TSK_DEBUG_INFO ("@@ setServerRegion, regionId:%d, extRegionName:%s, bAppend:%d", serverRegionId, extServerRegionName.c_str(), (int)bAppend);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR ("== setServerRegion not inited");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetServerRegion);
        if (NULL == pMsg) {
            goto bail;
        }
        if (NULL == pMsg->m_param.apiSetServerRegion.pStrRegionName) {
            delete pMsg;
            pMsg = NULL;
            goto bail;
        }
        pMsg->m_param.apiSetServerRegion.regionId = serverRegionId;
        *(pMsg->m_param.apiSetServerRegion.pStrRegionName) = extServerRegionName;
        pMsg->m_param.apiSetServerRegion.bAppend = bAppend;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== setServerRegion");
        return;
    }
    
bail:
    TSK_DEBUG_ERROR ("== setServerRegion failed to send message");
}


YouMeErrorCode CYouMeVoiceEngine::start ()
{
    TSK_DEBUG_INFO ("@@ start");
    
    bool bRet = NgnEngine::getInstance ()->start ();
	TSK_DEBUG_INFO("== start");
    if (false == bRet)
    {
        return YOUME_ERROR_START_FAILED;
    }
    else
    {
        return YOUME_SUCCESS;
    }
  
}

YouMeErrorCode CYouMeVoiceEngine::stop ()
{
    
    TSK_DEBUG_INFO ("@@ stop");
    bool bRet = NgnEngine::getInstance ()->stop ();
    if (false == bRet)
    {
        return YOUME_ERROR_STOP_FAILED;
    }
    else
    {
        return YOUME_SUCCESS;
    }
    TSK_DEBUG_INFO ("== stop");
}

/**
 *  功能描述:切换语音输出设备
 *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
 *
 *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
YouMeErrorCode CYouMeVoiceEngine::setOutputToSpeaker (bool bOutputToSpeaker)
{
    TSK_DEBUG_INFO ("@@ setOutputToSpeaker:%d", bOutputToSpeaker);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized() || NULL == m_avSessionMgr) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetOutputToSpeaker);
        if (pMsg) {
            pMsg->m_param.bTrue = bOutputToSpeaker;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setOutputToSpeaker");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setOutputToSpeaker failed");
    return YOUME_ERROR_MEMORY_OUT;
}

/**
 *  功能描述:扬声器静音打开/关闭
 *
 *  @param bOn:true——打开，false——关闭
 *  @return 无
 */
YouMeErrorCode CYouMeVoiceEngine::setSpeakerMute (bool mute)
{
    TSK_DEBUG_INFO("@@ setSpeakerMute:%d", (int32_t)mute);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSpeakerMute);
        if (pMsg) {
            pMsg->m_param.bTrue = mute;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setSpeakerMute");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setSpeakerMute delayed");
    return YOUME_ERROR_MEMORY_OUT;
}

void CYouMeVoiceEngine::setAutoSendStatus( bool bAutoSend ){
    
    TSK_DEBUG_INFO("@@ setAutoSendStatus:%d", bAutoSend);
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetAutoSendStatus);
        if (pMsg) {
            pMsg->m_param.bTrue = bAutoSend;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setAutoSendStatus");
            return ;
        }
    }
    
    TSK_DEBUG_INFO("== setAutoSendStatus failed");
    return ;
}

/**
 *  功能描述:获取扬声器静音状态
 *
 *  @return true——打开，false——关闭
 */
bool CYouMeVoiceEngine::getSpeakerMute ()
{
    bool ret = mPPcmCallback && !mPcmOutputToSpeaker ? false : m_bSpeakerMute;
    TSK_DEBUG_INFO("@@== getSpeakerMute:%d", (int32_t)ret);
    return ret;
}

bool CYouMeVoiceEngine::isMicrophoneMute ()
{
    TSK_DEBUG_INFO("@@== isMicrophoneMute:%d", (int32_t)m_bMicMute);
    return m_bMicMute;
}

YouMeErrorCode CYouMeVoiceEngine::setMicrophoneMute (bool mute)
{
    TSK_DEBUG_INFO("@@ setMicrophoneMute:%d", (int32_t)mute);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetMicMute);
        if (pMsg) {
            pMsg->m_param.bTrue = mute;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setMicrophoneMute");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setMicrophoneMute delayed");
    return YOUME_ERROR_MEMORY_OUT;
}

unsigned int CYouMeVoiceEngine::getVolume ()
{
    TSK_DEBUG_INFO("@@== getVolume:%u", m_nVolume);
    return m_nVolume;
}

void CYouMeVoiceEngine::setVolume (const unsigned int &uiVolume)
{
    TSK_DEBUG_INFO("@@ setVolume:%d", uiVolume);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    if (uiVolume > 100) {
        TSK_DEBUG_ERROR ("== setVolume: Invalid parameter");
        return;
    }
    m_nVolume = uiVolume;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetVolume);
        if (pMsg) {
            pMsg->m_param.u32Value = uiVolume;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setVolume");
            return;
        }
    }
    
    TSK_DEBUG_INFO("== setVolume delayed");
}

YouMeErrorCode CYouMeVoiceEngine::joinChannelSingleMode(const std::string & strUserID, const std::string & strChannelID
                                                    , YouMeUserRole_t eUserRole, const std::string & strJoinAppKey, bool bVideoAutoRecv)
{
    setJoinChannelKey( strJoinAppKey );
    return joinChannelSingleMode(strUserID, strChannelID, eUserRole, bVideoAutoRecv);
}

YouMeErrorCode CYouMeVoiceEngine::joinChannelSingleMode(const std::string & strUserID, const std::string & strChannelID
                                                    , YouMeUserRole_t eUserRole, bool bVideoAutoRecv)
{
    TSK_DEBUG_INFO ("@@ joinChannelSingleMode");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (ROOM_MODE_NONE == m_roomMode) {
        TSK_DEBUG_INFO("######## Fixed in single room mode ########");
        m_roomMode = ROOM_MODE_SINGLE;
    }
    
    if (ROOM_MODE_SINGLE != m_roomMode) {
        TSK_DEBUG_ERROR("@@ joinChannelSingleMode: It's not in multi-room mode, call joinChannelMultiMode() instead");
        return YOUME_ERROR_WRONG_CHANNEL_MODE;
    }
    
    mTmpRole = eUserRole;
    //mRole = eUserRole;
    
    bool needMic = false;
    switch (eUserRole) {
        case YOUME_USER_TALKER_FREE:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_TALKER_ON_DEMAND:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_GUSET:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_LISTENER:
            needMic = false;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_COMMANDER:
        case YOUME_USER_HOST:
            needMic = true;
            mAllowPlayBGM = true;
            mAllowMonitor = true;
            break;
        default:
            TSK_DEBUG_ERROR("Invalid UserRole:%d", (int)eUserRole);
            return YOUME_ERROR_INVALID_PARAM;
    }
    
    return joinChannelProxy(strUserID, strChannelID, eUserRole, needMic, bVideoAutoRecv);
}

YouMeErrorCode CYouMeVoiceEngine::joinChannelMultiMode(const std::string & strUserID, const std::string & strChannelID, YouMeUserRole_t eUserRole)
{
    TSK_DEBUG_INFO ("@@ joinChannelMultiMode");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (ROOM_MODE_NONE == m_roomMode) {
        TSK_DEBUG_INFO("######## Fixed in multi room mode ########");
        m_roomMode = ROOM_MODE_MULTI;
    }
    
    if (ROOM_MODE_MULTI != m_roomMode) {
        TSK_DEBUG_ERROR("@@ joinChannelMultiMode: It's in single-room mode, call joinChannelSingleMode instead");
        return YOUME_ERROR_WRONG_CHANNEL_MODE;
    }
    
    mTmpRole = eUserRole;
    
    bool needMic = false;
    switch (eUserRole) {
        case YOUME_USER_TALKER_FREE:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_TALKER_ON_DEMAND:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_GUSET:
            needMic = true;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_LISTENER:
            needMic = false;
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_COMMANDER:
        case YOUME_USER_HOST:
            needMic = true;
            mAllowPlayBGM = true;
            mAllowMonitor = true;
            break;
        default:
            TSK_DEBUG_ERROR("Invalid UserRole:%d", (int)eUserRole);
            return YOUME_ERROR_INVALID_PARAM;
    }
    
    return joinChannelProxy(strUserID, strChannelID, eUserRole, needMic, true);
}

// This functions should not be called from outside, it should only be call by the API functions.
// mState should be locked before calling this function.
YouMeErrorCode CYouMeVoiceEngine::joinChannelProxy (const std::string& strUserID, const string &strChannelID,
                                                    YouMeUserRole_t eUserRole, bool needMic, bool bVideoAutoRecv)

{
    TSK_DEBUG_INFO ("@@ joinChannelProxy ChannelID:%s, UserID:%s, needMic:%d autoRecv:%d",
		strChannelID.c_str(), strUserID.c_str(), needMic, bVideoAutoRecv);
    
    if (strChannelID.empty()) {
        TSK_DEBUG_ERROR("== ChannelID is empty");
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (strUserID.empty()) {
        TSK_DEBUG_ERROR("== UserID is empty");
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    //校验一下房间ID，只允许字母，数字，下划线
    for (int i=0; i < strChannelID.length(); i++) {
        if (!IsValidChar(strChannelID.at(i))) {
            TSK_DEBUG_ERROR("== ChannelID is invalid");
            return YOUME_ERROR_INVALID_PARAM;
        }
    }
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    bool bUseMobileNetwork = getUseMobileNetWorkEnabled ();
    
    // This should be after the state checking, because mPNetworkService is only valid after init()
    if (this->mPNetworkService == NULL)
    {
        TSK_DEBUG_ERROR("== mPNetworkService is NULL");
        errCode = YOUME_ERROR_UNKNOWN;
        goto bail;
    }
    
    TSK_DEBUG_INFO ("UseMobileNetwork:%d, isMobileNetwork:%d", (int)bUseMobileNetwork, (int)mPNetworkService->isMobileNetwork());
    
    if ((!bUseMobileNetwork) && mPNetworkService->isMobileNetwork ())
    {
        TSK_DEBUG_ERROR("== Mobile network is not allowed");
        errCode = YOUME_ERROR_NOT_ALLOWED_MOBILE_NETWROK;
        goto bail;
    }
    
    if (m_pMainMsgLoop) {
        m_reconnctWait.Reset();
        
        CMessageBlock::MsgType msgType = CMessageBlock::MsgApiJoinConfSingle;
        if (ROOM_MODE_MULTI == m_roomMode) {
            msgType = CMessageBlock::MsgApiJoinConfMulti;
        }
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(msgType);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        if ((NULL == pMsg->m_param.apiJoinConf.pStrRoomID)
            || (NULL == pMsg->m_param.apiJoinConf.pStrUserID)) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        *(pMsg->m_param.apiJoinConf.pStrRoomID) = strChannelID;
        *(pMsg->m_param.apiJoinConf.pStrUserID) = strUserID;
        pMsg->m_param.apiJoinConf.eUserRole = eUserRole;
        pMsg->m_param.apiJoinConf.needMic = needMic;
        pMsg->m_param.apiJoinConf.videoAutoRecv = bVideoAutoRecv;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== joinChannelProxy");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
bail:
    TSK_DEBUG_INFO("== joinChannelProxy failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::leaveChannelMultiMode(const string& strChannelID)
{
    TSK_DEBUG_INFO ("@@ leaveChannel, ChannelID:%s", strChannelID.c_str());
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (ROOM_MODE_MULTI != m_roomMode) {
        TSK_DEBUG_ERROR("@@ leaveChannelMultiMode: It's in single-room mode, call leaveChannelAll instead");
        return YOUME_ERROR_WRONG_CHANNEL_MODE;
    }
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiLeaveConfMulti);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        if (NULL == pMsg->m_param.apiLeaveConf.pStrRoomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        *(pMsg->m_param.apiLeaveConf.pStrRoomID) = strChannelID;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== leaveChannel");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
bail:
    TSK_DEBUG_ERROR ("== leaveChannel failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::leaveChannelAll()
{
    TSK_DEBUG_INFO ("@@ leaveChannelAll");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR ("== not inited");
        return YOUME_ERROR_WRONG_STATE;
    }
    
    // Now we are in STATE_TERMINATING_LAST, no more message will be allowed to be sent into the main loop.
    // So clearing it will make sure the MsgApiLeaveConf will be handled immediately.
    if (m_pMainMsgLoop) {
        m_pMainMsgLoop->ClearMessageQueue();
    }
    
    CSDKValidate::GetInstance()->Abort();
    m_loginService.Abort();
    m_workerThreadCondWait.SetSignal();
    m_reconnctWait.SetSignal();

    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiLeaveConfAll);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        pMsg->m_param.bTrue = true;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== leaveChannelAll");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
bail:
    TSK_DEBUG_ERROR ("== leaveChannelAll failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::getChannelUserList( const char*  channelID, int maxCount, bool notifyMemChange ){
    TSK_DEBUG_INFO ("@@ getChannelUserList");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR ("== not inited");
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if( maxCount == 0 ){
        TSK_DEBUG_ERROR ("== max count == 0  ");
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    CRoomManager::RoomInfo_t roomInfo;
    if( !m_pRoomMgr->getRoomInfo(channelID, roomInfo)){
        TSK_DEBUG_ERROR ("== not in the room ");
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiGetChannelUserList);
        if (pMsg) {
            if (NULL == pMsg->m_param.apiGetUserList.pStrRoomID ) {
                delete pMsg;
                pMsg = NULL;
                return YOUME_ERROR_MEMORY_OUT;
            }
            
            *(pMsg->m_param.apiGetUserList.pStrRoomID) = roomInfo.idFull ;
            pMsg->m_param.apiGetUserList.maxCount = maxCount;
            pMsg->m_param.apiGetUserList.bNotifyMemChange = notifyMemChange ;
            
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO ("== getChannelUserList");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO ("== getChannelUserList failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::speakToChannel(const std::string & strChannelID)
{
    TSK_DEBUG_INFO ("@@ speakToChannel ChannelID:%s", strChannelID.c_str());
    
    if (strChannelID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== speakToConference wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (ROOM_MODE_MULTI != m_roomMode) {
        TSK_DEBUG_ERROR("== speakToConference : not multi-room mode");
        return YOUME_ERROR_API_NOT_SUPPORTED;
    }
    
    if (NULL == m_avSessionMgr) {
        TSK_DEBUG_INFO ("== speakToConference : m_avSessionMgr is NULL, channel not exist");
        return YOUME_ERROR_CHANNEL_NOT_EXIST;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiSpeakToConference);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        if (NULL == pMsg->m_param.apiSpeakToConference.pStrRoomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto bail;
        }
        *(pMsg->m_param.apiSpeakToConference.pStrRoomID) = strChannelID;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== speakToChannel");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
bail:
    TSK_DEBUG_INFO("== speakToChannel failed to send message");
    return errCode;
}

void CYouMeVoiceEngine::OnRoomEvent(const std::string& strRoomIDFull, RoomEventType eventType, RoomEventResult result)
{
    std::string strRoomID;
    removeAppKeyFromRoomId(strRoomIDFull, strRoomID);
    TSK_DEBUG_INFO("@@ OnRoomEvent, RoomID:%s, eventType:%d, result:%d", strRoomID.c_str(), eventType, result);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO("== OnRoomEvent, wrong state:%s", stateToString(mState));
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiRoomEvent);
        if (NULL == pMsg) {
            goto bail;
        }
        if (NULL == pMsg->m_param.apiRoomEvent.pStrRoomID) {
            delete pMsg;
            pMsg = NULL;
            goto bail;
        }
        *(pMsg->m_param.apiRoomEvent.pStrRoomID) = strRoomID;
        pMsg->m_param.apiRoomEvent.eventType = eventType;
        pMsg->m_param.apiRoomEvent.result = result;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== OnRoomEvent");
        return;
    }
    
bail:
    TSK_DEBUG_INFO("== OnRoomEvent failed to send message");
}

//通用状态事件通知
void CYouMeVoiceEngine::OnCommonStatusEvent(STATUS_EVENT_TYPE_t eventType, const std::string&strUserID, int iStauts)
{
    TSK_DEBUG_INFO ("@@ OnCommonStatusEvent:%d_%s_%d",eventType ,strUserID.c_str(), iStauts);
    
    YouMeEvent event;
    bool needNotity = true;
    
    switch (eventType) {
        case MIC_CTR_STATUS:
            //接收麦克风控制指令
            if (iStauts == 0) {
                event = YOUME_EVENT_MIC_CTR_ON;
                setMicrophoneMute(false);
                mNeedInputAudioFrame = true;
            } else {
                event = YOUME_EVENT_MIC_CTR_OFF;
                setMicrophoneMute(true);
                mNeedInputAudioFrame = false;
            }
            // setMicrophoneMute(iStauts!=0);
            break;
        case SPEAKER_CTR_STATUS:
            //接收扬声器控制指令
            if (iStauts == 0) {
                event = YOUME_EVENT_SPEAKER_CTR_ON;
                setSpeakerMute(false);
            } else {
                event = YOUME_EVENT_SPEAKER_CTR_OFF;
                setSpeakerMute(true);
            }
            //setSpeakerMute(iStauts!=0);
            break;
            
        case MIC_STATUS:
            if (iStauts == 0) {
                event = YOUME_EVENT_OTHERS_MIC_ON;
            } else {
                event = YOUME_EVENT_OTHERS_MIC_OFF;
            }
            break;
            
        case SPEAKER_STATUS:
            if (iStauts == 0) {
                event = YOUME_EVENT_OTHERS_SPEAKER_ON;
            } else {
                event = YOUME_EVENT_OTHERS_SPEAKER_OFF;
            }
            break;
            
        case AVOID_STATUS:
            if (iStauts == 0) {
                event = YOUME_EVENT_LISTEN_OTHER_ON;
            } else {
                event = YOUME_EVENT_LISTEN_OTHER_OFF;
            }
            break;

        case IDENTITY_MODIFY:
            // do nothing
            needNotity = false;
            break;
        default:
            // unknown event return?
            break;
    }
    
    if (needNotity) {
        sendCbMsgCallCommonStatus(event, strUserID, iStauts);
    }
    TSK_DEBUG_INFO("== OnCommonStatusEvent");
}
////接收成员麦克风状态控制指令
//void CYouMeVoiceEngine::OnReciveMicphoneControllMsg(const std::string&strUserID,int iStauts)
//{
//    TSK_DEBUG_INFO ("OnReciveMicphoneControllMsg:%s_%d", strUserID.c_str(), iStauts);
//    setMicrophoneMute(iStauts!=0);
//    if (NULL != mPConferenceCallback) {
//        mPConferenceCallback->onMicControllStatusEvent(strUserID, iStauts);
//    }
//}

////接收成员扬声器状态控制指令
//void CYouMeVoiceEngine::OnReciveSpeakerControllMsg(const std::string&strUserID,int iStauts)
//{
//    TSK_DEBUG_INFO ("OnReciveSpeakerControllMsg:%s_%d", strUserID.c_str(), iStauts);
//    setSpeakerMute(iStauts!=0);
//    if (NULL != mPConferenceCallback) {
//        mPConferenceCallback->onSpeakerControllStatusEvent(strUserID, iStauts);
//    }
//}

// 视频流到来通知
void CYouMeVoiceEngine::OnReciveOtherVideoDataOnNotify(const std::string &strUserIDs, const std::string & strRoomId,uint16_t videoID, uint16_t width, uint16_t height)
{
    TSK_DEBUG_INFO("@@ OnReciveVideoDataOnNotify: %s, roomid: %s", strUserIDs.c_str(), strRoomId.c_str());
	OnOtherVideoInputStatusChgNfy(strUserIDs, 1, false, videoID);
    sendCbMsgCallOtherVideoDataOn(strUserIDs, strRoomId, width, height);
    TSK_DEBUG_INFO("== OnReciveVideoDataOnNotify");
}


//// 视频流断开通知
//void CYouMeVoiceEngine::OnReciveOtherVideoDataOffNotify(const std::string &strUserIDs, const std::string & strRoomId)
//{
//    TSK_DEBUG_INFO("@@ OnReciveVideoDataOffNotify: %s, roomid: %s", strUserIDs.c_str(), strRoomId.c_str());
//    sendCbMsgCallOtherVideoDataOff(strUserIDs, strRoomId);
//    TSK_DEBUG_INFO("== OnReciveVideoDataOffNotify");
//}

//// 摄像头状态改变通知
//void CYouMeVoiceEngine::OnReciveOtherCameraStatusChgNotify(const std::string &strUserIDs, const std::string & strRoomId, int status)
//{
//    TSK_DEBUG_INFO("@@ OnReciveOtherCameraStatusChgNotify: %s, roomid: %s, status: %d", strUserIDs.c_str(), strRoomId.c_str(), status);
//    sendCbMsgCallOtherCameraStatus(strUserIDs, strRoomId, status);
//    TSK_DEBUG_INFO("== OnReciveOtherCameraStatusChgNotify");
//}

void CYouMeVoiceEngine::OnReceiveSessionUserIdPair(const SessionUserIdPairVector_t &idPairVector)
{
    TSK_DEBUG_INFO ("@@ %s", __FUNCTION__);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiOnReceiveSessionUserIdPair);
        if (NULL == pMsg) {
            return;
        }
        if (NULL == pMsg->m_param.apiOnReceiveSessionUserIdPair.pIdPairVector) {
            delete pMsg;
            pMsg = NULL;
            return;
        }
        *(pMsg->m_param.apiOnReceiveSessionUserIdPair.pIdPairVector) = idPairVector;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== %s", __FUNCTION__);
        return;
    }
    TSK_DEBUG_INFO ("== %s failed", __FUNCTION__);
}

void CYouMeVoiceEngine::OnMemberChange( const std::string& strRoomID, std::list<MemberChangeInner>& listMemberChange, bool bUpdate ) {
    std::string roomShort ="";
    removeAppKeyFromRoomId( strRoomID, roomShort );
    
    TSK_DEBUG_INFO ("@@ OnMemberChange:%s, bUpdate:%d", roomShort.c_str(), bUpdate);
    if ( m_pCbMsgLoop ) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgCbMemChange );
        if (NULL == pMsg) {
            return ;
        }
        if (NULL == pMsg->m_param.cbMemChange.roomID || NULL == pMsg->m_param.cbMemChange.pListMemChagne ) {
            delete pMsg;
            pMsg = NULL;
            return ;
        }
        
        std::copy(listMemberChange.begin(), listMemberChange.end(), std::back_inserter( *pMsg->m_param.cbMemChange.pListMemChagne));
        
        *(pMsg->m_param.cbMemChange.roomID) = roomShort;
        pMsg->m_param.cbMemChange.bUpdate  = bUpdate;
        
        m_pCbMsgLoop->SendMessage(pMsg);

        if (bUpdate) {
            // remove member in rtp manager
            for ( auto iter = listMemberChange.begin(); iter != listMemberChange.end(); ++iter ) {
                
                // check m_SessionUserIdMap
                if (iter->sessionId > 0) {
                    if (!iter->isJoin)  // leave room
                    {
                        {
                            // delete user in video input status cache
                            std::lock_guard< std::mutex> lock(m_mutexOtherResolution);

                            auto userIter = m_mapCurOtherUserID.find(iter->userID);
                            if (userIter != m_mapCurOtherUserID.end()) {
                                m_mapCurOtherUserID.erase( userIter );
                            }
                            
                        }

                        {
                            // delete user in m_SessionUserIdMap
                            std::lock_guard< std::mutex > lock(m_mutexSessionMap);
                            for (auto it2 = m_SessionUserIdMap.begin(); it2 != m_SessionUserIdMap.end(); ++it2) {
                                if (it2->second == iter->userID) {
                                    m_SessionUserIdMap.erase(it2);
                                    TSK_DEBUG_INFO ("remove user:%s sessionid:%d from m_SessionUserIdMap", iter->userID.c_str(), iter->sessionId);
                                    break;
                                }
                            }
                        }

                        {
                            // deleter user in m_UserVideoInfoMap
                            std::lock_guard< std::mutex > lock(m_mutexVideoInfoMap);
                            auto userinfo_iter = m_UserVideoInfoMap.find(iter->userID);
                            if (userinfo_iter != m_UserVideoInfoMap.end()) {
                                m_UserVideoInfoMap.erase(userinfo_iter);
                            } 
                        }

                        {
                            // deleter user in m_UserVideoInfoList
                            std::lock_guard< std::mutex > lock(m_mutexVideoInfoCache);
                            for (auto cacheiter = m_UserVideoInfoList.begin(); cacheiter != m_UserVideoInfoList.end(); ++cacheiter) {
                                if (cacheiter->userId == iter->userID) {
                                    m_UserVideoInfoList.erase(cacheiter);
                                    break;
                                }
                            }
                        }
     
                    } else { // join room

                        std::lock_guard< std::mutex > lock(m_mutexSessionMap);
                        auto it2 = m_SessionUserIdMap.begin();
                        for (; it2 != m_SessionUserIdMap.end(); ++it2) {
                            if (it2->second == iter->userID && (it2->first != iter->sessionId)) {
                                
                                it2 = m_SessionUserIdMap.erase(it2);
                                m_SessionUserIdMap.insert(SessionUserIdMap_t::value_type(iter->sessionId, iter->userID));
                                TSK_DEBUG_INFO ("1st insert user:%s sessionid:%d from m_SessionUserIdMap", iter->userID.c_str(), iter->sessionId);
                                break;
                            }
                        }

                        if (it2 == m_SessionUserIdMap.end()) {
                            m_SessionUserIdMap.insert(SessionUserIdMap_t::value_type(iter->sessionId, iter->userID));
                            TSK_DEBUG_INFO ("2nd insert user:%s sessionid:%d from m_SessionUserIdMap", iter->userID.c_str(), iter->sessionId);
                        }
                    }                    
                }


                if (!iter->isJoin && iter->sessionId > 0 && m_avSessionMgr) {
                    tsk_bool_t ret = tsk_false;
                    tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                    if (session) {
                        ret = trtp_manager_remove_member(TDAV_SESSION_AV(session)->rtp_manager, iter->sessionId);
                        if (ret) {
                            TSK_DEBUG_INFO ("remove rtcp source user:%s sessionid:%d success", iter->userID.c_str(), iter->sessionId);
                        } else {
                            TSK_DEBUG_INFO ("remove rtcp source user:%s sessionid:%d fail", iter->userID.c_str(), iter->sessionId);
                        }
                    }
                    

                    // release audio/video resource
                    m_avSessionMgr->releaseUserResource(iter->sessionId, tmedia_audiovideo);
                    TSK_DEBUG_INFO ("release audio/video resource, user:%s, sessionId:%d", iter->userID.c_str(), iter->sessionId);
                }
            }

            // check member count to determin to start or stop send stream(only for video)
            if (m_avSessionMgr) {
                tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                if (session) {
                    if (m_SessionUserIdMap.size() > 0) {
                        // start send audio/video data
                        trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 1);
                    } else {
                        // stop send audio/video data
                        trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 0);
                    }

                    TSK_DEBUG_INFO ("memberchange count %d", m_SessionUserIdMap.size());
                    // 当房间里只有两个人时启动p2p通路检测，其它情况则停止通路检测
                    // trtp_manager内部保证若没有设置局域网信息，则不启动通路检测定时器
                    if (1 == m_SessionUserIdMap.size()) {
                        trtp_manager_start_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
                    } else {
                        trtp_manager_stop_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
                    }
                }
            }
        } else {
            if (listMemberChange.size() > 1) {
                // start send audio/video data
                if (m_avSessionMgr) {
                   tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
                   if (session) {
                        trtp_manager_set_send_flag(TDAV_SESSION_AV(session)->rtp_manager, 1);

                        TSK_DEBUG_INFO ("get usermap count %d", listMemberChange.size());
                        // 当房间里只有两个人时启动p2p通路检测，其它情况则停止通路检测
                        // trtp_manager内部保证若没有设置局域网信息，则不启动通路检测定时器
                        // 注意这里做大于等于2的判断，是为了兼容后进来的获取房间用户数量不正确的bug，该定时器只有局域网信息已设置的场景下才会启动
                        if (listMemberChange.size() >= 2) {
                            trtp_manager_start_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
                        } else {
                            trtp_manager_stop_route_check_timer(TDAV_SESSION_AV(session)->rtp_manager);
                        }
                   }
               }
            }
        }

        return ;
    }
    
    TSK_DEBUG_INFO ("== %s failed", __FUNCTION__);
}

void CYouMeVoiceEngine::OnGrabMicNotify(int mode, int ntype, int getMicFlag, int autoOpenMicFlag, int hasMicFlag, 
                                        int talkTime, const std::string& strRoomID, const std::string strUserID, const std::string& strContent)
{
    TSK_DEBUG_INFO("@@ OnGrabMicNotify:Room:%s mode:%d type:%d getMic:%d autoopenMic:%d hasMic:%d talkTime:%d User:%s content:%s",
                   strRoomID.c_str(), mode, ntype, getMicFlag, autoOpenMicFlag, hasMicFlag, talkTime, strUserID.c_str(), strContent.c_str());
    
    std::string roomid = "";
    if (!strRoomID.empty()){
        removeAppKeyFromRoomId(strRoomID, roomid);
        TSK_DEBUG_INFO("@@ OnGrabMicNotify:%s", roomid.c_str());
    }
    
    if (!roomid.empty() && !m_pRoomMgr->isInRoom(roomid)){
        TSK_DEBUG_INFO("@@ OnGrabMicNotify[Not In Room!]");
        return;
    }
    
    int msg = YouMeProtocol::MSG_FIGHT_4_MIC_NOTIFY;
    int ex = 0;
    int nerr = 0;
    if (mode == 1){
        if (ntype == YouMeProtocol::FIGHT_MIC_EVENT_TYPE::FIGHT_TYPE_INIT_NOTIFY){
            ex = NgnLoginServiceCallback::GRAB_MIC_START;
        }
        else if (ntype == YouMeProtocol::FIGHT_MIC_EVENT_TYPE::FIGHT_TYPE_DEINIT_NOTIFY){
            ex = NgnLoginServiceCallback::GRAB_MIC_STOP;
        }
        else if (ntype == YouMeProtocol::FIGHT_MIC_EVENT_TYPE::FIGHT_TYPE_GET_MIC_NOTIFY){
            ex = NgnLoginServiceCallback::GRAB_MIC_GET;
        }
        else if (ntype == YouMeProtocol::FIGHT_MIC_EVENT_TYPE::FIGHT_TYPE_RELEASE_MIC_NOTIFY){
            ex = NgnLoginServiceCallback::GRAB_MIC_FREE;
            
            if (!strUserID.empty() && strUserID == mStrUserID && m_hasGrabMic){
                m_hasGrabMic = false;
                if (m_bAutoOpenMic){
                    setMicrophoneMute(true);
                }
                sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_GRABMIC_ENDMIC, YouMeErrorCode::YOUME_SUCCESS, roomid, strContent);
            }
        }
        
        if (hasMicFlag == 1){
            sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_GRABMIC_NOTIFY_HASMIC, YouMeErrorCode::YOUME_SUCCESS, roomid, strContent);
        }
        else {
            sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_GRABMIC_NOTIFY_NOMIC, YouMeErrorCode::YOUME_SUCCESS, roomid, strContent);
        }
    }
    else if (mode == 2){
        msg = YouMeProtocol::MSG_FIGHT_4_MIC;
        if (getMicFlag != 1){
            nerr = YouMeProtocol::FIGHT_MIC_ERROR_CODE::FIGHT_MIC_FULL;
        }
    }
    
    if (mode == 1 && (ex == NgnLoginServiceCallback::GRAB_MIC_GET || ex == NgnLoginServiceCallback::GRAB_MIC_FREE)){
        YouMeBroadcast bc = ex == NgnLoginServiceCallback::GRAB_MIC_GET ? YouMeBroadcast::YOUME_BROADCAST_GRABMIC_BROADCAST_GETMIC : YouMeBroadcast::YOUME_BROADCAST_GRABMIC_BROADCAST_FREEMIC;
        sendCbMsgCallBroadcastEvent(bc, roomid, strUserID, "", strContent);
    }
    else {
        YouMeEvent evt = YouMeEvent::YOUME_EVENT_EOF;
        YouMeErrorCode err = YouMeErrorCode::YOUME_ERROR_UNKNOWN;
        
        if (!CYouMeVoiceEngine::ToYMData(msg, ex, nerr, &evt, &err))
            return;
        
        std::string sp = "";
        if (evt == YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_OK){
            m_hasGrabMic = true;
            if (autoOpenMicFlag == 1){
                m_bAutoOpenMic = true;
            }
            if (m_bAutoOpenMic){
                setMicrophoneMute(false);
            }
            char buf[64];
            sprintf(buf, "%d", talkTime);
            sp = buf;
        }
        
        sendCbMsgCallEvent(evt, err, roomid, sp.length() > 0? sp: strContent);
    }
}


void CYouMeVoiceEngine::OnInviteMicNotify(int mode, int ntype, int errCode, int talkTime, 
                                          const std::string& strRoomID, const std::string& fromUserID, const std::string& toUserID, const std::string& strContent)
{
    TSK_DEBUG_INFO("@@ OnInviteMicNotify:mode:%d type:%d err:%d talktime:%d room:%s from:%s to:%s content:%s",
                   mode, ntype, errCode, talkTime, strRoomID.c_str(), fromUserID.c_str(), toUserID.c_str(), strContent.c_str());
    
    if (!fromUserID.empty() && !toUserID.empty() && fromUserID == toUserID){
        TSK_DEBUG_INFO("@@ OnInviteMicNotify[Can Not Call/Answer Self!]");
        return;
    }
    
    std::string roomid = "";
    if (!strRoomID.empty()){
        removeAppKeyFromRoomId(strRoomID, roomid);
    }
    
    string uid = "";
    if(mode != 1) {
        switch (ntype)
        {
            case YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_REQUEST:
                if (!toUserID.empty() && toUserID != mStrUserID){
                    TSK_DEBUG_INFO("@@ OnInviteMicNotify[Not Call ME!]");
                    return;
                }
                uid = fromUserID;
                break;
            case YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_RESPONSE:
                if (!toUserID.empty() && toUserID != mStrUserID){
                    TSK_DEBUG_INFO("@@ OnInviteMicNotify[Not Answer ME!]");
                    return;
                }
                uid = fromUserID;
                break;
            case YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_CANCEL:
            {
                bool bok = false;
                if (!fromUserID.empty() && fromUserID == mStrUserID){
                    uid = toUserID;
                    bok = true;
                }
                else if (!toUserID.empty() && toUserID == mStrUserID){
                    uid = fromUserID;
                    bok = true;
                }
                if (!bok){
                    TSK_DEBUG_INFO("@@ OnInviteMicNotify[Not Cancel ME!]");
                    return;
                }
            }
                break;
            default:
                break;
        }
    }
    
    if (mode == 1 && (ntype == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_SUCCESS || ntype == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_END)){
        YouMeBroadcast bc = ntype == YouMeProtocol::INVITE_EVENT_TYPE::INVITE_TYPE_SUCCESS ?
        YouMeBroadcast::YOUME_BROADCAST_INVITEMIC_BROADCAST_CONNECT : YouMeBroadcast::YOUME_BROADCAST_INVITEMIC_BROADCAST_DISCONNECT;
        sendCbMsgCallBroadcastEvent(bc, "", fromUserID, toUserID, strContent);
    }
    else{
        YouMeEvent evt = YouMeEvent::YOUME_EVENT_EOF;
        YouMeErrorCode err = YouMeErrorCode::YOUME_ERROR_UNKNOWN;
        
        if (!CYouMeVoiceEngine::ToYMData(YouMeProtocol::MSG_INVITE_NOTIFY, ntype, errCode, &evt, &err))
            return;
        
        sendCbMsgCallEvent(evt, err, uid, strContent);
        
        if (evt == YouMeEvent::YOUME_EVENT_INVITEMIC_NOTIFY_ANSWER && err == YOUME_SUCCESS){
            if (!m_hasInviteMic){
                m_hasInviteMic = true;
                mInviteMicOK = true;
                
                std::string sp = "";
                char buf[64];
                sprintf(buf, "%d", talkTime);
                sp = buf;
                
                sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_INVITEMIC_CAN_TALK, err, roomid, sp);
            }
        }
        else if (evt == YouMeEvent::YOUME_EVENT_INVITEMIC_NOTIFY_CANCEL && (err == YOUME_SUCCESS || err == YOUME_ERROR_INVITEMIC_TIMEOUT)){
            if (m_hasInviteMic){
                m_hasInviteMic = false;
                mInviteMicOK = false;
                sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_INVITEMIC_CANNOT_TALK, err, roomid);
            }
        }
    }
}


void CYouMeVoiceEngine::OnCommonEvent(int msgID, int wparam, int lparam, int errCode, const std::string strRoomID, int nSessionID , std::string strParam )
{
    TSK_DEBUG_INFO("@@ OnCommonEvent:Msg:%d WParam:%d LParam:%d Err:%d Room:%s MsgSession:%d",
                   msgID, wparam, lparam, errCode, strRoomID.c_str(), nSessionID);
    
    if ( nSessionID != 0 && nSessionID != mSessionID){
        TSK_DEBUG_INFO("@@ OnCommonEvent[Diff Session!]:MsgSession:%d SelfSession:%d", nSessionID, mSessionID);
        //return;
    }
    
    std::string roomid = "";
    if (!strRoomID.empty()){
        removeAppKeyFromRoomId(strRoomID, roomid);
        TSK_DEBUG_INFO("@@ OnCommonEvent:%s", roomid.c_str());
    }
    
    if (!roomid.empty() && !m_pRoomMgr->isInRoom(roomid)){
        TSK_DEBUG_INFO("@@ OnCommonEvent[Not In Room!]");
        return;
    }
    
    YouMeEvent evt = YouMeEvent::YOUME_EVENT_EOF;
    YouMeErrorCode err = YouMeErrorCode::YOUME_ERROR_UNKNOWN;
    
    if (!CYouMeVoiceEngine::ToYMData(msgID, wparam, errCode, &evt, &err))
        return;
    
    std::string sp = "";
    
    //抢麦特殊处理
    if (evt == YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_OK ){
        m_hasGrabMic = true;
        if (wparam == 1){
            m_bAutoOpenMic = true;
        }
        if (m_bAutoOpenMic){
            setMicrophoneMute(false);
        }
        char buf[64];
        sprintf(buf, "%d", lparam);
        sp = buf;
    }
    else if (evt == YouMeEvent::YOUME_EVENT_GRABMIC_RELEASE_OK){
        m_hasGrabMic = false;
    }
    else if ( evt == YouMeEvent::YOUME_EVENT_SEND_MESSAGE_RESULT ){
        stringstream ss;
        ss<< lparam ;
        
        sp = ss.str();
    }
    else if ( evt == YouMeEvent::YOUME_EVENT_MESSAGE_NOTIFY ){
        sp = strParam;
        std::string fromUserID = getUserNameBySessionId(nSessionID);
    }
    else
    {
        sp = strParam;
    }
    
    sendCbMsgCallEvent(evt, err, roomid, sp);
    
    //连麦特殊处理
    if (evt == YouMeEvent::YOUME_EVENT_INVITEMIC_RESPONSE_OK && err == YOUME_SUCCESS){
        if (!m_hasInviteMic){
            m_hasInviteMic = true;
            mInviteMicOK = true;
            
            char buf[64];
            sprintf(buf, "%d", lparam);
            sp = buf;
            sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_INVITEMIC_CAN_TALK, err, roomid, sp);
        }
    }
    else if (evt == YouMeEvent::YOUME_EVENT_INVITEMIC_STOP_OK && err == YOUME_SUCCESS){
        if (m_hasInviteMic){
            m_hasInviteMic = false;
            mInviteMicOK = false;
            sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_INVITEMIC_CANNOT_TALK, err, roomid);
        }
    }
}

void CYouMeVoiceEngine::doCheckVideoSendStatus() {
    
    uint32_t width,height;
    MediaSessionMgr::getVideoNetResolutionForSecond(width,height);
    
    // 宽高任一值非法均返回
    if(width == 0 || height == 0)
    {
        return;
    }
    
    auto iter2 = m_UserVideoInfoMap.begin();
    for (; iter2 != m_UserVideoInfoMap.end(); ++iter2) {
        if (!iter2->second)
            break;
    }

    do {
        if (!m_avSessionMgr) {
            break;
        }

        tmedia_session_t * session = m_avSessionMgr->getSession(tmedia_video);
        if (!session) {
            break;
        }

        // 至少有一个人接收大流则需要发送大流
        if (m_UserVideoInfoMap.size() == 0 || iter2 != m_UserVideoInfoMap.end()) {
            TSK_DEBUG_INFO ("OnUserVideoInfoNotify start send main stream");
            trtp_manager_set_send_status(TDAV_SESSION_AV(session)->rtp_manager, 0);
        } else {
            TSK_DEBUG_INFO ("OnUserVideoInfoNotify stop send main stream");
            trtp_manager_set_send_status(TDAV_SESSION_AV(session)->rtp_manager, 1);
        }
        return;
    } while(0);

    TSK_DEBUG_WARN ("avsession not ready, stream check will be done later");
}

void CYouMeVoiceEngine::OnUserVideoInfoNotify(const struct SetUserVideoInfoNotify &info) {
    TSK_DEBUG_INFO ("@@ OnUserVideoInfoNotify userid:%s, videotype:%d", info.usrId.c_str(), info.videoType);

    std::string userid = info.usrId;
    int videotype = info.videoType;

    if (userid.empty() || (videotype !=0 && videotype != 1)) {
        TSK_DEBUG_INFO ("OnUserVideoInfoNotify input invalid userid:%s, sessionId:%d, type:%d"
            , info.usrId.c_str(), info.sessionId, info.videoType);
        return;
    }

    {
        std::lock_guard< std::mutex > lock(m_mutexVideoInfoMap);
        auto iter = m_UserVideoInfoMap.find(userid);
        if (iter != m_UserVideoInfoMap.end()) {
            m_UserVideoInfoMap[userid] = videotype;
        } else {
            // insert user 
            m_UserVideoInfoMap.insert(UserVideoInfoMap_t::value_type(userid, videotype));
        }
        doCheckVideoSendStatus();
    }    

}

////接收语音消息被屏蔽一路的通知
//void CYouMeVoiceEngine::OnReciveAvoidControllMsg(const std::string&strUserID,int iStauts)
//{
//    TSK_DEBUG_INFO ("OnReciveAvoidControllMsg:%s_%d", strUserID.c_str(), iStauts);
//    if (NULL != mPConferenceCallback) {
//        mPConferenceCallback->onAvoidStatusEvent(strUserID, iStauts);
//    }
//}


YouMeErrorCode CYouMeVoiceEngine::setOtherMicMute(const std::string &strUserID,bool mute)
{
    bool isOn = !mute ;
    TSK_DEBUG_INFO ("@@ setOtherMicMute, UserID:%s, isOn:%d", strUserID.c_str(), (int32_t)isOn);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop ) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetOthersMicOn);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        if (NULL == pMsg->m_param.apiSetUserBool.pStrUserID) {
            delete pMsg;
            pMsg = NULL;
            return YOUME_ERROR_MEMORY_OUT;
        }
        *(pMsg->m_param.apiSetUserBool.pStrUserID) = strUserID;
        pMsg->m_param.apiSetUserBool.bTrue = isOn;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== setOtherMicMute");
        return YOUME_SUCCESS;
    }
    TSK_DEBUG_INFO ("== setOtherMicMute failed");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::setOtherSpeakerMute (const std::string& strUserID,bool mute)
{
    bool isOn = !mute;
    TSK_DEBUG_INFO ("@@ setOtherSpeakerMute, userId:%s, isOn:%d", strUserID.c_str(), (int32_t)isOn);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop ) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetOthersSpeakerOn);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        if (NULL == pMsg->m_param.apiSetUserBool.pStrUserID) {
            delete pMsg;
            pMsg = NULL;
            return YOUME_ERROR_MEMORY_OUT;
        }
        *(pMsg->m_param.apiSetUserBool.pStrUserID) = strUserID;
        pMsg->m_param.apiSetUserBool.bTrue = isOn;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== setOtherSpeakerMute");
        return YOUME_SUCCESS;
    }
    TSK_DEBUG_INFO ("== setOtherSpeakerMute failed");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::setListenOtherVoice (const std::string& strUserID,bool isOn)
{
    TSK_DEBUG_INFO ("@@ setListenOtherVoice, userId:%s, isOn:%d", strUserID.c_str(), (int32_t)isOn);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetOthersMuteToMe);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        if (NULL == pMsg->m_param.apiSetUserBool.pStrUserID) {
            delete pMsg;
            pMsg = NULL;
            return YOUME_ERROR_MEMORY_OUT;
        }
        *(pMsg->m_param.apiSetUserBool.pStrUserID) = strUserID;
        pMsg->m_param.apiSetUserBool.bTrue = isOn;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO ("== setListenOtherVoice");
        return YOUME_SUCCESS;
    }
    TSK_DEBUG_INFO ("== setListenOtherVoice failed");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrame(void* data, int len, int width, int	height, int fmt, int rotation, int mirror, uint64_t timestamp, int streamID)
{
    TMEDIA_I_AM_ACTIVE(200, "@@ inputVideoFrame w:%d, h:%d, fmt:%d, len:%d, ts:%llu ", width, height, fmt, len, timestamp);
    if(data == NULL || len < 1 || width < 1 || height < 1 )
    {
        TSK_DEBUG_ERROR("inputVideoFrame param error, w:%d, h:%d, fmt:%d, len:%d, ts:%llu", width, height, fmt, len, timestamp);
        return YOUME_ERROR_INVALID_PARAM;
    }
    if (m_pMainMsgLoop && m_isInRoom ) {

#ifdef OS_LINUX
        prctl(PR_SET_NAME, "inputVideoFrame");
#endif

        //H264 ENCRYPT暂时不抽帧
        if(VIDEO_FMT_H264 != fmt && VIDEO_FMT_ENCRYPT != fmt)
        {
            // fiter video frame
            bool isDrop = checkFrameTstoDrop(timestamp);
            if (isDrop) {
                TSK_DEBUG_INFO("drop frame share ts:%llu", timestamp);
                return YOUME_SUCCESS;
            }
        }
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutput(data, len, width, height, fmt, rotation, mirror, timestamp, 1, streamID);
        if (YOUME_SUCCESS == ret && !m_bInputVideoIsOpen) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
            if (pMsg) {
                pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
                pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
                return YOUME_SUCCESS;
            }
        }
        return ret;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrame wrong state");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForShare(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    TMEDIA_I_AM_ACTIVE(200, "@@ inputVideoFrameForShare w:%d, h:%d, fmt:%d, len:%d, ts:%llu ", width, height, fmt, len, timestamp);
    if(data == NULL || len < 1 || width < 1 || height < 1 )
    {
        TSK_DEBUG_ERROR("inputVideoFrameForShare param error, w:%d, h:%d, fmt:%d, len:%d, ts:%llu", width, height, fmt, len, timestamp);
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (m_pMainMsgLoop && m_isInRoom) {
        // check video frame length
        if (checkInputVideoFrameLen(width, height, fmt, len)) {
            // TSK_DEBUG_ERROR("inputVideoFrameForShare data length error, w:%d, h:%d, fmt:%d, len:%d, ts:%llu", width, height, fmt, len, timestamp);
            return YOUME_ERROR_INVALID_PARAM;
        }

		// fiter video frame
		bool isDrop = checkFrameTstoDropForShare(timestamp);
		if (isDrop) {
			// TSK_DEBUG_INFO("drop frame share ts:%llu", timestamp);
			return YOUME_SUCCESS;
		}

        // 仅适用于外部输入共享视频数据, 视频分辨率变化时更新编码设置
        if (!m_bStartshare && !m_bSaveScreen && (shareWidth != width || shareHeight != height)) {
            shareWidth = width;
            shareHeight = height;
            TSK_DEBUG_INFO("inputVideoFrameForShare switch resolution w:%d, h:%d", width, height);
            MediaSessionMgr::setVideoNetResolutionForShare( shareWidth, shareHeight );
        }
        
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutput(data, len, width, height, fmt, rotation, mirror, timestamp, 0);
        if (YOUME_SUCCESS == ret && !m_bInputShareIsOpen) {
            int currentBusinessTypeFlag = tmedia_get_video_share_type();
            tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);

            Config_SetInt("STOP_SHARE_INPUT", 0);
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
            if (pMsg) {
                pMsg->m_param.apiShareInputStatusChange.inputStatus = 1;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
                return YOUME_SUCCESS;
            }
        }
        return ret;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrameForShare wrong state");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForMacShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp) {
    TMEDIA_I_AM_ACTIVE(200, "@@ inputVideoFrameForMacShare w:%d, h:%d, fmt:%d, ts:%llu ", width, height, fmt, timestamp);

    if (m_pMainMsgLoop && m_isInRoom) {
        // fiter video frame
        bool isDrop = checkFrameTstoDropForShare(timestamp);
        if (isDrop) {
            // TSK_DEBUG_INFO("drop frame share ts:%llu", timestamp);
            return YOUME_SUCCESS;
        }

        FrameImage* frame = new FrameImage(width, height, pixelbuffer);
        frame->fmt = fmt;
        frame->timestamp = timestamp;
        frame->rotate = rotation;
        frame->mirror = mirror;
        
        int tempWidth = frame->width, tempHeight = frame->height;
        if (90 == frame->rotate || 270 == frame->rotate) {
            tempWidth = frame->height;
            tempHeight = frame->width;
        }
        
        // 仅适用于外部输入共享视频数据, 视频分辨率变化时更新编码设置
        if ((shareWidth && shareHeight) && (frame->rotate != lastRotation)) {
            lastRotation = frame->rotate;
            if (90 == frame->rotate || 270 == frame->rotate) {
                MediaSessionMgr::setVideoNetResolutionForShare( shareHeight, shareWidth );
            } else {
                MediaSessionMgr::setVideoNetResolutionForShare( shareWidth, shareHeight );
            }
        }

        // for screen share, no need preview, renderflag:0
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForIOS(frame->data, tempWidth, tempHeight, frame->fmt, frame->rotate, 0, frame->timestamp, 0);
        if (YOUME_SUCCESS == ret && !m_bInputShareIsOpen) {
            int currentBusinessTypeFlag = tmedia_get_video_share_type();
            tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);

            Config_SetInt("STOP_SHARE_INPUT", 0);
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
            if (pMsg) {
                pMsg->m_param.apiShareInputStatusChange.inputStatus = 1;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
//                return YOUME_SUCCESS;
            }
        }

        delete frame;
    }
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForIOSShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    TMEDIA_I_AM_ACTIVE(200, "@@ inputVideoFrameForIOSShare w:%d, h:%d, fmt:%d, rotate:%d, ts:%llu ", width, height, fmt, rotation, timestamp);
    
    if (m_pMainMsgLoop && m_isInRoom) {
#if TARGET_OS_IPHONE
        bool cacheLastPixel = true;
        bool isDrop = checkFrameTstoDrop(timestamp);
        if (isDrop) {
            // TSK_DEBUG_INFO("drop frame ios ts:%llu", timestamp);
            return YOUME_SUCCESS;
        }

        // 1、若已经打开缓存重发功能，则先检查内存是否足够，若内存不足则暂时关闭缓存功能，待内存足够时再恢复缓存重发
        if (1 == m_iosRetrans) {
            // 新数据到达可以直接将缓存数据清理掉
            {
                std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
                while (!m_videoQueue.empty()) { //最多允许积累1个数据
                    FrameImage* pMsg = m_videoQueue.front();
                    m_videoQueue.pop_front();
                    if (pMsg) {
                        ICameraManager::getInstance()->decreaseBufferRef(pMsg->data);
                        // TSK_DEBUG_INFO("mem overflow delete frame:%llu", pMsg->timestamp);
                        delete pMsg;
                        pMsg = NULL;
                    }
                }
            }

            // 首先检查剩余内存是否足够
            uint64_t currentMemoryUsageMB = 0;
            int maxMemLimit = 1024;//MB

            if(ICameraManager::getInstance()->isUnderAppExtension())
            {
                maxMemLimit = 50; //扩展模式限制最多用50MB内存
            }
            currentMemoryUsageMB = ICameraManager::getInstance()->usedMemory();
            if(maxMemLimit - currentMemoryUsageMB < 15){
                //内存过大直接跳过处理
                return YOUME_ERROR_MEMORY_OUT;
            }

            if((maxMemLimit - currentMemoryUsageMB) > (width * height * 3 / 1048576)) {
                ICameraManager::getInstance()->setCacheLastPixelEnable(true);
            } else {
                cacheLastPixel = false;
                ICameraManager::getInstance()->setCacheLastPixelEnable(false);
            }
        }

        // 2、直接用系统回调的pixel做旋转/缩放处理，同时若有打开缓存重发功能，则在旋转/缩放后cache       
        int tempWidth = width, tempHeight = height;
        if (90 == rotation || 270 == rotation) {
            tempWidth = height;
            tempHeight = width;
        }
        
        if ((shareWidth && shareHeight) && (rotation != lastRotation)) {
            lastRotation = rotation;
            if (90 == rotation || 270 == rotation) {
                MediaSessionMgr::setVideoNetResolutionForShare( shareHeight, shareWidth );
            } else {
                MediaSessionMgr::setVideoNetResolutionForShare( shareWidth, shareHeight );
            }
        }

        // for screen share, no need preview, renderflag:0
        ICameraManager::getInstance()->lockBufferRef(pixelbuffer); 
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForIOS(pixelbuffer, tempWidth, tempHeight, fmt, rotation, 0, timestamp, 0);
        if (YOUME_SUCCESS == ret && !m_bInputShareIsOpen) {
            int currentBusinessTypeFlag = tmedia_get_video_share_type();
            tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);

            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
            if (pMsg) {
                pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
                pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
            }
        }
        ICameraManager::getInstance()->unlockBufferRef(pixelbuffer);

        // 3、若已经打开异步重发功能则将最近一帧数据放入队列，若外部超时未输入新的frame则重发队列重的frame
        if (1 == m_iosRetrans && true == cacheLastPixel) {

            // 考虑到画面旋转，获取最新的分辨率
            uint32_t lastFrameWidth = 0, lastFrameHeight = 0;
            MediaSessionMgr::getVideoNetResolutionForShare( lastFrameWidth, lastFrameHeight );
            
            void * tempBuffer = ICameraManager::getInstance()->getLastPixelRef();
            if (tempBuffer) {
                FrameImage* frameImage = new FrameImage(lastFrameWidth, lastFrameHeight, tempBuffer);
                frameImage->fmt = fmt;
                frameImage->timestamp = timestamp;
                frameImage->rotate = 0;
                frameImage->mirror = 0;
                {
                    std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
                    m_videoQueue.push_back(frameImage);
                    m_videoQueueCond.notify_one();
                }
            }
        }
#endif

        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrameForIOS wrong state");
    return YOUME_ERROR_WRONG_STATE;
}

// YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForIOSShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
//     // TSK_DEBUG_INFO("@@ inputVideoFrameForIOS w:%d, h:%d, fmt:%d, rotate:%d, ts:%llu ", width, height, fmt, rotation, timestamp);

//     if (m_pMainMsgLoop && m_isInRoom) {
//         uint64_t currentMemoryUsageMB = 0;
//         int maxMemLimit = 1024;//MB
//         bool isDrop = checkFrameTstoDrop(timestamp);
//         if (isDrop) {
//             // TSK_DEBUG_INFO("drop frame ios ts:%llu", timestamp);
//             return YOUME_SUCCESS;
//         }

// #if TARGET_OS_IPHONE
//         if(ICameraManager::getInstance()->isUnderAppExtension())
//         {
//             maxMemLimit = 50; //扩展模式限制最多用50MB内存
//         }
//         currentMemoryUsageMB = ICameraManager::getInstance()->usedMemory();
//         if(maxMemLimit - currentMemoryUsageMB < 15){
//             //内存过大直接跳过处理
//             return YOUME_ERROR_MEMORY_OUT;
//         }
//         ICameraManager::getInstance()->lockBufferRef(pixelbuffer);
// #endif
//         //如果剩余内存大于拷贝数据需要的两倍内存大小，才拷贝数据
//         //50MB 是iOS扩展模式的限制大小，currentMemoryUsageMB 是已经使用的大小，width*heigh * 3 是yuv420数据大小的两倍
//         if((maxMemLimit - currentMemoryUsageMB) > (width * height * 3 / 1048576)){ 
//             // 对于同步模式，该copy作为强刷处理；对于异步模式，用于后续处理
//             void * tempBuffer = NULL;
//             #if TARGET_OS_IPHONE
//             tempBuffer = ICameraManager::getInstance()->copyPixelbuffer(pixelbuffer);
//             #endif
//             FrameImage* frameImage = new FrameImage(width, height, tempBuffer);//, len, mirror, timestamp, fmt);
//             frameImage->fmt = fmt;
//             frameImage->timestamp = timestamp;
//             frameImage->rotate = rotation;
//             frameImage->mirror = mirror;
//             {
//                 std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
//                 while (!m_videoQueue.empty()) { //最多允许积累1个数据
//                     FrameImage* pMsg = m_videoQueue.front();
//                     m_videoQueue.pop_front();
//                     if (pMsg) {
//                         #if TARGET_OS_IPHONE
//                         ICameraManager::getInstance()->decreaseBufferRef(pMsg->data);
//                         #endif
//                         if (1 == m_iosDeliverMode) {
//                             TSK_DEBUG_INFO("timeout delete frame:%llu", pMsg->timestamp);
//                         }
//                         delete pMsg;
//                         pMsg = NULL;
//                     }
//                 }
//                 m_videoQueue.push_back(frameImage);
//                 m_videoQueueCond.notify_one();
//             }
//         }else{
//             //如果内存超过了，把就的缓存数据清理掉
//             {
//                 std::lock_guard<std::mutex> queueLock(m_videoQueueMutex);
//                 while (!m_videoQueue.empty()) { //最多允许积累1个数据
//                     FrameImage* pMsg = m_videoQueue.front();
//                     m_videoQueue.pop_front();
//                     if (pMsg) {
//                         #if TARGET_OS_IPHONE
//                         ICameraManager::getInstance()->decreaseBufferRef(pMsg->data);
//                         #endif
//                         TSK_DEBUG_INFO("mem overflow delete frame:%llu", pMsg->timestamp);
//                         delete pMsg;
//                         pMsg = NULL;
//                     }
//                 }
//             }
//         }

//         // 0: 同步处理，该模式下直接用系统回调的pixel做处理
//         if (0 == m_iosDeliverMode) {
//             FrameImage* frame = new FrameImage(width, height, pixelbuffer);//, len, mirror, timestamp, fmt);
//             frame->fmt = fmt;
//             frame->timestamp = timestamp;
//             frame->rotate = rotation;
//             frame->mirror = mirror;
            
//             int tempWidth = frame->width, tempHeight = frame->height;
//             if (90 == frame->rotate || 270 == frame->rotate) {
//                 tempWidth = frame->height;
//                 tempHeight = frame->width;
//             }
            
//             if ((shareWidth && shareHeight) && (frame->rotate != lastRotation)) {
//                 lastRotation = frame->rotate;
//                 if (90 == frame->rotate || 270 == frame->rotate) {
//                     MediaSessionMgr::setVideoNetResolutionForShare( shareHeight, shareWidth );
//                 } else {
//                     MediaSessionMgr::setVideoNetResolutionForShare( shareWidth, shareHeight );
//                 }
//             }

//             // for screen share, no need preview, renderflag:0
//             YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForIOS(frame->data, tempWidth, tempHeight, frame->fmt, frame->rotate, 0, frame->timestamp, 0);
//             if (YOUME_SUCCESS == ret && !m_bInputShareIsOpen) {
//                 int currentBusinessTypeFlag = tmedia_get_video_share_type();
//                 tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);

//                 CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
//                 if (pMsg) {
//                     pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
//                     pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
//                     m_pMainMsgLoop->SendMessage(pMsg);
//                     TSK_DEBUG_INFO("== startinputVideoFrame");
//                 }
//             }

//             delete frame;
//         } 
// #if TARGET_OS_IPHONE
//         ICameraManager::getInstance()->unlockBufferRef(pixelbuffer);
// #endif        
//         return YOUME_SUCCESS;
//     }
    
//     TSK_DEBUG_INFO("== inputVideoFrameForIOS wrong state");
//     return YOUME_ERROR_WRONG_STATE;
// }

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    if (m_pMainMsgLoop && m_isInRoom) {
        // for camera preview, renderflag:1
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForIOS(pixelbuffer, width, height, fmt, rotation, mirror, timestamp, 1);
        if (YOUME_SUCCESS == ret && !m_bInputVideoIsOpen) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
            if (pMsg) {
                pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
                pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
                return YOUME_SUCCESS;
            }
        }
        return ret;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrameForIOS wrong state");
    return YOUME_ERROR_WRONG_STATE;
}

#if ANDROID
YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForAndroid(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    if (m_pMainMsgLoop && m_isInRoom) {
        int renderflag = 1;
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForAndroid(textureId, matrix, width, height, fmt, rotation, mirror, timestamp, renderflag);
        if (YOUME_SUCCESS == ret && !m_bInputVideoIsOpen) {
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
            if (pMsg) {
                pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
                pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
                return YOUME_SUCCESS;
            }
        }
        return ret;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrameForAndroid wrong state");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::inputVideoFrameForAndroidShare(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    TMEDIA_I_AM_ACTIVE(200, "@@ inputVideoFrameForAndroidShare w:%d, h:%d, fmt:%d, ts:%llu ", width, height, fmt, timestamp);

    if (m_pMainMsgLoop && m_isInRoom) {
        int renderflag = 0;
        YouMeErrorCode ret = ICameraManager::getInstance()->videoDataOutputGLESForAndroid(textureId, matrix, width, height, fmt, rotation, mirror, timestamp, 0);
        if (YOUME_SUCCESS == ret && !m_bInputShareIsOpen) {
            Config_SetInt("STOP_SHARE_INPUT", 0);
            CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
            if (pMsg) {
                pMsg->m_param.apiVideoInputStatusChange.inputStatus = 1;
                pMsg->m_param.apiVideoInputStatusChange.errorCode = ret;
                m_pMainMsgLoop->SendMessage(pMsg);
                TSK_DEBUG_INFO("== startinputVideoFrame");
                return YOUME_SUCCESS;
            }
        }
        return ret;
    }
    
    TSK_DEBUG_INFO("== inputVideoFrameForAndroidShare wrong state");
    return YOUME_ERROR_WRONG_STATE;
}
#endif

void CYouMeVoiceEngine::audioMixCallback(void* data, size_t length, YMAudioFrameInfo info) {
    CYouMeVoiceEngine::getInstance()->mixAudioTrack(data, length, info.channels, info.sampleRate, info.bytesPerFrame / info.channels, info.timestamp, info.isFloat, !info.isBigEndian, !info.isNonInterleaved, false);
}

YouMeErrorCode CYouMeVoiceEngine::inputAudioFrameForMix(int streamId, void* data, int len, YMAudioFrameInfo info, uint64_t timestamp) {
    if(!info.isSignedInteger) {
        //todu 暂时只支持int16_t
        return YOUME_ERROR_INVALID_PARAM;
    }
    if(info.isFloat) {
        //todu 暂时只支持int16_t
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if(info.bytesPerFrame/info.channels != 2) {
        //todu 暂时只支持int16_t
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if(info.channels != 1 && info.channels != 2) {
        // 输入支持单声道和双声道，输出暂时只支持单声道
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    switch (info.sampleRate) {
        case 8000:
        case 16000:
        case 24000:
        case 22050:
        case 32000:
        case 44100:
        case 48000:
            break;
        default:
            return YOUME_ERROR_INVALID_PARAM;
    }

    if (!m_audioMixer || mIsReconnecting || !m_pMainMsgLoop || !m_isInRoom) {
        return YOUME_ERROR_WRONG_STATE;
    }

    m_audioMixer->inputAudioFrameForMix(data, len, streamId, info);
    
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::setRecvCustomDataCallback(IYouMeCustomDataCallback* pCallback)
{
	m_pCustomDataCallback = pCallback;
}

void CYouMeVoiceEngine::setTranslateCallback( IYouMeTranslateCallback* pCallback )
{
    m_pTranslateCallback = pCallback;
}

YouMeErrorCode CYouMeVoiceEngine::inputCustomData(const void* data, int len, uint64_t timestamp, std::string userId)
{
    uint32_t uSessionId = 0;
	if (m_isInRoom) {
        // 获取目标用户sessionId
        if (!userId.empty()) {
            uSessionId = getSessionIdByUserName(userId);
        }

        // 发送自定义数据给指定用户
		int ret = CCustomInputManager::getInstance()->PostInputData(data, len, timestamp, uSessionId);
		if (YOUME_SUCCESS == ret) {			
			return YOUME_SUCCESS;
		}
		return (YouMeErrorCode)ret;
	}

	TSK_DEBUG_INFO("== inputCustonData wrong state");
	return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoFrameRawCbEnabled(bool enabled){
    if (m_isInRoom) {
        TSK_DEBUG_ERROR("setVideoFrameRawCbEnabled invalid when user in room");
        return YOUME_ERROR_WRONG_STATE;
    }
    TSK_DEBUG_INFO("== setVideoFrameRawCbEnabled:%d",enabled);
    tmedia_defaults_set_video_frame_raw_cb_enabled(enabled);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoDecodeRawCbEnabled(bool enabled){
    TSK_DEBUG_INFO("== setVideoDecodeRawCbEnabled:%d",enabled);
    tmedia_defaults_set_video_decode_raw_cb_enabled(enabled);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::stopInputVideoFrame()
{
    TSK_DEBUG_INFO ("@@ stopInputVideoFrame isInRoom:%d", m_isInRoom);
    if (m_pMainMsgLoop && m_isInRoom) {
        
        /*
         * TODO 增加数据上报
         */
        
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
        if (pMsg) {
            pMsg->m_param.apiVideoInputStatusChange.inputStatus = 0;
            pMsg->m_param.apiVideoInputStatusChange.errorCode = YOUME_SUCCESS;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== stopInputVideoFrame");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== stopInputVideoFrame");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::stopInputVideoFrameForShare()
{
    TSK_DEBUG_INFO ("@@ stopInputVideoFrameForShare isInRoom:%d", m_isInRoom);
    
    if (m_pMainMsgLoop) {
        Config_SetInt("STOP_SHARE_INPUT", 1);
        /*
         * TODO 增加数据上报
         */
        
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiShareInputStatus);
        if (pMsg) {
            pMsg->m_param.apiShareInputStatusChange.inputStatus = 0;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== stopInputVideoFrameForShare");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== stopInputVideoFrameForShare");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::queryUsersVideoInfo(std::vector<std::string>& userList)
{
    TSK_DEBUG_INFO ("@@ getUsersVideoInfo isInRoom:%d", m_isInRoom);
    if (m_pMainMsgLoop && m_isInRoom) {
        
        if(userList.size() == 0)
            return  YOUME_ERROR_INVALID_PARAM;
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiQueryUsersVideoInfo);
        if (pMsg) {
            for (int i = 0; i < userList.size(); ++i) {
                (*pMsg->m_param.apiQueryUsersVideoInfo.userList).push_back(userList[i]);
            }
           // (*pMsg->m_param.apiQueryUsersVideoInfo.userList) = userList;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== getUsersVideoInfo");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== getUsersVideoInfo");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::setUsersVideoInfo(std::vector<IYouMeVoiceEngine::userVideoInfo>& videoInfoList)
{
    TSK_DEBUG_INFO ("@@ setUsersVideoInfo isInRoom:%d, list count:%d", m_isInRoom, videoInfoList.size());
    if (m_pMainMsgLoop) {
        
        if(videoInfoList.size() == 0) {
            TSK_DEBUG_INFO ("setUsersVideoInfo input invalid size is zero");
            return  YOUME_ERROR_INVALID_PARAM;
        }

        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetUsersVideoInfo);
        if (pMsg) {
            for (int i = 0; i < videoInfoList.size(); ++i) {
                 
                TSK_DEBUG_INFO("videoinfo userid:%s, videoid:%d \n", videoInfoList[i].userId.c_str(), videoInfoList[i].resolutionType);

                //跳过自己
                if( videoInfoList[i].userId == mStrUserID )    {
                    continue;
                }

                (*pMsg->m_param.apiSetUsersVideoInfo.videoInfoList).push_back(videoInfoList[i]);

                {
                    // 检查该用户是否在缓存列表中
                    std::lock_guard< std::mutex > lock(m_mutexVideoInfoCache);
                    auto iter = m_UserVideoInfoList.begin();
                    for (; iter != m_UserVideoInfoList.end(); ++iter) {
                        if (iter->userId == videoInfoList[i].userId) {
                            iter->resolutionType = videoInfoList[i].resolutionType;
                            break;
                        }
                    }

                    if (iter == m_UserVideoInfoList.end()) {
                        m_UserVideoInfoList.push_back(videoInfoList[i]);
                    }
                }
            }

            if ((*pMsg->m_param.apiSetUsersVideoInfo.videoInfoList).size() > 0) {
                m_pMainMsgLoop->SendMessage(pMsg);
            } else {
                delete pMsg;
            }
            
            TSK_DEBUG_INFO("== setUsersVideoInfo");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setUsersVideoInfo");
    return YOUME_ERROR_WRONG_STATE;
}

YouMeErrorCode CYouMeVoiceEngine::setTCPMode(int iUseTCP)
{
    TSK_DEBUG_INFO ("@@ setTCPMode:%d", iUseTCP);
	//已经在房间的话，设置错误
	if (m_isInRoom)
	{
		return YOUME_ERROR_WRONG_STATE;
	}
	Config_SetInt("rtp_use_tcp", iUseTCP);
    TSK_DEBUG_INFO ("== setTCPMode:%d", iUseTCP);
	return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::setUseMobileNetworkEnabled (bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setUseMobileNetworkEnabled:%d", bEnabled);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (isStateInitialized()) {
        bool bRet = CNgnMemoryConfiguration::getInstance()->SetConfiguration (NgnConfigurationEntry::NETWORK_USE_MOBILE, bEnabled);
        if (false == bRet) {
            TSK_DEBUG_ERROR ("Failed to setUseMobileNetworkEnabled!");
        }
    }
    
    TSK_DEBUG_INFO ("== setUseMobileNetworkEnabled");
    return;
}
bool CYouMeVoiceEngine::getUseMobileNetWorkEnabled ()
{
    TSK_DEBUG_INFO ("@@ getUseMobileNetWorkEnabled");
    bool bMobileNet = true;
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (isStateInitialized()) {
        bMobileNet = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::NETWORK_USE_MOBILE, NgnConfigurationEntry::DEFAULT_NETWORK_USE_3G);
    }
    TSK_DEBUG_INFO ("== getUseMobileNetWorkEnabled:%d", (int32_t)bMobileNet);
    return bMobileNet;
}

/**
 *  功能描述:获取启用/禁用声学回声消除功能状态，启用智能参数后AEC和ANS的手动设置失效，SDK会根据需要智能控制，即可保证通话效果又能降低CPU和电量消耗
 *
 *  @return true-启用，false-禁用
 */
void CYouMeVoiceEngine::setAECEnabled (bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setAECEnabled:%d", bEnabled);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    bool bRet = CNgnMemoryConfiguration::getInstance()->SetConfiguration (NgnConfigurationEntry::GENERAL_AEC, bEnabled);
    if (false == bRet) {
        TSK_DEBUG_INFO ("== failed setAECEnabled");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetAecEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setAECEnabled");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setAECEnabled delayed");
}

bool CYouMeVoiceEngine::getAECEnabled ()
{
    TSK_DEBUG_INFO ("@@ getAECEnabled");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    bool bEnabled = true;
    if (isStateInitialized()) {
        bEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::GENERAL_AEC, NgnConfigurationEntry::DEFAULT_GENERAL_AEC);
    }
    TSK_DEBUG_INFO ("== getAECEnabled:%d", (int32_t)bEnabled);
    return bEnabled;
}

//启用/不启用背景噪音抑制功能,默认启用
void CYouMeVoiceEngine::setANSEnabled (bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setANSEnabled, bEnabled:%d", bEnabled);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    bool bRet = CNgnMemoryConfiguration::getInstance()->SetConfiguration (NgnConfigurationEntry::GENERAL_NR, bEnabled);
    if (false == bRet) {
        TSK_DEBUG_INFO ("== failed setANSEnabled");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetAnsEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setANSEnabled");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setANSEnabled delayed");
}

bool CYouMeVoiceEngine::getANSEnabled ()
{
    TSK_DEBUG_INFO ("@@ getANSEnabled");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    bool bEnabled = true;
    if (isStateInitialized()) {
        bEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::GENERAL_NR, NgnConfigurationEntry::DEFAULT_GENERAL_NR);
    }
    TSK_DEBUG_INFO ("== getANSEnabled:%d", (int32_t)bEnabled);
    return bEnabled;
}

//启用/不启用自动增益补偿功能,默认启用
void CYouMeVoiceEngine::setAGCEnabled (bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setAGCEnabled:%d", bEnabled);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    bool bRet = CNgnMemoryConfiguration::getInstance()->SetConfiguration (NgnConfigurationEntry::GENERAL_AGC, bEnabled);
    if (false == bRet) {
        TSK_DEBUG_INFO ("== failed setAGCEnabled");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetAgcEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setAGCEnabled");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setAGCEnabled delayed");
}

bool CYouMeVoiceEngine::getAGCEnabled ()
{
    TSK_DEBUG_INFO ("@@ getAGCEnabled");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    bool bEnabled = true;
    if (isStateInitialized()) {
        bEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::GENERAL_AGC, NgnConfigurationEntry::DEFAULT_GENERAL_AGC);
    }
    TSK_DEBUG_INFO ("== getAGCEnabled:%d", bEnabled);
    return bEnabled;
}

//启用/不启用静音检测功能,默认启用
void CYouMeVoiceEngine::setVADEnabled (bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setVADEnabled:%d", bEnabled);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    bool bRet = CNgnMemoryConfiguration::getInstance()->SetConfiguration (NgnConfigurationEntry::GENERAL_VAD, bEnabled);
    if (false == bRet) {
        TSK_DEBUG_INFO ("== failed setVADEnabled");
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetVadEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setVADEnabled");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setVADEnabled delayed");
}

bool CYouMeVoiceEngine::getVADEnabled ()
{
    TSK_DEBUG_INFO ("@@ getVADEnabled");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    bool bEnabled = true;
    if (isStateInitialized()) {
        bEnabled = CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::GENERAL_VAD, NgnConfigurationEntry::DEFAULT_GENERAL_VAD);
    }
    TSK_DEBUG_INFO ("== getVADEnabled:%d", bEnabled);
    return bEnabled;
}

void CYouMeVoiceEngine::setSoundtouchEnabled(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setSoundtouchEnabled:%d", bEnabled);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSoundTouchEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setSoundtouchEnabled");
            return;
        }
    }
    
    TSK_DEBUG_INFO ("== setSoundtouchEnabled failed");
}

void CYouMeVoiceEngine::setSoundtouchTempo(float fTempo)
{
    TSK_DEBUG_INFO ("@@ setSoundtouchTempo:%f", fTempo);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSoundTouchTempo);
        if (pMsg) {
            pMsg->m_param.fValue = fTempo;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setSoundtouchTempo");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setSoundtouchTempo failed");
}

void CYouMeVoiceEngine::setSoundtouchRate(float fRate)
{
    TSK_DEBUG_INFO ("@@ setSoundtouchRate:%f", fRate);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSoundTouchRate);
        if (pMsg) {
            pMsg->m_param.fValue = fRate;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setSoundtouchRate");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setSoundtouchRate failed");
}

void CYouMeVoiceEngine::setSoundtouchPitch(float fPitch)
{
    TSK_DEBUG_INFO ("@@ setSoundtouchPitch:%f", fPitch);
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (m_pMainMsgLoop && isStateInitialized()) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSoundTouchPitch);
        if (pMsg) {
            pMsg->m_param.fValue = fPitch;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setSoundtouchPitch");
            return;
        }
    }
    TSK_DEBUG_INFO ("== setSoundtouchPitch failed");
}

void CYouMeVoiceEngine::onNetWorkChanged (NETWORK_TYPE type)
{
    TSK_DEBUG_INFO ("@@ onNetWorkChanged, networktype:%d, mState:%s", type, stateToString(mState));
    
#ifdef __APPLE__
    if(preNetworkType == type){
        TSK_DEBUG_INFO ("== onNetWorkChanged,same network: nothing to do");
        return;
    }
    preNetworkType = type;
#endif
    
    if(NETWORK_TYPE_NO == type) {
        TSK_DEBUG_INFO ("== onNetWorkChanged, no network: nothing to do");
        return;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO ("== onNetWorkChanged, not in room");
        return;
    }
    
    if (mIsReconnecting) {
        TSK_DEBUG_INFO ("== onNetWorkChanged, reconnecting is in progress");
        return;
    }
    mIsReconnecting = true;
    
    if (!CNgnMemoryConfiguration::getInstance()->GetConfiguration (NgnConfigurationEntry::AUTO_RECONNECT, NgnConfigurationEntry::DEFAULT_AUTO_RECONNECT)) {
        TSK_DEBUG_INFO ("== Server config: no reconnect on network change");
        return;
    }
    
    bool bUseMobileNetwork = getUseMobileNetWorkEnabled();
    TSK_DEBUG_INFO ("bUseMobileNetwork:%d  isMobileNetwork:%d", bUseMobileNetwork, mPNetworkService->isMobileNetwork ());
    if ((!bUseMobileNetwork) && mPNetworkService->isMobileNetwork ()) {
        TSK_DEBUG_ERROR ("Mobile network is not allowed");
        leaveChannelAll();
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiReconnect);
        if (pMsg) {
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== onNetWorkChanged");
            return;
        }
    }
    
    TSK_DEBUG_INFO("== onNetWorkChanged: failed to send message");
}

void CYouMeVoiceEngine::onHeadSetPlugin (int state)
{
    TSK_DEBUG_INFO ("@@ onHeadSetPlugin, state:%d", state);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO("== wrong state:%s", stateToString(mState));
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiOnHeadsetPlugin);
        if (pMsg) {
            pMsg->m_param.i32Value = (int32_t)state;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== onHeadSetPlugin");
            return;
        }
    }
    
    TSK_DEBUG_INFO("== onHeadSetPlugin failed");
}

/**
 *  功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面。
 *  @param pBuf 指向PCM数据的缓冲区
 *  @param nSizeInByte 缓冲区中数据的大小，以Byte为单位
 *  @param nChannelNum 声道数量
 *  @param nSampleRateHz 采样率, 以Hz为单位
 *  @param nBytesPerSample 一个声道里面每个采样的字节数，典型情况下，如果数据是整型，该值为2，如果是浮点数，该值为4
 *  @param bFloat, true - 数据是浮点数， false - 数据是整型
 *  @param bLittleEndian, true - 数据是小端存储，false - 数据是大端存储
 *  @param bInterleaved, 用于多声道数据时，true - 不同声道采样交错存储，false - 先存储一个声音数据，再存储另一个声道数据
 *  @param bForSpeaker, true - 该数据要混到扬声器输出， false - 该数据要混到麦克风输入
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
YouMeErrorCode CYouMeVoiceEngine::mixAudioTrack(const void* pBuf, int nSizeInByte,
                                                int nChannelNUm, int nSampleRate,
                                                int nBytesPerSample, uint64_t nTimestamp, bool bFloat,
                                                bool bLittleEndian, bool bInterleaved, bool bForSpeaker)
{
    if ((nChannelNUm > 2) || !nChannelNUm || !pBuf || !nSizeInByte || !nSampleRate || !nBytesPerSample) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (mIsReconnecting) {
        return YOUME_ERROR_WRONG_STATE;
    }

    //YOUME_EVENT_MIC_CTR_ON,YOUME_EVENT_MIC_CTR_OFF,同时控制外部输入inputAudioFrame
    if(!mNeedInputAudioFrame)
    {
        TMEDIA_I_AM_ACTIVE(1000, "Not need InputAudioFrame");
        return YOUME_SUCCESS;
    }
	
#ifdef OS_LINUX
    prctl(PR_SET_NAME, "inputAudioFrame");
#endif

    TMEDIA_I_AM_ACTIVE(1000, "nSizeInByte:(%d), channel(%d) nSampleRate:(%d), timeStamp:(%lld) bInterleaved(%d), [0x%x][0x%x][0x%x][0x%x]"
    //TSK_DEBUG_INFO("nSizeInByte:(%d), channel(%d) nSampleRate:(%d), timeStamp:(%lld) bInterleaved(%d), [0x%x][0x%x][0x%x][0x%x]"
                , nSizeInByte, nChannelNUm, nSampleRate, nTimestamp, bInterleaved, *(char*)pBuf, *((char*)pBuf+1), *((char*)pBuf+2), *((char*)pBuf+3));

    if(m_avSessionExit.load())
        return YOUME_ERROR_WRONG_STATE;
    std::lock_guard<std::recursive_mutex> sessionMgrLock(m_avSessionMgrMutex);
    
    if (m_avSessionMgr) {
        //TSK_DEBUG_INFO("addr:%d,%d, size:%d, samplerate:%d, ch:%d, float:%d", pi2Buf2, pi2Buf2[0], nSizeInByte, nSampleRate, nChannelNUm, bFloat);
        m_avSessionMgr->setMixAudioTrack(pBuf, nSizeInByte, nChannelNUm, nSampleRate,
                                         nBytesPerSample, nTimestamp, bFloat, bLittleEndian, bInterleaved, bForSpeaker);
        return YOUME_SUCCESS;
    } else {
        return YOUME_ERROR_WRONG_STATE;
    }
}

YouMeErrorCode CYouMeVoiceEngine::playBackgroundMusic(const std::string& strFilePath, bool bRepeat)
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("@@ playBackgroundMusic music:%s, repeat:%d", strFilePath.c_str(), bRepeat);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (!mAllowPlayBGM) {
        TSK_DEBUG_ERROR("== Playing background music is not allowed");
        return YOUME_ERROR_API_NOT_SUPPORTED;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiPlayBgm);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        if (NULL == pMsg->m_param.apiPlayBgm.pStrFilePath) {
            delete pMsg;
            pMsg = NULL;
            return YOUME_ERROR_MEMORY_OUT;
        }
        *(pMsg->m_param.apiPlayBgm.pStrFilePath) = strFilePath;
        pMsg->m_param.apiPlayBgm.bRepeat = bRepeat;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== playBackgroundMusic");
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_ERROR("== playBackgroundMusic failed");
    return YOUME_ERROR_MEMORY_OUT;
#else
    TSK_DEBUG_ERROR("@@ Not support background music playback!");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

YouMeErrorCode CYouMeVoiceEngine::pauseBackgroundMusic()
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("@@ pauseBackgroundMusic");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiPauseBgm);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        pMsg->m_param.bTrue = true;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== pauseBackgroundMusic");
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_ERROR("== pauseBackgroundMusic failed");
    return YOUME_ERROR_MEMORY_OUT;
#else
    TSK_DEBUG_ERROR("@@ Not support background music playback!");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

YouMeErrorCode CYouMeVoiceEngine::resumeBackgroundMusic()
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("@@ resumeBackgroundMusic");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiPauseBgm);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        pMsg->m_param.bTrue = false;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== resumeBackgroundMusic");
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_ERROR("== resumeBackgroundMusic failed");
    return YOUME_ERROR_MEMORY_OUT;
#else
    TSK_DEBUG_ERROR("@@ Not support background music playback!");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

YouMeErrorCode CYouMeVoiceEngine::stopBackgroundMusic()
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("@@ stopBackgroundMusic");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiStopBgm);
        if (NULL == pMsg) {
            return YOUME_ERROR_MEMORY_OUT;
        }
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== stopBackgroundMusic");
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_ERROR("== stopBackgroundMusic failed");
    return YOUME_ERROR_MEMORY_OUT;
#else
    TSK_DEBUG_ERROR("@@ Not support background music playback!");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

// TODO : check the possible conflict between the Game background audio and
//        the user specified background music.
YouMeErrorCode CYouMeVoiceEngine::setBackgroundMusicVolume(int vol)
{
#if FFMPEG_SUPPORT
    TSK_DEBUG_INFO("@@ setBackgroundMusicVolume, vol:%d", vol);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_nMixToMicVol = (uint32_t)vol;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetBgmVolume);
        if (pMsg) {
            pMsg->m_param.apiSetBgmVolume.volume = (uint32_t)vol;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setBackgroundMusicVolume");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setBackgroundMusicVolume delayed");
    return YOUME_SUCCESS;
    
#else
    TSK_DEBUG_ERROR("@@ Not support background music playback!");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

YouMeErrorCode CYouMeVoiceEngine::setHeadsetMonitorOn(bool bMicEnabled, bool bBgmEnabled)
{
    TSK_DEBUG_INFO ("@@ setHeadsetMonitorOn mic:%d, bgm:%d", bMicEnabled, bBgmEnabled);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }

    if (!mAllowMonitor) {
        TSK_DEBUG_ERROR("== Voice monitor is not allowed");
    	return YOUME_ERROR_API_NOT_SUPPORTED;
    }
    
    m_bMicBypassToSpeaker = bMicEnabled;
    m_bBgmBypassToSpeaker = bBgmEnabled;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetMicAndBgmBypassToSpeaker);
        if (pMsg) {
            pMsg->m_param.apiSetMicAndBgmBypassToSpeaker.bMicEnable = bMicEnabled;
            pMsg->m_param.apiSetMicAndBgmBypassToSpeaker.bBgmEnable = bBgmEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setMicAndBgmBypassToSpeaker");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setHeadsetMonitorOn delayed");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setReverbEnabled(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setReverbEnabled:%d", bEnabled);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_bReverbEnabled = bEnabled;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetReverbEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setReverbEnabled");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setReverbEnabled delayed");
    return YOUME_SUCCESS;
}

//YouMeErrorCode CYouMeVoiceEngine::setBackgroundMusicDelay(int delay)
//{
//    TSK_DEBUG_INFO ("@@ setBackgroundMusicDelay delay:%d", delay);
//    
//    lock_guard<recursive_mutex> stateLock(mStateMutex);
//    if (!isStateInitialized()) {
//        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
//        return YOUME_ERROR_WRONG_STATE;
//    }
//    
//    m_BgmDelay = delay;
//    
//    if (m_pMainMsgLoop && isStateInRoom(mState)) {
//        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetBgmDelay);
//        if (pMsg) {
//            pMsg->m_param.u32Value = m_BgmDelay;
//            m_pMainMsgLoop->SendMessage(pMsg);
//            TSK_DEBUG_INFO("== setBackgroundMusicDelay");
//            return YOUME_SUCCESS;
//        }
//    }
//    
//    TSK_DEBUG_INFO("== setBackgroundMusicDelay delayed");
//    return YOUME_SUCCESS;
//}

YouMeErrorCode CYouMeVoiceEngine::pauseChannel(bool needCallback)
{
    TSK_DEBUG_INFO ("@@ pauseChannel");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiPauseConf);
        if (pMsg) {
            pMsg->m_param.bTrue = needCallback;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== pauseChannel");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== pauseChannel failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::resumeChannel(bool needCallback)
{
    TSK_DEBUG_INFO ("@@ resumeChannel");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiResumeConf);
        if (pMsg) {
            pMsg->m_param.bTrue = needCallback;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== resumeChannel");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== resumeChannel failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::setVadCallbackEnabled(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setVadCallbackEnable:%d", bEnabled);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_bVadCallbackEnabled = bEnabled;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetVadCallbackEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setVadCallbackEnable");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setVadCallbackEnable failed");
    return YOUME_ERROR_MEMORY_OUT;
}

void CYouMeVoiceEngine::setRecordingTimeMs(unsigned int timeMs)
{
    //TSK_DEBUG_INFO ("@@ setRecordingTimestamp:%d", timeMs);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetRecordingTimeMs);
        if (pMsg) {
            pMsg->m_param.u32Value = timeMs;
            m_pMainMsgLoop->SendMessage(pMsg);
            return;
        }
    }
}

void CYouMeVoiceEngine::setPlayingTimeMs(unsigned int timeMs)
{
    //TSK_DEBUG_INFO ("@@ setPlayingTimestamp:%d", timeMs);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetPlayingTimeMs);
        if (pMsg) {
            pMsg->m_param.u32Value = timeMs;
            m_pMainMsgLoop->SendMessage(pMsg);
            return;
        }
    }
}

YouMeErrorCode CYouMeVoiceEngine::setPcmCallbackEnable(IYouMePcmCallback* pcmCallback, int flag, bool bOutputToSpeaker, int nOutputSampleRate, int nOutputChannel)
{
    
    TSK_DEBUG_INFO ("@@ setPcmCallbackEnable:%p , flag:%d, bOutputToSpeaker:%d samplerate:%d, channels:%d", pcmCallback, flag,  bOutputToSpeaker, nOutputSampleRate, nOutputChannel);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (mPPcmCallback == pcmCallback && mPcmOutputToSpeaker == bOutputToSpeaker) {
        return YOUME_SUCCESS;
    }
    
    mPPcmCallback = pcmCallback;
    mPcmCallbackFlag = flag;
    mPcmOutputToSpeaker = bOutputToSpeaker;
    m_nPCMCallbackSampleRate = nOutputSampleRate;
    
    m_nPCMCallbackChannel = nOutputChannel;
    if(nOutputChannel != 1 && nOutputChannel != 2)
    {
        m_nPCMCallbackChannel = 1;
    }
    
    
    if(tmedia_defaults_get_external_input_mode() && !mPcmOutputToSpeaker) {
        tmedia_defaults_set_audio_faked_consumer(true);
    }else {
        tmedia_defaults_set_audio_faked_consumer(false);
    }
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO("== setPcmCallback before Initializ pcmCallback:%p, OutputToSpeaker:%d", pcmCallback, bOutputToSpeaker);
        return YOUME_SUCCESS;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetPcmCallback);
        if (pMsg) {
            pMsg->m_param.apiSetPcmCallback.pcmCallback = (void*)pcmCallback;
            pMsg->m_param.apiSetPcmCallback.pcmCallbackFlag = mPcmCallbackFlag;
            
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setPcmCallback");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setPcmCallback failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::setTranscriberEnabled(bool enable, IYouMeTranscriberCallback* transcriberCallback )
{
    TSK_DEBUG_INFO ("@@ setTranscriberEnabled:%p , enable:%d", transcriberCallback, enable );
#ifdef TRANSSCRIBER
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if( transcriberCallback == nullptr )
    {
        enable = false;
    }
    
    if (mPTranscriberCallback == transcriberCallback  && mTranscriberEnabled == enable) {
        return YOUME_SUCCESS;
    }
    
    if (!isStateInitialized()) {
        TSK_DEBUG_INFO("== setTranscriberEnabled before Initializ ");
        return YOUME_ERROR_WRONG_STATE;
    }
    
    mPTranscriberCallback = transcriberCallback;
    mTranscriberEnabled = enable;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetTranscriberCallback);
        if (pMsg) {
            pMsg->m_param.apiSetTranscriberCallback.transcriberCallback = (void*)transcriberCallback;
            pMsg->m_param.apiSetTranscriberCallback.enable = enable;
            
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setTranscriberEnabled");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setTranscriberEnabled failed");
    return YOUME_ERROR_MEMORY_OUT;
}

void CYouMeVoiceEngine::videoPreDecodeCallback(void* data, tsk_size_t size, int32_t sessionId, int64_t timestamp) {
    std::string userid = CYouMeVoiceEngine::getInstance()->getUserNameBySessionId(sessionId);
    TMEDIA_I_AM_ACTIVE(100, "== videoPreDecodeCallback data:%p, size:%d, sessionId:%d, userid:%s, ts:%lld", data, size, sessionId, userid.c_str(), timestamp);
    if(NULL != getInstance()->mPVideoPreDecodeCallback) {
        getInstance()->mPVideoPreDecodeCallback->onVideoPreDecode(userid.c_str(), data, size, timestamp);
    }
}

YouMeErrorCode CYouMeVoiceEngine::setVideoPreDecodeCallbackEnable(IYouMeVideoPreDecodeCallback* preDecodeCallback, bool needDecodeandRender)
{
    TSK_DEBUG_INFO ("@@ setVideoPreDecodeCallbackEnable:%p , needDecodeandRender:%d, mPreVideoNeedDecodeandRender:%d", preDecodeCallback,  needDecodeandRender, mPreVideoNeedDecodeandRender);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (mPVideoPreDecodeCallback == preDecodeCallback && mPreVideoNeedDecodeandRender == needDecodeandRender) {
        TSK_DEBUG_INFO("== setVideoPreDecodeCallback already set success");
        return YOUME_SUCCESS;
    }
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== setVideoPreDecodeCallback need after init SDK");
        return YOUME_ERROR_WRONG_STATE;
    }

    if (m_avSessionMgr != NULL) {
        TSK_DEBUG_ERROR("== setVideoPreDecodeCallback need before JoinChannel");
        return YOUME_ERROR_WRONG_STATE;
    }

    mPVideoPreDecodeCallback = preDecodeCallback;
    mPreVideoNeedDecodeandRender = needDecodeandRender;

    TSK_DEBUG_INFO("== setVideoPreDecodeCallbackEnable success");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoEncodeParamCallbackEnable(bool isReport)
{
    TSK_DEBUG_INFO ("@@ setVideoEncodeParamCallbackEnable enable:%d ", isReport);
    m_bVideoEncodeParamReport = isReport;
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setMicLevelCallback(int maxLevel)
{
    TSK_DEBUG_INFO ("@@ setMicLevelCallback:%d", maxLevel);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_nMaxMicLevelCallback = maxLevel;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetMicLevelCallback);
        if (pMsg) {
            pMsg->m_param.i32Value = maxLevel;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setMicLevelCallback");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setMicLevelCallback failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::setFarendVoiceLevelCallback(int maxLevel)
{
    TSK_DEBUG_INFO ("@@ setFarendVoiceLevelCallback:%d", maxLevel);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_nMaxFarendVoiceLevel = maxLevel;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetFarendVoiceLevelCallback);
        if (pMsg) {
            pMsg->m_param.i32Value = maxLevel;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setFarendVoiceLevelCallback");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setFarendVoiceLevelCallback failed");
    return YOUME_ERROR_MEMORY_OUT;
}

YouMeErrorCode CYouMeVoiceEngine::setReleaseMicWhenMute(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setReleaseMicWhenMute:%d", bEnabled);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    m_bReleaseMicWhenMute = bEnabled;
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetReleaseMicWhenMute);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setReleaseMicWhenMute");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setReleaseMicWhenMute failed");
    return YOUME_ERROR_MEMORY_OUT;
}


YouMeErrorCode CYouMeVoiceEngine::setExitCommModeWhenHeadsetPlugin(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setExitCommModeWhenHeadsetPlugin:%d", bEnabled);

#if defined(__APPLE__) && TARGET_OS_OSX
	TSK_DEBUG_INFO ("== setExitCommModeWhenHeadsetPlugin failed, not supported on OSX");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
    
#ifdef WIN32
	TSK_DEBUG_INFO ("== setExitCommModeWhenHeadsetPlugin failed, not supported on Windows");
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    bool bExitCommModeWhenHeadsetPluginAllow = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::EXIT_COMMMODE_API_ENABLE, NgnConfigurationEntry::DEF_EXIT_COMMMODE_API_ENABLE);
    TSK_DEBUG_INFO("== setExitCommModeWhenHeadsetPlugin bExitCommModeWhenHeadsetPluginAllow:%d", bExitCommModeWhenHeadsetPluginAllow);
    if (!bExitCommModeWhenHeadsetPluginAllow) {
        m_bExitCommModeWhenHeadsetPlugin = false;
        TSK_DEBUG_INFO("== setExitCommModeWhenHeadsetPlugin is not allowed");
        return YOUME_ERROR_API_NOT_SUPPORTED;
    }
    
    
    m_bExitCommModeWhenHeadsetPlugin = bEnabled;
    
    TSK_DEBUG_INFO("== setExitCommModeWhenHeadsetPlugin");
    return YOUME_SUCCESS;
}


void CYouMeVoiceEngine::setExternalInputMode( bool bInputModeEnabled )
{
    MediaSessionMgr::defaultsSetExternalInputMode(bInputModeEnabled);
    // 外部输入模式下默认不开启log
    if (bInputModeEnabled && !m_bSetLogLevel) {
        tsk_set_log_maxLevelFile(LOG_DISABLED);
        tsk_set_log_maxLevelConsole(LOG_DISABLED);
    }
    // 为了防止这条被写入文件就放到后面了
    TSK_DEBUG_INFO("@@== setExternalInputMode: bInputModeEnabled:%d", bInputModeEnabled );
}

YouMeErrorCode CYouMeVoiceEngine::openVideoEncoder(const std::string& strFilePath)
{
    TSK_DEBUG_INFO ("@@ openVideoEncoder");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiOpenVideoEncoder);
        if (pMsg) {
            if (NULL == pMsg->m_param.apiOpenVideoEncoder.pStrFilePath) {
                delete pMsg;
                pMsg = NULL;
                return YOUME_ERROR_MEMORY_OUT;
            }
            *(pMsg->m_param.apiOpenVideoEncoder.pStrFilePath) = strFilePath;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== openVideoEncoder");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== openVideoEncoder delayed");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoCallback(IYouMeVideoFrameCallback * cb)
{
    TSK_DEBUG_INFO ("@@ setVideoCallback");
    mPVideoCallback = cb;
    if (cb) {
        setVideoRenderCbEnable(true);
    } else {
        setVideoRenderCbEnable(false);
    }
    YouMeVideoMixerAdapter::getInstance()->setVideoFrameCallback(cb);
    return YOUME_SUCCESS;
}

//IYouMeVideoCallback * CYouMeVoiceEngine::getVideoCallback()
//{
//    return mPVideoCallback;
//}

int CYouMeVoiceEngine::createRender(const std::string & userId)
{
    return CVideoManager::getInstance()->createRender(userId);
}

int CYouMeVoiceEngine::deleteRender(int renderId)
{
    TSK_DEBUG_INFO ("@@ deleteRender");
    {
        lock_guard<recursive_mutex> stateLock(mStateMutex);
        if (!isStateInitialized()) {
            TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
            return YOUME_ERROR_WRONG_STATE;
        }
    }
    CVideoManager::getInstance()->deleteRender(renderId);
    return 0;
}

int CYouMeVoiceEngine::deleteRender(const std::string & userID)
{
    TSK_DEBUG_INFO ("@@ deleteRender");
    {
        lock_guard<recursive_mutex> stateLock(mStateMutex);
        if (!isStateInitialized()) {
            TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
            return YOUME_ERROR_WRONG_STATE;
        }
    }
    CVideoChannelManager::getInstance()->deleteRender(userID.c_str());
    return 0;
}

YouMeErrorCode CYouMeVoiceEngine::startCapture()
{
    TSK_DEBUG_INFO ("@@ startCapture");

    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
#ifdef ANDROID
    JNI_startRequestPermissionForApi23_camera();
#endif

    YouMeErrorCode ret = ICameraManager::getInstance()->startCapture();
    if(YOUME_SUCCESS == ret) {
        AVStatistic::getInstance()->NotifyStartVideo();
        AVStatistic::getInstance()->NotifyVideoStat( mStrUserID,  true );
        this->m_bCameraIsOpen = true;
    }
    
    if (m_pMainMsgLoop) {
//        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiStartCapture);
//        if (pMsg) {
//            pMsg->m_param.apiCamStatusChange.cameraStatus = 1;
//            m_pMainMsgLoop->SendMessage(pMsg);
//            TSK_DEBUG_INFO("== startCapture");
//        }
        CMessageBlock *pMsg_input = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
        if (pMsg_input) {
            pMsg_input->m_param.apiVideoInputStatusChange.inputStatus = 1;
            pMsg_input->m_param.apiVideoInputStatusChange.errorCode = ret;
            m_pMainMsgLoop->SendMessage(pMsg_input);
            TSK_DEBUG_INFO("== startinputVideoFrame");
        }
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_INFO("== startCapture");
    return ret;
}

YouMeErrorCode CYouMeVoiceEngine::stopCapture()
{
    TSK_DEBUG_INFO ("@@ stopCapture");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    YouMeErrorCode ret = ICameraManager::getInstance()->stopCapture();
    
    if(YOUME_SUCCESS == ret || YOUME_ERROR_CAMERA_EXCEPTION == ret) {
        AVStatistic::getInstance()->NotifyVideoStat( mStrUserID,  false );
        this->m_bCameraIsOpen = false;
    }
  
#ifdef ANDROID
    JNI_stopRequestPermissionForApi23_camera();
#endif
    
    if (m_pMainMsgLoop) {
//        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiStopCapture);
//        if (pMsg) {
//            pMsg->m_param.apiCamStatusChange.cameraStatus = 0;
//            m_pMainMsgLoop->SendMessage(pMsg);
//            TSK_DEBUG_INFO("== stopCapture");
//        }
        CMessageBlock *pMsg_input = new(nothrow) CMessageBlock(CMessageBlock::MsgApiVideoInputStatus);
        if (pMsg_input) {
            pMsg_input->m_param.apiVideoInputStatusChange.inputStatus = 0;
            pMsg_input->m_param.apiVideoInputStatusChange.errorCode = ret;
            m_pMainMsgLoop->SendMessage(pMsg_input);
            TSK_DEBUG_INFO("== stopInputVideoFrame");
        }
        return YOUME_SUCCESS;
    }
    
    TSK_DEBUG_INFO("== stopCapture");
    return ret;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoPreviewFps(int fps)
{
    TSK_DEBUG_INFO ("@@ setVideoPreviewFps fps:%d", fps);
    if (fps < 1 || fps > 60) {
        return YOUME_ERROR_INVALID_PARAM;
    }

    unsigned int width = 0;
    unsigned int height = 0;
    tmedia_defaults_get_camera_size( &width, &height );
    ICameraManager::getInstance()->setCaptureProperty(fps, width, height);
    YouMeVideoMixerAdapter::getInstance()->setVideoFps(fps);

    m_previewFps = fps;
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoFps(int fps)
{
    TSK_DEBUG_INFO ("@@ setVideoFps fps:%d", fps);
    if (fps < 1 || fps > 60) {
        return YOUME_ERROR_INVALID_PARAM;
    }

    if (0 == m_previewFps) {
        unsigned int width = 0;
        unsigned int height = 0;
        tmedia_defaults_get_camera_size( &width, &height );
        ICameraManager::getInstance()->setCaptureProperty(fps, width, height);
        YouMeVideoMixerAdapter::getInstance()->setVideoFps(fps);
    }
    
    MediaSessionMgr::setVideoFps(fps);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoFpsForSecond(int fps)
{
    TSK_DEBUG_INFO ("@@ setVideoFpsForSecond fps:%d", fps);
    if (fps < 1 || fps > 30) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    MediaSessionMgr::setVideoFpsForSecond(fps);
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoFpsForShare(int fps)
{
    TSK_DEBUG_INFO ("@@ setVideoFpsForShare fps:%d", fps);
    if (fps < 1 || fps > 60) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    shareFps = fps;
    MediaSessionMgr::setVideoFpsForShare(fps);

    #ifdef ANDROID
        JNI_screenRecorderSetFps(fps);
    #endif
        
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::openBeautify(bool open)
{
    TSK_DEBUG_INFO ("@@ openBeautify %d",open);
    if(open)
       YouMeVideoMixerAdapter::getInstance()->openBeautify();
    else
       YouMeVideoMixerAdapter::getInstance()->closeBeautify();
    //ICameraManager::getInstance()->openBeautify(open);
    TSK_DEBUG_INFO("== openBeautify");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::beautifyChanged(float param)
{
    TSK_DEBUG_INFO ("@@ beautifyChanged %f",param);
    YouMeVideoMixerAdapter::getInstance()->setBeautifyLevel(param);
    //ICameraManager::getInstance()->beautifyChanged(param);
    TSK_DEBUG_INFO("== beautifyChanged");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::stretchFace(bool stretch)
{
    TSK_DEBUG_INFO ("@@ stretchFace %d",stretch);
    ICameraManager::getInstance()->stretchFace(stretch);
    TSK_DEBUG_INFO("== stretchFace");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoLocalResolution(int width, int height)
{
    TSK_DEBUG_INFO ("@@ setVideoLocalResolution width:%d, height:%d", width, height);
    if (width < 0 || height < 0){
        return YOUME_ERROR_INVALID_PARAM;
    }
    int sendFPS = tmedia_defaults_get_video_fps();
    int fps =  m_previewFps > 0 ? m_previewFps : sendFPS;
    ICameraManager::getInstance()->setCaptureProperty(fps, width, height);
    MediaSessionMgr::setVideoLocalResolution(width, height);
    {
        //report stop camera event
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportVideoInfo videoInfo;
        if( mSessionID == -1 )
        {
            videoInfo.sessionid = 0;
        }
        else{
            videoInfo.sessionid = mSessionID;
        }
        
        videoInfo.width = width;
        videoInfo.height = height;
        videoInfo.fps = sendFPS;
        videoInfo.smooth = tsk_true;
        videoInfo.delay = 0;
        videoInfo.sdk_version = SDK_NUMBER;
        videoInfo.platform = NgnApplication::getInstance()->getPlatform();
        videoInfo.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report(videoInfo);
    }
    TSK_DEBUG_INFO("== setVideoLocalResolution");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setCaptureFrontCameraEnable(bool enable)
{
    TSK_DEBUG_INFO ("@@ setCaptureFrontCameraEnable");
    ICameraManager::getInstance()->setCaptureFrontCameraEnable(enable);
    TSK_DEBUG_INFO("== setCaptureFrontCameraEnable");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::switchCamera()
{
    TSK_DEBUG_INFO ("@@ switchCamera");
    ICameraManager::getInstance()->switchCamera();
    TSK_DEBUG_INFO("== switchCamera");
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::resetCamera()
{
    TSK_DEBUG_INFO ("@@ resetCamera");
#ifdef ANDROID
    if (YOUME_SUCCESS == ICameraManager::getInstance()->resetCamera())
    {
        this->m_bCameraIsOpen = true;
    }
    
#endif
    TSK_DEBUG_INFO("== resetCamera");
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::getSdkInfo(string &strInfo)
{
    //暂时先这么处理，自己去搞吧。 joexie
#ifdef WIN32
    TSK_DEBUG_INFO("@@ getDebugInfo");
    char szInfo[64];
    strInfo = "";
    _snprintf(szInfo, sizeof(szInfo), "sdkver:%s_%d.%d.%d.%d\n",
              BRANCH_TAG_NAME, MAIN_VER, MINOR_VER, RELEASE_NUMBER, BUILD_NUMBER);
    strInfo += szInfo;
    if (mRedirectServerInfoVec.size() > 0) {
        _snprintf(szInfo, sizeof(szInfo), "redirect: %s:%d\n", mRedirectServerInfoVec[0].host.c_str(), mRedirectServerInfoVec[0].port);
        strInfo += szInfo;
    }
    _snprintf(szInfo, sizeof(szInfo), "mcu: %s:%d\n", mMcuAddr.c_str(), mMcuSignalPort);
    strInfo += szInfo;
#else
    TSK_DEBUG_INFO("@@ getDebugInfo");
    char szInfo[64];
    strInfo = "";
    snprintf(szInfo, sizeof(szInfo), "sdkver:%s_%d.%d.%d.%d BGM:%d\n",
             BRANCH_TAG_NAME, MAIN_VER, MINOR_VER, RELEASE_NUMBER, BUILD_NUMBER, FFMPEG_SUPPORT);
    strInfo += szInfo;
    if (mRedirectServerInfoVec.size() > 0) {
        snprintf(szInfo, sizeof(szInfo), "redirect: %s:%d\n", mRedirectServerInfoVec[0].host.c_str(), mRedirectServerInfoVec[0].port);
        strInfo += szInfo;
    }
    snprintf(szInfo, sizeof(szInfo), "mcu: %s:%d\n", mMcuAddr.c_str(), mMcuSignalPort);
    strInfo += szInfo;
#endif // WIN32
    
}

YouMeErrorCode CYouMeVoiceEngine::setGrabMicOption(const std::string & strChannelID, int mode, int maxAllowCount, int maxTalkTime, unsigned int voteTime)
{
    TSK_DEBUG_INFO("@@ setGrabMicOption ChannelID:%s Mode:%d MaxAllowCount:%d MaxMicTime:%d VoteTime:%d",
                   strChannelID.c_str(), mode, maxAllowCount, maxTalkTime, voteTime);
    
    CRoomManager::RoomInfo_t* pinfo = m_RoomPropMgr->findRoomInfo(strChannelID);
    if (pinfo){
        pinfo->grabmicInfo.maxAllowCount = maxAllowCount;
        pinfo->grabmicInfo.maxTalkTime = maxTalkTime;
        pinfo->grabmicInfo.voteTime = voteTime;
        pinfo->grabmicInfo.mode = mode;
    }
    else{
        CRoomManager::RoomInfo_t info;
        info.grabmicInfo.maxAllowCount = maxAllowCount;
        info.grabmicInfo.maxTalkTime = maxTalkTime;
        info.grabmicInfo.voteTime = voteTime;
        info.grabmicInfo.mode = mode;
        m_RoomPropMgr->addRoom(strChannelID, info);
    }
    
    return YouMeErrorCode::YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::startGrabMicAction(const std::string & strChannelID, const string& strContent)
{
    TSK_DEBUG_INFO("@@ startGrabMicAction ChannelID:%s Content:%s", strChannelID.c_str(), strContent.c_str());
    
    if (strChannelID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== startGrabMicAction wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiStartGrabMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiStartGrabMic.roomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiStartGrabMic.pJsonData && !strContent.empty()){
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiStartGrabMic.roomID) = strChannelID;
        *(pMsg->m_param.apiStartGrabMic.pJsonData) = strContent;
        
        CRoomManager::RoomInfo_t info;
        if (!m_RoomPropMgr->getRoomInfo(strChannelID, info)){
            info.grabmicInfo.maxAllowCount = 1;
            info.grabmicInfo.maxTalkTime = 30;
            info.grabmicInfo.voteTime = 30;
            info.grabmicInfo.mode = 1;
        }
        
        pMsg->m_param.apiStartGrabMic.maxCount = info.grabmicInfo.maxAllowCount;
        pMsg->m_param.apiStartGrabMic.maxTime = info.grabmicInfo.maxTalkTime;
        pMsg->m_param.apiStartGrabMic.mode = info.grabmicInfo.mode;
        pMsg->m_param.apiStartGrabMic.voteTime = info.grabmicInfo.voteTime;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== startGrabMicAction");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== startGrabMicAction failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::stopGrabMicAction(const std::string & strChannelID, const string& strContent)
{
    TSK_DEBUG_INFO("@@ stopGrabMicAction ChannelID:%s Content:%s",
                   strChannelID.c_str(), strContent.c_str());
    
    if (strChannelID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== stopGrabMicAction wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiStopGrabMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiStopGrabMic.roomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiStopGrabMic.pJsonData && !strContent.empty()){
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiStopGrabMic.roomID) = strChannelID;
        *(pMsg->m_param.apiStopGrabMic.pJsonData) = strContent;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== stopGrabMicAction");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== stopGrabMicAction failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::requestGrabMic(const std::string & strChannelID, int score, bool isAutoOpenMic, const string& strContent)
{
    TSK_DEBUG_INFO("@@ requestGrabMic ChannelID:%s score:%d isAutoOpenMic:%d Content:%s",
                   strChannelID.c_str(), score, isAutoOpenMic, strContent.c_str());
    
    if (strChannelID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== requestGrabMic wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiReqGrabMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiReqGrabMic.roomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        if (NULL == pMsg->m_param.apiReqGrabMic.pJsonData && !strContent.empty()){
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        *(pMsg->m_param.apiReqGrabMic.roomID) = strChannelID;
        *(pMsg->m_param.apiReqGrabMic.pJsonData) = strContent;
        pMsg->m_param.apiReqGrabMic.score = score;
        pMsg->m_param.apiReqGrabMic.bAutoOpen = isAutoOpenMic;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== requestGrabMic");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== requestGrabMic failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::releaseGrabMic(const std::string & strChannelID)
{
    TSK_DEBUG_INFO("@@ releaseGrabMic ChannelID:%s",
                   strChannelID.c_str());
    
    if (strChannelID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== releaseGrabMic wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiFreeGrabMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiFreeGrabMic.roomID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiFreeGrabMic.roomID) = strChannelID;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== releaseGrabMic");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== releaseGrabMic failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::setInviteMicOption(const std::string & strChannelID, int waitTimeout, int maxTalkTime)
{
    TSK_DEBUG_INFO("@@ setInviteMicOption ChannelID:%s waitTimeout:%d maxMicTime:%d",
                   strChannelID.c_str(), waitTimeout, maxTalkTime);
    
    CRoomManager::RoomInfo_t* pinfo = m_RoomPropMgr->findRoomInfo(strChannelID);
    if (pinfo){
        pinfo->invitemicInfo.waitTimeout = waitTimeout;
        pinfo->invitemicInfo.maxTalkTime = maxTalkTime;
        pinfo->invitemicInfo.bStateBroadcast = true;
    }
    else{
        CRoomManager::RoomInfo_t info;
        info.invitemicInfo.waitTimeout = waitTimeout;
        info.invitemicInfo.maxTalkTime = maxTalkTime;
        info.invitemicInfo.bStateBroadcast = true;
        m_RoomPropMgr->addRoom(strChannelID, info);
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== setInviteMicOption wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiInitInviteMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiInitInviteMic.roomID) = strChannelID;
        pMsg->m_param.apiInitInviteMic.waitTime = waitTimeout;
        pMsg->m_param.apiInitInviteMic.talkTime = maxTalkTime;
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== setInviteMicOption");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== setInviteMicOption failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::requestInviteMic(const std::string& strChannelID, const std::string & strUserID, const string& strContent)
{
    TSK_DEBUG_INFO("@@ requestInviteMic ChannelID:%s UserID:%s Content:%s", strChannelID.c_str(), strUserID.c_str(), strContent.c_str());
    
    if (strUserID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== requestInviteMic wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiReqInviteMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiReqInviteMic.userID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiReqInviteMic.pJsonData && !strContent.empty()){
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiReqInviteMic.roomID) = strChannelID;
        *(pMsg->m_param.apiReqInviteMic.userID) = strUserID;
        *(pMsg->m_param.apiReqInviteMic.pJsonData) = strContent;
        
        CRoomManager::RoomInfo_t info;
        if (!m_RoomPropMgr->getRoomInfo(strChannelID, info)){
            info.invitemicInfo.waitTimeout = 30;
            info.invitemicInfo.maxTalkTime = -1;
            info.invitemicInfo.bStateBroadcast = true;
        }
        
        pMsg->m_param.apiReqInviteMic.waitTime = info.invitemicInfo.waitTimeout;
        pMsg->m_param.apiReqInviteMic.maxTime = info.invitemicInfo.maxTalkTime;
        pMsg->m_param.apiReqInviteMic.bBroadcast = info.invitemicInfo.bStateBroadcast;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== requestInviteMic");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== requestInviteMic failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::responseInviteMic(const std::string & strUserID, bool isAccept, const string& strContent)
{
    TSK_DEBUG_INFO("@@ responseInviteMic UserID:%s isAccept:%d Content:%s",
                   strUserID.c_str(), isAccept, strContent.c_str());
    
    if (strUserID.empty()) {
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== responseInviteMic wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiRspInviteMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiRspInviteMic.userID) {
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        if (NULL == pMsg->m_param.apiRspInviteMic.pJsonData && !strContent.empty()){
            delete pMsg;
            pMsg = NULL;
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiRspInviteMic.roomID) = "";
        *(pMsg->m_param.apiRspInviteMic.userID) = strUserID;
        *(pMsg->m_param.apiRspInviteMic.pJsonData) = strContent;
        pMsg->m_param.apiRspInviteMic.bAccept = isAccept;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== responseInviteMic");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== responseInviteMic failed to send message");
    return errCode;
}


YouMeErrorCode CYouMeVoiceEngine::stopInviteMic()
{
    TSK_DEBUG_INFO("@@ stopInviteMic ");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== stopInviteMic wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiStopInviteMic);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiStopInviteMic.roomID) = "";
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== stopInviteMic");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== stopInviteMic failed to send message");
    return errCode;
}

YouMeErrorCode  CYouMeVoiceEngine::sendMessage( const char* pChannelID, const char* pContent, const char* pToUserID, int* requestID  ){
    TSK_DEBUG_INFO("@@ sendMessage ");
    
    if( pChannelID == nullptr || strlen(pChannelID) == 0
       || pContent == nullptr ||  strlen(pContent) == 0  ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if( strlen(pContent) >= 3 * 1024 ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== sendMessage wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if( !m_pRoomMgr->isInRoom(pChannelID) ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiSendMessage);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        *(pMsg->m_param.apiSendMessage.roomID) = pChannelID;
        *(pMsg->m_param.apiSendMessage.content) = pContent;
        if (pToUserID != NULL) {
            *(pMsg->m_param.apiSendMessage.toUserID) = pToUserID;
        }
        pMsg->m_param.apiSendMessage.requestID = GetUniqueSerial();
        
        if( requestID != nullptr ){
            *requestID = pMsg->m_param.apiSendMessage.requestID;
        }
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== sendMessage");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== sendMessage failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::kickOther( const char* pUserID, const char* pChannelID , int lastTime )
{
    TSK_DEBUG_INFO("@@ kickOther ");
    
    if( pChannelID == nullptr || strlen(pChannelID) == 0
       || pUserID == nullptr ||  strlen(pUserID) == 0  ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== kickOther wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if( !m_pRoomMgr->isInRoom(pChannelID) ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiKickOther);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        *(pMsg->m_param.apiKickOther.roomID) = pChannelID;
        *(pMsg->m_param.apiKickOther.userID ) = pUserID;
        pMsg->m_param.apiKickOther.lastTime  = lastTime;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== kickOther");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== kickOther failed to send message");
    return errCode;
}

YouMeErrorCode CYouMeVoiceEngine::setUserRole(YouMeUserRole_t eUserRole)
{
    TSK_DEBUG_INFO("@@ setUserRole %d %d", (int)eUserRole, (int)mRole);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (NULL == m_avSessionMgr) {
        TSK_DEBUG_INFO ("== setUserRole : m_avSessionMgr is NULL, channel not exist");
        return YOUME_ERROR_CHANNEL_NOT_EXIST;
    }

    if (mRole == eUserRole){
        TSK_DEBUG_INFO("== setUserRole is same role!");
        return YOUME_SUCCESS;
    }
    mRole = eUserRole;
    
    switch (eUserRole) {
        case YOUME_USER_TALKER_FREE:
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_TALKER_ON_DEMAND:
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_GUSET:
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_LISTENER:
            mAllowPlayBGM = false;
            mAllowMonitor = false;
            break;
        case YOUME_USER_COMMANDER:
        case YOUME_USER_HOST:
            mAllowPlayBGM = true;
            mAllowMonitor = true;
            break;
        default:
            TSK_DEBUG_ERROR("== Invalid UserRole:%d", (int)eUserRole);
            return YOUME_ERROR_INVALID_PARAM;
    }
    
    if (!mAllowPlayBGM){
        if (m_bBgmStarted){
            stopBackgroundMusic();
        }
    }
    
    if (!mAllowMonitor){
		if (m_bMicBypassToSpeaker || m_bBgmBypassToSpeaker){
			setHeadsetMonitorOn(false, false);
		}
	}
    
    bool needmic = NeedMic(eUserRole);
    
    TSK_DEBUG_INFO("== needmic: now:%d old:%d mute:%d",
                   (int)needmic, (int)mNeedMic, (int)m_bMicMute);
    
    if (needmic != mNeedMic){
        mNeedMic = needmic;
        
        if(mNeedMic == m_bMicMute){
            setMicrophoneMute(!mNeedMic);
        }
    }
        
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
    
    TSK_DEBUG_INFO ("SendMsg type %d to %s",YouMeProtocol::IDENTITY_MODIFY,mStrUserID.c_str());
    YouMeProtocol::YouMeVoice_Command_CommonStatus commonStatusEvent;
    commonStatusEvent.set_allocated_head(CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_COMMON_STATUS));
    commonStatusEvent.set_eventtype(YouMeProtocol::IDENTITY_MODIFY);
    commonStatusEvent.set_userid(mStrUserID);
    commonStatusEvent.set_sessionid (mSessionID);
    commonStatusEvent.set_status(role);
    std::string strRoleData;
    commonStatusEvent.SerializeToString(&strRoleData);
    m_loginService.AddTCPQueue(YouMeProtocol::MSG_COMMON_STATUS, strRoleData.c_str(), strRoleData.length());
    
    return YOUME_SUCCESS;
}

YouMeUserRole_t CYouMeVoiceEngine::getUserRole()
{
    TSK_DEBUG_INFO("@@== getUserRole:%d", (int)mRole);
    return mRole;
}

bool CYouMeVoiceEngine::isBackgroundMusicPlaying()
{
    TSK_DEBUG_INFO("@@== isBackgroundMusicPlaying:%d/%d", (int)m_bBgmStarted, (int)m_bBgmPaused);
    return m_bBgmStarted;
}

bool CYouMeVoiceEngine::isInited()
{
    TSK_DEBUG_INFO("@@== isInited");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    return isStateInitialized();
}

bool CYouMeVoiceEngine::isInRoom(const string& strChannelID)
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if(isStateInitialized()) {
        TSK_DEBUG_INFO("@@== isInRoom:%s", strChannelID.c_str());
        return m_pRoomMgr->isInRoom(strChannelID);
    }
    return false;
}

bool CYouMeVoiceEngine::isInRoom()
{
    TSK_DEBUG_INFO("@@== isInRoom");
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if(isStateInitialized()) {
        return m_pRoomMgr->getRoomCount() > 0;
    }
    return false;
}

YouMeErrorCode CYouMeVoiceEngine::setUserLogPath(const std::string& strFilePath)
{
    TSK_DEBUG_INFO("@@ setUserLogPath:%s", strFilePath.c_str());
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (mState == STATE_INITIALIZING || isStateInitialized())
    {
        TSK_DEBUG_ERROR("== setUserLogPath failed: already init");
        return YOUME_ERROR_WRONG_STATE;
    }
    
    int dirIndex;
    youmecommon::CXFile fileMgr;
    // try touch the file
#ifdef WIN32
    dirIndex = strFilePath.find_last_of("\\");
#else
    dirIndex =  strFilePath.find_last_of("/");
#endif
    if(dirIndex <= 0){
        TSK_DEBUG_INFO("== setUserLogPath faild:%s", strFilePath.c_str());
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string dir(strFilePath.substr(0, dirIndex + 1));
#ifndef WIN32
    if (!fileMgr.make_dir_tree(dir.c_str()))
    {
        TSK_DEBUG_INFO("== setUserLogPath faild:%s", strFilePath.c_str());
        return YOUME_ERROR_INVALID_PARAM;
    }
#else
    unsigned len = dir.size() * 2;// 预留字节数
    setlocale(LC_CTYPE, "");     //必须调用此函数
    wchar_t *p = new wchar_t[len];// 申请一段内存存放转换后的字符串
    mbstowcs(p, dir.c_str(), len);// 转换
    std::wstring str(p);
    delete[] p;// 释放申请的内存
    if (!fileMgr.make_dir_tree(str.c_str()))
    {
        TSK_DEBUG_INFO("== setUserLogPath faild:%s", strFilePath.c_str());
        return YOUME_ERROR_INVALID_PARAM;
    }
#endif
    /*if (fileMgr.LoadFile(strFilePath, youmecommon::CXFile::Mode_Open_ALWAYS) != 0 )
    {
        TSK_DEBUG_INFO("== SetUserLogPath faild:%s", strFilePath.c_str());
        return YOUME_ERROR_INVALID_PARAM;
    }
    fileMgr.Close();*/
    
    NgnApplication::getInstance()->setUserLogPath(strFilePath);
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::doStartGrabMicAction(const std::string & strChannelID, int maxAllowCount, int maxMicTime, int mode, unsigned int voteTime, const string& strContent)
{
    TSK_DEBUG_INFO("$$ doStartGrabMicAction");
    
    {
        YouMeProtocol::YouMeVoice_Command_Fight4MicInitRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_FIGHT_4_MIC_INIT));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(ToYMRoomID(strChannelID));
        getReq.set_mode(mode);
        getReq.set_max_num(maxAllowCount);
        getReq.set_talk_time_out(maxMicTime);
        getReq.set_judge_time_out(voteTime);
        getReq.set_mic_enable_flag(0);
        getReq.set_notify_flag(1);
        getReq.set_json_str(strContent);
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_FIGHT_4_MIC_INIT, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doStartGrabMicAction end");
}


void CYouMeVoiceEngine::doStopGrabMicAction(const std::string & strChannelID, const string& strContent)
{
    TSK_DEBUG_INFO("$$ doStopGrabMicAction");
    
    {
        YouMeProtocol::YouMeVoice_Command_Fight4MicDeinitRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_FIGHT_4_MIC_DEINIT));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(ToYMRoomID(strChannelID));
        getReq.set_json_str(strContent);
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_FIGHT_4_MIC_DEINIT, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doStopGrabMicAction end");
}


void CYouMeVoiceEngine::doRequestGrabMic(const std::string & strChannelID, int score, bool isAutoOpenMic, const string& strContent)
{
    TSK_DEBUG_INFO("$$ doRequestGrabMic");
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strChannelID, roomInfo)){
        sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_GRABMIC_REQUEST_FAILED, YOUME_ERROR_CHANNEL_NOT_EXIST, strChannelID.c_str(), strContent.c_str());
        TSK_DEBUG_ERROR("[doRequestGrabMic]== not in the room %s", strChannelID.c_str());
        return;
    }
    
    m_bAutoOpenMic = isAutoOpenMic;
    
    {
        YouMeProtocol::YouMeVoice_Command_Fight4MicRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_FIGHT_4_MIC));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(roomInfo.idFull);
        getReq.set_json_params(strContent);
        getReq.set_score(score);
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_FIGHT_4_MIC, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doRequestGrabMic end");
}


void CYouMeVoiceEngine::doFreeGrabMic(const std::string & strChannelID)
{
    TSK_DEBUG_INFO("$$ doFreeGrabMic");
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strChannelID, roomInfo)){
        sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_GRABMIC_RELEASE_FAILED, YOUME_ERROR_CHANNEL_NOT_EXIST, strChannelID.c_str());
        TSK_DEBUG_ERROR("[doFreeGrabMic]== not in the room %s", strChannelID.c_str());
        return;
    }
    
    {
        YouMeProtocol::YouMeVoice_Command_ReleaseMicRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_RELEASE_MIC));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(roomInfo.idFull);
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_RELEASE_MIC, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doFreeGrabMic end");
}


void CYouMeVoiceEngine::doRequestInviteMic(const std::string & strChannelID, const std::string & strUserID, int waitTimeout, int maxMicTime, bool isNeedBroadcast, const string& strContent)
{
    TSK_DEBUG_INFO("$$ doRequestInviteMic");
    
    CRoomManager::RoomInfo_t roomInfo;
    if (!m_pRoomMgr->getRoomInfo(strChannelID, roomInfo)){
        sendCbMsgCallEvent(YouMeEvent::YOUME_EVENT_INVITEMIC_REQUEST_FAILED, YOUME_ERROR_CHANNEL_NOT_EXIST, strChannelID.c_str(), strContent.c_str());
        TSK_DEBUG_ERROR("[doRequestInviteMic]== not in the room %s", strChannelID.c_str());
        return;
    }
    
    {
        YouMeProtocol::YouMeVoice_Command_InviteRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_INVITE));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(roomInfo.idFull);
        getReq.set_to_user_id(strUserID);
        getReq.set_notify_flag(isNeedBroadcast ? 1 : 0);
        getReq.set_json_str(strContent);
        getReq.set_connect_time_out(waitTimeout);
        getReq.set_talk_time_out(maxMicTime);
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_INVITE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doRequestInviteMic end");
}


void CYouMeVoiceEngine::doResponseInviteMic(const std::string & strChannelID, const std::string & strUserID, bool isAccept, const string& strContent)
{
    TSK_DEBUG_INFO("$$ doResponseInviteMic");
    
    {
        YouMeProtocol::YouMeVoice_Command_AcceptRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_ACCEPT));
        getReq.set_session_id(mSessionID);
        getReq.set_to_user_id(strUserID);
        getReq.set_json_str(strContent);
        getReq.set_error_code(isAccept ? YouMeProtocol::INVITE_ERROR_CODE::INVITE_SUCCESS : YouMeProtocol::INVITE_ERROR_CODE::INVITE_USER_REJECT);
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_ACCEPT, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doResponseInviteMic end");
}


void CYouMeVoiceEngine::doStopInviteMic(const std::string & strChannelID)
{
    TSK_DEBUG_INFO("$$ doStopInviteMic");
    
    {
        YouMeProtocol::YouMeVoice_Command_InviteByeRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_INVITE_BYE));
        getReq.set_session_id(mSessionID);
        
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_INVITE_BYE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doStopInviteMic end");
}


void CYouMeVoiceEngine::doInitInviteMic(const std::string & strChannelID, int waitTimeout, int maxMicTime)
{
    TSK_DEBUG_INFO("$$ doInitInviteMic");
    
    {
        YouMeProtocol::YouMeVoice_Command_InviteInitRequest  getReq;
        getReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_INVITE_INIT));
        getReq.set_session_id(mSessionID);
        getReq.set_room_id(strChannelID);
        getReq.set_connect_time_out(waitTimeout);
        getReq.set_talk_time_out(maxMicTime);
        std::string strReqData;
        getReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_INVITE_INIT, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doInitInviteMic end");
}

void CYouMeVoiceEngine::doSendMessage( int requestID, const std::string & strChannelID, const std::string & strContent, const std::string & strToUserID){
    TSK_DEBUG_INFO("$$ doSendMessage");
    
    {
        YouMeProtocol::YouMeVoice_Command_SendMessageRequest  sendMsgReq;
        sendMsgReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_SEND_MESSAGE));
        sendMsgReq.set_sessionid(mSessionID);
        
        sendMsgReq.set_to_channel_id( ToYMRoomID(strChannelID) );
        sendMsgReq.set_msg_content( strContent );
        if (!strToUserID.empty()) {
            sendMsgReq.set_to_userid( strToUserID );
        }
        
        sendMsgReq.set_msg_id( requestID );
        std::string strReqData;
        sendMsgReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_SEND_MESSAGE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doSendMessage end");
}

void CYouMeVoiceEngine::doKickOther( const std::string& strChannelID, const std::string& strUserID,  int lastTime )
{
    TSK_DEBUG_INFO("$$ doKickOther");
    
    {
        YouMeProtocol::YouMeVoice_Command_KickingRequest  kickReq;
        kickReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_KICK_USRE));
        kickReq.set_sessionid(mSessionID);
        
        kickReq.set_channel_id( ToYMRoomID( strChannelID) );
        kickReq.set_user_id( strUserID );
        kickReq.set_kick_time( lastTime );
        
        std::string strReqData;
        kickReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_KICK_USRE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doKickOther end");
    
}

void CYouMeVoiceEngine::doStartPush( std::string roomid, std::string userid,std::string pushUrl  )
{
    TSK_DEBUG_INFO("$$ doStartPush");
    
    {
        YouMeProtocol::YouMeVoice_Video_SetPushSingle_Req  pushReq;
        pushReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_SET_PUSH_SINGLE));
        pushReq.set_sessionid(mSessionID);
        
        pushReq.set_channelid( ToYMRoomID( roomid) );
        pushReq.set_user_id( userid );
        pushReq.set_push_url( pushUrl );
        
        std::string strReqData;
        pushReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_SET_PUSH_SINGLE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doStartPush end");
    
}
void CYouMeVoiceEngine::doStopPush( std::string roomid, std::string userid )
{
    TSK_DEBUG_INFO("$$ doStopPush");
    
    {
        YouMeProtocol::YouMeVoice_Video_RemovePushSingle_Req  stopPushReq;
        stopPushReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_REMOVE_PUSH_SINGLE));
        stopPushReq.set_sessionid(mSessionID);
        
        stopPushReq.set_channelid( ToYMRoomID( roomid) );
        stopPushReq.set_user_id( userid );
        
        std::string strReqData;
        stopPushReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_REMOVE_PUSH_SINGLE, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doStopPush end");
    
}

void CYouMeVoiceEngine::doSetPushMix(  std::string roomid, std::string userid, std::string pushUrl, int width, int height  )
{
    TSK_DEBUG_INFO("$$ doSetPushMix");
    
    {
        YouMeProtocol::YouMeVoice_Video_SetPushMix_Req  setPushReq;
        setPushReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_SET_PUSH_MIX));
        setPushReq.set_sessionid(mSessionID);
        
        setPushReq.set_channelid( ToYMRoomID( roomid) );
        setPushReq.set_primary_userid( userid );
        setPushReq.set_push_url( pushUrl );
        setPushReq.set_video_width( width );
        setPushReq.set_video_height( height );
        
        std::string strReqData;
        setPushReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_SET_PUSH_MIX, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doSetPushMix end");
    
}
void CYouMeVoiceEngine::doClearPushMix(  std::string roomid )
{
    TSK_DEBUG_INFO("$$ doClearPushMix");
    
    {
        YouMeProtocol::YouMeVoice_Video_ClearPushMix_Req  clearPushReq;
        clearPushReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_CLEAR_PUSH_MIX));
        clearPushReq.set_sessionid(mSessionID);
        
        clearPushReq.set_channelid( ToYMRoomID( roomid) );
        
        std::string strReqData;
        clearPushReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_CLEAR_PUSH_MIX, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doClearPushMix end");
    
}

void CYouMeVoiceEngine::doAddPushMixUser( std::string roomid, std::string userid, int x, int y, int z, int width, int height )
{
    TSK_DEBUG_INFO("$$ doAddPushMixUser");
    
    {
        YouMeProtocol::YouMeVoice_Video_AddPushMixUser_Req  addPushReq;
        addPushReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_ADD_PUSH_MIX_USER));
        addPushReq.set_sessionid(mSessionID);
        
        addPushReq.set_channelid( ToYMRoomID( roomid) );
        addPushReq.set_userid( userid );
        addPushReq.set_x( x );
        addPushReq.set_y( y );
        addPushReq.set_z( z );
        addPushReq.set_video_width( width );
        addPushReq.set_video_height( height );
        
        std::string strReqData;
        addPushReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_ADD_PUSH_MIX_USER, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doAddPushMixUser end");
    
}
void CYouMeVoiceEngine::doRemovePushMixUser( std::string roomid, std::string userid )
{
    TSK_DEBUG_INFO("$$ doAddPushMixUser");
    
    {
        YouMeProtocol::YouMeVoice_Video_DelPushMixUser_Req  removePushUserReq;
        removePushUserReq.set_allocated_head(CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_REMOVE_PUSH_MIX_USER));
        removePushUserReq.set_sessionid(mSessionID);
        
        removePushUserReq.set_channelid( ToYMRoomID( roomid) );
        removePushUserReq.set_userid( userid );
        
        std::string strReqData;
        removePushUserReq.SerializeToString(&strReqData);
        
        m_loginService.AddTCPQueue(YouMeProtocol::MSG_REMOVE_PUSH_MIX_USER, strReqData.c_str(), strReqData.length());
    }
    
    TSK_DEBUG_INFO("$$ doAddPushMixUser end");
    
}

void CYouMeVoiceEngine::doCallEvent(const YouMeEvent eventType, const YouMeErrorCode errCode /* = YOUME_SUCCESS */, const std::string& strRoomID /*= ""*/, const std::string& strParam /*= ""*/)
{
    sendCbMsgCallEvent(eventType, errCode, strRoomID, strParam);
    /*
     if (mPEventCallback) {
     TSK_DEBUG_INFO("Call back event:%d errcode:%d room:%s param:%s", eventType, errCode, strRoomID.c_str(), strParam.c_str());
     mPEventCallback->onEvent(eventType, errCode, strRoomID.c_str(), strParam.c_str());
     }
     */
}

/****
 * 获取重连间隔时间 毫秒
 */
uint32_t CYouMeVoiceEngine::getReconnectWaitMsec(int retry)
{
    uint32_t msec = 0;
    
    /*****************************************
     *       |  base |  rand | time(second)
     * ------|-------|-------|----------------
     *   0   |   0   |   1   | rand( 0 -  1)
     *   1   |   1   |   1   | rand( 1 -  2)
     *   2   |   2   |   1   | rand( 2 -  3)
     *   3   |   3   |   2   | rand( 3 -  5)
     *   4   |   5   |   2   | rand( 5 -  7)
     *   5   |   7   |   3   | rand( 7 - 10)
     *   6   |   10  |   5   | rand(10 - 15)
     *   7   |   15  |   5   | rand(15 - 20)
     *   8   |   20  |   5   | rand(20 - 25)
     *   9   |   25  |   5   | rand(25 - 30)
     *  10   |   30  |   5   | rand(30 - 35)
     *****************************************/
    int reconnectMsec[11][2] = {
        {0,    1},
        {1,    1},
        {2,    1},
        {3,    2},
        {5,    2},
        {7,    3},
        {10,   5},
        {15,   5},
        {20,   5},
        {25,   5},
        {30,   5}
    };
    
    srand((unsigned)time(NULL) + retry);
    
    if (retry < 0 || retry > 10) {
        msec = 30 * 1000 + rand() % 5000; /* default or more then 11 times */
    } else {
        msec = reconnectMsec[retry][0] * 1000 + rand() % (reconnectMsec[retry][1] * 1000);
    }
    
    return msec;
}

YouMeErrorCode CYouMeVoiceEngine::setVideoRenderCbEnable(bool bEnabled)
{
    TSK_DEBUG_INFO ("@@ setVideoRenderCbEnable");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetVideoRenderCbEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = bEnabled;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setVideoRen derCbEnable");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setVideoRenderCbEnable failed");
    return YOUME_ERROR_MEMORY_OUT;
}

std::string CYouMeVoiceEngine::getShareUserName( std::string userName )
{
    if( userName.empty() )
    {
        return "";
    }
    else{
        std::stringstream ss;
        ss<<userName<<"_share";
        return ss.str();
    }
}

const std::string CYouMeVoiceEngine::addNewSession(int32_t sessionId, int32_t i4Width, int32_t i4Height, int videoid){

	std::string userId;
	std::shared_ptr<CVideoRenderInfo> renderInfo = nullptr;


	if (2 == videoid) {
		// for share video steam
		//        std::string share_uid = "youme_share";
		// sessionId = -2;
		renderInfo = CVideoChannelManager::getInstance()->getSessionToRenderInfo(sessionId);
		if (NULL == renderInfo) {
			TMEDIA_I_AM_ACTIVE(60, "can not get share renderinfo.  sessionid:%d", sessionId);
			std::shared_ptr<CVideoUserInfo> userInfo = CVideoChannelManager::getInstance()->getUserInfo(sessionId);
			if (userInfo && !userInfo->userId.empty()) {
				userId = userInfo->userId;
				userInfo->m_isVideo = true;
				userInfo->m_initWidth = i4Width;
				userInfo->m_initHeight = i4Height;
				CVideoChannelManager::getInstance()->insertUser(sessionId, userInfo->userId);
			}
			else{
				std::string share_uid = "";
				//share的session取了负数，所以这里找一下，原本session的名字
				std::shared_ptr<CVideoUserInfo> userInfoVideo = CVideoChannelManager::getInstance()->getUserInfo(-sessionId);
				if (userInfoVideo && !userInfoVideo->userId.empty())
				{
					share_uid = getShareUserName(userInfoVideo->userId);
				}

				CVideoChannelManager::getInstance()->insertUser(sessionId, share_uid);
				std::shared_ptr<CVideoUserInfo> userInfo = CVideoChannelManager::getInstance()->getUserInfo(sessionId);
				if (userInfo){
					userInfo->m_isVideo = true;
					userInfo->m_initWidth = i4Width;
					userInfo->m_initHeight = i4Height;
				}
			}
		}
		else {
			userId = renderInfo->userId;
			if (renderInfo->m_isFristVideo){
				renderInfo->m_isFristVideo = false;
				CYouMeVoiceEngine::getInstance()->OnReciveOtherVideoDataOnNotify(userId, CNgnTalkManager::getInstance()->m_strTalkRoomID, videoid, i4Width, i4Height);
			}
		}
	}
	else {

		// std::shared_ptr<CVideoUserInfo> userInfo = CVideoChannelManager::getInstance()->getUserInfo(sessionId);
		renderInfo = CVideoChannelManager::getInstance()->getSessionToRenderInfo(sessionId);
		if (NULL == renderInfo) {
			TMEDIA_I_AM_ACTIVE(60, "this session is coming first. sessionId:%d", sessionId);
			std::shared_ptr<CVideoUserInfo> userInfo = CVideoChannelManager::getInstance()->getUserInfo(sessionId);
			if (userInfo && !userInfo->userId.empty()) {
				userId = userInfo->userId;
				userInfo->m_isVideo = true;
				userInfo->m_initWidth = i4Width;
				userInfo->m_initHeight = i4Height;
				CVideoChannelManager::getInstance()->insertUser(sessionId, userInfo->userId);
			}
			else{
				CVideoChannelManager::getInstance()->insertUser(sessionId);
				std::shared_ptr<CVideoUserInfo> userInfo = CVideoChannelManager::getInstance()->getUserInfo(sessionId);
				if (userInfo){
					userInfo->m_isVideo = true;
					userInfo->m_initWidth = i4Width;
					userInfo->m_initHeight = i4Height;
				}
			}

		}
		else {
			userId = renderInfo->userId;
			if (renderInfo->m_isFristVideo){
				renderInfo->m_isFristVideo = false;
				CYouMeVoiceEngine::getInstance()->OnReciveOtherVideoDataOnNotify(userId, CNgnTalkManager::getInstance()->m_strTalkRoomID, videoid, i4Width, i4Height);
			}
		}
	}
	return userId;
}

void CYouMeVoiceEngine::videoRenderCb(int32_t sessionId, int32_t i4Width, int32_t i4Height, int32_t i4Rotation, int32_t i4BufSize, const void *vBuf, uint64_t timestamp, int videoid)
{
    TMEDIA_I_AM_ACTIVE(100, "CYouMeVoiceEngine::videoRenderCb sessionid[%d] videoid[%d]", sessionId, videoid);
	std::string userId = addNewSession(sessionId, i4Width, i4Height, videoid);
    AVStatistic::getInstance()->addVideoFrame( 1 , sessionId );
    {
        //为了上报，要记录远端视频的分辨率
        std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
        auto iter = m_mapOtherResolution.find( sessionId );
        if( iter == m_mapOtherResolution.end() ){
            InnerSize size;
            size.width = i4Width;
            size.height = i4Height;
            
            m_mapOtherResolution[ sessionId ] = size;
            
            //TSK_DEBUG_INFO("PinkyLog: set Resolution:session:%d, width:%d, height:%d", sessionId, size.width, size.height );
        }
    }
    
    if (userId.empty()) {
            TMEDIA_I_AM_ACTIVE(60, "user render is not ready!!! (sessionId=%d)", sessionId);
    } else {
       if (i4BufSize) {
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameRemote(userId, (void*)vBuf, i4BufSize, i4Width, i4Height, VIDEO_FMT_YUV420P, timestamp);
        }
        else{
#ifdef __APPLE__
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameRemoteForIOS(userId, (void*)vBuf, i4Width, i4Height, VIDEO_FMT_CVPIXELBUFFER, timestamp);
#elif defined(ANDROID)
            int * tmpBuffer = (int*)vBuf;
            int fmt = tmpBuffer[0];
            int textureId = tmpBuffer[1];
            float* matrix = (float*)(tmpBuffer+2);
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameRemoteForAndroid(userId, textureId, matrix, i4Width, i4Height, fmt, timestamp);
#endif
        }
    }
}

YouMeErrorCode CYouMeVoiceEngine::setVideoRuntimeEventCb()
{
    TSK_DEBUG_INFO ("@@ setVideoRuntimeEventCb");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetVideoRuntimeEventCb);
        if (pMsg) {
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setVideoRuntimeEventCb");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setVideoRuntimeEventCb failed");
    return YOUME_ERROR_MEMORY_OUT;
}

void CYouMeVoiceEngine::videoRuntimeEventCb(int type, int32_t sessionId, bool needUploadPic, bool needReport)
{
    TSK_DEBUG_INFO ("@@ videocodec videoRuntimeEventCb type:%d, sessionId:%d, needUploadPic:%d, needReport:%d", type, sessionId, needUploadPic, needReport);
    // Pair userId from sessionId
    std::string userId = CVideoManager::getInstance()->getUserId(sessionId);
    youmeRTC::ReportVideoEvent videoEvent;
    YouMeEvent_t ymEvent = YOUME_EVENT_EOF;
    switch ((tdav_session_video_check_unusual_t)type) {
        case tdav_session_video_normal:
            videoEvent.event = REPORT_VIDEO_NORMAL;
            break;
        case tdav_session_video_shutdown:
            videoEvent.event = REPORT_VIDEO_SHUTDOWN;
            ymEvent = YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN;
            break;
        case tdav_session_video_data_error:
            videoEvent.event = REPORT_VIDEO_DATA_ERROR;
            ymEvent = YOUME_EVENT_OTHERS_DATA_ERROR;
            break;
        case tdav_session_video_network_bad:
            videoEvent.event = REPORT_VIDEO_NETWORK_BAD;
            ymEvent = YOUME_EVENT_OTHERS_NETWORK_BAD;
            break;
        case tdav_session_video_black_full:
            videoEvent.event = REPORT_VIDEO_BLACK_FULL;
            ymEvent = YOUME_EVENT_OTHERS_BLACK_FULL;
            break;
        case tdav_session_video_green_full:
            videoEvent.event = REPORT_VIDEO_GREEN_FULL;
            ymEvent = YOUME_EVENT_OTHERS_GREEN_FULL;
            break;
        case tdav_session_video_black_border:
            videoEvent.event = REPORT_VIDEO_BLACK_BORDER;
            ymEvent = YOUME_EVENT_OTHERS_BLACK_BORDER;
            break;
        case tdav_session_video_green_border:
            videoEvent.event = REPORT_VIDEO_GREEN_BORDER;
            ymEvent = YOUME_EVENT_OTHERS_GREEN_BORDER;
            break;
        case tdav_session_video_blurred_screen:
            videoEvent.event = REPORT_VIDEO_BLURRED_SCREEN;
            ymEvent = YOUME_EVENT_OTHERS_BLURRED_SCREEN;
            break;
        case tdav_session_video_encoder_error:
            videoEvent.event = REPORT_VIDEO_ENCODER_ERROR;
            ymEvent = YOUME_EVENT_OTHERS_ENCODER_ERROR;
            break;
        case tdav_session_video_decoder_error:
            videoEvent.event = REPORT_VIDEO_DECODER_ERROR;
            ymEvent = YOUME_EVENT_OTHERS_DECODER_ERROR;
            break;
        default:
            break;
    }

    if(needReport){
        ReportService * report = ReportService::getInstance();
        videoEvent.sessionid = sessionId;
        videoEvent.platform = NgnApplication::getInstance()->getPlatform();
        videoEvent.canal_id = NgnApplication::getInstance()->getCanalID();
        videoEvent.sdk_version = SDK_NUMBER;
        report->report(videoEvent);
    }
    
    if (needUploadPic){
        TSK_DEBUG_INFO("#### Upload log and video snap shot");
        MonitoringCenter::getInstance()->UploadLog(UploadType_Notify, 0, true);
    }
    CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(ymEvent, YOUME_SUCCESS, "", userId);
    //需要清除renderid 不然收不到video_on
    if(ymEvent == YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN){
        CVideoChannelManager::getInstance()->deleteRender(userId);
    }
    TSK_DEBUG_INFO("== videoRuntimeEventCb" );
}

YouMeErrorCode CYouMeVoiceEngine::setStaticsQosCb()
{
    TSK_DEBUG_INFO ("@@ setStaticsQosCb");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetStaticsQosCb);
        if (pMsg) {
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== setStaticsQosCb");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== setStaticsQosCb failed");
    return YOUME_ERROR_MEMORY_OUT;
}

void CYouMeVoiceEngine::onStaticsQosCb(int media_type, int32_t sessionId, int source_type, int audio_loss)
{
    TSK_DEBUG_INFO ("@@ videocodec videoSwitchSourceCb media:%d, sessionId:%d, source_type:%d, audio_loss:%d"
        , media_type, sessionId, source_type, audio_loss);

    // media_type 0:audio, 1:video
    if (1 == media_type) {
        std::string userid = CYouMeVoiceEngine::getInstance()->getUserNameBySessionId(sessionId);
        std::vector<IYouMeVoiceEngine::userVideoInfo> *videoInfoList = new(nothrow) std::vector<IYouMeVoiceEngine::userVideoInfo>();

        videoInfoList->clear();

        IYouMeVoiceEngine::userVideoInfo *videoinfo = new IYouMeVoiceEngine::userVideoInfo();
        videoinfo->userId = userid;
        videoinfo->resolutionType = source_type;

        videoInfoList->push_back(*videoinfo);
        CYouMeVoiceEngine::getInstance()->setUsersVideoInfo(*videoInfoList);

        delete videoinfo;
        delete videoInfoList;
    } else {
        CYouMeVoiceEngine::getInstance()->setOpusPacketLossPerc(audio_loss);
        CYouMeVoiceEngine::getInstance()->setOpusBitrate(audio_loss);
    }
}

void CYouMeVoiceEngine::onVideoEncodeAdjustCb(int width, int height, int bitrate, int fps)
{
    // report to caller level
    if (CYouMeVoiceEngine::getInstance()->m_bVideoEncodeParamReport) {
        TSK_DEBUG_INFO ("@@ onVideoEncodeAdjust width:%d, height:%d, bitrate:%d, fps:%d", width, height, bitrate, fps);

        youmecommon::Value jsonRoot;
        jsonRoot["width"] = youmecommon::Value( width );
        jsonRoot["height"] = youmecommon::Value( height );
        jsonRoot["bitrate"] = youmecommon::Value( bitrate );
        jsonRoot["fps"] = youmecommon::Value( fps );
        
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT, YOUME_SUCCESS, "", XStringToUTF8(jsonRoot.toSimpleString()));
    }
}

void CYouMeVoiceEngine::onVideoDecodeParamCb(int32_t sessionId, int width, int height)
{
    std::string userid = CYouMeVoiceEngine::getInstance()->getUserNameBySessionId(sessionId);
    TSK_DEBUG_INFO ("@@ onVideoDecodeParamCb user:%s, width:%d, height:%d", userid.c_str(), width, height);

    // report to caller level
    // if (CYouMeVoiceEngine::getInstance()->m_bVideoEncodeParamReport) {

        youmecommon::Value jsonRoot;
        jsonRoot["userid"] = youmecommon::Value( userid );
        jsonRoot["width"] = youmecommon::Value( width );
        jsonRoot["height"] = youmecommon::Value( height );
        
        // TSK_DEBUG_INFO("onVideoEncodeAdjustCb param[%s]", XStringToUTF8(jsonRoot.toSimpleString()).c_str());

        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(YOUME_EVENT_VIDEO_DECODE_PARAM_REPORT, YOUME_SUCCESS, "", XStringToUTF8(jsonRoot.toSimpleString()));
    // }
}


typedef enum rtp_engine_event_type_s
{
	RTP_ENGINE_EVENT_TYPE_UNKNOW = 0,
	RTP_ENGINE_EVENT_TYPE_P2P_SUCCESS = 0x1,  // p2p 通路检测成功
	RTP_ENGINE_EVENT_TYPE_P2P_FAIL = 0x2,     // p2p 通路检测失败，走server转发
	RTP_ENGINE_EVENT_TYPE_P2P_CHANGE = 0x3,   // p2p 切换到server转发
	RTP_ENGINE_EVENT_TYPE_VIDEO_ON = 0x10,
}rtp_engine_event_type_t;

// todo: 多预留一些参数，便于后面扩展，计划将上面两个回调都集成进来
void CYouMeVoiceEngine::onVideoEventCb(int type, void *data, int error) {

    int routeChangeFlag = Config_GetInt("route_change_flag", 0);

    TSK_DEBUG_INFO ("@@ onVideoEventCb type:%d, error:%d, route change:%d", type, error, routeChangeFlag);
	if (RTP_ENGINE_EVENT_TYPE_P2P_SUCCESS == type) {
        // p2p 通路检测成功，即当前route为p2p
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(YOUME_EVENT_RTP_ROUTE_P2P, YOUME_SUCCESS, "", CYouMeVoiceEngine::getInstance()->mStrUserID);
	}
	else if (RTP_ENGINE_EVENT_TYPE_P2P_FAIL == type) {
        // p2p 通路检测失败，若可以切换，则切换到server，否则退出房间
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(YOUME_EVENT_RTP_ROUTE_SEREVER, YOUME_SUCCESS, "", CYouMeVoiceEngine::getInstance()->mStrUserID);
	}
	else if (RTP_ENGINE_EVENT_TYPE_P2P_CHANGE == type) {
        // runtime过程中，p2p切换到server
        CYouMeVoiceEngine::getInstance()->sendCbMsgCallEvent(YOUME_EVENT_RTP_ROUTE_CHANGE_TO_SERVER, YOUME_SUCCESS, "", CYouMeVoiceEngine::getInstance()->mStrUserID);
    }
	else if (RTP_ENGINE_EVENT_TYPE_VIDEO_ON == type){
		struct video_on_info
		{
			int session_id;
			int width;
			int height;
			int video_id;
		};
		video_on_info *info = (video_on_info*)data;
		if (info){
			addNewSession(info->session_id, info->width, info->height, info->video_id);
		}
	}

	if ((type != RTP_ENGINE_EVENT_TYPE_P2P_SUCCESS && !routeChangeFlag)
		&& type != RTP_ENGINE_EVENT_TYPE_VIDEO_ON) {
        // leave room
        CYouMeVoiceEngine::getInstance()->leaveChannelAll();
    }
}

void CYouMeVoiceEngine::setOpusPacketLossPerc(int audio_loss)
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        return;
    }
    
    int packetLossPerc = tmedia_defaults_get_opus_packet_loss_perc();;
    if(audio_loss >= 12){
        packetLossPerc = 100;
    }else if(audio_loss >= 10){
        packetLossPerc = 80;
    }else if(audio_loss >= 8){
        packetLossPerc = 60;
    }else if(audio_loss >= 6){
        packetLossPerc = 30;
    }else if(audio_loss >= 4){
        packetLossPerc = 20;
    }else {
        packetLossPerc = 10;
    }
    
    if(m_nOpusPacketLossPerc == packetLossPerc) {
        return;
    }else {
        m_nOpusPacketLossPerc = packetLossPerc;
    }
    
    TSK_DEBUG_INFO("== setOpusPacketLossPerc:%d, audio loss:%d", packetLossPerc, audio_loss);
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgSetOpusPacketLossPerc);
        if (pMsg) {
            pMsg->m_param.i32Value = packetLossPerc;
            m_pMainMsgLoop->SendMessage(pMsg);
        }
    }
}

void CYouMeVoiceEngine::setOpusBitrate(int audio_loss)
{
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        return;
    }
    
    int bitrate = tmedia_defaults_get_opus_encoder_bitrate();
    if(audio_loss >= 60){
        bitrate = bitrate * 0.25f;
    }if(audio_loss >= 50){
        bitrate = bitrate * 0.4f;
    }else if(audio_loss >= 40){
        bitrate = bitrate * 0.5f;
    }else if(audio_loss >= 30){
        bitrate = bitrate * 0.6f;
    }else if(audio_loss >= 20){
        bitrate = bitrate * 0.7f;
    }else if(audio_loss >= 10){
        bitrate = bitrate * 0.8f;
    }else if(audio_loss >= 5){
        bitrate = bitrate * 0.9f;
    }else {
        bitrate = bitrate * 1.0f;
    }
    if (bitrate < 8000) {
        bitrate = 8000;
    }
    
    if(m_nOpusPacketBitrate == bitrate) {
        return;
    }else {
        m_nOpusPacketBitrate = bitrate;
    }
    TSK_DEBUG_INFO("== setOpusBitrate:%d, audio loss:%d", bitrate, audio_loss);
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgSetOpusBitrate);
        if (pMsg) {
            pMsg->m_param.i32Value = bitrate;
            m_pMainMsgLoop->SendMessage(pMsg);
        }
    }
}

YouMeErrorCode CYouMeVoiceEngine::maskVideoByUserId(const std::string& userId, bool mask)
{
    TSK_DEBUG_INFO ("@@ maskVideoByUserId");
    
    // 发送回调消息
    if (mask) {
        sendCbMsgCallEvent(YOUME_EVENT_MASK_VIDEO_FOR_USER, YOUME_SUCCESS, "", userId);
        AVStatistic::getInstance()->NotifyVideoStat( userId, false );
    } else {
        sendCbMsgCallEvent(YOUME_EVENT_RESUME_VIDEO_FOR_USER, YOUME_SUCCESS, "", userId);
        AVStatistic::getInstance()->NotifyVideoStat( userId, true );
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiMaskVideoByUserId);
        if (pMsg) {
            if (NULL == pMsg->m_param.apiMaskVideoByUserId.pStrUserId) {
                delete pMsg;
                pMsg = NULL;
                return YOUME_ERROR_MEMORY_OUT;
            }
            *(pMsg->m_param.apiMaskVideoByUserId.pStrUserId) = userId;
            pMsg->m_param.apiMaskVideoByUserId.mask = (mask) ? 1 : 2;
            m_pMainMsgLoop->SendMessage(pMsg);
            TSK_DEBUG_INFO("== maskVideoByUserId");
            return YOUME_SUCCESS;
        }
    }
    
    TSK_DEBUG_INFO("== maskVideoByUserId delayed");
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::OnMaskVideoByUserIdNfy(const std::string &selfUserId, const std::string &fromUserId, int mask)
{
    TSK_DEBUG_INFO ("$$ OnMaskVideoByUserIdNfy, selfUserId:%s fromUserId:%s, mask:%d", selfUserId.c_str(), fromUserId.c_str(), mask);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (mask == 1) {
        sendCbMsgCallEvent(YOUME_EVENT_MASK_VIDEO_BY_OTHER_USER, errCode, "", fromUserId);
    } else if (mask == 2) {
        sendCbMsgCallEvent(YOUME_EVENT_RESUME_VIDEO_BY_OTHER_USER, errCode, "", fromUserId);
    }
    TSK_DEBUG_INFO ("== OnMaskVideoByUserIdNfy");
}

//void CYouMeVoiceEngine::OnWhoseCamStatusChgNfy(const std::string &camChgUserId, int camStatus)
//{
//    TSK_DEBUG_INFO ("$$ OnWhoseCamStatusChgNfy, camChgUserId:%s camStatus:%d", camChgUserId.c_str(), camStatus);
//    YouMeErrorCode errCode = YOUME_SUCCESS;
//    if (camStatus == 0) {
//        sendCbMsgCallEvent(YOUME_EVENT_OTHERS_CAMERA_PAUSE, errCode, "", camChgUserId);
//    } else if (camStatus == 1) {
//        sendCbMsgCallEvent(YOUME_EVENT_OTHERS_CAMERA_RESUME, errCode, "", camChgUserId);
//    }
//    TSK_DEBUG_INFO ("== OnWhoseCamStatusChgNfy");
//}

int CYouMeVoiceEngine::calcSumResolution( const std::string& exceptUser ){
    std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
    int sumResolution = 0;
    for( auto iter = m_mapCurOtherUserID.begin(); iter != m_mapCurOtherUserID.end(); ++iter ){
        if( exceptUser == iter->first ){
            continue;
        }
        
        int nSessionId = getSessionIdByUserName( iter->first );
        if( nSessionId == 0 ){
            continue;
        }
        
        auto itResolution = m_mapOtherResolution.find( nSessionId );
        if( itResolution == m_mapOtherResolution.end() ){
            continue;
        }
        
        sumResolution += itResolution->second.width * itResolution->second.height;
    }
    
    return sumResolution;
           
}

void CYouMeVoiceEngine::OnOtherVideoInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus, bool bLeave,int videoId)
{
    TSK_DEBUG_INFO ("$$ OnOtherVideoInputStatusChgNfy, inputChgUserId:%s inputStatus:%d", inputChgUserId.c_str(), inputStatus);
    if( inputChgUserId == mStrUserID )    {
        return;
    }
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (inputStatus == 0) {
        bool bFind = false;
        for( auto iter = m_mapCurOtherUserID.begin(); iter != m_mapCurOtherUserID.end(); ++iter )
        {
            if( inputChgUserId == iter->first ){
                bFind = true;
            }
        }
        //列表里没有，多发了吧
        if( !bFind )
        {
            return;
        }
        
        if( !bLeave && 2 != videoId ){
            int inputChgSessionId = getSessionIdByUserName(inputChgUserId);
            TSK_DEBUG_INFO ("OnOtherVideoInputStatusChgNfy userID:%s, sessionId:%d", inputChgUserId.c_str(), inputChgSessionId);
//            if (inputChgSessionId > 0) {
//                m_avSessionMgr->releaseUserResource(inputChgSessionId, tmedia_video);
//            }
            
            sendCbMsgCallEvent(YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP, errCode, "", inputChgUserId);
        }
        CVideoChannelManager::getInstance()->deleteRender(inputChgUserId);
        AVStatistic::getInstance()->NotifyVideoStat(inputChgUserId , false );
        {
            // Search map
            UserIdStartRenderMap_t::iterator it = m_UserIdStartRenderMap.find(inputChgUserId);
            // uint64_t startTiming = 0;
            if (it == m_UserIdStartRenderMap.end()) {
                TSK_DEBUG_WARN("m_UserIdStartRenderMap not match userId(%s)", inputChgUserId.c_str());
                return;
            } else {
                // startTiming = it->second;
            }
            
            uint64_t curTimeMs = tsk_gettimeofday_ms();
            unsigned int duration = curTimeMs - m_LastCheckTimeMs;
            m_LastCheckTimeMs = curTimeMs;
            if (m_NumOfCamera <= 0) {
                TSK_DEBUG_WARN("Camera stream number is impossible(%d)", m_NumOfCamera);
            }
            
            /* 停止渲染 */
            unsigned width, height;
            tmedia_defaults_get_video_size(&width, &height);
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportRecvVideoInfo videoOff;
            videoOff.roomid = mRoomID;
            videoOff.other_userid = inputChgUserId;
            videoOff.width  = width;
            videoOff.height = height;
            //videoOff.video_render_duration = (unsigned int)(tsk_gettimeofday_ms() - startTiming);
            videoOff.video_render_duration = duration;
            videoOff.report_type = REPORT_VIDEO_STOP_RENDER;
            videoOff.sdk_version = SDK_NUMBER;
            videoOff.platform    = NgnApplication::getInstance()->getPlatform();
            videoOff.num_camera_stream = m_NumOfCamera;
            videoOff.sum_resolution = calcSumResolution( "" );
            videoOff.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoOff);
            
            //TSK_DEBUG_INFO("PinkyLog: Stop:sum_resolution:%d", videoOff.sum_resolution );
            
            m_NumOfCamera--;
        }
        
        if(2 != videoId){
            std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
            
            auto userIter = m_mapCurOtherUserID.find(inputChgUserId);
            if (userIter != m_mapCurOtherUserID.end()) {
                m_mapCurOtherUserID.erase( userIter );
            }
            
            //TSK_DEBUG_INFO("PinkyLog: remove User :%s, curCount:%d", inputChgUserId.c_str(), m_mapCurOtherUserID.size()  );
        }
    } else if (inputStatus == 1) {
        
        //检测是否重复通知，如果一个start的userid已经存在并且是在1s内接收的，那就不处理了
        uint64_t videoStatusChgTs = tsk_gettimeofday_ms();
        for( auto iter = m_mapCurOtherUserID.begin(); iter != m_mapCurOtherUserID.end(); ++iter )
        {
            if( inputChgUserId == iter->first && videoStatusChgTs <= (iter->second + 1000) )
            {
				TSK_DEBUG_INFO("repeat notify start input :%s", inputChgUserId.c_str());
                return;
            }
        }
        AVStatistic::getInstance()->NotifyVideoStat(inputChgUserId , true );
        
        if(2 != videoId) sendCbMsgCallEvent(YOUME_EVENT_OTHERS_VIDEO_INPUT_START, errCode, "", inputChgUserId);
        {
            uint64_t curTimeMs = tsk_gettimeofday_ms();
            unsigned int duration = curTimeMs - m_LastCheckTimeMs;
            m_LastCheckTimeMs = curTimeMs;
            if (!m_NumOfCamera) {
                duration = 0;
            }
            
            /* 开始渲染 */
            unsigned width, height;
            tmedia_defaults_get_video_size(&width, &height);
            ReportService * report = ReportService::getInstance();
            youmeRTC::ReportRecvVideoInfo videoOn;
            videoOn.roomid = mRoomID;
            videoOn.other_userid = inputChgUserId;
            videoOn.width  = width;
            videoOn.height = height;
            videoOn.video_render_duration = duration; //tsk_gettimeofday_ms();
            videoOn.report_type = REPORT_VIDEO_START_RENDER;
            videoOn.sdk_version = SDK_NUMBER;
            videoOn.platform    = NgnApplication::getInstance()->getPlatform();
            videoOn.num_camera_stream = m_NumOfCamera;
            videoOn.sum_resolution = calcSumResolution(inputChgUserId);
            videoOn.canal_id = NgnApplication::getInstance()->getCanalID();
            report->report(videoOn);
            
            //TSK_DEBUG_INFO("PinkyLog: Start:sum_resolution:%d", videoOn.sum_resolution );
            
            m_NumOfCamera++;
            
            // Inser map
            UserIdStartRenderMap_t::iterator it = m_UserIdStartRenderMap.find(inputChgUserId);
            if (it == m_UserIdStartRenderMap.end()) {
                m_UserIdStartRenderMap.insert(UserIdStartRenderMap_t::value_type(inputChgUserId, tsk_gettimeofday_ms()));
            } else {
                it->second = tsk_gettimeofday_ms();
            }
        }
        
        if(2 != videoId){
            std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
            m_mapCurOtherUserID[inputChgUserId] = videoStatusChgTs;
            TSK_DEBUG_INFO("mark: add User :%s, curCount:%d", inputChgUserId.c_str(), m_mapCurOtherUserID.size()  );
        }
    }
    TSK_DEBUG_INFO ("== OnOtherVideoInputStatusChgNfy");
}

void CYouMeVoiceEngine::OnOtherAudioInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus)
{
    TSK_DEBUG_INFO ("$$ OnOtherAudioInputStatusChgNfy, inputChgUserId:%s inputStatus:%d", inputChgUserId.c_str(), inputStatus);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (inputStatus == 0) {
        // TODO 七牛无需求，暂时不回调至上层
    } else if (inputStatus == 1) {
        // TODO 七牛无需求，暂时不回调至上层
    }
    TSK_DEBUG_INFO ("== OnOtherAudioInputStatusChgNfy");
}

void CYouMeVoiceEngine::OnOtheShareInputStatusChgNfy(const std::string &inputChgUserId, int inputStatus)
{
    TSK_DEBUG_INFO ("$$ OnOtheShareInputStatusChgNfy, inputChgUserId:%s inputStatus:%d", inputChgUserId.c_str(), inputStatus);
    std::string strUserID = getShareUserName( inputChgUserId );

    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (inputStatus == 0) {
        
        sendCbMsgCallEvent(YOUME_EVENT_OTHERS_SHARE_INPUT_STOP, errCode, "", strUserID);

        CVideoChannelManager::getInstance()->deleteRender(strUserID);
        AVStatistic::getInstance()->NotifyVideoStat(strUserID , false );
        {
            std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
            auto userIter = m_mapCurOtherUserID.find(strUserID);
            if (userIter != m_mapCurOtherUserID.end()) {
                m_mapCurOtherUserID.erase( userIter );
            }
            
            //TSK_DEBUG_INFO("PinkyLog: remove User :%s, curCount:%d", strUserID.c_str(), m_mapCurOtherUserID.size()  );
        }
    } else if (inputStatus == 1) {

        //检测是否重复通知，如果一个start的userid已经存在并且是在1s内接收的，那就不处理了
        uint64_t videoStatusChgTs = tsk_gettimeofday_ms();
        for( auto iter = m_mapCurOtherUserID.begin(); iter != m_mapCurOtherUserID.end(); ++iter )
        {
            if( strUserID == iter->first && videoStatusChgTs <= (iter->second + 1000))
            {
                return;
            }
        }
        AVStatistic::getInstance()->NotifyVideoStat(strUserID , true );
        
        sendCbMsgCallEvent(YOUME_EVENT_OTHERS_SHARE_INPUT_START, errCode, "", strUserID);
        
        {
            std::lock_guard< std::mutex> lock(m_mutexOtherResolution);
            m_mapCurOtherUserID[inputChgUserId] = videoStatusChgTs;
            //TSK_DEBUG_INFO("PinkyLog: add User :%s, curCount:%d", inputChgUserId.c_str(), m_mapCurOtherUserID.size()  );
        }
    }
    TSK_DEBUG_INFO ("== OnOtheShareInputStatusChgNfy");
}

void CYouMeVoiceEngine::setAudioQuality(YOUME_AUDIO_QUALITY quality)
{
    TSK_DEBUG_INFO ("@@ setAudioQuality, quality=%d", quality);
    if (quality == LOW_QUALITY) {
#if ( defined(ANDROID)  || defined ( MAC_OS ) )
        tmedia_defaults_set_playback_sample_rate(16000, 1 );
#else
        if(MediaSessionMgr::defaultsGetExternalInputMode()){
            tmedia_defaults_set_playback_sample_rate(48000, 1 );
        }else{
            tmedia_defaults_set_playback_sample_rate(16000, 1 );
        }
#endif
        tmedia_defaults_set_record_sample_rate(16000, 1 );
        tmedia_defaults_set_opus_encoder_bitrate(25000);
    } else if (quality == HIGH_QUALITY) {
        tmedia_defaults_set_playback_sample_rate(48000, 1 );
        tmedia_defaults_set_record_sample_rate(48000, 1 );
    }
    TSK_DEBUG_INFO ("== setAudioQuality");
}

void CYouMeVoiceEngine::setScreenSharedEGLContext(void* gles)
{
    TSK_DEBUG_INFO ("@@ setScreenSharedEGLContext");

    tmedia_set_video_egl_share_context(gles);
    
    TSK_DEBUG_INFO ("== setScreenSharedEGLContext");
}

void CYouMeVoiceEngine::setVideoCodeBitrate(unsigned int maxBitrate,  unsigned int minBitrate )
{
    TSK_DEBUG_INFO ("@@ setVideoCodeBitrate, max=%d, min=%d", maxBitrate, minBitrate );
    tmedia_defaults_set_video_codec_bitrate( maxBitrate );
    
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("max_bitrate",maxBitrate);
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("min_bitrate",minBitrate);
    TSK_DEBUG_INFO ("== setVideoCodeBitrate");
}

/**
 该方法作用已经改变，非-1 的参数表示使用VBR，默认SDK使用CBR恒定码率
 */
YouMeErrorCode CYouMeVoiceEngine::setVBR( bool useVBR )
{
    TSK_DEBUG_INFO ("@@ setVBR: %d", useVBR);
   
    //bool bAllowed  = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VIDEO_ALLOW_FIX_QUALITY,
    //                                                                                NgnConfigurationEntry::DEF_VIDEO_ALLOW_FIX_QUALITY);
    //if( bAllowed )
    //{
    //    TSK_DEBUG_INFO ("== setVideoQuality allowed");
    if(useVBR){
        tmedia_defaults_set_video_codec_quality_max( 1 );
        tmedia_defaults_set_video_codec_quality_min( 1 );
    }else{
        tmedia_defaults_set_video_codec_quality_max( -1 );
        tmedia_defaults_set_video_codec_quality_min( -1 );
    }
        
    return YOUME_SUCCESS;
    //}
    //else{
    //    TSK_DEBUG_INFO ("== setVideoQuality");
    //    return YOUME_ERROR_API_NOT_ALLOWED;
    //}
}

YouMeErrorCode CYouMeVoiceEngine::setVBRForSecond( bool useVBR )
{
    TSK_DEBUG_INFO ("@@ setVBRForSecond: %d", useVBR );
    if(useVBR){
        tmedia_defaults_set_video_codec_quality_max_child( 1 );
        tmedia_defaults_set_video_codec_quality_min_child( 1 );
    }else{
        tmedia_defaults_set_video_codec_quality_max_child( -1 );
        tmedia_defaults_set_video_codec_quality_min_child( -1 );
    }
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::setVBRForShare( bool useVBR )
{
    TSK_DEBUG_INFO ("@@ setVBRForShare: %d", useVBR );
    if(useVBR){
        tmedia_defaults_set_video_codec_quality_max_share( 1 );
        tmedia_defaults_set_video_codec_quality_min_share( 1 );
    }else{
        tmedia_defaults_set_video_codec_quality_max_share( -1 );
        tmedia_defaults_set_video_codec_quality_min_share( -1 );
    }
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::setVideoCodeBitrateForSecond(unsigned int maxBitrate,  unsigned int minBitrate )
{
    TSK_DEBUG_INFO ("@@ setVideoCodeBitrateForSecond, max=%d, min=%d", maxBitrate, minBitrate );
    tmedia_defaults_set_video_codec_bitrate_child( maxBitrate );
    
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("max_bitrate_second",maxBitrate);
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("min_bitrate_second",minBitrate);
    TSK_DEBUG_INFO ("== setVideoCodeBitrateForSecond");
}

void CYouMeVoiceEngine::setVideoCodeBitrateForShare(unsigned int maxBitrate,  unsigned int minBitrate )
{
    TSK_DEBUG_INFO ("@@ setVideoCodeBitrateForShare, max=%d, min=%d", maxBitrate, minBitrate );
    tmedia_defaults_set_video_codec_bitrate_share( maxBitrate );
    
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("max_bitrate_share",maxBitrate);
    CNgnMemoryConfiguration::getInstance()->SetConfiguration("min_bitrate_share",minBitrate);
    TSK_DEBUG_INFO ("== setVideoCodeBitrateForShare");
}

unsigned int CYouMeVoiceEngine::getCurrentVideoCodeBitrate( )
{
    return  tmedia_get_video_current_bitrate();
}

void CYouMeVoiceEngine::setVideoHardwareCodeEnable( bool bEnable )
{
    TSK_DEBUG_INFO ("@@ setVideoHardwareCodeEnable, bEnable=%d", bEnable );
    tmedia_defaults_set_video_hwcodec_enabled( bEnable );
    TSK_DEBUG_INFO ("== setVideoHardwareCodeEnable");
}

bool CYouMeVoiceEngine::getVideoHardwareCodeEnable( )
{
    return tmedia_defaults_get_video_hwcodec_enabled() && Config_GetBool("HW_ENCODE", 0) ;
}

bool CYouMeVoiceEngine::getUseGL( )
{
    return tmedia_defaults_get_video_hwcodec_enabled() && Config_GetBool("HW_ENCODE", 0) && (!tmedia_defaults_get_video_frame_raw_cb_enabled()) ;
}

void CYouMeVoiceEngine::setVideoNoFrameTimeout(int timeout)
{
    TSK_DEBUG_INFO ("@@ setVideoNoFrameTimeout, timeout=%d", timeout);
    if (timeout > 0) {
        tmedia_defaults_set_video_noframe_monitor_timeout((uint32_t)timeout);
    } else {
        TSK_DEBUG_WARN("Video no frame timeout should be a positive value.(current:%d)", timeout);
    }
    TSK_DEBUG_INFO ("== setVideoNoFrameTimeout");
}

YouMeErrorCode CYouMeVoiceEngine::startPush( std::string pushUrl )
{
    TSK_DEBUG_INFO("@@ startPush ");
    
    if( pushUrl.empty() ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== startPush wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiStartPush);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        *(pMsg->m_param.apiStartPush.url) = pushUrl;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== startPush");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== startPush failed to send message");
    return errCode;
    
}
YouMeErrorCode CYouMeVoiceEngine::stopPush()
{
    TSK_DEBUG_INFO("@@ stopPush ");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== stopPush wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiStopPush);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== stopPush");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== stopPush failed to send message");
    return errCode;
    
}

YouMeErrorCode CYouMeVoiceEngine::setPushMix( std::string pushUrl , int width, int height )
{
    TSK_DEBUG_INFO("@@ setPushMix ");
    
    if( pushUrl.empty() ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== setPushMix wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiSetPushMix);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        *(pMsg->m_param.apiSetMixPush.url) = pushUrl;
        pMsg->m_param.apiSetMixPush.width = width;
        pMsg->m_param.apiSetMixPush.height = height;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== setPushMix");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== setPushMix failed to send message");
    return errCode;
}
YouMeErrorCode CYouMeVoiceEngine::clearPushMix()
{
    TSK_DEBUG_INFO("@@ clearPushMix ");
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== clearPushMix wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiClearPushMix);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== clearPushMix");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== clearPushMix failed to send message");
    return errCode;
    
    
}
YouMeErrorCode CYouMeVoiceEngine::addPushMixUser( std::string userId, int x, int y, int z, int width, int height )
{
    TSK_DEBUG_INFO("@@ addPushMixUser ");
    
    if( userId.empty() ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== addPushMixUser wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiAddPushMixUser);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
        *(pMsg->m_param.apiAddMixPushUser.userID ) = userId;
        pMsg->m_param.apiAddMixPushUser.x = x;
        pMsg->m_param.apiAddMixPushUser.y = y;
        pMsg->m_param.apiAddMixPushUser.z = z;
        pMsg->m_param.apiAddMixPushUser.width = width;
        pMsg->m_param.apiAddMixPushUser.height = height;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== addPushMixUser");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== addPushMixUser failed to send message");
    return errCode;
    
}
YouMeErrorCode CYouMeVoiceEngine::removePushMixUser( std::string userId )
{
    TSK_DEBUG_INFO("@@ removePushMixUser ");
    
    if( userId.empty() ){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    YouMeErrorCode errCode = YOUME_SUCCESS;
    if (!isStateInitialized() || !m_isInRoom) {
        TSK_DEBUG_ERROR("== removePushMixUser wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow)CMessageBlock(CMessageBlock::MsgApiRemovePushMixUser);
        if (NULL == pMsg) {
            errCode = YOUME_ERROR_MEMORY_OUT;
            goto fail;
        }
        
       *(pMsg->m_param.apiRemoveMixPushUser.userID ) = userId;
        
        m_pMainMsgLoop->SendMessage(pMsg);
        TSK_DEBUG_INFO("== removePushMixUser");
        return YOUME_SUCCESS;
    }
    
    errCode = YOUME_ERROR_UNKNOWN;
fail:
    TSK_DEBUG_INFO("== addPushMixUser failed to send message");
    return errCode;
    
}

void CYouMeVoiceEngine::videoShareCb (void *handle, char *data, uint32_t len, uint64_t timestamp, uint32_t w, uint32_t h)
{
    int width = w;
    //CYouMeVoiceEngine::getInstance ()->shareWidth;
    int height = h;
    //CYouMeVoiceEngine::getInstance ()->shareHeight;

	CYouMeVoiceEngine::getInstance ()->setVideoNetResolutionForShare (w, h);

    // convert timestamp unit from ns to ms
	uint64_t timestampMs = tsk_gettimeofday_ms();
	
    CYouMeVoiceEngine::getInstance ()->inputVideoFrameForShare (data, len, width, height, VIDEO_FMT_YUV420P, 0, 0, timestampMs);

}

void CYouMeVoiceEngine::videoShareCbForMac(char *data, uint32_t len, int width, int height, uint64_t timestamp)
{
    uint64_t timestampMs = tsk_gettimeofday_ms();

    // 窗口录制时采用窗口实际分辨率
    if (VIDEO_SHARE_TYPE_WINDOW == CYouMeVoiceEngine::getInstance()->shareMode) {
        do {
            int tempShareWidth = width;
            int tempShareHeight = height;

            // 检查当前窗口采集分辨率是否发生改变
            if (CYouMeVoiceEngine::getInstance()->lastShareCaptrureWidth
                && CYouMeVoiceEngine::getInstance()->lastShareCaptureHeight
                && (CYouMeVoiceEngine::getInstance()->lastShareCaptrureWidth == width) 
                && (CYouMeVoiceEngine::getInstance()->lastShareCaptureHeight == height)) {
                break;
            }

            // 调整窗口录制分辨率，目前限制窗口最大分辨率为1080P
            if (width * height > 1920 * 1080) {
                //scale
                float wScale = 1920.0f / width;
                float hScale = 1080.0f / height;
                
                float scale = (wScale > hScale ? hScale : wScale);
                tempShareWidth = width * scale;
                tempShareHeight = height * scale;
            }

            // 窗口录制编码分辨率4字节对齐
            tempShareWidth += (16 - tempShareWidth%16);
            tempShareHeight += (16 - tempShareHeight%16);

            // 重新设置编码分辨率
            MediaSessionMgr::setVideoNetResolutionForShare(tempShareWidth, tempShareHeight);
        } while(0);

        CYouMeVoiceEngine::getInstance()->lastShareCaptrureWidth = width;
        CYouMeVoiceEngine::getInstance()->lastShareCaptureHeight = height;
    }
   
    CYouMeVoiceEngine::getInstance()->inputVideoFrameForMacShare(data, width, height, 0, 0, 0, timestampMs);
}

void CYouMeVoiceEngine::stopCaptureAndEncode()
{
#ifdef WIN32
	StopStream();
	DestroyMedia();
	stopInputVideoFrameForShare();
#elif TARGET_OS_OSX
	if (!capture) {
		TSK_DEBUG_WARN("share stream state invalid");
		return;
	}
	StopStream(capture);
	DestroyMedia(capture);
	stopInputVideoFrameForShare();
	capture = nullptr;
#endif
    shareMode = VIDEO_SHARE_TYPE_UNKNOW;
	bInitCaptureAndEncode = false;
}

YouMeErrorCode CYouMeVoiceEngine::stopShareStream()
{
    TSK_DEBUG_INFO("@@ stopShareStream");
	if (m_bStartshare != true) {
		TSK_DEBUG_WARN("@@ Share stream is not started!");
		return YOUME_SUCCESS;
	}

    lock_guard<recursive_mutex> stateLock(mStateMutex);  

	if (!m_bSaveScreen) {
		stopCaptureAndEncode();
	}
    
    m_bStartshare = false;
    int currentBusinessType = tmedia_get_video_share_type();
    tmedia_set_video_share_type(currentBusinessType & (~ENABLE_SHARE));
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::stopSaveScreen()
{
    TSK_DEBUG_INFO("@@ stopSaveScreen");
    if (m_bSaveScreen != true) {
        TSK_DEBUG_WARN("@@ save screen is not started!");
        return YOUME_SUCCESS;
    }
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);

#if  ( TARGET_OS_OSX || WIN32 ) && FFMPEG_SUPPORT
    YMVideoRecorderManager::getInstance()->stopRecord( mStrUserID );
    if(!m_bStartshare) {
        stopCaptureAndEncode();
    }
    
    int currentBusinessType = tmedia_get_video_share_type();
    tmedia_set_video_share_type(currentBusinessType & (~ENABLE_SAVE));
    m_bSaveScreen = false;
#else
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif

    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSaveScreenEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = false;
            m_pMainMsgLoop->SendMessage(pMsg);
        }
    }
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::checkCondition(const std::string&  filePath)
{
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== startSaveScreen wrong state:%s", stateToString(mState));
        return YOUME_ERROR_WRONG_STATE;
    }
    
    if( !isInRoom() )
    {
        TSK_DEBUG_ERROR("== startSaveScreen not in room ");
        return YOUME_ERROR_NOT_IN_CHANNEL;
    }
    
    //检查文件名后缀是否.mp4
    auto pos = filePath.find( ".mp4" );
    if( pos == string::npos || pos != filePath.length() - 4 )
    {
        TSK_DEBUG_ERROR("== startSaveScreen Only support fmt mp4");
        return YOUME_ERROR_INVALID_PARAM;
    }

    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::prepareSaveScreen(const std::string&  filePath)
{
#if (TARGET_OS_OSX || WIN32) && FFMPEG_SUPPORT
    YMVideoRecorderManager::getInstance()->startRecord( mStrUserID , filePath );
    int currentBusinessType = tmedia_get_video_share_type();
    tmedia_set_video_share_type(currentBusinessType | ENABLE_SAVE);
#else
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
    
    if (m_pMainMsgLoop) {
        CMessageBlock *pMsg = new(nothrow) CMessageBlock(CMessageBlock::MsgApiSetSaveScreenEnabled);
        if (pMsg) {
            pMsg->m_param.bTrue = true;
            m_pMainMsgLoop->SendMessage(pMsg);
        }
    }

    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::startSaveScreen( const std::string&  filePath)
{
    TSK_DEBUG_INFO("@@ startSaveScreen");
    YouMeErrorCode ret = YOUME_SUCCESS;

    if (m_bSaveScreen != false) {
        TSK_DEBUG_WARN("@@ save screen is started!");
        return YOUME_SUCCESS;
    }
    
#if TARGET_OS_OSX && FFMPEG_SUPPORT
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    ret = checkCondition(filePath);
    if (ret != YOUME_SUCCESS) {
        return ret;
    }

    ret = startCaptureAndEncode(VIDEO_SHARE_TYPE_SCREEN, DEFAULT_WINDOW_ID);
    if (ret != YOUME_SUCCESS) {
        TSK_DEBUG_ERROR("startCaptureAndEncode failed!");
        return ret;
    }
    
    ret = prepareSaveScreen(filePath);
    if (ret != YOUME_SUCCESS) {
        return ret;
    }

    m_bSaveScreen = true;
    TSK_DEBUG_INFO("== startSaveScreen");
    return YOUME_SUCCESS;
#else
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

#ifdef WIN32
YouMeErrorCode CYouMeVoiceEngine::startCaptureAndEncode(int mode, HWND renderHandle, HWND captureHandle)
{
	int fps = 0, width = 0, height = 0;
	YouMeErrorCode ret = YOUME_SUCCESS;

	if (bInitCaptureAndEncode != false) {
		TSK_DEBUG_WARN("startCaptureAndEncode has been inited!");
		return YOUME_SUCCESS;
	}

	do {
		if ((mode != VIDEO_SHARE_TYPE_WINDOW && mode != VIDEO_SHARE_TYPE_SCREEN)) {
			TSK_DEBUG_ERROR("startShareStream invalid parameter");
			ret = YOUME_ERROR_INVALID_PARAM;
			break;
		}

        //HINSTANCE hInstance = AfxGetInstanceHandle(); 
        HMODULE hm = NULL;
        void *callerAddress = _ReturnAddress();
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)callerAddress, &hm);
        HINSTANCE hInstance = (HINSTANCE)hm;
        if (InitMedia(hInstance) < 0) {
            TSK_DEBUG_ERROR("init video share module failed");
            ret = YOUME_ERROR_INIT_SHARE_FAIL;
            break;
        }

		// 这里设置的是采集帧率，后面再encode前会有抽帧处理
		// 采集帧率默认最低是30帧
        fps = (shareFps > 30 ? shareFps : 30);
		if (!shareWidth || !shareHeight) {
			width = 1280;
			height = 720;
		} else {
			width = shareWidth;
			height = shareHeight;
		}

        SetRenderWnd(renderHandle);
        int minFps = Config_GetInt("MIN_VIDEO_SHARE_CAPTURE_FPS", 5);
		SetCaptureFPS(minFps, fps);
		SetPictureCallback((MEDIA_PICTURE_CALLBACK)videoShareCb, NULL);

		SetCaptureMode(mode);
		SetCaptureWindow(captureHandle, 0, 0, 0, 0);
		SetOutputSize(width, height);

		int stat = StartStream(false);
		if (stat < 0) {
			TSK_DEBUG_ERROR("startShareStream fail, ret:%x", stat);
			ret = YOUME_ERROR_START_SHARE_FAIL;
			break;
		}
	} while (0);

	if (YOUME_SUCCESS == ret)
	{
		bInitCaptureAndEncode = true;
        shareMode = (YOUME_VIDEO_SHARE_TYPE_t)mode;

        lastShareCaptrureWidth = 0;
        lastShareCaptureHeight = 0;
	}
	
	TSK_DEBUG_INFO("== startCaptureAndEncode, ret:%d", ret);
	return ret;
}

YouMeErrorCode CYouMeVoiceEngine::setShareExclusiveWnd(HWND windowid)
{
	YouMeErrorCode ret = YOUME_SUCCESS;
	
	if (AddExcludeWindow(windowid) != 0)
	{
		ret = YOUME_ERROR_INVALID_PARAM;
	}

	TSK_DEBUG_INFO("== setShareExclusiveWnd, windowid:%d ret:%d", windowid, ret);
	return ret;
}

YouMeErrorCode CYouMeVoiceEngine::startShareStream(int mode, HWND renderHandle, HWND captureHandle)
{
	YouMeErrorCode ret = YOUME_SUCCESS;

    TSK_DEBUG_INFO ("@@ startShareStream mode:%d  captureHandle=0x%x", mode, captureHandle);
    if (m_bStartshare != false) {
        TSK_DEBUG_WARN("startShareStream share state invalid");
        return YOUME_SUCCESS;
    }

	do {
		lock_guard<recursive_mutex> stateLock(mStateMutex);
		if (!isStateInitialized()) {
			TSK_DEBUG_ERROR("== startShareStream wrong state:%s", stateToString(mState));
			ret = YOUME_ERROR_WRONG_STATE;
			break;
		}

		if (!isInRoom())
		{
			TSK_DEBUG_ERROR("== startShareStream not in room ");
			ret = YOUME_ERROR_NOT_IN_CHANNEL;
			break;
		}

		ret = startCaptureAndEncode(mode, renderHandle, captureHandle);
		if (ret != YOUME_SUCCESS) {
			TSK_DEBUG_ERROR("== startCaptureAndEncode failed!");
			break;
		}

		int currentBusinessTypeFlag = tmedia_get_video_share_type();
		tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);
	} while (0);

	if (YOUME_SUCCESS == ret)
	{
		m_bStartshare = true;
	}

    TSK_DEBUG_INFO("== startShareStream, ret:%d", ret);
    return ret;
}

YouMeErrorCode CYouMeVoiceEngine::getWindowInfoList(HWND *pHwnd, char *pWinName, int *winCount)
{
    TSK_DEBUG_INFO("@@ getWindowInfoList");
    YMWINDOWINFO ymWindowInfo[100];
    memset(ymWindowInfo, 0, sizeof(YMWINDOWINFO)*50);
    int count = 0;
    
    GetWindowList(ymWindowInfo, &count);
    TSK_DEBUG_INFO("getWindowInfoList count[%d]", count);

    for (int i = 0; i < count; i++) {
        TSK_DEBUG_INFO("window index[%d] handle[%x] name[%s]", i, ymWindowInfo[i].hWnd, XStringToUTF8(XString(ymWindowInfo[i].szWindowName)).c_str());
        *(pHwnd+i) = ymWindowInfo[i].hWnd;
        memcpy(&pWinName[i*256], XStringToUTF8(XString(ymWindowInfo[i].szWindowName)).c_str(), MAX_LOADSTRING);
    }
    *winCount = count;
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::getScreenInfoList (HWND *pHwnd, char *pScreenName, int *ScreenCount)
{
    TSK_DEBUG_INFO ("@@ getScreenInfoList");
    YMWINDOWINFO ymWindowInfo[100];
    memset (ymWindowInfo, 0, sizeof (YMWINDOWINFO) * 50);
    int count = 0;

    GetScreenList (ymWindowInfo, &count);
    TSK_DEBUG_INFO ("getScreenInfoList count[%d]", count);

    for (int i = 0; i < count; i++)
    {
        TSK_DEBUG_INFO ("screen index[%d] handle[%x] name[%s]", i, ymWindowInfo[i].hWnd, XStringToUTF8 (XString (ymWindowInfo[i].szWindowName)).c_str ());
        *(pHwnd + i) = ymWindowInfo[i].hWnd;
        memcpy (&pScreenName[i * 256], XStringToUTF8 (XString (ymWindowInfo[i].szWindowName)).c_str (), MAX_LOADSTRING);
    }
    *ScreenCount = count;

    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::startSaveScreen( const std::string&  filePath, HWND renderHandle, HWND captureHandle)
{
    TSK_DEBUG_INFO("@@ startSaveScreen");
    YouMeErrorCode ret = YOUME_SUCCESS;

	if (m_bSaveScreen != false) {
		TSK_DEBUG_WARN("@@ save screen is started!");
		return YOUME_SUCCESS;
	}
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    
    ret = checkCondition(filePath);
    if (ret != YOUME_SUCCESS) {
        return ret;
    }
    
    ret = startCaptureAndEncode(VIDEO_SHARE_TYPE_SCREEN, renderHandle, captureHandle);
    if (ret != YOUME_SUCCESS) {
        TSK_DEBUG_ERROR("startCaptureAndEncode failed!");
        return ret;
    }

    ret = prepareSaveScreen(filePath);
    if (ret != YOUME_SUCCESS) {
        return ret;
    }

    m_bSaveScreen = true;
    TSK_DEBUG_INFO("== startSaveScreen");
    return YOUME_SUCCESS;
}

#elif TARGET_OS_OSX
YouMeErrorCode CYouMeVoiceEngine::checkSharePermission()
{
    bool ret = ICameraManager::getInstance()->checkScreenCapturePermission();
    if (!ret) {
        TSK_DEBUG_INFO("@@ checkSharePermission no permission");
        return YOUME_ERROR_SHARE_NO_PERMISSION;
    }
    return YOUME_SUCCESS;
}

void CYouMeVoiceEngine::setShareContext(void* context)
{
    SetShareContext( context );
}

YouMeErrorCode CYouMeVoiceEngine::setShareExclusiveWnd( int windowid )
{
    YouMeErrorCode ret = YOUME_SUCCESS;
    TSK_DEBUG_INFO("@@ setShareExclusiveWnd  windowid:%d",  windowid);
    
    lock_guard<recursive_mutex> stateLock(mStateMutex);
    if (!isStateInitialized()) {
        TSK_DEBUG_ERROR("== startSaveScreen wrong state:%s", stateToString(mState));
        ret = YOUME_ERROR_WRONG_STATE;
        return ret;
    }
    
    if( windowid == 0 )
    {
        m_vecShareExclusiveWnd.clear();
        if( capture )
        {
            SetExclusiveWnd(  capture, 0 );
        }
    }
    else{
        m_vecShareExclusiveWnd.push_back( windowid );
        if( capture )
        {
            SetExclusiveWnd(  capture, windowid );
        }
    }

    return ret;
}

YouMeErrorCode CYouMeVoiceEngine::startShareStream(int mode, int windowid)
{
	YouMeErrorCode ret = YOUME_SUCCESS;
    TSK_DEBUG_INFO("@@ startShareStream mode:%d, windowid:%d", mode, windowid);
	if (m_bStartshare != false) {
		TSK_DEBUG_WARN("startShareStream has been started!");
		return YOUME_SUCCESS;
	}
    
	do {
		lock_guard<recursive_mutex> stateLock(mStateMutex);
		if (!isStateInitialized()) {
			TSK_DEBUG_ERROR("== startSaveScreen wrong state:%s", stateToString(mState));
			ret = YOUME_ERROR_WRONG_STATE;
			break;
		}

		if( !isInRoom() )
		{
			TSK_DEBUG_ERROR("== startSaveScreen not in room ");
			ret = YOUME_ERROR_NOT_IN_CHANNEL;
			break;
		}

		ret = startCaptureAndEncode(mode, windowid);
		if (ret != YOUME_SUCCESS) {
			TSK_DEBUG_ERROR("startCaptureAndEncode failed!");
			break;
		}

		int currentBusinessTypeFlag = tmedia_get_video_share_type();
		tmedia_set_video_share_type(currentBusinessTypeFlag | ENABLE_SHARE);
	}while(0);

	if (YOUME_SUCCESS == ret) {
		m_bStartshare = true;
	}
    
	TSK_DEBUG_INFO("== startShareStream, ret:%d", ret);
    return ret;
    
    // for window share test
    #if 0
    char arrayOwner[100][256] = {0};
    char arrayWindow[100][256] = {0};
    int  arrayId[100] = {0};
    
    int count = 0;
    IYouMeVoiceEngine::getInstance()->GetWindowInfoList(*arrayOwner, *arrayWindow, arrayId, &count);
    
    youmecommon::Value jsonRoot;
    jsonRoot["count"] = youmecommon::Value(count);
    
    for( int i = 0; i < count; ++i ){
        youmecommon::Value jsonWindowInfo;
        jsonWindowInfo["id"] = youmecommon::Value(arrayId[i] );
        jsonWindowInfo["owner"] = youmecommon::Value( arrayOwner[i] );
        jsonWindowInfo["window"] = youmecommon::Value( arrayWindow[i] );
        
        jsonRoot["winlist"].append(jsonWindowInfo);
    }
    
    TSK_DEBUG_INFO("window count[%d] window info[%s]", count, XStringToUTF8(jsonRoot.toSimpleString()).c_str());

    
    return YOUME_SUCCESS;
    #endif
}

YouMeErrorCode CYouMeVoiceEngine::getWindowInfoList(char *pWinOwner, char *pWinName, int *pWinId, int *winCount)
{
    int count = 0;
    obs_data_t window_info[100];
    memset(&window_info, 0, sizeof(obs_data_t)*100);

    GetWindowList(2, window_info, &count);
    TSK_DEBUG_INFO("getWindowInfoList count[%d]", count);

    for (int i = 0; i < count; i++) {
        TSK_DEBUG_INFO("window index,%d:owner[%s] window[%s] id[%d]", i, window_info[i].owner_name, window_info[i].window_name, window_info[i].window_id);
        memcpy(&pWinOwner[i*512], window_info[i].owner_name, 512);
        memcpy(&pWinName[i*512], window_info[i].window_name, 512);
        *(pWinId+i) = window_info[i].window_id;        
    }
    *winCount = count;
    
    return YOUME_SUCCESS;
}

YouMeErrorCode CYouMeVoiceEngine::startCaptureAndEncode(int mode, int windowid)
{
	YouMeErrorCode ret = YOUME_SUCCESS;

	if (bInitCaptureAndEncode != false) {
		TSK_DEBUG_WARN("startCaptureAndEncode has been inited!");
		return YOUME_SUCCESS;
	}

	do {
		// 2 is window capture, 3 is screen capture
		if (mode != VIDEO_SHARE_TYPE_WINDOW && mode != VIDEO_SHARE_TYPE_SCREEN) {
			ret = YOUME_ERROR_INVALID_PARAM;
			break;
		}

		obs_data_t settings;
		memset(&settings, 0, sizeof(obs_data_t));
		settings.window_id = windowid;
		capture = InitMedia(mode, &settings);
		if (!capture) {
			TSK_DEBUG_ERROR("startCaptureAndEncode InitMedia fail!");
			ret = YOUME_ERROR_INIT_SHARE_FAIL;
			break;
		}
        
        for( int i = 0 ; i < m_vecShareExclusiveWnd.size() ; i++ )
        {
            SetExclusiveWnd( capture,  m_vecShareExclusiveWnd[i] );
        }
        
		SetCaptureFramerate(capture, shareFps);
		SetCaptureVideoCallback(capture, videoShareCbForMac);

		int stat = StartStream(capture);
		if (stat < 0) {
			TSK_DEBUG_ERROR("startCaptureAndEncode fail, ret:%x", stat);
			ret = YOUME_ERROR_START_SHARE_FAIL;
			break;
		}
	} while (0);

	if (YOUME_SUCCESS == ret) {
		bInitCaptureAndEncode = true;
        shareMode = (YOUME_VIDEO_SHARE_TYPE_t)mode;
        lastShareCaptrureWidth = 0;
        lastShareCaptureHeight = 0;

        // 由于窗口录制时会改变share编码器，这里需要重新设置编码分辨率
        MediaSessionMgr::setVideoNetResolutionForShare(shareWidth, shareHeight);
	}

	TSK_DEBUG_INFO("== startCaptureAndEncode, ret:%d", ret);
	return ret;
}
#endif

// 目前改抽帧处理仅应用于共享视频流， videoid=2
bool CYouMeVoiceEngine::checkFrameTstoDropForShare(uint64_t timestamp)
{
	// use local timestamp to check video fps
	bool is_drop = true;
	int fps = tmedia_defaults_get_video_fps_share();

	uint64_t newTime = tsk_time_now();
	if ((newTime - m_nextVideoTsForShare + 5) / (1000.0f / fps) >= m_frameCountForShare && m_frameCountForShare < fps)
	{
		m_frameCountForShare++;
		is_drop = false;
	}

	if (newTime - m_nextVideoTsForShare >= 1000)
	{
		m_nextVideoTsForShare = newTime;
		m_frameCountForShare = 0;
	}
	return is_drop;  
}

// 目前改抽帧处理仅应用于摄像头视频流， videoid=0
bool CYouMeVoiceEngine::checkFrameTstoDrop(uint64_t timestamp)
{
    // use local timestamp to check video fps
    bool is_drop = true;
    //如果设置了本地预览帧率，就使用本地预览帧率设置，否则用编码帧率
    int fps =  m_previewFps > 0 ? m_previewFps : tmedia_defaults_get_video_fps();

    uint64_t newTime = tsk_time_now();
    if ((newTime - m_nextVideoTs + 5) / (1000.0f / fps) >= m_frameCount && m_frameCount < fps)
    {
        m_frameCount++;
        is_drop = false;
    }

	if (newTime - m_nextVideoTs >= 1000)
	{
		m_nextVideoTs = newTime;
		m_frameCount = 0;
	}
	return is_drop;  
}

uint64_t CYouMeVoiceEngine::getBusinessId() {
    return mBusinessID;
}

void CYouMeVoiceEngine::initLangCodeMap()
{
    m_langCodeMap[LANG_AUTO] = __XT("auto");
    m_langCodeMap[LANG_AF] = __XT("af");                        //南非荷兰语
    m_langCodeMap[LANG_AM] = __XT("am");                        //阿姆哈拉语
    m_langCodeMap[LANG_AR] = __XT("ar");                        //阿拉伯语
    m_langCodeMap[LANG_AR_AE] = __XT("ae");                    //阿拉伯语+阿拉伯联合酋长国 O
    m_langCodeMap[LANG_AR_BH] = __XT("bh");                    //阿拉伯语+巴林 O
    m_langCodeMap[LANG_AR_DZ] = __XT("dz");                    //阿拉伯语+阿尔及利亚 O
    m_langCodeMap[LANG_AR_KW] = __XT("kw");                    //阿拉伯语+科威特 O
    m_langCodeMap[LANG_AR_LB] = __XT("lb");                    //阿拉伯语+黎巴嫩 O
    m_langCodeMap[LANG_AR_OM] = __XT("om");                    //阿拉伯语+阿曼 O
    m_langCodeMap[LANG_AR_SA] = __XT("sa");                    //阿拉伯语+沙特阿拉伯 O
    m_langCodeMap[LANG_AR_SD] = __XT("sd");                    //阿拉伯语+苏丹 O
    m_langCodeMap[LANG_AR_TN] = __XT("tn");                    //阿拉伯语+突尼斯 O
    m_langCodeMap[LANG_AZ] = __XT("az");                        //阿塞拜疆
    m_langCodeMap[LANG_BE] = __XT("be");                        //白俄罗斯语
    m_langCodeMap[LANG_BG] = __XT("bg");                        //保加利亚语
    m_langCodeMap[LANG_BN] = __XT("bn");                        //孟加拉
    m_langCodeMap[LANG_BS] = __XT("bs");                        //波斯尼亚语
    m_langCodeMap[LANG_CA] = __XT("ca");                        //加泰罗尼亚语
    m_langCodeMap[LANG_CA_ES] = __XT("es");                    //加泰罗尼亚语+西班牙 O
    m_langCodeMap[LANG_CO] = __XT("co");                        //科西嘉
    m_langCodeMap[LANG_CS] = __XT("cs");                        //捷克语
    m_langCodeMap[LANG_CY] = __XT("cy");                        //威尔士语
    m_langCodeMap[LANG_DA] = __XT("da");                        //丹麦语
    m_langCodeMap[LANG_DE] = __XT("de");                        //德语
    m_langCodeMap[LANG_DE_CH] = __XT("ch");                    //德语+瑞士 O
    m_langCodeMap[LANG_DE_LU] = __XT("lu");                    //德语+卢森堡 O
    m_langCodeMap[LANG_EL] = __XT("el");                        //希腊语
    m_langCodeMap[LANG_EN] = __XT("en");                        //英语+英国
    m_langCodeMap[LANG_EN_CA] = __XT("ca");                    //英语+加拿大 O
    m_langCodeMap[LANG_EN_IE] = __XT("ie");                    //英语+爱尔兰 O
    m_langCodeMap[LANG_EN_ZA] = __XT("za");                    //英语+南非 O
    m_langCodeMap[LANG_EO] = __XT("eo");                        //世界语
    m_langCodeMap[LANG_ES] = __XT("es");                        //西班牙语
    m_langCodeMap[LANG_ES_BO] = __XT("bo");                    //西班牙语+玻利维亚 O
    m_langCodeMap[LANG_ES_AR] = __XT("ar");                    //西班牙语+阿根廷 O
    m_langCodeMap[LANG_ES_CO] = __XT("co");                    //西班牙语+哥伦比亚 O
    m_langCodeMap[LANG_ES_CR] = __XT("cr");                    //西班牙语+哥斯达黎加 O
    m_langCodeMap[LANG_ES_ES] = __XT("es");                    //西班牙语+西班牙 O X
    m_langCodeMap[LANG_ET] = __XT("et");                        //爱沙尼亚语
    m_langCodeMap[LANG_ES_PA] = __XT("pa");                    //西班牙语+巴拿马 O
    m_langCodeMap[LANG_ES_SV] = __XT("sv");                    //西班牙语+萨尔瓦多 O
    m_langCodeMap[LANG_ES_VE] = __XT("ve");                    //西班牙语+委内瑞拉 O
    m_langCodeMap[LANG_ET_EE] = __XT("ee");                    //爱沙尼亚语+爱沙尼亚 O
    m_langCodeMap[LANG_EU] = __XT("eu");                        //巴斯克
    m_langCodeMap[LANG_FA] = __XT("fa");                        //波斯语
    m_langCodeMap[LANG_FI] = __XT("fi");                        //芬兰语
    m_langCodeMap[LANG_FR] = __XT("fr");                        //法语
    m_langCodeMap[LANG_FR_BE] = __XT("be");                    //法语+比利时 O
    m_langCodeMap[LANG_FR_CA] = __XT("ca");                    //法语+加拿大 O
    m_langCodeMap[LANG_FR_CH] = __XT("ch");                    //法语+瑞士 O
    m_langCodeMap[LANG_FR_LU] = __XT("lu");                    //法语+卢森堡 O
    m_langCodeMap[LANG_FY] = __XT("fy");                        //弗里斯兰
    m_langCodeMap[LANG_GA] = __XT("ga");                        //爱尔兰语
    m_langCodeMap[LANG_GD] = __XT("gd");                        //苏格兰盖尔语
    m_langCodeMap[LANG_GL] = __XT("gl");                        //加利西亚
    m_langCodeMap[LANG_GU] = __XT("gu");                        //古吉拉特文
    m_langCodeMap[LANG_HA] = __XT("ha");                        //豪撒语
    m_langCodeMap[LANG_HI] = __XT("hi");                        //印地语
    m_langCodeMap[LANG_HR] = __XT("hr");                        //克罗地亚语
    m_langCodeMap[LANG_HT] = __XT("ht");                        //海地克里奥尔
    m_langCodeMap[LANG_HU] = __XT("hu");                        //匈牙利语
    m_langCodeMap[LANG_HY] = __XT("hy");                        //亚美尼亚
    m_langCodeMap[LANG_ID] = __XT("id");                        //印度尼西亚
    m_langCodeMap[LANG_IG] = __XT("ig");                        //伊博
    m_langCodeMap[LANG_IS] = __XT("is");                        //冰岛语
    m_langCodeMap[LANG_IT] = __XT("it");                        //意大利语
    m_langCodeMap[LANG_IT_CH] = __XT("ch");                    //意大利语+瑞士 O
    m_langCodeMap[LANG_JA] = __XT("ja");                        //日语
    m_langCodeMap[LANG_KA] = __XT("ka");                        //格鲁吉亚语
    m_langCodeMap[LANG_KK] = __XT("kk");                        //哈萨克语
    m_langCodeMap[LANG_KN] = __XT("kn");                        //卡纳达
    m_langCodeMap[LANG_KM] = __XT("km");                        //高棉语
    m_langCodeMap[LANG_KO] = __XT("ko");                        //朝鲜语
    m_langCodeMap[LANG_KO_KR] = __XT("kr");                    //朝鲜语+南朝鲜 O
    m_langCodeMap[LANG_KU] = __XT("ku");                        //库尔德
    m_langCodeMap[LANG_KY] = __XT("ky");                        //吉尔吉斯斯坦
    m_langCodeMap[LANG_LA] = __XT("la");                        //拉丁语
    m_langCodeMap[LANG_LB] = __XT("lb");                        //卢森堡语
    m_langCodeMap[LANG_LO] = __XT("lo");                        //老挝
    m_langCodeMap[LANG_LT] = __XT("lt");                        //立陶宛语
    m_langCodeMap[LANG_LV] = __XT("lv");                        //拉托维亚语+列托
    m_langCodeMap[LANG_MG] = __XT("mg");                        //马尔加什
    m_langCodeMap[LANG_MI] = __XT("mi");                        //毛利
    m_langCodeMap[LANG_MK] = __XT("mk");                        //马其顿语
    m_langCodeMap[LANG_ML] = __XT("ml");                        //马拉雅拉姆
    m_langCodeMap[LANG_MN] = __XT("mn");                        //蒙古
    m_langCodeMap[LANG_MR] = __XT("mr");                        //马拉地语
    m_langCodeMap[LANG_MS] = __XT("ms");                        //马来语
    m_langCodeMap[LANG_MT] = __XT("mt");                        //马耳他
    m_langCodeMap[LANG_MY] = __XT("my");                        //缅甸
    m_langCodeMap[LANG_NL] = __XT("nl");                        //荷兰语
    m_langCodeMap[LANG_NL_BE] = __XT("be");                    //荷兰语+比利时 O
    m_langCodeMap[LANG_NE] = __XT("ne");                        //尼泊尔
    m_langCodeMap[LANG_NO] = __XT("no");                        //挪威语
    m_langCodeMap[LANG_NY] = __XT("ny");                        //齐切瓦语
    m_langCodeMap[LANG_PL] = __XT("pl");                        //波兰语
    m_langCodeMap[LANG_PS] = __XT("ps");                        //普什图语
    m_langCodeMap[LANG_PT] = __XT("pt");                        //葡萄牙语
    m_langCodeMap[LANG_PT_BR] = __XT("br");                    //葡萄牙语+巴西 O
    m_langCodeMap[LANG_RO] = __XT("ro");                        //罗马尼亚语
    m_langCodeMap[LANG_RU] = __XT("ru");                        //俄语
    m_langCodeMap[LANG_SD] = __XT("sd");                        //信德
    m_langCodeMap[LANG_SI] = __XT("si");                        //僧伽罗语
    m_langCodeMap[LANG_SK] = __XT("sk");                        //斯洛伐克语
    m_langCodeMap[LANG_SL] = __XT("sl");                        //斯洛语尼亚语
    m_langCodeMap[LANG_SM] = __XT("sm");                        //萨摩亚
    m_langCodeMap[LANG_SN] = __XT("sn");                        //修纳
    m_langCodeMap[LANG_SO] = __XT("so");                        //索马里
    m_langCodeMap[LANG_SQ] = __XT("sq");                        //阿尔巴尼亚语
    m_langCodeMap[LANG_SR] = __XT("sr");                        //塞尔维亚语
    m_langCodeMap[LANG_ST] = __XT("st");                        //塞索托语
    m_langCodeMap[LANG_SU] = __XT("su");                        //巽他语
    m_langCodeMap[LANG_SV] = __XT("sv");                        //瑞典语
    m_langCodeMap[LANG_SV_SE] = __XT("se");                    //瑞典语+瑞典 O
    m_langCodeMap[LANG_SW] = __XT("sw");                        //斯瓦希里语
    m_langCodeMap[LANG_TA] = __XT("ta");                        //泰米尔
    m_langCodeMap[LANG_TE] = __XT("te");                        //泰卢固语
    m_langCodeMap[LANG_TG] = __XT("tg");                        //塔吉克斯坦
    m_langCodeMap[LANG_TH] = __XT("th");                        //泰语
    m_langCodeMap[LANG_TL] = __XT("tl");                        //菲律宾
    m_langCodeMap[LANG_TR] = __XT("tr");                        //土耳其语
    m_langCodeMap[LANG_UK] = __XT("uk");                        //乌克兰语
    m_langCodeMap[LANG_UR] = __XT("ur");                        //乌尔都语
    m_langCodeMap[LANG_UZ] = __XT("uz");                        //乌兹别克斯坦
    m_langCodeMap[LANG_VI] = __XT("vi");                        //越南
    m_langCodeMap[LANG_XH] = __XT("xh");                        //科萨
    m_langCodeMap[LANG_YID] = __XT("yi");                        //意第绪语
    m_langCodeMap[LANG_YO] = __XT("yo");                        //约鲁巴语
    m_langCodeMap[LANG_ZH] = __XT("zh-cn");                         //汉语
    m_langCodeMap[LANG_ZH_TW] = __XT("zh-tw");                   //繁体
    m_langCodeMap[LANG_ZU] = __XT("zu");                        //祖鲁语
    
}

bool CYouMeVoiceEngine::checkInputVideoFrameLen(int width, int height, int fmt, int len)
{
    int validSize = 0;
    switch (fmt) {
        case VIDEO_FMT_RGBA32:
        case VIDEO_FMT_BGRA32:
        case VIDEO_FMT_ABGR32:
            validSize = width * height * 4;
            break;
        case VIDEO_FMT_YUV420P:
        case VIDEO_FMT_NV21:
        case VIDEO_FMT_YV12:
        case VIDEO_FMT_NV12:
            validSize = width * height * 3 / 2;
            break;
        case VIDEO_FMT_RGB24:
            validSize = width * height * 3;
            break;
        default:
            validSize = len;
            break;
    }
    
    // 需要考虑stride对齐的情况, 采集的数据可能会多一些字节
    if (validSize > len) {
        TSK_DEBUG_ERROR("checkInputVideoFrameLen w:%d, h:%d, fmt:%u, intput len:%d, valid len:%d", width, height, fmt, len, validSize);
        return -1;
    }

    return 0;
}

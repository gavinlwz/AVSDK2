//
//  IYouMeVoiceEngine.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/9/7.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#define YOUMEDLL_EXPORTS

#include "IYouMeVoiceEngine.h"
#include "AVStatistic.h"
#include "CameraManager.h"
#include "NgnConfigurationEntry.h"
#include "NgnMemoryConfiguration.hpp"
#include "YouMeEngineManagerForQiniu.hpp"
#include "YouMeVoiceEngine.h"
#include "tinymedia/tmedia_defaults.h"
#include "tsk_debug.h"
#include "version.h"
#include <math.h>
#include <mutex>
#include <string.h>


static IYouMeVoiceEngine *mPInstance = NULL;

static CYouMeVoiceEngine *mPEngineImp = NULL;

// 0 正式服， 1 测试服，2 开发测试服，3 商务服
int g_serverMode = SERVER_MODE_FORMAL;
std::string g_serverIp = "192.168.0.1";
int g_serverPort = 0;

IYouMeVoiceEngine *IYouMeVoiceEngine::getInstance ()
{
    if (NULL == mPInstance)
    {
        mPInstance = new IYouMeVoiceEngine ();
    }
    return mPInstance;
}

void IYouMeVoiceEngine::destroy ()
{
    delete mPInstance;
    mPInstance = NULL;
}

//公共接口
YouMeErrorCode IYouMeVoiceEngine::init (IYouMeEventCallback *pEventCallback, const char *pAPPKey, const char *pAPPSecret, YOUME_RTC_SERVER_REGION serverRegionId, const char *pExtServerRegionName)
{
    std::string strAPPKey = "";
    std::string strAPPSecret = "";
    std::string strExtServerRegionName = "";
    if (pAPPKey && pAPPSecret && pExtServerRegionName)
    {
        strAPPKey = pAPPKey;
        strAPPSecret = pAPPSecret;
        strExtServerRegionName = pExtServerRegionName;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->init (pEventCallback, strAPPKey, strAPPSecret, serverRegionId, strExtServerRegionName);
}

void IYouMeVoiceEngine::setToken (const char *pToken, uint32_t uTimeStamp)
{
    mPEngineImp->setToken (pToken, uTimeStamp);
}

void IYouMeVoiceEngine::setRecvCustomDataCallback (IYouMeCustomDataCallback *pCallback)
{
    mPEngineImp->setRecvCustomDataCallback (pCallback);
}

void IYouMeVoiceEngine::setTranslateCallback (IYouMeTranslateCallback *pCallback)
{
    mPEngineImp->setTranslateCallback (pCallback);
}


YouMeErrorCode IYouMeVoiceEngine::inputCustomData (const void *data, int len, uint64_t timestamp, std::string userId)
{
    return mPEngineImp->inputCustomData (data, len, timestamp, userId);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoFrameRawCbEnabled (bool enabled)
{
    return mPEngineImp->setVideoFrameRawCbEnabled (enabled);
}

YouMeErrorCode IYouMeVoiceEngine::unInit ()
{
    return mPEngineImp->unInit ();
}

YouMeErrorCode IYouMeVoiceEngine::requestRestApi (const char *strCommand, const char *strQueryBody, int *requestID)
{
    return mPEngineImp->requestRestApi (strCommand, strQueryBody, requestID);
}

void IYouMeVoiceEngine::setRestApiCallback (IRestApiCallback *cb)
{
    return mPEngineImp->setRestApiCallback (cb);
}

void IYouMeVoiceEngine::setMemberChangeCallback (IYouMeMemberChangeCallback *cb)
{
    return mPEngineImp->setMemberChangeCallback (cb);
}

void IYouMeVoiceEngine::setNotifyCallback (IYouMeChannelMsgCallback *cb)
{
    return mPEngineImp->setNotifyCallback (cb);
}

void IYouMeVoiceEngine::setServerRegion (YOUME_RTC_SERVER_REGION regionId, const char *extRegionName, bool bAppend)
{
    std::string strExtRegionName = extRegionName;
    mPEngineImp->setServerRegion (regionId, strExtRegionName, bAppend);
}

/**
 *  功能描述:切换语音输出设备
 *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
 *
 *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
YouMeErrorCode IYouMeVoiceEngine::setOutputToSpeaker (bool bOutputToSpeaker)
{
    return mPEngineImp->setOutputToSpeaker (bOutputToSpeaker);
}

/**
 *  功能描述:扬声器静音打开/关闭
 *
 *  @param bOn:true——打开，false——关闭
 *  @return 无
 */
void IYouMeVoiceEngine::setSpeakerMute (bool bOn)
{
    TSK_DEBUG_INFO ("Enter");
    mPEngineImp->setSpeakerMute (bOn);
}

/**
 *  功能描述:获取扬声器静音状态
 *
 *  @return true——打开，false——关闭
 */
bool IYouMeVoiceEngine::getSpeakerMute ()
{
    TSK_DEBUG_INFO ("Enter");

    return mPEngineImp->getSpeakerMute ();
}

/**
 *  功能描述:获取麦克风静音状态
 *
 *  @return true——打开，false——关闭
 */
bool IYouMeVoiceEngine::getMicrophoneMute ()
{
    TSK_DEBUG_INFO ("Enter");

    return mPEngineImp->isMicrophoneMute ();
}

/**
 *  功能描述:麦克风静音打开/关闭
 *
 *  @param bOn:true——打开，false——关闭
 *  @return 无
 */
void IYouMeVoiceEngine::setMicrophoneMute (bool mute)
{
    TSK_DEBUG_INFO ("Enter");

    mPEngineImp->setMicrophoneMute (mute);
}

void IYouMeVoiceEngine::setAutoSendStatus (bool bAutoSend)
{
    TSK_DEBUG_INFO ("Enter");
    mPEngineImp->setAutoSendStatus (bAutoSend);
}


//获取当前音量大小
unsigned int IYouMeVoiceEngine::getVolume ()
{
    TSK_DEBUG_INFO ("Enter");

    return mPEngineImp->getVolume ();
}
//调节音量
void IYouMeVoiceEngine::setVolume (const unsigned int &uiVolume)
{
    TSK_DEBUG_INFO ("Enter");

    return mPEngineImp->setVolume (uiVolume);
}

//多人语音接口
YouMeErrorCode IYouMeVoiceEngine::joinChannelSingleMode (const char *pUserID, const char *pChannelID, YouMeUserRole_t eUserRole, bool bVideoAutoRecv)
{
    return joinChannelSingleMode (pUserID, pChannelID, eUserRole, nullptr, bVideoAutoRecv);
}

/** 多房间入口 */
YouMeErrorCode IYouMeVoiceEngine::joinChannelMultiMode (const char *pUserID, const char *pChannelID, YouMeUserRole_t eUserRole)
{
    std::string strUserID = "";
    std::string strChannelID = "";
    if (pUserID && pChannelID)
    {
        strUserID = pUserID;
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->joinChannelMultiMode (strUserID, strChannelID, eUserRole);
}

YouMeErrorCode IYouMeVoiceEngine::joinChannelSingleMode (const char *pUserID, const char *pChannelID, YouMeUserRole_t eUserRole, const char *pJoinAppKey, bool bVideoAutoRecv)
{
    std::string strUserID = "";
    std::string strChannelID = "";
    std::string strAppKey = "";
    if (pUserID && pChannelID)
    {
        strUserID = pUserID;
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }

    if (pJoinAppKey)
    {
        strAppKey = pJoinAppKey;
    }

    return mPEngineImp->joinChannelSingleMode (pUserID, pChannelID, eUserRole, strAppKey, bVideoAutoRecv);
}

/** 指定房间进行语音 */
YouMeErrorCode IYouMeVoiceEngine::speakToChannel (const char *pChannelID)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->speakToChannel (strChannelID);
}

YouMeErrorCode IYouMeVoiceEngine::leaveChannelMultiMode (const char *pChannelID)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->leaveChannelMultiMode (strChannelID);
}

YouMeErrorCode IYouMeVoiceEngine::leaveChannelAll ()
{
    return mPEngineImp->leaveChannelAll ();
}

YouMeErrorCode IYouMeVoiceEngine::getChannelUserList (const char *channelID, int maxCount, bool notifyMemChange)
{
#ifdef YOUME_VIDEO
    return YOUME_ERROR_API_NOT_SUPPORTED;
#else
    return mPEngineImp->getChannelUserList (channelID, maxCount, notifyMemChange);
#endif
}

int IYouMeVoiceEngine::getSDKVersion ()
{
    return SDK_NUMBER;
}
//双人语音接口
/**
 *  功能描述:是否可使用移动网络
 *
 *  @return true-可以使用，false-禁用
 */
bool IYouMeVoiceEngine::getUseMobileNetworkEnabled ()
{
    return mPEngineImp->getUseMobileNetWorkEnabled ();
}
//设置接口，可选
void IYouMeVoiceEngine::setUseMobileNetworkEnabled (bool bEnabled)
{
    return mPEngineImp->setUseMobileNetworkEnabled (bEnabled);
}

YouMeErrorCode IYouMeVoiceEngine::setLocalConnectionInfo (const char *pLocalIP, int iLocalPort, const char *pRemoteIP, int iRemotePort, int iNetType)
{
    return mPEngineImp->setLocalConnectionInfo (pLocalIP, iLocalPort, pRemoteIP, iRemotePort, iNetType);
}

void IYouMeVoiceEngine::clearLocalConnectionInfo ()
{
    mPEngineImp->clearLocalConnectionInfo ();
}

YouMeErrorCode IYouMeVoiceEngine::setRouteChangeFlag (bool enable)
{
    return mPEngineImp->setRouteChangeFlag (enable);
}

//设置房间里其他人的麦克风状态
YouMeErrorCode IYouMeVoiceEngine::setOtherMicMute (const char *pUserID, bool mute)
{
    std::string strUserID = "";
    if (pUserID)
    {
        strUserID = pUserID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setOtherMicMute (strUserID, mute);
}

//设置房间里其他人的扬声器状态
YouMeErrorCode IYouMeVoiceEngine::setOtherSpeakerMute (const char *pUserID, bool mute)
{
    std::string strUserID = "";
    if (pUserID)
    {
        strUserID = pUserID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setOtherSpeakerMute (strUserID, mute);
}
//
////设置屏蔽某人说话
YouMeErrorCode IYouMeVoiceEngine::setListenOtherVoice (const char *pUserID, bool on)
{
    std::string strUserID = "";
    if (pUserID)
    {
        strUserID = pUserID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setListenOtherVoice (strUserID, on);
}


YouMeErrorCode IYouMeVoiceEngine::inputVideoFrame (void *data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int streamID)
{
    return mPEngineImp->inputVideoFrame (data, len, width, height, fmt, rotation, mirror, timestamp, streamID);
}

YouMeErrorCode IYouMeVoiceEngine::inputVideoFrameForShare (void *data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return mPEngineImp->inputVideoFrameForShare (data, len, width, height, fmt, rotation, mirror, timestamp);
}

YouMeErrorCode IYouMeVoiceEngine::inputVideoFrameForIOS (void *pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return mPEngineImp->inputVideoFrameForIOS (pixelbuffer, width, height, fmt, rotation, mirror, timestamp);
}

YouMeErrorCode IYouMeVoiceEngine::inputVideoFrameForIOSShare (void *pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return mPEngineImp->inputVideoFrameForIOSShare (pixelbuffer, width, height, fmt, rotation, mirror, timestamp);
}

#if ANDROID
YouMeErrorCode IYouMeVoiceEngine::inputVideoFrameForAndroid (int textureId, float *matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return mPEngineImp->inputVideoFrameForAndroid (textureId, matrix, width, height, fmt, rotation, mirror, timestamp);
}

YouMeErrorCode IYouMeVoiceEngine::inputVideoFrameForAndroidShare (int textureId, float *matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return mPEngineImp->inputVideoFrameForAndroidShare (textureId, matrix, width, height, fmt, rotation, mirror, timestamp);
}
#endif

YouMeErrorCode IYouMeVoiceEngine::stopInputVideoFrame ()
{
    return mPEngineImp->stopInputVideoFrame ();
}

YouMeErrorCode IYouMeVoiceEngine::stopInputVideoFrameForShare ()
{
    return mPEngineImp->stopInputVideoFrameForShare ();
}

YouMeErrorCode IYouMeVoiceEngine::inputAudioFrame (void *data, int len, uint64_t timestamp, int channels, int bInterleaved)
{
    if (mPEngineImp->m_isInRoom)
    {
        int inputSampleRate = mPEngineImp->m_inputSampleRate;

        // AudioTrack* audioTrack = new AudioTrack(data, len, inputSampleRate, 1, timestamp);
        // AudioTrack* audioTrack = new AudioTrack(len, inputSampleRate, 1, timestamp);
        // YouMeEngineManagerForQiniu::getInstance()->pushAudioTrack(audioTrack);

        return mPEngineImp->mixAudioTrack (data, len, channels, inputSampleRate, 2, timestamp, false, true, bInterleaved, false);
    }
    else
    {
        return YOUME_ERROR_WRONG_STATE;
    }
}


YouMeErrorCode IYouMeVoiceEngine::inputAudioFrameForMix (int streamId, void *data, int len, YMAudioFrameInfo frameInfo, uint64_t timestamp)
{
    if (mPEngineImp->m_isInRoom)
    {
        return mPEngineImp->inputAudioFrameForMix (streamId, data, len, frameInfo, timestamp);
    }
    else
    {
        return YOUME_ERROR_WRONG_STATE;
    }
}

YouMeErrorCode IYouMeVoiceEngine::playBackgroundMusic (const char *pFilePath, bool bRepeat)
{
    std::string strFilePath = "";
    if (pFilePath)
    {
        strFilePath = pFilePath;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->playBackgroundMusic (strFilePath, bRepeat);
}

YouMeErrorCode IYouMeVoiceEngine::pauseBackgroundMusic ()
{
    return mPEngineImp->pauseBackgroundMusic ();
}

YouMeErrorCode IYouMeVoiceEngine::resumeBackgroundMusic ()
{
    return mPEngineImp->resumeBackgroundMusic ();
}

YouMeErrorCode IYouMeVoiceEngine::stopBackgroundMusic ()
{
    return mPEngineImp->stopBackgroundMusic ();
}

YouMeErrorCode IYouMeVoiceEngine::setBackgroundMusicVolume (int vol)
{
    return mPEngineImp->setBackgroundMusicVolume (vol);
}

YouMeErrorCode IYouMeVoiceEngine::setHeadsetMonitorOn (bool micEnabled, bool bgmEnabled)
{
    return mPEngineImp->setHeadsetMonitorOn (micEnabled, bgmEnabled);
}

YouMeErrorCode IYouMeVoiceEngine::setReverbEnabled (bool enabled)
{
    return mPEngineImp->setReverbEnabled (enabled);
}

YouMeErrorCode IYouMeVoiceEngine::setVadCallbackEnabled (bool enabled)
{
    return mPEngineImp->setVadCallbackEnabled (enabled);
}

YouMeErrorCode IYouMeVoiceEngine::pauseChannel ()
{
    return mPEngineImp->pauseChannel ();
}

YouMeErrorCode IYouMeVoiceEngine::resumeChannel ()
{
    return mPEngineImp->resumeChannel ();
}

void IYouMeVoiceEngine::setRecordingTimeMs (unsigned int timeMs)
{
    return mPEngineImp->setRecordingTimeMs (timeMs);
}

YouMeErrorCode IYouMeVoiceEngine::setMicLevelCallback (int maxLevel)
{
    return mPEngineImp->setMicLevelCallback (maxLevel);
}

YouMeErrorCode IYouMeVoiceEngine::setFarendVoiceLevelCallback (int maxLevel)
{
    return mPEngineImp->setFarendVoiceLevelCallback (maxLevel);
}

YouMeErrorCode IYouMeVoiceEngine::setReleaseMicWhenMute (bool enabled)
{
    return mPEngineImp->setReleaseMicWhenMute (enabled);
}

YouMeErrorCode IYouMeVoiceEngine::setExitCommModeWhenHeadsetPlugin (bool enabled)
{
    return mPEngineImp->setExitCommModeWhenHeadsetPlugin (enabled);
}

void IYouMeVoiceEngine::setPlayingTimeMs (unsigned int timeMs)
{
    return mPEngineImp->setPlayingTimeMs (timeMs);
}

YouMeErrorCode IYouMeVoiceEngine::setPcmCallbackEnable (IYouMePcmCallback *pcmCallback, int flag, bool bOutputToSpeaker, int nOutputSampleRate, int nOutputChannel)
{
    return mPEngineImp->setPcmCallbackEnable (pcmCallback, flag, bOutputToSpeaker, nOutputSampleRate, nOutputChannel);
}

YouMeErrorCode IYouMeVoiceEngine::setTranscriberEnabled (bool enable, IYouMeTranscriberCallback *transcriberCallback)
{
    return mPEngineImp->setTranscriberEnabled (enable, transcriberCallback);
};

YouMeErrorCode IYouMeVoiceEngine::setVideoPreDecodeCallbackEnable (IYouMeVideoPreDecodeCallback *preDecodeCallback, bool needDecodeandRender)
{
    return mPEngineImp->setVideoPreDecodeCallbackEnable (preDecodeCallback, needDecodeandRender);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoEncodeParamCallbackEnable (bool isReport)
{
    return mPEngineImp->setVideoEncodeParamCallbackEnable (isReport);
}

YouMeErrorCode IYouMeVoiceEngine::setGrabMicOption (const char *pChannelID, int mode, int maxAllowCount, int maxTalkTime, unsigned int voteTime)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setGrabMicOption (strChannelID, mode, maxAllowCount, maxTalkTime, voteTime);
}

YouMeErrorCode IYouMeVoiceEngine::startGrabMicAction (const char *pChannelID, const char *pContent)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string strContent = pContent ? pContent : "";
    return mPEngineImp->startGrabMicAction (strChannelID, strContent);
}


YouMeErrorCode IYouMeVoiceEngine::stopGrabMicAction (const char *pChannelID, const char *pContent)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string strContent = pContent ? pContent : "";
    return mPEngineImp->stopGrabMicAction (strChannelID, strContent);
}


YouMeErrorCode IYouMeVoiceEngine::requestGrabMic (const char *pChannelID, int score, bool isAutoOpenMic, const char *pContent)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string strContent = pContent ? pContent : "";
    return mPEngineImp->requestGrabMic (strChannelID, score, isAutoOpenMic, strContent);
}


YouMeErrorCode IYouMeVoiceEngine::releaseGrabMic (const char *pChannelID)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->releaseGrabMic (strChannelID);
}


YouMeErrorCode IYouMeVoiceEngine::setInviteMicOption (const char *pChannelID, int waitTimeout, int maxTalkTime)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setInviteMicOption (strChannelID, waitTimeout, maxTalkTime);
}

YouMeErrorCode IYouMeVoiceEngine::requestInviteMic (const char *pChannelID, const char *pUserID, const char *pContent)
{
    std::string strUserID = "";
    if (pUserID)
    {
        strUserID = pUserID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string strContent = pContent ? pContent : "";
    std::string strChannelID = pChannelID ? pChannelID : "";
    return mPEngineImp->requestInviteMic (strChannelID, strUserID, strContent);
}


YouMeErrorCode IYouMeVoiceEngine::responseInviteMic (const char *pUserID, bool isAccept, const char *pContent)
{
    std::string strUserID = "";
    if (pUserID)
    {
        strUserID = pUserID;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string strContent = pContent ? pContent : "";
    return mPEngineImp->responseInviteMic (strUserID, isAccept, strContent);
}


YouMeErrorCode IYouMeVoiceEngine::stopInviteMic ()
{
    return mPEngineImp->stopInviteMic ();
}

YouMeErrorCode IYouMeVoiceEngine::sendMessage (const char *pChannelID, const char *pContent, const char *pUserID, int *requestID)
{
    return mPEngineImp->sendMessage (pChannelID, pContent, pUserID, requestID);
}

YouMeErrorCode IYouMeVoiceEngine::kickOtherFromChannel (const char *pUserID, const char *pChannelID, int lastTime)
{
    return mPEngineImp->kickOther (pUserID, pChannelID, lastTime);
}

void IYouMeVoiceEngine::setExternalInputMode (bool bInputModeEnabled)
{
    return mPEngineImp->setExternalInputMode (bInputModeEnabled);
}

YouMeErrorCode IYouMeVoiceEngine::openVideoEncoder (const char *pFilePath)
{
    std::string strFilePath = "";
    if (pFilePath)
    {
        strFilePath = pFilePath;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->openVideoEncoder (strFilePath);
}

int IYouMeVoiceEngine::createRender (const char *userId)
{
    if (userId == NULL || strlen (userId) == 0)
    {
        return YOUME_ERROR_INVALID_PARAM;
    }

    std::string user = userId;
    return mPEngineImp->createRender (user);
}

int IYouMeVoiceEngine::deleteRender (int renderId)
{
    return mPEngineImp->deleteRender (renderId);
}

int IYouMeVoiceEngine::deleteRender (const char *userId)
{
    return mPEngineImp->deleteRender (userId);
}

YouMeErrorCode IYouMeVoiceEngine::maskVideoByUserId (const char *userId, bool mask)
{
    if (userId == NULL || strlen (userId) == 0)
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    std::string user = userId;
    return mPEngineImp->maskVideoByUserId (user, mask);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoCallback (IYouMeVideoFrameCallback *cb)
{
    //    if (cb == NULL) {
    //        TSK_DEBUG_INFO("########### IYouMeVoiceEngine::setVideoCallback:%x", cb);
    //        return YOUME_ERROR_INVALID_PARAM;
    //    }

    return mPEngineImp->setVideoCallback (cb);
}

YouMeErrorCode IYouMeVoiceEngine::startCapture ()
{
    return mPEngineImp->startCapture ();
}

YouMeErrorCode IYouMeVoiceEngine::stopCapture ()
{
    return mPEngineImp->stopCapture ();
}

YouMeErrorCode IYouMeVoiceEngine::setVideoPreviewFps (int fps)
{
    return mPEngineImp->setVideoPreviewFps (fps);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoFps (int fps)
{
    return mPEngineImp->setVideoFps (fps);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoFpsForSecond (int fps)
{
    return mPEngineImp->setVideoFpsForSecond (fps);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoFpsForShare (int fps)
{
    return mPEngineImp->setVideoFpsForShare (fps);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoLocalResolution (int width, int height)
{
    return mPEngineImp->setVideoLocalResolution (width, height);
}

YouMeErrorCode IYouMeVoiceEngine::setCaptureFrontCameraEnable (bool enable)
{
    return mPEngineImp->setCaptureFrontCameraEnable (enable);
}

YouMeErrorCode IYouMeVoiceEngine::switchCamera ()
{
    return mPEngineImp->switchCamera ();
}

YouMeErrorCode IYouMeVoiceEngine::resetCamera ()
{
    return mPEngineImp->resetCamera ();
}

void IYouMeVoiceEngine::setLogLevel (YOUME_LOG_LEVEL consoleLevel, YOUME_LOG_LEVEL fileLevel)
{
    return mPEngineImp->setLogLevel (consoleLevel, fileLevel);
}
YouMeErrorCode IYouMeVoiceEngine::setExternalInputSampleRate (YOUME_SAMPLE_RATE inputSampleRate, YOUME_SAMPLE_RATE mixedCallbackSampleRate)
{
    return mPEngineImp->setExternalInputSampleRate (inputSampleRate, mixedCallbackSampleRate);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoSmooth (int enable)
{
    return mPEngineImp->setVideoSmooth (enable);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoNetAdjustmode (int mode)
{
    return mPEngineImp->setVideoNetAdjustmode (mode);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoNetResolution (int width, int height)
{
    return mPEngineImp->setVideoNetResolution (width, height);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoNetResolutionForSecond (int width, int height)
{
    return mPEngineImp->setVideoNetResolutionForSecond (width, height);
}

YouMeErrorCode IYouMeVoiceEngine::setVideoNetResolutionForShare (int width, int height)
{
    return mPEngineImp->setVideoNetResolutionForShare (width, height);
}

void IYouMeVoiceEngine::setAVStatisticInterval (int interval)
{
    return mPEngineImp->setAVStatisticInterval (interval);
}

void IYouMeVoiceEngine::setAVStatisticCallback (IYouMeAVStatisticCallback *cb)
{
    return mPEngineImp->setAVStatisticCallback (cb);
}

void IYouMeVoiceEngine::setAudioQuality (YOUME_AUDIO_QUALITY_t quality)
{
    return mPEngineImp->setAudioQuality (quality);
}

void IYouMeVoiceEngine::setVideoCodeBitrate (unsigned int maxBitrate, unsigned int minBitrate)
{
    return mPEngineImp->setVideoCodeBitrate (maxBitrate, minBitrate);
}

YouMeErrorCode IYouMeVoiceEngine::setVBR (bool useVBR)
{
    return mPEngineImp->setVBR (useVBR);
}

YouMeErrorCode IYouMeVoiceEngine::setVBRForSecond (bool useVBR)
{
    return mPEngineImp->setVBRForSecond (useVBR);
}

YouMeErrorCode IYouMeVoiceEngine::setVBRForShare (bool useVBR)
{
    return mPEngineImp->setVBRForShare (useVBR);
}

void IYouMeVoiceEngine::setVideoCodeBitrateForSecond (unsigned int maxBitrate, unsigned int minBitrate)
{
    return mPEngineImp->setVideoCodeBitrateForSecond (maxBitrate, minBitrate);
}

void IYouMeVoiceEngine::setVideoCodeBitrateForShare (unsigned int maxBitrate, unsigned int minBitrate)
{
    return mPEngineImp->setVideoCodeBitrateForShare (maxBitrate, minBitrate);
}

unsigned int IYouMeVoiceEngine::getCurrentVideoCodeBitrate ()
{
    return mPEngineImp->getCurrentVideoCodeBitrate ();
}

void IYouMeVoiceEngine::setVideoHardwareCodeEnable (bool bEnable)
{
    return mPEngineImp->setVideoHardwareCodeEnable (bEnable);
}

bool IYouMeVoiceEngine::getVideoHardwareCodeEnable ()
{
    return mPEngineImp->getVideoHardwareCodeEnable ();
}

bool IYouMeVoiceEngine::getUseGL ()
{
    return mPEngineImp->getUseGL ();
}

void IYouMeVoiceEngine::setVideoNoFrameTimeout (int timeout)
{
    return mPEngineImp->setVideoNoFrameTimeout (timeout);
}


YouMeErrorCode IYouMeVoiceEngine::queryUsersVideoInfo (std::vector<std::string> &userList)
{
    return mPEngineImp->queryUsersVideoInfo (userList);
}


YouMeErrorCode IYouMeVoiceEngine::setUsersVideoInfo (std::vector<IYouMeVoiceEngine::userVideoInfo> &videoInfoList)
{
    return mPEngineImp->setUsersVideoInfo (videoInfoList);
}

// 进入房间后，切换身份
YouMeErrorCode IYouMeVoiceEngine::setUserRole (YouMeUserRole_t eUserRole)
{
    return mPEngineImp->setUserRole (eUserRole);
}

// 获取身份
YouMeUserRole_t IYouMeVoiceEngine::getUserRole ()
{
    return mPEngineImp->getUserRole ();
}

// 背景音乐是否在播放
bool IYouMeVoiceEngine::isBackgroundMusicPlaying ()
{
    return mPEngineImp->isBackgroundMusicPlaying ();
}

// 是否初始化成功
bool IYouMeVoiceEngine::isInited ()
{
    return mPEngineImp->isInited ();
}

// 是否在某个语音房间内
bool IYouMeVoiceEngine::isInChannel (const char *pChannelID)
{
    std::string strChannelID = "";
    if (pChannelID)
    {
        strChannelID = pChannelID;
    }
    else
    {
        return false;
    }

    return mPEngineImp->isInRoom (strChannelID);
}

// 是否在某个语音房间内
bool IYouMeVoiceEngine::isInChannel ()
{
    return mPEngineImp->isInRoom ();
}

YouMeErrorCode IYouMeVoiceEngine::setUserLogPath (const char *pFilePath)
{
    std::string strFilePath = "";
    if (pFilePath)
    {
        strFilePath = pFilePath;
    }
    else
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    return mPEngineImp->setUserLogPath (strFilePath);
}

YouMeErrorCode IYouMeVoiceEngine::setTCPMode (int iUseTCP)
{
    return mPEngineImp->setTCPMode (iUseTCP);
}

YouMeErrorCode IYouMeVoiceEngine::openBeautify (bool open)
{
    return mPEngineImp->openBeautify (open);
}

YouMeErrorCode IYouMeVoiceEngine::beautifyChanged (float param)
{
    return mPEngineImp->beautifyChanged (param);
}

// YouMeErrorCode IYouMeVoiceEngine::stretchFace(bool stretch)
//{
//    return mPEngineImp->stretchFace(stretch);
//}

IYouMeVoiceEngine::IYouMeVoiceEngine ()
{
    mPEngineImp = CYouMeVoiceEngine::getInstance ();
}

bool IYouMeVoiceEngine::releaseMicSync ()
{
    return mPEngineImp->releaseMicSync ();
}

bool IYouMeVoiceEngine::resumeMicSync ()
{
    return mPEngineImp->resumeMicSync ();
}


int IYouMeVoiceEngine::getCameraCount ()
{
    return mPEngineImp->getCameraCount ();
}


const char *IYouMeVoiceEngine::getCameraName (int cameraId)
{
    std::string name = mPEngineImp->getCameraName (cameraId);
    char *cstr = new char[name.size () + 1];
    strcpy (cstr, name.c_str ());
    return cstr;
}


YouMeErrorCode IYouMeVoiceEngine::setOpenCameraId (int cameraId)
{
    return mPEngineImp->setOpenCameraId (cameraId);
}

int IYouMeVoiceEngine::getRecordDeviceCount ()
{
    return mPEngineImp->getRecordDeviceCount ();
}

bool IYouMeVoiceEngine::getRecordDeviceInfo (int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceUId[MAX_DEVICE_ID_LENGTH])
{
    return mPEngineImp->getRecordDeviceInfo (index, deviceName, deviceUId);
}

YouMeErrorCode IYouMeVoiceEngine::setRecordDevice (const char *deviceUId)
{
    return mPEngineImp->setRecordDevice (deviceUId);
}

void IYouMeVoiceEngine::setMixVideoSize (int width, int height)
{
    mPEngineImp->setMixVideoSize (width, height);
}
void IYouMeVoiceEngine::addMixOverlayVideo (const char *userId, int x, int y, int z, int width, int height)
{
    mPEngineImp->addMixOverlayVideo (userId, x, y, z, width, height);
}
void IYouMeVoiceEngine::removeMixOverlayVideo (const char *userId)
{
    mPEngineImp->removeMixOverlayVideo (userId);
}
void IYouMeVoiceEngine::removeAllOverlayVideo ()
{
    mPEngineImp->removeAllOverlayVideo ();
}


YouMeErrorCode IYouMeVoiceEngine::setExternalFilterEnabled (bool enabled)
{
    return mPEngineImp->setExternalFilterEnabled (enabled);
}

int IYouMeVoiceEngine::enableLocalVideoRender (bool enabled)
{
    return mPEngineImp->enableLocalVideoRender (enabled);
}

int IYouMeVoiceEngine::enableLocalVideoSend (bool enabled)
{
    return mPEngineImp->enableLocalVideoSend (enabled);
}

int IYouMeVoiceEngine::muteAllRemoteVideoStreams (bool mute)
{
    return mPEngineImp->muteAllRemoteVideoStreams (mute);
}

int IYouMeVoiceEngine::setDefaultMuteAllRemoteVideoStreams (bool mute)
{
    return mPEngineImp->setDefaultMuteAllRemoteVideoStreams (mute);
}

int IYouMeVoiceEngine::muteRemoteVideoStream (std::string &uid, bool mute)
{
    return mPEngineImp->muteRemoteVideoStream (uid, mute);
}

#if TARGET_OS_IPHONE
bool IYouMeVoiceEngine::isCameraZoomSupported ()
{
    return mPEngineImp->isCameraZoomSupported ();
}
float IYouMeVoiceEngine::setCameraZoomFactor (float zoomFactor)
{
    return mPEngineImp->setCameraZoomFactor (zoomFactor);
}
bool IYouMeVoiceEngine::isCameraFocusPositionInPreviewSupported ()
{
    return mPEngineImp->isCameraFocusPositionInPreviewSupported ();
}
bool IYouMeVoiceEngine::setCameraFocusPositionInPreview (float x, float y)
{
    return mPEngineImp->setCameraFocusPositionInPreview (x, y);
}
bool IYouMeVoiceEngine::isCameraTorchSupported ()
{
    return mPEngineImp->isCameraTorchSupported ();
}
bool IYouMeVoiceEngine::setCameraTorchOn (bool isOn)
{
    return mPEngineImp->setCameraTorchOn (isOn);
}
bool IYouMeVoiceEngine::isCameraAutoFocusFaceModeSupported ()
{
    return mPEngineImp->isCameraAutoFocusFaceModeSupported ();
}
bool IYouMeVoiceEngine::setCameraAutoFocusFaceModeEnabled (bool enable)
{
    return mPEngineImp->setCameraAutoFocusFaceModeEnabled (enable);
}
#endif
void IYouMeVoiceEngine::setLocalVideoMirrorMode (YouMeVideoMirrorMode mode)
{
    return mPEngineImp->setLocalVideoMirrorMode (mode);
}

void IYouMeVoiceEngine::setLocalVideoPreviewMirror (bool enable)
{
    if (enable)
    {
        mPEngineImp->setLocalVideoMirrorMode (YOUME_VIDEO_MIRROR_MODE_AUTO);
    }
    else
    {
        mPEngineImp->setLocalVideoMirrorMode (YOUME_VIDEO_MIRROR_MODE_FAR);
    }
}

YouMeErrorCode IYouMeVoiceEngine::startPush (const char *pushUrl)
{
    return mPEngineImp->startPush (pushUrl);
}
YouMeErrorCode IYouMeVoiceEngine::stopPush ()
{
    return mPEngineImp->stopPush ();
}

YouMeErrorCode IYouMeVoiceEngine::setPushMix (const char *pushUrl, int width, int height)
{
    return mPEngineImp->setPushMix (pushUrl, width, height);
}
YouMeErrorCode IYouMeVoiceEngine::clearPushMix ()
{
    return mPEngineImp->clearPushMix ();
}
YouMeErrorCode IYouMeVoiceEngine::addPushMixUser (const char *userId, int x, int y, int z, int width, int height)
{
    return mPEngineImp->addPushMixUser (userId, x, y, z, width, height);
}
YouMeErrorCode IYouMeVoiceEngine::removePushMixUser (const char *userId)
{
    return mPEngineImp->removePushMixUser (userId);
}

#ifdef WIN32
YouMeErrorCode IYouMeVoiceEngine::StartShareStream (int mode, HWND renderHandle, HWND captureHandle)
{
    return CYouMeVoiceEngine::getInstance ()->startShareStream (mode, renderHandle, captureHandle);
}

YouMeErrorCode IYouMeVoiceEngine::setShareExclusiveWnd (HWND windowid)
{
    return CYouMeVoiceEngine::getInstance ()->setShareExclusiveWnd (windowid);
}

void IYouMeVoiceEngine::GetWindowInfoList (HWND *pHwnd, char *pWinName, int *winCount)
{
    CYouMeVoiceEngine::getInstance ()->getWindowInfoList (pHwnd, pWinName, winCount);
}

void IYouMeVoiceEngine::GetScreenInfoList (HWND *pHwnd, char *pScreenName, int *screenCount)
{
    CYouMeVoiceEngine::getInstance ()->getScreenInfoList (pHwnd, pScreenName, screenCount);
}

YouMeErrorCode IYouMeVoiceEngine::startSaveScreen (const std::string &filePath, HWND renderHandle, HWND captureHandle)
{
    return CYouMeVoiceEngine::getInstance ()->startSaveScreen (filePath, renderHandle, captureHandle);
}
#elif TARGET_OS_OSX // for mac screen record
YouMeErrorCode IYouMeVoiceEngine::checkSharePermission ()
{
    return CYouMeVoiceEngine::getInstance ()->checkSharePermission ();
}

YouMeErrorCode IYouMeVoiceEngine::StartShareStream (int mode, int windowid)
{
    return CYouMeVoiceEngine::getInstance ()->startShareStream (mode, windowid);
}

YouMeErrorCode IYouMeVoiceEngine::setShareExclusiveWnd (int windowid)
{
    return CYouMeVoiceEngine::getInstance ()->setShareExclusiveWnd (windowid);
}

void IYouMeVoiceEngine::setShareContext (void *context)
{
    return CYouMeVoiceEngine::getInstance ()->setShareContext (context);
}

void IYouMeVoiceEngine::GetWindowInfoList (char *pWinOwner, char *pWinName, int *pWinId, int *winCount)
{
    CYouMeVoiceEngine::getInstance ()->getWindowInfoList (pWinOwner, pWinName, pWinId, winCount);
}

#endif

YouMeErrorCode IYouMeVoiceEngine::startSaveScreen (const std::string &filePath)
{
    return CYouMeVoiceEngine::getInstance ()->startSaveScreen (filePath);
}

void IYouMeVoiceEngine::StopShareStream ()
{
    CYouMeVoiceEngine::getInstance ()->stopShareStream ();
}

void IYouMeVoiceEngine::stopSaveScreen ()
{
    CYouMeVoiceEngine::getInstance ()->stopSaveScreen ();
}

YouMeErrorCode IYouMeVoiceEngine::switchResolutionForLandscape ()
{
    return CYouMeVoiceEngine::getInstance ()->switchResolutionForLandscape ();
}

YouMeErrorCode IYouMeVoiceEngine::resetResolutionForPortrait ()
{
    return CYouMeVoiceEngine::getInstance ()->resetResolutionForPortrait ();
}

YouMeErrorCode IYouMeVoiceEngine::translateText (unsigned int *requestID, const char *text, YouMeLanguageCode destLangCode, YouMeLanguageCode srcLangCode)
{
    return CYouMeVoiceEngine::getInstance ()->translateText (requestID, text, destLangCode, srcLangCode);
}

IYouMeVoiceEngine::~IYouMeVoiceEngine ()
{
    CYouMeVoiceEngine::destroy ();
}

void SetServerMode (SERVER_MODE serverMode)
{
    TSK_DEBUG_INFO ("Set server mode:%d", serverMode);
    g_serverMode = serverMode;
}

void SetServerIpPort (const char *ip, int port)
{
    if (!ip)
    {
        return;
    }
    g_serverIp = ip;
    g_serverPort = port;
}

bool GetSoundtouchEnabled ()
{
    return CNgnMemoryConfiguration::getInstance ()->GetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_ENABLED, NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_ENABLED) != 0;
}

void SetSoundtouchEnabled (bool bEnabled)
{
    if (GetSoundtouchEnabled () == bEnabled)
        return;

    CNgnMemoryConfiguration::getInstance ()->SetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_ENABLED, bEnabled ? 1 : 0);

    CYouMeVoiceEngine::getInstance ()->setSoundtouchEnabled (bEnabled);
}

float GetSoundtouchTempo ()
{
    int nTempo = CNgnMemoryConfiguration::getInstance ()->GetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_TEMPO, NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_TEMPO);

    return ((float)nTempo) / 100.0f;
}

void SetSoundtouchTempo (float fTempo)
{
    const float EPSINON = 0.00001;
    if (fabsf (fTempo - GetSoundtouchTempo ()) <= EPSINON)
        return;

    CNgnMemoryConfiguration::getInstance ()->SetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_TEMPO, int(fTempo * 100));

    CYouMeVoiceEngine::getInstance ()->setSoundtouchTempo (fTempo);
}

float GetSoundtouchRate ()
{
    int nRate = CNgnMemoryConfiguration::getInstance ()->GetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_RATE, NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_RATE);

    return ((float)nRate) / 100.0f;
}

void SetSoundtouchRate (float fRate)
{
    const float EPSINON = 0.00001;
    if (fabsf (fRate - GetSoundtouchRate ()) <= EPSINON)
        return;

    CNgnMemoryConfiguration::getInstance ()->SetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_RATE, int(fRate * 100));

    CYouMeVoiceEngine::getInstance ()->setSoundtouchRate (fRate);
}

float GetSoundtouchPitch ()
{
    int nPitch = CNgnMemoryConfiguration::getInstance ()->GetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_PITCH, NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_PITCH);

    return ((float)nPitch) / 100.0f;
}

void SetSoundtouchPitch (float fPitch)
{
    const float EPSINON = 0.00001;
    if (fabsf (fPitch - GetSoundtouchPitch ()) <= EPSINON)
        return;

    CNgnMemoryConfiguration::getInstance ()->SetConfiguration (NgnConfigurationEntry::SOUNDTOUCH_PITCH, int(fPitch * 100));

    CYouMeVoiceEngine::getInstance ()->setSoundtouchPitch (fPitch);
}

void SetNetworkChange ()
{
    CYouMeVoiceEngine::getInstance ()->onNetWorkChanged (NETWORK_TYPE_MOBILE);
}


#if defined(__APPLE__)
#include "tinydav/tdav_apple.h"
#endif
void SetTestCategory (int iCategory)
{
#if defined(__APPLE__)
    tdav_SetTestCategory (iCategory);
#endif
}

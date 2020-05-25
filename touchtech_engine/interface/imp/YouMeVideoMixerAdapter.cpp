//
//  YouMeVideoMixerAdapter.cpp
//  youme_voice_engine
//
//  Created by bhb on 2017/12/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include"YouMeVideoMixerAdapter.h"
#include "YouMeEngineVideoCodec.hpp"
#include "MediaSessionMgr.h"
#include <YouMeCommon/TimeUtil.h>
#include "tsk_debug.h"
#include "AVStatistic.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#if TARGET_OS_IPHONE || TARGET_OS_OSX
#include "GLESVideoMixer.h"
#define GLES_MIXER 1
#elif ANDROID
#include "VideoMixerDroid.h"
#define GLES_MIXER 1
#else
#define GLES_MIXER 0

#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "../../../../baseWrapper/src/XConfigCWrapper.hpp"
#ifdef __cplusplus
}
#endif

#define CALLBACK_TIME_COST_FLOOR_MS 33

YouMeVideoMixerAdapter::YouMeVideoMixerAdapter()
:m_videoMixer(nullptr)
,m_useGLES(false)
,m_openBeauty(false)
,m_beautyLevel(0.0f)
,m_videoFrameOutCb(nullptr)
,m_videoFramePushCb(nullptr)
,m_videoWidthForFirst(0)
,m_videoHeightForFirst(0)
,m_videoWidthForSecond(0)
,m_videoHeightForSecond(0)
,m_videoFps(0)
,m_localVideoRenderEnabled(true)
,m_localVideoSendEnabled(true)
,m_remoteAllVideoRenderEnabled(true)
,m_remoteDefaultAllVideoRenderEnabled(true)
,m_filterExtEnabled(false)
{
    
}
YouMeVideoMixerAdapter::~YouMeVideoMixerAdapter()
{
   releaseMixer();
}

YouMeVideoMixerAdapter* YouMeVideoMixerAdapter::getInstance()
{
    static YouMeVideoMixerAdapter* instance = nullptr;
    if(!instance){
        instance = new YouMeVideoMixerAdapter();
        //instance->initMixerAdapter();
    }
    
    return instance;
}

void YouMeVideoMixerAdapter::initMixerAdapter(bool isGLES)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if(isGLES != m_useGLES && m_videoMixer){
        delete m_videoMixer;
        m_videoMixer = nullptr;
    }
    else if(isGLES == m_useGLES && m_videoMixer){
        return;
    }
    m_useGLES = false;
    if (isGLES &&
        GLES_MIXER &&
        !tmedia_defaults_get_video_frame_raw_cb_enabled()) {
#if TARGET_OS_IPHONE || TARGET_OS_OSX
        if(tmedia_defaults_get_video_hwcodec_enabled() && Config_GetBool("HW_ENCODE", 0) ){
            m_videoMixer = new videocore::GLESVideoMixer();
             m_useGLES = true;
        }
#elif defined(ANDROID)
        if(tmedia_defaults_get_video_hwcodec_enabled() && Config_GetBool("HW_ENCODE", 0) ){
            m_videoMixer = VideoMixerDroid::createVideoMixer();
            // 强制打开opengl
            if(1/*VideoMixerDroid::isSupportGLES()*/){
                 ((VideoMixerDroid*)m_videoMixer)->initMixer();
                  m_useGLES = true;
            }
            else{
                ((VideoMixerDroid*)m_videoMixer)->setOpenEncoderRawCallBack(true);
            }
        }
        
#endif
    }
    if(m_videoMixer){
        setVideoFps(m_videoFps);
        setVideoNetResolutionWidth(m_videoWidthForFirst, m_videoHeightForFirst);
        setVideoNetResolutionWidthForSecond(m_videoWidthForSecond, m_videoHeightForSecond);
        if (m_openBeauty)
            openBeautify();
        else
            closeBeautify();
        setBeautifyLevel(m_beautyLevel);
        setExternalFilterEnabled(m_filterExtEnabled);
        TSK_DEBUG_INFO("opengles init mixer ok!");
    }
}

void YouMeVideoMixerAdapter::releaseMixer()
{
    std::unique_lock<std::mutex> lock(m_mutex);
#ifdef ANDROID
    if (m_videoMixer && ((VideoMixerDroid*)m_videoMixer)->setOpenEncoderRawCallBack(true)) {
        return;
    }
#endif
    if(m_videoMixer){
        delete m_videoMixer;
        m_videoMixer = nullptr;
        m_useGLES = false;
    }
}

void YouMeVideoMixerAdapter::resetMixer(bool isGLES)
{
    if(!m_videoMixer)
        return;

#ifdef ANDROID
    if (!((VideoMixerDroid*)m_videoMixer)->setOpenEncoderRawCallBack(true))
         m_useGLES = false;
#else
    m_useGLES = false;
#endif
//    if(m_openBeauty && !MediaSessionMgr::defaultsGetExternalInputMode()){
//        ICameraManager::getInstance()->stopCapture();
//        ICameraManager::getInstance()->openBeautify(m_openBeauty);
//        ICameraManager::getInstance()->beautifyChanged(m_beautyLevel);
//        ICameraManager::getInstance()->startCapture();
//    }
    TSK_DEBUG_INFO("opengles mixer -> sw!! \n");
}

void YouMeVideoMixerAdapter::leavedRoom(){
    
    m_localVideoRenderEnabled = true;
    m_localVideoSendEnabled = true;
    m_remoteAllVideoRenderEnabled = true;
    m_mapMuteUsers.clear();
    
}


void YouMeVideoMixerAdapter::setMixVideoSize(int width, int height)
{
  //  std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->setMixVideoSize(width, height);
    YouMeEngineManagerForQiniu::getInstance()->setMixVideoSize(width, height);
}

void YouMeVideoMixerAdapter::setVideoNetResolutionWidth(int width, int height)
{
  //  std::unique_lock<std::mutex> lock(m_mutex);
    if(width == 0 || height == 0)
        return;
    if(m_videoMixer)
        m_videoMixer->setVideoNetResolutionWidth(width, height);
    m_videoWidthForFirst = width;
    m_videoHeightForFirst = height;
}

void YouMeVideoMixerAdapter::setVideoNetResolutionWidthForSecond(int width, int height)
{
  //  std::unique_lock<std::mutex> lock(m_mutex);
    if(width == 0 || height == 0)
        return;
    if(m_videoMixer)
        m_videoMixer->setVideoNetResolutionWidthForSecond(width, height);
    m_videoWidthForSecond= width;
    m_videoHeightForSecond = height;
}

void YouMeVideoMixerAdapter::addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height)
{
  //  std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->addMixOverlayVideo(userId, x, y, z, width, height);
    YouMeEngineManagerForQiniu::getInstance()->addMixOverlayVideo(userId, x, y, z, width, height);
}

void YouMeVideoMixerAdapter::removeMixOverlayVideo(std::string userId)
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->removeMixOverlayVideo(userId);
    YouMeEngineManagerForQiniu::getInstance()->removeMixOverlayVideo(userId);
}

void YouMeVideoMixerAdapter::removeAllOverlayVideo()
{
 //   std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->removeAllOverlayVideo();
    YouMeEngineManagerForQiniu::getInstance()->removeAllOverlayVideo();
}

void YouMeVideoMixerAdapter::setVideoFrameCallback(IYouMeVideoFrameCallback* cb)
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    m_videoFrameOutCb = cb;
}

bool YouMeVideoMixerAdapter::needMixing(std::string userId)
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_useGLES){
        if(m_videoMixer){
           m_videoMixer->needMixing(userId);
        }
    }
    else
    {
       return YouMeEngineManagerForQiniu::getInstance()->needMixing(userId);
    }
    return false;
}

void YouMeVideoMixerAdapter::openBeautify()
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->openBeautify();
    m_openBeauty = true;
}

void YouMeVideoMixerAdapter::closeBeautify()
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->closeBeautify();
    m_openBeauty = false;
}

void YouMeVideoMixerAdapter::setBeautifyLevel(float level)
{
//    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->setBeautifyLevel(level);
    m_beautyLevel = level;
}

void YouMeVideoMixerAdapter::pushVideoFrameLocal(std::string userId, void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    
    if(!m_useGLES){
        Frame* frame = new Frame(data, len, width, height, fmt, mirror, timestamp);
        YouMeEngineManagerForQiniu::getInstance()->pushFrame(frame);
    }
    else if(m_videoMixer){
#ifdef __APPLE__
    ((videocore::GLESVideoMixer*)m_videoMixer)->pushBufferForLocal(userId, data, len, width, height, rotation, mirror, timestamp, (videocore::BufferFormatType)fmt);
#elif defined(ANDROID)
    ((VideoMixerDroid*)m_videoMixer)->pushVideoBufferRawForLocalDroid(userId, fmt, data, len, width, height, rotation, mirror, timestamp);
#endif
    }
}

void YouMeVideoMixerAdapter::pushVideoFrameLocalForIOS(std::string userId, void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
  
    if(!m_videoMixer || !m_useGLES)
        return;
#ifdef __APPLE__
    ((videocore::GLESVideoMixer*)m_videoMixer)->pushBufferForLocal(userId, (CVPixelBufferRef)pixelbuffer, rotation, mirror, timestamp);
#endif
}

void YouMeVideoMixerAdapter::pushVideoFrameLocalForAndroid(std::string userId, int texture, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
   
    if(!m_videoMixer || !m_useGLES)
        return;
#ifdef ANDROID
    ((VideoMixerDroid*)m_videoMixer)->pushVideoBufferGLESForLocalDroid(userId, fmt, texture, matrix, width, height, rotation, mirror, timestamp);
#endif
}

void YouMeVideoMixerAdapter::pushVideoFrameRemote(std::string userId, void* data, int len, int width, int height, int fmt, uint64_t timestamp){
    if(!m_remoteAllVideoRenderEnabled
       || !m_remoteDefaultAllVideoRenderEnabled
       || m_mapMuteUsers.find(userId) != m_mapMuteUsers.end())
        return;
   
    if(!m_useGLES){
        YouMeEngineManagerForQiniu::getInstance()->frameRender(userId, width, height, 0, len, data, timestamp);
    }
    else if(m_videoMixer){
#ifdef ANDROID
        ((VideoMixerDroid*)m_videoMixer)->pushVideoBufferRawForRemoteDroid(userId, fmt, data, len, width, height, timestamp);
#endif
    }

    if(m_videoFrameOutCb){
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameCallback(userId.c_str(), data, len, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameCallback cost:%lld", t2 - t1);
        }
    }
}

void YouMeVideoMixerAdapter::pushVideoFrameRemoteForIOS(std::string userId, void* pixelbuffer, int width, int height, int fmt, uint64_t timestamp){
    if(!m_remoteAllVideoRenderEnabled
       || !m_remoteDefaultAllVideoRenderEnabled
       || m_mapMuteUsers.find(userId) != m_mapMuteUsers.end())
        return;
    
    if(m_videoFrameOutCb){
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameCallbackGLESForIOS(userId.c_str(), pixelbuffer, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameCallbackGLESForIOS cost:%lld", t2 - t1);
        }
    }
    if(!m_videoMixer || !m_useGLES)
        return;
#ifdef __APPLE__
    ((videocore::GLESVideoMixer*)m_videoMixer)->pushBufferForRemote(userId, (CVPixelBufferRef)pixelbuffer, timestamp);
#endif
}

void YouMeVideoMixerAdapter::pushVideoFrameRemoteForAndroid(std::string userId, int texture, float* matrix, int width, int height, int fmt, uint64_t timestamp){
    if(!m_remoteAllVideoRenderEnabled
       || !m_remoteDefaultAllVideoRenderEnabled
       || m_mapMuteUsers.find(userId) != m_mapMuteUsers.end())
        return;
    
    if(m_videoFrameOutCb){
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameCallbackGLESForAndroid(userId.c_str(), texture, matrix, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameCallbackGLESForAndroid cost:%lld", t2 - t1);
        }
    }
    if(!m_videoMixer || !m_useGLES)
        return;
#ifdef ANDROID
    ((VideoMixerDroid*)m_videoMixer)->pushVideoBufferGLESForRemoteDroid(userId, fmt, texture, matrix, width, height, timestamp);
#endif
}

void YouMeVideoMixerAdapter::pushVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp){
    AVStatistic::getInstance()->NotifyGetRenderFrame();

    if(m_videoFrameOutCb && m_localVideoRenderEnabled){
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameMixedCallback(data, len, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameMixedCallback cost:%lld", t2 - t1);
        }
    }
}

void YouMeVideoMixerAdapter::pushVideoFrameMixedCallbackkForIOS(void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp){
    AVStatistic::getInstance()->NotifyGetRenderFrame();

    if (m_videoFrameOutCb && m_localVideoRenderEnabled) {
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameMixedCallbackGLESForIOS(pixelBuffer, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameMixedCallbackGLESForIOS cost:%lld", t2 - t1);
        }
    }
}

void YouMeVideoMixerAdapter::pushVideoFrameMixedCallbackkForAndroid(int textureId, float* matrix, int width, int height, int fmt, uint64_t timestamp){
    AVStatistic::getInstance()->NotifyGetRenderFrame();

    if (m_videoFrameOutCb && m_localVideoRenderEnabled) {
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        m_videoFrameOutCb->onVideoFrameMixedCallbackGLESForAndroid(textureId, matrix, width, height, fmt, timestamp);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoFrameMixedCallbackGLESForAndroid cost:%lld", t2 - t1);
        }
    }
}

void YouMeVideoMixerAdapter::pushVideoPreviewFrame(const void *data, int len, int width, int height, uint64_t timestamp, int fmt){
    if(m_videoFramePushCb && m_localVideoSendEnabled){
        m_videoFramePushCb->onPreviewFrame(data, len, width, height, timestamp, fmt);
    }
}

void YouMeVideoMixerAdapter::pushVideoPreviewFrameNew(const void *data, int len, int width, int height, int videoid, uint64_t timestamp, int fmt){
    if(m_videoFramePushCb && m_localVideoSendEnabled){
        m_videoFramePushCb->onPreviewFrameNew(data, len, width, height, videoid, timestamp, fmt);
    }
}

int  YouMeVideoMixerAdapter::pushVideoPreviewFrameGLES(const void *data, int width, int height, int videoid, uint64_t timestamp){
    if(m_videoFramePushCb && m_localVideoSendEnabled){
       return m_videoFramePushCb->onPreviewFrameGLES(data, width, height, videoid, timestamp);
    }
    return 0;
}

static float youme_texmatrix[16] = {1,0, 0, 0,\
    0,  1, 0, 0,\
    0,  0, 1, 0,\
    0,  0, 0, 1};

int  YouMeVideoMixerAdapter::pushVideoPreviewFrameGLESForShare(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp){
    int ret = 0;
    CodecSource source;
    source.fmt = fmt;
    source.textureid = texture;
    if(matrix)
       memcpy(source.matrix, matrix, 16*sizeof(float));
    else
       memcpy(source.matrix, youme_texmatrix, 16*sizeof(float));
    
    pushVideoPreviewFrameGLES(&source, width, height, 2, timestamp);

    return 0;
}

int  YouMeVideoMixerAdapter::pushVideoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror){
    if(m_videoFrameOutCb) {
        XINT64 t1 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        int ret = m_videoFrameOutCb->onVideoRenderFilterCallback(texture, width, height, rotation, mirror);
        XINT64 t2 = youmecommon::CTimeUtil::GetTimeOfDay_MS();
        if (t2 - t1 > CALLBACK_TIME_COST_FLOOR_MS) {
            TSK_DEBUG_WARN("[time warning]: onVideoRenderFilterCallback cost:%lld", t2 - t1);
        }
        return ret;
    }
    return 0;
}

void YouMeVideoMixerAdapter::registerCodecCallback(CameraPreviewCallback *cb)
{
 //   std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
       m_videoMixer->registerCodecCallback(cb);

    YouMeEngineVideoCodec::getInstance()->registerCodecCallback(cb);
    m_videoFramePushCb = cb;
}

void YouMeVideoMixerAdapter::unregisterCodecCallback()
{
 //   std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
        m_videoMixer->unregisterCodecCallback();
    
    YouMeEngineVideoCodec::getInstance()->unregisterCodecCallback();
}

bool YouMeVideoMixerAdapter::isSupportGLES(){
    return m_useGLES;
}

bool YouMeVideoMixerAdapter::useTextureOES(){
    if(!m_videoMixer || !m_useGLES)
        return false;
#ifdef ANDROID
    return ((VideoMixerDroid*)m_videoMixer)->useTextureOES();
#endif
    return false;
}

void YouMeVideoMixerAdapter::setVideoFps(int fps){
    if(fps == 0)
        return;
 //   std::unique_lock<std::mutex> lock(m_mutex);
    if(m_videoMixer)
       m_videoMixer->setVideoFps(fps);
    m_videoFps = fps;

    YouMeEngineManagerForQiniu::getInstance()->setPreviewFps(fps);
}

void YouMeVideoMixerAdapter::openGLES(bool enabled){
    if (enabled && !m_useGLES && !m_videoMixer) {
        initMixerAdapter(enabled);
    }
    else{
#ifdef ANDROID
        if(VideoMixerDroid::m_isSupportGLES && !enabled)
            return;
#endif
         m_useGLES = enabled;
    }
}

int YouMeVideoMixerAdapter::enableLocalVideoRender(bool enabled){
    
    m_localVideoRenderEnabled = enabled;
    return 0;
}

int YouMeVideoMixerAdapter::enableLocalVideoSend(bool enabled){
    
    m_localVideoSendEnabled = enabled;
    return 0;
}

int YouMeVideoMixerAdapter::muteAllRemoteVideoStreams(bool mute){
    
    m_remoteAllVideoRenderEnabled = !mute;
    return 0;
}

int YouMeVideoMixerAdapter::setDefaultMuteAllRemoteVideoStreams(bool mute){
    
    m_remoteDefaultAllVideoRenderEnabled = !mute;
    return 0;
}

int YouMeVideoMixerAdapter::muteRemoteVideoStream(std::string& uid, bool mute){
    if(mute){
       m_mapMuteUsers.insert(std::pair<std::string, bool>(uid, mute));
    }
    else{
        std::map<std::string, bool>::iterator iter = m_mapMuteUsers.find(uid);
        if (iter != m_mapMuteUsers.end()) {
            m_mapMuteUsers.erase(iter);
        }
    }
    return 0;
}

bool YouMeVideoMixerAdapter::setExternalFilterEnabled(bool enabled){
    TSK_DEBUG_INFO("@@ setExternalFilterEnabled enabled:%d\n", enabled);
    if (enabled && !m_useGLES && !m_videoMixer) {
        initMixerAdapter(enabled);
    }
    if(m_videoMixer){
        if(m_videoMixer->setExternalFilterEnabled(enabled)){
             m_filterExtEnabled = enabled;
            return true;
        }
        return false;
    }
    return true;
}



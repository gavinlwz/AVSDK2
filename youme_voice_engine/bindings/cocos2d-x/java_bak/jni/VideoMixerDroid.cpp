//
//  VideoMixerDroid.cpp
//  youme_voice_engine
//
//  Created by bhb on 2018/1/15.
//  Copyright © 2018年 Youme. All rights reserved.
//

#include "VideoMixerDroid.h"
#include "tinymedia/tmedia_defaults.h"
#include "AudioMgr.h"
#include "YouMeVideoMixerAdapter.h"
#include "YouMeEngineVideoCodec.hpp"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../../../../baseWrapper/src/XConfigCWrapper.hpp"
#ifdef __cplusplus
}
#endif

bool VideoMixerDroid::m_isSupportGLES = true;;
VideoMixerDroid::VideoMixerDroid()
:m_cameraPreviewCallback(nullptr)
,m_useTextureOES(false)
{
    JNI_setVideoFrameMixCallback(this);
}

VideoMixerDroid::~VideoMixerDroid(){
     glObject obj = tmedia_get_video_egl_context();
     if(obj){
         JNI_deleteVideoMixEGLContext(obj);
     }
     TSK_DEBUG_INFO("clear EGLContext");
     tmedia_set_video_egl_context(nullptr);
     JNI_setVideoFrameMixCallback(nullptr);
     //JNI_mixExit();
}

VideoMixerDroid*  VideoMixerDroid::createVideoMixer()
{
    return new VideoMixerDroid();
}

bool  VideoMixerDroid::isSupportGLES(){
 
    if(m_isSupportGLES){
        if(!youme::webrtc::H264Encoder::IsSupportedGLES() ||
           !tmedia_defaults_get_video_hwcodec_enabled()){
            m_isSupportGLES = false;
        }
    }
    return m_isSupportGLES;
}

void  VideoMixerDroid::setSupportGLES(bool gl)
{
    m_isSupportGLES = gl;
}

glObject VideoMixerDroid::getVideoMixEGLContext()
{
    return (glObject)JNI_getVideoMixEGLContext();
}

void VideoMixerDroid::initMixer(){
    JNI_initMixer();
    glObject obj = JNI_getVideoMixEGLContext();
    TSK_DEBUG_INFO("EGLContext :%x\n", obj);
    tmedia_set_video_egl_context(obj);
}

void VideoMixerDroid::setMixVideoSize(int width, int height){
    JNI_setVideoMixSize(width, height);
}

void VideoMixerDroid::setVideoNetResolutionWidth(int width, int height){
    JNI_setVideoNetResolutionWidth(width, height);
}

void VideoMixerDroid::setVideoNetResolutionWidthForSecond(int width, int height){
    JNI_setVideoNetResolutionWidthForSecond(width, height);
}

void VideoMixerDroid::addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height){
    JNI_addMixOverlayVideo(userId, x, y, z, width, height);
}

void VideoMixerDroid::removeMixOverlayVideo(std::string userId){
    JNI_removeMixOverlayVideo(userId);
}

void VideoMixerDroid::removeAllOverlayVideo(){
    m_useTextureOES = false;
    JNI_removeAllOverlayVideo();
}

bool VideoMixerDroid::needMixing(std::string userId){
    return true;
}

void VideoMixerDroid::openBeautify(){
    JNI_openBeautify();
}

void VideoMixerDroid::closeBeautify(){
    JNI_closeBeautify();
}

void VideoMixerDroid::setBeautifyLevel(float level){
    JNI_setBeautifyLevel(level);
}

void VideoMixerDroid::setVideoFps(int fps)
{
    JNI_setVideoFps(fps);
}


void VideoMixerDroid::pushVideoBufferGLESForLocalDroid(std::string userId, int fmt, int texture, float* matrix, int width, int height, int rotation, int mirror, uint64_t timestamp){
    if(!m_useTextureOES) m_useTextureOES = true;
    JNI_pushVideoFrameGLESForLocal(userId, fmt, texture, matrix, width, height, rotation, mirror, timestamp);
}

void VideoMixerDroid::pushVideoBufferRawForLocalDroid(std::string userId, int fmt, void* data, int size, int width, int height, int rotation, int mirror, uint64_t timestamp){
    if(m_useTextureOES) return;
    JNI_pushVideoFrameRawForLocal(userId, fmt, data, size, width, height, rotation, mirror, timestamp);
}

void VideoMixerDroid::pushVideoBufferGLESForRemoteDroid(std::string userId, int fmt, int texture, float* matrix, int width, int height, uint64_t timestamp){
    JNI_pushVideoFrameGLESForRemote(userId, fmt, texture, matrix, width, height, timestamp);
}

void VideoMixerDroid::pushVideoBufferRawForRemoteDroid(std::string userId, int fmt, const void* data, int size, int width, int height, uint64_t timestamp)
{
    JNI_pushVideoFrameRawForRemote(userId, fmt, data, size, width, height, timestamp);
}

void VideoMixerDroid::registerCodecCallback(CameraPreviewCallback *cb){
    m_cameraPreviewCallback = cb;
}

void VideoMixerDroid::unregisterCodecCallback(){
    m_cameraPreviewCallback = nullptr;
}

void VideoMixerDroid::videoFrameMixerCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestmap){
    YouMeVideoMixerAdapter::getInstance()->pushVideoFrameMixedCallbackkForAndroid(texture, matrix, width, height, fmt, timestmap);
}

static float youme_texmatrix[16] = {1,0, 0, 0,\
    0,  1, 0, 0,\
    0,  0, 1, 0,\
    0,  0, 0, 1};

void VideoMixerDroid::videoNetFirstCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp){
  
    int ret = 0;
    CodecSource source;
    source.fmt = fmt;
    source.textureid = texture;
    if(matrix)
       memcpy(source.matrix, matrix, 16*sizeof(float));
    else
       memcpy(source.matrix, youme_texmatrix, 16*sizeof(float));
    if(m_cameraPreviewCallback)
       ret = YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLES(&source,width,height,0,timestamp);
    if(ret == 1)//切换到软编码
    {
        if (YouMeVideoMixerAdapter::getInstance()->isSupportGLES()) {
            TSK_DEBUG_INFO("clear EGLContext");
            tmedia_set_video_egl_context(nullptr);
            YouMeVideoMixerAdapter::getInstance()->resetMixer(false);
            m_isSupportGLES = false;
        }
    }
    
}
void VideoMixerDroid::videoNetSecondCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp)
{
    int  ret = 0;
    CodecSource source;
    source.fmt = fmt;
    source.textureid = texture;
    if(matrix)
        memcpy(source.matrix, matrix, 16*sizeof(float));
    else
        memcpy(source.matrix, youme_texmatrix, 16*sizeof(float));
    if(m_cameraPreviewCallback)
        ret = YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLES(&source,width,height,1,timestamp);
    if(ret == 1)//切换到软编码
    {
        if (YouMeVideoMixerAdapter::getInstance()->isSupportGLES()) {
            TSK_DEBUG_INFO("clear EGLContext");
            tmedia_set_video_egl_context(nullptr);
            YouMeVideoMixerAdapter::getInstance()->resetMixer(false);
            m_isSupportGLES = false;
        }
    }
}

void VideoMixerDroid::videoFrameMixerRawCallback(int fmt,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp){
    YouMeVideoMixerAdapter::getInstance()->pushVideoFrameMixedCallback(buffer, bufferLen, width, height, fmt, timestamp);
    
}
void VideoMixerDroid::videoNetFirstRawCallback(int fmt,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp){
    
    FrameImage* frameImage = new FrameImage(width, height, buffer, bufferLen, 0, 0, timestamp);
    YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
    
}
void VideoMixerDroid::videoNetSecondRawCallback(int fmt,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp){
    
    FrameImage* frameImage = new FrameImage(width, height, buffer, bufferLen, 0, 1, timestamp);
    YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
}

int  VideoMixerDroid::videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror){
    
    YouMeVideoMixerAdapter::getInstance()->pushVideoRenderFilterCallback(texture, width, height, rotation, mirror);
}

void VideoMixerDroid::videoMixerUseTextureOES(bool use){
    //m_useTextureOES = use;
    YouMeVideoMixerAdapter::getInstance()->openGLES(use);
    TSK_DEBUG_INFO("@@ videoMixerUseTextureOES use:%d\n", use);
}

bool VideoMixerDroid::useTextureOES(){
    return m_useTextureOES;
}

bool VideoMixerDroid::setOpenMixerRawCallBack(bool enabled){
    TSK_DEBUG_INFO("@@ setOpenMixerRawCallBack enabled:%d\n", enabled);
    return JNI_setOpenMixerRawCallBack(enabled);
}

bool VideoMixerDroid::setOpenEncoderRawCallBack(bool enabled){
    TSK_DEBUG_INFO("@@ setOpenEncoderRawCallBack enabled:%d\n", enabled);
    return JNI_setOpenEncoderRawCallBack(enabled);
}

bool VideoMixerDroid::setExternalFilterEnabled(bool enabled){
    TSK_DEBUG_INFO("@@ setExternalFilterEnabled enabled:%d\n", enabled);
   return JNI_setExternalFilterEnabled(enabled);
}

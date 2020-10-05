//
//  VideoMixerMgr.h
//  youme_voice_engine
//
//  Created by bhb on 2018/1/15.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifndef VideoMixerMgr_h
#define VideoMixerMgr_h
#include <string>
//#include <jni.h>
#include "IYouMeVideoMixer.h"

class IAndroidVideoMixerCallBack{
public:
    virtual void videoFrameMixerCallback(int type,  int texture, float* matrix, int width, int height, uint64_t timestamp) = 0;
    virtual void videoNetFirstCallback(int type,  int texture, float* matrix, int width, int height, uint64_t timestamp) = 0;
    virtual void videoNetSecondCallback(int type,  int texture, float* matrix, int width, int height, uint64_t timestamp) = 0;
   
    virtual void videoFrameMixerRawCallback(int type,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp) = 0;
    virtual void videoNetFirstRawCallback(int type,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp) = 0;
    virtual void videoNetSecondRawCallback(int type,  void* buffer, int bufferLen,  int width, int height, uint64_t timestamp) = 0;

    virtual int  videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror) = 0;
    virtual void videoMixerUseTextureOES(bool use) = 0;
};

#ifdef ANDROID

glObject JNI_getVideoMixEGLContext();

void JNI_deleteVideoMixEGLContext(glObject object);

void JNI_initMixer();

void JNI_setVideoMixEGLContext(glObject object);

void JNI_setVideoMixSize(int width, int height);

void JNI_setVideoNetResolutionWidth(int width, int height);

void JNI_setVideoNetResolutionWidthForSecond(int width, int height);

void JNI_addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height);

void JNI_removeMixOverlayVideo(std::string userId);

void JNI_removeAllOverlayVideo();

void JNI_pushVideoFrameGLESForLocal(std::string userId, int type, int texture, float* matrix, int width, int height, int rotation, int mirror, uint64_t timestamp) ;
void JNI_pushVideoFrameRawForLocal(std::string userId, int type, const void* buff, int buffSize, int width, int height,  int rotation, int mirror, uint64_t timestamp);
void JNI_pushVideoFrameGLESForRemote(std::string userId, int type, int texture, float* matrix, int width, int height, uint64_t timestamp);
void JNI_pushVideoFrameRawForRemote(std::string userId, int type, const void* buff, int buffSize, int width, int height,  uint64_t timestamp);

void JNI_openBeautify() ;

void JNI_closeBeautify();

void JNI_setBeautifyLevel(float Level);

void JNI_onPause();

void JNI_onResume();

void JNI_mixExit();

void JNI_setVideoFrameMixCallback(IAndroidVideoMixerCallBack* cb);

void JNI_setVideoFps(int fps);

bool JNI_setOpenMixerRawCallBack(bool enabled);

bool JNI_setOpenEncoderRawCallBack(bool enabled);

bool JNI_setExternalFilterEnabled(bool enabled);

#endif
#endif /* VideoMixerMgr_h */

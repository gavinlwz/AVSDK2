//
//  VideoMixerDroid.hpp
//  youme_voice_engine
//
//  Created by bhb on 2018/1/15.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifndef VideoMixerDroid_hpp
#define VideoMixerDroid_hpp

#include <stdio.h>
#include "IYouMeVideoMixer.h"
#include "ICameraManager.hpp"
#include "VideoMixerMgr.h"


class VideoMixerDroid: public IYouMeVideoMixer, IAndroidVideoMixerCallBack{
    
public:
    VideoMixerDroid();
    ~VideoMixerDroid();
    
    static  VideoMixerDroid*  createVideoMixer();
    static  bool  isSupportGLES();
    static  void  setSupportGLES(bool gl);
    
    glObject getVideoMixEGLContext();
    
    void initMixer();
    
    void setMixVideoSize(int width, int height);
    
    void setVideoNetResolutionWidth(int width, int height);
    
    void setVideoNetResolutionWidthForSecond(int width, int height);
    
    void addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height);
    
    void removeMixOverlayVideo(std::string userId);
    
    void removeAllOverlayVideo();
    
    bool needMixing(std::string userId);
    
    void openBeautify();
    
    void closeBeautify();
    
    void setBeautifyLevel(float level);
    
    void pushVideoBufferRawForLocalDroid(std::string userId, int fmt, void* data, int size, int width, int height, int rotation, int mirror, uint64_t timestamp);
    
    void pushVideoBufferGLESForLocalDroid(std::string userId, int fmt, int texture, float* matrix, int width, int height, int rotation, int mirror, uint64_t timestamp);
    
    void pushVideoBufferRawForRemoteDroid(std::string userId, int fmt, const void* data, int size, int width, int height, uint64_t timestamp);
    
    void pushVideoBufferGLESForRemoteDroid(std::string userId, int fmt, int texture, float* matrix, int width, int height, uint64_t timestamp);
    
    void registerCodecCallback(CameraPreviewCallback *cb);
    
    void unregisterCodecCallback();
    
    void setVideoFps(int fps);
    
    virtual void videoFrameMixerCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp);
    virtual void videoNetFirstCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp);
    virtual void videoNetSecondCallback(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp);
    
    virtual void videoFrameMixerRawCallback(int fmt,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp);
    virtual void videoNetFirstRawCallback(int fmt,  void* buffer, int bufferLen, int width, int height, uint64_t timestamp);
    virtual void videoNetSecondRawCallback(int fmt,  void* buffer, int bufferLen,  int width, int height, uint64_t timestamp);
    
    virtual int  videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror);

    void videoMixerUseTextureOES(bool use);
    
    bool useTextureOES();
    
    bool setOpenMixerRawCallBack(bool enabled);
    
    bool setOpenEncoderRawCallBack(bool enabled);
    
    virtual bool setExternalFilterEnabled(bool enabled);
public:
    static bool m_isSupportGLES;
private:
    CameraPreviewCallback * m_cameraPreviewCallback;
    bool m_useTextureOES;
    
};

#endif /* VideoMixerDroid_hpp */

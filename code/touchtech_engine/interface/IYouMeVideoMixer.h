//
//  IVideoMixer.h
//  youme_voice_engine
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//
#ifndef videocore_IVideoMixer_hpp
#define videocore_IVideoMixer_hpp
#include "IYouMeVideoCallback.h"
#include "ICameraManager.hpp"

typedef void*   glObject;
/*! IYouMeVideoMixer interface.  Defines the required interface methods for Video mixers (compositors). */

class IYouMeVideoMixer
{
public:
    virtual ~IYouMeVideoMixer() {};
    
    virtual glObject getVideoMixEGLContext(){return NULL;};
    
    virtual void setMixVideoSize(int width, int height) = 0;
    
    virtual void setVideoNetResolutionWidth(int width, int height){}
    
    virtual void setVideoNetResolutionWidthForSecond(int width, int height){}
    
    virtual void addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height) = 0;
    
    virtual void removeMixOverlayVideo(std::string userId) = 0;
    
    virtual void removeAllOverlayVideo() = 0;
    
    virtual bool needMixing(std::string userId) = 0;
    
    virtual void openBeautify(){};
    
    virtual void closeBeautify(){};
    
    virtual void setBeautifyLevel(float level){};
    
    virtual void registerCodecCallback(CameraPreviewCallback *cb){};
    
    virtual void unregisterCodecCallback(){};
    
    virtual void setVideoFps(int fps){};

    virtual bool setExternalFilterEnabled(bool enabled){return false;};
};

#endif

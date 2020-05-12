//
//  YouMeVideoMixerAdapter_h.h
//  youme_voice_engine
//
//  Created by bhb on 2017/12/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef YouMeVideoMixerAdapter_h
#define YouMeVideoMixerAdapter_h

//#define GLES_MIXER 0
#include<mutex>
#include "YouMeEngineManagerForQiniu.hpp"


class YouMeVideoMixerAdapter
{
public:
    YouMeVideoMixerAdapter();
    ~YouMeVideoMixerAdapter();
    
    static YouMeVideoMixerAdapter* getInstance();
    
    void initMixerAdapter(bool isGLES = true);
    
    void releaseMixer();
    
    void resetMixer(bool isGLES = true);
    
    void leavedRoom();
    
    void setMixVideoSize(int width, int height);
    
    void setVideoNetResolutionWidth(int width, int height);
    
    void setVideoNetResolutionWidthForSecond(int width, int height);
    
    void addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height);
    
    void removeMixOverlayVideo(std::string userId);
    
    void removeAllOverlayVideo();
    
    void setVideoFrameCallback(IYouMeVideoFrameCallback* cb);
    
    bool needMixing(std::string userId);
    
    void openBeautify();
    
    void closeBeautify();
    
    void setBeautifyLevel(float level);
    
    //本地输入
    void pushVideoFrameLocal(std::string userId, void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    
    void pushVideoFrameLocalForIOS(std::string userId, void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);
    
    void pushVideoFrameLocalForAndroid(std::string userId, int texture, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);

    //远端回调
    void pushVideoFrameRemote(std::string userId, void* data, int len, int width, int height, int fmt, uint64_t timestamp);
    
    void pushVideoFrameRemoteForIOS(std::string userId, void* pixelbuffer, int width, int height, int fmt, uint64_t timestamp);
    
    void pushVideoFrameRemoteForAndroid(std::string userId, int texture, float* matrix, int width, int height, int fmt, uint64_t timestamp);
    
    //合流回调
    void pushVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp);
    
    void pushVideoFrameMixedCallbackkForIOS(void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp);
    
    void pushVideoFrameMixedCallbackkForAndroid(int texture, float* matrix, int width, int height, int fmt, uint64_t timestamp);

    //网络发送
    void pushVideoPreviewFrame(const void *data, int len, int width, int height, uint64_t timestamp, int fmt);
    
    void pushVideoPreviewFrameNew(const void *data, int len, int width, int height, int videoid, uint64_t timestamp, int fmt);
    
    int  pushVideoPreviewFrameGLES(const void *data, int width, int height, int videoid, uint64_t timestamp);
    // 用于共享视频流发送（外部输入texture）
    int pushVideoPreviewFrameGLESForShare(int fmt,  int texture, float* matrix, int width, int height, uint64_t timestamp);
    
    //自定义滤镜
    int  pushVideoRenderFilterCallback(int textureId, int width, int height, int rotation, int mirror);
    
    void registerCodecCallback(CameraPreviewCallback *cb);
    
    void unregisterCodecCallback();
    
    bool isSupportGLES();
    
    bool useTextureOES();
    
    void setVideoFps(int fps);
    
    void openGLES(bool enabled);
    
    //兼容声网添加
    int enableLocalVideoRender(bool enabled);
    
    int enableLocalVideoSend(bool enabled);
    
    int muteAllRemoteVideoStreams(bool mute);
    
    int setDefaultMuteAllRemoteVideoStreams(bool mute);
    
    int muteRemoteVideoStream(std::string& uid, bool mute);

    //设置外部滤镜回调开关
    bool setExternalFilterEnabled(bool enabled);
private:
    IYouMeVideoMixer* m_videoMixer;
    std::mutex  m_mutex;
    bool m_useGLES;
 
    int m_videoFps;
    int m_videoWidthForFirst;
    int m_videoHeightForFirst;;
    int m_videoWidthForSecond;
    int m_videoHeightForSecond;
    bool m_openBeauty;
    float m_beautyLevel;
    
    IYouMeVideoFrameCallback* m_videoFrameOutCb;
    CameraPreviewCallback* m_videoFramePushCb;
    
    bool m_localVideoRenderEnabled;
    bool m_localVideoSendEnabled;
    bool m_remoteAllVideoRenderEnabled;
    bool m_remoteDefaultAllVideoRenderEnabled;
    
    std::map<std::string, bool> m_mapMuteUsers;

    bool m_filterExtEnabled;
};



#endif /* YouMeVideoMixerAdapter_h */

//
//  CameraManager.h
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/11.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef _CAMERA_MANAGER_LINUX_H_
#define _CAMERA_MANAGER_LINUX_H_

#include "ICameraManager.hpp"


class CameraManager : public ICameraManager
{
private:
    static CameraManager *sInstance;
    CameraPreviewCallback *cameraPreviewCallback;
    IYouMeEventCallback *youmeEventCallback;
    CameraManager();

public:
    static CameraManager *getInstance();
    
public:
    
    YouMeErrorCode registerCameraPreviewCallback(CameraPreviewCallback *cb) override;
    YouMeErrorCode unregisterCameraPreviewCallback();
    YouMeErrorCode registerYoumeEventCallback(IYouMeEventCallback *cb) override;
    YouMeErrorCode unregisterYoumeEventCallback() override;
    YouMeErrorCode startCapture() override;
    YouMeErrorCode stopCapture() override;
    YouMeErrorCode setCaptureProperty(float fFps, int nWidth, int nHeight) override;
    YouMeErrorCode setCaptureFrontCameraEnable(bool enable) override;
    bool isCaptureFrontCameraEnable() override;
    YouMeErrorCode switchCamera() override;
    
    void openBeautify(bool open) override;
    void beautifyChanged(float param) override;
    void stretchFace(bool stretch) override;
    void setLocalVideoMirrorMode(YouMeVideoMirrorMode_t mirrorMode) override;
    
    YouMeErrorCode videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID = 0);
    //YouMeErrorCode videoDataOutputGLESForAndroid(int texture, float* matrix,int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp);

private:
    // float _beautyLevel = 0.0f;
    // bool  _closeing = false;
    // bool  openFailed = false;
};

#endif

//
//  CameraManager.h
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/11.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef _CAMERA_MANAGER_H_
#define _CAMERA_MANAGER_H_

#import "YouMeConstDefine.h"
#import "IYouMeVideoCallback.h"
#import "ICameraManager.hpp"

class CameraManager : public ICameraManager{
private:
    static CameraManager *sInstance;
    //static CameraController *cameraController;
    CameraPreviewCallback *cameraPreviewCallback;
    IYouMeEventCallback *youmeEventCallback;
public:
    static CameraManager *getInstance ();
    CameraManager();
    ~CameraManager();
    YouMeErrorCode registerCameraPreviewCallback(CameraPreviewCallback *cb) override;
    YouMeErrorCode unregisterCameraPreviewCallback() override;
    YouMeErrorCode registerYoumeEventCallback(IYouMeEventCallback *cb) override;
    YouMeErrorCode unregisterYoumeEventCallback() override;
    YouMeErrorCode startCapture() override;
    YouMeErrorCode stopCapture() override;
    YouMeErrorCode setCaptureProperty(float fFps, int nWidth, int nHeight) override;
    YouMeErrorCode setCaptureFrontCameraEnable(bool enable) override;
    bool isCaptureFrontCameraEnable() override;
    void setCameraOpenStatus(bool status);
    YouMeErrorCode switchCamera() override;
    
    #if TARGET_OS_OSX
    int getCameraCount() override;
    std::string getCameraName(int cameraId) override;
    YouMeErrorCode setOpenCameraId(int cameraId) override;
    #endif
    
    void openBeautify(bool open) override;
    void beautifyChanged(float param) override;
    void stretchFace(bool stretch) override;
    void callbackEvent(const YouMeEvent event, const YouMeErrorCode error, const char * room, const char * param);
    YouMeErrorCode videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID = 0) ;
    YouMeErrorCode videoDataOutputGLESForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag) ;
    
#if TARGET_OS_IPHONE
    bool isCameraZoomSupported() override ;
    float setCameraZoomFactor(float zoomFactor) override ;
    bool isCameraFocusPositionInPreviewSupported() override ;
    bool setCameraFocusPositionInPreview( float x , float y ) override ;
    bool isCameraTorchSupported() override ;
    bool setCameraTorchOn( bool isOn ) override ;
    bool isCameraAutoFocusFaceModeSupported() override ;
    bool setCameraAutoFocusFaceModeEnabled( bool enable ) override ;
    void decreaseBufferRef(void *pixelbuffer) override;
    void lockBufferRef(void * pixelbuffer) override;
    void unlockBufferRef(void *pixelbuffer) override;

    void setCacheLastPixelEnable(bool enable);
    void *copyPixelbuffer(void* pixelbuffer) override;
    void *getLastPixelRef() override;
    bool isUnderAppExtension () override;
    int64_t usedMemory() override;
#endif
    
    void *videoRotate(void* pixelbuffer, int rotateType);
    

    virtual void setShareVideoEncResolution (int width, int height);
    virtual void setLocalVideoMirrorMode( YouMeVideoMirrorMode  mirrorMode );
    virtual bool checkScreenCapturePermission();
    
private:
    uint8_t* _videoData;
    uint32_t _videoDataSize;
    bool _opening;
    bool _closeing;
    bool _underAppExtension;

    int _encWidth;
    int _encHeight;

    bool _isCacheLastPixel;
    void* _lastPixelRef;
};

#endif

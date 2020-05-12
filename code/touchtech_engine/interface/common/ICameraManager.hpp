//
//  CameraManager.hpp
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/20.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef CameraManager_hpp
#define CameraManager_hpp

#include <stdio.h>
#include <mutex>
#include <memory>
#include "tsk.h"
#include "YouMeConstDefine.h"
#include "IYouMeEventCallback.h"
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>

//typedef enum VIDEO_FMT_E
//{
//    VIDEO_I420         = 0x0,
//    // ANDROID part
//    ANDROID_VIDEO_NV21 = 0x11,
//    ANDROID_VIDEO_YV12 = 0x32315659,
//    // IOS part
//    IOS_VIDEO_NV12   = 0x2,
//    IOS_VIDEO_32BGRA = 0x3,
//    VIDEO_RGB24 = 0x4
//} VIDEO_FMT_T;



class FrameImage
{
private:
    bool need_delete_data=false;
    int iSlot = -1;
public:
    int width;
    int height;
    int mirror;
    int videoid;
    void* data;
    uint64_t timestamp;
    bool double_stream = false;
    int len;
    int fmt;
    int rotate;

public:
    FrameImage(int width, int height, void* data);
    FrameImage(int width, int height, void* data, int len, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P);
    FrameImage(int width, int height, void* data, int len, int mirror, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P);
    FrameImage(int width, int height, void* data, int len, int mirror, int videoid, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P);
    FrameImage(int width, int height);
    ~FrameImage();
};

class CameraPreviewCallback
{
public:
    virtual void onPreviewFrame(const void *data, int size, int width, int height, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P) = 0;
    virtual void onPreviewFrameNew(const void *data, int size, int width, int height, int videoid, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P) = 0;
    virtual int  onPreviewFrameGLES(const void *data, int width, int height, int videoid, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P) = 0;
};

class ICameraManager {
private:
    uint64_t duration;
    uint64_t start_timestamp;
    
protected:
    bool open;
    std::recursive_mutex* mutex;
    
private:
    static ICameraManager *sInstance;
    //CameraPreviewCallback *cameraPreviewCallback;
public:
    static ICameraManager *getInstance();
    
    // A virtual destructor to make sure the correct destructor of the derived classes will called.
    ICameraManager();
    virtual ~ICameraManager();
    virtual YouMeErrorCode registerCameraPreviewCallback(CameraPreviewCallback *cb) = 0;
    virtual YouMeErrorCode unregisterCameraPreviewCallback() = 0;
    virtual YouMeErrorCode registerYoumeEventCallback(IYouMeEventCallback *cb) = 0;
    virtual YouMeErrorCode unregisterYoumeEventCallback() = 0;
    virtual YouMeErrorCode startCapture();
    virtual YouMeErrorCode stopCapture();
    virtual YouMeErrorCode setCaptureProperty(float fFps, int nWidth, int nHeight) = 0;
    virtual YouMeErrorCode setCaptureFrontCameraEnable(bool enable) = 0;
    virtual bool isCaptureFrontCameraEnable() = 0;
    virtual YouMeErrorCode switchCamera() = 0;
    
    #if defined(WIN32) || TARGET_OS_OSX
    virtual int getCameraCount() = 0;
    virtual std::string getCameraName(int cameraId) = 0;
    virtual YouMeErrorCode setOpenCameraId(int cameraId) = 0;
    #endif
    
    virtual YouMeErrorCode videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID = 0);
    
    virtual void openBeautify(bool open)= 0;
    virtual void beautifyChanged(float param)= 0;
    virtual void stretchFace(bool stretch)= 0;
    
    #if ANDROID
    virtual YouMeErrorCode videoDataOutputGLESForAndroid(int texture, float* matrix,int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag);
    virtual YouMeErrorCode resetCamera() = 0;
    #endif
    
    #if TARGET_OS_IPHONE || ANDROID
    virtual bool isCameraZoomSupported() = 0 ;
    virtual float setCameraZoomFactor(float zoomFactor) = 0 ;
    virtual bool isCameraFocusPositionInPreviewSupported() = 0 ;
    virtual bool setCameraFocusPositionInPreview( float x , float y ) = 0 ;
    virtual bool isCameraTorchSupported() = 0 ;
    virtual bool setCameraTorchOn( bool isOn ) = 0 ;
    virtual bool isCameraAutoFocusFaceModeSupported() = 0 ;
    virtual bool setCameraAutoFocusFaceModeEnabled( bool enable ) = 0 ;
    #endif

    virtual void setLocalVideoMirrorMode( YouMeVideoMirrorMode  mirrorMode ) = 0 ;

    #if TARGET_OS_IPHONE || TARGET_OS_OSX
    virtual void setShareVideoEncResolution(int width, int height) = 0;
	virtual bool checkScreenCapturePermission() = 0;
    #endif
	
    virtual YouMeErrorCode videoDataOutputGLESForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag);
    
    #if TARGET_OS_IPHONE
    virtual void decreaseBufferRef(void *buffer) = 0;
    virtual void lockBufferRef(void *buffer) = 0;
    virtual void unlockBufferRef(void *buffer) = 0;
    virtual void *copyPixelbuffer(void* pixelbuffer) = 0;
    #endif
	
    virtual uint64_t getDuration();
    virtual uint64_t resetDuration();
    static int add(int a, int b);
    std::shared_ptr<FrameImage> scale(std::shared_ptr<FrameImage> src, int out_width, int out_height, int mode);
    void scale(uint8_t* src_data,
                               int src_width,
                               int src_height,
                               uint8_t* out_data,
                               int out_width,
                               int out_height,
                               int mode);
    std::shared_ptr<FrameImage> crop(std::shared_ptr<FrameImage> src, int width, int height);
    
    void mirror(uint8_t* src_data,
                int src_width,
                int src_height);
    int scale(FrameImage* frameImage, int out_width ,int out_height, int mode);
    
public:
    void rotation_and_mirror(FrameImage* frameImage, int degree, bool need_mirror);
    int format_transfer(FrameImage* frameImage, int videoFmt);
    int transfer_yv12_to_yuv420(FrameImage* frameImage);
    int transfer_nv21_to_yuv420(FrameImage* frameImage);
    int transfer_nv12_to_yuv420(FrameImage* frameImage);
    int transfer_yuv_to_yuv420(FrameImage* frameImage);
    int transfer_32bgra_to_yuv420(FrameImage* frameImage);
    int transfer_32rgba_to_yuv420(FrameImage* frameImage);
    int transfer_32abgr_to_yuv420(FrameImage* frameImage);
	int transfer_rgb24_to_yuv420(FrameImage* frameImage);
    void transpose(FrameImage* frameImage);
    void mirror(FrameImage* frameImage);
    
    std::shared_ptr<FrameImage> video_scale_yuv_zoom(std::shared_ptr<FrameImage> src, int out_width ,int out_height);
};


#endif /* CameraManager_hpp */

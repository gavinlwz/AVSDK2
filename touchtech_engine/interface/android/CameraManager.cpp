//
//  CameraManager.m
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/11.
//  Copyright © 2017年 Youme. All rights reserved.
//


#include "AudioMgr.h"
#include "CameraManager.h"
#include "YouMeEngineManagerForQiniu.hpp"
#include "tmedia_utils.h"
#include "NgnTalkManager.hpp"
#include "YouMeEngineVideoCodec.hpp"
#include "YouMeVideoMixerAdapter.h"

#define CLAMP(i)   (((i) < 0) ? 0 : (((i) > 255) ? 255 : (i)))


CameraManager *CameraManager::sInstance = NULL;
CameraManager *CameraManager::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new CameraManager ();
    }
    return sInstance;
}

CameraManager::CameraManager()
{
    this->cameraPreviewCallback = NULL;
    this->enableFrontCamera = true;
}

YouMeErrorCode CameraManager::registerCameraPreviewCallback(CameraPreviewCallback *cb)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    this->cameraPreviewCallback = cb;
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::unregisterCameraPreviewCallback()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    this->cameraPreviewCallback = NULL;
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::registerYoumeEventCallback(IYouMeEventCallback *cb)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    this->youmeEventCallback = cb;
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::unregisterYoumeEventCallback()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    this->youmeEventCallback = NULL;
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::startCapture()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    YouMeErrorCode ret = YOUME_SUCCESS;
    if(this->open) {
        TSK_DEBUG_WARN("unexpected repeat calling");
        return ret;
    }
    set_capture_frontCameraEnable(enableFrontCamera);
    if(start_capture() != 0){
        ret = YOUME_ERROR_CAMERA_OPEN_FAILED;
        openFailed = true;
    }else{
        ICameraManager::startCapture();
        this->open = true;
    }
    return ret;
}

YouMeErrorCode CameraManager::stopCapture()
{
    _closeing  = true;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    YouMeErrorCode ret = YOUME_SUCCESS;
    if(!this->open) {
        TSK_DEBUG_WARN("unexpected repeat calling");
        _closeing = false;
        return ret;
    }
    ICameraManager::stopCapture();
    if(stop_capture()) {
        ret = YOUME_ERROR_CAMERA_EXCEPTION;
    }
    this->open = false;
    _closeing = false;
    return ret;
}

YouMeErrorCode CameraManager::setCaptureProperty(float fFps, int nWidth, int nHeight)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    YouMeErrorCode ret = YOUME_SUCCESS;
    set_capture_property((int)fFps, nWidth, nHeight);
    return ret;
}

YouMeErrorCode CameraManager::setCaptureFrontCameraEnable(bool enable)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    enableFrontCamera = enable;
    set_capture_frontCameraEnable(enable);
    return YOUME_SUCCESS;
}

bool CameraManager::isCaptureFrontCameraEnable() {
    return enableFrontCamera;
}

YouMeErrorCode CameraManager::switchCamera()
{
    YouMeErrorCode ret = YOUME_SUCCESS;
    _closeing = true;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->open) {
        if(switch_camera() != 0){
            ret = YOUME_ERROR_CAMERA_OPEN_FAILED;
        }else{
            enableFrontCamera = !enableFrontCamera;
        }
    }
    _closeing = false;
    return ret;
}

YouMeErrorCode CameraManager::resetCamera()
{
    YouMeErrorCode ret = YOUME_ERROR_CAMERA_OPEN_FAILED;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->openFailed) {
        this->openFailed = (start_capture() != 0);
        if (!this->openFailed) {
            this->open = true;
            ret = YOUME_SUCCESS;
        }
    }
    
    return ret;

}

YouMeErrorCode CameraManager::videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID)
{
    if(_closeing)
        return YOUME_SUCCESS;
    if(!data)
        return YOUME_ERROR_INVALID_PARAM;

    // render flag表示是否需要渲染，在local端主要是用作区分camera和share流
    // 但是考虑到外部应用场景（例如投屏），外部编码输入H264
    // 同时还有一个考虑就是只发送share流时，服务端不统计半程丢包率
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    // renderflag:是否渲染，这里用作区分摄像头视频和共享视频输入
    // 这里有点需要注意的是，外部输入视频走的是摄像头视频分支
    if (renderflag) {
        if(YouMeVideoMixerAdapter::getInstance()->isSupportGLES() &&
        !YouMeVideoMixerAdapter::getInstance()->useTextureOES() &&
        fmt != VIDEO_FMT_H264 && fmt != VIDEO_FMT_ENCRYPT){
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, data, len, width, height, fmt, rotation, mirror, timestamp);
        }
        else if(!YouMeVideoMixerAdapter::getInstance()->isSupportGLES() && fmt != VIDEO_FMT_H264 && fmt != VIDEO_FMT_ENCRYPT){
            FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp);
            len = ICameraManager::format_transfer(frameImage, fmt);
            if(rotation)
                ICameraManager::rotation_and_mirror(frameImage, rotation, tsk_false);
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, frameImage->data, len, frameImage->width, frameImage->height, VIDEO_FMT_YUV420P, 0, mirror, timestamp);
            YouMeEngineVideoCodec::getInstance()->pushFrame(frameImage, true);
        } else {

            FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp, fmt);
            frameImage->videoid = 0;
            frameImage->double_stream = false; 
            
            // 针对外部输入H264或加密视频数据，外部可设置videoID
            if (VIDEO_FMT_ENCRYPT == fmt || VIDEO_FMT_H264 == fmt) {
                frameImage->videoid = streamID;
                frameImage->double_stream = true;   // 带上videoID
            }

            YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
        }
    }else {
        FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp, fmt);
        len = ICameraManager::format_transfer(frameImage, fmt);
        if (fmt != VIDEO_FMT_H264 && fmt != VIDEO_FMT_ENCRYPT) {
            frameImage->len = len;
            frameImage->fmt = VIDEO_FMT_YUV420P;
        }
        
        if(rotation)
            ICameraManager::rotation_and_mirror(frameImage, rotation, tsk_false);

        frameImage->videoid = 2;    // share video stream type
        frameImage->double_stream = true;

        YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
    }
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::videoDataOutputGLESForAndroid(int texture, float* matrix,int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag){
    if(_closeing)
        return YOUME_SUCCESS;

    if (YouMeVideoMixerAdapter::getInstance()->isSupportGLES()) {
        if (1 == renderflag) {
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocalForAndroid(CNgnTalkManager::getInstance()->m_strUserID, \
                                                                        texture,
                                                                        matrix,
                                                                        width,
                                                                        height,
                                                                        fmt,
                                                                        rotation,
                                                                        mirror,
                                                                        timestamp);
        } else {
            // for screen share stream，no need preview, videoid = 2
            YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLESForShare(fmt,
                                                                        texture,
                                                                        matrix,
                                                                        width,
                                                                        height,
                                                                        timestamp);
        }

        
        return YOUME_SUCCESS;
    }
    
    return YOUME_ERROR_UNKNOWN;
    
}

void CameraManager::openBeautify(bool open)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    
    open_beauty( open );
    
    if(open){
        set_video_beauty_level(_beautyLevel);
    }
    else{
        set_video_beauty_level(0.0f);
    }
}
void CameraManager::beautifyChanged(float param)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    YouMeErrorCode ret = YOUME_SUCCESS;

    _beautyLevel = param;
    set_video_beauty_level(param);
}
void CameraManager::stretchFace(bool stretch)
{
    
}

bool CameraManager::isCameraZoomSupported(){
    return JNI_isCameraZoomSupported();
}
float CameraManager::setCameraZoomFactor(float zoomFactor){
    return JNI_setCameraZoomFactor(zoomFactor);
}
bool CameraManager::isCameraFocusPositionInPreviewSupported(){
    return JNI_isCameraFocusPositionInPreviewSupported();
}
bool CameraManager::setCameraFocusPositionInPreview( float x , float y ){
    return JNI_setCameraFocusPositionInPreview(x, y);
}
bool CameraManager::isCameraTorchSupported(){
    return JNI_isCameraTorchSupported();
}
bool CameraManager::setCameraTorchOn( bool isOn ){
    return JNI_setCameraTorchOn(isOn);
}
bool CameraManager::isCameraAutoFocusFaceModeSupported(){
    return JNI_isCameraAutoFocusFaceModeSupported();
}
bool CameraManager::setCameraAutoFocusFaceModeEnabled( bool enable ){
    return JNI_setCameraAutoFocusFaceModeEnabled(enable);
}

void CameraManager::setLocalVideoMirrorMode(YouMeVideoMirrorMode_t mirrorMode)
{
    JNI_setLocalVideoMirrorMode(mirrorMode);
}


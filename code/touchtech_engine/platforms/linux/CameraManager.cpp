//
//  CameraManager.m
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/11.
//  Copyright © 2017年 Youme. All rights reserved.
//


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
}


YouMeErrorCode CameraManager::registerCameraPreviewCallback(CameraPreviewCallback *cb)
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::unregisterCameraPreviewCallback()
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::registerYoumeEventCallback(IYouMeEventCallback *cb)
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::unregisterYoumeEventCallback()
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::startCapture()
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::stopCapture()
{

    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::setCaptureProperty(float fFps, int nWidth, int nHeight)
{
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::setCaptureFrontCameraEnable(bool enable)
{
    return YOUME_SUCCESS;
}

bool CameraManager::isCaptureFrontCameraEnable() {

    return true;
}

YouMeErrorCode CameraManager::switchCamera()
{

    return YOUME_SUCCESS;   
}

#if 0
YouMeErrorCode CameraManager::resetCamera()
{

    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->openFailed) {
        this->openFailed = (start_capture() != 0);
    }

    return YOUME_SUCCESS;

}
#endif

YouMeErrorCode CameraManager::videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID){
    // TSK_DEBUG_INFO ("@@ CameraManager::videoDataOutput fmt:%d ts:%u", fmt, timestamp);
    if (!data)
        return YOUME_ERROR_INVALID_PARAM;

    // if (!this->open){
    //     return YOUME_ERROR_WRONG_STATE;
    // }

    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp, fmt);

    // h.264 frame no need transfer and rotate
    if (fmt != VIDEO_FMT_H264) {
        len = ICameraManager::format_transfer(frameImage, fmt);
        if(rotation)
            ICameraManager::rotation_and_mirror(frameImage, rotation, tsk_false);
    }

    // no render for linux version so far
    /* if (renderflag) {
        YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, frameImage->data, len, frameImage->width, frameImage->height, fmt, rotation, mirror, timestamp);
        YouMeEngineVideoCodec::getInstance()->pushFrame(frameImage, true);
    } else */
    {
        frameImage->videoid = 0;
        frameImage->double_stream = false; 
        YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
    }
    
    return YOUME_SUCCESS;
}

void CameraManager::openBeautify(bool open)
{
    return;
}
void CameraManager::beautifyChanged(float param)
{
    return;
}
void CameraManager::stretchFace(bool stretch)
{
   return; 
}

void CameraManager::setLocalVideoMirrorMode(YouMeVideoMirrorMode_t mirrorMode)
{
    return;
}

#if 0
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
#endif

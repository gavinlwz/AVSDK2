//
//  CameraManager.cpp
//  youme_voice_engine
//
//  Created by bhb on 2017/10/18.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "CameraManager.h"
#include "../interface/IYouMeVoiceEngine.h"
#include "NgnTalkManager.hpp"
#include "../interface/YouMeEngineManagerForQiniu.hpp"
#include "YouMeEngineVideoCodec.hpp"
#include "YouMeVideoMixerAdapter.h"
#include "tinymedia/tmedia_defaults.h"

#define CLAMP(i)   (((i) < 0) ? 0 : (((i) > 255) ? 255 : (i)))

static void AMSampleGrabberCallBack(BYTE * pBuffer, long lBufferSize, int width, int height, int fps, double dblSampleTime, void* context)
{
    CameraManager * pThis = (CameraManager*)context;
    if (pThis)
    {
		int fmt = pThis->m_samplingMode ? VIDEO_FMT_YUV420P : VIDEO_FMT_RGB24;
		int rotation = pThis->m_samplingMode ? 0 : 180;
		int mirror = pThis->m_samplingMode ? YOUME_VIDEO_MIRROR_MODE_NEAR : YOUME_VIDEO_MIRROR_MODE_FAR;

        if (height < 0) {
            height = abs(height);
            rotation += 180;
        }
        width = abs(width);
        
        if (YOUME_VIDEO_MIRROR_MODE_FAR == CameraManager::getInstance()->m_mirrorMode) {
            if (YOUME_VIDEO_MIRROR_MODE_NEAR == mirror) {
                mirror = YOUME_VIDEO_MIRROR_MODE_DISABLED;
            } else {
                mirror = YOUME_VIDEO_MIRROR_MODE_ENABLED;
            }
        }

		uint64_t pts = tsk_gettimeofday_ms();
		pThis->videoDataOutput(pBuffer, lBufferSize, width, height, fmt, rotation, mirror, pts, 2);
    }
}

CameraManager *CameraManager::getInstance()
{
    static CameraManager* sInstance = NULL;
    if (NULL == sInstance)
    {
        sInstance = new CameraManager ();
    }
    return sInstance;
}

CameraManager::CameraManager()
{
    m_width = 640;
    m_height = 480;
    m_fps = 15;
    m_cameraId = 0;
    m_cameraCount = -1;
    m_samplingMode = 0;
    this->cameraPreviewCallback = NULL;
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
    if(this->open) {
        TSK_DEBUG_WARN("unexpected repeat calling");
        return YOUME_SUCCESS;
    }
	std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    m_amcapdshow.SetCallBack(AMSampleGrabberCallBack, this);
    m_samplingMode = tmedia_defaults_get_video_sampling_mode();
    if (m_amcapdshow.OpenCamera(m_cameraId, m_samplingMode, false, m_width, m_height, m_fps)){
        ICameraManager::startCapture();
        this->open = true;
        return YOUME_SUCCESS;
    }
    else  {
        return YOUME_ERROR_START_FAILED;
    }
    return YOUME_ERROR_START_FAILED;
}

YouMeErrorCode CameraManager::stopCapture()
{
    if(!this->open) {
        TSK_DEBUG_WARN("unexpected repeat calling");
        return YOUME_SUCCESS;
    }
	this->open = false;
	std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    m_amcapdshow.CloseCamera();
    ICameraManager::stopCapture();
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::setCaptureProperty(float fFps, int nWidth, int nHeight)
{
    m_width = nWidth;
    m_height = nHeight;
    m_fps = fFps;

    TSK_DEBUG_INFO("setCaptureProperty mfps:%d, fps:%f, w:%d, h:%d", m_fps, fFps, nWidth, nHeight);
    //std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    //[[CameraController getInstance]setCapturePropertyWithFps:fFps Width:nWidth Height:nHeight];
	//YouMeEngineManagerForQiniu::getInstance()->setMixVideoSize(nWidth, nHeight);
    return YOUME_SUCCESS;
}

YouMeErrorCode CameraManager::setCaptureFrontCameraEnable(bool enable)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    //[[CameraController getInstance] setCaptureFrontCameraEnable:enable];
    return YOUME_SUCCESS;
}

bool CameraManager::isCaptureFrontCameraEnable()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    //return [[CameraController getInstance] isCaptureFrontCameraEnable];
    return false;
}

void CameraManager::setLocalVideoMirrorMode(YouMeVideoMirrorMode_t mirrorMode)
{
    m_mirrorMode = mirrorMode;
}

YouMeErrorCode CameraManager::switchCamera()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    
    return YOUME_ERROR_UNKNOWN;
}

YouMeErrorCode CameraManager::videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int stream){
	if (!data)
        return YOUME_ERROR_INVALID_PARAM;

	if (!this->open && renderflag == 2) {
		//如果摄像头已经进入关闭状态，并且是内置摄像头的输入，就停止输入
		return YOUME_SUCCESS;
	}

    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp, fmt);
    len = ICameraManager::format_transfer(frameImage, fmt);
    if (fmt != VIDEO_FMT_H264) {
        frameImage->len = len;
        frameImage->fmt = VIDEO_FMT_YUV420P;

        if(rotation)
            ICameraManager::rotation_and_mirror(frameImage, rotation, tsk_false);
    }

    if (renderflag) {
        if (fmt != VIDEO_FMT_H264) {
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, frameImage->data, len, frameImage->width, frameImage->height, fmt, rotation, mirror, timestamp);
        }
        
        YouMeEngineVideoCodec::getInstance()->pushFrame(frameImage, true);
    } else {
        frameImage->videoid = 2;    // share video stream type
        frameImage->double_stream = true; 
        YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
    }
    
    
    return YOUME_SUCCESS;
}

int  CameraManager::getCameraCount()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    m_cameraCount = CCameraCaptureDShow::CameraCount();
    return m_cameraCount;
}
std::string CameraManager::getCameraName(int cameraId)
{
    return CCameraCaptureDShow::CameraName(cameraId);
}
YouMeErrorCode CameraManager::setOpenCameraId(int cameraId)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if (m_cameraCount == -1) {
        m_cameraCount = CCameraCaptureDShow::CameraCount();
    }
    if(cameraId >= m_cameraCount) 
    {
        return YOUME_ERROR_INVALID_PARAM;
    }
    m_cameraId = cameraId;
    return YOUME_SUCCESS;
}

//
//  CameraManager.hpp
//  youme_voice_engine
//
//  Created by bhb on 2017/10/18.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef CameraManager_Win_hpp
#define CameraManager_Win_hpp

#include "ICameraManager.hpp"
#include "CameraCaptureDShow.h"

class CameraManager : public ICameraManager
{
private:
    CameraPreviewCallback *cameraPreviewCallback;
    IYouMeEventCallback *youmeEventCallback;
    CameraManager();
    bool enableFrontCamera = true;
	CCameraCaptureDShow m_amcapdshow;
	int m_width, m_height, m_fps;
    int m_cameraId;
	int m_cameraCount;
    
public:
    int m_samplingMode;
    YouMeVideoMirrorMode_t m_mirrorMode = YOUME_VIDEO_MIRROR_MODE_AUTO;


public:
    static CameraManager *getInstance();
    
public:
	YouMeErrorCode registerCameraPreviewCallback(CameraPreviewCallback *cb) override;
	YouMeErrorCode unregisterCameraPreviewCallback() override;
    YouMeErrorCode registerYoumeEventCallback(IYouMeEventCallback *cb) override;
    YouMeErrorCode unregisterYoumeEventCallback() override;
	YouMeErrorCode startCapture() override;
	YouMeErrorCode stopCapture() override;
	YouMeErrorCode setCaptureProperty(float fFps, int nWidth, int nHeight) override;
	YouMeErrorCode setCaptureFrontCameraEnable(bool enable) override;
	bool isCaptureFrontCameraEnable() override;
	YouMeErrorCode switchCamera() override;
    void setLocalVideoMirrorMode(YouMeVideoMirrorMode_t mirrorMode) override;

	//IYouMeVideoCallback *getVideoCallback() override;
	YouMeErrorCode videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID = 0);
      
	virtual void openBeautify(bool open){};
    virtual void beautifyChanged(float param){};
    virtual void stretchFace(bool stretch){};
    
    int  getCameraCount() override;
    std::string getCameraName(int cameraId) override;
    YouMeErrorCode setOpenCameraId(int cameraId) override;
};

#endif /* CameraManager_Win_hpp */

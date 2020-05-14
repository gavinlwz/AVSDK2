//
//  AudioMgr.h
//  youme_voice_engine
//
//  Created by wanglei on 15/12/29.
//  Copyright © 2015年 youme. All rights reserved.
//

#ifndef AudioMgr_h
#define AudioMgr_h
#ifdef ANDROID

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

void start_voice ();
void stop_voice ();
void init_audio_settings(bool outputToSpeaker);
bool JNI_startRequestPermissionForApi23();
void JNI_stopRequestPermissionForApi23();
bool JNI_startRequestPermissionForApi23_camera();
void JNI_stopRequestPermissionForApi23_camera();
void JNI_Set_Mic_isMute(int isMute);
int  start_capture();
int  stop_capture();
void set_capture_property(int nFps, int nWidth, int nHeight);
void set_capture_frontCameraEnable(bool enable);
int  switch_camera();
void open_beauty( bool open );
void set_video_beauty_level(float level);
void JNI_onAudioFrameCallbackID(std::string userId, const void * data, int len, uint64_t timestamp);
void JNI_onAudioFrameMixedCallbackID(const void * data, int len, uint64_t timestamp);
    
bool JNI_isCameraZoomSupported();
float JNI_setCameraZoomFactor(float zoomFactor);
bool JNI_isCameraFocusPositionInPreviewSupported();
bool JNI_setCameraFocusPositionInPreview(float x , float y );
bool JNI_isCameraTorchSupported();
bool JNI_setCameraTorchOn(bool isOn );
bool JNI_isCameraAutoFocusFaceModeSupported();
bool JNI_setCameraAutoFocusFaceModeEnabled(bool enable );
void JNI_screenRecorderSetResolution(int width, int height);
void JNI_screenRecorderSetFps(int fps);
void JNI_setLocalVideoMirrorMode(int mirrorMode);

#ifdef __cplusplus
}
#endif
#endif
#endif

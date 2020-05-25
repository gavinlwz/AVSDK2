//
//  CameraController.h
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/12.
//  Copyright © 2017年 Youme. All rights reserved.
//


#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#ifdef MAC_OS
    //#import <UIKit/UIDevice.h>
    #import <AppKit/NSApplication.h>
#else
    #import <UIKit/UIDevice.h>
    #import <UIKit/UIApplication.h>
#endif
#include "YouMeConstDefine.h"
#include <atomic>

//#if TARGET_OS_IPHONE
//#if TARGET_IPHONE_SIMULATOR
//
//#else
//
//#define OPEN_BEAUTIFY
//
//#endif
//#endif


#ifdef OPEN_BEAUTIFY
//#import <MuseProcessor/MuseProcessor.h>
#endif

@interface CameraController : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>
{
    Float32 fFps;
    int nWidth;
    int nHeight;
    BOOL needLoseFrame;
    long totalTime;
    long frameCount;
    YouMeVideoMirrorMode_t mirrorMode;
    BOOL enableFrontCamera;
    int macCameraDeviceCount;
    int macCameraDeviceId;
    
    uint8_t * _brgaBuffer;
    std::atomic<bool> _abortCameraRunning;
#ifdef OPEN_BEAUTIFY
    CVPixelBufferRef    _resultBufferNormal;
#endif
}

@property (nonatomic,  retain) AVCaptureSession *avCaptureSession;
@property (nonatomic,  retain) AVCaptureDevice *avCaptureDevice;
@property (nonatomic,  retain) AVCaptureDeviceInput *avCaptureDeviceInput;
@property (nonatomic,  retain) AVCaptureVideoDataOutput *avCaptureVideoDataOutput;
#if OS_OBJECT_USE_OBJC
@property (nonatomic, strong) dispatch_queue_t videoDataQueue;
#else
@property (nonatomic, assign) dispatch_queue_t videoDataQueue;
#endif

@property (nonatomic,  retain) AVCaptureConnection *avCaptureConnection;
@property (atomic,  assign) int screenOrientation;
@property (atomic,  assign) AVCaptureVideoOrientation videoOrientation;

#ifdef OPEN_BEAUTIFY
//@property (nonatomic, strong)       MuseRealTimeProcessor       *processor;
@property (nonatomic, assign)       bool   openBeautify;
#endif

@property (nonatomic, assign)       bool  torchOn;
@property (nonatomic, assign)       bool  autoFoucs;

+ (CameraController *)getInstance;
-(void)test;
- (instancetype)init;
-(void)setCapturePropertyWithFps:(float)fps Width:(int)width Height:(int)height;
-(void)setCaptureFrontCameraEnable:(BOOL)enable;
-(BOOL)isCaptureFrontCameraEnable;
-(BOOL)startCapture;
-(BOOL)stopCapture;
#if TARGET_OS_IPHONE
-(BOOL)checkCameraAuthorization;
#endif
-(BOOL)selectiOSCameraDevice:(BOOL)isFront;
-(BOOL)selectMacCameraDevice:(int)cameraId;
-(BOOL)createInput;
-(BOOL)createOutput;
-(BOOL)createSession;
-(void)reorientCamera;
-(BOOL)configSession;
-(BOOL)switchCamera;
-(int) getCameraCount;
-(NSString*) getCameraName:(int)cameraId;
-(YouMeErrorCode_t) setOpenCameraId:(int)cameraId;
-(void)toYUV420PFromOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer;
- (void)openBeautify:(bool)open;
- (void)beautifyChanged:(float)param;
- (void)stretchFace:(bool)stretch;
- (void)setLocalVideoMirrorMode:(YouMeVideoMirrorMode_t) mirrorMode ;

#if TARGET_OS_IPHONE
- (BOOL)isCameraZoomSupported;
- (CGFloat)setCameraZoomFactor:(CGFloat)zoomFactor;
- (BOOL)isCameraFocusPositionInPreviewSupported;
- (BOOL)setCameraFocusPositionInPreview:(CGPoint)position;
- (BOOL)isCameraTorchSupported;
- (BOOL)setCameraTorchOn:(BOOL)isOn;
- (BOOL)isCameraAutoFocusFaceModeSupported;
- (BOOL)setCameraAutoFocusFaceModeEnabled:(BOOL)enable;
#endif

- (BOOL)checkScreenCapturePermission;
@end

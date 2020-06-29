//
//  CameraController.m
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/12.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import "CameraManager.h"
#import "CameraController.h"

/*
 * iOS可支持的摄像头采集分辨率：
 * AVCaptureSessionPreset352x288、AVCaptureSessionPreset640x480、AVCaptureSessionPreset1280x720、AVCaptureSessionPreset1920x1080
 * Mac OS可支持的摄像头采集分辨率：
 * AVCaptureSessionPreset352x288、AVCaptureSessionPreset640x480、AVCaptureSessionPreset1280x720
 */

#if TARGET_OS_IPHONE
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#endif

@interface CameraController ()

@end

@implementation CameraController

static CameraController *s_CameraController_sharedInstance = nil;

+ (CameraController *)getInstance
{
    @synchronized (self)
    {
        if (s_CameraController_sharedInstance == nil)
        {
            s_CameraController_sharedInstance = [[self alloc] init];
        }
    }
    
    return s_CameraController_sharedInstance;
}

-(void)test{
    NSLog(@"This is CameraController test api.");
    @autoreleasepool {
        //CameraManager::getInstance()->test();
    }
}

- (instancetype)init {
    self->fFps = 15;
    self->nWidth = 352;
    self->nHeight = 288;
    self->needLoseFrame = false;
    self->totalTime = 0;
    self->frameCount = 0;
    self->mirrorMode = YOUME_VIDEO_MIRROR_MODE_AUTO;
    self->enableFrontCamera = YES;
    self->macCameraDeviceCount = -1;
    self->macCameraDeviceId = 0;
    self->_brgaBuffer = nil;
    self->_abortCameraRunning = true;
    self->_torchOn = false;
    self->_autoFoucs = false;
#ifdef OPEN_BEAUTIFY
    //    self.openBeautify = YES;
//    self.processor = nil;
    //    if( self.openBeautify ){
    //默认就new 出来，加速开关速度
//    self.processor = [[MuseRealTimeProcessor alloc] init];
//    YMOpenGLContext* currentContext = [YMOpenGLContext currentContext];
//    if (currentContext) {
//        NSLog(@"openglel api:%ul",currentContext.API);
//        [self.processor setupWithAPI:currentContext.API];
//    }else{
//        [self.processor setupWithAPI:kEAGLRenderingAPIOpenGLES2];
//    }
    [YMOpenGLContext  setCurrentContext:currentContext];
    //  [self beautifyChanged:0.6];
    //    }
#endif
    self.screenOrientation = 0;
#ifndef MAC_OS
    [self statusBarOrientationChange: nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(statusBarOrientationChange:)name:UIApplicationDidChangeStatusBarOrientationNotification object:nil];
#else
    [[NSNotificationCenter defaultCenter] addObserverForName:AVCaptureDeviceWasDisconnectedNotification
                                          object:nil
                                          queue:[NSOperationQueue mainQueue]
                                          usingBlock:^(NSNotification *note)
                                         {
                                             [self handleDisconnect:note.object];
                                         }];
    
    [[NSNotificationCenter defaultCenter] addObserverForName:AVCaptureDeviceWasConnectedNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *note)
                                         {
                                             [self handleConnect:note.object];
                                         }];
#endif
    return [super init];
}

- (void)handleDisconnect:(AVCaptureDevice *)device {
    if (![device hasMediaType:AVMediaTypeVideo])
    {
        return;
    }
    self->macCameraDeviceId = -1;
    [self getCameraCount];
    TSK_DEBUG_INFO("Send Event callback, event(%d):%s, errCode:%d, room:%s, param:%s",
                   YOUME_EVENT_CAMERA_DEVICE_DISCONNECT,
                   "CAMERA_DEVICE_DISCONNECT",
                   YOUME_SUCCESS,
                   NULL,
                   [device localizedName].UTF8String);
    CameraManager::getInstance()->callbackEvent(YOUME_EVENT_CAMERA_DEVICE_DISCONNECT, YOUME_SUCCESS, "", [device localizedName].UTF8String);
}

- (void)handleConnect:(AVCaptureDevice *)device {
    if (![device hasMediaType:AVMediaTypeVideo])
    {
        return;
    }
    self->macCameraDeviceId = -1;
    [self getCameraCount];
    TSK_DEBUG_INFO("Send Event callback, event(%d):%s, errCode:%d, room:%s, param:%s",
                   YOUME_EVENT_CAMERA_DEVICE_CONNECT,
                   "CAMERA_DEVICE_CONNECT",
                   YOUME_SUCCESS,
                   NULL,
                   [device localizedName].UTF8String);
    CameraManager::getInstance()->callbackEvent(YOUME_EVENT_CAMERA_DEVICE_CONNECT, YOUME_SUCCESS, "", [device localizedName].UTF8String);
}

#ifndef MAC_OS
- (void)statusBarOrientationChange:(NSNotification *)notification {
    switch([[UIApplication sharedApplication] statusBarOrientation]) {
        case UIInterfaceOrientationPortrait:
            self.screenOrientation = 0;
            self.videoOrientation = AVCaptureVideoOrientationPortrait;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            self.screenOrientation = 180;
            self.videoOrientation = AVCaptureVideoOrientationPortraitUpsideDown;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            self.screenOrientation = 270;
            self.videoOrientation = AVCaptureVideoOrientationLandscapeLeft;
            break;
        case UIInterfaceOrientationLandscapeRight:
            self.screenOrientation = 90;
            self.videoOrientation = AVCaptureVideoOrientationLandscapeRight;
            break;
        default:
            break;
    }
    if(self.avCaptureConnection) [self.avCaptureConnection setVideoOrientation:self.videoOrientation];
    TSK_DEBUG_INFO("statusBarOrientationChange: %d", self.screenOrientation);
}
#endif

-(void)setCapturePropertyWithFps:(float)fps Width:(int)width Height:(int)height {
    self->fFps = fps;
    self->nWidth = width;
    self->nHeight = height;
}

-(void)setCaptureFrontCameraEnable:(BOOL)enable {
    self->enableFrontCamera = enable;
}

-(BOOL)isCaptureFrontCameraEnable {
    return self->enableFrontCamera;
}

-(BOOL)startCapture {
    NSLog(@"%s() is called.", __FUNCTION__);
    BOOL ret = NO;
#if TARGET_OS_IPHONE
    if(![self checkCameraAuthorization]) {
        NSLog(@"camera is not allowed");
        return ret;
    }
    
    if(![self selectiOSCameraDevice:enableFrontCamera]) {
        NSLog(@"iOS Front camera is not avaiable");
        return ret;
    }
#else
    if(![self selectMacCameraDevice:self->macCameraDeviceId]) {
        NSLog(@"FaceTime HD Camera is not avaiable");
        return ret;
    }
#endif
    
    if(![self createSession]) {
        NSLog(@"create session failed.");
        return ret;
    }
    
    if(![self createInput]) {
        NSLog(@"createInput failed.");
        return ret;
    }
    
    if(![self createOutput]) {
        NSLog(@"createOutput failed.");
        return ret;
    }
    
    if(![self configSession]) {
        NSLog(@"configSession failed.");
        return ret;
    }
    
    [self.avCaptureSession startRunning];
    _abortCameraRunning = false;
    NSLog(@"%s() is called end.", __FUNCTION__);
    return YES;
}

-(BOOL)stopCapture {
    _abortCameraRunning = true;
    [self.avCaptureSession stopRunning];
    if(_brgaBuffer){
        free(_brgaBuffer);
        _brgaBuffer = nil;
    }
    NSLog(@"%s() is called.", __FUNCTION__);
    return YES;
}

#if TARGET_OS_IPHONE
-(BOOL)checkCameraAuthorization{
    __block BOOL isAvail = NO;
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo])
    {
        case AVAuthorizationStatusAuthorized:
        {
            NSLog(@"AVAuthorizationStatusAuthorized");
            isAvail = YES;
            break;
        }
        case AVAuthorizationStatusNotDetermined:
        {
            NSLog(@"AVAuthorizationStatusNotDetermined");
            
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted){
                if(!granted) {
                    NSLog(@"granted is false");
                } else {
                    isAvail = YES;
                    _abortCameraRunning = false;
                    NSLog(@"granted is true");
                    
                    if(![self selectiOSCameraDevice:enableFrontCamera]) {
                        NSLog(@"front camera is not allowed");
                        return;
                    }
                    
                    if(![self createSession]) {
                        NSLog(@"create session failed.");
                        return;
                    }
                    
                    if(![self createInput]) {
                        NSLog(@"createInput failed.");
                        return;
                    }
                    
                    if(![self createOutput]) {
                        NSLog(@"createOutput failed.");
                        return;
                    }
                    
                    if(![self configSession]) {
                        NSLog(@"configSession failed.");
                        return;
                    }
                    
                    [self.avCaptureSession startRunning];
                    CameraManager::getInstance()->setCameraOpenStatus(true);
                    NSLog(@"%s() is called end.", __FUNCTION__);
                    return;
                }
            }];
            break;
        }
        case AVAuthorizationStatusDenied:
        {
            NSLog(@"AVAuthorizationStatusDenied");
            break;
        }
        case AVAuthorizationStatusRestricted:
        {
            NSLog(@"AVAuthorizationStatusRestricted");
            break;
        }
        default:
        {
            NSLog(@"default");
            break;
        }
    }
    
    return isAvail;
}
#endif

- (AVCaptureDevice *)getiOSCamera:(AVCaptureDevicePosition)position
{
    //获取前置摄像头设备
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices)
    {
        if (device.position == position)
            return device;
    }
    return nil;
}

- (AVCaptureDevice *)getMacCamera:(int)cameraId
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices) {
        if (cameraId == [devices indexOfObject:device]) {
            return device;
        }
    }
    return nil;
}

-(BOOL)selectiOSCameraDevice:(BOOL)isFront{
    AVCaptureDevice *videoDevice;
    /*
     if(isFront) {
     videoDevice = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInDualCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionFront];
     if(!videoDevice) {
     videoDevice = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionFront];
     }
     } else {
     videoDevice = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInDualCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionBack];
     
     if(!videoDevice) {
     videoDevice = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionBack];
     }
     }
     */
    if(isFront) {
        videoDevice = [self getiOSCamera:AVCaptureDevicePositionFront];
    } else {
        videoDevice = [self getiOSCamera:AVCaptureDevicePositionBack];
    }
    
    if(!videoDevice) {
        videoDevice = [self getiOSCamera:AVCaptureDevicePositionUnspecified];
    }
    
    if(!videoDevice) {
        return NO;
    }
    
    self.avCaptureDevice = videoDevice;
    return YES;
}

-(BOOL)selectMacCameraDevice:(int)cameraId{
    AVCaptureDevice *videoDevice = [self getMacCamera:cameraId];

    if(!videoDevice) {
        NSLog(@"id:[%d] camera is not avaiable", cameraId);
        videoDevice = [self getMacCamera:0];
    }
    
    if(!videoDevice) {
        return NO;
    }
    
    self.avCaptureDevice = videoDevice;
    return YES;
}

-(BOOL)createInput{
    NSError *error = nil;
    AVCaptureDeviceInput* captureDeviceInput = [AVCaptureDeviceInput deviceInputWithDevice:self.avCaptureDevice error:&error];
    
    if(!captureDeviceInput) {
        TSK_DEBUG_ERROR("Could not create video device input:%d", [error code]);
        return NO;
    }else{
        NSLog(@"Create input successfully");
        self.avCaptureDeviceInput = captureDeviceInput;
        
    }
    return YES;
}

-(BOOL)createOutput{
    self.videoDataQueue = dispatch_queue_create("video data queue", DISPATCH_QUEUE_SERIAL);
    
    if(!self.videoDataQueue) {
        NSLog(@"%s() is failed", __FUNCTION__);
        return NO;
    }
    self.avCaptureVideoDataOutput = [[AVCaptureVideoDataOutput alloc]init];
    if(!self.avCaptureVideoDataOutput){
        NSLog(@"%s() is failed-1", __FUNCTION__);
        return NO;
    }
    
    self.avCaptureVideoDataOutput.videoSettings = [NSDictionary dictionaryWithObject:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA] forKey:(id)kCVPixelBufferPixelFormatTypeKey];
    [self.avCaptureVideoDataOutput setSampleBufferDelegate:self queue:self.videoDataQueue];
    self.avCaptureVideoDataOutput.alwaysDiscardsLateVideoFrames = YES;
//    if(!SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"7.0")) {
//        AVCaptureConnection* conn = [self.avCaptureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo];
//        if([conn isVideoMinFrameDurationSupported]) {
//            [conn setVideoMinFrameDuration:CMTimeMake(1, (int32_t)self->fFps)];
//        }
//        if([conn isVideoMaxFrameDurationSupported]) {
//            [conn setVideoMaxFrameDuration:CMTimeMake(1, (int32_t)self->fFps)];
//        }
//    }
    return YES;
}

-(BOOL)createSession{
    self.avCaptureSession = [[AVCaptureSession alloc]init];
    if(!self.avCaptureSession) {
        NSLog(@"%s() is failed", __FUNCTION__);
        return NO;
    } else {
        return YES;
    }
}

#if TARGET_OS_IPHONE
-(void) reorientCamera
{
    if(!self.avCaptureSession) return;
    
    auto orientation =  [[UIApplication sharedApplication] statusBarOrientation];
    /*UIInterfaceOrientation orientationNew;
     // use interface orientation as fallback if device orientation is facedown, faceup or unknown
     if(orientation==UIDeviceOrientationFaceDown || orientation==UIDeviceOrientationFaceUp || orientation==UIDeviceOrientationUnknown) {
     orientationNew =[[UIApplication sharedApplication] statusBarOrientation];
     }*/
    
    //bool reorient = false;
    AVCaptureSession* session = (AVCaptureSession*)self.avCaptureSession;
    [session beginConfiguration];
    
    for (AVCaptureVideoDataOutput* output in session.outputs) {
        for (AVCaptureConnection * av in output.connections) {
            //            if (enableFrontCamera) {
            //                av.videoMirrored = YES;
            //            }else{
            //                av.videoMirrored = NO;
            //            }
            switch (orientation) {
                    // UIInterfaceOrientationPortraitUpsideDown, UIDeviceOrientationPortraitUpsideDown
                case UIInterfaceOrientationPortraitUpsideDown:
                    if(av.videoOrientation != AVCaptureVideoOrientationPortraitUpsideDown) {
                        av.videoOrientation = AVCaptureVideoOrientationPortraitUpsideDown;
                        //    reorient = true;
                    }
                    break;
                    // UIInterfaceOrientationLandscapeRight, UIDeviceOrientationLandscapeLeft
                case UIInterfaceOrientationLandscapeRight:
                    if(av.videoOrientation != AVCaptureVideoOrientationLandscapeRight) {
                        av.videoOrientation = AVCaptureVideoOrientationLandscapeRight;
                        //    reorient = true;
                    }
                    break;
                    // UIInterfaceOrientationLandscapeLeft, UIDeviceOrientationLandscapeRight
                case UIInterfaceOrientationLandscapeLeft:
                    if(av.videoOrientation != AVCaptureVideoOrientationLandscapeLeft) {
                        av.videoOrientation = AVCaptureVideoOrientationLandscapeLeft;
                        //   reorient = true;
                    }
                    break;
                    // UIInterfaceOrientationPortrait, UIDeviceOrientationPortrait
                case UIInterfaceOrientationPortrait:
                    if(av.videoOrientation != AVCaptureVideoOrientationPortrait) {
                        av.videoOrientation = AVCaptureVideoOrientationPortrait;
                        //    reorient = true;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    [session commitConfiguration];
}
#endif

-(void)dumpCameraInfo:(AVCaptureDevice* )device{
    NSObject *range = nil;
    NSObject *format = nil;
    // dump camera supported modes
    TSK_DEBUG_INFO("Camera Supported modes:\n");
    for (format in [device valueForKey:@"formats"]) {
        CMFormatDescriptionRef formatDescription;
        CMVideoDimensions dimensions;
        
        formatDescription = (__bridge CMFormatDescriptionRef) [format performSelector:@selector(formatDescription)];
        dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);
        
        for (range in [format valueForKey:@"videoSupportedFrameRateRanges"]) {
            double min_framerate;
            double max_framerate;
            
            [[range valueForKey:@"minFrameRate"] getValue:&min_framerate];
            [[range valueForKey:@"maxFrameRate"] getValue:&max_framerate];
            TSK_DEBUG_INFO("  %dx%d@[%f %f]fps\n",
                           dimensions.width, dimensions.height,
                           min_framerate, max_framerate);
        }
    }
}
    
-(BOOL)configSession{
    [self.avCaptureSession beginConfiguration];
    
    NSObject *currentFormat = nil;

    NSObject *suitableDimensionFormat = nil;
    NSObject *maxDimensionFormat = nil;
    
    NSObject *currentRange = nil;
 
    
    if([self.avCaptureSession canAddInput:self.avCaptureDeviceInput]) {
        [self.avCaptureSession addInput:self.avCaptureDeviceInput];
        NSLog(@"Add video device input to the session");
    } else {
        NSLog(@"Could not add video device input to the session");
        [self.avCaptureSession commitConfiguration];
        return NO;
    }
    
    if([self.avCaptureSession canAddOutput:self.avCaptureVideoDataOutput]){
        [self.avCaptureSession addOutput:self.avCaptureVideoDataOutput];
        NSLog(@"Add photo output to the session");
    } else {
        NSLog(@"Could not add photo output to the session");
        [self.avCaptureSession commitConfiguration];
        return NO;
    }
    
    [self dumpCameraInfo:self.avCaptureDevice];
    
    for (currentFormat in [self.avCaptureDevice valueForKey:@"formats"]) {
        CMVideoDimensions currentDimensions = CMVideoFormatDescriptionGetDimensions((__bridge CMFormatDescriptionRef) [currentFormat performSelector:@selector(formatDescription)]);
        if (maxDimensionFormat == nil) {
            maxDimensionFormat = currentFormat;
        }else {
            CMVideoDimensions maxDimensions = CMVideoFormatDescriptionGetDimensions((__bridge CMFormatDescriptionRef) [maxDimensionFormat performSelector:@selector(formatDescription)]);
            if (currentDimensions.width * currentDimensions.height > maxDimensions.width * maxDimensions.height) {
                maxDimensionFormat = currentFormat;
            }
        }
        
        if (currentDimensions.width * currentDimensions.height >= self->nWidth * self->nHeight){
            if (suitableDimensionFormat == nil) {
                suitableDimensionFormat = currentFormat;
            } else {
                CMVideoDimensions suitableDimensions = CMVideoFormatDescriptionGetDimensions((__bridge CMFormatDescriptionRef) [suitableDimensionFormat performSelector:@selector(formatDescription)]);
                if (currentDimensions.width * currentDimensions.height < suitableDimensions.width * suitableDimensions.height) {
                    suitableDimensionFormat = currentFormat;
                } else {
                    continue;
                }
            }
        }
    }
    if (suitableDimensionFormat == nil) {
        // 外部设置分辨率过高，使用可支持的最大分辨率
        TSK_DEBUG_WARN("resolution setting too high, use the max resolution settings that device can support");
        suitableDimensionFormat = maxDimensionFormat;
    }
    CMVideoDimensions suitableDimensions = CMVideoFormatDescriptionGetDimensions((__bridge CMFormatDescriptionRef) [suitableDimensionFormat performSelector:@selector(formatDescription)]);
    TSK_DEBUG_INFO("find suitable resolution, width:%d, height:%d", suitableDimensions.width, suitableDimensions.height);
    
    self->needLoseFrame = true;
    self->totalTime = 0;
    self->frameCount = 0;
    NSValue* supported_min_framerate_duration_value = nil;
    double supported_max_framerate_double = 0;
    if ([self.avCaptureDevice lockForConfiguration:NULL] == YES) {
        for (currentRange in [suitableDimensionFormat valueForKey:@"videoSupportedFrameRateRanges"]) {
            double range_max_framerate;
            double range_min_framerate;
            [[currentRange valueForKey:@"maxFrameRate"] getValue:&range_max_framerate];
            [[currentRange valueForKey:@"minFrameRate"] getValue:&range_min_framerate];
            if (range_max_framerate > supported_max_framerate_double) {
                supported_max_framerate_double = range_max_framerate;
                supported_min_framerate_duration_value = [currentRange valueForKey:@"minFrameDuration"];;
            }
            if (fabs (self->fFps - range_min_framerate) < 0.01 && self->fFps != range_min_framerate){
                // Mac上一些USB摄像头只能设置14.999993或30.000030之类不连续的帧率，只能通过这种方式设置，否则崩溃
                NSValue *max_frame_duration = [currentRange valueForKey:@"maxFrameDuration"];
                [self.avCaptureDevice setValue:suitableDimensionFormat forKey:@"activeFormat"];
                [self.avCaptureDevice setValue:max_frame_duration forKey:@"activeVideoMinFrameDuration"];
                [self.avCaptureDevice setValue:max_frame_duration forKey:@"activeVideoMaxFrameDuration"];
                self->needLoseFrame = false;
                break;
            }
            if(fabs (self->fFps - range_max_framerate) < 0.01 && self->fFps != range_max_framerate) {
                // Mac上一些USB摄像头只能设置14.999993或30.000030之类不连续的帧率，只能通过这种方式设置，否则崩溃
                NSValue *min_frame_duration = [currentRange valueForKey:@"minFrameDuration"];
                [self.avCaptureDevice setValue:suitableDimensionFormat forKey:@"activeFormat"];
                [self.avCaptureDevice setValue:min_frame_duration forKey:@"activeVideoMinFrameDuration"];
                [self.avCaptureDevice setValue:min_frame_duration forKey:@"activeVideoMaxFrameDuration"];
                self->needLoseFrame = false;
                break;
            }
            if( self->fFps >= range_min_framerate && self->fFps <= range_max_framerate) {
                // Mac和iOS自带摄像头的情况
#if TARGET_OS_OSX
                [self.avCaptureDevice setValue:suitableDimensionFormat forKey:@"activeFormat"];
#else
                // iOS通过@"activeFormat"的方式设置分辨率会有问题
                [self setDimensionsForiOS:suitableDimensions];
#endif
                [self.avCaptureDevice setActiveVideoMinFrameDuration:CMTimeMake(1, self->fFps)];
                [self.avCaptureDevice setActiveVideoMaxFrameDuration:CMTimeMake(1, self->fFps)];
                self->needLoseFrame = false;
                break;
            }
        }
        if (self->needLoseFrame) {
            TSK_DEBUG_WARN("Could not match frame rate, use supported max framerate:%f and will drop frame", supported_max_framerate_double);
            [self.avCaptureDevice setValue:suitableDimensionFormat forKey:@"activeFormat"];
            [self.avCaptureDevice setValue:supported_min_framerate_duration_value forKey:@"activeVideoMinFrameDuration"];
            [self.avCaptureDevice setValue:supported_min_framerate_duration_value forKey:@"activeVideoMaxFrameDuration"];
        }
        [self.avCaptureDevice unlockForConfiguration];
    } else {
        TSK_DEBUG_INFO("Could not lock device for configuration");
        return NO;
    }
    
    self.avCaptureConnection = [self.avCaptureVideoDataOutput connectionWithMediaType:AVMediaTypeVideo];

    [self.avCaptureSession commitConfiguration];
#if TARGET_OS_IPHONE
    [self reorientCamera];
#endif
    return YES;
}

#if TARGET_OS_IPHONE
-(void) setDimensionsForiOS:(CMVideoDimensions) dimensions {
    int pixelSize = dimensions.width * dimensions.height;
    if (pixelSize == 352*288) {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset352x288;
    }else if (pixelSize == 640*480) {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset640x480;
    }else if (pixelSize == 1280*720) {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset1280x720;
    }else if (pixelSize == 1920*1080) {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset1920x1080;
    }else if (pixelSize == 3840*2160) {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset3840x2160;
    }else {
        self.avCaptureSession.sessionPreset = AVCaptureSessionPreset640x480;
    }
}
#endif

//-(int) getCloselyCameraSize {
//    int ReqTmpWidth = nWidth;
//    int ReqTmpHeight = nHeight;
//    
//    //先查找preview中是否存在与surfaceview相同宽高的尺寸
//    float wRatio = 1.0f;
//    float hRatio = 1.0f;
//    int result = 999;
//    for (int i = 0; i < sizeof(supportedCameraSizes)/sizeof(CGSize); i++) {
//        CGSize size = supportedCameraSizes[i];
//        wRatio = (((float) size.width) / ReqTmpWidth);
//        hRatio = (((float) size.height) / ReqTmpHeight);
//        if((wRatio >= 1.0) && (hRatio >= 1.0) && i < result) {
//            result = i;
//        }
//    }
//    return result;
//    
//}

-(BOOL)switchCamera{
    self->enableFrontCamera = !self->enableFrontCamera;
    NSLog(@"%s() is called. enableFrontCamera:%d", __FUNCTION__, self->enableFrontCamera);
    if(![self stopCapture]) {
        NSLog(@"%s() stopCapture failed.", __FUNCTION__);
        return NO;
    }
    if(![self startCapture]) {
        NSLog(@"%s() startCapture failed.", __FUNCTION__);
        return NO;
    }
    return YES;
}

-(int) getCameraCount{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    [self dumpAllCameraName];
    self->macCameraDeviceCount = devices.count;
    return devices.count;
}

-(void) dumpAllCameraName {
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for( int i = 0; i < [devices count]; i++) {
        const char *name = [[devices[i] localizedName] UTF8String];
        TSK_DEBUG_INFO("camera[%d]:%s\n", i, name);
    }
}

-(NSString*) getCameraName:(int)cameraId {
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    if (cameraId < 0 && cameraId > devices.count - 1) {
        return @"";
    }
    AVCaptureDevice* device = [devices objectAtIndex:cameraId];
    if (device) {
        return [device localizedName];
    }else {
        return @"";
    }
    return @"";
}

-(YouMeErrorCode_t) setOpenCameraId:(int)cameraId {
    if (self->macCameraDeviceCount == -1) {
        [self getCameraCount];
    }
    if (cameraId >= self->macCameraDeviceCount) {
        TSK_DEBUG_INFO("== setOpenCameraId invalid param:%d, camera count:%d", cameraId, self->macCameraDeviceCount);
        return YOUME_ERROR_INVALID_PARAM;
    }
    if (cameraId == self->macCameraDeviceId) {
        return YOUME_SUCCESS;
    }
    self->macCameraDeviceId = cameraId;
    if (!self->_abortCameraRunning) {
        TSK_DEBUG_INFO("== setOpenCameraId reset camera");
        if(![self stopCapture]) {
            NSLog(@"%s() stopCapture failed.", __FUNCTION__);
            return YOUME_ERROR_UNKNOWN;
        }
        if(![self startCapture]) {
            NSLog(@"%s() startCapture failed.", __FUNCTION__);
            return YOUME_ERROR_UNKNOWN;
        }
        return YOUME_SUCCESS;
    }
    return YOUME_SUCCESS;
}

-(void)toYUV420PFromOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer{
    
#if 0
    const int kFlags = 0;
    //捕捉数据输出 要怎么处理虽你便
    CVPixelBufferRef videoFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
    /*Lock the buffer*/
    if(CVPixelBufferLockBaseAddress(videoFrame, kFlags) != kCVReturnSuccess) {
        NSLog(@"lock data failed.");
        return;
    }
    
    uint8_t* baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(videoFrame);
    size_t bufferSize = CVPixelBufferGetDataSize(videoFrame);
    size_t width = CVPixelBufferGetWidth(videoFrame);
    size_t height = CVPixelBufferGetHeight(videoFrame);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
    size_t bytesPerPixel = bytesPerRow/width;
    int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
    
    switch (pixelFormat) {
        case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
            NSLog(@"Capture pixel format=420v");
            break;
        case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
            NSLog(@"Capture pixel format=420f");
            break;
        case kCVPixelFormatType_32BGRA:
            NSLog(@"Capture pixel format=BGRA");
            // transfer the 32BGRA -> YUV420P
            break;
        default:
            NSLog(@"Capture pixel format=others(0x%x)", pixelFormat);
            break;
    }
    
    size_t planeCount = CVPixelBufferGetPlaneCount(videoFrame);
    for(size_t i = 0; i < planeCount; i++) {
        void* addr = CVPixelBufferGetBaseAddressOfPlane(videoFrame, i);
        size_t bpr = CVPixelBufferGetBytesPerRowOfPlane(videoFrame, i);
        NSLog(@"planeIdx:%d addr:%x bpt:%d", i, addr, bpr);
    }
    
    CameraManager::getInstance()->videoDataOutputWith32BGRA((int)self->fFps, width, height, 0 baseAddress, bufferSize);
    /*We unlock the buffer*/
    CVPixelBufferUnlockBaseAddress(videoFrame, kFlags);
    NSLog(@"video data:addr:0x%x size:%d w:%d h:%d bpp:%d bpr:%d fmt:%x",
          baseAddress, bufferSize, width, height, bytesPerPixel, bytesPerRow, pixelFormat);
#endif
}

#ifdef OPEN_BEAUTIFY
- (void)checkLocalPixelBufferForSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
    
    size_t inputW = CVPixelBufferGetWidth( pixelBuffer);
    size_t inputH = CVPixelBufferGetHeight(pixelBuffer);
    
    size_t allocW = inputW;
    size_t allocH = inputH;
    
    BOOL realloc = NO;
    if (!(_resultBufferNormal)) {
        realloc = YES;
    } else {
        size_t resultW = CVPixelBufferGetWidth( _resultBufferNormal);
        size_t resultH = CVPixelBufferGetHeight(_resultBufferNormal);
        
        if (resultW != allocW || resultH != allocH) {
            realloc = YES;
        }
    }
    
    if (realloc) {
        CVPixelBufferRef pNormal = _resultBufferNormal;
        _resultBufferNormal = NULL;
        if (pNormal) {
            CFRelease(pNormal);
        }
        
        CFDictionaryRef empty; // empty value for attr value.
        CFMutableDictionaryRef attrs;
        // empty IOSurface properties dictionary
        empty = CFDictionaryCreate(kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(attrs, kCVPixelBufferIOSurfacePropertiesKey, empty);
        
        CVReturn err = CVPixelBufferCreate(kCFAllocatorDefault, allocW, allocH, kCVPixelFormatType_32BGRA, attrs, &pNormal);
        if (err) {
            NSLog(@"Create local pixel buffer error:%d", err);
        } else {
            _resultBufferNormal = pNormal;
        }
        
        CFRelease(empty);
        CFRelease(attrs);
    }
}
#endif

- (void)beautifyChanged:(float)param
{
#ifdef OPEN_BEAUTIFY
//    if(self.processor != nil){
//        self.processor.beautify = param;
//    }
#endif
}

- (void)stretchFace:(bool)stretch
{
#ifdef OPEN_BEAUTIFY
//    if(self.processor != nil){
//        self.processor.stretchFace = stretch;
//    }
#endif
}

- (void)openBeautify:(bool)open
{
#ifdef OPEN_BEAUTIFY
    self.openBeautify = open;
#endif
}

-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection{
    if (_abortCameraRunning) {
        return;
    }
    int mirror = self->mirrorMode ;
    if ((enableFrontCamera)) {
        if (YOUME_VIDEO_MIRROR_MODE_AUTO == mirror) {
            mirror = YOUME_VIDEO_MIRROR_MODE_NEAR;
        } else if (YOUME_VIDEO_MIRROR_MODE_FAR == mirror) {
            mirror = YOUME_VIDEO_MIRROR_MODE_DISABLED;
        }
    } else {
        mirror = YOUME_VIDEO_MIRROR_MODE_DISABLED;
    }
    
    if (connection == self.avCaptureConnection) {
#if 0
        const int kFlags = 0;
        //捕捉数据输出 要怎么处理虽你便
        CFRetain(sampleBuffer);
#ifdef OPEN_BEAUTIFY
        CVPixelBufferRef videoFrame;
//        if(self.openBeautify && self.processor != nil){
//            [self checkLocalPixelBufferForSampleBuffer:sampleBuffer];
//            videoFrame = _resultBufferNormal;
//            [self.processor effectPixelBuffer:sampleBuffer toPixelBuffer:videoFrame background:NULL mirror:NO mirrorResult:NO orientation:self.videoOrientation recordSec:0 trailerAnimation:0];
//            /*Lock the buffer*/
//            if(CVPixelBufferLockBaseAddress(videoFrame, kFlags) != kCVReturnSuccess) {
//                NSLog(@"lock data failed.");
//                CFRelease(sampleBuffer);
//                return;
//            }
//        }else{
            videoFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
            /*Lock the buffer*/
            if(CVPixelBufferLockBaseAddress(videoFrame, kFlags) != kCVReturnSuccess) {
                NSLog(@"lock data failed.");
                CFRelease(sampleBuffer);
                return;
            }
//        }
#else
        CVPixelBufferRef videoFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
        /*Lock the buffer*/
        if(CVPixelBufferLockBaseAddress(videoFrame, kFlags) != kCVReturnSuccess) {
            NSLog(@"lock data failed.");
            CFRelease(sampleBuffer);
            return;
        }
#endif
        CFRetain( videoFrame );
        
        uint8_t* baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(videoFrame);
        size_t bufferSize = CVPixelBufferGetDataSize(videoFrame);
        size_t width = CVPixelBufferGetWidth(videoFrame);
        size_t height = CVPixelBufferGetHeight(videoFrame);
        size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
        //        size_t bytesPerPixel = bytesPerRow/width;
        int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
        
        switch (pixelFormat) {
            case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
                //NSLog(@"Capture pixel format=420v");
                break;
            case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
                //NSLog(@"Capture pixel format=420f");
                break;
            case kCVPixelFormatType_32BGRA:
                //NSLog(@"Capture pixel format=BGRA");
                // transfer the 32BGRA -> YUV420P
                break;
            default:
                NSLog(@"Capture pixel format=others(0x%x)", pixelFormat);
                break;
        }
        
        int rotationDegree = 0;
        switch([connection videoOrientation]) {
            case AVCaptureVideoOrientationPortrait:
                rotationDegree = 0;
                break;
            case AVCaptureVideoOrientationPortraitUpsideDown:
                rotationDegree = 180;
                break;
            case AVCaptureVideoOrientationLandscapeRight:
                rotationDegree = 90;
                break;
            case AVCaptureVideoOrientationLandscapeLeft:
                rotationDegree = 270;
                break;
        }
        
        rotationDegree = (360 + rotationDegree - self.screenOrientation)%360;
        
        uint8_t* buffer;
        if(bytesPerRow != width*4)
        {
            if(!_brgaBuffer){
                _brgaBuffer = (uint8_t*)malloc(width*height*4);
            }
            buffer = self->_brgaBuffer;
            for (int i = 0; i < height; i++) {
                memcpy(&buffer[i*width*4], &baseAddress[i*bytesPerRow], width*4);
            }
        }
        else{
            buffer = baseAddress;
        }
        
        CameraManager::getInstance()->videoDataOutput((int)self->fFps, width, height, rotationDegree, buffer, bufferSize, 3, mirror, 1);
        /*We unlock the buffer*/
        CFRelease(videoFrame);
        CFRelease(sampleBuffer);
        CVPixelBufferUnlockBaseAddress(videoFrame, kFlags);
        //NSLog(@"video data:addr:0x%x size:%d w:%d h:%d d:%d bpp:%d bpr:%d fmt:%x",
        //      baseAddress, bufferSize, width, height, rotationDegree, bytesPerPixel, bytesPerRow, pixelFormat);
#else
        int rotationDegree = 0;
//        switch([connection videoOrientation]) {
//            case AVCaptureVideoOrientationPortrait:
//                rotationDegree = 0;
//                break;
//            case AVCaptureVideoOrientationPortraitUpsideDown:
//                rotationDegree = 180;
//                break;
//            case AVCaptureVideoOrientationLandscapeRight:
//                rotationDegree = 90;
//                break;
//            case AVCaptureVideoOrientationLandscapeLeft:
//                rotationDegree = 270;
//                break;
//        }
//        if(self.isCaptureFrontCameraEnable){
//            //前置摄像头旋转处理
//            if(rotationDegree > 0){
//                rotationDegree = (360 - rotationDegree + self.screenOrientation) % 360;
//            }else{
//                rotationDegree = self.screenOrientation % 360;
//            }
//        }else{
//
//        }

        CFRetain(sampleBuffer);
        CVPixelBufferRef videoFrame = CMSampleBufferGetImageBuffer(sampleBuffer);
        CFRelease(sampleBuffer);
        size_t width = CVPixelBufferGetWidth(videoFrame);
        size_t height = CVPixelBufferGetHeight(videoFrame);
        UInt64 recordTime = [[NSDate date] timeIntervalSince1970] * 1000;
        if (self->needLoseFrame) {
            long recordTime = [[NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]*1000] longValue];
            if ((recordTime - self->totalTime)/(1000.0f/self->fFps) >= self->frameCount && self->frameCount < self->fFps)
            {
                self->frameCount++;
                CameraManager::getInstance()->videoDataOutputGLESForIOS(videoFrame, width, height, 0, rotationDegree, mirror, recordTime, 1);
            }
            if (recordTime - self->totalTime >= 1000)
            {
                self->totalTime = recordTime;
                self->frameCount = 0;
            }
        }else {
            CameraManager::getInstance()->videoDataOutputGLESForIOS(videoFrame, width, height, 0, rotationDegree, mirror, recordTime, 1);
        }
#endif
        
    }
}

#if TARGET_OS_IPHONE
- (BOOL)isCameraZoomSupported{
    if(!self.avCaptureDeviceInput)
        return FALSE;
    return self.avCaptureDeviceInput.device.activeFormat.videoMaxZoomFactor > 1.0;
}
- (CGFloat)setCameraZoomFactor:(CGFloat)zoomFactor{
 
    if(![self isCameraZoomSupported]) {
        return 1.0f;
    }
    
    NSError* err = nil;
    AVCaptureDevice* device = (AVCaptureDevice*)_avCaptureDevice;
    if(self.avCaptureDeviceInput.device.activeFormat.videoMaxZoomFactor < zoomFactor)
        return device.videoZoomFactor;
  
    if([device lockForConfiguration:&err]) {
        //[device rampToVideoZoomFactor:zoomFactor withRate:10];
        device.videoZoomFactor = zoomFactor;
        [device unlockForConfiguration];
    } else {
        NSLog(@"Error while locking device for zoomFactor POI: %@", err);
        return 1.0f;
    }
    return zoomFactor;
    
}
- (BOOL)isCameraFocusPositionInPreviewSupported{
    
    if(!_avCaptureDevice)
        return FALSE;
    AVCaptureDevice* device = (AVCaptureDevice*)_avCaptureDevice;
    return device.focusPointOfInterestSupported;
}
- (BOOL)setCameraFocusPositionInPreview:(CGPoint)position{
    if(!_avCaptureDevice)
        return FALSE;
    AVCaptureDevice* device = (AVCaptureDevice*)_avCaptureDevice;
    BOOL ret = device.focusPointOfInterestSupported;
    
    if(ret) {
        NSError* err = nil;
        if([device lockForConfiguration:&err]) {
            if ([device isWhiteBalanceModeSupported:AVCaptureWhiteBalanceModeAutoWhiteBalance]) {
                [device setWhiteBalanceMode:AVCaptureWhiteBalanceModeAutoWhiteBalance];
            }
            
            if ([device isFocusModeSupported:AVCaptureFocusModeContinuousAutoFocus]) {
                [device setFocusMode:AVCaptureFocusModeAutoFocus];
                [device setFocusPointOfInterest:position];
            }
            
            if([device isExposurePointOfInterestSupported] && [device isExposureModeSupported:AVCaptureExposureModeContinuousAutoExposure]) {
                [device setExposurePointOfInterest:position];
                [device setExposureMode:AVCaptureExposureModeContinuousAutoExposure];
            }
            [device unlockForConfiguration];
        } else {
            NSLog(@"Error while locking device for focus POI: %@", err);
            ret = FALSE;
        }
    } else {
        NSLog(@"Focus POI not supported");
    }
    
    return ret;
    
}
- (BOOL)isCameraTorchSupported{
    if(!_avCaptureSession)
        return FALSE;
    
    AVCaptureSession* session = (AVCaptureSession*)_avCaptureSession;
    AVCaptureDeviceInput* currentCameraInput = [session.inputs objectAtIndex:0];
    return currentCameraInput.device.torchAvailable;
}
- (BOOL)setCameraTorchOn:(BOOL)isOn{
    BOOL ret = FALSE;
    if(!_avCaptureSession)
        return ret;
    
    AVCaptureSession* session = (AVCaptureSession*)_avCaptureSession;
    
    [session beginConfiguration];
    AVCaptureDeviceInput* currentCameraInput = [session.inputs objectAtIndex:0];
    
    if(currentCameraInput.device.torchAvailable) {
        NSError* err = nil;
        if([currentCameraInput.device lockForConfiguration:&err]) {
            [currentCameraInput.device setTorchMode:( isOn ? AVCaptureTorchModeOn : AVCaptureTorchModeOff ) ];
            [currentCameraInput.device unlockForConfiguration];
            ret = (currentCameraInput.device.torchMode == AVCaptureTorchModeOn);
        } else {
            NSLog(@"Error while locking device for torch: %@", err);
            ret = FALSE;
        }
    } else {
        NSLog(@"Torch not available in current camera input");
    }
    [session commitConfiguration];
    _torchOn = ret;
    return ret;
}

- (BOOL)isCameraAutoFocusFaceModeSupported{
    if(!_avCaptureSession)
        return FALSE;
    AVCaptureDevice* device = (AVCaptureDevice*)_avCaptureSession;
    AVCaptureFocusMode newMode = AVCaptureFocusModeContinuousAutoFocus;
    return [device isFocusModeSupported:newMode];
}

- (BOOL)setCameraAutoFocusFaceModeEnabled:(BOOL)enable{
    if(!_avCaptureSession)
        return FALSE;
    AVCaptureDevice* device = (AVCaptureDevice*)_avCaptureSession;
    AVCaptureFocusMode newMode = enable ?  AVCaptureFocusModeContinuousAutoFocus : AVCaptureFocusModeAutoFocus;
    BOOL ret = [device isFocusModeSupported:newMode];
    
    if(ret) {
        NSError *err = nil;
        if ([device lockForConfiguration:&err]) {
            device.focusMode = newMode;
            [device unlockForConfiguration];
        } else {
            NSLog(@"Error while locking device for autofocus: %@", err);
            ret = FALSE;
        }
    } else {
        NSLog(@"Focus mode not supported: %@", enable ? @"AVCaptureFocusModeContinuousAutoFocus" : @"AVCaptureFocusModeAutoFocus");
    }
    
    return ret;
}
#endif

- (void)setLocalVideoMirrorMode:(YouMeVideoMirrorMode_t) mirrorMode
{
    self->mirrorMode = mirrorMode;
}


/*
- (void)setFrameRateWithDurationInternal:(int)fps {
    if(self->fFps == fps)
        return;
    CMTime frameDuration = CMTimeMake(1, fps);
    AVCaptureDevice *videoDevice             = _avCaptureDevice;
    NSError *error = nil;
    NSArray *supportedFrameRateRanges        = [videoDevice.activeFormat videoSupportedFrameRateRanges];
    BOOL frameRateSupported                  = NO;
    for(AVFrameRateRange *range in supportedFrameRateRanges){
        if(CMTIME_COMPARE_INLINE(frameDuration, >=, range.minFrameDuration) &&
           CMTIME_COMPARE_INLINE(frameDuration, <=, range.maxFrameDuration)) {
            frameRateSupported  = YES;
            break;
        }
    }
    
    if(frameRateSupported && [videoDevice lockForConfiguration:&error]) {
        [videoDevice setActiveVideoMaxFrameDuration:frameDuration];
        [videoDevice setActiveVideoMinFrameDuration:frameDuration];
        [videoDevice unlockForConfiguration];
    }
    self->fFps = fps;
}
*/

- (bool)canRecordScreen
{
#ifdef MAC_OS
    #if __clang_major__ >= 9
    // Xcode 9+
    if (@available(macOS 10.15, *)) {
    #else
    // Xcode 8-
    // 1575 is macos 10.14.6
    if (floor(NSFoundationVersionNumber) > 1575.0) {
    #endif

        CFArrayRef windowList = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
        NSUInteger numberOfWindows = CFArrayGetCount(windowList);
        NSUInteger enableScreen = 0;
        for (int idx = 0; idx < numberOfWindows; idx++) {
            NSDictionary *windowInfo = (NSDictionary *)CFArrayGetValueAtIndex(windowList, idx);
            NSString *windowName = windowInfo[(__bridge id)kCGWindowName];
            NSString *ownerName = windowInfo[(__bridge id)kCGWindowOwnerName];
            if ([ownerName isEqualToString:@"SystemUIServer"] && windowName) {
                enableScreen++;
                break;
            } else {
                //no kCGWindowName detected -> not enabled
                continue; //breaking early, numberOfWindowsWithName not increased
            }
        }
        CFRelease(windowList);
        return (1 == enableScreen);
    }
#endif
    return true;
}

- (void)showScreenRecordingPrompt
{
#ifdef MAC_OS
    // macos 10.14 and lower do not require screen recording permission to get window titles
    #if __clang_major__ >= 9
    // Xcode 9+
    if (@available(macOS 10.15, *)) {
    #else
    // Xcode 8-
    // 1575 is macos 10.14.6
    if (floor(NSFoundationVersionNumber) >= 1575.0) {
    #endif
//    if(@available(macos 10.15, *)) {
        // To minimize the intrusion just make a 1px image of the upper left corner
        // This way there is no real possibilty to access any private data
        CGImageRef c = CGWindowListCreateImage(
                                        CGRectMake(0, 0, 1, 1),
                                        kCGWindowListOptionOnScreenOnly,
                                        kCGNullWindowID,
                                        kCGWindowImageDefault);

        CFRelease(c);
    }
#endif
}

- (void)openPrivacySetting{
#ifdef MAC_OS
    NSString *urlString = @"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture";
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:urlString]];
#endif
}

- (bool)checkScreenCapturePermission
{
#ifdef MAC_OS
    // check screen capture permisson
    bool ret = [self canRecordScreen];
    if (!ret)
    {
        // request screen capture permisson
        [self showScreenRecordingPrompt];
        
        // clear permisson record
        [self openPrivacySetting];
        return false;
    }
#endif
    return true;
}

@end

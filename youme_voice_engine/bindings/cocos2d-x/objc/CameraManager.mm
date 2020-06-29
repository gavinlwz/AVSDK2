//
//  CameraManager.m
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/11.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import <Foundation/NSDictionary.h>
#import <CoreVideo/CVPixelBuffer.h>
#import <ImageIO/CGImageProperties.h>

#import "IYouMeVoiceEngine.h"
#import "CameraManager.h"
#import "NgnTalkManager.hpp"
#import "YouMeEngineManagerForQiniu.hpp"
#import "CameraController.h"
#import "YouMeEngineVideoCodec.hpp"
#import "YouMeVideoMixerAdapter.h"
#import "tmedia_defaults.h"

#import <sys/sysctl.h>
#import <mach/mach.h>
#import <malloc/malloc.h>

#define CLAMP(i)   (((i) < 0) ? 0 : (((i) > 255) ? 255 : (i)))
#if defined(__APPLE__)
#define IS_UNDER_APP_EXTENSION ([[[NSBundle mainBundle] executablePath] containsString:@".appex/"])
#endif

CIContext *_ciContext = nil;
CFDictionaryRef optionsDictionary = nil;

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
    _videoData = NULL;
    _videoDataSize = 0;
    _opening = false;
    _closeing = false;

    _encWidth =  640;
    _encHeight = 480;
    _isCacheLastPixel = true;
    _lastPixelRef = NULL;
    _underAppExtension = IS_UNDER_APP_EXTENSION;
    
    NSMutableDictionary *option = [[NSMutableDictionary alloc] init];
    [option setObject: [NSNull null] forKey: kCIContextWorkingColorSpace];
    [option setObject: @"NO" forKey: kCIContextUseSoftwareRenderer];
    
    // 利用CIIMAGE做视频缩放和旋转，复用CICONTEXT，减少每次申请释放的资源消耗
    _ciContext = [CIContext contextWithOptions:option];

    const void *keys[] = {
        kCVPixelBufferMetalCompatibilityKey,
        kCVPixelBufferIOSurfacePropertiesKey,
        kCVPixelBufferBytesPerRowAlignmentKey,
    };
    const void *values[] = {
        (__bridge const void *)([NSNumber numberWithBool:YES]),
        (__bridge const void *)(@{}),
        (__bridge const void *)(@(32)),
    };
    // 创建pixelbuffer属性
    optionsDictionary = CFDictionaryCreate(NULL, keys, values, 3, NULL, NULL);
    
    if (_underAppExtension) {
        TSK_DEBUG_INFO("camera app extension mode");
    }
}

CameraManager::~CameraManager()
{
    if (optionsDictionary) {
        CFRelease(optionsDictionary);
        optionsDictionary = nil;
    }
    
    if(_videoData)
        delete [] _videoData;
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

void CameraManager::callbackEvent(const YouMeEvent event, const YouMeErrorCode error, const char * room, const char * param)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    this->youmeEventCallback->onEvent(event, error, room, param);
}

YouMeErrorCode CameraManager::startCapture()
{
    NSLog(@"%s() is called.", __FUNCTION__);
    if (_opening || _closeing) {
        return YOUME_SUCCESS;
    }
    _opening = true;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->open) {
        NSLog(@"%s() unexpected repeat calling", __FUNCTION__);
        _opening = false;
        return YOUME_SUCCESS;
    }
    CameraController *cameraController = [CameraController getInstance];
    if([cameraController startCapture]){
        ICameraManager::startCapture();
        this->open = true;
        _opening = false;
        return YOUME_SUCCESS;
    } else  {
        _opening = false;
        return YOUME_ERROR_START_FAILED;
    }
}

YouMeErrorCode CameraManager::stopCapture()
{
    NSLog(@"%s() is called.", __FUNCTION__);
    if (_opening || _closeing) {
        return YOUME_SUCCESS;
    }
    _closeing = true;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(!this->open) {
        NSLog(@"%s() unexpected repeat calling", __FUNCTION__);
        _closeing = false;
        return YOUME_SUCCESS;
    }
    CameraController *cameraController = [CameraController getInstance];
    if([cameraController stopCapture]){
        ICameraManager::stopCapture();
        this->open = false;
        _closeing = false;
        return YOUME_SUCCESS;
    } else  {
        _closeing = false;
        return YOUME_ERROR_STOP_FAILED;
    }
}

YouMeErrorCode CameraManager::setCaptureProperty(float fFps, int nWidth, int nHeight)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    NSLog(@"%s(%f, %d, %d) is called.", __FUNCTION__, fFps, nWidth, nHeight);
    [[CameraController getInstance]setCapturePropertyWithFps:fFps Width:nWidth Height:nHeight];
    return YOUME_SUCCESS;
}

void CameraManager::setLocalVideoMirrorMode( YouMeVideoMirrorMode  mirrorMode )
{
    [[CameraController getInstance]setLocalVideoMirrorMode: mirrorMode];
}

YouMeErrorCode CameraManager::setCaptureFrontCameraEnable(bool enable)
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    NSLog(@"%s(%d) is called.", __FUNCTION__, enable?1:-1);
    [[CameraController getInstance] setCaptureFrontCameraEnable:enable];
    return YOUME_SUCCESS;
}

bool CameraManager::isCaptureFrontCameraEnable()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    return [[CameraController getInstance] isCaptureFrontCameraEnable];
}

void CameraManager::setCameraOpenStatus(bool status)
{
    this->open = status;
}

YouMeErrorCode CameraManager::switchCamera()
{
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(this->open) {
        if(![[CameraController getInstance] switchCamera]) {
            return YOUME_ERROR_UNKNOWN;
        } else {
            return YOUME_SUCCESS;
        }
    } else {
        return YOUME_ERROR_UNKNOWN;
    }
}

#if TARGET_OS_OSX
int CameraManager::getCameraCount()
{
    return [[CameraController getInstance] getCameraCount];
}

std::string CameraManager::getCameraName(int cameraId)
{
    NSString* name = [[CameraController getInstance] getCameraName:cameraId];
    return [name UTF8String];
}

YouMeErrorCode CameraManager::setOpenCameraId(int cameraId)
{
    return [[CameraController getInstance] setOpenCameraId:cameraId];
}
#endif //TARGET_OS_OSX

YouMeErrorCode CameraManager::videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID)
{
    if(!data)
        return YOUME_ERROR_INVALID_PARAM;
    std::lock_guard<std::recursive_mutex> stateLock(*mutex);
    if(YouMeVideoMixerAdapter::getInstance()->isSupportGLES() && renderflag){
        YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, data, len, width, height, fmt, rotation, mirror, timestamp);
    }
    else {
        
        FrameImage* frameImage = new FrameImage(width, height, data, len, mirror, timestamp, fmt);
        len = ICameraManager::format_transfer(frameImage, fmt);
        if (fmt != VIDEO_FMT_H264) {
            frameImage->len = len;
            frameImage->fmt = VIDEO_FMT_YUV420P;
        }
        
        if(rotation)
            ICameraManager::rotation_and_mirror(frameImage, rotation, tsk_false);
        
        if (renderflag) {
            YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocal(CNgnTalkManager::getInstance()->m_strUserID, frameImage->data, len, frameImage->width, frameImage->height, VIDEO_FMT_YUV420P, 0, mirror, timestamp);
            YouMeEngineVideoCodec::getInstance()->pushFrame(frameImage, true);
        } else {
            if (width != _encWidth || height != _encHeight) {
                // scale share video size to enc video resolution
                int scale_size = ICameraManager::scale(frameImage, _encWidth, _encHeight, 0);
                frameImage->len = scale_size;
            }

            frameImage->videoid = 2;    // share video stream type
            frameImage->double_stream = true; 
            YouMeEngineVideoCodec::getInstance()->pushFrameNew(frameImage);
        }

    }
    
    return YOUME_SUCCESS;
}

void *CameraManager::videoRotate(void* input_pixelbuffer, int rotation)
{
    CVPixelBufferRef pixelbuffer = (CVPixelBufferRef)input_pixelbuffer;
    
    int screenWidth = (int)CVPixelBufferGetWidth(pixelbuffer);
    int screenHeight = (int)CVPixelBufferGetHeight(pixelbuffer);
    
    unsigned int encWidth = 0, encHeight = 0;
    tmedia_defaults_get_video_size_share(&encWidth, &encHeight);
    
    // create new pixelbuffer
    CVPixelBufferRef newPixcelBuffer = nil;
    CVPixelBufferCreate(kCFAllocatorDefault, encWidth, encHeight, kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, optionsDictionary, &newPixcelBuffer);

    CVPixelBufferLockBaseAddress(pixelbuffer, kCVPixelBufferLock_ReadOnly);
@autoreleasepool
{
    NSDictionary *opt = @{
        (id)kCVPixelBufferBytesPerRowAlignmentKey : @(32),
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange),
        (id)kCVPixelBufferIOSurfacePropertiesKey : @{},
    };
    CIImage *ciimage = [CIImage imageWithCVPixelBuffer:pixelbuffer options:opt];

    float widthScale = 1.0, heightScale = 1.0;
    if (kCGImagePropertyOrientationRight == rotation || kCGImagePropertyOrientationLeft == rotation) {
        heightScale = encHeight * 1.0 / screenWidth;
        widthScale = encWidth * 1.0 / screenHeight;
    } else {
        heightScale = encHeight * 1.0 / screenHeight;
        widthScale = encWidth * 1.0 / screenWidth;
    }

    // rotate and scale
    CIImage *wImage  = [ciimage imageByApplyingOrientation:rotation];
    CIImage *newImage = [wImage imageByApplyingTransform:CGAffineTransformMakeScale(widthScale, heightScale)];
    [_ciContext render:newImage toCVPixelBuffer:newPixcelBuffer];
}

    CVPixelBufferUnlockBaseAddress((CVPixelBufferRef)pixelbuffer, kCVPixelBufferLock_ReadOnly);
    
    return newPixcelBuffer;
}

YouMeErrorCode CameraManager::videoDataOutputGLESForIOS(void* inputPixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag)
{
    if(!inputPixelbuffer)
        return YOUME_ERROR_INVALID_PARAM;

    CVPixelBufferRef pixelbuffer;
    if(!renderflag)
    {
        int rotateType = kCGImagePropertyOrientationUp;
        switch (rotation) {
            case 90:
                rotateType = kCGImagePropertyOrientationRight;
                break;
            case 180:
                rotateType = kCGImagePropertyOrientationDown;
                break;
            case 270:
                rotateType = kCGImagePropertyOrientationLeft;
                break;
            default:
                break;
        }
        pixelbuffer = (CVPixelBufferRef)videoRotate(inputPixelbuffer, rotateType);
        rotation = 0;
        
        // copy to backup, send when no frame input
#if TARGET_OS_IPHONE
        _lastPixelRef = NULL;
        if (_isCacheLastPixel) {
           _lastPixelRef = copyPixelbuffer(pixelbuffer);
        }
#endif
        
        if (!pixelbuffer) {
            return YOUME_ERROR_INVALID_PARAM;
        }
        //int pixelFmt = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
    }else{
        pixelbuffer = (CVPixelBufferRef)inputPixelbuffer;
    }
    
    YouMeErrorCode ret = YOUME_SUCCESS;
    do {
        if(YouMeVideoMixerAdapter::getInstance()->isSupportGLES()) {
            if(!renderflag) {
                // 这里是共享视频 需要带上共享流 videoid:2
                int ret = YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameGLES(pixelbuffer, CVPixelBufferGetWidth(pixelbuffer), CVPixelBufferGetHeight(pixelbuffer), 2, timestamp);
                if (ret == 1) {
                    // 若硬编码打开失败，则切换到软编码 尽量保证视频ios共享正常
                    YouMeVideoMixerAdapter::getInstance()->resetMixer(false);
                }
            } else {
                YouMeVideoMixerAdapter::getInstance()->pushVideoFrameLocalForIOS(CNgnTalkManager::getInstance()->m_strUserID, pixelbuffer, width, height, fmt, rotation, mirror, timestamp);
            }
        } else {
            CVPixelBufferRef videoFrame = (CVPixelBufferRef)pixelbuffer;
            CVPixelBufferLockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
            uint8_t* baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(videoFrame);
            if(!baseAddress)
            {
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_MEMORY_OUT;
                break;
            }
            
            size_t videowidth = CVPixelBufferGetWidth(videoFrame);
            size_t videoheight = CVPixelBufferGetHeight(videoFrame);
            size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
            int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
            if(pixelFormat != kCVPixelFormatType_32BGRA && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
               && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange){
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_INVALID_PARAM;
                break;
            }
            
            size_t bufferSize = 0;
            if (kCVPixelFormatType_32BGRA == pixelFormat) {
                bufferSize = CVPixelBufferGetDataSize(videoFrame);
            } else {
                bufferSize = videowidth * videoheight * 3 / 2;
            }
            
            if (_videoDataSize < bufferSize) {
                if(_videoData)
                    delete [] _videoData;
                _videoData = new uint8_t[bufferSize];
                _videoDataSize = bufferSize;
            }
            if(!_videoData){
                CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
                ret = YOUME_ERROR_MEMORY_OUT;
                break;
            }
            
            int fmt = VIDEO_FMT_BGRA32;
            if (kCVPixelFormatType_32BGRA == pixelFormat) {
                if(bytesPerRow != videowidth*4 ||
                   bufferSize != bytesPerRow*videoheight)
                {
                    bufferSize = videoheight*videowidth*4;
                    for (int i = 0; i < videoheight; i++) {
                        memcpy(&_videoData[i*videowidth*4], &baseAddress[i*bytesPerRow], videowidth*4);
                    }
                }
                else{
                    memcpy(_videoData, baseAddress, bufferSize);
                }
                
                fmt = VIDEO_FMT_BGRA32;
            } else {
                size_t y_size = videowidth * videoheight;
                size_t uv_size = y_size / 2;
                
                //y plane
                uint8_t *y_Buffer = (uint8_t*) CVPixelBufferGetBaseAddressOfPlane( videoFrame , 0 );;
                int src_y_stride = CVPixelBufferGetBytesPerRowOfPlane(videoFrame, 0);
                
                if (videowidth != src_y_stride) {
                    for (int i = 0; i < videoheight; i++) {
                        memcpy(&_videoData[i*videowidth], &y_Buffer[i*src_y_stride], videowidth);
                    }
                } else {
                    memcpy( _videoData,   y_Buffer,  y_size );
                }
                
                //uv_plane
                uint8_t * uv_Buffer = (uint8_t*) CVPixelBufferGetBaseAddressOfPlane( videoFrame, 1 );
                int src_uv_stride = CVPixelBufferGetBytesPerRowOfPlane(videoFrame, 1);
                
                uint8_t * _uData = (_videoData + y_size);
                uint8_t * _vData = (_uData + uv_size/2);
                int index = 0;
                if (videowidth != src_uv_stride) {
                    for (int i = 0; i < videoheight/2; ++i) {
                        //memcpy(&_videoData[i*videowidth + y_size], &uv_Buffer[i*src_uv_stride], videowidth);
                        for (index = 0; index < videowidth/2; ++index) {
                            *(_uData + i*videowidth/2 + index) = *(uv_Buffer + i*src_uv_stride + index * 2);
                            *(_vData + i*videowidth/2 + index) = *(uv_Buffer + i*src_uv_stride + index * 2 + 1);
                        }
                    }
                } else {
                    //memcpy( _videoData + y_size, uv_Buffer, uv_size );
                    for (index = 0; index < (uv_size/2); ++index) {
                        *(_uData + index) = *(uv_Buffer + index * 2);
                        *(_vData + index) = *(uv_Buffer + index * 2 + 1);
                    }
                }
                fmt = VIDEO_FMT_YUV420P;
            }
            CVPixelBufferUnlockBaseAddress(videoFrame, kCVPixelBufferLock_ReadOnly);
            
            if (renderflag) {
                videoDataOutput(_videoData, bufferSize, videowidth, videoheight, fmt, rotation, mirror, timestamp, 1);
            } else {
                YouMeVideoMixerAdapter::getInstance()->pushVideoPreviewFrameNew(_videoData, bufferSize, videowidth, videoheight, 2, timestamp, fmt);
            }
        }
    } while(0);
    
    if(!renderflag)
    {
        //iOS APP EXTENSION模式运行不需要渲染
        CVPixelBufferRelease(pixelbuffer);
    }
    return ret;
}

void CameraManager::openBeautify(bool open)
{
    [[CameraController getInstance] openBeautify:open];
}
void CameraManager::beautifyChanged(float param)
{
    [[CameraController getInstance] beautifyChanged:param];
}
void CameraManager::stretchFace(bool stretch)
{
    [[CameraController getInstance] stretchFace:stretch];
}
 #if TARGET_OS_IPHONE
bool CameraManager::isCameraZoomSupported()
{
    return [[CameraController getInstance] isCameraZoomSupported];
}
float CameraManager::setCameraZoomFactor(float zoomFactor)
{
    return [[CameraController getInstance] setCameraZoomFactor: zoomFactor];
}
bool CameraManager::isCameraFocusPositionInPreviewSupported()
{
    return [[CameraController getInstance] isCameraFocusPositionInPreviewSupported];
}
bool CameraManager::setCameraFocusPositionInPreview( float x , float y )
{
    return [[CameraController getInstance] setCameraFocusPositionInPreview: CGPointMake(x , y )];
}
bool CameraManager::isCameraTorchSupported()
{
    return [[CameraController getInstance] isCameraTorchSupported];
}
bool CameraManager::setCameraTorchOn( bool isOn )
{
    return [[CameraController getInstance] setCameraTorchOn: isOn];
}
bool CameraManager::isCameraAutoFocusFaceModeSupported()
{
    return [[CameraController getInstance]isCameraAutoFocusFaceModeSupported];
}
bool CameraManager::setCameraAutoFocusFaceModeEnabled( bool enable )
{
    return [[CameraController getInstance]setCameraAutoFocusFaceModeEnabled:enable];
}

void CameraManager::decreaseBufferRef(void *pixelbuffer)
{
    if (pixelbuffer) {
        CVPixelBufferRelease((CVBufferRef)pixelbuffer);
    }
}

void CameraManager::lockBufferRef(void * pixelbuffer)
{
    if (pixelbuffer) {
        CVPixelBufferLockBaseAddress((CVBufferRef)pixelbuffer, kCVPixelBufferLock_ReadOnly);
    }
}

void CameraManager::unlockBufferRef(void *pixelbuffer)
{
    if (pixelbuffer) {
        CVPixelBufferUnlockBaseAddress((CVBufferRef)pixelbuffer, kCVPixelBufferLock_ReadOnly);
    }
}

void *CameraManager::copyPixelbuffer(void* pixelbuffer)
{
    if (!pixelbuffer) {
        return NULL;
    }
    
    CVBufferRef pixelBuffer = (CVBufferRef)pixelbuffer;
    // Get pixel buffer info 
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    // 目前仅支持yuv格式的pixelbuf
    OSType pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);//kCVPixelFormatType_420YpCbCr8BiPlanarFullRange; // Copy the pixel buffer
    if (pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarFullRange \
        && pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange ) {
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        return NULL;
    }
        
    int bufferWidth = (int)CVPixelBufferGetWidth(pixelBuffer);
    int bufferHeight = (int)CVPixelBufferGetHeight(pixelBuffer);
    
    CVPixelBufferRef pixelBufferCopy;
    
@autoreleasepool
{
    NSDictionary *pixelAttributes = @{
                    (id)kCVPixelBufferBytesPerRowAlignmentKey : @(32)
                };
    
    CVPixelBufferCreate(kCFAllocatorDefault, bufferWidth, bufferHeight, pixelFormat, (__bridge CFDictionaryRef)(pixelAttributes), &pixelBufferCopy);
}
    CVPixelBufferLockBaseAddress(pixelBufferCopy, 0);

    int src_y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
    uint8_t *yDestPlane = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBufferCopy, 0); //YUV
    uint8_t *yPlane = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0);
    memcpy( yDestPlane, yPlane, src_y_stride * CVPixelBufferGetHeightOfPlane(pixelBuffer, 0) );
    
    int src_uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1);
    uint8_t *uvDestPlane = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBufferCopy, 1);
    uint8_t *uvPlane = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1);
    memcpy(uvDestPlane, uvPlane, src_uv_stride * CVPixelBufferGetHeightOfPlane(pixelBuffer, 1));
    
    CVPixelBufferUnlockBaseAddress(pixelBufferCopy, 0);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    return pixelBufferCopy; 
}

void CameraManager::setCacheLastPixelEnable(bool enable) {
    _isCacheLastPixel = enable;
}

void *CameraManager::getLastPixelRef() {
    return _lastPixelRef;
}

int64_t CameraManager::usedMemory()
{
    int64_t memoryUsageInByte = 0;
    task_vm_info_data_t vmInfo;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    kern_return_t kernelReturn = task_info(mach_task_self(), TASK_VM_INFO, (task_info_t) &vmInfo, &count);
    if(kernelReturn == KERN_SUCCESS) {
        memoryUsageInByte = (int64_t) vmInfo.phys_footprint;
        //NSLog(@"Memory in use (in bytes): %lld", memoryUsageInByte);
    } else {
        //NSLog(@"Error with task_info(): %s", mach_error_string(kernelReturn));
    }
    return memoryUsageInByte / 1048576; //convert to MB
}

bool CameraManager::isUnderAppExtension() {
    return _underAppExtension;
}
#endif

void CameraManager::setShareVideoEncResolution(int width, int height)
{
    _encWidth = width;
    _encHeight = height;
}

bool CameraManager::checkScreenCapturePermission()
{
     return [[CameraController getInstance] checkScreenCapturePermission];
}

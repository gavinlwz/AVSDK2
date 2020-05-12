//
//  CameraManager.cpp
//  youme_voice_engine
//
//  Created by 游密 on 2017/4/20.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "tsk_memory.h"
#include "tmedia_utils.h"
#include "ICameraManager.hpp"
#include "CameraManager.h"
#include <YouMeCommon/yuvlib/libyuv.h>
#include "TLSMemory.h"

#define CLAMP(i)   (((i) < 0) ? 0 : (((i) > 255) ? 255 : (i)))

ICameraManager *ICameraManager::sInstance = NULL;
std::recursive_mutex* g_mutex = new std::recursive_mutex();

ICameraManager* ICameraManager::getInstance()
{
    std::lock_guard<std::recursive_mutex> stateLock(*g_mutex);
    if (NULL == sInstance)
    {
        sInstance = CameraManager::getInstance();
    }
    return sInstance;
}

ICameraManager::ICameraManager()
{
    duration = 0;
    start_timestamp = 0;
    open = false;
    mutex = new std::recursive_mutex();
}

ICameraManager::~ICameraManager() {
    delete mutex;
}

YouMeErrorCode ICameraManager::startCapture() {
    start_timestamp = tsk_gettimeofday_ms();
    return YOUME_SUCCESS;
}

YouMeErrorCode ICameraManager::stopCapture() {
    duration += (tsk_gettimeofday_ms() - start_timestamp);
    start_timestamp = 0;
    return YOUME_SUCCESS;
}

// add parameter renderflag: 0: no need local render, 1: meed local render
YouMeErrorCode ICameraManager::videoDataOutput(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag, int streamID){
    return YOUME_SUCCESS;
}

YouMeErrorCode ICameraManager::videoDataOutputGLESForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag){
    return YOUME_SUCCESS;
}

#if ANDROID
YouMeErrorCode ICameraManager::videoDataOutputGLESForAndroid(int texture, float* matrix,int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp, int renderflag){
    return YOUME_SUCCESS;
}
#endif

uint64_t ICameraManager::getDuration() {
    return duration;
}

uint64_t ICameraManager::resetDuration() {
    duration = 0;
    start_timestamp = 0;
    return 0;
}

int ICameraManager::transfer_nv12_to_yuv420(FrameImage* frameImage) {
    int w = frameImage->width;
    int h = frameImage->height;
    int yLen = w * h;
    int vLen = yLen / 4;
    int index = 0;
    uint8_t *data = (uint8_t*)frameImage->data + yLen;
	int iSlot = -1;
	uint8_t *vData = (uint8_t*)CTLSMemory::GetInstance()->Allocate(w * h / 4, iSlot);
    
    for (index = 0; index < vLen; index++) {
        *(vData + index) = *(data + index * 2 + 1);
        *(data + index) = *(data + index * 2);
    }
    memcpy(data + vLen, vData, vLen);
	CTLSMemory::GetInstance()->Free(iSlot);
    return w * h * 3 / 2;
}

int ICameraManager::transfer_yuv_to_yuv420(FrameImage* frameImage) {
    int width = frameImage->width;
    int height = frameImage->height;
    int yuvBufSize = width * height * 3 / 2;
    int iSlot = -1;
    uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, iSlot);
    
    int src_stride_u = (width+1) / 2;
    memcpy(yuvBuf, frameImage->data, yuvBufSize);

    //Y    
    //U
    uint8_t* pOffset = yuvBuf+(width*height);
    uint8_t*  pU = (uint8_t*)frameImage->data +  width*height;
    for(int h=0; h<height/2; h++)
    {            
        memcpy(pU, pOffset+(h*width), src_stride_u);
        pU += src_stride_u;
    }

    //V
    uint8_t*  pV = pU;
    pOffset = yuvBuf + (width*height + (width+1)/2);
    for(int h=0; h<height/2; h++)
    {            
        memcpy(pV, pOffset+(h*width), src_stride_u);
        pV += src_stride_u;
    }

    if(yuvBuf) {
        CTLSMemory::GetInstance()->Free(iSlot);
    }
    return yuvBufSize;
}

int ICameraManager::transfer_nv21_to_yuv420(FrameImage* frameImage) {
    int w = frameImage->width;
    int h = frameImage->height;
    int yLen = w * h;
    int vLen = yLen / 4;
    int index = 0;
    uint8_t *data = (uint8_t*)frameImage->data + yLen;
	int iSlot = -1;
	uint8_t *vData = (uint8_t*)CTLSMemory::GetInstance()->Allocate(w * h / 4, iSlot);
    
    for (index = 0; index < vLen; index++) {
        *(vData + index) = *(data + index * 2);
        *(data + index) = *(data + index * 2 + 1);
    }
    memcpy(data + vLen, vData, vLen);
	CTLSMemory::GetInstance()->Free(iSlot);
    return w * h * 3 / 2;
}

int ICameraManager::transfer_yv12_to_yuv420(FrameImage* frameImage) {
    int w = frameImage->width;
    int h = frameImage->height;
    int yLen = w * h;
    int vLen = yLen / 4;
    uint8_t *data = (uint8_t*)frameImage->data + yLen;
	int iSlot = -1;
	uint8_t *vData = (uint8_t*)CTLSMemory::GetInstance()->Allocate(w * h / 4, iSlot);
    
    memcpy(vData, data, vLen);
    memcpy(data, data + vLen, vLen);
    memcpy(data + vLen, vData, vLen);
	CTLSMemory::GetInstance()->Free(iSlot);
    return w * h * 3 / 2;
}

int ICameraManager::transfer_32bgra_to_yuv420(FrameImage* frameImage) {

    int width = frameImage->width;
    int height = frameImage->height;
    int yuvBufSize = width * height * 3 / 2;
    int yuvSlot = -1;
	uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, yuvSlot);
    const int y_length = width * height;
    const int uv_stride = (width+1) / 2;
    int uv_length = uv_stride * ((height+1) / 2);
    unsigned char *Y_data_Dst_rotate = yuvBuf;
    unsigned char *U_data_Dst_rotate = yuvBuf + y_length;
    unsigned char *V_data_Dst_rotate = U_data_Dst_rotate + uv_length;
    int Dst_Stride_Y = width;
    YOUME_libyuv::ARGBToI420(
            (const uint8*)frameImage->data,
            width*4,
            Y_data_Dst_rotate, Dst_Stride_Y,
            U_data_Dst_rotate, uv_stride,
            V_data_Dst_rotate, uv_stride,
            width, height);

    memcpy(frameImage->data, yuvBuf, yuvBufSize);
    if(yuvBuf) {
		CTLSMemory::GetInstance()->Free(yuvSlot);
    }
    return yuvBufSize;
}

int ICameraManager::transfer_32rgba_to_yuv420(FrameImage* frameImage) {
    
    int width = frameImage->width;
    int height = frameImage->height;
    int yuvBufSize = width * height * 3 / 2;
    int iSlot = -1;
    uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, iSlot);
    
    const int y_length = width * height;
    const int32 uv_stride = (width+1) / 2;
    int uv_length = uv_stride * ((height+1) / 2);
    unsigned char *Y_data_Dst_rotate = yuvBuf;
    unsigned char *U_data_Dst_rotate = yuvBuf + y_length;
    unsigned char *V_data_Dst_rotate = U_data_Dst_rotate + uv_length;
    int Dst_Stride_Y = width;
    //没有实现？
    YOUME_libyuv::BGRAToI420((const uint8*)frameImage->data,
                             width*4,
                             Y_data_Dst_rotate, Dst_Stride_Y,
                             U_data_Dst_rotate, uv_stride,
                             V_data_Dst_rotate, uv_stride,
                             width, height);
    
    memcpy(frameImage->data, yuvBuf, yuvBufSize);
    if(yuvBuf) {
        CTLSMemory::GetInstance()->Free(iSlot);
    }
    return yuvBufSize;
}

int ICameraManager::transfer_32abgr_to_yuv420(FrameImage* frameImage) {
    
    int width = frameImage->width;
    int height = frameImage->height;
    int yuvBufSize = width * height * 3 / 2;
    int iSlot = -1;
    uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, iSlot);
    
    const int y_length = width * height;
    const int32 uv_stride = (width+1) / 2;
    int uv_length = uv_stride * ((height+1) / 2);
    unsigned char *Y_data_Dst_rotate = yuvBuf;
    unsigned char *U_data_Dst_rotate = yuvBuf + y_length;
    unsigned char *V_data_Dst_rotate = U_data_Dst_rotate + uv_length;
    int Dst_Stride_Y = width;
    //没有实现？
    YOUME_libyuv::ABGRToI420((const uint8*)frameImage->data,
                             width*4,
                             Y_data_Dst_rotate, Dst_Stride_Y,
                             U_data_Dst_rotate, uv_stride,
                             V_data_Dst_rotate, uv_stride,
                             width, height);
    
    memcpy(frameImage->data, yuvBuf, yuvBufSize);
    if(yuvBuf) {
        CTLSMemory::GetInstance()->Free(iSlot);
    }
    return yuvBufSize;
}

int ICameraManager::transfer_rgb24_to_yuv420(FrameImage* frameImage) {

	int width = frameImage->width;
	int height = frameImage->height;
	int yuvBufSize = width * height * 3 / 2;
	int iSlot = -1;
	uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, iSlot);

	const int y_length = width * height;
	const int32 uv_stride = (width + 1) / 2;
	int uv_length = uv_stride * ((height + 1) / 2);
	unsigned char *Y_data_Dst_rotate = yuvBuf;
	unsigned char *U_data_Dst_rotate = yuvBuf + y_length;
	unsigned char *V_data_Dst_rotate = U_data_Dst_rotate + uv_length;

	int Dst_Stride_Y = width;
	YOUME_libyuv::RGB24ToI420((const uint8*)frameImage->data,
		width * 3,
		Y_data_Dst_rotate, Dst_Stride_Y,
		U_data_Dst_rotate, uv_stride,
		V_data_Dst_rotate, uv_stride,
		width, height);

	memcpy(frameImage->data, yuvBuf, yuvBufSize);
	if (yuvBuf) {
		CTLSMemory::GetInstance()->Free(iSlot);
	}
	return yuvBufSize;
}

int ICameraManager::format_transfer(FrameImage* frameImage, int videoFmt) {
    int size = 0;
    if (!frameImage) {
        return size;
    }
    switch (videoFmt) {
        case VIDEO_FMT_NV21:
            size = transfer_nv21_to_yuv420(frameImage);
            break;
        case VIDEO_FMT_YV12:
            size = transfer_yv12_to_yuv420(frameImage);
            break;
        case VIDEO_FMT_BGRA32:
            size = transfer_32bgra_to_yuv420(frameImage);
            break;
		case VIDEO_FMT_RGBA32:
			size = transfer_32rgba_to_yuv420(frameImage);
			break;
		case VIDEO_FMT_ABGR32:
			size = transfer_32abgr_to_yuv420(frameImage);
			break;
        case VIDEO_FMT_RGB24:
            size = transfer_rgb24_to_yuv420(frameImage);
            break;
        case VIDEO_FMT_NV12:
            size = transfer_nv12_to_yuv420(frameImage);
            break;
        case VIDEO_FMT_TEXTURE_YUV:
            size = transfer_yuv_to_yuv420(frameImage);
            break;
        case VIDEO_FMT_H264:
        case VIDEO_FMT_ENCRYPT:
            // do nothing
            size = frameImage->len;
            break;
        case VIDEO_FMT_YUV420P:
        default:
            size = frameImage->height * frameImage->width * 3 / 2;
            break;
    }
    return size;
}


void ICameraManager::rotation_and_mirror(FrameImage* frameImage, int degree, bool need_mirror) {
    //TSK_DEBUG_ERROR("-----------chengjlchengjl--%d %d degree:%d, need_mirror:%d", frameImage->width, frameImage->height, degree,need_mirror );
    switch(degree) {
        case 90:
        case 180:
        case 270:
        {
            int src_width  = frameImage->width;
            int src_height = frameImage->height;
            
            //copy origin data
            unsigned char *  origdata = NULL;
            unsigned char * Dst_data = (unsigned char *)(frameImage->data);
            int size = (frameImage->width) * (frameImage->height) * 3 / 2;
           // origdata = (unsigned char *)tsk_calloc(1, size);
			int iSlot = -1;
			origdata = (unsigned char *)CTLSMemory::GetInstance()->Allocate(size, iSlot);
            memcpy(origdata, Dst_data, size * sizeof(unsigned char));
            
            //YUV420 image size
            int I420_Y_Size = src_width * src_height;
            int I420_U_Size = src_width * src_height / 4 ;
            
            unsigned char *Y_data_src = NULL;
            unsigned char *U_data_src = NULL;
            unsigned char *V_data_src = NULL;
            
            int Dst_Stride_Y_rotate;
            int Dst_Stride_U_rotate;
            int Dst_Stride_V_rotate;
            int rotate_width;
            int rotate_height;
            
            int Dst_Stride_Y = src_width;
            int Dst_Stride_U = src_width >> 1;
            int Dst_Stride_V = Dst_Stride_U;
            
            //最终写入目标
            unsigned char *Y_data_Dst = Dst_data;
            unsigned char *U_data_Dst = Dst_data + I420_Y_Size;
            unsigned char *V_data_Dst = Dst_data + I420_Y_Size + I420_U_Size;
            
            if (degree == YOUME_libyuv::kRotate90 || degree == YOUME_libyuv::kRotate270) {
                Dst_Stride_Y_rotate = src_height;
                Dst_Stride_U_rotate = (src_height+1) >> 1;
                Dst_Stride_V_rotate = Dst_Stride_U_rotate;
                rotate_width        = src_height;
                rotate_height       = src_width;
            }
            else {
                Dst_Stride_Y_rotate = src_width;
                Dst_Stride_U_rotate = (src_width+1) >> 1;
                Dst_Stride_V_rotate = Dst_Stride_U_rotate;
                rotate_width        = src_width;
                rotate_height       = src_height;
            }
            
            //mirror
            if (need_mirror) {
                Y_data_src = Dst_data;
                U_data_src = Dst_data + I420_Y_Size;
                V_data_src = Dst_data + I420_Y_Size + I420_U_Size;
                
                unsigned char *Y_data_Dst_rotate = origdata;
                unsigned char *U_data_Dst_rotate = origdata + I420_Y_Size;
                unsigned char *V_data_Dst_rotate = origdata + I420_Y_Size + I420_U_Size;

                YOUME_libyuv::I420Rotate(Y_data_src, Dst_Stride_Y,
                                         U_data_src, Dst_Stride_U,
                                         V_data_src, Dst_Stride_V,
                                         Y_data_Dst_rotate, Dst_Stride_Y_rotate,
                                         U_data_Dst_rotate, Dst_Stride_U_rotate,
                                         V_data_Dst_rotate, Dst_Stride_V_rotate,
                                         src_width, src_height,
                                         (YOUME_libyuv::RotationMode) degree);
                YOUME_libyuv::I420Mirror(Y_data_Dst_rotate, Dst_Stride_Y_rotate,
                                         U_data_Dst_rotate, Dst_Stride_U_rotate,
                                         V_data_Dst_rotate, Dst_Stride_V_rotate,
                                         Y_data_Dst, Dst_Stride_Y_rotate,
                                         U_data_Dst, Dst_Stride_U_rotate,
                                         V_data_Dst, Dst_Stride_V_rotate,
                                         rotate_width, rotate_height);
                
            } else {
                
                Y_data_src = origdata;
                U_data_src = origdata + I420_Y_Size;
                V_data_src = origdata + I420_Y_Size + I420_U_Size;
                
                YOUME_libyuv::I420Rotate(Y_data_src, Dst_Stride_Y,
                                         U_data_src, Dst_Stride_U,
                                         V_data_src, Dst_Stride_V,
                                         Y_data_Dst, Dst_Stride_Y_rotate,
                                         U_data_Dst, Dst_Stride_U_rotate,
                                         V_data_Dst, Dst_Stride_V_rotate,
                                         src_width, src_height,
                                         (YOUME_libyuv::RotationMode) degree);
            }
            
            if (degree == YOUME_libyuv::kRotate90 || degree == YOUME_libyuv::kRotate270){
                frameImage->width = src_height;
                frameImage->height = src_width;
            }
			CTLSMemory::GetInstance()->Free(iSlot);
            break;
        }
        case 0:
        {
            //mirror
            if (need_mirror) {
                mirror(frameImage);
            }
            break;
        }
    }
    
}

void ICameraManager::transpose(FrameImage* frameImage) {
    if (NULL == frameImage) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return;
    }

    uint8_t* origdata = NULL;
    uint8_t* yuvdata = (uint8_t*)(frameImage->data);
    int size = (frameImage->width) * (frameImage->height) * 3 / 2;
    //origdata = (uint8_t*)tsk_calloc(1, size);
	int iSlot = -1;
	origdata = (unsigned char *)CTLSMemory::GetInstance()->Allocate(size,iSlot);
    memcpy(origdata, frameImage->data, size);

    int h = frameImage->width;
    int w = frameImage->height;

    int i, j;
    uint8_t* dest;
    uint8_t* src;

    //Yplane
    dest = (uint8_t*)yuvdata;
    src = (uint8_t*)origdata;
    for(i = 0; i < h; i++) {
        for(j = 0; j < w; j++) {
            dest[i * w + j] = src[j * h + i];
        }
    }

    //Uplane
    dest = (uint8_t*)(yuvdata + (w * h));
    src = origdata + (w * h);
    for(i = 0; i < h/2; i++) {
        for(j = 0; j < w/2; j++) {
            dest[i * w / 2 + j] = src[j * h / 2 + i];
        }
    }

    //Vplane
    dest = (uint8_t*)(yuvdata + (w * h * 5 / 4));
    src = origdata + (w * h * 5 / 4);
    for(i = 0; i < h/2; i++) {
        for(j = 0; j < w/2; j++) {
            dest[i * w / 2 + j] = src[j * h / 2 + i];
        }
    }

    //TSK_DEBUG_ERROR("---chengjlchengjl---(%d-%d) (%d-%d)", frameImage->width, frameImage->height, w, h);

    frameImage->width = w;
    frameImage->height = h;
   // tsk_free((void**)&origdata);
	CTLSMemory::GetInstance()->Free(iSlot);
}

void ICameraManager::mirror(FrameImage* frameImage) {
    if (NULL == frameImage) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return;
    }

    int src_width  = frameImage->width;
    int src_height = frameImage->height;

    //copy origin data
    unsigned char *  origdata = NULL;
    unsigned char * Dst_data = (unsigned char *)(frameImage->data);
    int size = (frameImage->width) * (frameImage->height) * 3 / 2;
	int iSlot = -1;
	origdata = origdata = (unsigned char *)CTLSMemory::GetInstance()->Allocate(size, iSlot);
    memcpy(origdata, frameImage->data, size);

    //YUV420 image size
    int I420_Y_Size = src_width * src_height;
    int I420_U_Size = (src_width >> 1) * (src_height >> 1);
//    int I420_V_Size = I420_U_Size;

    unsigned char *Y_data_src = origdata;
    unsigned char *U_data_src = origdata + I420_Y_Size ;
    unsigned char *V_data_src = origdata + I420_Y_Size + I420_U_Size;


    int Src_Stride_Y = src_width;
    int Src_Stride_U = (src_width+1) >> 1;
    int Src_Stride_V = Src_Stride_U;

    //最终写入目标
    unsigned char *Y_data_Dst_rotate = Dst_data;
    unsigned char *U_data_Dst_rotate = Dst_data + I420_Y_Size;
    unsigned char *V_data_Dst_rotate = Dst_data + I420_Y_Size + I420_U_Size;

    //mirro
    int Dst_Stride_Y_mirror = src_width;
    int Dst_Stride_U_mirror = (src_width+1) >> 1;
    int Dst_Stride_V_mirror = Dst_Stride_U_mirror;
    YOUME_libyuv::I420Mirror(Y_data_src, Src_Stride_Y,
            U_data_src, Src_Stride_U,
            V_data_src, Src_Stride_V,
            Y_data_Dst_rotate, Dst_Stride_Y_mirror,
            U_data_Dst_rotate, Dst_Stride_U_mirror,
            V_data_Dst_rotate, Dst_Stride_V_mirror,
            src_width, src_height);

	CTLSMemory::GetInstance()->Free(iSlot);
}

void ICameraManager::mirror(uint8_t* src_data,
            int src_width,
            int src_height){
    if (NULL == src_data) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return;
    }
    
    //copy origin data
    unsigned char *  origdata = NULL;
    unsigned char * Dst_data = (unsigned char *)(src_data);
    int size = src_width * src_height * 3 / 2;
    int iSlot = -1;
    origdata = origdata = (unsigned char *)CTLSMemory::GetInstance()->Allocate(size, iSlot);
    memcpy(origdata, src_data, size);
    
    //YUV420 image size
    int I420_Y_Size = src_width * src_height;
    int I420_U_Size = (src_width >> 1) * (src_height >> 1);
    //    int I420_V_Size = I420_U_Size;
    
    unsigned char *Y_data_src = origdata;
    unsigned char *U_data_src = origdata + I420_Y_Size ;
    unsigned char *V_data_src = origdata + I420_Y_Size + I420_U_Size;
    
    
    int Src_Stride_Y = src_width;
    int Src_Stride_U = (src_width+1) >> 1;
    int Src_Stride_V = Src_Stride_U;
    
    //最终写入目标
    unsigned char *Y_data_Dst_rotate = Dst_data;
    unsigned char *U_data_Dst_rotate = Dst_data + I420_Y_Size;
    unsigned char *V_data_Dst_rotate = Dst_data + I420_Y_Size + I420_U_Size;
    
    //mirro
    int Dst_Stride_Y_mirror = src_width;
    int Dst_Stride_U_mirror = (src_width+1) >> 1;
    int Dst_Stride_V_mirror = Dst_Stride_U_mirror;
    YOUME_libyuv::I420Mirror(Y_data_src, Src_Stride_Y,
                             U_data_src, Src_Stride_U,
                             V_data_src, Src_Stride_V,
                             Y_data_Dst_rotate, Dst_Stride_Y_mirror,
                             U_data_Dst_rotate, Dst_Stride_U_mirror,
                             V_data_Dst_rotate, Dst_Stride_V_mirror,
                             src_width, src_height);
    
    CTLSMemory::GetInstance()->Free(iSlot);
}

std::shared_ptr<FrameImage> ICameraManager::scale(std::shared_ptr<FrameImage> src, int out_width, int out_height, int mode) {
    // mode : fitXY、fitCenter、centerCrop、centerInside...
    // default mode is centerCrop
    std::shared_ptr<FrameImage> scaledFrame = video_scale_yuv_zoom(src, out_width, out_height);

    return scaledFrame;

}

std::shared_ptr<FrameImage> ICameraManager::video_scale_yuv_zoom(std::shared_ptr<FrameImage> src, int out_width ,int out_height) {
    //////////////////////////////////////////////////////////////////////////
    std::shared_ptr<FrameImage> dest = std::shared_ptr<FrameImage>(new FrameImage(out_width, out_height));
    int src_width_ = src->width;
    int src_height_= src->height;
    int dst_width_ = out_width;
    int dst_height_ = out_height;

    // Making sure that destination frame is of sufficient size.

    // We want to preserve aspect ratio instead of stretching the frame.
    // Therefore, we need to crop the source frame. Calculate the largest center
    // aligned region of the source frame that can be used.
//    const int cropped_src_width =
//            std::min(src_width_, dst_width_ * src_height_ / dst_height_);
//    const int cropped_src_height =
//            std::min(src_height_, dst_height_ * src_width_ / dst_width_);
    int cropped_src_width, cropped_src_height;
    if (dst_width_ * src_height_ > src_width_ * dst_height_) {
        cropped_src_width = src_width_;
        cropped_src_height = dst_height_*src_width_/(float)dst_width_;
    }
    else{
        cropped_src_height = src_height_;
        cropped_src_width = dst_width_*src_height_/(float)dst_height_;
    }
    // Make sure the offsets are even to avoid rounding errors for the U/V planes.
    const int src_offset_x = ((src_width_ - cropped_src_width) / 2) & ~1;
    const int src_offset_y = ((src_height_ - cropped_src_height) / 2) & ~1;

    //YUV420 image size
    int I420_Y_Size = src_width_ * src_height_;
    int I420_U_Size = src_width_ * src_height_ / 4;
    //    int I420_V_Size = I420_U_Size;

    unsigned char *Y_data_src = (unsigned char *) src->data;
    unsigned char *U_data_src = Y_data_src + I420_Y_Size;
    unsigned char *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;
    int src_stride_Y = src_width_;
    int src_stride_U = (src_width_+1) >> 1;
    int src_stride_V = src_stride_U;

    //最终写入目标
    int dest_I420_Y_Size = dst_width_ * dst_height_;
    int dest_I420_U_Size = dst_width_ * dst_height_ / 4;
    unsigned char *Y_data_dest = (unsigned char *) dest->data;
    unsigned char *U_data_dest = Y_data_dest + dest_I420_Y_Size;
    unsigned char *V_data_dest = Y_data_dest + dest_I420_Y_Size + dest_I420_U_Size;
    int dest_stride_Y = dst_width_;
    int dest_stride_U = (dst_width_+1) >> 1;
    int dest_stride_V = dest_stride_U;

    const uint8_t* y_ptr =
            Y_data_src +
                    src_offset_y * src_stride_Y +
                    src_offset_x;
    const uint8_t* u_ptr =
            U_data_src +
                    src_offset_y / 2 * src_stride_U +
                    src_offset_x / 2;
    const uint8_t* v_ptr =
            V_data_src +
                    src_offset_y / 2 * src_stride_V +
                    src_offset_x / 2;

    YOUME_libyuv::I420Scale(
            y_ptr,
            src_stride_Y,
            u_ptr,
            src_stride_U,
            v_ptr,
            src_stride_V,
            cropped_src_width, cropped_src_height,
            Y_data_dest,
            dest_stride_Y,
            U_data_dest,
            dest_stride_U,
            V_data_dest,
            dest_stride_V,
            dst_width_, dst_height_,
            YOUME_libyuv::kFilterLinear);

    return dest;
}

int ICameraManager::scale(FrameImage* frameImage, int out_width ,int out_height, int mode) {

    int yuvBufSize = out_width * out_height * 3 / 2;
    int iSlot = -1;
    uint8_t* yuvBuf = (uint8_t*)CTLSMemory::GetInstance()->Allocate(yuvBufSize, iSlot);

    scale((uint8_t*)frameImage->data, frameImage->width, frameImage->height,
            yuvBuf, out_width, out_height, mode);

    memcpy(frameImage->data, yuvBuf, yuvBufSize);
    if (yuvBuf) {
        CTLSMemory::GetInstance()->Free(iSlot);
    }
    frameImage->width = out_width;
    frameImage->height = out_height;
    
    return yuvBufSize;
}

void ICameraManager::scale(uint8_t* src_data,
                           int src_width,
                           int src_height,
                           uint8_t* out_data,
                           int out_width,
                           int out_height,
                           int mode) {
    // mode : fitXY、fitCenter、centerCrop、centerInside...
    // default mode is centerCrop

    uint8_t* src_data_ = src_data;
    int src_width_ = src_width;
    int src_height_= src_height;
    uint8_t* det_data_ = out_data;
    int dst_width_ = out_width;
    int dst_height_ = out_height;
    
    // Making sure that destination frame is of sufficient size.
    int cropped_src_width = 0, cropped_src_height = 0;
    if (dst_width_ * src_height_ >= src_width_ * dst_height_) {
        cropped_src_width = src_width_;
        cropped_src_height = dst_height_*src_width_/(float)dst_width_;
    }
    else{
        cropped_src_height = src_height_;
        cropped_src_width = dst_width_*src_height_/(float)dst_height_;
    }

    // Make sure the offsets are even to avoid rounding errors for the U/V planes.
    const int src_offset_x = ((src_width_ - cropped_src_width) / 2) & ~1;
    const int src_offset_y = ((src_height_ - cropped_src_height) / 2) & ~1;
    
    //YUV420 image size
    int I420_Y_Size = src_width_ * src_height_;
    int I420_U_Size = src_width_ * src_height_ / 4;
    //    int I420_V_Size = I420_U_Size;
    
    uint8_t *Y_data_src = src_data_;
    uint8_t *U_data_src = Y_data_src + I420_Y_Size;
    uint8_t *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;
    int src_stride_Y = src_width_;
    int src_stride_U = (src_width_+1) >> 1;
    int src_stride_V = src_stride_U;
    
    //最终写入目标
    int dest_I420_Y_Size = dst_width_ * dst_height_;
    int dest_I420_U_Size = dst_width_ * dst_height_ / 4;
    uint8_t *Y_data_dest = det_data_;
    uint8_t *U_data_dest = Y_data_dest + dest_I420_Y_Size;
    uint8_t *V_data_dest = Y_data_dest + dest_I420_Y_Size + dest_I420_U_Size;
    int dest_stride_Y = dst_width_;
    int dest_stride_U = (dst_width_+1) >> 1;
    int dest_stride_V = dest_stride_U;
    
    const uint8_t* y_ptr =
    Y_data_src +
    src_offset_y * src_stride_Y +
    src_offset_x;
    const uint8_t* u_ptr =
    U_data_src +
    src_offset_y / 2 * src_stride_U +
    src_offset_x / 2;
    const uint8_t* v_ptr =
    V_data_src +
    src_offset_y / 2 * src_stride_V +
    src_offset_x / 2;
    
    YOUME_libyuv::I420Scale(
                            y_ptr,
                            src_stride_Y,
                            u_ptr,
                            src_stride_U,
                            v_ptr,
                            src_stride_V,
                            cropped_src_width, cropped_src_height,
                            Y_data_dest,
                            dest_stride_Y,
                            U_data_dest,
                            dest_stride_U,
                            V_data_dest,
                            dest_stride_V,
                            dst_width_, dst_height_,
                            YOUME_libyuv::kFilterLinear);
    
    
}

std::shared_ptr<FrameImage> ICameraManager::crop(std::shared_ptr<FrameImage> src, int width, int height) {
    std::shared_ptr<FrameImage> dest = std::shared_ptr<FrameImage>(new FrameImage(width, height));
    
    uint8_t* ydest = (uint8_t*)dest->data;
    uint8_t* udest = ydest + dest->width * dest->height;
    uint8_t* vdest = ydest + dest->width * dest->height * 5 / 4;
    
    uint8_t* ysrc = (uint8_t*)src->data;;
    uint8_t* usrc = ysrc + src->width * src->height;;
    uint8_t* vsrc = ysrc + src->width * src->height * 5 / 4;;
    
    int x, y;
    x = (dest->width - src->width) / 2;
    y = (dest->height - src->height) / 2;
    // yplane
    for(int row = 0; row < src->height; row++) {
        for(int column = 0; column < src->width; column++) {
            if(((row + y) >= 0) && ((row + y) < dest->height) && ((column + x) >= 0) && ((column + x) < dest->width)) {
                ydest[(row + y) * dest->width + (column + x)] = ysrc[row * src->width + column];
            }
        }
    }

    // uplane
    for(int row = 0; row < src->height/2; row++) {
        for(int column = 0; column < src->width/2; column++) {
            if(((row + y/2) >= 0) && ((row + y/2) < dest->height/2) && ((column + x/2) >= 0) && ((column + x/2) < dest->width/2)) {
                udest[(row + y/2) * dest->width/2 + (column + x/2)] = usrc[row * src->width/2 + column];
            }
        }
    }

    // vplane
    for(int row = 0; row < src->height/2; row++) {
        for(int column = 0; column < src->width/2; column++) {
            if(((row + y/2) >= 0) && ((row + y/2) < dest->height/2) && ((column + x/2) >= 0) && ((column + x/2) < dest->width/2)) {
                vdest[(row + y/2) * dest->width/2 + (column + x/2)] = vsrc[row * src->width/2 + column];
            }
        }
    }
    
    return dest;
}

int ICameraManager::add(int a, int b) {
    return a+b;
}

FrameImage::FrameImage(int width, int height, void* data) {
    this->height = height;
    this->width = width;
    this->data = data;
    this->len = width * height * 3 / 2;
    this->fmt = VIDEO_FMT_YUV420P;
    mirror = 0;
}

FrameImage::FrameImage(int width, int height, void* data, int len, uint64_t timestamp, int fmt) {
    this->height = height;
    this->width = width;
    this->timestamp = timestamp;
    this->len = len;
    this->fmt = fmt;
    this->data = CTLSMemory::GetInstance()->Allocate(len, this->iSlot);
    memcpy(this->data, data, len);
    need_delete_data = true;
    mirror = 0;
}

FrameImage::FrameImage(int width, int height, void* data, int len, int mirror, uint64_t timestamp, int fmt) {
    this->height = height;
    this->width = width;
    this->timestamp = timestamp;
    this->len = len;
    this->fmt = fmt;
    this->data = CTLSMemory::GetInstance()->Allocate(len, this->iSlot);
    memcpy(this->data, data, len);
    need_delete_data = true;
    this->mirror = mirror;
}

FrameImage::FrameImage(int width, int height, void* data, int len, int mirror, int videoid, uint64_t timestamp, int fmt) {
    this->height = height;
    this->width = width;
    this->timestamp = timestamp;
    this->len = len;
    this->fmt = fmt;
    this->data = CTLSMemory::GetInstance()->Allocate(len, this->iSlot);
    memcpy(this->data, data, len);
    need_delete_data = true;
    this->mirror = mirror;
    this->videoid = videoid;
    this->double_stream = true;
}

FrameImage::FrameImage(int width, int height) {
    this->height = height;
    this->width = width;
    this->fmt = VIDEO_FMT_YUV420P;
    this->len = this->height * this->width * 3 / 2;
    this->data = CTLSMemory::GetInstance()->Allocate(this->len, this->iSlot);
    memset(this->data, 128, this->len);
    need_delete_data = true;
}

FrameImage::~FrameImage() {
    if (need_delete_data) {
        CTLSMemory::GetInstance()->Free(this->iSlot);
        //free(this->data);
    }
}

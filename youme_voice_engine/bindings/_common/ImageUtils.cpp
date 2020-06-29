//
//  ImageUtils.cpp
//  youme_voice_engine
//
//  Created by mac on 2017/8/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include <algorithm>
#include "tsk_debug.h"
#include "ImageUtils.hpp"
#include <YouMeCommon/yuvlib/libyuv.h>
#include "TLSMemory.h"
namespace youme_voice_engine {
Image::Image(int width, int height, void* data) {
    this->height = height;
    this->width = width;
    this->size = this->height * this->width * 3 / 2;
    //this->data = malloc(this->size);
    this->data = CTLSMemory::GetInstance()->Allocate(this->size, this->iSlot);
    memcpy(this->data, data, this->size);
}

Image::Image(int width, int height) {
    this->height = height;
    this->width = width;
    this->size = this->height * this->width * 3 / 2;
    //this->data = malloc(this->size);
    this->data = CTLSMemory::GetInstance()->Allocate(this->size, this->iSlot);
    //memset(this->data, 128, this->size);
}

Image::~Image() {
    if(this->data) {
        CTLSMemory::GetInstance()->Free(this->iSlot);
        //free(this->data);
    }
}


Image* ImageUtils::transpose(Image* src) {
    if (NULL == src) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return NULL;
    }
    
    Image* dest = new Image(src->height, src->width);
    
    uint8_t* ydest = (uint8_t*)dest->data;
    uint8_t* udest = ydest + dest->width * dest->height;
    uint8_t* vdest = ydest + dest->width * dest->height * 5 / 4;
    
    uint8_t* ysrc = (uint8_t*)src->data;;
    uint8_t* usrc = ysrc + src->width * src->height;;
    uint8_t* vsrc = ysrc + src->width * src->height * 5 / 4;;
    
    int w = dest->width;
    int h = dest->height;
    
    // yplane
    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            ydest[i * w + j] = ysrc[j * h + i];
        }
    }
    
    //Uplane
    for(int i = 0; i < h/2; i++) {
        for(int j = 0; j < w/2; j++) {
            udest[i * w / 2 + j] = usrc[j * h / 2 + i];
        }
    }
    
    //Vplane
    for(int i = 0; i < h/2; i++) {
        for(int j = 0; j < w/2; j++) {
            vdest[i * w / 2 + j] = vsrc[j * h / 2 + i];
        }
    }
    return dest;
}

Image* ImageUtils::mirror(Image* src) {
    if (NULL == src) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return NULL;
    }

    Image* dest = new Image(src->width, src->height);

    int src_width  = src->width;
    int src_height = src->height;

    //copy origin data
    unsigned char * Dst_data = (unsigned char *)(dest->data);

    //YUV420 image size
    int I420_Y_Size = src_width * src_height;
    int I420_U_Size = (src_width >> 1) * (src_height >> 1);
//    int I420_V_Size = I420_U_Size;

    unsigned char *Y_data_src = (unsigned char *)src->data;
    unsigned char *U_data_src = Y_data_src + I420_Y_Size ;
    unsigned char *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;


    int Src_Stride_Y = src_width;
    int Src_Stride_U = src_width >> 1;
    int Src_Stride_V = Src_Stride_U;

    //最终写入目标
    unsigned char *Y_data_Dst_rotate = Dst_data;
    unsigned char *U_data_Dst_rotate = Dst_data + I420_Y_Size;
    unsigned char *V_data_Dst_rotate = Dst_data + I420_Y_Size + I420_U_Size;

    //mirro
    int Dst_Stride_Y_mirror = src_width;
    int Dst_Stride_U_mirror = src_width >> 1;
    int Dst_Stride_V_mirror = Dst_Stride_U_mirror;
    YOUME_libyuv::I420Mirror(Y_data_src, Src_Stride_Y,
            U_data_src, Src_Stride_U,
            V_data_src, Src_Stride_V,
            Y_data_Dst_rotate, Dst_Stride_Y_mirror,
            U_data_Dst_rotate, Dst_Stride_U_mirror,
            V_data_Dst_rotate, Dst_Stride_V_mirror,
            src_width, src_height);

    return dest;
}

Image* ImageUtils::rotate(Image* src, int rotation) {
    if (NULL == src) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return NULL;
    }
    
    if((0 != rotation) && (90 != rotation) && (180 != rotation) && (270 != rotation)) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return NULL;
    }
    
    if(0 == rotation) {
        Image* dest = new Image(src->width, src->height, src->data);
        return dest;
    }

    int src_width  = src->width;
    int src_height = src->height;
    Image* dest = new Image(src->width, src->height);
    //copy origin data
    unsigned char * Dst_data = (unsigned char *)(dest->data);

    //YUV420 image size
    int I420_Y_Size = src_width * src_height;
//            int I420_U_Size = (src_width >> 1) * (src_height >> 1);
    int I420_U_Size = src_width * src_height / 4 ;
    //    int I420_V_Size = I420_U_Size;

    unsigned char *Y_data_src = NULL;
    unsigned char *U_data_src = NULL;
    unsigned char *V_data_src = NULL;

    int Dst_Stride_Y_rotate;
    int Dst_Stride_U_rotate;
    int Dst_Stride_V_rotate;

    int Dst_Stride_Y = src_width;
    int Dst_Stride_U = src_width >> 1;
    int Dst_Stride_V = Dst_Stride_U;

    //最终写入目标
    unsigned char *Y_data_Dst_rotate = Dst_data;
    unsigned char *U_data_Dst_rotate = Dst_data + I420_Y_Size;
    unsigned char *V_data_Dst_rotate = Dst_data + I420_Y_Size + I420_U_Size;

    if (rotation == YOUME_libyuv::kRotate90 || rotation == YOUME_libyuv::kRotate270) {
        Dst_Stride_Y_rotate = src_height;
        Dst_Stride_U_rotate = src_height >> 1;
        Dst_Stride_V_rotate = Dst_Stride_U_rotate;
    }
    else {
        Dst_Stride_Y_rotate = src_width;
        Dst_Stride_U_rotate = src_width >> 1;
        Dst_Stride_V_rotate = Dst_Stride_U_rotate;
    }


    Y_data_src = (unsigned char *)src->data;
    U_data_src = Y_data_src + I420_Y_Size;
    V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;

    YOUME_libyuv::I420Rotate(Y_data_src, Dst_Stride_Y,
            U_data_src, Dst_Stride_U,
            V_data_src, Dst_Stride_V,
            Y_data_Dst_rotate, Dst_Stride_Y_rotate,
            U_data_Dst_rotate, Dst_Stride_U_rotate,
            V_data_Dst_rotate, Dst_Stride_V_rotate,
            src_width, src_height,
            (YOUME_libyuv::RotationMode) rotation);

    if (rotation == YOUME_libyuv::kRotate90 || rotation == YOUME_libyuv::kRotate270){
        dest->width = src_height;
        dest->height = src_width;
    }

    return dest;
}

Image* ImageUtils::centerScale(Image* src, int out_width, int out_height) {
//    std::shared_ptr<FrameImage> dest = std::shared_ptr<FrameImage>(new FrameImage(out_width, out_height));
    if (NULL == src) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return NULL;
    }

    Image* dest = new Image(out_width, out_height);

    int src_width_ = src->width;
    int src_height_= src->height;
    int dst_width_ = out_width;
    int dst_height_ = out_height;

    // Making sure that destination frame is of sufficient size.

    // We want to preserve aspect ratio instead of stretching the frame.
    // Therefore, we need to crop the source frame. Calculate the largest center
    // aligned region of the source frame that can be used.
    const int cropped_src_width =
            std::min(src_width_, dst_width_ * src_height_ / dst_height_);
    const int cropped_src_height =
            std::min(src_height_, dst_height_ * src_width_ / dst_width_);
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



void ImageUtils::centerScale_new(char * in_buff, int in_width, int in_height, char * out_buff, int out_width, int out_height) {
    //    std::shared_ptr<FrameImage> dest = std::shared_ptr<FrameImage>(new FrameImage(out_width, out_height));
    if (out_buff == NULL|| in_buff == NULL) {
        TSK_DEBUG_ERROR("Invalid parameter.");
        return ;
    }
    

    int src_width_ = in_width;
    int src_height_= in_height;
    int dst_width_ = out_width;
    int dst_height_ = out_height;
    
    // Making sure that destination frame is of sufficient size.
    
    // We want to preserve aspect ratio instead of stretching the frame.
    // Therefore, we need to crop the source frame. Calculate the largest center
    // aligned region of the source frame that can be used.
    const int cropped_src_width =
    std::min(src_width_, dst_width_ * src_height_ / dst_height_);
    const int cropped_src_height =
    std::min(src_height_, dst_height_ * src_width_ / dst_width_);
    // Make sure the offsets are even to avoid rounding errors for the U/V planes.
    const int src_offset_x = ((src_width_ - cropped_src_width) / 2) & ~1;
    const int src_offset_y = ((src_height_ - cropped_src_height) / 2) & ~1;
    
    //YUV420 image size
    int I420_Y_Size = src_width_ * src_height_;
    int I420_U_Size = src_width_ * src_height_ / 4;
    //    int I420_V_Size = I420_U_Size;
    
    unsigned char *Y_data_src = (unsigned char *) in_buff;
    unsigned char *U_data_src = Y_data_src + I420_Y_Size;
    unsigned char *V_data_src = Y_data_src + I420_Y_Size + I420_U_Size;
    int src_stride_Y = src_width_;
    int src_stride_U = (src_width_+1) >> 1;
    int src_stride_V = src_stride_U;
    
    //最终写入目标
    int dest_I420_Y_Size = dst_width_ * dst_height_;
    int dest_I420_U_Size = dst_width_ * dst_height_ / 4;
    unsigned char *Y_data_dest = (unsigned char *) out_buff;
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
    
}
} //namespace youme_voice_engine 

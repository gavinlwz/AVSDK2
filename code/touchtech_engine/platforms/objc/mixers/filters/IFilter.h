//
//  IFilter.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//
#ifndef videocore_IFilter_hpp
#define videocore_IFilter_hpp
#include <string>

#define VIDEO_FILTER_NAME_BGRA          "com.videocore.filters.bgra"
#define VIDEO_FILTER_NAME_TOBGRA        "com.videocore.filters.tobgra"
#define VIDEO_FILTER_NAME_BEAUTY        "com.videocore.filters.beauty"
#define VIDEO_FILTER_NAME_YUV           "com.videocore.filters.yuvtobgra"
#define VIDEO_FILTER_NAME_NV12          "com.videocore.filters.nv12tobgra"

namespace videocore {
    
    struct GLRect
    {
        int x;
        int y;
        int w;
        int h;
    };
    class IFilter {
    
    public:
        virtual ~IFilter() {} ;
        
        virtual void initialize() = 0;
        virtual bool initialized() const = 0;
        
        virtual void bind() = 0;
        virtual void unbind() = 0;
        
        virtual void setBeautyLevel(float level){};
        
        virtual std::string const name() = 0;
        
        virtual void draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height){};
        
        virtual void makeMatrix(GLRect &rc, uint32_t frameWidth, uint32_t frameHeight, uint32_t bkWidth, uint32_t bkHeight, uint32_t mirror = 0, uint32_t rotation = 0){};
    };
}

#endif

//
//  BeautyVideoFilter.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youmi. All rights reserved.
//

#ifndef BeautyVideoFilter_hpp
#define BeautyVideoFilter_hpp



#include "../IVideoFilter.h"

namespace videocore {
    namespace filters {
        class BeautyVideoFilter : public IVideoFilter {
            
        public:
            BeautyVideoFilter();
            ~BeautyVideoFilter();
            
        public:
            virtual void initialize();
            virtual bool initialized() const { return m_initialized; };
            virtual std::string const name() { return VIDEO_FILTER_NAME_BEAUTY; };
            virtual void bind();
            virtual void unbind();
            virtual void setBeautyLevel(float level);
        public:
            void getBeautyParam(float* fBeautyParam);
            const char * const vertexKernel() const ;
            const char * const pixelKernel() const ;
            
        private:
            static bool registerFilter();
            static bool s_registered;
        private:
            unsigned int m_uWH;
            unsigned int m_beautyParam;
            unsigned int m_uWidth;
            unsigned int m_uHeight;
            
            bool m_initialized;
            bool m_bound;
            float m_level;
            
        };
    }
}



#endif /* BeautyVideoFilter_hpp */

/*
struct fbo
{
    int textureid;
    int framebufferid;
    CVPixelBufferRef pixelbuffer; //ios
}

class GLESVideoProducer
{
public:
    void init()
    {
         1、初始化opengl，创建eglcontext
        2、创建两组fbo, ios:创建CVPixelBufferRef 关联textureid
         3、创建线程
    }
    void release();
         
    void inputFrame();
    
    void threadFun()
    {
        index->0,1,0,1;
        1、美颜
        2、翻转，镜像
        3、缩放裁剪 编码
        4、缩放裁剪 合流
    }
private:
    fbo  encoderfbo[2];
    fbo  mixfbo[2];
    
}


class GLESVideoMixer
{
public:
    void init();
    void inputFrame();
    void threadFun()
    {
        index->0,1,0,1;
        1、mixing
    }
private:
    std::map<userid, frame>;   //只保留一帧图像
    fbo  mixfbo[2];
}

*/



//
//  BasicVideoFilterYUVToBGRA.hpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/30.
//  Copyright © 2017年 youmi. All rights reserved.
//

#ifndef BasicVideoFilterYUVToBGRA_hpp
#define BasicVideoFilterYUVToBGRA_hpp

#include "../IVideoFilter.h"
#ifdef MAC_OS
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#include <OpenGL/OpenGL.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif



namespace videocore {
    namespace filters {
        class BasicVideoFilterYUVToBGRA : public IVideoFilter
        {
        public:
            BasicVideoFilterYUVToBGRA();
            ~BasicVideoFilterYUVToBGRA();
            
        public:
            virtual void initialize();
            virtual bool initialized() const { return m_initialized; };
            virtual std::string const name() { return VIDEO_FILTER_NAME_YUV; };
            virtual void bind();
            virtual void unbind();
            virtual void draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height);
        public:
            const char * const vertexKernel() const ;
            const char * const pixelKernel() const ;
            
        private:
            void CreateTexture(int nW, int nH);
            void ChangedGl(int nW, int nH);
        private:
            static bool registerFilter();
            static bool s_registered;
            
        private:
            bool m_initialized;
            bool m_bound;
            
            GLuint m_nTextureY;
            GLuint m_nTextureU;
            GLuint m_nTextureV;

            int m_dwSamplerY;
            int m_dwSamplerU;
            int m_dwSamplerV;
            
            int m_width;
            int m_height;
    
        };
    }
}

#endif /* BasicVideoFilterYUVToBGRA_hpp */

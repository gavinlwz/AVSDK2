//
//  BasicVideoFilterBGRA.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youmi. All rights reserved.
//
#ifndef videocore_BasicVideoFilterBGRA_h
#define videocore_BasicVideoFilterBGRA_h

#include "../IVideoFilter.h"

namespace videocore {
    namespace filters {
        class BasicVideoFilterBGRA : public IVideoFilter {
            
        public:
            BasicVideoFilterBGRA();
            ~BasicVideoFilterBGRA();
        
        public:
            virtual void initialize();
            virtual bool initialized() const { return m_initialized; };
            virtual std::string const name() { return VIDEO_FILTER_NAME_BGRA; };
            virtual void bind();
            virtual void unbind();
            virtual void draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height);
        public:
            const char * const vertexKernel() const ;
            const char * const pixelKernel() const ;
            
            void createTexture(int nW, int nH);
        private:
            static bool registerFilter();
            static bool s_registered;
        private:
            bool m_initialized;
            bool m_bound;
            GLuint  m_textureId;
            int  m_width;
            int  m_height;
            
            
        };
    }
}

#endif /* defined(videocore_BasicVideoFilterBGRA_h) */

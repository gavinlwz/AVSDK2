//
//
//  BasicVideoFilterNV12ToBGRA.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/30.
//  Copyright © 2017年 youmi. All rights reserved.
//

#include "BasicVideoFilterNV12ToBGRA.h"
#include <dlfcn.h>
#include <TargetConditionals.h>
#ifdef MAC_OS
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#else
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES3/gl.h>
#endif
#include "BasicVideoFilterBGRA.h"
#include "../../GLESUtil.h"
#include "../FilterFactory.h"

namespace videocore { namespace filters {

    
    bool BasicVideoFilterNV12ToBGRA::s_registered = BasicVideoFilterNV12ToBGRA::registerFilter();
    
    bool BasicVideoFilterNV12ToBGRA::registerFilter()
    {
        FilterFactory::_register("com.videocore.filters.nv12tobgra", []() { return new BasicVideoFilterNV12ToBGRA(); });
        return true;
    }
    
    BasicVideoFilterNV12ToBGRA::BasicVideoFilterNV12ToBGRA()
    : IVideoFilter(), m_initialized(false), m_bound(false), m_width(0), m_height(0)
    {
        m_nTextureY = -1;
        m_nTextureUV = -1;
    }
    BasicVideoFilterNV12ToBGRA::~BasicVideoFilterNV12ToBGRA()
    {
        if (0 != m_program)
            glDeleteProgram(m_program);

        if (-1 != m_nTextureY)
            glDeleteTextures(1, &m_nTextureY);
        
        if (-1 != m_nTextureUV)
            glDeleteTextures(1, &m_nTextureUV);
    }
    
    const char * const BasicVideoFilterNV12ToBGRA::vertexKernel() const
    {
        
        KERNEL(GL_ES2_3, m_language,
               attribute vec4 aPos;
               attribute vec2 aCoord;
               varying vec2   vCoord;
               uniform mat4   uMat;
               void main() {
                   gl_Position = uMat*aPos;
                   vCoord = aCoord.xy;
               }
               )
        
        return nullptr;
    }
    
    const char * const  BasicVideoFilterNV12ToBGRA::pixelKernel() const
    {
#ifdef MAC_OS
        KERNEL(GL_ES2_3, m_language,
               varying  vec2 vCoord;
               uniform sampler2D SamplerY;
               uniform sampler2D SamplerU;
               void main()
               {
               mediump vec3 yuv;
               lowp vec3 rgb;
               yuv.x = texture2D(SamplerY, vCoord).r;
               yuv.yz = texture2D(SamplerU, vCoord).rg - vec2(0.5, 0.5);
               rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;
               gl_FragColor = vec4(rgb, 1);
               }
               )
        
#else
        KERNEL(GL_ES2_3, m_language,
               varying lowp vec2 vCoord;
               uniform sampler2D SamplerY;
               uniform sampler2D SamplerU;
               void main()
               {
               mediump vec3 yuv;
               lowp vec3 rgb;
               yuv.x = texture2D(SamplerY, vCoord).r - (16.0/ 256.0);
               yuv.yz = texture2D(SamplerU, vCoord).rg - vec2(0.5, 0.5);
               rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;
               gl_FragColor = vec4(rgb, 1.0);
               }
               )
#endif
        
        return nullptr;
    }
    
    void BasicVideoFilterNV12ToBGRA::CreateTexture(int nW, int nH)
    {
        int y_size = nW * nH;
        int uv_size = nW * nH / 2;
        
        uint8_t * black_buf = new uint8_t[y_size + uv_size];
        if (black_buf) {
            memset(black_buf, 0, y_size + uv_size);
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTextureY);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf);
#endif
        glBindTexture(GL_TEXTURE_2D, 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTextureUV);
        nW = nW >> 1;
        nH = nH >> 1;
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, nW, nH, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, black_buf + y_size);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG_EXT, nW, nH, 0, GL_RG_EXT, GL_UNSIGNED_BYTE, black_buf + y_size);
#endif
        
        delete [] black_buf;
    }
    
    void BasicVideoFilterNV12ToBGRA::ChangedGl(int nW, int nH)
    {
        if (-1 != m_nTextureY)
            glDeleteTextures(1, &m_nTextureY);
        glGenTextures(1, &m_nTextureY);
        
        if (-1 != m_nTextureUV)
            glDeleteTextures(1, &m_nTextureUV);
        glGenTextures(1, &m_nTextureUV);
        
        CreateTexture(nW, nH);

    }
    
    void BasicVideoFilterNV12ToBGRA::draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height)
    {
        if(m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;
            ChangedGl(width, height);
        }
        

#ifdef MAC_OS
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTextureY);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTextureUV);
        glUniform1i(m_dwSamplerUV, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buff + width * height);

#else
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTextureUV);
//        glUniform1i(m_dwSamplerUV, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_RG_EXT, GL_UNSIGNED_BYTE, buff + width * height);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTextureY);
//        glUniform1i(m_dwSamplerY, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff);
        
 #endif
    }
    
    void BasicVideoFilterNV12ToBGRA::initialize()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2: {
                setProgram(build_program(vertexKernel(), pixelKernel()));
                glUseProgram(m_program);
                
                m_uMatrix = glGetUniformLocation(m_program, "uMat");
                m_attrpos = glGetAttribLocation(m_program, "aPos");
                m_attrtex = glGetAttribLocation(m_program, "aCoord");
                
                m_dwSamplerY = glGetUniformLocation(m_program, "SamplerY");
                m_dwSamplerUV = glGetUniformLocation(m_program, "SamplerU");
                glUniform1i(m_dwSamplerY, 0);
                glUniform1i(m_dwSamplerUV, 1);
                m_initialized = true;
            }
                break;
            case GL_3:
                break;
        }
    }
    void BasicVideoFilterNV12ToBGRA::bind()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2:
            {
                if(!m_bound) {
                    if(!initialized()) {
                        initialize();
                    }
                   glUseProgram(m_program);
                }
                
               IVideoFilter::bind();
            }
                break;
            case GL_3:
                break;
        }
    }
    void BasicVideoFilterNV12ToBGRA::unbind()
    {
        m_bound = false;
    }
}
}


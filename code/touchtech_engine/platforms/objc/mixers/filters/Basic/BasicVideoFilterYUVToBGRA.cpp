//
//
//  BasicVideoFilterYUVToBGRA.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/30.
//  Copyright © 2017年 youmi. All rights reserved.
//

#include "BasicVideoFilterYUVToBGRA.h"
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

    
    bool BasicVideoFilterYUVToBGRA::s_registered = BasicVideoFilterYUVToBGRA::registerFilter();
    
    bool BasicVideoFilterYUVToBGRA::registerFilter()
    {
        FilterFactory::_register("com.videocore.filters.yuvtobgra", []() { return new BasicVideoFilterYUVToBGRA(); });
        return true;
    }
    
    BasicVideoFilterYUVToBGRA::BasicVideoFilterYUVToBGRA()
    : IVideoFilter(), m_initialized(false), m_bound(false), m_width(0), m_height(0)
    {
        m_nTextureY = -1;
        m_nTextureU = -1;
        m_nTextureV = -1;
    }
    BasicVideoFilterYUVToBGRA::~BasicVideoFilterYUVToBGRA()
    {
        if (0 != m_program)
            glDeleteProgram(m_program);

        if (-1 != m_nTextureY)
            glDeleteTextures(1, &m_nTextureY);
        
        if (-1 != m_nTextureU)
            glDeleteTextures(1, &m_nTextureU);

        if (-1 != m_nTextureV)
            glDeleteTextures(1, &m_nTextureV);
    }
    
    const char * const BasicVideoFilterYUVToBGRA::vertexKernel() const
    {
        
        KERNEL(GL_ES2_3, m_language,
               attribute vec4 aPos;
               attribute vec4 aCoord;
               varying vec2   vCoord;
               uniform mat4   uMat;
               void main() {
                   gl_Position = uMat*aPos;
                   vCoord = aCoord.xy;
               }
               )
        
        return nullptr;
    }
    
    const char * const  BasicVideoFilterYUVToBGRA::pixelKernel() const
    {
#ifdef MAC_OS
        KERNEL(GL_ES2_3, m_language,
               varying  vec2 vCoord;
               uniform sampler2D SamplerY;
               uniform sampler2D SamplerU;
               uniform sampler2D SamplerV;
               void main()
               {
               mediump vec3 yuv;
               lowp vec3 rgb;
               yuv.x = texture2D(SamplerY, vCoord).r;
               yuv.y = texture2D(SamplerU, vCoord).r - 0.5;
               yuv.z = texture2D(SamplerV, vCoord).r - 0.5;
               rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;
               gl_FragColor = vec4(rgb, 1);
               }
               )
        
#else
        KERNEL(GL_ES2_3, m_language,
               varying lowp vec2 vCoord;
               uniform sampler2D SamplerY;
               uniform sampler2D SamplerU;
               uniform sampler2D SamplerV;
               void main()
               {
               mediump vec3 yuv;
               lowp vec3 rgb;
               yuv.x = texture2D(SamplerY, vCoord).r - (16.0/ 256.0);
               yuv.y = texture2D(SamplerU, vCoord).r - 0.5;
               yuv.z = texture2D(SamplerV, vCoord).r - 0.5;
               rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;
               gl_FragColor = vec4(rgb, 1.0);
               }
               )
#endif
        
        return nullptr;
    }
    
    void BasicVideoFilterYUVToBGRA::CreateTexture(int nW, int nH)
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
     

        nW = nW >> 1;
        nH = nH >> 1;   
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTextureU);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf + y_size);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf + y_size);
#endif

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_nTextureV);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf + y_size + uv_size / 2);
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nW, nH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, black_buf + y_size + uv_size / 2);
#endif

        delete [] black_buf;
    }
    
    void BasicVideoFilterYUVToBGRA::ChangedGl(int nW, int nH)
    {
        if (-1 != m_nTextureY)
            glDeleteTextures(1, &m_nTextureY);
        glGenTextures(1, &m_nTextureY);
        
        if (-1 != m_nTextureU)
            glDeleteTextures(1, &m_nTextureU);
        glGenTextures(1, &m_nTextureU);

        if (-1 != m_nTextureV)
            glDeleteTextures(1, &m_nTextureV);
        glGenTextures(1, &m_nTextureV);

        CreateTexture(nW, nH);

    }
    
    void BasicVideoFilterYUVToBGRA::draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height)
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
        glBindTexture(GL_TEXTURE_2D, m_nTextureU);
        glUniform1i(m_dwSamplerU, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + width * height);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_nTextureV);
        glUniform1i(m_dwSamplerV, 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + width * height * 5/4);

#else
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_nTextureU);
//        glUniform1i(m_dwSamplerU, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + width * height);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_nTextureV);
//        glUniform1i(m_dwSamplerV, 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff + width * height * 5/4);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_nTextureY);
//        glUniform1i(m_dwSamplerY, 0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, buff);
#endif
    }
    
    void BasicVideoFilterYUVToBGRA::initialize()
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
                m_dwSamplerU = glGetUniformLocation(m_program, "SamplerU");
                m_dwSamplerV = glGetUniformLocation(m_program, "SamplerV");
                
                glUniform1i(m_dwSamplerY, 0);
                glUniform1i(m_dwSamplerU, 1);
                glUniform1i(m_dwSamplerV, 2);
                
                m_initialized = true;
            }
                break;
            case GL_3:
                break;
        }
    }
    void BasicVideoFilterYUVToBGRA::bind()
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
    void BasicVideoFilterYUVToBGRA::unbind()
    {
        m_bound = false;
    }
}
}


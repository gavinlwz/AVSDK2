//
//  BasicVideoFilterBGRA.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youmi. All rights reserved.
//


#include <dlfcn.h>
#include <TargetConditionals.h>
#ifdef MAC_OS
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES3/gl.h>
#endif
#include "BasicVideoFilterToBGRA.h"
#include "../../GLESUtil.h"
#include "../FilterFactory.h"

namespace videocore { namespace filters {
    
    
    bool BasicVideoFilterToBGRA::s_registered = BasicVideoFilterToBGRA::registerFilter();
    
    bool BasicVideoFilterToBGRA::registerFilter()
    {
        FilterFactory::_register("com.videocore.filters.tobgra", []() { return new BasicVideoFilterToBGRA(); });
        return true;
    }
    
    BasicVideoFilterToBGRA::BasicVideoFilterToBGRA()
    : IVideoFilter(), m_initialized(false), m_bound(false),m_textureId(-1),m_width(0), m_height(0)
    {
        
    }
    BasicVideoFilterToBGRA::~BasicVideoFilterToBGRA()
    {
        if (0  != m_program)
            glDeleteProgram(m_program);
        if (-1 != m_textureId)
            glDeleteTextures(1, &m_textureId);
    }
    
    
    const char * const BasicVideoFilterToBGRA::vertexKernel() const
    {
        
        
        KERNEL(GL_ES2_3, m_language,
               attribute vec4 aPos;
               attribute vec4 aCoord;
               varying vec2   vCoord;
               uniform mat4   uMat;
               void main(void) {
                   gl_Position = uMat*aPos;
                   vCoord = aCoord.xy;
               }
               )
        return nullptr;
    }
    
    const char * const  BasicVideoFilterToBGRA::pixelKernel() const
    {
#if MAC_OS
        KERNEL(GL_ES2_3, m_language,
               varying vec2      vCoord;
               uniform sampler2D uTex0;
               void main(void) {
                   gl_FragColor = texture2D(uTex0, vCoord).bgra;
               }
               )
#else
        KERNEL(GL_ES2_3, m_language,
               precision mediump float;
               varying lowp vec2      vCoord;
               uniform sampler2D uTex0;
               void main(void) {
                   gl_FragColor = texture2D(uTex0, vCoord).bgra;
               }
               )
        
#endif
        
        return nullptr;
    }
    
    void BasicVideoFilterToBGRA::createTexture(int nW, int nH)
    {
        if (-1 != m_textureId)
            glDeleteTextures(1, &m_textureId);
        glGenTextures(1, &m_textureId);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nW, nH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    
    void BasicVideoFilterToBGRA::draw(const uint8_t* buff, uint32_t buffLen, uint32_t width, uint32_t height)
    {
        if(m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;
            createTexture(width, height);
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buff);
        
    }
    
    void BasicVideoFilterToBGRA::initialize()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2: {
                setProgram(build_program(vertexKernel(), pixelKernel()));
                
                m_uMatrix = glGetUniformLocation(m_program, "uMat");
                m_attrpos = glGetAttribLocation(m_program, "aPos");
                m_attrtex = glGetAttribLocation(m_program, "aCoord");
                int unitex = glGetUniformLocation(m_program, "uTex0");
                glUniform1i(unitex, 0);
                
                m_initialized = true;
            }
                break;
            case GL_3:
                break;
        }
    }
    void BasicVideoFilterToBGRA::bind()
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
                // glUniformMatrix4fv(m_uMatrix, 1, GL_FALSE, m_matrix);
                
            }
                break;
            case GL_3:
                break;
        }
    }
    void BasicVideoFilterToBGRA::unbind()
    {
        m_bound = false;
    }
}
}

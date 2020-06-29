//
//  GLESFbo.cpp
//  youme_voice_engine
//
//  Created by bhb on 2018/2/1.
//  Copyright © 2018年 Youme. All rights reserved.
//


#include "GLESFbo.h"

GLESFbo::GLESFbo(){
     m_width = 0;
     m_height = 0;
     m_frameBuffer = 0;
     m_renderBuffer = 0;
     m_frameBufferTexture = 0;
}
GLESFbo::~GLESFbo(){
    uninitFBO();
}
int GLESFbo::getTexture(){
     return m_frameBufferTexture;
}
int GLESFbo::getFrameBuffer(){
    return m_frameBuffer;
}
int GLESFbo::getRenderBuffer(){
     return m_renderBuffer;
}
bool GLESFbo::initFBO(int width, int height){
    if (width == 0 || height == 0)
        return false;
    
    if (width == m_width && height == m_height)
        return true;
    uninitFBO();

    ///////////////// create FrameBufferTextures
    glGenTextures(1, &m_frameBufferTexture);
    glBindTexture(GL_TEXTURE_2D, m_frameBufferTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    ////////////////////////// create FrameBuffer
    glGenFramebuffers(1, &m_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_frameBufferTexture, 0);
    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer not complete, status=%d\n" , status);
        return false;
    }
    // Switch back to the default framebuffer.
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_width  = width;
    m_height = height;
    return  true;
}
void GLESFbo::uninitFBO(){
    if(m_frameBufferTexture != 0){
        glDeleteTextures(1, &m_frameBufferTexture);
        m_frameBufferTexture = 0;
    }
    if(m_frameBuffer != 0) {
        glDeleteFramebuffers(1, &m_frameBuffer);
        m_frameBuffer = 0;
    }
    if(m_renderBuffer != 0) {
        glDeleteRenderbuffers(1, &m_renderBuffer);
        m_renderBuffer = 0;
    }
    m_width = 0;
    m_height = 0;
}



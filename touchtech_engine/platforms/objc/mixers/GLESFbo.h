//
//  GLESFbo.h
//  youme_voice_engine
//
//  Created by bhb on 2018/2/1.
//  Copyright © 2018年 Youme. All rights reserved.
//

#ifndef GLESFbo_h
#define GLESFbo_h


#include <stdio.h>
#include "GLESUtil.h"

class GLESFbo{
public:
    GLESFbo();
    ~GLESFbo();
    int getTexture();
    int getFrameBuffer();
    int getRenderBuffer();
    bool initFBO(int width, int height);
    void uninitFBO();
private:
     int m_width;
     int m_height;
     GLuint m_frameBuffer;
     GLuint m_renderBuffer;
     GLuint m_frameBufferTexture;
};



#endif /* GLESFbo_h */

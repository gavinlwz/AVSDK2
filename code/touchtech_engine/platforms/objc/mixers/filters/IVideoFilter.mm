//
//  IVideoFilter.m
//  VideoCodecTest
//
//  Created by bhb on 2017/9/5.
//  Copyright © 2017年 youmi. All rights reserved.
//


#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>
#include "IVideoFilter.h"

namespace videocore {


    IVideoFilter::IVideoFilter() : m_program(0), m_dimensions({ 1.f, 1.f }), m_language(GL_ES2_3) {
        
        memcpy(m_squareVertices, squareVertices, sizeof(squareVertices));
        memcpy(m_coordVertices, coordVertices, sizeof(coordVertices));
        GLKMatrix4 viewMatrix = GLKMatrix4Identity;
        incomingMatrix(viewMatrix.m);
        m_vboPos = m_vboTex = 0;
        m_rc.x = m_rc.y = m_rc.w = m_rc.h = 0;
        m_frameWidth = m_frameHeight = 0;
        m_bkWidth = m_bkHeight = 0;
        m_change = false;
        m_mirror = 0;
        m_rotation = 0;
    };
    IVideoFilter::~IVideoFilter()
    {
        if (m_vboPos)
            glDeleteBuffers(1, &m_vboPos);
        if (m_vboTex)
            glDeleteBuffers(1, &m_vboTex);
    }
    void IVideoFilter::makeMatrix(GLRect &rc, uint32_t frameWidth, uint32_t frameHeight, uint32_t bkWidth, uint32_t bkHeight, uint32_t mirror, uint32_t rotation)
    {
        if(frameWidth == 0 || frameHeight == 0 || rc.w == 0 || rc.h == 0)
            return;
        if (rc.x != m_rc.x ||
            rc.y != m_rc.y ||
            rc.w != m_rc.w ||
            rc.h != m_rc.h ||
            frameWidth != m_frameWidth ||
            frameHeight != m_frameHeight ||
            bkWidth != m_bkWidth ||
            bkHeight != m_bkHeight ||
            mirror != m_mirror ||
            rotation != m_rotation) {
            
            m_rc.x = rc.x;
            m_rc.y = rc.y;
            m_rc.w = rc.w;
            m_rc.h = rc.h;
            m_frameWidth = frameWidth;
            m_frameHeight = frameHeight;
            m_bkWidth = bkWidth;
            m_bkHeight = bkHeight;
            m_mirror = mirror;
            m_rotation = rotation;
            m_change = true;
        }
        else{
            return;
        }
        float width, height;
        width = rc.w;
        height = rc.h;
        
        //texture Vertices Crop
        if(rc.w*frameHeight > rc.h*frameWidth)
        {
            width = frameWidth;
            height = rc.h/(float)rc.w * frameWidth;
        }
        else{
            height = frameHeight;
            width = rc.w/(float)rc.h * frameHeight;
        }

        float xspace, yspace;
        xspace = (frameWidth/2.f - width/2.f)/frameWidth;
        yspace = (frameHeight/2.f - height/2.f)/frameHeight;
        m_coordVertices[0] = xspace;
        m_coordVertices[1] = yspace;
        m_coordVertices[2] = 1.f - xspace;
        m_coordVertices[3] = yspace;
        m_coordVertices[4] = xspace;
        m_coordVertices[5] = 1.f - yspace;
        m_coordVertices[6] = 1.f - xspace;
        m_coordVertices[7] = yspace;
        m_coordVertices[8] = 1.f - xspace;
        m_coordVertices[9] = 1.f - yspace;
        m_coordVertices[10] = xspace;
        m_coordVertices[11] = 1.f - yspace;
        
        //square Vertices
        float left = rc.x/(float)bkWidth*2 - 1;
        float top = (rc.y+rc.h)/(float)bkHeight*2 - 1;
        float right = (rc.x+rc.w)/(float)bkWidth*2 - 1;
        float bottom = rc.y/(float)bkHeight*2 - 1;;
        m_squareVertices[0] = left;
        m_squareVertices[1] = bottom;
        m_squareVertices[2] = right;
        m_squareVertices[3] = bottom;
        m_squareVertices[4] = left;
        m_squareVertices[5] = top;
        m_squareVertices[6] = right;
        m_squareVertices[7] = bottom;
        m_squareVertices[8] = right;
        m_squareVertices[9] = top;
        m_squareVertices[10] = left;
        m_squareVertices[11] = top;
        
        GLKMatrix4 viewMatrix = GLKMatrix4Identity;
        if (m_mirror) {
            GLKMatrix4 modelViewMatrix = GLKMatrix4Scale(GLKMatrix4Identity,  -1.0f,  1.0f, 1.0f);
            viewMatrix =GLKMatrix4Multiply(viewMatrix, modelViewMatrix);
        }
        
        if (m_rotation) {
            GLKMatrix4 modelViewMatrix = GLKMatrix4Rotate(GLKMatrix4Identity, GLKMathDegreesToRadians(m_rotation), 0.0f,  0.0f, 1.f);
            viewMatrix =GLKMatrix4Multiply(viewMatrix, modelViewMatrix);
        }
        
        incomingMatrix(viewMatrix.m);
    }

    void IVideoFilter::bind()
    {
        if (m_change) {
            if (m_vboPos)
                glDeleteBuffers(1, &m_vboPos);
            if (m_vboTex)
                glDeleteBuffers(1, &m_vboTex);
            
            glGenBuffers(1, &m_vboPos);
            glBindBuffer(GL_ARRAY_BUFFER, m_vboPos);
            glBufferData(GL_ARRAY_BUFFER, sizeof(m_squareVertices), m_squareVertices, GL_STATIC_DRAW);
            
            glGenBuffers(1, &m_vboTex);
            glBindBuffer(GL_ARRAY_BUFFER, m_vboTex);
            glBufferData(GL_ARRAY_BUFFER, sizeof(m_coordVertices), m_coordVertices, GL_STATIC_DRAW);
            
            m_change = false;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vboPos);
        glEnableVertexAttribArray(m_attrpos);
        glVertexAttribPointer(m_attrpos, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vboTex);
        glEnableVertexAttribArray(m_attrtex);
        glVertexAttribPointer(m_attrtex, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glUniformMatrix4fv(m_uMatrix, 1, GL_FALSE, m_matrix);
#if MAC_OS
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
#else
        glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
    }

};



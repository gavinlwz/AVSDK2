//
//  IVideoFilter.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//

#ifndef videocore_IVideoFilter_hpp
#define videocore_IVideoFilter_hpp

#include "IFilter.h"
#ifdef MAC_OS
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif

#define KERNEL(_language, _target, _kernelstr) if(_language == _target){ do { return # _kernelstr ; } while(0); }

#define KERNEL2(_language, _target, _kernelstr1, _kernelstr2) if(_language == _target){ do { return # _kernelstr1 # _kernelstr2  ; } while(0); }


namespace videocore {

    enum FilterLanguage {
        GL_ES2_3,
        GL_2,
        GL_3
    };
    
    static const GLfloat squareVertices[] = {
        -1.0f,  -1.0f,
         1.0f,  -1.0f,
        -1.0f,   1.0f,
         1.0f,  -1.0f,
         1.0f,   1.0f,
        -1.0f,   1.0f,
    };
    
    
    static const GLfloat coordVertices[] = {
        0.0f,  0.0f,
        1.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  0.0f,
        1.0f,  1.0f,
        0.0f,  1.0f
    };

    
    class IVideoFilter : public IFilter {
        
    public:
        virtual ~IVideoFilter();

        virtual const char * const vertexKernel() const = 0;
        virtual const char * const pixelKernel() const = 0;

        
    public:
        void incomingMatrix(float* matrix) { memcpy(m_matrix, matrix, sizeof(float)*16); };
        void imageDimensions(float w, float h) { m_dimensions.w = w; m_dimensions.h = h; };
        
        void setFilterLanguage(FilterLanguage language) { m_language = language ; };
        void setProgram(int program) { m_program = program; };
        void setAttrib(int attrpos, int attrtex){m_attrpos = attrpos; m_attrtex = attrtex;};
        const int program() const { return m_program; };
        void makeMatrix(GLRect &rc, uint32_t frameWidth, uint32_t frameHeight, uint32_t bkWidth, uint32_t bkHeight, uint32_t mirror = 0, uint32_t rotation = 0);
        virtual void bind();
        protected:
        IVideoFilter();
      
        float m_matrix[16];
        struct { float w, h;} m_dimensions;
        GLuint m_vboPos, m_vboTex;
        int m_program;
        int m_attrpos, m_attrtex;
        int m_uMatrix;
        float m_squareVertices[12];
        float m_coordVertices[12];
        FilterLanguage m_language;
        
        bool m_change;
    private:
        GLRect m_rc;
        uint32_t m_frameWidth;
        uint32_t m_frameHeight;
        uint32_t m_bkWidth;
        uint32_t m_bkHeight;
        uint32_t m_mirror;
        uint32_t m_rotation;
        
        
    };
}

#endif

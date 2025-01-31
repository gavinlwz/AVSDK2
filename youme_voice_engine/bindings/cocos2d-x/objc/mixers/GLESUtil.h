﻿//
//  GLESUtil.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.

#ifndef videocore_GLESUtil_h
#define videocore_GLESUtil_h

#ifdef MAC_OS
//#define DEBUG
#endif

#ifdef MAC_OS
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#else
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/ES3/gl.h>
#endif
#import <string.h>

#define DLog(...) printf(__VA_ARGS__);

#define BUFFER_OFFSET(i) ((void*)(i))
#define BUFFER_OFFSET_POSITION BUFFER_OFFSET(0)
#define BUFFER_OFFSET_TEXTURE  BUFFER_OFFSET(8)
#define BUFFER_SIZE_POSITION 2
#define BUFFER_SIZE_TEXTURE  2
#define BUFFER_STRIDE (sizeof(float) * 4)

#ifdef DEBUG
#define GL_ERRORS(line) { GLenum glerr; while((glerr = glGetError())) {\
switch(glerr)\
{\
case GL_NO_ERROR:\
break;\
case GL_INVALID_ENUM:\
DLog("OGL(" __FILE__ "):: %d: Invalid Enum\n", line );\
break;\
case GL_INVALID_VALUE:\
DLog("OGL(" __FILE__ "):: %d: Invalid Value\n", line );\
break;\
case GL_INVALID_OPERATION:\
DLog("OGL(" __FILE__ "):: %d: Invalid Operation\n", line );\
break;\
case GL_OUT_OF_MEMORY:\
DLog("OGL(" __FILE__ "):: %d: Out of Memory\n", line );\
break;\
} } }

#define GL_FRAMEBUFFER_STATUS(line) { GLenum status; status = glCheckFramebufferStatus(GL_FRAMEBUFFER); {\
switch(status)\
{\
case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:\
DLog("OGL(" __FILE__ "):: %d: Incomplete attachment\n", line);\
break;\
case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:\
DLog("OGL(" __FILE__ "):: %d: Incomplete missing attachment\n", line);\
break;\
case GL_FRAMEBUFFER_UNSUPPORTED:\
DLog("OGL(" __FILE__ "):: %d: Framebuffer combination unsupported\n",line);\
break;\
} } }
/*
case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:\
DLog("OGL(" __FILE__ "):: %d: Incomplete dimensions\n", line);\
break;\
 */

#else
#define GL_ERRORS(line)
#define GL_FRAMEBUFFER_STATUS(line)
#endif


static float s_vbo [] =
{
    -1.f, -1.f,       0.f, 0.f, // 0
    1.f, -1.f,        1.f, 0.f, // 1
    -1.f, 1.f,        0.f, 1.f, // 2
    
    1.f, -1.f,        1.f, 0.f, // 1
    1.f, 1.f,         1.f, 1.f, // 3
    -1.f, 1.f,        0.f, 1.f, // 2
};



static const char s_vs [] =
"attribute vec2 aPos;"
"attribute vec2 aCoord;"
"varying vec2 vCoord;"
"void main(void) {"
"gl_Position = vec4(aPos,0.,1.);"
"vCoord = aCoord;"
"}";
static const char s_vs_mat [] =
"attribute vec2 aPos;"
"attribute vec2 aCoord;"
"varying vec2 vCoord;"
"uniform mat4 uMat;"
"void main(void) {"
"gl_Position = uMat* vec4(aPos,0.,1.);"
"vCoord = aCoord;"
"}"
;
static const char s_fs[] =
"precision mediump float;"
"varying vec2 vCoord;"
"uniform sampler2D uTex0;"
"void main(void) {"
"gl_FragData[0] = texture2D(uTex0, vCoord);"
"}";

static inline GLuint compile_shader(GLuint type, const char * source)
{
    
    int shaderStringLength;
    shaderStringLength = strlen(source);
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, &shaderStringLength);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
#ifdef DEBUG
    if (!compiled) {
        GLint length;
        char *log;
        
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        
        log =  new char[length];
        glGetShaderInfoLog(shader, length, &length, &log[0]);
        DLog("%s compilation error: %s\n", (type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER"), log);
        delete [] log;
        
        return 0;
    }
#endif
    return shader;
    
}

static inline GLuint build_program(const char * vertex, const char * fragment)
{
    GLuint  vshad,
    fshad,
    p;
    
    GLint   len;
#ifdef DEBUG
    char*   log;
#endif
    
    vshad = compile_shader(GL_VERTEX_SHADER, vertex);
    fshad = compile_shader(GL_FRAGMENT_SHADER, fragment);
    
    p = glCreateProgram();
    glAttachShader(p, vshad);
    glAttachShader(p, fshad);
    glLinkProgram(p);
    glGetProgramiv(p,GL_INFO_LOG_LENGTH, &len);
    
    
#ifdef DEBUG
    if(len) {
        log =  new char[len];
        glGetProgramInfoLog(p, len, &len, log);
        
        DLog("program log: %s\n", log);
        delete [] log;
    }
#endif
    
    
    glDeleteShader(vshad);
    glDeleteShader(fshad);
    return p;
}


#endif

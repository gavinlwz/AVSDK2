//
//  VideoDrawYUV.m
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//

#import "VideoDrawYUV.h"
#import "GLESUtil.h"

#ifdef MAC_OS
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <QuartzCore/CAOpenGLLayer.h>
#else
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/CAEAGLLayer.h>
#endif

#import <GLKit/GLKit.h>


@interface VideoDrawYUV(){
    
    GLuint                  _framebuffer;
    GLuint                  _renderBuffer;
    GLuint                  _shaderProgram;
    GLuint                  _textureYUV[3];
    GLuint                  _matrixPos;
    GLuint                  _dwTexture[3];
    GLuint                  _dwPosition;
    GLuint                  _dwTexCoordIn;
    
    GLKMatrix4              _modelMatrix;
    GLKMatrix4              _viewMatrix;
    
    GLuint                  _videoW;
    GLuint                  _videoH;
    GLsizei                 _viewScale;
    YMColor *               _renderBgColor;
    
    GLVideoRenderPosition    _position;
    GLVideoRenderPosition    _positionNew;
    GLfloat                _squareVertices[8];
    GLfloat                _coordVertices[8];
    GLVideoRenderMode      _renderMode;
    
}

@property (nonatomic, strong) YMOpenGLContext* context;
@property (nonatomic, weak) YMView* ymView;
@property (atomic, assign) CGSize boundSize;
@end


@implementation VideoDrawYUV


- (instancetype)init:(YMOpenGLContext*)context view:(YMView*)view{
    if ((self = [super init])) {
        [self initGLES:context view:view];
    }
    return self;
}


- (void) dealloc
{
    [self releaseGLES];
}

- (void) initGLES:(YMOpenGLContext*)context view:(YMView*)view
{
    if(!context || !view || _context)
        return;
    _context = (YMOpenGLContext*)context;
    _ymView = view;
    _boundSize = [view bounds].size;
    _textureYUV[0] = _textureYUV[1] = _textureYUV[2] = 0;
    _videoW = _videoH = 0;
    _shaderProgram = 0;
    _renderBuffer = 0;
    _framebuffer = 0;
#ifdef MAC_OS
    _viewScale = [YMScreen mainScreen].backingScaleFactor;
    _renderBgColor = [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
#else
    _viewScale = [YMScreen mainScreen].scale;
    _renderBgColor = [[YMColor alloc] initWithRed:0.0 green:0.0  blue:0.0  alpha:1.0];
#endif
    memcpy(_squareVertices, squareVertices, sizeof(_squareVertices));
    memcpy(_coordVertices, coordVertices, sizeof(_coordVertices));
    VideoDrawYUV* bSelf = self;
    //youme_main_async(^{
        [bSelf setupGLES];
        [bSelf setupGLESBuffers];
    //});
    
}

- (void) setupGLES
{
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    YM_SET_OPENGL_CURRENT_CONTEXT(self.context)
    
    _shaderProgram = build_program(s_vs_mat_yuv, s_fs_yuv);
    glUseProgram(_shaderProgram);
    
    _dwTexture[0] = glGetUniformLocation(_shaderProgram, "SamplerY");
    _dwTexture[1] = glGetUniformLocation(_shaderProgram, "SamplerU");
    _dwTexture[2] = glGetUniformLocation(_shaderProgram, "SamplerV");
    _dwPosition = glGetAttribLocation(_shaderProgram, "aPos");
    _dwTexCoordIn = glGetAttribLocation(_shaderProgram, "aCoord");
    _matrixPos = glGetUniformLocation(_shaderProgram, "uMat");
    glUniform1i(_dwTexture[0], 0);
    glUniform1i(_dwTexture[1], 1);
    glUniform1i(_dwTexture[2], 2);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    YM_SET_OPENGL_CURRENT_CONTEXT(current)
}

- (void)setupGLESBuffers{
    
#ifndef MAC_OS
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    YM_SET_OPENGL_CURRENT_CONTEXT(self.context)
    glGenFramebuffers(1, &_framebuffer);
    glGenRenderbuffers(1, &_renderBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    if (![self.context renderbufferStorage:GL_RENDERBUFFER fromDrawable:[_ymView layer]])
    {
        NSLog(@"attach渲染缓冲区失败");
    }

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        NSLog(@"创建缓冲区错误 0x%x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
    

    YM_SET_OPENGL_CURRENT_CONTEXT(current)
#endif
}

- (void)releaseGLESBuffers{
    
    if(_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    if(_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }
    if (_textureYUV[0]){
        glDeleteTextures(3, _textureYUV);
        _textureYUV[0] = 0;
        _videoW = 0;
        _videoH = 0;
    }
}

-(void)releaseGLES{
    VideoDrawYUV* bSelf = self;
    //youme_main_sync(^{
        YMOpenGLContext* current = [YMOpenGLContext currentContext];
        YM_SET_OPENGL_CURRENT_CONTEXT(self.context)
        if(_shaderProgram) {
            glDeleteProgram(_shaderProgram);
        }
        [bSelf releaseGLESBuffers];
        YM_SET_OPENGL_CURRENT_CONTEXT(current)
        bSelf.context = nil;
    //});
}

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer size:(CGSize)size{
    
}

- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h{
    
     VideoDrawYUV* bSelf = self;
    //youme_main_async(^{
        // 没到找Mac对应方法
#ifndef MAC_OS
        if([UIApplication sharedApplication].applicationState == UIApplicationStateBackground)
            return;
#endif
        youme_main_async(^{
            if(bSelf){
                self.boundSize = [bSelf.ymView bounds].size;
            }
        });
        CGSize boundSize = self.boundSize;
        YMOpenGLContext* current = [YMOpenGLContext currentContext];
        YM_SET_OPENGL_CURRENT_CONTEXT(bSelf.context)
        if (w != _videoW || h != _videoH) {
            [bSelf setupYUVTexture:w height:h];
            _videoW = w;
            _videoH = h;
        }
        uint8_t*blackData = (uint8_t*)data;
        if(_positionNew != _position)
            [bSelf positionChange:_positionNew];
        glViewport(0, 0, self.boundSize.width*_viewScale, self.boundSize.height*_viewScale);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(_shaderProgram);

#ifdef MAC_OS
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackData);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[1]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackData + w * h);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, blackData + w * h * 5 / 4);
#else
         glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED_EXT, GL_UNSIGNED_BYTE, blackData);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[1]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_RED_EXT, GL_UNSIGNED_BYTE, blackData + w * h);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, _textureYUV[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w/2, h/2, GL_RED_EXT, GL_UNSIGNED_BYTE, blackData + w * h * 5 / 4);
#endif
        glVertexAttribPointer(_dwPosition, 2, GL_FLOAT, 0, 0, _squareVertices);
        glEnableVertexAttribArray(_dwPosition);
        glVertexAttribPointer(_dwTexCoordIn, 2, GL_FLOAT, 0, 0, _coordVertices);
        glEnableVertexAttribArray(_dwTexCoordIn);
        
        _modelMatrix = GLKMatrix4Identity;
        _viewMatrix = GLKMatrix4Identity;
        
 
        float wfac = boundSize.width / (float)w;
        float hfac = boundSize.height / (float)h;
        float mult = 1.0f;
        if (_renderMode == GLVideoRenderModeHidden){
            mult = wfac > hfac ? wfac : hfac;
        }
        else if(_renderMode == GLVideoRenderModeFit){
            mult = wfac < hfac ? wfac : hfac;
        }
        else if(_renderMode == GLVideoRenderModeAdaptive){
            if ((boundSize.width - boundSize.height) * (w - h) >= 0) {
                
                mult = wfac > hfac ? wfac : hfac;
            }
            else{
                mult = wfac < hfac ? wfac : hfac;
            }
            
        }
        
        wfac = w*mult / boundSize.width;
        hfac = h*mult / boundSize.height;
        
        _modelMatrix = GLKMatrix4Scale(_modelMatrix,   wfac,  hfac, 1.f);
        _viewMatrix = GLKMatrix4Multiply(_viewMatrix, _modelMatrix);
        glUniformMatrix4fv(_matrixPos, 1, GL_FALSE, _viewMatrix.m);
        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef MAC_OS
        [bSelf.context flushBuffer];
#else
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
        [bSelf.context presentRenderbuffer:GL_RENDERBUFFER];
#endif
        YM_SET_OPENGL_CURRENT_CONTEXT(current)
        
    //});
}


- (void)setupYUVTexture:(int)width height:(int)height
{
    if (_textureYUV[0])
    {
        glDeleteTextures(3, _textureYUV);
    }
    glGenTextures(3, _textureYUV);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[0]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width, height, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, NULL);
#endif
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[1]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width/2, height/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, NULL);
#endif
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _textureYUV[2]);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef MAC_OS
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_EXT, width/2, height/2, 0, GL_RED_EXT, GL_UNSIGNED_BYTE, NULL);
#endif
    
}

- (void)setRenderPosition:(GLVideoRenderPosition)position{
    _positionNew = position;
}

- (void)setRenderBackgroudColor:(YMColor*) color{
    _renderBgColor = color;
}

- (void)clearFrame{
    
     VideoDrawYUV* bSelf = self;
    //youme_main_async(^{
        YMOpenGLContext* current = [YMOpenGLContext currentContext];
        YM_SET_OPENGL_CURRENT_CONTEXT(bSelf.context)
        CGFloat r, g, b, a;
        [_renderBgColor getRed: &r  green: &g blue:&b alpha:&a ];
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
#ifdef MAC_OS
        [bSelf.context flushBuffer];
#else
        [bSelf.context presentRenderbuffer:GL_RENDERBUFFER];
#endif
        YM_SET_OPENGL_CURRENT_CONTEXT(current)
    //});
    
}

- (void) positionChange:(GLVideoRenderPosition)position
{
    _position = position;
    if (GLVideoRenderPositionAll == position) {
        memcpy(_coordVertices, coordVertices, sizeof(_coordVertices));
    }
    else if(GLVideoRenderPositionLeft == position)
    {
        memcpy(_coordVertices, coordVertices, sizeof(_coordVertices));
        _coordVertices[2] = 0.5f;
        _coordVertices[6] = 0.5f;
    }
    else if(GLVideoRenderPositionRight == position)
    {
        memcpy(_coordVertices, coordVertices, sizeof(_coordVertices));
        _coordVertices[0] = 0.5f;
        _coordVertices[4] = 0.5f;
    }
}

- (void)setRenderMode:(GLVideoRenderMode)mode{
    _renderMode = mode;
}

@end




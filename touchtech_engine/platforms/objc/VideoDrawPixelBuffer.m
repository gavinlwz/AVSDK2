//
//  VideoDrawPixelBuffer.m
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//

#import "VideoDrawPixelBuffer.h"
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

@interface VideoDrawPixelBuffer(){
    
    GLuint _renderBuffer;
    GLuint _shaderProgram;
    GLuint _vbo;
    GLuint _fbo;
    GLuint _vao;
    GLuint _matrixPos;
    
    int _currentBuffer;
    int _currentUseBuffer;
    
    CVPixelBufferRef _currentRef[2];
    YMCVOpenGLTextureCacheRef _cache;
#ifdef MAC_OS
    GLint _texture;
    int  m_width;
    int  m_height;
#else
    YMCVOpenGLTextureRef _texture[2];
#endif
    
    GLKMatrix4 _modelMatrix;
    GLKMatrix4 _viewMatrix;
    GLVideoRenderPosition  _position;
    GLVideoRenderPosition  _positionNew;
    
    GLuint _dwPosition;
    GLuint _dwTexCoordIn;
    GLsizei   _viewScale;
    
    YMColor *   _renderBgColor;
    
    Boolean _pause;
    
    GLVideoRenderMode _renderMode;
    NSRecursiveLock *lock;
}
@property (nonatomic, strong) YMOpenGLContext* context;
@property (nonatomic, weak) YMView* ymView;
@property (atomic, assign) CGSize boundSize;
@end

@implementation VideoDrawPixelBuffer

- (instancetype)init:(YMOpenGLContext*)context view:(YMView*)view
{
    if ((self = [super init])) {
        lock = [[NSRecursiveLock alloc] init];
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
    [lock lock];
    _context = (YMOpenGLContext*)context;
    _ymView = view;
    _boundSize = [view bounds].size;
    _currentRef [0] = _currentRef[1] = nil;
#ifdef MAC_OS
    _texture = 0;
#else
    _texture[0] = _texture[1] = nil;
#endif
    _shaderProgram = 0;
    _renderBuffer = 0;
    _currentBuffer = 1;
    _currentUseBuffer = 0;
    _pause = NO;
    _renderMode = GLVideoRenderModeAdaptive;
#ifdef MAC_OS
    _viewScale = [[YMScreen mainScreen] backingScaleFactor];
    _renderBgColor = [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
    [self setupGLES];
    [self setupGLESBuffers];
    youme_main_async(^{
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationWillResignActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationWillResignActiveNotification object:nil];
        
    });
#else
    _viewScale = [YMScreen mainScreen].scale;
    _renderBgColor = [[YMColor alloc] initWithRed:0.0 green:0.0  blue:0.0  alpha:1.0];
    [self setupGLES];
    [self setupGLESBuffers];
    youme_main_async(^{
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationDidBecomeActiveNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(notification:) name:YMApplicationWillResignActiveNotification object:nil];
        
    });
#endif
    [lock unlock];
}

- (void) notification: (NSNotification*) notification {
    
    if([notification.name isEqualToString:YMApplicationWillResignActiveNotification]) {
        _pause = NO;
        //[self setupGLESBuffers];
    } else if([notification.name isEqualToString:YMApplicationWillResignActiveNotification]) {
        _pause = YES;
        //[self releaseGLESBuffers];
    }
}

-(void) releaseGLES
{
    [lock lock];
    VideoDrawPixelBuffer* bSelf = self;
    //youme_main_sync(^{
        YMOpenGLContext* current = [YMOpenGLContext currentContext];
        YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
        if(_shaderProgram) {
            glDeleteProgram(_shaderProgram);
        }
        if(_vbo) {
            glDeleteBuffers(1, &_vbo);
        }
        if(_vao) {
            glDeleteVertexArrays(1, &_vao);
        }
#ifdef MAC_OS
        [bSelf releaseGLESBuffers];
        //CVOpenGLTextureCacheFlush(_cache,0);
        //CFRelease(_cache);
        YM_SET_OPENGL_CURRENT_CONTEXT(current);
        [[NSNotificationCenter defaultCenter] removeObserver:self];
#else
        [bSelf releaseGLESBuffers];
        if(_cache)
        {
            YMCVOpenGLTextureCacheFlush(_cache,0);
            CFRelease(_cache);
            _cache = NULL;
        }
        
        YM_SET_OPENGL_CURRENT_CONTEXT(current);
        [[NSNotificationCenter defaultCenter] removeObserver:self];
#endif
    //});
    [lock unlock];
}

- (void) setupGLES
{
    [lock lock];
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
#ifdef MAC_OS
    //CVOpenGLTextureCacheCreate(kCFAllocatorDefault, NULL, self.context.CGLContextObj, self.context.pixelFormat.CGLPixelFormatObj, NULL, &_cache);
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);
#else
    CVOpenGLESTextureCacheCreate(kCFAllocatorDefault, NULL, self.context, NULL, &_cache);
    glGenVertexArraysOES(1, &_vao);
    glBindVertexArrayOES(_vao);
#endif

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_vbo), s_vbo, GL_STATIC_DRAW);
    
    _shaderProgram = build_program(s_vs_mat, s_fs);
    glUseProgram(_shaderProgram);
    
    int unitex = glGetUniformLocation(_shaderProgram, "uTex0");
    _dwPosition = glGetAttribLocation(_shaderProgram, "aPos");
    _dwTexCoordIn = glGetAttribLocation(_shaderProgram, "aCoord");
    _matrixPos = glGetUniformLocation(_shaderProgram, "uMat");
    
    glUniform1i(unitex, 0);
    
    glEnableVertexAttribArray(_dwPosition);
    glEnableVertexAttribArray(_dwTexCoordIn);
    glVertexAttribPointer(_dwPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, BUFFER_OFFSET(0));
    glVertexAttribPointer(_dwTexCoordIn, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, BUFFER_OFFSET(8));
    
    YM_SET_OPENGL_CURRENT_CONTEXT(current);
    [lock unlock];
}

- (void)setupGLESBuffers{
    [lock lock];
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
#ifndef MAC_OS
    if(_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
    }

    if(_fbo) {
        glDeleteFramebuffers(1, &_fbo);
    }
    glGenRenderbuffers(1, &_renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    [self.context renderbufferStorage:GL_RENDERBUFFER fromDrawable:_ymView.layer];
    GL_ERRORS(__LINE__)
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
#endif
    
    GL_ERRORS(__LINE__)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glClearColor(0.f, 0.f, 0.f, 1.f);
    //glViewport(0, 0, self.glLayer.bounds.size.width, self.glLayer.bounds.size.height);
    YM_SET_OPENGL_CURRENT_CONTEXT(current);
    [lock unlock];
    NSLog(@"OpenGLES View setupGLESBuffers!\n");
}

- (void)releaseGLESBuffers{
    [lock lock];
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
#ifdef MAC_OS
    if (0 != _texture) {
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
#else
    if(_texture[0]) {
        CFRelease(_texture[0]);
        _texture[0] = nil;
    }
    if(_texture[1]) {
        CFRelease(_texture[1]);
        _texture[1] = nil;
    }
#endif
    if(_currentRef[0]) {
        CVPixelBufferRelease(_currentRef[0]);
        _currentRef[0] = nil;
    }
    if(_currentRef[1]) {
        CVPixelBufferRelease(_currentRef[1]);
        _currentRef[1] = nil;
    }
    
    if(_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    if(_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }
    _currentUseBuffer = 0 ;
    YM_SET_OPENGL_CURRENT_CONTEXT(current);
    [lock unlock];
    NSLog(@"OpenGLES View releaseGLESBuffers!\n");
}
/*
 - (void) displayPixelBuffer:(CVPixelBufferRef)pixelBuffer
 {
 if(_pause || ++_currentUseBuffer > 2){
 if (_currentUseBuffer == 8) {
 _currentUseBuffer = 0;
 }
 else{
 return;
 }
 }
 
 bool updateTexture = false;
 if(pixelBuffer != _currentRef[_currentBuffer]) {
 _currentBuffer = !_currentBuffer;
 }
 if(pixelBuffer != _currentRef[_currentBuffer]) {
 if(_currentRef[_currentBuffer]){
 CVPixelBufferRelease(_currentRef[_currentBuffer]);
 }
 _currentRef[_currentBuffer] = CVPixelBufferRetain(pixelBuffer);
 updateTexture = true;
 }
 int currentBuffer = _currentBuffer;
 __block VideoDrawPixelBuffer* bSelf = self;
 youme_main_async( ^{
 CGSize boundSize = _glLayer.bounds.size;
 YMOpenGLContext* current = [YMOpenGLContext currentContext];
 [YMOpenGLContext setCurrentContext:bSelf.context];
 glViewport(0, 0, boundSize.width*_viewScale, boundSize.height*_viewScale);
 glClear(GL_COLOR_BUFFER_BIT);
 if(!bSelf->_currentRef[currentBuffer]){
 [YMOpenGLContext setCurrentContext:current];
 --_currentUseBuffer;
 return;
 }
 if(_positionNew != _position)
 [self positionChange:_positionNew];
 if(updateTexture) {
 if(bSelf->_texture[currentBuffer]) {
 CFRelease(bSelf->_texture[currentBuffer]);
 bSelf->_texture[currentBuffer] = NULL;
 }
 if(kCVReturnSuccess != CVPixelBufferLockBaseAddress(bSelf->_currentRef[currentBuffer], kCVPixelBufferLock_ReadOnly)){
 [YMOpenGLContext setCurrentContext:current];
 NSLog(@"pixelbuffer display lock error !!\n");
 --_currentUseBuffer;
 return;
 }
 GLsizei width = (GLsizei)CVPixelBufferGetWidth(bSelf->_currentRef[currentBuffer]);
 GLsizei height = (GLsizei)CVPixelBufferGetHeight(bSelf->_currentRef[currentBuffer]);
 YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
 bSelf->_cache,
 bSelf->_currentRef[currentBuffer],
 NULL,
 GL_TEXTURE_2D,
 GL_RGBA,
 width,
 height,
 GL_BGRA,
 GL_UNSIGNED_BYTE,
 0,
 &bSelf->_texture[currentBuffer]);
 
 glBindTexture(GL_TEXTURE_2D, YMCVOpenGLTextureGetName(bSelf->_texture[currentBuffer]));
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 CVPixelBufferUnlockBaseAddress(bSelf->_currentRef[currentBuffer], kCVPixelBufferLock_ReadOnly);
 
 }
 
 // draw
 glBindFramebuffer(GL_FRAMEBUFFER, bSelf->_fbo);
 glBindTexture(GL_TEXTURE_2D, YMCVOpenGLTextureGetName(bSelf->_texture[currentBuffer]));
 
 _modelMatrix = GLKMatrix4Identity;
 _viewMatrix = GLKMatrix4Identity;
 
 float width = CVPixelBufferGetWidth(bSelf->_currentRef[currentBuffer]);
 float height = CVPixelBufferGetHeight(bSelf->_currentRef[currentBuffer]);
 
 bool aspectFit = false;
 float wfac = boundSize.width / width;
 float hfac = boundSize.height / height;
 const float mult = (aspectFit ? (wfac < hfac) : (wfac > hfac)) ? wfac : hfac;
 
 wfac = width*mult / boundSize.width;
 hfac = height*mult / boundSize.height;
 
 _modelMatrix = GLKMatrix4Scale(_modelMatrix,   wfac,  -hfac, 1.f);
 _viewMatrix = GLKMatrix4Multiply(_viewMatrix, _modelMatrix);
 glUniformMatrix4fv(bSelf->_matrixPos, 1, GL_FALSE, _viewMatrix.m);
 glDrawArrays(GL_TRIANGLES, 0, 6);
 GL_ERRORS(__LINE__)
 glBindRenderbuffer(GL_RENDERBUFFER, bSelf->_renderBuffer);
 [bSelf.context presentRenderbuffer:GL_RENDERBUFFER];
 [YMOpenGLContext setCurrentContext:current];
 YMCVOpenGLTextureCacheFlush(_cache,0);
 --_currentUseBuffer;
 });
 
 }
 */

#ifdef MAC_OS
- (void) createTextureWidth:(int)nW Height:(int)nH
{
    if (0 != _texture)
        glDeleteTextures(1, &_texture);
    glGenTextures(1, &_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nW, nH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}
#endif
/*
int bmp_write(unsigned char *image, int xsize, int ysize, char *filename)
{
    unsigned char header[54] = {0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
        54, 0, 0, 0,40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 32, 0,
        0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0
    };
    long file_size = (long)xsize * (long)ysize * 4+ 54;
    header[2] =(unsigned char)(file_size &0x000000ff);
    header[3] = (file_size >> 8) &0x000000ff;
    header[4] =(file_size >> 16) & 0x000000ff;
    header[5] =(file_size >> 24) & 0x000000ff;
    long width =xsize;
    header[18] =width & 0x000000ff;
    header[19] =(width >> 8) &0x000000ff;
    header[20] =(width >> 16) &0x000000ff;
    header[21] =(width >> 24) &0x000000ff;
    long height= ysize;
    header[22] =height &0x000000ff;
    header[23] =(height >> 8) &0x000000ff;
    header[24] =(height >> 16) &0x000000ff;
    header[25] =(height >> 24) &0x000000ff;
    char fname_bmp[128] = {0};
    sprintf(fname_bmp, "%s.bmp", filename);
    
    FILE *fp;
    if (!(fp =fopen(fname_bmp, "wb")))
        return -1;
    fwrite(header, sizeof(unsigned char), 54, fp);
    fwrite(image, sizeof(unsigned char), (size_t)(long)xsize * ysize * 4,fp);
    fclose(fp);
    return 0;
}
int scount = 0;
void SaveRawPixels(int width, int height) {
    if (scount++ != 20)
        return;
    
    int size = width * height * 4;
    uint8_t* buf = (uint8_t*)malloc(size);
    memset(buf, 0, size);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    bmp_write(buf, width, height, "/Users/youmi/Desktop/test111");
    
    free(buf);
}
*/
- (void) displayPixelBuffer:(CVPixelBufferRef)pixelBuffer
{
    if(_pause){
        return;
    }
    [lock lock];
    @autoreleasepool {
    CVPixelBufferRef  pixelBufferRef = CVPixelBufferRetain(pixelBuffer);;
    VideoDrawPixelBuffer* bSelf = self;
    //youme_main_async( ^{
        //CGSize boundSize = [_ymView bounds].size;
        youme_main_async(^{
            if(bSelf && bSelf.ymView){
                self.boundSize = [bSelf.ymView bounds].size;
            }
        });
        CGSize boundSize = self.boundSize;
        YMOpenGLContext* current = [YMOpenGLContext currentContext];
        YM_SET_OPENGL_CURRENT_CONTEXT(bSelf.context)
        glViewport(0, 0, boundSize.width*_viewScale, boundSize.height*_viewScale);
        CGFloat r, g, b, a;
        [_renderBgColor getRed: &r  green: &g blue:&b alpha:&a ];
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
        // draw
        glBindFramebuffer(GL_FRAMEBUFFER, bSelf->_fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, bSelf->_renderBuffer);
#ifndef MAC_OS
        YMCVOpenGLTextureRef _texture = NULL;;
        if(_positionNew != _position)
            [self positionChange:_positionNew];
        
        CVPixelBufferLockBaseAddress(pixelBufferRef, kCVPixelBufferLock_ReadOnly);
        GLsizei width = (GLsizei)CVPixelBufferGetWidth(pixelBufferRef);
        GLsizei height = (GLsizei)CVPixelBufferGetHeight(pixelBufferRef);
        
        YMCVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
                                                     bSelf->_cache,
                                                     pixelBufferRef,
                                                     NULL,
                                                     GL_TEXTURE_2D,
                                                     GL_RGBA,
                                                     width,
                                                     height,
                                                     GL_BGRA,
                                                     GL_UNSIGNED_BYTE,
                                                     0,
                                                     &_texture);


        glBindTexture(GL_TEXTURE_2D, YMCVOpenGLTextureGetName(_texture));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CVPixelBufferUnlockBaseAddress(pixelBufferRef, kCVPixelBufferLock_ReadOnly);

#else
        
        if(_positionNew != _position)
            [self positionChange:_positionNew];
        
        CVPixelBufferLockBaseAddress(pixelBufferRef, kCVPixelBufferLock_ReadOnly);
        GLsizei width = (GLsizei)CVPixelBufferGetWidth(pixelBufferRef);
        GLsizei height = (GLsizei)CVPixelBufferGetHeight(pixelBufferRef);
        
        uint8_t* data = (uint8_t*)CVPixelBufferGetBaseAddress(pixelBufferRef);

        if(m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;
            [self createTextureWidth:width Height:height];
        }
        

        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        CVPixelBufferUnlockBaseAddress(pixelBufferRef, kCVPixelBufferLock_ReadOnly);
#endif
        _modelMatrix = GLKMatrix4Identity;
        _viewMatrix = GLKMatrix4Identity;
        
  
        float wfac = boundSize.width / (float)width;
        float hfac = boundSize.height / (float)height;
        float mult = 1.0f;
        if (_renderMode == GLVideoRenderModeHidden){
            mult = wfac > hfac ? wfac : hfac;
        }
        else if(_renderMode == GLVideoRenderModeFit){
            mult = wfac < hfac ? wfac : hfac;
        }
        else if(_renderMode == GLVideoRenderModeAdaptive){
            if ((boundSize.width - boundSize.height) * (width - height) >= 0) {
                
                mult = wfac > hfac ? wfac : hfac;
            }
            else{
                mult = wfac < hfac ? wfac : hfac;
            }
          
        }
        wfac = width*mult / boundSize.width;
        hfac = height*mult / boundSize.height;
        _modelMatrix = GLKMatrix4Scale(GLKMatrix4Identity,   wfac,  -hfac, 1.f);
        _viewMatrix = GLKMatrix4Multiply(GLKMatrix4Identity, _modelMatrix);
        glUniformMatrix4fv(bSelf->_matrixPos, 1, GL_FALSE, _viewMatrix.m);
#ifdef MAC_OS
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
#else
        glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
        GL_ERRORS(__LINE__)

#ifdef MAC_OS
        //SaveRawPixels(boundSize.width, boundSize.height);
        [bSelf.context flushBuffer];

        YM_SET_OPENGL_CURRENT_CONTEXT(current)
        //YMCVOpenGLTextureCacheFlush(_cache,0);
        CVPixelBufferRelease(pixelBuffer);
#else
        if(_ymView){
            [bSelf.context presentRenderbuffer:GL_RENDERBUFFER];
            YM_SET_OPENGL_CURRENT_CONTEXT(current)
            YMCVOpenGLTextureCacheFlush(_cache,0);
        }
        CFRelease(_texture);
        CVPixelBufferRelease(pixelBuffer);
#endif
    }
    //});
    [lock unlock];
}

- (void) positionChange:(NSUInteger)position
{
    _position = position;
    glUseProgram(_shaderProgram);
    int unitex = glGetUniformLocation(_shaderProgram, "uTex0");
    
    float* f_vbo = (float*)malloc(sizeof(s_vbo));
    memcpy(f_vbo, s_vbo, sizeof(s_vbo));
    
    if (position == GLVideoRenderPositionLeft) {
        f_vbo[6] = 0.5f;
        f_vbo[14] = 0.5f;
        f_vbo[18] = 0.5f;
    }
    else if(position == GLVideoRenderPositionRight)
    {
        f_vbo[2] = 0.5f;
        f_vbo[10] = 0.5f;
        f_vbo[22] = 0.5f;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_vbo), NULL, GL_STATIC_DRAW);
    GLvoid* ColorBuffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(s_vbo), GL_MAP_WRITE_BIT);
    memcpy(ColorBuffer, f_vbo, sizeof(s_vbo));
    glUnmapBuffer(GL_ARRAY_BUFFER);
    
    glUniform1i(unitex, 0);
    glEnableVertexAttribArray(_dwPosition);
    glEnableVertexAttribArray(_dwTexCoordIn);
    glVertexAttribPointer(_dwPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, BUFFER_OFFSET(0));
    glVertexAttribPointer(_dwTexCoordIn, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, BUFFER_OFFSET(8));
    free(f_vbo);
}

- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h{
    
}

- (void)setRenderPosition:(GLVideoRenderPosition)position{
    _positionNew = position;
}

- (void)setRenderBackgroudColor:(YMColor*) color{
    _renderBgColor = color;
}

- (void)clearFrame{
    [lock lock];
     VideoDrawPixelBuffer* bSelf = self;
//    youme_main_async(^{
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
//    });
    [lock unlock];
}

- (void)setRenderMode:(GLVideoRenderMode)mode{
    _renderMode = mode;
}

@end

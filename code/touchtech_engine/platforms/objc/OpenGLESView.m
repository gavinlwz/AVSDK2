//
//  OpenGLESView.m
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//

#include "config.h"
#import "OpenGLESView.h"
#import "GLESUtil.h"

#ifdef MAC_OS
#import <AppKit/NSOpenGL.h>
#import <AppKit/NSOpenGLLayer.h>
#import <AppKit/NSOpenGLView.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>
#import <QuartzCore/CAOpenGLLayer.h>
#else
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/CAEAGLLayer.h>
#endif

#import <GLKit/GLKit.h>
#import "IVideoDraw.h"
#import "VideoDrawYUV.h"
#import "VideoDrawPixelBuffer.h"


typedef NS_ENUM(NSInteger, GLESViewType) {
    GLESViewPixelbuffer = 0,
    GLESViewRawbuffer,
};

@interface  OpenGLESView(){
    YMColor *      _renderBgColor;
    GLVideoRenderPosition  _position;
    GLVideoRenderMode _renderMode;
    NSRecursiveLock *glLock;
}
@property (nonatomic, strong) YMOpenGLContext* context;
@property (nonatomic) YMOpenGLLayer* glLayer;
@property (atomic, retain) IVideoDraw* glVideoDraw;
@end

@implementation OpenGLESView



+ (Class) layerClass
{
    return [YMOpenGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        glLock = [[NSRecursiveLock alloc] init];
        [self initInternal];
    }
    return self;
}
- (instancetype) init {
    if ((self = [super init])) {
        [self initInternal];
    }
    return self;
}


- (void) initInternal {
    // Initialization code
    self.glLayer = (YMOpenGLLayer*)self.layer;
    self.glLayer.opaque = YES;
#ifdef MAC_OS
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAAllowOfflineRenderers,
        0,
        0
    };
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pf)
    {
        NSLog(@"No OpenGL pixel format");
    }
    self.context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
    [self setOpenGLContext:self.context];
    
    if ([self respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
    {
        [self setWantsBestResolutionOpenGLSurface:YES];
    }
    self.layer.contentsScale = [[YMScreen mainScreen] backingScaleFactor];
    _renderBgColor = [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
    
#else
    self.glLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                       kEAGLColorFormatRGB565, kEAGLDrawablePropertyColorFormat,
                                       nil];
    self.context = [[YMOpenGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
#if defined(DEBUG)
    CGLEnable([context CGLContextObj], kCGLCECrashOnRemovedFunctions);
#endif
    self.contentScaleFactor = [YMScreen mainScreen].scale;
    _renderBgColor = [[YMColor alloc] initWithRed:0.0 green:0.0  blue:0.0  alpha:1.0];
    
#endif
    _position = GLVideoRenderPositionAll;
    _renderMode = GLVideoRenderModeAdaptive;
    _glVideoDraw = [self createGLView:GLESViewPixelbuffer];
}
- (void) dealloc
{
}

#ifdef MAC_OS
- (CGColorRef)NSColorToCGColor:(NSColor *)color
{
    NSInteger numberOfComponents = [color numberOfComponents];
    CGFloat components[numberOfComponents];
    CGColorSpaceRef colorSpace = [[color colorSpace] CGColorSpace];
    [color getComponents:(CGFloat *)&components];
    CGColorRef cgColor = CGColorCreate(colorSpace, components);
    
    return cgColor;
}
#endif

- (void) layoutSubviews
{
    [glLock lock];
#ifdef MAC_OS
    self.layer.backgroundColor = [self NSColorToCGColor:[YMColor blackColor]];

    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    if(self.glVideoDraw){
        YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
        glViewport(0, 0, self.bounds.size.width*[[YMScreen mainScreen] backingScaleFactor], self.bounds.size.height*[[YMScreen mainScreen] backingScaleFactor]);
        CGFloat r, g, b, a;
        [_renderBgColor getRed: &r  green: &g blue:&b alpha:&a ];
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
        if(self.glVideoDraw){
            [self.glVideoDraw releaseGLESBuffers];
            [self.glVideoDraw setupGLESBuffers];
        }
        YM_SET_OPENGL_CURRENT_CONTEXT(current);
    }
#else
    self.backgroundColor = [UIColor blackColor];
    
    YMOpenGLContext* current = [YMOpenGLContext currentContext];
    if(self.glVideoDraw){
        YM_SET_OPENGL_CURRENT_CONTEXT(self.context);
        glViewport(0, 0, self.bounds.size.width*[UIScreen mainScreen].scale, self.bounds.size.height*[UIScreen mainScreen].scale);
        CGFloat r, g, b, a;
        [_renderBgColor getRed: &r  green: &g blue:&b alpha:&a ];
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
        if(self.glVideoDraw){
            [self.glVideoDraw releaseGLESBuffers];
            [self.glVideoDraw setupGLESBuffers];
        }
        YM_SET_OPENGL_CURRENT_CONTEXT(current);
    }
#endif
    [glLock unlock];
}

- (IVideoDraw*)createGLView:(GLESViewType)classType{
    OpenGLESView* bSelf = self;
    IVideoDraw* videoView;
    [glLock lock];
    if(classType == GLESViewPixelbuffer)
        videoView = [[VideoDrawPixelBuffer alloc] init:bSelf.context view:self];
    else
        videoView = [[VideoDrawYUV alloc] init:bSelf.context view:self];
    [videoView setRenderBackgroudColor:_renderBgColor];
    [videoView setRenderPosition:_position];
    [videoView setRenderMode:_renderMode];
    [glLock unlock];
    return videoView;
}

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer{
    CVPixelBufferRetain(pixelBuffer);;
    [glLock lock];
//    youme_main_sync(^{
        if(!self.glVideoDraw){
            self.glVideoDraw = [self createGLView:GLESViewPixelbuffer];
            
        }
        else if(![self.glVideoDraw isKindOfClass:[VideoDrawPixelBuffer class]]){
            self.glVideoDraw = nil;
            self.glVideoDraw = [self createGLView:GLESViewPixelbuffer];
        }
        
       [self.glVideoDraw displayPixelBuffer:pixelBuffer];
       CVPixelBufferRelease(pixelBuffer);
    [glLock unlock];
//    });
}

- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h{
    [glLock lock];
    //youme_main_sync(^{
        if(!self.glVideoDraw){
            self.glVideoDraw = [self createGLView:GLESViewRawbuffer];
        }
        else if(![self.glVideoDraw isKindOfClass:[VideoDrawYUV class]]){
            [self.glVideoDraw releaseGLES];
            self.glVideoDraw = nil;
            self.glVideoDraw = [self createGLView:GLESViewRawbuffer];
        }
        [self.glVideoDraw displayYUV420pData:data width:w height:h];
    [glLock unlock];
    //});
}

- (void)setRenderBackgroudColor:(YMColor*) color{
    if(self.glVideoDraw){
        [self.glVideoDraw setRenderBackgroudColor:color];
    }
    _renderBgColor = color;
}

- (void)clearFrame{
    if(self.glVideoDraw){
        [self.glVideoDraw clearFrame];
    }
}

- (void)setRenderPosition:(GLVideoRenderPosition)position{
    if(self.glVideoDraw){
        [self.glVideoDraw setRenderPosition:position];
    }
    _position = position;
}

- (void)setRenderMode:(GLVideoRenderMode)mode{
    if(self.glVideoDraw){
        [self.glVideoDraw setRenderMode:mode];
    }
    _renderMode = mode;
}

@end

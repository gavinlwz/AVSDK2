//
//  OpenGLView20.h
//  MyTest
//
//  Created by smy  on 12/20/11.
//  Copyright (c) 2011 ZY.SYM. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>
#include <sys/time.h>

//视频显示区域
typedef NS_ENUM(NSInteger, GLRenderPosition) {
    GLRenderPositionAll = 0,
    GLRenderPositionLeft,
    GLRenderPositionRight
};

@interface OpenGLView20 : UIView
{
	/** 
	 OpenGL绘图上下文
	 */
    EAGLContext             *_glContext; 
	
	/** 
	 帧缓冲区
	 */
    GLuint                  _framebuffer; 
	
	/** 
	 渲染缓冲区
	 */
    GLuint                  _renderBuffer; 
	
	/** 
	 着色器句柄
	 */
    GLuint                  _program;  
	
	/** 
	 YUV纹理数组
	 */
    GLuint                  _textureYUV[3]; 
	
	/** 
	 视频宽度
	 */
    GLuint                  _videoW;  
	
	/** 
	 视频高度
	 */
    GLuint                  _videoH;
    
    GLsizei                 _viewScale;
	   
    //void                    *_pYuvData;
    
    UIColor *            _renderBgColor;
    
#ifdef DEBUG
    struct timeval      _time;
    NSInteger           _frameRate;
#endif
}
#pragma mark - 接口
- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h;
- (void)setVideoSize:(GLuint)width height:(GLuint)height;

/**
 * 设置渲染的背景色
 */
- (void)setRenderBackgroudColor:(UIColor*) color;

/**
 *清除画面，清除后显示背景色
 */
- (void)clearFrame;

/**
 * 设置显示区域，0:全部，1：左边：2右边
 */
- (void)setRenderPosition:(GLRenderPosition)position;


@end

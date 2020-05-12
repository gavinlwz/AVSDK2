//
//  OpenGLESView.h
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//


#import <Foundation/Foundation.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif
#import <GLKit/GLKit.h>

typedef NS_ENUM(NSInteger, GLVideoRenderPosition) {
    GLVideoRenderPositionAll = 0,
    GLVideoRenderPositionLeft,
    GLVideoRenderPositionRight
};

typedef NS_ENUM(NSInteger, GLVideoRenderMode) {
    GLVideoRenderModeHidden = 0,
    GLVideoRenderModeFit = 1,
    GLVideoRenderModeAdaptive = 2
};

#if TARGET_OS_OSX
@interface OpenGLESView : NSOpenGLView
#else
@interface OpenGLESView : UIView
#endif

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;


- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h;

/**
 * 设置渲染的背景色
 */
#if TARGET_OS_OSX
- (void)setRenderBackgroudColor:(NSColor*) color;
#else
- (void)setRenderBackgroudColor:(UIColor*) color;
#endif

/**
 *清除画面，清除后显示背景色
 */
- (void)clearFrame;

/**
 * 设置显示区域，0:全部，1：左边：2右边
 */
- (void)setRenderPosition:(GLVideoRenderPosition)position;

- (void)setRenderMode:(GLVideoRenderMode)mode;
@end


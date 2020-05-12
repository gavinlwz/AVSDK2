//
//  IVideoDraw.h
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//

#include "config.h"
#import <Foundation/Foundation.h>
#ifdef MAC_OS
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif
#import <GLKit/GLKit.h>
#import "OpenGLESView.h"



static void youme_main_async(dispatch_block_t block){
    
    if ([NSThread isMainThread]) {
        block();
    }
    else{
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

static void youme_main_sync(dispatch_block_t block){
    
    if ([NSThread isMainThread]) {
        block();
    }
    else{
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

@interface IVideoDraw : NSObject

- (instancetype)init:(YMOpenGLContext*)context view:(YMView*)view;

- (void)setupGLESBuffers;

- (void)releaseGLESBuffers;

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;

- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h;

- (void)setRenderBackgroudColor:(YMColor*) color;

- (void)clearFrame;

- (void)releaseGLES;

- (void)setRenderPosition:(GLVideoRenderPosition)position;

- (void)setRenderMode:(GLVideoRenderMode)mode;
@end




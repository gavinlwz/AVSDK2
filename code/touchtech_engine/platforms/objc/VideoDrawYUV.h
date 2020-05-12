//
//  VideoDrawYUV.h
//  youme_voice_engine
//
//  Created by bhb on 2018/1/22.
//  Copyright © 2018年 Youme. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "IVideoDraw.h"

@interface VideoDrawYUV : IVideoDraw

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


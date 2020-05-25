//
//  IVideoDraw.m
//  youme_voice_engine
//
//  Created by bhb on 2018/1/23.
//  Copyright © 2018年 Youme. All rights reserved.
//

#import "IVideoDraw.h"

@implementation IVideoDraw
- (instancetype)init:(YMOpenGLContext*)context glLayer:(YMOpenGLLayer*)layer{
    if ((self = [super init])) {
    }
    return self;
}

- (void)setupGLESBuffers{
    
}

- (void)releaseGLESBuffers{
    
}

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer size:(CGSize)size{
    
}

- (void)displayYUV420pData:(void *)data width:(NSInteger)w height:(NSInteger)h size:(CGSize)size{
    
}

- (void)setRenderBackgroudColor:(YMColor*) color{
    
}

- (void)clearFrame{
    
}

- (void)setRenderPosition:(GLVideoRenderPosition)position{
    
}

- (void)setRenderMode:(GLVideoRenderMode)mode{
    
}

@end

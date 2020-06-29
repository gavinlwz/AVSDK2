//
//  RenderGroupMacView.h
//  YmTalkTest
//
//  Created by zalejiang on 2017/11/27.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import <AppKit/AppKit.h>
#import "OpenGLESView.h"

@interface RenderGroupMacView : NSView
// OpenGL ES
@property (atomic,strong)  NSMutableArray<OpenGLESView*>* openGlViews;
@property (atomic, strong) NSString* strChooseUser;


/*
 */
-(BOOL)applyRenderViewWithTag:(NSString*)tag;

-(BOOL)deattachRenderViewWithTag:(NSString*)tag;

-(void)cleanAll;

-(void)displayRenderViewWithData:(void *)data width:(NSInteger)w height:(NSInteger)h tag:(NSString*)strTag;

-(void)displayRenderViewWithData:(CVPixelBufferRef)pixelBuffer tag:(NSString*)strTag;
@end

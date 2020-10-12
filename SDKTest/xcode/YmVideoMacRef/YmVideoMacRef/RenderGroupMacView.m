//
//  RenderGroupMacView.m
//  YmTalkTest
//
//  Created by zalejiang on 2017/11/27.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import "RenderGroupMacView.h"

@interface RenderGroupMacView ()
@property (atomic,assign) int lastAllocIndex;
@property (atomic,strong) NSMutableDictionary *tagToRenderView;
@property (atomic, strong) NSMutableArray* arrayCacheOrder;
@end

@implementation RenderGroupMacView

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code
}
*/

-(id)init {
    if (self = [super init]) {
        [self configRenderViews];
    }
    
    return self;
}

-(id)initWithCoder:(NSCoder *)aDecoder {
    if (self = [super initWithCoder:aDecoder]) {
        [self configRenderViews];
    }
    
    return self;
    
}


-(void)configRenderViews {
    self.openGlViews = [NSMutableArray new];
    self.tagToRenderView = [NSMutableDictionary dictionaryWithCapacity:0];
    self.arrayCacheOrder = [NSMutableArray arrayWithCapacity:0];
    self.strChooseUser = @"";
    self.lastAllocIndex = 0;
    //==========================      创建渲染组件      ==========================================================
    int renderViewMargin = 5;
    CGRect viewSize = [self frame];
    int renderViewWidth = (CGRectGetWidth(viewSize) - 25) / 2;
    int renderViewHeight = renderViewWidth * 3 / 4;
    //创建四个视频渲染组件
    OpenGLESView* glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(renderViewMargin, CGRectGetHeight(viewSize) - renderViewMargin - renderViewHeight , renderViewWidth, renderViewHeight)];
    [self.openGlViews addObject:glView];
    [glView setRenderBackgroudColor:  [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.0f] ];
    [self addSubview:glView];
    [glView addGestureRecognizer:[[NSGestureRecognizer alloc] initWithTarget:self action:@selector(handleClickTap:)]];

    glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(renderViewWidth + 2* renderViewMargin, CGRectGetHeight(viewSize) - renderViewMargin - renderViewHeight, renderViewWidth, renderViewHeight)];
    [glView setRenderBackgroudColor:  [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f] ];
    [self.openGlViews addObject:glView];
    [self addSubview:glView];
    [glView addGestureRecognizer:[[NSGestureRecognizer alloc] initWithTarget:self action:@selector(handleClickTap:)]];
     
    glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(renderViewMargin, CGRectGetHeight(viewSize) - renderViewMargin - 2 * renderViewHeight , renderViewWidth, renderViewHeight)];
    [glView setRenderBackgroudColor:  [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f] ];
    [self.openGlViews addObject:glView];
    [self addSubview:glView];
    [glView addGestureRecognizer:[[NSGestureRecognizer alloc] initWithTarget:self action:@selector(handleClickTap:)]];
    
    glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(renderViewWidth + 2* renderViewMargin, CGRectGetHeight(viewSize) - renderViewMargin - 2 * renderViewHeight, renderViewWidth, renderViewHeight)];
    [glView setRenderBackgroudColor:  [NSColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f] ];
    [self.openGlViews addObject:glView];
    [self addSubview:glView];
    [glView addGestureRecognizer:[[NSGestureRecognizer alloc] initWithTarget:self action:@selector(handleClickTap:)]];
}


-(void)handleClickTap:(NSGestureRecognizer*)gest {
    NSView* view = [gest view];
    
    for( NSString* key in [self.tagToRenderView allKeys] ){
        if( self.tagToRenderView[key] == view ){
            self.strChooseUser =  key;
            return ;
        }
    }
}


-(BOOL)applyRenderViewWithTag:(NSString*)tag {
    if (_lastAllocIndex >= self.openGlViews.count) {
        return NO;
    }
    
    [self.arrayCacheOrder addObject:tag];

    [_tagToRenderView setObject:self.openGlViews[_lastAllocIndex] forKey:tag];
    
    self.openGlViews[_lastAllocIndex].hidden = NO;

    
    _lastAllocIndex++;
    return YES;
}

-(BOOL)deattachRenderViewWithTag:(NSString*)tag {
    if (![_tagToRenderView objectForKey:tag]) {
        return NO;
    }
    
    [self.arrayCacheOrder removeObject:tag];
    
    for (NSInteger nIndex = 0; nIndex < self.arrayCacheOrder.count; nIndex++) {
        NSString* strKeyEnum = [self.arrayCacheOrder objectAtIndex:nIndex];
        [_tagToRenderView setObject:self.openGlViews[nIndex] forKey:strKeyEnum];
    }
    
    [_tagToRenderView removeObjectForKey:tag];
    
    //
    _lastAllocIndex--;
    
    [self.openGlViews[_lastAllocIndex] clearFrame];
    
    [self.openGlViews[_lastAllocIndex] setHidden:YES];
    
    
    return YES;
}


-(void)displayRenderViewWithData:(void *)data width:(NSInteger)w height:(NSInteger)h tag:(NSString*)strTag {
    OpenGLESView* view20 = [_tagToRenderView objectForKey:strTag];
    if (view20) {
        [view20 displayYUV420pData:data width:w height:h];
    }
}

-(void)displayRenderViewWithData:(CVPixelBufferRef)pixelBuffer tag:(NSString*)strTag{
    OpenGLESView* view20 = [_tagToRenderView objectForKey:strTag];
    if (view20) {
        [view20 displayPixelBuffer:pixelBuffer];
    }
}



-(void)cleanAll {
    _lastAllocIndex = 0;
    
    [self.tagToRenderView removeAllObjects];
    
    [self.arrayCacheOrder removeAllObjects];
    
    [self.openGlViews enumerateObjectsUsingBlock:^(OpenGLESView * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
        [obj clearFrame];
        obj.hidden = YES;
    }];
    
   
    
    
}

@end

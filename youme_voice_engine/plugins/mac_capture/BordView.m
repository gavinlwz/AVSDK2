//
//  BordView.m
//  MacAVLearn
//
//  Created by pinky on 2020/4/28.
//  Copyright © 2020 pinky. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BordView.h"

@interface  BordView()
{
    int m_borderSize ;
}
@end

@implementation BordView

-(id)init
{
    self = [super init];
    m_borderSize = 10;
    return self;
}

-(void) setBorderSize:(int)size
{
    m_borderSize = size;
}

-(void)drawRect:(NSRect)dirtyRect
{
    NSRect rect = [self bounds];
    
    //线的宽度是向两边扩散的，所以要调整下坐标
    rect.origin.x = m_borderSize/2;
    rect.origin.y = m_borderSize/2;
    rect.size.width -= m_borderSize;
    rect.size.height -=   m_borderSize;

    [[NSColor colorWithRed:0 green:0.8 blue:0 alpha:1] set];
    NSBezierPath *path = [NSBezierPath bezierPathWithRect:rect];
    path.lineWidth = m_borderSize;
    [path stroke];

    [super drawRect:dirtyRect];
}

@end

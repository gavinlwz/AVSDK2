//
//  BordWindow.m
//  MacAVLearn
//
//  Created by pinky on 2020/4/27.
//  Copyright © 2020 pinky. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "BordWindow.h"
#import "BordView.h"

@interface BordWindow()
{
    BordView* m_view;
}
@end

@implementation  BordWindow

-(id) initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag
{
    if (self = [super initWithContentRect: contentRect
                                styleMask: NSBorderlessWindowMask | NSResizableWindowMask
                                  backing: bufferingType
                                    defer: TRUE])
    {

        [self setOpaque:NO];
        [self setBackgroundColor:[NSColor clearColor]];
        
        m_view = [[BordView alloc] initWithFrame: NSMakeRect( 0, 0 , self.frame.size.width, self.frame.size.height )];
        [self.contentView addSubview: m_view];
        
        [[NSNotificationCenter defaultCenter] addObserver: self
        selector:@selector(windowDidResize:)
        name:NSWindowDidResizeNotification
        object:self];

    }

    return self;

}

- (void)windowDidResize:(NSNotification *)notification
{
    [m_view setFrame: NSMakeRect( 0, 0,  self.frame.size.width,  self.frame.size.height )];
    [m_view setNeedsDisplay: true ];
}

-(void) setBorderSize:(int)size
{
    [m_view setBorderSize: size ];
}

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen
{
  //return the unaltered frame, or do some other interesting things
  return frameRect;
}



-(void) dealloc
{
    [[NSNotificationCenter defaultCenter]  removeObserver: self ];
}

@end


@implementation  YMWindowController

@end

//
//  NSObject+SelectorOnMainThreadMutipleObjects.h
//  YmTalkTest
//
//  Created by zalejiang on 2017/11/27.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSObject (SelectorOnMainThreadMutipleObjects)
- (void) performSelectorOnMainThread:(SEL)selector withObject:(id)arg1 withObject:(id)arg2 withObject:(id)arg3 waitUntilDone:(BOOL)wait;
@end

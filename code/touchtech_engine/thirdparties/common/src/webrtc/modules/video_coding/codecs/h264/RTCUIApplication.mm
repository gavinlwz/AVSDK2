/*
 *  Copyright 2016 The WebRTC@AnyRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "RTCUIApplication.h"

#if defined(WEBRTC_IOS)

#ifdef MAC_OS
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

bool RTCIsUIApplicationActive() {
  UIApplicationState state = [UIApplication sharedApplication].applicationState;
  return state == UIApplicationStateActive;
}

#endif // WEBRTC_IOS

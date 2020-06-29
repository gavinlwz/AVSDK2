/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"

#if defined(WEBRTC_IOS)
#ifdef MAC_OS
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif
#endif
namespace youme{
    namespace webrtc {

        bool IsH264CodecSupportedObjC() {
        #if defined(WEBRTC_OBJC_H264) && defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
            // Supported on iOS8+ or Mac OSX 10.8+
            return true;
        #else
            // TODO(tkchin): Support OS/X once we stop mixing libstdc++ and libc++ on
            // OSX 10.9.
            return false;
        #endif
        }

    }  // namespace webrtc
}

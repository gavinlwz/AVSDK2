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

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_

#if !defined(_WIN32) && !defined(OS_LINUX)
#define HARDWARE_CODECS     1
#endif

#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)

#include <Availability.h>
#if (defined(__IPHONE_8_0) &&                            \
     __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_8_0) || \
    (defined(__MAC_10_8) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_8)
#define WEBRTC_VIDEO_TOOLBOX_SUPPORTED 1
#endif

#endif  // defined(WEBRTC_IOS) || defined(WEBRTC_MAC)

#include "webrtc/modules/video_coding/include/video_codec_interface.h"
#pragma pack(push, 1) //字节对齐
typedef struct tagCodecSource{
    int fmt;
    int textureid;
    float matrix[16];
}CodecSource;
#pragma pack(pop)
namespace youme {
namespace webrtc {

class H264Encoder : public VideoEncoder {
 public:
  static VideoEncoder* Create();
  static bool IsSupported();
  static bool IsSupportedGLES();
  static void SetEGLContext(void* eglContext);
  virtual ~H264Encoder() override {}
};

class H264Decoder : public VideoDecoder {
 public:
  static VideoDecoder* Create();
  static bool IsSupported();
  static void SetEGLContext(void* eglContext);
 virtual ~H264Decoder() override {}
};
    
class H264SpsParser{
public:
    static bool SpsParser(uint8_t* buff, int buffLen, uint32_t* width, uint32_t* height);
};
    
}  // namespace webrtc
}  // namespace youme
#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_INCLUDE_H264_H_

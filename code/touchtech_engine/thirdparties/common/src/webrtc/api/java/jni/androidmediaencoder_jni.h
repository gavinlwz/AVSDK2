/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_API_JAVA_JNI_ANDROIDMEDIAENCODER_JNI_H_
#define WEBRTC_API_JAVA_JNI_ANDROIDMEDIAENCODER_JNI_H_

#include <vector>

#include "webrtc/api/java/jni/jni_helpers.h"
#include "webrtc/media/engine/webrtcvideoencoderfactory.h"

namespace youme {
namespace webrtc {

// Implementation of Android MediaCodec based encoder factory.
class MediaCodecVideoEncoderFactory {
 public:
  MediaCodecVideoEncoderFactory();
  virtual ~MediaCodecVideoEncoderFactory();

  static void SetEGLContext(jobject egl_context);
  static bool IsSupported();
  static bool IsSupportedGLES();
  static webrtc::VideoEncoder* CreateVideoEncoder();
 private:
  static jobject eglbase_;
  static jobject egl_context_;
  // Empty if platform support is lacking, const after ctor returns.
  std::vector<VideoCodec> supported_codecs_;
};

}  // namespace webrtc
}  // namespace youme

#endif  // WEBRTC_API_JAVA_JNI_ANDROIDMEDIAENCODER_JNI_H_

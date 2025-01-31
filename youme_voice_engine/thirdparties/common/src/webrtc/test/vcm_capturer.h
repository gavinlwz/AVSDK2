﻿/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_VCM_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_VCM_CAPTURER_H_

#include "webrtc/base/criticalsection.h"
#include "webrtc/common_types.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_capture/video_capture.h"
#include "webrtc/test/video_capturer.h"

namespace webrtc {
namespace test {

class VcmCapturer : public VideoCapturer, public VideoCaptureDataCallback {
 public:
  static VcmCapturer* Create(VideoCaptureInput* input,
                             size_t width,
                             size_t height,
                             size_t target_fps);
  virtual ~VcmCapturer();

  void Start() override;
  void Stop() override;

  void OnIncomingCapturedFrame(const int32_t id,
                               const VideoFrame& frame) override;  // NOLINT
  void OnCaptureDelayChanged(const int32_t id, const int32_t delay) override;

 private:
  explicit VcmCapturer(VideoCaptureInput* input);
  bool Init(size_t width, size_t height, size_t target_fps);
  void Destroy();

  rtc::CriticalSection crit_;
  bool started_ GUARDED_BY(crit_);
  VideoCaptureModule* vcm_;
  VideoCaptureCapability capability_;
};
}  // test
}  // webrtc

#endif  // WEBRTC_VIDEO_ENGINE_TEST_COMMON_VCM_CAPTURER_H_

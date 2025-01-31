﻿/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_DEFLICKERING_H_
#define WEBRTC_MODULES_VIDEO_PROCESSING_DEFLICKERING_H_

#include <string.h>  // NULL

#include "webrtc/modules/video_processing/include/video_processing.h"
#include "webrtc/typedefs.h"

namespace youme {
namespace webrtc {

class VPMDeflickering {
 public:
  VPMDeflickering();
  ~VPMDeflickering();

  void Reset();
  int32_t ProcessFrame(VideoFrame* frame,
                       VideoProcessingModule::FrameStats* stats);

 private:
  int32_t PreDetection(uint32_t timestamp,
                       const VideoProcessingModule::FrameStats& stats);

  int32_t DetectFlicker();

  enum { kMeanBufferLength = 32 };
  enum { kFrameHistory_size = 15 };
  enum { kNumProbs = 12 };
  enum { kNumQuants = kNumProbs + 2 };
  enum { kMaxOnlyLength = 5 };

  uint32_t  mean_buffer_length_;
  uint8_t   detection_state_;    // 0: No flickering
                                // 1: Flickering detected
                                // 2: In flickering
  int32_t    mean_buffer_[kMeanBufferLength];
  uint32_t   timestamp_buffer_[kMeanBufferLength];
  uint32_t   frame_rate_;
  static const uint16_t prob_uw16_[kNumProbs];
  static const uint16_t weight_uw16_[kNumQuants - kMaxOnlyLength];
  uint8_t quant_hist_uw8_[kFrameHistory_size][kNumQuants];
};

}  // namespace webrtc
}  // namespace youme

#endif // WEBRTC_MODULES_VIDEO_PROCESSING_DEFLICKERING_H_

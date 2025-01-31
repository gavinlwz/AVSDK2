﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * frame_preprocessor.h
 */
#ifndef WEBRTC_MODULES_VIDEO_PROCESSING_FRAME_PREPROCESSOR_H_
#define WEBRTC_MODULES_VIDEO_PROCESSING_FRAME_PREPROCESSOR_H_

#include "webrtc/modules/video_processing/include/video_processing.h"
#include "webrtc/modules/video_processing/content_analysis.h"
#include "webrtc/modules/video_processing/spatial_resampler.h"
#include "webrtc/modules/video_processing/video_decimator.h"
#include "webrtc/typedefs.h"

namespace youme {
namespace webrtc {

class VPMFramePreprocessor {
 public:
  VPMFramePreprocessor();
  ~VPMFramePreprocessor();

  void Reset();

  // Enable temporal decimation.
  void EnableTemporalDecimation(bool enable);

  void SetInputFrameResampleMode(VideoFrameResampling resampling_mode);

  // Enable content analysis.
  void EnableContentAnalysis(bool enable);

  // Set target resolution: frame rate and dimension.
  int32_t SetTargetResolution(uint32_t width, uint32_t height,
                              uint32_t frame_rate);

  // Set target frame rate.
  void SetTargetFramerate(int frame_rate);

  // Update incoming frame rate/dimension.
  void UpdateIncomingframe_rate();

  int32_t updateIncomingFrameSize(uint32_t width, uint32_t height);

  // Set decimated values: frame rate/dimension.
  uint32_t Decimatedframe_rate();
  uint32_t DecimatedWidth() const;
  uint32_t DecimatedHeight() const;

  // Preprocess output:
  int32_t PreprocessFrame(const VideoFrame& frame,
                          VideoFrame** processed_frame);
  VideoContentMetrics* ContentMetrics() const;

 private:
  // The content does not change so much every frame, so to reduce complexity
  // we can compute new content metrics every |kSkipFrameCA| frames.
  enum { kSkipFrameCA = 2 };

  VideoContentMetrics* content_metrics_;
  VideoFrame resampled_frame_;
  VPMSpatialResampler* spatial_resampler_;
  VPMContentAnalysis* ca_;
  VPMVideoDecimator* vd_;
  bool enable_ca_;
  int frame_cnt_;

};

}  // namespace webrtc
}  // namespace youme

#endif // WEBRTC_MODULES_VIDEO_PROCESSING_FRAME_PREPROCESSOR_H_

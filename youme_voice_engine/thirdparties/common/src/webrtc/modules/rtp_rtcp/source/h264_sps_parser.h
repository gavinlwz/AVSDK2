﻿/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_H264_SPS_PARSER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_H264_SPS_PARSER_H_

#include "webrtc/base/common.h"

namespace youme {
namespace webrtc {

// A class for parsing out sequence parameter set (SPS) data from an H264 NALU.
// Currently, only resolution is read without being ignored.
class H264SpsParser {
 public:
  H264SpsParser(const uint8_t* sps, size_t byte_length);
  // Parses the SPS to completion. Returns true if the SPS was parsed correctly.
  bool Parse();
  uint16_t width() { return width_; }
  uint16_t height() { return height_; }

 private:
  const uint8_t* const sps_;
  const size_t byte_length_;

  uint16_t width_;
  uint16_t height_;
};

}  // namespace webrtc
}  // namespace youme
#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_H264_SPS_PARSER_H_

﻿/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_REPORT_BLOCK_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_REPORT_BLOCK_H_

#include "webrtc/base/basictypes.h"

namespace youme {
namespace webrtc {
namespace rtcp {

class ReportBlock {
 public:
  static const size_t kLength = 24;

  ReportBlock();
  ~ReportBlock() {}

  bool Parse(const uint8_t* buffer, size_t length);

  // Fills buffer with the ReportBlock.
  // Consumes ReportBlock::kLength bytes.
  void Create(uint8_t* buffer) const;

  void To(uint32_t ssrc) { source_ssrc_ = ssrc; }
  void WithFractionLost(uint8_t fraction_lost) {
    fraction_lost_ = fraction_lost;
  }
  bool WithCumulativeLost(uint32_t cumulative_lost);
  void WithExtHighestSeqNum(uint32_t ext_highest_seq_num) {
    extended_high_seq_num_ = ext_highest_seq_num;
  }
  void WithJitter(uint32_t jitter) { jitter_ = jitter; }
  void WithLastSr(uint32_t last_sr) { last_sr_ = last_sr; }
  void WithDelayLastSr(uint32_t delay_last_sr) {
    delay_since_last_sr_ = delay_last_sr;
  }

  uint32_t source_ssrc() const { return source_ssrc_; }
  uint8_t fraction_lost() const { return fraction_lost_; }
  uint32_t cumulative_lost() const { return cumulative_lost_; }
  uint32_t extended_high_seq_num() const { return extended_high_seq_num_; }
  uint32_t jitter() const { return jitter_; }
  uint32_t last_sr() const { return last_sr_; }
  uint32_t delay_since_last_sr() const { return delay_since_last_sr_; }

 private:
  uint32_t source_ssrc_;
  uint8_t fraction_lost_;
  uint32_t cumulative_lost_;
  uint32_t extended_high_seq_num_;
  uint32_t jitter_;
  uint32_t last_sr_;
  uint32_t delay_since_last_sr_;
};

}  // namespace rtcp
}  // namespace webrtc
}  // namespace youme
#endif  // WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_PACKET_REPORT_BLOCK_H_

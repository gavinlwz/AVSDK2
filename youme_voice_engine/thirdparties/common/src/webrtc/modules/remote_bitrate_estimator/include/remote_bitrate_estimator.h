﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This class estimates the incoming available bandwidth.

#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_

#include <map>
#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/modules/include/module.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;

// RemoteBitrateObserver is used to signal changes in bitrate estimates for
// the incoming streams.
class RemoteBitrateObserver {
 public:
  // Called when a receive channel group has a new bitrate estimate for the
  // incoming streams.
  virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs,
                                       unsigned int bitrate) = 0;

  virtual ~RemoteBitrateObserver() {}
};

struct ReceiveBandwidthEstimatorStats {
  ReceiveBandwidthEstimatorStats() : total_propagation_time_delta_ms(0) {}

  // The "propagation_time_delta" of a frame is defined as (d_arrival - d_sent),
  // where d_arrival is the delta of the arrival times of the frame and the
  // previous frame, d_sent is the delta of the sent times of the frame and
  // the previous frame. The sent time is calculated from the RTP timestamp.

  // |total_propagation_time_delta_ms| is the sum of the propagation_time_deltas
  // of all received frames, except that it's is adjusted to 0 when it becomes
  // negative.
  int total_propagation_time_delta_ms;
  // The propagation_time_deltas for the frames arrived in the last
  // kProcessIntervalMs using the clock passed to
  // RemoteBitrateEstimatorFactory::Create.
  std::vector<int> recent_propagation_time_delta_ms;
  // The arrival times for the frames arrived in the last kProcessIntervalMs
  // using the clock passed to RemoteBitrateEstimatorFactory::Create.
  std::vector<int64_t> recent_arrival_time_ms;
};

class RemoteBitrateEstimator : public CallStatsObserver, public Module {
 public:
  static const int kDefaultMinBitrateBps = 30000;
  virtual ~RemoteBitrateEstimator() {}

  virtual void IncomingPacketFeedbackVector(
      const std::vector<PacketInfo>& packet_feedback_vector) {
    assert(false);
  }

  // Called for each incoming packet. Updates the incoming payload bitrate
  // estimate and the over-use detector. If an over-use is detected the
  // remote bitrate estimate will be updated. Note that |payload_size| is the
  // packet size excluding headers.
  // Note that |arrival_time_ms| can be of an arbitrary time base.
  virtual void IncomingPacket(int64_t arrival_time_ms,
                              size_t payload_size,
                              const RTPHeader& header,
                              bool was_paced) = 0;

  // Removes all data for |ssrc|.
  virtual void RemoveStream(unsigned int ssrc) = 0;

  // Returns true if a valid estimate exists and sets |bitrate_bps| to the
  // estimated payload bitrate in bits per second. |ssrcs| is the list of ssrcs
  // currently being received and of which the bitrate estimate is based upon.
  virtual bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                              unsigned int* bitrate_bps) const = 0;

  // Returns true if the statistics are available.
  virtual bool GetStats(ReceiveBandwidthEstimatorStats* output) const = 0;

  virtual void SetMinBitrate(int min_bitrate_bps) = 0;

 protected:
  static const int64_t kProcessIntervalMs = 500;
  static const int64_t kStreamTimeOutMs = 2000;
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_INCLUDE_REMOTE_BITRATE_ESTIMATOR_H_

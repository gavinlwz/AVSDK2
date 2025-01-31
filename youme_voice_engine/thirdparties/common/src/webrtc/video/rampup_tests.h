﻿/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_VIDEO_RAMPUP_TESTS_H_
#define WEBRTC_VIDEO_RAMPUP_TESTS_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/call.h"
#include "webrtc/call/transport_adapter.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/system_wrappers/include/event_wrapper.h"
#include "webrtc/test/call_test.h"

namespace webrtc {

static const int kTransmissionTimeOffsetExtensionId = 6;
static const int kAbsSendTimeExtensionId = 7;
static const int kTransportSequenceNumberExtensionId = 8;
static const unsigned int kSingleStreamTargetBps = 1000000;

class Clock;
class PacketRouter;
class ReceiveStatistics;
class RtpHeaderParser;
class RTPPayloadRegistry;
class RtpRtcp;

class RampUpTester : public test::EndToEndTest {
 public:
  RampUpTester(size_t num_streams,
               unsigned int start_bitrate_bps,
               const std::string& extension_type,
               bool rtx,
               bool red);
  ~RampUpTester() override;

  void PerformTest() override;

 protected:
  virtual bool PollStats();

  void AccumulateStats(const VideoSendStream::StreamStats& stream,
                       size_t* total_packets_sent,
                       size_t* total_sent,
                       size_t* padding_sent,
                       size_t* media_sent) const;

  void ReportResult(const std::string& measurement,
                    size_t value,
                    const std::string& units) const;
  void TriggerTestDone();

  rtc::Event event_;
  Clock* const clock_;
  FakeNetworkPipe::Config forward_transport_config_;
  const size_t num_streams_;
  const bool rtx_;
  const bool red_;
  VideoSendStream* send_stream_;
  test::PacketTransport* send_transport_;

 private:
  typedef std::map<uint32_t, uint32_t> SsrcMap;

  Call::Config GetSenderCallConfig() override;
  void OnStreamsCreated(
      VideoSendStream* send_stream,
      const std::vector<VideoReceiveStream*>& receive_streams) override;
  void OnTransportsCreated(test::PacketTransport* send_transport,
                           test::PacketTransport* receive_transport) override;
  size_t GetNumStreams() const;
  void ModifyConfigs(VideoSendStream::Config* send_config,
                     std::vector<VideoReceiveStream::Config>* receive_configs,
                     VideoEncoderConfig* encoder_config) override;
  void OnCallsCreated(Call* sender_call, Call* receiver_call) override;

  static bool BitrateStatsPollingThread(void* obj);

  const int start_bitrate_bps_;
  bool start_bitrate_verified_;
  int expected_bitrate_bps_;
  int64_t test_start_ms_;
  int64_t ramp_up_finished_ms_;

  const std::string extension_type_;
  std::vector<uint32_t> ssrcs_;
  std::vector<uint32_t> rtx_ssrcs_;
  SsrcMap rtx_ssrc_map_;

  rtc::scoped_ptr<ThreadWrapper> poller_thread_;
  Call* sender_call_;
};

class RampUpDownUpTester : public RampUpTester {
 public:
  RampUpDownUpTester(size_t num_streams,
                     unsigned int start_bitrate_bps,
                     const std::string& extension_type,
                     bool rtx,
                     bool red);
  ~RampUpDownUpTester() override;

 protected:
  bool PollStats() override;

 private:
  static const int kHighBandwidthLimitBps = 80000;
  static const int kExpectedHighBitrateBps = 60000;
  static const int kLowBandwidthLimitBps = 20000;
  static const int kExpectedLowBitrateBps = 20000;
  enum TestStates { kFirstRampup, kLowRate, kSecondRampup };

  Call::Config GetReceiverCallConfig() override;

  std::string GetModifierString() const;
  void EvolveTestState(int bitrate_bps, bool suspended);

  TestStates test_state_;
  int64_t state_start_ms_;
  int64_t interval_start_ms_;
  int sent_bytes_;
};
}  // namespace webrtc
#endif  // WEBRTC_VIDEO_RAMPUP_TESTS_H_

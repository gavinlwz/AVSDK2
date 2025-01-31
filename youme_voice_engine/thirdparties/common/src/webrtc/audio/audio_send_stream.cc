﻿/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/audio/audio_send_stream.h"

#include <string>

#include "webrtc/audio/audio_state.h"
#include "webrtc/audio/conversion.h"
#include "webrtc/audio/scoped_voe_interface.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_rtp_rtcp.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"

namespace webrtc {
std::string AudioSendStream::Config::Rtp::ToString() const {
  std::stringstream ss;
  ss << "{ssrc: " << ssrc;
  ss << ", extensions: [";
  for (size_t i = 0; i < extensions.size(); ++i) {
    ss << extensions[i].ToString();
    if (i != extensions.size() - 1) {
      ss << ", ";
    }
  }
  ss << ']';
  ss << ", c_name: " << c_name;
  ss << '}';
  return ss.str();
}

std::string AudioSendStream::Config::ToString() const {
  std::stringstream ss;
  ss << "{rtp: " << rtp.ToString();
  ss << ", voe_channel_id: " << voe_channel_id;
  // TODO(solenberg): Encoder config.
  ss << ", cng_payload_type: " << cng_payload_type;
  ss << ", red_payload_type: " << red_payload_type;
  ss << '}';
  return ss.str();
}

namespace internal {

AudioSendStream::AudioSendStream(
    const webrtc::AudioSendStream::Config& config,
    const rtc::scoped_refptr<webrtc::AudioState>& audio_state)
    : config_(config), audio_state_(audio_state) {
  LOG(LS_INFO) << "AudioSendStream: " << config_.ToString();
  RTC_DCHECK_NE(config_.voe_channel_id, -1);
  RTC_DCHECK(audio_state_.get());

  const int channel_id = config.voe_channel_id;
  ScopedVoEInterface<VoERTP_RTCP> rtp(voice_engine());
  int error = rtp->SetRTCPStatus(channel_id, true);
  RTC_DCHECK_EQ(0, error);
  error = rtp->SetLocalSSRC(channel_id, config.rtp.ssrc);
  RTC_DCHECK_EQ(0, error);
  error = rtp->SetRTCP_CNAME(channel_id, config.rtp.c_name.c_str());
  RTC_DCHECK_EQ(0, error);
  for (const auto& extension : config.rtp.extensions) {
    // One-byte-extension local identifiers are in the range 1-14 inclusive.
    RTC_DCHECK_GE(extension.id, 1);
    RTC_DCHECK_LE(extension.id, 14);
    if (extension.name == RtpExtension::kAbsSendTime) {
      error = rtp->SetSendAbsoluteSenderTimeStatus(channel_id, true,
                                                   extension.id);
      RTC_DCHECK_EQ(0, error);
    } else if (extension.name == RtpExtension::kAudioLevel) {
      error = rtp->SetSendAudioLevelIndicationStatus(channel_id, true,
                                                     extension.id);
      RTC_DCHECK_EQ(0, error);
    } else {
      RTC_NOTREACHED() << "Registering unsupported RTP extension.";
    }
  }
}

AudioSendStream::~AudioSendStream() {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  LOG(LS_INFO) << "~AudioSendStream: " << config_.ToString();
}

void AudioSendStream::Start() {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
}

void AudioSendStream::Stop() {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
}

void AudioSendStream::SignalNetworkState(NetworkState state) {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
}

bool AudioSendStream::DeliverRtcp(const uint8_t* packet, size_t length) {
  // TODO(solenberg): Tests call this function on a network thread, libjingle
  // calls on the worker thread. We should move towards always using a network
  // thread. Then this check can be enabled.
  // RTC_DCHECK(!thread_checker_.CalledOnValidThread());
  return false;
}

webrtc::AudioSendStream::Stats AudioSendStream::GetStats() const {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  webrtc::AudioSendStream::Stats stats;
  stats.local_ssrc = config_.rtp.ssrc;
  ScopedVoEInterface<VoEAudioProcessing> processing(voice_engine());
  ScopedVoEInterface<VoECodec> codec(voice_engine());
  ScopedVoEInterface<VoERTP_RTCP> rtp(voice_engine());
  ScopedVoEInterface<VoEVolumeControl> volume(voice_engine());

  webrtc::CallStatistics call_stats = {0};
  int error = rtp->GetRTCPStatistics(config_.voe_channel_id, call_stats);
  RTC_DCHECK_EQ(0, error);
  stats.bytes_sent = call_stats.bytesSent;
  stats.packets_sent = call_stats.packetsSent;
  // RTT isn't known until a RTCP report is received. Until then, VoiceEngine
  // returns 0 to indicate an error value.
  if (call_stats.rttMs > 0) {
    stats.rtt_ms = call_stats.rttMs;
  }
  // TODO(solenberg): [was ajm]: Re-enable this metric once we have a reliable
  //                  implementation.
  stats.aec_quality_min = -1;

  webrtc::CodecInst codec_inst = {0};
  if (codec->GetSendCodec(config_.voe_channel_id, codec_inst) != -1) {
    RTC_DCHECK_NE(codec_inst.pltype, -1);
    stats.codec_name = codec_inst.plname;

    // Get data from the last remote RTCP report.
    std::vector<webrtc::ReportBlock> blocks;
    error = rtp->GetRemoteRTCPReportBlocks(config_.voe_channel_id, &blocks);
    RTC_DCHECK_EQ(0, error);
    for (const webrtc::ReportBlock& block : blocks) {
      // Lookup report for send ssrc only.
      if (block.source_SSRC == stats.local_ssrc) {
        stats.packets_lost = block.cumulative_num_packets_lost;
        stats.fraction_lost = Q8ToFloat(block.fraction_lost);
        stats.ext_seqnum = block.extended_highest_sequence_number;
        // Convert samples to milliseconds.
        if (codec_inst.plfreq / 1000 > 0) {
          stats.jitter_ms =
              block.interarrival_jitter / (codec_inst.plfreq / 1000);
        }
        break;
      }
    }
  }

  // Local speech level.
  {
    unsigned int level = 0;
    error = volume->GetSpeechInputLevelFullRange(level);
    RTC_DCHECK_EQ(0, error);
    stats.audio_level = static_cast<int32_t>(level);
  }

  bool echo_metrics_on = false;
  error = processing->GetEcMetricsStatus(echo_metrics_on);
  RTC_DCHECK_EQ(0, error);
  if (echo_metrics_on) {
    // These can also be negative, but in practice -1 is only used to signal
    // insufficient data, since the resolution is limited to multiples of 4 ms.
    int median = -1;
    int std = -1;
    float dummy = 0.0f;
    error = processing->GetEcDelayMetrics(median, std, dummy);
    RTC_DCHECK_EQ(0, error);
    stats.echo_delay_median_ms = median;
    stats.echo_delay_std_ms = std;

    // These can take on valid negative values, so use the lowest possible level
    // as default rather than -1.
    int erl = -100;
    int erle = -100;
    int dummy1 = 0;
    int dummy2 = 0;
    error = processing->GetEchoMetrics(erl, erle, dummy1, dummy2);
    RTC_DCHECK_EQ(0, error);
    stats.echo_return_loss = erl;
    stats.echo_return_loss_enhancement = erle;
  }

  internal::AudioState* audio_state =
      static_cast<internal::AudioState*>(audio_state_.get());
  stats.typing_noise_detected = audio_state->typing_noise_detected();

  return stats;
}

const webrtc::AudioSendStream::Config& AudioSendStream::config() const {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  return config_;
}

VoiceEngine* AudioSendStream::voice_engine() const {
  internal::AudioState* audio_state =
      static_cast<internal::AudioState*>(audio_state_.get());
  VoiceEngine* voice_engine = audio_state->voice_engine();
  RTC_DCHECK(voice_engine);
  return voice_engine;
}
}  // namespace internal
}  // namespace webrtc

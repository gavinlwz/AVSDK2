﻿/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/main/acm2/codec_manager.h"

#include "webrtc/base/checks.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/acm2/rent_a_codec.h"
#include "webrtc/system_wrappers/include/trace.h"

namespace webrtc {
namespace acm2 {

namespace {

// Check if the given codec is a valid to be registered as send codec.
int IsValidSendCodec(const CodecInst& send_codec) {
  int dummy_id = 0;
  if ((send_codec.channels != 1) && (send_codec.channels != 2)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                 "Wrong number of channels (%d, only mono and stereo are "
                 "supported)",
                 send_codec.channels);
    return -1;
  }

  auto maybe_codec_id = RentACodec::CodecIdByInst(send_codec);
  if (!maybe_codec_id) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                 "Invalid codec setting for the send codec.");
    return -1;
  }

  // Telephone-event cannot be a send codec.
  if (!STR_CASE_CMP(send_codec.plname, "telephone-event")) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                 "telephone-event cannot be a send codec");
    return -1;
  }

  if (!RentACodec::IsSupportedNumChannels(*maybe_codec_id, send_codec.channels)
           .value_or(false)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                 "%d number of channels not supportedn for %s.",
                 send_codec.channels, send_codec.plname);
    return -1;
  }
  return RentACodec::CodecIndexFromId(*maybe_codec_id).value_or(-1);
}

bool IsIsac(const CodecInst& codec) {
  return
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
      !STR_CASE_CMP(codec.plname, "isac") ||
#endif
      false;
}

bool IsOpus(const CodecInst& codec) {
  return
#ifdef WEBRTC_CODEC_OPUS
      !STR_CASE_CMP(codec.plname, "opus") ||
#endif
      false;
}

bool IsPcmU(const CodecInst& codec) {
  return !STR_CASE_CMP(codec.plname, "pcmu");
}

bool IsPcmA(const CodecInst& codec) {
  return !STR_CASE_CMP(codec.plname, "pcma");
}

bool IsPcm16B(const CodecInst& codec) {
  return !STR_CASE_CMP(codec.plname, "l16");
}

bool IsIlbc(const CodecInst& codec) {
  return
#ifdef WEBRTC_CODEC_ILBC
      !STR_CASE_CMP(codec.plname, "ilbc") ||
#endif
      false;
}

bool IsG722(const CodecInst& codec) {
  return
#ifdef WEBRTC_CODEC_G722
      !STR_CASE_CMP(codec.plname, "g722") ||
#endif
      false;
}

bool CodecSupported(const CodecInst& codec) {
  return IsOpus(codec) || IsPcmU(codec) || IsPcmA(codec) || IsPcm16B(codec) ||
         IsIlbc(codec) || IsG722(codec) || IsIsac(codec);
}

const CodecInst kEmptyCodecInst = {-1, "noCodecRegistered", 0, 0, 0, 0};
}  // namespace

CodecManager::CodecManager()
    : dtx_enabled_(false),
      vad_mode_(VADNormal),
      send_codec_inst_(kEmptyCodecInst),
      red_enabled_(false),
      codec_fec_enabled_(false),
      encoder_is_opus_(false) {
  // Register the default payload types for RED and CNG.
  for (const CodecInst& ci : RentACodec::Database()) {
    RentACodec::RegisterCngPayloadType(&cng_payload_types_, ci);
    RentACodec::RegisterRedPayloadType(&red_payload_types_, ci);
  }
  thread_checker_.DetachFromThread();
}

CodecManager::~CodecManager() = default;

int CodecManager::RegisterEncoder(const CodecInst& send_codec) {
  RTC_DCHECK(thread_checker_.CalledOnValidThread());
  int codec_id = IsValidSendCodec(send_codec);

  // Check for reported errors from function IsValidSendCodec().
  if (codec_id < 0) {
    return -1;
  }

  int dummy_id = 0;
  switch (RentACodec::RegisterRedPayloadType(&red_payload_types_, send_codec)) {
    case RentACodec::RegistrationResult::kOk:
      return 0;
    case RentACodec::RegistrationResult::kBadFreq:
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                   "RegisterSendCodec() failed, invalid frequency for RED"
                   " registration");
      return -1;
    case RentACodec::RegistrationResult::kSkip:
      break;
  }
  switch (RentACodec::RegisterCngPayloadType(&cng_payload_types_, send_codec)) {
    case RentACodec::RegistrationResult::kOk:
      return 0;
    case RentACodec::RegistrationResult::kBadFreq:
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, dummy_id,
                   "RegisterSendCodec() failed, invalid frequency for CNG"
                   " registration");
      return -1;
    case RentACodec::RegistrationResult::kSkip:
      break;
  }

  // Set Stereo, and make sure VAD and DTX is turned off.
  if (send_codec.channels != 1) {
    if (dtx_enabled_) {
      WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, dummy_id,
                   "VAD/DTX is turned off, not supported when sending stereo.");
    }
    dtx_enabled_ = false;
  }

  // Check if the codec is already registered as send codec.
  bool new_codec = true;
  if (CurrentEncoder()) {
    auto new_codec_id = RentACodec::CodecIdByInst(send_codec_inst_);
    RTC_DCHECK(new_codec_id);
    auto old_codec_id = RentACodec::CodecIdFromIndex(codec_id);
    new_codec = !old_codec_id || *new_codec_id != *old_codec_id;
  }

  if (RedPayloadType(send_codec.plfreq) == -1) {
    red_enabled_ = false;
  }

  encoder_is_opus_ = IsOpus(send_codec);

  if (new_codec) {
    // This is a new codec. Register it and return.
    RTC_DCHECK(CodecSupported(send_codec));
    if (IsOpus(send_codec)) {
      // VAD/DTX not supported.
      dtx_enabled_ = false;
    }
    AudioEncoder* enc = rent_a_codec_.RentEncoder(send_codec);
    if (!enc)
      return -1;
    RentEncoderStack(enc, send_codec.plfreq);
    RTC_DCHECK(CurrentEncoder());

    codec_fec_enabled_ = codec_fec_enabled_ &&
                         enc->SetFec(codec_fec_enabled_);

    send_codec_inst_ = send_codec;
    return 0;
  }

  // This is an existing codec; re-create it if any parameters have changed.
  if (send_codec_inst_.plfreq != send_codec.plfreq ||
      send_codec_inst_.pacsize != send_codec.pacsize ||
      send_codec_inst_.channels != send_codec.channels) {
    AudioEncoder* enc = rent_a_codec_.RentEncoder(send_codec);
    if (!enc)
      return -1;
    RentEncoderStack(enc, send_codec.plfreq);
    RTC_DCHECK(CurrentEncoder());
  }
  send_codec_inst_.plfreq = send_codec.plfreq;
  send_codec_inst_.pacsize = send_codec.pacsize;
  send_codec_inst_.channels = send_codec.channels;
  send_codec_inst_.pltype = send_codec.pltype;

  // Check if a change in Rate is required.
  if (send_codec.rate != send_codec_inst_.rate) {
    CurrentEncoder()->SetTargetBitrate(send_codec.rate);
    send_codec_inst_.rate = send_codec.rate;
  }

  codec_fec_enabled_ =
      codec_fec_enabled_ && CurrentEncoder()->SetFec(codec_fec_enabled_);

  return 0;
}

void CodecManager::RegisterEncoder(AudioEncoder* external_speech_encoder) {
  // Make up a CodecInst.
  send_codec_inst_.channels = external_speech_encoder->NumChannels();
  send_codec_inst_.plfreq = external_speech_encoder->SampleRateHz();
  send_codec_inst_.pacsize = rtc::CheckedDivExact(
      static_cast<int>(external_speech_encoder->Max10MsFramesInAPacket() *
                       send_codec_inst_.plfreq),
      100);
  send_codec_inst_.pltype = -1;  // Not valid.
  send_codec_inst_.rate = -1;    // Not valid.
  static const char kName[] = "external";
  memcpy(send_codec_inst_.plname, kName, sizeof(kName));

  if (send_codec_inst_.channels != 1)
    dtx_enabled_ = false;
  if (codec_fec_enabled_) {
    // Switch FEC on. On failure, remember that FEC is off.
    if (!external_speech_encoder->SetFec(true))
      codec_fec_enabled_ = false;
  } else {
    // Switch FEC off. This shouldn't fail.
    const bool success = external_speech_encoder->SetFec(false);
    RTC_DCHECK(success);
  }

  RentEncoderStack(external_speech_encoder,
                   external_speech_encoder->SampleRateHz());
}

rtc::Optional<CodecInst> CodecManager::GetCodecInst() const {
  int dummy_id = 0;
  WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, dummy_id,
               "SendCodec()");

  if (!CurrentEncoder()) {
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, dummy_id,
                 "SendCodec Failed, no codec is registered");
    return rtc::Optional<CodecInst>();
  }
  return rtc::Optional<CodecInst>(send_codec_inst_);
}

bool CodecManager::SetCopyRed(bool enable) {
  if (enable && codec_fec_enabled_) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, 0,
                 "Codec internal FEC and RED cannot be co-enabled.");
    return false;
  }
  if (enable && RedPayloadType(send_codec_inst_.plfreq) == -1) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, 0,
                 "Cannot enable RED at %i Hz.", send_codec_inst_.plfreq);
    return false;
  }
  if (red_enabled_ != enable) {
    red_enabled_ = enable;
    if (CurrentEncoder())
      RentEncoderStack(rent_a_codec_.GetEncoder(), send_codec_inst_.plfreq);
  }
  return true;
}

int CodecManager::SetVAD(bool enable, ACMVADMode mode) {
  // Sanity check of the mode.
  RTC_DCHECK(mode == VADNormal || mode == VADLowBitrate || mode == VADAggr ||
             mode == VADVeryAggr);

  // Check that the send codec is mono. We don't support VAD/DTX for stereo
  // sending.
  auto* enc = rent_a_codec_.GetEncoder();
  const bool stereo_send = enc ? (enc->NumChannels() != 1) : false;
  if (enable && stereo_send) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, 0,
                 "VAD/DTX not supported for stereo sending");
    dtx_enabled_ = false;
    return -1;
  }

  // If a send codec is registered, set VAD/DTX for the codec.
  if (IsOpus(send_codec_inst_)) {
    // VAD/DTX not supported.
    dtx_enabled_ = false;
    return 0;
  }

  if (dtx_enabled_ != enable || vad_mode_ != mode) {
    dtx_enabled_ = enable;
    vad_mode_ = mode;
    if (enc)
      RentEncoderStack(enc, send_codec_inst_.plfreq);
  }
  return 0;
}

void CodecManager::VAD(bool* dtx_enabled,
                       bool* vad_enabled,
                       ACMVADMode* mode) const {
  *dtx_enabled = dtx_enabled_;
  *vad_enabled = dtx_enabled_;
  *mode = vad_mode_;
}

int CodecManager::SetCodecFEC(bool enable_codec_fec) {
  if (enable_codec_fec == true && red_enabled_ == true) {
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, 0,
                 "Codec internal FEC and RED cannot be co-enabled.");
    return -1;
  }

  RTC_CHECK(CurrentEncoder());
  codec_fec_enabled_ =
      CurrentEncoder()->SetFec(enable_codec_fec) && enable_codec_fec;
  return codec_fec_enabled_ == enable_codec_fec ? 0 : -1;
}

AudioDecoder* CodecManager::GetAudioDecoder(const CodecInst& codec) {
  return IsIsac(codec) ? rent_a_codec_.RentIsacDecoder() : nullptr;
}

int CodecManager::CngPayloadType(int rtp_timestamp_rate_hz) const {
  RTC_CHECK(rtp_timestamp_rate_hz == 8000 || rtp_timestamp_rate_hz == 16000 ||
            rtp_timestamp_rate_hz == 32000 || rtp_timestamp_rate_hz == 48000)
      << rtp_timestamp_rate_hz << " Hz is not supported";
  auto it = cng_payload_types_.find(rtp_timestamp_rate_hz);
  return it == cng_payload_types_.end() ? -1 : it->second;
}

int CodecManager::RedPayloadType(int rtp_timestamp_rate_hz) const {
  RTC_CHECK(rtp_timestamp_rate_hz == 8000 || rtp_timestamp_rate_hz == 16000 ||
            rtp_timestamp_rate_hz == 32000 || rtp_timestamp_rate_hz == 48000)
      << rtp_timestamp_rate_hz << " Hz is not supported";
  auto it = red_payload_types_.find(rtp_timestamp_rate_hz);
  return it == red_payload_types_.end() ? -1 : it->second;
}

void CodecManager::RentEncoderStack(AudioEncoder* speech_encoder,
                                    int sample_rate_hz) {
  auto cng_config =
      dtx_enabled_ ? rtc::Optional<RentACodec::CngConfig>(RentACodec::CngConfig{
                         CngPayloadType(sample_rate_hz), vad_mode_})
                   : rtc::Optional<RentACodec::CngConfig>();
  auto red_pt = red_enabled_
                    ? rtc::Optional<int>(RedPayloadType(sample_rate_hz))
                    : rtc::Optional<int>();
  rent_a_codec_.RentEncoderStack(speech_encoder, cng_config, red_pt);
}

}  // namespace acm2
}  // namespace webrtc

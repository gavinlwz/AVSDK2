﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/noise_suppression_impl.h"

#include <assert.h>

#include "webrtc/modules/audio_processing/audio_buffer.h"
#if defined(WEBRTC_NS_FLOAT)
#include "webrtc/modules/audio_processing/ns/noise_suppression.h"
#elif defined(WEBRTC_NS_FIXED)
#include "webrtc/modules/audio_processing/ns/noise_suppression_x.h"
#endif
#include "webrtc/system_wrappers/include/critical_section_wrapper.h"


namespace webrtc {

#if defined(WEBRTC_NS_FLOAT)
typedef NsHandle Handle;
#elif defined(WEBRTC_NS_FIXED)
typedef NsxHandle Handle;
#endif

namespace {
int MapSetting(NoiseSuppression::Level level) {
  switch (level) {
    case NoiseSuppression::kLow:
      return 0;
    case NoiseSuppression::kModerate:
      return 1;
    case NoiseSuppression::kHigh:
      return 2;
    case NoiseSuppression::kVeryHigh:
      return 3;
  }
  assert(false);
  return -1;
}
}  // namespace

NoiseSuppressionImpl::NoiseSuppressionImpl(const AudioProcessing* apm,
                                           CriticalSectionWrapper* crit)
  : ProcessingComponent(),
    apm_(apm),
    crit_(crit),
    level_(kModerate) {}

NoiseSuppressionImpl::~NoiseSuppressionImpl() {}

int NoiseSuppressionImpl::AnalyzeCaptureAudio(AudioBuffer* audio) {
#if defined(WEBRTC_NS_FLOAT)
  if (!is_component_enabled()) {
    return apm_->kNoError;
  }
  assert(audio->num_frames_per_band() <= 160);
  assert(audio->num_channels() == num_handles());

  for (int i = 0; i < num_handles(); ++i) {
    Handle* my_handle = static_cast<Handle*>(handle(i));

    WebRtcNs_Analyze(my_handle, audio->split_bands_const_f(i)[kBand0To8kHz]);
  }
#endif
  return apm_->kNoError;
}

int NoiseSuppressionImpl::ProcessCaptureAudio(AudioBuffer* audio) {
  if (!is_component_enabled()) {
    return apm_->kNoError;
  }
  assert(audio->num_frames_per_band() <= 160);
  assert(audio->num_channels() == num_handles());

  for (int i = 0; i < num_handles(); ++i) {
    Handle* my_handle = static_cast<Handle*>(handle(i));
#if defined(WEBRTC_NS_FLOAT)
    WebRtcNs_Process(my_handle,
                     audio->split_bands_const_f(i),
                     audio->num_bands(),
                     audio->split_bands_f(i));
#elif defined(WEBRTC_NS_FIXED)
    WebRtcNsx_Process(my_handle,
                      audio->split_bands_const(i),
                      audio->num_bands(),
                      audio->split_bands(i));
#endif
  }
  return apm_->kNoError;
}

int NoiseSuppressionImpl::Enable(bool enable) {
  CriticalSectionScoped crit_scoped(crit_);
  return EnableComponent(enable);
}

bool NoiseSuppressionImpl::is_enabled() const {
  return is_component_enabled();
}

int NoiseSuppressionImpl::set_level(Level level) {
  CriticalSectionScoped crit_scoped(crit_);
  if (MapSetting(level) == -1) {
    return apm_->kBadParameterError;
  }

  level_ = level;
  return Configure();
}

NoiseSuppression::Level NoiseSuppressionImpl::level() const {
  return level_;
}

float NoiseSuppressionImpl::speech_probability() const {
#if defined(WEBRTC_NS_FLOAT)
  float probability_average = 0.0f;
  for (int i = 0; i < num_handles(); i++) {
    Handle* my_handle = static_cast<Handle*>(handle(i));
    probability_average += WebRtcNs_prior_speech_probability(my_handle);
  }
  return probability_average / num_handles();
#elif defined(WEBRTC_NS_FIXED)
  // Currently not available for the fixed point implementation.
  return apm_->kUnsupportedFunctionError;
#endif
}

void* NoiseSuppressionImpl::CreateHandle() const {
#if defined(WEBRTC_NS_FLOAT)
  return WebRtcNs_Create();
#elif defined(WEBRTC_NS_FIXED)
  return WebRtcNsx_Create();
#endif
}

void NoiseSuppressionImpl::DestroyHandle(void* handle) const {
#if defined(WEBRTC_NS_FLOAT)
  WebRtcNs_Free(static_cast<Handle*>(handle));
#elif defined(WEBRTC_NS_FIXED)
  WebRtcNsx_Free(static_cast<Handle*>(handle));
#endif
}

int NoiseSuppressionImpl::InitializeHandle(void* handle) const {
#if defined(WEBRTC_NS_FLOAT)
  return WebRtcNs_Init(static_cast<Handle*>(handle),
                       apm_->proc_sample_rate_hz());
#elif defined(WEBRTC_NS_FIXED)
  return WebRtcNsx_Init(static_cast<Handle*>(handle),
                        apm_->proc_sample_rate_hz());
#endif
}

int NoiseSuppressionImpl::ConfigureHandle(void* handle) const {
#if defined(WEBRTC_NS_FLOAT)
  return WebRtcNs_set_policy(static_cast<Handle*>(handle),
                             MapSetting(level_));
#elif defined(WEBRTC_NS_FIXED)
  return WebRtcNsx_set_policy(static_cast<Handle*>(handle),
                              MapSetting(level_));
#endif
}

int NoiseSuppressionImpl::num_handles_required() const {
  return apm_->num_output_channels();
}

int NoiseSuppressionImpl::GetHandleError(void* handle) const {
  // The NS has no get_error() function.
  assert(handle != NULL);
  return apm_->kUnspecifiedError;
}
}  // namespace webrtc

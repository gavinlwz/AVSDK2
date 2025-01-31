﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/common_audio/swap_queue.h"
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/processing_component.h"

namespace webrtc {

class AudioBuffer;
class CriticalSectionWrapper;

class EchoControlMobileImpl : public EchoControlMobile,
                              public ProcessingComponent {
 public:
  EchoControlMobileImpl(const AudioProcessing* apm,
                        CriticalSectionWrapper* crit);
  virtual ~EchoControlMobileImpl();

  int ProcessRenderAudio(const AudioBuffer* audio);
  int ProcessCaptureAudio(AudioBuffer* audio);

  // EchoControlMobile implementation.
  bool is_enabled() const override;
  RoutingMode routing_mode() const override;
  bool is_comfort_noise_enabled() const override;

  // ProcessingComponent implementation.
  int Initialize() override;

  // Reads render side data that has been queued on the render call.
  void ReadQueuedRenderData();

 private:
  // EchoControlMobile implementation.
  int Enable(bool enable) override;
  int set_routing_mode(RoutingMode mode) override;
  int enable_comfort_noise(bool enable) override;
  int SetEchoPath(const void* echo_path, size_t size_bytes) override;
  int GetEchoPath(void* echo_path, size_t size_bytes) const override;

  // ProcessingComponent implementation.
  void* CreateHandle() const override;
  int InitializeHandle(void* handle) const override;
  int ConfigureHandle(void* handle) const override;
  void DestroyHandle(void* handle) const override;
  int num_handles_required() const override;
  int GetHandleError(void* handle) const override;

  void AllocateRenderQueue();

  const AudioProcessing* apm_;
  CriticalSectionWrapper* crit_;
  RoutingMode routing_mode_;
  bool comfort_noise_enabled_;
  unsigned char* external_echo_path_;

  size_t render_queue_element_max_size_;
  std::vector<int16_t> render_queue_buffer_;
  std::vector<int16_t> capture_queue_buffer_;
  rtc::scoped_ptr<
      SwapQueue<std::vector<int16_t>, RenderQueueItemVerifier<int16_t>>>
      render_signal_queue_;
};
}  // namespace webrtc

#endif  // WEBRTC_MODULES_AUDIO_PROCESSING_ECHO_CONTROL_MOBILE_IMPL_H_

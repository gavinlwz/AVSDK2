/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_NEW_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_NEW_H_

#include "webrtc/common_audio/ring_buffer.h"
#include "aec_core_new.h"

namespace youme{
namespace webrtc_new {
        
typedef struct {
  int delayCtr;
  int sampFreq;
  int splitSampFreq;
  int scSampFreq;
  float sampFactor;  // scSampRate / sampFreq
  short skewMode;
  int bufSizeStart;
  int knownDelay;
  int rate_factor;

  short initFlag;  // indicates if AEC has been initialized

  // Variables used for averaging far end buffer size
  short counter;
  int sum;
  short firstVal;
  short checkBufSizeCtr;

  // Variables used for delay shifts
  short msInSndCardBuf;
  short filtDelay;  // Filtered delay estimate.
  int timeForDelayChange;
  int startup_phase;
  int checkBuffSize;
  short lastDelayDiff;

#ifdef WEBRTC_AEC_DEBUG_DUMP
  FILE* bufFile;
  FILE* delayFile;
  FILE* skewFile;
#endif

  // Structures
  void* resampler;

  int skewFrCtr;
  int resample;  // if the skew is small enough we don't resample
  int highSkewCtr;
  float skew;

  RingBuffer* far_pre_buf;  // Time domain far-end pre-buffer.
  RingBuffer* media_pre_buf;  // Time domain local media pre-buffer.

  int lastError;

  int farend_started;
  int startup_phase_delay_change;

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DEVICE_DELAY
  float FarBufTooShort;
  int16_t play_delay_ms;
  int16_t rec_delay_ms;
  int16_t debug_tmparr[48000 * 2];
  int16_t debug_tmparr_cp[48000 * 2];
  int16_t debug_tmparr_w_pos;
  int16_t debug_tmparr_r_pos;
  int16_t non_causality_cnt;
#endif

  AecCore* aec;
} Aec;

    }  // namespace webrtc_new
}  // namespace youme

#endif  // WEBRTC_MODULES_AUDIO_PROCESSING_AEC_ECHO_CANCELLATION_INTERNAL_NEW_H_

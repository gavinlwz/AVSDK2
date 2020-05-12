/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "delay_estimator_wrapper_new.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__APPLE__)
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#endif

#include "delay_estimator_new.h"
#include "delay_estimator_internal_new.h"
#ifndef __cplusplus
#include "webrtc/system_wrappers/include/compile_assert_c.h"
#endif
#include "webrtc/system_wrappers/include/cpu_features_wrapper.h"

#include "tsk_debug.h"

namespace youme {
  namespace webrtc_new {
// Only bit |kBandFirst| through bit |kBandLast| are processed and
// |kBandFirst| - |kBandLast| must be < 32.
enum { kBandFirst = 12 };
enum { kBandLast = 43 };

static __inline uint32_t SetBit(uint32_t in, int pos) {
  uint32_t mask = (1 << pos);
  uint32_t out = (in | mask);

  return out;
}

// Calculates the mean recursively. Same version as WebRtc_MeanEstimatorFix(),
// but for float.
//
// Inputs:
//    - new_value             : New additional value.
//    - scale                 : Scale for smoothing (should be less than 1.0).
//
// Input/Output:
//    - mean_value            : Pointer to the mean value for updating.
//
static void MeanEstimatorFloat(float new_value,
                               float scale,
                               float* mean_value) {
  assert(scale < 1.0f);
  *mean_value += (new_value - *mean_value) * scale;
}

// Computes the binary spectrum by comparing the input |spectrum| with a
// |threshold_spectrum|. Float and fixed point versions.
//
// Inputs:
//      - spectrum            : Spectrum of which the binary spectrum should be
//                              calculated.
//      - threshold_spectrum  : Threshold spectrum with which the input
//                              spectrum is compared.
// Return:
//      - out                 : Binary spectrum.
//
static uint32_t BinarySpectrumFix(const uint16_t* spectrum,
                                  SpectrumType* threshold_spectrum,
                                  int q_domain,
                                  int* threshold_initialized) {
  int i = kBandFirst;
  uint32_t out = 0;

  assert(q_domain < 16);

  if (!(*threshold_initialized)) {
    // Set the |threshold_spectrum| to half the input |spectrum| as starting
    // value. This speeds up the convergence.
    for (i = kBandFirst; i <= kBandLast; i++) {
      if (spectrum[i] > 0) {
        // Convert input spectrum from Q(|q_domain|) to Q15.
        int32_t spectrum_q15 = ((int32_t) spectrum[i]) << (15 - q_domain);
        threshold_spectrum[i].int32_ = (spectrum_q15 >> 1);
        *threshold_initialized = 1;
      }
    }
  }
  for (i = kBandFirst; i <= kBandLast; i++) {
    // Convert input spectrum from Q(|q_domain|) to Q15.
    int32_t spectrum_q15 = ((int32_t) spectrum[i]) << (15 - q_domain);
    // Update the |threshold_spectrum|.
    WebRtc_MeanEstimatorFix(spectrum_q15, 6, &(threshold_spectrum[i].int32_));
    // Convert |spectrum| at current frequency bin to a binary value.
    if (spectrum_q15 > threshold_spectrum[i].int32_) {
      out = SetBit(out, i - kBandFirst);
    }
  }

  return out;
}

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
static uint32_t BinarySpectrumFloat(const float* spectrum,
                                    SpectrumType* threshold_spectrum,
                                    int* threshold_initialized, int mult, int16_t *iLog_Energy) {
  int iBandFirst, iBandLast;
  int i = kBandFirst;
  uint32_t out = 0;
  const float kScale = 1 / 64.0;
  double spectrum_avg = 0.0;

  if (1 == mult)
  {
      iBandFirst = kBandFirst;
      iBandLast = kBandLast;
  }
  else
  {
      iBandFirst = 4;
      iBandLast = 16 + iBandFirst;
  }

  if (!(*threshold_initialized)) {
    // Set the |threshold_spectrum| to half the input |spectrum| as starting
    // value. This speeds up the convergence.
    for (i = iBandFirst; i <= iBandLast; i++) {
      if (spectrum[i] > 0.0f) {
        threshold_spectrum[i].float_ = (spectrum[i] / 2);
        *threshold_initialized = 1;
      }
    }
  }
      
  for (i = iBandFirst; i <= iBandLast; i++) {
    spectrum_avg += spectrum[i];
    // Update the |threshold_spectrum|.
    MeanEstimatorFloat(spectrum[i], kScale, &(threshold_spectrum[i].float_));
    // Convert |spectrum| at current frequency bin to a binary value.
    if (spectrum[i] > threshold_spectrum[i].float_) {
      out = SetBit(out, i - iBandFirst);
    }
  }

  spectrum_avg = spectrum_avg/(iBandLast - iBandFirst + 1);
  *iLog_Energy = 0;
  if (spectrum_avg > 0)
  {
	*iLog_Energy = log10(spectrum_avg) / log10(2.0f);
  }

  return out;
}
#else
static uint32_t BinarySpectrumFloat(const float* spectrum,
                                    SpectrumType* threshold_spectrum,
                                    int* threshold_initialized) {
  int i = kBandFirst;
  uint32_t out = 0;
  const float kScale = 1 / 64.0;

  if (!(*threshold_initialized)) {
    // Set the |threshold_spectrum| to half the input |spectrum| as starting
    // value. This speeds up the convergence.
    for (i = kBandFirst; i <= kBandLast; i++) {
      if (spectrum[i] > 0.0f) {
        threshold_spectrum[i].float_ = (spectrum[i] / 2);
        *threshold_initialized = 1;
      }
    }
  }

  for (i = kBandFirst; i <= kBandLast; i++) {
    // Update the |threshold_spectrum|.
    MeanEstimatorFloat(spectrum[i], kScale, &(threshold_spectrum[i].float_));
    // Convert |spectrum| at current frequency bin to a binary value.
    if (spectrum[i] > threshold_spectrum[i].float_) {
      out = SetBit(out, i - kBandFirst);
    }
  }

  return out;
}
#endif

void WebRtc_FreeDelayEstimatorFarend(void* handle) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

  if (handle == NULL) {
    return;
  }

  free(self->mean_far_spectrum);
  self->mean_far_spectrum = NULL;

  WebRtc_FreeBinaryDelayEstimatorFarend(self->binary_farend);
  self->binary_farend = NULL;

  free(self);
}

void* WebRtc_CreateDelayEstimatorFarend(int spectrum_size, int history_size) {
  DelayEstimatorFarend* self = NULL;

  // Check if the sub band used in the delay estimation is small enough to fit
  // the binary spectra in a uint32_t.
    
#ifdef __cplusplus
    static_assert(kBandLast - kBandFirst < 32, "kBandLast - kBandFirst >= 32!!!");
#else
    COMPILE_ASSERT(PART_LEN % 16 == 0);
#endif
    
  if (spectrum_size >= kBandLast) {
    self = (DelayEstimatorFarend*)malloc(sizeof(DelayEstimatorFarend));
  }

  if (self != NULL) {
    int memory_fail = 0;

    // Allocate memory for the binary far-end spectrum handling.
    self->binary_farend = WebRtc_CreateBinaryDelayEstimatorFarend(history_size);
    memory_fail |= (self->binary_farend == NULL);

    // Allocate memory for spectrum buffers.
    self->mean_far_spectrum = (SpectrumType *)malloc(spectrum_size * sizeof(SpectrumType));
    memory_fail |= (self->mean_far_spectrum == NULL);

    self->spectrum_size = spectrum_size;

    if (memory_fail) {
      WebRtc_FreeDelayEstimatorFarend(self);
      self = NULL;
    }
  }

  return self;
}

int WebRtc_InitDelayEstimatorFarend(void* handle) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;

  if (self == NULL) {
    return -1;
  }

  // Initialize far-end part of binary delay estimator.
  WebRtc_InitBinaryDelayEstimatorFarend(self->binary_farend);

  // Set averaged far and near end spectra to zero.
  memset(self->mean_far_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  // Reset initialization indicators.
  self->far_spectrum_initialized = 0;

  return 0;
}

void WebRtc_SoftResetDelayEstimatorFarend(void* handle, int delay_shift) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  assert(self != NULL);
  WebRtc_SoftResetBinaryDelayEstimatorFarend(self->binary_farend, delay_shift);
}

int WebRtc_AddFarSpectrumFix(void* handle,
                             const uint16_t* far_spectrum,
                             int spectrum_size,
                             int far_q) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    // Empty far end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }
  if (far_q > 15) {
    // If |far_q| is larger than 15 we cannot guarantee no wrap around.
    return -1;
  }

  // Get binary spectrum.
  binary_spectrum = BinarySpectrumFix(far_spectrum, self->mean_far_spectrum,
                                      far_q, &(self->far_spectrum_initialized));
  WebRtc_AddBinaryFarSpectrum(self->binary_farend, binary_spectrum);

  return 0;
}

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
int WebRtc_AddFarSpectrumFloat(void* handle,
                               const float* far_spectrum,
                               int spectrum_size, int mult) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  uint32_t binary_spectrum = 0;
  int16_t iLog_Energy = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    // Empty far end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }

  // Get binary spectrum.
  binary_spectrum = BinarySpectrumFloat(far_spectrum, self->mean_far_spectrum,
                                        &(self->far_spectrum_initialized), mult, &iLog_Energy);
  WebRtc_AddBinaryFarSpectrum_OP(self->binary_farend, binary_spectrum, iLog_Energy);

  return 0;
}
#else
int WebRtc_AddFarSpectrumFloat(void* handle,
                               const float* far_spectrum,
                               int spectrum_size) {
  DelayEstimatorFarend* self = (DelayEstimatorFarend*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (far_spectrum == NULL) {
    // Empty far end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }

  // Get binary spectrum.
  binary_spectrum = BinarySpectrumFloat(far_spectrum, self->mean_far_spectrum,
                                        &(self->far_spectrum_initialized));
  WebRtc_AddBinaryFarSpectrum(self->binary_farend, binary_spectrum);

  return 0;
}
#endif
void WebRtc_FreeDelayEstimator(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (handle == NULL) {
    return;
  }

  free(self->mean_near_spectrum);
  self->mean_near_spectrum = NULL;

  WebRtc_FreeBinaryDelayEstimator(self->binary_handle);
  self->binary_handle = NULL;

  free(self);
}

void* WebRtc_CreateDelayEstimator(void* farend_handle, 
                                          void* media_handle, int max_lookahead) {
  DelayEstimator* self = NULL;
  DelayEstimatorFarend* farend = (DelayEstimatorFarend*) farend_handle;
  DelayEstimatorFarend* media_end = (DelayEstimatorFarend*) media_handle;

  if (farend_handle != NULL) {
    self = (DelayEstimator*)malloc(sizeof(DelayEstimator));
  }

  if (self != NULL) {
    int memory_fail = 0;

    // Allocate memory for the farend spectrum handling.
    self->binary_handle =
        WebRtc_CreateBinaryDelayEstimator(farend->binary_farend, 
                                          media_end->binary_farend, max_lookahead);
    memory_fail |= (self->binary_handle == NULL);

    // Allocate memory for spectrum buffers.
    self->mean_near_spectrum = (SpectrumType *)malloc(farend->spectrum_size *
                                      sizeof(SpectrumType));
    memory_fail |= (self->mean_near_spectrum == NULL);

    self->spectrum_size = farend->spectrum_size;
      self->_playoutDelay = 20;
      self->playoutDelayMeasurementCounter = 0;
    if (memory_fail) {
      WebRtc_FreeDelayEstimator(self);
      self = NULL;
    }
  }

  return self;
}

int WebRtc_InitDelayEstimator(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }

  // Initialize binary delay estimator.
  WebRtc_InitBinaryDelayEstimator(self->binary_handle);

  // Set averaged far and near end spectra to zero.
  memset(self->mean_near_spectrum, 0,
         sizeof(SpectrumType) * self->spectrum_size);
  // Reset initialization indicators.
  self->near_spectrum_initialized = 0;

  return 0;
}

int WebRtc_SoftResetDelayEstimator(void* handle, int delay_shift) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  return WebRtc_SoftResetBinaryDelayEstimator(self->binary_handle, delay_shift);
}

int WebRtc_set_history_size(void* handle, int history_size) {
  DelayEstimator* self = (DelayEstimator*)handle;

  if ((self == NULL) || (history_size <= 1)) {
    return -1;
  }
  return WebRtc_AllocateHistoryBufferMemory(self->binary_handle, history_size);
}

int WebRtc_history_size(const void* handle) {
  const DelayEstimator* self = (DelayEstimator*)handle;

  if (self == NULL) {
    return -1;
  }
  if (self->binary_handle->farend->history_size !=
      self->binary_handle->history_size) {
    // Non matching history sizes.
    return -1;
  }
  return self->binary_handle->history_size;
}

int WebRtc_set_lookahead(void* handle, int lookahead) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  assert(self->binary_handle != NULL);
  if ((lookahead > self->binary_handle->near_history_size - 1) ||
      (lookahead < 0)) {
    return -1;
  }
  self->binary_handle->lookahead = lookahead;
  return self->binary_handle->lookahead;
}

int WebRtc_lookahead(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  assert(self->binary_handle != NULL);
  return self->binary_handle->lookahead;
}

int WebRtc_set_allowed_offset(void* handle, int allowed_offset) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if ((self == NULL) || (allowed_offset < 0)) {
    return -1;
  }
  self->binary_handle->allowed_offset = allowed_offset;
  return 0;
}

int WebRtc_get_allowed_offset(const void* handle) {
  const DelayEstimator* self = (const DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  return self->binary_handle->allowed_offset;
}

int WebRtc_enable_robust_validation(void* handle, int enable) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  if ((enable < 0) || (enable > 1)) {
    return -1;
  }
  assert(self->binary_handle != NULL);
  self->binary_handle->robust_validation_enabled = enable;
  return 0;
}

int WebRtc_is_robust_validation_enabled(const void* handle) {
  const DelayEstimator* self = (const DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }
  return self->binary_handle->robust_validation_enabled;
}

int WebRtc_DelayEstimatorProcessFix(void* handle,
                                    const uint16_t* near_spectrum,
                                    int spectrum_size,
                                    int near_q) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (near_spectrum == NULL) {
    // Empty near end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }
  if (near_q > 15) {
    // If |near_q| is larger than 15 we cannot guarantee no wrap around.
    return -1;
  }

  // Get binary spectra.
  binary_spectrum = BinarySpectrumFix(near_spectrum,
                                      self->mean_near_spectrum,
                                      near_q,
                                      &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle, binary_spectrum);
}
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
int WebRtc_DelayEstimatorProcessFloat(void* handle,
                                      const float* near_spectrum,
                                      int spectrum_size, int mult, int16_t *vadR) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_spectrum = 0;
  int i = 0;
  int16_t iLog_Energy = 0;

  if (self == NULL) {
    return -1;
  }
  if (near_spectrum == NULL) {
    // Empty near end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }

  for (i = 0; i < self->binary_handle->history_size; i ++)
  {
      self->binary_handle->sDelay_VadR[i] = vadR[i];
  }

  // Get binary spectrum.
  binary_spectrum = BinarySpectrumFloat(near_spectrum, self->mean_near_spectrum,
                                        &(self->near_spectrum_initialized), mult, &iLog_Energy);
#if defined(WEBRTC_HAS_NEON)
  //return WebRtc_ProcessBinarySpectrum_OP_neon(self->binary_handle, binary_spectrum); neon 函数收敛存在问题
  return WebRtc_ProcessBinarySpectrum_OP(self->binary_handle, binary_spectrum);
#elif defined(WEBRTC_DETECT_NEON)
  if(g_webrtc_neon_support_flag != 0){
    return WebRtc_ProcessBinarySpectrum_OP(self->binary_handle, binary_spectrum);
  }
  else{
    return WebRtc_ProcessBinarySpectrum_OP(self->binary_handle, binary_spectrum);
  }
#else
  return WebRtc_ProcessBinarySpectrum_OP(self->binary_handle, binary_spectrum);
#endif
}

int WebRtc_DelayEstimatorProcessFloat_media(void* handle,
                                      int mult, int16_t *VadMedia) {
  DelayEstimator* self = (DelayEstimator*) handle;
  int i = 0;

  if (self == NULL) {
    return -1;
  }

  for (i = 0; i < self->binary_handle->history_size; i ++)
  {
      self->binary_handle->sDelay_VadMedia[i] = VadMedia[i];
  }

// #if defined(WEBRTC_HAS_NEON)
//   return WebRtc_ProcessBinarySpectrum_OP_neon(self->binary_handle, binary_spectrum);
// #elif defined(WEBRTC_DETECT_NEON)
//   if(g_webrtc_neon_support_flag != 0){
//     return WebRtc_ProcessBinarySpectrum_OP_neon(self->binary_handle, binary_spectrum);
//   }
//   else{
//     return WebRtc_ProcessBinarySpectrum_OP(self->binary_handle, binary_spectrum);
//   }
// #else
//   return WebRtc_ProcessBinarySpectrum_OP_media(self->binary_handle);
// #endif
  return WebRtc_ProcessBinarySpectrum_OP_media(self->binary_handle);
}


#else
int WebRtc_DelayEstimatorProcessFloat(void* handle,
                                      const float* near_spectrum,
                                      int spectrum_size) {
  DelayEstimator* self = (DelayEstimator*) handle;
  uint32_t binary_spectrum = 0;

  if (self == NULL) {
    return -1;
  }
  if (near_spectrum == NULL) {
    // Empty near end spectrum.
    return -1;
  }
  if (spectrum_size != self->spectrum_size) {
    // Data sizes don't match.
    return -1;
  }

  // Get binary spectrum.
  binary_spectrum = BinarySpectrumFloat(near_spectrum, self->mean_near_spectrum,
                                        &(self->near_spectrum_initialized));

  return WebRtc_ProcessBinarySpectrum(self->binary_handle, binary_spectrum);
}
#endif

int WebRtc_last_delay(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;

  if (self == NULL) {
    return -1;
  }

  return WebRtc_binary_last_delay(self->binary_handle);
}

float WebRtc_last_delay_quality(void* handle) {
  DelayEstimator* self = (DelayEstimator*) handle;
  assert(self != NULL);
  return WebRtc_binary_last_delay_quality(self->binary_handle);
}
      
#if defined(__APPLE__)
int WebRtc_DelayEstimatorProcessFloat_Apple(void* handle) {
    DelayEstimator* self = static_cast<DelayEstimator*>(handle);
  
  if (self->playoutDelayMeasurementCounter >= 100) {
      self->playoutDelayMeasurementCounter++;
      self->_playoutDelay = 0;
      // Update HW and OS delay every second, unlikely to change

#ifdef UPDATE_AUDIO_DEVICE_DELAY_ENABLED//add by Rookie to resolve incoming call terminate, restart audio device failed
      // HW output latency
      Float32 f32(0);
      UInt32 size = sizeof(f32);
      OSStatus result = AudioSessionGetProperty(
                                                kAudioSessionProperty_CurrentHardwareOutputLatency, &size, &f32);
      if (0 != result) {
          TSK_DEBUG_INFO("error HW latency (result=%d)", result);
//          WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
//                       "error HW latency (result=%d)", result);
      }
      self->_playoutDelay += static_cast<int>(f32 * 1000000);
      
      // HW buffer duration
      f32 = 0;
      result = AudioSessionGetProperty(
                                       kAudioSessionProperty_CurrentHardwareIOBufferDuration, &size, &f32);
      if (0 != result) {
          TSK_DEBUG_INFO("error HW buffer duration (result=%d)", result);
//          WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
//                       "error HW buffer duration (result=%d)", result);
      }
      self->_playoutDelay += static_cast<int>(f32 * 1000000);
      
      // AU latency
      //        Float64 f64(0);
      //        size = sizeof(f64);
      //        result = AudioUnitGetProperty(_auRemoteIO,
      //            kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0, &f64, &size);
      //        if (0 != result) {
      //            WEBRTC_TRACE(kTraceError, kTraceAudioDevice, _id,
      //                         "error AU latency (result=%d)", result);
      //        }
      //        _playoutDelay += static_cast<int>(f64 * 1000000);
      
      //if(_playoutDelay < 500) //jkh
      //{
      //    _playoutDelay = 500;
      //}
      // To ms
      //_playoutDelay = (_playoutDelay - 500) / 1000;
      self->_playoutDelay = (self->_playoutDelay + 500) / 1000;
      
#else
      //Default 20ms--refer to iphone4s, ipod delay 21ms
      self->_playoutDelay = 20;
      
#endif
      TSK_DEBUG_INFO("AEC delay estimate value:%d", self->_playoutDelay);
  }
    self->playoutDelayMeasurementCounter++;
    return self->_playoutDelay;
}
#endif
      
    }//namespace webrtc_new
}//namespace youme

﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_RESAMPLER_NEW_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_RESAMPLER_NEW_H_

#include "aec_core_new.h"

namespace youme{
    
namespace webrtc_new {
        
enum {
  kResamplingDelay = 1
};
enum {
  kResamplerBufferSize = FRAME_LEN * 4
};

// Unless otherwise specified, functions return 0 on success and -1 on error.
void* WebRtcAec_CreateResampler();  // Returns NULL on error.
int WebRtcAec_InitResampler(void* resampInst, int deviceSampleRateHz);
void WebRtcAec_FreeResampler(void* resampInst);

// Estimates skew from raw measurement.
int WebRtcAec_GetSkew(void* resampInst, int rawSkew, float* skewEst);

// Resamples input using linear interpolation.
void WebRtcAec_ResampleLinear(void* resampInst,
                              const float* inspeech,
                              size_t size,
                              float skew,
                              float* outspeech,
                              size_t* size_out);

    }  // namespace webrtc_new
}  // namespace youme

#endif  // WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_RESAMPLER_NEW_H_

﻿/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_AUDIO_LAPPED_TRANSFORM_H_
#define WEBRTC_COMMON_AUDIO_LAPPED_TRANSFORM_H_

#include <complex>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/common_audio/blocker.h"
#include "webrtc/common_audio/real_fourier.h"
#include "webrtc/system_wrappers/include/aligned_array.h"

namespace webrtc {

// Helper class for audio processing modules which operate on frequency domain
// input derived from the windowed time domain audio stream.
//
// The input audio chunk is sliced into possibly overlapping blocks, multiplied
// by a window and transformed with an FFT implementation. The transformed data
// is supplied to the given callback for processing. The processed output is
// then inverse transformed into the time domain and spliced back into a chunk
// which constitutes the final output of this processing module.
class LappedTransform {
 public:
  class Callback {
   public:
    virtual ~Callback() {}

    virtual void ProcessAudioBlock(const std::complex<float>* const* in_block,
                                   int num_in_channels, size_t frames,
                                   int num_out_channels,
                                   std::complex<float>* const* out_block) = 0;
  };

  // Construct a transform instance. |chunk_length| is the number of samples in
  // each channel. |window| defines the window, owned by the caller (a copy is
  // made internally); |window| should have length equal to |block_length|.
  // |block_length| defines the length of a block, in samples.
  // |shift_amount| is in samples. |callback| is the caller-owned audio
  // processing function called for each block of the input chunk.
  LappedTransform(int num_in_channels,
                  int num_out_channels,
                  size_t chunk_length,
                  const float* window,
                  size_t block_length,
                  size_t shift_amount,
                  Callback* callback);
  ~LappedTransform() {}

  // Main audio processing helper method. Internally slices |in_chunk| into
  // blocks, transforms them to frequency domain, calls the callback for each
  // block and returns a de-blocked time domain chunk of audio through
  // |out_chunk|. Both buffers are caller-owned.
  void ProcessChunk(const float* const* in_chunk, float* const* out_chunk);

  // Get the chunk length.
  //
  // The chunk length is the number of samples per channel that must be passed
  // to ProcessChunk via the parameter in_chunk.
  //
  // Returns the same chunk_length passed to the LappedTransform constructor.
  size_t chunk_length() const { return chunk_length_; }

  // Get the number of input channels.
  //
  // This is the number of arrays that must be passed to ProcessChunk via
  // in_chunk.
  //
  // Returns the same num_in_channels passed to the LappedTransform constructor.
  int num_in_channels() const { return num_in_channels_; }

  // Get the number of output channels.
  //
  // This is the number of arrays that must be passed to ProcessChunk via
  // out_chunk.
  //
  // Returns the same num_out_channels passed to the LappedTransform
  // constructor.
  int num_out_channels() const { return num_out_channels_; }

 private:
  // Internal middleware callback, given to the blocker. Transforms each block
  // and hands it over to the processing method given at construction time.
  class BlockThunk : public BlockerCallback {
   public:
    explicit BlockThunk(LappedTransform* parent) : parent_(parent) {}

    virtual void ProcessBlock(const float* const* input, size_t num_frames,
                              int num_input_channels, int num_output_channels,
                              float* const* output);

   private:
    LappedTransform* const parent_;
  } blocker_callback_;

  const int num_in_channels_;
  const int num_out_channels_;

  const size_t block_length_;
  const size_t chunk_length_;

  Callback* const block_processor_;
  Blocker blocker_;

  rtc::scoped_ptr<RealFourier> fft_;
  const size_t cplx_length_;
  AlignedArray<float> real_buf_;
  AlignedArray<std::complex<float> > cplx_pre_;
  AlignedArray<std::complex<float> > cplx_post_;
};

}  // namespace webrtc

#endif  // WEBRTC_COMMON_AUDIO_LAPPED_TRANSFORM_H_


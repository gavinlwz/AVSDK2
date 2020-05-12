/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * The core AEC algorithm, neon version of speed-critical functions.
 *
 * Based on aec_core_sse2.c.
 */

#include "webrtc/typedefs.h"

#if defined(WEBRTC_HAS_NEON)

#include <arm_neon.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>  // memset


#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/aec_new/aec_common_new.h"
#include "webrtc/modules/audio_processing/aec_new/aec_core_internal_new.h"
#include "webrtc/modules/audio_processing/aec_new/aec_rdft_new.h"

namespace youme {
namespace webrtc_new {

enum { kShiftExponentIntoTopMantissa = 8 };
enum { kFloatExponentShift = 23 };

__inline static float MulRe(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bRe - aIm * bIm;
}

__inline static float MulIm(float aRe, float aIm, float bRe, float bIm) {
  return aRe * bIm + aIm * bRe;
}

static int CmpFloat(const void* a, const void* b) {
    const float* da = (const float*)a;
    const float* db = (const float*)b;

    return (*da > *db) - (*da < *db);
}

static void FilterFarNEON(AecCore* aec, float yf[2][PART_LEN1]) {
  int i;
  const int num_partitions = aec->num_partitions;
  for (i = 0; i < num_partitions; i++) {
    int j;
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    // Check for wrap
    if (i + aec->xfBufBlockPos >= num_partitions) {
      xPos -= num_partitions * PART_LEN1;
    }

    // vectorized code (four at once)
    for (j = 0; j + 3 < PART_LEN1; j += 4) {
      const float32x4_t xfBuf_re = vld1q_f32(&aec->xfBuf[0][xPos + j]);
      const float32x4_t xfBuf_im = vld1q_f32(&aec->xfBuf[1][xPos + j]);
      const float32x4_t wfBuf_re = vld1q_f32(&aec->wfBuf[0][pos + j]);
      const float32x4_t wfBuf_im = vld1q_f32(&aec->wfBuf[1][pos + j]);
      const float32x4_t yf_re = vld1q_f32(&yf[0][j]);
      const float32x4_t yf_im = vld1q_f32(&yf[1][j]);
      const float32x4_t a = vmulq_f32(xfBuf_re, wfBuf_re);
      const float32x4_t e = vmlsq_f32(a, xfBuf_im, wfBuf_im);
      const float32x4_t c = vmulq_f32(xfBuf_re, wfBuf_im);
      const float32x4_t f = vmlaq_f32(c, xfBuf_im, wfBuf_re);
      const float32x4_t g = vaddq_f32(yf_re, e);
      const float32x4_t h = vaddq_f32(yf_im, f);
      vst1q_f32(&yf[0][j], g);
      vst1q_f32(&yf[1][j], h);
    }
    // scalar code for the remaining items.
    for (; j < PART_LEN1; j++) {
      yf[0][j] += MulRe(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
      yf[1][j] += MulIm(aec->xfBuf[0][xPos + j],
                        aec->xfBuf[1][xPos + j],
                        aec->wfBuf[0][pos + j],
                        aec->wfBuf[1][pos + j]);
    }
  }
}

static float32x4_t vcosq_f32(float32x4_t w){
    float32x4_t w1, cos_w;
    
    const float32x4_t C0 = vdupq_n_f32(1.00016073f);
    const float32x4_t C1 = vdupq_n_f32(-0.00284838f);
    const float32x4_t C2 = vdupq_n_f32(-0.48788043f);
    const float32x4_t C3 = vdupq_n_f32(-0.02105969f);
    const float32x4_t C4 = vdupq_n_f32(0.05948788f);
    const float32x4_t C5 = vdupq_n_f32(-0.00757423f);
    const float32x4_t k2pi = vdupq_n_f32(6.28318530717959f);

    w1 = vsubq_f32(k2pi, w);
    w = vminq_f32(w, w1);

    cos_w = C5;
    cos_w = vmlaq_f32(C4, w, cos_w);
    cos_w = vmlaq_f32(C3, w, cos_w);
    cos_w = vmlaq_f32(C2, w, cos_w);
    cos_w = vmlaq_f32(C1, w, cos_w);
    cos_w = vmlaq_f32(C0, w, cos_w);

    return cos_w;
}

static float32x4_t vsinq_f32(float32x4_t w){
    const float pi = 3.14159265358979f;
    float vcoff_w[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float32x4_t w1, sin_w;
    
    const float32x4_t C0 = vdupq_n_f32(0.00131326f);
    const float32x4_t C1 = vdupq_n_f32(0.98260203f);
    const float32x4_t C2 = vdupq_n_f32(0.05446856f);
    const float32x4_t C3 = vdupq_n_f32(-0.23379257f);
    const float32x4_t C4 = vdupq_n_f32(0.03720924f);
    const float32x4_t k2pi = vdupq_n_f32(6.28318530717959f);

    w1 = vsubq_f32(k2pi, w);
    w1 = vminq_f32(w, w1);

    sin_w = C4;
    sin_w = vmlaq_f32(C3, w1, sin_w);
    sin_w = vmlaq_f32(C2, w1, sin_w);
    sin_w = vmlaq_f32(C1, w1, sin_w);
    sin_w = vmlaq_f32(C0, w1, sin_w);

    if(vgetq_lane_f32(w, 0) > pi) vcoff_w[0] = -1.0f;
    if(vgetq_lane_f32(w, 1) > pi) vcoff_w[1] = -1.0f;
    if(vgetq_lane_f32(w, 2) > pi) vcoff_w[2] = -1.0f;
    if(vgetq_lane_f32(w, 3) > pi) vcoff_w[3] = -1.0f;
    sin_w = vmulq_f32(sin_w, vld1q_f32(&vcoff_w[0]));

    return sin_w;
}

// ARM64's arm_neon.h has already defined vdivq_f32 vsqrtq_f32.
#if !defined (__aarch64__)//(__ARM64_ARCH_8__)
static float32x4_t vdivq_f32(float32x4_t a, float32x4_t b) {
  int i;
  float32x4_t x = vrecpeq_f32(b);
  // from arm documentation
  // The Newton-Raphson iteration:
  //     x[n+1] = x[n] * (2 - d * x[n])
  // converges to (1/d) if x0 is the result of VRECPE applied to d.
  //
  // Note: The precision did not improve after 2 iterations.
  for (i = 0; i < 2; i++) {
    x = vmulq_f32(vrecpsq_f32(b, x), x);
  }
  // a/b = a*(1/b)
  return vmulq_f32(a, x);
}

static float32x4_t vsqrtq_f32(float32x4_t s) {
  int i;
  float32x4_t x = vrsqrteq_f32(s);

  // Code to handle sqrt(0).
  // If the input to sqrtf() is zero, a zero will be returned.
  // If the input to vrsqrteq_f32() is zero, positive infinity is returned.
  const uint32x4_t vec_p_inf = vdupq_n_u32(0x7F800000);
  // check for divide by zero
  const uint32x4_t div_by_zero = vceqq_u32(vec_p_inf, vreinterpretq_u32_f32(x));
  // zero out the positive infinity results
  x = vreinterpretq_f32_u32(vandq_u32(vmvnq_u32(div_by_zero),
                                      vreinterpretq_u32_f32(x)));
  // from arm documentation
  // The Newton-Raphson iteration:
  //     x[n+1] = x[n] * (3 - d * (x[n] * x[n])) / 2)
  // converges to (1/√d) if x0 is the result of VRSQRTE applied to d.
  //
  // Note: The precision did not improve after 2 iterations.
  for (i = 0; i < 2; i++) {
    x = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, x), s), x);
  }
  // sqrt(s) = s * 1/sqrt(s)
  return vmulq_f32(s, x);;
}
#endif  // WEBRTC_ARCH_ARM64

static void ScaleErrorSignalNEON(AecCore* aec, float ef[2][PART_LEN1]) {
  const float mu = aec->extended_filter_enabled ? kExtendedMu : aec->normal_mu;
  const float error_threshold = aec->extended_filter_enabled ?
      kExtendedErrorThreshold : aec->normal_error_threshold;
  const float32x4_t k1e_10f = vdupq_n_f32(1e-10f);
  const float32x4_t kMu = vmovq_n_f32(mu);
  const float32x4_t kThresh = vmovq_n_f32(error_threshold);
  int i;
  // vectorized code (four at once)
  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    const float32x4_t xPow = vld1q_f32(&aec->xPow[i]);
    const float32x4_t ef_re_base = vld1q_f32(&ef[0][i]);
    const float32x4_t ef_im_base = vld1q_f32(&ef[1][i]);
    const float32x4_t xPowPlus = vaddq_f32(xPow, k1e_10f);
    float32x4_t ef_re = vdivq_f32(ef_re_base, xPowPlus);
    float32x4_t ef_im = vdivq_f32(ef_im_base, xPowPlus);
    const float32x4_t ef_re2 = vmulq_f32(ef_re, ef_re);
    const float32x4_t ef_sum2 = vmlaq_f32(ef_re2, ef_im, ef_im);
    const float32x4_t absEf = vsqrtq_f32(ef_sum2);
    const uint32x4_t bigger = vcgtq_f32(absEf, kThresh);
    const float32x4_t absEfPlus = vaddq_f32(absEf, k1e_10f);
    const float32x4_t absEfInv = vdivq_f32(kThresh, absEfPlus);
    uint32x4_t ef_re_if = vreinterpretq_u32_f32(vmulq_f32(ef_re, absEfInv));
    uint32x4_t ef_im_if = vreinterpretq_u32_f32(vmulq_f32(ef_im, absEfInv));
    uint32x4_t ef_re_u32 = vandq_u32(vmvnq_u32(bigger),
                                     vreinterpretq_u32_f32(ef_re));
    uint32x4_t ef_im_u32 = vandq_u32(vmvnq_u32(bigger),
                                     vreinterpretq_u32_f32(ef_im));
    ef_re_if = vandq_u32(bigger, ef_re_if);
    ef_im_if = vandq_u32(bigger, ef_im_if);
    ef_re_u32 = vorrq_u32(ef_re_u32, ef_re_if);
    ef_im_u32 = vorrq_u32(ef_im_u32, ef_im_if);
    ef_re = vmulq_f32(vreinterpretq_f32_u32(ef_re_u32), kMu);
    ef_im = vmulq_f32(vreinterpretq_f32_u32(ef_im_u32), kMu);
    vst1q_f32(&ef[0][i], ef_re);
    vst1q_f32(&ef[1][i], ef_im);
  }
  // scalar code for the remaining items.
  for (; i < PART_LEN1; i++) {
    float abs_ef;
    ef[0][i] /= (aec->xPow[i] + 1e-10f);
    ef[1][i] /= (aec->xPow[i] + 1e-10f);
    abs_ef = sqrtf(ef[0][i] * ef[0][i] + ef[1][i] * ef[1][i]);

    if (abs_ef > error_threshold) {
      abs_ef = error_threshold / (abs_ef + 1e-10f);
      ef[0][i] *= abs_ef;
      ef[1][i] *= abs_ef;
    }

    // Stepsize factor
    ef[0][i] *= mu;
    ef[1][i] *= mu;
  }
}

static void FilterAdaptationNEON(AecCore* aec,
                                 float* fft,
                                 float ef[2][PART_LEN1]) {
  int i;
  const int num_partitions = aec->num_partitions;
  for (i = 0; i < num_partitions; i++) {
    int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
    int pos = i * PART_LEN1;
    int j;
    // Check for wrap
    if (i + aec->xfBufBlockPos >= num_partitions) {
      xPos -= num_partitions * PART_LEN1;
    }

    // Process the whole array...
    for (j = 0; j < PART_LEN; j += 4) {
      // Load xfBuf and ef.
      const float32x4_t xfBuf_re = vld1q_f32(&aec->xfBuf[0][xPos + j]);
      const float32x4_t xfBuf_im = vld1q_f32(&aec->xfBuf[1][xPos + j]);
      const float32x4_t ef_re = vld1q_f32(&ef[0][j]);
      const float32x4_t ef_im = vld1q_f32(&ef[1][j]);
      // Calculate the product of conjugate(xfBuf) by ef.
      //   re(conjugate(a) * b) = aRe * bRe + aIm * bIm
      //   im(conjugate(a) * b)=  aRe * bIm - aIm * bRe
      const float32x4_t a = vmulq_f32(xfBuf_re, ef_re);
      const float32x4_t e = vmlaq_f32(a, xfBuf_im, ef_im);
      const float32x4_t c = vmulq_f32(xfBuf_re, ef_im);
      const float32x4_t f = vmlsq_f32(c, xfBuf_im, ef_re);
      // Interleave real and imaginary parts.
      const float32x4x2_t g_n_h = vzipq_f32(e, f);
      // Store
      vst1q_f32(&fft[2 * j + 0], g_n_h.val[0]);
      vst1q_f32(&fft[2 * j + 4], g_n_h.val[1]);
    }
    // ... and fixup the first imaginary entry.
    fft[1] = MulRe(aec->xfBuf[0][xPos + PART_LEN],
                   -aec->xfBuf[1][xPos + PART_LEN],
                   ef[0][PART_LEN],
                   ef[1][PART_LEN]);

    aec_rdft_inverse_128(fft);
    memset(fft + PART_LEN, 0, sizeof(float) * PART_LEN);

    // fft scaling
    {
      const float scale = 2.0f / PART_LEN2;
      const float32x4_t scale_ps = vmovq_n_f32(scale);
      for (j = 0; j < PART_LEN; j += 4) {
        const float32x4_t fft_ps = vld1q_f32(&fft[j]);
        const float32x4_t fft_scale = vmulq_f32(fft_ps, scale_ps);
        vst1q_f32(&fft[j], fft_scale);
      }
    }
    aec_rdft_forward_128(fft);

    {
      const float wt1 = aec->wfBuf[1][pos];
      aec->wfBuf[0][pos + PART_LEN] += fft[1];
      for (j = 0; j < PART_LEN; j += 4) {
        float32x4_t wtBuf_re = vld1q_f32(&aec->wfBuf[0][pos + j]);
        float32x4_t wtBuf_im = vld1q_f32(&aec->wfBuf[1][pos + j]);
        const float32x4_t fft0 = vld1q_f32(&fft[2 * j + 0]);
        const float32x4_t fft4 = vld1q_f32(&fft[2 * j + 4]);
        const float32x4x2_t fft_re_im = vuzpq_f32(fft0, fft4);
        wtBuf_re = vaddq_f32(wtBuf_re, fft_re_im.val[0]);
        wtBuf_im = vaddq_f32(wtBuf_im, fft_re_im.val[1]);

        vst1q_f32(&aec->wfBuf[0][pos + j], wtBuf_re);
        vst1q_f32(&aec->wfBuf[1][pos + j], wtBuf_im);
      }
      aec->wfBuf[1][pos] = wt1;
    }
  }
}

static float32x4_t vpowq_f32(float32x4_t a, float32x4_t b) {
  // a^b = exp2(b * log2(a))
  //   exp2(x) and log2(x) are calculated using polynomial approximations.
  float32x4_t log2_a, b_log2_a, a_exp_b;

  // Calculate log2(x), x = a.
  {
    // To calculate log2(x), we decompose x like this:
    //   x = y * 2^n
    //     n is an integer
    //     y is in the [1.0, 2.0) range
    //
    //   log2(x) = log2(y) + n
    //     n       can be evaluated by playing with float representation.
    //     log2(y) in a small range can be approximated, this code uses an order
    //             five polynomial approximation. The coefficients have been
    //             estimated with the Remez algorithm and the resulting
    //             polynomial has a maximum relative error of 0.00086%.

    // Compute n.
    //    This is done by masking the exponent, shifting it into the top bit of
    //    the mantissa, putting eight into the biased exponent (to shift/
    //    compensate the fact that the exponent has been shifted in the top/
    //    fractional part and finally getting rid of the implicit leading one
    //    from the mantissa by substracting it out.
    const uint32x4_t vec_float_exponent_mask = vdupq_n_u32(0x7F800000);
    const uint32x4_t vec_eight_biased_exponent = vdupq_n_u32(0x43800000);
    const uint32x4_t vec_implicit_leading_one = vdupq_n_u32(0x43BF8000);
    const uint32x4_t two_n = vandq_u32(vreinterpretq_u32_f32(a),
                                       vec_float_exponent_mask);
    const uint32x4_t n_1 = vshrq_n_u32(two_n, kShiftExponentIntoTopMantissa);
    const uint32x4_t n_0 = vorrq_u32(n_1, vec_eight_biased_exponent);
    const float32x4_t n =
        vsubq_f32(vreinterpretq_f32_u32(n_0),
                  vreinterpretq_f32_u32(vec_implicit_leading_one));
    // Compute y.
    const uint32x4_t vec_mantissa_mask = vdupq_n_u32(0x007FFFFF);
    const uint32x4_t vec_zero_biased_exponent_is_one = vdupq_n_u32(0x3F800000);
    const uint32x4_t mantissa = vandq_u32(vreinterpretq_u32_f32(a),
                                          vec_mantissa_mask);
    const float32x4_t y =
        vreinterpretq_f32_u32(vorrq_u32(mantissa,
                                        vec_zero_biased_exponent_is_one));
    // Approximate log2(y) ~= (y - 1) * pol5(y).
    //    pol5(y) = C5 * y^5 + C4 * y^4 + C3 * y^3 + C2 * y^2 + C1 * y + C0
    const float32x4_t C5 = vdupq_n_f32(-3.4436006e-2f);
    const float32x4_t C4 = vdupq_n_f32(3.1821337e-1f);
    const float32x4_t C3 = vdupq_n_f32(-1.2315303f);
    const float32x4_t C2 = vdupq_n_f32(2.5988452f);
    const float32x4_t C1 = vdupq_n_f32(-3.3241990f);
    const float32x4_t C0 = vdupq_n_f32(3.1157899f);
    float32x4_t pol5_y = C5;
    pol5_y = vmlaq_f32(C4, y, pol5_y);
    pol5_y = vmlaq_f32(C3, y, pol5_y);
    pol5_y = vmlaq_f32(C2, y, pol5_y);
    pol5_y = vmlaq_f32(C1, y, pol5_y);
    pol5_y = vmlaq_f32(C0, y, pol5_y);
    const float32x4_t y_minus_one =
        vsubq_f32(y, vreinterpretq_f32_u32(vec_zero_biased_exponent_is_one));
    const float32x4_t log2_y = vmulq_f32(y_minus_one, pol5_y);

    // Combine parts.
    log2_a = vaddq_f32(n, log2_y);
  }

  // b * log2(a)
  b_log2_a = vmulq_f32(b, log2_a);

  // Calculate exp2(x), x = b * log2(a).
  {
    // To calculate 2^x, we decompose x like this:
    //   x = n + y
    //     n is an integer, the value of x - 0.5 rounded down, therefore
    //     y is in the [0.5, 1.5) range
    //
    //   2^x = 2^n * 2^y
    //     2^n can be evaluated by playing with float representation.
    //     2^y in a small range can be approximated, this code uses an order two
    //         polynomial approximation. The coefficients have been estimated
    //         with the Remez algorithm and the resulting polynomial has a
    //         maximum relative error of 0.17%.
    // To avoid over/underflow, we reduce the range of input to ]-127, 129].
    const float32x4_t max_input = vdupq_n_f32(129.f);
    const float32x4_t min_input = vdupq_n_f32(-126.99999f);
    const float32x4_t x_min = vminq_f32(b_log2_a, max_input);
    const float32x4_t x_max = vmaxq_f32(x_min, min_input);
    // Compute n.
    const float32x4_t half = vdupq_n_f32(0.5f);
    const float32x4_t x_minus_half = vsubq_f32(x_max, half);
    const int32x4_t x_minus_half_floor = vcvtq_s32_f32(x_minus_half);

    // Compute 2^n.
    const int32x4_t float_exponent_bias = vdupq_n_s32(127);
    const int32x4_t two_n_exponent =
        vaddq_s32(x_minus_half_floor, float_exponent_bias);
    const float32x4_t two_n =
        vreinterpretq_f32_s32(vshlq_n_s32(two_n_exponent, kFloatExponentShift));
    // Compute y.
    const float32x4_t y = vsubq_f32(x_max, vcvtq_f32_s32(x_minus_half_floor));

    // Approximate 2^y ~= C2 * y^2 + C1 * y + C0.
    const float32x4_t C0 = vdupq_n_f32(1.00000338f);
    const float32x4_t C1 = vdupq_n_f32(0.69314844f);
    const float32x4_t C2 = vdupq_n_f32(0.24015562f);
    const float32x4_t C3 = vdupq_n_f32(0.05549279f);
    const float32x4_t C4 = vdupq_n_f32(0.00983013f);
    const float32x4_t C5 = vdupq_n_f32(0.00135819f);
    float32x4_t exp2_y = C5;
    exp2_y = vmlaq_f32(C4, y, exp2_y);
    exp2_y = vmlaq_f32(C3, y, exp2_y);
    exp2_y = vmlaq_f32(C2, y, exp2_y);
    exp2_y = vmlaq_f32(C1, y, exp2_y);
    exp2_y = vmlaq_f32(C0, y, exp2_y);

    // Combine parts.
    a_exp_b = vmulq_f32(exp2_y, two_n);
  }

  return a_exp_b;
}

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
static void WebRtcAec_ERL_EstHnl (AecCore* aec, float hNl[PART_LEN1]) {

    int i = 0, p, q;
    float fReEcho, fEstEcho, fErlTmp = 1;
    float hNlErl[PART_LEN1] = {0}, hNlErlTmp;
    float hNlErlSave[PART_LEN1] = {0};
    float afHnlAvg = 0;
    int sStartNum, sEndNum, sSBNum;

    float afErlTmp[PART_LEN1] = {0.0f}, fhntmp[4];
    float32x4_t erltmp, erlsm, recho, efpsd, xfpsd, yfpsd, hnltmp, kzero, kone, k255;
    float32x4_t hnlsv_0, hnlsv_1, hnlsv_2, hnlavg, k1div3, k0125;

    short asSubBand[kSubBandNum + 1] = {0, 5, 9, 13, 17, 25, 37, 49, 65};

    if (aec->iBeginOfEcho)
    {
        fErlTmp = 4;
    }

    int iNumTmp = aec->mult > 1 ? 6 : 8;
    afErlTmp[0] = (0 > asSubBand[iNumTmp]) ? (2 * fErlTmp) : fErlTmp;
    for (i = 1; i < PART_LEN1; i++) {
        afErlTmp[i] = (i > asSubBand[iNumTmp]) ? (2 * afErlTmp[ i - 1]) : afErlTmp[ i - 1];
    }

    kzero = vdupq_n_f32(0.0f);
    kone = vdupq_n_f32(1.0f);
    k255 = vdupq_n_f32(255.0f);
    for (i = 0; i < PART_LEN; i += 4) {
        ////fErlTmp = (i > asSubBand[iNumTmp]) ? (2 * fErlTmp) : fErlTmp;
        //fEstEcho = aec->afXfPSD[i] * aec->afERLSm[i] * fErlTmp;
        xfpsd = vld1q_f32(&aec->afXfPSD[i]);
        erlsm = vld1q_f32(&aec->afERLSm[i]);
        erltmp = vld1q_f32(&afErlTmp[i]);
        recho = vmulq_f32(xfpsd, vmulq_f32(erlsm, erltmp));
        //fReEcho = fEstEcho - aec->afYfPSD[i];
        yfpsd = vld1q_f32(&aec->afYfPSD[i]);
        recho = vsubq_f32(recho, yfpsd);
        //fReEcho = fReEcho > 0 ? fReEcho : 0;
        recho = vmaxq_f32(kzero, recho);
        //aec->afEfPSD[i] = aec->afEfPSD[i] > 225 ? aec->afEfPSD[i] : 225;
        efpsd = vmaxq_f32(vld1q_f32(&aec->afEfPSD[i]), k255);
        vst1q_f32(&aec->afEfPSD[i], efpsd);
        //hNlErlTmp = fReEcho / aec->afEfPSD[i];
        hnltmp = vdivq_f32(recho, efpsd);
        //hNlErlTmp = hNlErlTmp > 1 ? 1 : hNlErlTmp;
        hnltmp = vminq_f32(hnltmp, kone);
        hnltmp = vsubq_f32(kone, hnltmp);
        //hNlErl[i] = 1 - hNlErlTmp;
        vst1q_f32(&hNlErl[i], hnltmp);
        //hNlErlSave[i] = hNlErl[i];
        vst1q_f32(&hNlErlSave[i], hnltmp);
    }
    for (; i < PART_LEN1; i++) {
        ////fErlTmp = (i > asSubBand[iNumTmp]) ? (2 * fErlTmp) : fErlTmp;
        fEstEcho = aec->afXfPSD[i] * aec->afERLSm[i] * fErlTmp;
        fReEcho = fEstEcho - aec->afYfPSD[i];
        fReEcho = fReEcho > 0 ? fReEcho : 0;
        aec->afEfPSD[i] = aec->afEfPSD[i] > 225 ? aec->afEfPSD[i] : 225;
        hNlErlTmp = fReEcho / aec->afEfPSD[i];
        hNlErlTmp = hNlErlTmp > 1 ? 1 : hNlErlTmp;
        hNlErl[i] = 1 - hNlErlTmp;
        hNlErlSave[i] = hNlErl[i];
    }
    fErlTmp = afErlTmp[PART_LEN];

    p = (PART_LEN - 1)>>2;
    k0125 = vdupq_n_f32(0.125f);
    k1div3 = vdupq_n_f32(1.0f / 3.0f);
    i = 0;
    for (q = 0; q < p; q++)
    {
        //afHnlAvg = (hNlErlSave[i-1] + hNlErlSave[i] + hNlErlSave[i+1])/3;
        hnlsv_0 = vld1q_f32(&hNlErlSave[i]);
        hnlsv_1 = vld1q_f32(&hNlErlSave[i + 1]);
        hnlsv_2 = vld1q_f32(&hNlErlSave[i + 2]);
        hnlavg = vaddq_f32(hnlsv_0, vaddq_f32(hnlsv_1, hnlsv_2));
        hnlavg = vmulq_f32(k1div3, hnlavg);

        //afHnlAvg = afHnlAvg < 0.125 ? 0 : afHnlAvg;
        //hnlavg = vandq_u32(vcgeq_f32(hnlavg, k0125), hnlavg);
        vst1q_f32(&fhntmp[0], hnlavg);
        if(fhntmp[0] < 0.125f)fhntmp[0] = 0.0f;
        if(fhntmp[1] < 0.125f)fhntmp[1] = 0.0f;
        if(fhntmp[2] < 0.125f)fhntmp[2] = 0.0f;
        if(fhntmp[3] < 0.125f)fhntmp[3] = 0.0f;
        hnlavg = vld1q_f32(&fhntmp[0]);

        //hNlErl[i] = afHnlAvg;
        vst1q_f32(&hNlErl[i + 1], hnlavg);

        i += 4;
    }
    for (i += 1; i < PART_LEN; i++)
    {
        afHnlAvg = (hNlErlSave[i-1] + hNlErlSave[i] + hNlErlSave[i+1])/3;
        afHnlAvg = afHnlAvg < 0.125 ? 0 : afHnlAvg;
        hNlErl[i] = afHnlAvg;
    }

    for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++)
    {
        sStartNum = asSubBand[sSBNum];
        sEndNum = asSubBand[sSBNum + 1];
        afHnlAvg = 0;
        for (i = sStartNum; i < sEndNum; i++) {
            afHnlAvg += hNlErl[i];
        }
        afHnlAvg = afHnlAvg / (sEndNum - sStartNum);
        afHnlAvg = afHnlAvg < 0.25 ? 0 : afHnlAvg;
        for (i = sStartNum; i < sEndNum; i++)
        {
            fErlTmp = hNlErl[i] > afHnlAvg ? afHnlAvg : hNlErl[i];
            if (hNl[i] < fErlTmp)
            {
                hNl[i] = hNl[i] * 0.5 + fErlTmp * 0.5;
            }
        }
    }
}

static void OverdriveAndSuppressNEON(AecCore* aec,
                                 float hNl[PART_LEN1],
                                 float hNlFb[kSubBandNum],
                                 float efw[2][PART_LEN1]) {

    short asSubBand[kSubBandNum + 1] = {0, 5, 9, 13, 17, 25, 37, 49, 65};
    int i, p, q, sSBNum, sStartNum, sEndNum, iTmp;
    float afHnlAvg[kSubBandNum] = {0};
    float fHnlVoiceAvg = 0, fTmp = 0, fHnl1000UpAvg = 0, fHnl500Avg = 0;
    //int iHzPerNum = 4000 * aec->mult / PART_LEN;
    int iNum4500Hz = 0, iNum1000Hz = 0, iNum4000Hz = 0;
    float hNlTmp[PART_LEN1], fHnl1000DownAvg;

    float32x4_t fods, fhnl, fodc;

    if (aec->mult > 1)
    {
        iNum4500Hz = 6;
        iNum1000Hz = 1;
        iNum4000Hz = PART_LEN1 / 2;
    }
    else
    {
        iNum4500Hz = 7;
        iNum1000Hz = 3;
        iNum4000Hz = PART_LEN1;
    }

    if (aec->iBeginOfEcho > 0)
    {
        memcpy(hNlTmp, hNl, sizeof(float) * PART_LEN1);
        qsort(hNlTmp, PART_LEN1, sizeof(float), CmpFloat);
        fTmp = hNlTmp[(int)floor(0.25 * (PART_LEN1 - 1))];
        iTmp = 2 / aec->mult;
        //for (i = asSubBand[iTmp]; i < PART_LEN1; i++)
        //{
        //    hNl[i] = fTmp > hNl[i] ? hNl[i] : fTmp;
        //}
        i = asSubBand[iTmp];
        p = i>>2;
        float32x4_t ftmp, hn1;
        ftmp = vdupq_n_f32(fTmp);
        for(q = 0; q < p; q++){
            hn1 = vld1q_f32(&hNl[i]);
            hn1 = vminq_f32(hn1, ftmp);
            vst1q_f32(&hNl[i], hn1);
            i += 4;
        }
        for(; i < PART_LEN1; i++){
            hNl[i] = fTmp > hNl[i] ? hNl[i] : fTmp; 
        }
    }


    for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
        sStartNum = asSubBand[sSBNum];
        sEndNum = asSubBand[sSBNum + 1];
        for (i = sStartNum; i < sEndNum; i++) {
            // Weight subbands
            if (hNl[i] > hNlFb[sSBNum]) {
                hNl[i] = WebRtcAec_weightCurve[i] * hNlFb[sSBNum] +
                         (1 - WebRtcAec_weightCurve[i]) * hNl[i];
            }
            //hNl[i] = powf(hNl[i], aec->overDriveSm[sSBNum] * WebRtcAec_overDriveCurve[i]);
        }
        p = (sEndNum - sStartNum)>>2;
        fods = vdupq_n_f32(aec->overDriveSm[sSBNum]);
        i = sStartNum;
        for(q = 0; q < p; q++){
            fodc = vld1q_f32(&WebRtcAec_overDriveCurve[i]);
            fhnl = vld1q_f32(&hNl[i]);
            fhnl = vpowq_f32(fhnl, vmulq_f32(fods, fodc));
            vst1q_f32(&hNl[i], fhnl);
            i += 4;
        }
        for(; i < sEndNum; i++){
            hNl[i] = powf(hNl[i], aec->overDriveSm[sSBNum] * WebRtcAec_overDriveCurve[i]);
        }
    }

    short sDtFlag = WebRtcAec_DTDetection(aec, hNl);

    if (0 == aec->iBeginOfEcho){
        WebRtcAec_ERL_EstHnl(aec, hNl);
    }

    for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
        sStartNum = asSubBand[sSBNum];
        sEndNum = asSubBand[sSBNum + 1];
        for (i = sStartNum; i < sEndNum; i++) {
            afHnlAvg[sSBNum] += hNl[i];
        }
        afHnlAvg[sSBNum] = afHnlAvg[sSBNum] / (sEndNum - sStartNum);
        if (sSBNum < iNum4500Hz)
        {
            fHnlVoiceAvg += afHnlAvg[sSBNum];
        }
        if (sSBNum > iNum1000Hz)
        {
            fHnl1000UpAvg += afHnlAvg[sSBNum];
        }
    }
    if (aec->mult > 1)
    {
        fHnlVoiceAvg = fHnlVoiceAvg / 6;
        fHnl1000UpAvg = fHnl1000UpAvg / 6;
        fHnl1000DownAvg = (afHnlAvg[0] + afHnlAvg[1])/2;
        fHnl500Avg = afHnlAvg[0];
    }
    else
    {
        fHnlVoiceAvg = fHnlVoiceAvg / 7;
        fHnl1000UpAvg = fHnl1000UpAvg / 4;
        fHnl1000DownAvg = (afHnlAvg[0] + afHnlAvg[1] + afHnlAvg[2] + afHnlAvg[3])/4;
        fHnl500Avg = (afHnlAvg[0] + afHnlAvg[1])/2;
    }

    if (aec->dt_flag > 0) {
        for (i = 0; i < iNum4000Hz; i++) {
            hNl[i] = hNl[i] * (1 + hNl[i]);
            hNl[i] = hNl[i] < 1 ? hNl[i] : 1;
        }
    }
    else{
        if (fHnlVoiceAvg < 0.125 && fHnl500Avg < 0.0625) {
            for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
                sStartNum = asSubBand[sSBNum];
                sEndNum = asSubBand[sSBNum + 1];
                for (i = sStartNum; i < sEndNum; i++) {
                    hNl[i] = 0;
                }
            }
        }

    }
#if 1
    iTmp = 4 / aec->mult;
    // if there are voice echo at 0~4000Hz, we should cut the 1000Hz
    // aboved signals, to get rid of the non-linear echo.
    if (fHnlVoiceAvg < 0.125) {
        for (sSBNum = iTmp; sSBNum < kSubBandNum; sSBNum++) {
            sStartNum = asSubBand[sSBNum];
            sEndNum = asSubBand[sSBNum + 1];
            for (i = sStartNum; i < sEndNum; i++) {
                hNl[i] = 0;
            }
        }
    }

    // if there are echo under 1000Hz 
    if ((fHnl1000DownAvg < 0.15) && (fHnl1000UpAvg < 0.5))
    {
        for (i = asSubBand[iTmp]; i < PART_LEN1; i++)
        {
            hNl[i] = 0;
        }
    }
#endif
    aec->echoState = fHnl1000DownAvg < 0.15 ? 1 : 0;
    aec->fHnlVoiceAvg = fHnlVoiceAvg;
    //aec->dt_flag_old = fHnl500Avg * 10000;

    for (i = 0; i < PART_LEN1; i++) {
        // Suppress error signal
        efw[0][i] *= hNl[i];
        efw[1][i] *= hNl[i];

        // Ooura fft returns incorrect sign on imaginary component. It matters here
        // because we are making an additive change with comfort noise.
        efw[1][i] *= -1;
    }
}
#else
static void  OverdriveAndSuppressNEON(AecCore* aec,
                                 float hNl[PART_LEN1],
                                 const float hNlFb,
                                 float efw[2][PART_LEN1]) {
    int i;
    for (i = 0; i < PART_LEN1; i++) {
        // Weight subbands
        if (hNl[i] > hNlFb) {
            hNl[i] = WebRtcAec_weightCurve[i] * hNlFb +
                     (1 - WebRtcAec_weightCurve[i]) * hNl[i];
        }
        hNl[i] = powf(hNl[i], aec->overDriveSm * WebRtcAec_overDriveCurve[i]);

        // Suppress error signal
        efw[0][i] *= hNl[i];
        efw[1][i] *= hNl[i];

        // Ooura fft returns incorrect sign on imaginary component. It matters here
        // because we are making an additive change with comfort noise.
        efw[1][i] *= -1;
    }
}
#endif
static int PartitionDelay(const AecCore* aec) {
  // Measures the energy in each filter partition and returns the partition with
  // highest energy.
  // TODO(bjornv): Spread computational cost by computing one partition per
  // block?
  float wfEnMax = 0;
  int i;
  int delay = 0;

  for (i = 0; i < aec->num_partitions; i++) {
    int j;
    int pos = i * PART_LEN1;
    float wfEn = 0;
    float32x4_t vec_wfEn = vdupq_n_f32(0.0f);
    // vectorized code (four at once)
    for (j = 0; j + 3 < PART_LEN1; j += 4) {
      const float32x4_t vec_wfBuf0 = vld1q_f32(&aec->wfBuf[0][pos + j]);
      const float32x4_t vec_wfBuf1 = vld1q_f32(&aec->wfBuf[1][pos + j]);
      vec_wfEn = vmlaq_f32(vec_wfEn, vec_wfBuf0, vec_wfBuf0);
      vec_wfEn = vmlaq_f32(vec_wfEn, vec_wfBuf1, vec_wfBuf1);
    }
    {
      float32x2_t vec_total;
      // A B C D
      vec_total = vpadd_f32(vget_low_f32(vec_wfEn), vget_high_f32(vec_wfEn));
      // A+B C+D
      vec_total = vpadd_f32(vec_total, vec_total);
      // A+B+C+D A+B+C+D
      wfEn = vget_lane_f32(vec_total, 0);
    }

    // scalar code for the remaining items.
    for (; j < PART_LEN1; j++) {
      wfEn += aec->wfBuf[0][pos + j] * aec->wfBuf[0][pos + j] +
              aec->wfBuf[1][pos + j] * aec->wfBuf[1][pos + j];
    }

    if (wfEn > wfEnMax) {
      wfEnMax = wfEn;
      delay = i;
    }
  }
  return delay;
}

// Updates the following smoothed  Power Spectral Densities (PSD):
//  - sd  : near-end
//  - se  : residual echo
//  - sx  : far-end
//  - sde : cross-PSD of near-end and residual echo
//  - sxd : cross-PSD of near-end and far-end
//
// In addition to updating the PSDs, also the filter diverge state is determined
// upon actions are taken.
static void SmoothedPSD(AecCore* aec,
                        float efw[2][PART_LEN1],
                        float dfw[2][PART_LEN1],
                        float xfw[2][PART_LEN1]) {
  // Power estimate smoothing coefficients.
  const float* ptrGCoh = aec->extended_filter_enabled
      ? WebRtcAec_kExtendedSmoothingCoefficients[aec->mult - 1]
      : WebRtcAec_kNormalSmoothingCoefficients[aec->mult - 1];
  int i;
  float sdSum = 0, seSum = 0;
  const float32x4_t vec_15 =  vdupq_n_f32(WebRtcAec_kMinFarendPSD);
  float32x4_t vec_sdSum = vdupq_n_f32(0.0f);
  float32x4_t vec_seSum = vdupq_n_f32(0.0f);

  for (i = 0; i + 3 < PART_LEN1; i += 4) {
    const float32x4_t vec_dfw0 = vld1q_f32(&dfw[0][i]);
    const float32x4_t vec_dfw1 = vld1q_f32(&dfw[1][i]);
    const float32x4_t vec_efw0 = vld1q_f32(&efw[0][i]);
    const float32x4_t vec_efw1 = vld1q_f32(&efw[1][i]);
    const float32x4_t vec_xfw0 = vld1q_f32(&xfw[0][i]);
    const float32x4_t vec_xfw1 = vld1q_f32(&xfw[1][i]);
    float32x4_t vec_sd = vmulq_n_f32(vld1q_f32(&aec->sd[i]), ptrGCoh[0]);
    float32x4_t vec_se = vmulq_n_f32(vld1q_f32(&aec->se[i]), ptrGCoh[0]);
    float32x4_t vec_sx = vmulq_n_f32(vld1q_f32(&aec->sx[i]), ptrGCoh[0]);
    float32x4_t vec_dfw_sumsq = vmulq_f32(vec_dfw0, vec_dfw0);
    float32x4_t vec_efw_sumsq = vmulq_f32(vec_efw0, vec_efw0);
    float32x4_t vec_xfw_sumsq = vmulq_f32(vec_xfw0, vec_xfw0);

    vec_dfw_sumsq = vmlaq_f32(vec_dfw_sumsq, vec_dfw1, vec_dfw1);
    vec_efw_sumsq = vmlaq_f32(vec_efw_sumsq, vec_efw1, vec_efw1);
    vec_xfw_sumsq = vmlaq_f32(vec_xfw_sumsq, vec_xfw1, vec_xfw1);
    vec_xfw_sumsq = vmaxq_f32(vec_xfw_sumsq, vec_15);
    vec_sd = vmlaq_n_f32(vec_sd, vec_dfw_sumsq, ptrGCoh[1]);
    vec_se = vmlaq_n_f32(vec_se, vec_efw_sumsq, ptrGCoh[1]);
    vec_sx = vmlaq_n_f32(vec_sx, vec_xfw_sumsq, ptrGCoh[1]);

    vst1q_f32(&aec->sd[i], vec_sd);
    vst1q_f32(&aec->se[i], vec_se);
    vst1q_f32(&aec->sx[i], vec_sx);

    {
      float32x4x2_t vec_sde = vld2q_f32(&aec->sde[i][0]);
      float32x4_t vec_dfwefw0011 = vmulq_f32(vec_dfw0, vec_efw0);
      float32x4_t vec_dfwefw0110 = vmulq_f32(vec_dfw0, vec_efw1);
      vec_sde.val[0] = vmulq_n_f32(vec_sde.val[0], ptrGCoh[0]);
      vec_sde.val[1] = vmulq_n_f32(vec_sde.val[1], ptrGCoh[0]);
      vec_dfwefw0011 = vmlaq_f32(vec_dfwefw0011, vec_dfw1, vec_efw1);
      vec_dfwefw0110 = vmlsq_f32(vec_dfwefw0110, vec_dfw1, vec_efw0);
      vec_sde.val[0] = vmlaq_n_f32(vec_sde.val[0], vec_dfwefw0011, ptrGCoh[1]);
      vec_sde.val[1] = vmlaq_n_f32(vec_sde.val[1], vec_dfwefw0110, ptrGCoh[1]);
      vst2q_f32(&aec->sde[i][0], vec_sde);
    }

    {
      float32x4x2_t vec_sxd = vld2q_f32(&aec->sxd[i][0]);
      float32x4_t vec_dfwxfw0011 = vmulq_f32(vec_dfw0, vec_xfw0);
      float32x4_t vec_dfwxfw0110 = vmulq_f32(vec_dfw0, vec_xfw1);
      vec_sxd.val[0] = vmulq_n_f32(vec_sxd.val[0], ptrGCoh[0]);
      vec_sxd.val[1] = vmulq_n_f32(vec_sxd.val[1], ptrGCoh[0]);
      vec_dfwxfw0011 = vmlaq_f32(vec_dfwxfw0011, vec_dfw1, vec_xfw1);
      vec_dfwxfw0110 = vmlsq_f32(vec_dfwxfw0110, vec_dfw1, vec_xfw0);
      vec_sxd.val[0] = vmlaq_n_f32(vec_sxd.val[0], vec_dfwxfw0011, ptrGCoh[1]);
      vec_sxd.val[1] = vmlaq_n_f32(vec_sxd.val[1], vec_dfwxfw0110, ptrGCoh[1]);
      vst2q_f32(&aec->sxd[i][0], vec_sxd);
    }

    vec_sdSum = vaddq_f32(vec_sdSum, vec_sd);
    vec_seSum = vaddq_f32(vec_seSum, vec_se);
  }
  {
    float32x2_t vec_sdSum_total;
    float32x2_t vec_seSum_total;
    // A B C D
    vec_sdSum_total = vpadd_f32(vget_low_f32(vec_sdSum),
                                vget_high_f32(vec_sdSum));
    vec_seSum_total = vpadd_f32(vget_low_f32(vec_seSum),
                                vget_high_f32(vec_seSum));
    // A+B C+D
    vec_sdSum_total = vpadd_f32(vec_sdSum_total, vec_sdSum_total);
    vec_seSum_total = vpadd_f32(vec_seSum_total, vec_seSum_total);
    // A+B+C+D A+B+C+D
    sdSum = vget_lane_f32(vec_sdSum_total, 0);
    seSum = vget_lane_f32(vec_seSum_total, 0);
  }

  // scalar code for the remaining items.
  for (; i < PART_LEN1; i++) {
    aec->sd[i] = ptrGCoh[0] * aec->sd[i] +
                 ptrGCoh[1] * (dfw[0][i] * dfw[0][i] + dfw[1][i] * dfw[1][i]);
    aec->se[i] = ptrGCoh[0] * aec->se[i] +
                 ptrGCoh[1] * (efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i]);
    // We threshold here to protect against the ill-effects of a zero farend.
    // The threshold is not arbitrarily chosen, but balances protection and
    // adverse interaction with the algorithm's tuning.
    // TODO(bjornv): investigate further why this is so sensitive.
    aec->sx[i] =
        ptrGCoh[0] * aec->sx[i] +
        ptrGCoh[1] * WEBRTC_SPL_MAX(
            xfw[0][i] * xfw[0][i] + xfw[1][i] * xfw[1][i],
            WebRtcAec_kMinFarendPSD);

    aec->sde[i][0] =
        ptrGCoh[0] * aec->sde[i][0] +
        ptrGCoh[1] * (dfw[0][i] * efw[0][i] + dfw[1][i] * efw[1][i]);
    aec->sde[i][1] =
        ptrGCoh[0] * aec->sde[i][1] +
        ptrGCoh[1] * (dfw[0][i] * efw[1][i] - dfw[1][i] * efw[0][i]);

    aec->sxd[i][0] =
        ptrGCoh[0] * aec->sxd[i][0] +
        ptrGCoh[1] * (dfw[0][i] * xfw[0][i] + dfw[1][i] * xfw[1][i]);
    aec->sxd[i][1] =
        ptrGCoh[0] * aec->sxd[i][1] +
        ptrGCoh[1] * (dfw[0][i] * xfw[1][i] - dfw[1][i] * xfw[0][i]);

    sdSum += aec->sd[i];
    seSum += aec->se[i];
  }

  // Divergent filter safeguard.
  aec->divergeState = (aec->divergeState ? 1.05f : 1.0f) * seSum > sdSum;

  if (aec->divergeState)
    memcpy(efw, dfw, sizeof(efw[0][0]) * 2 * PART_LEN1);

  // Reset if error is significantly larger than nearend (13 dB).
  if (!aec->extended_filter_enabled && seSum > (19.95f * sdSum))
    memset(aec->wfBuf, 0, sizeof(aec->wfBuf));
}

static void PSDSm_For_ERL_Estimation(AecCore* aec,
                        float efw[2][PART_LEN1],
                        float dfw[2][PART_LEN1],
                        float xfw[2][PART_LEN1])
{
    int i = 0, j = 0;
    float dfTmp, efTmp, xfTmp, yfTmp;
    float ef_df_compare = 0;
    float fSmStep = 0.5, fTmp;
    float adftmp[4], aeftmp[4], ksstmp[4];
    float32x4_t kmin, ktow, kone, k075, kssp, kssq, xftmp, yftmp, eftmp, dftmp;
    float32x4_t dfwr, dfwi, efwr, efwi, xfwr, xfwi, yfwr, yfwi, xfp, yfp, dfp, efp;

    kone = vdupq_n_f32(1.0f);
    ktow = vdupq_n_f32(2.0f);
    k075 = vdupq_n_f32(0.75f);
    kmin = vdupq_n_f32(WebRtcAec_kMinFarendPSD);
    for (i = 0; i< PART_LEN; i += 4) {
        //dfTmp = dfw[0][i] * dfw[0][i] + dfw[1][i] * dfw[1][i];
        dfwr = vld1q_f32(&dfw[0][i]);
        dfwi = vld1q_f32(&dfw[1][i]);
        //efTmp = efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i];
        efwr = vld1q_f32(&efw[0][i]);
        efwi = vld1q_f32(&efw[1][i]);
        //xfTmp = WEBRTC_SPL_MAX(
        //        xfw[0][i] * xfw[0][i] + xfw[1][i] * xfw[1][i],
        //        WebRtcAec_kMinFarendPSD);
        xfwr = vld1q_f32(&xfw[0][i]);
        xfwi = vld1q_f32(&xfw[1][i]);
        //yfTmp = aec->afYfwBuf[0][i] * aec->afYfwBuf[0][i] + 
        //        aec->afYfwBuf[1][i] * aec->afYfwBuf[1][i];
        yfwr = vld1q_f32(&aec->afYfwBuf[0][i]);
        yfwi = vld1q_f32(&aec->afYfwBuf[1][i]);

        dftmp = vaddq_f32(vmulq_f32(dfwr, dfwr), vmulq_f32(dfwi, dfwi));
        eftmp = vaddq_f32(vmulq_f32(efwr, efwr), vmulq_f32(efwi, efwi));
        yftmp = vaddq_f32(vmulq_f32(yfwr, yfwr), vmulq_f32(yfwi, yfwi));
        xftmp = vaddq_f32(vmulq_f32(xfwr, xfwr), vmulq_f32(xfwi, xfwi));
        xftmp = vmaxq_f32(xftmp, kmin);

        //fTmp = xfTmp/WEBRTC_SPL_MAX(aec->afXfPSD[i], WebRtcAec_kMinFarendPSD);
        efp = vld1q_f32(&aec->afEfPSD[i]);
        dfp = vld1q_f32(&aec->afDfPSD[i]);
        yfp = vld1q_f32(&aec->afYfPSD[i]);
        xfp = vld1q_f32(&aec->afXfPSD[i]);
        kssp = vmaxq_f32(xfp, kmin);
        kssp = vdivq_f32(xftmp, kssp);

        //fTmp = (fTmp > 2) ? 2 : fTmp;
        kssp = vminq_f32(kssp, ktow);
        //fTmp = (fTmp < 1) ? 0.75 : fTmp;
        vst1q_f32(&ksstmp[0], kssp);
        for(j = 0; j < 4; j++){
            if(ksstmp[j] < 1.0f){
                ksstmp[j] = 0.75f;
            }
        }
        kssp = vld1q_f32(&ksstmp[0]);
        
        
        //fTmp = 1 - fTmp;
        kssp = vsubq_f32(kone, kssp);
        //fSmStep = (fTmp >= 0) ? fTmp : (0 - fTmp);
        kssp = vabsq_f32(kssp);
        kssq = vsubq_f32(kone, kssp);
        
        //aec->afDfPSD[i] = aec->afDfPSD[i] * (1 - fSmStep) + dfTmp * fSmStep;
        //aec->afXfPSD[i] = aec->afXfPSD[i] * (1 - fSmStep) + xfTmp * fSmStep;
        //aec->afYfPSD[i] = aec->afYfPSD[i] * (1 - fSmStep) + yfTmp * fSmStep;
        dfp = vaddq_f32(vmulq_f32(kssq, dfp), vmulq_f32(kssp, dftmp));
        xfp = vaddq_f32(vmulq_f32(kssq, xfp), vmulq_f32(kssp, xftmp));
        yfp = vaddq_f32(vmulq_f32(kssq, yfp), vmulq_f32(kssp, yftmp));

        vst1q_f32(&adftmp[0], dftmp);
        vst1q_f32(&aeftmp[0], eftmp);

        //if (efTmp > dfTmp) {
        //    efw[0][i] = dfw[0][i];
        //    efw[1][i] = dfw[1][i];
        //    efTmp = dfTmp;
        //    ef_df_compare++;
        //}
        
        for(j = 0; j < 4; j++){
          if(aeftmp[j] > adftmp[j]){
            efw[0][i + j] = dfw[0][i + j];
            efw[1][i + j] = dfw[1][i + j];
            aeftmp[j] = adftmp[j];
            ef_df_compare++;
          }
        }
        
        //aec->afEfPSD[i] = aec->afEfPSD[i] * (1 - fSmStep) + efTmp * fSmStep;
        eftmp = vld1q_f32(&aeftmp[0]);
        efp = vaddq_f32(vmulq_f32(kssq, efp), vmulq_f32(kssp, eftmp));


        vst1q_f32(&aec->afDfPSD[i], dfp);
        vst1q_f32(&aec->afEfPSD[i], efp);
        vst1q_f32(&aec->afYfPSD[i], yfp);
        vst1q_f32(&aec->afXfPSD[i], xfp);
    }
    // The remain index PART_LEN
    {
        dfTmp = dfw[0][i] * dfw[0][i] + dfw[1][i] * dfw[1][i];
        efTmp = efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i];
        xfTmp = WEBRTC_SPL_MAX(
                xfw[0][i] * xfw[0][i] + xfw[1][i] * xfw[1][i],
                WebRtcAec_kMinFarendPSD);
        yfTmp = aec->afYfwBuf[0][i] * aec->afYfwBuf[0][i] + 
                aec->afYfwBuf[1][i] * aec->afYfwBuf[1][i];

        fTmp = xfTmp/WEBRTC_SPL_MAX(aec->afXfPSD[i], WebRtcAec_kMinFarendPSD);
        fTmp = (fTmp > 2) ? 2 : fTmp;
        fTmp = (fTmp < 1) ? 0.75 : fTmp;
        fTmp = 1 - fTmp;
        fSmStep = (fTmp >= 0) ? fTmp : (0 - fTmp);

        aec->afDfPSD[i] = aec->afDfPSD[i] * (1 - fSmStep) + dfTmp * fSmStep;
        aec->afXfPSD[i] = aec->afXfPSD[i] * (1 - fSmStep) + xfTmp * fSmStep;
        aec->afYfPSD[i] = aec->afYfPSD[i] * (1 - fSmStep) + yfTmp * fSmStep;
        if (efTmp > dfTmp) {
            efw[0][i] = dfw[0][i];
            efw[1][i] = dfw[1][i];
            efTmp = dfTmp;
            ef_df_compare++;
        }
        aec->afEfPSD[i] = aec->afEfPSD[i] * (1 - fSmStep) + efTmp * fSmStep;
    }

    if ((ef_df_compare > 32) && (aec->filter_reseted >= 0))
    {
      aec->filter_reseted++;
    }
    else if ((ef_df_compare < 10) && (aec->filter_reseted <= 0))
    {
      aec->filter_reseted--;
    }
    else
    {
      aec->filter_reseted = 0;
    }

};


// Window time domain data to be used by the fft.
__inline static void WindowData(float* x_windowed, const float* x) {
  int i;
  for (i = 0; i < PART_LEN; i += 4) {
    const float32x4_t vec_Buf1 = vld1q_f32(&x[i]);
    const float32x4_t vec_Buf2 = vld1q_f32(&x[PART_LEN + i]);
    const float32x4_t vec_sqrtHanning = vld1q_f32(&WebRtcAec_sqrtHanning[i]);
    // A B C D
    float32x4_t vec_sqrtHanning_rev =
        vld1q_f32(&WebRtcAec_sqrtHanning[PART_LEN - i - 3]);
    // B A D C
    vec_sqrtHanning_rev = vrev64q_f32(vec_sqrtHanning_rev);
    // D C B A
    vec_sqrtHanning_rev = vcombine_f32(vget_high_f32(vec_sqrtHanning_rev),
                                       vget_low_f32(vec_sqrtHanning_rev));
    vst1q_f32(&x_windowed[i], vmulq_f32(vec_Buf1, vec_sqrtHanning));
    vst1q_f32(&x_windowed[PART_LEN + i],
            vmulq_f32(vec_Buf2, vec_sqrtHanning_rev));
  }
}

// Puts fft output data into a complex valued array.
__inline static void StoreAsComplex(const float* data,
                                    float data_complex[2][PART_LEN1]) {
  int i;
  for (i = 0; i < PART_LEN; i += 4) {
    const float32x4x2_t vec_data = vld2q_f32(&data[2 * i]);
    vst1q_f32(&data_complex[0][i], vec_data.val[0]);
    vst1q_f32(&data_complex[1][i], vec_data.val[1]);
  }
  // fix beginning/end values
  data_complex[1][0] = 0;
  data_complex[1][PART_LEN] = 0;
  data_complex[0][0] = data[0];
  data_complex[0][PART_LEN] = data[1];
}

static void SubbandCoherenceNEON(AecCore* aec,
                                 float efw[2][PART_LEN1],
                                 float xfw[2][PART_LEN1],
                                 float* fft,
                                 float* cohde,
                                 float* cohxd) {
  float dfw[2][PART_LEN1];
  int i;

  if (aec->delayEstCtr == 0)
    aec->delayIdx = PartitionDelay(aec);

  // Use delayed far.
  memcpy(xfw,
         aec->xfwBuf + aec->delayIdx * PART_LEN1,
         sizeof(xfw[0][0]) * 2 * PART_LEN1);

  // Windowed near fft
  WindowData(fft, aec->dBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, dfw);

  // Windowed error fft
  WindowData(fft, aec->eBuf);
  aec_rdft_forward_128(fft);
  StoreAsComplex(fft, efw);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    PSDSm_For_ERL_Estimation(aec, efw, dfw, xfw);
#endif

  SmoothedPSD(aec, efw, dfw, xfw);

  {
    const float32x4_t vec_1eminus10 =  vdupq_n_f32(1e-10f);

    // Subband coherence
    for (i = 0; i + 3 < PART_LEN1; i += 4) {
      const float32x4_t vec_sd = vld1q_f32(&aec->sd[i]);
      const float32x4_t vec_se = vld1q_f32(&aec->se[i]);
      const float32x4_t vec_sx = vld1q_f32(&aec->sx[i]);
      const float32x4_t vec_sdse = vmlaq_f32(vec_1eminus10, vec_sd, vec_se);
      const float32x4_t vec_sdsx = vmlaq_f32(vec_1eminus10, vec_sd, vec_sx);
      float32x4x2_t vec_sde = vld2q_f32(&aec->sde[i][0]);
      float32x4x2_t vec_sxd = vld2q_f32(&aec->sxd[i][0]);
      float32x4_t vec_cohde = vmulq_f32(vec_sde.val[0], vec_sde.val[0]);
      float32x4_t vec_cohxd = vmulq_f32(vec_sxd.val[0], vec_sxd.val[0]);
      vec_cohde = vmlaq_f32(vec_cohde, vec_sde.val[1], vec_sde.val[1]);
      vec_cohde = vdivq_f32(vec_cohde, vec_sdse);
      vec_cohxd = vmlaq_f32(vec_cohxd, vec_sxd.val[1], vec_sxd.val[1]);
      vec_cohxd = vdivq_f32(vec_cohxd, vec_sdsx);

      vst1q_f32(&cohde[i], vec_cohde);
      vst1q_f32(&cohxd[i], vec_cohxd);
    }
  }
  // scalar code for the remaining items.
  for (; i < PART_LEN1; i++) {
    cohde[i] =
        (aec->sde[i][0] * aec->sde[i][0] + aec->sde[i][1] * aec->sde[i][1]) /
        (aec->sd[i] * aec->se[i] + 1e-10f);
    cohxd[i] =
        (aec->sxd[i][0] * aec->sxd[i][0] + aec->sxd[i][1] * aec->sxd[i][1]) /
        (aec->sx[i] * aec->sd[i] + 1e-10f);
  }
}

void ComfortNoiseNEON(AecCore* aec,
                         float efw[2][PART_LEN1],
                         complex_t* comfortNoiseHband,
                         const float* noisePow,
                         const float* lambda) {
    int i, num;
    float rand[PART_LEN];
    float noiseAvg, tmp, tmpAvg, randtmp[4], noisetmp[4], costmp[4], sintmp[4];
    int16_t randW16[PART_LEN];
    complex_t u[PART_LEN1];

    //float noise, pi2 = 6.28318530717959f;

    const float32x4_t k2pi = vdupq_n_f32(6.28318530717959f);
    const float32x4_t k32768 = vdupq_n_f32(32768.0f);
    const float32x4_t kone = vdupq_n_f32(1.0f);
    const float32x4_t kzero = vdupq_n_f32(0.0f);
    float32x4x2_t vexpw;
    float32x4_t randw, nsamp, vsinw, vcosw, flamb, ftmpr, ftmpi, fefwr, fefwi, tmpavg;

    // Generate a uniform random array on [0 1]
    YoumeWebRtcSpl_RandUArray(randW16, PART_LEN, &aec->seed);
    randw = vdupq_n_f32(0.0f);
    for (i = 0; i < PART_LEN; i += 4) {
        //rand[i] = ((float)randW16[i]) / 32768;
        //randw = vld1q_f32(&((float)randW16[i]));
        randtmp[0] = (float)randW16[i + 0];
        randtmp[1] = (float)randW16[i + 1];
        randtmp[2] = (float)randW16[i + 2];
        randtmp[3] = (float)randW16[i + 3];
        randw = vld1q_f32(&randtmp[0]);
        vst1q_f32(&rand[i], vdivq_f32(randw, k32768));
    }

    // Reject LF noise
    u[0][0] = 0;
    u[0][1] = 0;
    for (i = 1; i < PART_LEN1; i += 4) {
        //tmp = pi2 * rand[i - 1];
        randw = vmulq_f32(k2pi, vld1q_f32(&rand[i - 1]));
        //noise = sqrtf(noisePow[i]);
        nsamp = vsqrtq_f32(vld1q_f32(&noisePow[i]));

        vst1q_f32(&randtmp[0], randw);
        vst1q_f32(&noisetmp[0], nsamp);
        costmp[0] = cosf(randtmp[0]);
        sintmp[0] = sinf(randtmp[0]);
        costmp[1] = cosf(randtmp[1]);
        sintmp[1] = sinf(randtmp[1]);
        costmp[2] = cosf(randtmp[2]);
        sintmp[2] = sinf(randtmp[2]);
        costmp[3] = cosf(randtmp[3]);
        sintmp[3] = sinf(randtmp[3]);        
        //u[i][0] = noise * cosf(tmp);
        //u[i][1] = -noise * sinf(tmp);
        vcosw = vcosq_f32(randw);
        vsinw = vsinq_f32(randw);
        vexpw.val[0] = vmulq_f32(nsamp, vcosw);
        vexpw.val[1] = vnegq_f32(vmulq_f32(nsamp, vsinw));
        vst2q_f32(&u[i][0], vexpw);
    }
    u[PART_LEN][1] = 0;

    for (i = 0; i < PART_LEN; i += 4) {
        // This is the proper weighting to match the background noise power
        //tmp = sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
        flamb = vld1q_f32(&lambda[i]);
        flamb = vsubq_f32(kone, vmulq_f32(flamb, flamb));
        flamb = vmaxq_f32(kzero, flamb);
        flamb = vsqrtq_f32(flamb);

        //efw[0][i] += tmp * u[i][0];
        //efw[1][i] += tmp * u[i][1];
        fefwr = vld1q_f32(&efw[0][i]);
        fefwi = vld1q_f32(&efw[1][i]);
        vexpw = vld2q_f32(&u[i][0]);
        ftmpr = vmulq_f32(vexpw.val[0], flamb);
        ftmpi = vmulq_f32(vexpw.val[1], flamb);
        vst1q_f32(&efw[0][i], vaddq_f32(fefwr, ftmpr));
        vst1q_f32(&efw[1][i], vaddq_f32(fefwi, ftmpi));
    }
    {
        tmp = sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
        // tmp = 1 - lambda[i];
        efw[0][i] += tmp * u[i][0];
        efw[1][i] += tmp * u[i][1];      
    }

    // For H band comfort noise
    // TODO: don't compute noise and "tmp" twice. Use the previous results.
    noiseAvg = 0.0;
    tmpAvg = 0.0;
    num = 0;
    if (aec->num_bands > 1/* && flagHbandCn == 1*/) {

        // average noise scale
        // average over second half of freq spectrum (i.e., 4->8khz)
        // TODO: we shouldn't need num. We know how many elements we're summing.
        for (i = PART_LEN1 >> 1; i < PART_LEN; i += 4) {
            num += 4;
            //noiseAvg += sqrtf(noisePow[i]);
            nsamp = vsqrtq_f32(vld1q_f32(&noisePow[i]));
            noiseAvg += vgetq_lane_f32(nsamp, 0);
            noiseAvg += vgetq_lane_f32(nsamp, 1);
            noiseAvg += vgetq_lane_f32(nsamp, 2);
            noiseAvg += vgetq_lane_f32(nsamp, 3);
        }
        {
            num++;
            noiseAvg += sqrtf(noisePow[i]);
        }
        noiseAvg /= (float)num;

        // average nlp scale
        // average over second half of freq spectrum (i.e., 4->8khz)
        // TODO: we shouldn't need num. We know how many elements we're summing.
        num = 0;
        for (i = PART_LEN1 >> 1; i < PART_LEN; i += 4) {
            num += 4;
            //tmpAvg += sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
            flamb = vld1q_f32(&lambda[i]);
            flamb = vsubq_f32(kone, vmulq_f32(flamb, flamb));
            flamb = vsqrtq_f32(vmaxq_f32(kzero, flamb));
            tmpAvg += vgetq_lane_f32(flamb, 0);
            tmpAvg += vgetq_lane_f32(flamb, 1);
            tmpAvg += vgetq_lane_f32(flamb, 2);
            tmpAvg += vgetq_lane_f32(flamb, 3);
        }
        {
            num++;
            tmpAvg += sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
        }
        tmpAvg /= (float)num;

        // Use average noise for H band
        // TODO: we should probably have a new random vector here.
        // Reject LF noise
        u[0][0] = 0;
        u[0][1] = 0;
        nsamp = vdupq_n_f32(noiseAvg);
        for (i = 1; i < PART_LEN1; i += 4) {
            //tmp = pi2 * rand[i - 1];
            randw = vmulq_f32(k2pi, vld1q_f32(&rand[i - 1]));

            // Use average noise for H band
            //u[i][0] = noiseAvg * (float)cos(tmp);
            //u[i][1] = -noiseAvg * (float)sin(tmp);
            vcosw = vcosq_f32(randw);
            vsinw = vsinq_f32(randw);
            vexpw.val[0] = vmulq_f32(nsamp, vcosw);
            vexpw.val[1] = vnegq_f32(vmulq_f32(nsamp, vsinw));      
            vst2q_f32(&u[i][0], vexpw);
        }
        u[PART_LEN][1] = 0;

        tmpavg = vdupq_n_f32(tmpAvg);
        for (i = 0; i < PART_LEN; i += 4) {
            // Use average NLP weight for H band
            //comfortNoiseHband[i][0] = tmpAvg * u[i][0];
            //comfortNoiseHband[i][1] = tmpAvg * u[i][1];
          vexpw = vld2q_f32(&u[i][0]);
          vexpw.val[0] = vmulq_f32(vexpw.val[0], tmpavg);
          vexpw.val[1] = vmulq_f32(vexpw.val[1], tmpavg);
          vst2q_f32(&comfortNoiseHband[i][0], vexpw);
        }
    }
}

void WebRtcAec_InitAec_neon(void) {
  WebRtcAec_FilterFar = FilterFarNEON;
  WebRtcAec_ScaleErrorSignal = ScaleErrorSignalNEON;
  WebRtcAec_FilterAdaptation = FilterAdaptationNEON;
  WebRtcAec_OverdriveAndSuppress = OverdriveAndSuppressNEON;
  WebRtcAec_SubbandCoherence = SubbandCoherenceNEON;
  WebRtcAec_ComfortNoise = ComfortNoiseNEON;
}
    }  // namespace webrtc_new
}  // namespace youme

#endif
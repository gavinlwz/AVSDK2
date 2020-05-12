/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_INTERNAL_NEW_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_INTERNAL_NEW_H_

#include <stdio.h>
#include "webrtc/common_audio/ring_buffer.h"
#include "aec_common_new.h"
#include "aec_core_new.h"
#include "webrtc/typedefs.h"

namespace youme{
namespace webrtc_new {
        
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
  #define kSubBandNum (8)
  #define kSubBand4500HzNum (6)
#endif
// Number of partitions for the extended filter mode. The first one is an enum
// to be used in array declarations, as it represents the maximum filter length.
enum {
  kExtendedNumPartitions = 32
};
static const int kNormalNumPartitions = 12;

// Delay estimator constants, used for logging and delay compensation if
// if reported delays are disabled.
enum {
  kLookaheadBlocks = 15
};
enum {
  // 500 ms for 16 kHz which is equivalent with the limit of reported delays.
  kHistorySizeBlocks = 250 //125
};

// Extended filter adaptation parameters.
// TODO(ajm): No narrowband tuning yet.
static const float kExtendedMu = 0.4f;
static const float kExtendedErrorThreshold = 1.0e-6f;

static const int16_t kNoiseEstIncCount = 5;

typedef struct PowerLevel {
  float sfrsum;
  int sfrcounter;
  float framelevel;
  float frsum;
  int frcounter;
  float minlevel;
  float averagelevel;
} PowerLevel;

typedef struct {
    short       sLogEnergyMax;
    short       sLogEnergyMin;
    short       sLogEnergyCur;
    short       sVADCount;
    short       sNoiseCount;
    short       sLogEnergySm;
}TimeVad_t;

struct AecCore {
  int farBufWritePos, farBufReadPos;

  int knownDelay;
  int inSamples, outSamples;
  int delayEstCtr;

  RingBuffer* nearFrBuf;
  RingBuffer* outFrBuf;

  RingBuffer* nearFrBufH[NUM_HIGH_BANDS_MAX];
  RingBuffer* outFrBufH[NUM_HIGH_BANDS_MAX];

  float dBuf[PART_LEN2];  // nearend
  float eBuf[PART_LEN2];  // error

  float dBufH[NUM_HIGH_BANDS_MAX][PART_LEN2];  // nearend

  float xPow[PART_LEN1];
  float dPow[PART_LEN1];
  float dMinPow[PART_LEN1];
#ifndef WEBRTC_AEC_NEW_CNG
  float dInitMinPow[PART_LEN1];
#endif
  float* noisePow;
#ifdef WEBRTC_AEC_NEW_CNG
  int noiseEstTooLowCtr[PART_LEN1];
  float noiseEst_save[8][PART_LEN1];
  float dMinPow_out[PART_LEN1];
  float NoEchoCount;
#endif
  float xfBuf[2][kExtendedNumPartitions * PART_LEN1];  // farend fft buffer
  float wfBuf[2][kExtendedNumPartitions * PART_LEN1];  // filter fft
  complex_t sde[PART_LEN1];  // cross-psd of nearend and error
  complex_t sxd[PART_LEN1];  // cross-psd of farend and nearend
  // Farend windowed fft buffer.
  complex_t xfwBuf[kExtendedNumPartitions * PART_LEN1];

  float sx[PART_LEN1], sd[PART_LEN1], se[PART_LEN1];  // far, near, error psd
  float hNs[PART_LEN1];
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
  float hNlFbMin[kSubBandNum], hNlFbLocalMin[kSubBandNum];
  float hNlXdAvgMin[kSubBandNum];
  int hNlNewMin[kSubBandNum], hNlMinCtr[kSubBandNum];
  float overDrive[kSubBandNum], overDriveSm[kSubBandNum];
#else
  float hNlFbMin, hNlFbLocalMin;
  float hNlXdAvgMin;
  int hNlNewMin, hNlMinCtr;
  float overDrive, overDriveSm;
#endif
  int nlp_mode;
  float outBuf[PART_LEN];
  int delayIdx;

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
  float stNearState[kSubBandNum], echoStateSub[kSubBandNum];
  short echoState;
#else
  short stNearState, echoState;
#endif
  short divergeState;

  int xfBufBlockPos;

  RingBuffer* far_buf;
  RingBuffer* far_buf_windowed;
  RingBuffer* media_buf;
  int system_delay;  // Current system delay buffered in AEC.

  int mult;  // sampling frequency multiple
  int sampFreq;
  size_t num_bands;
  uint32_t seed;

  float normal_mu;               // stepsize
  float normal_error_threshold;  // error threshold

  int noiseEstCtr;

  PowerLevel farlevel;
  PowerLevel nearlevel;
  PowerLevel linoutlevel;
  PowerLevel nlpoutlevel;

  int metricsMode;
  int stateCounter;
  Stats erl;
  Stats erle;
  Stats aNlp;
  Stats rerl;

  // Quantities to control H band scaling for SWB input
  int freq_avg_ic;       // initial bin for averaging nlp gain
  int flag_Hband_cn;     // for comfort noise
  float cn_scale_Hband;  // scale for comfort noise in H band

  int delay_metrics_delivered;
  int delay_histogram[kHistorySizeBlocks];
  int num_delay_values;
  int delay_median;
  int delay_std;
  float fraction_poor_delays;
  int delay_logging_enabled;
  void* delay_estimator_farend;
  void* delay_estimator;
  void* delay_estimator_media;
  int delay_tmp;
  uint64_t delay_start_time_ms;

  // Variables associated with delay correction through signal based delay
  // estimation feedback.
  int signal_delay_correction;
  int previous_delay;
  int delay_correction_count;
  int shift_offset;
  float delay_quality_threshold;
  int frame_count;

  // 0 = delay agnostic mode (signal based delay correction) disabled.
  // Otherwise enabled.
  int delay_agnostic_enabled;
  // 1 = extended filter mode enabled, 0 = disabled.
  int extended_filter_enabled;
  // Runtime selection of number of filter partitions.
  int num_partitions;

  RingBuffer* far_time_buf;
  RingBuffer* media_time_buf;
#ifdef WEBRTC_AEC_DEBUG_DUMP
  // Sequence number of this AEC instance, so that different instances can
  // choose different dump file names.
  int instance_index;

  // Number of times we've restarted dumping; used to pick new dump file names
  // each time.
  int debug_dump_count;

  rtc_WavWriter* farFile;
  rtc_WavWriter* nearFile;
  rtc_WavWriter* outFile;
  rtc_WavWriter* outLinearFile;
  FILE* e_fft_file;
#endif
  RingBuffer* far_buf_save;
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
  FILE *time_debug_file;

  int buffer_far_count;
  int aec_count;
  int delay_est;
  int media_delay_est;
  int delay_est_old;
  int his_read_offset;
  int has_modified_readpos;

  int filter_reseted;
  int dt_flag;
  int dt_flag_old;

  float fLowHnlSm;
  float fHnlVoiceAvg;
  float fHighHnlSm;

  uint32_t      uiFrameCount;
  uint32_t      uiMediaCount;
  TimeVad_t           time_VadS;
  TimeVad_t           time_VadR;
  TimeVad_t           time_VadMedia;    

  float afERLSm[PART_LEN1];
  float afDfPSD[PART_LEN1];
  float afXfPSD[PART_LEN1];
  float afYfPSD[PART_LEN1];
  float afEfPSD[PART_LEN1];
  float afYSaveBuf[PART_LEN];
  float afYfwBuf[2][PART_LEN1];
  int iVadRCount;
  int iNoiseRCount;
  int iNoiseRCountOld;
  int iBeginOfEcho;
  short asVadR[kHistorySizeBlocks];
  short asVadMedia[kHistorySizeBlocks];
  int FarEndBufferLength;
  int old_write_pos;
  float FarPlayOutDelta;
  float FarPlayOutWaitCnt;
  float DeltaIsTooHigh;

  //debug
  float d_save[PART_LEN];
  float hNlErlAvg;
#endif

#ifdef DEBUG_RECORD
  int16_t aec_ref_data[kHistorySizeBlocks * PART_LEN2];
  int16_t aec_ref_data_tmp[kHistorySizeBlocks * PART_LEN2];
  int aec_ref_write_pos;
  int aec_ref_read_pos;
  int16_t aec_sin_data[kHistorySizeBlocks * PART_LEN2];
  int16_t aec_sin_data_tmp[kHistorySizeBlocks * PART_LEN2];
  int aec_sin_write_pos;
  int aec_sin_read_pos;
  int16_t aec_offset_data[kHistorySizeBlocks * PART_LEN2 * 2];
  int16_t aec_offset_data_tmp[kHistorySizeBlocks * PART_LEN2 * 2];
  int aec_offset_write_pos;
  int aec_offset_read_pos;
  int file_opened_flag;
#endif

};

typedef void (*WebRtcAecFilterFar)(AecCore* aec, float yf[2][PART_LEN1]);
extern WebRtcAecFilterFar WebRtcAec_FilterFar;
typedef void (*WebRtcAecScaleErrorSignal)(AecCore* aec, float ef[2][PART_LEN1]);
extern WebRtcAecScaleErrorSignal WebRtcAec_ScaleErrorSignal;
typedef void (*WebRtcAecFilterAdaptation)(AecCore* aec,
                                          float* fft,
                                          float ef[2][PART_LEN1]);
extern WebRtcAecFilterAdaptation WebRtcAec_FilterAdaptation;
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
typedef void (*WebRtcAecOverdriveAndSuppress)(AecCore* aec,
                                              float hNl[PART_LEN1],
                                              float hNlFb[kSubBandNum],
                                              float efw[2][PART_LEN1]);
#else
typedef void (*WebRtcAecOverdriveAndSuppress)(AecCore* aec,
                                              float hNl[PART_LEN1],
                                              const float hNlFb,
                                              float efw[2][PART_LEN1]);
#endif
extern WebRtcAecOverdriveAndSuppress WebRtcAec_OverdriveAndSuppress;

typedef void (*WebRtcAecComfortNoise)(AecCore* aec,
                                      float efw[2][PART_LEN1],
                                      complex_t* comfortNoiseHband,
                                      const float* noisePow,
                                      const float* lambda);
extern WebRtcAecComfortNoise WebRtcAec_ComfortNoise;

typedef void (*WebRtcAecSubBandCoherence)(AecCore* aec,
                                          float efw[2][PART_LEN1],
                                          float xfw[2][PART_LEN1],
                                          float* fft,
                                          float* cohde,
                                          float* cohxd);
extern WebRtcAecSubBandCoherence WebRtcAec_SubbandCoherence;

short WebRtcAec_TimeVAD(TimeVad_t *inst, float *pw16_outData, short pw16_len, uint32_t uiFrameCount);

short WebRtcAec_DTDetection(AecCore* aec, float hNl[PART_LEN1]);

static void WebRtcAec_ERL_Estimation(AecCore* aec, float hNl[PART_LEN1], float efw[2][PART_LEN1]);

static void WebRtcAec_ERL_EstHnl (AecCore* aec, float hNl[PART_LEN1]);

void WebRtcAec_BeginOfEchoDetection (AecCore* aec);

static void PSDSm_For_ERL_Estimation(AecCore* aec, float efw[2][PART_LEN1], float dfw[2][PART_LEN1], float xfw[2][PART_LEN1]);

static void EstEchoPathDelay(AecCore* aec);

}  // namespace webrtc_new
} //namespace youme

#endif  // WEBRTC_MODULES_AUDIO_PROCESSING_AEC_AEC_CORE_INTERNAL_NEW_H_

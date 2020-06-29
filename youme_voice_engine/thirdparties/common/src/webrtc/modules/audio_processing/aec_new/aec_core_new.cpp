/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/*
 * The core AEC algorithm, which is presented with time-aligned signals.
 */

#include "webrtc/modules/audio_processing/aec_new/aec_core_new.h"

#include "tsk_debug.h"
#include <YouMeCommon/TimeUtil.h>

#include <stdio.h>
#include <stdarg.h>

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>  // size_t
#include <stdlib.h>
#include <string.h>

#include "webrtc/common_audio/ring_buffer.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_processing/aec_new/aec_common_new.h"
#include "webrtc/modules/audio_processing/aec_new/aec_core_internal_new.h"
#include "webrtc/modules/audio_processing/aec_new/aec_rdft_new.h"
#include "webrtc/modules/audio_processing/logging/aec_logging.h"
#include "webrtc/modules/audio_processing/utility/webrtc_new/delay_estimator_wrapper_new.h"
#include "webrtc/system_wrappers/include/cpu_features_wrapper.h"
#include "webrtc/typedefs.h"

#ifdef DEBUG_RECORD
#include <time.h>
#ifndef __APPLE__
#else
#include <sys/time.h>
#endif


static FILE * farFile;
static FILE * noisy_nearFile;
static FILE * delay_nearFile;
static FILE * offset_nearFile;
#if defined(_WIN32)
#define snprintf _snprintf
#endif
#endif

namespace youme{
namespace webrtc_new {
        
// Buffer size (samples)
static const size_t kBufSizePartitions = 250;  // 1 second of audio in 16 kHz.

// Metrics
static const int subCountLen = 4;
static const int countLen = 50;
static const int kDelayMetricsAggregationWindow = 1250;  // 5 seconds at 16 kHz.

// Quantities to control H band scaling for SWB input
static const int flagHbandCn = 1;  // flag for adding comfort noise in H band
static const float cnScaleHband =
    (float)0.4;  // scale for comfort noise in H band
// Initial bin for averaging nlp gain in low band
static const int freqAvgIc = PART_LEN / 2;

// Matlab code to produce table:
// win = sqrt(hanning(63)); win = [0 ; win(1:32)];
// fprintf(1, '\t%.14f, %.14f, %.14f,\n', win);
ALIGN16_BEG const float ALIGN16_END WebRtcAec_sqrtHanning[65] = {
    0.00000000000000f, 0.02454122852291f, 0.04906767432742f, 0.07356456359967f,
    0.09801714032956f, 0.12241067519922f, 0.14673047445536f, 0.17096188876030f,
    0.19509032201613f, 0.21910124015687f, 0.24298017990326f, 0.26671275747490f,
    0.29028467725446f, 0.31368174039889f, 0.33688985339222f, 0.35989503653499f,
    0.38268343236509f, 0.40524131400499f, 0.42755509343028f, 0.44961132965461f,
    0.47139673682600f, 0.49289819222978f, 0.51410274419322f, 0.53499761988710f,
    0.55557023301960f, 0.57580819141785f, 0.59569930449243f, 0.61523159058063f,
    0.63439328416365f, 0.65317284295378f, 0.67155895484702f, 0.68954054473707f,
    0.70710678118655f, 0.72424708295147f, 0.74095112535496f, 0.75720884650648f,
    0.77301045336274f, 0.78834642762661f, 0.80320753148064f, 0.81758481315158f,
    0.83146961230255f, 0.84485356524971f, 0.85772861000027f, 0.87008699110871f,
    0.88192126434835f, 0.89322430119552f, 0.90398929312344f, 0.91420975570353f,
    0.92387953251129f, 0.93299279883474f, 0.94154406518302f, 0.94952818059304f,
    0.95694033573221f, 0.96377606579544f, 0.97003125319454f, 0.97570213003853f,
    0.98078528040323f, 0.98527764238894f, 0.98917650996478f, 0.99247953459871f,
    0.99518472667220f, 0.99729045667869f, 0.99879545620517f, 0.99969881869620f,
    1.00000000000000f
};

// Matlab code to produce table:
// weightCurve = [0 ; 0.3 * sqrt(linspace(0,1,64))' + 0.1];
// fprintf(1, '\t%.4f, %.4f, %.4f, %.4f, %.4f, %.4f,\n', weightCurve);
ALIGN16_BEG const float ALIGN16_END WebRtcAec_weightCurve[65] = {
    0.0000f, 0.1000f, 0.1378f, 0.1535f, 0.1655f, 0.1756f, 0.1845f, 0.1926f,
    0.2000f, 0.2069f, 0.2134f, 0.2195f, 0.2254f, 0.2309f, 0.2363f, 0.2414f,
    0.2464f, 0.2512f, 0.2558f, 0.2604f, 0.2648f, 0.2690f, 0.2732f, 0.2773f,
    0.2813f, 0.2852f, 0.2890f, 0.2927f, 0.2964f, 0.3000f, 0.3035f, 0.3070f,
    0.3104f, 0.3138f, 0.3171f, 0.3204f, 0.3236f, 0.3268f, 0.3299f, 0.3330f,
    0.3360f, 0.3390f, 0.3420f, 0.3449f, 0.3478f, 0.3507f, 0.3535f, 0.3563f,
    0.3591f, 0.3619f, 0.3646f, 0.3673f, 0.3699f, 0.3726f, 0.3752f, 0.3777f,
    0.3803f, 0.3828f, 0.3854f, 0.3878f, 0.3903f, 0.3928f, 0.3952f, 0.3976f,
    0.4000f
};

// Matlab code to produce table:
// overDriveCurve = [sqrt(linspace(0,1,65))' + 1];
// fprintf(1, '\t%.4f, %.4f, %.4f, %.4f, %.4f, %.4f,\n', overDriveCurve);
ALIGN16_BEG const float ALIGN16_END WebRtcAec_overDriveCurve[65] = {
    1.0000f, 1.1250f, 1.1768f, 1.2165f, 1.2500f, 1.2795f, 1.3062f, 1.3307f,
    1.3536f, 1.3750f, 1.3953f, 1.4146f, 1.4330f, 1.4507f, 1.4677f, 1.4841f,
    1.5000f, 1.5154f, 1.5303f, 1.5449f, 1.5590f, 1.5728f, 1.5863f, 1.5995f,
    1.6124f, 1.6250f, 1.6374f, 1.6495f, 1.6614f, 1.6731f, 1.6847f, 1.6960f,
    1.7071f, 1.7181f, 1.7289f, 1.7395f, 1.7500f, 1.7603f, 1.7706f, 1.7806f,
    1.7906f, 1.8004f, 1.8101f, 1.8197f, 1.8292f, 1.8385f, 1.8478f, 1.8570f,
    1.8660f, 1.8750f, 1.8839f, 1.8927f, 1.9014f, 1.9100f, 1.9186f, 1.9270f,
    1.9354f, 1.9437f, 1.9520f, 1.9601f, 1.9682f, 1.9763f, 1.9843f, 1.9922f,
    2.0000f
};

// Delay Agnostic AEC parameters, still under development and may change.
static const float kDelayQualityThresholdMax = 0.07f;
static const float kDelayQualityThresholdMin = 0.01f;
static const int kInitialShiftOffset = 5;
#if !defined(RTC_ANDROID)
static const int kDelayCorrectionStart = 1500;  // 10 ms chunks
#endif

// Target suppression levels for nlp modes.
// log{0.001, 0.00001, 0.00000001}
static const float kTargetSupp[3] = { -6.9f, -11.5f, -18.4f};

// Two sets of parameters, one for the extended filter mode.
static const float kExtendedMinOverDrive[3] = {3.0f, 6.0f, 15.0f};
static const float kNormalMinOverDrive[3] = {1.0f, 2.0f, 5.0f};
const float WebRtcAec_kExtendedSmoothingCoefficients[2][2] = {{0.9f, 0.1f},
    {0.92f, 0.08f}
};
const float WebRtcAec_kNormalSmoothingCoefficients[2][2] = {{0.9f, 0.1f},
    {0.93f, 0.07f}
};

// Number of partitions forming the NLP's "preferred" bands.
enum {
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    kPrefBandSize = 60
#else
    kPrefBandSize = 24
#endif
};

#ifdef WEBRTC_AEC_DEBUG_DUMP
extern int webrtc_aec_instance_count;
#endif

#ifdef DEBUG_RECORD
static int64_t GetTimeInMS()
{
#if defined(_WIN32)
    //return (int64_t)timeGetTime();
    return (int64_t)0;
#else
    int64_t val = 0;
    struct timespec tEnd;
#ifndef __APPLE__
    clock_gettime(CLOCK_REALTIME, &tEnd);
#else
    struct timeval tVal;
    struct timezone tZone;
    tZone.tz_minuteswest = 0;
    tZone.tz_dsttime = 0;
    gettimeofday(&tVal,&tZone);
    TIMEVAL_TO_TIMESPEC(&tVal,&tEnd);
#endif
    val = (int64_t)tEnd.tv_sec * 1000 + tEnd.tv_nsec / 1000000;
    return val;
#endif

}
#endif
        
WebRtcAecFilterFar WebRtcAec_FilterFar;
WebRtcAecScaleErrorSignal WebRtcAec_ScaleErrorSignal;
WebRtcAecFilterAdaptation WebRtcAec_FilterAdaptation;
WebRtcAecOverdriveAndSuppress WebRtcAec_OverdriveAndSuppress;
WebRtcAecComfortNoise WebRtcAec_ComfortNoise;
WebRtcAecSubBandCoherence WebRtcAec_SubbandCoherence;

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

static void FilterFar(AecCore* aec, float yf[2][PART_LEN1]) {
    int i;
    for (i = 0; i < aec->num_partitions; i++) {
        int j;
        int xPos = (i + aec->xfBufBlockPos) * PART_LEN1;
        int pos = i * PART_LEN1;
        // Check for wrap
        if (i + aec->xfBufBlockPos >= aec->num_partitions) {
            xPos -= aec->num_partitions * (PART_LEN1);
        }

        for (j = 0; j < PART_LEN1; j++) {
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

static void ScaleErrorSignal(AecCore* aec, float ef[2][PART_LEN1]) {
    const float mu = aec->extended_filter_enabled ? kExtendedMu : aec->normal_mu;
    const float error_threshold = aec->extended_filter_enabled
                                  ? kExtendedErrorThreshold
                                  : aec->normal_error_threshold;
    int i;
    float abs_ef;

    for (i = 0; i < (PART_LEN1); i++) {
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

// Time-unconstrined filter adaptation.
// TODO(andrew): consider for a low-complexity mode.
// static void FilterAdaptationUnconstrained(AecCore* aec, float *fft,
//                                          float ef[2][PART_LEN1]) {
//  int i, j;
//  for (i = 0; i < aec->num_partitions; i++) {
//    int xPos = (i + aec->xfBufBlockPos)*(PART_LEN1);
//    int pos;
//    // Check for wrap
//    if (i + aec->xfBufBlockPos >= aec->num_partitions) {
//      xPos -= aec->num_partitions * PART_LEN1;
//    }
//
//    pos = i * PART_LEN1;
//
//    for (j = 0; j < PART_LEN1; j++) {
//      aec->wfBuf[0][pos + j] += MulRe(aec->xfBuf[0][xPos + j],
//                                      -aec->xfBuf[1][xPos + j],
//                                      ef[0][j], ef[1][j]);
//      aec->wfBuf[1][pos + j] += MulIm(aec->xfBuf[0][xPos + j],
//                                      -aec->xfBuf[1][xPos + j],
//                                      ef[0][j], ef[1][j]);
//    }
//  }
//}

static void FilterAdaptation(AecCore* aec, float* fft, float ef[2][PART_LEN1]) {
    int i, j;
    for (i = 0; i < aec->num_partitions; i++) {
        int xPos = (i + aec->xfBufBlockPos) * (PART_LEN1);
        int pos;
        // Check for wrap
        if (i + aec->xfBufBlockPos >= aec->num_partitions) {
            xPos -= aec->num_partitions * PART_LEN1;
        }

        pos = i * PART_LEN1;

        for (j = 0; j < PART_LEN; j++) {

            fft[2 * j] = MulRe(aec->xfBuf[0][xPos + j],
                               -aec->xfBuf[1][xPos + j],
                               ef[0][j],
                               ef[1][j]);
            fft[2 * j + 1] = MulIm(aec->xfBuf[0][xPos + j],
                                   -aec->xfBuf[1][xPos + j],
                                   ef[0][j],
                                   ef[1][j]);
        }
        fft[1] = MulRe(aec->xfBuf[0][xPos + PART_LEN],
                       -aec->xfBuf[1][xPos + PART_LEN],
                       ef[0][PART_LEN],
                       ef[1][PART_LEN]);

        aec_rdft_inverse_128(fft);
        memset(fft + PART_LEN, 0, sizeof(float) * PART_LEN);

        // fft scaling
        {
            float scale = 2.0f / PART_LEN2;
            for (j = 0; j < PART_LEN; j++) {
                fft[j] *= scale;
            }
        }
        aec_rdft_forward_128(fft);

        aec->wfBuf[0][pos] += fft[0];
        aec->wfBuf[0][pos + PART_LEN] += fft[1];

        for (j = 1; j < PART_LEN; j++) {
            aec->wfBuf[0][pos + j] += fft[2 * j];
            aec->wfBuf[1][pos + j] += fft[2 * j + 1];
        }
    }
}
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
static void OverdriveAndSuppress(AecCore* aec,
                                 float hNl[PART_LEN1],
                                 float hNlFb[kSubBandNum],
                                 float efw[2][PART_LEN1]) {

    short asSubBand[kSubBandNum + 1] = {0, 5, 9, 13, 17, 25, 37, 49, 65};
    int i, sSBNum, sStartNum, sEndNum, iTmp;
    float afHnlAvg[kSubBandNum] = {0};
    float fHnlVoiceAvg = 0, fTmp = 0, fHnl1000UpAvg = 0, fHnl500Avg = 0;
    //int iHzPerNum = 4000 * aec->mult / PART_LEN;
    int iNum4500Hz = 0, iNum1000Hz = 0, iNum4000Hz = 0;
    float hNlTmp[PART_LEN1], fHnl1000DownAvg;

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
        for (i = asSubBand[iTmp]; i < PART_LEN1; i++)
        {
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
static void OverdriveAndSuppress(AecCore* aec,
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
        for (j = 0; j < PART_LEN1; j++) {
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

// Threshold to protect against the ill-effects of a zero far-end.
const float WebRtcAec_kMinFarendPSD = 15;

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

    for (i = 0; i < PART_LEN1; i++) {

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

// Window time domain data to be used by the fft.
__inline static void WindowData(float* x_windowed, const float* x) {
    int i;
    for (i = 0; i < PART_LEN; i++) {
        x_windowed[i] = x[i] * WebRtcAec_sqrtHanning[i];
        x_windowed[PART_LEN + i] =
            x[PART_LEN + i] * WebRtcAec_sqrtHanning[PART_LEN - i];
    }
}

// Puts fft output data into a complex valued array.
__inline static void StoreAsComplex(const float* data,
                                    float data_complex[2][PART_LEN1]) {
    int i;
    data_complex[0][0] = data[0];
    data_complex[1][0] = 0;
    for (i = 1; i < PART_LEN; i++) {
        data_complex[0][i] = data[2 * i];
        data_complex[1][i] = data[2 * i + 1];
    }
    data_complex[0][PART_LEN] = data[1];
    data_complex[1][PART_LEN] = 0;
}

static void SubbandCoherence(AecCore* aec,
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

    // Subband coherence
    for (i = 0; i < PART_LEN1; i++) {
        cohde[i] =
            (aec->sde[i][0] * aec->sde[i][0] + aec->sde[i][1] * aec->sde[i][1]) /
            (aec->sd[i] * aec->se[i] + 1e-10f);
        cohxd[i] =
            (aec->sxd[i][0] * aec->sxd[i][0] + aec->sxd[i][1] * aec->sxd[i][1]) /
            (aec->sx[i] * aec->sd[i] + 1e-10f);
    }
}

static void GetHighbandGain(const float* lambda, float* nlpGainHband) {
    int i;

    nlpGainHband[0] = (float)0.0;
    for (i = freqAvgIc; i < PART_LEN1 - 1; i++) {
        nlpGainHband[0] += lambda[i];
    }
    nlpGainHband[0] /= (float)(PART_LEN1 - 1 - freqAvgIc);
}

static void ComfortNoise(AecCore* aec,
                         float efw[2][PART_LEN1],
                         complex_t* comfortNoiseHband,
                         const float* noisePow,
                         const float* lambda) {
    int i, num;
    float rand[PART_LEN];
    float noise, noiseAvg, tmp, tmpAvg;
    int16_t randW16[PART_LEN];
    complex_t u[PART_LEN1];

    const float pi2 = 6.28318530717959f;

    // Generate a uniform random array on [0 1]
    YoumeWebRtcSpl_RandUArray(randW16, PART_LEN, &aec->seed);
    for (i = 0; i < PART_LEN; i++) {
        rand[i] = ((float)randW16[i]) / 32768;
    }

    // Reject LF noise
    u[0][0] = 0;
    u[0][1] = 0;
    for (i = 1; i < PART_LEN1; i++) {
        tmp = pi2 * rand[i - 1];

        noise = sqrtf(noisePow[i]);
        u[i][0] = noise * cosf(tmp);
        u[i][1] = -noise * sinf(tmp);
    }
    u[PART_LEN][1] = 0;

    for (i = 0; i < PART_LEN1; i++) {
        // This is the proper weighting to match the background noise power
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
    if (aec->num_bands > 1 && flagHbandCn == 1) {

        // average noise scale
        // average over second half of freq spectrum (i.e., 4->8khz)
        // TODO: we shouldn't need num. We know how many elements we're summing.
        for (i = PART_LEN1 >> 1; i < PART_LEN1; i++) {
            num++;
            noiseAvg += sqrtf(noisePow[i]);
        }
        noiseAvg /= (float)num;

        // average nlp scale
        // average over second half of freq spectrum (i.e., 4->8khz)
        // TODO: we shouldn't need num. We know how many elements we're summing.
        num = 0;
        for (i = PART_LEN1 >> 1; i < PART_LEN1; i++) {
            num++;
            tmpAvg += sqrtf(WEBRTC_SPL_MAX(1 - lambda[i] * lambda[i], 0));
        }
        tmpAvg /= (float)num;

        // Use average noise for H band
        // TODO: we should probably have a new random vector here.
        // Reject LF noise
        u[0][0] = 0;
        u[0][1] = 0;
        for (i = 1; i < PART_LEN1; i++) {
            tmp = pi2 * rand[i - 1];

            // Use average noise for H band
            u[i][0] = noiseAvg * (float)cos(tmp);
            u[i][1] = -noiseAvg * (float)sin(tmp);
        }
        u[PART_LEN][1] = 0;

        for (i = 0; i < PART_LEN1; i++) {
            // Use average NLP weight for H band
            comfortNoiseHband[i][0] = tmpAvg * u[i][0];
            comfortNoiseHband[i][1] = tmpAvg * u[i][1];
        }
    }
}

static void InitLevel(PowerLevel* level) {
    const float kBigFloat = 1E17f;

    level->averagelevel = 0;
    level->framelevel = 0;
    level->minlevel = kBigFloat;
    level->frsum = 0;
    level->sfrsum = 0;
    level->frcounter = 0;
    level->sfrcounter = 0;
}

static void InitStats(Stats* stats) {
    stats->instant = kOffsetLevel;
    stats->average = kOffsetLevel;
    stats->max = kOffsetLevel;
    stats->min = kOffsetLevel * (-1);
    stats->sum = 0;
    stats->hisum = 0;
    stats->himean = kOffsetLevel;
    stats->counter = 0;
    stats->hicounter = 0;
}

static void InitMetrics(AecCore* self) {
    self->stateCounter = 0;
    InitLevel(&self->farlevel);
    InitLevel(&self->nearlevel);
    InitLevel(&self->linoutlevel);
    InitLevel(&self->nlpoutlevel);

    InitStats(&self->erl);
    InitStats(&self->erle);
    InitStats(&self->aNlp);
    InitStats(&self->rerl);
}

static void UpdateLevel(PowerLevel* level, float in[2][PART_LEN1]) {
    // Do the energy calculation in the frequency domain. The FFT is performed on
    // a segment of PART_LEN2 samples due to overlap, but we only want the energy
    // of half that data (the last PART_LEN samples). Parseval's relation states
    // that the energy is preserved according to
    //
    // \sum_{n=0}^{N-1} |x(n)|^2 = 1/N * \sum_{n=0}^{N-1} |X(n)|^2
    //                           = ENERGY,
    //
    // where N = PART_LEN2. Since we are only interested in calculating the energy
    // for the last PART_LEN samples we approximate by calculating ENERGY and
    // divide by 2,
    //
    // \sum_{n=N/2}^{N-1} |x(n)|^2 ~= ENERGY / 2
    //
    // Since we deal with real valued time domain signals we only store frequency
    // bins [0, PART_LEN], which is what |in| consists of. To calculate ENERGY we
    // need to add the contribution from the missing part in
    // [PART_LEN+1, PART_LEN2-1]. These values are, up to a phase shift, identical
    // with the values in [1, PART_LEN-1], hence multiply those values by 2. This
    // is the values in the for loop below, but multiplication by 2 and division
    // by 2 cancel.

    // TODO(bjornv): Investigate reusing energy calculations performed at other
    // places in the code.
    int k = 1;
    // Imaginary parts are zero at end points and left out of the calculation.
    float energy = (in[0][0] * in[0][0]) / 2;
    energy += (in[0][PART_LEN] * in[0][PART_LEN]) / 2;

    for (k = 1; k < PART_LEN; k++) {
        energy += (in[0][k] * in[0][k] + in[1][k] * in[1][k]);
    }
    energy /= PART_LEN2;

    level->sfrsum += energy;
    level->sfrcounter++;

    if (level->sfrcounter > subCountLen) {
        level->framelevel = level->sfrsum / (subCountLen * PART_LEN);
        level->sfrsum = 0;
        level->sfrcounter = 0;
        if (level->framelevel > 0) {
            if (level->framelevel < level->minlevel) {
                level->minlevel = level->framelevel;  // New minimum.
            } else {
                level->minlevel *= (1 + 0.001f);  // Small increase.
            }
        }
        level->frcounter++;
        level->frsum += level->framelevel;
        if (level->frcounter > countLen) {
            level->averagelevel = level->frsum / countLen;
            level->frsum = 0;
            level->frcounter = 0;
        }
    }
}

static void UpdateMetrics(AecCore* aec) {
    float dtmp, dtmp2;

    const float actThresholdNoisy = 8.0f;
    const float actThresholdClean = 40.0f;
    const float safety = 0.99995f;
    const float noisyPower = 300000.0f;

    float actThreshold;
    float echo, suppressedEcho;

    if (aec->echoState) {  // Check if echo is likely present
        aec->stateCounter++;
    }

    if (aec->farlevel.frcounter == 0) {

        if (aec->farlevel.minlevel < noisyPower) {
            actThreshold = actThresholdClean;
        } else {
            actThreshold = actThresholdNoisy;
        }

        if ((aec->stateCounter > (0.5f * countLen * subCountLen)) &&
                (aec->farlevel.sfrcounter == 0)

                // Estimate in active far-end segments only
                &&
                (aec->farlevel.averagelevel >
                 (actThreshold * aec->farlevel.minlevel))) {

            // Subtract noise power
            echo = aec->nearlevel.averagelevel - safety * aec->nearlevel.minlevel;

            // ERL
            dtmp = 10 * (float)log10(aec->farlevel.averagelevel /
                                     aec->nearlevel.averagelevel +
                                     1e-10f);
            dtmp2 = 10 * (float)log10(aec->farlevel.averagelevel / echo + 1e-10f);

            aec->erl.instant = dtmp;
            if (dtmp > aec->erl.max) {
                aec->erl.max = dtmp;
            }

            if (dtmp < aec->erl.min) {
                aec->erl.min = dtmp;
            }

            aec->erl.counter++;
            aec->erl.sum += dtmp;
            aec->erl.average = aec->erl.sum / aec->erl.counter;

            // Upper mean
            if (dtmp > aec->erl.average) {
                aec->erl.hicounter++;
                aec->erl.hisum += dtmp;
                aec->erl.himean = aec->erl.hisum / aec->erl.hicounter;
            }

            // A_NLP
            dtmp = 10 * (float)log10(aec->nearlevel.averagelevel /
                                     (2 * aec->linoutlevel.averagelevel) +
                                     1e-10f);

            // subtract noise power
            suppressedEcho = 2 * (aec->linoutlevel.averagelevel -
                                  safety * aec->linoutlevel.minlevel);

            dtmp2 = 10 * (float)log10(echo / suppressedEcho + 1e-10f);

            aec->aNlp.instant = dtmp2;
            if (dtmp > aec->aNlp.max) {
                aec->aNlp.max = dtmp;
            }

            if (dtmp < aec->aNlp.min) {
                aec->aNlp.min = dtmp;
            }

            aec->aNlp.counter++;
            aec->aNlp.sum += dtmp;
            aec->aNlp.average = aec->aNlp.sum / aec->aNlp.counter;

            // Upper mean
            if (dtmp > aec->aNlp.average) {
                aec->aNlp.hicounter++;
                aec->aNlp.hisum += dtmp;
                aec->aNlp.himean = aec->aNlp.hisum / aec->aNlp.hicounter;
            }

            // ERLE

            // subtract noise power
            suppressedEcho = 2 * (aec->nlpoutlevel.averagelevel -
                                  safety * aec->nlpoutlevel.minlevel);

            dtmp = 10 * (float)log10(aec->nearlevel.averagelevel /
                                     (2 * aec->nlpoutlevel.averagelevel) +
                                     1e-10f);
            dtmp2 = 10 * (float)log10(echo / suppressedEcho + 1e-10f);

            dtmp = dtmp2;
            aec->erle.instant = dtmp;
            if (dtmp > aec->erle.max) {
                aec->erle.max = dtmp;
            }

            if (dtmp < aec->erle.min) {
                aec->erle.min = dtmp;
            }

            aec->erle.counter++;
            aec->erle.sum += dtmp;
            aec->erle.average = aec->erle.sum / aec->erle.counter;

            // Upper mean
            if (dtmp > aec->erle.average) {
                aec->erle.hicounter++;
                aec->erle.hisum += dtmp;
                aec->erle.himean = aec->erle.hisum / aec->erle.hicounter;
            }
        }

        aec->stateCounter = 0;
    }
}

static void UpdateDelayMetrics(AecCore* self) {
    int i = 0;
    int delay_values = 0;
    int median = 0;
    int lookahead = WebRtc_lookahead(self->delay_estimator);
    const int kMsPerBlock = PART_LEN / (self->mult * 8);
    int64_t l1_norm = 0;

    if (self->num_delay_values == 0) {
        // We have no new delay value data. Even though -1 is a valid |median| in
        // the sense that we allow negative values, it will practically never be
        // used since multiples of |kMsPerBlock| will always be returned.
        // We therefore use -1 to indicate in the logs that the delay estimator was
        // not able to estimate the delay.
        self->delay_median = -1;
        self->delay_std = -1;
        self->fraction_poor_delays = -1;
        return;
    }

    // Start value for median count down.
    delay_values = self->num_delay_values >> 1;
    // Get median of delay values since last update.
    for (i = 0; i < kHistorySizeBlocks; i++) {
        delay_values -= self->delay_histogram[i];
        if (delay_values < 0) {
            median = i;
            break;
        }
    }
    // Account for lookahead.
    self->delay_median = (median - lookahead) * kMsPerBlock;

    // Calculate the L1 norm, with median value as central moment.
    for (i = 0; i < kHistorySizeBlocks; i++) {
        l1_norm += abs(i - median) * self->delay_histogram[i];
    }
    self->delay_std = (int)((l1_norm + self->num_delay_values / 2) /
                            self->num_delay_values) * kMsPerBlock;

    // Determine fraction of delays that are out of bounds, that is, either
    // negative (anti-causal system) or larger than the AEC filter length.
    {
        int num_delays_out_of_bounds = self->num_delay_values;
        const int histogram_length = sizeof(self->delay_histogram) /
                                     sizeof(self->delay_histogram[0]);
        for (i = lookahead; i < lookahead + self->num_partitions; ++i) {
            if (i < histogram_length)
                num_delays_out_of_bounds -= self->delay_histogram[i];
        }
        self->fraction_poor_delays = (float)num_delays_out_of_bounds /
                                     self->num_delay_values;
    }

    // Reset histogram.
    memset(self->delay_histogram, 0, sizeof(self->delay_histogram));
    self->num_delay_values = 0;

    return;
}

static void TimeToFrequency(float time_data[PART_LEN2],
                            float freq_data[2][PART_LEN1],
                            int window) {
    int i = 0;

    // TODO(bjornv): Should we have a different function/wrapper for windowed FFT?
    if (window) {
        for (i = 0; i < PART_LEN; i++) {
            time_data[i] *= WebRtcAec_sqrtHanning[i];
            time_data[PART_LEN + i] *= WebRtcAec_sqrtHanning[PART_LEN - i];
        }
    }

    aec_rdft_forward_128(time_data);
    // Reorder.
    freq_data[1][0] = 0;
    freq_data[1][PART_LEN] = 0;
    freq_data[0][0] = time_data[0];
    freq_data[0][PART_LEN] = time_data[1];
    for (i = 1; i < PART_LEN; i++) {
        freq_data[0][i] = time_data[2 * i];
        freq_data[1][i] = time_data[2 * i + 1];
    }
}

static int MoveFarReadPtrWithoutSystemDelayUpdate(AecCore* self, int elements) {
    int iTmp1, iTmp2;
    WebRtc_youme_MoveReadPtr(self->far_buf_windowed, elements);
    iTmp1 = WebRtc_youme_MoveReadPtr(self->far_time_buf, elements);
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    iTmp2 = WebRtc_youme_MoveReadPtr(self->far_buf_save, elements);
#endif
    WebRtc_youme_MoveReadPtr(self->media_buf, elements);
    WebRtc_youme_MoveReadPtr(self->media_time_buf, elements);
#ifdef DEBUG_RECORD
    if(delay_nearFile){
        fprintf(delay_nearFile,"\t\t\t\t\t\t\telements:%d\tfar_time_buf:%d\tfar_buf_save:%d",elements, iTmp1, iTmp2);
        if (iTmp1 == iTmp2)
        {
            fprintf(delay_nearFile,"\n");
        }
        else
        {
            fprintf(delay_nearFile,"\tERROR\n");
        }
    }
#endif 
    return WebRtc_youme_MoveReadPtr(self->far_buf, elements);   
}

static int SignalBasedDelayCorrection(AecCore* self) {
    int delay_correction = 0;
    int last_delay = -2;
    assert(self != NULL);
#if !defined(RTC_ANDROID)
    // On desktops, turn on correction after |kDelayCorrectionStart| frames.  This
    // is to let the delay estimation get a chance to converge.  Also, if the
    // playout audio volume is low (or even muted) the delay estimation can return
    // a very large delay, which will break the AEC if it is applied.
    if (self->frame_count < kDelayCorrectionStart) {
        return 0;
    }
#endif

    // 1. Check for non-negative delay estimate.  Note that the estimates we get
    //    from the delay estimation are not compensated for lookahead.  Hence, a
    //    negative |last_delay| is an invalid one.
    // 2. Verify that there is a delay change.  In addition, only allow a change
    //    if the delay is outside a certain region taking the AEC filter length
    //    into account.
    // TODO(bjornv): Investigate if we can remove the non-zero delay change check.
    // 3. Only allow delay correction if the delay estimation quality exceeds
    //    |delay_quality_threshold|.
    // 4. Finally, verify that the proposed |delay_correction| is feasible by
    //    comparing with the size of the far-end buffer.
    last_delay = WebRtc_last_delay(self->delay_estimator);
    if ((last_delay >= 0) &&
            (last_delay != self->previous_delay) &&
            (WebRtc_last_delay_quality(self->delay_estimator) >
             self->delay_quality_threshold)) {
        int delay = last_delay - WebRtc_lookahead(self->delay_estimator);
        // Allow for a slack in the actual delay, defined by a |lower_bound| and an
        // |upper_bound|.  The adaptive echo cancellation filter is currently
        // |num_partitions| (of 64 samples) long.  If the delay estimate is negative
        // or at least 3/4 of the filter length we open up for correction.
        const int lower_bound = 0;
        const int upper_bound = self->num_partitions * 3 / 4;
        const int do_correction = delay <= lower_bound || delay > upper_bound;
        if (do_correction == 1) {
            int available_read = (int)WebRtc_youme_available_read(self->far_buf);
            // With |shift_offset| we gradually rely on the delay estimates.  For
            // positive delays we reduce the correction by |shift_offset| to lower the
            // risk of pushing the AEC into a non causal state.  For negative delays
            // we rely on the values up to a rounding error, hence compensate by 1
            // element to make sure to push the delay into the causal region.
            delay_correction = -delay;
            delay_correction += delay > self->shift_offset ? self->shift_offset : 1;
            self->shift_offset--;
            self->shift_offset = (self->shift_offset <= 1 ? 1 : self->shift_offset);
            if (delay_correction > available_read - self->mult - 1) {
                // There is not enough data in the buffer to perform this shift.  Hence,
                // we do not rely on the delay estimate and do nothing.
                delay_correction = 0;
            } else {
                self->previous_delay = last_delay;
                ++self->delay_correction_count;
            }
        }
    }
    // Update the |delay_quality_threshold| once we have our first delay
    // correction.
    if (self->delay_correction_count > 0) {
        float delay_quality = WebRtc_last_delay_quality(self->delay_estimator);
        delay_quality = (delay_quality > kDelayQualityThresholdMax ?
                         kDelayQualityThresholdMax : delay_quality);
        self->delay_quality_threshold =
            (delay_quality > self->delay_quality_threshold ? delay_quality :
             self->delay_quality_threshold);
    }
    return delay_correction;
}

static void NonLinearProcessing(AecCore* aec,
                                float* output,
                                float* const* outputH) {
    float efw[2][PART_LEN1], xfw[2][PART_LEN1];
    complex_t comfortNoiseHband[PART_LEN1];
    float fft[PART_LEN2];
    float scale, dtmp;
    float nlpGainHband;
    int i;
    size_t j;

    // Coherence and non-linear filter
    float cohde[PART_LEN1], cohxd[PART_LEN1];
    float hNlDeAvg, hNlXdAvg;
    float hNl[PART_LEN1];
    float hNlPref[kPrefBandSize];
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    float hNlFb[kSubBandNum], hNlFbLow = 0;
    const float prefBandQuant = 0.75f, prefBandQuantLow = 0.5f;
    int prefBandSize = kPrefBandSize;// aec->mult;
    int minPrefBand = 4 / aec->mult;
#else
    float hNlFb = 0, hNlFbLow = 0;
    const float prefBandQuant = 0.75f, prefBandQuantLow = 0.5f;
    const int prefBandSize = kPrefBandSize;// aec->mult;
    const int minPrefBand = 4 / aec->mult;
#endif
    // Power estimate smoothing coefficients.
    const float* min_overdrive = aec->extended_filter_enabled
                                 ? kExtendedMinOverDrive
                                 : kNormalMinOverDrive;

    // Filter energy
    const int delayEstInterval = 10 * aec->mult;

    float* xfw_ptr = NULL;

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    short asSubBand[kSubBandNum + 1] = {0, 5, 9, 13, 17, 25, 37, 49, 65};
    int aiOD_Thr[kSubBandNum] = {6, 9, 9, 9, 9, 9, 15, 21};
#endif

    aec->delayEstCtr++;
    if (aec->delayEstCtr == delayEstInterval) {
        aec->delayEstCtr = 0;
    }

    // initialize comfort noise for H band
    memset(comfortNoiseHband, 0, sizeof(comfortNoiseHband));
    nlpGainHband = (float)0.0;
    dtmp = (float)0.0;

    // We should always have at least one element stored in |far_buf|.
    assert(WebRtc_youme_available_read(aec->far_buf_windowed) > 0);
    // NLP
    WebRtc_youme_ReadBuffer(aec->far_buf_windowed, (void**)&xfw_ptr, &xfw[0][0], 1);

    // TODO(bjornv): Investigate if we can reuse |far_buf_windowed| instead of
    // |xfwBuf|.
    // Buffer far.
    memcpy(aec->xfwBuf, xfw_ptr, sizeof(float) * 2 * PART_LEN1);

    WebRtcAec_SubbandCoherence(aec, efw, xfw, fft, cohde, cohxd);

    //#############################################
    for (i = 1; i < PART_LEN1; i++) {
        //cohde[i] = 1;
        //cohxd[i] = 0;
    }
    //#############################################
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    short sSBNum = 0;

    for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
        minPrefBand = asSubBand[sSBNum];
        prefBandSize = asSubBand[sSBNum + 1] - asSubBand[sSBNum];

        hNlXdAvg = 0;
        for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
            hNlXdAvg += cohxd[i];
        }
        hNlXdAvg /= prefBandSize;
        hNlXdAvg = 1 - hNlXdAvg;

        hNlDeAvg = 0;
        for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
            hNlDeAvg += cohde[i];
        }
        hNlDeAvg /= prefBandSize;

        if (hNlXdAvg < 0.75f && hNlXdAvg < aec->hNlXdAvgMin[sSBNum]) {
            aec->hNlXdAvgMin[sSBNum] = hNlXdAvg;
        }

        if (hNlDeAvg > 0.98f && hNlXdAvg > 0.9f) {
            aec->stNearState[sSBNum] = 1;
        } else if (hNlDeAvg < 0.95f || hNlXdAvg < 0.8f) {
            aec->stNearState[sSBNum] = 0;
        }
        if (aec->hNlXdAvgMin[sSBNum] == 1) {
            aec->echoStateSub[sSBNum] = 0;
            aec->overDrive[sSBNum] = min_overdrive[aec->nlp_mode];

            if (aec->stNearState[sSBNum] == 1) {
                for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
                    hNl[i] = cohde[i];
                }
                hNlFb[sSBNum] = hNlDeAvg;
                hNlFbLow = hNlDeAvg;
            } else {
                for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
                    hNl[i] = 1 - cohxd[i];
                }
                hNlFb[sSBNum] = hNlXdAvg;
                hNlFbLow = hNlXdAvg;
            }
        } else {

            if (aec->stNearState[sSBNum] == 1) {
                aec->echoStateSub[sSBNum] = 0;
                for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
                    hNl[i] = cohde[i];
                }
                hNlFb[sSBNum] = hNlDeAvg;
                hNlFbLow = hNlDeAvg;
            } else {
                aec->echoStateSub[sSBNum] = 1;
                for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
                    hNl[i] = WEBRTC_SPL_MIN(cohde[i], 1 - cohxd[i]);
                }

                // Select an order statistic from the preferred bands.
                // TODO: Using quicksort now, but a selection algorithm may be preferred.
                memcpy(hNlPref, &hNl[minPrefBand], sizeof(float) * prefBandSize);
                qsort(hNlPref, prefBandSize, sizeof(float), CmpFloat);
                hNlFb[sSBNum] = hNlPref[(int)floor(prefBandQuant * (prefBandSize - 1))];
                hNlFbLow = hNlPref[(int)floor(prefBandQuantLow * (prefBandSize - 1))];
            }
        }
        // Track the local filter minimum to determine suppression overdrive.
        if (hNlFbLow < 0.6f && hNlFbLow < aec->hNlFbLocalMin[sSBNum]) {
            aec->hNlFbLocalMin[sSBNum] = hNlFbLow;
            aec->hNlFbMin[sSBNum] = hNlFbLow;
            aec->hNlNewMin[sSBNum] = 1;
            aec->hNlMinCtr[sSBNum] = 0;
        }
        aec->hNlFbLocalMin[sSBNum] =
            WEBRTC_SPL_MIN(aec->hNlFbLocalMin[sSBNum] + 0.0008f / aec->mult, 1);
        aec->hNlXdAvgMin[sSBNum] = WEBRTC_SPL_MIN(aec->hNlXdAvgMin[sSBNum] + 0.0006f / aec->mult, 1);

        if (aec->hNlNewMin[sSBNum] == 1) {
            aec->hNlMinCtr[sSBNum]++;
        }
        if (aec->hNlMinCtr[sSBNum] == 2) {
            aec->hNlNewMin[sSBNum] = 0;
            aec->hNlMinCtr[sSBNum] = 0;
            aec->overDrive[sSBNum] =
                WEBRTC_SPL_MAX(kTargetSupp[aec->nlp_mode] /
                               ((float)log(aec->hNlFbMin[sSBNum] + 1e-10f) + 1e-10f),
                               min_overdrive[aec->nlp_mode]);
        }
    }

    float overDrive = 0;
    float overDriveLow = 0;

    minPrefBand = 0;
    prefBandSize = kSubBandNum;

    memcpy(hNlPref, &aec->overDrive[minPrefBand], sizeof(float) * prefBandSize);
    qsort(hNlPref, prefBandSize, sizeof(float), CmpFloat);
    overDrive = hNlPref[(int)floor(0.75 * (kSubBandNum - 1))];
    overDriveLow = hNlPref[(int)floor(0.25 * (kSubBandNum - 1))];

    // Smooth the overdrive.
    for (sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
        if (0 == aec->stNearState[sSBNum]) {
            if (aec->overDrive[sSBNum] < overDrive) {
                aec->overDrive[sSBNum] = overDrive;
            }
        } else {
            if (aec->overDrive[sSBNum] < overDriveLow) {
                aec->overDrive[sSBNum] = overDriveLow;
            }
        }

        if (aec->dt_flag > 0) {
            if ((sSBNum < kSubBand4500HzNum + 1) && (aec->overDrive[sSBNum] > aiOD_Thr[sSBNum] )) {
                aec->overDrive[sSBNum] = aec->overDrive[sSBNum] * 0.99 + aiOD_Thr[sSBNum] * 0.01;
            }
        } else {
            aec->overDrive[sSBNum] = aec->overDrive[sSBNum] > aiOD_Thr[kSubBandNum - 1] ? aiOD_Thr[kSubBandNum - 1] : aec->overDrive[sSBNum];
        }

        if (aec->overDrive[sSBNum] < aec->overDriveSm[sSBNum]) {
            aec->overDriveSm[sSBNum] = 0.99f * aec->overDriveSm[sSBNum] + 0.01f * aec->overDrive[sSBNum];
        } else {
            aec->overDriveSm[sSBNum] = 0.9f * aec->overDriveSm[sSBNum] + 0.1f * aec->overDrive[sSBNum];
        }
    }

#else
    hNlXdAvg = 0;
    for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
        hNlXdAvg += cohxd[i];
    }
    hNlXdAvg /= prefBandSize;
    hNlXdAvg = 1 - hNlXdAvg;

    hNlDeAvg = 0;
    for (i = minPrefBand; i < prefBandSize + minPrefBand; i++) {
        hNlDeAvg += cohde[i];
    }
    hNlDeAvg /= prefBandSize;

    if (hNlXdAvg < 0.75f && hNlXdAvg < aec->hNlXdAvgMin) {
        aec->hNlXdAvgMin = hNlXdAvg;
    }

    if (hNlDeAvg > 0.98f && hNlXdAvg > 0.9f) {
        aec->stNearState = 1;
    } else if (hNlDeAvg < 0.95f || hNlXdAvg < 0.8f) {
        aec->stNearState = 0;
    }

    if (aec->hNlXdAvgMin == 1) {
        aec->echoState = 0;
        aec->overDrive = min_overdrive[aec->nlp_mode];

        if (aec->stNearState == 1) {
            memcpy(hNl, cohde, sizeof(hNl));
            hNlFb = hNlDeAvg;
            hNlFbLow = hNlDeAvg;
        } else {
            for (i = 0; i < PART_LEN1; i++) {
                hNl[i] = 1 - cohxd[i];
            }
            hNlFb = hNlXdAvg;
            hNlFbLow = hNlXdAvg;
        }
    } else {

        if (aec->stNearState == 1) {
            aec->echoState = 0;
            memcpy(hNl, cohde, sizeof(hNl));
            hNlFb = hNlDeAvg;
            hNlFbLow = hNlDeAvg;
        } else {
            aec->echoState = 1;
            for (i = 0; i < PART_LEN1; i++) {
                hNl[i] = WEBRTC_SPL_MIN(cohde[i], 1 - cohxd[i]);
            }

            // Select an order statistic from the preferred bands.
            // TODO: Using quicksort now, but a selection algorithm may be preferred.
            memcpy(hNlPref, &hNl[minPrefBand], sizeof(float) * prefBandSize);
            qsort(hNlPref, prefBandSize, sizeof(float), CmpFloat);
            hNlFb = hNlPref[(int)floor(prefBandQuant * (prefBandSize - 1))];
            hNlFbLow = hNlPref[(int)floor(prefBandQuantLow * (prefBandSize - 1))];
        }
    }

    // Track the local filter minimum to determine suppression overdrive.
    if (hNlFbLow < 0.6f && hNlFbLow < aec->hNlFbLocalMin) {
        aec->hNlFbLocalMin = hNlFbLow;
        aec->hNlFbMin = hNlFbLow;
        aec->hNlNewMin = 1;
        aec->hNlMinCtr = 0;
    }
    aec->hNlFbLocalMin =
        WEBRTC_SPL_MIN(aec->hNlFbLocalMin + 0.0008f / aec->mult, 1);
    aec->hNlXdAvgMin = WEBRTC_SPL_MIN(aec->hNlXdAvgMin + 0.0006f / aec->mult, 1);

    if (aec->hNlNewMin == 1) {
        aec->hNlMinCtr++;
    }
    if (aec->hNlMinCtr == 2) {
        aec->hNlNewMin = 0;
        aec->hNlMinCtr = 0;
        aec->overDrive =
            WEBRTC_SPL_MAX(kTargetSupp[aec->nlp_mode] /
                           ((float)log(aec->hNlFbMin + 1e-10f) + 1e-10f),
                           min_overdrive[aec->nlp_mode]);
    }

    // Smooth the overdrive.
    if (aec->overDrive < aec->overDriveSm) {
        aec->overDriveSm = 0.99f * aec->overDriveSm + 0.01f * aec->overDrive;
    } else {
        aec->overDriveSm = 0.9f * aec->overDriveSm + 0.1f * aec->overDrive;
    }
#endif

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    //aec->dt_flag_old = hNlXdAvg * 10000;
    //aec->dt_flag_old = aec->stNearState * 16384;
#endif

    WebRtcAec_OverdriveAndSuppress(aec, hNl, hNlFb, efw);

    // Add comfort noise.
    WebRtcAec_ComfortNoise(aec, efw, comfortNoiseHband, aec->noisePow, hNl);
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    if (0 == aec->iBeginOfEcho)
    {
        WebRtcAec_ERL_Estimation(aec, hNl, efw);
    }
#endif
    // TODO(bjornv): Investigate how to take the windowing below into account if
    // needed.
    if (aec->metricsMode == 1) {
        // Note that we have a scaling by two in the time domain |eBuf|.
        // In addition the time domain signal is windowed before transformation,
        // losing half the energy on the average. We take care of the first
        // scaling only in UpdateMetrics().
        UpdateLevel(&aec->nlpoutlevel, efw);
    }
    // Inverse error fft.
    fft[0] = efw[0][0];
    fft[1] = efw[0][PART_LEN];
    for (i = 1; i < PART_LEN; i++) {
        fft[2 * i] = efw[0][i];
        // Sign change required by Ooura fft.
        fft[2 * i + 1] = -efw[1][i];
    }
    aec_rdft_inverse_128(fft);

    // Overlap and add to obtain output.
    scale = 2.0f / PART_LEN2;
    for (i = 0; i < PART_LEN; i++) {
        fft[i] *= scale;  // fft scaling
        fft[i] = fft[i] * WebRtcAec_sqrtHanning[i] + aec->outBuf[i];

        fft[PART_LEN + i] *= scale;  // fft scaling
        aec->outBuf[i] = fft[PART_LEN + i] * WebRtcAec_sqrtHanning[PART_LEN - i];

        // Saturate output to keep it in the allowed range.
        output[i] = WEBRTC_SPL_SAT(
                        WEBRTC_SPL_WORD16_MAX, fft[i], WEBRTC_SPL_WORD16_MIN);
    }

    // For H band
    if (aec->num_bands > 1) {

        // H band gain
        // average nlp over low band: average over second half of freq spectrum
        // (4->8khz)
        GetHighbandGain(hNl, &nlpGainHband);

        // Inverse comfort_noise
        if (flagHbandCn == 1) {
            fft[0] = comfortNoiseHband[0][0];
            fft[1] = comfortNoiseHband[PART_LEN][0];
            for (i = 1; i < PART_LEN; i++) {
                fft[2 * i] = comfortNoiseHband[i][0];
                fft[2 * i + 1] = comfortNoiseHband[i][1];
            }
            aec_rdft_inverse_128(fft);
            scale = 2.0f / PART_LEN2;
        }

        // compute gain factor
        for (j = 0; j < aec->num_bands - 1; ++j) {
            for (i = 0; i < PART_LEN; i++) {
                dtmp = aec->dBufH[j][i];
                dtmp = dtmp * nlpGainHband;  // for variable gain

                // add some comfort noise where Hband is attenuated
                if (flagHbandCn == 1 && j == 0) {
                    fft[i] *= scale;  // fft scaling
                    dtmp += cnScaleHband * fft[i];
                }

                // Saturate output to keep it in the allowed range.
                outputH[j][i] = WEBRTC_SPL_SAT(
                                    WEBRTC_SPL_WORD16_MAX, dtmp, WEBRTC_SPL_WORD16_MIN);
            }
        }
    }

    // Copy the current block to the old position.
    memcpy(aec->dBuf, aec->dBuf + PART_LEN, sizeof(float) * PART_LEN);
    memcpy(aec->eBuf, aec->eBuf + PART_LEN, sizeof(float) * PART_LEN);

    // Copy the current block to the old position for H band
    for (j = 0; j < aec->num_bands - 1; ++j) {
        memcpy(aec->dBufH[j], aec->dBufH[j] + PART_LEN, sizeof(float) * PART_LEN);
    }

    memmove(aec->xfwBuf + PART_LEN1,
            aec->xfwBuf,
            sizeof(aec->xfwBuf) - sizeof(complex_t) * PART_LEN1);
}

static void ProcessBlock(AecCore* aec) {
    size_t i;
    float y[PART_LEN], e[PART_LEN];
    float scale;

    float fft[PART_LEN2];
    float xf[2][PART_LEN1], yf[2][PART_LEN1], ef[2][PART_LEN1];
    float df[2][PART_LEN1];
    float far_spectrum = 0.0f;
    float near_spectrum = 0.0f;
    float abs_far_spectrum[PART_LEN1];
    float abs_near_spectrum[PART_LEN1];

    const float gPow[2] = {0.9f, 0.1f};

    // Noise estimate constants.
    const int noiseInitBlocks = 500 * aec->mult;
    const float step = 0.1f;
    const float ramp = 1.0002f;
    const float gInitNoise[2] = {0.999f, 0.001f};

    float nearend[PART_LEN];
    float* nearend_ptr = NULL;
    float output[PART_LEN];
    float outputH[NUM_HIGH_BANDS_MAX][PART_LEN];
    float* outputH_ptr[NUM_HIGH_BANDS_MAX];
    for (i = 0; i < NUM_HIGH_BANDS_MAX; ++i) {
        outputH_ptr[i] = outputH[i];
    }

    float* xf_ptr = NULL;
    float farend[PART_LEN];
    float farend1[PART_LEN];
    float* farend_ptr = NULL;
    farend_ptr = farend1;

    float* xf_ptr_filter = NULL;
    float xf_ptr_filter1[2*PART_LEN1];
    xf_ptr_filter = xf_ptr_filter1;
    short sRVadFlag = 0;
    short sSVadFlag = 0;
    //int16_t debug_out[PART_LEN];
    #ifdef WEBRTC_AEC_TO_PHONE_DEBUG  
      aec->uiFrameCount++;
      float minTrack_step = 0.0;
      short NoisePos = 0;
      short j = 0;
      float ftmp1, ftmp2, ftmp3;
    #endif
    // Concatenate old and new nearend blocks.
    for (i = 0; i < aec->num_bands - 1; ++i) {
        WebRtc_youme_ReadBuffer(aec->nearFrBufH[i],
                          (void**)&nearend_ptr,
                          nearend,
                          PART_LEN);
        memcpy(aec->dBufH[i] + PART_LEN, nearend_ptr, sizeof(nearend));
    }
    WebRtc_youme_ReadBuffer(aec->nearFrBuf, (void**)&nearend_ptr, nearend, PART_LEN);
    memcpy(aec->dBuf + PART_LEN, nearend_ptr, sizeof(nearend));

    // ---------- Ooura fft ----------

    WebRtc_youme_ReadBuffer(aec->far_time_buf, (void**)&farend_ptr, farend, 1);
#ifdef DEBUG_RECORD
    int16_t tmp[PART_LEN], tmp1[PART_LEN];
    for (i = 0; i < PART_LEN; i++)
    {
        tmp[i] = (int16_t)farend_ptr[i];
        tmp1[i] = (int16_t)nearend_ptr[i];
    }

    if (noisy_nearFile) {
        memcpy(aec->aec_sin_data + PART_LEN * aec->aec_sin_write_pos, tmp1, sizeof(tmp1));
    }
    
    if (farFile) {
        memcpy(aec->aec_ref_data + PART_LEN * aec->aec_ref_write_pos, tmp, sizeof(tmp));
    }
#endif

#ifdef WEBRTC_AEC_DEBUG_DUMP
    {
        RTC_AEC_DEBUG_WAV_WRITE(aec->farFile, farend_ptr, PART_LEN);
        RTC_AEC_DEBUG_WAV_WRITE(aec->nearFile, nearend_ptr, PART_LEN);
    }
#endif

    // We should always have at least one element stored in |far_buf|.
    int available_far_data = WebRtc_youme_available_read(aec->far_buf);
    assert(available_far_data > 0);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
    available_far_data = WebRtc_youme_available_read(aec->far_time_buf);
    int iTmp_pos = WebRtc_youme_get_write_pos(aec->far_time_buf);

    if (-1 == aec->old_write_pos)
    {
        aec->old_write_pos = iTmp_pos;
        aec->FarEndBufferLength = available_far_data - 1;
    }else
    {
        int Pos_diff = iTmp_pos - aec->old_write_pos;
        if (Pos_diff < 0)
        {
            int iTmp_ele_cnt = WebRtc_youme_get_element_count(aec->far_time_buf);
            Pos_diff = Pos_diff + iTmp_ele_cnt;
        }
        aec->old_write_pos = iTmp_pos;
        aec->FarEndBufferLength += (Pos_diff - 1);
        
        if (Pos_diff > 0)
        {
            float sm_Step = 0.05f;
            if (aec->uiFrameCount < 100)
            {
                sm_Step = 0.5f;
            } 

            if (aec->FarPlayOutDelta < aec->FarPlayOutWaitCnt)
            {
                aec->DeltaIsTooHigh = 0;
                aec->FarPlayOutDelta = aec->FarPlayOutDelta * (1 - sm_Step) + aec->FarPlayOutWaitCnt * sm_Step;
            }
            else
            {
                float sm_Step2 = 0.001;
                if (aec->FarPlayOutDelta > aec->FarPlayOutWaitCnt + PART_LEN)
                {
                    aec->DeltaIsTooHigh += aec->FarPlayOutWaitCnt;
                }
                else
                {
                    aec->DeltaIsTooHigh = 0;
                }
                if (aec->DeltaIsTooHigh > aec->sampFreq)
                {
                    sm_Step2 = 0.1;
                }
                aec->FarPlayOutDelta = aec->FarPlayOutDelta * (1 - sm_Step2) + aec->FarPlayOutWaitCnt * sm_Step2;
            }
            aec->FarPlayOutWaitCnt = PART_LEN;
        }
        else
        {
            aec->FarPlayOutWaitCnt += PART_LEN;
        }
    }
#endif

    int ilookahead = WebRtc_lookahead(aec->delay_estimator);
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    // as the filter only support a 48ms long echo delay,
    // if the delay is larger than 48ms, we should read the earlier far-end data.
    int read_offset = 0;
    int iDelay_diff = aec->delay_est - aec->delay_est_old;
    iDelay_diff = iDelay_diff > 0 ? iDelay_diff : (0 - iDelay_diff);
    if (((iDelay_diff > 0 && 0 == aec->has_modified_readpos) || (iDelay_diff > 2)) && (aec->delay_est > 0)) { // if delay changed
        if (0 == aec->xfBufBlockPos) { // makesure use the same delay in a 48ms block
            // get the new read offset
            if (aec->has_modified_readpos) {
                read_offset = aec->delay_est - aec->delay_est_old;
                available_far_data = WebRtc_youme_available_read(aec->far_buf_save);
            } else {
                // considering of the difference between the two methods that we use
                // to estimate the echo delay, try to move the ReadPtr a little smaller
                // than the real echo delay.
                read_offset = aec->delay_est - ilookahead - 5;
                available_far_data = WebRtc_youme_available_read(aec->far_buf_save);
                // if is the first time to move the ReadPtr, we only move it backward
                // if (read_offset < 0) {
                //     read_offset = 0;
                // }
            }
        }

        read_offset = 0 - read_offset;
        // makesure when we moving ReadPtr forward, there is enough data in far_buf
        if ((0 != read_offset) && (read_offset <= available_far_data - 3)) {
            read_offset = WebRtc_youme_MoveReadPtr_Back(aec->far_buf_save, read_offset);
            read_offset = WebRtc_youme_MoveReadPtr_Back(aec->far_buf_windowed, read_offset);
            aec->delay_est_old = aec->delay_est;
            aec->has_modified_readpos = 1;

            aec->his_read_offset -= read_offset; //get the total read_offset for debug
        }
    }
    // if ((aec->delay_est != aec->delay_est_old) && (aec->delay_est < (aec->num_partitions/2 + ilookahead)))
    // {
    //     read_offset = aec->his_read_offset;
    //     read_offset = WebRtc_youme_MoveReadPtr_Back(aec->far_buf_save, read_offset);
    //     read_offset = WebRtc_youme_MoveReadPtr_Back(aec->far_buf_windowed, read_offset);
    //     aec->his_read_offset = 0;
    //     aec->has_modified_readpos = 0;
    //     aec->delay_est_old = 0;
    // }
    WebRtc_youme_ReadBuffer(aec->far_buf_save, (void**) &xf_ptr_filter, &xf[0][0], 1);
    WebRtc_youme_ReadBuffer(aec->far_buf, (void**)&xf_ptr, &xf[0][0], 1);
#else
    WebRtc_youme_ReadBuffer(aec->far_buf, (void**)&xf_ptr, &xf[0][0], 1);
    for (i = 0; i < PART_LEN1; i++)
    {
        xf_ptr_filter[i] = xf_ptr[i];
        xf_ptr_filter[PART_LEN1 + i] = xf_ptr[PART_LEN1 + i];
    }
#endif

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
#ifdef DEBUG_RECORD
    {
        int16_t tmp[PART_LEN2];
        for (i = 0; i < PART_LEN; i++)
        {
            tmp[i] = (int16_t)aec->delay_est * 100;
        }
        if(delay_nearFile)
        {
            //(void)fwrite(tmp, sizeof(int16_t), PART_LEN, delay_nearFile);
            int64_t ltime = GetTimeInMS();
            fprintf(delay_nearFile,"%lld\t###########################\n", ltime);
        }
        for (i = 0; i < PART_LEN2-1; i+=2)
        {
            //tmp[i] = (int16_t)aec->his_read_offset * 100;
            tmp[i] = (int16_t)aec->FarEndBufferLength * 100;
            //tmp[i+1] = available_far_data * 100;
            //tmp[i] = aec->his_read_offset * 10;
            tmp[i+1] = aec->delay_est * 100;
            //tmp[i+1] = (int16_t)(aec->FarPlayOutDelta+PART_LEN/2)/PART_LEN * 100;
        }
        if (offset_nearFile) {
            memcpy(aec->aec_offset_data + PART_LEN2 * aec->aec_offset_write_pos, tmp, sizeof(tmp));
        }

    }
#endif
#endif

    // Near fft
    memcpy(fft, aec->dBuf, sizeof(float) * PART_LEN2);
    TimeToFrequency(fft, df, 0);

    // Power smoothing
    for (i = 0; i < PART_LEN1; i++) {
        far_spectrum = (xf_ptr_filter[i] * xf_ptr_filter[i]) +
                       (xf_ptr_filter[PART_LEN1 + i] * xf_ptr_filter[PART_LEN1 + i]);
        aec->xPow[i] =
            gPow[0] * aec->xPow[i] + gPow[1] * aec->num_partitions * far_spectrum;

        // Calculate absolute spectra
        far_spectrum = (xf_ptr[i] * xf_ptr[i]) +
                       (xf_ptr[PART_LEN1 + i] * xf_ptr[PART_LEN1 + i]);
        abs_far_spectrum[i] = sqrtf(far_spectrum);

        near_spectrum = df[0][i] * df[0][i] + df[1][i] * df[1][i];
        aec->dPow[i] = gPow[0] * aec->dPow[i] + gPow[1] * near_spectrum;
        // Calculate absolute spectra

        abs_near_spectrum[i] = sqrtf(near_spectrum);
    }

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    sSVadFlag = WebRtcAec_TimeVAD(&(aec->time_VadS), nearend_ptr, PART_LEN, aec->uiFrameCount);
    sRVadFlag = WebRtcAec_TimeVAD(&(aec->time_VadR), farend_ptr, PART_LEN, aec->uiFrameCount);
    memmove(&(aec->asVadR[1]), &(aec->asVadR[0]), (kHistorySizeBlocks-1) * sizeof(short));
    aec->asVadR[0] = sRVadFlag;

    sRVadFlag = 1;
#else
    sRVadFlag = 1;
#endif

#ifdef WEBRTC_AEC_NEW_CNG

    if (0 == aec->echoState)
    {
        aec->NoEchoCount++;
    }
    else
    {
        aec->NoEchoCount = 0;
    }

    if ((aec->time_VadS.sNoiseCount >= 25) && (aec->NoEchoCount >= 25))
    {
        for (i = 0; i < PART_LEN1; i++)
        {
            if (aec->dMinPow[i] < aec->dPow[i])
            {
                aec->noiseEstTooLowCtr[i]++;
                if (aec->noiseEstTooLowCtr[i] >= 1)
                {
                    minTrack_step = 0.00025*(1 + aec->noiseEstTooLowCtr[i]/(kNoiseEstIncCount*50)); 
                    minTrack_step = minTrack_step > 0.0015 ? 0.0015 : minTrack_step;
                    aec->dMinPow[i] += ((aec->dPow[i] - aec->dMinPow[i])*minTrack_step);
                    //aec->noiseEstTooLowCtr[i] = 0;
                }               
            }
        }
    }

    for (i = 0; i < PART_LEN1; i++)
    {
        aec->dMinPow_out[i] = 0;
        if (aec->dMinPow[i] >= aec->dPow[i])
        {
            aec->noiseEstTooLowCtr[i] = 0;
            aec->dMinPow[i] -= ((aec->dMinPow[i] - aec->dPow[i])*0.75);
        }
    }

    for (j = 0; j < 7; j+=2)
    {
        for (i = 0; i < PART_LEN1; i++)
        {
            ftmp1 = (aec->noiseEst_save[j][i] + aec->noiseEst_save[j+1][i]) * 0.5;
            aec->dMinPow_out[i] += ftmp1 * 0.25;
        }
    }

    NoisePos = aec->uiFrameCount % 8;
    for (i = 0; i < PART_LEN1; i++)
    {
        ftmp2 = aec->dPow[i];
        ftmp3 = aec->dMinPow[i];
        ftmp2 = ftmp2 > ftmp3 ? ftmp3 : ftmp2;
        aec->noiseEst_save[NoisePos][i] = ftmp2;

        ftmp1 = aec->dMinPow_out[i];
        ftmp2 = ftmp2 > ftmp1 ? ftmp1 : ftmp2;
        aec->dMinPow_out[i] = ftmp2 * 1.414;// * 2;
    }

    aec->noisePow = aec->dMinPow_out;

#else
    // Estimate noise power. Wait until dPow is more stable.
    if (aec->noiseEstCtr > 50) {
        for (i = 0; i < PART_LEN1; i++) {
            if (aec->dPow[i] < aec->dMinPow[i]) {
                aec->dMinPow[i] =
                    (aec->dPow[i] + step * (aec->dMinPow[i] - aec->dPow[i])) * ramp;
            } else {
                aec->dMinPow[i] *= ramp;
            }
        }
    }

    // Smooth increasing noise power from zero at the start,
    // to avoid a sudden burst of comfort noise.
    if (aec->noiseEstCtr < noiseInitBlocks) {
        aec->noiseEstCtr++;
        for (i = 0; i < PART_LEN1; i++) {
            if (aec->dMinPow[i] > aec->dInitMinPow[i]) {
                aec->dInitMinPow[i] = gInitNoise[0] * aec->dInitMinPow[i] +
                                      gInitNoise[1] * aec->dMinPow[i];
            } else {
                aec->dInitMinPow[i] = aec->dMinPow[i];
            }
        }
        aec->noisePow = aec->dInitMinPow;
    } else {
        aec->noisePow = aec->dMinPow;
    }
#endif

    //sRVadFlag = 1;
    if (aec->delay_logging_enabled) {
        sRVadFlag = 1;
    }
    // Block wise delay estimation used for logging
    if (sRVadFlag) {
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
        if (WebRtc_AddFarSpectrumFloat(
                    aec->delay_estimator_farend, abs_far_spectrum, PART_LEN1, aec->mult) == 0) 
#else
        if (WebRtc_AddFarSpectrumFloat(
                    aec->delay_estimator_farend, abs_far_spectrum, PART_LEN1) == 0) 
#endif
        {
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
            int delay_estimate = WebRtc_DelayEstimatorProcessFloat(
                                     aec->delay_estimator, abs_near_spectrum, 
                                     PART_LEN1, aec->mult, aec->asVadR);
#else
            int delay_estimate = WebRtc_DelayEstimatorProcessFloat(
                                     aec->delay_estimator, abs_near_spectrum, PART_LEN1);
#endif



            if (aec->delay_tmp == 0 && delay_estimate < 0) {
                aec->delay_start_time_ms = youmecommon::CTimeUtil::GetTimeOfDay_MS();
                aec->delay_tmp = delay_estimate;
            }
            if (aec->delay_tmp < 0 && delay_estimate > 0) {
                int64_t time_cost_ms = youmecommon::CTimeUtil::GetTimeOfDay_MS() - aec->delay_start_time_ms;
                TSK_DEBUG_INFO("AEC delay estimate value:%d, consume time:%f", delay_estimate, ((double)time_cost_ms)/1000); 
                aec->delay_tmp = 0;
            }

            if (delay_estimate >= 0) {
      #ifdef WEBRTC_AEC_TO_PHONE_DEBUG
              aec->delay_est = delay_estimate;
              //aec->delay_est = 30;
              //aec->delay_est_old = delay_estimate;
      #endif
            }

            if (aec->delay_logging_enabled) {
                if (delay_estimate >= 0) {
                    // Update delay estimate buffer.
                    aec->delay_histogram[delay_estimate]++;
                    aec->num_delay_values++;
                }
                if (aec->delay_metrics_delivered == 1 &&
                        aec->num_delay_values >= kDelayMetricsAggregationWindow) {
                    UpdateDelayMetrics(aec);
                }
            }
        }
    }

    // Update the xfBuf block position.
    aec->xfBufBlockPos--;
    if (aec->xfBufBlockPos == -1) {
        aec->xfBufBlockPos = aec->num_partitions - 1;
    }

    // Buffer xf
    memcpy(aec->xfBuf[0] + aec->xfBufBlockPos * PART_LEN1,
           xf_ptr_filter,
           sizeof(float) * PART_LEN1);
    memcpy(aec->xfBuf[1] + aec->xfBufBlockPos * PART_LEN1,
           &xf_ptr_filter[PART_LEN1],
           sizeof(float) * PART_LEN1);

    memset(yf, 0, sizeof(yf));

    // Filter far
    WebRtcAec_FilterFar(aec, yf);

    // Inverse fft to obtain echo estimate and error.
    fft[0] = yf[0][0];
    fft[1] = yf[0][PART_LEN];
    for (i = 1; i < PART_LEN; i++) {
        fft[2 * i] = yf[0][i];
        fft[2 * i + 1] = yf[1][i];
    }
    aec_rdft_inverse_128(fft);

    scale = 2.0f / PART_LEN2;
    for (i = 0; i < PART_LEN; i++) {
        y[i] = fft[PART_LEN + i] * scale;  // fft scaling
    }

    for (i = 0; i < PART_LEN; i++) {
        e[i] = nearend_ptr[i] - y[i];
    }

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    WebRtcAec_BeginOfEchoDetection (aec);
    // yfw fft
    memcpy(fft, aec->afYSaveBuf, sizeof(float) * PART_LEN);
    memcpy(fft + PART_LEN, y, sizeof(float) * PART_LEN);
    TimeToFrequency(fft, aec->afYfwBuf, 1);    
    memcpy(aec->afYSaveBuf, y, sizeof(float) * PART_LEN);
#endif

    // Error fft
    memcpy(aec->eBuf + PART_LEN, e, sizeof(float) * PART_LEN);
    memset(fft, 0, sizeof(float) * PART_LEN);
    memcpy(fft + PART_LEN, e, sizeof(float) * PART_LEN);
    // TODO(bjornv): Change to use TimeToFrequency().
    aec_rdft_forward_128(fft);

    ef[1][0] = 0;
    ef[1][PART_LEN] = 0;
    ef[0][0] = fft[0];
    ef[0][PART_LEN] = fft[1];
    for (i = 1; i < PART_LEN; i++) {
        ef[0][i] = fft[2 * i];
        ef[1][i] = fft[2 * i + 1];
    }

    RTC_AEC_DEBUG_RAW_WRITE(aec->e_fft_file,
                            &ef[0][0],
                            sizeof(ef[0][0]) * PART_LEN1 * 2);

    if (aec->metricsMode == 1) {
        // Note that the first PART_LEN samples in fft (before transformation) are
        // zero. Hence, the scaling by two in UpdateLevel() should not be
        // performed. That scaling is taken care of in UpdateMetrics() instead.
        UpdateLevel(&aec->linoutlevel, ef);
    }

    // Scale error signal inversely with far power.
    WebRtcAec_ScaleErrorSignal(aec, ef);
    WebRtcAec_FilterAdaptation(aec, fft, ef);
    NonLinearProcessing(aec, output, outputH_ptr);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_PCM
    float fErlAvg = 0;
    for (i = 0; i < PART_LEN/2; i++)
    {
        fErlAvg += aec->afERLSm[i];
    }
    fErlAvg = fErlAvg / (PART_LEN1/2);
    ilookahead = WebRtc_lookahead(aec->delay_estimator);

    int iNearState = 0;
    for (int sSBNum = 0; sSBNum < kSubBandNum; sSBNum++) {
        iNearState += aec->stNearState[sSBNum];
    }

    for (i = 0; i < PART_LEN; i++) {
        //debug_out[i] = iNearState * 100;
        //debug_out[i] = aec->dt_flag;
        //debug_out[i] = aec->delay_est * 100;
        debug_out[i] = aec->delayIdx * 100;
        //debug_out[i] = sRVadFlag * 16384;
        //debug_out[i] = sSVadFlag * 16384;
        //debug_out[i] = aec->time_VadR.sLogEnergyCur;
        //debug_out[i] = aec->his_read_offset * 100;
        //debug_out[i] = aec->fLowHnlSm * 10000;
        //debug_out[i] = aec->fHighHnlSm * 10000;
        //debug_out[i] = aec->fHnlVoiceAvg * 10000;
        //debug_out[i] = aec->filter_reseted * 100;
        //debug_out[i] = fErlAvg * 1000;
        //debug_out[i] = aec->hNlErlAvg * 10000;
        //debug_out[i] = aec->asVadR[aec->delay_est - ilookahead] * 16384;
        //debug_out[i] = aec->iBeginOfEcho * 16384;
        //debug_out[i] = aec->delay_est_old;
        //debug_out[i] = aec->uiFrameCount;
        //debug_out[i] = aec->dt_flag_old;
        //debug_out[i] = y[i];
        //debug_out[i] = e[i];
        //debug_out[i] = aec->FarEndBufferLength * 100;

    }
    printf("%d\n", aec->uiFrameCount);
    (void)fwrite(debug_out, sizeof(int16_t), PART_LEN, aec->time_debug_file);
#endif
#endif

    if (aec->metricsMode == 1) {
        // Update power levels and echo metrics
        UpdateLevel(&aec->farlevel, (float(*)[PART_LEN1])xf_ptr_filter);
        UpdateLevel(&aec->nearlevel, df);
        UpdateMetrics(aec);
    }

    // Store the output block.
    WebRtc_youme_WriteBuffer(aec->outFrBuf, output, PART_LEN);
    // For high bands
    for (i = 0; i < aec->num_bands - 1; ++i) {
        WebRtc_youme_WriteBuffer(aec->outFrBufH[i], outputH[i], PART_LEN);
    }

    RTC_AEC_DEBUG_WAV_WRITE(aec->outLinearFile, e, PART_LEN);
    RTC_AEC_DEBUG_WAV_WRITE(aec->outFile, output, PART_LEN);

#ifdef DEBUG_RECORD
    int tmp_rec_count = kHistorySizeBlocks * 2 - 1;
    aec->aec_ref_write_pos++;
    aec->aec_sin_write_pos++;
    aec->aec_offset_write_pos++;
    if (farFile)
    {
        aec->aec_ref_write_pos = 
        aec->aec_ref_write_pos > tmp_rec_count ? 0 : aec->aec_ref_write_pos;
    }
    else
    {
        aec->aec_ref_write_pos = 0;
    }
    if (noisy_nearFile)
    {
        aec->aec_sin_write_pos = 
        aec->aec_sin_write_pos > tmp_rec_count ? 0 : aec->aec_sin_write_pos;
    }
    else
    {
        aec->aec_sin_write_pos = 0;
    }
    if (offset_nearFile)
    {
        aec->aec_offset_write_pos = 
        aec->aec_offset_write_pos > tmp_rec_count ? 0 : aec->aec_offset_write_pos;
    }
    else
    {
        aec->aec_offset_write_pos = 0;
    }
#endif
}

AecCore* WebRtcAec_CreateAec() {
    int i;
    AecCore* aec = (AecCore*)malloc(sizeof(AecCore));
    if (!aec) {
        return NULL;
    }

    aec->nearFrBuf = WebRtc_youme_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
    if (!aec->nearFrBuf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }

    aec->outFrBuf = WebRtc_youme_CreateBuffer(FRAME_LEN + PART_LEN, sizeof(float));
    if (!aec->outFrBuf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }

    for (i = 0; i < NUM_HIGH_BANDS_MAX; ++i) {
        aec->nearFrBufH[i] = WebRtc_youme_CreateBuffer(FRAME_LEN + PART_LEN,
                             sizeof(float));
        if (!aec->nearFrBufH[i]) {
            WebRtcAec_FreeAec(aec);
            return NULL;
        }
        aec->outFrBufH[i] = WebRtc_youme_CreateBuffer(FRAME_LEN + PART_LEN,
                                                sizeof(float));
        if (!aec->outFrBufH[i]) {
            WebRtcAec_FreeAec(aec);
            return NULL;
        }
    }

    // Create far-end buffers.
    aec->far_buf =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
    if (!aec->far_buf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    aec->media_buf =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
    if (!aec->media_buf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    aec->far_buf_windowed =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
    if (!aec->far_buf_windowed) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    aec->far_time_buf =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * PART_LEN);
    if (!aec->far_time_buf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    aec->media_time_buf =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * PART_LEN);
    if (!aec->media_time_buf) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    aec->far_buf_save =
        WebRtc_youme_CreateBuffer(kBufSizePartitions, sizeof(float) * 2 * PART_LEN1);
    if (!aec->far_buf_save) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
#endif
#ifdef WEBRTC_AEC_DEBUG_DUMP
    aec->instance_index = webrtc_aec_instance_count;
    aec->farFile = aec->nearFile = aec->outFile = aec->outLinearFile = NULL;
    aec->debug_dump_count = 0;
#endif
    aec->delay_estimator_farend =
    WebRtc_CreateDelayEstimatorFarend(PART_LEN1, kHistorySizeBlocks);
    if (aec->delay_estimator_farend == NULL) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    aec->delay_estimator_media =
    WebRtc_CreateDelayEstimatorFarend(PART_LEN1, kHistorySizeBlocks);
    if (aec->delay_estimator_media == NULL) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
    // We create the delay_estimator with the same amount of maximum lookahead as
    // the delay history size (kHistorySizeBlocks) for symmetry reasons.
    aec->delay_estimator = WebRtc_CreateDelayEstimator(
                               aec->delay_estimator_farend, 
                               aec->delay_estimator_media, kHistorySizeBlocks);

    if (aec->delay_estimator == NULL) {
        WebRtcAec_FreeAec(aec);
        return NULL;
    }
#ifdef RTC_ANDROID
    aec->delay_agnostic_enabled = 1;  // DA-AEC enabled by default.
    // DA-AEC assumes the system is causal from the beginning and will self adjust
    // the lookahead when shifting is required.
    WebRtc_set_lookahead(aec->delay_estimator, 0);
#else
    aec->delay_agnostic_enabled = 0;
    WebRtc_set_lookahead(aec->delay_estimator, kLookaheadBlocks);
#endif
    aec->delay_agnostic_enabled = 0;
    WebRtc_set_lookahead(aec->delay_estimator, kLookaheadBlocks);

    aec->extended_filter_enabled = 0;

    // Assembly optimization
    WebRtcAec_FilterFar = FilterFar;
    WebRtcAec_ScaleErrorSignal = ScaleErrorSignal;
    WebRtcAec_FilterAdaptation = FilterAdaptation;
    WebRtcAec_OverdriveAndSuppress = OverdriveAndSuppress;
    WebRtcAec_ComfortNoise = ComfortNoise;
    WebRtcAec_SubbandCoherence = SubbandCoherence;

#if defined(WEBRTC_ARCH_X86_FAMILY)
    if (WebRtc_GetCPUInfo(kSSE2)) {
        //WebRtcAec_InitAec_SSE2();
    }
#endif

#if defined(MIPS_FPU_LE)
    WebRtcAec_InitAec_mips();
#endif

#if defined(WEBRTC_HAS_NEON)
    WebRtcAec_InitAec_neon();
#elif defined(WEBRTC_DETECT_NEON)
	if (g_webrtc_neon_support_flag != 0) {
        WebRtcAec_InitAec_neon();
    }
#endif

    aec_rdft_init();

    return aec;
}

void WebRtcAec_FreeAec(AecCore* aec) {
    int i;
    if (aec == NULL) {
        return;
    }

    WebRtc_youme_FreeBuffer(aec->nearFrBuf);
    WebRtc_youme_FreeBuffer(aec->outFrBuf);

    for (i = 0; i < NUM_HIGH_BANDS_MAX; ++i) {
        WebRtc_youme_FreeBuffer(aec->nearFrBufH[i]);
        WebRtc_youme_FreeBuffer(aec->outFrBufH[i]);
    }

    WebRtc_youme_FreeBuffer(aec->far_buf);
    WebRtc_youme_FreeBuffer(aec->far_buf_windowed);
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    WebRtc_youme_FreeBuffer(aec->far_buf_save);
#endif
    WebRtc_youme_FreeBuffer(aec->media_buf);
    WebRtc_youme_FreeBuffer(aec->far_time_buf);
    WebRtc_youme_FreeBuffer(aec->media_time_buf);
    RTC_AEC_DEBUG_WAV_CLOSE(aec->farFile);
    RTC_AEC_DEBUG_WAV_CLOSE(aec->nearFile);
    RTC_AEC_DEBUG_WAV_CLOSE(aec->outFile);
    RTC_AEC_DEBUG_WAV_CLOSE(aec->outLinearFile);
    RTC_AEC_DEBUG_RAW_CLOSE(aec->e_fft_file);

    WebRtc_FreeDelayEstimator(aec->delay_estimator);
    WebRtc_FreeDelayEstimatorFarend(aec->delay_estimator_farend);
    WebRtc_FreeDelayEstimatorFarend(aec->delay_estimator_media);

    free(aec);
}

int WebRtcAec_InitAec(AecCore* aec, int sampFreq) {
    int i;

    aec->sampFreq = sampFreq;

    if (sampFreq == 8000) {
        aec->normal_mu = 0.6f;
        aec->normal_error_threshold = 2e-6f;
        aec->num_bands = 1;
    } else {
        aec->normal_mu = 0.5f;
        aec->normal_error_threshold = 1.5e-6f;
        aec->num_bands = (size_t)(sampFreq / 16000);
    }

    WebRtc_youme_InitBuffer(aec->nearFrBuf);
    WebRtc_youme_InitBuffer(aec->outFrBuf);
    for (i = 0; i < NUM_HIGH_BANDS_MAX; ++i) {
        WebRtc_youme_InitBuffer(aec->nearFrBufH[i]);
        WebRtc_youme_InitBuffer(aec->outFrBufH[i]);
    }

    // Initialize far-end buffers.
    WebRtc_youme_InitBuffer(aec->far_buf);
    WebRtc_youme_InitBuffer(aec->far_buf_windowed);
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    WebRtc_youme_InitBuffer(aec->far_buf_save);
#endif
    WebRtc_youme_InitBuffer(aec->media_buf);
    WebRtc_youme_InitBuffer(aec->media_time_buf);
#ifndef WEBRTC_AEC_DEBUG_DUMP
    WebRtc_youme_InitBuffer(aec->far_time_buf);
#else
    WebRtc_youme_InitBuffer(aec->far_time_buf);
    {
        int process_rate = sampFreq > 16000 ? 16000 : sampFreq;
        RTC_AEC_DEBUG_WAV_REOPEN("aec_far", aec->instance_index,
                                 aec->debug_dump_count, process_rate,
                                 &aec->farFile );
        RTC_AEC_DEBUG_WAV_REOPEN("aec_near", aec->instance_index,
                                 aec->debug_dump_count, process_rate,
                                 &aec->nearFile);
        RTC_AEC_DEBUG_WAV_REOPEN("aec_out", aec->instance_index,
                                 aec->debug_dump_count, process_rate,
                                 &aec->outFile );
        RTC_AEC_DEBUG_WAV_REOPEN("aec_out_linear", aec->instance_index,
                                 aec->debug_dump_count, process_rate,
                                 &aec->outLinearFile);
    }

    RTC_AEC_DEBUG_RAW_OPEN("aec_e_fft",
                           aec->debug_dump_count,
                           &aec->e_fft_file);

    ++aec->debug_dump_count;
#endif

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_PCM
    aec->time_debug_file = fopen("time_debug_file.pcm", "wb");
#endif
#endif

    aec->system_delay = 0;

    if (WebRtc_InitDelayEstimatorFarend(aec->delay_estimator_farend) != 0) {
        return -1;
    }
    if (WebRtc_InitDelayEstimatorFarend(aec->delay_estimator_media) != 0) {
        return -1;
    }
    if (WebRtc_InitDelayEstimator(aec->delay_estimator) != 0) {
        return -1;
    }
    aec->delay_logging_enabled = 0;
    aec->delay_tmp = 0;
    aec->delay_start_time_ms = 0;
    aec->delay_metrics_delivered = 0;
    memset(aec->delay_histogram, 0, sizeof(aec->delay_histogram));
    aec->num_delay_values = 0;
    aec->delay_median = -1;
    aec->delay_std = -1;
    aec->fraction_poor_delays = -1.0f;

    aec->signal_delay_correction = 0;
    aec->previous_delay = -2;  // (-2): Uninitialized.
    aec->delay_correction_count = 0;
    aec->shift_offset = kInitialShiftOffset;
    aec->delay_quality_threshold = kDelayQualityThresholdMin;

    aec->num_partitions = kNormalNumPartitions;

    // Update the delay estimator with filter length.  We use half the
    // |num_partitions| to take the echo path into account.  In practice we say
    // that the echo has a duration of maximum half |num_partitions|, which is not
    // true, but serves as a crude measure.
    WebRtc_set_allowed_offset(aec->delay_estimator, aec->num_partitions / 2);
    // TODO(bjornv): I currently hard coded the enable.  Once we've established
    // that AECM has no performance regression, robust_validation will be enabled
    // all the time and the APIs to turn it on/off will be removed.  Hence, remove
    // this line then.
    WebRtc_enable_robust_validation(aec->delay_estimator, 1);
    aec->frame_count = 0;

    // Default target suppression mode.
    aec->nlp_mode = 1;

    // Sampling frequency multiplier w.r.t. 8 kHz.
    // In case of multiple bands we process the lower band in 16 kHz, hence the
    // multiplier is always 2.
    if (aec->num_bands > 1) {
        aec->mult = 2;
    } else {
        aec->mult = (short)aec->sampFreq / 8000;
    }

    aec->farBufWritePos = 0;
    aec->farBufReadPos = 0;

    aec->inSamples = 0;
    aec->outSamples = 0;
    aec->knownDelay = 0;

    // Initialize buffers
    memset(aec->dBuf, 0, sizeof(aec->dBuf));
    memset(aec->eBuf, 0, sizeof(aec->eBuf));
    // For H bands
    for (i = 0; i < NUM_HIGH_BANDS_MAX; ++i) {
        memset(aec->dBufH[i], 0, sizeof(aec->dBufH[i]));
    }

    memset(aec->xPow, 0, sizeof(aec->xPow));
    memset(aec->dPow, 0, sizeof(aec->dPow));
#ifdef  WEBRTC_AEC_NEW_CNG
    aec->noisePow = aec->dMinPow;
    aec->NoEchoCount = 0;
    for (i = 0; i < PART_LEN1; i++)
    {
        aec->noiseEstTooLowCtr[i] = 0;
    }
#else
    memset(aec->dInitMinPow, 0, sizeof(aec->dInitMinPow));
    aec->noisePow = aec->dInitMinPow;
#endif
    // Initial comfort noise power
    for (i = 0; i < PART_LEN1; i++) {
        aec->dMinPow[i] = 1.0e6f;
    }
    aec->noiseEstCtr = 0;

    // Holds the last block written to
    aec->xfBufBlockPos = 0;
    // TODO: Investigate need for these initializations. Deleting them doesn't
    //       change the output at all and yields 0.4% overall speedup.
    memset(aec->xfBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
    memset(aec->wfBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
    memset(aec->sde, 0, sizeof(complex_t) * PART_LEN1);
    memset(aec->sxd, 0, sizeof(complex_t) * PART_LEN1);
    memset(
        aec->xfwBuf, 0, sizeof(complex_t) * kExtendedNumPartitions * PART_LEN1);
    memset(aec->se, 0, sizeof(float) * PART_LEN1);

    // To prevent numerical instability in the first block.
    for (i = 0; i < PART_LEN1; i++) {
        aec->sd[i] = 1;
    }
    for (i = 0; i < PART_LEN1; i++) {
        aec->sx[i] = 1;
    }

    memset(aec->hNs, 0, sizeof(aec->hNs));
    memset(aec->outBuf, 0, sizeof(float) * PART_LEN);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
    for (i = 0; i < kSubBandNum; i++) {
        aec->hNlFbMin[i] = 1;
        aec->hNlFbLocalMin[i] = 1;
        aec->hNlXdAvgMin[i] = 1;
        aec->hNlNewMin[i] = 0;
        aec->hNlMinCtr[i] = 0;
        aec->overDrive[i] = 2;
        aec->overDriveSm[i] = 2;
        aec->stNearState[i] = 0;
        aec->echoStateSub[i] = 0;
    }
    aec->delayIdx = 0;
    aec->divergeState = 0;
#else
    aec->hNlFbMin = 1;
    aec->hNlFbLocalMin = 1;
    aec->hNlXdAvgMin = 1;
    aec->hNlNewMin = 0;
    aec->hNlMinCtr = 0;
    aec->overDrive = 2;
    aec->overDriveSm = 2;
    aec->delayIdx = 0;
    aec->stNearState = 0;
    aec->echoState = 0;
    aec->divergeState = 0;
#endif
    aec->seed = 777;
    aec->delayEstCtr = 0;

    // Metrics disabled by default
    aec->metricsMode = 0;
    InitMetrics(aec);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    FILE *time_debug_file;

    aec->buffer_far_count = 0;
    aec->aec_count = 0;
    aec->delay_est = 0;
    aec->media_delay_est = 0;
    aec->delay_est_old = 0;
    aec->his_read_offset = 0;
    aec->has_modified_readpos = 0;
    aec->filter_reseted = 0;;
    aec->uiFrameCount = 0;
    aec->fLowHnlSm = 0;
    aec->fHnlVoiceAvg = 0;
    aec->fHighHnlSm = 0;
    aec->iVadRCount = 0;
    aec->iNoiseRCount = 0;
    aec->iNoiseRCountOld = 0;
    aec->iBeginOfEcho = 0;
    aec->FarEndBufferLength = 0;
    aec->FarPlayOutDelta = 0;
    aec->DeltaIsTooHigh = 0;
    aec->FarPlayOutWaitCnt = 0;
    aec->old_write_pos = -1;
#ifdef DEBUG_RECORD
    aec->file_opened_flag = 0;
    aec->aec_ref_write_pos = 0;
    aec->aec_ref_read_pos = 0;
    aec->aec_sin_write_pos = 0;
    aec->aec_sin_read_pos = 0;
    aec->aec_offset_write_pos = 0;
    aec->aec_offset_read_pos = 0;
    memset(aec->aec_ref_data, 0, sizeof(short) * kHistorySizeBlocks * PART_LEN2);
    memset(aec->aec_sin_data, 0, sizeof(short) * kHistorySizeBlocks * PART_LEN2);
    memset(aec->aec_offset_data, 0, sizeof(short) * kHistorySizeBlocks * PART_LEN2 * 2);
#endif
    memset(&aec->time_VadS, 0, sizeof(TimeVad_t));
    memset(&aec->time_VadR, 0, sizeof(TimeVad_t));
    memset(&aec->time_VadMedia, 0, sizeof(TimeVad_t));

    for (i = 0; i < PART_LEN1; i++)
    {
        aec->afERLSm[i] = 128;
    }

    memset(aec->afDfPSD, 0, sizeof(float) * PART_LEN1);
    memset(aec->afXfPSD, 0, sizeof(float) * PART_LEN1);
    memset(aec->afYfPSD, 0, sizeof(float) * PART_LEN1);
    memset(aec->afEfPSD, 0, sizeof(float) * PART_LEN1);
    memset(aec->afYSaveBuf, 0, sizeof(float) * PART_LEN);
    memset(aec->afYfwBuf, 0, sizeof(float) * PART_LEN1 * 2);

    //debug
    memset(aec->d_save, 0, sizeof(float) * PART_LEN);
    memset(aec->asVadR, 0, sizeof(short) * kHistorySizeBlocks);
#endif
    aec->uiMediaCount = 0;

    return 0;
}

void WebRtcAec_BufferFarendPartition(AecCore* aec, const float* farend) {
    float fft[PART_LEN2];
    float xf[2][PART_LEN1];

#ifdef DEBUG_RECORD
    if(delay_nearFile){
        int64_t ltime = GetTimeInMS();
        fprintf(delay_nearFile,"%lld\tfill in FarEndBuf\n", ltime);
    }
#endif 

    // Check if the buffer is full, and in that case flush the oldest data.
    if (WebRtc_youme_available_write(aec->far_buf) < 1) {
        WebRtcAec_MoveFarReadPtr(aec, 1);
    }
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    if (WebRtc_youme_available_write(aec->far_buf_save) < 1) {
        WebRtcAec_MoveFarReadPtr(aec, 1);
    }
#endif
    // Convert far-end partition to the frequency domain without windowing.
    memcpy(fft, farend, sizeof(float) * PART_LEN2);
    TimeToFrequency(fft, xf, 0);
    WebRtc_youme_WriteBuffer(aec->far_buf, &xf[0][0], 1);

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG
    WebRtc_youme_WriteBuffer(aec->far_buf_save, &xf[0][0], 1);
#endif

    // Convert far-end partition to the frequency domain with windowing.
    memcpy(fft, farend, sizeof(float) * PART_LEN2);
    TimeToFrequency(fft, xf, 1);
    WebRtc_youme_WriteBuffer(aec->far_buf_windowed, &xf[0][0], 1);
}

void WebRtcAec_BufferFarendPartition_DelayEst(AecCore* aec, const float* farend) {
    float fft[PART_LEN2];
    float xf[2][PART_LEN1];

    // Convert far-end partition to the frequency domain without windowing.
    memcpy(fft, farend, sizeof(float) * PART_LEN2);
    TimeToFrequency(fft, xf, 0);
    WebRtc_youme_WriteBuffer(aec->media_buf, &xf[0][0], 1);

}

int WebRtcAec_MoveFarReadPtr(AecCore* aec, int elements) {
    int elements_moved = MoveFarReadPtrWithoutSystemDelayUpdate(aec, elements);
    aec->system_delay -= elements_moved * PART_LEN;
#ifdef DEBUG_RECORD
    if(delay_nearFile){
        fprintf(delay_nearFile,"system_delay reduced:%d\tnew system_delay:%d\n",(elements_moved * PART_LEN), aec->system_delay);
    }
#endif 
    return elements_moved;
}

void WebRtcAec_ProcessFrames(AecCore* aec,
                             const float* const* nearend,
                             size_t num_bands,
                             size_t num_samples,
                             int knownDelay,
                             int16_t msInSndCardBuf,
                             float* const* out) {
    size_t i, j;
    int out_elements = 0;

    aec->frame_count++;
    // For each frame the process is as follows:
    // 1) If the system_delay indicates on being too small for processing a
    //    frame we stuff the buffer with enough data for 10 ms.
    // 2 a) Adjust the buffer to the system delay, by moving the read pointer.
    //   b) Apply signal based delay correction, if we have detected poor AEC
    //    performance.
    // 3) TODO(bjornv): Investigate if we need to add this:
    //    If we can't move read pointer due to buffer size limitations we
    //    flush/stuff the buffer.
    // 4) Process as many partitions as possible.
    // 5) Update the |system_delay| with respect to a full frame of FRAME_LEN
    //    samples. Even though we will have data left to process (we work with
    //    partitions) we consider updating a whole frame, since that's the
    //    amount of data we input and output in audio_processing.
    // 6) Update the outputs.

    // The AEC has two different delay estimation algorithms built in.  The
    // first relies on delay input values from the user and the amount of
    // shifted buffer elements is controlled by |knownDelay|.  This delay will
    // give a guess on how much we need to shift far-end buffers to align with
    // the near-end signal.  The other delay estimation algorithm uses the
    // far- and near-end signals to find the offset between them.  This one
    // (called "signal delay") is then used to fine tune the alignment, or
    // simply compensate for errors in the system based one.
    // Note that the two algorithms operate independently.  Currently, we only
    // allow one algorithm to be turned on.

    assert(aec->num_bands == num_bands);

#ifdef DEBUG_RECORD
    if(delay_nearFile){
        fprintf(delay_nearFile,"msInSndCardBuf:%dms\n",msInSndCardBuf);
    }
#endif

    for (j = 0; j < num_samples; j += FRAME_LEN) {
        // TODO(bjornv): Change the near-end buffer handling to be the same as for
        // far-end, that is, with a near_pre_buf.
        // Buffer the near-end frame.
        WebRtc_youme_WriteBuffer(aec->nearFrBuf, &nearend[0][j], FRAME_LEN);
        // For H band
        for (i = 1; i < num_bands; ++i) {
            WebRtc_youme_WriteBuffer(aec->nearFrBufH[i - 1], &nearend[i][j], FRAME_LEN);
        }

        // 1) At most we process |aec->mult|+1 partitions in 10 ms. Make sure we
        // have enough far-end data for that by stuffing the buffer if the
        // |system_delay| indicates others.
        if (aec->system_delay < FRAME_LEN) {
#ifdef DEBUG_RECORD
            if(delay_nearFile){
                fprintf(delay_nearFile,"system_delay:%d\tsystem_delay added:%d\n",aec->system_delay, (aec->mult + 1));
            }
#endif
            // We don't have enough data so we rewind 10 ms.
            WebRtcAec_MoveFarReadPtr(aec, -(aec->mult + 1));
        }

        if (!aec->delay_agnostic_enabled) {
            // 2 a) Compensate for a possible change in the system delay.

            // TODO(bjornv): Investigate how we should round the delay difference;
            // right now we know that incoming |knownDelay| is underestimated when
            // it's less than |aec->knownDelay|. We therefore, round (-32) in that
            // direction. In the other direction, we don't have this situation, but
            // might flush one partition too little. This can cause non-causality,
            // which should be investigated. Maybe, allow for a non-symmetric
            // rounding, like -16.
            int move_elements = (aec->knownDelay - knownDelay - 32) / PART_LEN;           
            int moved_elements =
                MoveFarReadPtrWithoutSystemDelayUpdate(aec, move_elements);
#ifdef DEBUG_RECORD
            if(delay_nearFile){
                fprintf(delay_nearFile,"Delay Changed moved_elements:%d\n",moved_elements);
            }
#endif   
            aec->knownDelay -= moved_elements * PART_LEN;
        } else {
            // 2 b) Apply signal based delay correction.
            int move_elements = SignalBasedDelayCorrection(aec);
            int moved_elements =
                MoveFarReadPtrWithoutSystemDelayUpdate(aec, move_elements);
            int far_near_buffer_diff = WebRtc_youme_available_read(aec->far_buf) -
                                       WebRtc_youme_available_read(aec->nearFrBuf) / PART_LEN;
            WebRtc_SoftResetDelayEstimator(aec->delay_estimator, moved_elements);
            WebRtc_SoftResetDelayEstimatorFarend(aec->delay_estimator_farend,
                                                 moved_elements);
            aec->signal_delay_correction += moved_elements;
            // If we rely on reported system delay values only, a buffer underrun here
            // can never occur since we've taken care of that in 1) above.  Here, we
            // apply signal based delay correction and can therefore end up with
            // buffer underruns since the delay estimation can be wrong.  We therefore
            // stuff the buffer with enough elements if needed.
            if (far_near_buffer_diff < 0) {
                WebRtcAec_MoveFarReadPtr(aec, far_near_buffer_diff);
            }
        }

        // 4) Process as many blocks as possible.
        while (WebRtc_youme_available_read(aec->nearFrBuf) >= PART_LEN) {
            ProcessBlock(aec);
            EstEchoPathDelay(aec);
        }

        // 5) Update system delay with respect to the entire frame.
        aec->system_delay -= FRAME_LEN;
#ifdef DEBUG_RECORD
        if(delay_nearFile){
            fprintf(delay_nearFile,"after aec system_delay:%d\n",aec->system_delay);
        }
#endif  
        // 6) Update output frame.
        // Stuff the out buffer if we have less than a frame to output.
        // This should only happen for the first frame.
        out_elements = (int)WebRtc_youme_available_read(aec->outFrBuf);
        if (out_elements < FRAME_LEN) {
            WebRtc_youme_MoveReadPtr(aec->outFrBuf, out_elements - FRAME_LEN);
            for (i = 0; i < num_bands - 1; ++i) {
                WebRtc_youme_MoveReadPtr(aec->outFrBufH[i], out_elements - FRAME_LEN);
            }
        }
        // Obtain an output frame.
        WebRtc_youme_ReadBuffer(aec->outFrBuf, NULL, &out[0][j], FRAME_LEN);
        // For H bands.
        for (i = 1; i < num_bands; ++i) {
            WebRtc_youme_ReadBuffer(aec->outFrBufH[i - 1], NULL, &out[i][j], FRAME_LEN);
        }
    }
}

int WebRtcAec_GetDelayMetricsCore(AecCore* self, int* median, int* std,
                                  float* fraction_poor_delays) {
    assert(self != NULL);
    assert(median != NULL);
    assert(std != NULL);

    if (self->delay_logging_enabled == 0) {
        // Logging disabled.
        return -1;
    }

    if (self->delay_metrics_delivered == 0) {
        UpdateDelayMetrics(self);
        self->delay_metrics_delivered = 1;
    }
    *median = self->delay_median;
    *std = self->delay_std;
    *fraction_poor_delays = self->fraction_poor_delays;

    return 0;
}

int WebRtcAec_echo_state(AecCore* self) { return self->echoState; }

int WebRtcAec_media_echo_delay(AecCore* self) 
{
    int est_tmp = self->media_delay_est;
    int ilookahead = WebRtc_lookahead(self->delay_estimator);

    est_tmp = est_tmp - ilookahead;

    if (1 == self->mult)
    {
        return (est_tmp * 8); 
    }
    else
    {
        return (est_tmp * 4); 
    }
}

void WebRtcAec_GetEchoStats(AecCore* self,
                            Stats* erl,
                            Stats* erle,
                            Stats* a_nlp) {
    assert(erl != NULL);
    assert(erle != NULL);
    assert(a_nlp != NULL);
    *erl = self->erl;
    *erle = self->erle;
    *a_nlp = self->aNlp;
}

void* WebRtcAec_far_time_buf(AecCore* self) { return self->far_time_buf; }
void* WebRtcAec_media_time_buf(AecCore* self) { return self->media_time_buf; }

void WebRtcAec_SetConfigCore(AecCore* self,
                             int nlp_mode,
                             int metrics_mode,
                             int delay_logging) {
    assert(nlp_mode >= 0 && nlp_mode < 3);
    self->nlp_mode = nlp_mode;
    self->metricsMode = metrics_mode;
    if (self->metricsMode) {
        InitMetrics(self);
    }
    // Turn on delay logging if it is either set explicitly or if delay agnostic
    // AEC is enabled (which requires delay estimates).
    self->delay_logging_enabled = delay_logging || self->delay_agnostic_enabled;
    //if (self->delay_logging_enabled) {
    //  memset(self->delay_histogram, 0, sizeof(self->delay_histogram));
    //}
    memset(self->delay_histogram, 0, sizeof(self->delay_histogram));
}

void WebRtcAec_enable_delay_agnostic(AecCore* self, int enable) {
    self->delay_agnostic_enabled = enable;
}

int WebRtcAec_delay_agnostic_enabled(AecCore* self) {
    return self->delay_agnostic_enabled;
}

void WebRtcAec_enable_extended_filter(AecCore* self, int enable) {
    self->extended_filter_enabled = enable;
    self->num_partitions = enable ? kExtendedNumPartitions : kNormalNumPartitions;
    // Update the delay estimator with filter length.  See InitAEC() for details.
    WebRtc_set_allowed_offset(self->delay_estimator, self->num_partitions / 2);
}

int WebRtcAec_extended_filter_enabled(AecCore* self) {
    return self->extended_filter_enabled;
}

int WebRtcAec_system_delay(AecCore* self) { return self->system_delay; }

void WebRtcAec_SetSystemDelay(AecCore* self, int delay) {
    assert(delay >= 0);
#ifdef DEBUG_RECORD
    if(delay_nearFile){
        fprintf(delay_nearFile,"old system_delay:%d\tnew system_delay:%d\n",self->system_delay, delay);
    }
#endif
    self->system_delay = delay;
}


short WebRtcAec_TimeVAD(TimeVad_t *inst, float *pw16_outData, short pw16_len, uint32_t uiFrameCount) {

#define WEBRTC_SPL_MUL_16_16_RSFT_opt(a, b, c) \
    ((WEBRTC_SPL_MUL_16_16(a, b)+(2<<(c-1))) >> (c))

    int16_t i = 0;
    int16_t sTmp1, sTmp2;
    short sVADFlag = 0;// sVADCount = 0;
    int16_t sSamplesPerCall = pw16_len;//, sSNR_Threshold = 2560;
    double dTotalEnergy = 0;
    int16_t sLogEnergy = 0;
    double dTotalLogEnergy = 0;

    int16_t sCurFactor = 16384;//Q15
    int16_t sCurFactor1 = 128; //Q15
    int16_t sCurFactor2 = 16;  //Q15
    //int16_t sCurFactor3 = 1638;//Q15

    TimeVad_t *stt;
    stt = (TimeVad_t *) inst;

    for (i = 0; i < sSamplesPerCall; i++) {
        dTotalEnergy += pw16_outData[i] * pw16_outData[i];
    }
    dTotalEnergy = dTotalEnergy / sSamplesPerCall;
    if (0 == dTotalEnergy) {
        dTotalLogEnergy = 0;
    } else {
        dTotalLogEnergy = log10(dTotalEnergy);
    }
    sLogEnergy = (int16_t)(dTotalLogEnergy * 1024); //Q10

    sCurFactor = (stt->sLogEnergyCur < sLogEnergy) ? 32767 : 16384;
    sTmp1 = WEBRTC_SPL_MUL_16_16_RSFT_opt(stt->sLogEnergyCur, 32767 - sCurFactor, 15);
    sTmp2 = WEBRTC_SPL_MUL_16_16_RSFT_opt(sLogEnergy, sCurFactor, 15);
    stt->sLogEnergyCur = sTmp1 + sTmp2;

    sLogEnergy = stt->sLogEnergyCur;

    if (stt->sVADCount > 200) {
        sTmp1 = stt->sVADCount >> 7;
        sTmp1 = (sTmp1 > 3) ? 3 : sTmp1;
        sCurFactor2 = 16 << sTmp1;
    } else {
        sCurFactor2 = 16;
    }

    if (uiFrameCount < 100) {
        sCurFactor2 = 4096;
    }

    if (stt->sLogEnergyMin > sLogEnergy) {
        stt->sLogEnergyMin = sLogEnergy;
    } else {
        sTmp1 = WEBRTC_SPL_MUL_16_16_RSFT_opt(stt->sLogEnergyMin, 32767 - sCurFactor2, 15);
        sTmp2 = WEBRTC_SPL_MUL_16_16_RSFT_opt(sLogEnergy, sCurFactor2, 15);
        stt->sLogEnergyMin = sTmp1 + sTmp2;
    }

    if (stt->sLogEnergyMax < sLogEnergy) {
        stt->sLogEnergyMax = sLogEnergy;
    } else {
        sTmp1 = WEBRTC_SPL_MUL_16_16_RSFT_opt(stt->sLogEnergyMax, 32767 - sCurFactor1, 15);
        sTmp2 = WEBRTC_SPL_MUL_16_16_RSFT_opt(sLogEnergy, sCurFactor1, 15);
        stt->sLogEnergyMax = sTmp1 + sTmp2;
    }

    if ((stt->sLogEnergyMax - stt->sLogEnergyMin > 1024)
            && (stt->sLogEnergyCur - stt->sLogEnergyMin > 1024)) {
        sVADFlag = 1;
    } else {
        sVADFlag = 0;
    }

    if (stt->sLogEnergyCur < 2999) {
        sVADFlag = 0;
    }

    if (0 < sVADFlag) {
        stt->sVADCount++;
        stt->sNoiseCount = 0;
    } else {
        stt->sVADCount = 0;
        stt->sNoiseCount++;
    }

    stt->sVADCount = (stt->sVADCount > 32767) ? 32767 : stt->sVADCount;
    stt->sNoiseCount = (stt->sNoiseCount > 32767) ? 32767 : stt->sNoiseCount;

    return sVADFlag;
}
#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_NLP
short WebRtcAec_DTDetection(AecCore* aec, float hNl[PART_LEN1]) {
    int i = 0, iNum = 0, iStartNum = 0, iEndNum = PART_LEN1;
    float fAvgHnlLow = 0, fAvgHnlEcho = 0, fAvgHnlHigh = 0;
    int iHzPerNum = 4000 * aec->mult / PART_LEN;
    float ftmp;

    //1000~2000Hz
    iNum = 0;
    iStartNum = 1000 / iHzPerNum;
    iEndNum = 2000 / iHzPerNum + 1;
    for (i = iStartNum; i < iEndNum; i++) {
        fAvgHnlEcho += hNl[i];
        iNum++;
    }
    fAvgHnlEcho = fAvgHnlEcho / iNum;

    //0~500Hz
    iNum = 0;
    iStartNum = 0;
    iEndNum = 500 / iHzPerNum + 1;
    for (i = iStartNum; i < iEndNum; i++) {
        fAvgHnlLow += hNl[i];
        iNum++;
    }
    fAvgHnlLow = fAvgHnlLow / iNum;

    ftmp = fAvgHnlLow - fAvgHnlEcho;
    ftmp = ftmp > 0 ? ftmp : 0;

    if (aec->fLowHnlSm < ftmp) {
        aec->fLowHnlSm = aec->fLowHnlSm * 0.9 + ftmp * 0.1;
    } else {
        aec->fLowHnlSm = aec->fLowHnlSm * 0.95 + ftmp * 0.05;
    }
    aec->fLowHnlSm = aec->fLowHnlSm > 0.3 ? 0.3 : aec->fLowHnlSm;

    //2500~4000Hz
    iNum = 0;
    iStartNum = 2500 / iHzPerNum;
    iEndNum = 4000 / iHzPerNum + 1;
    for (i = iStartNum; i < iEndNum; i++) {
        fAvgHnlHigh += hNl[i];
        iNum++;
    }
    fAvgHnlHigh = fAvgHnlHigh / iNum;

    ftmp = fAvgHnlHigh - fAvgHnlEcho;
    ftmp = ftmp > 0 ? ftmp : 0;

    if (aec->fHighHnlSm < ftmp) {
        aec->fHighHnlSm = aec->fHighHnlSm * 0.9 + ftmp * 0.1;
    } else {
        aec->fHighHnlSm = aec->fHighHnlSm * 0.99 + ftmp * 0.01;
    }
    aec->fHighHnlSm = aec->fHighHnlSm > 0.3 ? 0.3 : aec->fHighHnlSm;

    if (aec->fHighHnlSm > 0.1 && aec->fLowHnlSm > 0.1) {
        aec->dt_flag = 16384;
    } else {
        aec->dt_flag = 0;
    }

    return 0;
}

static void PSDSm_For_ERL_Estimation(AecCore* aec,
                        float efw[2][PART_LEN1],
                        float dfw[2][PART_LEN1],
                        float xfw[2][PART_LEN1])
{
    int i = 0;
    float dfTmp, efTmp, xfTmp, yfTmp;
    float ef_df_compare = 0;
    float fSmStep = 0.5, fTmp;

    for (i = 0; i < PART_LEN1; i++) {
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

    if (aec->filter_reseted > 20)
    {
      //memset(aec->wfBuf, 0, sizeof(aec->wfBuf));
    }

};

static void WebRtcAec_ERL_Estimation(AecCore* aec, float hNl[PART_LEN1], float efw[2][PART_LEN1]) {

    int i = 0;
    float fERLtmp = 0, efTmp = 0, echoTmp = 0;
    float afERL[PART_LEN1] = {0};
    
    if ((aec->fHnlVoiceAvg < 0.25) && (aec->time_VadS.sVADCount > 0)) {
        for (i = 0; i < PART_LEN1; i++) {
            if (hNl[i] < 0.0075) {
                efTmp = efw[0][i] * efw[0][i] + efw[1][i] * efw[1][i];
                echoTmp = aec->afDfPSD[i] - efTmp;
                echoTmp = echoTmp > 0 ? echoTmp : 0;
                fERLtmp = echoTmp/WEBRTC_SPL_MAX(aec->afXfPSD[i], WebRtcAec_kMinFarendPSD);
#ifndef ANDROID
                fERLtmp = fERLtmp > 1024 ? 1024 : fERLtmp;
#endif
                fERLtmp = (fERLtmp < (1.0/1024.0)) ? (1.0/1024.0) : fERLtmp;
                if (fERLtmp > aec->afERLSm[i]) {
                    aec->afERLSm[i] = aec->afERLSm[i] * 0.8 + fERLtmp * 0.2;
                }else{
                    aec->afERLSm[i] = aec->afERLSm[i] * 0.995 + fERLtmp * 0.005;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < PART_LEN1; i++) {
            aec->afERLSm[i] = aec->afERLSm[i] * 1.0003;
#ifndef ANDROID
            aec->afERLSm[i] = aec->afERLSm[i] > 1024 ? 1024 : aec->afERLSm[i];
#endif
        }
    }

}

static void WebRtcAec_ERL_EstHnl (AecCore* aec, float hNl[PART_LEN1]) {

    int i = 0;
    float fReEcho, fEstEcho, fErlTmp = 1;
    float hNlErl[PART_LEN1] = {0}, hNlErlTmp;
    float hNlErlSave[PART_LEN1] = {0};
    float afHnlAvg = 0;
    int sStartNum, sEndNum, sSBNum;

    short asSubBand[kSubBandNum + 1] = {0, 5, 9, 13, 17, 25, 37, 49, 65};

    if (aec->iBeginOfEcho)
    {
        fErlTmp = 4;
    }

    int iNumTmp = aec->mult > 1 ? 6 : 8;
    for (i = 0; i < PART_LEN1; i++) {
        fErlTmp = (i > asSubBand[iNumTmp]) ? (2 * fErlTmp) : fErlTmp;
        fEstEcho = aec->afXfPSD[i] * aec->afERLSm[i] * fErlTmp;
        fReEcho = fEstEcho - aec->afYfPSD[i];
        fReEcho = fReEcho > 0 ? fReEcho : 0;
        aec->afEfPSD[i] = aec->afEfPSD[i] > 225 ? aec->afEfPSD[i] : 225;
        hNlErlTmp = fReEcho / aec->afEfPSD[i];
        hNlErlTmp = hNlErlTmp > 1 ? 1 : hNlErlTmp;
        hNlErl[i] = 1 - hNlErlTmp;
        hNlErlSave[i] = hNlErl[i];
    }

    for (i = 1; i < PART_LEN; i++)
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

void WebRtcAec_BeginOfEchoDetection (AecCore* aec) {
    
    int ilookahead = WebRtc_lookahead(aec->delay_estimator);
    int iVadR = aec->asVadR[aec->delay_est - ilookahead];

    if (iVadR > 0)
    {
        if (aec->iNoiseRCount >= 20)
        {
            aec->iNoiseRCountOld = aec->iNoiseRCount;
        }
        aec->iVadRCount++;
        aec->iNoiseRCount = 0;
        if (aec->iVadRCount > 25)
        {
            aec->iNoiseRCountOld = 0;
        }
    }
    else
    {
        aec->iVadRCount = 0;
        aec->iNoiseRCount++;
    }

    aec->iBeginOfEcho = 0;
    if ((aec->iVadRCount > 0) && (aec->iNoiseRCountOld > 30))
    {
        aec->iBeginOfEcho = 1;
    }   
}

#endif

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
int WebRtcAec_FarPlayOutDelta(AecCore* self) { return (int)self->FarPlayOutDelta; }
#endif

#ifdef DEBUG_RECORD
void WebRtcAec_start_recording_c(AecCore* const aec, char * file)
{
    char path[512];
    if (0 == aec->file_opened_flag)
    {
        if(farFile)
        {
            fclose(farFile);farFile = NULL;
        }
        snprintf(path, 512,"%s_r2_far.pcm", file);
        farFile = fopen(path, "wb+");

        if(noisy_nearFile)
        {
            fclose(noisy_nearFile);noisy_nearFile = NULL;
        }
        snprintf(path, 512, "%s_s2_noisy_near.pcm", file);
        noisy_nearFile = fopen(path, "wb+");
#if 0
        if(delay_nearFile)
        {
            fclose(delay_nearFile);delay_nearFile = NULL;
        }
        //snprintf(path, 512, "%s_r2_echo_delay.pcm", file);
        //delay_nearFile = fopen(path, "wb+");
        snprintf(path, 512, "%s_r2_far_moved.pcm", file);
        delay_nearFile = fopen(path, "w");
#endif
        if(offset_nearFile)
        {
            fclose(offset_nearFile);offset_nearFile = NULL;
        }
        snprintf(path, 512, "%s_r2_offset.pcm", file);
        offset_nearFile = fopen(path, "wb+");

        aec->file_opened_flag = 1;
    }
}

void WebRtcAec_RecordingToFile_c(AecCore* const aec, int16_t record_flag)
{
    int16_t tmp_ref_read_pos = aec->aec_ref_read_pos;
    aec->aec_ref_read_pos = aec->aec_ref_write_pos;

    int16_t tmp_sin_read_pos = aec->aec_sin_read_pos;
    aec->aec_sin_read_pos = aec->aec_sin_write_pos;

    int16_t tmp_offset_read_pos = aec->aec_offset_read_pos;
    aec->aec_offset_read_pos = aec->aec_offset_write_pos;

    if (1 == aec->file_opened_flag)
    {
        int tmp_ref_read_offset = tmp_ref_read_pos * PART_LEN;
        int tmp_ref_read_num = (aec->aec_ref_read_pos - tmp_ref_read_pos) * PART_LEN;
        if (aec->aec_ref_read_pos < tmp_ref_read_pos)
        {
            tmp_ref_read_num = (kHistorySizeBlocks * 2 - tmp_ref_read_pos) * PART_LEN;
            memcpy(aec->aec_ref_data_tmp, aec->aec_ref_data + tmp_ref_read_offset, tmp_ref_read_num * sizeof(int16_t));

            int tmp_ref_read_num2 = aec->aec_ref_read_pos * PART_LEN;
            memcpy(aec->aec_ref_data_tmp + tmp_ref_read_num, aec->aec_ref_data, tmp_ref_read_num2 * sizeof(int16_t));

            tmp_ref_read_num = tmp_ref_read_num + tmp_ref_read_num2;
        }
        else
        {
            memcpy(aec->aec_ref_data_tmp, aec->aec_ref_data + tmp_ref_read_offset, tmp_ref_read_num  * sizeof(int16_t));
        }

        int tmp_sin_read_offset = tmp_sin_read_pos * PART_LEN;
        int tmp_sin_read_num = (aec->aec_sin_read_pos - tmp_sin_read_pos) * PART_LEN;
        if (aec->aec_sin_read_pos < tmp_sin_read_pos)
        {
            tmp_sin_read_num = (kHistorySizeBlocks * 2 - tmp_sin_read_pos) * PART_LEN;
            memcpy(aec->aec_sin_data_tmp, aec->aec_sin_data + tmp_sin_read_offset, tmp_sin_read_num * sizeof(int16_t));

            int tmp_sin_read_num2 = aec->aec_sin_read_pos * PART_LEN;
            memcpy(aec->aec_sin_data_tmp + tmp_sin_read_num, aec->aec_sin_data, tmp_sin_read_num2 * sizeof(int16_t));

            tmp_sin_read_num = tmp_sin_read_num + tmp_sin_read_num2;
        }
        else
        {
            memcpy(aec->aec_sin_data_tmp, aec->aec_sin_data + tmp_sin_read_offset, tmp_sin_read_num * sizeof(int16_t));
        }

        int tmp_offset_read_offset = tmp_offset_read_pos * PART_LEN2;
        int tmp_offset_read_num = (aec->aec_offset_read_pos - tmp_offset_read_pos) * PART_LEN2;
        if (aec->aec_offset_read_pos < tmp_offset_read_pos)
        {
            tmp_offset_read_num = (kHistorySizeBlocks * 2 - tmp_offset_read_pos) * PART_LEN2;
            memcpy(aec->aec_offset_data_tmp, aec->aec_offset_data + tmp_offset_read_offset, tmp_offset_read_num  * sizeof(int16_t));

            int tmp_offset_read_num2 = aec->aec_offset_read_pos * PART_LEN2;
            memcpy(aec->aec_offset_data_tmp + tmp_offset_read_num, aec->aec_offset_data, tmp_offset_read_num2 * sizeof(int16_t));

            tmp_offset_read_num = tmp_offset_read_num + tmp_offset_read_num2;
        }
        else
        {
            memcpy(aec->aec_offset_data_tmp, aec->aec_offset_data + tmp_offset_read_offset, tmp_offset_read_num * sizeof(int16_t));
        }

        if ((3 == record_flag || -1 == record_flag) && farFile && tmp_ref_read_num)
        {
            (void)fwrite(aec->aec_ref_data_tmp, sizeof(int16_t), tmp_ref_read_num, farFile);
        }
        if ((4 == record_flag || -1 == record_flag) && noisy_nearFile && tmp_sin_read_num)
        {
            (void)fwrite(aec->aec_sin_data_tmp, sizeof(int16_t), tmp_sin_read_num, noisy_nearFile);
        }
        if ((5 == record_flag || -1 == record_flag) && offset_nearFile && tmp_offset_read_num)
        {
            (void)fwrite(aec->aec_offset_data_tmp, sizeof(int16_t), tmp_offset_read_num, offset_nearFile);
        }
    }
}

void WebRtcAec_stop_recording_c(AecCore* const aec)
{
    if(delay_nearFile)
    {
        fclose(delay_nearFile);delay_nearFile = NULL;
    }

    if(offset_nearFile)
    {
        fclose(offset_nearFile);offset_nearFile = NULL;
    }

    if(noisy_nearFile)
    {
        fclose(noisy_nearFile);noisy_nearFile = NULL;
    }

    if(farFile)
    {
        fclose(farFile);farFile = NULL;
    }
    aec->file_opened_flag = 0;
}
#endif

//ProcessBlock
static void EstEchoPathDelay(AecCore* aec) {
    
    float abs_media_spectrum[PART_LEN1];
    float media[PART_LEN], mediaend[PART_LEN];
    float* media_ptr = media;
    float aMediaf_ptr[2*PART_LEN1];
    float* mediaf_ptr = aMediaf_ptr;
    float xf[2][PART_LEN1];
    float media_spectrum = 0.0f;

    short sMediaVadFlag = 0, i = 0;

    aec->uiMediaCount++;
    size_t read_count = WebRtc_youme_ReadBuffer(aec->media_time_buf, (void**)&media_ptr, mediaend, 1);
    
    //read_count<=0表示没有可读取的数据。无需处理
    if(read_count<=0)
    {
        return;
    }

    sMediaVadFlag = WebRtcAec_TimeVAD(&(aec->time_VadMedia), media_ptr, PART_LEN, aec->uiMediaCount);
    memmove(&(aec->asVadMedia[1]), &(aec->asVadMedia[0]), (kHistorySizeBlocks-1) * sizeof(short));
    aec->asVadMedia[0] = sMediaVadFlag;

    WebRtc_youme_ReadBuffer(aec->media_buf, (void**)&mediaf_ptr, &xf[0][0], 1);

    for (i = 0; i < PART_LEN1; i++) {
        // Calculate absolute spectra
        media_spectrum = (mediaf_ptr[i] * mediaf_ptr[i]) +
                       (mediaf_ptr[PART_LEN1 + i] * mediaf_ptr[PART_LEN1 + i]);
        abs_media_spectrum[i] = sqrtf(media_spectrum);
    }

#ifdef WEBRTC_AEC_TO_PHONE_DEBUG_DELAY
    sMediaVadFlag = 1;
    if (sMediaVadFlag) {
        if (WebRtc_AddFarSpectrumFloat(
                    aec->delay_estimator_media, abs_media_spectrum, PART_LEN1, aec->mult) == 0) 
        {
            int delay_estimate = WebRtc_DelayEstimatorProcessFloat_media(
                                     aec->delay_estimator, aec->mult, aec->asVadMedia);
            if (delay_estimate >= 0) {
                aec->media_delay_est = delay_estimate;
            }
        }
    }
#endif

}
    }  // namespace webrtc_new
} //namespace youme




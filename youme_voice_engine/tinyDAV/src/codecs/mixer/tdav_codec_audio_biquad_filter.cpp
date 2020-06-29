//
//  tdav_codec_audio_biquad_filter.cpp
//  youme_voice_engine
//
//  Created by paul on 30/09/16.
//  Copyright © 2016 youme. All rights reserved.
//

#include "tsk_memory.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_biquad_filter.h"
#include "tsk_debug.h"
#include "tsk_string.h"
#include <math.h>
#include "../../audio/stdinc.h"

/**
 * pi2Coeffs:     Coefficients result
 * i2Fs:          Sample rate
 * i2Fc:          Cut off frequency
 * i2dBGain:      dB gain in peq, 1dB per step
 * i2QValue:      Q value
 * 2FilterType:   hpf/lpf/peq support
 */
void vBiqTblCreate(int32_t *pi4Coeffs, int16_t i2Fs, int16_t i2Fc, int16_t i2dBGain, double Q, int16_t *pi2Scale, FILTER_TYPE eFilterType)
{
    int16_t A;
    int32_t Cs, Sn;
    int16_t fIndex;
    int32_t Alpha;
    int32_t b0,b1,b2,a0,a1,a2;
    int16_t i2Scale, i2extScale;
    
    if (i2Fc > 8000) {
        TSK_DEBUG_ERROR("FC(%d) > Fs/2(8000)", i2Fc);
        return;
    }
    // 1. Calculate the A=10^(dBGain/40)
    A = dBGainTbl[i2dBGain + MAXDB]; // Q11
    // 2. Calculate the cos(2*pi*fc/fs) & sin(2*pi*fc/fs)
    fIndex = (int16_t)(i2Fc / 10);
    Cs = (int32_t)(CosTbl[fIndex]) >> 1; // Q29
    Sn = (int32_t)(SinTbl[fIndex]) >> 1; // Q29
    // 3. Calculate the alpha=sn/(2*Q)
    double fQTmp;
    fQTmp = (double)(Sn >> 1) / (Q * Q29_ONE_1);
    Alpha = (int32_t)(fQTmp * Q29_ONE_1); // Q29
    // 4. Coefficient calculation
    switch (eFilterType) {
        case BIQUAD_LPF:
            i2Scale = 29;
            b0 = (Q29_ONE - Cs) >> 1;
            b1 = Q29_ONE - Cs;
            b2 = b0;
            a0 = Q29_ONE + Alpha;
            a1 = -(Cs << 1);
            a2 = Q29_ONE - Alpha;
            break;
        case BIQUAD_HPF:
            i2Scale = 29;
            b0 = (Q29_ONE + Cs) >> 1;
            b1 = -(Q29_ONE + Cs);
            b2 = b0;
            a0 = Q29_ONE + Alpha;
            a1 = -(Cs << 1);
            a2 = Q29_ONE - Alpha;
            break;
        case BIQUAD_PEQ:
            i2extScale = 2;
            i2Scale = 29 - i2extScale;
            b0 = (int32_t)(((int64_t)Alpha * A) >> (11 + i2extScale)); // Q27
            b0 = (Q29_ONE >> i2extScale) + b0; // Q27
            b1 = -Cs >> (i2extScale - 1); // Q27
            b2 = (int32_t)(((int64_t)Alpha * A) >> (11 + i2extScale)); // Q27
            b2 = (Q29_ONE >> i2extScale) - b2; // Q27
            a0 = (int32_t)((int64_t)Alpha * (Q11_ONE_1 >> i2extScale) / A); // Q27
            a0 = (Q29_ONE >> i2extScale) + a0; // Q27
            a1 = -Cs >> (i2extScale - 1); // Q27
            a2 = (int16_t)((int64_t)Alpha * (Q11_ONE_1 >> i2extScale) / A); // Q27
            a2 = (Q29_ONE >> i2extScale) - a2; // Q27
            break;
        default:
            break;
    }
    // 5. Normalize
    b0 = (int32_t)((int64_t)b0 * (int64_t)(0x1 << (i2Scale)) / a0);
    b1 = (int32_t)((int64_t)b1 * (int64_t)(0x1 << (i2Scale)) / a0);
    b2 = (int32_t)((int64_t)b2 * (int64_t)(0x1 << (i2Scale)) / a0);
    a1 = (int32_t)((int64_t)a1 * (int64_t)(0x1 << (i2Scale)) / a0);
    a2 = (int32_t)((int64_t)a2 * (int64_t)(0x1 << (i2Scale)) / a0);
    // 6. Output
    pi4Coeffs[0] = b0;
    pi4Coeffs[1] = b1;
    pi4Coeffs[2] = b2;
    pi4Coeffs[3] = a1;
    pi4Coeffs[4] = a2;
    
    *pi2Scale = i2Scale;
}

/**
 * pi2InData:     In data
 * pi2History:    History data
 * pi2Coeffs:     Coefficients result
 */
void vBiqFilterProc(int16_t *pi2InData, int16_t  *pi2History, int32_t *pi4Coeffs, int16_t i2Scale, int16_t i2Size)
{
    int i;
    int16_t x1, x2, y1, y2, x;
    int64_t y, y111, y222, y333, y444, y555;
    int headroom = 0;
    
    if (!pi2InData || !pi2History || !pi4Coeffs) {
        return;
    }
    
    for (i = 0; i < i2Size; i++) {
        // 1. Load history
        x1 = pi2History[0];
        x2 = pi2History[1];
        y1 = pi2History[2];
        y2 = pi2History[3];
        // 2. Filter data
        x = pi2InData[i] >> headroom;
        y111 = (int64_t)x*pi4Coeffs[0];
        y222 = (int64_t)x1*pi4Coeffs[1];
        y333 = (int64_t)x2*pi4Coeffs[2];
        y444 = (int64_t)y1*pi4Coeffs[3];
        y555 = (int64_t)y2*pi4Coeffs[4];
        y = y111 + y222 + y333 - y444 - y555;
        y += (1 << (i2Scale - 1));
        y >>= (i2Scale - headroom);
        if (y > 32767) {
            y = 32767;
        } else if (y < -32768) {
            y = -32768;
        }
        pi2InData[i] = (int16_t)y;
        // 3. Save history(with headroom)
        pi2History[0] = x;
        pi2History[1] = x1;
        pi2History[2] = pi2InData[i] >> headroom;
        pi2History[3] = y1;
    }
}

/**
 * pi2InData:     In data
 * pi2History:    History data
 * pi2Coeffs:     Coefficients result
 */
void vBiqFilterProcFloat(int16_t *pi2InData, float *pfHistory, int32_t *pi4Coeffs, int16_t i2Scale, int16_t i2Size)
{
    int i;
    int16_t x;
    float x1, x2, y1, y2;
    double y, y111, y222, y333, y444, y555, z;
    double fScale = powf(2.0, i2Scale);
    
    if (!pi2InData || !pfHistory || !pi4Coeffs) {
        return;
    }
    
    for (i = 0; i < i2Size; i++) {
        // 1. Load history
        x1 = pfHistory[0];
        x2 = pfHistory[1];
        y1 = pfHistory[2];
        y2 = pfHistory[3];
        // 2. Filter data
        x = pi2InData[i];
        y111 = (double)x * ((double)pi4Coeffs[0] / fScale);
        y222 = (double)x1 * ((double)pi4Coeffs[1] / fScale);
        y333 = (double)x2 * ((double)pi4Coeffs[2] / fScale);
        y444 = (double)y1 * ((double)pi4Coeffs[3] / fScale);
        y555 = (double)y2 * ((double)pi4Coeffs[4] / fScale);
        z = y111 + y222 + y333 - y444 - y555;
        if (z > 32767.0f) {
            y = 32767.0f;
        } else if (z < -32768.0f) {
            y = -32768.0f;
        } else {
            if (z > 0.0f)
                y = z + 0.5f;
            else
                y = z - 0.5f;
        }
        pi2InData[i] = (int16_t)y;
        // 3. Save history(with headroom)
        pfHistory[0] = (float)x;
        pfHistory[1] = x1;
        pfHistory[2] = (float)z;
        pfHistory[3] = y1;
    }
}

/**
 * reverb_filter_t: Instance
 * pi2InData:       Input Data
 */
void vReverbFilterProc(void *pReverbInst, int16_t *pi2InData, int16_t i2Size)
{
    reverb_filter_t *pInst = (reverb_filter_t *)pReverbInst;
    int i, k;
    int16_t i2TempData, i2TempDataExt;
    int16_t i2Data = 0;
    int16_t i2OutData = 0;
    int32_t i4Data = 0;
    for (i = 0; i < i2Size; i++) {
        // Pre-Gain
        i4Data = ((int32_t)pi2InData[i] * (int32_t)i2FixGain) << 1;
        i2Data = (int16_t)(i4Data >> REVERB_Q);
        i2OutData = 0;
        // 8-set comb filters processing
        for (k = 0; k < REVERB_COMB_FILT_NUM; k++) {
            i2TempData = *(pInst->pi2CombBuf[k] + pInst->i2CombHistoryIdx[k]);
            i4Data = (int32_t)i2TempData * (int32_t)i2Damp2 + (int32_t)(pInst->i2CombHistoryData[k]) * (int32_t)i2Damp1; // No saturion because Damp1/2 is < 1
            pInst->i2CombHistoryData[k] = (int16_t)(i4Data >> REVERB_Q);
            i4Data = (int32_t)(pInst->i2CombHistoryData[k]) * (int32_t)i2FeedBack;
            *(pInst->pi2CombBuf[k] + pInst->i2CombHistoryIdx[k]) = i2Data + (int16_t)(i4Data >> REVERB_Q);
            pInst->i2CombHistoryIdx[k] += 1;
            if (pInst->i2CombHistoryIdx[k] >= pInst->i2ConstComDelayTbl[k]) {
                pInst->i2CombHistoryIdx[k] = 0;
            }
            i2OutData += i2TempData;
        }
        // 4-set allpass filters processing
        for (k = 0; k < REVERB_ALLPASS_FILT_NUM; k++) {
            i2TempDataExt = *(pInst->pi2AllpassBuf[k] + pInst->i2AllpassHistoryIdx[k]);
            i4Data = (int32_t)i2OutData * (int32_t)i2FeedBackAllpass;
            i2TempData = i2TempDataExt - (int16_t)(i4Data >> REVERB_Q);
            i4Data = (int32_t)i2TempDataExt * (int32_t)i2FeedBackAllpass;
            *(pInst->pi2AllpassBuf[k] + pInst->i2AllpassHistoryIdx[k]) = i2OutData + (int16_t)(i4Data >> REVERB_Q);
            pInst->i2AllpassHistoryIdx[k] += 1;
            if (pInst->i2AllpassHistoryIdx[k] >= pInst->i2ConstAllpassDelayTbl[k]) {
                pInst->i2AllpassHistoryIdx[k] = 0;
            }
            i2OutData = i2TempData;
        }
        i4Data = ((int32_t)i2OutData * (int32_t)i2Wet) + ((int32_t)pi2InData[i] * (int32_t)i2Dry);
        pi2InData[i] = (int16_t)(i4Data >> REVERB_Q);
    }
}

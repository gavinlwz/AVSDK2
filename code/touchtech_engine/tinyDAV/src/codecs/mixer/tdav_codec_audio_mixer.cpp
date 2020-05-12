//
//  tdav_codec_audio_mixer.cpp
//  youme_voice_engine
//
//  Created by peter on 6/25/16.
//  Copyright © 2016 youme. All rights reserved.
//

#include "tsk_memory.h"
// this should be the last include
#include "tinydav/codecs/mixer/tdav_codec_audio_mixer.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h" // Use downsample open source function
#include "tsk_debug.h"
#include "../../audio/stdinc.h"

// 论文:
// http://wenku.baidu.com/view/884624d97f1922791688e85b.html

typedef struct
{
    int Rsh;
    int K;
} TABLE;

// N取2的整数次幂. N越大, 高强度信号越失真. 推荐取 8或16
#define ITEMS 5
const static TABLE table[ITEMS] = {// N=8
    {3, 0},
    {6, 28672},
    {9, 32256},
    {12, 32704},
    {15, 32760}
};
/*
 const static TABLE table[ITEMS] = {// N=16
 {3, 0},
 {6, 30720},
 {9, 32640},
 {12, 32760},
 {15, 32767}
 };
 */
#define MIN(m, n) ((m < n)?(m):(n))
#define MAX(m, n) ((m > n)?(m):(n))
#define ABS(n) ((n < 0)?(-n):(n))
#define Q 16
#define MAXS16 (0x7FFF)
#define SGN(n) ((n < 0) ? (-1) : (1))

/**
 * short* in     Sample 输入流数组
 * int in_tracks 输入流音轨数 (Sample输入流数组长度)
 * return        混音后的结果数值, short 型
 */
static inline int16_t do_mix(int16_t * in, size_t in_tracks)
{
    long total = 0;
    long n, c, d;
    for (; in_tracks;) {
        total += in[--in_tracks];
    }
    n = MIN((ABS(total) >> (Q - 1)), ITEMS - 1);
    c = ABS(total) & MAXS16;
    d = (c << 2) + (c << 1) + c;
    return SGN(total) * ((d >> table[n].Rsh) + table[n].K);
}

int16_t do_mixing(int16_t * in, size_t in_tracks) {
    return do_mix(in, in_tracks);
}

void tdav_codec_mixer_mix16(const AudioMixMeta* pInputPtr, const size_t nTrackCountTotal, int16_t* pOutBufferPtr, const size_t nMixSampleCountMax)
{
    size_t nPos = 0, nTrackIdx = 0, nTrackCountToMix = 0;
    int16_t* pInBufferPtr = (int16_t*)tsk_malloc( sizeof(int16_t) * nTrackCountTotal);
    AudioMixMeta meta;
    for (;nPos < nMixSampleCountMax; nPos++) {
        nTrackIdx = 0;
        nTrackCountToMix = 0;
        for (;nTrackIdx < nTrackCountTotal; nTrackIdx++ ) {
            meta = pInputPtr[nTrackIdx];
            if (nPos >= meta.sampleCount) {
                continue;
            }
            pInBufferPtr[nTrackCountToMix++] = meta.bufferPtr[nPos];
        }
        pOutBufferPtr[nPos] = do_mix(pInBufferPtr, nTrackCountToMix);
    }
    
    tsk_free((void**) &pInBufferPtr);
}

void tdav_codec_mixer_normal(int16_t* pInBuf1, int16_t* pInBuf2, int16_t* pOutBuf, size_t nMixSampleCountMax)
{
    int32_t sum = 0;
    int32_t n, c, d;
    
    if (!pInBuf1 || !pInBuf2 || !pOutBuf) {
        TSK_DEBUG_ERROR("Audio normal mixer error");
        return;
    }
    
    for (int nPos = 0; nPos < nMixSampleCountMax; nPos += 2) {
        sum = *(pInBuf1 + nPos) + *(pInBuf2 + nPos);
        n   = MIN((ABS(sum) >> (Q - 1)), ITEMS - 1);
        c   = ABS(sum) & MAXS16;
        d   = (c << 2) + (c << 1) + c;
        *(pOutBuf + nPos) = SGN(sum) * ((d >> table[n].Rsh) + table[n].K);
        
        sum = *(pInBuf1 + nPos + 1) + *(pInBuf2 + nPos + 1);
        n   = MIN((ABS(sum) >> (Q - 1)), ITEMS - 1);
        c   = ABS(sum) & MAXS16;
        d   = (c << 2) + (c << 1) + c;
        *(pOutBuf + nPos + 1) = SGN(sum) * ((d >> table[n].Rsh) + table[n].K);
    }
}

#define RESMPL_HISTORY_BLK 8
#define K44TO32(X) X/8
#define K32TO22(X) X/11
#define K32TO24(X) X/3
#define K48TO32(X) X/2

const int16_t BIQ_HPF_FILTER_TBL_16K[5][5] = { // 2nd order IIR high pass filter table, Q13, {B[0],B[1],B[2],A[1],A[2]}
    {8012, -16024, 8012, -16020, 7836}, //  80 Hz
    {7749, -15499, 7749, -15475, 7331}, // 200 Hz
    {7537, -15074, 7537, -15022, 6935}, // 300 Hz
    {7130, -14259, 7130, -14121, 6206}, // 500 Hz
    {6202, -12404, 6202, -11913, 4702}, //1000 Hz
};

const int16_t BIQ_LPF_FILTER_TBL_16K[5][5] = { // 2nd order IIR low pass filter table, Q13, {B[0],B[1],B[2],A[1],A[2]}
    {69, 138, 69, -14121, 6206},        // 500 Hz
    {164, 329, 164, -12788, 5254},      // 800 Hz
    {245, 490, 245, -11913, 4702},      //1000 Hz
    {799, 15995, 799, -7723, 2730},     //2000 Hz
    {3425, 6850, 3425, 3792, 1717},     //5000 Hz
};

void tdav_codec_iir_base16K (int16_t*pi2InputBuf, int16_t* pi2OutputBuf, const int16_t *pCoeff, const size_t nInByteNum)
{
    static int16_t i2History1[2][2] = {0};
    static int16_t i2History2[2][2] = {0};
    const  int16_t *i2Coeffs = pCoeff;
    
    int16_t x1, x2, y1, y2, x;
    int32_t y;
    
    if (!pi2InputBuf || !pi2OutputBuf || !pCoeff) {
        return;
    }
    
    for (int i = 0; i < nInByteNum / 2; i++) {
        // Load history
        x1 = i2History1[0][0];
        x2 = i2History1[0][1];
        y1 = i2History1[1][0];
        y2 = i2History1[1][1];
        // Filter data
        x = pi2InputBuf[i];
        y = (int32_t)x*i2Coeffs[0] + (int32_t)x1*i2Coeffs[1] + (int32_t)x2*i2Coeffs[2] - (int32_t)y1*i2Coeffs[3] - (int32_t)y2*i2Coeffs[4];
        pi2InputBuf[i] = (int16_t)(y >> 13);
        // Save history
        i2History1[0][0] = x;
        i2History1[0][1] = x1;
        i2History1[1][0] = pi2InputBuf[i];
        i2History1[1][1] = y1;
    }
    
    for (int i = 0; i < nInByteNum / 2; i++) {
        // Load history
        x1 = i2History2[0][0];
        x2 = i2History2[0][1];
        y1 = i2History2[1][0];
        y2 = i2History2[1][1];
        // Filter data
        x = pi2InputBuf[i];
        y = (int32_t)x*i2Coeffs[0] + (int32_t)x1*i2Coeffs[1] + (int32_t)x2*i2Coeffs[2] - (int32_t)y1*i2Coeffs[3] - (int32_t)y2*i2Coeffs[4];
        pi2OutputBuf[i] = (int16_t)(y >> 13);
        // Save history
        i2History2[0][0] = x;
        i2History2[0][1] = x1;
        i2History2[1][0] = pi2OutputBuf[i];
        i2History2[1][1] = y1;
    }
}

tsk_bool_t tdav_codec_detect_audio(int16_t* pi2InputBuf, const size_t nInByteNum)
{
    tsk_bool_t eRes = false;
    int16_t    i2SampleNum = nInByteNum / sizeof(int16_t);
    int16_t    i2MaxSample = 0, i2TmpSample = 0;
    int16_t    *pi2TmpBuf  = new int16_t[i2SampleNum];
    int32_t    i4Average   = 0;
    const  int16_t i2MuteThre  = 0x100;   //    dB
    const  int16_t i2VoiceThre = 0x800;  //    dB
    static int16_t i2AdaptThre = 0x300;   //     dB
    static uint8_t u1ResultBit = 0x0;     // 160 ms results
    uint8_t    c = 0, n = 0;
    
    if (!i2SampleNum) {
        return false;
    }
    
    memcpy(pi2TmpBuf, pi2InputBuf, nInByteNum);
    
    // Step 1: Filter the DC noise, use the 80Hz high pass filter
    tdav_codec_iir_base16K (pi2TmpBuf, pi2TmpBuf, BIQ_HPF_FILTER_TBL_16K[0], nInByteNum);
    
    // Step 2: Find peak and do the adptive level detect
    for (int i = 0; i < i2SampleNum; i++) {
        i2TmpSample = ABS(*(pi2TmpBuf + i));
        i2MaxSample = MAX(i2MaxSample, i2TmpSample);
        i4Average  += i2TmpSample;
    }
    i4Average /= i2SampleNum;
    
    if (i4Average < i2MuteThre) { // In this case, the result is detected as mute always
        eRes = false;
    } else if (i4Average > i2VoiceThre) { // In this case, the result is detected as voice always
        i2AdaptThre = i4Average >> 2;
        i2AdaptThre = MAX(i2AdaptThre, i2MuteThre);
        eRes = true;
    } else {
        if (i4Average > i2AdaptThre) {
            // Decrease the adaptive threshold
            i2AdaptThre = i4Average * 3 / 4;
            i2AdaptThre = MAX(i2AdaptThre, i2MuteThre);
            eRes = true;
        } else {
            // Kep the adaptive threshold
            eRes = false;
        }
    }
    
    // Step 3: Push the current result into the result-array
    u1ResultBit <<= 1;
    u1ResultBit &= 0xfe;
    u1ResultBit |= (eRes ? 0x1 : 0x0);
    
    // Step 4: Smooth the result，数有多少个“1”
    //if ((u1ResultBit & 0x07) == 0x7) {
    //    return true;
    //} else {
    //    return false;
    //}
    n = u1ResultBit & 0x1f;
    for (c = 0; n; ++c) {
        n &= (n - 1);
    }
    if (c >= 3) {
        return true;
    } else {
        return false;
    }
}

tsk_bool_t tdav_codec_ringbuf_write_available (int32_t i4Wp, int32_t i4Rp, int32_t i4WriteSz, int32_t i4BufferSz)
{
    int32_t i4Gap;
    if (i4Wp > i4Rp) {
        i4Gap = i4BufferSz - i4Wp + i4Rp;
    } else {
        i4Gap = i4Rp - i4Wp;
    }
    
    if (i4Gap > i4WriteSz) {
        return true;
    } else {
        return false;
    }
}

tsk_bool_t tdav_codec_ringbuf_read_available (int32_t i4Wp, int32_t i4Rp, int32_t i4ReadSz, int32_t i4BufferSz)
{
    int32_t i4Gap;
    if (i4Wp > i4Rp) {
        i4Gap = i4Wp - i4Rp;
    } else {
        i4Gap = i4BufferSz - i4Rp + i4Wp;
    }
    
    if (i4Gap > i4ReadSz) {
        return true;
    } else {
        return false;
    }
}

void tdav_codec_apply_volume_gain(float fGain, void* pBuf, int nBytes, const uint8_t nBytesPerSample)
{
    if ((fGain < 0.0f) || (1.0f == fGain)) {
        return;
    }
    
    if (2 == nBytesPerSample) {
        uint8_t *pData = (uint8_t*)pBuf;
        int pcmMin = - (1 << 15);
        int pcmMax = (1 << 15) - 1;
        int i=0;
        for (i = 0; i < nBytes - 1; i += 2) {
            short oldValue = 0;
            short newValue;
            int   tempValue;
            
            oldValue |= ((pData[i]) & 0xFF);
            oldValue |= ((pData[i + 1] << 8) & 0xFF00);
            if (oldValue >= 0) {
                tempValue = (int)(oldValue * fGain + 0.5f);
            } else {
                tempValue = (int)(oldValue * fGain - 0.5f);
            }
            
            if(tempValue > pcmMax) {
                newValue = (short)pcmMax;
            } else if(tempValue < pcmMin) {
                newValue = (short)pcmMin;
            } else {
                newValue = (short)tempValue;
            }
            pData[i] = (uint8_t) (newValue & 0xFF);
            pData[i + 1] = (uint8_t) ((newValue & 0xFF00) >> 8);
        }
    } else if (1 == nBytesPerSample) {
        int8_t *pData = (int8_t*)pBuf;
        int pcmMin = - (1 << 7);
        int pcmMax = (1 << 7) - 1;
        int i = 0;
        for (i = 0; i < nBytes; i++) {
            int tempValue;
            if (pData[i] >= 0) {
                tempValue= (int)(pData[i] * fGain + 0.5f);
            } else {
                tempValue= (int)(pData[i] * fGain - 0.5f);
            }
            
            if(tempValue > pcmMax) {
                pData[i] = (int8_t)pcmMax;
            } else if(tempValue < pcmMin) {
                pData[i]  = (int8_t)pcmMin;
            } else {
                pData[i]  = (int8_t)tempValue;
            }
        }
    }
}

void tdav_codec_float_to_int16 (void *pInput, void* pOutput, uint8_t* pu1SampleSz, uint32_t* pu4TotalSz, tsk_bool_t bFloat)
{
    int16_t i2SampleNumTotal = *pu4TotalSz / *pu1SampleSz;
    int16_t *pi2Buf = (int16_t *)pOutput;
    if (!pInput || !pOutput) {
        return;
    }
    
    if (bFloat && *pu1SampleSz == 4) {
        float* pf4Buf = (float*)pInput;
        for (int i = 0; i < i2SampleNumTotal; i++) {
            pi2Buf[i] = (int16_t)(pf4Buf[i] * 32767 + 0.5); // float -> int16 + rounding
        }
        *pu4TotalSz /= 2;     // transfer to int16 byte size
        *pu1SampleSz /= 2;
    }
    if (!bFloat && *pu1SampleSz == 2) {
        memcpy (pOutput, pInput, *pu4TotalSz);
    }
    return;
}

void tdav_codec_int16_to_float (void *pInput, void* pOutput, uint8_t* pu1SampleSz, uint32_t* pu4TotalSz, tsk_bool_t bInt16)
{
    int16_t i2SampleNumTotal = *pu4TotalSz / *pu1SampleSz;
    float *pf4Buf = (float *)pOutput;
    if (!pInput || !pOutput) {
        return;
    }
    
    if ( bInt16 && *pu1SampleSz == 2) {
        int16_t* pi2Buf = (int16_t*)pInput;
        for (int i = 0; i < i2SampleNumTotal; i++) {
            pf4Buf[i] =    (float)pi2Buf[i] / 32767 ;  // int16 -> float
        }
        *pu4TotalSz *= 2;     // transfer to int16 byte size
        *pu1SampleSz *= 2;
    }
    if (!bInt16 && *pu1SampleSz == 4) {     //如果是float，直接拷贝
        memcpy (pOutput, pInput, *pu4TotalSz);
    }
    return;
}

void tdav_codec_multichannel_interleaved_to_mono (void *pInput, void* pOutput, uint8_t* pu1ChNum, uint32_t* pu4TotalSz, tsk_bool_t bInterleaved)
{
    int16_t *pi2Buf1 = (int16_t *)pInput;
    int16_t *pi2Buf2 = (int16_t *)pOutput;
    int16_t i2SamplePerCh = *pu4TotalSz / sizeof(int16_t) / *pu1ChNum;
    
    if (!pInput || !pOutput || !(*pu1ChNum)) {
        return;
    }
    
    if ((*pu1ChNum == 2) && (bInterleaved)) {
        for (int i = 0; i < i2SamplePerCh; i++) {
            pi2Buf2[i] = pi2Buf1[i * (*pu1ChNum)]; // left channel
            pi2Buf2[i + i2SamplePerCh] = pi2Buf1[i * (*pu1ChNum) + 1]; // right channel
        }
    } else {
        memcpy(pi2Buf2, pi2Buf1, *pu4TotalSz);
    }
    
    // Downmix to MONO here, fix me!!
    if (*pu1ChNum == 2) {
        for (int i = 0; i < i2SamplePerCh; i++) {
            pi2Buf2[i] = (pi2Buf2[i] >> 1) + (pi2Buf2[i + i2SamplePerCh] >> 1);
        }
        *pu1ChNum /= 2;
        *pu4TotalSz /= 2;
    }
}

void tdav_codec_volume_smooth_gain (void *pInput, int16_t i2SamplePerCh, int16_t i2FadeSample, int32_t i4CurVol, int32_t *pi4LastVol)
{
    int32_t i4VolStep = 0;
    int64_t i8Mul = 0;
    int16_t *pi2Buf = (int16_t *)pInput;
    
    if (!pInput) {
        return;
    }
    
    i2FadeSample = (i2FadeSample > i2SamplePerCh) ? i2SamplePerCh : i2FadeSample;
    
    if (*pi4LastVol != i4CurVol) {
        i4VolStep = (i4CurVol - *pi4LastVol) / i2FadeSample;
        for (int i = 0; i < i2FadeSample; i++) {
            i8Mul = (int64_t)((int64_t)pi2Buf[i] * (*pi4LastVol)) >> 31;
            *pi4LastVol += i4VolStep;
            pi2Buf[i] = (int16_t)i8Mul;
        }
        *pi4LastVol = i4CurVol;
        for (int i = i2FadeSample; i < i2SamplePerCh; i++) {
            i8Mul = (int64_t)((int64_t)pi2Buf[i] * (*pi4LastVol)) >> 31;
            pi2Buf[i] = (int16_t)i8Mul;
        }
    } else {
        for (int i = 0; i < i2SamplePerCh; i++) {
            i8Mul = (int64_t)((int64_t)pi2Buf[i] * (*pi4LastVol)) >> 31;
            pi2Buf[i] = (int16_t)i8Mul;
        }
    }
}

int16_t tdav_codec_get_current_delayblk (int16_t i2ReadBlk, int16_t i2WriteBlk, int16_t i2MaxBlk)
{
    int16_t i2CurDelayBlk = 0;
    if (i2WriteBlk >= i2ReadBlk) {
        i2CurDelayBlk = i2WriteBlk - i2ReadBlk;
    } else {
        i2CurDelayBlk = i2WriteBlk + i2MaxBlk + 1 - i2ReadBlk;
    }
    return i2CurDelayBlk;
}

void tdav_codec_increase_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2BytePerBlk, int16_t i2Maxblk)
{
    if (!pDelayBuf || !pInputBuf || !pOutputBuf || !i2BytePerBlk) {
        TSK_DEBUG_ERROR("Delay buffer NOT initialized!");
        return;
    }
    // 1. Write delay buffer
    memcpy((void *)((char *)pDelayBuf + (*pi2WriteBlk * i2BytePerBlk)), pInputBuf, i2BytePerBlk);
    // 2. Increase the write block
    *pi2WriteBlk = *pi2WriteBlk + 1;
    if ((*pi2WriteBlk) > i2Maxblk) {
        *pi2WriteBlk = 0;
    }
    if ((*pi2WriteBlk) == (*pi2ReadBlk)) {
        TSK_DEBUG_ERROR("Delay write block is equal to read block, need debug!");
        return;
    }
    // 3. Output zero data, keep the read block
    memset(pOutputBuf, 0, i2BytePerBlk);
}

void tdav_codec_decrease_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2GapDelay, int16_t i2BytePerBlk, int16_t i2Maxblk)
{
    if (!pDelayBuf || !pInputBuf || !pOutputBuf || !i2BytePerBlk) {
        TSK_DEBUG_ERROR("Delay buffer NOT initialized!");
        return;
    }
    // 1. Write delay buffer
    memcpy((void *)((char *)pDelayBuf + (*pi2WriteBlk * i2BytePerBlk)), pInputBuf, i2BytePerBlk);
    // 2. Increase the write block
    *pi2WriteBlk = *pi2WriteBlk + 1;
    if ((*pi2WriteBlk) > i2Maxblk) {
        *pi2WriteBlk = 0;
    }
    if ((*pi2WriteBlk) == (*pi2ReadBlk)) {
        TSK_DEBUG_ERROR("Delay write block is equal to read block, need debug!");
        return;
    }
    // 3. Pull up the read block and read
    *pi2ReadBlk = *pi2ReadBlk + i2GapDelay;
    if ((*pi2ReadBlk) > i2Maxblk) {
        *pi2ReadBlk = *pi2ReadBlk - (i2Maxblk + 1);
    }
    if ((*pi2ReadBlk) == (*pi2WriteBlk)) {
        TSK_DEBUG_ERROR("Delay read block is equal to write block, need debug!");
        return;
    }
    memcpy(pOutputBuf, (void *)((char *)pDelayBuf + (*pi2ReadBlk * i2BytePerBlk)), i2BytePerBlk);
    // 4. Increase the read block
    *pi2ReadBlk = *pi2ReadBlk + 1;
    if ((*pi2ReadBlk) > i2Maxblk) {
        *pi2ReadBlk = 0;
    }
}

void tdav_codec_equal_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2BytePerBlk, int16_t i2Maxblk)
{
    if (!pDelayBuf || !pInputBuf || !pOutputBuf || !i2BytePerBlk) {
        TSK_DEBUG_ERROR("Delay buffer NOT initialized!");
        return;
    }
    // 1. Write delay buffer
    memcpy((void *)((char *)pDelayBuf + (*pi2WriteBlk * i2BytePerBlk)), pInputBuf, i2BytePerBlk);
    // 2. Increase the write block
    *pi2WriteBlk = *pi2WriteBlk + 1;
    if ((*pi2WriteBlk) > i2Maxblk) {
        *pi2WriteBlk = 0;
    }
    // 3. Read delay buffer
    memcpy(pOutputBuf, (void *)((char *)pDelayBuf + (*pi2ReadBlk * i2BytePerBlk)), i2BytePerBlk);
    // 4. Increase the read block
    *pi2ReadBlk = *pi2ReadBlk + 1;
    if ((*pi2ReadBlk) > i2Maxblk) {
        *pi2ReadBlk = 0;
    }
}

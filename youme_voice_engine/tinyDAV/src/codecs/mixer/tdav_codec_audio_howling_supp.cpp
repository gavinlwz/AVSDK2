//
//  tdav_codec_audio_howling_supp.cpp
//  youme_voice_engine
//
//  Created by paul on 29/09/16.
//  Copyright © 2016 youme. All rights reserved.
//

#include "tsk_memory.h"
#include "tinydav/codecs/mixer/tdav_codec_audio_howling_supp.h"
#include "tsk_debug.h"
#include "tsk_string.h"
#include "../../audio/stdinc.h"
#include <math.h>

/**
 * int16_t* pInData    Sample input array
 * int16_t* pOutData   Sample output array
 * int16_t  i2Len      Input/Output array length, currently 512 support only, if you MUST enlarge the FFT NUM, remember to re-create the window table
 * char   * pCharType  Window type, currently Hanning/Hamming support only,
 */
void tdav_codec_audio_prefft_window_processing(int16_t *pInData, int16_t *pOutData, int16_t i2Len, window_type_t eWindowType)
{
    int16_t *pWindow;
    int16_t i2Loop = 0;
    int16_t i2Mul1, i2Mul2;
    if (WINDOW_HANNING == eWindowType) {
        pWindow = window_hanning; // Q14
    } else if (WINDOW_HAMMING == eWindowType) {
        pWindow = window_hamming; // Q14
    } else {
        pWindow = window_hanning; // Q14
    }
    
    for (i2Loop = 0; i2Loop < i2Len; i2Loop++) {
        i2Mul1 = *(pInData + i2Loop);
        i2Mul2 = *(pWindow + i2Loop);
        *(pOutData + i2Loop) = (int16_t)((int32_t)((int32_t)i2Mul1 * (int32_t)i2Mul2) >> 14);
    }
}

/**
 * int16_t* pInData    Sample input array
 * double * pOutData   Sample output array
 * int16_t  i2Len      Input/Output array length
 */
void tdav_codec_audio_howling_spectrum_power(int16_t *pInData, int32_t *pOutData, int16_t i2Len)
{
    int i, j;
    
    if (!pInData || !pOutData) {
        TSK_DEBUG_ERROR("NULL address: pInData=%d, pOutData=%d", pInData, pOutData);
        return;
    }
    pOutData[0] = (int32_t)pInData[0] * (int32_t)pInData[0];
    if (!pOutData[0]) {
        pOutData[0] = 1;
    }
    for (i = 1, j = 1; i < i2Len - 1; i += 2, j++) {
        pOutData[j] = ((int32_t)pInData[i] * (int32_t)pInData[i]) + ((int32_t)pInData[i + 1] * (int32_t)pInData[i + 1]);
        if (!pOutData[j]) {
            pOutData[j] = 1;
        }
    }
    pOutData[j] = (int32_t)pInData[i] * (int32_t)pInData[i];
    if (!pOutData[j]) {
        pOutData[j] = 1;
    }
}

/**
 * int32_t  i4Num      Input sample
 * return int16_t      The number of pre-zeros
 */
int16_t tdav_codec_audio_howling_normW32(int32_t i4Num)
{
    int16_t zeros;
    
    if (i4Num == 0) {
        return 0;
    }
    else if (i4Num < 0) {
        i4Num = ~i4Num;
    }
    
    if (!(0xFFFF8000 & i4Num)) {
        zeros = 16;
    } else {
        zeros = 0;
    }
    if (!(0xFF800000 & (i4Num << zeros))) zeros += 8;
    if (!(0xF8000000 & (i4Num << zeros))) zeros += 4;
    if (!(0xE0000000 & (i4Num << zeros))) zeros += 2;
    if (!(0xC0000000 & (i4Num << zeros))) zeros += 1;
    
    return zeros;
}

/**
 * uint32_t u4Num      Input sample
 * return int16_t      The bit number of sample
 */
int16_t tdav_codec_audio_howling_bitsU32(uint32_t u4Num)
{
    int16_t bits;
    
    if (0xFFFF0000 & u4Num) {
        bits = 16;
    } else {
        bits = 0;
    }
    if (0x0000FF00 & (u4Num >> bits)) bits += 8;
    if (0x000000F0 & (u4Num >> bits)) bits += 4;
    if (0x0000000C & (u4Num >> bits)) bits += 2;
    if (0x00000002 & (u4Num >> bits)) bits += 1;
    if (0x00000001 & (u4Num >> bits)) bits += 1;
    
    return bits;
}

void tdav_codec_audio_dc_remove(int16_t *pi2Data, int16_t i2Len)
{
    int16_t i;
    int32_t tmp = 0;
    double  fTmp = 0.0f, nOffset = 0.0f;
    
    for(i = 0; i < i2Len; i++)
    {
        tmp += pi2Data[i];
    }
    fTmp = -(double)tmp/i2Len;
    if(fTmp < 0) {
        nOffset = fTmp - 0.5f;
    } else if (fTmp > 0.0f) {
        nOffset = fTmp + 0.5f;
    }
    
    for(i = 0; i < i2Len; i++)
    {
        pi2Data[i] = (int16_t)((double)pi2Data[i] + nOffset);
        
        if(pi2Data[i] > 32000) {
            pi2Data[i] = 32000;
        } else if (pi2Data[i] < -32000) {
            pi2Data[i] = -32000;
        }
    }
}

//
//  tdav_codec_audio_mixer.h
//  youme_voice_engine
//
//  Created by peter on 6/25/16.
//  Copyright © 2016 youme. All rights reserved.
//

#ifndef tdav_codec_audio_mixer_h
#define tdav_codec_audio_mixer_h

#ifdef __cplusplus
extern "C" {
#endif
    
#include <sys/types.h>

typedef struct audioMixMeta {
    int16_t * bufferPtr;
    uint32_t  sampleCount;
} AudioMixMeta;

int16_t do_mixing(int16_t * in, size_t in_tracks);
void tdav_codec_mixer_mix16 (const AudioMixMeta* pInputPtr, const size_t nTrackCountTotal, int16_t* pOutBufferPtr, const size_t nMixSampleCountMax);
void tdav_codec_mixer_normal(int16_t* pInBuf1, int16_t* pInBuf2, int16_t* pOutBuf, const size_t nMixSampleCountMax);

tsk_bool_t tdav_codec_ringbuf_write_available (int32_t i4Wp, int32_t i4Rp, int32_t i4WriteSz, int32_t i4BufferSz);
tsk_bool_t tdav_codec_ringbuf_read_available (int32_t i4Wp, int32_t i4Rp, int32_t i4ReadSz, int32_t i4BufferSz);
    
void tdav_codec_apply_volume_gain(float fGain, void* pBuf, int nBytes, const uint8_t nBytesPerSample);

void tdav_codec_iir_base16K(int16_t* pi2InputBuf, int16_t* pi2OutputBuf, const int16_t *pCoeff, const size_t nInByteNum);
void tdav_codec_float_to_int16 (void *pInput, void* pOutput, uint8_t* pu1SampleSz, uint32_t* pu4TotalSz, tsk_bool_t bFloat);
void tdav_codec_int16_to_float (void *pInput, void* pOutput, uint8_t* pu1SampleSz, uint32_t* pu4TotalSz, tsk_bool_t bInt16);
void tdav_codec_multichannel_interleaved_to_mono (void *pInput, void* pOutput, uint8_t* pu1ChNum, uint32_t* pu4TotalSz, tsk_bool_t bInterleaved);
void tdav_codec_volume_smooth_gain (void *pInput, int16_t i2SamplePerCh, int16_t i2FadeSample, int32_t i4CurVol, int32_t *pi4LastVol);
tsk_bool_t tdav_codec_detect_audio(int16_t* pi2InputBuf, const size_t nInByteNum);
int16_t tdav_codec_get_current_delayblk (int16_t i2ReadBlk, int16_t i2WriteBlk, int16_t i2MaxBlk);
void tdav_codec_increase_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2BytePerBlk, int16_t i2Maxblk);
void tdav_codec_decrease_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2GapDelay, int16_t i2BytePerBlk, int16_t i2Maxblk);
void tdav_codec_equal_delay (void *pDelayBuf, void *pInputBuf, void *pOutputBuf, int16_t *pi2ReadBlk, int16_t *pi2WriteBlk, int16_t i2BytePerBlk, int16_t i2Maxblk);
    
#ifdef __cplusplus
}
#endif
        
#endif /* tdav_codec_audio_mixer_h */

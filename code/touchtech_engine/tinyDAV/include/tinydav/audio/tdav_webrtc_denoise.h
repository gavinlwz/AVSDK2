/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/
/**@file tdav_webrtc_denoise.h
 * @brief Google WebRTC Denoiser (Noise suppression, AGC, AEC) Plugin
 *
*
 *

 */
#ifndef TINYDAV_WEBRTC_DENOISE_H
#define TINYDAV_WEBRTC_DENOISE_H

#include "tinydav_config.h"

#if HAVE_WEBRTC && (!defined(HAVE_WEBRTC_DENOISE) || HAVE_WEBRTC_DENOISE)

#include "tinymedia/tmedia_denoise.h"
#include "tinydav/codecs/mixer/speex_resampler.h"

#include "tsk_safeobj.h"

#ifdef __cplusplus
/* Speex denoiser works better than WebRTC's denoiser. This is obvious on Android. */
#if !defined(PREFER_SPEEX_DENOISER)
#define PREFER_SPEEX_DENOISER 0
#endif

#if TDAV_UNDER_MOBILE
#include "echo_control_mobile.h"
#define TDAV_WebRtcAecm_Create() WebRtcAecm_Create ()
#define TDAV_WebRtcAecm_Free(aecInst) WebRtcAecm_Free (aecInst)
#define TDAV_WebRtcAecm_Init(aecInst, sampFreq, scSampFreq) WebRtcAecm_Init (aecInst, sampFreq)
#define TDAV_WebRtcAecm_BufferFarend(aecInst, farend, nrOfSamples) WebRtcAecm_BufferFarend (aecInst, farend, nrOfSamples)
#define TDAV_WebRtcAecm_Process(aecInst, nearendNoisy, nearendClean, out, outH, nrOfSamples, msInSndCardBuf, skew) \
    WebRtcAecm_Process (aecInst, nearendNoisy, nearendClean, out, nrOfSamples, msInSndCardBuf)
#else
#include "echo_control_mobile.h"
#define TDAV_WebRtcAecm_Create() WebRtcAecm_Create ()
#define TDAV_WebRtcAecm_Free(aecInst) WebRtcAecm_Free (aecInst)
#define TDAV_WebRtcAecm_Init(aecInst, sampFreq, scSampFreq) WebRtcAecm_Init (aecInst, sampFreq)
#define TDAV_WebRtcAecm_BufferFarend(aecInst, farend, nrOfSamples) WebRtcAecm_BufferFarend (aecInst, farend, nrOfSamples)
#define TDAV_WebRtcAecm_Process(aecInst, nearendNoisy, nearendClean, out, outH, nrOfSamples, msInSndCardBuf, skew) \
    WebRtcAecm_Process (aecInst, nearendNoisy, nearendClean, out, nrOfSamples, msInSndCardBuf)
#endif

// Use fixed implementation for Noise Suppression
#if TDAV_UNDER_MOBILE
#include "noise_suppression_x.h"
#define TDAV_WebRtcNs_Process(NS_inst, speechFrame, num_bands, outframe) WebRtcNsx_Process (NS_inst, speechFrame, num_bands, outframe)
#define TDAV_WebRtcNs_Init(NS_inst, fs) WebRtcNsx_Init (NS_inst, fs)
#define TDAV_WebRtcNs_set_policy(NS_inst, policy) WebRtcNsx_set_policy (NS_inst, policy)
#define TDAV_WebRtcNs_Free(NS_inst) WebRtcNsx_Free (NS_inst)
#define TDAV_WebRtcNs_Create(NS_inst) WebRtcNsx_Create (NS_inst)
#define TDAV_NsHandle NsxHandle
#else
#include "noise_suppression_x.h"
#define TDAV_WebRtcNs_Process(NS_inst, speechFrame, num_bands, outframe) WebRtcNsx_Process (NS_inst, speechFrame, num_bands, outframe)
#define TDAV_WebRtcNs_Init(NS_inst, fs) WebRtcNsx_Init (NS_inst, fs)
#define TDAV_WebRtcNs_set_policy(NS_inst, policy) WebRtcNsx_set_policy (NS_inst, policy)
#define TDAV_WebRtcNs_Free(NS_inst) WebRtcNsx_Free (NS_inst)
#define TDAV_WebRtcNs_Create(NS_inst) WebRtcNsx_Create (NS_inst)
#define TDAV_NsHandle NsxHandle
#endif

#include "webrtc_vad.h"
#define TDAV_VADHandle VadInst
#define TDAV_WebRtcVad_Create(VAD_inst) WebRtcVad_Create (VAD_inst)
#define TDAV_WebRtcVad_Init(VAD_inst) WebRtcVad_Init (VAD_inst)
#define TDAV_WebRtcVad_SetMode(VAD_inst, mode) WebRtcVad_set_mode (VAD_inst, mode)
#define TDAV_WebRtcVad_Process(VAD_inst, fs, audio_frame, frame_length) WebRtcVad_Process (VAD_inst, fs, audio_frame, frame_length)
#define TDAV_WebRtcVad_Free(VAD_inst) WebRtcVad_Free (VAD_inst)

#include "webrtc_cng.h"
#define TDAV_CNGHandle CNG_dec_inst
#define TDAV_WebRtcCng_Create(CNG_inst) WebRtcCng_CreateDec (CNG_inst)
#define TDAV_WebRtcCng_Init(CNG_inst) WebRtcCng_InitDec (CNG_inst)
#define TDAV_WebRtcCng_Free(CNG_inst) WebRtcCng_FreeDec (CNG_inst)
#define TDAV_WebRtcCng_UpdateSid(CNG_inst, SID, length) WebRtcCng_UpdateSid (CNG_inst, SID, length)
#define TDAV_WebRtcCng_Generate(CNG_inst, outData, nrOfSamples, new_period) WebRtcCng_Generate (CNG_inst, outData, nrOfSamples, new_period);

#include "gain_control.h"
#define TDAV_AGCHandle void
#define TDAV_WebRtcAgc_Create(AGC_inst) WebRtcAgc_Create (AGC_inst)
#define TDAV_WebRtcAgc_Init(AGC_inst, minLevel, maxLevel, agcMode, fs) WebRtcAgc_Init (AGC_inst, minLevel, maxLevel, agcMode, fs)
#define TDAV_WebRtcAgc_Set_Config(AGC_inst, agcConfig) WebRtcAgc_set_config(AGC_inst, agcConfig)
#define TDAV_WebRtcAgc_Free(AGC_inst) WebRtcAgc_Free (AGC_inst)
#define TDAV_WebRtcAgc_Process(AGC_inst, inNear, num_bands, samples, out, inMicLevel, outMicLevel, echo, saturationWarning) \
    WebRtcAgc_Process (AGC_inst, inNear, num_bands, samples, out, inMicLevel, outMicLevel, echo, saturationWarning)
#define TDAV_WebRtcAgc_AddFarend(AGC_inst, inFar, samples) WebRtcAgc_AddFarend (AGC_inst, inFar, samples)
#define TDAV_WebRtcAgc_VirtualMic(AGC_inst, inNear, num_bands, samples, inMicLevel, outMicLevel) \
WebRtcAgc_VirtualMic(AGC_inst, inNear, num_bands, samples, inMicLevel, outMicLevel)
#define TDAV_WebRtcAgc_AddMic(AGC_inst, inNear, num_bands, samples) WebRtcAgc_AddMic(AGC_inst, inNear, num_bands, samples)

#endif  // __cplusplus


TDAV_BEGIN_DECLS

extern const tmedia_denoise_plugin_def_t *tdav_webrtc_denoise_plugin_def_t;

TDAV_END_DECLS

#endif /* #if HAVE_WEBRTC */

#endif /* TINYDAV_WEBRTC_DENOISE_H */

/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* The global function contained in this file initializes SPL function
 * pointers, currently only for ARM platforms.
 *
 * Some code came from common/rtcd.c in the WebM project.
 */

#if TARGET_OS_IPHONE
#include "ios_config.h"
#else
#include "android_config.h"
#endif

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
//#include "webrtc/system_wrappers/include/cpu_features_wrapper.h"

/* Declare function pointers. */
MaxAbsValueW16 YoumeWebRtcSpl_MaxAbsValueW16;
MaxAbsValueW32 YoumeWebRtcSpl_MaxAbsValueW32;
MaxValueW16 YoumeWebRtcSpl_MaxValueW16;
MaxValueW32 YoumeWebRtcSpl_MaxValueW32;
MinValueW16 YoumeWebRtcSpl_MinValueW16;
MinValueW32 YoumeWebRtcSpl_MinValueW32;
CrossCorrelation YoumeWebRtcSpl_CrossCorrelation;
DownsampleFast YoumeWebRtcSpl_DownsampleFast;
ScaleAndAddVectorsWithRound YoumeWebRtcSpl_ScaleAndAddVectorsWithRound;

#if (defined(WEBRTC_DETECT_NEON) || !defined(WEBRTC_HAS_NEON)) && !defined(MIPS32_LE)
/* Initialize function pointers to the generic C version. */
static void InitPointersToC ()
{
    YoumeWebRtcSpl_MaxAbsValueW16 = YoumeWebRtcSpl_MaxAbsValueW16C;
    YoumeWebRtcSpl_MaxAbsValueW32 = YoumeWebRtcSpl_MaxAbsValueW32C;
    YoumeWebRtcSpl_MaxValueW16 = YoumeWebRtcSpl_MaxValueW16C;
    YoumeWebRtcSpl_MaxValueW32 = YoumeWebRtcSpl_MaxValueW32C;
    YoumeWebRtcSpl_MinValueW16 = YoumeWebRtcSpl_MinValueW16C;
    YoumeWebRtcSpl_MinValueW32 = YoumeWebRtcSpl_MinValueW32C;
    YoumeWebRtcSpl_CrossCorrelation = YoumeWebRtcSpl_CrossCorrelationC;
    YoumeWebRtcSpl_DownsampleFast = YoumeWebRtcSpl_DownsampleFastC;
    YoumeWebRtcSpl_ScaleAndAddVectorsWithRound = YoumeWebRtcSpl_ScaleAndAddVectorsWithRoundC;
}
#endif

#if defined(WEBRTC_DETECT_NEON) || defined(WEBRTC_HAS_NEON)
/* Initialize function pointers to the Neon version. */
static void InitPointersToNeon ()
{
    YoumeWebRtcSpl_MaxAbsValueW16 = YoumeWebRtcSpl_MaxAbsValueW16Neon;
    YoumeWebRtcSpl_MaxAbsValueW32 = YoumeWebRtcSpl_MaxAbsValueW32Neon;
    YoumeWebRtcSpl_MaxValueW16 = YoumeWebRtcSpl_MaxValueW16Neon;
    YoumeWebRtcSpl_MaxValueW32 = YoumeWebRtcSpl_MaxValueW32Neon;
    YoumeWebRtcSpl_MinValueW16 = YoumeWebRtcSpl_MinValueW16Neon;
    YoumeWebRtcSpl_MinValueW32 = YoumeWebRtcSpl_MinValueW32Neon;
    YoumeWebRtcSpl_CrossCorrelation = YoumeWebRtcSpl_CrossCorrelationNeon;
    YoumeWebRtcSpl_DownsampleFast = YoumeWebRtcSpl_DownsampleFastNeon;
    YoumeWebRtcSpl_ScaleAndAddVectorsWithRound = YoumeWebRtcSpl_ScaleAndAddVectorsWithRoundC;
}
#endif

#if defined(MIPS32_LE)
/* Initialize function pointers to the MIPS version. */
static void InitPointersToMIPS ()
{
    YoumeWebRtcSpl_MaxAbsValueW16 = YoumeWebRtcSpl_MaxAbsValueW16_mips;
    YoumeWebRtcSpl_MaxValueW16 = YoumeWebRtcSpl_MaxValueW16_mips;
    YoumeWebRtcSpl_MaxValueW32 = YoumeWebRtcSpl_MaxValueW32_mips;
    YoumeWebRtcSpl_MinValueW16 = YoumeWebRtcSpl_MinValueW16_mips;
    YoumeWebRtcSpl_MinValueW32 = YoumeWebRtcSpl_MinValueW32_mips;
    YoumeWebRtcSpl_CrossCorrelation = YoumeWebRtcSpl_CrossCorrelation_mips;
    YoumeWebRtcSpl_DownsampleFast = YoumeWebRtcSpl_DownsampleFast_mips;
#if defined(MIPS_DSP_R1_LE)
    YoumeWebRtcSpl_MaxAbsValueW32 = YoumeWebRtcSpl_MaxAbsValueW32_mips;
    YoumeWebRtcSpl_ScaleAndAddVectorsWithRound = YoumeWebRtcSpl_ScaleAndAddVectorsWithRound_mips;
#else
    YoumeWebRtcSpl_MaxAbsValueW32 = YoumeWebRtcSpl_MaxAbsValueW32C;
    YoumeWebRtcSpl_ScaleAndAddVectorsWithRound = YoumeWebRtcSpl_ScaleAndAddVectorsWithRoundC;
#endif
}
#endif

static void InitFunctionPointers (void)
{
#if defined(WEBRTC_DETECT_NEON)
    if ((WebRtc_GetCPUFeaturesARM () & kCPUFeatureNEON) != 0)
    {
        InitPointersToNeon ();
    }
    else
    {
        InitPointersToC ();
    }
#elif defined(WEBRTC_HAS_NEON)
    InitPointersToNeon ();
#elif defined(MIPS32_LE)
    InitPointersToMIPS ();
#else
    InitPointersToC ();
#endif /* WEBRTC_DETECT_NEON */
}

#if defined(WEBRTC_POSIX) && !defined(_WIN32)
#include <pthread.h>
static void once (void (*func)(void))
{
    static pthread_once_t lock = PTHREAD_ONCE_INIT;
    pthread_once (&lock, func);
}
#elif defined(_WIN32)
#include <windows.h>

static void once (void (*func)(void))
{
    /* Didn't use InitializeCriticalSection() since there's no race-free context
     * in which to execute it.
     *
     * TODO(kma): Change to different implementation (e.g.
     * InterlockedCompareExchangePointer) to avoid issues similar to
     * http://code.google.com/p/webm/issues/detail?id=467.
     */
    static CRITICAL_SECTION lock = { (void *)((size_t)-1), -1, 0, 0, 0, 0 };
    static int done = 0;

    EnterCriticalSection (&lock);
    if (!done)
    {
        func ();
        done = 1;
    }
    LeaveCriticalSection (&lock);
}

/* There's no fallback version as an #else block here to ensure thread safety.
 * In case of neither pthread for WEBRTC_POSIX nor _WIN32 is present, build
 * system should pick it up.
 */
#endif /* WEBRTC_POSIX */



void YoumeWebRtcSpl_Init ()
{
    once (InitFunctionPointers);
}

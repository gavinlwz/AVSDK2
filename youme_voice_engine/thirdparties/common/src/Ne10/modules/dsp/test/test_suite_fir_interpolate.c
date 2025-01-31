﻿/*
 *  Copyright 2012-15 ARM Limited and Contributors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of ARM Limited nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY ARM LIMITED AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL ARM LIMITED AND CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NE10 Library : test_suite_fir_interpolate.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "NE10_dsp.h"
#include "seatest.h"


/* ----------------------------------------------------------------------
** Global defines
** ------------------------------------------------------------------- */

/* Max FFT Length 1024 and double buffer for real and imag */
#define TEST_LENGTH_SAMPLES 480
#define MAX_BLOCKSIZE 320
#define MAX_NUMTAPS 100

#define TEST_COUNT 5000

//input and output
static ne10_float32_t * guarded_in_c = NULL;
static ne10_float32_t * guarded_in_neon = NULL;
static ne10_float32_t * in_c = NULL;
static ne10_float32_t * in_neon = NULL;

static ne10_float32_t * guarded_out_c = NULL;
static ne10_float32_t * guarded_out_neon = NULL;
static ne10_float32_t * out_c = NULL;
static ne10_float32_t * out_neon = NULL;

static ne10_float32_t * guarded_fir_state_c = NULL;
static ne10_float32_t * guarded_fir_state_neon = NULL;
static ne10_float32_t * fir_state_c = NULL;
static ne10_float32_t * fir_state_neon = NULL;

static ne10_float32_t snr = 0.0f;

#ifdef PERFORMANCE_TEST
static ne10_int64_t time_c = 0;
static ne10_int64_t time_neon = 0;
static ne10_float32_t time_speedup = 0.0f;
static ne10_float32_t time_savings = 0.0f;
#endif

/* ----------------------------------------------------------------------
* Coefficients for 32-tap filter  for F32
* ------------------------------------------------------------------- */

static ne10_float32_t testCoeffs32_f32[32] =
{
    0.068186,   0.064344,   -0.162450,  0.057015,   0.029743,   0.010066,   0.047792,   0.021273,
    -0.096447,  -0.211652,  -0.086613,  0.057501,   -0.187605,  -0.167199,  -0.026983,  -0.025464,
    -0.061495,  0.110914,   -0.081973,  -0.055231,  -0.074430,  -0.196536,  0.016845,   -0.096493,
    0.039625,   -0.110273,  -0.042966,  -0.043804,  0.087350,   -0.085191,  0.009420,   0.086440
};

/* ----------------------------------------------------------------------
* Coefficients for 8-tap filter for F32
* ------------------------------------------------------------------- */

static ne10_float32_t testCoeffs8_f32[8] =
{
    0.039625,   -0.110273,  -0.042966,  -0.043804,  0.087350,   -0.085191,  0.009420,   0.086440
};

/* ----------------------------------------------------------------------
** Coefficients for 1-tap filter for F32
** ------------------------------------------------------------------- */

static ne10_float32_t testCoeffs1_f32 = 0.086440;

/* ----------------------------------------------------------------------
** Coefficients for 27-tap filter for F32
** ------------------------------------------------------------------- */
static ne10_float32_t testCoeffs27_f32[27] =
{
    0.010066,   0.047792,   0.021273,   -0.096447,  -0.211652,  -0.086613,  0.057501,   -0.187605,
    -0.167199,  -0.026983,  -0.025464,  -0.061495,  0.110914,   -0.081973,  -0.055231,  -0.074430,
    -0.196536,  0.016845,   -0.096493,  0.039625,   -0.110273,  -0.042966,  -0.043804,  0.087350,
    -0.085191,  0.009420,   0.086440
};

static ne10_float32_t testCoeffs6_f32[6] =
{
    -0.042966,  -0.043804,  0.087350,   -0.085191,  0.009420,   0.086440
};

/* ----------------------------------------------------------------------
** Test input data for F32
** Generated by the MATLAB rand() function
** ------------------------------------------------------------------- */

static ne10_float32_t testInput_f32[80] =
{
    -0.432565,  -1.665584,  0.125332,   0.287676,   -1.146471,  1.190915,   1.189164,   -0.037633,
    0.327292,   0.174639,   -0.186709,  0.725791,   -0.588317,  2.183186,   -0.136396,  0.113931,
    1.066768,   0.059281,   -0.095648,  -0.832349,  0.294411,   -1.336182,  0.714325,   1.623562,
    -0.691776,  0.857997,   1.254001,   -1.593730,  -1.440964,  0.571148,   -0.399886,  0.689997,
    0.815622,   0.711908,   1.290250,   0.668601,   1.190838,   -1.202457,  -0.019790,  -0.156717,
    -1.604086,  0.257304,   -1.056473,  1.415141,   -0.805090,  0.528743,   0.219321,   -0.921902,
    -2.170674,  -0.059188,  -1.010634,  0.614463,   0.507741,   1.692430,   0.591283,   -0.643595,
    0.380337,   -1.009116,  -0.019511,  -0.048221,  0.000043,   -0.317859,  1.095004,   -1.873990,
    0.428183,   0.895638,   0.730957,   0.577857,   0.040314,   0.677089,   0.568900,   -0.255645,
    -0.377469,  -0.295887,  -1.475135,  -0.234004,  0.118445,   0.314809,   1.443508,   -0.350975
};

/* ----------------------------------------------------------------------
** Defines each of the tests performed
** ------------------------------------------------------------------- */
typedef struct
{
    ne10_uint32_t blockSize;
    ne10_uint32_t numTaps;
    ne10_uint32_t D;
    ne10_uint32_t numFrames;
    ne10_float32_t *coeffsF32;
    ne10_float32_t *inputF32;
} test_config;

/* All Test configurations, 100% Code Coverage */
static test_config CONFIG[] = {{0, 1, 1, 10, &testCoeffs6_f32[0], &testInput_f32[0]},
    {8, 6, 6, 10, &testCoeffs6_f32[0], &testInput_f32[0]},
    {8, 8, 2, 10, &testCoeffs8_f32[0], &testInput_f32[0]},
    {8, 27, 4, 10, &testCoeffs27_f32[0], &testInput_f32[0]},
    {8, 32, 4, 10, &testCoeffs32_f32[0], &testInput_f32[0]},
    {80, 6, 6, 1, &testCoeffs6_f32[0], &testInput_f32[0]},
    {80, 8, 2, 1, &testCoeffs8_f32[0], &testInput_f32[0]},
    {80, 27, 4, 1, &testCoeffs27_f32[0], &testInput_f32[0]},
    {80, 32, 4, 1, &testCoeffs32_f32[0], &testInput_f32[0]}
};
static test_config CONFIG_PERF[] =
{
    {8, 27, 3, 10, &testCoeffs27_f32[0], &testInput_f32[0]},
    {8, 32, 4, 10, &testCoeffs32_f32[0], &testInput_f32[0]},
    {80, 27, 3, 1, &testCoeffs27_f32[0], &testInput_f32[0]},
    {80, 32, 4, 1, &testCoeffs32_f32[0], &testInput_f32[0]}
};

#define NUM_TESTS (sizeof(CONFIG) / sizeof(CONFIG[0]) )
#define NUM_PERF_TESTS (sizeof(CONFIG_PERF) / sizeof(CONFIG_PERF[0]) )


void test_fir_interpolate_case0()
{
    ne10_float32_t *p_src = testInput_f32;
    ne10_fir_interpolate_instance_f32_t SC, SN;

    ne10_uint16_t loop = 0;
    ne10_uint16_t block = 0;
    ne10_uint16_t k = 0;
    ne10_uint16_t i = 0;
    ne10_uint16_t pos = 0;
    ne10_uint16_t length = 0;

    test_config *config;
    ne10_result_t status_c = NE10_OK;
    ne10_result_t status_neon = NE10_OK;

    fprintf (stdout, "----------%30s start\n", __FUNCTION__);

    /* init input memory */
    NE10_SRC_ALLOC (in_c, guarded_in_c, TEST_LENGTH_SAMPLES); // 16 extra bytes at the begining and 16 extra bytes at the end
    NE10_SRC_ALLOC (in_neon, guarded_in_neon, TEST_LENGTH_SAMPLES); // 16 extra bytes at the begining and 16 extra bytes at the end

    /* init dst memory */
    NE10_DST_ALLOC (out_c, guarded_out_c, TEST_LENGTH_SAMPLES);
    NE10_DST_ALLOC (out_neon, guarded_out_neon, TEST_LENGTH_SAMPLES);

    /* init state memory */
    NE10_DST_ALLOC (fir_state_c, guarded_fir_state_c, MAX_NUMTAPS + MAX_BLOCKSIZE);
    NE10_DST_ALLOC (fir_state_neon, guarded_fir_state_neon, MAX_NUMTAPS + MAX_BLOCKSIZE);

#ifdef ENABLE_NE10_FIR_INTERPOLATE_FLOAT_NEON
#if defined (SMOKE_TEST)||(REGRESSION_TEST)
    for (loop = 0; loop < NUM_TESTS; loop++)
    {
        config = &CONFIG[loop];
        length = config->numFrames * config->blockSize * config->D;

        /* Initialize the CFFT/CIFFT module */
        status_c = ne10_fir_interpolate_init_float (&SC, config->D, config->numTaps, config->coeffsF32, fir_state_c, config->blockSize);
        status_neon = ne10_fir_interpolate_init_float (&SN, config->D, config->numTaps, config->coeffsF32, fir_state_neon, config->blockSize);

        if ( ( (status_c == NE10_ERR) || (status_neon == NE10_ERR)))
        {
            if (config->numTaps == 27)
            {
                fprintf (stdout, "length of input data is wrong!\n");
                continue;
            }
            else
            {
                fprintf (stdout, "initialization error\n");
            }
        }
        /* copy input to input buffer */
        for (i = 0; i < 80; i++)
        {
            in_c[i] = testInput_f32[i];
            in_neon[i] = testInput_f32[i];
        }

        GUARD_ARRAY (out_c, TEST_LENGTH_SAMPLES);
        GUARD_ARRAY (out_neon, TEST_LENGTH_SAMPLES);

        for (block = 0; block < config->numFrames; block++)
        {
            ne10_fir_interpolate_float_c (&SC, in_c + (block * config->blockSize), out_c + (block * config->blockSize * config->D), config->blockSize);
        }
        for (block = 0; block < config->numFrames; block++)
        {
            ne10_fir_interpolate_float_neon (&SN, in_neon + (block * config->blockSize), out_neon + (block * config->blockSize * config->D), config->blockSize);
        }

        CHECK_ARRAY_GUARD (out_c, TEST_LENGTH_SAMPLES);
        CHECK_ARRAY_GUARD (out_neon, TEST_LENGTH_SAMPLES);

        //conformance test 1: compare snr
        snr = CAL_SNR_FLOAT32 (out_c, out_neon, length);
        assert_false ( (snr < SNR_THRESHOLD));

        //conformance test 2: compare output of C and neon
#if defined (DEBUG_TRACE)
        printf ("--------------------config %d\n", loop);
        printf ("snr %f\n", snr);
#endif
        for (pos = 0; pos < length; pos++)
        {
#if defined (DEBUG_TRACE)
            printf ("pos %d \n", pos);
            printf ("c %f (0x%04X) neon %f (0x%04X)\n", out_c[pos], * (ne10_uint32_t*) &out_c[pos], out_neon[pos], * (ne10_uint32_t*) &out_neon[pos]);
#endif
            assert_float_vec_equal (&out_c[pos], &out_neon[pos], ERROR_MARGIN_SMALL, 1);
        }

    }
#endif
#endif // ENABLE_NE10_FIR_INTERPOLATE_FLOAT_NEON

#ifdef PERFORMANCE_TEST
    fprintf (stdout, "%25s%20s%20s%20s%20s\n", "FIR Length&Taps", "C Time in ms", "NEON Time in ms", "Time Savings", "Performance Ratio");
    for (loop = 0; loop < NUM_PERF_TESTS; loop++)
    {
        config = &CONFIG_PERF[loop];
        length = config->numFrames * config->blockSize * config->D;

        /* Initialize the CFFT/CIFFT module */
        status_c = ne10_fir_interpolate_init_float (&SC, config->D, config->numTaps, config->coeffsF32, fir_state_c, config->blockSize);
        status_neon = ne10_fir_interpolate_init_float (&SN, config->D, config->numTaps, config->coeffsF32, fir_state_neon, config->blockSize);

        if ( ( (status_c == NE10_ERR) || (status_neon == NE10_ERR)))
        {
            if (config->numTaps == 27)
            {
                fprintf (stdout, "length of input data is wrong!\n");
                continue;
            }
            else
            {
                fprintf (stdout, "initialization error\n");
            }
        }

        /* copy input to input buffer */
        for (i = 0; i < 80; i++)
        {
            in_c[i] = testInput_f32[i];
            in_neon[i] = testInput_f32[i];
        }

        GET_TIME
        (
            time_c,
        {
            for (k = 0; k < TEST_COUNT; k++)
            {
                for (block = 0; block < config->numFrames; block++)
                {
                    ne10_fir_interpolate_float_c (&SC, in_c + (block * config->blockSize), out_c + (block * config->blockSize * config->D), config->blockSize);
                }
            }
        }
        );
#ifdef ENABLE_NE10_FIR_INTERPOLATE_FLOAT_NEON
        GET_TIME
        (
            time_neon,
        {
            for (k = 0; k < TEST_COUNT; k++)
            {
                for (block = 0; block < config->numFrames; block++)
                {
                    ne10_fir_interpolate_float_neon (&SN, in_neon + (block * config->blockSize), out_neon + (block * config->blockSize * config->D), config->blockSize);
                }
            }
        }
        );
#endif // ENABLE_NE10_FIR_INTERPOLATE_FLOAT_NEON

        time_speedup = (ne10_float32_t) time_c / time_neon;
        time_savings = ( ( (ne10_float32_t) (time_c - time_neon)) / time_c) * 100;
        ne10_log (__FUNCTION__, "%20d,%4d%20lld%20lld%19.2f%%%18.2f:1\n", config->numTaps, time_c, time_neon, time_savings, time_speedup);
    }
#endif

    free (guarded_in_c);
    free (guarded_in_neon);
    free (guarded_out_c);
    free (guarded_out_neon);
    free (guarded_fir_state_c);
    free (guarded_fir_state_neon);
    fprintf (stdout, "----------%30s end\n", __FUNCTION__);
}

void test_fir_interpolate()
{
    test_fir_interpolate_case0();
}

static void my_test_setup (void)
{
    ne10_log_buffer_ptr = ne10_log_buffer;
}

void test_fixture_fir_interpolate (void)
{
    test_fixture_start();               // starts a fixture

    fixture_setup (my_test_setup);

    run_test (test_fir_interpolate);       // run tests

    test_fixture_end();                 // ends a fixture
}

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
 * NE10 Library : test_suite_fir.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "NE10_dsp.h"
#include "seatest.h"


/* ----------------------------------------------------------------------
** Global defines
** ------------------------------------------------------------------- */

/* Max data Length and block size, numTaps */
#define TEST_LENGTH_SAMPLES 320
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
** Coefficients for 3-tap filter  for F32
** ------------------------------------------------------------------- */

static ne10_float32_t testCoeffs3_f32[3] =
{
    0.125332306474830680,    -1.665584378238097000,    -0.432564811528220680
};

static ne10_float32_t testCoeffs4_f32[4] =
{
    1.189164201652103100,    1.190915465642998800,    -1.146471350681463700,    0.287676420358548850
};

/* ----------------------------------------------------------------------
** Coefficients for 7-tap filter for F32
** ------------------------------------------------------------------- */

static ne10_float32_t testCoeffs7_f32[7] =
{
    1.189164201652103100,    1.190915465642998800,    -1.146471350681463700,    0.287676420358548850,    0.125332306474830680,    -1.665584378238097000,    -0.432564811528220680
};

/* ----------------------------------------------------------------------
** Coefficients for 1-tap filter for F32
** ------------------------------------------------------------------- */

ne10_float32_t testCoeffs1_f32 = -0.432564811528220680;

/* ----------------------------------------------------------------------
** Coefficients for 32-tap filter for F32
** ------------------------------------------------------------------- */
static ne10_float32_t testCoeffs32_f32[32] =
{
    0.689997375464345140,    -0.399885577715363150,    0.571147623658177950,    -1.440964431901020000,    -1.593729576447476800,    1.254001421602532400,    0.857996672828262640,    -0.691775701702286750,
    1.623562064446270700,    0.714324551818952160,    -1.336181857937804000,    0.294410816392640380,    -0.832349463650022490,    -0.095648405483669041,    0.059281460523605348,    1.066768211359188800,
    0.113931313520809620,    -0.136395883086595700,    2.183185818197101100,    -0.588316543014188680,    0.725790548293302700,    -0.186708577681439360,    0.174639142820924520,    0.327292361408654140,
    -0.037633276593317645,    1.189164201652103100,    1.190915465642998800,    -1.146471350681463700,    0.287676420358548850,    0.125332306474830680,    -1.665584378238097000,    -0.432564811528220680

};

/* ----------------------------------------------------------------------
** Test input data for F32
** Generated by the MATLAB rand() function
** ------------------------------------------------------------------- */

static ne10_float32_t testInput_f32[TEST_LENGTH_SAMPLES] =
{
    -0.432564811528220680,    -1.665584378238097000,    0.125332306474830680,    0.287676420358548850,    -1.146471350681463700,    1.190915465642998800,    1.189164201652103100,    -0.037633276593317645,
    0.327292361408654140,    0.174639142820924520,    -0.186708577681439360,    0.725790548293302700,    -0.588316543014188680,    2.183185818197101100,    -0.136395883086595700,    0.113931313520809620,
    1.066768211359188800,    0.059281460523605348,    -0.095648405483669041,    -0.832349463650022490,    0.294410816392640380,    -1.336181857937804000,    0.714324551818952160,    1.623562064446270700,
    -0.691775701702286750,    0.857996672828262640,    1.254001421602532400,    -1.593729576447476800,    -1.440964431901020000,    0.571147623658177950,    -0.399885577715363150,    0.689997375464345140,
    0.815622288876143300,    0.711908323500893280,    1.290249754932477000,    0.668600505682040320,    1.190838074243369100,    -1.202457114773944000,    -0.019789557768770449,    -0.156717298831980680,
    -1.604085562001158500,    0.257304234677489860,    -1.056472928081482400,    1.415141485872338600,    -0.805090404196879830,    0.528743010962224870,    0.219320672667622370,    -0.921901624355539130,
    -2.170674494305262500,    -0.059187824521191180,    -1.010633706474247400,    0.614463048895480980,    0.507740785341985520,    1.692429870190521400,    0.591282586924175900,    -0.643595202682526120,
    0.380337251713910140,    -1.009115524340785000,    -0.019510669530289293,    -0.048220789145312269,    0.000043191841625545,    -0.317859451247687890,    1.095003738787492500,    -1.873990257640960800,
    0.428183273045162850,    0.895638471211751770,    0.730957338429453320,    0.577857346330798440,    0.040314031618440292,    0.677089187597304740,    0.568900205200723040,    -0.255645415631964800,
    -0.377468955522361260,    -0.295887110003557050,    -1.475134505855259400,    -0.234004047656033030,    0.118444837054121300,    0.314809043395055830,    1.443508244349820600,    -0.350974738327741790,
    0.623233851138494170,    0.799048618147778280,    0.940889940727780430,    -0.992091735543795260,    0.212035152165055420,    0.237882072875578690,    -1.007763391678268000,    -0.742044752133603880,
    1.082294953155333600,    -0.131499702945273520,    0.389880489687038980,    0.087987106579793015,    -0.635465225479316160,    -0.559573302196241020,    0.443653489503667400,    -0.949903798547645390,
    0.781181617878391470,    0.568960645723273870,    -0.821714291696255650,    -0.265606851332549080,    -1.187777016469804000,    -2.202320717323438300,    0.986337391002022670,    -0.518635066344746210,
    0.327367564080834390,    0.234057012847184940,    0.021466138879094456,    -1.003944466747724900,    -0.947146064738541350,    -0.374429195029165610,    -1.185886213808528200,    -1.055902923523691000,
    1.472479934419915100,    0.055743831837843170,    -1.217317453704551000,    -0.041227133686432105,    -1.128343864320228600,    -1.349277543102494600,    -0.261101623061621050,    0.953465445504818490,
    0.128644430046645000,    0.656467513885396040,    -1.167819364726638800,    -0.460605179506150430,    -0.262439952838332660,    -1.213152068493906600,    -1.319436998109536900,    0.931217514995436150,
    0.011244896384133726,    -0.645145815691170240,    0.805728793112375660,    0.231626010780436540,    -0.989759671682004180,    1.339585700610387500,    0.289502034538413220,    1.478917057681278000,
    1.138028012858370600,    -0.684138585136339630,    -1.291936044965937800,    -0.072926276263646728,    -0.330598879892764320,    -0.843627639154799660,    0.497769664182782460,    1.488490470903483400,
    -0.546475894767622590,    -0.846758163883059470,    -0.246336528084899750,    0.663024145855907740,    -0.854197374468979920,    -1.201314815339040900,    -0.119869428057387190,    -0.065294014841586534,
    0.485295555916543940,    -0.595490902619475900,    -0.149667743824475260,    -0.434751931152533360,    -0.079330223023420576,    1.535152266122147500,    -0.606482859277265640,    -1.347362673850240400,
    0.469383119866330020,    -0.903566942617776370,    0.035879638729476929,    -0.627531219966831480,    0.535397954249105970,    0.552883517423822020,    -0.203690479567357890,    -2.054324680556606000,
    0.132560731417279840,    1.592940703766015300,    1.018411788624710400,    -1.580402499303162200,    -0.078661919359452090,    -0.681656860002363030,    -1.024553057429031600,    -1.234353477984261800,
    0.288807018730339650,    -0.429303004551915000,    0.055801190176472580,    -0.367873566740638040,    -0.464973367171118420,    0.370960583848951750,    0.728282931551494710,    2.112160169771504700,
    -1.357297743096753200,    -1.022610144334205900,    1.037834198718760300,    -0.389799548476830680,    -1.381265624019837300,    0.315542632772364660,    1.553242568515348100,    0.707893884632475820,
    1.957384755147506100,    0.504542353592165700,    1.864529020485302900,    -0.339811777414963770,    -1.139779402313234800,    -0.211123483380257990,    1.190244936251201500,    -1.116208757785609900,
    0.635274134747121470,    -0.601412126269725180,    0.551184711824902030,    -1.099840454710813400,    0.085990593293718429,    -2.004563321590791900,    -0.493087917659696950,    0.462048011799193080,
    -0.321004692181292070,    1.236555651601916100,    -0.631279656725146410,    -2.325211128883771100,    -1.231636533325015200,    1.055648387902459600,    -0.113223989369024890,    0.379223622685032900,
    0.944199726747308340,    -2.120426688224211500,    -0.644678915541936900,    -0.704301728433608940,    -1.018137216399070700,    -0.182081868411385240,    1.521013239005587000,    -0.038438763886711559,
    1.227447989009716500,    -0.696204800032888760,    0.007524486523014446,    -0.782893044378287220,    0.586938559214430940,    -0.251207374568881810,    0.480135822842600760,    0.668155034433640550,
    -0.078321196273411942,    0.889172618412599090,    2.309287485952386600,    0.524638679771098350,    -0.011787323951306753,    0.913140817761370680,    0.055940678888401998,    -1.107069894826007200,
    0.485497707312810220,    -0.005005073755531385,    -0.276217859354758950,    1.276452473674392700,    1.863400613184537500,    -0.522559301636399080,    0.103424446937314980,    -0.807649130897180490,
    0.680438583748945720,    -2.364589847941581000,    0.990114872049490450,    0.218899120881176610,    0.261662460161401660,    1.213444494975346900,    -0.274666986456781450,    -0.133134450813529370,
    -1.270500203708376600,    -1.663606452829772000,    -0.703554261536754930,    0.280880488523302110,    -0.541209329916194080,    -1.333530729736392500,    1.072686267890143200,    -0.712085452494355840,
    -0.011285561230685560,    -0.000817029195695836,    -0.249436284695434440,    0.396575318711651580,    -0.264013354922243150,    -1.664010876930589000,    -1.028975099543801000,    0.243094700224565000,
    -1.256590107833816600,    -0.347183189733526130,    -0.941372193428328560,    -1.174560281302443800,    -1.021141686935775000,    -0.401666734596788310,    0.173665668562307250,    -0.116118493350510720,
    1.064119148986353500,    -0.245386296751669620,    -1.517539131089555600,    0.009734159125951119,    0.071372864855954732,    0.316535813768508200,    0.499825667796478360,    1.278084146714109700,
    -0.547816146921157760,    0.260808398879074590,    -0.013176671873511559,    -0.580264002141952510,    2.136308422805308600,    -0.257617115653480830,    -1.409528489369198400,    1.770100892851614400,
    0.325545984760710010,    -1.119039575381311600,    0.620350139445524750,    1.269781847189774600,    -0.896042506421914520,    0.135175444758436850,    -0.139040010040442590,    -1.163395293837265400,
    1.183719539936856500,    -0.015429661783325022,    0.536218694718617050,    -0.716428623725855470,    -0.655559389503905910,    0.314362763310748140,    0.106814075934587750,    1.848216218018968700,
    -0.275105675438811310,    2.212554078989680900,    1.508525756096146700,    -1.945078599919331000,    -1.680542777522645400,    -0.573534134105876060,    -0.185816527367659470,    0.008934115676567702
};

/* ----------------------------------------------------------------------
** Defines each of the tests performed
** ------------------------------------------------------------------- */
typedef struct
{
    ne10_uint32_t blockSize;
    ne10_uint32_t numTaps;
    ne10_uint32_t numFrames;
    ne10_float32_t *coeffsF32;
    ne10_float32_t *inputF32;
} test_config;

/* Test configurationsfor conformance test, 100% Code Coverage */
static test_config CONFIG[] =
{
    {64, 32, 5, &testCoeffs32_f32[0], &testInput_f32[0]},
    {64, 3, 5, &testCoeffs3_f32[0], &testInput_f32[0]},
    {64, 7, 5, &testCoeffs7_f32[0], &testInput_f32[0]},
    {64, 1, 5, &testCoeffs1_f32, &testInput_f32[0]},
    {5, 3, 64, &testCoeffs3_f32[0], &testInput_f32[0]},
    {2, 7, 160, &testCoeffs7_f32[0], &testInput_f32[0]},
    {4, 1, 80, &testCoeffs1_f32, &testInput_f32[0]},
    {32, 32, 10, &testCoeffs32_f32[0], &testInput_f32[0]},
    {7, 4, 1, &testCoeffs4_f32[0], &testInput_f32[0]},
    {8, 4, 1, &testCoeffs4_f32[0], &testInput_f32[0]},
    {9, 4, 1, &testCoeffs4_f32[0], &testInput_f32[0]},
};
/* Test configurations for performance test */
static test_config CONFIG_PERF[] =
{
    {64, 32, 5, &testCoeffs32_f32[0], &testInput_f32[0]},
    {64, 3, 5, &testCoeffs3_f32[0], &testInput_f32[0]},
    {64, 7, 5, &testCoeffs7_f32[0], &testInput_f32[0]},
};

#define NUM_TESTS (sizeof(CONFIG) / sizeof(CONFIG[0]) )
#define NUM_PERF_TESTS (sizeof(CONFIG_PERF) / sizeof(CONFIG_PERF[0]) )

void test_fir_case0()
{
    ne10_float32_t *p_src = testInput_f32;
    ne10_fir_instance_f32_t SC, SN;

    ne10_uint16_t loop = 0;
    ne10_uint16_t block = 0;
    ne10_uint16_t k = 0;
    ne10_uint16_t i = 0;
    ne10_uint16_t pos = 0;

    test_config *config;
    ne10_result_t status = NE10_OK;

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

    /*
     * @TODO Separate neon and c version test. Mixing of these 2 tests makes
     * it difficult to disable and enable one of them. Current macro
     * ENABLE_NE10_FIR_FLOAT_NEON disables both of them, which should only
     * disable the neon version ideally.
     */
#ifdef ENABLE_NE10_FIR_FLOAT_NEON
#if defined (SMOKE_TEST)||(REGRESSION_TEST)
    for (loop = 0; loop < NUM_TESTS; loop++)
    {
        config = &CONFIG[loop];

        /* Initialize the CFFT/CIFFT module */
        ne10_fir_init_float (&SC, config->numTaps, config->coeffsF32, fir_state_c, config->blockSize);
        ne10_fir_init_float (&SN, config->numTaps, config->coeffsF32, fir_state_neon, config->blockSize);

        /* copy input to input buffer */
        for (i = 0; i < TEST_LENGTH_SAMPLES; i++)
        {
            in_c[i] = testInput_f32[i];
            in_neon[i] = testInput_f32[i];
        }

        GUARD_ARRAY (out_c, TEST_LENGTH_SAMPLES);
        GUARD_ARRAY (out_neon, TEST_LENGTH_SAMPLES);

        for (block = 0; block < config->numFrames; block++)
        {
            ne10_fir_float_c (&SC, in_c + (block * config->blockSize), out_c + (block * config->blockSize), config->blockSize);
        }

        for (block = 0; block < config->numFrames; block++)
        {
            ne10_fir_float_neon (&SN, in_neon + (block * config->blockSize), out_neon + (block * config->blockSize), config->blockSize);
        }

        CHECK_ARRAY_GUARD (out_c, TEST_LENGTH_SAMPLES);
        CHECK_ARRAY_GUARD (out_neon, TEST_LENGTH_SAMPLES);

        //conformance test 1: compare snr
        snr = CAL_SNR_FLOAT32 (out_c, out_neon, TEST_LENGTH_SAMPLES);
        assert_false ( (snr < SNR_THRESHOLD));

        //conformance test 2: compare output of C and neon
#if defined (DEBUG_TRACE)
        printf ("--------------------config %d\n", loop);
        printf ("snr %f\n", snr);
#endif
        for (pos = 0; pos < TEST_LENGTH_SAMPLES; pos++)
        {
#if defined (DEBUG_TRACE)
            printf ("pos %d \n", pos);
            printf ("c %f (0x%04X) neon %f (0x%04X)\n", out_c[pos], * (ne10_uint32_t*) &out_c[pos], out_neon[pos], * (ne10_uint32_t*) &out_neon[pos]);
#endif
            assert_float_vec_equal (&out_c[pos], &out_neon[pos], ERROR_MARGIN_SMALL, 1);
        }
    }
#endif
#endif // ENABLE_NE10_FIR_FLOAT_NEON

#ifdef PERFORMANCE_TEST
    fprintf (stdout, "%25s%20s%20s%20s%20s\n", "FIR Length&Taps", "C Time in ms", "NEON Time in ms", "Time Savings", "Performance Ratio");
    for (loop = 0; loop < NUM_PERF_TESTS; loop++)
    {
        config = &CONFIG_PERF[loop];

        /* Initialize the CFFT/CIFFT module */
        ne10_fir_init_float (&SC, config->numTaps, config->coeffsF32, fir_state_c, config->blockSize);
        ne10_fir_init_float (&SN, config->numTaps, config->coeffsF32, fir_state_neon, config->blockSize);

        /* copy input to input buffer */
        for (i = 0; i < TEST_LENGTH_SAMPLES; i++)
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
                    ne10_fir_float_c (&SC, in_c + (block * config->blockSize), out_c + (block * config->blockSize), config->blockSize);
                }
            }
        }
        );

#ifdef ENABLE_NE10_FIR_FLOAT_NEON
        GET_TIME
        (
            time_neon,
        {
            for (k = 0; k < TEST_COUNT; k++)
            {
                for (block = 0; block < config->numFrames; block++)
                {
                    ne10_fir_float_neon (&SN, in_neon + (block * config->blockSize), out_neon + (block * config->blockSize), config->blockSize);
                }
            }
        }
        );
#endif // ENABLE_NE10_FIR_FLOAT_NEON

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

void test_fir()
{
    test_fir_case0();
}

/* ----------------------------------------------------------------------
** end of fir test
** ------------------------------------------------------------------- */

static void my_test_setup (void)
{
    ne10_log_buffer_ptr = ne10_log_buffer;
}

void test_fixture_fir (void)
{
    test_fixture_start();               // starts a fixture

    fixture_setup (my_test_setup);

    run_test (test_fir);       // run tests

    test_fixture_end();                 // ends a fixture
}

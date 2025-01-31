﻿/*
 *  Copyright 2011-15 ARM Limited and Contributors.
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
 * NE10 Library : math/NE10_addmat.c
 */

#include "NE10_types.h"
#include "macros.h"

#include <assert.h>
namespace youme {

ne10_result_t ne10_mulmat_2x2f_c (ne10_mat2x2f_t * dst, ne10_mat2x2f_t * src1, ne10_mat2x2f_t * src2, ne10_uint32_t count)
{
#define A1 src1[ itr ].c1.r1
#define A2 src2[ itr ].c1.r1
#define B1 src1[ itr ].c1.r2
#define B2 src2[ itr ].c1.r2
#define C1 src1[ itr ].c2.r1
#define C2 src2[ itr ].c2.r1
#define D1 src1[ itr ].c2.r2
#define D2 src2[ itr ].c2.r2

    NE10_X_OPERATION_FLOAT_C
    (
        dst[ itr ].c1.r1 = (A1 * A2) + (C1 * B2);
        dst[ itr ].c1.r2 = (B1 * A2) + (D1 * B2);

        dst[ itr ].c2.r1 = (A1 * C2) + (C1 * D2);
        dst[ itr ].c2.r2 = (B1 * C2) + (D1 * D2);
    );

#undef A1
#undef A2
#undef B1
#undef B2
#undef C1
#undef C2
#undef D1
#undef D2
}

ne10_result_t ne10_mulmat_3x3f_c (ne10_mat3x3f_t * dst, ne10_mat3x3f_t * src1, ne10_mat3x3f_t * src2, ne10_uint32_t count)
{
#define A1 src1[ itr ].c1.r1
#define A2 src2[ itr ].c1.r1
#define B1 src1[ itr ].c1.r2
#define B2 src2[ itr ].c1.r2
#define C1 src1[ itr ].c1.r3
#define C2 src2[ itr ].c1.r3
#define D1 src1[ itr ].c2.r1
#define D2 src2[ itr ].c2.r1
#define E1 src1[ itr ].c2.r2
#define E2 src2[ itr ].c2.r2
#define F1 src1[ itr ].c2.r3
#define F2 src2[ itr ].c2.r3
#define G1 src1[ itr ].c3.r1
#define G2 src2[ itr ].c3.r1
#define H1 src1[ itr ].c3.r2
#define H2 src2[ itr ].c3.r2
#define I1 src1[ itr ].c3.r3
#define I2 src2[ itr ].c3.r3

    NE10_X_OPERATION_FLOAT_C
    (
        dst[ itr ].c1.r1 = (A1 * A2) + (D1 * B2) + (G1 * C2);
        dst[ itr ].c1.r2 = (B1 * A2) + (E1 * B2) + (H1 * C2);
        dst[ itr ].c1.r3 = (C1 * A2) + (F1 * B2) + (I1 * C2);

        dst[ itr ].c2.r1 = (A1 * D2) + (D1 * E2) + (G1 * F2);
        dst[ itr ].c2.r2 = (B1 * D2) + (E1 * E2) + (H1 * F2);
        dst[ itr ].c2.r3 = (C1 * D2) + (F1 * E2) + (I1 * F2);

        dst[ itr ].c3.r1 = (A1 * G2) + (D1 * H2) + (G1 * I2);
        dst[ itr ].c3.r2 = (B1 * G2) + (E1 * H2) + (H1 * I2);
        dst[ itr ].c3.r3 = (C1 * G2) + (F1 * H2) + (I1 * I2);
    );

#undef A1
#undef A2
#undef B1
#undef B2
#undef C1
#undef C2
#undef D1
#undef D2
#undef E1
#undef E2
#undef F1
#undef F2
#undef G1
#undef G2
#undef H1
#undef H2
#undef I1
#undef I2
}

ne10_result_t ne10_mulmat_4x4f_c (ne10_mat4x4f_t * dst, ne10_mat4x4f_t * src1, ne10_mat4x4f_t * src2, ne10_uint32_t count)
{
#define A1 src1[ itr ].c1.r1
#define A2 src2[ itr ].c1.r1
#define B1 src1[ itr ].c1.r2
#define B2 src2[ itr ].c1.r2
#define C1 src1[ itr ].c1.r3
#define C2 src2[ itr ].c1.r3
#define D1 src1[ itr ].c1.r4
#define D2 src2[ itr ].c1.r4

#define E1 src1[ itr ].c2.r1
#define E2 src2[ itr ].c2.r1
#define F1 src1[ itr ].c2.r2
#define F2 src2[ itr ].c2.r2
#define G1 src1[ itr ].c2.r3
#define G2 src2[ itr ].c2.r3
#define H1 src1[ itr ].c2.r4
#define H2 src2[ itr ].c2.r4

#define I1 src1[ itr ].c3.r1
#define I2 src2[ itr ].c3.r1
#define J1 src1[ itr ].c3.r2
#define J2 src2[ itr ].c3.r2
#define K1 src1[ itr ].c3.r3
#define K2 src2[ itr ].c3.r3
#define L1 src1[ itr ].c3.r4
#define L2 src2[ itr ].c3.r4

#define M1 src1[ itr ].c4.r1
#define M2 src2[ itr ].c4.r1
#define N1 src1[ itr ].c4.r2
#define N2 src2[ itr ].c4.r2
#define O1 src1[ itr ].c4.r3
#define O2 src2[ itr ].c4.r3
#define P1 src1[ itr ].c4.r4
#define P2 src2[ itr ].c4.r4

    NE10_X_OPERATION_FLOAT_C
    (
        dst[ itr ].c1.r1 = (A1 * A2) + (E1 * B2) + (I1 * C2) + (M1 * D2);
        dst[ itr ].c1.r2 = (B1 * A2) + (F1 * B2) + (J1 * C2) + (N1 * D2);
        dst[ itr ].c1.r3 = (C1 * A2) + (G1 * B2) + (K1 * C2) + (O1 * D2);
        dst[ itr ].c1.r4 = (D1 * A2) + (H1 * B2) + (L1 * C2) + (P1 * D2);

        dst[ itr ].c2.r1 = (A1 * E2) + (E1 * F2) + (I1 * G2) + (M1 * H2);
        dst[ itr ].c2.r2 = (B1 * E2) + (F1 * F2) + (J1 * G2) + (N1 * H2);
        dst[ itr ].c2.r3 = (C1 * E2) + (G1 * F2) + (K1 * G2) + (O1 * H2);
        dst[ itr ].c2.r4 = (D1 * E2) + (H1 * F2) + (L1 * G2) + (P1 * H2);

        dst[ itr ].c3.r1 = (A1 * I2) + (E1 * J2) + (I1 * K2) + (M1 * L2);
        dst[ itr ].c3.r2 = (B1 * I2) + (F1 * J2) + (J1 * K2) + (N1 * L2);
        dst[ itr ].c3.r3 = (C1 * I2) + (G1 * J2) + (K1 * K2) + (O1 * L2);
        dst[ itr ].c3.r4 = (D1 * I2) + (H1 * J2) + (L1 * K2) + (P1 * L2);

        dst[ itr ].c4.r1 = (A1 * M2) + (E1 * N2) + (I1 * O2) + (M1 * P2);
        dst[ itr ].c4.r2 = (B1 * M2) + (F1 * N2) + (J1 * O2) + (N1 * P2);
        dst[ itr ].c4.r3 = (C1 * M2) + (G1 * N2) + (K1 * O2) + (O1 * P2);
        dst[ itr ].c4.r4 = (D1 * M2) + (H1 * N2) + (L1 * O2) + (P1 * P2);
    );

#undef A1
#undef A2
#undef B1
#undef B2
#undef C1
#undef C2
#undef D1
#undef D2
#undef E1
#undef E2
#undef F1
#undef F2
#undef G1
#undef G2
#undef H1
#undef H2
#undef I1
#undef I2
#undef J1
#undef J2
#undef K1
#undef K2
#undef L1
#undef L2
#undef M1
#undef M2
#undef N1
#undef N2
#undef O1
#undef O2
#undef P1
#undef P2
}


}  // namespace youme
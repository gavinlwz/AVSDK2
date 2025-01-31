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
 * NE10 Library : math/NE10_identitymat.c
 */

#include "NE10_types.h"
#include "macros.h"

namespace youme {

ne10_result_t ne10_identitymat_2x2f_c (ne10_mat2x2f_t * dst, ne10_uint32_t count)
{
    ne10_mat2x2f_t *src = dst; // dummy placeholder

    NE10_DETMAT_OPERATION_X_C
    (
        dst[ itr ].c1.r1 =  1.0f;
        dst[ itr ].c1.r2 =  0.0f;
        dst[ itr ].c2.r1 =  0.0f;
        dst[ itr ].c2.r2 =  1.0f;
    );
}

ne10_result_t ne10_identitymat_3x3f_c (ne10_mat3x3f_t * dst, ne10_uint32_t count)
{
    ne10_mat3x3f_t *src = dst; // dummy placeholder

    NE10_DETMAT_OPERATION_X_C
    (
        dst[ itr ].c1.r1 =  1.0f;
        dst[ itr ].c1.r2 =  0.0f;
        dst[ itr ].c1.r3 =  0.0f;

        dst[ itr ].c2.r1 =  0.0f;
        dst[ itr ].c2.r2 =  1.0f;
        dst[ itr ].c2.r3 =  0.0f;

        dst[ itr ].c3.r1 =  0.0f;
        dst[ itr ].c3.r2 =  0.0f;
        dst[ itr ].c3.r3 =  1.0f;
    );
}

ne10_result_t ne10_identitymat_4x4f_c (ne10_mat4x4f_t * dst, ne10_uint32_t count)
{
    ne10_mat4x4f_t *src = dst; // dummy placeholder

    NE10_DETMAT_OPERATION_X_C
    (
        dst[ itr ].c1.r1 =  1.0f;
        dst[ itr ].c1.r2 =  0.0f;
        dst[ itr ].c1.r3 =  0.0f;
        dst[ itr ].c1.r4 =  0.0f;

        dst[ itr ].c2.r1 =  0.0f;
        dst[ itr ].c2.r2 =  1.0f;
        dst[ itr ].c2.r3 =  0.0f;
        dst[ itr ].c2.r4 =  0.0f;

        dst[ itr ].c3.r1 =  0.0f;
        dst[ itr ].c3.r2 =  0.0f;
        dst[ itr ].c3.r3 =  1.0f;
        dst[ itr ].c3.r4 =  0.0f;

        dst[ itr ].c4.r1 =  0.0f;
        dst[ itr ].c4.r2 =  0.0f;
        dst[ itr ].c4.r3 =  0.0f;
        dst[ itr ].c4.r4 =  1.0f;
    );
}

}  // namespace youme
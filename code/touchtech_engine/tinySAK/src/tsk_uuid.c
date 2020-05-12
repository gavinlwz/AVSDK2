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

/**@file tsk_uuid.c
 * @brief Universally Unique Identifier (UUID version 5) implementation (RFC 4122).
 *		  This implementation is not fully conform to RFC 4122 but could be safely used to generate
 random UUIDs.
 *
 *
 *

 */
#include "tsk_uuid.h"

#include "tsk_sha1.h"
#include "tsk_string.h"
#include "tsk_time.h"

#include <stdlib.h>
#include <string.h>

/**@defgroup tsk_uuid_group niversally Unique Identifier (UUID version 5) implementation (RFC 4122).
*/

/**@ingroup tsk_uuid_group
*/
int tsk_uuidgenerate (tsk_uuidstring_t *result)
{
    /* From wikipedia
    *	Version 5 UUIDs use a scheme with SHA-1 hashing, otherwise it is the same idea as in version
    *3.
    *	RFC 4122 states that version 5 is preferred over version 3 name based UUIDs.
    *	Note that the 160 bit SHA-1 hash is truncated to 128 bits to make the length work out.
    */
    tsk_sha1string_t sha1result;
    tsk_istr_t now;
    unsigned i, k;
    static char HEX[] = "0123456789abcdef";

    tsk_itoa (tsk_time_now (), &now);
    tsk_sha1compute (now, sizeof (now), &sha1result);

    /* XOR the SHA-1 result with random numbers. */
    for (i = 0; i < (TSK_UUID_DIGEST_SIZE * 2); i += 4)
    {
#if 0
		*((uint32_t*)&sha1result[i]) ^= rand();
#else
        k = rand ();
        sha1result[i] ^= k, sha1result[i + 1] ^= k, sha1result[i + 2] ^= k, sha1result[i + 3] ^= k;
#endif

        for (k = 0; k < sizeof (uint32_t); k++)
        {
            sha1result[i + k] = HEX[sha1result[i + k] & 0x0F]; /* To hexa. */
        }
    }

    /* f47ac10b-58cc-4372-a567-0e02b2c3d479 */
    memcpy (&(*result)[0], &sha1result[0], 8);
    (*result)[8] = '-';

    memcpy (&(*result)[9], &sha1result[8], 4);
    (*result)[13] = '-';

    memcpy (&(*result)[14], &sha1result[12], 4);
    (*result)[18] = '-';

    memcpy (&(*result)[19], &sha1result[16], 4);
    (*result)[23] = '-';

    memcpy (&(*result)[24], &sha1result[20], 12);
    (*result)[36] = '\0';

    return 0;
}

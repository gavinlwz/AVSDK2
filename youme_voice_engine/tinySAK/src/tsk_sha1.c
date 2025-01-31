﻿/*
 *
 * Based on the RFC 3174
 * 
 * Full Copyright Statement
 *
 *  Copyright (C) The Internet Society (2001).  All Rights Reserved.
 *  Copyright (C) Mamadou Diop		   (2009)
 *
 *  This document and translations of it may be copied and furnished to
 *  others, and derivative works that comment on or otherwise explain it
 *  or assist in its implementation may be prepared, copied, published
 *  and distributed, in whole or in part, without restriction of any
 *  kind, provided that the above copyright notice and this paragraph are
 *  included on all such copies and derivative works.  However, this
 *  document itself may not be modified in any way, such as by removing
 *  the copyright notice or references to the Internet Society or other
 *  Internet organizations, except as needed for the purpose of
 *  developing Internet standards in which case the procedures for
 *  copyrights defined in the Internet Standards process must be
 *  followed, or as required to translate it into languages other than
 *  English.
 *
 *  The limited permissions granted above are perpetual and will not be
 *  revoked by the Internet Society or its successors or assigns.

 *  This document and the information contained herein is provided on an
 *  "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
 *  TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
 *  BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
 *  HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
 *  MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * 
 *  Description:
 *	  This file implements the Secure Hashing Algorithm 1 as
 *	  defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *	  The SHA-1, produces a 160-bit message digest for a given
 *	  data stream.  It should take about 2**n steps to find a
 *	  message with the same digest as a given message and
 *	  2**(n/2) to find any two messages with the same digest,
 *	  when n is the digest size in bits.  Therefore, this
 *	  algorithm can serve as a means of providing a
 *	  "fingerprint" for a message.
 *
 *  Portability Issues:
 *	  SHA-1 is defined in terms of 32-bit "words".  This code
 *	  uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *	  bit unsigned integer types.  If your C compiler does not
 *	  support 32 bit unsigned integers, this code is not
 *	  appropriate.
 *
 *  Caveats:
 *	  SHA-1 is designed to work with messages less than 2^64 bits
 *	  long.  Although SHA-1 allows a message digest to be generated
 *	  for messages of any number of bits less than 2^64, this
 *	  implementation only works with messages with a length that is
 *	  a multiple of the size of an 8-bit character.
 *
 */

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

/**@file tsk_sha1.c
 * @brief US Secure Hash Algorithm 1 (RFC 3174)
 *
 *
 *

 */
#include "tsk_sha1.h"

#include "tsk_string.h"

/**@defgroup tsk_sha1_group SHA1 (RFC 3174) utility functions.
 *  Copyright (C) The Internet Society (2001).  All Rights Reserved.<br>
 *  Copyright (C) Mamadou Diop		   (2009)<br>
 *
 *	This file implements the Secure Hashing Algorithm 1 as
 *	defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *	  The SHA-1, produces a 160-bit message digest for a given
 *	  data stream.  It should take about 2**n steps to find a
 *	  message with the same digest as a given message and
 *	  2**(n/2) to find any two messages with the same digest,
 *	  when n is the digest size in bits.  Therefore, this
 *	  algorithm can serve as a means of providing a
 *	  "fingerprint" for a message.
 * 
*/

/**@ingroup tsk_sha1_group
 *  Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

/* Local Function Prototyptes */
void SHA1PadMessage(tsk_sha1context_t *);
void SHA1ProcessMessageBlock(tsk_sha1context_t *);

/**@ingroup tsk_sha1_group
 *
 *  This function will initialize the @a context in preparation
 *  for computing a new SHA1 message digest.
 *
 *@param context The context to reset.
 *
 *@retval @ref tsk_sha1_errcode_t code.
 */
tsk_sha1_errcode_t tsk_sha1reset(tsk_sha1context_t *context)
{
    if (!context){
        return shaNull;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x67452301;
    context->Intermediate_Hash[1]   = 0xEFCDAB89;
    context->Intermediate_Hash[2]   = 0x98BADCFE;
    context->Intermediate_Hash[3]   = 0x10325476;
    context->Intermediate_Hash[4]   = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;

    return shaSuccess;
}

/**@ingroup tsk_sha1_group
 *  This function will return the 160-bit message digest into the
 *  Message_Digest array  provided by the caller.
 *  NOTE: The first octet of hash is stored in the 0th element,
 *        the last octet of hash in the 19th element.
 * @param context The @a context to use to calculate the SHA-1 hash.
 * @param Message_Digest A pointer the the sha1 digest result.
 * @retval @ref tsk_sha1_errcode_t code.
 */
tsk_sha1_errcode_t tsk_sha1result( tsk_sha1context_t *context, tsk_sha1digest_t Message_Digest)
{
    int32_t i;

    if (!context || !Message_Digest){
        return shaNull;
    }

    if (context->Corrupted){
        return (tsk_sha1_errcode_t)context->Corrupted;
    }

    if (!context->Computed){
        SHA1PadMessage(context);
        for(i=0; i<64; ++i){
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;

    }

    for(i = 0; i < TSK_SHA1_DIGEST_SIZE; ++i){
        Message_Digest[i] = context->Intermediate_Hash[i>>2]
                            >> 8 * ( 3 - ( i & 0x03 ) );
    }

    return shaSuccess;
}

/**@ingroup tsk_sha1_group
 *
 * This function accepts an array of octets as the next portion of the message.
 *
 * @param context The sha1 context.
 * @param message_array An array of characters representing the next portion of the message.
 * @param length The @a length of the message in message_array
 * @retval @ref tsk_sha1_errcode_t code.
 */
tsk_sha1_errcode_t tsk_sha1input(tsk_sha1context_t    *context,
								 const uint8_t  *message_array,
								 unsigned  length)
{
    if (!length){
        return shaSuccess;
    }

    if (!context || !message_array){
        return shaNull;
    }

    if (context->Computed){
        context->Corrupted = shaStateError;

        return shaStateError;
    }

    if (context->Corrupted){
         return (tsk_sha1_errcode_t)context->Corrupted;
    }
    while(length-- && !context->Corrupted)
    {
    context->Message_Block[context->Message_Block_Index++] =
                    (*message_array & 0xFF);

    context->Length_Low += 8;
    if (context->Length_Low == 0){
        context->Length_High++;
        if (context->Length_High == 0)
        {
            /* Message is too long */
            context->Corrupted = 1;
        }
    }

    if (context->Message_Block_Index == 64){
        SHA1ProcessMessageBlock(context);
    }

    message_array++;
    }

    return shaSuccess;
}

/**@ingroup tsk_sha1_group
 *
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 * @param context The sha1 context.
 *
 */
void SHA1ProcessMessageBlock(tsk_sha1context_t *context)
{
	/*
	*      Many of the variable names in this code, especially the
	*      single character names, were used because those were the
	*      names used in the publication.
	*/
    const uint32_t K[] =    {       /* Constants defined in SHA-1   */
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
                            };
    int32_t           t;                 /* Loop counter                */
    uint32_t      temp;              /* Temporary word value        */
    uint32_t      W[80];             /* Word sequence               */
    uint32_t      A, B, C, D, E;     /* Word buffers                */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++){
        W[t] = context->Message_Block[t * 4] << 24;
        W[t] |= context->Message_Block[t * 4 + 1] << 16;
        W[t] |= context->Message_Block[t * 4 + 2] << 8;
        W[t] |= context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++){
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    for(t = 0; t < 20; t++){
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);

        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++){
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++){
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++){
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
}

/**@ingroup tsk_sha1_group
 *
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  @param context The sha1 context.
 *
 */

void SHA1PadMessage(tsk_sha1context_t *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55){
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64){
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56){
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else{
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56){
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;

    SHA1ProcessMessageBlock(context);
}

/**@ingroup tsk_sha1_group
* Computes the sha1 digest result.
* @param Message_Digest A pointer to the sha1 digest result.
* @param context The sha1 context.
*/
void tsk_sha1final(uint8_t *Message_Digest, tsk_sha1context_t *context)
{	
	int32_t i;

	SHA1PadMessage(context);
	for(i = 0; i<64; ++i) {
		context->Message_Block[i] = 0;
	}
	context->Length_Low = 0;    /* and clear length */
	context->Length_High = 0;
	
	for(i = 0; i < TSK_SHA1_DIGEST_SIZE; ++i) {
		Message_Digest[i] = context->Intermediate_Hash[i>>2] >> 8*(3-(i&0x03));
	}
}


/**@ingroup tsk_sha1_group
 *	Calculates sha1 digest result (hexadecimal string). 
 *
 * @param input	The input data for which to calculate the SHA-1 hash. 
 * @param size	The size of the input data. 
 * @param result SHA-1 hash result as a hexadecimal string. 
 *
 * @retval @ref tsk_sha1_errcode_t code.
 * @sa @ref TSK_SHA1_DIGEST_CALC
**/
tsk_sha1_errcode_t tsk_sha1compute(const char* input, tsk_size_t size, tsk_sha1string_t *result)
{
	tsk_sha1_errcode_t ret;
	tsk_sha1context_t sha;
	uint8_t digest[TSK_SHA1_DIGEST_SIZE];
	
	(*result)[TSK_SHA1_STRING_SIZE] = '\0';

	if( (ret = tsk_sha1reset(&sha)) != shaSuccess ){
		return ret;
	}
	else if ( (ret = tsk_sha1input(&sha, (uint8_t*)input, (unsigned int)size)) != shaSuccess ){
		return ret;
	}
	else if( (ret = tsk_sha1result(&sha, digest)) != shaSuccess ){
		return ret;
	}

	tsk_str_from_hex(digest, TSK_SHA1_DIGEST_SIZE, (char*)*result);

	return shaSuccess;
}

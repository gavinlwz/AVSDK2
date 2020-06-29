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

/**@file tsk_hmac.c
 * @brief HMAC: Keyed-Hashing for Message Authentication (RFC 2104) / FIPS-198-1.
 *
 *
 *

 */
#include "tsk_hmac.h"

#include "tsk_string.h"
#include "tsk_buffer.h"

#include <string.h>

/**@defgroup tsk_hmac_group Keyed-Hashing for Message Authentication (RFC 2104/ FIPS-198-1).
*/

/**@ingroup tsk_hmac_group
*/
typedef enum tsk_hash_type_e { md5, sha1 } tsk_hash_type_t;

int tsk_hmac_xxxcompute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_hash_type_t type, uint8_t* digest)
{
#define TSK_MAX_BLOCK_SIZE	TSK_SHA1_BLOCK_SIZE

	tsk_size_t i, newkey_size;

	tsk_size_t block_size = type == md5 ? TSK_MD5_BLOCK_SIZE : TSK_SHA1_BLOCK_SIZE; // Only SHA-1 and MD5 are supported for now
	tsk_size_t digest_size = type == md5 ? TSK_MD5_DIGEST_SIZE : TSK_SHA1_DIGEST_SIZE;
	char hkey [TSK_MAX_BLOCK_SIZE];
	
	uint8_t ipad [TSK_MAX_BLOCK_SIZE];
	uint8_t opad [TSK_MAX_BLOCK_SIZE];
	

	memset(ipad, 0, sizeof(ipad));
	memset(opad, 0, sizeof(ipad));

	/*
	*	H(K XOR opad, H(K XOR ipad, input))
	*/

	// Check key len
	if (key_size > block_size){
		if(type == md5){
			TSK_MD5_DIGEST_CALC(key, key_size, (uint8_t*)hkey);
		}
		else if(type == sha1){
			TSK_SHA1_DIGEST_CALC((uint8_t*)key, (unsigned int)key_size, (uint8_t*)hkey);
		}
		else return -3;
		
		newkey_size = digest_size;
	}
	else{
		memcpy(hkey, key, key_size);
		newkey_size = key_size;
	}

	memcpy(ipad, hkey, newkey_size);
	memcpy(opad, hkey, newkey_size);
	
	/* [K XOR ipad] and [K XOR opad]*/
	for (i=0; i<block_size; i++){
		ipad[i] ^= 0x36;
		opad[i] ^= 0x5c;
	}
	
	
	{
		tsk_buffer_t *passx; // pass1 or pass2
		int pass1_done = 0;
		
		passx = tsk_buffer_create(ipad, block_size); // pass1
		tsk_buffer_append(passx, input, input_size);

digest_compute:
		if(type == md5){
			TSK_MD5_DIGEST_CALC(TSK_BUFFER_TO_U8(passx), (unsigned int)TSK_BUFFER_SIZE(passx), digest);
		}
		else{
			TSK_SHA1_DIGEST_CALC(TSK_BUFFER_TO_U8(passx), (unsigned int)TSK_BUFFER_SIZE(passx), digest);
		}

		if(pass1_done){
			TSK_OBJECT_SAFE_FREE(passx);
			goto pass1_and_pass2_done;
		}
		else{
			pass1_done = 1;
		}

		tsk_buffer_cleanup(passx);
		tsk_buffer_append(passx, opad, block_size); // pass2
		tsk_buffer_append(passx, digest, digest_size);

		goto digest_compute;
	}

pass1_and_pass2_done:

	return 0;
}


/**@ingroup tsk_hmac_group
 *
 * Calculate HMAC-MD5 hash (hexa-string) as per RFC 2104.
 *
 * @author	Mamadou
 * @date	12/29/2009
 *
 * @param [in,out]	input	The input data.
 * @param	input_size		The size of the input. 
 * @param [in,out]	key		The input key. 
 * @param	key_size		The size of the input key. 
 * @param [out]	result		Pointer to the result.
 *
 * @return	Zero if succeed and non-zero error code otherwise. 
**/
int hmac_md5_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_md5string_t *result)
{
	tsk_md5digest_t digest;
	int ret;

	if((ret = hmac_md5digest_compute(input, input_size, key, key_size, digest))){
		return ret;
	}
	tsk_str_from_hex(digest, TSK_MD5_DIGEST_SIZE, *result);
	(*result)[TSK_MD5_STRING_SIZE] = '\0';

	return 0;
}


/**@ingroup tsk_hmac_group
 *
 * Calculate HMAC-MD5 hash (bytes) as per RFC 2104. 
 *
 * @author	Mamadou
 * @date	12/29/2009
 *
 * @param [in,out]	input	The input data. 
 * @param	input_size		The Size of the input. 
 * @param [in,out]	key		The input key. 
 * @param	key_size		The size of the input key. 
 * @param	result			Pointer to the result. 
 *
 * @return	Zero if succeed and non-zero error code otherwise.
**/
int hmac_md5digest_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_md5digest_t result)
{
	return tsk_hmac_xxxcompute(input, input_size, key, key_size, md5, result);
}

/**@ingroup tsk_hmac_group
 *
 * Calculate HMAC-SHA-1 hash (hexa-string) as per RFC 2104.
 *
 * @author	Mamadou
 * @date	12/29/2009
 *
 * @param [in,out]	input	The input data.  
 * @param	input_size		The Size of the input. 
 * @param [in,out]	key		The input key. 
 * @param	key_size		The size of the input key.
 * @param [out]	result		Pointer to the result.
 *
 * @return	Zero if succeed and non-zero error code otherwise.
**/
int hmac_sha1_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_sha1string_t *result)
{
	tsk_sha1digest_t digest;
	int ret;

	if((ret = hmac_sha1digest_compute(input, input_size, key, key_size, digest))){
		return ret;
	}
	tsk_str_from_hex((uint8_t*)digest, TSK_SHA1_DIGEST_SIZE, (char*)*result);
	(*result)[TSK_SHA1_STRING_SIZE] = '\0';

	return 0;
}

/**@ingroup tsk_hmac_group
 *
 * Calculate HMAC-SHA-1 hash (bytes) as per RFC 2104.
 *
 * @author	Mamadou
 * @date	12/29/2009
 *
 * @param [in,out]	input	If non-null, the input. 
 * @param	input_size		The size of the input. 
 * @param [in,out]	key		The input key. 
 * @param	key_size		The size of the input key.
 * @param	result			Pointer to the result. 
 *
 * @return	Zero if succeed and non-zero error code otherwise. 
**/
int hmac_sha1digest_compute(const uint8_t* input, tsk_size_t input_size, const char* key, tsk_size_t key_size, tsk_sha1digest_t result)
{
	return tsk_hmac_xxxcompute(input, input_size, key, key_size, sha1, (uint8_t*)result);
}


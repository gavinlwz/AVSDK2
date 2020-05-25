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

/**@file tsk_url.c
 * @brief Utility functions to encode/decode urls.
 *
 */
#include "tsk_url.h"
#include "tsk_memory.h"
#include "tsk_string.h"

#include <ctype.h>
#include <string.h>

/**@defgroup tsk_url_group Utility functions to encode/decode urls.
*/


/**@ingroup tsk_url_group
* Encode an url.
* @param url The url to encode
* @retval The encoded url. It is up to you to free the returned string.
*
* @sa tsk_url_decode
*
*/
char* tsk_url_encode(const char* url) {
	char *purl = (char*)url, *buf = (char*)tsk_malloc(tsk_strlen(url) * 3 + 1), *pbuf = buf;
	while (*purl) {
		if (isalnum(*purl) || *purl == '-' || *purl == '_' || *purl == '.' || *purl == '~'){
			*pbuf++ = *purl;
		}
		else if (*purl == ' '){
			*pbuf++ = '+';
		}
		else{
			*pbuf++ = '%', *pbuf++ = tsk_b10tob16(*purl >> 4), *pbuf++ = tsk_b10tob16(*purl & 15);
		}
		purl++;
	}
	*pbuf = '\0';
	return buf;
}

/**@ingroup tsk_url_group
* Decode an url.
* @param url The url to encode
* @retval The decoded url. It is up to you to free the returned string.
*
* @sa tsk_url_encode
*/
char* tsk_url_decode(const char* url) {
	char *purl = (char*)url, *buf = (char*)tsk_malloc(tsk_strlen(url) + 1), *pbuf = buf;
	while (*purl) {
		if (*purl == '%') {
			if (purl[1] && purl[2]) {
				*pbuf++ = tsk_b16tob10(purl[1]) << 4 | tsk_b16tob10(purl[2]);
				purl += 2;
			}
		}
		else if (*purl == '+') {
			*pbuf++ = ' ';
		}
		else {
			*pbuf++ = *purl;
		}
		purl++;
	}
	*pbuf = '\0';
	return buf;
}


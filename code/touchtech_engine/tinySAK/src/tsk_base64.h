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

/**@file tsk_base64.h
 * @brief Base64 encoder and decoder as per RFC 4648.
 *
 *
 *

 */
#ifndef TINYSAK_BASE64_H
#define TINYSAK_BASE64_H

#include "tinysak_config.h"

TSK_BEGIN_DECLS

/**@ingroup tsk_base64_group
* Guess the output(encoded) size.
* @param IN_LEN The input size.
*/
#define TSK_BASE64_ENCODE_LEN(IN_LEN)		((2 + (IN_LEN) - (((IN_LEN) + 2) % 3)) * 4 / 3)
/**@ingroup tsk_base64_group
* Guess the output(decoded) size.
* @param IN_LEN The input size.
*/
#define TSK_BASE64_DECODE_LEN(IN_LEN)		(((IN_LEN * 3)/4) + 2)

TINYSAK_API tsk_size_t tsk_base64_encode(const uint8_t* input, tsk_size_t input_size, char **output);
TINYSAK_API tsk_size_t tsk_base64_decode(const uint8_t* input, tsk_size_t input_size, char **output);

TSK_END_DECLS

#endif /* TINYSAK_BASE64_H */

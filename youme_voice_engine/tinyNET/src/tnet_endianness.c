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

/**@file tnet_endianness.c
 * @brief Byte Ordering.
 *
 *
 *

 */
#include "tnet_endianness.h"

#include "tnet.h"

extern tsk_bool_t tnet_isBigEndian;

/** Converts a 16-bit value from host to TCP/IP network byte order (big-endian).
* @param x The 16-bit (in host byte order) value to convert.
* @retval @a x in TCP/IP network byte order.
*/
unsigned short tnet_htons(unsigned short x)
{
	if(tnet_is_BE()){
		return x;
	}
	else{
		return ((((uint16_t)(x) & 0xff00) >> 8)		|
						(((uint16_t)(x) & 0x00ff) << 8));
	}
}

/* Memory alignment hack */
unsigned short tnet_htons_2(const void* px)
{
	unsigned short y = TSK_TO_UINT16((const uint8_t*)px);
	return tnet_htons(y);
}

/** Converts a 32-bit value from host to TCP/IP network byte order (big-endian).
* @param x The 32-bit (in host byte order) value to convert.
* @retval @a x in TCP/IP network byte order.
*/
unsigned long tnet_htonl(unsigned long x)
{
	if(tnet_is_BE()){
		return x;
	}
	else{
		return ((((uint32_t)(x) & 0xff000000) >> 24)	| \
						(((uint32_t)(x) & 0x00ff0000) >> 8)		| \
						(((uint32_t)(x) & 0x0000ff00) << 8)		| \
						(((uint32_t)(x) & 0x000000ff) << 24));
	}
}
uint64_t tnet_htonll(uint64_t val)
{
    if(tnet_is_BE()){
        return val;
    }
    else{
         return (((unsigned long long )tnet_htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)tnet_htonl((int)(val >> 32));
    }
}
uint64_t tnet_ntohll(uint64_t val)
{
    if(tnet_is_BE()){
        return val;
    }
    else{
        return (((unsigned long long )tnet_ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)tnet_ntohl((int)(val >> 32));
    }
}
/* Memory alignment hack */
unsigned long tnet_htonl_2(const void* px)
{
	unsigned long y = TSK_TO_UINT32((const uint8_t*)px);
	return tnet_htonl(y);
}


/** Indicates whether we are on a Big Endian host or not.<br>
* <b>IMPORTANT</b>: Before calling this function, you should initialize the network stack by using
* @ref tnet_startup().
* @retval @a tsk_true if the program is runnin on a Big Endian host and @a tsk_false otherwise.
*/
tsk_bool_t tnet_is_BE(){
	/* If LITTLE_ENDIAN or BIG_ENDIAN macros have been defined in config.h ==> use them
	* otherwise ==> dyn retrieve the endianness
	*/
#if LITTLE_ENDIAN
	return tsk_false;
#elif BIG_ENDIAN
	return tsk_true;
#else
	return tnet_isBigEndian;
#endif
}

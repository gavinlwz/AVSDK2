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

/**@file tsdp_header.c
 * @brief Defines a SDP header/line (<type>=<value>).
 *
 *
 *

 */
#include "tinysdp/headers/tsdp_header.h"


#include "tsk_string.h"


/** Gets the name of the SDP header with a type equal to @a type. 
 * @param	type	The @a type of the header for which to retrieve the name. 
 *
 * @return	The name of the header.
**/
char tsdp_header_get_name(tsdp_header_type_t type)
{
	switch(type)
	{
		case tsdp_htype_A: return 'a';
		case tsdp_htype_C: return 'c';
		case tsdp_htype_M: return 'm';
		case tsdp_htype_O: return 'o';

		default: return '*';
	}
}

char tsdp_header_get_nameex(const tsdp_header_t *self)
{
	if(self){
        return tsdp_header_get_name(self->type);		
	}
	return '*';
}


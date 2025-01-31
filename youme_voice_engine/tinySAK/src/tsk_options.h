﻿/*******************************************************************
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

/**@file tsk_options.h
 * @brief Options.
 *
 *
 *

 */
#ifndef _TINYSAK_OPTIONS_H_
#define _TINYSAK_OPTIONS_H_

#include "tinysak_config.h"
#include "tsk_object.h"
#include "tsk_list.h"
#include "tsk_buffer.h"


TSK_BEGIN_DECLS

#define TSK_OPTION_VA_ARGS(id, value)		tsk_option_def_t, (int)id, (const char*)value

#define TSK_OPTION(self)				((tsk_option_t*)(self))

/**@ingroup tsk_options_group
* Parameter.
*/
typedef struct tsk_option_s
{
	TSK_DECLARE_OBJECT;

	int id;
	char* value;
	
	tsk_bool_t tag;
}
tsk_option_t;

typedef tsk_list_t tsk_options_L_t; /**< List of @ref tsk_option_t elements. */

TINYSAK_API tsk_option_t* tsk_option_create(int id, const char* value);
TINYSAK_API tsk_option_t* tsk_option_create_null();

TINYSAK_API int tsk_options_have_option(const tsk_options_L_t *self, int id);
TINYSAK_API int tsk_options_add_option(tsk_options_L_t **self, int id, const char* value);
TINYSAK_API int tsk_options_add_option_2(tsk_options_L_t **self, const tsk_option_t* option);
TINYSAK_API int tsk_options_remove_option(tsk_options_L_t *self, int id);
TINYSAK_API const tsk_option_t *tsk_options_get_option_by_id(const tsk_options_L_t *self, int id);
TINYSAK_API const char *tsk_options_get_option_value(const tsk_options_L_t *self, int id);
TINYSAK_API int tsk_options_get_option_value_as_int(const tsk_options_L_t *self, int id);

TINYSAK_GEXTERN const tsk_object_def_t *tsk_option_def_t;

TSK_END_DECLS

#endif /* _TINYSAK_OPTIONS_H_ */


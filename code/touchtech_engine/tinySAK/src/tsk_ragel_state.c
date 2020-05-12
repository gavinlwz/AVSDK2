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

/**@file tsk_ragel_state.c
 * @brief Ragel state for SIP, HTTP and MSRP parsing.
 *
 *
 *

 */
#include "tsk_ragel_state.h"

/**@defgroup tsk_ragel_state_group Ragel state for SIP, HTTP and MSRP parsing.
*/

/**@ingroup tsk_ragel_state_group
* Initialize/Reset the ragel state with default values.
* @param state The ragel @a state to initialize.
* @param data The @a data to parse.
* @param size The @a size of the data.
*/
void tsk_ragel_state_init(tsk_ragel_state_t *state, const char *data, tsk_size_t size)
{
	state->cs = 0;
	state->tag_start = state->p = data;
	state->eoh = state->eof = state->tag_end = state->pe = state->p + size;
}


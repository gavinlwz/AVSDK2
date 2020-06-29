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

/**@file tsdp_header.h
 * @brief Defines a SDP header/line (<type>=<value>).
 *
 *
 *

 */
#ifndef TINYSDP_HEADER_H
#define TINYSDP_HEADER_H

#include "tinysdp_config.h"

#include "tsk_ragel_state.h"
#include "tsk_list.h"

TSDP_BEGIN_DECLS

struct tsdp_header_s;

#define TSDP_HEADER(self)					((tsdp_header_t*)(self))
#define TSDP_HEADER_CONST(self)				((const tsdp_header_t*)(self))
#define TSDP_HEADER_VALUE_TOSTRING_F(self)	((tsdp_header_value_tostring_f)(self))

typedef int (*tsdp_header_value_tostring_f)(const struct tsdp_header_s* header, tsk_buffer_t* output);
typedef struct tsdp_header_s* (*tsdp_header_clone_f)(const struct tsdp_header_s* header);

#define TSDP_HTYPE_V_RANK		0
#define TSDP_HTYPE_O_RANK		1
#define TSDP_HTYPE_S_RANK		2
#define TSDP_HTYPE_I_RANK		3
#define TSDP_HTYPE_U_RANK		4
#define TSDP_HTYPE_E_RANK		5
#define TSDP_HTYPE_P_RANK		6
#define TSDP_HTYPE_C_RANK		7
#define TSDP_HTYPE_B_RANK		8
#define TSDP_HTYPE_Z_RANK		11
#define TSDP_HTYPE_K_RANK		12
#define TSDP_HTYPE_A_RANK		13
#define TSDP_HTYPE_T_RANK		9
#define TSDP_HTYPE_R_RANK		10
#define TSDP_HTYPE_M_RANK		14
//#define TSDP_HTYPE_I_RANK		15
//#define TSDP_HTYPE_C_RANK		16
//#define TSDP_HTYPE_B_RANK		17

#define TSDP_HTYPE_DUMMY_RANK	255

/**
 * @enum	tsdp_header_type_e
 *
 * @brief	List of all supported headers.
**/
typedef enum tsdp_header_type_e
{
	tsdp_htype_A,
	tsdp_htype_C,
    tsdp_htype_M,
	tsdp_htype_O,
}
tsdp_header_type_t;

/*================================
*/
typedef struct tsdp_header_s
{
	TSK_DECLARE_OBJECT;
	tsdp_header_type_t type;
	//! Because all SDP headers shall appear in a fixed order, the rank is used to place each header. 
	// Info: RFC 4566 - 5. SDP Specification.
	uint8_t rank;
}
tsdp_header_t;

#define TSDP_DECLARE_HEADER tsdp_header_t __header__
typedef tsk_list_t tsdp_headers_L_t; /**< List of @ref tsdp_header_t elements. */
/*
================================*/

TINYSDP_API char tsdp_header_get_name(tsdp_header_type_t type);
TINYSDP_API char tsdp_header_get_nameex(const tsdp_header_t *self);


TSDP_END_DECLS

#endif /* TINYSDP_HEADER_H */

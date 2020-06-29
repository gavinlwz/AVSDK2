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

/**@file tsk_common.h
 * Commons.
 */

#ifndef _TINYSAK_COMMON_H_
#define _TINYSAK_COMMON_H_

typedef int tsk_boolean_t;
#define tsk_bool_t tsk_boolean_t

/**@def tsk_true
* True (1).
*/
/**@def tsk_false
* False (0).
*/
#define tsk_true	1
#define tsk_false	0

#define TSK_MIN(a,b)					(((a) < (b)) ? (a) : (b))
#define TSK_MAX(a,b)					(((a) > (b)) ? (a) : (b))
#define TSK_ABS(a)						(((a)< 0) ? -(a) : (a))
#define TSK_CLAMP(nMin, nVal, nMax)		((nVal) > (nMax)) ? (nMax) : (((nVal) < (nMin)) ? (nMin) : (nVal))

// used to avoid doing *((uint32_t*)ptr) which don't respect memory alignment on
// some embedded (ARM,?...) platforms
#define TSK_TO_UINT32(u8_ptr) (((uint32_t)(u8_ptr)[0]) | ((uint32_t)(u8_ptr)[1])<<8 | ((uint32_t)(u8_ptr)[2])<<16 | ((uint32_t)(u8_ptr)[3])<<24)
#define TSK_TO_INT32(u8_ptr) (((int32_t)(u8_ptr)[0]) | ((int32_t)(u8_ptr)[1])<<8 | ((int32_t)(u8_ptr)[2])<<16 | ((int32_t)(u8_ptr)[3])<<24)
#define TSK_TO_UINT16(u8_ptr) (((uint16_t)(u8_ptr)[0]) | ((uint16_t)(u8_ptr)[1])<<8)

typedef int tsk_ssize_t; /**< Signed size */
#if defined (_SIZE_T_DEFINED) || defined(_SIZE_T)
typedef size_t tsk_size_t;
#else
typedef unsigned int tsk_size_t;
#endif


#if defined(va_copy)
#	define tsk_va_copy(D, S)       va_copy((D), (S))
#elif defined(__va_copy)
#	define tsk_va_copy(D, S)       __va_copy((D), (S))
#else
#	define tsk_va_copy(D, S)       ((D) = (S))
#endif

// Type promotion
#if defined(__GNUC__) || TSK_UNDER_APPLE
#	define tsk_va_arg_u8(ap)	(uint8_t)va_arg((ap), int)
#	define tsk_va_arg_u16(ap)	(uint16_t)va_arg((ap), int)
#	define tsk_va_arg_float(ap)	(float)va_arg((ap), double)
#else
#	define tsk_va_arg_u8(ap)	va_arg((ap), uint8_t)
#	define tsk_va_arg_u16(ap)	va_arg((ap), uint16_t)
#	define tsk_va_arg_float(ap)	va_arg((ap), float)
#endif

#ifdef NULL
#define tsk_null    NULL /**< Null pointer */
#else
#define tsk_null    0  /**< Null pointer */
#endif

#if defined(__GNUC__) || (HAVE___SYNC_FETCH_AND_ADD && HAVE___SYNC_FETCH_AND_SUB)
#	define tsk_atomic_inc(_ptr_) __sync_fetch_and_add((_ptr_), 1)
#	define tsk_atomic_dec(_ptr_) __sync_fetch_and_sub((_ptr_), 1)
#elif defined(_MSC_VER)
#	define tsk_atomic_inc(_ptr_) InterlockedIncrement((_ptr_))
#	define tsk_atomic_dec(_ptr_) InterlockedDecrement((_ptr_))
#else
#	define tsk_atomic_inc(_ptr_) ++(*(_ptr_))
#	define tsk_atomic_dec(_ptr_) --(*(_ptr_))
#endif

// Substract with saturation
#define tsk_subsat_int32_ptr(pvoid_p0, pvoid_p1, pint_ret) { \
	int64_t int64_ret = (int64_t)(((const char*)(pvoid_p0)) - ((const char*)(pvoid_p1))); \
	*(pint_ret) = (int)((int64_ret > 2147483647) ? 2147483647 : ((int64_ret < -2147483647) ? -2147483647 : int64_ret)); \
}

#endif /* _TINYSAK_COMMON_H_ */

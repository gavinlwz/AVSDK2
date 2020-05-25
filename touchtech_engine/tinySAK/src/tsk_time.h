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

/**@file tsk_time.h
 * @brief Datetime functions.
 *
 *
 *

 */
#ifndef _TINYSAK_TIME_H_
#define _TINYSAK_TIME_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

//#if defined(__SYMBIAN32__) || ANDROID /* Forward declaration */
struct timeval;
struct timezone;
struct timespec;
//#endif

/**@ingroup tsk_time_group
*/
#define TSK_TIME_S_2_MS(S) ((S)*1000)
#define TSK_TIME_MS_2_S(MS) ((MS)/1000)

TINYSAK_API int tsk_gettimeofday(struct timeval *tv, struct timezone *tz);
TINYSAK_API uint64_t tsk_gettimeofday_ms();
TINYSAK_API uint64_t tsk_gettimeofday_us();
TINYSAK_API uint64_t tsk_timeval_diff_us(struct timeval *tv1, struct timeval *tv2);
TINYSAK_API uint64_t tsk_time_get_ms(const struct timeval *tv);
TINYSAK_API uint64_t tsk_time_epoch();
TINYSAK_API uint64_t tsk_time_now();
TINYSAK_API uint64_t tsk_time_ntp();
TINYSAK_API uint64_t tsk_time_get_ntp_ms(const struct timeval *tv);

TSK_END_DECLS

#endif /* _TINYSAK_TIME_H_ */


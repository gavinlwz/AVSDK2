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

/**@file tsk_semaphore.h
 * @brief Pthread Semaphore.
 *
 *
 *

 */
#ifndef _TINYSAK_SEMAPHORE_H_
#define _TINYSAK_SEMAPHORE_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

typedef void tsk_semaphore_handle_t;

TINYSAK_API tsk_semaphore_handle_t* tsk_semaphore_create();
TINYSAK_API tsk_semaphore_handle_t* tsk_semaphore_create_2(int initial_val);
TINYSAK_API int tsk_semaphore_increment(tsk_semaphore_handle_t* handle);
TINYSAK_API int tsk_semaphore_decrement(tsk_semaphore_handle_t* handle);
TINYSAK_API int tsk_semaphore_decrement_time(tsk_semaphore_handle_t* handle, int wait_ms);   //0返回成功，非0表示超时，目前只有mac起效，其他平台等同于无时间限制
TINYSAK_API void tsk_semaphore_destroy(tsk_semaphore_handle_t** handle);

TSK_END_DECLS

#endif /* _TINYSAK_SEMAPHORE_H_ */


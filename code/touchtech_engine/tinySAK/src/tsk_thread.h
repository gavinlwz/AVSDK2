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

/**@file tsk_thread.h
 * @brief Utility functions for threading.
 *
 *
 *

 */
#ifndef _TINYSAK_THREAD_H_
#define _TINYSAK_THREAD_H_

#include "tinysak_config.h"

typedef void tsk_thread_handle_t;
#if TSK_UNDER_WINDOWS
typedef unsigned long tsk_thread_id_t;
#define TSK_THREAD_PRIORITY_LOW THREAD_PRIORITY_LOWEST
#define TSK_THREAD_PRIORITY_MEDIUM THREAD_PRIORITY_NORMAL
#define TSK_THREAD_PRIORITY_HIGH THREAD_PRIORITY_HIGHEST
#define TSK_THREAD_PRIORITY_TIME_CRITICAL THREAD_PRIORITY_TIME_CRITICAL
#else
#include <pthread.h>
#include <sched.h>
typedef pthread_t tsk_thread_id_t;
#define TSK_THREAD_PRIORITY_LOW sched_get_priority_min (SCHED_OTHER)
#define TSK_THREAD_PRIORITY_TIME_CRITICAL sched_get_priority_max (SCHED_OTHER)
#define TSK_THREAD_PRIORITY_MEDIUM \
    ((TSK_THREAD_PRIORITY_TIME_CRITICAL - TSK_THREAD_PRIORITY_LOW) >> 1)
#define TSK_THREAD_PRIORITY_HIGH ((TSK_THREAD_PRIORITY_MEDIUM * 3) >> 1)
#endif

TSK_BEGIN_DECLS

TINYSAK_API void tsk_thread_sleep (uint64_t ms);
TINYSAK_API int tsk_thread_create (tsk_thread_handle_t **handle, void *(TSK_STDCALL *start)(void *), void *arg);
TINYSAK_API int tsk_thread_set_priority (tsk_thread_handle_t *handle, int32_t priority);
TINYSAK_API int tsk_thread_set_priority_2 (int32_t priority);
TINYSAK_API tsk_thread_id_t tsk_thread_get_id ();
TINYSAK_API tsk_bool_t tsk_thread_id_equals (tsk_thread_id_t *id_1, tsk_thread_id_t *id_2);
TINYSAK_API int tsk_thread_destroy (tsk_thread_handle_t **handle);
TINYSAK_API int tsk_thread_join (tsk_thread_handle_t **handle);
TINYSAK_API void tsk_thread_detach(tsk_thread_handle_t** handle);
TSK_END_DECLS

#endif

//
//  tsk_atom.h
//  youme_voice_engine
//
//  Created by Jackie on 2017/6/1.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tsk_atom_h
#define tsk_atom_h

#include "tinysak_config.h"

TSK_BEGIN_DECLS


/*
 * 这段关于原子操作的宏 主要在 VS 和 GCC 中跑
 *
 * 为什么要用原子操作,因为它快,很快. 接近硬件层,怎么使用会做具体的注释
 */

#if TSK_UNDER_WINDOWS
//这里主要 _WIN32 操作 ,对于WIN64 没有做了,本质一样,函数后面加上64. 这里定位就是从简单的跨平台来来
#include <windows.h>

//全部采用后置原子操作,先返回old的值 (前置等价 => tmp = v ; v = v + a ; return tmp)
#define SC_ATOM_ADD(v,a) \
    InterlockedAdd(&(v),(a))

//将a的值设置给v,返回设置之前的值
#define SC_ATOM_SET(v,a) \
    InterlockedExchange(&(v),(a))

// v == c ? swap(v,a) ; return true : return false.
#define SC_ATOM_COM(v,c,a) \
    ( c == InterlockedCompareExchange(&(v), (a), c))

//第一次使用 v最好是 0
#define SC_ATOM_LOCK(v) \
    while(SC_ATOM_SET(v,1)) { \
        Sleep(0); \
    }

#define SC_ATOM_UNLOCK(v) \
    SC_ATOM_SET(v,0)

#else

#include <unistd.h>

//全部采用后置原子操作,先返回old的值 (前置等价 => a = i++)
#define SC_ATOM_ADD(v,a) __sync_fetch_and_add(&(v),(a))

//将a的值设置给v,返回设置之前的值
#define SC_ATOM_SET(v,a) __sync_lock_test_and_set(&(v),(a))

// v == c ? swap(v,a) return true : return false.
#define SC_ATOM_CMP(v,c,a) __sync_bool_compare_and_swap(&(v), (cmp), (val))

//等待的秒数,因环境而定 2是我自己测试的一个值
#define _INT_USLEEP (2)

#define SC_ATOM_LOCK(v) \
    while(SC_ATOM_SET(v,1)) { \
        usleep(_INT_USLEEP); \
    }

#define SC_ATOM_UNLOCK(v) __sync_lock_release(&(v))


#endif /* _MSC_VER || __GNU__*/



/**@ingroup tsk_mutex_group
 * Mutex handle.
 */

TSK_END_DECLS


#endif /* tsk_atom_h */

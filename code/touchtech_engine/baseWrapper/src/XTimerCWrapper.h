//
//  XTimerCWrapper.hpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/5.
//  Copyright © 2016年 youme. All rights reserved.
//
 
#ifndef XTimerCWrapper_hpp
#define XTimerCWrapper_hpp
//这个是给C 调用的，包装，C++ 不要用这个,必须要调用了C++ 初始化，内部使用接口，所以不保证初始化的调用
#include <stdio.h>
typedef long xt_timer_id_t;
#define xt_timer_manager_handle_t void
#define XT_INVALID_TIMER_ID -1
#define XT_TIMER_ID_IS_VALID(id) ((id) != XT_INVALID_TIMER_ID)
#define XT_TIMER_CALLBACK_F(callback) ((xt_timer_callback_f)callback)
#ifdef __cplusplus
extern "C" {
#endif
    typedef int (*xt_timer_callback_f) (const void*arg,long timerid);
   
    
    long xt_timer_mgr_global_schedule(unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam);
    long xt_timer_mgr_global_schedule_loop(unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam);
    int  xt_timer_mgr_global_cancel(long iTimerID);
  
    //模拟的一套函数
    xt_timer_manager_handle_t* xt_timer_manager_create();
    int xt_timer_manager_start(xt_timer_manager_handle_t*);
    long xt_timer_manager_schedule(xt_timer_manager_handle_t* manager,unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam);
    int xt_timer_manager_stop(xt_timer_manager_handle_t* manager);
    int xt_timer_manager_cancel(xt_timer_manager_handle_t* manager,long iTimerID);
    int xt_timer_manager_destroy(xt_timer_manager_handle_t**manager);

    // 高精度定时器（基于std::chrono）
    int xt_timer_wait(unsigned int uWaitMs);
#ifdef __cplusplus
}
#endif

#endif /* XTimerCWrapper_hpp */

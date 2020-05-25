//
//  XTimerCWrapper.cpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/5.
//  Copyright © 2016年 youme. All rights reserved.
//

#include <thread>
#include "XTimerCWrapper.h"
#include "XTimer.h"

xt_timer_manager_handle_t* xt_timer_manager_create()
{
    return new CXTimer;
}


int xt_timer_manager_start(xt_timer_manager_handle_t* manger_handle)
{
    CXTimer* pTimer = (CXTimer*)manger_handle;
    if (pTimer->Init())
    {
        return  0;
    }
    return -1;
}

 long xt_timer_manager_schedule(xt_timer_manager_handle_t* manger_handle,unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam)
{
    CXTimer* pTimer = (CXTimer*)manger_handle;
    return pTimer->PostTimer(ulTimeout, (TimerCallback)callback, pParam,true);
}

int xt_timer_manager_destroy(xt_timer_manager_handle_t**manager)
{
    if ((NULL == manager) || (NULL == *manager)) {
        return -1;
    }
    CXTimer* pTimer = (CXTimer*)(*manager);
    pTimer->UnInit();
    delete  pTimer;
    *manager = NULL;
    return 0;
}


int xt_timer_manager_stop(xt_timer_manager_handle_t* manager)
{
    if (NULL == manager){
        return -1;
    }
    CXTimer* pTimer = (CXTimer*)(manager);
    pTimer->UnInit();
    return 0;
}

int xt_timer_manager_cancel(xt_timer_manager_handle_t* manager,long iTimerID)
{
    if (NULL == manager){
        return -1;
    }
    CXTimer* pTimer = (CXTimer*)(manager);
    pTimer->CancleTimer(iTimerID);
    return 0;
}

long xt_timer_mgr_global_schedule(unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam)
{
	return CXTimer::GetInstance()->PostTimer(ulTimeout, (TimerCallback)callback, pParam, true);
}
long xt_timer_mgr_global_schedule_loop(unsigned long long ulTimeout, xt_timer_callback_f callback, void* pParam)
{
	return CXTimer::GetInstance()->PostTimer(ulTimeout, (TimerCallback)callback, pParam, false);
}

int xt_timer_mgr_global_cancel(long iTimerID)
{
    if (CXTimer::GetInstance()->CancleTimer(iTimerID))
    {
        return 0;
    }
    return  -1;
}

// for high resolution timer
int xt_timer_wait(unsigned int uWaitMs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(uWaitMs));
    return 0;
}
//
//  tsk_cond.c
//  youme_voice_engine
//
//  Created by bhb on 2017/9/26.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "tsk_cond.h"
#include <stdlib.h>
#if TSK_UNDER_WINDOWS
#include <windows.h>
typedef HANDLE  CONT_T;
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
struct _cond_t_
{
    pthread_mutex_t event_mutex_;
    pthread_cond_t event_cond_;
    tsk_bool_t is_manual_reset_;
    tsk_bool_t event_status_;
} ;
typedef struct _cond_t_  CONT_T;
#endif

#if TSK_UNDER_WINDOWS

tsk_cond_handle_t* tsk_cond_create(tsk_bool_t manual_reset, tsk_bool_t initially_signaled)
{
    CONT_T event_handle_ = CreateEvent(NULL,                 // Security attributes.
                                  manual_reset,
                                  initially_signaled,
                                  NULL);                // Name.
    return (tsk_cond_handle_t*)event_handle_;
}

void  tsk_cond_set(tsk_cond_handle_t* handle)
{
    if(!handle) return;
    SetEvent((CONT_T)handle);
}

void  tsk_cond_reset(tsk_cond_handle_t* handle)
{
    if(!handle) return;
    ResetEvent((CONT_T)handle);
}

tsk_bool_t  tsk_cond_wait(tsk_cond_handle_t* handle, int milliseconds)
{
    if(!handle) return tsk_false;
    int ms = (milliseconds == -1) ? INFINITE : milliseconds;
    return (WaitForSingleObject((CONT_T)handle, ms) == WAIT_OBJECT_0);
}

void  tsk_cond_destroy(tsk_cond_handle_t** handle)
{
     if(handle == NULL || *handle == NULL) return;
     CloseHandle((CONT_T)*handle);
     *handle = NULL;
}

#else
tsk_cond_handle_t* tsk_cond_create(tsk_bool_t manual_reset, tsk_bool_t initially_signaled)
{
//    pthread_mutex_t event_mutex_;
//    pthread_cond_t event_cond_;

    
    CONT_T * cont_t = (CONT_T *)malloc(sizeof(CONT_T));
    cont_t->event_status_ = initially_signaled;
    cont_t->is_manual_reset_ = manual_reset;
    if(pthread_mutex_init(& cont_t->event_mutex_, NULL) != 0){
        free(cont_t);
        return tsk_null;
    }
    if(pthread_cond_init(&cont_t->event_cond_, NULL) != 0)
    {
        pthread_mutex_destroy(&cont_t->event_mutex_);
        free(cont_t);
        return tsk_null;
    }
//    cont_t->event_mutex_ = event_mutex_;
//    cont_t->event_cond_ = event_cond_;

    return (tsk_cond_handle_t*)cont_t;
}

void  tsk_cond_set(tsk_cond_handle_t* handle)
{
    if(!handle) return;
    CONT_T * cont_t = (CONT_T *)handle;
    pthread_mutex_lock(&cont_t->event_mutex_);
    cont_t->event_status_ = tsk_true;
    pthread_cond_broadcast(&cont_t->event_cond_);
    //pthread_cond_signal(&cont_t->event_cond_);
    pthread_mutex_unlock(&cont_t->event_mutex_);
}

void  tsk_cond_reset(tsk_cond_handle_t* handle)
{
    if(!handle) return;
    CONT_T * cont_t = (CONT_T *)handle;
    pthread_mutex_lock(&cont_t->event_mutex_);
    cont_t->event_status_ = tsk_false;
    pthread_mutex_unlock(&cont_t->event_mutex_);
}

tsk_bool_t  tsk_cond_wait(tsk_cond_handle_t* handle, int milliseconds)
{
    if(!handle) return tsk_false;
    
    CONT_T * cont_t = (CONT_T *)handle;
    pthread_mutex_lock(&cont_t->event_mutex_);
    int error = 0;
    
    if (milliseconds != -1) {
        // Converting from seconds and microseconds (1e-6) plus
        // milliseconds (1e-3) to seconds and nanoseconds (1e-9).
        
        struct timespec ts;
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
        // Use relative time version, which tends to be more efficient for
        // pthread implementations where provided (like on Android).
        ts.tv_sec = milliseconds / 1000;
        ts.tv_nsec = (milliseconds % 1000) * 1000000;
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        
        ts.tv_sec = tv.tv_sec + (milliseconds / 1000);
        ts.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
        
        // Handle overflow.
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
#endif
        
        while (!cont_t->event_status_ && error == 0)
        {
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
            error = pthread_cond_timedwait_relative_np(&cont_t->event_cond_, &cont_t->event_mutex_, &ts);
#else
            error = pthread_cond_timedwait(&cont_t->event_cond_, &cont_t->event_mutex_, &ts);
#endif
        }
    } else {
        while (!cont_t->event_status_ && error == 0)
            error = pthread_cond_wait(&cont_t->event_cond_, &cont_t->event_mutex_);
    }
    
    // NOTE(liulk): Exactly one thread will auto-reset this event. All
    // the other threads will think it's unsignaled.  This seems to be
    // consistent with auto-reset events in WEBRTC_WIN
    if (error == 0 && !cont_t->is_manual_reset_)
        cont_t->event_status_ = tsk_false;
    
    pthread_mutex_unlock(&cont_t->event_mutex_);
    
    return (error == 0);
}

void  tsk_cond_destroy(tsk_cond_handle_t** handle)
{
    if(handle == NULL || *handle == NULL) return;
    
    CONT_T * cont_t = (CONT_T *)*handle;
    pthread_mutex_destroy(&cont_t->event_mutex_);
    pthread_cond_destroy(&cont_t->event_cond_);
    free(cont_t);
    *handle = NULL;
}


#endif


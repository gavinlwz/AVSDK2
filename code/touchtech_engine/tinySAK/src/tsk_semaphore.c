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

/**@file tsk_semaphore.c
 * @brief Pthread/Windows Semaphore utility functions.
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tsk_semaphore.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_string.h"
#include "tsk_time.h"

/* Apple claims that they fully support POSIX semaphore but ...
 */
#if defined(__APPLE__) /* Mac OSX/Darwin/Iphone/Ipod Touch */
#import <sys/sysctl.h>
#	define TSK_USE_NAMED_SEM	1
#else
#	define TSK_USE_NAMED_SEM	0
#endif

#if !defined(NAME_MAX)
#   define	NAME_MAX		  255
#endif

#if TSK_UNDER_WINDOWS /* Windows XP/Vista/7/CE */

#	include <windows.h>
#	include "tsk_errno.h"
#	define SEMAPHORE_S void
typedef HANDLE	SEMAPHORE_T;
#	if TSK_UNDER_WINDOWS_RT
#		if !defined(CreateSemaphoreEx)
#			define CreateSemaphoreEx CreateSemaphoreExW
#		endif
#	endif
//#else if define(__APPLE__) /* Mac OSX/Darwin/Iphone/Ipod Touch */
//#	include <march/semaphore.h>
//#	include <march/task.h>

#elif defined MAC_OS
#include <dispatch/dispatch.h>
typedef dispatch_semaphore_t SEMAPHORE_T;
#define SEMAPHORE_S dispatch_semaphore_t
#define GET_SEM(PSEM) ((PSEM))

#else /* All *nix */

#	include <pthread.h>
#	include <semaphore.h>
#	if TSK_USE_NAMED_SEM
#	include <fcntl.h> /* O_CREAT */
#	include <sys/stat.h> /* S_IRUSR, S_IWUSR*/

typedef struct named_sem_s
{
    sem_t* sem;
    char name [NAME_MAX + 1];
} named_sem_t;
#		define SEMAPHORE_S named_sem_t
#		define GET_SEM(PSEM) (((named_sem_t*)(PSEM))->sem)
#	else
#		define SEMAPHORE_S sem_t
#		define GET_SEM(PSEM) ((PSEM))
#	endif /* TSK_USE_NAMED_SEM */
typedef sem_t* SEMAPHORE_T;

#endif

#if defined(__GNUC__) || defined(__SYMBIAN32__)
#	include <errno.h>
#endif



/**@defgroup tsk_semaphore_group Pthread/Windows Semaphore functions.
 */

/**@ingroup tsk_semaphore_group
 * Creates new semaphore handle.
 * @retval A New semaphore handle.
 * You MUST call @ref tsk_semaphore_destroy to free the semaphore.
 * @sa @ref tsk_semaphore_destroy
 */
tsk_semaphore_handle_t* tsk_semaphore_create()
{
    return tsk_semaphore_create_2(0);
}

tsk_semaphore_handle_t* tsk_semaphore_create_2(int initial_val)
{
    SEMAPHORE_T handle = tsk_null;
    
#if TSK_UNDER_WINDOWS
#	if TSK_UNDER_WINDOWS_RT
    handle = CreateSemaphoreEx(NULL, initial_val, 0x7FFFFFFF, NULL, 0x00000000, SEMAPHORE_ALL_ACCESS);
#	else
    handle = CreateSemaphore(NULL, initial_val, 0x7FFFFFFF, NULL);
#	endif
    
#elif defined MAC_OS
    //dispatch_semaphore_create这套接口要求signal和wait次数匹配，不然release会崩溃
    //所以只能create成0，才能判断
    handle = dispatch_semaphore_create( 0 );
    for( int i = 0 ; i < initial_val ; i++ )
    {
        dispatch_semaphore_signal( handle );
    }
#else
    handle = tsk_calloc(1, sizeof(SEMAPHORE_S));
    
#if TSK_USE_NAMED_SEM
    named_sem_t * nsem = (named_sem_t*)handle;
    snprintf(nsem->name, (sizeof(nsem->name)/sizeof(nsem->name[0])) - 1, "/sem/%llu/%d.", tsk_time_epoch(), rand() ^ rand());
    if ((nsem->sem = sem_open(nsem->name, O_CREAT /*| O_EXCL*/, S_IRUSR | S_IWUSR, initial_val)) == SEM_FAILED) {
#else
        if (sem_init((SEMAPHORE_T)handle, 0, initial_val)) {
#endif
            TSK_FREE(handle);
            TSK_DEBUG_ERROR("Failed to initialize the new semaphore (errno=%d).", errno);
        }
#endif
        if (!handle) {
            TSK_DEBUG_ERROR("Failed to create new semaphore");
        }
        return handle;
    }
    
    /**@ingroup tsk_semaphore_group
     * Increments a semaphore.
     * @param handle The semaphore to increment.
     * @retval Zero if succeed and otherwise the function returns -1 and sets errno to indicate the error.
     * @sa @ref tsk_semaphore_decrement.
     */
    int tsk_semaphore_increment(tsk_semaphore_handle_t* handle)
    {
        int ret = EINVAL;
        if (handle) {
#if TSK_UNDER_WINDOWS
            if((ret = ReleaseSemaphore((SEMAPHORE_T)handle, 1L, NULL) ? 0 : -1))
#elif defined MAC_OS
            dispatch_semaphore_signal( (SEMAPHORE_T)handle );
            ret = 0;
            if( 0 ) //没有失败的判断
#else
            if((ret = sem_post((SEMAPHORE_T)GET_SEM(handle))))
#endif
            {
                TSK_DEBUG_ERROR("sem_post function failed: %d", ret);
            }
        }
        return ret;
    }
    
    /**@ingroup tsk_semaphore_group
     * Decrements a semaphore.
     * @param handle The semaphore to decrement.
     * @retval Zero if succeed and otherwise the function returns -1 and sets errno to indicate the error.
     * @sa @ref tsk_semaphore_increment.
     */
    int tsk_semaphore_decrement(tsk_semaphore_handle_t* handle)
    {
        int ret = EINVAL;
        if (handle) 
        {
#if TSK_UNDER_WINDOWS
#	    if TSK_UNDER_WINDOWS_RT
            ret = (WaitForSingleObjectEx((SEMAPHORE_T)handle, INFINITE, TRUE) == WAIT_OBJECT_0) ? 0 : -1;
#	    else
            ret = (WaitForSingleObject((SEMAPHORE_T)handle, INFINITE) == WAIT_OBJECT_0) ? 0 : -1;
#       endif
            if (ret)	
            {
                TSK_DEBUG_ERROR("sem_wait function failed: %d", ret);
            }
            
#elif defined MAC_OS
            ret = dispatch_semaphore_wait( (SEMAPHORE_T)handle, DISPATCH_TIME_FOREVER  );
            if(ret)
            {
                TSK_DEBUG_ERROR("sem_wait function failed: %d", errno);
            }
#else
            do 
            {
                ret = sem_wait((SEMAPHORE_T)GET_SEM(handle));
            }
            while ( errno == EINTR );
            if(ret)	
            {
                TSK_DEBUG_ERROR("sem_wait function failed: %d", errno);
            }
#endif
        }
        
        return ret;
    }
    
    TINYSAK_API int tsk_semaphore_decrement_time(tsk_semaphore_handle_t* handle, int wait_ms)
    {
        int ret = EINVAL;
#ifdef MAC_OS
        if (handle)
        {
            ret = dispatch_semaphore_wait( (SEMAPHORE_T)handle, dispatch_time( DISPATCH_TIME_NOW,  NSEC_PER_MSEC * wait_ms)  );
            if(ret)
            {
                TSK_DEBUG_ERROR("sem_wait function failed: %d", errno);
            }
            return ret;
        }
#elif defined(__APPLE__)
        struct timeval  curTimeVal;
        
        gettimeofday(&curTimeVal, NULL);
        long long tv_sec = curTimeVal.tv_sec + ((curTimeVal.tv_usec + wait_ms*1000) / 1000000);
        long long tv_nsec = ((curTimeVal.tv_usec + wait_ms*1000) % 1000000) * 1000;
        
        struct timeval timenow;
        struct timespec sleepytime;
        int retcode;
        
        /* This is just to avoid a completely busy wait */
        sleepytime.tv_sec = 0;
        sleepytime.tv_nsec = 10000000; /* 10ms */
        
        while ((retcode = sem_trywait((SEMAPHORE_T)GET_SEM(handle))) == -1)
        {
            gettimeofday (&timenow, NULL);
            
            if (timenow.tv_sec >= tv_sec &&
                (timenow.tv_usec * 1000) >= tv_nsec)
            {
                return -1;
            }
            nanosleep (&sleepytime, NULL);
        }
        
        return 0;
#else
        return tsk_semaphore_decrement( handle );
#endif
        return ret;
        
    }
    
    /**@ingroup tsk_semaphore_group
     * Destroy a semaphore previously created using @ref tsk_semaphore_create.
     * @param handle The semaphore to free.
     * @sa @ref tsk_semaphore_create
     */
    void tsk_semaphore_destroy(tsk_semaphore_handle_t** handle)
    {
        if(handle && *handle)
        {
#if TSK_UNDER_WINDOWS
            CloseHandle((SEMAPHORE_T)*handle);
            *handle = tsk_null;
#elif defined MAC_OS
            //把*handle信号量处理成当前值为0
            {
                while ( dispatch_semaphore_wait( *handle, DISPATCH_TIME_NOW) == 0 ) {
                }
                while( dispatch_semaphore_signal(  *handle ) != 0 ){
                }
            }
            dispatch_release(*handle);
            *handle = NULL;
#else
#	if TSK_USE_NAMED_SEM
            named_sem_t * nsem = ((named_sem_t*)*handle);
            sem_close(nsem->sem);
            sem_unlink(nsem->name);
#else
            sem_destroy((SEMAPHORE_T)GET_SEM(*handle));
#endif /* TSK_USE_NAMED_SEM */
            tsk_free(handle);
#endif
        }
        else{
            TSK_DEBUG_WARN("Cannot free an uninitialized semaphore object");
        }
    }
    

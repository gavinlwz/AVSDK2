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

/**@file tsk_thread.c
 * @brief Utility functions for threading.
 *
 *
 *

 */
#include "XOsWrapper.h"
#include "tsk_thread.h"
#include "tsk_debug.h"
#include "tsk_memory.h"

#if TSK_UNDER_WINDOWS
#	include <windows.h>
#endif
#if TSK_UNDER_WINDOWS_RT
#	include "../winrt/ThreadEmulation.h"
	using namespace ThreadEmulation;
#endif

#include <string.h>

/**@defgroup tsk_thread_group Utility functions for threading.
*/

/**@ingroup tsk_thread_group
*/
void tsk_thread_sleep(uint64_t ms)
{
#if 0
#if TSK_UNDER_WINDOWS
	Sleep((DWORD)ms);
#else
	struct timespec interval; 

	interval.tv_sec = (long)(ms/1000); 
	interval.tv_nsec = (long)(ms%1000) * 1000000; 
	nanosleep(&interval, 0);
#endif
#else
    XSleep(ms);
#endif
}

/**@ingroup tsk_thread_group
* Creates a new thread.
* @param handle Handle id of the newly created thread. The returned handle should be destroyed using @ref tsk_thread_join()
* @param start The function to be run as the new thread's start routine 
* @param arg An address for the argument for the thread's start routine 
* @retval If successful, returns zero. Otherwise, an error number is returned to indicate the error
*/
int tsk_thread_create(tsk_thread_handle_t** handle, void *(TSK_STDCALL *start) (void *), void *arg)
{
#if TSK_UNDER_WINDOWS
	*((HANDLE*)handle) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, arg, 0, NULL);
	return *((HANDLE*)handle) ? 0 : -1;
#else
	*handle = tsk_calloc(1, sizeof(pthread_t));
	return pthread_create((pthread_t*)*handle, 0, start, arg);
#endif
}

/**@ingroup tsk_thread_group
 */
int tsk_thread_set_priority(tsk_thread_handle_t* handle, int32_t priority)
{
	if(!handle){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
#if TSK_UNDER_WINDOWS
	{
		int ret = SetThreadPriority(handle, priority) ? 0 : -1;
#if TSK_UNDER_WINDOWS_RT
		// It's not possible to set priority on WP8 when thread is not in suspended state -> do nothing and don't bother us
		if(ret){
			TSK_DEBUG_INFO("SetThreadPriority() failed but nothing to worry about");
			ret = 0;
		}
#endif
		return ret;
	}
#else
	struct sched_param sp;
	int ret;
    memset(&sp, 0, sizeof(struct sched_param));
    sp.sched_priority = priority;
    if ((ret = pthread_setschedparam(*((pthread_t*)handle), SCHED_OTHER, &sp))) {
        TSK_DEBUG_ERROR("Failed to change priority to %d with error code=%d", priority, ret);
        return ret;
    }
    return 0;
#endif
}

/**@ingroup tsk_thread_group
 */
int tsk_thread_set_priority_2(int32_t priority)
{
#if TSK_UNDER_WINDOWS
	TSK_DEBUG_ERROR("Not implemented");
	return -1;
#else
	pthread_t thread = pthread_self();
	return tsk_thread_set_priority(&thread, priority);
#endif
}

tsk_thread_id_t tsk_thread_get_id()
{
#if TSK_UNDER_WINDOWS
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

tsk_bool_t tsk_thread_id_equals(tsk_thread_id_t* id_1, tsk_thread_id_t *id_2)
{
	if(!id_1 || !id_2){
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_false;
	}
#if TSK_UNDER_WINDOWS
	return (*id_1 == *id_2);
#else
	return (pthread_equal(*id_1, *id_2) != 0);
#endif
}

/**@ingroup tsk_thread_group
* Join a thread.
* @param handle Pthread handle to the target thread.
* @retval If successful, returns zero. Otherwise, an error number is returned to indicate the error. 
*/
int tsk_thread_join(tsk_thread_handle_t** handle)
{
	int ret;

	if(!handle){
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	if(!*handle){
		TSK_DEBUG_WARN("Cannot join NULL handle");
		return 0;
	}
    
#if TSK_UNDER_WINDOWS
#	if TSK_UNDER_WINDOWS_RT
	ret = (WaitForSingleObjectEx(*((HANDLE*)handle), INFINITE, TRUE) == WAIT_FAILED) ? -1 : 0;
#	else
	ret = (WaitForSingleObject(*((HANDLE*)handle), INFINITE) == WAIT_FAILED) ? -1 : 0;
#endif
	if(ret == 0){
		ret = tsk_thread_destroy(handle);
	} else {
		TSK_DEBUG_ERROR("tsk_thread_join fail, ret:%d", ret);
	}
#else
	if((ret = pthread_join(*((pthread_t*)*handle), 0)) == 0){
		ret = tsk_thread_destroy(handle);
	} else {
		TSK_DEBUG_ERROR("pthread_join fail, ret:%d", ret);
	}
#endif

	return ret;
}

int tsk_thread_destroy(tsk_thread_handle_t** handle)
{
	if(handle && *handle){
#if TSK_UNDER_WINDOWS
		CloseHandle(*((HANDLE*)handle));
		*handle = tsk_null;
#else
		tsk_free(handle);
#endif
	}
	return 0;
}

void tsk_thread_detach(tsk_thread_handle_t** handle)
{
#if TSK_UNDER_WINDOWS
	// windows env do nothing
	
#else
	// for android/ios/mac 
	pthread_detach(*((pthread_t*)*handle));
#endif
}


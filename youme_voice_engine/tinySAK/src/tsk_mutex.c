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

/**@file tsk_mutex.c
 * @brief Pthread/Windows Mutex utility functions.
 *
 *
 *

 */
#include "tsk_mutex.h"
#include "tsk_memory.h"
#include "tsk_debug.h"

#if TSK_UNDER_WINDOWS
#include <windows.h>
#include "tsk_errno.h"
typedef HANDLE MUTEX_T;
#define MUTEX_S void
#define TSK_ERROR_NOT_OWNER ERROR_NOT_OWNER
#else
#include <pthread.h>
#define MUTEX_S pthread_mutex_t
typedef MUTEX_S *MUTEX_T;
#define TSK_ERROR_NOT_OWNER EPERM
#if !defined(TSK_RECURSIVE_MUTEXATTR)
#if defined(PTHREAD_MUTEX_RECURSIVE)
#define TSK_RECURSIVE_MUTEXATTR PTHREAD_MUTEX_RECURSIVE
#else
#define TSK_RECURSIVE_MUTEXATTR PTHREAD_MUTEX_RECURSIVE_NP
#endif
#endif /* TSK_RECURSIVE_MUTEXATTR */
#endif

#if defined(__GNUC__) || defined(__SYMBIAN32__)
#include <errno.h>
#endif

/**@defgroup tsk_mutex_group Pthread/Windows Mutex utility functions.
*/

/**@ingroup tsk_mutex_group
* Creates new recursive mutex handle.
* @retval New mutex handle. It is up to you free the returned handle using  @ref tsk_mutex_destroy.
* @sa @ref tsk_mutex_destroy() @ref tsk_mutex_create_2()
*/
tsk_mutex_handle_t *tsk_mutex_create ()
{
    return tsk_mutex_create_2 (tsk_true);
}

/**@ingroup tsk_mutex_group
 * Creates new recursive mutex handle.
 * @param recursive Whether we want a recursive/reentrant mutex or not. For more inforation about reentrant mutexes: http://en.wikipedia.org/wiki/Reentrant_mutex.
 * @retval New mutex handle. It is up to you free the returned handle using  @ref tsk_mutex_destroy.
 * @sa @ref tsk_mutex_destroy.
 */
tsk_mutex_handle_t *tsk_mutex_create_2 (tsk_bool_t recursive)
{
    MUTEX_T handle = tsk_null;

#if TSK_UNDER_WINDOWS
#if TSK_UNDER_WINDOWS_RT
    handle = CreateMutexEx (NULL, NULL, 0x00000000, MUTEX_ALL_ACCESS);
#else
    handle = CreateMutex (NULL, FALSE, NULL);
#endif
#else
    int ret;
    pthread_mutexattr_t mta;

    if ((ret = pthread_mutexattr_init (&mta)))
    {
        TSK_DEBUG_ERROR ("pthread_mutexattr_init failed with error code %d", ret);
        return tsk_null;
    }
    if (recursive && (ret = pthread_mutexattr_settype (&mta, TSK_RECURSIVE_MUTEXATTR)))
    {
        TSK_DEBUG_ERROR ("pthread_mutexattr_settype failed with error code %d", ret);
        pthread_mutexattr_destroy (&mta);
        return tsk_null;
    }

    /* if we are here: all is ok */
    handle = tsk_calloc (1, sizeof (MUTEX_S));
    if (pthread_mutex_init ((MUTEX_T)handle, &mta))
    {
        TSK_FREE (handle);
    }
    pthread_mutexattr_destroy (&mta);
#endif

    if (!handle)
    {
        TSK_DEBUG_ERROR ("Failed to create new mutex.");
    }
    return handle;
}

/**@ingroup tsk_mutex_group
* Lock a mutex. You must use @ref tsk_mutex_unlock to unlock the mutex.
* @param handle The handle of the mutex to lock.
* @retval Zero if succeed and non-zero error code otherwise.
* @sa @ref tsk_mutex_unlock.
*/
int tsk_mutex_lock (tsk_mutex_handle_t *handle)
{
    int ret = EINVAL;
    if (handle)
    {

#if TSK_UNDER_WINDOWS
#if TSK_UNDER_WINDOWS_RT
        if ((ret = WaitForSingleObjectEx ((MUTEX_T)handle, INFINITE, TRUE)) == WAIT_FAILED)
#else
        if ((ret = WaitForSingleObject ((MUTEX_T)handle, INFINITE)) == WAIT_FAILED)
#endif
#else
        if ((ret = pthread_mutex_lock ((MUTEX_T)handle)))
#endif
        {
            TSK_DEBUG_ERROR ("Failed to lock the mutex: %d", ret);
        }
    }
    return ret;
}

/**@ingroup tsk_mutex_group
* Unlock a mutex previously locked using @ref tsk_mutex_lock.
* @param handle The handle of the mutex to unlock.
* @retval Zero if succeed and non-zero otherwise.
* @sa @ref tsk_mutex_lock.
*/
int tsk_mutex_unlock (tsk_mutex_handle_t *handle)
{
    int ret = EINVAL;
    if (handle)
    {
#if TSK_UNDER_WINDOWS
        if ((ret = ReleaseMutex ((MUTEX_T)handle) ? 0 : -1))
        {
            ret = GetLastError ();
#else
        if ((ret = pthread_mutex_unlock ((MUTEX_T)handle)))
        {
#endif
            if (ret == TSK_ERROR_NOT_OWNER)
            {
                TSK_DEBUG_WARN ("The calling thread does not own the mutex: %d", ret);
            }
            else
            {
                TSK_DEBUG_ERROR ("Failed to unlock the mutex: %d", ret);
            }
        }
    }
    return ret;
}

/**@ingroup tsk_mutex_group
* Free/destroy a mutex.
* @param handle The mutex to free.
* @sa @ref tsk_mutex_create.
*/
void tsk_mutex_destroy (tsk_mutex_handle_t **handle)
{
    if (handle && *handle)
    {
#if TSK_UNDER_WINDOWS
        CloseHandle ((MUTEX_T)*handle);
        *handle = tsk_null;
#else
        pthread_mutex_destroy ((MUTEX_T)*handle);
        tsk_free (handle);
#endif
    }
    else
    {
        TSK_DEBUG_WARN ("Cannot free an uninitialized mutex");
    }
}

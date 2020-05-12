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

/**@file tsk_mutex.h
 * @brief Pthread Mutex.
 *
 *
 *

 */
#ifndef _TINYSAK_MUTEX_H_
#define _TINYSAK_MUTEX_H_

#include "tinysak_config.h"

TSK_BEGIN_DECLS

/**@ingroup tsk_mutex_group
* Mutex handle.
*/
typedef void tsk_mutex_handle_t;

TINYSAK_API tsk_mutex_handle_t* tsk_mutex_create();
TINYSAK_API tsk_mutex_handle_t* tsk_mutex_create_2(tsk_bool_t recursive);
TINYSAK_API int tsk_mutex_lock(tsk_mutex_handle_t* handle);
TINYSAK_API int tsk_mutex_unlock(tsk_mutex_handle_t* handle);
TINYSAK_API void tsk_mutex_destroy(tsk_mutex_handle_t** handle);

TSK_END_DECLS

#endif /* _TINYSAK_MUTEX_H_ */


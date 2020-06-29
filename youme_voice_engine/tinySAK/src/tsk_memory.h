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

/**@file tsk_memory.h
 * @brief Useful memory management functions to handle memory.
 *
 *
 *

 */
#ifndef _TINYSAK_MEMORY_H_
#define _TINYSAK_MEMORY_H_

#include "tinysak_config.h"

#include <stdlib.h> /* tsk_size_t */

/**@ingroup tsk_memory_group
* @def TSK_SAFE_FREE
* Safely free the memory pointed by @a ptr.
*/
/**@ingroup tsk_memory_group
* @def TSK_FREE
* Safely free the memory pointed by @a ptr.
*/

TSK_BEGIN_DECLS

#define TSK_SAFE_FREE(ptr) (void)tsk_free((void**)(&ptr));
#define TSK_FREE(ptr) TSK_SAFE_FREE((ptr))
#define TSK_SAFE_FREE_ARRAY(pptr, count){ \
	int __i; \
	for(__i = 0; __i < (count); ++__i) \
		TSK_SAFE_FREE((pptr)[__i]); \
}
#define TSK_FREE_ARRAY(pptr, count) TSK_SAFE_FREE_ARRAY((pptr), (count))
#define TSK_SAFE_FREE_TABLE(pptr) TSK_SAFE_FREE_ARRAY((pptr), (sizeof((pptr))/sizeof((pptr)[0])))
#define TSK_FREE_TABLE(pptr) TSK_SAFE_FREE_TABLE((pptr))

TINYSAK_API void* tsk_malloc(tsk_size_t size);
TINYSAK_API void* tsk_realloc (void * ptr, tsk_size_t size);
TINYSAK_API void tsk_free(void** ptr);
TINYSAK_API void* tsk_calloc(tsk_size_t num, tsk_size_t size);

TSK_END_DECLS

#endif /* _TINYSAK_MEMORY_H_ */


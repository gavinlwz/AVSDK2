//
//  tsk_cond.h
//  youme_voice_engine
//
//  Created by bhb on 2017/9/26.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tsk_cond_h
#define tsk_cond_h

#include "tinysak_config.h"

TSK_BEGIN_DECLS


typedef void tsk_cond_handle_t;

TINYSAK_API tsk_cond_handle_t* tsk_cond_create(tsk_bool_t manual_reset, tsk_bool_t initially_signaled);
TINYSAK_API void        tsk_cond_set(tsk_cond_handle_t* handle);
TINYSAK_API void        tsk_cond_reset(tsk_cond_handle_t* handle);
TINYSAK_API tsk_bool_t  tsk_cond_wait(tsk_cond_handle_t* handle, int milliseconds);
TINYSAK_API void        tsk_cond_destroy(tsk_cond_handle_t** handle);

TSK_END_DECLS

#endif /* tsk_cond_h */

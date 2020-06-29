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
/**@file tdav_win32.h
 * @brief tinyDAV WIN32 helper functions.
 *
*
 *

 */
#ifndef TINYMEDIA_TDAV_WIN32_H
#define TINYMEDIA_TDAV_WIN32_H

#include "tinydav_config.h"

#include "tinysak_config.h" /* tsk_bool_t */

#if TDAV_UNDER_WINDOWS

TDAV_BEGIN_DECLS

int tdav_win32_init();
int tdav_win32_get_osversion(unsigned long* version_major, unsigned long* version_minor);
tsk_bool_t tdav_win32_is_win8_or_later();
tsk_bool_t tdav_win32_is_win7_or_later();
tsk_bool_t tdav_win32_is_winvista_or_later();
tsk_bool_t tdav_win32_is_winxp_or_later();
const char* tdav_get_current_directory_const();
int tdav_win32_deinit();

TDAV_END_DECLS

#endif /* TDAV_UNDER_WINDOWS */

#endif /* TINYMEDIA_TDAV_WIN32_H */

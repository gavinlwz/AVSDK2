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

/**@file tinysak_config.h
 * @brief Global configuration file.
 *
 * This file incude all your preferences or configuration. All specific configuration
 * must be defined in this file. You must include this file in all your header files.
 *
 *
 *

 */

#ifndef _TINYSAK_H_
#define _TINYSAK_H_

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition.
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define TSK_UNDER_WINDOWS	1
#	if defined(_WIN32_WCE) || defined(UNDER_CE)
#		define TSK_UNDER_WINDOWS_CE	1
#		define TSK_STDCALL			__cdecl
#	else
#		define TSK_STDCALL 
#	endif
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define TSK_UNDER_WINDOWS_RT		1
#	endif
#else
#	define TSK_STDCALL
#endif

// OS X or iOS
#if defined(__APPLE__)
#	define TSK_UNDER_APPLE				1
#   include <TargetConditionals.h>
#   include <Availability.h>
#endif
#if TARGET_OS_MAC
#	define TSK_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define TSK_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define TSK_UNDER_IPHONE_SIMULATOR	1
#endif

/* Used on Windows and Symbian systems to export/import public functions and global variables.
*/
#if !defined(__GNUC__) && defined(TINYSAK_EXPORTS)
# 	define TINYSAK_API		__declspec(dllexport)
#	define TINYSAK_GEXTERN	extern __declspec(dllexport)
#elif !defined(__GNUC__) && !defined(TINYSAK_IMPORTS_IGNORE)
# 	define TINYSAK_API		__declspec(dllimport)
#	define TINYSAK_GEXTERN	__declspec(dllimport)
#else
#	define TINYSAK_API
#	define TINYSAK_GEXTERN	extern
#endif

/* Guards against C++ name mangling */
#ifdef __cplusplus
#	define TSK_BEGIN_DECLS extern "C" {
#	define TSK_END_DECLS }
#else
#	define TSK_BEGIN_DECLS 
#	define TSK_END_DECLS
#endif

#if defined(_MSC_VER)
#	define TSK_INLINE	__forceinline
#elif defined(__GNUC__) && !defined(__APPLE__)
#	define TSK_INLINE	__inline
#else
#	define TSK_INLINE	
#endif


/* Disable some well-known warnings for M$ Visual Studio*/
#ifdef _MSC_VER
#	if !defined(_CRT_SECURE_NO_WARNINGS)
#		define _CRT_SECURE_NO_WARNINGS
#	endif /* _CRT_SECURE_NO_WARNINGS */
#	pragma warning( disable : 4996 )
#endif

/*	Features */
#if !defined (HAVE_GETTIMEOFDAY)
	#if TSK_UNDER_WINDOWS
	#	define HAVE_GETTIMEOFDAY				0
	#else
	#	define HAVE_GETTIMEOFDAY				1
	#endif
#endif /* HAVE_GETTIMEOFDAY */

#if ANDROID
#	define HAVE_CLOCK_GETTIME				1
#endif

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "tsk_common.h"


#if HAVE_CONFIG_H
#if WIN32
#include "win_config.h"
#elif ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#endif

#endif /* _TINYSAK_H_ */


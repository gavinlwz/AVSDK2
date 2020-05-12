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

#ifndef TINYMEDIA_CONFIG_H
#define TINYMEDIA_CONFIG_H

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition.
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define TMEDIA_UNDER_WINDOWS	1
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define TMEDIA_UNDER_WINDOWS_RT		1
#	endif
#endif

// OS X or iOS
#if defined(__APPLE__)
#	define TMEDIA_UNDER_APPLE				1
#   include <TargetConditionals.h>
#   include <Availability.h>
#endif
#if TARGET_OS_MAC
#	define TMEDIA_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define TMEDIA_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define TMEDIA_UNDER_IPHONE_SIMULATOR	1
#endif

#if (TMEDIA_UNDER_WINDOWS || defined(__SYMBIAN32__)) && defined(TINYMEDIA_EXPORTS)
# 	define TINYMEDIA_API		__declspec(dllexport)
# 	define TINYMEDIA_GEXTERN extern __declspec(dllexport)
#elif (TMEDIA_UNDER_WINDOWS || defined(__SYMBIAN32__)) && !defined(TINYMEDIA_IMPORTS_IGNORE)
# 	define TINYMEDIA_API __declspec(dllimport)
# 	define TINYMEDIA_GEXTERN __declspec(dllimport)
#else
#	define TINYMEDIA_API
#	define TINYMEDIA_GEXTERN	extern
#endif

/* Guards against C++ name mangling 
*/
#ifdef __cplusplus
#	define TMEDIA_BEGIN_DECLS extern "C" {
#	define TMEDIA_END_DECLS }
#else
#	define TMEDIA_BEGIN_DECLS 
#	define TMEDIA_END_DECLS
#endif

/* Disable some well-known warnings
*/
#ifdef _MSC_VER
#	if !defined(_CRT_SECURE_NO_WARNINGS)
#		define _CRT_SECURE_NO_WARNINGS
#	endif /* _CRT_SECURE_NO_WARNINGS */
#endif


#include <stdint.h>
#ifdef __SYMBIAN32__
#include <stdlib.h>
#endif

#if HAVE_CONFIG_H
	#if ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#endif

#endif // TINYMEDIA_CONFIG_H

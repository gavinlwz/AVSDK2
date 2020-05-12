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


#ifndef TINYDAV_CONFIG_H
#define TINYDAV_CONFIG_H

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define TDAV_UNDER_WINDOWS	1
#	if defined(_WIN32_WCE) || defined(UNDER_CE)
#		define TDAV_UNDER_WINDOWS_CE	1
#	endif
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define TDAV_UNDER_WINDOWS_RT			1
#		if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#			define TDAV_UNDER_WINDOWS_PHONE		1
#		endif
#	endif
#endif

// OS X or iOS
#if defined(__APPLE__)
#	define TDAV_UNDER_APPLE				1
#   include <TargetConditionals.h>
#   include <Availability.h>
#endif
#if TARGET_OS_MAC
#	define TDAV_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define TDAV_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define TDAV_UNDER_IPHONE_SIMULATOR	1
#endif

// x86
#if TDAV_UNDER_WINDOWS || defined(__x86_64__) || defined(__x86__) || defined(__i386__)
#	define TDAV_UNDER_X86				1
#endif

// Mobile
#if defined(_WIN32_WCE) || defined(ANDROID) || TDAV_UNDER_IPHONE || TDAV_UNDER_IPHONE_SIMULATOR || TDAV_UNDER_WINDOWS_PHONE
#	define TDAV_UNDER_MOBILE	1
#endif

#if (TDAV_UNDER_WINDOWS || defined(__SYMBIAN32__)) && defined(TINYDAV_EXPORTS)
# 	define TINYDAV_API		__declspec(dllexport)
# 	define TINYDAV_GEXTERN extern __declspec(dllexport)
#elif (TDAV_UNDER_WINDOWS || defined(__SYMBIAN32__)) && !defined(TINYDAV_IMPORTS_IGNORE)
# 	define TINYDAV_API __declspec(dllimport)
# 	define TINYDAV_GEXTERN __declspec(dllimport)
#else
#	define TINYDAV_API
#	define TINYDAV_GEXTERN	extern
#endif

/* Guards against C++ name mangling  */
#ifdef __cplusplus
#	define TDAV_BEGIN_DECLS extern "C" {
#	define TDAV_END_DECLS }
#else
#	define TDAV_BEGIN_DECLS 
#	define TDAV_END_DECLS
#endif

#ifdef _MSC_VER
#if HAVE_FFMPEG // FFMPeg warnings (treated as errors)
#	pragma warning (disable:4244) 
#endif
#	if !defined(__cplusplus)
#	define inline __inline
#	endif
#	if !defined(_CRT_SECURE_NO_WARNINGS)
#		define _CRT_SECURE_NO_WARNINGS
#	endif /* _CRT_SECURE_NO_WARNINGS */
#endif


#include <stdint.h>
#ifdef __SYMBIAN32__
#include <stdlib.h>
#endif

#define HAVE_CONFIG_H 1
#if HAVE_CONFIG_H
	#if ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#endif

#ifndef USE_WEBRTC_RESAMPLE
#define USE_WEBRTC_RESAMPLE
#endif

#endif // TINYDAV_CONFIG_H

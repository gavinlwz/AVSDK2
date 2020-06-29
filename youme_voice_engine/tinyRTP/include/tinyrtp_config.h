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

#ifndef TINYRTP_CONFIG_H
#define TINYRTP_CONFIG_H

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition.
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define TRTP_UNDER_WINDOWS	1
#	if defined(_WIN32_WCE) || defined(UNDER_CE)
#		define TRTP_UNDER_WINDOWS_CE	1
#	endif
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define TRTP_UNDER_WINDOWS_RT		1
#	endif
#endif

#if (TRTP_UNDER_WINDOWS || defined(__SYMBIAN32__)) && defined(TINYRTP_EXPORTS)
# 	define TINYRTP_API		__declspec(dllexport)
# 	define TINYRTP_GEXTERN __declspec(dllexport)
#elif (TRTP_UNDER_WINDOWS || defined(__SYMBIAN32__)) && !defined(TINYRTP_IMPORTS_IGNORE)
# 	define TINYRTP_API __declspec(dllimport)
# 	define TINYRTP_GEXTERN __declspec(dllimport)
#else
#	define TINYRTP_API
#	define TINYRTP_GEXTERN	extern
#endif

/* Guards against C++ name mangling 
*/
#ifdef __cplusplus
#	define TRTP_BEGIN_DECLS extern "C" {
#	define TRTP_END_DECLS }
#else
#	define TRTP_BEGIN_DECLS 
#	define TRTP_END_DECLS
#endif

/* Disable some well-known warnings
*/
#ifdef _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS
#endif


#if !defined(TRTP_RTP_VERSION)

#endif /* TRTP_RTP_VERSION */
//版本号不要乱改。 协议版本不一样，处理方式不一样
#define TRTP_RTP_VERSION 3 //卧槽。 rtp 头的协议版本号是2 bit。 范围是0-3. 尼玛坑爹啊。这么小有毛用啊


#include <stdint.h>
#ifdef __SYMBIAN32__
#   include <stdlib.h>
#endif

#if defined(__APPLE__)
#   include <TargetConditionals.h>
#   include <Availability.h>
#endif

#if HAVE_CONFIG_H
	#if ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#endif

#endif // TINYRTP_CONFIG_H

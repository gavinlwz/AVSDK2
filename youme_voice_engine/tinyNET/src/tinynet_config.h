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

/**@file tinynet_config.h
 * @brief Global configuration file.
 *
 * This file incude all your preferences or configuration. All specific configuration
 * must be defined in this file. You must include this file in all your header files.
 *
 *
 *

 */

#ifndef _TINYNET_H_
#define _TINYNET_H_
#define HAVE_CONFIG_H 1
#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition.
#if defined(WIN32)|| defined(_WIN32) || defined(_WIN32_WCE)
#	define TNET_UNDER_WINDOWS	1
#	if defined(_WIN32_WCE) || defined(UNDER_CE)
#		define TNET_UNDER_WINDOWS_CE	1
#	endif
#	if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP || WINAPI_FAMILY == WINAPI_FAMILY_APP)
#		define TNET_UNDER_WINDOWS_RT		1
#		if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#			define TNET_UNDER_WINDOWS_PHONE		1
#		endif
#	endif
#   define TNET_UNDER_WINDOWS_DESKTOP (TNET_UNDER_WINDOWS && !TNET_UNDER_WINDOWS_CE && !TNET_UNDER_WINDOWS_RT && !TNET_UNDER_WINDOWS_PHONE)
#endif

// OS X or iOS
#if defined(__APPLE__)
#	define TNET_UNDER_APPLE				1
#   include <TargetConditionals.h>
#   include <Availability.h>
#endif
#if TARGET_OS_MAC
#	define TNET_UNDER_MAC				1
#endif
#if TARGET_OS_IPHONE
#	define TNET_UNDER_IPHONE			1
#endif
#if TARGET_IPHONE_SIMULATOR
#	define TNET_UNDER_IPHONE_SIMULATOR	1
#endif

/**@def  TINYNET_API
* Used on Windows and Sysbian systems to export public functions.
*/
#if !defined(__GNUC__) && defined(TINYNET_EXPORTS)
# 	define TINYNET_API __declspec(dllexport)
# 	define TINYNET_GEXTERN extern __declspec(dllexport)
#elif !defined(__GNUC__) && !defined(TINYNET_IMPORTS_IGNORE)
# 	define TINYNET_API __declspec(dllimport)
# 	define TINYNET_GEXTERN __declspec(dllimport)
#else
#	define TINYNET_API
#	define TINYNET_GEXTERN	extern
#endif

/* define "TNET_DEPRECATED(func)" macro */
#if defined(__GNUC__)
#	define TNET_DEPRECATED(func) __attribute__ ((deprecated)) func
#elif defined(_MSC_VER)
#	define TNET_DEPRECATED(func) __declspec(deprecated) func
#else
#	pragma message("WARNING: Deprecated not supported for this compiler")
#	define TNET_DEPRECATED(func) func
#endif

/* Guards against C++ name mangling  */
#ifdef __cplusplus
#	define TNET_BEGIN_DECLS extern "C" {
#	define TNET_END_DECLS }
#else
#	define TNET_BEGIN_DECLS 
#	define TNET_END_DECLS
#endif

#if defined(_MSC_VER)
#	define TNET_INLINE	__forceinline
#elif defined(__GNUC__) && !defined(__APPLE__) && !defined(__cplusplus)
#	define TNET_INLINE	__inline
#else
#	define TNET_INLINE	
#endif

/* have poll()? */
/* Do not use WSAPoll event if it's supported under Vista */
#if !HAVE_CONFIG_H
#	if ANDROID || defined(__APPLE__)
#		define USE_POLL	1
#		define HAVE_POLL	1
#		define HAVE_POLL_H	1
#	endif
#endif

/* Used in TURN/STUN2 attributes. */
#define TNET_SOFTWARE	"IM-client/OMA1.0 youme/v2.0.0"
#define TNET_IANA_PEN		35368
#define TNET_RESOLV_CONF_PATH "/etc/resolv.conf"

#include <stdint.h>

#if HAVE_CONFIG_H
#if ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#elif defined(__APPLE__)
#	define HAVE_GETIFADDRS		1
#	define HAVE_IFADDRS_H		1
#	define HAVE_DNS_H			1
#	define HAVE_NET_ROUTE_H		1
#	define HAVE_NET_IF_DL_H		1
#	define HAVE_STRUCT_RT_METRICS	1
#	define HAVE_STRUCT_SOCKADDR_DL	1
#	define HAVE_SYS_PARAM_H		1
#	define TNET_HAVE_SS_LEN		1
#	define TNET_HAVE_SA_LEN		0
#   if (__IPHONE_OS_VERSION_MIN_REQUIRED >= 50000)
#       define HAVE_GSSAPI_H 1
#   endif
#endif

#endif /* _TINYNET_H_ */



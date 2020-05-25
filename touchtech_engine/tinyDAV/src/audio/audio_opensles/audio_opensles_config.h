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
#ifndef AUDIO_OPENSLES_CONFIG_H
#define AUDIO_OPENSLES_CONFIG_H

#ifdef __SYMBIAN32__
#undef _WIN32 /* Because of WINSCW */
#endif

// Windows (XP/Vista/7/CE and Windows Mobile) macro definition
#if defined(WIN32) || defined(_WIN32) || defined(_WIN32_WCE)
#define AUDIO_OPENSLES_UNDER_WINDOWS 1
#endif

// OS X or iOS
#if defined(__APPLE__)
#define AUDIO_OPENSLES_UNDER_APPLE 1
#endif
#if TARGET_OS_MAC
#define AUDIO_OPENSLES_UNDER_MAC 1
#endif
#if TARGET_OS_IPHONE
#define AUDIO_OPENSLES_UNDER_IPHONE 1
#endif
#if TARGET_IPHONE_SIMULATOR
#define AUDIO_OPENSLES_UNDER_IPHONE_SIMULATOR 1
#endif

#if defined(ANDROID)
#define AUDIO_OPENSLES_UNDER_ANDROID 1
#endif

// x86
#if AUDIO_OPENSLES_UNDER_WINDOWS || defined(__x86_64__) || defined(__x86__) || defined(__i386__)
#define AUDIO_OPENSLES_UNDER_X86 1
#endif

// Mobile
#if defined(_WIN32_WCE) || defined(ANDROID) // iOS (not true)=> || defined(IOS)
#define AUDIO_OPENSLES_UNDER_MOBILE 1
#endif

#if (AUDIO_OPENSLES_UNDER_WINDOWS || defined(__SYMBIAN32__)) && defined(AUDIO_OPENSLES_EXPORTS)
#define AUDIO_OPENSLES_API __declspec(dllexport)
#define AUDIO_OPENSLES_GEXTERN __declspec(dllexport)
#elif(AUDIO_OPENSLES_UNDER_WINDOWS || defined(__SYMBIAN32__))
#define AUDIO_OPENSLES_API __declspec(dllimport)
#define AUDIO_OPENSLES_GEXTERN __declspec(dllimport)
#else
#define AUDIO_OPENSLES_API
#define AUDIO_OPENSLES_GEXTERN extern
#endif

// Guards against C++ name mangling
#ifdef __cplusplus
#define AUDIO_OPENSLES_BEGIN_DECLS extern "C" {
#define AUDIO_OPENSLES_END_DECLS }
#else
#define AUDIO_OPENSLES_BEGIN_DECLS
#define AUDIO_OPENSLES_END_DECLS
#endif

#ifdef _MSC_VER
#if HAVE_FFMPEG // FFMPeg warnings (treated as errors)
#pragma warning(disable : 4244)
#endif
#define inline __inline
#define _CRT_SECURE_NO_WARNINGS
#endif

// Detecting C99 compilers
#if (__STDC_VERSION__ == 199901L) && !defined(__C99__)
#define __C99__
#endif

#include <stdint.h>
#ifdef __SYMBIAN32__
#include <stdlib.h>
#endif
#endif // AUDIO_OPENSLES_CONFIG_H
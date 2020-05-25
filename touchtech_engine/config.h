#define MCU_REDIRECT 1
#define SDK_VALIDATE 1

/* fatal debug */
#define DEBUG_LEVEL DEBUG_LEVEL_INFO

#define __OPTIMIZE__ 1
#define FIXED_POINT 1
#define DISABLE_FLOAT_API 1

/* HAVE_WEBRTC */
#define HAVE_WEBRTC 1
#ifdef WIN32
#else
#ifndef WEBRTC_POSIX
#define WEBRTC_POSIX 1
#endif
#endif


/* HAVE_WEBRTC_DENOISE */
#define HAVE_WEBRTC_DENOISE 1

#define WEBRTC_AEC_AGGRESSIVE 0

#define WEBRTC_NS_AGGRESSIVE 0


//#define WEBRTC_DETECT_ARM_NEON 0


/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Check whether PTHREAD_MUTEX_RECURSIVE_NP is defined */
#define TSK_RECURSIVE_MUTEXATTR PTHREAD_MUTEX_RECURSIVE

/* Setting USE_POLL to 1 for backward compatibility */
#define USE_POLL 1


/* Define if <alsa/asoundlib.h> header exist */
#ifdef OS_LINUX
#define HAVE_ALSA_ASOUNDLIB_H 1
#endif

/* Checks if the installed libsrtp version support append_salt_to_key()
   function */
/* #undef HAVE_APPEND_SALT_TO_KEY */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#ifndef WIN32
#define HAVE_ARPA_INET_H 1
#endif


/* Define to 1 if we have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 if you have the `closedir' function. */
#define HAVE_CLOSEDIR 1

/* Define to 1 if you have the <dirent.h> header file. */
#ifndef WIN32

#define HAVE_DIRENT_H 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if <dns.h> header exist */
#define HAVE_DNS_H 0

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getdtablesize' function. */
#ifndef WIN32
#define HAVE_GETDTABLESIZE 1
#endif
/* Define to 1 if you have the 'getifaddrs' function */
/* #undef HAVE_GETIFADDRS */

/* Define to 1 if you have the `getpid' function. */
#ifndef WIN32
	#define HAVE_GETPID 1
#endif
/* Define to 1 if you have the `getrlimit' function. */
#ifndef WIN32
#define HAVE_GETRLIMIT 1
#endif



/* Define to 1 if you have the `inet_ntop' function. */
#ifndef WIN32

#define HAVE_INET_NTOP 1
#endif

/* Define to 1 if you have the `inet_pton' function. */
#define HAVE_INET_PTON 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

#define HAVE_LIBOPUS 1

/* Define to 1 if you have the <opus/opus.h> header file. */
#define HAVE_OPUS_OPUS_H 1


/* Define if <linux/soundcard.h> header exist */
#define HAVE_LINUX_SOUNDCARD_H 0

/* Define if <linux/videodev2.h> header exist */
#define HAVE_LINUX_VIDEODEV2_H 0

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1


/* Define if <net/if_dl.h> header exist */
#define HAVE_NET_IF_DL_H 0


/* Define if <net/route.h> header exist */
#define HAVE_NET_ROUTE_H 1

/* Define to 1 if you have the `opendir' function. */
#ifndef WIN32

#define HAVE_OPENDIR 1
#endif


/* HAVE_OPENSSL_DTLS */
#define HAVE_OPENSSL_DTLS 0

/* HAVE_OPENSSL_DTLS_SRTP */
#define HAVE_OPENSSL_DTLS_SRTP 0


/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `stricmp' function. */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnicmp' function. */
/* #undef HAVE_STRNICMP */

/* Define to 1 if you have the `strtok_r' function. */
#define HAVE_STRTOK_R 1

/* Define to 1 if you have the `strtok_s' function. */
/* #undef HAVE_STRTOK_S */

/* Define to 1 if the system has the type `struct dirent'. */
#define HAVE_STRUCT_DIRENT 1

/* Define to 1 if the system has the type `struct rlimit'. */
#define HAVE_STRUCT_RLIMIT 1

/* Define to 1 if the system has the type `struct rt_metrics'. */
/* #undef HAVE_STRUCT_RT_METRICS */

/* Define to 1 if the system has the type `struct sockaddr_dl'. */
/* #undef HAVE_STRUCT_SOCKADDR_DL */

/* Define to 1 if you have the <sys/param.h> header file. */
#ifndef WIN32
#define HAVE_SYS_PARAM_H 1
#endif
/* Define to 1 if you have the <sys/resource.h> header file. */
#ifndef WIN32
	#define HAVE_SYS_RESOURCE_H 1
#endif


/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef WIN32

#define HAVE_UNISTD_H 1
/* Check whether __sync_fetch_and_add is defined */
#define HAVE___SYNC_FETCH_AND_ADD 1

/* Check whether __sync_fetch_and_sub is defined */
#define HAVE___SYNC_FETCH_AND_SUB 1

#endif

/* Define to 1 if you have the <video/tdav_session_video.h> header file. */
#define HAVE_VIDEO 1

/* Define MacOS & iOS frameworks identifier. */


#if MAC_OS

    #define YMApplicationDidBecomeActiveNotification NSApplicationDidBecomeActiveNotification
    #define YMApplicationWillResignActiveNotification NSApplicationWillResignActiveNotification

    #define YMView NSView
    #define YMColor NSColor
    #define YMScreen NSScreen
    #define YMOpenGLContext NSOpenGLContext
    #define YMOpenGLLayer CAOpenGLLayer
    #define YMCVOpenGLTextureRef CVOpenGLTextureRef
    #define YMCVOpenGLTextureCacheRef CVOpenGLTextureCacheRef
    #define YMCVOpenGLTextureCacheCreateTextureFromImage CVOpenGLTextureCacheCreateTextureFromImage
    #define YMCVOpenGLTextureCacheFlush CVOpenGLTextureCacheFlush
    #define YMCVOpenGLTextureGetName CVOpenGLTextureGetName

    #define YM_SET_OPENGL_CURRENT_CONTEXT(context) [context makeCurrentContext];
    #define YM_CVOpenGLTextureGetTarget CVOpenGLTextureGetTarget
#else

    #define YMApplicationDidBecomeActiveNotification UIApplicationDidBecomeActiveNotification
    #define YMApplicationWillResignActiveNotification UIApplicationWillResignActiveNotification

    #define YMView UIView
    #define YMColor UIColor
    #define YMScreen UIScreen
    #define YMOpenGLContext EAGLContext
    #define YMOpenGLLayer CAEAGLLayer
    #define YMCVOpenGLTextureRef CVOpenGLESTextureRef
    #define YMCVOpenGLTextureCacheRef CVOpenGLESTextureCacheRef
    #define YMCVOpenGLTextureCacheCreateTextureFromImage CVOpenGLESTextureCacheCreateTextureFromImage
    #define YMCVOpenGLTextureCacheFlush CVOpenGLESTextureCacheFlush
    #define YMCVOpenGLTextureGetName CVOpenGLESTextureGetName

    #define YM_SET_OPENGL_CURRENT_CONTEXT(context) [YMOpenGLContext setCurrentContext:context];
    #define YM_CVOpenGLTextureGetTarget CVOpenGLESTextureGetTarget
#endif



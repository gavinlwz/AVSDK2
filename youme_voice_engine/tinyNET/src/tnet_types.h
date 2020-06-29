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

/**@file tnet_types.h
 * @brief ????.
 *
 */
#ifndef TNET_TYPES_H
#define TNET_TYPES_H

#include "tinynet_config.h"

#if TNET_UNDER_WINDOWS
#	include	<winsock2.h>
#	include	<ws2tcpip.h>
#	if !TNET_UNDER_WINDOWS_RT
#		include <iphlpapi.h>
#	endif
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/select.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <fcntl.h>
#	include <sys/ioctl.h>
#	include <unistd.h>
#	include <net/if.h>
#	if HAVE_IFADDRS_H
#		include <ifaddrs.h>
#	endif
#	if HAVE_POLL_H
#		include <poll.h>
#	endif /* HAVE_POLL_H */
#endif

#if defined(TNET_HAVE_SCTP)
#include <netinet/sctp.h>
#endif

#include "tsk_errno.h"
#include "tsk_list.h"

TNET_BEGIN_DECLS

#if !defined(TNET_FINGERPRINT_MAX)
#	define TNET_FINGERPRINT_MAX	256
#endif /* TNET_FINGERPRINT_MAX */

#if !defined(TNET_DTLS_MTU)
#	define TNET_DTLS_MTU 900
#endif /* TNET_DTLS_MTU */

typedef int tnet_fd_t;
typedef uint16_t tnet_port_t;
typedef int tnet_family_t;
typedef char tnet_host_t[NI_MAXHOST];
typedef char tnet_ip_t[INET6_ADDRSTRLEN];
typedef uint8_t tnet_mac_address[6];
typedef unsigned char tnet_fingerprint_t[TNET_FINGERPRINT_MAX + 1];

typedef tsk_list_t tnet_interfaces_L_t; /**< List of @ref tnet_interface_t elements*/
typedef tsk_list_t tnet_addresses_L_t; /**< List of @ref tnet_address_t elements*/


typedef enum tnet_proxy_type_e {
    tnet_proxy_type_none = 0x00,
    tnet_proxy_type_http = (0x01 << 0), // CONNECT using HTTP then starting SSL handshaking if needed
    tnet_proxy_type_https = (0x01 << 1), // CONNECT using HTTPS then starting SSL handshaking if needed
    tnet_proxy_type_socks4 = (0x01 << 2),
    tnet_proxy_type_socks4a = (0x01 << 3),
    tnet_proxy_type_socks5 = (0x01 << 4),
}
tnet_proxy_type_t;


#if TNET_UNDER_WINDOWS
#	define TNET_INVALID_SOCKET				INVALID_SOCKET
#	define TNET_ERROR_WOULDBLOCK			WSAEWOULDBLOCK
#   define TNET_ERROR_NOSPACE               WSAEWOULDBLOCK
#	define TNET_ERROR_INPROGRESS			WSAEINPROGRESS
#	define TNET_ERROR_CONNRESET				WSAECONNRESET
#	define TNET_ERROR_INTR					WSAEINTR
#	define TNET_ERROR_ISCONN				WSAEISCONN
#	define TNET_ERROR_EAGAIN				TNET_ERROR_WOULDBLOCK /* WinSock FIX */
#	if (TNET_UNDER_WINDOWS_RT || TNET_UNDER_WINDOWS_CE) /* gai_strerrorA() links against FormatMessageA which is not allowed on the store */
#		if !defined (WC_ERR_INVALID_CHARS)
#			define WC_ERR_INVALID_CHARS 0
#		endif
		static TNET_INLINE const char* tnet_gai_strerror(int ecode)
		{
			static char aBuff[1024] = {0};
			
			WCHAR *wBuff = gai_strerrorW(ecode);
			int len;
			if((len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wBuff, wcslen(wBuff), aBuff, sizeof(aBuff) - 1, NULL, NULL)) > 0)
			{
				aBuff[len] = '\0';
			}
			else
			{
				aBuff[0] = '\0';
			}
			return aBuff;
		}
#	else
#		define tnet_gai_strerror			gai_strerrorA
#	endif
#else
#	define TNET_INVALID_SOCKET				-1
#	define TNET_ERROR_WOULDBLOCK			EWOULDBLOCK
#	define TNET_ERROR_INPROGRESS			EINPROGRESS
#	define TNET_ERROR_CONNRESET				ECONNRESET
#	define TNET_ERROR_INTR					EINTR
#	define TNET_ERROR_ISCONN				EISCONN
#	define TNET_ERROR_EAGAIN				EAGAIN
#   define TNET_ERROR_NOSPACE               ENOBUFS
#	define tnet_gai_strerror				gai_strerror
#endif
#define TNET_INVALID_FD				TNET_INVALID_SOCKET

#ifdef _WIN32_WCE
typedef TCHAR tnet_error_t[512];
#else
typedef char tnet_error_t[512];
#endif

TNET_END_DECLS

#endif /* TNET_TYPES_H */



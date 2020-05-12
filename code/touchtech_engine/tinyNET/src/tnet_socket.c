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

/**@file tnet.h
 * @brief Protocol agnostic socket.
 *
 */

#include "tnet_socket.h"

#include "tnet_utils.h"

#include "tsk_string.h"
#include "tsk_debug.h"

#include <string.h>



static int tnet_socket_close(tnet_socket_t *sock);


tnet_socket_t* tnet_socket_create_2(const char* host, tnet_port_t port_, tnet_socket_type_t type, tsk_bool_t nonblocking, tsk_bool_t bindsocket)
{
	tnet_socket_t *sock;
	if ((sock = tsk_object_new(tnet_socket_def_t))) {
		int status;
		tsk_istr_t port;
		struct addrinfo *result = tsk_null;
		struct addrinfo *ptr = tsk_null;
		struct addrinfo hints;
		tnet_host_t local_hostname;

		sock->port = port_;
		tsk_itoa(sock->port, &port);
		sock->type = type;

		memset(local_hostname, 0, sizeof(local_hostname));

		/* Get the local host name */
		if (host != TNET_SOCKET_HOST_ANY && !tsk_strempty(host)){
			memcpy(local_hostname, host, tsk_strlen(host) > sizeof(local_hostname) - 1 ? sizeof(local_hostname) - 1 : tsk_strlen(host));
		}
		else{
			if (TNET_SOCKET_TYPE_IS_IPV6(sock->type)){
				memcpy(local_hostname, "::", 2);
			}
			else {
				memcpy(local_hostname, "0.0.0.0", 7);
			}
		}

		/* hints address info structure */
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = TNET_SOCKET_TYPE_IS_IPV46(sock->type) ? AF_UNSPEC : (TNET_SOCKET_TYPE_IS_IPV6(sock->type) ? AF_INET6 : AF_INET);
		hints.ai_socktype = TNET_SOCKET_TYPE_IS_STREAM(sock->type) ? SOCK_STREAM : SOCK_DGRAM;
		hints.ai_protocol = TNET_SOCKET_TYPE_IS_STREAM(sock->type) ? IPPROTO_TCP : IPPROTO_UDP;
		hints.ai_flags = AI_PASSIVE
#if !TNET_UNDER_WINDOWS || _WIN32_WINNT>=0x600
			| AI_ADDRCONFIG
#endif
			;

		/* Performs getaddrinfo */
		if ((status = tnet_getaddrinfo(local_hostname, port, &hints, &result))) {
			TNET_PRINT_LAST_ERROR("tnet_getaddrinfo(family=%d, hostname=%s and port=%s) failed: [%s]",
				hints.ai_family, local_hostname, port, tnet_gai_strerror(status));
			goto bail;
	}

		/* Find our address. */
		for (ptr = result; ptr; ptr = ptr->ai_next){
			sock->fd = (tnet_fd_t)tnet_soccket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (ptr->ai_family != AF_INET6 && ptr->ai_family != AF_INET){
				continue;
			}
			
			//
			if (TNET_SOCKET_TYPE_IS_STREAM(sock->type)) {
				if ((status = tnet_sockfd_reuseaddr(sock->fd, 1))) {
					// do not break...continue
				}
			}

			if (bindsocket){
				/* Bind the socket */
				if ((status = bind(sock->fd, ptr->ai_addr, (int)ptr->ai_addrlen))){
					TSK_DEBUG_ERROR("bind to [%s:%s]have failed", local_hostname, port);
					tnet_socket_close(sock);
					continue;
				}

				/* Get local IP string. */
				if ((status = tnet_get_ip_n_port(sock->fd, tsk_true/*local*/, &sock->ip, &sock->port))) /* % */
					//if((status = tnet_getnameinfo(ptr->ai_addr, ptr->ai_addrlen, sock->ip, sizeof(sock->ip), 0, 0, NI_NUMERICHOST)))
				{
					TSK_DEBUG_ERROR("Failed to get local IP and port.");
					tnet_socket_close(sock);
					continue;
				}
			}

			/* sets the real socket type (if ipv46) */
			if (ptr->ai_family == AF_INET6) {
				TNET_SOCKET_TYPE_SET_IPV6Only(sock->type);
			}
			else {
				TNET_SOCKET_TYPE_SET_IPV4Only(sock->type);
			}
			break;
		}

		/* Check socket validity. */
		if (!TNET_SOCKET_IS_VALID(sock)) {
			TNET_PRINT_LAST_ERROR("Invalid socket.");
			goto bail;
		}

#if TNET_UNDER_IPHONE || TNET_UNDER_IPHONE_SIMULATOR
		/* disable SIGPIPE signal */
		{
			int yes = 1;
			if (setsockopt(sock->fd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&yes, sizeof(int))){
				TNET_PRINT_LAST_ERROR("setsockopt(SO_NOSIGPIPE) have failed.");
			}
		}
#endif /* TNET_UNDER_IPHONE */

		/* Sets the socket to nonblocking mode */
		if(nonblocking){
			if((status = tnet_sockfd_set_nonblocking(sock->fd))){
				goto bail;
			}
		}

	bail:
		/* Free addrinfo */
		tnet_freeaddrinfo(result);

		/* Close socket if failed. */
		if (status){
			if (TNET_SOCKET_IS_VALID(sock)){
				tnet_socket_close(sock);
			}
			return tsk_null;
		}
}

	return sock;
}


tnet_socket_t* tnet_socket_create(const char* host, tnet_port_t port, tnet_socket_type_t type)
{
	return tnet_socket_create_2(host, port, type, tsk_true, tsk_true);
}

/**@ingroup tnet_socket_group
* Returns the number of bytes sent (or negative value on error)
*/
int tnet_socket_send_stream(tnet_socket_t* self, const void* data, tsk_size_t size)
{
	if (!self || self->fd == TNET_INVALID_FD || !data || !size || !TNET_SOCKET_TYPE_IS_STREAM(self->type)) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}
	
	return (int)tnet_sockfd_send(self->fd, data, size, 0);
}

/**@ingroup tnet_socket_group
 * 	Closes a socket.
 * @param sock The socket to close.
 * @retval	Zero if succeed and nonzero error code otherwise.
 **/
static int tnet_socket_close(tnet_socket_t *sock)
{
	return tnet_sockfd_close(&(sock->fd));
}

//=================================================================================================
//	SOCKET object definition
//
static tsk_object_t* tnet_socket_ctor(tsk_object_t * self, va_list * app)
{
	tnet_socket_t *sock = self;
	if (sock){
	}
	return self;
}

static tsk_object_t* tnet_socket_dtor(tsk_object_t * self)
{
	tnet_socket_t *sock = self;

	if (sock){
		/* Close the socket */
		if (TNET_SOCKET_IS_VALID(sock)){
			tnet_socket_close(sock);
		}
		
	}

	return self;
}

static const tsk_object_def_t tnet_socket_def_s =
{
	sizeof(tnet_socket_t),
	tnet_socket_ctor,
	tnet_socket_dtor,
	tsk_null,
};
const tsk_object_def_t *tnet_socket_def_t = &tnet_socket_def_s;



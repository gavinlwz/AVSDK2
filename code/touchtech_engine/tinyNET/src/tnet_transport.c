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

/**@file tnet_transport.c
 * @brief Network transport layer.
 *
 * <h2>10.2	Tansport</h2>
 * A transport layer always has a master socket which determine what kind of network traffic we
 *expect (stream or dgram).
 * Stream transport can manage TCP, TLS and SCTP sockets. Datagram socket can only manage UDP
 *sockets. <br>
 * A transport can hold both IPv4 and IPv6 sockets.
 */
#include "tnet_transport.h"



#include "tsk_memory.h"
#include "tsk_string.h"
#include "tsk_debug.h"
#include "tsk_thread.h"
#include "tsk_buffer.h"

#include <string.h> /* memcpy, ...(<#void * #>, <#const void * #>, <#tsk_size_t #>) */

#ifndef TNET_CIPHER_LIST
#define TNET_CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"
#endif

extern int tnet_transport_prepare (tnet_transport_t *transport);
extern int tnet_transport_unprepare (tnet_transport_t *transport);
extern void *TSK_STDCALL tnet_transport_mainthread (void *param);
extern int tnet_transport_stop (tnet_transport_t *transport);

static void *TSK_STDCALL run (void *self);

static int _tnet_transport_ssl_init (tnet_transport_t *transport)
{
    if (!transport)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    return 0;
}

static int _tnet_transport_ssl_deinit (tnet_transport_t *transport)
{
    if (!transport)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    return 0;
}

tnet_transport_t *
tnet_transport_create (const char *host, tnet_port_t port, tnet_socket_type_t type, const char *description)
{
    tnet_transport_t *transport;

    if ((transport = tsk_object_new (tnet_transport_def_t)))
    {
        transport->description = tsk_strdup (description);
        transport->local_host = tsk_strdup (host);
        transport->req_local_port = port;
        transport->type = type;
        transport->context = tnet_transport_context_create ();

        if ((transport->master =
             tnet_socket_create (transport->local_host, transport->req_local_port, transport->type)))
        {
            transport->local_ip = tsk_strdup (transport->master->ip);
            transport->bind_local_port = transport->master->port;
        }
        else
        {
            TSK_DEBUG_ERROR ("Failed to create master socket");
            TSK_OBJECT_SAFE_FREE (transport);
        }

        if (_tnet_transport_ssl_init (transport) != 0)
        {
            TSK_DEBUG_ERROR ("Failed to initialize TLS and/or DTLS caps");
            TSK_OBJECT_SAFE_FREE (transport);
        }
        // set priority
        tsk_runnable_set_priority (TSK_RUNNABLE (transport), TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }

    return transport;
}

tnet_transport_t *tnet_transport_create_2 (tnet_socket_t *master, const char *description)
{
    tnet_transport_t *transport;
    if (!master)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    if ((transport = tsk_object_new (tnet_transport_def_t)))
    {
        transport->description = tsk_strdup (description);
        transport->local_host = tsk_strdup (master->ip);
        transport->req_local_port = master->port;
        transport->type = master->type;

        transport->master = tsk_object_ref (master);
        transport->local_ip = tsk_strdup (transport->master->ip);
        transport->bind_local_port = transport->master->port;

        transport->context = tnet_transport_context_create ();

        if (_tnet_transport_ssl_init (transport) != 0)
        {
            TSK_DEBUG_ERROR ("Failed to initialize TLS and/or DTLS caps");
            TSK_OBJECT_SAFE_FREE (transport);
        }

        // set priority
        tsk_runnable_set_priority (TSK_RUNNABLE (transport), TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }

    return transport;
}

tnet_transport_event_t *
tnet_transport_event_create (tnet_transport_event_type_t type, const void *callback_data, tnet_fd_t fd)
{
    return tsk_object_new (tnet_transport_event_def_t, type, callback_data, fd);
}

int tnet_transport_start(tnet_transport_handle_t *handle, const char *host,	tnet_port_t port)
{
    int ret = -1;
    if (handle)
    {
        tnet_transport_t *transport = handle;

        /* prepare transport */
        if ((ret = tnet_transport_prepare (transport)))
        {
            TSK_DEBUG_ERROR ("Failed to prepare transport.");
            goto bail;
        }
		//tcp 并且addr 不为空连接
		if (TNET_SOCKET_TYPE_IS_TCP(transport->type) && host)
		{
			tnet_transport_connectto_3(handle, transport->master, host, port, transport->type);
		}


        /* start transport */
        TSK_RUNNABLE (transport)->run = run;
        if ((ret = tsk_runnable_start (TSK_RUNNABLE (transport), tnet_transport_event_def_t)))
        {
            TSK_DEBUG_ERROR ("Failed to start transport.");
            goto bail;
        }
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
    }

bail:
    return ret;
}

int tnet_transport_issecure (const tnet_transport_handle_t *handle)
{
    if (handle)
    {
        const tnet_transport_t *transport = handle;
        if (transport->master)
        {
            return TNET_SOCKET_TYPE_IS_SECURE (transport->master->type);
        }
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
    }
    return 0;
}

const char *tnet_transport_get_description (const tnet_transport_handle_t *handle)
{
    if (handle)
    {
        const tnet_transport_t *transport = handle;
        return transport->description;
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
        return tsk_null;
    }
}

int tnet_transport_get_ip_n_port (const tnet_transport_handle_t *handle, tnet_fd_t fd, tnet_ip_t *ip, tnet_port_t *port)
{
    if (handle)
    {
        return tnet_get_ip_n_port (fd, tsk_true /*local*/, ip, port);
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
    }
    return -1;
}

int tnet_transport_get_ip_n_port_2 (const tnet_transport_handle_t *handle, tnet_ip_t *ip, tnet_port_t *port)
{
    const tnet_transport_t *transport = handle;
    if (transport)
    {
        // do not check the master, let the application die if "null"
        if (ip)
        {
            memcpy (*ip, transport->master->ip, sizeof (transport->master->ip));
        }
        if (port)
        {
            *port = transport->master->port;
        }
        return 0;
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
        return -1;
    }
}


tnet_socket_type_t tnet_transport_get_type (const tnet_transport_handle_t *handle)
{
    if (!handle)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tnet_socket_type_invalid;
    }
    return ((const tnet_transport_t *)handle)->type;
}

tnet_fd_t tnet_transport_get_master_fd (const tnet_transport_handle_t *handle)
{
    if (!handle)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return TNET_INVALID_FD;
    }
    return ((const tnet_transport_t *)handle)->master ? ((const tnet_transport_t *)handle)->master->fd :
                                                        TNET_INVALID_FD;
}

/**
 * Connects a socket.
 * @param handle The transport to use to connect() the socket. The new socket will be managed by
 * this transport.
 * @param host The remote @a host to connect() to.
 * @param port The remote @a port to connect() to.
 * @param type The type of the socket to use to connect() to the remote @a host.
 * @retval The newly connected socket. For non-blocking sockets you should use @ref
 * tnet_sockfd_waitUntilWritable to check
 * the socket for writability.
 * @sa tnet_sockfd_waitUntilWritable.
 */
tnet_fd_t tnet_transport_connectto (const tnet_transport_handle_t *handle, const char *host, tnet_port_t port, tnet_socket_type_t type)
{
    return tnet_transport_connectto_3 (handle, tsk_null /*socket*/, host, port, type);
}

tnet_fd_t tnet_transport_connectto_3 (const tnet_transport_handle_t *handle,
                                      struct tnet_socket_s *socket,
                                      const char *host,
                                      tnet_port_t port,
                                      tnet_socket_type_t type)
{
    tnet_transport_t *transport = (tnet_transport_t *)handle;
    struct sockaddr_storage to;
    int status = -1;
    tnet_fd_t fd = socket ? socket->fd : TNET_INVALID_FD;
   
    tsk_bool_t owe_socket = socket ? tsk_false : tsk_true;
    const char *to_host = host;
    tnet_port_t to_port = port;
    tnet_socket_type_t to_type = type;
  

    if (!transport || !transport->master)
    {
        TSK_DEBUG_ERROR ("Invalid transport handle");
        goto bail;
    }

    if ((TNET_SOCKET_TYPE_IS_STREAM (transport->master->type) && !TNET_SOCKET_TYPE_IS_STREAM (type)) ||
        (TNET_SOCKET_TYPE_IS_DGRAM (transport->master->type) && !TNET_SOCKET_TYPE_IS_DGRAM (type)))
    {
        TSK_DEBUG_ERROR ("Master/destination types mismatch [%u/%u]", transport->master->type, type);
        goto bail;
    }

    
    /* Init destination sockaddr fields */
    if ((status = tnet_sockaddr_init (to_host, to_port, to_type, &to)))
    {
        TSK_DEBUG_ERROR ("Invalid HOST/PORT [%s/%u]", host, port);
        goto bail;
    }
    if (TNET_SOCKET_TYPE_IS_IPV46 (type))
    {
        /* Update the type (unambiguously) */
        if (to.ss_family == AF_INET6)
        {
            TNET_SOCKET_TYPE_SET_IPV6Only (type);
        }
        else
        {
            TNET_SOCKET_TYPE_SET_IPV4Only (type);
        }
    }

    /*
     * STREAM ==> create new socket and connect it to the remote host.
     * DGRAM ==> connect the master to the remote host.
     */
    if (fd == TNET_INVALID_FD)
    {
        // Create client socket descriptor.
        if ((status = tnet_sockfd_init (transport->local_host, TNET_SOCKET_PORT_ANY, to_type, &fd)))
        {
            TSK_DEBUG_ERROR ("Failed to create new sockfd.");
            goto bail;
        }
    }

    if ((status = tnet_sockfd_connectto (fd, (const struct sockaddr_storage *)&to)))
    {
        if (fd != transport->master->fd)
        {
            tnet_sockfd_close (&fd);
        }
        goto bail;
    }
    else
    {
        static const tsk_bool_t __isClient = tsk_true;
        /* Add the socket */
        // socket must be added after connect() otherwise many Linux systems will return POLLHUP as
        // the fd is not active yet
        if ((status = tnet_transport_add_socket_2 (handle, fd, to_type, owe_socket, __isClient,
                                                    host, port)))
        {
            TNET_PRINT_LAST_ERROR ("Failed to add new socket");
            tnet_sockfd_close (&fd);
            goto bail;
        }
    }

bail:
    return status == 0 ? fd : TNET_INVALID_FD;
}


tsk_size_t tnet_transport_send_wrapper(const tnet_transport_handle_t *handle, tnet_fd_t from, const void* buf, tsk_size_t size)
{
	tnet_transport_t *transport = (tnet_transport_t *)handle;
	//加上2字节的长度
	short netLen = htons(size);
	//发送长度
	if (tnet_transport_send(handle, transport->master->fd, &netLen, sizeof(netLen)) != sizeof(netLen))
	{
		return -1;
	}
	//发送数据
	return tnet_transport_send(handle, transport->master->fd, buf, size);
}

int tnet_transport_set_callback (const tnet_transport_handle_t *handle, tnet_transport_cb_f callback, const void *callback_data)
{
    tnet_transport_t *transport = (tnet_transport_t *)handle;

    if (!transport)
    {
        TSK_DEBUG_ERROR ("Invalid server handle.");
        return -1;
    }

    transport->callback = callback;
    transport->callback_data = callback_data;
    return 0;
}

int tnet_transport_shutdown (tnet_transport_handle_t *handle)
{
    if (handle)
    {
        int ret;
        if ((ret = tnet_transport_stop (handle)) == 0)
        {
            ret = tnet_transport_unprepare (handle);
        }
        return ret;
    }
    else
    {
        TSK_DEBUG_ERROR ("NULL transport object.");
        return -1;
    }
}



/*
 * Runnable interface implementation.
 */
static void *TSK_STDCALL run (void *self)
{
    int ret = 0;
    tsk_list_item_t *curr;
    tnet_transport_t *transport = self;

    TSK_DEBUG_INFO ("Transport::run(%s) - enter", transport->description);

    /* create main thread */
    if ((ret = tsk_thread_create (transport->mainThreadId, tnet_transport_mainthread, transport)))
    { /* More important than "tsk_runnable_start" ==> start it first. */
        TSK_FREE (transport->context); /* Otherwise (tsk_thread_create is ok) will be freed when
                                          mainthread exit. */
        TSK_DEBUG_FATAL ("Failed to create main thread [%d]", ret);
        return tsk_null;
    }
/* set thread priority
 iOS and OSX: no incoming pkts (STUN, rtp, dtls...) when thread priority is changed -> to be checked
 */
#if !TNET_UNDER_APPLE
    ret = tsk_thread_set_priority (transport->mainThreadId[0], TSK_THREAD_PRIORITY_TIME_CRITICAL);
#endif

    TSK_RUNNABLE_RUN_BEGIN (transport);

    if ((curr = TSK_RUNNABLE_POP_FIRST_SAFE (TSK_RUNNABLE (transport))))
    {
        const tnet_transport_event_t *e = (const tnet_transport_event_t *)curr->data;

        if (transport->callback)
        {
            transport->callback (e);
        }
        tsk_object_unref (curr);
    }

    TSK_RUNNABLE_RUN_END (transport);

    TSK_DEBUG_INFO ("Transport::run(%s) - exit", transport->description);

    return tsk_null;
}


//=================================================================================================
//	Transport object definition
//
static tsk_object_t *tnet_transport_ctor (tsk_object_t *self, va_list *app)
{
    tnet_transport_t *transport = self;
    if (transport)
    {
    }
    return self;
}

static tsk_object_t *tnet_transport_dtor (tsk_object_t *self)
{
    tnet_transport_t *transport = self;
    if (transport)
    {
        tnet_transport_set_callback (transport, tsk_null, tsk_null);
        tnet_transport_shutdown (transport);
        TSK_OBJECT_SAFE_FREE (transport->master);
        TSK_OBJECT_SAFE_FREE (transport->context);
        TSK_OBJECT_SAFE_FREE (transport->natt_ctx);
        TSK_FREE (transport->local_ip);
        TSK_FREE (transport->local_host);

       
        _tnet_transport_ssl_deinit (transport); // openssl contexts

        TSK_DEBUG_INFO ("*** Transport (%s) destroyed ***", transport->description);
        TSK_FREE (transport->description);
    }

    return self;
}

static const tsk_object_def_t tnet_transport_def_s = {
    sizeof (tnet_transport_t),
    tnet_transport_ctor,
    tnet_transport_dtor,
    tsk_null,
};
const tsk_object_def_t *tnet_transport_def_t = &tnet_transport_def_s;


//=================================================================================================
//	Transport event object definition
//
static tsk_object_t *tnet_transport_event_ctor (tsk_object_t *self, va_list *app)
{
    tnet_transport_event_t *e = self;
    if (e)
    {
        e->type = va_arg (*app, tnet_transport_event_type_t);
        e->callback_data = va_arg (*app, const void *);
        e->local_fd = va_arg (*app, tnet_fd_t);
    }
    return self;
}

static tsk_object_t *tnet_transport_event_dtor (tsk_object_t *self)
{
    tnet_transport_event_t *e = self;
    if (e)
    {
        TSK_FREE (e->data);
    }

    return self;
}

static const tsk_object_def_t tnet_transport_event_def_s = {
    sizeof (tnet_transport_event_t),
    tnet_transport_event_ctor,
    tnet_transport_event_dtor,
    0,
};
const tsk_object_def_t *tnet_transport_event_def_t = &tnet_transport_event_def_s;

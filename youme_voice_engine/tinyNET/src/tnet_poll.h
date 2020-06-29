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

/**@file tnet_poll.h
 * @brief poll() method implementation for multiplexing network sockets.
 *
 *
 *

 */
#ifndef TNET_POLL_H
#define TNET_POLL_H

#include "tinynet_config.h"

TNET_BEGIN_DECLS

#if USE_POLL

#include "tnet_types.h"

typedef unsigned long tnet_nfds_t;

#if HAVE_POLL

typedef struct pollfd tnet_pollfd_t;

#define TNET_POLLRDNORM  POLLRDNORM 
#define TNET_POLLRDBAND  POLLRDBAND
#define TNET_POLLIN      POLLIN
#define TNET_POLLPRI     POLLPRI

#define TNET_POLLWRNORM  POLLWRNORM
#define TNET_POLLOUT     POLLOUT
#define TNET_POLLWRBAND  POLLWRBAND

#define TNET_POLLERR     POLLERR
#define TNET_POLLHUP     POLLHUP
#define TNET_POLLNVAL    POLLNVAL

#if TNET_UNDER_WINDOWS
#	define tnet_poll	WSAPoll
#else
#	define tnet_poll	poll
#endif /* TNET_UNDER_WINDOWS */

#else

typedef struct tnet_pollfd_s
{
    tnet_fd_t  fd;
    short   events;
    short   revents;
}
tnet_pollfd_t;

#define TNET_POLLIN      0x0001    /* There is data to read */
#define TNET_POLLPRI     0x0002    /* There is urgent data to read */
#define TNET_POLLOUT     0x0004    /* Writing now will not block */
#define TNET_POLLERR     0x0008    /* Error condition */
#define TNET_POLLHUP     0x0010    /* Hung up */
#define TNET_POLLNVAL    0x0020    /* Invalid request: fd not open */

int tnet_poll(tnet_pollfd_t fds[ ], tnet_nfds_t nfds, int timeout);

#endif /* HAVE_POLL */

#endif /* TNET_USE_POLL */

TNET_END_DECLS

#endif /* TNET_POLL_H */


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
#include "tnet_poll.h"

#if USE_POLL && (!HAVE_POLL || !HAVE_POLL_H)

/**
 *
 * @brief	poll() method implementation for multiplexing network sockets. 
 *
 * @param	fds		An array of pollfd structures. 
 * @param	nfds	The number of file descriptors set in fds[ ]. 
 * @param	timeout	How long poll() should wait for an event to occur (in milliseconds). 
 *
 * @return	The number of elements in fdarray for which an revents member of the POLLFD structure is nonzero.
 *			If the the returned value if <0 this mean that error occurred.
**/

int tnet_poll(tnet_pollfd_t fds[ ], tnet_nfds_t nfds, int timeout)
{
	tsk_size_t i;
	int ret;
	int highest_fd = -1;

	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	struct timeval timetowait;

	/*
	* cleanup fd_sets
	*/
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	/*
	*	set timeout
	*/
	if(timeout >=0){
		timetowait.tv_sec = (timeout/1000);
		timetowait.tv_usec = (timeout%1000) * 1000;
	}

	/*
	* add descriptors to the fd_sets
	*/
	for(i = 0; i<nfds; i++){
		if(fds[i].fd != TNET_INVALID_FD){
			if(fds[i].events & TNET_POLLIN){
				FD_SET(fds[i].fd, &readfds);
			}
			if(fds[i].events & TNET_POLLOUT){
				FD_SET(fds[i].fd, &writefds);
			}
			if(fds[i].events & TNET_POLLPRI){
				FD_SET(fds[i].fd, &exceptfds);
			}
		}
		
		highest_fd = (highest_fd < fds[i].fd)  ? fds[i].fd : highest_fd;
	}
	
	/*======================================
	* select
	*/
	if((ret = select(highest_fd + 1, &readfds, &writefds, &exceptfds, (timeout >=0) ? &timetowait : 0)) >= 0){
		for(i = 0; i<nfds; i++){
			if(fds[i].fd != TNET_INVALID_FD){
				fds[i].revents = 0;

				if(FD_ISSET(fds[i].fd, &readfds)){
					fds[i].revents |= TNET_POLLIN;
				}
				if(FD_ISSET(fds[i].fd, &writefds)){
					fds[i].revents |= TNET_POLLOUT;
				}
				if(FD_ISSET(fds[i].fd, &exceptfds)){
					fds[i].revents |= TNET_POLLPRI;
				}
			}
		}
	}

	return ret;
}

#endif /* TNET_USE_POLL */



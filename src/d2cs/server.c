/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "common/setup_before.h"
#include "setup.h"

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/memset.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/psock.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"

#include "d2gs.h"
#include "net.h"
#include "s2s.h"
#include "gamequeue.h"
#include "game.h"
#include "connection.h"
#include "serverqueue.h"
#include "server.h"
#include "prefs.h"
#include "d2ladder.h"
#include "handle_signal.h"
#include "common/addr.h"
#include "common/list.h"
#include "common/hashtable.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static int server_purge_list(void);
static int server_listen(void);
static int server_accept(int sock);
static int server_handle_timed_event(void);
static int server_handle_socket(void);
static int server_loop(void);
static int server_cleanup(void);

t_addrlist		* server_listen_addrs;

static int server_listen(void)
{
	t_addr		* curr_laddr;
	t_addr_data	laddr_data;
	int		sock;

	if (!(server_listen_addrs=addrlist_create(prefs_get_servaddrs(),INADDR_ANY,D2CS_SERVER_PORT))) {
		log_error("error create listening address list");
		return -1;
	}
	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=net_listen(addr_get_ip(curr_laddr),addr_get_port(curr_laddr),PSOCK_SOCK_STREAM);
		if (sock<0) {
			log_error("error listen socket");
			return -1;
		} 
		log_info("listen on %s", addr_num_to_addr_str(addr_get_ip(curr_laddr),addr_get_port(curr_laddr)));
		if (psock_ctl(sock,PSOCK_NONBLOCK)<0) {
			log_error("error set listen socket in non-blocking mode");
		}
		laddr_data.p=(void *)sock;
		addr_set_data(curr_laddr,laddr_data);
	}
	END_LIST_TRAVERSE_DATA()
	return 0;
}

static int server_accept(int sock)
{
	int			csock;
	struct sockaddr_in	caddr, raddr;
	int			caddr_len, raddr_len, val;
	unsigned int		ip;
	unsigned short		port;

	caddr_len=sizeof(caddr);
	memset(&caddr,0,sizeof(caddr));
	csock=psock_accept(sock,(struct sockaddr *)&caddr,&caddr_len);
	if (csock<0) {
		log_error("error accept new connection");
		return -1;
	} else log_info("accept connection from %s",inet_ntoa(caddr.sin_addr));

	val=1;
	if (psock_setsockopt(csock, PSOCK_SOL_SOCKET, PSOCK_SO_KEEPALIVE, &val,sizeof(val))<0) {
		log_warn("error set sock option keep alive");
	}
	if (psock_ctl(csock, PSOCK_NONBLOCK)<0) {
		log_error("error set socket to non-blocking mode");
		psock_close(csock);
		return -1;
	}

	raddr_len=sizeof(raddr);
	memset(&raddr,0,sizeof(raddr));
	ip=port=0;
	if (psock_getsockname(csock,(struct sockaddr *)&raddr, &raddr_len)<0) {
		log_warn("unable to get local socket info");
	} else {
		if (raddr.sin_family!=PSOCK_AF_INET) {
			log_warn("got bad socket family %d",raddr.sin_family);
		} else {
			ip=ntohl(raddr.sin_addr.s_addr);
			port=ntohs(raddr.sin_port);
		}
	}
	if (!conn_create(csock,ip,port,ntohl(caddr.sin_addr.s_addr),ntohs(caddr.sin_port))) {
		log_error("error create new connection");
		psock_close(csock);
		return -1;
	}
	return 0;
}

static int server_purge_list(void)
{
	hashtable_purge(connlist());
	list_purge(gamelist());
	list_purge(d2gslist());
	list_purge(sqlist());
	list_purge(gqlist());
	return 0;
}

static int server_handle_timed_event(void)
{
	static  time_t	prev_list_purgetime=0;
	static  time_t	prev_gamequeue_checktime=0;
	static	time_t	prev_s2s_checktime=0;
	static	time_t	prev_sq_checktime=0;
	static  time_t	prev_d2ladder_refresh_time=0;
	static  time_t	prev_s2s_keepalive_time=0;
	static  time_t	prev_timeout_checktime;
	time_t		now;

	now=time(NULL);
	if (now-prev_list_purgetime>prefs_get_list_purgeinterval()) {
		server_purge_list();
		gamelist_check_voidgame();
		prev_list_purgetime=now;
	}
	if (now-prev_gamequeue_checktime>prefs_get_gamequeue_checkinterval()) {
		gqlist_update_all_clients();
		prev_gamequeue_checktime=now;
	}
	if (now-prev_s2s_checktime>prefs_get_s2s_retryinterval()) {
		s2s_check();
		prev_s2s_checktime=now;
	}
	if (now-prev_sq_checktime>prefs_get_sq_checkinterval()) {
		sqlist_check_timeout();
		prev_sq_checktime=now;
	}
	if (now-prev_d2ladder_refresh_time>prefs_get_d2ladder_refresh_interval()) {
		d2ladder_refresh();
		prev_d2ladder_refresh_time=now;
	}
	if (now-prev_s2s_keepalive_time>prefs_get_s2s_keepalive_interval()) {
		d2gs_keepalive();
		prev_s2s_keepalive_time=now;
	}
	if (now-prev_timeout_checktime>prefs_get_timeout_checkinterval()) {
		connlist_check_timeout();
		prev_timeout_checktime=now;
	}
	return 0;
}

static int server_handle_socket(void)
{
	t_psock_fd_set	rfds, wfds;
	int		highest_fd, sock;
	t_addr		* curr_laddr;
	t_connection	* c;
	struct timeval	tv;


	PSOCK_FD_ZERO(&rfds);
	PSOCK_FD_ZERO(&wfds);
	highest_fd=-1;

	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=(int)addr_get_data(curr_laddr).p;
		PSOCK_FD_SET(sock,&rfds);
		if (sock>highest_fd) highest_fd=sock;
	}
	END_LIST_TRAVERSE_DATA()

	BEGIN_HASHTABLE_TRAVERSE_DATA(connlist(),c)
	{
		if (conn_get_state(c)==conn_state_destroy) {
			conn_destroy(c);
			continue;
		}
		sock=conn_get_socket(c);
		if (queue_get_length((t_queue const * const *)conn_get_out_queue(c))>0) {
			PSOCK_FD_SET(sock,&wfds);
		}
		if (conn_get_state(c)==conn_state_connecting) {
			PSOCK_FD_SET(sock,&wfds);
		} else {
			PSOCK_FD_SET(sock,&rfds);
		}
		if (sock>highest_fd) highest_fd=sock;
	}
	END_HASHTABLE_TRAVERSE_DATA()

	tv.tv_sec=0;
	tv.tv_usec=BNETD_POLL_INTERVAL * 1000;
	switch (psock_select(highest_fd+1,&rfds, &wfds, NULL, &tv)) {
		case -1:
			log_error("failed to select (%s)",strerror(psock_errno()));
			return -1;
		case 0:
			return 0;
		default:
			break;
	}

	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=(int)addr_get_data(curr_laddr).p;
		if (PSOCK_FD_ISSET(sock,&rfds)) {
			server_accept(sock);
		}
	}
	END_LIST_TRAVERSE_DATA()

	BEGIN_HASHTABLE_TRAVERSE_DATA(connlist(),c)
	{
		sock=conn_get_socket(c);
		if (PSOCK_FD_ISSET(sock,&rfds)) {
			conn_add_socket_flag(c,SOCKET_FLAG_READ);
		}
		if (PSOCK_FD_ISSET(sock,&wfds)) {
			conn_add_socket_flag(c,SOCKET_FLAG_WRITE);
		}
		if (conn_handle_socket(c)<0) {
			conn_destroy(c);
		}
		
	}
	END_HASHTABLE_TRAVERSE_DATA()
	return 0;
}

static int server_loop(void)
{
	unsigned int count;

	count=0;
	while (1) {
		if (handle_signal()<0) break;
		if (++count>=(1000/BNETD_POLL_INTERVAL)) {
			server_handle_timed_event();
			count=0;
		}
		server_handle_socket();
	}
	return 0;
}

static int server_cleanup(void)
{
	t_addr		* curr_laddr;
	int		sock;

	BEGIN_LIST_TRAVERSE_DATA(server_listen_addrs,curr_laddr)
	{
		sock=(int)addr_get_data(curr_laddr).p;
		psock_close(sock);
	}
	END_LIST_TRAVERSE_DATA()
	addrlist_destroy(server_listen_addrs);
	return 0;
}

extern int server_process(void)
{
	handle_signal_init();
	if (psock_init()<0) {
		log_error("failed to init network");
		return -1;
	}
	log_info("network initialized");
	if (s2s_init()<0) {
		log_error("failed to init s2s connection");
		return -1;
	}
	if (server_listen()<0) {
		log_error("failed to setup listen socket");
		return -1;
	}
	log_info("entering server loop");
	server_loop();
	log_info("exit from server loop,cleanup");
	server_cleanup();
	return 0;
}

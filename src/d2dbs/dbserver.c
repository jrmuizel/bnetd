/*
 * Copyright (C) 2001		sousou	(liupeng.cs@263.net)
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

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
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

#include "dbserver.h"
#include "charlock.h"
#include "d2ladder.h"
#include "dbspacket.h"
#include "prefs.h"
#include "handle_signal.h"
#include "common/list.h"
#include "common/eventlog.h"
#include "common/setup_after.h"


static int		dbs_packet_gs_id = 0;
static t_preset_d2gsid	*preset_d2gsid_head = NULL;
t_list * dbs_server_connection_list = NULL;
SOCKET dbs_server_listen_socket ;

/* dbs_server_main
 * The module's driver function -- we just call other functions and
 * interpret their results.
 */

static int dbs_handle_timed_events(void);
static void dbs_on_exit(void);

SOCKET dbs_server_init(const char* pcAddress, unsigned short  nPort);
void dbs_server_loop(SOCKET ListeningSocket);
int dbs_server_setup_fdsets(fd_set * pReadFDs, fd_set * pWriteFDs, 
        fd_set * pExceptFDs, SOCKET ListeningSocket ) ;
BOOL dbs_server_read_data(t_d2dbs_connection* conn) ;
BOOL dbs_server_write_data(t_d2dbs_connection* conn) ;
int dbs_server_list_add_socket(SOCKET sd, unsigned int ipaddr);
static int setsockopt_keepalive(SOCKET sock);
static unsigned int get_preset_d2gsid(unsigned int ipaddr);

int dbs_server_main(const char* pcAddress, unsigned short nPort)
{
	log_info("establishing the listener...");
	dbs_server_listen_socket = dbs_server_init(pcAddress, htons(nPort));
	if (dbs_server_listen_socket == INVALID_SOCKET) {
		log_error("dbs_server_init error ");
		return 3;
	}
	log_info("waiting for connections...");
	dbs_server_loop(dbs_server_listen_socket);
	dbs_on_exit();
	return 0;
}

/* dbs_server_init
 * Sets up a listener on the given interface and port, returning the
 * listening socket if successful; if not, returns INVALID_SOCKET.
 */
SOCKET dbs_server_init(const char* pcAddress, unsigned short  nPort)
{
	SOCKET sd;
	struct sockaddr_in sinInterface;
	int val;

	if (! (dbs_server_connection_list=list_create()))
	{
		log_error("list_create() failed");
		return INVALID_SOCKET;
	}

	if(d2ladder_init()==-1)
	{
		log_error("d2ladder_init() failed");
		return INVALID_SOCKET;
	}

	if(cl_init(DEFAULT_HASHTBL_LEN, DEFAULT_GS_MAX)==-1)
	{
		log_error("cl_init() failed");
		return INVALID_SOCKET;
	}

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == INVALID_SOCKET) 
	{
		log_error("socket() failed : %s",strerror(errno));
		return INVALID_SOCKET;
	}

	val = 1;
	if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == SOCKET_ERROR)
	{
		log_error("setsockopt() failed : %s",strerror(errno));
	}

	sinInterface.sin_family = AF_INET;
	sinInterface.sin_addr.s_addr = INADDR_ANY;
	sinInterface.sin_port = nPort;
	if (bind(sd, (struct sockaddr*)&sinInterface, sizeof(struct sockaddr_in)) == SOCKET_ERROR) 
	{
		log_error("bind() failed : %s",strerror(errno));
		return INVALID_SOCKET;
	}
	if(listen(sd, SOMAXCONN) == SOCKET_ERROR)
	{
		log_error("listen() failed");
		return INVALID_SOCKET;
	}

	return sd;
}


/* dbs_server_setup_fdsets
 * Set up the three FD sets used with select() with the sockets in the
 * connection list.  Also add one for the listener socket, if we have
 * one.
 */

int dbs_server_setup_fdsets(fd_set * pReadFDs, fd_set * pWriteFDs, fd_set * pExceptFDs, SOCKET lsocket ) 
{
	t_elem const * elem;
	t_d2dbs_connection* it;

	int highest_fd;
	FD_ZERO(pReadFDs);
	FD_ZERO(pWriteFDs);
	FD_ZERO(pExceptFDs);
	/* Add the listener socket to the read and except FD sets, if there is one. */
	if (lsocket != INVALID_SOCKET) {
		FD_SET(lsocket, pReadFDs);
		FD_SET(lsocket, pExceptFDs);
	}
	highest_fd=lsocket;

	LIST_TRAVERSE_CONST(dbs_server_connection_list,elem)
	{		
		if (!(it=elem_get_data(elem))) continue;
		if (it->nCharsInReadBuffer < (kBufferSize-kMaxPacketLength)) {
			/* There's space in the read buffer, so pay attention to incoming data. */
			FD_SET(it->sd, pReadFDs);
		}
		if (it->nCharsInWriteBuffer > 0) {
			FD_SET(it->sd, pWriteFDs);
		}
		FD_SET(it->sd, pExceptFDs);
		if(highest_fd < it->sd) highest_fd=it->sd;
	}
	return highest_fd;
}

/* dbs_server_read_data
 * Data came in on a client socket, so read it into the buffer.  Returns
 * false on failure, or when the client closes its half of the
 * connection.  (WSAEWOULDBLOCK doesn't count as a failure.)
 */
BOOL dbs_server_read_data(t_d2dbs_connection* conn) 
{
	int nBytes;

	nBytes = recv(conn->sd, conn->ReadBuf + conn->nCharsInReadBuffer, \
		kBufferSize - conn->nCharsInReadBuffer, 0);
	if (nBytes == 0) {
		log_info("Socket %d was closed by the client. Shutting down.",conn->sd);
		return FALSE;
	} else if (nBytes == SOCKET_ERROR) {
		int err;
		int errlen = sizeof(err);
		getsockopt(conn->sd, SOL_SOCKET, SO_ERROR, (char*)&err, &errlen);
		if(err == WSAEWOULDBLOCK) {
			return TRUE;
		} else {
			log_error("recv() failed : %s",strerror(err));
			return FALSE;
		}
	}
	conn->nCharsInReadBuffer += nBytes;
	return TRUE;
}


/* dbs_server_write_data
 * The connection is writable, so send any pending data.  Returns
 * false on failure.  (WSAEWOULDBLOCK doesn't count as a failure.)
 */
BOOL dbs_server_write_data(t_d2dbs_connection* conn) 
{
	unsigned int sendlen;
	int nBytes ;

	if(conn->nCharsInWriteBuffer>kMaxPacketLength) sendlen=kMaxPacketLength;
	else sendlen=conn->nCharsInWriteBuffer;
	nBytes = send(conn->sd, conn->WriteBuf, sendlen, 0);
	if (nBytes == SOCKET_ERROR) {
	        int err;
		int errlen = sizeof(err);
		getsockopt(conn->sd, SOL_SOCKET, SO_ERROR, (char*)&err, &errlen);
		if(err == WSAEWOULDBLOCK) {
			return TRUE;
		} else {
			log_error("send() failed : %s",strerror(err));
			return FALSE;
		}
	}
	if (nBytes == conn->nCharsInWriteBuffer) {
		conn->nCharsInWriteBuffer = 0;
	} else {
		conn->nCharsInWriteBuffer -= nBytes;
		memmove(conn->WriteBuf, conn->WriteBuf + nBytes, conn->nCharsInWriteBuffer);
	}
	return TRUE;
}

int dbs_server_list_add_socket(SOCKET sd, unsigned int ipaddr)
{
	t_d2dbs_connection	*it;
	struct in_addr		in;

	if (!(it=malloc(sizeof(t_d2dbs_connection)))) {
		log_error("malloc() failed");
		return 0;
	}
	memset(it, 0, sizeof(t_d2dbs_connection));
	it->sd=sd;
	it->ipaddr=ipaddr;
	it->major=0;
	it->minor=0;
	it->type=0;
	it->stats=0;
	it->verified=0;
	it->serverid=get_preset_d2gsid(ipaddr);
	it->last_active=time(NULL);
	it->nCharsInReadBuffer=0;
	it->nCharsInWriteBuffer=0;
	list_append_data(dbs_server_connection_list,it);
	in.s_addr = htonl(ipaddr);
	strncpy(it->serverip, inet_ntoa(in), sizeof(it->serverip)-1);

	return 1;
}

static int dbs_handle_timed_events(void)
{
	static	time_t		prev_ladder_save_time=0;
	static	time_t		prev_keepalive_save_time=0;
	static  time_t		prev_timeout_checktime=0;
	time_t			now;

	now=time(NULL);
	if (now-prev_ladder_save_time>prefs_get_laddersave_interval()) {
		d2ladder_saveladder();
		prev_ladder_save_time=now;
	}
	if (now-prev_keepalive_save_time>prefs_get_keepalive_interval()) {
		dbs_keepalive();
		prev_keepalive_save_time=now;
	}
	if (now-prev_timeout_checktime>prefs_get_timeout_checkinterval()) {
		dbs_check_timeout();
		prev_timeout_checktime=now;
	}
	return 0;
}

void dbs_server_loop(SOCKET lsocket)
{
	struct sockaddr_in sinRemote;
	SOCKET sd ;
	fd_set ReadFDs, WriteFDs, ExceptFDs;
	t_elem * elem;
	t_d2dbs_connection* it;
	BOOL bOK ;
	const char* pcErrorType ;
	struct timeval         tv;
	int highest_fd;
	int listpurgecount;
	int nAddrSize = sizeof(sinRemote);
	
	listpurgecount=0;
	while (1) {
		if (handle_signal()<0) break;
		dbs_handle_timed_events();
		highest_fd=dbs_server_setup_fdsets(&ReadFDs, &WriteFDs, &ExceptFDs, lsocket);

		tv.tv_sec  = 0;
		tv.tv_usec = SELECT_TIME_OUT;
		switch (select(highest_fd+1, &ReadFDs, &WriteFDs, &ExceptFDs, &tv) ) {
			case -1:
				log_error("select() failed : %s",strerror(errno));
				continue;
			case 0:
				continue;
			default:
				break;
		}

		if (FD_ISSET(lsocket, &ReadFDs)) {
			sd = accept(lsocket, (struct sockaddr*)&sinRemote, &nAddrSize);
			if (sd == INVALID_SOCKET) {
				log_error("accept() failed : %s",strerror(errno));
				return;
			}
			
			log_info("accepted connection from %s:%d , socket %d .",
				inet_ntoa(sinRemote.sin_addr) , ntohs(sinRemote.sin_port), sd);
			log_d2gs("accepted connection from %s:%d , socket %d .",
				inet_ntoa(sinRemote.sin_addr) , ntohs(sinRemote.sin_port), sd);
			setsockopt_keepalive(sd);
			dbs_server_list_add_socket(sd, ntohl(sinRemote.sin_addr.s_addr));
			if (psock_ctl(sd,O_NONBLOCK)<0) {
				log_error("could not set TCP socket [%d] to non-blocking mode (closing connection) (psock_ctl: %s)", sd,strerror(psock_errno()));
				close(sd);
			}
		} else if (FD_ISSET(lsocket, &ExceptFDs)) {
			int err;
			int errlen = sizeof(err);
			getsockopt(lsocket, SOL_SOCKET, SO_ERROR,
				(char*)&err, &errlen);
			log_error("exception on listening socket: %s",strerror(err));
			return;
		}
		
		LIST_TRAVERSE(dbs_server_connection_list,elem)
		{
			bOK = TRUE;
			pcErrorType = 0;
			
			if (!(it=elem_get_data(elem))) continue;
			if (FD_ISSET(it->sd, &ExceptFDs)) {
				bOK = FALSE;
				pcErrorType = "General socket error";
				FD_CLR(it->sd, &ExceptFDs);
			} else {
				
				if (FD_ISSET(it->sd, &ReadFDs)) {
					bOK = dbs_server_read_data(it);
					pcErrorType = "Read error";
					FD_CLR(it->sd, &ReadFDs);
				}
				
				if (FD_ISSET(it->sd, &WriteFDs)) {
					bOK = dbs_server_write_data(it);
					pcErrorType = "Write error";
					FD_CLR(it->sd, &WriteFDs);
				}
			}
			
			if (!bOK) {
				int err;
				int errlen = sizeof(err);
				getsockopt(it->sd, SOL_SOCKET, SO_ERROR, (char*)&err, &errlen);
				if (err != NO_ERROR) {
					log_error("data socket error : %s",strerror(err));
				}
				dbs_server_shutdown_connection(it);
				list_remove_elem(dbs_server_connection_list,elem);
				listpurgecount++;
			} else {
				if(dbs_packet_handle(it)==-1) {
					log_error("dbs_packet_handle() failed");
					dbs_server_shutdown_connection(it);
					list_remove_elem(dbs_server_connection_list,elem);
					listpurgecount++;
				}   
			}
		}
		if(listpurgecount>100) {
			list_purge(dbs_server_connection_list);
			listpurgecount=0;
		}
	}
}

static void dbs_on_exit(void)
{
	t_elem * elem;
	t_d2dbs_connection* it;

	if(dbs_server_listen_socket>0) close(dbs_server_listen_socket);
	dbs_server_listen_socket=INVALID_SOCKET;

	LIST_TRAVERSE(dbs_server_connection_list,elem)
	{
		if (!(it=elem_get_data(elem))) continue;
		dbs_server_shutdown_connection(it);
		list_remove_elem(dbs_server_connection_list,elem);
	}
	cl_destroy();
	d2ladder_destroy();
	list_destroy(dbs_server_connection_list);
	log_info("dbserver stopped");
}

int dbs_server_shutdown_connection(t_d2dbs_connection* conn)
{
	shutdown(conn->sd, PSOCK_SHUT_RDWR) ;
	close(conn->sd);
	if (conn->verified && conn->type==CONNECT_CLASS_D2GS_TO_D2DBS) {
		log_info("unlock all characters on gs %s(%d)",conn->serverip,conn->serverid);
		log_d2gs("unlock all characters on gs %s(%d)",conn->serverip,conn->serverid);
		log_d2gs("close connection to gs on socket %d", conn->sd);
		cl_unlock_all_char_by_gsid(conn->serverid);
	}
	free(conn);
	return 1;
}

static int setsockopt_keepalive(SOCKET sock)
{
	int		optval;
	socklen_t	optlen;

	optval = 1;
	optlen = sizeof(optval);
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void*)&optval, optlen)) {
		log_info("failed set KEEPALIVE for socket %d, errno=%d", sock, errno);
		return -1;
	} else {
		log_info("set KEEPALIVE option for socket %d", sock);
		return 0;
	}
}

static unsigned int get_preset_d2gsid(unsigned int ipaddr)
{
	t_preset_d2gsid		*pgsid;

	pgsid = preset_d2gsid_head;
	while(pgsid)
	{
		if (pgsid->ipaddr == ipaddr)
			return pgsid->d2gsid;
		pgsid = pgsid->next;
	}
	/* not found, build a new item */
	pgsid = malloc(sizeof(t_preset_d2gsid));
	if (!pgsid) {
		log_warn("failed malloc memory for t_preset_d2gsid");
		return ++dbs_packet_gs_id;
	}
	pgsid->ipaddr = ipaddr;
	pgsid->d2gsid = ++dbs_packet_gs_id;
	/* add to list */
	pgsid->next = preset_d2gsid_head;
	preset_d2gsid_head = pgsid;
	return preset_d2gsid_head->d2gsid;
}

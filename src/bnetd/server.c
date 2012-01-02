/*
 * Copyright (C) 1998,1999  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
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
#define SERVER_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
#ifdef WIN32
# include <conio.h> /* for kbhit() and getch() */
#endif
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
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
#ifdef HAVE_POLL_H
# include <poll.h>
#else
# ifdef HAVE_SYS_POLL_H
#  include <sys/poll.h>
# endif
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "compat/difftime.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef DO_POSIXSIG
# include <signal.h>
# include "compat/signal.h"
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
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
#include "common/packet.h"
#include "connection.h"
#include "common/hexdump.h"
#include "common/eventlog.h"
#include "message.h"
#include "common/queue.h"
#include "handle_auth.h"
#include "handle_bits.h"
#include "handle_bnet.h"
#include "handle_bot.h"
#include "handle_telnet.h"
#include "handle_file.h"
#include "handle_init.h"
#include "handle_d2cs.h"
#include "handle_irc.h"
#include "handle_udp.h"
#include "common/network.h"
#include "prefs.h"
#include "account.h"
#include "common/tracker.h"
#include "common/list.h"
#include "adbanner.h"
#include "timer.h"
#include "common/addr.h"
#include "common/util.h"
#ifdef WITH_BITS
# include "bits.h"
# include "bits_query.h"
# include "bits_ext.h"
# include "bits_motd.h"
#endif
#include "ipban.h"
#include "helpfile.h"
#include "gametrans.h"
#include "query.h"
#include "autoupdate.h"
#include "versioncheck.h"
#include "realm.h"
#include "channel.h"
#include "game.h"
#include "server.h"
#include "common/setup_after.h"


extern FILE * hexstrm; /* from main.c */


static void server_quit_wraper(void);

#ifdef DO_POSIXSIG
static void quit_sig_handle(int unused);
static void restart_sig_handle(int unused);
static void save_sig_handle(int unused);
#endif
#ifdef USE_CHECK_ALLOC
static void memstat_sig_handle(int unused);
#endif


static time_t starttime;
static volatile time_t sigexittime=0;
static volatile int do_restart=0;
static volatile int do_save=0;
static volatile int got_epipe=0;
#ifdef USE_CHECK_ALLOC
static volatile int do_report_usage=0;
#endif


extern void server_quit_delay(unsigned int delay)
{
    if (delay)
	sigexittime = time(NULL)+(time_t)delay;
    else
	sigexittime = 0;
}


static void server_quit_wraper(void)
{
    if (sigexittime)
	sigexittime -= prefs_get_shutdown_decr();
    else
	sigexittime = time(NULL)+(time_t)prefs_get_shutdown_delay();
}


#ifdef DO_POSIXSIG
static void quit_sig_handle(int unused)
{
    server_quit_wraper();
}


static void restart_sig_handle(int unused)
{
    do_restart = 1;
}


static void save_sig_handle(int unused)
{
    do_save = 1;
}


static void pipe_sig_handle(int unused)
{
    got_epipe = 1;
}
#endif


#ifdef USE_CHECK_ALLOC
static void memstat_sig_handle(int unused)
{
    do_report_usage = 1;
}
#endif


extern unsigned int server_get_uptime(void)
{
    return (unsigned int)difftime(time(NULL),starttime);
}


extern unsigned int server_get_starttime(void)
{
    return starttime;
}


static char const * laddr_type_get_str(t_laddr_type laddr_type)
{
    switch (laddr_type)
    {
    case laddr_type_bnet:
	return "bnet";
    case laddr_type_irc:
	return "irc";
    case laddr_type_telnet:
	return "telnet";
    default:
	return "UNKNOWN";
    }
}


/* This is similar function to the FD_ISSET macro used with select.
 * Given a set of fds and a mask it determines if the socket was
 * selected.  It tries to take advantage of the ordering of the
 * fd array to avoid searching it (thereby making socket handling
 * O(n^2)) by saving the location of the next member.  It will
 * normally take one or two iterations of the top loop to find
 * the right entry.  That will normally fail once for every poll()
 * and then one iteration of the second loop will do the job.
 * I haven't seen it take logner than that and it should never
 * try to lookup an fd that isn't in the list.
 */
#ifdef HAVE_POLL
static int poll_check(struct pollfd const * fds, int num_fd, int sd, int mask)
{
    static int guess=0;
    int        i;
    
    for (i=guess; i<num_fd; i++)
	if (fds[i].fd==sd)
	{
	    guess = i;
	    return fds[i].revents & mask;
	}
    
    if (guess>=num_fd)
	guess = num_fd;
    
    for (i=0; i<guess; i++)
	if (fds[i].fd==sd)
	{
	    guess = i;
	    return fds[i].revents & mask;
	}
    
    return 0;
}
#endif


static int sd_accept(t_addr const * curr_laddr, t_laddr_info const * laddr_info, int ssocket, int usocket)
{
    char               tempa[32];
    int                csocket;
    struct sockaddr_in caddr;
    psock_t_socklen    caddr_len;
    unsigned int       raddr;
    unsigned short     rport;
		    
    if (!addr_get_addr_str(curr_laddr,tempa,sizeof(tempa)))
	strcpy(tempa,"x.x.x.x:x");
    
    /* accept the connection */
    memset(&caddr,0,sizeof(caddr)); /* not sure if this is needed... modern systems are ok anyway */
    caddr_len = sizeof(caddr);
    if ((csocket = psock_accept(ssocket,(struct sockaddr *)&caddr,&caddr_len))<0)
    {
	/* BSD, POSIX error for aborted connections, SYSV often uses EAGAIN or EPROTO */
	if (
#ifdef PSOCK_EWOULDBLOCK
	    psock_errno()==PSOCK_EWOULDBLOCK ||
#endif
#ifdef PSOCK_ECONNABORTED
	    psock_errno()==PSOCK_ECONNABORTED ||
#endif
#ifdef PSOCK_EPROTO
	    psock_errno()==PSOCK_EPROTO ||
#endif
	    0)
	    eventlog(eventlog_level_error,"sd_accept","client aborted connection on %s (psock_accept: %s)",tempa,strerror(psock_errno()));
	else /* EAGAIN can mean out of resources _or_ connection aborted :( */
	    if (
#ifdef PSOCK_EINTR
		psock_errno()!=PSOCK_EINTR &&
#endif
		1)
		eventlog(eventlog_level_error,"sd_accept","could not accept new connection on %s (psock_accept: %s)",tempa,strerror(psock_errno()));
	return -1;
    }
    
#ifdef HAVE_POLL
    if (csocket>=BNETD_MAX_SOCKETS) /* This check is a bit too strict (csocket is probably
                                     * greater than the number of connections) but this makes
                                     * life easier later.
                                     */
    {
	eventlog(eventlog_level_error,"sd_accept","csocket is beyond range allowed by BNETD_MAX_SOCKETS for poll() (%d>=%d)",csocket,BNETD_MAX_SOCKETS);
	psock_close(csocket);
	return -1;
    }
#else
# ifdef FD_SETSIZE
    if (csocket>=FD_SETSIZE) /* fd_set size is determined at compile time */
    {
	eventlog(eventlog_level_error,"sd_accept","csocket is beyond range allowed by FD_SETSIZE for select() (%d>=%d)",csocket,FD_SETSIZE);
	psock_close(csocket);
	return -1;
    }
# endif
#endif
    if (ipban_check(inet_ntoa(caddr.sin_addr))!=0)
    {
	eventlog(eventlog_level_error,"sd_accept","[%d] connection from banned address %s denied (closing connection)",csocket,inet_ntoa(caddr.sin_addr));
	psock_close(csocket);
	return -1;
    }
    
    eventlog(eventlog_level_info,"sd_accept","[%d] accepted connection from %s on %s",csocket,addr_num_to_addr_str(ntohl(caddr.sin_addr.s_addr),ntohs(caddr.sin_port)),tempa);
    
    if (prefs_get_use_keepalive())
    {
	int val=1;
	
	if (psock_setsockopt(csocket,PSOCK_SOL_SOCKET,PSOCK_SO_KEEPALIVE,&val,(psock_t_socklen)sizeof(val))<0)
	    eventlog(eventlog_level_error,"sd_accept","[%d] could not set socket option SO_KEEPALIVE (psock_setsockopt: %s)",csocket,strerror(psock_errno()));
	/* not a fatal error */
    }
    
    {
	struct sockaddr_in rsaddr;
	psock_t_socklen    rlen;
	
	memset(&rsaddr,0,sizeof(rsaddr)); /* not sure if this is needed... modern systems are ok anyway */
	rlen = sizeof(rsaddr);
	if (psock_getsockname(csocket,(struct sockaddr *)&rsaddr,&rlen)<0)
	{
	    eventlog(eventlog_level_error,"sd_accept","[%d] unable to determine real local port (psock_getsockname: %s)",csocket,strerror(psock_errno()));
	    /* not a fatal error */
	    raddr = addr_get_ip(curr_laddr);
	    rport = addr_get_port(curr_laddr);
	}
	else
	{
	    if (rsaddr.sin_family!=PSOCK_AF_INET)
	    {
		eventlog(eventlog_level_error,"sd_accept","local address returned with bad address family %d",(int)rsaddr.sin_family);
		/* not a fatal error */
		raddr = addr_get_ip(curr_laddr);
		rport = addr_get_port(curr_laddr);
	    }
	    else
	    {
		raddr = ntohl(rsaddr.sin_addr.s_addr);
		rport = ntohs(rsaddr.sin_port);
	    }
	}
    }
    
    if (psock_ctl(csocket,PSOCK_NONBLOCK)<0)
    {
	eventlog(eventlog_level_error,"sd_accept","[%d] could not set TCP socket to non-blocking mode (closing connection) (psock_ctl: %s)",csocket,strerror(psock_errno()));
	psock_close(csocket);
	return -1;
    }
    
    {
	t_connection * c;
	
	if (!(c = conn_create(csocket,usocket,raddr,rport,addr_get_ip(curr_laddr),addr_get_port(curr_laddr),ntohl(caddr.sin_addr.s_addr),ntohs(caddr.sin_port))))
	{
	    eventlog(eventlog_level_error,"sd_accept","[%d] unable to create new connection (closing connection)",csocket);
	    psock_close(csocket);
	    return -1;
	}
	
	eventlog(eventlog_level_debug,"sd_accept","[%d] client connected to a %s listening address",csocket,laddr_type_get_str(laddr_info->type));
	switch (laddr_info->type)
	{
	case laddr_type_irc:
	    conn_set_class(c,conn_class_irc);
	    conn_set_state(c,conn_state_connected);
	    break;
	case laddr_type_telnet:
	    conn_set_class(c,conn_class_telnet);
	    conn_set_state(c,conn_state_connected);
	    break;
	case laddr_type_bnet:
	default:
	    /* We have to wait for an initial "magic" byte on bnet connections to
             * tell us exactly what connection class we are dealing with.
             */
	    break;
	}
    }
    
    return 0;
}


static int sd_udpinput(t_addr * const curr_laddr, t_laddr_info const * laddr_info, int ssocket, int usocket)
{
    int             err;
    psock_t_socklen errlen;
    t_packet *      upacket;
    
    errlen = sizeof(err);
    if (psock_getsockopt(usocket,PSOCK_SOL_SOCKET,PSOCK_SO_ERROR,&err,&errlen)<0)
    {
        eventlog(eventlog_level_error,"sd_udpinput","[%d] unable to read socket error (psock_getsockopt: %s)",usocket,strerror(psock_errno()));
	return -1;
    }
    if (errlen>0 && err!=0) /* if it was an error, there is no packet to read */
    {
	eventlog(eventlog_level_error,"sd_udpinput","[%d] async UDP socket error notification (psock_getsockopt: %s)",usocket,strerror(err));
	return -1;
    }
    if (!(upacket = packet_create(packet_class_udp)))
    {
	eventlog(eventlog_level_error,"sd_udpinput","could not allocate raw packet for input");
	return -1;
    }
    
    {
	struct sockaddr_in fromaddr;
	psock_t_socklen    fromlen;
	int                len;
	
	fromlen = sizeof(fromaddr);
	if ((len = psock_recvfrom(usocket,packet_get_raw_data_build(upacket,0),MAX_PACKET_SIZE,0,(struct sockaddr *)&fromaddr,&fromlen))<0)
	{
	    if (
#ifdef PSOCK_EINTR
		psock_errno()!=PSOCK_EINTR &&
#endif
#ifdef PSOCK_EAGAIN
		psock_errno()!=PSOCK_EAGAIN &&
#endif
#ifdef PSOCK_EWOULDBLOCK
		psock_errno()!=PSOCK_EWOULDBLOCK &&
#endif
		1)
		eventlog(eventlog_level_error,"sd_udpinput","could not recv UDP datagram (psock_recvfrom: %s)",strerror(psock_errno()));
	    packet_del_ref(upacket);
	    return -1;
	}
	
	if (fromaddr.sin_family!=PSOCK_AF_INET)
	{
	    eventlog(eventlog_level_error,"sd_udpinput","got UDP datagram with bad address family %d",(int)fromaddr.sin_family);
	    packet_del_ref(upacket);
	    return -1;
	}
	
	packet_set_size(upacket,len);
	
	if (hexstrm)
	{
	    char tempa[32];
	    
	    if (!addr_get_addr_str(curr_laddr,tempa,sizeof(tempa)))
		strcpy(tempa,"x.x.x.x:x");
	    fprintf(hexstrm,"%d: recv class=%s[0x%02hx] type=%s[0x%04hx] from=%s to=%s length=%u\n",
		    usocket,
		    packet_get_class_str(upacket),(unsigned int)packet_get_class(upacket),
		    packet_get_type_str(upacket,packet_dir_from_client),packet_get_type(upacket),
		    addr_num_to_addr_str(ntohl(fromaddr.sin_addr.s_addr),ntohs(fromaddr.sin_port)),
		    tempa,
		    packet_get_size(upacket));
	    hexdump(hexstrm,packet_get_raw_data(upacket,0),packet_get_size(upacket));
	}
	
	handle_udp_packet(usocket,ntohl(fromaddr.sin_addr.s_addr),ntohs(fromaddr.sin_port),upacket);
	packet_del_ref(upacket);
    }
    
    return 0;
}


static int sd_tcpinput(int csocket, t_connection * c)
{
    unsigned int currsize;
    t_packet *   packet;
    
    currsize = conn_get_in_size(c);
    
    if (!*conn_get_in_queue(c))
    {
	switch (conn_get_class(c))
	{
	case conn_class_init:
	    if (!(packet = packet_create(packet_class_init)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate init packet for input");
		return -1;
	    }
	    break;
         case conn_class_d2cs_bnetd:
            if (!(packet = packet_create(packet_class_d2cs_bnetd)))
            {
                eventlog(eventlog_level_error,"server_process","could not allocate d2cs_bnetd packet");
                return -1;
            }
            break;
	case conn_class_bnet:
	    if (!(packet = packet_create(packet_class_bnet)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate bnet packet for input");
		return -1;
	    }
	    break;
	case conn_class_file:
	    if (!(packet = packet_create(packet_class_file)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate file packet for input");
		return -1;
	    }
	    break;
	case conn_class_bits:
	    if (!(packet = packet_create(packet_class_bits)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate BITS packet for input");
		return -1;
	    }
	    break;
	case conn_class_defer:
	case conn_class_bot:
	case conn_class_irc:
	case conn_class_telnet:
	    if (!(packet = packet_create(packet_class_raw)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate raw packet for input");
		return -1;
	    }
	    packet_set_size(packet,1); /* start by only reading one char */
	    break;
	case conn_class_auth:
	    if (!(packet = packet_create(packet_class_auth)))
	    {
		eventlog(eventlog_level_error,"sd_tcpinput","could not allocate auth packet for input");
		return -1;
	    }
	    break;
	default:
	    eventlog(eventlog_level_error,"sd_tcpinput","[%d] connection has bad class (closing connection)",conn_get_socket(c));
	    conn_destroy(c);
	    return -1;
	}
	queue_push_packet(conn_get_in_queue(c),packet);
	packet_del_ref(packet);
	if (!*conn_get_in_queue(c))
	    return -1; /* push failed */
	currsize = 0;
    }
    
    packet = queue_peek_packet((t_queue const * const *)conn_get_in_queue(c)); /* avoid warning */
    switch (net_recv_packet(csocket,packet,&currsize))
    {
    case -1:
	eventlog(eventlog_level_debug,"sd_tcpinput","[%d] read FAILED (closing connection)",conn_get_socket(c));
	conn_destroy(c);
	return -1;
	
    case 0: /* still working on it */
	/* eventlog(eventlog_level_debug,"sd_tcpinput","[%d] still reading \"%s\" packet (%u of %u bytes so far)",conn_get_socket(c),packet_get_class_str(packet),conn_get_in_size(c),packet_get_size(packet)); */
	conn_set_in_size(c,currsize);
	break;
	
    case 1: /* done reading */
	switch (conn_get_class(c))
	{
	case conn_class_defer:
	    {
		unsigned char const * const temp=packet_get_raw_data_const(packet,0);
		
		eventlog(eventlog_level_debug,"sd_tcpinput","[%d] got first packet byte %02x",conn_get_socket(c),(unsigned int)temp[0]);
		if (temp[0]==0xff) /* HACK: thankfully all bnet packet types end with ff */
		{
		    conn_set_class(c,conn_class_bnet);
		    conn_set_in_size(c,currsize);
		    packet_set_class(packet,packet_class_bnet);
		    eventlog(eventlog_level_debug,"sd_tcpinput","[%d] defered connection class is bnet",conn_get_socket(c));
		}
		else
		{
		    conn_set_class(c,conn_class_auth);
		    conn_set_in_size(c,currsize);
		    packet_set_class(packet,packet_class_auth);
		    eventlog(eventlog_level_debug,"sd_tcpinput","[%d] defered connection class is auth",conn_get_socket(c));
		}
	    }
	    break;
	
	case conn_class_bot:
	case conn_class_telnet:
	    if (currsize<MAX_PACKET_SIZE) /* if we overflow, we can't wait for the end of the line.
					     handle_*_packet() should take care of it */
	    {
		char const * const temp=packet_get_raw_data_const(packet,0);

		if (temp[currsize-1]!='\r' && temp[currsize-1]!='\n')
		{
		    conn_set_in_size(c,currsize);
		    packet_set_size(packet,currsize+1);
		    break; /* no end of line, get another char */
		}
	    }
	    /* got a complete line or overflow... fall through */
	
	default:
	    packet = queue_pull_packet(conn_get_in_queue(c));
	    
	    if (hexstrm)
	    {
		fprintf(hexstrm,"%d: recv class=%s[0x%02hx] type=%s[0x%04hx] length=%u\n",
			csocket,
			packet_get_class_str(packet),(unsigned int)packet_get_class(packet),
			packet_get_type_str(packet,packet_dir_from_client),packet_get_type(packet),
			packet_get_size(packet));
		hexdump(hexstrm,packet_get_raw_data_const(packet,0),packet_get_size(packet));
	    }
	    
	    if (conn_get_class(c)==conn_class_bot ||
		conn_get_class(c)==conn_class_telnet) /* NUL terminate the line to make life easier */
	    {
		char * const temp=packet_get_raw_data(packet,0);
		
		if (temp[currsize-1]=='\r' || temp[currsize-1]=='\n')
		    temp[currsize-1] = '\0'; /* have to do it here instead of above so everything
						is intact for the hexdump */
	    }
	    
	    {
		int ret;
		
		switch (conn_get_class(c))
		{
		case conn_class_bits:
#ifdef WITH_BITS
		    ret = handle_bits_packet(c,packet);
#else
		    eventlog(eventlog_level_error,"sd_tcpinput","[%d] BITS not enabled (closing connection)",conn_get_socket(c));
		    ret = -1;
#endif
		    break;
		case conn_class_init:
		    ret = handle_init_packet(c,packet);
		    break;
		case conn_class_bnet:
		    ret = handle_bnet_packet(c,packet);
		    break;
                case conn_class_d2cs_bnetd:
                    ret = handle_d2cs_packet(c,packet);
                    break;
		case conn_class_bot:
		    ret = handle_bot_packet(c,packet);
		    break;
		case conn_class_telnet:
		    ret = handle_telnet_packet(c,packet);
		    break;
		case conn_class_file:
		    ret = handle_file_packet(c,packet);
		    break;
		case conn_class_auth:
		    ret = handle_auth_packet(c,packet);
		    break;
		case conn_class_irc:
		    ret = handle_irc_packet(c,packet);
		    break;
		default:
		    eventlog(eventlog_level_error,"sd_tcpinput","[%d] bad packet class %d (closing connection)",conn_get_socket(c),(int)packet_get_class(packet));
		    ret = -1;
		}
		packet_del_ref(packet);
		if (ret<0)
		{
		    conn_destroy(c);
		    return -1;
		}
	    }
	    
	    conn_set_in_size(c,0);
	}
    }
    
    return 0;
}


static int sd_tcpoutput(int csocket, t_connection * c)
{
    unsigned int currsize;
    unsigned int totsize;
    t_packet *   packet;
    
    totsize = 0;
    for (;;)
    {
	currsize = conn_get_out_size(c);
	switch (net_send_packet(csocket,queue_peek_packet((t_queue const * const *)conn_get_out_queue(c)),&currsize)) /* avoid warning */
	{
	case -1:
	    conn_destroy(c);
	    return -1;
	    
	case 0: /* still working on it */
	    conn_set_out_size(c,currsize);
	    return 0; /* bail out */
	    
	case 1: /* done sending */
	    packet = queue_pull_packet(conn_get_out_queue(c));
	    
	    if (hexstrm)
	    {
		fprintf(hexstrm,"%d: send class=%s[0x%02hx] type=%s[0x%04hx] length=%u\n",
			csocket,
			packet_get_class_str(packet),(unsigned int)packet_get_class(packet),
			packet_get_type_str(packet,packet_dir_from_server),packet_get_type(packet),
			packet_get_size(packet));
		hexdump(hexstrm,packet_get_raw_data(packet,0),packet_get_size(packet));
	    }
	    
	    packet_del_ref(packet);
	    conn_set_out_size(c,0);
	    
	    /* stop at about 2KB (or until out of packets or EWOULDBLOCK) */
	    if (totsize>2048 || queue_get_length((t_queue const * const *)conn_get_out_queue(c))<1)
		return 0;
	    totsize += currsize;
	}
    }
    
    /* not reached */
}


extern int server_process(void)
{
    t_addrlist *    laddrs;
    t_addr *        curr_laddr;
    t_addr_data     laddr_data;
    t_laddr_info *  laddr_info;
#ifdef HAVE_POLL
    struct pollfd * fds;
    int             num_fd;
#else
    struct timeval  tv;
    t_psock_fd_set  rfds, wfds;
    int             highest_fd;
#endif
    time_t          curr_exittime, prev_exittime, prev_savetime, track_time, now;
    unsigned int    syncdelta;
    t_connection *  c;
    t_elem const *  acurr;
    t_elem const *  ccurr;
    int             csocket;
#ifdef DO_POSIXSIG
    sigset_t        block_set;
    sigset_t        save_set;
#endif
    unsigned int    count;
    
    if (psock_init()<0)
    {
	eventlog(eventlog_level_error,"server_process","could not initialize socket functions");
	return -1;
    }
    
    /* Start with the Battle.net address list */
    if (!(laddrs = addrlist_create(prefs_get_bnetdserv_addrs(),INADDR_ANY,BNETD_SERV_PORT)))
    {
	eventlog(eventlog_level_error,"server_process","could not create %s server address list from \"%s\"",laddr_type_get_str(laddr_type_bnet),prefs_get_bnetdserv_addrs());
	return -1;
    }
    /* Mark all these laddrs for being classic Battle.net service */
    LIST_TRAVERSE_CONST(laddrs,acurr)
    {
	curr_laddr = elem_get_data(acurr);
	if (addr_get_data(curr_laddr).p)
	    continue;
	if (!(laddr_info = malloc(sizeof(t_laddr_info))))
	{
	    eventlog(eventlog_level_error,"server_process","could not create %s address info (malloc: %s)",laddr_type_get_str(laddr_type_bnet),strerror(psock_errno()));
	    goto error_addr_list;
	}
        laddr_info->usocket = -1;
        laddr_info->ssocket = -1;
        laddr_info->type = laddr_type_bnet;
	laddr_data.p = laddr_info;
        if (addr_set_data(curr_laddr,laddr_data)<0)
	{
	    eventlog(eventlog_level_error,"server_process","could not set address data");
	    if (laddr_info->usocket!=-1)
	    {
		psock_close(laddr_info->usocket);
		laddr_info->usocket = -1;
	    }
	    if (laddr_info->ssocket!=-1)
	    {
		psock_close(laddr_info->ssocket);
		laddr_info->ssocket = -1;
	    }
	    goto error_poll;
	}
    }
    
    /* Append list of addresses to listen for IRC connections */
    if (addrlist_append(laddrs,prefs_get_irc_addrs(),INADDR_ANY,BNETD_IRC_PORT)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not create %s server address list from \"%s\"",laddr_type_get_str(laddr_type_irc),prefs_get_irc_addrs());
	goto error_addr_list;
    }
    /* Mark all new laddrs for being IRC service */
    LIST_TRAVERSE_CONST(laddrs,acurr)
    { 
	curr_laddr = elem_get_data(acurr);
	if (addr_get_data(curr_laddr).p)
	    continue;
	if (!(laddr_info = malloc(sizeof(t_laddr_info))))
	{
	    eventlog(eventlog_level_error,"server_process","could not create %s address info (malloc: %s)",laddr_type_get_str(laddr_type_irc),strerror(psock_errno()));
	    goto error_addr_list;
	}
        laddr_info->usocket = -1;
        laddr_info->ssocket = -1;
        laddr_info->type = laddr_type_irc;
	laddr_data.p = laddr_info;
	if (addr_set_data(curr_laddr,laddr_data)<0)
	{
	    eventlog(eventlog_level_error,"server_process","could not set address data");
	    if (laddr_info->usocket!=-1)
	    {
		psock_close(laddr_info->usocket);
		laddr_info->usocket = -1;
	    }
	    if (laddr_info->ssocket!=-1)
	    {
		psock_close(laddr_info->ssocket);
		laddr_info->ssocket = -1;
	    }
	    goto error_poll;
	}
    }
    
    /* Append list of addresses to listen for telnet connections */
    if (addrlist_append(laddrs,prefs_get_telnet_addrs(),INADDR_ANY,BNETD_TELNET_PORT)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not create %s server address list from \"%s\"",laddr_type_get_str(laddr_type_telnet),prefs_get_telnet_addrs());
	goto error_addr_list;
    }
    /* Mark all new laddrs for being telnet service */
    LIST_TRAVERSE_CONST(laddrs,acurr)
    { 
	curr_laddr = elem_get_data(acurr);
	if (addr_get_data(curr_laddr).p)
	    continue;
	if (!(laddr_info = malloc(sizeof(t_laddr_info))))
	{
	    eventlog(eventlog_level_error,"server_process","could not create %s address info (malloc: %s)",laddr_type_get_str(laddr_type_telnet),strerror(psock_errno()));
	    goto error_addr_list;
	}
        laddr_info->usocket = -1;
        laddr_info->ssocket = -1;
        laddr_info->type = laddr_type_telnet;
	laddr_data.p = laddr_info;
	if (addr_set_data(curr_laddr,laddr_data)<0)
	{
	    eventlog(eventlog_level_error,"server_process","could not set address data");
	    if (laddr_info->usocket!=-1)
	    {
		psock_close(laddr_info->usocket);
		laddr_info->usocket = -1;
	    }
	    if (laddr_info->ssocket!=-1)
	    {
		psock_close(laddr_info->ssocket);
		laddr_info->ssocket = -1;
	    }
	    goto error_poll;
	}
    }
    
    
#ifdef HAVE_POLL
    eventlog(eventlog_level_debug,"server_process","setting up poll structures");
    if (!(fds = malloc(BNETD_MAX_SOCKETS * sizeof (struct pollfd))))
    {
	eventlog(eventlog_level_error,"server_process","could not allocate poll structures");
	goto error_addr_list;
    }
#endif
    
    LIST_TRAVERSE_CONST(laddrs,acurr)
    {
	curr_laddr = elem_get_data(acurr);
	if (!(laddr_info = addr_get_data(curr_laddr).p))
	{
	    eventlog(eventlog_level_error,"server_process","NULL address info");
	    goto error_poll;
	}
	
	if ((laddr_info->ssocket = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_STREAM,0))<0)
	{
	    eventlog(eventlog_level_error,"server_process","could not create a %s listening socket (psock_socket: %s)",laddr_type_get_str(laddr_info->type),strerror(psock_errno()));
	    goto error_poll;
	}
#if defined(FD_SETSIZE) && !defined(HAVE_POLL)
	if (laddr_info->ssocket>=FD_SETSIZE) /* fd_set size is determined at compile time */
	{
	    eventlog(eventlog_level_error,"server_process","%s TCP socket is beyond range allowed by FD_SETSIZE for select() (%d>=%d)",laddr_type_get_str(laddr_info->type),laddr_info->ssocket,FD_SETSIZE);
	    psock_close(laddr_info->ssocket);
	    laddr_info->ssocket = -1;
	    goto error_poll;
	}
#endif
	if (laddr_info->type==laddr_type_bnet)
	{
	    if ((laddr_info->usocket = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_DGRAM,0))<0)
	    {
		eventlog(eventlog_level_error,"server_process","could not create UDP socket (psock_socket: %s)",strerror(psock_errno()));
		psock_close(laddr_info->ssocket);
		laddr_info->ssocket = -1;
		goto error_poll;
	    }
#if defined(FD_SETSIZE) && !defined(HAVE_POLL)
	    if (laddr_info->usocket>=FD_SETSIZE) /* fd_set size is determined at compile time */
	    {
		eventlog(eventlog_level_error,"server_process","%s UDP socket is beyond range allowed by FD_SETSIZE for select() (%d>=%d)",laddr_type_get_str(laddr_info->type),laddr_info->usocket,FD_SETSIZE);
		psock_close(laddr_info->usocket);
		laddr_info->usocket = -1;
		psock_close(laddr_info->ssocket);
		laddr_info->ssocket = -1;
		goto error_poll;
	    }
#endif
	}
	
	{
	    int val;
	    
	    if (laddr_info->ssocket!=-1)
	    {
		val = 1;
		if (psock_setsockopt(laddr_info->ssocket,PSOCK_SOL_SOCKET,PSOCK_SO_REUSEADDR,&val,(psock_t_socklen)sizeof(int))<0)
		    eventlog(eventlog_level_error,"server_process","could not set option SO_REUSEADDR on %s socket %d (psock_setsockopt: %s)",laddr_type_get_str(laddr_info->type),laddr_info->usocket,strerror(psock_errno()));
		/* not a fatal error... */
	    }
	    
	    if (laddr_info->usocket!=-1)
	    {
		val = 1;
		if (psock_setsockopt(laddr_info->usocket,PSOCK_SOL_SOCKET,PSOCK_SO_REUSEADDR,&val,(psock_t_socklen)sizeof(int))<0)
		    eventlog(eventlog_level_error,"server_process","could not set option SO_REUSEADDR on %s socket %d (psock_setsockopt: %s)",laddr_type_get_str(laddr_info->type),laddr_info->usocket,strerror(psock_errno()));
		/* not a fatal error... */
	    }
	}
	
	{
	    char               tempa[32];
	    struct sockaddr_in saddr;
	    
	    if (laddr_info->ssocket!=-1)
	    {
		memset(&saddr,0,sizeof(saddr));
		saddr.sin_family = PSOCK_AF_INET;
		saddr.sin_port = htons(addr_get_port(curr_laddr));
		saddr.sin_addr.s_addr = htonl(addr_get_ip(curr_laddr));
		if (psock_bind(laddr_info->ssocket,(struct sockaddr *)&saddr,(psock_t_socklen)sizeof(saddr))<0)
		{
		    if (!addr_get_addr_str(curr_laddr,tempa,sizeof(tempa)))
			strcpy(tempa,"x.x.x.x:x");
		    eventlog(eventlog_level_error,"server_process","could not bind %s socket to address %s TCP (psock_bind: %s)",laddr_type_get_str(laddr_info->type),tempa,strerror(psock_errno()));
		    goto error_poll;
		}
	    }
	    
	    if (laddr_info->usocket!=-1)
	    {
		memset(&saddr,0,sizeof(saddr));
		saddr.sin_family = PSOCK_AF_INET;
		saddr.sin_port = htons(addr_get_port(curr_laddr));
		saddr.sin_addr.s_addr = htonl(addr_get_ip(curr_laddr));
		if (psock_bind(laddr_info->usocket,(struct sockaddr *)&saddr,(psock_t_socklen)sizeof(saddr))<0)
		{
		    if (!addr_get_addr_str(curr_laddr,tempa,sizeof(tempa)))
			strcpy(tempa,"x.x.x.x:x");
		    eventlog(eventlog_level_error,"server_process","could not bind %s socket to address %s UDP (psock_bind: %s)",laddr_type_get_str(laddr_info->type),tempa,strerror(psock_errno()));
		    goto error_poll;
		}
	    }
	    
	    if (laddr_info->ssocket!=-1)
	    {
		/* tell socket to listen for connections */
		if (psock_listen(laddr_info->ssocket,LISTEN_QUEUE)<0)
		{
		    eventlog(eventlog_level_error,"server_process","could not set %s socket %d to listen (psock_listen: %s)",laddr_type_get_str(laddr_info->type),laddr_info->ssocket,strerror(psock_errno()));
		    goto error_poll;
		}
	    }
	    
	    if (!addr_get_addr_str(curr_laddr,tempa,sizeof(tempa)))
		strcpy(tempa,"x.x.x.x:x");
	    eventlog(eventlog_level_info,"server_process","listening for %s connections on %s TCP",laddr_type_get_str(laddr_info->type),tempa);
	}
	
	if (laddr_info->ssocket!=-1)
	    if (psock_ctl(laddr_info->ssocket,PSOCK_NONBLOCK)<0)
		eventlog(eventlog_level_error,"server_process","could not set %s TCP listen socket to non-blocking mode (psock_ctl: %s)",laddr_type_get_str(laddr_info->type),strerror(psock_errno()));
	if (laddr_info->usocket!=-1)
	    if (psock_ctl(laddr_info->usocket,PSOCK_NONBLOCK)<0)
		eventlog(eventlog_level_error,"server_process","could not set %s UDP socket to non-blocking mode (psock_ctl: %s)",laddr_type_get_str(laddr_info->type),strerror(psock_errno()));
    }
    
    /* setup signal handlers */
    prev_exittime = sigexittime;
    
#ifdef DO_POSIXSIG
    if (sigemptyset(&save_set)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigemptyset(&block_set)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigaddset(&block_set,SIGINT)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not add signal to set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigaddset(&block_set,SIGHUP)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not add signal to set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigaddset(&block_set,SIGTERM)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not add signal to set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigaddset(&block_set,SIGUSR1)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not add signal to set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    if (sigaddset(&block_set,SIGUSR2)<0)
    {
	eventlog(eventlog_level_error,"server_process","could not add signal to set (sigemptyset: %s)",strerror(errno));
	goto error_poll;
    }
    
    {
	struct sigaction quit_action;
	struct sigaction restart_action;
	struct sigaction save_action;
	struct sigaction pipe_action;
# ifdef USE_CHECK_ALLOC
	struct sigaction memstat_action;
# endif
	
	quit_action.sa_handler = quit_sig_handle;
	if (sigemptyset(&quit_action.sa_mask)<0)
	    eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	quit_action.sa_flags = SA_RESTART;
	
	restart_action.sa_handler = restart_sig_handle;
	if (sigemptyset(&restart_action.sa_mask)<0)
	    eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	restart_action.sa_flags = SA_RESTART;
	
	save_action.sa_handler = save_sig_handle;
	if (sigemptyset(&save_action.sa_mask)<0)
	    eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	save_action.sa_flags = SA_RESTART;
	
# ifdef USE_CHECK_ALLOC
	memstat_action.sa_handler = memstat_sig_handle;
	if (sigemptyset(&memstat_action.sa_mask)<0)
	    eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	memstat_action.sa_flags = SA_RESTART;
# endif
	
	pipe_action.sa_handler = pipe_sig_handle;
	if (sigemptyset(&pipe_action.sa_mask)<0)
	    eventlog(eventlog_level_error,"server_process","could not initialize signal set (sigemptyset: %s)",strerror(errno));
	pipe_action.sa_flags = SA_RESTART;
	
	if (sigaction(SIGINT,&quit_action,NULL)<0) /* control-c */
	    eventlog(eventlog_level_error,"server_process","could not set SIGINT signal handler (sigaction: %s)",strerror(errno));
	if (sigaction(SIGHUP,&restart_action,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not set SIGHUP signal handler (sigaction: %s)",strerror(errno));
	if (sigaction(SIGTERM,&quit_action,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not set SIGTERM signal handler (sigaction: %s)",strerror(errno));
	if (sigaction(SIGUSR1,&save_action,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not set SIGUSR1 signal handler (sigaction: %s)",strerror(errno));
# ifdef USE_CHECK_ALLOC
	if (sigaction(SIGUSR2,&memstat_action,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not set SIGUSR2 signal handler (sigaction: %s)",strerror(errno));
# endif
	if (sigaction(SIGPIPE,&pipe_action,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not set SIGPIPE signal handler (sigaction: %s)",strerror(errno));
    }
#endif
    
    syncdelta = prefs_get_user_sync_timer();
    
    track_time = time(NULL)-prefs_get_track();
    starttime=prev_savetime = time(NULL);
    count = 0;
    
    for (;;)
    {
#ifdef WIN32
	if (kbhit() && getch()=='q')
	    server_quit_wraper();
#endif
	
#ifdef DO_POSIXSIG
	if (sigprocmask(SIG_SETMASK,&save_set,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not unblock signals");
	/* receive signals here */
	if (sigprocmask(SIG_SETMASK,&block_set,NULL)<0)
	    eventlog(eventlog_level_error,"server_process","could not block signals");
#endif
	
	if (got_epipe)
	{
	    got_epipe = 0;
	    eventlog(eventlog_level_info,"server_process","handled SIGPIPE");
	}
	
	now = time(NULL);
	
	curr_exittime = sigexittime;
	if (curr_exittime && (curr_exittime<=now || connlist_get_length()<1))
	{
	    eventlog(eventlog_level_info,"server_process","the server is shutting down (%d connections left)",connlist_get_length());
	    break;
	}
	if (prev_exittime!=curr_exittime)
	{
	    t_message *  message;
	    char const * tstr;
	    
	    if (curr_exittime!=0)
	    {
		char text[29+256+2+32+24+1]; /* "The ... in " + time + " (" num + " connection ...)." + NUL */
	        
		tstr = seconds_to_timestr(curr_exittime-now);
		sprintf(text,"The server will shut down in %s (%d connections remaining).",tstr,connlist_get_length());
        	if ((message = message_create(message_type_error,NULL,NULL,text)))
		{
		    message_send_all(message);
		    message_destroy(message);
		}
		eventlog(eventlog_level_info,"server_process","the server will shut down in %s (%d connections remaining)",tstr,connlist_get_length());
	    }
	    else
	    {
		if ((message = message_create(message_type_error,NULL,NULL,"Server shutdown has been canceled.")))
		{
		    message_send_all(message);
		    message_destroy(message);
		}
		eventlog(eventlog_level_info,"server_process","server shutdown canceled");
	    }
	}
	prev_exittime = curr_exittime;
	
	if (syncdelta && prev_savetime+(time_t)syncdelta<=now)
	{
	    accountlist_save(prefs_get_user_sync_timer());
            gamelist_check_voidgame();
	    prev_savetime = now;
	}
	
	if (prefs_get_track() && track_time+(time_t)prefs_get_track()<=now)
	{
	    track_time = now;
	    tracker_send_report(laddrs);
	}
	
	if (do_save)
	{
	    eventlog(eventlog_level_info,"server_process","saving accounts due to signal");
	    accountlist_save(0);
	    do_save = 0;
	}
	
	if (do_restart)
	{
	    eventlog(eventlog_level_info,"server_process","reading configuration files");
	    if (preffile)
	    {
        	if (prefs_load(preffile)<0)
		    eventlog(eventlog_level_error,"server_process","could not parse configuration file");
	    }
	    else
		if (prefs_load(BNETD_DEFAULT_CONF_FILE)<0)
		    eventlog(eventlog_level_error,"server_process","using default configuration");
	    
	    if (eventlog_open(prefs_get_logfile())<0)
		eventlog(eventlog_level_error,"server_process","could not use the file \"%s\" for the eventlog",prefs_get_logfile());
	    
	    /* FIXME: load new network settings */
	    /* FIXME: load new ad banners */
	    
	    accountlist_load_default(); /* FIXME: free old one */
	    
	    /* FIXME: reload channel list, need some tests, bits is disabled */
	    #ifndef WITH_BITS
	    channellist_reload();
	    #endif

            realmlist_destroy();
            if (realmlist_create(prefs_get_realmfile())<0)
	        eventlog(eventlog_level_error,"main","could not load realm list");

	    autoupdate_unload();
	    if (autoupdate_load(prefs_get_mpqfile())<0)
	      eventlog(eventlog_level_error,"main","could not load autoupdate list");

	    versioncheck_unload();
	    if (versioncheck_load(prefs_get_versioncheck_file())<0)
	      eventlog(eventlog_level_error,"main","could not load versioncheck list");
	    	    
	    if (ipbanlist_unload()<0)
		eventlog(eventlog_level_error,"server_process","could not unload old IP ban list");
	    if (ipbanlist_load(prefs_get_ipbanfile())<0)
		eventlog(eventlog_level_error,"server_process","could not load new IP ban list");
	    
	    helpfile_unload();
	    if (helpfile_init(prefs_get_helpfile())<0)
		eventlog(eventlog_level_error,"server_process","could not load the helpfile");
	    
	    if (adbannerlist_destroy()<0)
		eventlog(eventlog_level_error,"server_process","could not unload old adbanner list");
	    if (adbannerlist_create(prefs_get_adfile())<0)
		eventlog(eventlog_level_error,"server_process","could not new adbanner list");
	    
	    if (gametrans_unload()<0)
		eventlog(eventlog_level_error,"server_process","could not unload old gametrans list");
	    if (gametrans_load(prefs_get_transfile())<0)
		eventlog(eventlog_level_error,"server_process","could not load new gametrans list");
	    
	    if (prefs_get_track())
		tracker_set_servers(prefs_get_trackserv_addrs());
#ifdef WITH_BITS
	    if (!bits_uplink_connection)
		bits_motd_load_from_file();
#endif
	    syncdelta = prefs_get_user_sync_timer();
	    
	    eventlog(eventlog_level_info,"server_process","done reconfiguring");
	    
	    do_restart = 0;
	}
	
#ifdef USE_CHECK_ALLOC
	if (do_report_usage)
	{
	    check_report_usage();
	    do_report_usage = 0;
	}
#endif
	
	count += BNETD_POLL_INTERVAL;
	if (count>=1000) /* only check timers once a second */
	{
	    timerlist_check_timers(now);
	    count = 0;
	}
	
#ifdef HAVE_POLL
	num_fd = 0;
#else
	/* loop over all connections to create the sets for select() */
	PSOCK_FD_ZERO(&rfds);
	PSOCK_FD_ZERO(&wfds);
	highest_fd = -1;
#endif
	
	LIST_TRAVERSE_CONST(laddrs,acurr)
	{
	    curr_laddr = elem_get_data(acurr);
	    if (!(laddr_info = addr_get_data(curr_laddr).p))
	    {
		eventlog(eventlog_level_error,"server_process","NULL address info");
		continue;
	    }
	    
#ifdef HAVE_POLL
	    if (laddr_info->ssocket!=-1)
	    {
		fds[num_fd].fd = laddr_info->ssocket;
		fds[num_fd].events = POLLIN;
		fds[num_fd].revents = 0;
		num_fd++;
	    }
	    if (laddr_info->usocket!=-1)
	    {
		fds[num_fd].fd = laddr_info->usocket;
		fds[num_fd].events = POLLIN;
		fds[num_fd].revents = 0;
		num_fd++;
	    }
#else
	    if (laddr_info->ssocket!=-1)
	    {
		PSOCK_FD_SET(laddr_info->ssocket,&rfds);
		if (laddr_info->ssocket>highest_fd)
        	    highest_fd = laddr_info->ssocket;
	    }
	    if (laddr_info->usocket!=-1)
	    {
		PSOCK_FD_SET(laddr_info->usocket,&rfds);
		if (laddr_info->usocket>highest_fd)
        	    highest_fd = laddr_info->usocket;
	    }
#endif
	}
	
	LIST_TRAVERSE_CONST(connlist(),ccurr)
	{
	    c = elem_get_data(ccurr);
            csocket = conn_get_socket(c);
	    
#ifdef HAVE_POLL
 	    fds[num_fd].fd = csocket;
	    fds[num_fd].events = POLLIN;
	    if (queue_get_length((t_queue const * const *)conn_get_out_queue(c))>0)
		fds[num_fd].events |= POLLOUT; 
	    fds[num_fd].revents = 0;
	    num_fd++;
#else
	    PSOCK_FD_SET(csocket,&rfds);
	    if (queue_get_length((t_queue const * const *)conn_get_out_queue(c))>0)
		PSOCK_FD_SET(csocket,&wfds); /* pending output, also check for writeability */
	    if (csocket>highest_fd)
        	highest_fd = csocket;
#endif
	}
	
	/* find which sockets need servicing */
#ifdef HAVE_POLL
	switch (poll(fds,num_fd,BNETD_POLL_INTERVAL))
#else
	/* always reset the select() timeout */
	tv.tv_sec  = 0;
	tv.tv_usec = BNETD_POLL_INTERVAL*1000;
	switch (psock_select(highest_fd+1,&rfds,&wfds,NULL,&tv))
#endif
	{
	case -1: /* error */
	    if (
#ifdef PSOCK_EINTR
		errno!=PSOCK_EINTR &&
#endif
		1)
#ifdef HAVE_POLL
	        eventlog(eventlog_level_error,"server_process","poll failed (poll: %s)",strerror(errno));
#else
	        eventlog(eventlog_level_error,"server_process","select failed (select: %s)",strerror(errno));
#endif
	case 0: /* timeout... no sockets need checking */
	    continue;
	}
	
	/* check for incoming connection */
	if (!curr_exittime) /* don't allow connections while exiting... */
	    LIST_TRAVERSE_CONST(laddrs,acurr)
	    {
		curr_laddr = elem_get_data(acurr);
		if (!(laddr_info = addr_get_data(curr_laddr).p))
		{
		    eventlog(eventlog_level_error,"server_process","NULL address info");
		    continue;
		}
		
		if (laddr_info->ssocket!=-1)
		{
#ifdef HAVE_POLL
		    if (poll_check(fds,num_fd,laddr_info->ssocket,POLLIN|POLLHUP|POLLERR))
#else
		    if (PSOCK_FD_ISSET(laddr_info->ssocket,&rfds))
#endif
			sd_accept(curr_laddr,laddr_info,laddr_info->ssocket,laddr_info->usocket);
		}
		
		if (laddr_info->usocket!=-1)
		{
#ifdef HAVE_POLL
		    if (poll_check(fds,num_fd,laddr_info->usocket,POLLIN|POLLHUP|POLLERR))
#else
		    if (PSOCK_FD_ISSET(laddr_info->usocket,&rfds))
#endif
			sd_udpinput(curr_laddr,laddr_info,laddr_info->ssocket,laddr_info->usocket);
		}
	    }
	
	/* search connections for sockets that need service */
	/* Uncomment the following line if you want to increase the size of your log :) */
	/*eventlog(eventlog_level_debug,"server_process","Checking for connected sockets that need service");*/
	LIST_TRAVERSE_CONST(connlist(),ccurr)
	{
	    c = elem_get_data(ccurr);
	    
	    if (conn_get_state(c)==conn_state_destroy)
	    {
		conn_destroy(c);
		continue;
	    }
            csocket = conn_get_socket(c);
	    
#ifdef HAVE_POLL
	    if (poll_check(fds,num_fd,csocket,POLLNVAL))
	    {
		eventlog(eventlog_level_error,"server_process","[%d] oops, using stale file descriptor (closing connection)",conn_get_socket(c));
		conn_destroy(c);
		continue;
	    }
#endif
	    
#ifdef HAVE_POLL
	    if (poll_check(fds,num_fd,csocket,POLLIN|POLLHUP|POLLERR))
#else
	    if (PSOCK_FD_ISSET(csocket,&rfds))
#endif
		sd_tcpinput(csocket,c);
	    
#ifdef HAVE_POLL
	    if (poll_check(fds,num_fd,csocket,POLLOUT|POLLERR))
#else
	    if (PSOCK_FD_ISSET(csocket,&wfds))
#endif
		sd_tcpoutput(csocket,c);
	}
	list_purge(connlist());
	
	/* check all pending queries */
	query_tick();
    }
    
    /* cleanup for server shutdown */
    
#ifdef HAVE_POLL
    free(fds);
#endif
    
    LIST_TRAVERSE_CONST(connlist(),ccurr)
    {
	c = elem_get_data(ccurr);
	conn_destroy(c);
    }
    
    LIST_TRAVERSE_CONST(laddrs,acurr)
    {
	curr_laddr = elem_get_data(acurr);
	if ((laddr_info = addr_get_data(curr_laddr).p))
	{
	    if (laddr_info->usocket!=-1)
		psock_close(laddr_info->usocket);
	    if (laddr_info->ssocket!=-1)
		psock_close(laddr_info->ssocket);
	    free(laddr_info);
	}
    }
    addrlist_destroy(laddrs);
    
    return 0;
    
    /************************************************************************/
    
    /* error cleanup */
    
error_poll:
    
#ifdef HAVE_POLL
    free(fds);
#endif
    
error_addr_list:
    LIST_TRAVERSE_CONST(laddrs,acurr)
    {
	curr_laddr = elem_get_data(acurr);
	if ((laddr_info = addr_get_data(curr_laddr).p))
	{
	    if (laddr_info->usocket!=-1)
		psock_close(laddr_info->usocket);
	    if (laddr_info->ssocket!=-1)
		psock_close(laddr_info->ssocket);
	    free(laddr_info);
	}
    }
    addrlist_destroy(laddrs);
	
    return -1;
}

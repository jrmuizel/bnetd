/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
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
#define CONNECTION_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
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
#include "compat/strtoul.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strdup.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
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
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#include "compat/psock.h"
#include "common/eventlog.h"
#include "account.h"
#include "realm.h"
#include "channel.h"
#include "game.h"
#include "common/queue.h"
#include "tick.h"
#include "common/packet.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "message.h"
#include "common/version.h"
#include "prefs.h"
#include "common/util.h"
#include "common/list.h"
#include "watch.h"
#include "timer.h"
#include "irc.h"
#ifdef WITH_BITS
# include "bits.h"
# include "bits_va.h"
# include "bits_login.h"
# include "bits_ext.h"
# include "bits_query.h"
# include "bits_packet.h"
# include "bits_game.h"
# include "bits_motd.h"
# include "bits_chat.h"
# include "query.h"
#endif
#include "game_conv.h"
#include "udptest_send.h"
#include "character.h"
#include "versioncheck.h"
#include "common/bnet_protocol.h"
#include "connection.h"
#include "common/setup_after.h"


static int      totalcount=0;
static t_list * conn_head=NULL;

static void conn_send_welcome(t_connection * c);
static void conn_send_issue(t_connection * c);


static void conn_send_welcome(t_connection * c)
{
    char const * filename;
    FILE *       fd;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_send_welcome","got NULL connection");
	return;
    }
    if (c->welcomed)
	return;
#ifdef WITH_BITS
    /* send the network-wide MOTD first */
    bits_motd_send(c);
#endif
    if ((filename = prefs_get_motdfile()))
	if ((fd = fopen(filename,"r")))
	{
	    message_send_file(c,fd);
	    if (fclose(fd)<0)
		eventlog(eventlog_level_error,"conn_send_welcome","could not close MOTD file \"%s\" after reading (fopen: %s)",filename,strerror(errno));
	}
	else
	    eventlog(eventlog_level_error,"conn_send_welcome","could not open MOTD file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
    
    c->welcomed = 1;
}


static void conn_send_issue(t_connection * c)
{
    char const * filename;
    FILE *       fd;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_send_issue","got NULL connection");
	return;
    }
    
    if ((filename = prefs_get_issuefile()))
	if ((fd = fopen(filename,"r")))
	{
	    message_send_file(c,fd);
	    if (fclose(fd)<0)
		eventlog(eventlog_level_error,"conn_send_issue","could not close issue file \"%s\" after reading (fopen: %s)",filename,strerror(errno));
	}
	else
	    eventlog(eventlog_level_error,"conn_send_issue","could not open issue file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
    else
	eventlog(eventlog_level_debug,"conn_send_issue","no issue file");
}


extern void conn_test_latency(t_connection * c, time_t now, t_timer_data delta)
{
    t_packet * packet;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_test_latency","got NULL connection");
        return;
    }
    
    if (now==(time_t)0) /* zero means user logged out before expiration */
	return;
    
    if (conn_get_class(c)==conn_class_irc) {
    	/* We should start pinging the client after we received the first line ... */
    	/* NOTE: RFC2812 only suggests that PINGs are being sent 
    	 * if no other activity is detected. However it explecitly 
    	 * allows PINGs to be sent if there is activity on this 
    	 * connection. In other words we just don't care :)
    	 */
	if (conn_get_ircping(c)!=0) {
	    eventlog(eventlog_level_warn,"conn_test_latency","[%d] ping timeout",conn_get_socket(c));
	    conn_set_latency(c,0); /* FIXME: close connection */
	}
	irc_send_ping(c);
    } else {
    	/* FIXME: I think real Battle.net sends these even before login */
    	if (conn_get_channel(c))
            if ((packet = packet_create(packet_class_bnet)))
	    {
	    	packet_set_size(packet,sizeof(t_server_echoreq));
	    	packet_set_type(packet,SERVER_ECHOREQ);
	    	bn_int_set(&packet->u.server_echoreq.ticks,get_ticks());
	    	queue_push_packet(conn_get_out_queue(c),packet);
	    	packet_del_ref(packet);
	    }
	    else
	    	eventlog(eventlog_level_error,"conn_test_latency","could not create packet");
    }
    
    if (timerlist_add_timer(c,now+(time_t)delta.n,conn_test_latency,delta)<0)
	eventlog(eventlog_level_error,"conn_test_latency","could not add timer");
}


static void conn_send_nullmsg(t_connection * c, time_t now, t_timer_data delta)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_send_nullmsg","got NULL connection");
        return;
    }
   
    if (now==(time_t)0) /* zero means user logged out before expiration */
        return;
   
    message_send_text(c,message_type_null,c,NULL);
    
    if (timerlist_add_timer(c,now+(time_t)delta.n,conn_send_nullmsg,delta)<0)
        eventlog(eventlog_level_error,"conn_send_nullmsg","could not add timer");
}


extern char const * conn_class_get_str(t_conn_class class)
{
    switch (class)
    {
    case conn_class_init:
	return "init";
    case conn_class_defer: /* either bnet or auth... wait to see */
	return "defer";
    case conn_class_bnet:
	return "bnet";
    case conn_class_file:
	return "file";
    case conn_class_bot:
	return "bot";
    case conn_class_bits:
	return "bits";
    case conn_class_remote:
    	return "remote";
    case conn_class_d2cs_bnetd:
	return "d2cs_bnetd";
    case conn_class_auth:
	return "auth";
    case conn_class_telnet:
	return "telnet";
    case conn_class_irc:
	return "irc";
    case conn_class_none:
	return "none";
    default:
	return "UNKNOWN";
    }
}


extern char const * conn_state_get_str(t_conn_state state)
{
    switch (state)
    {
    case conn_state_empty:
	return "empty";
    case conn_state_initial:
	return "initial";
    case conn_state_connected:
	return "connected";
    case conn_state_bot_username:
	return "bot_username";
    case conn_state_bot_password:
	return "bot_password";
    case conn_state_loggedin:
	return "loggedin";
    case conn_state_destroy:
	return "destroy";
    default:
	return "UNKNOWN";
    }
}


extern t_connection * conn_create(int tsock, int usock, unsigned int real_local_addr, unsigned short real_local_port, unsigned int local_addr, unsigned short local_port, unsigned int addr, unsigned short port)
{
    static int     sessionnum=1;
    t_connection * temp;
    
    if (tsock<0)
    {
        eventlog(eventlog_level_error,"conn_create","got bad TCP socket %d",tsock);
        return NULL;
    }
    if (usock<-1) /* -1 is allowed for some connection classes like bot, irc, and telnet */
    {
        eventlog(eventlog_level_error,"conn_create","got bad UDP socket %d",usock);
        return NULL;
    }
    
    if (!(temp = malloc(sizeof(t_connection))))
    {
        eventlog(eventlog_level_error,"conn_create","could not allocate memory for temp");
	return NULL;
    }
    
    temp->tcp_sock               = tsock;
    temp->tcp_addr               = addr;
    temp->tcp_port               = port;
    temp->udp_sock               = usock;
    temp->udp_addr               = addr; /* same for now but client can request it to be different */
    temp->local_addr             = local_addr;
    temp->local_port             = local_port;
    temp->real_local_addr        = real_local_addr;
    temp->real_local_port        = real_local_port;
    temp->udp_port               = port;
    temp->class                  = conn_class_init;
    temp->state                  = conn_state_initial;
    temp->sessionkey             = ((unsigned int)rand())^((unsigned int)time(NULL)+(unsigned int)real_local_port);
    temp->sessionnum             = sessionnum++;
    temp->secret                 = ((unsigned int)rand())^(totalcount+((unsigned int)time(NULL)));
    temp->flags                  = MF_PLUG;
    temp->latency                = 0;
    temp->settings.dnd           = NULL;
    temp->settings.away          = NULL;
    temp->settings.ignore_list   = NULL;
    temp->settings.ignore_count  = 0;
    temp->settings.quota.totcount= 0;
    if (!(temp->settings.quota.list = list_create()))
    {
	free(temp);
        eventlog(eventlog_level_error,"conn_create","could not create quota list");
	return NULL;
    }
    temp->versionid              = 0;
    temp->archtag                = NULL;
    temp->clienttag              = NULL;
    temp->clientver              = NULL;
    temp->country                = NULL;
    temp->tzbias                 = 0;
    temp->account                = NULL;
    temp->channel                = NULL;
    temp->game                   = NULL;
    temp->outqueue               = NULL;
    temp->outsize                = 0;
    temp->inqueue                = NULL;
    temp->insize                 = 0;
    temp->welcomed               = 0;
    temp->host                   = NULL;
    temp->user                   = NULL;
    temp->clientexe              = NULL;
    temp->owner                  = NULL;
    temp->cdkey                  = NULL;
    temp->botuser                = NULL;
    temp->last_message           = time(NULL);
    temp->realmname              = NULL;
    temp->character              = NULL;
    temp->bound                  = NULL;
    temp->lastsender             = NULL;
    temp->realminfo              = NULL;
    temp->charname               = NULL;
    temp->versioncheck           = NULL;
    temp->ircline		 = NULL;
    temp->ircping		 = 0;
    temp->ircpass		 = NULL;
#ifdef WITH_BITS
    temp->bits_game		 = 0;
    temp->sessionid		 = 0;
    temp->bits		 	 = NULL;
#endif
    temp->udpok                  = 0;
    
    if (list_prepend_data(conn_head,temp)<0)
    {
	free(temp);
	eventlog(eventlog_level_error,"conn_create","could not prepend temp");
	return NULL;
    }
    
    eventlog(eventlog_level_info,"conn_create","[%d][%d] sessionkey=0x%08x sessionnum=0x%08x",temp->tcp_sock,temp->udp_sock,temp->sessionkey,temp->sessionnum);
    
    return temp;
}


extern void conn_destroy(t_connection * c)
{
    char const * classstr;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_destroy","got NULL connection");
	return;
    }
    
    classstr = conn_class_get_str(c->class);
    
    if (list_remove_data(conn_head,c)<0)
    {
	eventlog(eventlog_level_error,"conn_destroy","could not remove item from list");
	return;
    }
    
    if (c->class==conn_class_d2cs_bnetd)
    {
        t_realm * realm;

        realm=realmlist_find_realm_by_ip(conn_get_addr(c));
        if (realm)
             realm_deactive(realm);
        else
        {
             eventlog(eventlog_level_error,"conn_destroy","could not find realm for d2cs connection");
        }
    }

    if (c->realmname) {
        realm_add_player_number(realmlist_find_realm(conn_get_realmname(c)),-1);
    }

    
    /* free the memory with user quota */
    {
	t_qline * qline;
	t_elem *  curr;
	
	LIST_TRAVERSE(c->settings.quota.list,curr)
	{
	    qline = elem_get_data(curr);
	    free(qline);
	    list_remove_elem(c->settings.quota.list,curr);
	}
	list_destroy(c->settings.quota.list);
    }
    
    /* if this user in a channel, notify everyone that the user has left */
    if (c->channel)
	channel_del_connection(c->channel,c);
    
    conn_set_game(c,NULL,NULL,NULL,game_type_none,0);
    
    watchlist_del_all_events(c);
    timerlist_del_all_timers(c);
    
    c->state = conn_state_empty;
    
    if (c->versioncheck)
	versioncheck_destroy((void *)c->versioncheck); /* avoid warning */
    
    if (c->lastsender)
	free((void *)c->lastsender); /* avoid warning */
    
    if (c->settings.away)
	free((void *)c->settings.away); /* avoid warning */
    if (c->settings.dnd)
	free((void *)c->settings.dnd); /* avoid warning */
    
    if (c->clienttag)
	free((void *)c->clienttag); /* avoid warning */
    if (c->archtag)
	free((void *)c->archtag); /* avoid warning */
    if (c->clientver)
	free((void *)c->clientver); /* avoid warning */
    if (c->country)
	free((void *)c->country); /* avoid warning */
    if (c->host)
	free((void *)c->host); /* avoid warning */
    if (c->user)
	free((void *)c->user); /* avoid warning */
    if (c->clientexe)
	free((void *)c->clientexe); /* avoid warning */
    if (c->owner)
	free((void *)c->owner); /* avoid warning */
    if (c->cdkey)
	free((void *)c->cdkey); /* avoid warning */
    if (c->botuser)
	free((void *)c->botuser); /* avoid warning */
    if (c->realmname)
	free((void *)c->realmname); /* avoid warning */
    if (c->realminfo)
	free((void *)c->realminfo); /* avoid warning */
    if (c->charname)
	free((void *)c->charname); /* avoid warning */
    if (c->ircline)
	free((void *)c->ircline); /* avoid warning */
    if (c->ircpass)
	free((void *)c->ircpass); /* avoid warning */
    
    if (c->bound)
	c->bound->bound = NULL;
    
    if (c->settings.ignore_count>0)
	if (!c->settings.ignore_list)
	    eventlog(eventlog_level_error,"conn_destroy","found NULL ignore_list with ignore_count=%u",c->settings.ignore_count);
	else
	    free(c->settings.ignore_list);
    
    if (c->account)
    {
	char const * tname;
	
	watchlist_notify_event(c->account,watch_event_logout);
#ifdef WITH_BITS
	send_bits_va_logout(conn_get_sessionid(c));
#endif
	tname = account_get_name(c->account);
	eventlog(eventlog_level_info,"conn_destroy","[%d] \"%s\" logged out",c->tcp_sock,tname);
	account_unget_name(tname);
	c->account = NULL; /* the account code will free the memory later */
    }
    
    /* make sure the connection is closed */
    psock_shutdown(c->tcp_sock,PSOCK_SHUT_RDWR);
    psock_close(c->tcp_sock);
    
    /* clear out the packet queues */
    queue_clear(&c->inqueue);
    queue_clear(&c->outqueue);
    
#ifdef WITH_BITS
    if (c->bits)
	destroy_bits_ext(c);
    /* clear references for special bits connections */
    if (c==bits_uplink_connection)
        bits_uplink_connection = NULL;
    if (c==bits_loopback_connection)
        bits_loopback_connection = NULL;
    /* look if there are references in bits routes to this connection */
    bits_route_del_conn(c);
#endif
    
    eventlog(eventlog_level_info,"conn_destroy","[%d] closed %s connection",c->tcp_sock,classstr);
    
    free(c);
}


extern int conn_match(t_connection const * c, char const * username)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_match","got NULL connection");
        return -1;
    }
    if (!username)
    {
        eventlog(eventlog_level_error,"conn_match","got NULL username");
        return -1;
    }
    
    if (!c->account)
	return 0;
    
    return account_match(c->account,username);
}


extern t_conn_class conn_get_class(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_class","got NULL connection");
        return conn_class_none;
    }
    
    return c->class;
}


extern void conn_set_class(t_connection * c, t_conn_class class)
{
    t_timer_data  data;
    unsigned long delta;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_class","got NULL connection");
        return;
    }
    
    if (c->class==class)
	return;
    
    c->class = class;
    
    if (class==conn_class_bnet)
    {
	
	if (prefs_get_udptest_port()!=0)
	    conn_set_game_port(c,(unsigned short)prefs_get_udptest_port());
	udptest_send(c);
	
	delta = prefs_get_latency();
	data.n = delta;
	if (timerlist_add_timer(c,time(NULL)+(time_t)delta,conn_test_latency,data)<0)
	    eventlog(eventlog_level_error,"conn_set_class","could not add timer");
    }
    else if (class==conn_class_bot || class==conn_class_telnet)
    {
	t_packet * rpacket;
	
	if (class==conn_class_bot)
	{
	    if ((delta = prefs_get_nullmsg())>0)
	    {
		data.n = delta;
		if (timerlist_add_timer(c,time(NULL)+(time_t)delta,conn_send_nullmsg,data)<0)
		    eventlog(eventlog_level_error,"conn_set_class","could not add timer");
	    }
	}
	
	conn_send_issue(c);
	
	if (!(rpacket = packet_create(packet_class_raw)))
	    eventlog(eventlog_level_error,"conn_set_class","could not create rpacket");
	else
	{
	    packet_append_ntstring(rpacket,"Username: ");
	    queue_push_packet(conn_get_out_queue(c),rpacket);
	    packet_del_ref(rpacket);
	}
    }
}


extern t_conn_state conn_get_state(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_state","got NULL connection");
        return conn_state_empty;
    }
    
    return c->state;
}


extern void conn_set_state(t_connection * c, t_conn_state state)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_state","got NULL connection");
        return;
    }
    
    c->state = state;
}

#ifdef WITH_BITS
extern void conn_set_sessionid(t_connection * c, unsigned int sessionid)
{
    if (!c)
    {
    	eventlog(eventlog_level_error,"conn_set_sessionid","got NULL connection");
	return;
    }
    c->sessionid = sessionid;
}

extern unsigned int conn_get_sessionid(t_connection const * c)
{
    if (!c)
    {
    	eventlog(eventlog_level_error,"conn_get_sessionid","got NULL connection");
	return 0;
    }
    return c->sessionid;
}

extern int conn_set_bits_game(t_connection * c, unsigned int gameid)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_set_bits_game","got NULL connection");
	return -1;
    }
    /* eventlog(eventlog_level_debug,"conn_set_bits_game","[%d] setting bits_game 0x%08x -> 0x%08x",c->tcp_sock,c->bits_game,gameid); */
    c->bits_game = gameid;
    return 0;
}

extern unsigned int conn_get_bits_game(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_bits_game","got NULL connection");
	return 0;
    }
    return c->bits_game;
}

#endif


extern unsigned int conn_get_sessionkey(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_sessionkey","got NULL connection");
        return 0;
    }
    
    return c->sessionkey;
}


extern unsigned int conn_get_sessionnum(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_sessionnum","got NULL connection");
        return 0;
    }
    
    return c->sessionnum;
}


extern unsigned int conn_get_secret(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_secret","got NULL connection");
        return 0;
    }
    
    return c->secret;
}


extern unsigned int conn_get_addr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_addr","got NULL connection");
        return 0;
    }
    
    return c->tcp_addr;
}


extern unsigned short conn_get_port(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_port","got NULL connection");
        return 0;
    }
    
    return c->tcp_port;
}


extern unsigned int conn_get_local_addr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_local_addr","got NULL connection");
        return 0;
    }
    
    return c->local_addr;
}


extern unsigned short conn_get_local_port(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_local_port","got NULL connection");
        return 0;
    }
    
    return c->local_port;
}


extern unsigned int conn_get_real_local_addr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_real_local_addr","got NULL connection");
        return 0;
    }
    
    return c->real_local_addr;
}


extern unsigned short conn_get_real_local_port(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_real_local_port","got NULL connection");
        return 0;
    }
    
    return c->real_local_port;
}


extern unsigned int conn_get_game_addr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_game_addr","got NULL connection");
        return 0;
    }
    
    return c->udp_addr;
}


extern int conn_set_game_addr(t_connection * c, unsigned int game_addr)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_game_addr","got NULL connection");
        return -1;
    }
    
    c->udp_addr = game_addr;
    return 0;
}


extern unsigned short conn_get_game_port(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_game_port","got NULL connection");
        return 0;
    }
    
    return c->udp_port;
}


extern int conn_set_game_port(t_connection * c, unsigned short game_port)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_game_port","got NULL connection");
        return -1;
    }
    
    c->udp_port = game_port;
    return 0;
}


extern void conn_set_host(t_connection * c, char const * host)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_host","got NULL connection");
        return;
    }
    if (!host)
    {
        eventlog(eventlog_level_error,"conn_set_host","got NULL host");
        return;
    }
    
    if (c->host)
	free((void *)c->host); /* avoid warning */
    if (!(c->host = strdup(host)))
	eventlog(eventlog_level_error,"conn_set_host","could not allocate memory for c->host");
}


extern void conn_set_user(t_connection * c, char const * user)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_user","got NULL connection");
        return;
    }
    if (!user)
    {
        eventlog(eventlog_level_error,"conn_set_user","got NULL user");
        return;
    }
    
    if (c->user)
	free((void *)c->user); /* avoid warning */
    if (!(c->user = strdup(user)))
	eventlog(eventlog_level_error,"conn_set_user","could not allocate memory for c->user");
}


extern void conn_set_owner(t_connection * c, char const * owner)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_owner","got NULL connection");
        return;
    }
    if (!owner)
    {
        eventlog(eventlog_level_error,"conn_set_owner","got NULL owner");
        return;
    }
    
    if (c->owner)
	free((void *)c->owner); /* avoid warning */
    if (!(c->owner = strdup(owner)))
	eventlog(eventlog_level_error,"conn_set_owner","could not allocate memory for c->owner");
}

extern const char * conn_get_user(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_user","got NULL connection");
	return NULL;
    }
    return c->user;
}

extern const char * conn_get_owner(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_user","got NULL connection");
	return NULL;
    }
    return c->owner;
}

extern void conn_set_cdkey(t_connection * c, char const * cdkey)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_cdkey","got NULL connection");
        return;
    }
    if (!cdkey)
    {
        eventlog(eventlog_level_error,"conn_set_cdkey","got NULL cdkey");
        return;
    }
    
    if (c->cdkey)
	free((void *)c->cdkey); /* avoid warning */
    if (!(c->cdkey = strdup(cdkey)))
	eventlog(eventlog_level_error,"conn_set_cdkey","could not allocate memory for c->cdkey");
}


extern char const * conn_get_clientexe(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_clientexe","got NULL connection");
        return NULL;
    }
    
    if (!c->clientexe)
	return "";
    return c->clientexe;
}


extern void conn_set_clientexe(t_connection * c, char const * clientexe)
{
    char const * temp;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_clientexe","got NULL connection");
        return;
    }
    if (!clientexe)
    {
        eventlog(eventlog_level_error,"conn_set_clientexe","got NULL clientexe");
        return;
    }
    
    if (!(temp = strdup(clientexe)))
    {
	eventlog(eventlog_level_error,"conn_set_clientexe","unable to allocate memory for clientexe");
	return;
    }
    if (c->clientexe)
	free((void *)c->clientexe); /* avoid warning */
    c->clientexe = temp;
}


extern char const * conn_get_clientver(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_clientver","got NULL connection");
        return NULL;
    }
    
    if (!c->clientver)
	return "";
    return c->clientver;
}


extern void conn_set_clientver(t_connection * c, char const * clientver)
{
    char const * temp;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_clientver","got NULL connection");
        return;
    }
    if (!clientver)
    {
        eventlog(eventlog_level_error,"conn_set_clientver","got NULL clientver");
        return;
    }
    
    if (!(temp = strdup(clientver)))
    {
	eventlog(eventlog_level_error,"conn_set_clientver","unable to allocate memory for clientver");
	return;
    }
    if (c->clientver)
	free((void *)c->clientver); /* avoid warning */
    c->clientver = temp;
}


extern char const * conn_get_archtag(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_archtag","got NULL connection");
        return NULL;
    }
    
    if (c->class==conn_class_auth && c->bound)
    {
	if (!c->bound->archtag)
	    return "UKWN";
	return c->bound->archtag;
    }
    if (!c->archtag)
	return "UKWN";
    return c->archtag;
}


extern void conn_set_archtag(t_connection * c, char const * archtag)
{
    char const * temp;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_archtag","got NULL connection");
        return;
    }
    if (!archtag)
    {
        eventlog(eventlog_level_error,"conn_set_archtag","[%d] got NULL archtag",conn_get_socket(c));
        return;
    }
    if (strlen(archtag)!=4)
    {
        eventlog(eventlog_level_error,"conn_set_archtag","[%d] got bad archtag",conn_get_socket(c));
        return;
    }
    
    if (!c->archtag || strcmp(c->archtag,archtag)!=0)
	eventlog(eventlog_level_info,"conn_set_archtag","[%d] setting client arch to \"%s\"",conn_get_socket(c),archtag);
    
    if (!(temp = strdup(archtag)))
    {
	eventlog(eventlog_level_error,"conn_set_archtag","[%d] unable to allocate memory for archtag",conn_get_socket(c));
	return;
    }
    if (c->archtag)
	free((void *)c->archtag); /* avoid warning */
    c->archtag = temp;
}


extern char const * conn_get_clienttag(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_clienttag","got NULL connection");
        return NULL;
    }
    
    if (c->class==conn_class_auth && c->bound)
    {
	if (!c->bound->clienttag)
	    return "UKWN";
	return c->bound->clienttag;
    }
    if (!c->clienttag)
	return "UKWN";
    return c->clienttag;
}


extern char const * conn_get_fake_clienttag(t_connection const * c)
{
    char const * clienttag;
    t_account *  account;

    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_fake_clienttag","got NULL connection");
        return NULL;
    }

    account = conn_get_account(c);
    if (account) /* BITS remote connections don't need to have an account */
	if ((clienttag = account_get_strattr(account,"BNET\\fakeclienttag")))
	    return clienttag;
    return conn_get_clienttag(c);
}


extern void conn_set_clienttag(t_connection * c, char const * clienttag)
{
    char const * temp;
    int          needrefresh;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_clienttag","got NULL connection");
        return;
    }
    if (!clienttag)
    {
        eventlog(eventlog_level_error,"conn_set_clienttag","[%d] got NULL clienttag",conn_get_socket(c));
        return;
    }
    if (strlen(clienttag)!=4)
    {
        eventlog(eventlog_level_error,"conn_set_clienttag","[%d] got bad clienttag",conn_get_socket(c));
        return;
    }
    
    if (!c->clienttag || strcmp(c->clienttag,clienttag)!=0)
    {
	eventlog(eventlog_level_info,"conn_set_clienttag","[%d] setting client type to \"%s\"",conn_get_socket(c),clienttag);
	needrefresh = 1;
    }
    else
	needrefresh = 0;
    
    if (!(temp = strdup(clienttag)))
    {
	eventlog(eventlog_level_error,"conn_set_clienttag","[%d] unable to allocate memory for clienttag",conn_get_socket(c));
	return;
    }
    if (c->clienttag)
	free((void *)c->clienttag); /* avoid warning */
    c->clienttag = temp;
    if (needrefresh && c->channel)
	channel_update_flags(c);
}


extern unsigned long conn_get_versionid(t_connection const * c)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_get_versionid","got NULL connection");
	return 0;
    }
    
    return c->versionid;
}


extern int conn_set_versionid(t_connection * c, unsigned long versionid)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_set_versionid","got NULL connection");
	return -1;
    }
    
    c->versionid = versionid;
    return 0;
}


extern int conn_get_tzbias(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_tzbias","got NULL connection");
        return 0;
    }
    
    return c->tzbias;
}


extern void conn_set_tzbias(t_connection * c, int tzbias)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_tzbias","got NULL connection");
        return;
    }
    
    c->tzbias = tzbias;
}


extern void conn_set_account(t_connection * c, t_account * account)
{
    t_connection * other;
    unsigned int   now;
    char const *   tname;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_account","got NULL connection");
        return;
    }
    if (!account)
    {
        eventlog(eventlog_level_error,"conn_set_account","got NULL account");
        return;
    }
#ifdef WITH_BITS
    if (conn_get_class(c)==conn_class_remote) {
	c->account = account;
	return;
    }
#endif
    now = (unsigned int)time(NULL);
    
    if ((other = connlist_find_connection((tname = account_get_name(account)))))
    {
	eventlog(eventlog_level_info,"conn_set_account","[%d] forcing logout of previous login for \"%s\"",conn_get_socket(c),tname);
	other->state = conn_state_destroy;
    }
    account_unget_name(tname);
    
    c->account = account;
    c->state = conn_state_loggedin;
    {
	char const * flagstr;
	
	if ((flagstr = account_get_strattr(account,"BNET\\flags\\initial")))
	    conn_add_flags(c,strtoul(flagstr,NULL,0));
    }
    
    if (account_get_fl_time(c->account)==0)
    {
	account_set_fl_time(c->account,now);
	account_set_fl_connection(c->account,c->tcp_addr);
	account_set_fl_host(c->account,c->host);
	account_set_fl_user(c->account,c->user);
	account_set_fl_clientexe(c->account,c->clientexe);
	account_set_fl_clienttag(c->account,c->clienttag);
	account_set_fl_clientver(c->account,c->clientver);
	account_set_fl_owner(c->account,c->owner);
	account_set_fl_cdkey(c->account,c->cdkey);
    }
    account_set_ll_time(c->account,now);
    account_set_ll_connection(c->account,c->tcp_addr);
    account_set_ll_host(c->account,c->host);
    account_set_ll_user(c->account,c->user);
    account_set_ll_clientexe(c->account,c->clientexe);
    account_set_ll_clienttag(c->account,c->clienttag);
    account_set_ll_clientver(c->account,c->clientver);
    account_set_ll_owner(c->account,c->owner);
    account_set_ll_cdkey(c->account,c->cdkey);
    
    if (c->host)
    {
	free((void *)c->host); /* avoid warning */
	c->host = NULL;
    }
    if (c->user)
    {
	free((void *)c->user); /* avoid warning */
	c->user = NULL;
    }
    if (c->clientexe)
    {
	free((void *)c->clientexe); /* avoid warning */
	c->clientexe = NULL;
    }
    if (c->owner)
    {
	free((void *)c->owner); /* avoid warning */
	c->owner = NULL;
    }
    if (c->cdkey)
    {
	free((void *)c->cdkey); /* avoid warning */
	c->cdkey = NULL;
    }
    
    if (c->botuser)
    {
	free((void *)c->botuser); /* avoid warning */
	c->botuser = NULL;
    }
    
    totalcount++;
    
    watchlist_notify_event(c->account,watch_event_login);
    
    return;
}


extern t_account * conn_get_account(t_connection const * c)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_get_account","got NULL connection");
	return NULL;
    }
    
    if (c->class==conn_class_auth && c->bound)
	return c->bound->account;
    return c->account;
}


extern int conn_set_botuser(t_connection * c, char const * username)
{
    char const * temp;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_set_botuser","got NULL connection");
	return -1;
    }
    if (!username)
    {
	eventlog(eventlog_level_error,"conn_set_botuser","got NULL username");
	return -1;
    }
    
    if (!(temp = strdup(username)))
    {
	eventlog(eventlog_level_error,"conn_set_botuser","unable to duplicate username");
	return -1;
    }
    if (c->botuser)
	free((void *)c->botuser); /* avoid warning */
    
    c->botuser = temp;
    
    return 0;
}


extern char const * conn_get_botuser(t_connection const * c)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_get_botuser","got NULL connection");
	return NULL;
    }
    
    return c->botuser;
}


extern unsigned int conn_get_flags(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_flags","got NULL connection");
        return 0;
    }
    
    return c->flags;
}


extern int conn_set_flags(t_connection * c, unsigned int flags)
{
    unsigned int oldflags;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_flags","got NULL connection");
        return -1;
    }
    oldflags = c->flags;
    c->flags = flags;
    
    if (oldflags!=c->flags && c->channel)
	channel_update_flags(c);
    
    return 0;
}


extern void conn_add_flags(t_connection * c, unsigned int flags)
{
    unsigned int oldflags;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_add_flags","got NULL connection");
        return;
    }
    oldflags = c->flags;
    c->flags |= flags;
    
    if (oldflags!=c->flags && c->channel)
	channel_update_flags(c);
}


extern void conn_del_flags(t_connection * c, unsigned int flags)
{
    unsigned int oldflags;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_del_flags","got NULL connection");
        return;
    }
    oldflags = c->flags;
    c->flags &= ~flags;
    
    if (oldflags!=c->flags && c->channel)
	channel_update_flags(c);
}


extern unsigned int conn_get_latency(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_latency","got NULL connection");
        return 0;
    }
    
    return c->latency;
}


extern void conn_set_latency(t_connection * c, unsigned int ms)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_set_latency","got NULL connection");
	return;
    }
    
    c->latency = ms;
    
    if (c->channel)
	channel_update_latency(c);
}


extern char const * conn_get_awaystr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_awaystr","got NULL connection");
        return NULL;
    }
    
    return c->settings.away;
}


extern int conn_set_awaystr(t_connection * c, char const * away)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_awaystr","got NULL connection");
        return -1;
    }
    
    if (c->settings.away)
	free((void *)c->settings.away); /* avoid warning */
    if (!away)
        c->settings.away = NULL;
    else
        if (!(c->settings.away = strdup(away)))
	{
	    eventlog(eventlog_level_error,"conn_set_awaystr","could not allocate away string");
	    return -1;
	}
    
    return 0;
}


extern char const * conn_get_dndstr(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_dndstr","got NULL connection");
        return NULL;
    }
    
    return c->settings.dnd;
}


extern int conn_set_dndstr(t_connection * c, char const * dnd)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_dndstr","got NULL connection");
        return -1;
    }
    
    if (c->settings.dnd)
	free((void *)c->settings.dnd); /* avoid warning */
    if (!dnd)
        c->settings.dnd = NULL;
    else
        if (!(c->settings.dnd = strdup(dnd)))
	{
	    eventlog(eventlog_level_error,"conn_set_dndstr","could not allocate dnd string");
	    return -1;
	}
    
    return 0;
}


extern int conn_add_ignore(t_connection * c, t_account * account)
{
    t_account * * newlist;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_add_ignore","got NULL connection");
        return -1;
    }
    if (!account)
    {
        eventlog(eventlog_level_error,"conn_add_ignore","got NULL account");
        return -1;
    }
    
    if (!(newlist = realloc(c->settings.ignore_list,sizeof(t_account const *)*(c->settings.ignore_count+1))))
    {
	eventlog(eventlog_level_error,"conn_add_ignore","could not allocate memory for newlist");
	return -1;
    }
    
    newlist[c->settings.ignore_count++] = account;
    c->settings.ignore_list = newlist;
    
    return 0;
}


extern int conn_del_ignore(t_connection * c, t_account const * account)
{
    t_account * * newlist;
    t_account *   temp;
    unsigned int  i;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_del_ignore","got NULL connection");
        return -1;
    }
    if (!account)
    {
        eventlog(eventlog_level_error,"conn_del_ignore","got NULL account");
        return -1;
    }
    
    for (i=0; i<c->settings.ignore_count; i++)
	if (c->settings.ignore_list[i]==account)
	    break;
    if (i==c->settings.ignore_count)
	return -1; /* not in list */
    
    /* swap entry to be deleted with last entry */
    temp = c->settings.ignore_list[c->settings.ignore_count-1];
    c->settings.ignore_list[c->settings.ignore_count-1] = c->settings.ignore_list[i];
    c->settings.ignore_list[i] = temp;
    
    if (c->settings.ignore_count==1) /* some realloc()s are buggy */
    {
	free(c->settings.ignore_list);
	newlist = NULL;
    }
    else
	newlist = realloc(c->settings.ignore_list,sizeof(t_account const *)*(c->settings.ignore_count-1));
    
    c->settings.ignore_count--;
    c->settings.ignore_list = newlist;
    
    return 0;
}


extern int conn_add_watch(t_connection * c, t_account * account)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_add_watch","got NULL connection");
        return -1;
    }
    
    if (watchlist_add_events(c,account,watch_event_login|watch_event_logout|watch_event_joingame|watch_event_leavegame)<0)
	return -1;
    return 0;
}


extern int conn_del_watch(t_connection * c, t_account * account)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_del_watch","got NULL connection");
        return -1;
    }
    
    if (watchlist_del_events(c,account,watch_event_login|watch_event_logout|watch_event_joingame|watch_event_leavegame)<0)
	return -1;
    return 0;
}


extern int conn_check_ignoring(t_connection const * c, char const * me)
{
    unsigned int i;
    t_account *  temp;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_check_ignoring","got NULL connection");
        return -1;
    }
    
    if (!me || !(temp = accountlist_find_account(me)))
	return -1;
    
    if (c->settings.ignore_list)
	for (i=0; i<c->settings.ignore_count; i++)
	    if (c->settings.ignore_list[i]==temp)
		return 1;
    
    return 0;
}


extern t_channel * conn_get_channel(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_channel","got NULL connection");
        return NULL;
    }
    
    return c->channel;
}


extern int conn_set_channel_var(t_connection * c, t_channel * channel)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_channel_var","got NULL connection");
        return -1;
    }
    c->channel = channel;
    return 0;
}


extern int conn_set_channel(t_connection * c, char const * channelname)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_channel","got NULL connection");
        return -1;
    }
    
    if (c->channel)
    {
	channel_del_connection(c->channel,c);
	c->channel = NULL;
    }
    
    if (channelname)
    {
#ifndef WITH_BITS
	unsigned int created = 0;
#endif

	/* if you're entering a channel, make sure they didn't exit a game without telling us */
	if (c->game)
	{
#ifndef WITH_BITS
            game_del_player(conn_get_game(c),c);
#else
            send_bits_game_leave(conn_get_bits_game(c),conn_get_sessionid(c));
	    conn_set_bits_game(c,0);
#endif
	    c->game = NULL;
	}

#ifdef WITH_BITS
	c->channel = channellist_find_channel_by_name(channelname,NULL,NULL);
	if (conn_get_channel(c)) {
	    if (channel_get_permanent(conn_get_channel(c))) {
		/* Oh no, this *could* be a rollover channel. We have to ask the master to put us in one instance of this channel. */
		t_query * q;

		q = query_create(bits_query_type_chat_channel_join_perm);
		if (!q) {
		    eventlog(eventlog_level_error,"conn_set_channel","could not create query");
		    return -1;
		}
		query_attach_conn(q,"client",c);
		send_bits_chat_channel_join_perm_request(query_get_qid(q),conn_get_sessionid(c),conn_get_latency(c),conn_get_flags(c),channelname,conn_get_country(c));
	    } else {
		channel_add_connection(conn_get_channel(c),c);
	    }
	} else {
	    t_query * q;
	    t_packet     * p;
		
	    p = packet_create(packet_class_bits);
	    if (!p) {
		eventlog(eventlog_level_error,"conn_set_channel","could not create packet");
		return -1;
	    }
	    q = query_create(bits_query_type_chat_channel_join_new);
	    if (!q) {
		eventlog(eventlog_level_error,"conn_set_channel","could not create query");
		packet_del_ref(p);
		return -1;
	    }
	    query_attach_conn(q,"client",c);
	    packet_set_size(p,sizeof(t_bits_chat_channel_join_new_request));
	    packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_NEW_REQUEST);
	    bn_int_set(&p->u.bits_chat_channel_join_new_request.qid,query_get_qid(q));
	    bn_int_set(&p->u.bits_chat_channel_join_new_request.flags,conn_get_flags(c));
	    bn_int_set(&p->u.bits_chat_channel_join_new_request.latency,conn_get_latency(c));
	    bn_int_set(&p->u.bits_chat_channel_join_new_request.sessionid,conn_get_sessionid(c));
	    packet_append_string(p,channelname);
	    bits_packet_generic(p,BITS_ADDR_MASTER);
	    send_bits_packet(p);
	    packet_del_ref(p);
	}
	eventlog(eventlog_level_debug,"conn_set_channel","[%d] joined channel \"%s\"",conn_get_socket(c),channelname);
#else
	created = 0;
	if (!(c->channel = channellist_find_channel_by_name(channelname, conn_get_country(c), conn_get_realmname(c))))
	{
	    if (!(c->channel = channel_create(channelname,channelname,NULL,0,1,1,prefs_get_chanlog(), NULL, NULL, -1)))
	    {
		eventlog(eventlog_level_error,"conn_set_channel","[%d] could not create channel on join \"%s\"",conn_get_socket(c),channelname);
		return -1;
	    }
	    created = 1;
	}
	
	if (channel_add_connection(conn_get_channel(c),c)<0)
	{
	    if (created)
		channel_destroy(c->channel);
	    c->channel = NULL;
            return -1;
	}
	eventlog(eventlog_level_info,"conn_set_channel","[%d] joined channel \"%s\"",conn_get_socket(c),channel_get_name(c->channel));
#endif
	conn_send_welcome(c);
    }
    
    return 0;
}


extern t_game * conn_get_game(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_game","got NULL connection");
        return NULL;
    }
    
    return c->game;
}


extern int conn_set_game(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_game","got NULL connection");
        return -1;
    }

#ifdef WITH_BITS
    eventlog(eventlog_level_debug,"conn_set_game","passing to conn_set_game_bits() with game_option_none");
    return conn_set_game_bits(c,gamename,gamepass,gameinfo,type,version,game_option_none);
#else    
    if (c->game)
    {
        if (gamename && !strcasecmp(gamename,game_get_name(c->game)))
        {
             eventlog(eventlog_level_error,"conn_set_game","[%d] tried to join a new game \"%s\" while already in a game \"%s\"!",conn_get_socket(c),gamename,game_get_name(c->game));
             return 0;
        }
        game_del_player(conn_get_game(c),c);
        c->game = NULL;
    }
    if (gamename)
    {
	if (!(c->game = gamelist_find_game(gamename,type)))
        { 
	    c->game = game_create(gamename,gamepass,gameinfo,type,version,conn_get_clienttag(c));
            if (c->game && conn_get_realmname(c) && conn_get_charname(c))
            {
                  game_set_realmname(c->game,conn_get_realmname(c));
                  realm_add_game_number(realmlist_find_realm(conn_get_realmname(c)),1);
	    }
	}
	if (c->game)
        {
            game_parse_info(c->game,gameinfo);
	    if (game_add_player(conn_get_game(c),gamepass,version,c)<0)
	    {
		c->game = NULL; /* bad password or version # */
		return -1;
	    }
    }
    }
    else
	c->game = NULL;
    return 0;
#endif /* !WITH_BITS */
}

#ifdef WITH_BITS
extern int conn_set_game_bits(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version, t_game_option option)
{

    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_game_bits","got NULL connection");
        return -1;
    }
    
    if (conn_get_bits_game(c)!=0) {
	/* We have no t_game on BITS clients so we have to do it manually */
	eventlog(eventlog_level_debug,"conn_set_game_bits","\"%s\" leaving bits_game %d",conn_get_chatname(c),conn_get_bits_game(c));
	send_bits_game_leave(conn_get_sessionid(c),conn_get_bits_game(c));
	conn_set_bits_game(c,0);
	c->game = NULL;
    }
    if (gamename)
    {
	t_bits_game *game;
	
	game = bits_gamelist_find_game(gamename,type);
	if (game) {
	    eventlog(eventlog_level_debug,"conn_set_game_bits","\"%s\" joining bits_game %d",conn_get_chatname(c),bits_game_get_id(game));
	    send_bits_game_join_request(conn_get_sessionid(c),bits_game_get_id(game),version);
	    conn_set_bits_game(c,bits_game_get_id(game));
	} else {
	    eventlog(eventlog_level_debug,"conn_set_game_bits","\"%s\" creating new bits_game",conn_get_chatname(c));
	    send_bits_game_create_request(gamename,gamepass,gameinfo,type,version,conn_get_sessionid(c),option);
	}
    } else {
	/* The following is just to be sure :) */
	c->game = NULL;
	conn_set_bits_game(c,0); /* also clear BITS game */
    }
    return 0;
}
#endif /* WITH_BITS */

extern t_queue * * conn_get_in_queue(t_connection * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_in_queue","got NULL connection");
        return NULL;
    }
    
    return &c->inqueue;
}


extern int conn_get_in_size(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_in_size","got NULL connection");
        return -1;
    }
    
    return c->insize;
}


extern void conn_set_in_size(t_connection * c, unsigned int size)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_in_size","got NULL connection");
        return;
    }
    
    c->insize = size;
}


extern t_queue * * conn_get_out_queue(t_connection * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_out_queue","got NULL connection");
        return NULL;
    }
#ifdef WITH_BITS
    if (c->bits) /* bits loopback */
	if (c->bits->type == bits_loop)
		return &c->inqueue;
#endif
    return &c->outqueue;
}


extern int conn_get_out_size(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_out_size","got NULL connection");
        return -1;
    }
#ifdef WITH_BITS
    if (c->bits) /* bits loopback */
	if (c->bits->type == bits_loop)
	    return 0;
#endif
    return c->outsize;
}


extern void conn_set_out_size(t_connection * c, unsigned int size)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_out_size","got NULL connection");
        return;
    }
#ifdef WITH_BITS
    if (c->bits) /* bits loopback */
	if (c->bits->type == bits_loop)
	    c->outsize = 0;
#endif
    
    c->outsize = size;
}


extern char const * conn_get_username(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_username","got NULL connection");
        return NULL;
    }
    
    if (conn_get_class(c)==conn_class_remote) {
#ifdef TESTUNGET
	return strdup(conn_get_botuser(c));
#else
	return conn_get_botuser(c);
#endif
    }
    if (c->class==conn_class_auth && c->bound)
	return account_get_name(c->bound->account);
    return account_get_name(c->account);
}


extern void conn_unget_username(t_connection const * c, char const * name)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_unget_username","got NULL connection");
        return;
    }
    
    account_unget_name(name);
}


extern char const * conn_get_chatname(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_chatname","got NULL connection");
        return NULL;
    }
    
    if (conn_get_class(c)==conn_class_remote)
    {
#ifdef TESTUNGET
	return strdup(conn_get_botuser(c));
#else
	return conn_get_botuser(c);
#endif
    }
    if ((c->class==conn_class_auth || c->class==conn_class_bnet) && c->bound)
    {
	if (c->character)
	    return character_get_name(c->character);
	if (c->bound->character)
	    return character_get_name(c->bound->character);
        eventlog(eventlog_level_error,"conn_get_chatname","[%d] got connection class %s bound to class %d without a character",conn_get_socket(c),conn_class_get_str(c->class),c->bound->class);
    }
    if (!c->account)
	return NULL; /* no name yet */
    return account_get_name(c->account);
}


extern int conn_unget_chatname(t_connection const * c, char const * name)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_unget_chatname","got NULL connection");
        return -1;
    }
    
    if ((c->class==conn_class_auth || c->class==conn_class_bnet) && c->bound)
	return 0;
    return account_unget_name(name);
}


extern char const * conn_get_chatcharname(t_connection const * c, t_connection const * dst)
{
    char const * accname;
    char const * clienttag;
    char *       chatcharname;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_chatcharname","got NULL connection");
        return NULL;
    }

    if (conn_get_class(c)==conn_class_remote)
    {
#ifdef TESTUNGET
        return strdup(conn_get_botuser(c));
#else
        return conn_get_botuser(c);
#endif
    }
    
    if (!c->account)
        return NULL; /* no name yet */
    
    /* for D2 Users */
    accname = account_get_name(c->account);
    if (!accname)
        return NULL;
    
    if (dst)
        clienttag = conn_get_clienttag(dst);
    else
        clienttag = NULL;
    
    if (clienttag && ((strcmp(clienttag, CLIENTTAG_DIABLO2DV) == 0) || (strcmp(clienttag, CLIENTTAG_DIABLO2XP) == 0)))
    {   
        if (c->charname)
        {
            if ((chatcharname = malloc(strlen(c->charname) + strlen(accname) + 2)))
        	sprintf(chatcharname, "%s*%s", c->charname, accname);
        }
        else
        {
            if ((chatcharname = malloc(strlen(accname) + 2)))
        	sprintf(chatcharname, "*%s", accname);
        }
    }
    else
        chatcharname = strdup(accname);
    
    account_unget_name(accname);
    return chatcharname;
}


extern int conn_unget_chatcharname(t_connection const * c, char const * name)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_unget_chatcharname","got NULL connection");
        return -1;
    }
    if (!name)
    {
	eventlog(eventlog_level_error,"conn_unget_chatcharname","got NULL name");
	return -1;
    }
    
    if (conn_get_class(c)==conn_class_remote)
    {
#ifdef TESTUNGET
        free((void *)name); /* avoid warning */
#endif
        return 0;
    }
    
    free((void *)name); /* avoid warning */
    return 0;
}


extern unsigned int conn_get_userid(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_userid","got NULL connection");
        return 0;
    }
    
    return account_get_uid(c->account);
}


extern int conn_get_socket(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_socket","got NULL connection");
        return -1;
    }
    
    return c->tcp_sock;
}


extern int conn_get_game_socket(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_game_socket","got NULL connection");
        return -1;
    }
    
    return c->udp_sock;
}


extern int conn_set_game_socket(t_connection * c, int usock)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_game_socket","got NULL connection");
        return -1;
    }
    
    c->udp_sock = usock;
    return 0;
}


extern char const * conn_get_playerinfo(t_connection const * c)
{
    t_account *  account;
    static char  playerinfo[2048];
    char const * clienttag;
    char         revtag[5];
    
#ifdef WITH_BITS
    /* Playerinfo taken from loginlist cache on bits clients */
    t_bits_loginlist_entry * lle;

    lle = bits_loginlist_bysessionid(conn_get_sessionid(c));
    if (!lle) {
	eventlog(eventlog_level_error,"conn_get_playerinfo","requested playerinfo for non-existent sessionid 0x%08x",conn_get_sessionid(c));
	if (!bits_master) return NULL;
    }
    if (!bits_master) {
	return lle->playerinfo;
    }
#endif
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_playerinfo","got NULL connection");
        return NULL;
    }
    if (!(account = conn_get_account(c)))
    {
	eventlog(eventlog_level_error,"conn_get_playerinfo","connection has no account");
	return NULL;
    }
    
    if (!(clienttag = conn_get_fake_clienttag(c)))
    {
	eventlog(eventlog_level_error,"conn_get_playerinfo","connection has NULL fakeclienttag");
	return NULL;
    }
    strcpy(revtag,clienttag);
    strreverse(revtag);
    
    if (strcmp(clienttag,CLIENTTAG_BNCHATBOT)==0)
    {
	strcpy(playerinfo,revtag); /* FIXME: what to return here? */
	eventlog(eventlog_level_debug,"conn_get_playerinfo","got CHAT clienttag, using best guess");
    }
    else if (strcmp(clienttag,CLIENTTAG_STARCRAFT)==0)
    {
	sprintf(playerinfo,"%s %u %u %u %u %u",
		revtag,
		account_get_ladder_rating(account,clienttag,ladder_id_normal),
		account_get_ladder_rank(account,clienttag,ladder_id_normal),
		account_get_normal_wins(account,clienttag),
		0,0);
    }
    else if (strcmp(clienttag,CLIENTTAG_BROODWARS)==0)
    {
	sprintf(playerinfo,"%s %u %u %u %u %u",
		revtag,
		account_get_ladder_rating(account,clienttag,ladder_id_normal),
		account_get_ladder_rank(account,clienttag,ladder_id_normal),
		account_get_normal_wins(account,clienttag),
		0,0);
    }
    else if (strcmp(clienttag,CLIENTTAG_SHAREWARE)==0)
    {
	sprintf(playerinfo,"%s %u %u %u %u %u",
		revtag,
		account_get_ladder_rating(account,clienttag,ladder_id_normal),
		account_get_ladder_rank(account,clienttag,ladder_id_normal),
		account_get_normal_wins(account,clienttag),
		0,0);
    }
    else if (strcmp(clienttag,CLIENTTAG_DIABLORTL)==0)
    {
	sprintf(playerinfo,"%s %u %u %u %u %u %u %u %u %u",
		revtag,
		account_get_normal_level(account,clienttag),
		account_get_normal_class(account,clienttag),
		account_get_normal_diablo_kills(account,clienttag),
		account_get_normal_strength(account,clienttag),
		account_get_normal_magic(account,clienttag),
		account_get_normal_dexterity(account,clienttag),
		account_get_normal_vitality(account,clienttag),
		account_get_normal_gold(account,clienttag),
		0);
    }
    else if (strcmp(clienttag,CLIENTTAG_DIABLOSHR)==0)
    {
	sprintf(playerinfo,"%s %u %u %u %u %u %u %u %u %u",
		revtag,
		account_get_normal_level(account,clienttag),
		account_get_normal_class(account,clienttag),
		account_get_normal_diablo_kills(account,clienttag),
		account_get_normal_strength(account,clienttag),
		account_get_normal_magic(account,clienttag),
		account_get_normal_dexterity(account,clienttag),
		account_get_normal_vitality(account,clienttag),
		account_get_normal_gold(account,clienttag),
		0);
    }
    else if (strcmp(clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	unsigned int a,b;
	
	a = account_get_ladder_rating(account,clienttag,ladder_id_normal);
	b = account_get_ladder_rating(account,clienttag,ladder_id_ironman);
	
	sprintf(playerinfo,"%s %u %u %u %u %u %u %u %u",
		revtag,
		a,
		account_get_ladder_rank(account,clienttag,ladder_id_normal),
		account_get_normal_wins(account,clienttag),
		0,
		0,
		(a>b) ? a : b,
		b,
		account_get_ladder_rank(account,clienttag,ladder_id_ironman));
    }
    else if ((strcmp(clienttag,CLIENTTAG_DIABLO2DV)==0) || (strcmp(clienttag,CLIENTTAG_DIABLO2XP)==0))
#if 0
    /* FIXME: Was this the old code? Can this stuff vanish? */
    /* Yes, this was the pre-d2close code, however I'm not sure the new code
     * takes care of all the cases. */
    {
	t_character * ch;
	
	if (c->character)
	    ch = c->character;
	else
	    if ((c->class==conn_class_auth || c->class==conn_class_bnet) && c->bound && c->bound->character)
		ch = c->bound->character;
	    else
		ch = NULL;
	    
	if (ch)
	    sprintf(playerinfo,"%s%s,%s,%s",
		    revtag,
		    character_get_realmname(ch),
		    character_get_name(ch),
		    character_get_playerinfo(ch));
	else
	    strcpy(playerinfo,revtag); /* open char */
    }
    else /* FIXME: this used to return the empty string... do some formats actually use that or not? */
	strcpy(playerinfo,revtag); /* best guess... */
   }
#endif
   {
       /* This sets portrait of character */
       if (!conn_get_realmname(c) || !conn_get_realminfo(c))
       {
           bn_int_tag_set((bn_int *)playerinfo,clienttag); /* FIXME: Is this attempting to reverse the tag?  This isn't really correct... why not use the revtag stuff like above or below? */
           playerinfo[strlen(clienttag)]='\0';
       }
       else
       {
           strcpy(playerinfo,conn_get_realminfo(c));
       }
    }
    else
        strcpy(playerinfo,revtag); /* open char */
    
#ifdef WITH_BITS
    if ((!lle->playerinfo)||(strcmp(playerinfo,lle->playerinfo)!=0)) {
	/* Update playerinfo if different from loginlist playerinfo or loginlist playerinfo is not yet set */
	send_bits_va_update_playerinfo(conn_get_sessionid(c),playerinfo,NULL);
	bits_loginlist_set_playerinfo(conn_get_sessionid(c),playerinfo);
    }
#endif
    
    return playerinfo;
}


extern int conn_set_playerinfo(t_connection const * c, char const * playerinfo)
{
    char const * clienttag;
    
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_playerinfo","got NULL connection");
        return -1;
    }
    if (!playerinfo)
    {
	eventlog(eventlog_level_error,"conn_set_playerinfo","got NULL playerinfo");
	return -1;
    }
    if (!(clienttag = conn_get_clienttag(c)))
    {
	eventlog(eventlog_level_error,"conn_set_playerinfo","connection has NULL clienttag");
	return -1;
    }
    
    if (strcmp(clienttag,CLIENTTAG_DIABLORTL)==0)
    {
	unsigned int level;
	unsigned int class;
	unsigned int diablo_kills;
	unsigned int strength;
	unsigned int magic;
	unsigned int dexterity;
	unsigned int vitality;
	unsigned int gold;
	
	if (sscanf(playerinfo,"LTRD %u %u %u %u %u %u %u %u %*u",
		   &level,
		   &class,
		   &diablo_kills,
		   &strength,
		   &magic,
		   &dexterity,
		   &vitality,
		   &gold)!=8)
	{
	    eventlog(eventlog_level_error,"conn_set_playerinfo","got bad playerinfo");
	    return -1;
	}
	
	account_set_normal_level(conn_get_account(c),clienttag,level);
	account_set_normal_class(conn_get_account(c),clienttag,class);
	account_set_normal_diablo_kills(conn_get_account(c),clienttag,diablo_kills);
	account_set_normal_strength(conn_get_account(c),clienttag,strength);
	account_set_normal_magic(conn_get_account(c),clienttag,magic);
	account_set_normal_dexterity(conn_get_account(c),clienttag,dexterity);
	account_set_normal_vitality(conn_get_account(c),clienttag,vitality);
	account_set_normal_gold(conn_get_account(c),clienttag,gold);
    }
    else if (strcmp(clienttag,CLIENTTAG_DIABLOSHR)==0)
    {
	unsigned int level;
	unsigned int class;
	unsigned int diablo_kills;
	unsigned int strength;
	unsigned int magic;
	unsigned int dexterity;
	unsigned int vitality;
	unsigned int gold;
	
	if (sscanf(playerinfo,"RHSD %u %u %u %u %u %u %u %u %*u",
		   &level,
		   &class,
		   &diablo_kills,
		   &strength,
		   &magic,
		   &dexterity,
		   &vitality,
		   &gold)!=8)
	{
	    eventlog(eventlog_level_error,"conn_set_playerinfo","got bad playerinfo");
	    return -1;
	}
	
	account_set_normal_level(conn_get_account(c),clienttag,level);
	account_set_normal_class(conn_get_account(c),clienttag,class);
	account_set_normal_diablo_kills(conn_get_account(c),clienttag,diablo_kills);
	account_set_normal_strength(conn_get_account(c),clienttag,strength);
	account_set_normal_magic(conn_get_account(c),clienttag,magic);
	account_set_normal_dexterity(conn_get_account(c),clienttag,dexterity);
	account_set_normal_vitality(conn_get_account(c),clienttag,vitality);
	account_set_normal_gold(conn_get_account(c),clienttag,gold);
    }
    else if (strcmp(clienttag,CLIENTTAG_DIABLO2DV)==0)
    {
	/* not much to do */ /* FIXME: get char name here? */
	eventlog(eventlog_level_error,"conn_set_playerinfo","[%d] unhandled playerinfo request for client \"%s\" playerinfo=\"%s\"",conn_get_socket(c),clienttag,playerinfo);
    }
    else if (strcmp(clienttag,CLIENTTAG_DIABLO2XP)==0)
    {
        /* in playerinfo we get strings of the form "Realmname,charname" */
	eventlog(eventlog_level_error,"conn_set_playerinfo","[%d] unhandled playerinfo request for client \"%s\" playerinfo=\"%s\"",conn_get_socket(c),clienttag,playerinfo);
    }
    else
    {
	eventlog(eventlog_level_warn,"conn_set_playerinfo","setting playerinfo for client \"%s\" not supported (playerinfo=\"%s\")",clienttag,playerinfo);
	return -1;
    }
    
#ifdef WITH_BITS
    /* Check if the playerinfo has to be updated in the network */
    conn_get_playerinfo(c);
#endif
    
    return 0;
}


extern char const * conn_get_realminfo(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_realminfo","got NULL connection");
        return NULL;
    }
    return c->realminfo;
}


extern int conn_set_realminfo(t_connection * c, char const * realminfo)
{
    char const * temp;

    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_realminfo","got NULL connection");
        return -1;
    }
    
    if (realminfo)
    {
	if (!(temp = strdup(realminfo)))
	{
	    eventlog(eventlog_level_error,"conn_set_realminfo","could not allocate memory for new realminfo");
	    return -1;
	}
    }
    else
      temp = NULL;
    
    if (c->realminfo) /* if it was set before, free it now */
	free((void *)c->realminfo); /* avoid warning */
    c->realminfo = temp;
    return 0;
}


extern char const * conn_get_charname(t_connection const * c)
{
    if (!c)
    {
       eventlog(eventlog_level_error,"conn_get_charname","got NULL connection");
       return NULL;
    }
    return c->charname;
}


extern int conn_set_charname(t_connection * c, char const * charname)
{
    char const * temp;

    if (!c)
    {
       eventlog(eventlog_level_error,"conn_set_charname","got NULL connection");
       return -1;
    }
    
    if (charname)
    {
	if (!(temp = strdup(charname)))
	{
	    eventlog(eventlog_level_error,"conn_set_charname","could not allocate memory for new charname");
	    return -1;
	}
    }    
    else
	temp = charname;

    if (c->charname) /* free it, if it was previously set */
       free((void *)c->charname); /* avoid warning */
    c->charname = temp;
    return 0;
}


extern int conn_set_idletime(t_connection * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_idletime","got NULL connection");
        return -1;
    }
    
    c->last_message = time(NULL);
    return 0;
}


extern unsigned int conn_get_idletime(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_idletime","got NULL connection");
        return 0;
    }
    
    return (unsigned int)difftime(time(NULL),c->last_message);
}


extern char const * conn_get_realmname(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_realmname","got NULL connection");
        return NULL;
    }
    
    if (c->class==conn_class_auth && c->bound)
	return c->bound->realmname;
    return c->realmname;
}


extern int conn_set_realmname(t_connection * c, char const * realmname)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_realmname","got NULL connection");
        return -1;
    }
    
    if (c->realmname)
	free((void *)c->realmname); /* avoid warning */
    if (!realmname)
        c->realmname = NULL;
    else
        if (!(c->realmname = strdup(realmname)))
	{
	    eventlog(eventlog_level_error,"conn_set_realmname","could not allocate realmname string");
	    return -1;
	}
	else
	    eventlog(eventlog_level_debug,"conn_set_realmname","[%d] set to \"%s\"",conn_get_socket(c),realmname);
    
    return 0;
}


extern int conn_set_character(t_connection * c, t_character * character)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_character","got NULL connection");
        return -1;
    }
    if (!character)
    {
        eventlog(eventlog_level_error,"conn_set_character","got NULL character");
	return -1;
    }
    
    c->character = character;
    
    return 0;
}


extern void conn_set_country(t_connection * c, char const * country)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_set_country","got NULL connection");
        return;
    }
    if (!country)
    {
        eventlog(eventlog_level_error,"conn_set_country","got NULL country");
        return;
    }
    
    if (c->country)
	free((void *)c->country); /* avoid warning */
    if (!(c->country = strdup(country)))
	eventlog(eventlog_level_error,"conn_set_country","could not allocate memory for c->country");
}


extern char const * conn_get_country(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_country","got NULL connection");
        return NULL;
    }
    
    return c->country;
}


extern int conn_bind(t_connection * c1, t_connection * c2)
{
    if (!c1)
    {
        eventlog(eventlog_level_error,"conn_bind","got NULL connection");
        return -1;
    }
    if (!c2)
    {
        eventlog(eventlog_level_error,"conn_bind","got NULL connection");
        return -1;
    }
    
    c1->bound = c2;
    c2->bound = c1;
    
    return 0;
}


extern int conn_set_ircline(t_connection * c, char const * line)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_set_ircline","got NULL connection");
	return -1;
    }
    if (!line) {
	eventlog(eventlog_level_error,"conn_set_ircline","got NULL line");
	return -1;
    }
    if (c->ircline)
    	free((void *)c->ircline); /* avoid warning */
    if (!(c->ircline = strdup(line)))
	eventlog(eventlog_level_error,"conn_set_ircline","could not allocate memory for c->ircline");
    return 0;
}


extern char const * conn_get_ircline(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_ircline","got NULL connection");
	return NULL;
    }
    return c->ircline;
}


extern int conn_set_ircpass(t_connection * c, char const * pass)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_set_ircpass","got NULL connection");
	return -1;
    }
    if (c->ircpass)
    	free((void *)c->ircpass); /* avoid warning */
    if (!pass)
    	c->ircpass = NULL;
    else
	if (!(c->ircpass = strdup(pass)))
	    eventlog(eventlog_level_error,"conn_set_ircpass","could not allocate memory for c->ircpass");
    return 0;
}


extern char const * conn_get_ircpass(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_ircpass","got NULL connection");
	return NULL;
    }
    return c->ircpass;
}


extern int conn_set_ircping(t_connection * c, unsigned int ping)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_set_ircping","got NULL connection");
	return -1;
    }
    c->ircping = ping;
    return 0;
}


extern unsigned int conn_get_ircping(t_connection const * c)
{
    if (!c) {
	eventlog(eventlog_level_error,"conn_get_ircping","got NULL connection");
	return -1;
    }
    return c->ircping;
}


extern int connlist_create(void)
{
    if (!(conn_head = list_create()))
	return -1;
    return 0;
}


extern int connlist_destroy(void)
{
    /* FIXME: if called with active connection, connection are not freed */
    if (list_destroy(conn_head)<0)
	return -1;
    conn_head = NULL;
    return 0;
}


extern t_list * connlist(void)
{
    return conn_head;
}


extern t_connection * connlist_find_connection(char const * username)
{
    t_connection *    c;
    t_account const * temp;
    t_elem const *    curr;
    
    if (!username)
    {
	eventlog(eventlog_level_error,"connlist_find_connection","got NULL username");
	return NULL;
    }
    
    if (!(temp = accountlist_find_account(username)))
	return NULL;
    
    LIST_TRAVERSE_CONST(conn_head,curr)
    {
	c = elem_get_data(curr);
	if (c->account==temp)
	    return c;
    }
    
    return NULL;
}


extern t_connection * connlist_find_connection_by_sessionkey(unsigned int sessionkey)
{
    t_connection * c;
    t_elem const * curr;
    
    LIST_TRAVERSE_CONST(conn_head,curr)
    {
	c = elem_get_data(curr);
	if (c->sessionkey==sessionkey)
	    return c;
    }
    
    return NULL;
}


extern t_connection * connlist_find_connection_by_sessionnum(unsigned int sessionnum)
{
    t_connection * c;
    t_elem const * curr;
    
    LIST_TRAVERSE_CONST(conn_head,curr)
    {
	c = elem_get_data(curr);
	if (c->sessionnum==sessionnum)
	    return c;
    }
    
    return NULL;
}

extern t_connection * connlist_find_connection_by_name(char const * name, char const * realmname)
{
    char           charname[CHAR_NAME_LEN];
    char const *   temp;
    unsigned int   n;
    t_connection * found;

    if (!name) {
	eventlog(eventlog_level_error,"connlist_find_connection_by_name","got NULL name");
	return NULL;
    }
    if (!name[0]) {
	eventlog(eventlog_level_error,"connlist_find_connection_by_name","got empty name");
	return NULL;
    }
    
    found = NULL;
    /* If * is a username */
    if (name[0]=='*')
    {
        name++;
        found = connlist_find_connection(name);
    }
    /* If @ is a charname */
    else if (name[0] == '@')
    {
        name++;
        found = connlist_find_connection_by_charname(name,realmname);
    }
    /* Is charname*username */
    else
    {
        if ((temp=strchr(name,'*')))
        {
             n = temp-name;
             if (n>=CHAR_NAME_LEN)
             	return NULL;
             strncpy(charname,name,n);
             charname[n] = '\0';
             realmname = temp+1;
             found = connlist_find_connection_by_charname(charname,realmname);
        }
        else
        {
             found = connlist_find_connection(name);
        }
   }
   return found;
}

extern t_connection * connlist_find_connection_by_charname(char const * charname, char const * realmname)
{
     t_connection    * c;
     t_elem const    * curr;

     if (!realmname) {
	eventlog(eventlog_level_error,"connlist_find_connection_by_charname","got NULL realmname");
     	return NULL;
     }
     LIST_TRAVERSE_CONST(conn_head, curr)
     {
        c = elem_get_data(curr);
        if (!c)
            continue;
        if (!c->charname)
            continue;
        if (!c->realmname)
            continue;
        if ((strcasecmp(c->charname, charname)==0)&&(strcasecmp(c->realmname,realmname)==0))
            return c;
     }
     return NULL;
}


#ifdef WITH_BITS
extern t_connection * connlist_find_connection_by_sessionid(unsigned int sessionid)
{
    t_connection * c;
    t_elem const * curr;
    
    LIST_TRAVERSE_CONST(conn_head,curr)
    {
	c = elem_get_data(curr);
	if (c->sessionid==sessionid)
	    return c;
    }
    
    return NULL;
}
#endif

extern int connlist_get_length(void)
{
    return list_get_length(conn_head);
}


extern int connlist_login_get_length(void)
{
    t_connection const * c;
    int                  count;
    t_elem const *       curr;
    
    count = 0;
    LIST_TRAVERSE_CONST(conn_head,curr)
    {
	c = elem_get_data(curr);
	if ((c->state==conn_state_loggedin)&&
	    ((c->class==conn_class_bnet)||(c->class==conn_class_bot)||(c->class==conn_class_telnet)||(c->class==conn_class_irc)))
	    count++;
    }
    
    return count;
}


extern int connlist_total_logins(void)
{
    return totalcount;
}


extern int conn_quota_exceeded(t_connection * con, char const * text)
{
    time_t    now;
    t_qline * qline;
    t_elem *  curr;
    int       needpurge;
    
    if (!prefs_get_quota()) return 0;
    if (strlen(text)>prefs_get_quota_maxline())
    {
	message_send_text(con,message_type_error,con,"Your line length quota has been exceeded!");
	return 1;
    }
    
    now = time(NULL);
    
    needpurge = 0;
    LIST_TRAVERSE(con->settings.quota.list,curr)
    {
	qline = elem_get_data(curr);
        if (now>=qline->inf+(time_t)prefs_get_quota_time())
	{
	    /* these lines are at least quota_time old */
	    list_remove_elem(con->settings.quota.list,curr);
	    if (qline->count>con->settings.quota.totcount)
		eventlog(eventlog_level_error,"conn_quota_exceeded","qline->count=%u but con->settings.quota.totcount=%u",qline->count,con->settings.quota.totcount);
	    con->settings.quota.totcount -= qline->count;
	    free(qline);
	    needpurge = 1;
	}
	else
	    break; /* old items are first, so we know nothing else will match */
    }
    if (needpurge)
	list_purge(con->settings.quota.list);
    
    if ((qline = malloc(sizeof(t_qline)))==NULL)
    {
	eventlog(eventlog_level_error,"conn_quota_exceeded","could not allocate qline");
	return 0;
    }
    qline->inf = now; /* set the moment */
    if (strlen(text)>prefs_get_quota_wrapline()) /* round up on the divide */
	qline->count = (strlen(text)+prefs_get_quota_wrapline()-1)/prefs_get_quota_wrapline();
    else
	qline->count = 1;
    
    if (list_append_data(con->settings.quota.list,qline)<0)
    {
	eventlog(eventlog_level_error,"conn_quota_exceeded","could not append to list");
	free(qline);
	return 0;
    }
    con->settings.quota.totcount += qline->count;
    
    if (con->settings.quota.totcount>=prefs_get_quota_lines())
    {
	message_send_text(con,message_type_error,con,"Your message quota has been exceeded!");
	if (con->settings.quota.totcount>=prefs_get_quota_dobae())
	{
	    /* kick out the dobae user for violation of the quota rule */
	    conn_set_state(con,conn_state_destroy);
	    if (con->channel)
		channel_message_log(con->channel,con,0,"DISCONNECTED FOR DOBAE ABUSE");
	    return 2;
	}
	return 1;
    }
    
    return 0;
}


extern int conn_set_lastsender(t_connection * c, char const * sender)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_set_lastsender","got NULL conection");
	return -1;
    }
    if (c->lastsender)
	free((void *)c->lastsender); /* avoid warning */
    if (!sender)
    {
	c->lastsender = NULL;
	return 0;
    }
    if (!(c->lastsender = strdup(sender)))
    {
	eventlog(eventlog_level_error,"conn_set_lastsender","could not allocate memory for c->lastsender");
	return -1;
    }
    
    return 0;
}


extern char const * conn_get_lastsender(t_connection const * c)
{
    if (!c) 
    {
	eventlog(eventlog_level_error,"conn_get_lastsender","got NULL connection");
	return NULL;
    }
    return c->lastsender;
}


extern t_versioncheck const * conn_get_versioncheck(t_connection const * c)
{
    if (!c)
    {
        eventlog(eventlog_level_error,"conn_get_versioncheck","got NULL connection");
        return NULL;
    }
    
    return c->versioncheck;
}


extern int conn_set_versioncheck(t_connection * c, t_versioncheck const * versioncheck)
{
    if (!c) 
    {
	eventlog(eventlog_level_error,"conn_set_versioncheck","got NULL connection");
	return -1;
    }
    if (!versioncheck) 
    {
	eventlog(eventlog_level_error,"conn_set_versioncheck","got NULL versioncheck");
	return -1;
    }
    
    c->versioncheck = versioncheck;
    
    return 0;
}


extern int conn_set_udpok(t_connection * c)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"conn_set_udpok","got NULL connection");
	return -1;
    }
    
    if (!c->udpok)
    {
	c->udpok = 1;
	c->flags &= ~MF_PLUG;
    }
    
    return 0;
}

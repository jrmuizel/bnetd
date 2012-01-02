/*
 * Copyright (C) 1999,2000,2001  Marco Ziech (mmz@gmx.net)
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
#ifdef WITH_BITS
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
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
#include <errno.h>
#include "compat/strerror.h"
#include "compat/strcasecmp.h"
#include "common/eventlog.h"
#include "common/bnethash.h"
#include "common/list.h"
#include "account.h"
#include "common/queue.h"
#include "connection.h"
#include "common/packet.h"
#include "common/bn_type.h"
#include "common/util.h"
#include "common/tag.h"
#include "bits.h"
#include "bits_va.h"
#include "bits_packet.h"
#include "bits_ext.h"
#include "bits_query.h"
#include "bits_login.h"
#include "common/setup_after.h"

t_bits_loginlist_entry * bits_loginlist = NULL;
int current_sessionid = 1; /* sessionid 0 is special and means "none" */

extern int send_bits_va_logout(int sessionid) {
    t_packet * p;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_set_attr","packet create failed");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_va_logout));
    packet_set_type(p,BITS_VA_LOGOUT);
    bn_int_set(&p->u.bits_va_logout.sessionid,sessionid);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int bits_loginlist_add(t_uint32 uid, t_uint16 loginhost, t_uint32 sessionid, bn_int const clienttag, unsigned int game_addr, unsigned short game_port, char const *chatname)
{
	t_bits_loginlist_entry * lle = bits_loginlist;
	char temp[5];

	if (!chatname) {
		eventlog(eventlog_level_error,"bits_loginlist_add","got NULL chatname");
		return -1;
	}
	while (lle) {
		if (lle->uid == uid) {
			eventlog(eventlog_level_debug,"bits_loginlist_add","user " UID_FORMAT " already logged in",uid);
			return 1;
		}
		lle = lle->next;
	}
	memcpy(&temp,clienttag,4);
	temp[4] = '\0';
	eventlog(eventlog_level_debug,"bits_loginlist_add","adding user (uid=#%u;loginhost=0x%04x;sessionid=0x%08x;clienttag=\"%s\";chatname=\"%s\")",uid,loginhost,sessionid,temp,chatname);
	lle = malloc(sizeof(t_bits_loginlist_entry));
	if (!lle) {
		eventlog(eventlog_level_error,"bits_loginlist_add","malloc failed: %s",strerror(errno));
		return -1;
	}
	lle->next = bits_loginlist;
	if (lle->next) lle->next->prev = lle;
	lle->prev = NULL;
	lle->uid = uid;
	memcpy(&lle->clienttag,clienttag,4);

	memcpy(&temp,lle->clienttag,4);
	temp[4]='\0';

	lle->sessionid = sessionid;
	lle->loginhost = loginhost;
	lle->game_addr = game_addr;
	lle->game_port = game_port;
	strncpy(lle->chatname,chatname,USER_NAME_MAX);
	lle->chatname[USER_NAME_MAX] = '\0';
	lle->playerinfo = NULL;
	bits_loginlist = lle;
	if (!bits_uplink_connection)
	    send_bits_va_login(lle,NULL);
	return 0;
}

extern int bits_loginlist_del(t_uint32 sessionid)
{
	t_bits_loginlist_entry * lle = bits_loginlist;

	while (lle) {
		if (lle->sessionid == sessionid) {
			if (!lle->prev) {
				bits_loginlist = lle->next;
			} else {
				lle->prev->next = lle->next;
			}
			if (lle->next) lle->next->prev = lle->prev;
			if (!bits_uplink_connection)
			    send_bits_va_masterlogout(lle,NULL);
			if (lle->playerinfo) free(lle->playerinfo);
			free(lle);
			eventlog(eventlog_level_debug,"bits_loginlist_del","removing user (sessionid=0x%08x)",sessionid);
			return 0;
		};
		lle = lle->next;
	}
	eventlog(eventlog_level_debug,"bits_loginlist_del","user with sessionid=0x%08x not found in loginlist",sessionid);
	return 1;
}

extern int bits_loginlist_set_playerinfo(unsigned int sessionid, char const * playerinfo)
{
	t_bits_loginlist_entry * lle;

	if (!playerinfo) {
		eventlog(eventlog_level_error,"bits_loginlist_set_playerinfo","got NULL playerinfo");
		return -1;
	}
	lle = bits_loginlist_bysessionid(sessionid);
	if (!lle) {
		eventlog(eventlog_level_warn,"bits_loginlist_set_playerinfo","user with sessionid 0x%08x not found",sessionid);
		return -1;
	}
	if (lle->playerinfo) free(lle->playerinfo);
	lle->playerinfo = strdup(playerinfo);
	return 0;
}

extern int send_bits_va_update_playerinfo(unsigned int sessionid, char const * playerinfo, t_connection * c)
{
    t_packet *p;
    
    if (!playerinfo) {
	eventlog(eventlog_level_error,"send_bits_va_update_playerinfo","got NULL playerinfo");
	return -1;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_update_playerinfo","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_va_update_playerinfo));
    packet_set_type(p,BITS_VA_UPDATE_PLAYERINFO);
    bits_packet_generic(p,BITS_ADDR_BCAST);
    bn_int_set(&p->u.bits_va_update_playerinfo.sessionid,sessionid);
    packet_append_string(p,playerinfo);
    if (c)
        send_bits_packet_on(p,c);
    else
    	send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_va_masterlogout(t_bits_loginlist_entry * lle, t_connection * c)
{
    t_packet *p;

    if (!lle) {
	eventlog(eventlog_level_error,"send_bits_va_logout","got NULL loginlist entry");
	return -1;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_logout","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_va_login));
    packet_set_type(p,BITS_VA_LOGOUT);
    bits_packet_generic(p,BITS_ADDR_BCAST);
    bn_int_set(&p->u.bits_va_logout.sessionid,lle->sessionid);
    if (c)
    	send_bits_packet_on(p,c);
    else
    	send_bits_packet(p);
    packet_del_ref(p);
    return 0;    
}

extern int send_bits_va_login(t_bits_loginlist_entry * lle, t_connection * c)
{
	t_packet *p;
	
	if (!lle) {
		eventlog(eventlog_level_error,"send_bits_va_login","got NULL loginlist entry");
		return -1;
	}
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_va_login","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_va_login));
	packet_set_type(p,BITS_VA_LOGIN);
	bits_packet_generic(p,BITS_ADDR_BCAST);
	bn_short_set(&p->u.bits_va_login.host,lle->loginhost);
	bn_int_set(&p->u.bits_va_login.uid,lle->uid);
	bn_int_set(&p->u.bits_va_login.sessionid,lle->sessionid);
	bn_int_set(&p->u.bits_va_login.game_addr,lle->game_addr);
	bn_short_set(&p->u.bits_va_login.game_port,lle->game_port);
	memcpy(&p->u.bits_va_login.clienttag,lle->clienttag,4);
	packet_append_string(p,lle->chatname);
    	if (c)
    	    send_bits_packet_on(p,c);
      	else
    	    send_bits_packet(p);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_va_loginreq(unsigned int qid, unsigned int uid, unsigned int gameaddr, unsigned short gameport, char const * clienttag)
{
    t_packet * p;
    char const * ct = CLIENTTAG_BNCHATBOT;

    if (!clienttag) {
    	eventlog(eventlog_level_error,"send_bits_va_loginreq","got NULL clienttag, falling back to \"%s\"",ct);
	/* fall back to default */
    } else {
	ct = clienttag;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_loginreq","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_va_loginreq));
    packet_set_type(p,BITS_VA_LOGINREQ);
    bn_int_set(&p->u.bits_va_loginreq.qid,qid);
    bn_int_set(&p->u.bits_va_loginreq.uid,uid);
    bn_short_set(&p->u.bits_va_loginreq.game_port,gameport);
    bn_int_set(&p->u.bits_va_loginreq.game_addr,gameaddr);
    memcpy(&p->u.bits_va_loginreq.clienttag,ct,4);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int bits_va_loginlist_get_free_sessionid(void)
{
    /* FIXME: may wrap (ok, may take a while ;) */
    if (current_sessionid == 0)
        current_sessionid++;
    return current_sessionid++;
}

extern t_bits_loginlist_entry * bits_loginlist_bysessionid(unsigned int sessionid)
{
    t_bits_loginlist_entry * lle;

    lle = bits_loginlist;
    while (lle) {
	if (lle->sessionid == sessionid) return lle;
	lle = lle->next;
    }
    return NULL;
}

extern t_bits_loginlist_entry * bits_loginlist_byname(char const * name)
{
    t_bits_loginlist_entry * lle;

    if (!name)
    {
    	eventlog(eventlog_level_error,"bits_loginlist_byname","got NULL name");
	return NULL;
    }
    lle = bits_loginlist;
    while (lle) {
	if (strcasecmp(lle->chatname,name)==0) return lle;
	lle = lle->next;
    }
    return NULL;
}

extern char const * bits_loginlist_get_name_bysessionid(unsigned int sessionid)
{
    t_bits_loginlist_entry * lle;

    lle = bits_loginlist;
    while (lle) {
	if (lle->sessionid==sessionid) return lle->chatname;
	lle = lle->next;
    }
    return NULL;
}

extern int bits_va_loginlist_sendall(t_connection * c)
{
    t_bits_loginlist_entry * lle;

    if (!c)
    {
    	eventlog(eventlog_level_error,"bits_va_loginlist_sendall","got NULL connection");
	return -1;
    }
    lle = bits_loginlist;
    while (lle) {
	send_bits_va_login(lle,c);
	if (lle->playerinfo)
	    send_bits_va_update_playerinfo(lle->sessionid,lle->playerinfo,c);
	lle = lle->next;
    }
    return 0;
}

#endif /* WITH_BITS */


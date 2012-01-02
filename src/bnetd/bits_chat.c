/*
 * Copyright (C) 2000  Marco Ziech (mmz@gmx.net)
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
#include <errno.h>
#include "compat/strerror.h"
#include "compat/strdup.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "common/eventlog.h"
#include "account.h"
#include "common/packet.h"
#include "channel.h"
#include "common/list.h"
#include "message.h"
#include "connection.h"
#include "common/bn_type.h"
#include "bits.h"
#include "bits_packet.h"
#include "bits_chat.h"
#include "bits_query.h"
#include "bits_login.h"
#include "common/setup_after.h"

extern int bits_chat_user(char const * srcname, char const *destname, int type, unsigned int latency, unsigned int flags, char const * text)
{
    t_bits_loginlist_entry * slle;
    t_bits_loginlist_entry * dlle;

    
    if (!srcname)
    {
	eventlog(eventlog_level_error,"bits_chat_user","got NULL srcname");
	return -1;
    }
    if (!destname)
    {
	eventlog(eventlog_level_error,"bits_chat_user","got NULL destname");
	return -1;
    }
    slle = bits_loginlist_byname(srcname);
    if (!slle)
    { /* Might be a bug or a very slow connection */
	t_connection * c;

    	eventlog(eventlog_level_error,"bits_chat_user","sender is not logged in");
	c = connlist_find_connection(srcname);
	if (!c) {
	    eventlog(eventlog_level_error,"bits_chat_user","sender does not have a connection to this server");
            return -1;
	}
	/* send a nice message to the user */
	message_send_text(c,message_type_error,c,"You don't exist. Go Away! ;)");	
	return -1;
    }
    dlle = bits_loginlist_byname(destname);
    if (!dlle)
    { /* User does not exist */
	t_connection * c;

	c = connlist_find_connection(srcname);
	if (!c) {
	    eventlog(eventlog_level_error,"bits_chat_user","sender does not have a connection to this server");
            return -1;
	}
	message_send_text(c,message_type_error,c,"This user is not logged on.");
	return 1;
    }    
    send_bits_chat_user(slle->sessionid,dlle->sessionid,type,latency,flags,text);
    return 0;
}

extern int send_bits_chat_user(int src, int dst, int type, unsigned int latency, unsigned int flags, char const * text)
{
    t_bits_loginlist_entry * lle;
    t_packet *p;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_chat_user","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_chat_user));
    packet_set_type(p,BITS_CHAT_USER);
    bn_int_set(&p->u.bits_chat_user.type,type);
    bn_int_set(&p->u.bits_chat_user.src,src);
    bn_int_set(&p->u.bits_chat_user.dst,dst);
    bn_int_set(&p->u.bits_chat_user.cflags,flags);
    bn_int_set(&p->u.bits_chat_user.clatency,latency);
    if (text) {
	packet_append_string(p,text);
    }
    lle = bits_loginlist_bysessionid(dst);
    if (lle)
    {  /* only send it to the host (may reduce network traffic) */
	bits_packet_generic(p,lle->loginhost);
    } else {
	/* broadcast it (usually should not happen) */
	eventlog(eventlog_level_warn,"send_bits_chat_user","broadcasting packet");
	bits_packet_generic(p,BITS_ADDR_BCAST);
    }
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_chat_channellist_add(t_channel * channel, t_connection * conn)
{
	t_packet *p;
	
	if (!channel) {
		eventlog(eventlog_level_error,"send_bits_chat_channellist_add","got NULL channel");
		return -1;
	}
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channellist_add","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channellist_add));
	packet_set_type(p,BITS_CHAT_CHANNELLIST_ADD);
	packet_append_string(p,channel_get_name(channel));
	if (channel_get_shortname(channel))
		packet_append_string(p,channel_get_shortname(channel));
	bn_int_set(&p->u.bits_chat_channellist_add.channelid,channel_get_channelid(channel));
	if (conn) {
		bits_packet_generic(p,BITS_ADDR_PEER);
		send_bits_packet_on(p,conn);
	} else {
		bits_packet_generic(p,BITS_ADDR_BCAST);
		send_bits_packet(p);
	}
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channellist_del(int channelid)
{
	t_packet * p;

	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_channellist_del","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channellist_del));
	packet_set_type(p,BITS_CHAT_CHANNELLIST_DEL);
	bits_packet_generic(p,BITS_ADDR_PEER);
	bn_int_set(&p->u.bits_chat_channellist_del.channelid,channelid);
	send_bits_packet_down(p);
	packet_del_ref(p);
	return 0;
}


extern int bits_chat_channellist_send_all(t_connection * conn)
{
	t_channel * channel;
	t_elem const * curr;
	
	LIST_TRAVERSE_CONST(channellist(),curr)
	{
		channel = elem_get_data(curr);
		send_bits_chat_channellist_add(channel,conn);
	}
	return 0;
}

extern int send_bits_chat_channel_server_join(int channelid)
{
	t_packet * p;

	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_server_join","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel_server_join));
	packet_set_type(p,BITS_CHAT_CHANNEL_SERVER_JOIN);
	bn_int_set(&p->u.bits_chat_channel_server_join.channelid,channelid);
	bits_packet_generic(p,BITS_ADDR_PEER);
	send_bits_packet_up(p);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channel_server_leave(int channelid)
{
	t_packet * p;

	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_server_leave","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel_server_leave));
	packet_set_type(p,BITS_CHAT_CHANNEL_SERVER_LEAVE);
	bn_int_set(&p->u.bits_chat_channel_server_leave.channelid,channelid);
	bits_packet_generic(p,BITS_ADDR_PEER);
	send_bits_packet_up(p);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channel_join(t_connection * conn, int channelid, t_bits_channelmember *member)
{
	t_packet * p;

	if (!member) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_join","got NULL member");
		return -1;
	}
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_join","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel_join));
	packet_set_type(p,BITS_CHAT_CHANNEL_JOIN);
	bn_int_set(&p->u.bits_chat_channel_join.channelid,channelid);
	bn_int_set(&p->u.bits_chat_channel_join.sessionid,member->sessionid);
	bn_int_set(&p->u.bits_chat_channel_join.flags,member->flags);
	bn_int_set(&p->u.bits_chat_channel_join.latency,member->latency);
	bits_packet_generic(p,BITS_ADDR_PEER);
	if (conn)
	    send_bits_packet_on(p,conn);
	else
	    send_bits_packet_for_channel(p,channelid,bits_uplink_connection);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channel_leave(int channelid, unsigned int sessionid)
{
	t_packet * p;

	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_leave","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel_leave));
	packet_set_type(p,BITS_CHAT_CHANNEL_LEAVE);
	bn_int_set(&p->u.bits_chat_channel_leave.channelid,channelid);
	bn_int_set(&p->u.bits_chat_channel_leave.sessionid,sessionid);
	bits_packet_generic(p,BITS_ADDR_PEER);
	send_bits_packet_for_channel(p,channelid,bits_uplink_connection);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channel(t_bits_channelmember * member, int channelid, int type, char const * text)
{
	t_packet * p;
	
	if (!member) {
		eventlog(eventlog_level_error,"send_bits_chat_channel","got NULL member");
		return -1;
	}

	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel));
	packet_set_type(p,BITS_CHAT_CHANNEL);
	bits_packet_generic(p,BITS_ADDR_PEER);
	bn_int_set(&p->u.bits_chat_channel.channelid,channelid);
	bn_int_set(&p->u.bits_chat_channel.type,type);
	bn_int_set(&p->u.bits_chat_channel.sessionid,member->sessionid);
	bn_int_set(&p->u.bits_chat_channel.flags,member->flags);
	bn_int_set(&p->u.bits_chat_channel.latency,member->latency);
	if (text)
		packet_append_string(p,text);
	send_bits_packet_for_channel(p,channelid,NULL);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_chat_channel_join_perm_request(t_uint32 qid, unsigned int sessionid, unsigned int latency, unsigned int flags, char const * name, char const * country)
{
	t_packet * p;
	
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_chat_channel_join_perm_request","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_chat_channel_join_perm_request));
	packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_PERM_REQUEST);
	bits_packet_generic(p,BITS_ADDR_MASTER);
	bn_int_set(&p->u.bits_chat_channel_join_perm_request.qid,qid);
	bn_int_set(&p->u.bits_chat_channel_join_perm_request.sessionid,sessionid);
	bn_int_set(&p->u.bits_chat_channel_join_perm_request.flags,flags);
	bn_int_set(&p->u.bits_chat_channel_join_perm_request.latency,latency);
	packet_append_string(p,name);
	packet_append_string(p,country);
	send_bits_packet(p);
	packet_del_ref(p);
	return 0;
}

#endif /* WITH_BITS */

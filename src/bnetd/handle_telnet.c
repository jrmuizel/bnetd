/*
 * Copyright (C) 2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
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
#ifdef BITS
# include "compat/memcpy.h"
#endif
#include <ctype.h>
#include "common/packet.h"
#include "common/tag.h"
#include "message.h"
#include "common/eventlog.h"
#include "command.h"
#include "account.h"
#include "connection.h"
#include "channel.h"
#include "common/queue.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/list.h"
#ifdef WITH_BITS
# include "query.h"
# include "bits.h"
# include "bits_va.h"
# include "bits_login.h"
# include "bits_query.h"
# include "bits_packet.h"
#endif
#include "handle_telnet.h"
#include "common/setup_after.h"


extern int handle_telnet_packet(t_connection * c, t_packet const * const packet)
{
    t_packet * rpacket;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"handle_telnet_packet","[%d] got NULL connection",conn_get_socket(c));
	return -1;
    }
    if (!packet)
    {
	eventlog(eventlog_level_error,"handle_telnet_packet","[%d] got NULL packet",conn_get_socket(c));
	return -1;
    }
    if (packet_get_class(packet)!=packet_class_raw)
    {
        eventlog(eventlog_level_error,"handle_telnet_packet","[%d] got bad packet (class %d)",conn_get_socket(c),(int)packet_get_class(packet));
        return -1;
    }
    
    {
	char const * const linestr=packet_get_str_const(packet,0,MAX_MESSAGE_LEN);
	
	if (packet_get_size(packet)<2) /* empty line */
	    return 0;
	if (!linestr)
	{
	    eventlog(eventlog_level_warn,"handle_telnet_packet","[%d] line too long",conn_get_socket(c));
	    return 0;
	}
	
	switch (conn_get_state(c))
	{
	case conn_state_connected:
	    conn_add_flags(c,MF_PLUG);
	    conn_set_clienttag(c,CLIENTTAG_BNCHATBOT);
	    
	    {
		char const * temp=linestr;
		
		if (temp[0]=='\004') /* FIXME: no echo, ignore for now (we always do no echo) */
		    temp = &temp[1];
		
		if (temp[0]=='\0') /* empty line */
		{
		    conn_set_state(c,conn_state_bot_username); /* don't look for ^D or reset tag and flags */
		    break;
		}
		
		conn_set_state(c,conn_state_bot_password);
		
		if (conn_set_botuser(c,temp)<0)
		    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not set username to \"%s\"",conn_get_socket(c),temp);
		
		{
		    char const * const msg="\r\nPassword: ";
		    
		    if (!(rpacket = packet_create(packet_class_raw)))
		    {
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			break;
		    }
#if 0 /* don't echo */
		    packet_append_ntstring(rpacket,conn_get_botuser(c));
#endif
		    packet_append_ntstring(rpacket,msg);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case conn_state_bot_username:
	    conn_set_state(c,conn_state_bot_password);
	    
	    if (conn_set_botuser(c,linestr)<0)
		eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not set username to \"%s\"",conn_get_socket(c),linestr);
	    
	    {
		char const * const temp="\r\nPassword: ";
	    	
		if (!(rpacket = packet_create(packet_class_raw)))
		{
		    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
		    break;
		}
#if 0 /* don't echo */
		packet_append_ntstring(rpacket,linestr);
#endif
		packet_append_ntstring(rpacket,temp);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case conn_state_bot_password:
	    {
		char const * const tempa="\r\nLogin failed.\r\n\r\nUsername: ";
		char const * const tempb="\r\nAccount has no bot access.\r\n\r\nUsername: ";
		char const * const botuser=conn_get_botuser(c);
		t_account *        account;
		char const *       oldstrhash1;
		t_hash             trypasshash1;
		t_hash             oldpasshash1;
		char const *       tname;
		char *             testpass;
		
		if (!botuser) /* error earlier in login */
		{
		    /* no log message... */
		    conn_set_state(c,conn_state_bot_username);
		    
		    if (!(rpacket = packet_create(packet_class_raw)))
		    {
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			break;
		    }
		    
		    packet_append_ntstring(rpacket,tempa);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		    break;
		}
		if (connlist_find_connection(botuser))
		{
		    eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (already logged in)",conn_get_socket(c),botuser);
		    conn_set_state(c,conn_state_bot_username);
		    
		    if (!(rpacket = packet_create(packet_class_raw)))
		    {
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			break;
		    }
		    
		    packet_append_ntstring(rpacket,tempa);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		    break;
		}
		if (!(account = accountlist_find_account(botuser)))
		{
#ifdef WITH_BITS
		    t_query * q;
		    
		    if (!bits_master) {
			bits_va_lock_account(botuser);
			if (!(account = accountlist_find_account(botuser))) {
			    eventlog(eventlog_level_warn,"handle_telnet_packet","account is still NULL");
			    conn_set_state(c,conn_state_bot_username);
		    
		    	    if (!(rpacket = packet_create(packet_class_raw)))
		    	    {
				eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
				break;
		    	    }
		    
		    	    packet_append_ntstring(rpacket,tempa);
		    	    queue_push_packet(conn_get_out_queue(c),rpacket);
		    	    packet_del_ref(rpacket);
		    	    break;
			} else {
			    q = query_create(bits_query_type_handle_packet_account_loaded);
			    query_attach_conn(q,"client",c);
			    query_attach_packet_const(q,"request",packet);
			    query_attach_account(q,"account",account);
			}
		    } else { /* bits master */
#endif
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (bad account)",conn_get_socket(c),botuser);
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempa);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
#ifdef WITH_BITS
		    } /* bits master */
#endif
		    break;
		}
#ifdef WITH_BITS
		if (account_is_invalid(account)) {
		    eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (bad account)",conn_get_socket(c),botuser);
		    conn_set_state(c,conn_state_bot_username);
		    
		    if (!(rpacket = packet_create(packet_class_raw)))
		    {
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			break;
		    }
		    
		    packet_append_ntstring(rpacket,tempa);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		    break;
		}
		if (!account_is_ready_for_use(account))
		    break; /* wait ... */
#endif
		if ((oldstrhash1 = account_get_pass(account)))
		{
		    if (hash_set_str(&oldpasshash1,oldstrhash1)<0)
		    {
			account_unget_pass(oldstrhash1);
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (corrupted passhash1?)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempa);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
		    account_unget_pass(oldstrhash1);
		    
		    if (!(testpass = strdup(linestr)))
			break;
		    {
			unsigned int i;
			
			for (i=0; i<strlen(testpass); i++)
			    if (isupper((int)testpass[i]))
				testpass[i] = tolower((int)testpass[i]);
		    }
		    if (bnet_hash(&trypasshash1,strlen(testpass),testpass)<0) /* FIXME: force to lowercase */
		    {
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (unable to hash password)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			free(testpass);
			
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempa);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
		    free(testpass);
		    if (hash_eq(trypasshash1,oldpasshash1)!=1)
		    {
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (wrong password)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempa);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
		    
		    
		    if (account_get_auth_botlogin(account)!=1) /* default to false */
		    {
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (no bot access)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempb);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
		    else if (account_get_auth_lock(account)==1) /* default to false */
		    {
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (this account is locked)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			conn_set_state(c,conn_state_bot_username);
			
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			
			packet_append_ntstring(rpacket,tempb);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
		    
		    eventlog(eventlog_level_info,"handle_telnet_packet","[%d] \"%s\" bot logged in (correct password)",conn_get_socket(c),(tname = account_get_name(account)));
		    account_unget_name(tname);
		}
		else
		{
		    eventlog(eventlog_level_info,"handle_telnet_packet","[%d] \"%s\" bot logged in (no password)",conn_get_socket(c),(tname = account_get_name(account)));
		    account_unget_name(tname);
		}
#ifdef WITH_BITS
		if (!bits_master)
		{
		    t_query *q;
		    
		    q = query_create(bits_query_type_bot_loginreq);
		    if (!q) {
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create query",conn_get_socket(c));
			break; /* FIXME: connection will hang */
		    }
		    query_attach_conn(q,"client",c);
		    query_attach_account(q,"account",account);
		    send_bits_va_loginreq(query_get_qid(q),account_get_uid(account),0,0,CLIENTTAG_BNCHATBOT);
		} else { /* bits master */
		    bn_int ct;
		    int sid;

		    if (conn_get_clienttag(c))
		        memcpy(&ct,conn_get_clienttag(c),4);
		    else
		        bn_int_set(&ct,0);
		    if (bits_loginlist_add(account_get_uid(account),BITS_ADDR_MASTER,(sid = bits_va_loginlist_get_free_sessionid()),ct,0,0,botuser)==1) {
			eventlog(eventlog_level_info,"handle_telnet_packet","[%d] bot login for \"%s\" refused (already logged in (bits))",conn_get_socket(c),botuser);
			conn_set_state(c,conn_state_bot_username);
			conn_set_sessionid(c,sid);
			if (!(rpacket = packet_create(packet_class_raw)))
			{
			    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
			    break;
			}
			packet_append_ntstring(rpacket,tempa);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			break;
		    }
#endif /*WITH_BITS*/
		    if (!(rpacket = packet_create(packet_class_raw))) /* if we got this far, let them log in even if this fails */
			eventlog(eventlog_level_error,"handle_telnet_packet","[%d] could not create rpacket",conn_get_socket(c));
		    else
		    {
			packet_append_ntstring(rpacket,"\r\n");
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		    }
		    message_send_text(c,message_type_uniqueid,c,(tname = account_get_name(account)));
		    account_unget_name(tname);
		    
		    conn_set_account(c,account);
		    
		    if (conn_set_channel(c,CHANNEL_NAME_CHAT)<0)
			conn_set_channel(c,CHANNEL_NAME_BANNED); /* should not fail */
#ifdef WITH_BITS
		}
#endif /*WITH_BITS*/
	    }
	    break;
	    
	case conn_state_loggedin:
	    {
		t_channel const * channel;
		
		conn_set_idletime(c);
		
		if ((channel = conn_get_channel(c)))
		    channel_message_log(channel,c,1,linestr);
		/* we don't log game commands currently */
		
		if (linestr[0]=='/')
		    handle_command(c,linestr);
		else
		    if (channel && !conn_quota_exceeded(c,linestr))
			channel_message_send(channel,message_type_talk,c,linestr);
		    /* else discard */
	    }
	    break;
	    
	default:
	    eventlog(eventlog_level_error,"handle_telnet_packet","[%d] unknown telnet connection state %d",conn_get_socket(c),(int)conn_get_state(c));
	}
    }
    
    return 0;
}

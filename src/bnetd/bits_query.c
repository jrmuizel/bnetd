/*
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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
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
#include "common/packet.h"
#include "connection.h"
#include "command.h"
#include "channel.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/network.h"
#include "account.h"
#include "common/list.h"
#include "common/addr.h"
#include "server.h"
#include "handle_bnet.h"
#include "handle_init.h"
#include "handle_bot.h"
#include "handle_telnet.h"
#include "handle_auth.h"
#include "handle_file.h"
#include "query.h"
#include "bits.h"
#include "common/bn_type.h"
#include "message.h"
#include "common/bits_protocol.h"
#include "common/bnet_protocol.h"
#include "bits_packet.h"
#include "bits_va.h"
#include "bits_login.h"
#include "bits_query.h"
#include "bits_chat.h"
#include "bits_rconn.h"
#include "bits_game.h"
#include "common/setup_after.h"

int bits_query_type_bits_va_create = 0; 
int bits_query_type_bits_va_lock = 0;
int bits_query_type_client_loginreq_1 = 0;
int bits_query_type_client_loginreq_2 = 0;
int bits_query_type_handle_command_account_locked = 0;
int bits_query_type_handle_command_account_loaded = 0;
int bits_query_type_handle_packet_account_loaded = 0;
int bits_query_type_bot_loginreq = 0;
int bits_query_type_chat_channel_join = 0;
int bits_query_type_chat_channel_join_new = 0;
int bits_query_type_chat_channel_join_perm = 0;
int bits_query_type_bits_auth = 0;

/***********************************************************/
/***                  BITS QUERY HANDLERS                ***/
/***********************************************************/

/* 
 * A query handler should usually follow the following layout:

static t_query_rc query_handler_foo_bar_wait(t_query *q, t_query_reason reason)
{
    t_connection *client;
    ... (more types) ...

    client = bits_query_get_attachment_conn(q,"client");
    ... (more initialisations) ...

    ... if your query has an error handler handler, don't check ...
    if (!client) {
	eventlog(eventlog_level_error,"query_handler_foo_bar_wait","client is NULL");
	return query_rc_error;
    }
    ... (more checks) ...

    switch (reason) {
	case query_reason_tick:
	... (called on every tick) ...
	case query_reason_condition:
	... (called if a certain condition is set) ...
	case query_reason_delete:
	... (free some special space) ...
	default:
	    return query_rc_unknown;
    }
}

 * This might help to understand older query handlers easier. They
 * should be structured more or less like this scheme. 
 * 						- Marco
 */

static t_query_rc query_handler_bits_va_create(t_query *q, t_query_reason reason)
{
    t_connection * c;
    t_packet * reply;

    c = query_get_attachment_conn(q,"client");
    reply = query_get_attachment_packet(q,"reply");

    switch (reason) {
    case query_reason_condition:
    	if (bits_packet_get_src(reply) != BITS_ADDR_MASTER) {
	    eventlog(eventlog_level_warn,"bits_query_process","got reply from client(0x%04x). Drop.",bits_packet_get_src(reply));
	    return query_rc_delete;
    	}
	if (bits_status_ok(reply->u.bits_va_create_reply.status)) {
	    t_packet * rpacket;
	
	    eventlog(eventlog_level_debug,"bits_query_process","got positive account_create_reply from master");
	    rpacket = packet_create(packet_class_bnet);
	    if (!rpacket) {
	    	eventlog(eventlog_level_error,"bits_query_process","could not create packet");
	    	return query_rc_delete;
	    }
	    packet_set_size(rpacket,sizeof(t_server_createacctreply1));
	    packet_set_type(rpacket,SERVER_CREATEACCTREPLY1);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown1,SERVER_CREATEACCTREPLY1_UNKNOWN1);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown2,SERVER_CREATEACCTREPLY1_UNKNOWN2);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown3,SERVER_CREATEACCTREPLY1_UNKNOWN3);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown4,SERVER_CREATEACCTREPLY1_UNKNOWN4);
	    bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_OK);
	    queue_push_packet(conn_get_out_queue(c),rpacket);
	    packet_del_ref(rpacket);
	    return query_rc_delete;
	} else {
	    t_packet *rpacket;

	    eventlog(eventlog_level_debug,"bits_query_process","got negative account_create_reply from master");
	    rpacket = packet_create(packet_class_bnet);
	    if (!rpacket) {
		eventlog(eventlog_level_error,"bits_query_process","could not create packet");
		return query_rc_delete;
	    }
	    packet_set_size(rpacket,sizeof(t_server_createacctreply1));
	    packet_set_type(rpacket,SERVER_CREATEACCTREPLY1);
	    bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_NO);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown1,SERVER_CREATEACCTREPLY1_UNKNOWN1);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown2,SERVER_CREATEACCTREPLY1_UNKNOWN2);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown3,SERVER_CREATEACCTREPLY1_UNKNOWN3);
	    bn_int_set(&rpacket->u.server_createacctreply1.unknown4,SERVER_CREATEACCTREPLY1_UNKNOWN4);
	    queue_push_packet(conn_get_out_queue(c),rpacket);
	    packet_del_ref(rpacket);
	    return query_rc_delete;
    	}
    default:
	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_client_loginreq_1(t_query *q, t_query_reason reason)
{
    t_connection * client;
    t_packet * request;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    request = query_get_attachment_packet(q,"request");
    account = query_get_attachment_account(q,"account");
    if (!client) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_1","{%d} client connection is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!request) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_1","{%d} client request is NULL",query_get_qid(q));
	return query_rc_error;
    }			
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_1","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }
    switch (reason) {
    case query_reason_tick:
	if (account_is_ready_for_use(account)) {
	    handle_bnet_packet(client,request);
	    return query_rc_delete;
    	}
	if (account_is_invalid(account)) {
	    t_packet * rpacket;

	    rpacket = packet_create(packet_class_bnet);
	    if (!rpacket) {
		eventlog(eventlog_level_error,"query_handler_client_loginreq_1","could not create packet");
		return query_rc_delete;
	    }
	    packet_set_size(rpacket,sizeof(t_server_loginreply1));
	    packet_set_type(rpacket,SERVER_LOGINREPLY1);
	    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
	    queue_push_packet(conn_get_out_queue(client),rpacket);		    
	    packet_del_ref(rpacket);
	    eventlog(eventlog_level_debug,"query_handler_client_loginreq_1","account "UID_FORMAT": login failed",account_get_uid(account));
	    bits_va_lock_check(); /* kick invalid accounts */
	    return query_rc_delete;
	}
	return query_rc_keep;
    default:
    	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_client_loginreq_2(t_query *q, t_query_reason reason)
{
    t_connection * client;
    t_packet * request;
    t_packet * reply;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    request = query_get_attachment_packet(q,"request");
    reply = query_get_attachment_packet(q,"reply");
    account = query_get_attachment_account(q,"account");
    if (!client) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_2","{%d} client connection is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!request) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_2","{%d} client request is NULL",query_get_qid(q));
	return query_rc_error;
    }			
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_client_loginreq_2","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_condition:
    if ((account_is_ready_for_use(account) && (reply))||(account_is_invalid(account))) {
	if ((account_is_ready_for_use(account))&&(bits_status_ok(reply->u.bits_va_loginreply.status))) {
 	    eventlog(eventlog_level_debug,"query_handler_client_loginreq_2","account "UID_FORMAT": login ok (sessionid=0x%08x)",account_get_uid(account),bn_int_get(reply->u.bits_va_loginreply.sessionid));
	    conn_set_sessionid(client,bn_int_get(reply->u.bits_va_loginreply.sessionid));
	    handle_bnet_packet(client,request);
	} else { /* account invalid */
	    t_packet * rpacket;

	    rpacket = packet_create(packet_class_bnet);
	    if (!rpacket) {
		eventlog(eventlog_level_error,"query_handler_client_loginreq_2","could not create packet");
		return query_rc_delete;
	    }
	    packet_set_size(rpacket,sizeof(t_server_loginreply1));
	    packet_set_type(rpacket,SERVER_LOGINREPLY1);
	    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
	    queue_push_packet(conn_get_out_queue(client),rpacket);		    
	    packet_del_ref(rpacket);
	    eventlog(eventlog_level_debug,"query_handler_client_loginreq_2","account "UID_FORMAT": login failed",account_get_uid(account));
	}
	return query_rc_delete;
    }
    return query_rc_keep;
    default:
        return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_handle_command_account_locked(t_query *q, t_query_reason reason)
{
    t_connection * client;
    char const * command;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    command = query_get_attachment_string(q,"command");
    account = query_get_attachment_account(q,"account");

    if (!client) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_locked","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!command) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_locked","{%d} command is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_locked","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_tick:
	if (account_get_bits_state(account)==account_state_valid) {
	    handle_command(client,command);
	    bits_va_lock_check(); /* kick invalid accounts */
	    return query_rc_delete;
	}
	return query_rc_keep;
    default:
	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_handle_command_account_loaded(t_query *q, t_query_reason reason)
{
    t_connection * client;
    char const * command;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    command = query_get_attachment_string(q,"command");
    account = query_get_attachment_account(q,"account");

    if (!client) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_loaded","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!command) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_loaded","{%d} command is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_handle_command_account_loaded","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_tick:
	if ((account_is_ready_for_use(account))||(account_is_invalid(account))) {
	    handle_command(client,command);
	    bits_va_lock_check(); /* kick invalid accounts */
	    return query_rc_delete;
	}
        return query_rc_keep;
    default:
	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_handle_packet_account_loaded(t_query *q, t_query_reason reason)
{
    t_connection * client;
    t_packet * packet;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    packet = query_get_attachment_packet(q,"request");
    account = query_get_attachment_account(q,"account");

    if (!client) {
	eventlog(eventlog_level_error,"query_handler_handle_packet_account_loaded","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!packet) {
	eventlog(eventlog_level_error,"query_handler_handle_packet_account_loaded","{%d} request is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_handle_packet_account_loaded","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_tick:
	if (!account_state_is_pending(account)) {
	    if ((account_is_ready_for_use(account))||(account_is_invalid(account))) {
		/* FIXME: It would be cool to have a generic handle_packet function here instead of the switch */
		
		switch (conn_get_class(client))
		{
		case conn_class_bits:
	  	    handle_bits_packet(client,packet);
	            break;
		case conn_class_init:
	            handle_init_packet(client,packet);
	            break;
		case conn_class_bnet:
		    handle_bnet_packet(client,packet);
		    break;
		case conn_class_bot:
		    handle_bot_packet(client,packet);
		    break;
		case conn_class_telnet:
		    handle_telnet_packet(client,packet);
		    break;
		case conn_class_file:
		    handle_file_packet(client,packet);
		    break;
		case conn_class_auth:
		    handle_auth_packet(client,packet);
		    break;
		default:
		    handle_bnet_packet(client,packet);
		}
		bits_va_lock_check(); /* kick invalid accounts */
		return query_rc_delete;
	    }
	}
	return query_rc_keep;
    default:
    	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_bot_loginreq(t_query *q, t_query_reason reason)
{
    t_connection * client;
    t_packet * reply;
    t_account * account;

    client = query_get_attachment_conn(q,"client");
    reply = query_get_attachment_packet(q,"reply");
    account = query_get_attachment_account(q,"account");

    if (!client) {
	eventlog(eventlog_level_error,"query_handler_bot_loginreq","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!reply) {
	eventlog(eventlog_level_error,"query_handler_bot_loginreq","{%d} reply is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!account) {
	eventlog(eventlog_level_error,"query_handler_bot_loginreq","{%d} account is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_condition:    
	if (!bits_status_ok(reply->u.bits_va_loginreply.status)) {
		t_packet * rpacket;
		char const * const temp = "\r\nYou are already logged in.\r\n\r\nUsername: ";
		
		conn_set_state(client,conn_state_bot_username);
		rpacket = packet_create(packet_class_raw);
		if (!rpacket)
		{
			eventlog(eventlog_level_error,"query_handler_bot_loginreq","could not create packet");
			return query_rc_delete;
		}
		packet_append_ntstring(rpacket,temp);
		queue_push_packet(conn_get_out_queue(client),rpacket);
		packet_del_ref(rpacket);
	} else {
		t_packet *rpacket;
		
	        if (!(rpacket = packet_create(packet_class_raw))) /* if we got this far, let them log in even if this fails */
		    	eventlog(eventlog_level_error,"query_handler_bot_loginreq","[%d] could not create rpacket",conn_get_socket(client));
   		else
    		{
    			char const * tname;
		    	packet_append_ntstring(rpacket,"\r\n");
			queue_push_packet(conn_get_out_queue(client),rpacket);
			packet_del_ref(rpacket);
			message_send_text(client,message_type_uniqueid,client,(tname = account_get_name(account)));
			account_unget_name(tname);
	    	}
    
   		conn_set_account(client,account);
		conn_set_sessionid(client,bn_int_get(reply->u.bits_va_loginreply.sessionid));
    
	        if (conn_set_channel(client,CHANNEL_NAME_CHAT)<0)
			conn_set_channel(client,CHANNEL_NAME_BANNED); /* should not fail */			
	}
	return query_rc_delete;
    default:
	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_chat_channel_join(t_query *q, t_query_reason reason)
{
    t_channel * channel;
    t_packet * reply;
    t_connection * client;

    client = query_get_attachment_conn(q,"client");
    reply = query_get_attachment_packet(q,"reply");
	
    if (!client) {
	eventlog(eventlog_level_error,"query_handler_chat_channel_join","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!reply) {
	eventlog(eventlog_level_error,"query_handler_chat_channel_join","{%d} reply is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
    case query_reason_condition:
	channel = channellist_find_channel_bychannelid(bn_int_get(reply->u.bits_chat_channel_join_reply.channelid));
	if (bits_status_ok(reply->u.bits_chat_channel_join_reply.status)) {
	    t_bits_channelmember * m;

	    conn_set_channel_var(client,channel);
	    /* if (q->type == query_type_chat_channel_join_new) */ /* Why was this here? (Dumb, I start asking myself ;) ) */
		channel_add_ref(channel);
	    message_send_text(client,message_type_channel,client,channel_get_name(channel));
	    for (m = channel_get_first(channel); m; m = channel_get_next()) {
		t_connection * rconn;
		t_bits_loginlist_entry * lle;

		lle = bits_loginlist_bysessionid(m->sessionid);
		rconn = bits_rconn_create(m->latency,m->flags,lle->sessionid);
		conn_set_botuser(rconn,lle->chatname);
		message_send_text(client,message_type_adduser,rconn,NULL);
		bits_rconn_destroy(rconn);
	    }
	} else {
	    channel_del_ref(channel);
	    if (bn_byte_get(reply->u.bits_chat_channel_join_reply.status)== BITS_STATUS_CHANNELDOESNOTEXIST) {
		message_send_text(client,message_type_channeldoesnotexist,client,channel_get_name(channel));
	    } else if (bn_byte_get(reply->u.bits_chat_channel_join_reply.status)== BITS_STATUS_CHANNELFULL) {
		message_send_text(client,message_type_channelfull,client,channel_get_name(channel));
	    } else if (bn_byte_get(reply->u.bits_chat_channel_join_reply.status)== BITS_STATUS_CHANNELRESTRICTED) {
		message_send_text(client,message_type_channelrestricted,client,channel_get_name(channel));
	    } else {
		/* not full but ... */
		message_send_text(client,message_type_channelfull,client,channel_get_name(channel));
	    }
	}
	return query_rc_delete;
    default:
	return query_rc_unknown;
    }
}

/* ------------------------------------------------------------------- */
static t_query_rc query_handler_bits_auth(t_query *q, t_query_reason reason)
{
    t_connection *client;
    t_packet *p;

    client = query_get_attachment_conn(q,"client");
    p = query_get_attachment_packet(q,"reply");
    if (!client) {
	eventlog(eventlog_level_error,"query_handler_bits_auth","{%d} client is NULL",query_get_qid(q));
	return query_rc_error;
    }
    if (!p) {
	eventlog(eventlog_level_error,"query_handler_bits_auth","{%d} reply is NULL",query_get_qid(q));
	return query_rc_error;
    }

    switch (reason) {
	case query_reason_condition:
	    bits_send_authreply(client,bn_byte_get(p->u.bits_auth_reply.status),bn_short_get(p->u.bits_auth_reply.address),BITS_ADDR_PEER,0);
    	    conn_set_state(client,conn_state_loggedin);
	    bits_setup_client(client);
	    return query_rc_delete;
	case query_reason_error:
	case query_reason_timeout:
	    eventlog(eventlog_level_error,"query_handler_bits_auth","closing connection");
	    conn_destroy(client);
	    return query_rc_delete;
	default:
	    return query_rc_unknown;
    }
}

/********************************************************************/


/************************************************************/
/***               END OF QUERY HANDLER PART              ***/
/************************************************************/

extern void bits_query_init(void) {
    eventlog(eventlog_level_debug,"bits_query_init","bits_query initialized");
    bits_query_type_bits_va_create                = query_register_handler(query_handler_bits_va_create, query_condition_wait, 0,QUERY_TTL_DEFAULT, "bits_query_type_bits_va_create");
    bits_query_type_client_loginreq_1             = query_register_handler(query_handler_client_loginreq_1, query_condition_none, 1, QUERY_TTL_DEFAULT, "bits_query_type_client_loginreq_1");
    bits_query_type_client_loginreq_2             = query_register_handler(query_handler_client_loginreq_2, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_client_loginreq_2");
    bits_query_type_handle_command_account_locked = query_register_handler(query_handler_handle_command_account_locked, query_condition_none,1, QUERY_TTL_DEFAULT, "bits_query_type_handle_command_account_locked");
    bits_query_type_handle_command_account_loaded = query_register_handler(query_handler_handle_command_account_loaded, query_condition_none,1, QUERY_TTL_DEFAULT, "bits_query_type_handle_command_account_loaded");
    bits_query_type_handle_packet_account_loaded  = query_register_handler(query_handler_handle_packet_account_loaded, query_condition_none,1, QUERY_TTL_DEFAULT, "bits_query_type_handle_packet_account_loaded");
    bits_query_type_bot_loginreq                  = query_register_handler(query_handler_bot_loginreq, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_bot_loginreq");
    bits_query_type_chat_channel_join             = query_register_handler(query_handler_chat_channel_join, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_chat_channel_join");
    bits_query_type_chat_channel_join_new         = query_register_handler(query_handler_chat_channel_join, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_chat_channel_join_new");
    bits_query_type_chat_channel_join_perm        = query_register_handler(query_handler_chat_channel_join, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_chat_channel_join_perm");
    bits_query_type_bits_va_lock                  = query_register_handler(NULL, query_condition_never, 0, QUERY_TTL_DEFAULT, "bits_query_type_bits_va_lock"); /* This is a 'dummy' query to save data over some 'ticks' */
    bits_query_type_bits_auth                     = query_register_handler(query_handler_bits_auth, query_condition_wait, 0, QUERY_TTL_DEFAULT, "bits_query_type_bits_auth");
}

extern void bits_query_destroy(void) {
    eventlog(eventlog_level_debug,"bits_query_destroy","bits_query finished");
}

#endif /* WITH_BITS */


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
#include "common/eventlog.h"
#include "common/bnethash.h"
#include "common/list.h"
#include "account.h"
#include "common/queue.h"
#include "connection.h"
#include "common/packet.h"
#include "common/bn_type.h"
#include "common/util.h"
#include "common/hashtable.h"
#include "query.h"
#include "bits.h"
#include "bits_va.h"
#include "bits_packet.h"
#include "bits_ext.h"
#include "bits_query.h"
#include "common/setup_after.h"

extern int bits_va_lock_account(const char *name) {
	t_account * ac;
	t_packet * p;
	t_query * q;
	
	if (!name) {
		eventlog(eventlog_level_error,"bits_va_lock_account","got NULL name");
		return -1;
	}
	if ((ac = accountlist_find_account(name))) {
		if (account_get_bits_state(ac)!=account_state_unknown) {
			eventlog(eventlog_level_warn,"bits_va_lock_account","tried to lock an account which is already locked");
			return 0;
		}
	} else {
		/* create a dummy account */
		ac = create_vaccount(name,0);
		if (!ac) {
			eventlog(eventlog_level_error,"bits_va_lock_account","could not create dummy vaccount");
			return -1;
		}
	}
	p = packet_create(packet_class_bits);
	packet_set_size(p, sizeof(t_bits_va_lock));
	packet_set_type(p, BITS_VA_LOCK);
	bits_packet_generic(p, BITS_ADDR_PEER);
	q = query_create(bits_query_type_bits_va_lock);
	if (!q) {
		eventlog(eventlog_level_error,"bits_va_lock_account","bits_query_push failed.");
		packet_destroy(p);
		return -1;
	}
	/*bits_query_set_processed(q,-1);*/
	bn_int_set(&p->u.bits_va_lock.qid,q->qid);
	packet_append_string(p,name);
	query_attach_account(q,"account",ac);
	send_bits_packet_up(p);
	packet_del_ref(p);
	account_set_bits_state(ac,account_state_pending); /* request sent */
	accountlist_add_account(ac);
	return 0;
}

extern int bits_va_unlock_account(t_account * account)
{
	if (!account)
	{
		eventlog(eventlog_level_error,"bits_va_unlock_account","got NULL account");
		return -1;
	}
	if (!bits_uplink_connection)
	{
		eventlog(eventlog_level_error,"bits_va_unlock_account","FIXME: bits master should never (un)lock account");
		return 0;
	}
	eventlog(eventlog_level_debug,"bits_va_unlock_account","unlocking account #%u",account_get_uid(account));
	bits_va_locklist_del(bits_uplink_connection,bits_va_locklist_byuid(bits_uplink_connection,account_get_uid(account)));
	if (account_get_bits_state(account)!=account_state_invalid)
	{
		if (!bits_va_locklist_is_locked_by(account_get_uid(account)))
		{
			send_bits_va_unlock(account_get_uid(account));
		}
	}
	if (accountlist_remove_account(account)<0) eventlog(eventlog_level_error,"bits_va_unlock_account","account is not in accountlist");
	account_destroy(account);
	return 0;
}

extern int bits_va_lock_check(void)
{
    t_entry *   curr;
    t_account * account;
    int count = 0;

    HASHTABLE_TRAVERSE(accountlist(),curr)
    {
	account = entry_get_data(curr);
	if ((account_get_bits_state(account)==account_state_delete)||(account_get_bits_state(account)==account_state_invalid))
	{
		bits_va_unlock_account(account);
		count++;
		curr=hashtable_get_first(accountlist()); /* restart */
	}
    }
    if (count > 0)
	eventlog(eventlog_level_debug,"bits_va_lock_check","purged %d accounts",count);
    return 0;
}

extern int bits_va_command_with_account_name(t_connection * c, char const * command, char const * name)
{
    t_query * q;
    t_account * account;
    
    if (!c) {
	eventlog(eventlog_level_error,"bits_va_command_with_account_name","got NULL connection");
	return -1;
    }
    if (!command) {
	eventlog(eventlog_level_error,"bits_va_command_with_account_name","got NULL command");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"bits_va_command_with_account_name","got NULL account name");
	return -1;
    }
    account = accountlist_find_account(name);
    if ((account)&&(account_is_invalid(account)))
    	return 0; /* We already know the account is invalid. */
    if ((!account)||(!account_is_ready_for_use(account))) {
	/* The account is not (yet) ready for use; we may need to fetch it ... */
	t_elem const * curr;

	if (account_name_is_unknown(name)) {
	    bits_va_lock_account(name);
	    account = accountlist_find_account(name);
	}
	
	LIST_TRAVERSE_CONST(query_list(),curr) {
	    t_connection * client;
	    char const * qcommand;

	    q = elem_get_data(curr);
	    client = query_get_attachment_conn(q,"client");
	    qcommand = query_get_attachment_string(q,"command");
	    if ((query_get_type(q)==bits_query_type_handle_command_account_loaded)&&(client==c)) {
	        if (strcmp(qcommand,command)) {
		    /* seems like this command was already called */
		    return 1; /* wait ... */
	        }
	    }
        }
        q = query_create(bits_query_type_handle_command_account_loaded);
	query_attach_conn(q,"client",c);
	query_attach_account(q,"account",account);
	query_attach_string(q,"command",command);
	return 1;
    }
    return 0;
}

extern int send_bits_va_unlock(unsigned int uid)
{
	t_packet * p;
	p = packet_create(packet_class_bits);
	if (!p)
	{
		eventlog(eventlog_level_error,"send_bits_va_unlock","packet create failed");
		return -1;
	}
	packet_set_size(p, sizeof(t_bits_va_unlock));
	packet_set_type(p, BITS_VA_UNLOCK);
	bits_packet_generic(p,BITS_ADDR_PEER);
	bn_int_set(&p->u.bits_va_unlock.uid,uid);
	send_bits_packet_up(p);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_va_create_req(t_connection *c, const char * username, bn_int const passhash1[5])
{
    t_packet * p;
	
    if (!username) {
	eventlog(eventlog_level_error,"send_bits_va_create_req","got NULL username");
	return -1;
    }
    p = packet_create(packet_class_bits);
    packet_set_size(p, sizeof(t_bits_va_create_req));
    packet_set_type(p, BITS_VA_CREATE_REQ);
    bits_packet_generic(p, BITS_ADDR_MASTER);
    memcpy(&p->u.bits_va_create_req.passhash1,passhash1,20);
    packet_append_string(p,username);
    bn_int_set(&p->u.bits_va_create_req.qid,0);
    if (c) {
	t_query *q;

	q = query_create(bits_query_type_bits_va_create);
	query_attach_conn(q,"client",c);
	query_attach_packet(q,"request",p);
	bn_int_set(&p->u.bits_va_create_req.qid,query_get_qid(q));
    }
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}


extern int send_bits_va_create_reply(const char * username, t_uint16 dest, unsigned char status, t_uint32 uid, t_uint32 qid)
{
    t_packet * p;

    if (!username) {
	eventlog(eventlog_level_error,"send_bits_va_create_reply","got NULL username");
    	return -1;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_create_reply","could not create packet");
	return -1;
    }
    packet_set_size(p, sizeof(t_bits_va_create_reply));
    packet_set_type(p, BITS_VA_CREATE_REPLY);
    bits_packet_generic(p, dest);
    bn_byte_set(&p->u.bits_va_create_reply.status,status);
    bn_int_set(&p->u.bits_va_create_reply.uid,uid);
    bn_int_set(&p->u.bits_va_create_reply.qid,qid);
    packet_append_string(p,username);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_va_get_allattr(unsigned int uid)
{
	t_packet * p;
	
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_va_get_allattr","failed to create packet");
		return -1;
	}
	packet_set_size(p, sizeof(t_bits_va_get_allattr));
	packet_set_type(p, BITS_VA_GET_ALLATTR);
	bits_packet_generic(p, BITS_ADDR_MASTER);
	bn_int_set(&p->u.bits_va_get_allattr.uid,uid);
	send_bits_packet_up(p);
	packet_del_ref(p);
	return 0;
}

/* uid is only used if account is NULL */
extern int send_bits_va_allattr(t_account * ac, unsigned int uid, t_uint16 dest)
{
	int key_fail = 0;
	t_packet * p;
	
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_va_allattr","failed to create a new packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_va_allattr));
	packet_set_type(p,BITS_VA_ALLATTR);
	if (ac) {
		char const * key;
		char const * val;
		bn_int_set(&p->u.bits_va_allattr.uid,account_get_uid(ac));
		bn_byte_set(&p->u.bits_va_allattr.status,BITS_STATUS_OK);
		account_set_accessed(ac,1);
   	/*	if (!account_is_loaded(ac)) {
      		}
	*/
		key = account_get_first_key(ac);
		while (key) {
			val = account_get_strattr(ac,key);
			if (!val) {
				eventlog(eventlog_level_error,"send_bits_va_allattr","val is NULL");
				key_fail = 1;
				break;
			}
			packet_append_string(p,key);
			packet_append_string(p,val);
			account_unget_strattr(val);
			key = account_get_next_key(ac,key);
		}
	}
	if ((key_fail) || (!ac)) {
		bn_int_set(&p->u.bits_va_allattr.uid,uid);
		bn_byte_set(&p->u.bits_va_allattr.status, key_fail ? BITS_STATUS_TOOBUSY : BITS_STATUS_NOACCT);
	}	
	bits_packet_generic(p,dest);
	send_bits_packet(p);
	packet_del_ref(p);
	return 0;
}

extern int send_bits_va_set_attr(unsigned int uid, char const * name, char const * val, t_connection * from)
{
    t_packet * p;

    if (!name) {
	eventlog(eventlog_level_error,"send_bits_va_set_attr","got NULL name");
	return -1;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_va_set_attr","packet create failed");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_va_set_attr));
    packet_set_type(p,BITS_VA_SET_ATTR);
    bn_int_set(&p->u.bits_va_set_attr.uid,uid);
    packet_append_string(p,name);
    if (val) packet_append_string(p,val);
    bits_packet_generic(p,BITS_ADDR_PEER);
    send_bits_packet_for_account(p,uid,from);
    packet_del_ref(p);
    return 0;
}

#endif /* WITH_BITS */

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

#include <stddef.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strcasecmp.h"

#include "connection.h"
#include "realm.h"
#include "account.h"
#include "d2cs/d2cs_bnetd_protocol.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/packet.h"
#include "common/addr.h"
#include "common/bn_type.h"
#include "handle_d2cs.h"
#include "common/setup_after.h"

static int on_d2cs_accountloginreq(t_connection * c, t_packet const * packet);
static int on_d2cs_charloginreq(t_connection * c, t_packet const * packet);
static int on_d2cs_authreply(t_connection * c, t_packet const * packet);

extern int handle_d2cs_packet(t_connection * c, t_packet const * packet)
{
	if (!c) {
		eventlog(eventlog_level_error,"handle_d2cs_packet","got NULL connection");
		return -1;
	}
	if (!packet) {
		eventlog(eventlog_level_error,"handle_d2cs_packet","got NULL packet");
		return -1;
	}
	if (packet_get_class(packet)!=packet_class_d2cs_bnetd) {
		eventlog(eventlog_level_error,"handle_d2cs_packet","got bad packet class %d",
		packet_get_class(packet));
		return -1;
	}
	switch (conn_get_state(c)) {
		case conn_state_connected:
			switch (packet_get_type(packet)) {
				case D2CS_BNETD_AUTHREPLY:
					on_d2cs_authreply(c,packet);
					break;
				default:
					eventlog(eventlog_level_error,"handle_d2cs_packet",\
					"got unknown packet type %d",packet_get_type(packet));
					break;
			}
			break;
		case conn_state_loggedin:
			switch (packet_get_type(packet)) {
				case D2CS_BNETD_ACCOUNTLOGINREQ:
					on_d2cs_accountloginreq(c,packet);
					break;
				case D2CS_BNETD_CHARLOGINREQ:
					on_d2cs_charloginreq(c,packet);
					break;
				default:
					eventlog(eventlog_level_error,"handle_d2cs_packet",\
					"got unknown packet type %d",packet_get_type(packet));
					break;
			}
			break;
		default:
			eventlog(eventlog_level_error,"handle_d2cs_packet",\
				"got unknown connection state %d",conn_get_state(c));
			break;
	}
	return 0;
}

static int on_d2cs_authreply(t_connection * c, t_packet const * packet)
{
	t_packet	* rpacket;
	unsigned int	version;
	unsigned int	try_version;
	unsigned int	reply;
	char const	* realmname;
	t_realm		* realm;

	if (packet_get_size(packet)<sizeof(t_d2cs_bnetd_authreply)) {
		eventlog(eventlog_level_error,"on_d2cs_authreply","got bad packet size");
		return -1;
	}
	if (!(realmname=packet_get_str_const(packet,sizeof(t_d2cs_bnetd_authreply),REALM_NAME_LEN))) {
		eventlog(eventlog_level_error,"on_d2cs_authreply","got bad realmname");
		return -1;
	}
        if (!(realm=realmlist_find_realm_by_ip(conn_get_addr(c)))) {
                eventlog(eventlog_level_error,"handle_init_packet", "realm not found");
                return -1;
        }
	if (realm_get_name(realm) && strcasecmp(realmname,realm_get_name(realm))) {
                eventlog(eventlog_level_error,"handle_init_packet", "warn: realm name mismatch %s %s",
			realm_get_name(realm),realmname);
	}

	version=prefs_get_d2cs_version();
	try_version=bn_int_get(packet->u.d2cs_bnetd_authreply.version);
	if (version && version != try_version) {
		eventlog(eventlog_level_error,"on_d2cs_authreply","d2cs version mismatch 0x%X - 0x%X",
			try_version,version);
		reply=BNETD_D2CS_AUTHREPLY_BAD_VERSION;
	} else {
		reply=BNETD_D2CS_AUTHREPLY_SUCCEED;
	}

	if (reply==BNETD_D2CS_AUTHREPLY_SUCCEED) {
		eventlog(eventlog_level_error,"on_d2cs_authreply","d2cs %s authed",
			addr_num_to_ip_str(conn_get_addr(c)));
		conn_set_state(c,conn_state_loggedin);
		if (prefs_allow_d2cs_setname()) realm_set_name(realm,realmname);
		realm_active(realm,c);
	} else {
		eventlog(eventlog_level_error,"on_d2cs_authreply","failed to auth d2cs %s",
			addr_num_to_ip_str(conn_get_addr(c)));
	}
	if ((rpacket=packet_create(packet_class_d2cs_bnetd))) {
		packet_set_size(rpacket,sizeof(t_bnetd_d2cs_authreply));
		packet_set_type(rpacket,BNETD_D2CS_AUTHREPLY);
		bn_int_set(&rpacket->u.bnetd_d2cs_authreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_d2cs_accountloginreq(t_connection * c, t_packet const * packet)
{
	unsigned int	sessionkey;
	unsigned int	sessionnum;
	unsigned int	salt;
	char const *	account;
	char const *	tname;
	t_connection	* client;
	int		reply;
	t_packet	* rpacket;
	struct
	{
		bn_int   salt;
		bn_int   sessionkey;
		bn_int   sessionnum;
		bn_int   secret;
		bn_int	 passhash[5];
	} temp;
	t_hash       secret_hash;
	char const * pass_str;
	t_hash	     passhash;
	t_hash	     try_hash;

	if (packet_get_size(packet)<sizeof(t_d2cs_bnetd_accountloginreq)) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","got bad packet size");
		return -1;
	}
	if (!(account=packet_get_str_const(packet,sizeof(t_d2cs_bnetd_accountloginreq),USER_NAME_MAX))) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","got bad account name");
		return -1;
	}
	sessionkey=bn_int_get(packet->u.d2cs_bnetd_accountloginreq.sessionkey);
	sessionnum=bn_int_get(packet->u.d2cs_bnetd_accountloginreq.sessionnum);
	salt=bn_int_get(packet->u.d2cs_bnetd_accountloginreq.seqno);
	if (!(client=connlist_find_connection_by_sessionnum(sessionnum))) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","sessionnum %d not found",sessionnum);
		reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
	} else if (sessionkey!=conn_get_sessionkey(client)) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","sessionkey %d not match",sessionkey);
		reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
	} else if (!(tname=conn_get_username(client))) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","got NULL username");
		reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
	} else if (strcasecmp(account,tname)) {
		eventlog(eventlog_level_error,"on_d2cs_accountloginreq","username %s not match",account);
		conn_unget_username(client,tname);
		reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
	} else {
		conn_unget_username(client,tname);
		bn_int_set(&temp.salt,salt);
		bn_int_set(&temp.sessionkey,sessionkey);
		bn_int_set(&temp.sessionnum,sessionnum);
		bn_int_set(&temp.secret,conn_get_secret(client));
		pass_str=account_get_pass(conn_get_account(client));
		if (hash_set_str(&passhash,pass_str)<0) {
			reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
		} else {
			hash_to_bnhash((t_hash const *)&passhash,temp.passhash);
			bnet_hash(&secret_hash,sizeof(temp),&temp);
			bnhash_to_hash(packet->u.d2cs_bnetd_accountloginreq.secret_hash,&try_hash);
			if (hash_eq(try_hash,secret_hash)==1) {
				eventlog(eventlog_level_debug,"on_d2cs_accountloginreq","user %s loggedin on d2cs",\
					account);
				reply=BNETD_D2CS_ACCOUNTLOGINREPLY_SUCCEED;
			} else {
				eventlog(eventlog_level_error,"on_d2cs_accountloginreq","user %s hash not match",\
					account);
				reply=BNETD_D2CS_ACCOUNTLOGINREPLY_FAILED;
			}
		}
		account_unget_pass(pass_str);
	}
	if ((rpacket=packet_create(packet_class_d2cs_bnetd))) {
		packet_set_size(rpacket,sizeof(t_bnetd_d2cs_accountloginreply));
		packet_set_type(rpacket,BNETD_D2CS_ACCOUNTLOGINREPLY);
		bn_int_set(&rpacket->u.bnetd_d2cs_accountloginreply.h.seqno,\
			bn_int_get(packet->u.d2cs_bnetd_accountloginreq.h.seqno));
		bn_int_set(&rpacket->u.bnetd_d2cs_accountloginreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

#define CHAR_PORTRAIT_LEN	0x30
static int on_d2cs_charloginreq(t_connection * c, t_packet const * packet)
{
	t_connection * client;
	char const * charname;
	char const * portrait;
	char const * clienttag;
	char		* temp;
	unsigned int	sessionnum;
	char const * tname;
	char const * realmname;
	unsigned int	pos, reply;
	t_packet	* rpacket;
	
	if (packet_get_size(packet)<sizeof(t_d2cs_bnetd_charloginreq)) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","got bad packet size");
		return -1;
	}
	sessionnum=bn_int_get(packet->u.d2cs_bnetd_charloginreq.sessionnum);
	pos=sizeof(t_d2cs_bnetd_charloginreq);
	if (!(charname=packet_get_str_const(packet,pos,CHAR_NAME_LEN))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","got bad character name");
		return -1;
	}
	pos+=strlen(charname)+1;
	if (!(portrait=packet_get_str_const(packet,pos,CHAR_PORTRAIT_LEN))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","got bad character portrait");
		return -1;
	}
	if (!(client=connlist_find_connection_by_sessionnum(sessionnum))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","user %d not found",sessionnum);
		reply = BNETD_D2CS_CHARLOGINREPLY_FAILED;
	} else if (!(clienttag=conn_get_clienttag(client))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","got NULL clienttag");
		reply = BNETD_D2CS_CHARLOGINREPLY_FAILED;
	} else if (!(realmname=conn_get_realmname(client))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","got NULL realm name");
		reply = BNETD_D2CS_CHARLOGINREPLY_FAILED;
	} else if (!(temp=malloc(strlen(clienttag)+strlen(realmname)+1+strlen(charname)+1+\
			strlen(portrait)+1))) {
		eventlog(eventlog_level_error,"on_d2cs_charloginreq","error allocate temp");
		reply = BNETD_D2CS_CHARLOGINREPLY_FAILED;
	} else {
		reply = BNETD_D2CS_CHARLOGINREPLY_SUCCEED;
		sprintf (temp,"PX2D%s,%s,%s",realmname,charname,portrait);
		bn_int_tag_set((bn_int *)temp,clienttag);
		conn_set_charname(client,charname);
		conn_set_realminfo(client,temp);
		free(temp);
		eventlog(eventlog_level_debug,"on_d2cs_charloginreq",\
			"loaded portrait for character %s",charname);
	}
	if ((rpacket=packet_create(packet_class_d2cs_bnetd))) {
		packet_set_size(rpacket,sizeof(t_bnetd_d2cs_charloginreply));
		packet_set_type(rpacket,BNETD_D2CS_CHARLOGINREPLY);
		bn_int_set(&rpacket->u.bnetd_d2cs_charloginreply.h.seqno,\
			bn_int_get(packet->u.d2cs_bnetd_charloginreq.h.seqno));
		bn_int_set(&rpacket->u.bnetd_d2cs_charloginreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

extern int handle_d2cs_init(t_connection * c)
{
	t_packet	* packet;

	if ((packet=packet_create(packet_class_d2cs_bnetd))) {
		packet_set_size(packet,sizeof(t_bnetd_d2cs_authreq));
		packet_set_type(packet,BNETD_D2CS_AUTHREQ);
		bn_int_set(&packet->u.bnetd_d2cs_authreq.sessionnum,conn_get_sessionnum(c));
		queue_push_packet(conn_get_out_queue(c),packet);
		packet_del_ref(packet);
	}
	eventlog(eventlog_level_info,"handle_d2cs_init","sent init packet to d2cs (sessionnum=%d)",
		conn_get_sessionnum(c));
	return 0;
}

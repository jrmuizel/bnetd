/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#include "common/bits_protocol.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "connection.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/addr.h"
#include "handle_bnet.h"
#include "query.h"
#include "bits.h"
#include "bits_query.h"
#include "bits_packet.h"
#include "bits_login.h"
#include "bits_va.h"
#include "bits_net.h"
#include "bits_ext.h"
#include "bits_rconn.h"
#include "bits_chat.h"
#include "bits_game.h"
#include "bits_motd.h"
#include "common/version.h"
#include "common/queue.h"
#include "common/list.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "message.h"
#include "account.h"
#include "channel.h"
#include "game.h"
#include "handle_bits.h"
#include "common/setup_after.h"

extern int handle_bits_packet(t_connection * conn,t_packet const * const packet)
{
	if (!packet) {
		eventlog(eventlog_level_error,"handle_bits_packet","got NULL packet");
		return -1;
	}
	if (conn_get_class(conn) != conn_class_bits ) {
		eventlog(eventlog_level_error,"handle_bits_packet","FIXME: handle_bits_packet without any reason (conn->class != conn_class_bits)");
		return -1;
	}
	if (conn_get_state(conn) == conn_state_empty) {
		eventlog(eventlog_level_error,"handle_bits_packet","FIXME: handle a packet on a non-connection?");
		return 0;
	}
	if (conn->bits == NULL) {
		eventlog(eventlog_level_error,"handle_bits_packet","BITS connection extension is NULL!");
		return -1;
	}
	if (packet_get_class(packet)!=packet_class_bits) {
		eventlog(eventlog_level_error,"handle_bits_packet","got non-BITS packet");
		return -1;
	}
	if ((conn!=bits_uplink_connection)&&(conn_get_state(conn) != conn_state_loggedin)&&(packet_get_type(packet)!=BITS_AUTH_REQUEST)&&(packet_get_type(packet)!=BITS_SESSION_REQUEST)) {
		eventlog(eventlog_level_error,"handle_bits_packet","[%d] client tried to send illegal pakets before login (type=%s:0x%04x;address=%s)",conn_get_socket(conn),packet_get_type_str(packet,packet_dir_from_server),packet_get_type(packet),addr_num_to_addr_str(conn_get_addr(conn),conn_get_port(conn)));
		/* might be an attacker ... */
		return -1;
	}
	bits_route_add(bits_packet_get_src(packet),conn);
	if ((bits_packet_get_ttl(packet) == 0)&&(bits_packet_get_dst(packet)!=bits_get_myaddr())) {
		eventlog(eventlog_level_warn,"handle_bits_packet","dropping packet with zero ttl (type=0x%04x,size=%d,source=0x%04x,dest=0x%04x)",packet_get_type(packet),packet_get_size(packet),bits_packet_get_src(packet),bits_packet_get_dst(packet));
		return 0;
	}
	if ((bits_packet_get_dst(packet)!=bits_get_myaddr())&&(bits_packet_get_dst(packet)!=BITS_ADDR_PEER)&&(bits_packet_get_dst(packet)!=BITS_ADDR_BCAST)) {
		t_packet * ptemp;
		ptemp = packet_duplicate(packet);
		bits_packet_set_ttl(ptemp,bits_packet_get_ttl(ptemp)-1);
		if ((bits_route_find(bits_packet_get_dst(packet)))||(bits_packet_get_dst(packet)==BITS_ADDR_MASTER)) {
			send_bits_packet(ptemp);
		} else { /* don't send it back */ 
			bits_packet_forward_bcast(conn,ptemp);
		}
		packet_del_ref(ptemp);
		return 0;
	}
	if (bits_debug) eventlog(eventlog_level_debug,"handle_bits_packet","DEBUG: [%d] got packet type 0x%04x (%s) from bits host 0x%04x (%d bytes)",conn_get_socket(conn),packet_get_type(packet),packet_get_type_str(packet,packet_dir_from_server),bits_packet_get_src(packet),packet_get_size(packet));
	if ((bits_packet_get_dst(packet)==BITS_ADDR_BCAST)&&(bits_packet_get_src(packet)!=bits_get_myaddr())) {
		t_packet * ptemp;
		ptemp = packet_duplicate(packet);
		bits_packet_forward_bcast(conn,ptemp);
		packet_del_ref(ptemp);
	}
	switch (packet_get_type(packet)) {
	/* ------------------------------------------------- */
		case BITS_SESSION_REPLY:
		{
			if (conn_get_state(conn)!=conn_state_initial) {
				eventlog(eventlog_level_error,"handle_bits_packet","got BITS_SESSION_REPLY in wrong state");
				break;
			}
			if (bits_status_ok(packet->u.bits_session_reply.status)) 
			{
				conn->sessionkey = bn_int_get(packet->u.bits_session_reply.sessionkey);
				conn->sessionnum = bn_int_get(packet->u.bits_session_reply.sessionnum);
				eventlog(eventlog_level_info,"handle_bits_packet","[%d] sessionkey=0x%08x sessionnum=0x%08x",conn_get_socket(conn),conn_get_sessionkey(conn),conn_get_sessionnum(conn));
				conn_set_state(conn,conn_state_connected);
			} else {
				eventlog(eventlog_level_error,"handle_bits_packet","[%d] sessionreq failed with status 0x%02x",conn_get_socket(conn),bn_byte_get(packet->u.bits_session_reply.status));
			}
		}
		break;
	/* ------------------------------------------------- */
		case BITS_SESSION_REQUEST:
		{
			t_packet *rpacket;

			if (conn_get_state(conn)!=conn_state_connected) {
				eventlog(eventlog_level_warn,"handle_bits_packet","BITS_SESSION_REQUEST in wrong state");
				return -1;
			}
			rpacket = packet_create(packet_class_bits);
			if (!rpacket) {
				eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
				break;
			}
			packet_set_size(rpacket,sizeof(t_bits_session_reply));
			packet_set_type(rpacket,BITS_SESSION_REPLY);
			bits_packet_generic(rpacket,BITS_ADDR_PEER);
			if ((memcmp(&packet->u.bits_session_request.protocoltag,BITS_PROTOCOLTAG,4)==0)&&
			    (memcmp(&packet->u.bits_session_request.clienttag,BITS_CLIENTTAG,4)==0)) {
				char const *v;
				v = packet_get_str_const(packet,sizeof(t_bits_session_request),strlen(BNETD_VERSION)+1);
				if (!v) {
					bn_byte_set(&rpacket->u.bits_session_reply.status,BITS_STATUS_AUTHWRONGVERSION);
				} else {
					if (strcmp(v,BNETD_VERSION)==0) {
						bn_byte_set(&rpacket->u.bits_session_reply.status,BITS_STATUS_OK);
						bn_int_set(&rpacket->u.bits_session_reply.sessionkey,conn_get_sessionkey(conn));
						bn_int_set(&rpacket->u.bits_session_reply.sessionnum,conn_get_sessionnum(conn));
					} else {
						eventlog(eventlog_level_warn,"handle_bits_packet","[%d] invalid peer version \"%s\".",conn_get_socket(conn),v);
						bn_byte_set(&rpacket->u.bits_session_reply.status,BITS_STATUS_AUTHWRONGVERSION);
					}
				}
			} else {
				bn_byte_set(&rpacket->u.bits_session_reply.status,BITS_STATUS_FAIL);
			}
			queue_push_packet(conn_get_out_queue(conn),rpacket);
			packet_del_ref(rpacket);
			conn_set_state(conn,conn_state_connected);
		}
		break;
	/* ------------------------------------------------- */
		case BITS_AUTH_REPLY:
			if ((conn_get_state(conn)!=conn_state_connected)&&(bits_get_myaddr()==BITS_ADDR_PEER)) {
				eventlog(eventlog_level_error,"handle_bits_packet","got BITS_AUTH_REPLY in wrong state");
				return 0;
			}
			if (conn->bits->type == bits_to_master) {
			    if (packet_get_size(packet) < sizeof(t_bits_auth_reply)) {
				eventlog(eventlog_level_error,"handle_bits_packet","packet smaller than BITS_AUTH_REPLY (%d bytes).",packet_get_size(packet));
			    }
			    if (bits_get_myaddr()==BITS_ADDR_PEER) {
				/* This packet (probably) is for this server */
				if (bits_status_ok(packet->u.bits_auth_reply.status)) {
					bits_uplink_connection->bits->myaddr = bn_short_get(packet->u.bits_auth_reply.address);
					eventlog(eventlog_level_info,"bits_handle_packet","authentication OK (myaddr=%d)",bits_uplink_connection->bits->myaddr);
					bits_uplink_connection->state = conn_state_loggedin;
				} else {
					eventlog(eventlog_level_error,"handle_bits_packet","authentication failed (status=%d)",bn_byte_get(packet->u.bits_auth_reply.status));
				}
			    } else {
				/* This packet is (probably) for another server ... forward it */
				t_query *q;

				q = query_find_byqid(bn_int_get(packet->u.bits_auth_reply.qid));
				if (q) {
				    query_answer(q,packet);
				    eventlog(eventlog_level_info,"handle_bits_packet","redirected BITS_AUTH_REPLY back to client (status=0x%02x;address=0x%04x)",bn_byte_get(packet->u.bits_auth_reply.status),bn_short_get(packet->u.bits_auth_reply.address));
				} else
				    eventlog(eventlog_level_info,"handle_bits_packet","failed to redirect BITS_AUTH_REPLY back to client (status=0x%02x;address=0x%04x)",bn_byte_get(packet->u.bits_auth_reply.status),bn_short_get(packet->u.bits_auth_reply.address));
			    }
			} else {
				eventlog(eventlog_level_debug,"handle_bits_packet","got BITS_AUTH_REPLY from slave ... dropped");
			}
			break;
	/* ------------------------------------------------- */
		case BITS_AUTH_REQUEST:
			if ((conn_get_state(conn)!=conn_state_connected)&&((bits_packet_get_src(packet)!=BITS_ADDR_PEER)&&(conn_get_state(conn)!=conn_state_loggedin))) {
				eventlog(eventlog_level_debug,"handle_bits_packet","got auth_request in wrong conn_state (%d)",conn_get_state(conn));
				return 0;
			}
			if (conn->bits->type == bits_to_slave) {
			    if (bits_master) {
				char const *username;

				username = packet_get_str_const(packet,sizeof(t_bits_auth_request),1024);
			    	if (!username) {
				    eventlog(eventlog_level_error,"handle_bits_packet","got malformed BITS_AUTH_REQUEST packet (username missing)");
				    break;
			    	}
				if (bits_check_bits_auth_request(conn,packet)) {
					/* OK */
				    short na;
				    t_bits_hostlist_entry * e;

				    na = bits_new_dyn_addr();
				    if (na!=0) { 
				    	e = malloc(sizeof(t_bits_hostlist_entry));
				    	if (!e) {
					    eventlog(eventlog_level_error,"malloc failed: %s",strerror(errno));
					    break;
				    	}
				    	e->address = na;
				    	e->name = strdup(username);
				    	list_append_data(bits_hostlist,e);
				    	/* compression *might* reduce traffic here */
				    	bits_send_authreply(conn,BITS_STATUS_OK,na,bits_packet_get_src(packet),bn_int_get(packet->u.bits_auth_request.qid));
				    	conn_set_state(conn,conn_state_loggedin);
				    	eventlog(eventlog_level_info,"handle_bits_packet","authentication successful for user \"%s\" (address=0x%04x)",username,na);
					bits_setup_client(conn);
				    	/* ready */
				    	eventlog(eventlog_level_debug,"handle_bits_packet","all lists uploaded to server 0x%04x",na);
				    } else {
				    	eventlog(eventlog_level_error,"handle_bits_packet","out of free addresses!");
				    	bits_send_authreply(conn,BITS_STATUS_TOOBUSY,0,bn_short_get(packet->u.bits.h.src_addr),bn_int_get(packet->u.bits_auth_request.qid));
				    }						
				} else {
					/* FAIL */
				    if (username) eventlog(eventlog_level_error,"handel_bits_packet","authentication failed for user \"%s\"",username);
				    bits_send_authreply(conn,BITS_STATUS_AUTHFAILED,0,bn_short_get(packet->u.bits.h.src_addr),bn_int_get(packet->u.bits_auth_request.qid));
				}
			    } else {
				/* forward to master server */
				t_packet * p;
				t_query *q;

				p = packet_duplicate(packet);
				if (!p) {
				    eventlog(eventlog_level_error,"handle_bits_packet","could not duplicate packet");
				    break;
				}
				q = query_create(bits_query_type_bits_auth);
				query_attach_conn(q,"client",conn);
				bn_int_set(&p->u.bits_auth_request.qid,query_get_qid(q));
				bn_int_set(&p->u.bits_auth_request.sessionkey,conn_get_sessionkey(conn));
				bits_packet_generic(p,BITS_ADDR_MASTER);
				send_bits_packet(p);
				packet_del_ref(p);
				eventlog(eventlog_level_info,"handle_bits_packet","[%d] forwarded BITS_AUTH_REQUEST to master server",conn_get_socket(conn));
			    }
			} else {
				eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_AUTH_REQUEST from master ... dropped");
			}
			break;
	/* ------------------------------------------------- */
		case BITS_VA_CREATE_REQ:
			if (conn->bits->myaddr != BITS_ADDR_MASTER) {
				eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_VA_CREATE_REQ although I'm not master.");
				break;
			}
			if (conn->bits->type == bits_to_slave) {
				t_account *ac;
				t_hash newhash;
				t_uint32 qid;
				char const *username;

				qid = bn_int_get(packet->u.bits_va_create_req.qid);
				username = packet_get_str_const(packet,sizeof(t_bits_va_create_req),USER_NAME_MAX);
				bnhash_to_hash(packet->u.bits_va_create_req.passhash1,&newhash);
				ac = account_create(username,hash_get_str(newhash));
				if (!ac) {
					eventlog(eventlog_level_warn,"handle_bits_packet","account_create failed.");
					send_bits_va_create_reply(username,bn_short_get(packet->u.bits.h.src_addr),BITS_STATUS_FAIL,0,qid);
					break;
				}
				if (!accountlist_add_account(ac)) {
					account_destroy(ac);
					eventlog(eventlog_level_warn,"handle_bits_packet","accountlist_add_account failed.");
					send_bits_va_create_reply(username,bn_short_get(packet->u.bits.h.src_addr),BITS_STATUS_FAIL,0,qid);
					break;
				}
				send_bits_va_create_reply(username,bits_packet_get_src(packet),BITS_STATUS_OK,account_get_uid(ac),qid);
				eventlog(eventlog_level_info,"handle_bits_packet","account \"%s\"(#%u) was successfully created.",username,account_get_uid(ac));				
			}
			break;
	/* ------------------------------------------------- */
		case BITS_VA_CREATE_REPLY:
			if (conn->bits->type == bits_to_master) {
				t_query *q;
				
				q = query_find_byqid(bn_int_get(packet->u.bits_va_create_reply.qid));
				query_answer(q,packet);
			} else {
				eventlog(eventlog_level_debug,"handle_bits_packet","got BITS_VA_CREATE_REPLY from slave.");
			}
			break;
	/* ------------------------------------------------- */	
		case BITS_VA_GET_ALLATTR:
			if (bits_uplink_connection == NULL) {
				char temp[32];
				t_account * ac;
				sprintf(temp,"#%u",bn_int_get(packet->u.bits_va_get_allattr.uid));
				ac = accountlist_find_account(temp);
				if (!ac) {
					eventlog(eventlog_level_warn,"handle_bits_packet","can't find an account with uid=#%u.",bn_int_get(packet->u.bits_va_get_allattr.uid));
				} else {
					eventlog(eventlog_level_debug,"handle_bits_packet","sending account attrs on request for uid=#%u.",account_get_uid(ac));
				}
				send_bits_va_allattr(ac,bn_int_get(packet->u.bits_va_get_allattr.uid),bits_packet_get_src(packet));
			} else {
				eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_VA_GET_ALLATTR although not master");
			}
			break;
	/* ------------------------------------------------- */	
		case BITS_VA_ALLATTR:
			{
			if (bits_status_ok(packet->u.bits_va_allattr.status)) {
				char temp[32];
				t_account * ac;
				int pos;
				eventlog(eventlog_level_debug,"handle_bits_packet","got all attrs for #%u.",bn_int_get(packet->u.bits_va_allattr.uid));
				sprintf(temp,"#%u",bn_int_get(packet->u.bits_va_allattr.uid));
				ac = accountlist_find_account(temp);
				if (!ac) {
					eventlog(eventlog_level_error,"handle_bits_packet","Cannot find account #%u in account list.",bn_int_get(packet->u.bits_va_allattr.uid));
					break;
				}
				account_set_loaded(ac,0); 
				pos = sizeof(t_bits_va_allattr);
				while (pos < packet_get_size(packet)) {
					const char *key;
					const char *val;
					key = packet_get_str_const(packet,pos,BITS_MAX_KEYVAL_LEN);
					val = packet_get_str_const(packet,pos+strlen(key)+1,BITS_MAX_KEYVAL_LEN);
					account_set_strattr_nobits(ac,key,val);
					pos += strlen(key)+1+strlen(val)+1;
				}
				account_set_loaded(ac,1); 
			} else {
				eventlog(eventlog_level_error,"handle_bits_packet","account #%u is invalid (status=0x%02x))",bn_int_get(packet->u.bits_va_allattr.uid),bn_byte_get(packet->u.bits_va_allattr.status));
				/* FIXME: Remove the account here */
			}
			}
			break;
	/* ------------------------------------------------- */	
		case BITS_VA_LOCK:
		{
			if (bits_uplink_connection!=NULL) {
				t_query * q;
				t_packet * rpacket;
				/* FIXME: check if the account is already locked by another client */
				
				rpacket = packet_create(packet_class_bits);
				if (!rpacket) {
					eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK)","packet_create failed.");
					break;
				}
				packet_set_size(rpacket,sizeof(t_bits_va_lock));
				packet_set_type(rpacket,BITS_VA_LOCK);
				packet_append_string(rpacket,packet_get_str_const(packet,sizeof(t_bits_va_lock),USER_NAME_MAX));
				q = query_create(bits_query_type_bits_va_lock);
				if (!q) {
					eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK)","bits_query_push failed.");
					packet_del_ref(rpacket);
					break;
				}
				query_attach_conn(q,"client",conn);
				query_attach_packet_const(q,"request",packet);
				bn_int_set(&rpacket->u.bits_va_lock.qid,query_get_qid(q));
				bits_packet_generic(rpacket,BITS_ADDR_PEER);
				send_bits_packet_up(rpacket);
				packet_del_ref(rpacket);
			} else {
				t_packet * rpacket;
				char * username;
				t_account * ac;

				rpacket = packet_create(packet_class_bits);
				if (!rpacket) {
					eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK)","packet_create failed.");
					break;
				}
				packet_set_size(rpacket,sizeof(t_bits_va_lock_ack));
				packet_set_type(rpacket,BITS_VA_LOCK_ACK);
				bn_int_set(&rpacket->u.bits_va_lock_ack.qid,bn_int_get(packet->u.bits_va_lock.qid));
				bits_packet_generic(rpacket,bits_packet_get_src(packet));
				username = (char *)packet_get_str_const(packet,sizeof(t_bits_va_lock),USER_NAME_MAX);
				ac = accountlist_find_account(username);
				if (!ac) {
					bn_byte_set(&rpacket->u.bits_va_lock_ack.status,BITS_STATUS_NOACCT);
					bn_int_set(&rpacket->u.bits_va_lock_ack.uid,0);
					eventlog(eventlog_level_info,"handle_bits_packet(BITS_VA_LOCK)","Request for non-existent account. (username=\"%s\")",username);
				} else {
					char const * tname;
					bn_byte_set(&rpacket->u.bits_va_lock_ack.status,BITS_STATUS_OK);
					bn_int_set(&rpacket->u.bits_va_lock_ack.uid,account_get_uid(ac));
					packet_append_string(rpacket,(tname = account_get_name(ac)));
					eventlog(eventlog_level_info,"handle_bits_packet(BITS_VA_LOCK)","Request for account #%u.",account_get_uid(ac));
					bits_va_locklist_add(conn,tname,account_get_uid(ac));
					account_unget_name(tname);
				}
				bits_packet_generic(rpacket,BITS_ADDR_PEER);
				send_bits_packet_on(rpacket,conn);
				packet_del_ref(rpacket);
			}
			break;
			
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_VA_LOCK_ACK:
		{
			t_query * q;
			t_account * account;
			t_connection * client;
			t_packet * request;
			
			q = query_find_byqid(bn_int_get(packet->u.bits_va_lock_ack.qid));
			if (!q) {
				eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK_ACK)","got BITS_VA_LOCK_ACK for non-existent qid %u.",bn_int_get(packet->u.bits_va_lock_ack.qid));
				break;
			} else {
				account = query_get_attachment_account(q,"account");
				client = query_get_attachment_conn(q,"client");
				request = query_get_attachment_packet(q,"request");
				if (account == NULL) {
					if (request) {
						t_packet * rpacket;
						
						rpacket = packet_create(packet_class_bits);
						if (!rpacket) {
							eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK_ACK)","packet create failed");
							break;
						}
						packet_set_size(rpacket,sizeof(t_bits_va_lock_ack));
						packet_set_type(rpacket,BITS_VA_LOCK_ACK);
						bits_packet_generic(rpacket,bits_packet_get_src(request));
						bn_byte_set(&rpacket->u.bits_va_lock_ack.status,bn_byte_get(packet->u.bits_va_lock_ack.status));
						bn_int_set(&rpacket->u.bits_va_lock_ack.uid,bn_int_get(packet->u.bits_va_lock_ack.uid));
						bn_int_set(&rpacket->u.bits_va_lock_ack.qid,bn_int_get(request->u.bits_va_lock_ack.qid));
						packet_append_string(rpacket,packet_get_str_const(packet,sizeof(t_bits_va_lock_ack),USER_NAME_MAX));
						if (bits_status_ok(packet->u.bits_va_lock_ack.status))
						{
							bits_va_locklist_add(client,packet_get_str_const(packet,sizeof(t_bits_va_lock_ack),USER_NAME_MAX),bn_int_get(packet->u.bits_va_lock_ack.uid));
						}						
						if (client)
						{
							send_bits_packet_on(rpacket,client);
						}
						else
						{
							send_bits_packet_down(rpacket);
							eventlog(eventlog_level_warn,"handle_bits_packet(BITS_VA_LOCK_ACK)","client connection is NULL");
						}
					} else {
						eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK_ACK)","FIXME: account is NULL");
						break;
					}
				}
				if (bits_status_ok(packet->u.bits_va_lock_ack.status)) {
					account_set_bits_state(account,account_state_valid);
					account_set_uid(account,bn_int_get(packet->u.bits_va_lock_ack.uid));
					/* add to own locklist */
					bits_va_locklist_add(bits_uplink_connection,packet_get_str_const(packet,sizeof(t_bits_va_lock_ack),USER_NAME_MAX),bn_int_get(packet->u.bits_va_lock_ack.uid));
					eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOCK_ACK)","locked account #%u",account_get_uid(account));
					send_bits_va_get_allattr(account_get_uid(account));
					bits_va_locklist_add(bits_uplink_connection,packet_get_str_const(packet,sizeof(t_bits_va_lock_ack),USER_NAME_MAX),bn_int_get(packet->u.bits_va_lock_ack.uid));
					query_delete(q);
				} else {
					eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOCK_ACK)","account lock failed");
					account_set_bits_state(account,account_state_invalid); /* remove the account later */
					query_delete(q);		
				}
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_VA_UNLOCK:
		{
			bits_va_locklist_del(conn,bits_va_locklist_byuid(conn,bn_int_get(packet->u.bits_va_unlock.uid)));	
			if (bits_va_locklist_is_locked_by(bn_int_get(packet->u.bits_va_unlock.uid)))
			{
				send_bits_va_unlock(bn_int_get(packet->u.bits_va_unlock.uid));
			}
			eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_UNLOCK)","account #%u unlocked by bits host 0x%04x",bn_int_get(packet->u.bits_va_unlock.uid),bits_packet_get_src(packet));
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_VA_SET_ATTR:
		{
			t_account * ac;
			char temp[32];
			char const * key;
			char const * val;
			key = packet_get_str_const(packet,sizeof(t_bits_va_set_attr),1024);
			if (packet_get_size(packet) <= strlen(key)+1+sizeof(t_bits_va_set_attr))
			{
				val = NULL; 
			}
			else
			{
				val = packet_get_str_const(packet,sizeof(t_bits_va_set_attr)+strlen(key)+1,1024);
			}
			sprintf(temp,"#%u",bn_int_get(packet->u.bits_va_set_attr.uid));
			ac = accountlist_find_account((char *)&temp);
			if (ac) {
				account_set_strattr_nobits(ac,key,val);
			}
			send_bits_va_set_attr(bn_int_get(packet->u.bits_va_set_attr.uid),key,val,conn);
			if (bits_debug)
			{ 
				if (val) eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_SET_ATTR)","DEBUG: key=\"%s\" val=\"%s\"",key,val);
				if (!val) eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_SET_ATTR)","DEBUG: key=\"%s\" val=NULL",key);
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_VA_LOGINREQ:
		{
			t_packet * rpacket;
			t_account * ac;
			char temp[32]; 

			if (bits_uplink_connection) {
				eventlog(eventlog_level_warn,"handle_bits_packet(BITS_VA_LOGINREQ)","FIXME: got loginreq but not master");
				break;
			}
			rpacket = packet_create(packet_class_bits);
			if (!rpacket) {
				eventlog(eventlog_level_error,"handle_bits_packet(BITS_VA_LOGINREQ)","packet create failed");
				return -1;
			}
			packet_set_size(rpacket,sizeof(t_bits_va_loginreply));
			packet_set_type(rpacket,BITS_VA_LOGINREPLY);
			bits_packet_generic(rpacket,bits_packet_get_src(packet));
			bn_int_set(&rpacket->u.bits_va_loginreply.qid,bn_int_get(packet->u.bits_va_loginreq.qid));
			sprintf(temp,"#%u",bn_int_get(packet->u.bits_va_loginreq.uid));
			ac = accountlist_find_account(temp);
			if (!ac) {
				eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOGINREQ)","user " UID_FORMAT " login on bits host 0x%04x failed: account does not exist",bn_int_get(packet->u.bits_va_loginreq.uid),bits_packet_get_src(packet));
				bn_byte_set(&rpacket->u.bits_va_loginreply.status,BITS_STATUS_FAIL);			
			} else {
				char const *tname;
				unsigned int sid;
				
				sid = bits_va_loginlist_get_free_sessionid();
				tname = account_get_name(ac);
		 		if (bits_loginlist_add(bn_int_get(packet->u.bits_va_loginreq.uid),bits_packet_get_src(packet),sid,packet->u.bits_va_loginreq.clienttag,bn_int_get(packet->u.bits_va_loginreq.game_addr),bn_short_get(packet->u.bits_va_loginreq.game_port),tname)==0) {
					char ct[5];
					t_connection * rconn;

					rconn = bits_rconn_create_by_sessionid(sid);
					ct[4]='\0';
					memcpy(ct,packet->u.bits_va_loginreq.clienttag,4);
					eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOGINREQ)","user \"%s\"(" UID_FORMAT ") logged in on bits host 0x%04x. (clienttag=\"%s\";sessionid=0x%08x)",tname,bn_int_get(packet->u.bits_va_loginreq.uid),bits_packet_get_src(packet),ct,sid);
					bn_byte_set(&rpacket->u.bits_va_loginreply.status,BITS_STATUS_OK);
					bn_int_set(&rpacket->u.bits_va_loginreply.sessionid,sid);
					/* Invoke the playerinfo update logic */
					conn_get_playerinfo(rconn);
					bits_rconn_destroy(rconn);
				} else {
					eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOGINREQ)","user " UID_FORMAT " login on bits host 0x%04x failed",bn_int_get(packet->u.bits_va_loginreq.uid),bits_packet_get_src(packet));
					bn_byte_set(&rpacket->u.bits_va_loginreply.status,BITS_STATUS_FAIL);
				}
				account_unget_name(tname);
			}
			send_bits_packet(rpacket);
			packet_del_ref(rpacket);
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_VA_LOGINREPLY:
		{
			t_query * q;
			
			q = query_find_byqid(bn_int_get(packet->u.bits_va_loginreply.qid));
			if (q) {
				query_answer(q,packet);
			} else {
				eventlog(eventlog_level_warn,"handle_bits_packet(BITS_VA_LOGINREPLY)","got packet with non-exsistent qid");
			}
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_VA_LOGIN:
		{
			eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOGIN)","user \"%s\"(#%u) logged in with sessionid 0x%08x",packet_get_str_const(packet,sizeof(t_bits_va_login),USER_NAME_MAX),bn_int_get(packet->u.bits_va_login.uid),bn_int_get(packet->u.bits_va_login.sessionid));
			bits_loginlist_add(bn_int_get(packet->u.bits_va_login.uid),bn_short_get(packet->u.bits_va_login.host),bn_int_get(packet->u.bits_va_login.sessionid),packet->u.bits_va_login.clienttag,bn_int_get(packet->u.bits_va_login.game_addr),bn_short_get(packet->u.bits_va_login.game_port),packet_get_str_const(packet,sizeof(t_bits_va_login),USER_NAME_MAX));			
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_VA_LOGOUT:
		{
			eventlog(eventlog_level_debug,"handle_bits_packet(BITS_VA_LOGOUT)","user with sessionid 0x%08x logged out",bn_int_get(packet->u.bits_va_logout.sessionid));
			bits_loginlist_del(bn_int_get(packet->u.bits_va_logout.sessionid));
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_VA_UPDATE_PLAYERINFO:
		{
			int sid;
			char const * playerinfo;

			sid = bn_int_get(packet->u.bits_va_update_playerinfo.sessionid);
			/* FIXME: maximum size set to 2048 */
			playerinfo = packet_get_str_const(packet,sizeof(t_bits_va_update_playerinfo),2048);
			eventlog(eventlog_level_debug,"handle_bits_packet","playerinfo for sessionid 0x%08x set to \"%s\"",sid,playerinfo);
			bits_loginlist_set_playerinfo(sid,playerinfo);
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_NET_DISCOVER:
			/* does nothing (not yet) */
			break;
		case BITS_NET_UNDISCOVER:
			bits_route_del(bits_packet_get_src(packet));
			break;
	/* ------------------------------------------------- */	
		case BITS_NET_MOTD:
			bits_motd_load_from_packet(packet);
			break;
	/* ------------------------------------------------- */
		case BITS_CHAT_USER:
		{
			t_connection *c;

			c = connlist_find_connection_by_sessionid(bn_int_get(packet->u.bits_chat_user.dst));
			if (c) {
				t_bits_loginlist_entry * lle;

				lle = bits_loginlist_bysessionid(bn_int_get(packet->u.bits_chat_user.src));
				if (lle)
				{
					t_connection *d;

					char temp[5];
					memcpy(&temp,lle->clienttag,4);
					temp[4]='\0';
					eventlog(eventlog_level_debug,"sdssa","%s",temp);

					d = bits_rconn_create(bn_int_get(packet->u.bits_chat_user.clatency),bn_int_get(packet->u.bits_chat_user.cflags),lle->sessionid);
					conn_set_botuser(d,lle->chatname);

					if (packet_get_size(packet) > sizeof(t_bits_chat_user)) {
						message_send_text(c,bn_int_get(packet->u.bits_chat_user.type),d,packet_get_str_const(packet,sizeof(t_bits_chat_user),MAX_MESSAGE_LEN));
					} else {
						message_send_text(c,bn_int_get(packet->u.bits_chat_user.type),d,NULL);
					}
					if (bn_int_get(packet->u.bits_chat_user.type)==message_type_whisper)
					    send_bits_chat_user(bn_int_get(packet->u.bits_chat_user.dst),
					                        bn_int_get(packet->u.bits_chat_user.src),
					                        message_type_whisperack,
					                        conn_get_latency(c),
					                        conn_get_flags(c),
					                        packet_get_str_const(packet,sizeof(t_bits_chat_user),MAX_MESSAGE_LEN));
					bits_rconn_destroy(d);
				}
			} else if (!bits_uplink_connection) {
				t_bits_loginlist_entry * lle;

				lle = bits_loginlist;
				while (lle) {
					if (lle->uid == bn_int_get(packet->u.bits_chat_user.dst)) break;
					lle = lle->next;
				}
				if (!lle) send_bits_chat_user(bn_int_get(packet->u.bits_chat_user.dst),bn_int_get(packet->u.bits_chat_user.src),message_type_error,0,0,"This user is not logged on.");
			}
		}
		break;
	/* ------------------------------------------------- */
		case BITS_CHAT_CHANNELLIST_ADD:
		{
			t_channel * channel;
			char const * name;
			char const * shortname = NULL;

			if (bits_master) break; /* ignore it */
			name = packet_get_str_const(packet,sizeof(t_bits_chat_channellist_add),CHANNEL_NAME_LEN);
			if (packet_get_size(packet) > sizeof(t_bits_chat_channellist_add)+strlen(name)+1) {
				shortname = packet_get_str_const(packet,sizeof(t_bits_chat_channellist_add)+strlen(name)+1,CHANNEL_NAME_LEN);
			}
			channel = channel_create(name,
			                         shortname, /* shortname */
			                         NULL, /* clienttag */
			                         1,    /* permflag*/
			                         1,    /* botflag */
			                         1,    /* operflag */
			                         0,    /* logflag */ /* FIXME: Ross added this as a quick fix... */
						 NULL, /* country */     /* | master server should handle that ... */
                                                 NULL, /* realm */
						 0);   /* maxmembers */  /*/  */
			channel_set_channelid(channel,bn_int_get(packet->u.bits_chat_channellist_add.channelid));
			if (shortname) {
				eventlog(eventlog_level_debug,"handle_bits_packet","adding channel #%u \"%s\" shortname=\"%s\"",channel_get_channelid(channel),channel_get_name(channel),shortname);
			} else {
				eventlog(eventlog_level_debug,"handle_bits_packet","adding channel #%u \"%s\" shortname=NULL",channel_get_channelid(channel),channel_get_name(channel));
			}			
		}
		break;
	/* ------------------------------------------------- */
		case BITS_CHAT_CHANNELLIST_DEL:
		{
			t_channel * channel;

			if (!bits_uplink_connection) break; /* ignore it */
			channel = channellist_find_channel_bychannelid(bn_int_get(packet->u.bits_chat_channellist_del.channelid));
			if (channel) {
				eventlog(eventlog_level_debug,"handle_bits_packet","removing channel #%u \"%s\"",channel_get_channelid(channel),channel_get_name(channel));
				channel_destroy(channel);
			} else {
				eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_CHAT_CHANNELLIST_DEL with non-existent channelid");
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_SERVER_JOIN:
		{
			int cid;
			t_channel * channel;
			
			cid = bn_int_get(packet->u.bits_chat_channel_server_join.channelid);
			channel = channellist_find_channel_bychannelid(cid);
			if (!channel) {
				eventlog(eventlog_level_error,"handle_bits_packet","channel #%u not found",cid);
			} else {
				t_bits_channelmember * m;
				
				channel_add_ref(channel);
				m = channel_get_first(channel);
				while (m) {
					send_bits_chat_channel_join(conn,cid,m);
					m = channel_get_next();
				}
			}
			bits_ext_channellist_add(conn,cid);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_SERVER_LEAVE:
		{
			int cid;

			cid = bn_int_get(packet->u.bits_chat_channel_server_leave.channelid);
			channel_del_ref(channellist_find_channel_bychannelid(cid));
			bits_ext_channellist_del(conn,cid);
			if (bits_uplink_connection) {
				if (!bits_ext_channellist_is_needed(cid)) {
					send_bits_chat_channel_server_leave(cid);
				}				
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_JOIN:
		{
			t_channel * channel;
			
			channel = channellist_find_channel_bychannelid(bn_int_get(packet->u.bits_chat_channel_join.channelid));
			if (!channel) {
				eventlog(eventlog_level_error,"handle_bits_packet","user join in unknown channel");
			} else {
				t_bits_channelmember * member;

				member = channel_find_member_bysessionid(channel,bn_int_get(packet->u.bits_chat_channel_join.sessionid));
				if (member) {
				    member->flags = bn_int_get(packet->u.bits_chat_channel_join.flags);
				    member->latency = bn_int_get(packet->u.bits_chat_channel_join.latency);	
				} else {
				    channel_add_member(channel,
				                   bn_int_get(packet->u.bits_chat_channel_join.sessionid),
				                   bn_int_get(packet->u.bits_chat_channel_join.flags),
				                   bn_int_get(packet->u.bits_chat_channel_join.latency));
				}
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_LEAVE:
		{
		    t_channel * channel;
			
		    channel = channellist_find_channel_bychannelid(bn_int_get(packet->u.bits_chat_channel_leave.channelid));
		    if (!channel) {
			eventlog(eventlog_level_error,"handle_bits_packet","user leave in unknown channel");
		    } else {
			t_bits_channelmember * member;

			member = channel_find_member_bysessionid(channel,bn_int_get(packet->u.bits_chat_channel_leave.sessionid));
			if (member) {
			    channel_del_member(channel,member);
			} /* else we just assume that the user was from this server and was removed earlier */
		    }
		}
		break;	
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_JOIN_REQUEST:
		{
			t_channel * channel;
			t_packet * p;

			if (bits_uplink_connection) {
				break;
			}
			p = packet_create(packet_class_bits);
			if (!p) {
				eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
				break;
			}
			packet_set_size(p,sizeof(t_bits_chat_channel_join_reply));
			packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_REPLY);
			bn_int_set(&p->u.bits_chat_channel_join_reply.qid,bn_int_get(packet->u.bits_chat_channel_join_request.qid));
			bn_int_set(&p->u.bits_chat_channel_join_reply.channelid,bn_int_get(packet->u.bits_chat_channel_join_request.channelid));
		        bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_OK);

			channel = channellist_find_channel_bychannelid(bn_int_get(packet->u.bits_chat_channel_join_request.channelid));
			if (!channel) {
				eventlog(eventlog_level_error,"handle_bits_packet","user join in unknown channel");
				bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_CHANNELDOESNOTEXIST);
			} else {
				t_bits_channelmember * member;

				member = channel_find_member_bysessionid(channel,bn_int_get(packet->u.bits_chat_channel_join_request.sessionid));
				if (member) {
				    bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_CHANNELRESTRICTED);
				} else {
				    channel_add_member(channel,
				                   bn_int_get(packet->u.bits_chat_channel_join_request.sessionid),
				                   bn_int_get(packet->u.bits_chat_channel_join_request.flags),
				                   bn_int_get(packet->u.bits_chat_channel_join_request.latency));
				}
			}
			bits_packet_generic(p,bits_packet_get_src(packet));
			send_bits_packet(p);
			packet_del_ref(p);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_JOIN_NEW_REQUEST:
		{
			t_channel * channel;
			t_packet * p;
			char const * channelname;

			if (!bits_master) {
				break;
			}
			p = packet_create(packet_class_bits);
			if (!p) {
				eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
				break;
			}
			packet_set_size(p,sizeof(t_bits_chat_channel_join_reply));
			packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_REPLY);
			bn_int_set(&p->u.bits_chat_channel_join_reply.qid,bn_int_get(packet->u.bits_chat_channel_join_new_request.qid));
		        bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_OK);
			
			channelname = packet_get_str_const(packet,sizeof(t_bits_chat_channel_join_new_request),CHANNEL_NAME_LEN);
			channel = channellist_find_channel_by_name(channelname,NULL, NULL);
			if (channel) {
				eventlog(eventlog_level_error,"handle_bits_packet","tried to create an existing channel");
				bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_CHANNELFULL); /* just for fun ;) */
			} else {
				    channel = channel_create(channelname,
				                             NULL,
				                             NULL,
				                             0,1,1,0,NULL,NULL,0); /* FIXME: Ross added this as a quick fix... */
				    bn_int_set(&p->u.bits_chat_channel_join_reply.channelid,channel_get_channelid(channel));                         
				    channel_add_member(channel,
				                   bn_int_get(packet->u.bits_chat_channel_join_new_request.sessionid),
				                   bn_int_get(packet->u.bits_chat_channel_join_new_request.flags),
				                   bn_int_get(packet->u.bits_chat_channel_join_new_request.latency));
			}
			bits_packet_generic(p,bits_packet_get_src(packet));
			send_bits_packet(p);
			packet_del_ref(p);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_JOIN_PERM_REQUEST:
		{
			t_channel * channel;
			t_packet * p;
			char const * name;
			char const * country;

			if (bits_uplink_connection) {
				break;
			}

			if (!(name = packet_get_str_const(packet,sizeof(t_bits_chat_channel_join_perm_request),CHANNEL_NAME_LEN))) {
				eventlog(eventlog_level_error,"handle_bits_packet","got malformed BITS_CHAT_CHANNEL_JOIN_PERM_REQUEST packet (no name)");
				break;
			}
			if (!(country = packet_get_str_const(packet,sizeof(t_bits_chat_channel_join_perm_request)+strlen(name)+1,CHANNEL_NAME_LEN))) {
				eventlog(eventlog_level_error,"handle_bits_packet","got malformed BITS_CHAT_CHANNEL_JOIN_PERM_REQUEST packet (no country)");
				break;
			}
			
			p = packet_create(packet_class_bits);
			if (!p) {
				eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
				break;
			}
			packet_set_size(p,sizeof(t_bits_chat_channel_join_reply));
			packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_REPLY);
			bn_int_set(&p->u.bits_chat_channel_join_reply.qid,bn_int_get(packet->u.bits_chat_channel_join_perm_request.qid));
		        bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_OK);

			channel = channellist_find_channel_by_name(name,country, NULL); /* FIXME: realm? */
			if (!channel) {
				eventlog(eventlog_level_error,"handle_bits_packet","permanent channel \"%s\" (%s) doesn't exist",name,country);
				bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_CHANNELDOESNOTEXIST);
			} else {
				t_bits_channelmember * member;

				member = channel_find_member_bysessionid(channel,bn_int_get(packet->u.bits_chat_channel_join_perm_request.sessionid));
				if (member) {
				    bn_byte_set(&p->u.bits_chat_channel_join_reply.status,BITS_STATUS_CHANNELRESTRICTED);
				} else {
				    channel_add_member(channel,
				                   bn_int_get(packet->u.bits_chat_channel_join_perm_request.sessionid),
				                   bn_int_get(packet->u.bits_chat_channel_join_perm_request.flags),
				                   bn_int_get(packet->u.bits_chat_channel_join_perm_request.latency));
				}
				bn_int_set(&p->u.bits_chat_channel_join_reply.channelid,channel_get_channelid(channel));
			}
			bits_packet_generic(p,bits_packet_get_src(packet));
			send_bits_packet(p);
			packet_del_ref(p);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL_JOIN_REPLY:
		{
			t_query * q;

			q = query_find_byqid(bn_int_get(packet->u.bits_chat_channel_join_reply.qid));
			if (q) {
				query_answer(q,packet);
			} else {
				eventlog(eventlog_level_error,"handle_bits_packet","got BITS_CHAT_CHANNEL_JOIN_REPLY for unknown query");
			}
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_CHAT_CHANNEL:
		{
			t_packet * p;
			t_connection *c;
			t_elem const * curr;
			t_connection *src;
			char const * text = NULL;
			t_bits_loginlist_entry *lle;

			p = packet_duplicate(packet);
			lle = bits_loginlist_bysessionid(bn_int_get(packet->u.bits_chat_channel.sessionid));
			if (!lle) {
				eventlog(eventlog_level_error,"handle_bits_packet(BITS_CHAT_CHANNEL)","source user is not logged in (anymore)... ignoring message");
				break;
			}
			src = bits_rconn_create(bn_int_get(packet->u.bits_chat_channel.flags),
			                        bn_int_get(packet->u.bits_chat_channel.latency),
			                        lle->sessionid);
			conn_set_botuser(src,lle->chatname);
			if (packet_get_size(packet)>sizeof(t_bits_chat_channel))
				text = packet_get_str_const(packet,sizeof(t_bits_chat_channel),MAX_MESSAGE_LEN);

			LIST_TRAVERSE_CONST(connlist(),curr)
			{
				c = elem_get_data(curr);
				if (conn_get_sessionid(c) == lle->sessionid)
				    continue; /* don't send the message to the sender */
				if ((conn_get_class(c) == conn_class_bnet)||(conn_get_class(c) == conn_class_bot)) {
				    if (conn_get_channel(c)) {
					if (channel_get_channelid(conn_get_channel(c))==bn_int_get(packet->u.bits_chat_channel.channelid)) {
					    message_send_text(c,bn_int_get(packet->u.bits_chat_channel.type),src,text);
					}	
				    }
				}
			}
			bits_rconn_destroy(src);
			send_bits_packet_for_channel(p,bn_int_get(packet->u.bits_chat_channel.channelid),conn);
			packet_del_ref(p);		
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAMELIST_ADD:
		{
			t_bits_game * game;
			
			if (bits_packet_get_dst(packet)==BITS_ADDR_PEER) {
				t_packet * p2 = packet_duplicate(packet);
				send_bits_packet_down(p2);
				packet_del_ref(p2);
			}
			/* evaluate packet */
			game = malloc(sizeof(t_bits_game));
			if (!game) {
				eventlog(eventlog_level_error,"handle_bits_packet(BITS_GAMELIST_ADD)","malloc failed: %s",strerror(errno));
				break;
			}
			game->id = bn_int_get(packet->u.bits_gamelist_add.id);
			game->type = bn_int_get(packet->u.bits_gamelist_add.type);
			game->status = bn_int_get(packet->u.bits_gamelist_add.status);
			memcpy(&game->clienttag,&packet->u.bits_gamelist_add.clienttag,4);
			game->owner = bn_int_get(packet->u.bits_gamelist_add.owner);
			game->name = strdup(packet_get_str_const(packet,sizeof(t_bits_gamelist_add),GAME_NAME_LEN));
			game->password = strdup(packet_get_str_const(packet,sizeof(t_bits_gamelist_add)+strlen(game->name)+1,GAME_PASS_LEN));
			game->info = strdup(packet_get_str_const(packet,sizeof(t_bits_gamelist_add)+strlen(game->name)+strlen(game->password)+2,GAME_INFO_LEN));

			eventlog(eventlog_level_debug,"handle_bits_packet(BITS_GAMELIST_ADD)","added game: id=0x%08x owner=0x%08x name=\"%s\" password=\"%s\" info=\"%s\"",game->id,game->owner,game->name,game->password,game->info);
			bits_gamelist_add(game);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAMELIST_DEL:
		{
			t_bits_game * game;
			
			if (bits_packet_get_dst(packet)==BITS_ADDR_PEER) {
				t_packet * p2 = packet_duplicate(packet);
				send_bits_packet_down(p2);
				packet_del_ref(p2);
			}

			game = bits_gamelist_find_game_byid(bn_int_get(packet->u.bits_gamelist_del.id));
			if (!game) {
				eventlog(eventlog_level_error,"handle_bits_packet(BITS_GAMELIST_DEL)","no game with id=0x%08x found",bn_int_get(packet->u.bits_gamelist_del.id));
				break;
			}
			bits_gamelist_del(game);
			if (game->name) free((void *) game->name); /* avoid warning */
			if (game->password) free((void *) game->password); /* avoid warning */
			if (game->info) free((void *) game->info); /* avoid warning */
			free(game);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_UPDATE:
		{
		    t_bits_game * bg;
		    t_game * g = NULL;
		    int update_master = 0;

		    bg = bits_gamelist_find_game_byid(bn_int_get(packet->u.bits_game_update.id));
		    if (!bg) {
			eventlog(eventlog_level_warn,"handle_bits_packet(BITS_GAME_UPDATE)","no bits game with id=0x%08x found",bn_int_get(packet->u.bits_game_update.id));
			break;
		    }
		    if (bits_master) {
			g = gamelist_find_game_byid(bn_int_get(packet->u.bits_game_update.id));
			if (!g) {
			    eventlog(eventlog_level_warn,"handle_bits_packet","BITS_GAME_UPDATE: no game with id=0x%08x found",bn_int_get(packet->u.bits_game_update.id));
			    break;
			}
			if (bits_packet_get_src(packet)!=BITS_ADDR_MASTER) 
			    update_master = 1;
		    }
		    switch (bn_byte_get(packet->u.bits_game_update.field)) {
		    case BITS_GAME_UPDATE_STATUS:
		    	{
			bn_int * status;

			status = (bn_int *) packet_get_data_const(packet, sizeof(t_bits_game_update), 4);
			eventlog(eventlog_level_debug,"handle_bits_packet","setting game 0x%08x status to %s",bits_game_get_id(bg),game_status_get_str(bn_int_get(*status)));
		 	if (update_master) {
				game_set_status(g,bn_int_get(*status));
		    	}
			bg->status=bn_int_get(*status);
			}
		 	break;
		    case BITS_GAME_UPDATE_OWNER:
			{
			bn_int * owner;

			owner = (bn_int *) packet_get_data_const(packet, sizeof(t_bits_game_update), 4);
			eventlog(eventlog_level_debug,"handle_bits_packet","setting game 0x%08x owner to 0x%08x",bits_game_get_id(bg),bn_int_get(*owner));
		 	if (update_master) {
				game_set_owner(g,bn_int_get(*owner));
		    	} 
			bg->owner = bn_int_get(*owner);
			}
			break;
		    case BITS_GAME_UPDATE_OPTION:
			{
			bn_int * option;

			option = (bn_int *) packet_get_data_const(packet, sizeof(t_bits_game_update), 4);
			eventlog(eventlog_level_debug,"handle_bits_packet","setting game 0x%08x option to %s",bits_game_get_id(bg),game_option_get_str(bn_int_get(*option)));
		 	if (update_master) {
				game_set_option(g,bn_int_get(*option));
		    	} 
			}
			break;
		    }
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_JOIN_REQUEST:
		{
		    t_packet * rpacket;
		    t_game * game;

		    if (bits_uplink_connection) {
			eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_GAME_JOIN_REQUEST although not master");
			break; /* ignore it */
		    }
		    rpacket = packet_create(packet_class_bits);
		    if (!rpacket) {
			eventlog(eventlog_level_error,"handle_bits_packet(BITS_GAME_JOIN_REQUEST)","could not create packet");
			break;
		    }
		    packet_set_size(rpacket,sizeof(t_bits_game_join_reply));
		    packet_set_type(rpacket,BITS_GAME_JOIN_REPLY);
		    bits_packet_generic(rpacket,bits_packet_get_src(packet));
		    bn_int_set(&rpacket->u.bits_game_join_reply.id,bn_int_get(packet->u.bits_game_join_request.id));
		    bn_int_set(&rpacket->u.bits_game_join_reply.sessionid,bn_int_get(packet->u.bits_game_join_request.sessionid));

		    game = gamelist_find_game_byid(bn_int_get(packet->u.bits_game_join_request.id));
		    if (!game) {
			eventlog(eventlog_level_error,"handle_bits_packet(BITS_GAME_JOIN_REQUEST)","can't find game with id=0x%08x",bn_int_get(packet->u.bits_game_join_request.id));
			bn_byte_set(&rpacket->u.bits_game_join_reply.status,BITS_STATUS_FAIL);
		    	send_bits_packet(rpacket);
		    	packet_del_ref(rpacket);
			break;
		    }
		    if (game_add_player(game,NULL,bn_int_get(packet->u.bits_game_join_request.version),bn_int_get(packet->u.bits_game_join_request.sessionid))<0) {
			bn_byte_set(&rpacket->u.bits_game_join_reply.status,BITS_STATUS_FAIL);
		    } else {
			bn_byte_set(&rpacket->u.bits_game_join_reply.status,BITS_STATUS_OK);
		    }
		    send_bits_packet(rpacket);
		    packet_del_ref(rpacket);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_JOIN_REPLY:
		{
		    if (!bits_status_ok(packet->u.bits_game_join_reply.status)) {
			eventlog(eventlog_level_debug,"handle_bits_packet","game hoin failed for user 0x%08x",bn_int_get(packet->u.bits_game_join_reply.sessionid));
		    	break; /* join failed */
		    }
		    /* Now we can set the gameid of the user ... */
		    conn_set_bits_game(connlist_find_connection_by_sessionid(bn_int_get(packet->u.bits_game_join_reply.sessionid)),
		    		       bn_int_get(packet->u.bits_game_join_reply.id));
		    eventlog(eventlog_level_debug,"handle_bits_packet","user 0x%08x joined game 0x%08x",bn_int_get(packet->u.bits_game_join_reply.sessionid),bn_int_get(packet->u.bits_game_join_reply.id));
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_LEAVE:
		{
		    t_game * game;
		    
		    if (!bits_master) {
			eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_GAME_LEAVE although not master");
			break; /* ignore it */
		    }
		    game = gamelist_find_game_byid(bn_int_get(packet->u.bits_game_leave.id));
		    if (!game) {
			eventlog(eventlog_level_error,"handle_bits_packet(BITS_GAME_LEAVE)","can't find game with id=0x%08x",bn_int_get(packet->u.bits_game_leave.id));
			break;			
		    }		    
		    game_del_player(game,bn_int_get(packet->u.bits_game_leave.sessionid));
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_CREATE_REQUEST:
		{
		    char const * gamename;
		    char const * gamepass;
		    char const * gameinfo;
		    t_game_type type;
		    int version;
		    unsigned int sessionid;
		    char ct[5];
		    t_bits_loginlist_entry * lle;
		    t_packet *rpacket;
	            t_game * game;

		    if (!bits_master) {
			eventlog(eventlog_level_error,"handle_bits_packet","got BITS_GAME_CREATE_REQUEST although not master");
			break;
		    }
		    gamename = packet_get_str_const(packet,sizeof(t_bits_game_create_request),GAME_NAME_LEN);
		    gamepass = packet_get_str_const(packet,sizeof(t_bits_game_create_request)+strlen(gamename)+1,GAME_PASS_LEN);
		    gameinfo = packet_get_str_const(packet,(sizeof(t_bits_game_create_request)+strlen(gamename)+strlen(gamepass)+2),GAME_INFO_LEN);
		    type = bn_int_get(packet->u.bits_game_create_request.type);
		    version = bn_int_get(packet->u.bits_game_create_request.version);
		    sessionid = bn_int_get(packet->u.bits_game_create_request.sessionid);
		    lle = bits_loginlist_bysessionid(sessionid);
		    if (!lle) {
			eventlog(eventlog_level_warn,"handle_bits_packet(BITS_GAME_CREATE_REQUEST)","no such sessionid 0x%08x",sessionid);
			break;
		    }
		    ct[4]='\0';
		    memcpy(ct,lle->clienttag,4);
		    eventlog(eventlog_level_debug,"handle_bits_packet","BITS_GAME_CREATE_REQUEST: gamename=\"%s\" gamepass=\"%s\" gameinfo=\"%s\" type=\"%s\" version=%d owner=0x%08x clienttag=\"%s\"",gamename,gamepass,gameinfo,game_type_get_str(type),version,sessionid,ct);
		    rpacket = packet_create(packet_class_bits);
		    if (!rpacket) {
			eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
			break;
		    }
		    packet_set_size(rpacket,sizeof(t_bits_game_create_reply));
		    packet_set_type(rpacket,BITS_GAME_CREATE_REPLY);
		    bits_packet_generic(rpacket,bits_packet_get_src(packet));
		    bn_int_set(&rpacket->u.bits_game_create_reply.sessionid,sessionid);
		    bn_int_set(&rpacket->u.bits_game_create_reply.version,version);
		    game = game_create(gamename,gamepass,gameinfo,type,version,ct);
		    if (!game) {
			bn_byte_set(&rpacket->u.bits_game_create_reply.status,BITS_STATUS_FAIL);
		    } else {
			bn_byte_set(&rpacket->u.bits_game_create_reply.status,BITS_STATUS_OK);
		    	bn_int_set(&rpacket->u.bits_game_create_reply.gameid,game_get_id(game));
			/* Now setup all the other stuff */
			game_set_option(game,bn_int_get(packet->u.bits_game_create_request.option));
			game_set_owner(game,sessionid);
			send_bits_gamelist_add(NULL,game);
		    }
		    send_bits_packet(rpacket);
		    packet_del_ref(rpacket);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_CREATE_REPLY:
		{
		    t_packet * rpacket;
		    t_connection * rc;
		    int version;

		    rpacket = packet_create(packet_class_bnet);
		    if (!rpacket)
		    {
			eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
			break;
		    }
		    version = bn_int_get(packet->u.bits_game_create_reply.version);

		    if (version==STARTVER_GW4) { /* Game version 4 */
			packet_set_size(rpacket,sizeof(t_server_startgame4_ack));
			packet_set_type(rpacket,SERVER_STARTGAME4_ACK);
			
			if (bits_status_ok(packet->u.bits_game_create_reply.status)) {
			    bn_int_set(&rpacket->u.server_startgame4_ack.reply,SERVER_STARTGAME4_ACK_OK);
			    send_bits_game_join_request(bn_int_get(packet->u.bits_game_create_reply.sessionid),bn_int_get(packet->u.bits_game_create_reply.gameid),STARTVER_GW4);
			} else
		    	    bn_int_set(&rpacket->u.server_startgame4_ack.reply,SERVER_STARTGAME4_ACK_NO);
		    } else if (version==STARTVER_GW3) { /* Game version 3 */
		    	packet_set_size(rpacket,sizeof(t_server_startgame3_ack));
			packet_set_type(rpacket,SERVER_STARTGAME3_ACK);
			    
			if (bits_status_ok(packet->u.bits_game_create_reply.status)) {
			    bn_int_set(&rpacket->u.server_startgame3_ack.reply,SERVER_STARTGAME3_ACK_OK);
			    send_bits_game_join_request(bn_int_get(packet->u.bits_game_create_reply.sessionid),bn_int_get(packet->u.bits_game_create_reply.gameid),STARTVER_GW3);
			} else
			    bn_int_set(&rpacket->u.server_startgame3_ack.reply,SERVER_STARTGAME3_ACK_NO);
		    } else if (version==STARTVER_GW1) { /* Game version 1 */
			packet_set_size(rpacket,sizeof(t_server_startgame1_ack));
			packet_set_type(rpacket,SERVER_STARTGAME1_ACK);
			    
			if (bits_status_ok(packet->u.bits_game_create_reply.status)) {
			    bn_int_set(&rpacket->u.server_startgame1_ack.reply,SERVER_STARTGAME1_ACK_OK);
			    send_bits_game_join_request(bn_int_get(packet->u.bits_game_create_reply.sessionid),bn_int_get(packet->u.bits_game_create_reply.gameid),STARTVER_GW3);
			} else
			    bn_int_set(&rpacket->u.server_startgame1_ack.reply,SERVER_STARTGAME1_ACK_NO);
		    } else {
			eventlog(eventlog_level_warn,"handle_bits_packet","got unsupported game version %d",version);
			packet_del_ref(rpacket);
			break;
		    }

		    rc = connlist_find_connection_by_sessionid(bn_int_get(packet->u.bits_game_create_reply.sessionid));
		    if (rc) {
	            	queue_push_packet(conn_get_out_queue(rc),rpacket);
	            	conn_set_bits_game(rc,bn_int_get(packet->u.bits_game_create_reply.gameid));
		    } else {
			eventlog(eventlog_level_error,"handle_bits_packet","no local connection with sessionid 0x%08x found",bn_int_get(packet->u.bits_game_create_reply.sessionid));
		    }
		    packet_del_ref(rpacket);
		}
		break;
	/* ------------------------------------------------- */	
		case BITS_GAME_REPORT:
		{
		    t_packet * p;
		    t_connection * rconn;

		    if (bits_uplink_connection) {
			eventlog(eventlog_level_warn,"handle_bits_packet","got BITS_GAME_REPORT although not master");
			break;
		    }
		    
		    p = packet_create(packet_class_bnet);
		    if (!p) {
			eventlog(eventlog_level_error,"handle_bits_packet","could not create packet");
			break;
		    }
		    
		    packet_append_data(p,packet_get_data_const(packet,sizeof(t_bits_game_report),bn_int_get(packet->u.bits_game_report.len)),bn_int_get(packet->u.bits_game_report.len));

		    rconn = bits_rconn_create_by_sessionid(bn_int_get(packet->u.bits_game_report.sessionid));
		    if (!rconn) {
			eventlog(eventlog_level_error,"handle_bits_packet","could not create rconn");
			packet_del_ref(p);
			break;
		    }
		    bits_rconn_set_game(rconn,gamelist_find_game_byid(bn_int_get(packet->u.bits_game_report.gameid)));

		    /* All information complete: ready to handle the packet*/
		    handle_bnet_packet(rconn,p);

		    bits_rconn_destroy(rconn);
		}
		break;
	/* ------------------------------------------------- */	
		default:
			eventlog(eventlog_level_info,"handle_bits_packet","unknown bits packet type [0x%04x]",packet_get_type(packet));
			return -1;
	}
	return 0;
}

#endif


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
#include "setup.h"

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h> /* needed to include netinet/in.h */
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h> /* FIXME: probably not needed... do some systems put types in here or someth
ing? */
#endif
#include "compat/psock.h"

#include "d2gs.h"
#include "handle_d2gs.h"
#include "serverqueue.h"
#include "game.h"
#include "connection.h"
#include "prefs.h"
#include "d2cs_d2gs_protocol.h"
#include "common/addr.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/bn_type.h"
#include "common/packet.h"
#include "common/setup_after.h"

DECLARE_PACKET_HANDLER(on_d2gs_authreply)
DECLARE_PACKET_HANDLER(on_d2gs_setgsinfo)
DECLARE_PACKET_HANDLER(on_d2gs_echoreply)
DECLARE_PACKET_HANDLER(on_d2gs_creategamereply)
DECLARE_PACKET_HANDLER(on_d2gs_joingamereply)
DECLARE_PACKET_HANDLER(on_d2gs_updategameinfo)
DECLARE_PACKET_HANDLER(on_d2gs_closegame)

static t_packet_handle_table d2gs_packet_handle_table[]={
/* FIXME: are these supposed to be zero/NULL? If so, make that explicit. */
	{}, {}, {}, {},  {}, {}, {}, {},  {}, {}, {}, {},  {}, {}, {}, {},
	{}, 
	{ sizeof(t_d2gs_d2cs_authreply),	conn_state_connected,	on_d2gs_authreply		},
	{ sizeof(t_d2gs_d2cs_setgsinfo),	conn_state_authed,	on_d2gs_setgsinfo		},
	{ sizeof(t_d2gs_d2cs_echoreply),	conn_state_any,		on_d2gs_echoreply		},
	{}, {}, {}, {},  {}, {}, {}, {},  {}, {}, {}, {},
	{ sizeof(t_d2gs_d2cs_creategamereply),	conn_state_authed,	on_d2gs_creategamereply		},
	{ sizeof(t_d2gs_d2cs_joingamereply),	conn_state_authed,	on_d2gs_joingamereply		},
	{ sizeof(t_d2gs_d2cs_updategameinfo),	conn_state_authed,	on_d2gs_updategameinfo		},
	{ sizeof(t_d2gs_d2cs_closegame),	conn_state_authed,	on_d2gs_closegame		},
};

static int on_d2gs_authreply(t_connection * c, t_packet * packet)
{
	t_d2gs		* gs;
	t_packet	* rpacket;
	unsigned int	reply;
	unsigned int	try_checksum, checksum;
	unsigned int	version;

	if (!(gs=d2gslist_find_gs(conn_get_d2gs_id(c)))) {
		log_error("game server %d not found",conn_get_d2gs_id(c));
		return -1;
	}

	version=bn_int_get(packet->u.d2gs_d2cs_authreply.version);
	try_checksum=bn_int_get(packet->u.d2gs_d2cs_authreply.checksum);
	checksum=d2gs_calc_checksum(c);
	if (prefs_get_d2gs_version() && (version != prefs_get_d2gs_version())) {
		log_error("game server %d version mismatch 0x%X - 0x%X",conn_get_d2gs_id(c),\
			version,prefs_get_d2gs_version());
		reply=D2CS_D2GS_AUTHREPLY_BAD_VERSION;
	} else if (prefs_get_d2gs_checksum() && try_checksum != checksum) {
		log_error("game server %d checksum mismach 0x%X - 0x%X",conn_get_d2gs_id(c),try_checksum,checksum);
		reply=D2CS_D2GS_AUTHREPLY_BAD_CHECKSUM;
	} else {
		reply=D2CS_D2GS_AUTHREPLY_SUCCEED;
	}

	if (reply==D2CS_D2GS_AUTHREPLY_SUCCEED) {
		log_info("game server %s authed",addr_num_to_ip_str(conn_get_addr(c)));
		conn_set_state(c,conn_state_authed);
		d2gs_active(gs,c);
	} else {
		log_error("game server %s failed to auth",addr_num_to_ip_str(conn_get_addr(c)));
		/* 
		conn_set_state(c,conn_state_destroy); 
		*/
	}
	if ((rpacket=packet_create(packet_class_d2gs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_d2gs_authreply));
		packet_set_type(rpacket,D2CS_D2GS_AUTHREPLY);
		bn_int_set(&rpacket->u.d2cs_d2gs_authreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_d2gs_setgsinfo(t_connection * c, t_packet * packet)
{
	t_d2gs		* gs;
	unsigned int	maxgame, prev_maxgame, i;

	if (!(gs=d2gslist_find_gs(conn_get_d2gs_id(c)))) {
		log_error("game server %d not found",conn_get_d2gs_id(c));
		return -1;
	}
	maxgame=bn_int_get(packet->u.d2gs_d2cs_setgsinfo.maxgame);
	prev_maxgame=d2gs_get_maxgame(gs);
	log_info("change game server %s max game from %d to %d",addr_num_to_ip_str(conn_get_addr(c)),\
		prev_maxgame, maxgame);
	d2gs_set_maxgame(gs,maxgame);
	for (i=prev_maxgame; i<maxgame; i++) gqlist_check_creategame();
	return 0;
}

static int on_d2gs_echoreply(t_connection * c, t_packet * packet)
{
	return 0;
}

static int on_d2gs_creategamereply(t_connection * c, t_packet * packet)
{
	t_packet	* opacket, * rpacket;
	t_sq		* sq;
	t_connection	* client;
	t_game		* game;
	int		result;
	int		reply;
	int		seqno;

	seqno=bn_int_get(packet->u.d2cs_d2gs.h.seqno);
	if (!(sq=sqlist_find_sq(seqno))) {
		log_error("seqno %d not found",seqno);
		return 0;
	}
	if (!(client=connlist_find_connection_by_sessionnum(sq_get_clientid(sq)))) {
		log_error("client %d not found",sq_get_clientid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(game=gamelist_find_game_by_id(sq_get_gameid(sq)))) {
		log_error("game %d not found",sq_get_gameid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(opacket=sq_get_packet(sq))) {
		log_error("previous packet not found (seqno: %d)",seqno);
		sq_destroy(sq);
		return 0;
	}

	result=bn_int_get(packet->u.d2gs_d2cs_creategamereply.result);
	if (result==D2GS_D2CS_CREATEGAME_SUCCEED) {
		game_set_d2gs_gameid(game,bn_int_get(packet->u.d2gs_d2cs_creategamereply.gameid));
		game_set_created(game,1);
		log_info("game %s created on gs %d",game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_CREATEGAMEREPLY_SUCCEED;
	} else {
		log_warn("failed to create game %s on gs %d",game_get_name(game),conn_get_d2gs_id(c));
		game_destroy(game);
		reply=D2CS_CLIENT_CREATEGAMEREPLY_FAILED;
	}

	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_creategamereply));
		packet_set_type(rpacket,D2CS_CLIENT_CREATEGAMEREPLY);
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.seqno,\
				bn_short_get(opacket->u.client_d2cs_creategamereq.seqno));
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.gameid,1);
		bn_short_set(&rpacket->u.d2cs_client_creategamereply.u1,1);
		bn_int_set(&rpacket->u.d2cs_client_creategamereply.reply,reply);
		queue_push_packet(conn_get_out_queue(client),rpacket);
		packet_del_ref(rpacket);
	}
	sq_destroy(sq);
	return 0;
}

static int on_d2gs_joingamereply(t_connection * c, t_packet * packet)
{
	t_sq		* sq;
	t_d2gs		* gs;
	t_connection	* client;
	t_game		* game;
	t_packet	* opacket, * rpacket;
	int		result;
	int		reply;
	int		seqno;

	seqno=bn_int_get(packet->u.d2cs_d2gs.h.seqno);
	if (!(sq=sqlist_find_sq(seqno))) {
		log_error("seqno %d not found",seqno);
		return 0;
	}
	if (!(client=connlist_find_connection_by_sessionnum(sq_get_clientid(sq)))) {
		log_error("client %d not found",sq_get_clientid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(game=gamelist_find_game_by_id(sq_get_gameid(sq)))) {
		log_error("game %d not found",sq_get_gameid(sq));
		sq_destroy(sq);
		return 0;
	}
	if (!(gs=game_get_d2gs(game))) {
		log_error("try join game without game server set");
		sq_destroy(sq);
		return 0;
	}
	if (!(opacket=sq_get_packet(sq))) {
		log_error("previous packet not found (seqno: %d)",seqno);
		sq_destroy(sq);
		return 0;
	}

	result=bn_int_get(packet->u.d2gs_d2cs_joingamereply.result);
	if (result==D2GS_D2CS_JOINGAME_SUCCEED) {
		log_info("added %s to game %s on gs %d",conn_get_charname(client),\
			game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED;
	} else {
		log_info("failed to add %s to game %s on gs %d",conn_get_charname(client),\
			game_get_name(game),conn_get_d2gs_id(c));
		reply=D2CS_CLIENT_JOINGAMEREPLY_FAILED;
	}

	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_joingamereply));
		packet_set_type(rpacket,D2CS_CLIENT_JOINGAMEREPLY);
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.seqno,\
				bn_short_get(opacket->u.client_d2cs_joingamereq.seqno));
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.gameid,game_get_d2gs_gameid(game));
		bn_short_set(&rpacket->u.d2cs_client_joingamereply.u1,0);
		bn_int_set(&rpacket->u.d2cs_client_joingamereply.reply,reply);
		if (reply == SERVER_JOINGAMEREPLY2_REPLY_OK) {
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.token,sq_get_gametoken(sq));
			bn_int_nset(&rpacket->u.d2cs_client_joingamereply.addr,d2gs_get_ip(gs));
		} else {
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.token,0);
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.addr,0);
		}
		queue_push_packet(conn_get_out_queue(client),rpacket);
		packet_del_ref(rpacket);
	}
	sq_destroy(sq);
	return 0;
}

static int on_d2gs_updategameinfo(t_connection * c, t_packet * packet)
{
	unsigned int	flag;
	char const	* charname;
	t_game		* game;
	unsigned int	charclass;
	unsigned int	charlevel;
	unsigned int	d2gs_gameid;
	unsigned int	d2gs_id;

	if (!(charname=packet_get_str_const(packet,sizeof(t_d2gs_d2cs_updategameinfo),MAX_CHARNAME_LEN))) {
		log_error("got bad charname");
		return 0;
	}
	d2gs_id=conn_get_d2gs_id(c);
	d2gs_gameid=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.gameid);
	charclass=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.charclass);
	charlevel=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.charlevel);
	flag=bn_int_get(packet->u.d2gs_d2cs_updategameinfo.flag);
	if (!(game=gamelist_find_game_by_d2gs_and_id(d2gs_id,d2gs_gameid))) {
		log_error("game %d not found on gs %d",d2gs_gameid,d2gs_id);
		return -1;
	}
	if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_ENTER) {
		game_add_character(game,charname,charclass,charlevel);
	} else if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_LEAVE) {
		game_del_character(game,charname);
	} else if (flag==D2GS_D2CS_UPDATEGAMEINFO_FLAG_UPDATE) {
		game_add_character(game,charname,charclass,charlevel);
	} else {
		log_error("got bad updategameinfo flag %d",flag);
	}
	return 0;
}

static int on_d2gs_closegame(t_connection * c, t_packet * packet)
{
	t_game	* game;

	if (!(game=gamelist_find_game_by_d2gs_and_id(conn_get_d2gs_id(c),\
		bn_int_get(packet->u.d2gs_d2cs_closegame.gameid)))) {
		return 0;
	}
	game_destroy(game);
	return 0;
}

extern int handle_d2gs_packet(t_connection * c, t_packet * packet)
{
	conn_process_packet(c,packet,d2gs_packet_handle_table,NELEMS(d2gs_packet_handle_table));
	return 0;
}

extern int handle_d2gs_init(t_connection * c)
{
	t_packet	* packet;

	if ((packet=packet_create(packet_class_d2gs))) {
		packet_set_size(packet,sizeof(t_d2cs_d2gs_authreq));
		packet_set_type(packet,D2CS_D2GS_AUTHREQ);
		bn_int_set(&packet->u.d2cs_d2gs_authreq.sessionnum,conn_get_sessionnum(c));
		packet_append_string(packet,prefs_get_realmname());
		queue_push_packet(conn_get_out_queue(c),packet);
		packet_del_ref(packet);
	}
	log_info("sent init packet to d2gs %d (sessionnum=%d)",conn_get_d2gs_id(c),conn_get_sessionnum(c));
	return 0;
}

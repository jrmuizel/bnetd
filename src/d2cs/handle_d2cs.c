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

#include <stdio.h>
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
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "compat/memcpy.h"
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#include "compat/pdir.h"
#include "d2charfile.h"
#include "connection.h"
#include "game.h"
#include "bnetd.h"
#include "d2cs_protocol.h"
#include "d2cs_bnetd_protocol.h"
#include "handle_d2cs.h"
#include "d2ladder.h"
#include "gamequeue.h"
#include "serverqueue.h"
#include "prefs.h"
#include "common/bn_type.h"
#include "common/queue.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static int d2cs_send_client_ladder(t_connection * c, unsigned char type, unsigned short from);
static unsigned int d2cs_try_joingame(t_connection const * c, t_game const * game, char const * gamepass);

DECLARE_PACKET_HANDLER(on_client_loginreq)
DECLARE_PACKET_HANDLER(on_client_createcharreq)
DECLARE_PACKET_HANDLER(on_client_creategamereq)
DECLARE_PACKET_HANDLER(on_client_joingamereq)
DECLARE_PACKET_HANDLER(on_client_gamelistreq)
DECLARE_PACKET_HANDLER(on_client_gameinforeq)
DECLARE_PACKET_HANDLER(on_client_charloginreq)
DECLARE_PACKET_HANDLER(on_client_deletecharreq)
DECLARE_PACKET_HANDLER(on_client_ladderreq)
DECLARE_PACKET_HANDLER(on_client_motdreq)
DECLARE_PACKET_HANDLER(on_client_cancelcreategame)
DECLARE_PACKET_HANDLER(on_client_charladderreq)
DECLARE_PACKET_HANDLER(on_client_charlistreq)
DECLARE_PACKET_HANDLER(on_client_convertcharreq)


static t_packet_handle_table d2cs_packet_handle_table[]={
	{},
	{ sizeof(t_client_d2cs_loginreq),	conn_state_connected,	on_client_loginreq	},
	{ sizeof(t_client_d2cs_createcharreq),	conn_state_authed|conn_state_char_authed,	
	on_client_createcharreq	},
	{ sizeof(t_client_d2cs_creategamereq),	conn_state_char_authed, on_client_creategamereq	},
	{ sizeof(t_client_d2cs_joingamereq),	conn_state_char_authed, on_client_joingamereq	},
	{ sizeof(t_client_d2cs_gamelistreq),	conn_state_char_authed, on_client_gamelistreq	},
	{ sizeof(t_client_d2cs_gameinforeq),	conn_state_char_authed,	on_client_gameinforeq	},
	{ sizeof(t_client_d2cs_charloginreq),	conn_state_authed|conn_state_char_authed,	
	on_client_charloginreq	},
	{}, {}, 
	{ sizeof(t_client_d2cs_deletecharreq),	conn_state_authed|conn_state_char_authed, 
	on_client_deletecharreq	},
	{}, {}, {}, {}, {}, {}, 
	{ sizeof(t_client_d2cs_ladderreq),	conn_state_char_authed,	on_client_ladderreq	},
	{ sizeof(t_client_d2cs_motdreq),	conn_state_char_authed, on_client_motdreq	},
	{ sizeof(t_client_d2cs_cancelcreategame),conn_state_char_authed,on_client_cancelcreategame},
	{}, {}, 
	{ sizeof(t_client_d2cs_charladderreq),	conn_state_char_authed, on_client_charladderreq	},
	{ sizeof(t_client_d2cs_charlistreq),	conn_state_authed|conn_state_char_authed,
	on_client_charlistreq	},
	{ sizeof(t_client_d2cs_convertcharreq), conn_state_authed|conn_state_char_authed,
	on_client_convertcharreq}
};

extern int handle_d2cs_packet(t_connection * c, t_packet * packet)
{
	return conn_process_packet(c,packet,d2cs_packet_handle_table,NELEMS(d2cs_packet_handle_table));
}

static int on_client_loginreq(t_connection * c, t_packet * packet)
{
	t_packet	* bnpacket;
	char const	* account;
	t_sq		* sq;
	unsigned int	sessionnum;

	if (!(account=packet_get_str_const(packet,sizeof(t_client_d2cs_loginreq),MAX_CHARNAME_LEN))) {
		log_error("got bad account name");
		return -1;
	}
	if (d2char_check_acctname(account)<0) {
		log_error("got bad account name");
		return -1;
	}
	if (!bnetd_conn()) {
		log_warn("d2cs is offline with bnetd, login request will be rejected");
		return -1;
	}
	sessionnum=bn_int_get(packet->u.client_d2cs_loginreq.sessionnum);
	conn_set_bnetd_sessionnum(c,sessionnum);
	log_info("got client (*%s) login request sessionnum=0x%X",account,sessionnum);
	if ((bnpacket=packet_create(packet_class_d2cs_bnetd))) {
		if ((sq=sq_create(conn_get_sessionnum(c),packet,0))) {
			packet_set_size(bnpacket,sizeof(t_d2cs_bnetd_accountloginreq));
			packet_set_type(bnpacket,D2CS_BNETD_ACCOUNTLOGINREQ);
			bn_int_set(&bnpacket->u.d2cs_bnetd_accountloginreq.h.seqno,sq_get_seqno(sq));
			bn_int_set(&bnpacket->u.d2cs_bnetd_accountloginreq.seqno,\
				bn_int_get(packet->u.client_d2cs_loginreq.seqno));
			bn_int_set(&bnpacket->u.d2cs_bnetd_accountloginreq.sessionkey,\
				bn_int_get(packet->u.client_d2cs_loginreq.sessionkey));
			bn_int_set(&bnpacket->u.d2cs_bnetd_accountloginreq.sessionnum,sessionnum);
			memcpy(bnpacket->u.d2cs_bnetd_accountloginreq.secret_hash,\
				packet->u.client_d2cs_loginreq.secret_hash,\
				sizeof(bnpacket->u.d2cs_bnetd_accountloginreq.secret_hash));
			packet_append_string(bnpacket,account);
			queue_push_packet(conn_get_out_queue(bnetd_conn()),bnpacket);
		}
		packet_del_ref(bnpacket);
	}
	return 0;
}

static int on_client_createcharreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket, * bnpacket;
	char const	* charname;
	char const	* account;
	t_sq		* sq;
	unsigned int	reply;
	unsigned short	status, class;
	t_d2charinfo_file	data;

	if (!(charname=packet_get_str_const(packet,sizeof(t_client_d2cs_createcharreq),MAX_CHARNAME_LEN))) {
		log_error("got bad character name");
		return -1;
	}
	if (!(account=conn_get_account(c))) {
		log_error("missing account for character %s",charname);
		return -1;
	}
	class=bn_short_get(packet->u.client_d2cs_createcharreq.class);
	status=bn_short_get(packet->u.client_d2cs_createcharreq.status);
	if (d2char_create(account,charname,class,status)<0) {
		log_warn("error create character %s for account %s",charname,account);
		reply=D2CS_CLIENT_CREATECHARREPLY_ALREADY_EXIST;
	} else if (d2charinfo_load(account,charname,&data)<0) {
		log_error("error loading charinfo for character %s(*%s)",charname,account);
		reply=D2CS_CLIENT_CREATECHARREPLY_FAILED;
	} else {
		log_info("character %s(*%s) created",charname,account);
		reply=D2CS_CLIENT_CREATECHARREPLY_SUCCEED;
		conn_set_charinfo(c,&data.summary);
		if ((bnpacket=packet_create(packet_class_d2cs_bnetd))) {
			if ((sq=sq_create(conn_get_sessionnum(c),packet,0))) {
				packet_set_size(bnpacket,sizeof(t_d2cs_bnetd_charloginreq));
				packet_set_type(bnpacket,D2CS_BNETD_CHARLOGINREQ);
				bn_int_set(&bnpacket->u.d2cs_bnetd_charloginreq.h.seqno,sq_get_seqno(sq));
				bn_int_set(&bnpacket->u.d2cs_bnetd_charloginreq.sessionnum,\
					conn_get_bnetd_sessionnum(c));
				packet_append_string(bnpacket,charname);
				packet_append_string(bnpacket,(char const *)&data.portrait);
				queue_push_packet(conn_get_out_queue(bnetd_conn()),bnpacket);
			}
			packet_del_ref(bnpacket);
		}
		return 0;
	}
	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_createcharreply));
		packet_set_type(rpacket,D2CS_CLIENT_CREATECHARREPLY);
		bn_int_set(&rpacket->u.d2cs_client_createcharreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_client_creategamereq(t_connection * c, t_packet * packet)
{
	char const	* gamename;
	char const	* gamepass;
	char const	* gamedesc;
	t_game		* game;
	t_d2gs		* gs;
	t_gq		* gq;
	unsigned int	tempflag,gameflag;
	unsigned int	leveldiff, maxchar, difficulty, expansion, hardcore;
	unsigned int	seqno, reply;
	unsigned int	pos;

	pos=sizeof(t_client_creategamereq);
	if (!(gamename=packet_get_str_const(packet,pos,MAX_GAMENAME_LEN))) {
		log_error("got bad game name");
		return -1;
	}
	pos+=strlen(gamename)+1;
	if (!(gamepass=packet_get_str_const(packet,pos,MAX_GAMEPASS_LEN))) {
		log_error("got bad game pass");
		return -1;
	}
	pos+=strlen(gamepass)+1;
	if (!(gamedesc=packet_get_str_const(packet,pos,MAX_GAMEDESC_LEN))) {
		log_error("got bad game desc");
		return -1;
	}
	tempflag=bn_int_get(packet->u.client_d2cs_creategamereq.gameflag);
	leveldiff=bn_byte_get(packet->u.client_d2cs_creategamereq.leveldiff);
	maxchar=bn_byte_get(packet->u.client_d2cs_creategamereq.maxchar);
	difficulty=gameflag_get_difficulty(tempflag);
	if (difficulty > conn_get_charinfo_difficulty(c)) {
		log_error("game difficulty exceed character limit %d %d",difficulty,\
			conn_get_charinfo_difficulty(c));
		return 0;
	}
	expansion=conn_get_charinfo_expansion(c);
	hardcore=conn_get_charinfo_hardcore(c);
	gameflag=gameflag_create(expansion,hardcore,difficulty);

	gs = NULL;
	game = NULL;
	gq=conn_get_gamequeue(c);
	if (gamelist_find_game(gamename)) {
		log_info("game name %s is already exist in gamelist",gamename);
		reply=D2CS_CLIENT_CREATEGAMEREPLY_NAME_EXIST;
	} else if (!gq && gqlist_find_game(gamename)) {
		log_info("game name %s is already exist in game queue",gamename);
		reply=D2CS_CLIENT_CREATEGAMEREPLY_NAME_EXIST;
	} else if (!(gs=d2gslist_choose_server())) {
		if (gq) {
			log_error("client %d is already in game queue",conn_get_sessionnum(c));
			conn_set_gamequeue(c,NULL);
			gq_destroy(gq);
			return 0;
		} else if ((gq=gq_create(conn_get_sessionnum(c), packet, gamename))) {
			conn_set_gamequeue(c,gq);
			d2cs_send_client_creategamewait(c,gqlist_get_length());
			return 0;
		}
		reply=D2CS_CLIENT_CREATEGAMEREPLY_FAILED;
	} else if (hardcore && conn_get_charinfo_dead(c)) {
		reply=D2CS_CLIENT_CREATEGAMEREPLY_FAILED;
	} else if (!(game=game_create(gamename,gamepass,gamedesc,gameflag))) {
		reply=D2CS_CLIENT_CREATEGAMEREPLY_NAME_EXIST;
	} else {
		reply=D2CS_CLIENT_CREATEGAMEREPLY_SUCCEED;
		game_set_d2gs(game,gs);
		d2gs_add_gamenum(gs, 1);
		game_set_gameflag_expansion(game,expansion);
		game_set_created(game,0);
		game_set_leveldiff(game,leveldiff);
		game_set_charlevel(game,conn_get_charinfo_level(c));
		game_set_maxchar(game,maxchar);
		game_set_gameflag_difficulty(game,difficulty);
		game_set_gameflag_hardcore(game,hardcore);
	}

	seqno=bn_short_get(packet->u.client_d2cs_creategamereq.seqno);
	if (reply!=D2CS_CLIENT_CREATEGAMEREPLY_SUCCEED) {
		t_packet	* rpacket;

		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_creategamereply));
			packet_set_type(rpacket,D2CS_CLIENT_CREATEGAMEREPLY);
			bn_short_set(&rpacket->u.d2cs_client_creategamereply.seqno,seqno);
			bn_short_set(&rpacket->u.d2cs_client_creategamereply.u1,0);
			bn_short_set(&rpacket->u.d2cs_client_creategamereply.gameid,0);
			bn_int_set(&rpacket->u.d2cs_client_creategamereply.reply,reply);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		}
	} else {
		t_packet	* gspacket;
		t_sq		* sq;

		if ((gspacket=packet_create(packet_class_d2gs))) {
			if ((sq=sq_create(conn_get_sessionnum(c),packet,game_get_id(game)))) {
				packet_set_size(gspacket,sizeof(t_d2cs_d2gs_creategamereq));
				packet_set_type(gspacket,D2CS_D2GS_CREATEGAMEREQ);
				bn_int_set(&gspacket->u.d2cs_d2gs_creategamereq.h.seqno,sq_get_seqno(sq));
				bn_byte_set(&gspacket->u.d2cs_d2gs_creategamereq.difficulty,difficulty);
				bn_byte_set(&gspacket->u.d2cs_d2gs_creategamereq.hardcore,hardcore);
				bn_byte_set(&gspacket->u.d2cs_d2gs_creategamereq.expansion,expansion);
				packet_append_string(gspacket,gamename);
				queue_push_packet(conn_get_out_queue(d2gs_get_connection(gs)),gspacket);
			}
			packet_del_ref(gspacket);
			log_info("request create game %s on gs %d",gamename,d2gs_get_id(gs));
		}
	}
	return 0;
}

static int on_client_joingamereq(t_connection * c, t_packet * packet)
{
	char const	* gamename;
	char const	* gamepass;
	char const	* charname;
	char const	* account;
	t_game		* game;
	t_d2gs		* gs;
	int		reply;
	unsigned int	pos;
	unsigned int	seqno;

	gs = NULL;
	pos=sizeof(t_client_d2cs_joingamereq);
	if (!(gamename=packet_get_str_const(packet,pos,MAX_GAMENAME_LEN))) {
		log_error("got bad game name");
		return -1;
	}
	pos+=strlen(gamename)+1;
	if (!(gamepass=packet_get_str_const(packet,pos,MAX_GAMEPASS_LEN))) {
		log_error("got bad game pass");
		return -1;
	}
	if (!(charname=conn_get_charname(c))) {
		log_error("missing character name for connection");
		return -1;
	}
	if (!(account=conn_get_account(c))) {
		log_error("missing account for connection");
		return -1;
	}
	if (conn_check_multilogin(c,charname)<0) {
		log_error("character %s is already logged in",charname);
		return -1;
	}
	if (!(game=gamelist_find_game(gamename))) {
		log_info("game %s not found",gamename);
		reply=D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST;
	} else if (!(gs=game_get_d2gs(game))) {
		log_error("missing game server for game %s",gamename);
		reply=D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST;
	} else { 
		reply=d2cs_try_joingame(c,game,gamepass);
	}
	
	seqno=bn_short_get(packet->u.client_d2cs_joingamereq.seqno);
	if (reply!=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED) {
		t_packet	* rpacket;

		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_joingamereply));
			packet_set_type(rpacket,D2CS_CLIENT_JOINGAMEREPLY);
			bn_short_set(&rpacket->u.d2cs_client_joingamereply.seqno,seqno);
			bn_short_set(&rpacket->u.d2cs_client_joingamereply.u1,0);
			bn_short_set(&rpacket->u.d2cs_client_joingamereply.gameid,0);
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.addr,0);
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.token,0);
			bn_int_set(&rpacket->u.d2cs_client_joingamereply.reply,reply);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		}
	} else {
		t_packet	* gspacket;
		t_sq		* sq;
		
		if ((gspacket=packet_create(packet_class_d2gs))) {
			if ((sq=sq_create(conn_get_sessionnum(c),packet,game_get_id(game)))) {
				packet_set_size(gspacket,sizeof(t_d2cs_d2gs_joingamereq));
				packet_set_type(gspacket,D2CS_D2GS_JOINGAMEREQ);
				bn_int_set(&gspacket->u.d2cs_d2gs_joingamereq.h.seqno,sq_get_seqno(sq));
				bn_int_set(&gspacket->u.d2cs_d2gs_joingamereq.gameid,\
					game_get_d2gs_gameid(game));
				sq_set_gametoken(sq,d2gs_add_token(gs));
				bn_int_set(&gspacket->u.d2cs_d2gs_joingamereq.token,sq_get_gametoken(sq));
				packet_append_string(gspacket,charname);
				packet_append_string(gspacket,account);
				queue_push_packet(conn_get_out_queue(d2gs_get_connection(gs)),gspacket);
			}
			packet_del_ref(gspacket);
			log_info("request join game %s for character %s on gs %d",gamename,\
				charname,d2gs_get_id(gs));
		}
	}
	return 0;
}

static int on_client_gamelistreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket;
	t_game		* game;
	unsigned int	count;
	unsigned int	seqno;
	time_t		now;
	unsigned int	maxlifetime;
	t_elem const	* start_elem;
	t_elem const	* elem;

	seqno=bn_short_get(packet->u.client_d2cs_gamelistreq.seqno);
	/* if (seqno%2) return 0; */
	count=0;
	now=time(NULL);
	maxlifetime=prefs_get_game_maxlifetime();

	elem=start_elem=gamelist_get_curr_elem();
	if (!elem) elem=list_get_first_const(gamelist());
	else elem=elem_get_next_const(elem);

	for (; elem != start_elem; elem=elem_get_next_const(elem)) {
		if (!elem) {
			elem=list_get_first_const(gamelist());
			if (elem == start_elem) break;
		}
		if (!(game=elem_get_data(elem))) {
			log_error("got NULL game");
			break;
		}
		if (maxlifetime && (now-game->create_time>maxlifetime)) continue;
		if (!game_get_currchar(game)) continue;
		if (!prefs_allow_gamelist_showall()) {
			if (conn_get_charinfo_difficulty(c)!=game_get_gameflag_difficulty(game)) continue;
		}
		if (d2cs_try_joingame(c,game,"")!=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED) continue;
		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_gamelistreply));
			packet_set_type(rpacket,D2CS_CLIENT_GAMELISTREPLY);
			bn_short_set(&rpacket->u.d2cs_client_gamelistreply.seqno,seqno);
			bn_int_set(&rpacket->u.d2cs_client_gamelistreply.token,game_get_id(game));
			bn_byte_set(&rpacket->u.d2cs_client_gamelistreply.currchar,game_get_currchar(game));
			bn_int_set(&rpacket->u.d2cs_client_gamelistreply.gameflag,game_get_gameflag(game));
			packet_append_string(rpacket,game_get_name(game));
			packet_append_string(rpacket,game_get_desc(game));
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
			count++;
			if (prefs_get_maxgamelist() && count>=prefs_get_maxgamelist()) break;
		}
	}
	gamelist_set_curr_elem(elem);
	if (count) {
		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_gamelistreply));
			packet_set_type(rpacket,D2CS_CLIENT_GAMELISTREPLY);
			bn_short_set(&rpacket->u.d2cs_client_gamelistreply.seqno,seqno);
			bn_int_set(&rpacket->u.d2cs_client_gamelistreply.token,0);
			bn_byte_set(&rpacket->u.d2cs_client_gamelistreply.currchar,0);
			bn_int_set(&rpacket->u.d2cs_client_gamelistreply.gameflag,0);
			packet_append_string(rpacket,"");
			packet_append_string(rpacket,"");
			packet_append_string(rpacket,"");
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		}
	}
	return 0;
}

static int on_client_gameinforeq(t_connection * c, t_packet * packet)
{
	t_game_charinfo	* info;
	t_packet	* rpacket;
	char const	* gamename;
	t_game		* game;
	unsigned int	seqno, n;

	if (!(gamename=packet_get_str_const(packet,sizeof(t_client_d2cs_gameinforeq),MAX_GAMENAME_LEN))) {
		log_error("got bad game name");
		return -1;
	}
	if (!(game=gamelist_find_game(gamename))) {
		log_error("game %s not found",gamename);
		return 0;
	}
	seqno=bn_short_get(packet->u.client_d2cs_gameinforeq.seqno);
	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_gameinforeply));
		packet_set_type(rpacket,D2CS_CLIENT_GAMEINFOREPLY);
		bn_short_set(&rpacket->u.d2cs_client_gameinforeply.seqno,seqno);
		bn_int_set(&rpacket->u.d2cs_client_gameinforeply.gameflag,game_get_gameflag(game));
		bn_int_set(&rpacket->u.d2cs_client_gameinforeply.etime,time(NULL)-game_get_create_time(game));
		bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.charlevel,game_get_charlevel(game));
		bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.leveldiff,game_get_leveldiff(game));
		bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.maxchar,game_get_maxchar(game));
		bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.u1,0);

		n=0;
		BEGIN_LIST_TRAVERSE_DATA_CONST(game_get_charlist(game),info)
		{
			if (!info->charname) {
				log_error("got NULL charname in game %s char list",gamename);
				continue;
			}
			packet_append_string(rpacket,info->charname);
			bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.class[n],info->class);
			bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.level[n],info->level);
			n++;
		}
		END_LIST_TRAVERSE_DATA_CONST()

		bn_byte_set(&rpacket->u.d2cs_client_gameinforeply.currchar,n);
		if (n!=game_get_currchar(game)) {
			log_error("game %s character list corrupted",gamename);
		}
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_client_charloginreq(t_connection * c, t_packet * packet)
{
	t_packet		* bnpacket;
	char const		* charname;
	char const		* account;
	t_sq			* sq;
	t_d2charinfo_file	data;

	if (!(charname=packet_get_str_const(packet,sizeof(t_client_d2cs_charloginreq),MAX_CHARNAME_LEN))) {
		log_error("got bad character name");
		return -1;
	}
	if (!(account=conn_get_account(c))) {
		log_error("missing account for connection");
		return -1;
	}
	if (d2charinfo_load(account,charname,&data)<0) {
		log_error("error loading charinfo for character %s(*%s)",charname,account);
		return -1;
	} else if (!bnetd_conn()) {
		log_error("no bnetd connection available,character login rejected");
		return -1;
	}
	conn_set_charinfo(c,&data.summary);
	log_info("got character %s(*%s) login request",charname,account);
	if ((bnpacket=packet_create(packet_class_d2cs_bnetd))) {
		if ((sq=sq_create(conn_get_sessionnum(c),packet,0))) {
			packet_set_size(bnpacket,sizeof(t_d2cs_bnetd_charloginreq));
			packet_set_type(bnpacket,D2CS_BNETD_CHARLOGINREQ);
			bn_int_set(&bnpacket->u.d2cs_bnetd_charloginreq.h.seqno,sq_get_seqno(sq));
			bn_int_set(&bnpacket->u.d2cs_bnetd_charloginreq.sessionnum,\
				conn_get_bnetd_sessionnum(c));
			packet_append_string(bnpacket,charname);
			packet_append_string(bnpacket,(char const *)&data.portrait);
			queue_push_packet(conn_get_out_queue(bnetd_conn()),bnpacket);
		}
		packet_del_ref(bnpacket);
	}
	return 0;
}

static int on_client_deletecharreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket;
	char const	* charname;
	char const	* account;
	unsigned int	reply;

	if (!(charname=packet_get_str_const(packet,sizeof(t_client_d2cs_deletecharreq),MAX_CHARNAME_LEN))) {
		log_error("got bad character name");
		return -1;
	}
	if (conn_check_multilogin(c,charname)<0) {
		log_error("character %s is already logged in",charname);
		return -1;
	}
	conn_set_charname(c,NULL);
	account=conn_get_account(c);
	if (d2char_delete(account,charname)<0) {
		log_error("failed to delete character %s(*%s)",charname,account);
		reply = D2CS_CLIENT_DELETECHARREPLY_FAILED;
	} else {
		reply = D2CS_CLIENT_DELETECHARREPLY_SUCCEED;
	}
	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_deletecharreply));
		packet_set_type(rpacket,D2CS_CLIENT_DELETECHARREPLY);
		bn_short_set(&rpacket->u.d2cs_client_deletecharreply.u1,0);
		bn_int_set(&rpacket->u.d2cs_client_deletecharreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_client_ladderreq(t_connection * c, t_packet * packet)
{
	unsigned char		type;
	unsigned short		start_pos;

	type=bn_byte_get(packet->u.client_d2cs_ladderreq.type);
	start_pos=bn_short_get(packet->u.client_d2cs_ladderreq.start_pos);
	d2cs_send_client_ladder(c,type,start_pos);
	return 0;
}

static int d2cs_send_client_ladder(t_connection * c, unsigned char type, unsigned short from)
{
	t_packet			* rpacket;
	t_d2cs_client_ladderinfo const	* ladderinfo;
	unsigned int			curr_len, cont_len, total_len;
	t_d2cs_client_ladderheader	ladderheader;
	t_d2cs_client_ladderinfoheader	infoheader;
	unsigned int			start_pos, count, count_per_packet, npacket;
	unsigned int			i, n, curr_pos;

	start_pos=from;
	count=prefs_get_ladderlist_count();
	if (d2ladder_get_ladder(&start_pos,&count,type,&ladderinfo)<0) {
		log_error("error get ladder for type %d start_pos %d",type,from);
		return 0;
	}

	count_per_packet=14;
	npacket=count/count_per_packet;
	if (count % count_per_packet) npacket++;

	curr_len=0;
	cont_len=0;
	total_len = count * sizeof(*ladderinfo) + sizeof(ladderheader) + sizeof(infoheader) * npacket;
	total_len -= 4;
	bn_short_set(&ladderheader.start_pos,start_pos);
	bn_short_set(&ladderheader.u1,0);
	bn_int_set(&ladderheader.count1,count);

	for (i=0; i< npacket; i++) {
		curr_len=0;
		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_ladderreply));
			packet_set_type(rpacket,D2CS_CLIENT_LADDERREPLY);
			bn_byte_set(&rpacket->u.d2cs_client_ladderreply.type, type);
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.total_len, total_len);
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.cont_len,cont_len);
			if (i==0) {
				bn_int_set(&infoheader.count2,count);
				packet_append_data(rpacket,&ladderheader,sizeof(ladderheader));
				curr_len += sizeof(ladderheader);
			} else {
				bn_int_set(&infoheader.count2,0);
			}
			packet_append_data(rpacket,&infoheader,sizeof(infoheader));
			curr_len += sizeof(infoheader);
			for (n=0; n< count_per_packet; n++) {
				curr_pos = n + i * count_per_packet;
				if (curr_pos >= count) break;
				packet_append_data(rpacket, ladderinfo+curr_pos, sizeof(*ladderinfo));
				curr_len += sizeof(*ladderinfo);
			}
			if (i==0) {
				packet_set_size(rpacket, packet_get_size(rpacket)-4);
				curr_len -= 4;
			}
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.curr_len,curr_len);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		}
		cont_len += curr_len;
	}
	return 0;
}

static int on_client_motdreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket;

	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_motdreply));
		packet_set_type(rpacket,D2CS_CLIENT_MOTDREPLY);
		bn_byte_set(&rpacket->u.d2cs_client_motdreply.u1,0);
		packet_append_string(rpacket,prefs_get_motd());
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

static int on_client_cancelcreategame(t_connection * c, t_packet * packet)
{
	t_gq	* gq;

	if (!(gq=conn_get_gamequeue(c))) {
		return 0;
	}
	conn_set_gamequeue(c,NULL);
	gq_destroy(gq);
	return 0;
}

static int on_client_charladderreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket;
	char const	* charname;
	unsigned int	expansion, hardcore, type;
	int		pos;

	if (!(charname=packet_get_str_const(packet,sizeof(t_client_d2cs_charladderreq),MAX_CHARNAME_LEN))) {
		log_error("got bad character name");
		return -1;
	}
	expansion=bn_int_get(packet->u.client_d2cs_charladderreq.expansion);
	hardcore=bn_int_get(packet->u.client_d2cs_charladderreq.hardcore);
	type=0;					/* avoid warning */
	if (hardcore && expansion) {
		type=D2LADDER_EXP_HC_OVERALL;
	} else if (!hardcore && expansion) {
		type=D2LADDER_EXP_STD_OVERALL;
	} else if (hardcore && !expansion) {
		type=D2LADDER_HC_OVERALL;
	} else if (!hardcore && !expansion) {
		type=D2LADDER_STD_OVERALL;
	}
	if ((pos=d2ladder_find_character_pos(type,charname))<0) {
		if ((rpacket=packet_create(packet_class_d2cs))) {
			packet_set_size(rpacket,sizeof(t_d2cs_client_ladderreply));
			packet_set_type(rpacket,D2CS_CLIENT_LADDERREPLY);
			bn_byte_set(&rpacket->u.d2cs_client_ladderreply.type, type);
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.total_len,0);
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.curr_len,0);
			bn_short_set(&rpacket->u.d2cs_client_ladderreply.cont_len,0);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		}
		return 0;
	}
	pos -= prefs_get_ladderlist_count()/2;
	if (pos < 0) pos=0;
	d2cs_send_client_ladder(c,type,pos);
	return 0;
}

static int on_client_charlistreq(t_connection * c, t_packet * packet)
{
	t_packet		* rpacket;
	t_pdir			* dir;
	char const		* account; 
	char const		* charname; 
	char			* path;
	t_d2charinfo_file       charinfo;
	unsigned int		n, maxchar;

	if (!(account=conn_get_account(c))) {
		log_error("missing account for connection");
		return -1;
	}
	if (!(path=malloc(strlen(prefs_get_charinfo_dir())+1+strlen(account)+1))) {
		log_error("error allocate memory for path");
		return 0;
	}
	d2char_get_infodir_name(path,account);
	maxchar=prefs_get_maxchar();
	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_charlistreply));
		packet_set_type(rpacket,D2CS_CLIENT_CHARLISTREPLY);
		bn_short_set(&rpacket->u.d2cs_client_charlistreply.u1,0);
		if (prefs_allow_newchar()) {
			bn_short_set(&rpacket->u.d2cs_client_charlistreply.maxchar,maxchar);
		} else {
			bn_short_set(&rpacket->u.d2cs_client_charlistreply.maxchar,0);
		}
		n=0;
		if (!(dir=p_opendir(path))) {
			log_info("(*%s) charinfo directory do not exist, building it",account);
			mkdir(path,S_IRWXU);
		} else {
			while ((charname=p_readdir(dir))) {
				if (charname[0]=='.') continue;
				if (d2charinfo_load(account,charname,&charinfo)<0) {
					log_error("error loading charinfo for %s(*%s)",charname,account);
					continue;
				}
				packet_append_string(rpacket,charinfo.header.charname);
				packet_append_string(rpacket,(char *)&charinfo.portrait);
				n++;
				if (n>=maxchar) break;
			}
			p_closedir(dir);
		}
		bn_short_set(&rpacket->u.d2cs_client_charlistreply.currchar,n);
		bn_short_set(&rpacket->u.d2cs_client_charlistreply.currchar2,n);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	free(path);
	return 0;
}

static int on_client_convertcharreq(t_connection * c, t_packet * packet)
{
	t_packet	* rpacket;
	char const	* charname;
	char const	* account;
	unsigned int	reply;

	if (!(charname=packet_get_str_const(packet,sizeof(t_client_d2cs_convertcharreq),MAX_CHARNAME_LEN))) {
		log_error("got bad character name");
		return -1;
	}
	if (conn_check_multilogin(c,charname)<0) {
		log_error("character %s is already logged in",charname);
		return -1;
	}
	account=conn_get_account(c);
	if (d2char_convert(account,charname)<0) {
		log_error("failed to convert character %s(*%s)",charname,account);
		reply = D2CS_CLIENT_CONVERTCHARREPLY_FAILED;
	} else {
		reply = D2CS_CLIENT_CONVERTCHARREPLY_SUCCEED;
	}
	if ((rpacket=packet_create(packet_class_d2cs))) {
		packet_set_size(rpacket,sizeof(t_d2cs_client_convertcharreply));
		packet_set_type(rpacket,D2CS_CLIENT_CONVERTCHARREPLY);
		bn_int_set(&rpacket->u.d2cs_client_convertcharreply.reply,reply);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	}
	return 0;
}

extern int d2cs_send_client_creategamewait(t_connection * c, unsigned int position)
{
	t_packet	* packet;

	ASSERT(c,-1);
	if ((packet=packet_create(packet_class_d2cs))) {
		packet_set_size(packet,sizeof(t_d2cs_client_creategamewait));
		packet_set_type(packet,D2CS_CLIENT_CREATEGAMEWAIT);
		bn_int_set(&packet->u.d2cs_client_creategamewait.position,position);
		queue_push_packet(conn_get_out_queue(c),packet);
		packet_del_ref(packet);
	}
	return 0;
}

extern int d2cs_handle_client_creategame(t_connection * c, t_packet * packet)
{
	return on_client_creategamereq(c,packet);
}

static unsigned int d2cs_try_joingame(t_connection const * c, t_game const * game, char const * gamepass)
{
	unsigned int reply;

	ASSERT(c,D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST);
	ASSERT(game,D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST);
	if (!game_get_created(game)) {
		reply=D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST;
	} else if (!game_get_d2gs(game)) {
		reply=D2CS_CLIENT_JOINGAMEREPLY_NOT_EXIST;
	} else if (conn_get_charinfo_expansion(c) != game_get_gameflag_expansion(game)) {
		reply=D2CS_CLIENT_JOINGAMEREPLY_LEVEL_LIMIT;
	} else if (conn_get_charinfo_hardcore(c) != game_get_gameflag_hardcore(game)) {
		reply=D2CS_CLIENT_JOINGAMEREPLY_LEVEL_LIMIT;
	} else if (conn_get_charinfo_difficulty(c) < game_get_gameflag_difficulty(game))  {
		reply=D2CS_CLIENT_JOINGAMEREPLY_LEVEL_LIMIT;
	} else if (prefs_allow_gamelimit()) {
		if (game_get_maxchar(game) <= game_get_currchar(game)) {
			reply=D2CS_CLIENT_JOINGAMEREPLY_GAME_FULL;
		} else if (conn_get_charinfo_level(c) > game_get_maxlevel(game)) {
			reply=D2CS_CLIENT_JOINGAMEREPLY_LEVEL_LIMIT;
		} else if (conn_get_charinfo_level(c) < game_get_minlevel(game)) {
			reply=D2CS_CLIENT_JOINGAMEREPLY_LEVEL_LIMIT;
		} else if (strcmp(game_get_pass(game),gamepass)) {
			reply=D2CS_CLIENT_JOINGAMEREPLY_BAD_PASS;
		} else {
			reply=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED;
		}
	} else {
		if (game_get_currchar(game) >= MAX_CHAR_PER_GAME) {
			reply=D2CS_CLIENT_JOINGAMEREPLY_GAME_FULL;
		} else {
			reply=D2CS_CLIENT_JOINGAMEREPLY_SUCCEED;
		}
	}
	return reply;
}

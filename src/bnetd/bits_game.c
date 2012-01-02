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
#define GAME_INTERNAL_ACCESS
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
#endif
#include <errno.h>
#include "account.h"
#include "account_wrap.h"
#include "compat/strerror.h"
#include "compat/strcasecmp.h"
#include "common/eventlog.h"
#include "connection.h"
#include "common/queue.h"
#include "game.h"
#include "game_conv.h"
#include "game_rule.h"
#include "gametrans.h"
#include "common/list.h"
#include "common/packet.h"
#include "common/bn_type.h"
#include "prefs.h"
#include "bits.h"
#include "bits_game.h"
#include "bits_packet.h"
#include "bits_net.h"
#include "bits_query.h"
#include "common/bits_protocol.h"
#include "bits_login.h"
#include "common/setup_after.h"


extern int send_bits_gamelist_add(t_connection * c, t_game const * game)
{
	t_bits_game g;

	if (!game) {
		eventlog(eventlog_level_error,"send_bits_gamelist_add","got NULL game");
		return -1;
	}
	g.name = game->name;
	g.password = game->pass;
	g.info = game->info;
	g.id = game->id;
	g.type = game->type;
	g.status = game->status;
	g.owner = game->owner;
	memcpy(&g.clienttag,game->clienttag,4);
	return send_bits_gamelist_add_bits(c,&g);
}

extern int send_bits_gamelist_add_bits(t_connection * c, t_bits_game const * game)
{
	t_packet * p;

	if (!game) {
		eventlog(eventlog_level_error,"send_bits_gamelist_add_bits","got NULL game");
		return -1;
	}
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_gamelist_add_bits","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_gamelist_add));
	packet_set_type(p,BITS_GAMELIST_ADD);
	/***/
	bn_int_set(&p->u.bits_gamelist_add.id,game->id);
	bn_int_set(&p->u.bits_gamelist_add.type,game->type);
	bn_int_set(&p->u.bits_gamelist_add.status,game->status);
		/* convert the clienttag */
	memcpy(&p->u.bits_gamelist_add.clienttag,&game->clienttag,4);
		/* in case  game->owner  is unsigned int */
	bn_int_set(&p->u.bits_gamelist_add.owner,game->owner);
		/* append the strings */
	packet_append_string(p,game->name);
	packet_append_string(p,game->password);
	packet_append_string(p,game->info);		
	/***/
	if (c) {
		/* send the packet on the connection */
		bits_packet_generic(p,BITS_ADDR_PEER);
		send_bits_packet_on(p,c);
	} else {
		/* broadcast the packet */
		bits_packet_generic(p,BITS_ADDR_BCAST);
		handle_bits_packet(bits_loopback_connection,p); /* also add the game on this server */
		send_bits_packet(p);
	}
	packet_del_ref(p);
	return 0;
}

extern int send_bits_gamelist_del(t_connection * c, t_game const * game)
{
	t_packet * p;

	if (!game) {
		eventlog(eventlog_level_error,"send_bits_gamelist_del","got NULL game");
		return -1;
	}
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_gamelist_del","could not create packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_gamelist_del));
	packet_set_type(p,BITS_GAMELIST_DEL);
	/***/
	bn_int_set(&p->u.bits_gamelist_del.id,game_get_id(game));
	/***/
	if (c) {
		/* send the packet on the connection */
		bits_packet_generic(p,BITS_ADDR_PEER);
		send_bits_packet_on(p,c);
	} else {
		/* broadcast the packet */
		bits_packet_generic(p,BITS_ADDR_BCAST);
		handle_bits_packet(bits_loopback_connection,p); /* also remove the game from this server */
		send_bits_packet(p);
	}
	packet_del_ref(p);
	return 0;}


extern int bits_game_sendlist(t_connection * c)
{
	t_elem const * curr;
		
	if (!bits_uplink_connection) { /* bits master server */
		t_game       * game;
		
		LIST_TRAVERSE_CONST(gamelist(),curr) {
			game = elem_get_data(curr);
			send_bits_gamelist_add(c,game);
		}
	} else { /* bits client */
		t_bits_game       * game;
		
		LIST_TRAVERSE_CONST(bits_gamelist(),curr) {
			game = elem_get_data(curr);
			send_bits_gamelist_add_bits(c,game);
		}
	}
		
	return 0;
}

extern int bits_game_update_status(unsigned int id, t_game_status status)
{
    t_packet * p;
    bn_int data;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"bits_game_update_status","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_update));
    packet_set_type(p,BITS_GAME_UPDATE);

    bn_int_set(&p->u.bits_game_update.id,id);
    bn_byte_set(&p->u.bits_game_update.field,BITS_GAME_UPDATE_STATUS);
    bn_int_set(&data,status);
    packet_append_data(p,data,4);
    bits_packet_generic(p,((bits_master)?(BITS_ADDR_BCAST):(BITS_ADDR_MASTER)));
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int bits_game_update_owner(unsigned int id, unsigned int owner)
{
    t_packet * p;
    bn_int data;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"bits_game_update_owner","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_update));
    packet_set_type(p,BITS_GAME_UPDATE);

    bn_int_set(&p->u.bits_game_update.id,id);
    bn_byte_set(&p->u.bits_game_update.field,BITS_GAME_UPDATE_OWNER);
    bn_int_set(&data,owner);
    packet_append_data(p,data,4);
    bits_packet_generic(p,((bits_master)?(BITS_ADDR_BCAST):(BITS_ADDR_MASTER)));
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int bits_game_update_option(unsigned int id, int option)
{
    t_packet * p;
    bn_int data;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"bits_game_update_option","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_update));
    packet_set_type(p,BITS_GAME_UPDATE);

    bn_int_set(&p->u.bits_game_update.id,id);
    bn_byte_set(&p->u.bits_game_update.field,BITS_GAME_UPDATE_OPTION);
    bn_int_set(&data,option);
    packet_append_data(p,data,4);
    bits_packet_generic(p,((bits_master)?(BITS_ADDR_BCAST):(BITS_ADDR_MASTER)));
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int bits_game_handle_client_gamelistreq(t_connection * conn, t_game_type gtype, int maxgames, char const * gamename)
{	/* FIXME: maxgames is ignored for now ... */
	if (!gamename) {
		eventlog(eventlog_level_error,"bits_game_handle_client_gamelistreq","got NULL gamename");
		return -1;
	}
	if (gamename[0]=='\0') {
	/* all public */
		t_packet * p;
		int counter;
		t_elem const * curr;
		t_bits_loginlist_entry * lle;

		p = packet_create(packet_class_bnet);
		if (!p) {
			eventlog(eventlog_level_error,"bits_game_handle_client_gamelistreq","could not create packet");
			return -1;
		}
		packet_set_size(p,sizeof(t_server_gamelistreply));
		packet_set_type(p,SERVER_GAMELISTREPLY);
		counter = 0;

		LIST_TRAVERSE_CONST(bits_gamelist(),curr)
		{
		t_bits_game * game;
		char ct[5];
		t_server_gamelistreply_game glgame;

		game = elem_get_data(curr);

		/** FILTER **/
		if (prefs_get_hide_pass_games() && strcmp(game->password,"")!=0)
		{
		    continue;
		}
		if (prefs_get_hide_started_games() && game->status!=game_status_open)
		{
		    continue;
		}
		/* convert the clienttag */
		ct[4]='\0';
		memcpy(ct,&game->clienttag,4);
		if (strcmp(ct,conn_get_clienttag(conn))!=0)
		{
		    continue;
		}
		if (gtype!=game_type_all && game->type!=gtype)
		{
		    continue;
		}
		/** OWNER **/
		lle = bits_loginlist_bysessionid(game->owner);
		if (!lle) {
		    eventlog(eventlog_level_warn,"bits_game_handle_client_gamelistreq","cannot find owner id=0x%08x for game id=0x%08x",game->owner,game->id);
		    continue;
		}
		/** glgame **/
		bn_int_set(&glgame.unknown7,SERVER_GAMELISTREPLY_GAME_UNKNOWN7);
		bn_short_set(&glgame.gametype,gtype_to_bngtype(game->type));
		bn_short_set(&glgame.unknown1,SERVER_GAMELISTREPLY_GAME_UNKNOWN1);
		bn_short_set(&glgame.unknown3,SERVER_GAMELISTREPLY_GAME_UNKNOWN3);
		bn_short_nset(&glgame.port,lle->game_port);
		bn_int_nset(&glgame.game_ip,lle->game_addr);
		bn_int_set(&glgame.unknown4,SERVER_GAMELISTREPLY_GAME_UNKNOWN4);
		bn_int_set(&glgame.unknown5,SERVER_GAMELISTREPLY_GAME_UNKNOWN5);
		switch (game->status)
		{
		case game_status_started:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_STARTED);
		    break;
		case game_status_full:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_FULL);
		    break;
		case game_status_open:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_OPEN);
		    break;
		case game_status_done:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_DONE);
		    break;
		default:
		    eventlog(eventlog_level_warn,"bits_game_handle_client_gamelistreq","game \"%s\" has bad status=%d",game->name,game->status);
		    bn_int_set(&glgame.status,0);
		}
		bn_int_set(&glgame.unknown6,SERVER_GAMELISTREPLY_GAME_UNKNOWN6);
		/** CHECK **/
		if (packet_get_size(p)+
		    sizeof(glgame)+
		    strlen(game->name)+1+
		    strlen(game->password)+1+
		    strlen(game->info)+1>MAX_PACKET_SIZE)
		{
		    eventlog(eventlog_level_debug,"bits_game_handle_client_gamelistreq","[%d] out of room for games",conn_get_socket(conn));
		    break; /* no more room */
		}

		packet_append_data(p,&glgame,sizeof(glgame));
		packet_append_string(p,game->name);
		packet_append_string(p,game->password);
		packet_append_string(p,game->info);
		counter++;
		} /* end list */

		bn_int_set(&p->u.server_gamelistreply.gamecount,counter);
		eventlog(eventlog_level_debug,"bits_game_handle_client_gamelistreq","[%d] GAMELISTREPLY sent %u games",conn_get_socket(conn),counter);

		queue_push_packet(conn_get_out_queue(conn),p);
		packet_del_ref(p);
	} else 	{
	    t_bits_game * game;
	    t_server_gamelistreply_game glgame;
	    unsigned int                addr;
	    unsigned short              port;
	    t_packet 		      * rpacket;
	
	    rpacket = packet_create(packet_class_bnet);
	    if (!rpacket) {
		eventlog(eventlog_level_error,"bits_game_handle_client_gamelistreq","could not create packet");
		return -1;
	    }
	    packet_set_size(rpacket,sizeof(t_server_gamelistreply));
	    packet_set_type(rpacket,SERVER_GAMELISTREPLY);
	    eventlog(eventlog_level_debug,"bits_game_handle_client_gamelistreq","[%d] GAMELISTREPLY looking for specific game tag=\"%s\" gtype=%d name=\"%s\"",conn_get_socket(conn),conn_get_clienttag(conn),(int)gtype,gamename);
	    if ((game = bits_gamelist_find_game(gamename,gtype)))
	    {
		bn_int_set(&glgame.unknown7,SERVER_GAMELISTREPLY_GAME_UNKNOWN7);
		bn_short_set(&glgame.gametype,gtype_to_bngtype(bits_game_get_type(game)));
		bn_short_set(&glgame.unknown1,SERVER_GAMELISTREPLY_GAME_UNKNOWN1);
		bn_short_set(&glgame.unknown3,SERVER_GAMELISTREPLY_GAME_UNKNOWN3);
		addr = bits_game_get_addr(game);
		port = bits_game_get_port(game);
		gametrans_net(conn_get_addr(conn),conn_get_port(conn),conn_get_local_addr(conn),conn_get_local_port(conn),&addr,&port);
		bn_short_nset(&glgame.port,port);
		bn_int_nset(&glgame.game_ip,addr);
		bn_int_set(&glgame.unknown4,SERVER_GAMELISTREPLY_GAME_UNKNOWN4);
		bn_int_set(&glgame.unknown5,SERVER_GAMELISTREPLY_GAME_UNKNOWN5);
		switch (bits_game_get_status(game))
		{
		case game_status_started:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_STARTED);
		    break;
		case game_status_full:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_FULL);
		    break;
		case game_status_open:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_OPEN);
		    break;
		case game_status_done:
		    bn_int_set(&glgame.status,SERVER_GAMELISTREPLY_GAME_STATUS_DONE);
		    break;
		default:
		    eventlog(eventlog_level_warn,"bits_game_handle_client_gamelistreq","[%d] game \"%s\" has bad status %d",conn_get_socket(conn),bits_game_get_name(game),bits_game_get_status(game));
		    bn_int_set(&glgame.status,0);
		}
		bn_int_set(&glgame.unknown6,SERVER_GAMELISTREPLY_GAME_UNKNOWN6);
		    
		packet_append_data(rpacket,&glgame,sizeof(glgame));
		packet_append_string(rpacket,bits_game_get_name(game));
		packet_append_string(rpacket,bits_game_get_pass(game));
		packet_append_string(rpacket,bits_game_get_info(game));
		bn_int_set(&rpacket->u.server_gamelistreply.gamecount,1);
	    	eventlog(eventlog_level_debug,"bits_game_handle_client_gamelistreq","[%d] GAMELISTREPLY found it",conn_get_socket(conn));
	    }
	    else
	    {
		bn_int_set(&rpacket->u.server_gamelistreply.gamecount,0);
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] GAMELISTREPLY doesn't seem to exist",conn_get_socket(conn));
	    }
	    queue_push_packet(conn_get_out_queue(conn),rpacket);
	    packet_del_ref(rpacket);
	}

	return 0;
}

extern void bits_game_handle_startgame1(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status)
{	/* We assume all arguments are valid */
    t_bits_game * bg;

    bg = bits_gamelist_find_game_byid(conn_get_bits_game(c));
    if (bg)
    {
	    switch (status)
	    {
	    case CLIENT_STARTGAME1_STATUS_STARTED:
		bits_game_set_status(bg,game_status_started);
		break;
	    case CLIENT_STARTGAME1_STATUS_FULL:
		bits_game_set_status(bg,game_status_full);
		break;
	    case CLIENT_STARTGAME1_STATUS_OPEN:
		bits_game_set_status(bg,game_status_open);
		break;
	    case CLIENT_STARTGAME1_STATUS_DONE:
		bits_game_set_status(bg,game_status_done);
		eventlog(eventlog_level_info,"bits_game_handle_startgame1","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
		break;
	    }
    }
    else
        if (status!=CLIENT_STARTGAME1_STATUS_DONE)
        {
	    t_game_type gtype;
			
	    gtype = bngtype_to_gtype(conn_get_clienttag(c),bngtype);
	    if ((gtype==game_type_ladder && account_get_auth_createladdergame(conn_get_account(c))==0) || /* default to true */
	        (gtype!=game_type_ladder && account_get_auth_createnormalgame(conn_get_account(c))==0)) /* default to true */
	    {
	        char const * tname;
			    
	        eventlog(eventlog_level_info,"bits_game_handle_startgame1","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
	        conn_unget_username(c,tname);
	    }
	    else
	        conn_set_game_bits(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW1,game_option_none);
        }
        else
	    eventlog(eventlog_level_info,"bits_game_handle_startgame1","[%d] client tried to set game status to DONE",conn_get_socket(c));
}

extern void bits_game_handle_startgame3(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status)
{	/* We assume all arguments are valid */
    t_bits_game * bg;

    bg = bits_gamelist_find_game_byid(conn_get_bits_game(c));
    if (bg)
    {
	    switch (status)
	    {
	    case CLIENT_STARTGAME3_STATUS_STARTED:
		bits_game_set_status(bg,game_status_started);
		break;
	    case CLIENT_STARTGAME3_STATUS_FULL:
		bits_game_set_status(bg,game_status_full);
		break;
	    case CLIENT_STARTGAME3_STATUS_OPEN1:
	    case CLIENT_STARTGAME3_STATUS_OPEN:
		bits_game_set_status(bg,game_status_open);
		break;
	    case CLIENT_STARTGAME3_STATUS_DONE:
		bits_game_set_status(bg,game_status_done);
		eventlog(eventlog_level_info,"bits_game_handle_startgame3","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
		break;
	    }
    }
    else
	if (status!=CLIENT_STARTGAME3_STATUS_DONE)
	{
	    t_game_type gtype;
			
	    gtype = bngtype_to_gtype(conn_get_clienttag(c),bngtype);
	    if ((gtype==game_type_ladder && account_get_auth_createladdergame(conn_get_account(c))==0) ||
	        (gtype!=game_type_ladder && account_get_auth_createnormalgame(conn_get_account(c))==0))
	    {
		char const * tname;
			    
		eventlog(eventlog_level_info,"bits_game_handle_startgame3","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
		conn_unget_username(c,tname);
	    }
	    else
		conn_set_game_bits(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW3,game_option_none);
	}
	else
	    eventlog(eventlog_level_info,"bits_game_handle_startgame3","[%d] client tried to set game status to DONE",conn_get_socket(c));
}

extern void bits_game_handle_startgame4(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status, unsigned short option)
{	/* We assume all arguments are valid */
    t_bits_game * bg;

    bg = bits_gamelist_find_game_byid(conn_get_bits_game(c));
    if (bg)
    {
	switch (status)
	{
	case CLIENT_STARTGAME4_STATUS_STARTED:
	    bits_game_set_status(bg,game_status_started);
	    break;
	case CLIENT_STARTGAME4_STATUS_FULL1:
	case CLIENT_STARTGAME4_STATUS_FULL2:
	    bits_game_set_status(bg,game_status_full);
	    break;
	case CLIENT_STARTGAME4_STATUS_INIT:
	case CLIENT_STARTGAME4_STATUS_OPEN1:
	case CLIENT_STARTGAME4_STATUS_OPEN2:
	case CLIENT_STARTGAME4_STATUS_OPEN3:
	    bits_game_set_status(bg,game_status_open);
	    break;
	case CLIENT_STARTGAME4_STATUS_DONE1:
	case CLIENT_STARTGAME4_STATUS_DONE2:
	    bits_game_set_status(bg,game_status_done);
	    eventlog(eventlog_level_info,"bits_game_handle_startgame4","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
	    break;
	}
    }
    else
    	if (status!=CLIENT_STARTGAME4_STATUS_DONE1 &&
	    status!=CLIENT_STARTGAME4_STATUS_DONE2)
	{
	    t_game_type gtype;
			
	    gtype = bngtype_to_gtype(conn_get_clienttag(c),bngtype);
	    if ((gtype==game_type_ladder && account_get_auth_createladdergame(conn_get_account(c))==0) ||
	        (gtype!=game_type_ladder && account_get_auth_createnormalgame(conn_get_account(c))==0))
	    {
		char const * tname;
				
		eventlog(eventlog_level_info,"bits_game_handle_startgame4","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
		conn_unget_username(c,tname);
	    }
	    else
	    	conn_set_game_bits(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW4,bngoption_to_goption(conn_get_clienttag(c),gtype,option));
        }
        else
	    eventlog(eventlog_level_info,"bits_game_handle_startgame4","[%d] client tried to set game status to DONE",conn_get_socket(c));
}

extern int send_bits_game_join_request(unsigned int sessionid, unsigned int gameid, int version)
{
    t_packet *p;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_game_join_request","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_join_request));
    packet_set_type(p,BITS_GAME_JOIN_REQUEST);
    bn_int_set(&p->u.bits_game_join_request.id,gameid);
    bn_int_set(&p->u.bits_game_join_request.sessionid,sessionid);
    bn_int_set(&p->u.bits_game_join_request.version,version);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_game_leave(unsigned int sessionid, unsigned int gameid)
{
    t_packet *p;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_game_leave","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_join_request));
    packet_set_type(p,BITS_GAME_LEAVE);
    bn_int_set(&p->u.bits_game_leave.id,gameid);
    bn_int_set(&p->u.bits_game_leave.sessionid,sessionid);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_game_create_request(char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version, unsigned int sessionid, t_game_option option)
{
    t_packet *p;

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_game_create_request","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_create_request));
    packet_set_type(p,BITS_GAME_CREATE_REQUEST);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    bn_int_set(&p->u.bits_game_create_request.sessionid,sessionid);
    bn_int_set(&p->u.bits_game_create_request.type,type);
    bn_int_set(&p->u.bits_game_create_request.version,version);
    bn_int_set(&p->u.bits_game_create_request.option,option);
    packet_append_string(p,gamename);
    packet_append_string(p,gamepass);
    packet_append_string(p,gameinfo);
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern int send_bits_game_report(t_connection * c, t_packet const * packet)
{
    t_packet * p;

    if (!c) {
	eventlog(eventlog_level_error,"send_bits_game_report","got NULL connection");
	return -1;
    }
    if (!packet) {
	eventlog(eventlog_level_error,"send_bits_game_report","got NULL packet");
	return -1;
    }

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_game_report","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_game_report));
    packet_set_type(p,BITS_GAME_REPORT);
    bits_packet_generic(p,BITS_ADDR_MASTER);

    bn_int_set(&p->u.bits_game_report.sessionid,conn_get_sessionid(c));
    bn_int_set(&p->u.bits_game_report.gameid,conn_get_bits_game(c));
    bn_int_set(&p->u.bits_game_report.len,packet_get_size(packet));
    packet_append_data(p,packet_get_data_const(packet,0,packet_get_size(packet)),packet_get_size(packet));
    /* FIXME: maybe we should compress it to save network traffic */
    
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
}

extern t_game * bits_game_create_temp(unsigned int gameid)
{
    t_game * g;
    t_bits_game * bg;
    t_bits_loginlist_entry * lle;

    if (gameid == 0) {
	return NULL; /* a game id of 0 means no game */
    }
    bg = bits_gamelist_find_game_byid(gameid);
    if (!bg) {
	eventlog(eventlog_level_error,"bits_game_create_temp","could not find bits game with gameid=0x%08x",gameid);
	return NULL;
    }
    g = malloc(sizeof(t_game));
    if (!g) {
	eventlog(eventlog_level_error,"bits_game_create_temp","malloc failed: %s",strerror(errno));
	return NULL;
    }
    memset(g,0,sizeof(t_game)); /* set everything to zero */
    g->name = bg->name;
    g->pass = bg->password;
    g->info = bg->info;
    g->id = bg->id;
    g->type = bg->type;
    g->status = bg->status;
    g->clienttag = malloc(5); /* avoid warning */
    if (!g->clienttag) {
	eventlog(eventlog_level_error,"bits_game_create_temp","malloc for clienttag failed: %s",strerror(errno));
	return NULL;	
    }
    memset((void *)g->clienttag,0,5); /* avoid warning */
    memcpy((void *)g->clienttag,&bg->clienttag,4); /* avoid warning */
    g->owner = bg->owner;
    lle = bits_loginlist_bysessionid(g->owner);
    if (lle) {
    	g->addr = lle->game_addr;
    	g->port = lle->game_addr;
    } else {
	eventlog(eventlog_level_warn,"bits_game_create_temp","bits game has invalid owner (id=0x%08x)",g->owner);
    	g->addr = 0;
    	g->port = 0;
    }
    game_parse_info(g,bg->info); 
    return g;   
}

extern int bits_game_destroy_temp(t_game * game)
{
    if (!game) {
	eventlog(eventlog_level_error,"bits_game_destroy_temp","got NULL game");
	return -1;
    }
    if (game->clienttag)
        free((void *) game->clienttag); /* avoid warning */
    free(game);
    return 0;
}

extern t_game_type bits_game_get_type(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_type","got NULL game");
		return game_type_none;
	}
	return game->type;
}

extern t_game_status bits_game_get_status(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_status","got NULL game");
		return game_status_done;
	}
	return game->status;
}

extern int bits_game_set_status(t_bits_game * game, t_game_status status)
{
    if (!game) {
	eventlog(eventlog_level_error,"bits_game_set_status","got NULL game");
	return -1;
    }
    game->status = status;
    bits_game_update_status(bits_game_get_id(game),status);
    return 0;	
}

extern char const * bits_game_get_name(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_name","got NULL game");
		return NULL;
	}
	return game->name;
}

extern char const * bits_game_get_info(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_info","got NULL game");
		return NULL;
	}
	return game->info;
}

extern char const * bits_game_get_pass(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_pass","got NULL game");
		return NULL;
	}
	return game->password;
}

extern unsigned int bits_game_get_id(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_game_get_id","got NULL game");
		return 0;
	}
	return game->id;
}

extern unsigned int bits_game_get_owner(t_bits_game const * game)
{
    if (!game) {
	eventlog(eventlog_level_error,"bits_game_get_owner","got NULL game");
	return 0;
    }
    return game->owner;
}

extern unsigned int bits_game_get_addr(t_bits_game const * game)
{
    t_bits_loginlist_entry * lle;

    if (!game) {
	eventlog(eventlog_level_error,"bits_game_get_addr","got NULL game");
	return 0;
    }
    lle = bits_loginlist_bysessionid(bits_game_get_owner(game));
    if (!lle) {
	eventlog(eventlog_level_error,"bits_game_get_addr","can't find game owner in the loginlist");
	return 0;
    }
    return lle->game_addr;
}

extern unsigned short bits_game_get_port(t_bits_game const * game)
{
   t_bits_loginlist_entry * lle;

   if (!game) {
	eventlog(eventlog_level_error,"bits_game_get_port","got NULL game");
	return 0;
    }
    lle = bits_loginlist_bysessionid(bits_game_get_owner(game));
    if (!lle) {
	eventlog(eventlog_level_error,"bits_game_get_port","can't find game owner in the loginlist");
	return 0;
    }
    return lle->game_port;
}


/* --- list stuff --- */
/* All the gamelist stuff from game.c converted to use t_bits_game instead of games ... */

static t_list * bits_gamelist_head=NULL;

extern int bits_gamelist_create(void)
{
    if (!(bits_gamelist_head = list_create()))
	return -1;
    eventlog(eventlog_level_info,"bits_gamelist_create","bits_gamelist created.");
    return 0;
}

extern int bits_gamelist_destroy(void)
{
    if (bits_gamelist_head)
    {
	/* free active games */
    	t_elem const * curr;
    	t_bits_game *       game;
	LIST_TRAVERSE_CONST(bits_gamelist_head,curr)
	{
	    game = elem_get_data(curr);
	    if (game->name) free((void *)game->name); /* avoid warning */
	    if (game->info) free((void *)game->info); /* avoid warning */
	    if (game->password) free((void *)game->password); /* avoid warning */
	    /* free game */
	    free(game);
	    list_remove_data(bits_gamelist_head,game);
	}
	/* destroy the list */
	list_purge(bits_gamelist_head);
	list_destroy(bits_gamelist_head);
	bits_gamelist_head = NULL;
    }
    eventlog(eventlog_level_info,"bits_gamelist_destroy","bits_gamelist destroyed");
    return 0;
}

extern int bits_gamelist_get_length(void)
{
    return list_get_length(bits_gamelist_head);
}

extern int bits_gamelist_add(t_bits_game * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_gamelist_add","got NULL game");
		return -1;
	}
	return list_prepend_data(bits_gamelist(),game);
}

extern int bits_gamelist_del(t_bits_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"bits_gamelist_del","got NULL game");
		return -1;
	}
	return list_remove_data(bits_gamelist(),game);
}


extern t_bits_game * bits_gamelist_find_game(char const * name, t_game_type type)
{
    t_elem const * curr;
    t_bits_game *       game;
    
    if (bits_gamelist_head)
	LIST_TRAVERSE_CONST(bits_gamelist_head,curr)
	{
	    game = elem_get_data(curr);
	    if ((type==game_type_all || game->type==type) && strcasecmp(name,game->name)==0)
		return game;
	}
    
    return NULL;
}

extern t_bits_game * bits_gamelist_find_game_byid(unsigned int id)
{
    t_elem const * curr;
    t_bits_game *       game;
    
    if (bits_gamelist_head)
	LIST_TRAVERSE_CONST(bits_gamelist_head,curr)
	{
	    game = elem_get_data(curr);
	    if (game->id == id)
		return game;
	}
    
    return NULL;	
}


extern t_list * bits_gamelist(void)
{
	return bits_gamelist_head;
}

#endif

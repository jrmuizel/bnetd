/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strcasecmp.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "account.h"
#include "connection.h"
#include "common/queue.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/auth_protocol.h"
#include "character.h"
#include "common/util.h"
#include "handle_auth.h"
#include "common/setup_after.h"


static int add_charlistreq_packet (t_packet * rpacket, t_account * account, char const * clienttag, char const * realmname, char const * name);


extern int handle_auth_packet(t_connection * c, t_packet const * const packet)
{
    t_packet * rpacket;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"handle_auth_packet","[%d] got NULL connection",conn_get_socket(c));
	return -1;
    }
    if (!packet)
    {
	eventlog(eventlog_level_error,"handle_auth_packet","[%d] got NULL packet",conn_get_socket(c));
	return -1;
    }
    if (packet_get_class(packet)!=packet_class_auth)
    {
	eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad packet (class %d)",conn_get_socket(c),(int)packet_get_class(packet));
	return -1;
    }
    
    switch (conn_get_state(c))
    {
    case conn_state_connected:
	switch (packet_get_type(packet))
	{
	case CLIENT_AUTHLOGINREQ:
	    if (packet_get_size(packet)<sizeof(t_client_authloginreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad AUTHLOGINREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_authloginreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const   * username;
		t_connection * normalcon;
		unsigned int   reply;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_authloginreq),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad AUTHLOGINREQ packet (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_debug,"handle_auth_packet","[%d] username = \"%s\"",conn_get_socket(c),username);
		
		if (!(normalcon = connlist_find_connection_by_sessionkey(bn_int_get(packet->u.client_authloginreq.sessionkey))))
		{
		    eventlog(eventlog_level_info,"handle_auth_packet","[%d] auth login for \"%s\" refused (bad session key %u)",conn_get_socket(c),username,bn_int_get(packet->u.client_authloginreq.sessionkey));
		    reply = SERVER_AUTHLOGINREPLY_REPLY_BANNED;
		}
		else
		{
		    unsigned int salt;
		    struct
		    {
			bn_int   salt;
			bn_int   sessionkey;
			bn_int   sessionnum;
			bn_int   secret;
		    } temp;
		    t_hash       secret_hash;
		    t_hash       try_hash;
		    char const * tname;
		    
		    salt = bn_int_get(packet->u.client_authloginreq.unknown9);
		    bn_int_set(&temp.salt,salt);
		    bn_int_set(&temp.sessionkey,conn_get_sessionkey(normalcon));
		    bn_int_set(&temp.sessionnum,conn_get_sessionnum(normalcon));
		    bn_int_set(&temp.secret,conn_get_secret(normalcon));
		    bnet_hash(&secret_hash,sizeof(temp),&temp);
		    
		    bnhash_to_hash(packet->u.client_authloginreq.secret_hash,&try_hash);
		    
		    if (hash_eq(try_hash,secret_hash)!=1)
		    {
			eventlog(eventlog_level_info,"handle_auth_packet","[%d] auth login for \"%s\" refused (bad password)",conn_get_socket(c),username);
			reply = SERVER_AUTHLOGINREPLY_REPLY_BANNED;
		    }
		    else
		    {
			if (strcasecmp(username,(tname = conn_get_username(normalcon)))!=0)
			{
			    eventlog(eventlog_level_info,"handle_auth_packet","[%d] auth login for \"%s\" refused (username does not match \"%s\")",conn_get_socket(c),username,tname);
			    conn_unget_username(normalcon,tname);
			    reply = SERVER_AUTHLOGINREPLY_REPLY_BANNED;
			}
			else
			{
			    conn_unget_username(normalcon,tname);
			    eventlog(eventlog_level_info,"handle_auth_packet","[%d] auth login for \"%s\" accepted (correct password)",conn_get_socket(c),username);
			    reply = SERVER_AUTHLOGINREPLY_REPLY_SUCCESS;
			    conn_bind(c,normalcon);
			    conn_set_state(c,conn_state_loggedin); /* FIXME: what about the other connection? */
			}
		    }
		}
		
		if ((rpacket = packet_create(packet_class_auth)))
		{
		    packet_set_size(rpacket,sizeof(t_server_authloginreply));
		    packet_set_type(rpacket,SERVER_AUTHLOGINREPLY);
		    bn_int_set(&rpacket->u.server_authloginreply.reply,reply);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	default:
	    eventlog(eventlog_level_error,"handle_auth_packet","[%d] unknown (unlogged in) auth packet type 0x%02hx, len %u",conn_get_socket(c),packet_get_type(packet),packet_get_size(packet));
	}
	break;
	
    case conn_state_loggedin:
	switch (packet_get_type(packet))
	{
	case CLIENT_CREATECHARREQ:
	    if (packet_get_size(packet)<sizeof(t_client_createcharreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CREATECHARREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_createcharreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *          charname;
		unsigned int          reply;
		t_character_class     charclass;
		t_character_expansion charexpansion;
		
		if (!(charname = packet_get_str_const(packet,sizeof(t_client_createcharreq),128)))
		{
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CREATECHARREQ packet (missing or too long charname)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_debug,"handle_auth_packet","[%d] CREATECHARREQ %u %04x %04x",
			 conn_get_socket(c),
			 bn_short_get(packet->u.client_createcharreq.class),
			 bn_short_get(packet->u.client_createcharreq.unknown1),
			 bn_short_get(packet->u.client_createcharreq.expansion));

		switch (bn_short_get(packet->u.client_createcharreq.class))
		{
		case CLIENT_CREATECHARREQ_CLASS_AMAZON:
		    charclass = character_class_amazon;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_SORCERESS:
		    charclass = character_class_sorceress;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_NECROMANCER:
		    charclass = character_class_necromancer;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_PALADIN:
		    charclass = character_class_paladin;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_BARBARIAN:
		    charclass = character_class_barbarian;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_DRUID:
		    charclass = character_class_druid;
		    break;
		case CLIENT_CREATECHARREQ_CLASS_ASSASSIN:
		    charclass = character_class_assassin;
		    break;
		default:
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got unknown character class %u",conn_get_socket(c),bn_short_get(packet->u.client_createcharreq.class));
		    charclass = character_class_none;
		}

		switch (bn_short_get(packet->u.client_createcharreq.expansion))
		{
		case CLIENT_CREATECHARREQ_EXPANSION_CLASSIC:
		    charexpansion = character_expansion_classic;
		    break;
		case CLIENT_CREATECHARREQ_EXPANSION_LOD:
		    charexpansion = character_expansion_lod;
		    break;
		default:
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got unknown character expansion %04x",conn_get_socket(c),bn_short_get(packet->u.client_createcharreq.expansion));
		    charexpansion = character_expansion_none;
		}

		if (character_create(conn_get_account(c),
				     conn_get_clienttag(c),
				     conn_get_realmname(c),
				     charname,
				     charclass,
				     charexpansion)<0)
		{
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] could not create character \"%s\"",conn_get_socket(c),charname);
		    reply = SERVER_CREATECHARREPLY_REPLY_REJECTED;
		}
		else
		{
		    conn_set_character(c,characterlist_find_character(conn_get_realmname(c),charname));
		    reply = SERVER_CREATECHARREPLY_REPLY_SUCCESS;
		}
		
		if ((rpacket = packet_create(packet_class_auth)))
		{
		    packet_set_size(rpacket,sizeof(t_server_createcharreply));
		    packet_set_type(rpacket,SERVER_CREATECHARREPLY);
		    bn_int_set(&rpacket->u.server_createcharreply.reply,reply);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_CREATEGAMEREQ:
	    if (packet_get_size(packet)<sizeof(t_client_creategamereq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CREATEGAMEREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_creategamereq),packet_get_size(packet));
		break;
	    }
	    
	    /* FIXME: actually create game */
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_creategamereply));
		packet_set_type(rpacket,SERVER_CREATEGAMEREPLY);
		bn_short_set(&rpacket->u.server_creategamereply.unknown1,bn_short_get(packet->u.client_creategamereq.unknown1));
		bn_short_set(&rpacket->u.server_creategamereply.unknown2,SERVER_CREATEGAMEREPLY_UNKNOWN2);
		bn_short_set(&rpacket->u.server_creategamereply.unknown3,SERVER_CREATEGAMEREPLY_UNKNOWN3);
		bn_int_set(&rpacket->u.server_creategamereply.reply,SERVER_CREATEGAMEREPLY_REPLY_OK);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_JOINGAMEREQ2:
	    if (packet_get_size(packet)<sizeof(t_client_joingamereq2))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad JOINGAMEREQ2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_joingamereq2),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_joingamereply2));
		packet_set_type(rpacket,SERVER_JOINGAMEREPLY2);
		bn_short_set(&rpacket->u.server_joingamereply2.unknown1,bn_short_get(packet->u.client_joingamereq2.unknown1));
		bn_int_set(&rpacket->u.server_joingamereply2.unknown2,SERVER_JOINGAMEREPLY2_UNKNOWN2);
#ifdef SENDSERVER
		bn_int_nset(&rpacket->u.server_joingamereply2.addr,conn_get_real_local_addr(c)); /* FIXME: game server may be on another machine */
#else
		bn_int_nset(&rpacket->u.server_joingamereply2.addr,conn_get_game_addr(c));
#endif
		bn_int_set(&rpacket->u.server_joingamereply2.unknown4,SERVER_JOINGAMEREPLY2_UNKNOWN4);
		bn_int_set(&rpacket->u.server_joingamereply2.reply,SERVER_JOINGAMEREPLY2_REPLY_OK);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_D2GAMELISTREQ:
	    if (packet_get_size(packet)<sizeof(t_client_d2gamelistreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad D2GAMELISTREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_d2gamelistreq),packet_get_size(packet));
		break;
	    }
	    
	    /* FIXME: actually get game list */
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_d2gamelistreply));
		packet_set_type(rpacket,SERVER_D2GAMELISTREPLY);
		bn_short_set(&rpacket->u.server_d2gamelistreply.unknown1,bn_short_get(packet->u.client_d2gamelistreq.unknown1));
		bn_int_set(&rpacket->u.server_d2gamelistreply.unknown2,SERVER_D2GAMELISTREPLY_UNKNOWN2);
		bn_byte_set(&rpacket->u.server_d2gamelistreply.unknown3,SERVER_D2GAMELISTREPLY_UNKNOWN3);
		bn_int_set(&rpacket->u.server_d2gamelistreply.unknown4,SERVER_D2GAMELISTREPLY_UNKNOWN4);
		packet_append_string(rpacket,"Nope"); /* game name */
		packet_append_string(rpacket,""); /* game pass? */
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_d2gamelistreply));
		packet_set_type(rpacket,SERVER_D2GAMELISTREPLY);
		bn_short_set(&rpacket->u.server_d2gamelistreply.unknown1,bn_short_get(packet->u.client_d2gamelistreq.unknown1));
		bn_int_set(&rpacket->u.server_d2gamelistreply.unknown2,SERVER_D2GAMELISTREPLY_UNKNOWN2);
		bn_byte_set(&rpacket->u.server_d2gamelistreply.unknown3,SERVER_D2GAMELISTREPLY_UNKNOWN3);
		bn_int_set(&rpacket->u.server_d2gamelistreply.unknown4,SERVER_D2GAMELISTREPLY_UNKNOWN4_END);
		packet_append_string(rpacket,"");
		packet_append_string(rpacket,"");
		packet_append_string(rpacket,"");
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_GAMEINFOREQ:
	    if (packet_get_size(packet)<sizeof(t_client_gameinforeq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad GAMEINFOREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_gameinforeq),packet_get_size(packet));
		break;
	    }
	    
	    /* FIXME: send real game info */
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_gameinforeply));
		packet_set_type(rpacket,SERVER_GAMEINFOREPLY);
		bn_short_set(&rpacket->u.server_gameinforeply.unknown1,bn_short_get(packet->u.client_gameinforeq.unknown1));
		bn_int_set(&rpacket->u.server_gameinforeply.unknown2,SERVER_GAMEINFOREPLY_UNKNOWN2);
		bn_int_set(&rpacket->u.server_gameinforeply.unknown3,SERVER_GAMEINFOREPLY_UNKNOWN3);
		bn_byte_set(&rpacket->u.server_gameinforeply.gamelevel,0x01);
		bn_byte_set(&rpacket->u.server_gameinforeply.leveldiff,0xff);
		bn_byte_set(&rpacket->u.server_gameinforeply.maxchar,0x08);
		bn_byte_set(&rpacket->u.server_gameinforeply.currchar,0x01);
                memset(rpacket->u.server_gameinforeply.class,0xcc,16);
                memset(rpacket->u.server_gameinforeply.level,0xcc,16);
		bn_byte_set(&rpacket->u.server_gameinforeply.unknown4,SERVER_GAMEINFOREPLY_UNKNOWN4);
		packet_append_string(rpacket,"Player1");
		packet_append_string(rpacket,"Player2");
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_CHARLOGINREQ:
	    if (packet_get_size(packet)<sizeof(t_client_charloginreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CHARLOGINREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_charloginreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *  charname;
		t_character * ch;
		char const *  charlist;
		
		if (!(charname = packet_get_str_const(packet,sizeof(t_client_charloginreq),128)))
		{
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CHARLOGINREQ packet (missing or too long charname)",conn_get_socket(c));
		    break;
		}
	        
	        if ((rpacket = packet_create(packet_class_auth)))
	        {
		    packet_set_size(rpacket,sizeof(t_server_charloginreply));
		    packet_set_type(rpacket,SERVER_CHARLOGINREPLY);
		    if ((ch = characterlist_find_character(conn_get_realmname(c),charname)))
		    {
			if (!(charlist = account_get_closed_characterlist(conn_get_account(c),conn_get_clienttag(c),conn_get_realmname(c))))
			{
			    eventlog(eventlog_level_info,"handle_auth_packet","[%d] character login for \"%s\" refused (account has no charlist)",conn_get_socket(c),charname);
			    bn_int_set(&rpacket->u.server_charloginreply.reply,0xffffffff); /* FIXME */
			}
			else
			{
			    if (character_verify_charlist(ch,charlist)<0)
			    {
				eventlog(eventlog_level_info,"handle_auth_packet","[%d] character login for \"%s\" refused (character not in account charlist)",conn_get_socket(c),charname);
				bn_int_set(&rpacket->u.server_charloginreply.reply,0xffffffff); /* FIXME */
			    }
			    else
			    {
				conn_set_character(c,ch);
				bn_int_set(&rpacket->u.server_charloginreply.reply,SERVER_CHARLOGINREPLY_REPLY_SUCCESS);
			    }
			    account_unget_closed_characterlist(conn_get_account(c),charlist);
			}
		    }
		    else
		    {
			eventlog(eventlog_level_info,"handle_auth_packet","[%d] character login for \"%s\" refused (no such character in realm \"%s\")",conn_get_socket(c),charname,conn_get_realmname(c));
			bn_int_set(&rpacket->u.server_charloginreply.reply,0xffffffff); /* FIXME */
		    }
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
	        }
	    }
	    break;
	    
	case CLIENT_DELETECHARREQ:
	    if (packet_get_size(packet)<sizeof(t_client_deletecharreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad DELETECHARREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_deletecharreq),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_deletecharreply));
		packet_set_type(rpacket,SERVER_DELETECHARREPLY);
		bn_short_set(&rpacket->u.server_deletecharreply.unknown1,bn_short_get(packet->u.client_deletecharreq.unknown1));
		bn_int_set(&rpacket->u.server_deletecharreply.reply,SERVER_DELETECHARREPLY_REPLY_SUCCESS);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_LADDERREQ2:
	    if (packet_get_size(packet)<sizeof(t_client_ladderreq2))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad LADDERREQ2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_ladderreq2),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_ladderreply2));
		packet_set_type(rpacket,SERVER_LADDERREPLY2);
		bn_byte_set(&rpacket->u.server_ladderreply2.unknown1,bn_byte_get(packet->u.client_ladderreq2.class));
		bn_short_set(&rpacket->u.server_ladderreply2.unknown2,SERVER_LADDERREPLY2_UNKNOWN2);
		bn_short_set(&rpacket->u.server_ladderreply2.unknown3,SERVER_LADDERREPLY2_UNKNOWN3);
		bn_short_set(&rpacket->u.server_ladderreply2.unknown4,SERVER_LADDERREPLY2_UNKNOWN4);
		bn_int_set(&rpacket->u.server_ladderreply2.unknown5,SERVER_LADDERREPLY2_UNKNOWN5);
		bn_int_set(&rpacket->u.server_ladderreply2.unknown6,SERVER_LADDERREPLY2_UNKNOWN6);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_AUTHMOTDREQ:
	    if (packet_get_size(packet)<sizeof(t_client_authmotdreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad AUTHMOTDREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_authmotdreq),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_auth)))
	    {
		packet_set_size(rpacket,sizeof(t_server_authmotdreply));
		packet_set_type(rpacket,SERVER_AUTHMOTDREPLY);
		bn_byte_set(&rpacket->u.server_authmotdreply.unknown1,SERVER_AUTHMOTDREPLY_UNKNOWN1);
		packet_append_string(rpacket,"Hello.");
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_CHARLISTREQ:
	    if (packet_get_size(packet)<sizeof(t_client_charlistreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CHARLISTREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_charlistreq),packet_get_size(packet));
		break;
	    }

            eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREQ.unknown1 = 0x%04x", bn_short_get(packet->u.client_charlistreq.unknown1));
            eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREQ.unknown2 = 0x%04x", bn_short_get(packet->u.client_charlistreq.unknown2));

	    if ((rpacket = packet_create(packet_class_auth)))
	    {
  	        /* FIXME: Set *real* Number of characters here */
	        int number_of_characters = 0;

		packet_set_size(rpacket,sizeof(t_server_charlistreply));
		packet_set_type(rpacket,SERVER_CHARLISTREPLY);

		bn_short_set(&rpacket->u.server_charlistreply.unknown1, bn_short_get(packet->u.client_charlistreq.unknown1));
		bn_short_set(&rpacket->u.server_charlistreply.unknown2, 0);

		{
		    char const * realmname = conn_get_realmname(c);
		    char const * charlist = account_get_closed_characterlist (conn_get_account(c), conn_get_clienttag(c), realmname);
		    char         tempname[32];

		    if (charlist == NULL)
		    {
		        eventlog(eventlog_level_debug,"handle_auth_packet","no characters in Realm %s",realmname);
		    }
		    else
		    {
		        char const * start;
			char const * next_char;
			int    list_len;
			int    name_len;
			int    i;

			eventlog(eventlog_level_debug,"handle_auth_packet","got characterlist \"%s\" for Realm %s",charlist,realmname);

			list_len  = strlen(charlist);
			start     = charlist;
			next_char = start;
			for (i = 0; i < list_len; i++, next_char++)
			{
			    if (',' == *next_char)
			    {
			        name_len = next_char - start;

				strncpy(tempname, start, name_len);
				tempname[name_len] = '\0';

				number_of_characters += add_charlistreq_packet(rpacket, conn_get_account(c), conn_get_clienttag(c), realmname, tempname);

				start = next_char + 1;
			    }
			}

			name_len = next_char - start;

			strncpy(tempname, start, name_len);
			tempname[name_len] = '\0';

			number_of_characters += add_charlistreq_packet(rpacket, conn_get_account(c), conn_get_clienttag(c), realmname, tempname);

			start = next_char + 1;
		    }    
		}

		bn_short_set(&rpacket->u.server_charlistreply.nchars_1, number_of_characters);
		bn_short_set(&rpacket->u.server_charlistreply.nchars_2, number_of_characters);

		eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREPLY.unknown1 = 0x%04x", bn_short_get(rpacket->u.server_charlistreply.unknown1));
		eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREPLY.nchars_1 = 0x%04x", bn_short_get(rpacket->u.server_charlistreply.nchars_1));
		eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREPLY.unknown2 = 0x%04x", bn_short_get(rpacket->u.server_charlistreply.unknown2));
		eventlog(eventlog_level_debug,"handle_auth_packet","CHARLISTREPLY.nchars_2 = 0x%04x", bn_short_get(rpacket->u.server_charlistreply.nchars_2));

		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;

	case CLIENT_CONVERTCHARREQ:
	    if (packet_get_size(packet)<sizeof(t_client_convertcharreq))
	    {
		eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CONVERTCHARREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_convertcharreq),packet_get_size(packet));
		break;
	    }

	    {
		char const * charname;
		if (!(charname = packet_get_str_const(packet,sizeof(t_client_convertcharreq),128)))
		{
		    eventlog(eventlog_level_error,"handle_auth_packet","[%d] got bad CONVERTCHARREQ packet (missing or too long charname)",conn_get_socket(c));
		    break;
		}

		eventlog(eventlog_level_debug,"handle_auth_packet","[%d] CONVERTCHARREQ %s, FIXME: Add code later",
			 conn_get_socket(c),
                         charname);
		
		if ((rpacket = packet_create(packet_class_auth)))
		{
		    packet_set_size(rpacket,sizeof(t_server_convertcharreply));
		    packet_set_type(rpacket,SERVER_CONVERTCHARREPLY);
                    bn_int_set(&rpacket->u.server_convertcharreply.unknown1, SERVER_CONVERTCHARREPLY_UNKNOWN1);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
            }
            break;
            
	default:
	    eventlog(eventlog_level_error,"handle_auth_packet","[%d] unknown (logged in) auth packet type 0x%02hx, len %u",conn_get_socket(c),packet_get_type(packet),packet_get_size(packet));
	}
	break;
	
    default:
	eventlog(eventlog_level_error,"handle_auth_packet","[%d] unknown auth connection state %d",conn_get_socket(c),(int)conn_get_state(c));
    }
    
    return 0;
}


static int add_charlistreq_packet (t_packet * rpacket, t_account * account, char const * clienttag, char const * realmname, char const * name)
{
    char const * chardata_as_hex;
    char         chardata_key[256]; /* FIXME: buffer overflow on realmnname or name */

    eventlog(eventlog_level_debug, "add_charlistreq_packet", "about to add charlist data for \"%s\" in realm \"%s\"", name, realmname);

    sprintf (chardata_key, "BNET\\Characters\\%s\\%s\\%s\\0", clienttag, realmname, name);
    chardata_as_hex = account_get_strattr(account, chardata_key);

    eventlog(eventlog_level_debug, "add_charlistreq_packet", "key \"%s\"", chardata_key);
    eventlog(eventlog_level_debug, "add_charlistreq_packet", "value \"%s\"", chardata_as_hex);

	    /* FIXME: This seems to be a request for character lists, try the following canned request
	     *        that I tcpdump(1)'ed from a real Battle Net connection

# 569 packet from server: type=0x0017(SERVER_CHARLISTREPLY) length=199 class=auth
0000:   C7 00 17 08 00 04 00 00   00 04 00 68 61 6B 61 6E    ...........hakan
0010:   41 73 73 54 65 6D 70 00   84 80 FF FF FF FF FF FF    AssTemp.........
0020:   FF FF FF FF FF 07 FF FF   FF FF FF FF FF FF FF FF    ................
0030:   FF 01 A1 80 80 80 FF FF   FF 00 68 61 6B 61 6E 44    ..........hakanD
0040:   72 75 54 65 6D 70 00 84   80 FF FF FF FF FF FF FF    ruTemp..........
0050:   FF FF FF FF 06 FF FF FF   FF FF FF FF FF FF FF FF    ................
0060:   01 A1 80 80 80 FF FF FF   00 68 61 6B 61 6E 50 61    .........hakanPa
0070:   6C 4F 6C 5F 64 00 84 80   FF FF FF FF FF FF FF FF    lOl_d...........
0080:   FF FF FF 04 FF FF FF FF   FF FF FF FF FF FF FF 01    ................
0090:   81 80 80 80 FF FF FF 00   68 61 6B 61 6E 50 61 6C    ........hakanPal
00A0:   54 65 6D 70 00 84 80 FF   FF FF FF FF FF FF FF FF    Temp............
00B0:   FF FF 04 FF FF FF FF FF   FF FF FF FF FF FF 01 A1    ................
00C0:   80 80 80 FF FF FF 00                                 .......         

# 20 packet from server: type=0x0017(SERVER_CHARLISTREPLY) length=164 class=auth
0000:   A4 00 17 08 00 00 00 00   00 00 00 61 73 61 00 84    ...........asa..
0010:   80 FF FF FF FF FF FF FF   FF FF FF FF 07 FF FF FF    ................
0020:   FF FF FF FF FF FF FF FF   01 A1 80 80 80 FF FF FF    ................
0030:   00 70 61 6C 00 84 80 FF   FF FF FF FF FF FF FF FF    .pal............
0040:   FF FF 04 FF FF FF FF FF   FF FF FF FF FF FF 01 A1    ................
0050:   80 80 80 FF FF FF 00 73   6F 72 63 00 84 80 FF FF    .......sorc.....
0060:   FF FF FF FF FF FF FF FF   FF 02 FF FF FF FF FF FF    ................
0070:   FF FF FF FF FF 01 A1 80   80 80 FF FF FF 00 61 6D    ..............am
0080:   61 00 84 80 FF FF FF FF   FF FF FF FF FF FF FF 01    a...............
0090:   FF FF FF FF FF FF FF FF   FF FF FF 01 A1 80 80 80    ................
00A0:   FF FF FF 00                                          ....            


	     */

    if (chardata_as_hex != NULL)
    {
        int  append_length;
        char chardata[64];
	int  length;
	
        length = hex_to_str(chardata_as_hex, chardata, 33); /* FIXME: make sure chardata_as_hex is long enough (33*3 (+1?) ) */
	/* Append a '\0' as terminator to character data */
	chardata[length] = '\0';
	length++;

        append_length = packet_append_string (rpacket, name);
	eventlog(eventlog_level_debug, "add_charlistreq_packet", "length %d for name \"%s\"", append_length, name);

	append_length = packet_append_string (rpacket, chardata);
	eventlog(eventlog_level_debug, "add_charlistreq_packet", "length %d [%d] for character data", append_length, length); /* length should be == strlen(chardata) */

	return 1;
    }

    return 0;
}

/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999,2000  Rob Crittenden (rcrit@greyoak.com)
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

#define VERSIONCHECK_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS /* FIXME: remove ? */
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
#ifdef WITH_BITS
# include "compat/memcpy.h"
#endif
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include "compat/strchr.h"
#include "compat/strdup.h"
#include "common/packet.h"
#include "common/bnet_protocol.h"
#include "common/tag.h"
#include "message.h"
#include "common/eventlog.h"
#include "command.h"
#include "account.h"
#include "connection.h"
#include "channel.h"
#include "game.h"
#include "common/queue.h"
#include "tick.h"
#include "file.h"
#include "prefs.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "ladder.h"
#include "adbanner.h"
#include "common/list.h"
#include "common/bnettime.h"
#include "common/addr.h"
#ifdef WITH_BITS
# include "query.h"
# include "bits.h"
# include "bits_login.h"
# include "bits_va.h"
# include "bits_query.h"
# include "bits_packet.h"
# include "bits_game.h"
#endif
#include "game_conv.h"
#include "gametrans.h"
#include "autoupdate.h"
#include "realm.h"
#include "character.h"
#include "versioncheck.h"
#include "common/proginfo.h"
#include "handle_bnet.h"
#include "common/setup_after.h"


extern int handle_bnet_packet(t_connection * c, t_packet const * const packet)
{
    t_packet * rpacket=NULL;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got NULL connection",conn_get_socket(c));
	return -1;
    }
    if (!packet)
    {
	eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got NULL packet",conn_get_socket(c));
	return -1;
    }
    if (packet_get_class(packet)!=packet_class_bnet)
    {
	eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad packet (class %d)",conn_get_socket(c),(int)packet_get_class(packet));
	return -1;
    }
    
    switch (conn_get_state(c))
    {
    case conn_state_connected:
	switch (packet_get_type(packet))
	{
	case CLIENT_UNKNOWN_1B:
	    if (packet_get_size(packet)<sizeof(t_client_unknown_1b))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad UNKNOWN_1B packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_unknown_1b),packet_get_size(packet));
		break;
	    }
	    
	    {
		unsigned int   newip;
		unsigned short newport;
		
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] UNKNOWN_1B unknown1=0x%04hx",conn_get_socket(c),bn_short_get(packet->u.client_unknown_1b.unknown1));
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] UNKNOWN_1B unknown2=0x%08x",conn_get_socket(c),bn_int_get(packet->u.client_unknown_1b.unknown2));
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] UNKNOWN_1B unknown3=0x%08x",conn_get_socket(c),bn_int_get(packet->u.client_unknown_1b.unknown3));
		
		newip = bn_int_nget(packet->u.client_unknown_1b.ip);
		newport = bn_short_nget(packet->u.client_unknown_1b.port);
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] UNKNOWN_1B set new UDP address to %s",conn_get_socket(c),addr_num_to_addr_str(newip,newport));
		conn_set_game_addr(c,newip);
		conn_set_game_port(c,newport);
	    }
	    break;
	    
	case CLIENT_COMPINFO1:
	    if (packet_get_size(packet)<sizeof(t_client_compinfo1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_compinfo1),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * host;
		char const * user;
		
		if (!(host = packet_get_str_const(packet,sizeof(t_client_compinfo1),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO1 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}
		if (!(user = packet_get_str_const(packet,sizeof(t_client_compinfo1)+strlen(host)+1,128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO1 packet (missing or too long user)",conn_get_socket(c));
		    break;
		}
		
		conn_set_host(c,host);
		conn_set_user(c,user);
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_compreply));
		packet_set_type(rpacket,SERVER_COMPREPLY);
		bn_int_set(&rpacket->u.server_compreply.reg_version,SERVER_COMPREPLY_REG_VERSION);
		bn_int_set(&rpacket->u.server_compreply.reg_auth,SERVER_COMPREPLY_REG_AUTH);
		bn_int_set(&rpacket->u.server_compreply.client_id,SERVER_COMPREPLY_CLIENT_ID);
		bn_int_set(&rpacket->u.server_compreply.client_token,SERVER_COMPREPLY_CLIENT_TOKEN);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_sessionkey1));
		packet_set_type(rpacket,SERVER_SESSIONKEY1);
		bn_int_set(&rpacket->u.server_sessionkey1.sessionkey,conn_get_sessionkey(c));
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    
	    break;
	    
	case CLIENT_COMPINFO2:
	    if (packet_get_size(packet)<sizeof(t_client_compinfo2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_compinfo2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * host;
		char const * user;
		
		if (!(host = packet_get_str_const(packet,sizeof(t_client_compinfo2),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO2 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}
		if (!(user = packet_get_str_const(packet,sizeof(t_client_compinfo2)+strlen(host)+1,128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COMPINFO2 packet (missing or too long user)",conn_get_socket(c));
		    break;
		}
		
		conn_set_host(c,host);
		conn_set_user(c,user);
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_compreply));
		packet_set_type(rpacket,SERVER_COMPREPLY);
		bn_int_set(&rpacket->u.server_compreply.reg_version,SERVER_COMPREPLY_REG_VERSION);
		bn_int_set(&rpacket->u.server_compreply.reg_auth,SERVER_COMPREPLY_REG_AUTH);
		bn_int_set(&rpacket->u.server_compreply.client_id,SERVER_COMPREPLY_CLIENT_ID);
		bn_int_set(&rpacket->u.server_compreply.client_token,SERVER_COMPREPLY_CLIENT_TOKEN);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_sessionkey2));
		packet_set_type(rpacket,SERVER_SESSIONKEY2);
		bn_int_set(&rpacket->u.server_sessionkey2.sessionnum,conn_get_sessionnum(c));
		bn_int_set(&rpacket->u.server_sessionkey2.sessionkey,conn_get_sessionkey(c));
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    
	    break;
	    
	case CLIENT_COUNTRYINFO1:
	    if (packet_get_size(packet)<sizeof(t_client_countryinfo1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_countryinfo1),packet_get_size(packet));
		break;
	    }
	    {
		char const * langstr;
		char const * countrycode;
		char const * country;
		char const * countryname;
		
		if (!(langstr = packet_get_str_const(packet,sizeof(t_client_countryinfo1),16)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO1 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}

		if (!(countrycode = packet_get_str_const(packet,sizeof(t_client_countryinfo1) + strlen(langstr) + 1 ,16)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO1 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}

		if (!(country = packet_get_str_const(packet,sizeof(t_client_countryinfo1) + strlen(langstr) + 1 + strlen(countrycode) + 1,16)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO1 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}

		if (!(countryname = packet_get_str_const(packet,sizeof(t_client_countryinfo1) + strlen(langstr) + 1 + strlen(countrycode) + 1 + strlen(country) + 1,128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO1 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] COUNTRYINFO1 packet from %s %s %s",conn_get_socket(c),langstr, countrycode, country);
		conn_set_country(c, country);
		conn_set_tzbias(c, uint32_to_int(bn_int_get(packet->u.client_countryinfo1.bias)));
	    }
	    break;
	    
	case CLIENT_COUNTRYINFO_109:
	    if (packet_get_size(packet)<sizeof(t_client_countryinfo_109))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO_109 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_countryinfo_109),packet_get_size(packet));
		break;
	    }
            
	    {
		char const * langstr;
		char const * countryname;

		if (!(langstr = packet_get_str_const(packet,sizeof(t_client_countryinfo_109),16)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO_109 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}
		
		if (!(countryname = packet_get_str_const(packet,sizeof(t_client_countryinfo_109)+strlen(langstr) + 1,128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad COUNTRYINFO_109 packet (missing or too long host)",conn_get_socket(c));
		    break;
		}
		
                eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] COUNTRYINFO_109 packet bias=%u lcid %u langid %u arch %04x client %04x versionid %04x", conn_get_socket(c),
			 bn_int_get(packet->u.client_countryinfo_109.bias),
			 bn_int_get(packet->u.client_countryinfo_109.lcid),
			 bn_int_get(packet->u.client_countryinfo_109.langid),
			 bn_int_get(packet->u.client_countryinfo_109.archtag),
			 bn_int_get(packet->u.client_countryinfo_109.clienttag),
			 bn_int_get(packet->u.client_countryinfo_109.versionid));
		
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] COUNTRYINFO_109 packet from \"%s\" \"%s\"", conn_get_socket(c),countryname,langstr);
		
		conn_set_country(c,langstr); /* FIXME: This isn't right.  We want USA not ENU (English-US) */
		conn_set_tzbias(c, uint32_to_int(bn_int_get(packet->u.client_countryinfo1.bias)));
		
		conn_set_versionid(c,bn_int_get(packet->u.client_countryinfo_109.versionid));
		if (bn_int_tag_eq(packet->u.client_countryinfo_109.archtag,ARCHTAG_X86)==0)
		    conn_set_archtag(c,ARCHTAG_X86);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.archtag,ARCHTAG_PPC)==0)
		    conn_set_archtag(c,ARCHTAG_PPC);
		else
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown client arch 0x%08x, don't expect this to work",conn_get_socket(c),bn_int_get(packet->u.client_countryinfo_109.archtag));
		
		if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_STARCRAFT)==0)
		    conn_set_clienttag(c,CLIENTTAG_STARCRAFT);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_BROODWARS)==0)
		    conn_set_clienttag(c,CLIENTTAG_BROODWARS);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_SHAREWARE)==0)
		    conn_set_clienttag(c,CLIENTTAG_SHAREWARE);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_DIABLORTL)==0)
		    conn_set_clienttag(c,CLIENTTAG_DIABLORTL);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_WARCIIBNE)==0)
		    conn_set_clienttag(c,CLIENTTAG_WARCIIBNE);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_DIABLO2DV)==0)
		    conn_set_clienttag(c,CLIENTTAG_DIABLO2DV);
		else if (bn_int_tag_eq(packet->u.client_countryinfo_109.clienttag,CLIENTTAG_DIABLO2XP)==0)
		    conn_set_clienttag(c,CLIENTTAG_DIABLO2XP);
		else
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown client program type 0x%08x, don't expect this to work",conn_get_socket(c),bn_int_get(packet->u.client_countryinfo_109.clienttag));
		
                /* First, send an ECHO_REQ */
                if ((rpacket = packet_create(packet_class_bnet)))
                {
                    packet_set_size(rpacket,sizeof(t_server_echoreq));
                    packet_set_type(rpacket,SERVER_ECHOREQ);
                    bn_int_set(&rpacket->u.server_echoreq.ticks,get_ticks());
                    queue_push_packet(conn_get_out_queue(c),rpacket);
                    packet_del_ref(rpacket);
                }
		
                if ((rpacket = packet_create(packet_class_bnet)))
                {
                    t_versioncheck * vc;

                    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] selecting version check",conn_get_socket(c));
                    vc = versioncheck_create(conn_get_archtag(c),conn_get_clienttag(c));
		    conn_set_versioncheck(c,vc);
                    packet_set_size(rpacket,sizeof(t_server_authreq_109));
                    packet_set_type(rpacket,SERVER_AUTHREQ_109);
                    bn_int_set(&rpacket->u.server_authreq_109.unknown1,SERVER_AUTHREQ_109_UNKNOWN1);
		    bn_int_set(&rpacket->u.server_authreq_109.sessionkey,conn_get_sessionkey(c));
		    bn_int_set(&rpacket->u.server_authreq_109.sessionnum,conn_get_sessionnum(c));
                    file_to_mod_time(versioncheck_get_mpqfile(vc),&rpacket->u.server_authreq_109.timestamp);
                    packet_append_string(rpacket,versioncheck_get_mpqfile(vc));
                    packet_append_string(rpacket,versioncheck_get_eqn(vc));
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] selected \"%s\" \"%s\"",conn_get_socket(c),versioncheck_get_mpqfile(vc),versioncheck_get_eqn(vc));                   
                    queue_push_packet(conn_get_out_queue(c),rpacket);
                    packet_del_ref(rpacket);
                }
	    }
	    break;
	    
	case CLIENT_UNKNOWN_2B: /* FIXME: what is this? */
	    if (packet_get_size(packet)<sizeof(t_client_unknown_2b))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad UNKNOWN_2B packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_unknown_2b),packet_get_size(packet));
		break;
	    }
	    break;
	    
	case CLIENT_PROGIDENT:
	    if (packet_get_size(packet)<sizeof(t_client_progident))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PROGIDENT packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_progident),packet_get_size(packet));
		break;
	    }
	    
	    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_PROGIDENT archtag=0x%08x clienttag=0x%08x versionid=0x%08x unknown1=0x%08x",
		     conn_get_socket(c),
		     bn_int_get(packet->u.client_progident.archtag),
		     bn_int_get(packet->u.client_progident.clienttag),
		     bn_int_get(packet->u.client_progident.versionid),
		     bn_int_get(packet->u.client_progident.unknown1));
	    
	    if (bn_int_tag_eq(packet->u.client_progident.archtag,ARCHTAG_X86)==0)
		conn_set_archtag(c,ARCHTAG_X86);
	    else if (bn_int_tag_eq(packet->u.client_progident.archtag,ARCHTAG_PPC)==0)
		conn_set_archtag(c,ARCHTAG_PPC);
	    else
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown client arch 0x%08x, don't expect this to work",conn_get_socket(c),bn_int_get(packet->u.client_progident.archtag));
	    
	    if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_STARCRAFT)==0)
		conn_set_clienttag(c,CLIENTTAG_STARCRAFT);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_BROODWARS)==0)
		conn_set_clienttag(c,CLIENTTAG_BROODWARS);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_SHAREWARE)==0)
		conn_set_clienttag(c,CLIENTTAG_SHAREWARE);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLORTL)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLORTL);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLOSHR)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLOSHR);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_WARCIIBNE)==0)
		conn_set_clienttag(c,CLIENTTAG_WARCIIBNE);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2DV)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLO2DV);
	    else if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2XP)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLO2XP);
	    else
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown client program type 0x%08x, don't expect this to work",conn_get_socket(c),bn_int_get(packet->u.client_progident.clienttag));
            
            if (prefs_get_skip_versioncheck())
	    {
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] attempting to skip version check by sending early authreply",conn_get_socket(c));
		/* skip over SERVER_AUTHREQ1 and CLIENT_AUTHREQ1 */
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_authreply1));
		    packet_set_type(rpacket,SERVER_AUTHREPLY1);
		    if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2XP)==0)
			bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_D2XP_MESSAGE_OK);
		    else
			bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_MESSAGE_OK);
		    packet_append_string(rpacket,"");
		    packet_append_string(rpacket,""); /* FIXME: what's the second string for? */
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    else
	    {
                t_versioncheck * vc;
                
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] selecting version check",conn_get_socket(c));
		vc = versioncheck_create(conn_get_archtag(c),conn_get_clienttag(c));
		conn_set_versioncheck(c,vc);
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_authreq1));
		    packet_set_type(rpacket,SERVER_AUTHREQ1);
		    file_to_mod_time(versioncheck_get_mpqfile(vc),&rpacket->u.server_authreq1.timestamp);
		    packet_append_string(rpacket,versioncheck_get_mpqfile(vc));
		    packet_append_string(rpacket,versioncheck_get_eqn(vc));
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] selected \"%s\" \"%s\"",conn_get_socket(c),versioncheck_get_mpqfile(vc),versioncheck_get_eqn(vc));
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_CLOSEGAME:
	    /* silly client always sends this when exiting "just in case" */
	    break;
	    
	case CLIENT_CREATEACCTREQ1:
	    if (packet_get_size(packet)<sizeof(t_client_createacctreq1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CREATEACCTREQ1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_createacctreq1),packet_get_size(packet));
		break;
	    }
	    
#ifdef WITH_BITS
	    if (!bits_master) { /* BITS client */
		const char * username;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_createacctreq1),128))) {
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CREATEACCTREQ1 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		send_bits_va_create_req(c,username,packet->u.client_createacctreq1.password_hash1);
	    } else /* standalone or BITS master */
#endif
	    {
		char const * username;
		t_account *  temp;
		t_hash       newpasshash1;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_createacctreq1),128))) /* longer than USER_NAME_MAX so we send back an error */
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CREATEACCTREQ1 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] new account requested for \"%s\"",conn_get_socket(c),username);
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_createacctreply1));
		packet_set_type(rpacket,SERVER_CREATEACCTREPLY1);
		
		if (prefs_get_allow_new_accounts()==0)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not created (disabled)",conn_get_socket(c));
		    bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_NO);
		}
		else
		{
		    bnhash_to_hash(packet->u.client_createacctreq1.password_hash1,&newpasshash1);
		    if (!(temp = account_create(username,hash_get_str(newpasshash1))))
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not created (failed)",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_NO);
		    }
		    else if (!accountlist_add_account(temp))
		    {
			account_destroy(temp);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not inserted",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_NO);
		    }
		    else
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account created",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply1.result,SERVER_CREATEACCTREPLY1_RESULT_OK);
		    }
		}
		bn_int_set(&rpacket->u.server_createacctreply1.unknown1,SERVER_CREATEACCTREPLY1_UNKNOWN1);
		bn_int_set(&rpacket->u.server_createacctreply1.unknown2,SERVER_CREATEACCTREPLY1_UNKNOWN2);
		bn_int_set(&rpacket->u.server_createacctreply1.unknown3,SERVER_CREATEACCTREPLY1_UNKNOWN3);
		bn_int_set(&rpacket->u.server_createacctreply1.unknown4,SERVER_CREATEACCTREPLY1_UNKNOWN4);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_CREATEACCTREQ2:
	    if (packet_get_size(packet)<sizeof(t_client_createacctreq2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CLIENT_CREATEACCTREQ2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_createacctreq2),packet_get_size(packet));
		break;
	    }
	    
#ifdef WITH_BITS
	    if (!bits_master) { /* BITS client */
		const char * username;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_createacctreq2),128))) {
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CLIENT_CREATEACCTREQ2 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		send_bits_va_create_req(c,username,packet->u.client_createacctreq2.password_hash1);
	    } else /* standalone or BITS master */
#endif
	    {
		char const * username;
		t_account *  temp;
		t_hash       newpasshash1;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_createacctreq2),128))) /* longer than USER_NAME_MAX so we send back an error */
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CREATEACCTREQ2 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] new account requested for \"%s\"",conn_get_socket(c),username);
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_createacctreply2));
		packet_set_type(rpacket,SERVER_CREATEACCTREPLY2);
		
		if (prefs_get_allow_new_accounts()==0)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not created (disabled)",conn_get_socket(c));
		    bn_int_set(&rpacket->u.server_createacctreply2.result,SERVER_CREATEACCTREPLY2_RESULT_SHORT);
		}
		else
		{
		    bnhash_to_hash(packet->u.client_createacctreq2.password_hash1,&newpasshash1);
		    if (!(temp = account_create(username,hash_get_str(newpasshash1))))
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not created (failed)",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply2.result,SERVER_CREATEACCTREPLY2_RESULT_SHORT); /* FIXME: return reason for failure */
		    }
		    else if (!accountlist_add_account(temp))
		    {
			account_destroy(temp);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account not inserted",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply2.result,SERVER_CREATEACCTREPLY2_RESULT_SHORT);
		    }
		    else
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] account created",conn_get_socket(c));
			bn_int_set(&rpacket->u.server_createacctreply2.result,SERVER_CREATEACCTREPLY2_RESULT_OK);
		    }
		}
		bn_int_set(&rpacket->u.server_createacctreply2.unknown1,SERVER_CREATEACCTREPLY2_UNKNOWN1);
		bn_int_set(&rpacket->u.server_createacctreply2.unknown2,SERVER_CREATEACCTREPLY2_UNKNOWN2);
		bn_int_set(&rpacket->u.server_createacctreply2.unknown3,SERVER_CREATEACCTREPLY2_UNKNOWN3);
		bn_int_set(&rpacket->u.server_createacctreply2.unknown4,SERVER_CREATEACCTREPLY2_UNKNOWN4);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_CHANGEPASSREQ:
	    if (packet_get_size(packet)<sizeof(t_client_changepassreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CHANGEPASSREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_changepassreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * username;
		t_account *  account;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_changepassreq),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CHANGEPASSREQ (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change requested for \"%s\"",conn_get_socket(c),username);
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_changepassack));
		packet_set_type(rpacket,SERVER_CHANGEPASSACK);
		
		/* fail if logged in or no account */
		if (connlist_find_connection(username) || !(account = accountlist_find_account(username)))
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" refused (no such account)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_FAIL);
		}
		else if (account_get_auth_changepass(account)==0) /* default to true */
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" refused (no change access)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_FAIL);
		}
		else if (conn_get_sessionkey(c)!=bn_int_get(packet->u.client_changepassreq.sessionkey))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] password change for \"%s\" refused (expected session key 0x%08x, got 0x%08x)",conn_get_socket(c),username,conn_get_sessionkey(c),bn_int_get(packet->u.client_changepassreq.sessionkey));
		    bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_FAIL);
		}   
		else
		{
		    struct
		    {
			bn_int   ticks;
			bn_int   sessionkey;
			bn_int   passhash1[5];
		    }            temp;
		    char const * oldstrhash1;
		    t_hash       oldpasshash1;
		    t_hash       oldpasshash2;
		    t_hash       trypasshash2;
		    t_hash       newpasshash1;
		    char const * tname;
		    
		    if ((oldstrhash1 = account_get_pass(account)))
		    {
			bn_int_set(&temp.ticks,bn_int_get(packet->u.client_changepassreq.ticks));
			bn_int_set(&temp.sessionkey,bn_int_get(packet->u.client_changepassreq.sessionkey));
			if (hash_set_str(&oldpasshash1,oldstrhash1)<0)
			{
			    account_unget_pass(oldstrhash1);
			    bnhash_to_hash(packet->u.client_changepassreq.newpassword_hash1,&newpasshash1);
			    account_set_pass(account,hash_get_str(newpasshash1));
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" successful (bad previous password)",conn_get_socket(c),(tname = account_get_name(account)));
			    account_unget_name(tname);
			    bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_SUCCESS);
			}
			else
			{
			    account_unget_pass(oldstrhash1);
			    hash_to_bnhash((t_hash const *)&oldpasshash1,temp.passhash1); /* avoid warning */
			    bnet_hash(&oldpasshash2,sizeof(temp),&temp); /* do the double hash */
			    bnhash_to_hash(packet->u.client_changepassreq.oldpassword_hash2,&trypasshash2);
			    
			    if (hash_eq(trypasshash2,oldpasshash2)==1)
			    {
				bnhash_to_hash(packet->u.client_changepassreq.newpassword_hash1,&newpasshash1);
				account_set_pass(account,hash_get_str(newpasshash1));
				eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" successful (previous password)",conn_get_socket(c),(tname = account_get_name(account)));
				account_unget_name(tname);
				bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_SUCCESS);
			    }
			    else
			    {
				eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" refused (wrong password)",conn_get_socket(c),(tname = account_get_name(account)));
				account_unget_name(tname);
				bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_FAIL);
			    }
			}
		    }
		    else
		    {
			bnhash_to_hash(packet->u.client_changepassreq.newpassword_hash1,&newpasshash1);
			account_set_pass(account,hash_get_str(newpasshash1));
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] password change for \"%s\" successful (no previous password)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			bn_int_set(&rpacket->u.server_changepassack.message,SERVER_CHANGEPASSACK_MESSAGE_SUCCESS);
		    }
		}
		
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_ECHOREPLY:
	    if (packet_get_size(packet)<sizeof(t_client_echoreply))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ECHOREPLY packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_echoreply),packet_get_size(packet));
		break;
	    }
	    
	    {
		unsigned int now;
		unsigned int then;
		
		now = get_ticks();
		then = bn_int_get(packet->u.client_echoreply.ticks);
		if (!now || !then || now<then)
		    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] bad timing in echo reply: now=%u then=%u",conn_get_socket(c),now,then);
		else
		    conn_set_latency(c,now-then);
	    }
	    break;
	    
	case CLIENT_AUTHREQ1:
	    if (packet_get_size(packet)<sizeof(t_client_authreq1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad AUTHREQ1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_authreq1),packet_get_size(packet));
		break;
	    }
	    
	    {
		char         verstr[16];
		char const * exeinfo;
		char const * versiontag;
		unsigned int versionid;
		unsigned int gameversion;
		unsigned int checksum;
		int          failed;
		
		failed = 0;
		if (bn_int_tag_eq(packet->u.client_authreq1.archtag,conn_get_archtag(c))<0)
		    failed = 1;
		if (bn_int_tag_eq(packet->u.client_authreq1.clienttag,conn_get_clienttag(c))<0)
		    failed = 1;

		

		if (!(exeinfo = packet_get_str_const(packet,sizeof(t_client_authreq1),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad AUTHREQ1 (missing or too long exeinfo)",conn_get_socket(c));
		    exeinfo = "badexe";
		    failed = 1;
		}
		versionid = bn_int_get(packet->u.client_authreq1.versionid);
		checksum = bn_int_get(packet->u.client_authreq1.checksum);
		gameversion = bn_int_get(packet->u.client_authreq1.gameversion);
		strcpy(verstr,vernum_to_verstr(gameversion));

		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_AUTHREQ1 archtag=0x%08x clienttag=0x%08x verstr=%s exeinfo=\"%s\" versionid=0x%08x gameversion=0x%08x checksum=0x%08x",
		         conn_get_socket(c),
		         bn_int_get(packet->u.client_authreq1.archtag),
		         bn_int_get(packet->u.client_authreq1.clienttag),
		         verstr,
		         exeinfo,
		         versionid,
		         gameversion,
		         checksum);
		
		conn_set_clientver(c,verstr);
		conn_set_clientexe(c,exeinfo);
		
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_authreply1));
		    packet_set_type(rpacket,SERVER_AUTHREPLY1);
		    
		    switch (versioncheck_validate(conn_get_versioncheck(c),
                                                  conn_get_archtag(c),
                                                  conn_get_clienttag(c),
                                                  exeinfo,
                                                  versionid,
                                                  gameversion,
                                                  checksum,
						  &versiontag))
		    {
		    case -1: /* failed test... client has been modified */
			if (!prefs_get_allow_bad_version())
			{
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client failed test (closing connection)",conn_get_socket(c));
			    failed = 1;
			}
			else
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client failed test, allowing anyway",conn_get_socket(c));
			break;
		    case 0: /* not listed in table... can't tell if client has been modified */
			if (!prefs_get_allow_unknown_version())
			{
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] unable to test client (closing connection)",conn_get_socket(c));
			    failed = 1;
			}
			else
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] unable to test client, allowing anyway",conn_get_socket(c));
			break;
		    /* 1 == test passed... client seems to be ok */
		    }
		    
		    if (failed)
		    {
			if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2XP)==0)
			    bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_D2XP_MESSAGE_BADVERSION);
			else
			    bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_MESSAGE_BADVERSION);
			packet_append_string(rpacket,"");
			/* FIXME: actually mark the connection for close since we can't trust the
			   client to do so */
		    }
		    else
		    {
			/* Only handle updates when there is an update file available. */
			if (autoupdate_file(conn_get_archtag(c), conn_get_clienttag(c),verstr, versiontag)!=NULL)
			{
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] an upgrade from %s-%s %s(%s) to %s is available \"%s\"",conn_get_socket(c),conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag,autoupdate_version(conn_get_archtag(c), conn_get_clienttag(c),verstr,versiontag),autoupdate_file(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag));
			    if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2XP)==0)
				bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_D2XP_MESSAGE_UPDATE);
			    else
				bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_MESSAGE_UPDATE);
			    packet_append_string(rpacket,autoupdate_file(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag));
			}
			else
			{
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] no upgrades from %s %s are available",conn_get_socket(c),conn_get_clienttag(c),verstr);
			    if (bn_int_tag_eq(packet->u.client_progident.clienttag,CLIENTTAG_DIABLO2XP)==0)
				bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_D2XP_MESSAGE_OK);
			    else
		        	bn_int_set(&rpacket->u.server_authreply1.message,SERVER_AUTHREPLY1_MESSAGE_OK);
			    packet_append_string(rpacket,"");
			}
		    }
		    
		    packet_append_string(rpacket,""); /* FIXME: what's the second string for? */
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;

	case CLIENT_AUTHREQ_109:
	    if (packet_get_size(packet)<sizeof(t_client_authreq_109))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad AUTHREQ_109 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_authreq_109),packet_get_size(packet));
		break;
	    }
	    
	    {
		char verstr[16];
		char const * exeinfo;
		char const * versiontag;
		unsigned int versionid;
		unsigned int gameversion;
                unsigned int checksum;
                int          failed;
		char const * owner;
		unsigned int count;
		unsigned int pos;

		failed = 0;
		count = bn_int_get(packet->u.client_authreq_109.cdkey_number);
		pos = sizeof(t_client_authreq_109) + (count * sizeof(t_cdkey_info));
			
		if (!(exeinfo = packet_get_str_const(packet,pos,128)))
		{
		  eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad AUTHREQ_109 (missing or too long exeinfo)",conn_get_socket(c));
		  exeinfo = "badexe";
		  failed = 1;
		}
		conn_set_clientexe(c,exeinfo);
		pos += strlen(exeinfo) + 1;

		if (!(owner = packet_get_str_const(packet, pos, 128)))
		{
		  eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad AUTHREQ_109 (missing or too long owner)",conn_get_socket(c)); 
		  owner = ""; /* maybe owner was missing, use empty string */
		}
		conn_set_owner(c,owner);
	    		
		versionid = conn_get_versionid(c);
		checksum = bn_int_get(packet->u.client_authreq_109.checksum);
		gameversion = bn_int_get(packet->u.client_authreq_109.gameversion);
		strcpy(verstr,vernum_to_verstr(bn_int_get(packet->u.client_authreq_109.gameversion)));
		conn_set_clientver(c,verstr);

		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_AUTHREQ_109 ticks=0x%08x, verstr=%s exeinfo=\"%s\" versionid=0x%08x gameversion=0x%08x checksum=0x%08x",
		         conn_get_socket(c),
                         bn_int_get(packet->u.client_authreq_109.ticks),
			 verstr,
			 exeinfo,
			 versionid,
			 gameversion,
			 checksum);

		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_authreply_109));
		    packet_set_type(rpacket,SERVER_AUTHREPLY_109);
		    
		      switch (versioncheck_validate(conn_get_versioncheck(c),
                                                  conn_get_archtag(c),
                                                  conn_get_clienttag(c),
                                                  exeinfo,
                                                  versionid,
                                                  gameversion,
						  checksum,
						  &versiontag))
		      {
		      case -1: /* failed test... client has been modified */
                        if (!prefs_get_allow_bad_version())
			  {
                            eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client failed test (closing connection)",conn_get_socket(c));
                            failed = 1;
			  }
                        else
			  eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client failed test, allowing anyway",conn_get_socket(c));
                        break;
		      case 0: /* not listed in table... can't tell if client has been modified */
                        if (!prefs_get_allow_unknown_version())
			  {
                            eventlog(eventlog_level_info,"handle_bnet_packet","[%d] unable to test client (closing connection)",conn_get_socket(c));
                            failed = 1;
			  }
                        else
			  eventlog(eventlog_level_info,"handle_bnet_packet","[%d] unable to test client, allowing anyway",conn_get_socket(c));
                        break;
			/* 1 == test passed... client seems to be ok */
		      }

                    if (failed)
		      {
			bn_int_set(&rpacket->u.server_authreply_109.message,SERVER_AUTHREPLY_109_MESSAGE_BADVERSION);
			packet_append_string(rpacket,"");
		      }
                    else
		    {
		      /* Only handle updates when there is an update file available. */
		      if (autoupdate_file(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag)!=NULL)
		      {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] an upgrade from %s-%s %s(%s) to %s is available \"%s\"",conn_get_socket(c),conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag,autoupdate_version(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag),autoupdate_file(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag));
			bn_int_set(&rpacket->u.server_authreply_109.message,SERVER_AUTHREPLY_109_MESSAGE_UPDATE);
			packet_append_string(rpacket,autoupdate_file(conn_get_archtag(c),conn_get_clienttag(c),verstr,versiontag));
		      }
		      else
		      {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] no upgrades from %s %s are available",conn_get_socket(c),conn_get_clienttag(c),verstr);
		        bn_int_set(&rpacket->u.server_authreply_109.message,SERVER_AUTHREPLY_109_MESSAGE_OK);
			packet_append_string(rpacket,"");
		      }
		    }
		    
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_REGSNOOPREPLY:
	    if (packet_get_size(packet)<sizeof(t_client_regsnoopreply))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REGSNOOPREPLY packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_regsnoopreply),packet_get_size(packet));
		break;
	    }
	    break;
	    
	case CLIENT_ICONREQ:
	    if (packet_get_size(packet)<sizeof(t_client_iconreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ICONREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_iconreq),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_iconreply));
		packet_set_type(rpacket,SERVER_ICONREPLY);
		file_to_mod_time(prefs_get_iconfile(),&rpacket->u.server_iconreply.timestamp);
		packet_append_string(rpacket,prefs_get_iconfile());
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_CDKEY:
	    if (packet_get_size(packet)<sizeof(t_client_cdkey))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_cdkey),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * cdkey;
		char const * owner;
		
		if (!(cdkey = packet_get_str_const(packet,sizeof(t_client_cdkey),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY packet (missing or too long cdkey)",conn_get_socket(c));
		    break;
		}
		if (!(owner = packet_get_str_const(packet,sizeof(t_client_cdkey)+strlen(cdkey)+1,128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY packet (missing or too long owner)",conn_get_socket(c));
		    break;
		}
		
		conn_set_cdkey(c,cdkey);
		conn_set_owner(c,owner);
                
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_cdkeyreply));
		    packet_set_type(rpacket,SERVER_CDKEYREPLY);
		    bn_int_set(&rpacket->u.server_cdkeyreply.message,SERVER_CDKEYREPLY_MESSAGE_OK);
		    packet_append_string(rpacket,owner);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
#if 0 /* Blizzard used this to track down pirates, should only be accepted by old clients */
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_regsnoopreq));
		packet_set_type(rpacket,SERVER_REGSNOOPREQ);
		bn_int_set(&rpacket->u.server_regsnoopreq.unknown1,SERVER_REGSNOOPREQ_UNKNOWN1); /* sequence num */
		bn_int_set(&rpacket->u.server_regsnoopreq.hkey,SERVER_REGSNOOPREQ_HKEY_CURRENT_USER);
		packet_append_string(rpacket,SERVER_REGSNOOPREQ_REGKEY);
		packet_append_string(rpacket,SERVER_REGSNOOPREQ_REGVALNAME);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
#endif
	    break;
	    
	case CLIENT_CDKEY2:
	    if (packet_get_size(packet)<sizeof(t_client_cdkey2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_cdkey2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * owner;
		
		if (!(owner = packet_get_str_const(packet,sizeof(t_client_cdkey2),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY2 packet (missing or too long owner)",conn_get_socket(c));
		    break;
		}
		
		conn_set_owner(c,owner);
                
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_cdkeyreply2));
		    packet_set_type(rpacket,SERVER_CDKEYREPLY2);
		    bn_int_set(&rpacket->u.server_cdkeyreply2.message,SERVER_CDKEYREPLY2_MESSAGE_OK);
		    packet_append_string(rpacket,owner);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_CDKEY3:
	    if (packet_get_size(packet)<sizeof(t_client_cdkey3))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY3 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_cdkey2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * owner;
		
		if (!(owner = packet_get_str_const(packet,sizeof(t_client_cdkey3),128)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CDKEY3 packet (missing or too long owner)",conn_get_socket(c));
		    break;
		}
		
		conn_set_owner(c,owner);
                
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_cdkeyreply3));
		    packet_set_type(rpacket,SERVER_CDKEYREPLY3);
		    bn_int_set(&rpacket->u.server_cdkeyreply3.message,SERVER_CDKEYREPLY3_MESSAGE_OK);
		    packet_append_string(rpacket,""); /* FIXME: owner, message, ??? */
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_UDPOK:
	    if (packet_get_size(packet)<sizeof(t_client_udpok))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad UDPOK packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_udpok),packet_get_size(packet));
		break;
	    }
	    /* we could check the contents but there really isn't any point */
	    conn_set_udpok(c);
	    break;
	    
	case CLIENT_TOSREQ:
	    if (packet_get_size(packet)<sizeof(t_client_tosreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad TOSREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_tosreq),packet_get_size(packet));
		break;
	    }

	    {
                char const * tosfile;

                if (!(tosfile = packet_get_str_const(packet,sizeof(t_client_tosreq),128)))
                {
                    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad TOSREQ packet (missing or too long tosfile)",conn_get_socket(c));
                    break;
                }
                eventlog(eventlog_level_info,"handle_bnet_packet","[%d] TOS requested: \"%s\" - type = 0x%02x",conn_get_socket(c),tosfile, bn_int_get(packet->u.client_tosreq.type));
           
 /* TODO: if type is TOSFILE make bnetd to send default tosfile if selected is not found */ 
            if ((rpacket = packet_create(packet_class_bnet)))
            {
                packet_set_size(rpacket,sizeof(t_server_tosreply));
                packet_set_type(rpacket,SERVER_TOSREPLY);
                bn_int_set(&rpacket->u.server_tosreply.type,bn_int_get(packet->u.client_tosreq.type));
                bn_int_set(&rpacket->u.server_tosreply.unknown2,bn_int_get(packet->u.client_tosreq.unknown2));
                /* Note from Sherpya:
                   timestamp -> 0x852b7d00 - 0x01c0e863 b.net send this (bn_int),
                   I suppose is not a long 
                   if bnserver-D2DV is bad diablo 2 crashes 
                   timestamp doesn't work correctly and starcraft 
                   needs name in client locale or displays hostname */
                file_to_mod_time(tosfile,&rpacket->u.server_tosreply.timestamp);
                packet_append_string(rpacket,tosfile);
                queue_push_packet(conn_get_out_queue(c),rpacket);
                packet_del_ref(rpacket);
            }
            }

	    break;
	    
	case CLIENT_STATSREQ:
	    if (packet_get_size(packet)<sizeof(t_client_statsreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_statsreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * name;
		char const * key;
		unsigned int name_count;
		unsigned int key_count;
		unsigned int i,j;
		unsigned int name_off;
		unsigned int keys_off;
		unsigned int key_off;
		t_account *  account;
		char const * tval;
		char const * tname;
		
		name_count = bn_int_get(packet->u.client_statsreq.name_count);
		key_count = bn_int_get(packet->u.client_statsreq.key_count);
		
		for (i=0,name_off=sizeof(t_client_statsreq);
		     i<name_count && (name = packet_get_str_const(packet,name_off,128));
		     i++,name_off+=strlen(name)+1);
		if (i<name_count)
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSREQ packet (only %u names of %u)",conn_get_socket(c),i,name_count);
		    break;
		}
		keys_off = name_off;
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_statsreply));
		packet_set_type(rpacket,SERVER_STATSREPLY);
		bn_int_set(&rpacket->u.server_statsreply.name_count,name_count);
		bn_int_set(&rpacket->u.server_statsreply.key_count,key_count);
		bn_int_set(&rpacket->u.server_statsreply.unknown1,bn_int_get(packet->u.client_statsreq.unknown1));
		
		for (i=0,name_off=sizeof(t_client_statsreq);
		     i<name_count && (name = packet_get_str_const(packet,name_off,128));
		     i++,name_off+=strlen(name)+1)
		{
		    account = accountlist_find_account(name);
		    for (j=0,key_off=keys_off;
			 j<key_count && (key = packet_get_str_const(packet,key_off,512));
			 j++,key_off+=strlen(key)+1)
			if (account && (tval = account_get_strattr(account,key)))
			{
			    packet_append_string(rpacket,tval);
			    account_unget_strattr(tval);
			}
			else
			{
			    packet_append_string(rpacket,""); /* FIXME: what should really happen here? */
			    if (account && key[0]!='\0')
			    {
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] no entry \"%s\" in account \"%s\"",conn_get_socket(c),key,(tname = account_get_name(account)));
				account_unget_name(tname);
			    }
			}
		}
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    
	    break;
	    
	case CLIENT_LOGINREQ1:
	    if (packet_get_size(packet)<sizeof(t_client_loginreq1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LOGINREQ1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_loginreq1),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * username;
		t_account *  account;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_loginreq1),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LOGINREQ1 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_loginreply1));
		packet_set_type(rpacket,SERVER_LOGINREPLY1);
		
		/* already logged in */
		if (connlist_find_connection(username) &&
		    prefs_get_kick_old_login()==0)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (already logged in)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
		}
		else
#ifdef WITH_BITS
                    if (!bits_master)
                    {
			t_query * q;
			
			if (account_name_is_unknown(username)) {
			    if (bits_va_lock_account(username)<0) {
				eventlog(eventlog_level_error,"handle_bnet_packet","bits_va_lock_account failed");
			   	packet_del_ref(rpacket);
				break;
			    }
			}
			account = accountlist_find_account(username);
			
			if (!account_is_ready_for_use(account)) {
			    q = query_create(bits_query_type_client_loginreq_1);
			    if (!q)
				eventlog(eventlog_level_error,"handle_bnet_packet","could not create bits query");
			    query_attach_conn(q,"client",c);
			    query_attach_packet_const(q,"request",packet);
			    query_attach_account(q,"account",account);
			    packet_del_ref(rpacket);
			    break;
			}
			/* Account is ready for use now */
                    }
#endif
		/* fail if no account */
		if (!(account = accountlist_find_account(username)))
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (no such account)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
		}
		else if (account_get_auth_bnetlogin(account)==0) /* default to true */
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (no bnet access)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
		}
		else if (account_get_auth_lock(account)==1) /* default to false */
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (this account is locked)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
		}
		else if (conn_get_sessionkey(c)!=bn_int_get(packet->u.client_loginreq1.sessionkey))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] login for \"%s\" refused (expected session key 0x%08x, got 0x%08x)",conn_get_socket(c),username,conn_get_sessionkey(c),bn_int_get(packet->u.client_loginreq1.sessionkey));
		    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
		}
		else
		{
		    struct
		    {
			bn_int   ticks;
			bn_int   sessionkey;
			bn_int   passhash1[5];
		    }            temp;
		    char const * oldstrhash1;
		    t_hash       oldpasshash1;
		    t_hash       oldpasshash2;
		    t_hash       trypasshash2;
		    char const * tname;
		    
		    if ((oldstrhash1 = account_get_pass(account)))
		    {
			bn_int_set(&temp.ticks,bn_int_get(packet->u.client_loginreq1.ticks));
			bn_int_set(&temp.sessionkey,bn_int_get(packet->u.client_loginreq1.sessionkey));
			if (hash_set_str(&oldpasshash1,oldstrhash1)<0)
			{
			    account_unget_pass(oldstrhash1);
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (corrupted passhash1?)",conn_get_socket(c),(tname = account_get_name(account)));
			    account_unget_name(tname);
			    bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
			}
			else
			{
			    account_unget_pass(oldstrhash1);
			    hash_to_bnhash((t_hash const *)&oldpasshash1,temp.passhash1); /* avoid warning */
			    
			    bnet_hash(&oldpasshash2,sizeof(temp),&temp); /* do the double hash */
			    bnhash_to_hash(packet->u.client_loginreq1.password_hash2,&trypasshash2);
			    
			    if (hash_eq(trypasshash2,oldpasshash2)==1)
			    {
#ifdef WITH_BITS
				int rc = 0;

				if (bits_master) {
				    bn_int ct;
				    int sid;

				    if (conn_get_clienttag(c))
				        memcpy(&ct,conn_get_clienttag(c),4);
				    else
				        bn_int_set(&ct,0);
				    tname = account_get_name(account);
				    rc = bits_loginlist_add(account_get_uid(account),
				                            BITS_ADDR_MASTER,
				                            (sid = bits_va_loginlist_get_free_sessionid()),
				                            ct,
				                            conn_get_game_addr(c),
				                            conn_get_game_port(c),
				                            tname);
				    account_unget_name(tname);
				    conn_set_sessionid(c,sid);
				    /* Invoke the playerinfo update logic */
				    conn_get_playerinfo(c);
				/* Maybe it was already ready for use before: check if we have to create a query */
				} else if ((!query_current)||(query_get_type(query_current) != bits_query_type_client_loginreq_2)) { /* slave/client server */
				    t_query * q;

			    	    q = query_create(bits_query_type_client_loginreq_2);
			    	    if (!q)
					eventlog(eventlog_level_error,"handle_bnet_packet","could not create bits query");
			    	    query_attach_conn(q,"client",c);
			    	    query_attach_packet_const(q,"request",packet);
			    	    query_attach_account(q,"account",account);
			    	    packet_del_ref(rpacket);
				    send_bits_va_loginreq(query_get_qid(q),
	                                                  account_get_uid(account),
			                                  conn_get_game_addr(c),
			                                  conn_get_game_port(c),
			                                  conn_get_clienttag(c));
				    break;
				}
				if (rc == 0) {
#endif
				conn_set_account(c,account);
				eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" logged in (correct password)",conn_get_socket(c),(tname = conn_get_username(c)));
				conn_unget_username(c,tname);
				bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_SUCCESS);
#ifdef WITH_BITS
				} else {
					eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (bits_loginlist_add returned %d)",conn_get_socket(c),(tname = account_get_name(account)),rc);
					account_unget_name(tname);
					bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
				}
#endif
			    }
			    else
			    {
				eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (wrong password)",conn_get_socket(c),(tname = account_get_name(account)));
				account_unget_name(tname);
				bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
			    }
			}
		    }
		    else
		    {
			conn_set_account(c,account);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" logged in (no password)",conn_get_socket(c),(tname = account_get_name(account)));
			account_unget_name(tname);
			bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_SUCCESS);
		    }
		}
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_PINGREQ:
	    if (packet_get_size(packet)<sizeof(t_client_pingreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PINGREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_pingreq),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_pingreply));
		packet_set_type(rpacket,SERVER_PINGREPLY);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_LOGINREQ2:
	    if (packet_get_size(packet)<sizeof(t_client_loginreq2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LOGINREQ2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_loginreq2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * username;
		t_account *  account;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_loginreq2),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LOGINREQ2 (missing or too long username)",conn_get_socket(c));
		    break;
		}
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_loginreply2));
		packet_set_type(rpacket,SERVER_LOGINREPLY2);
		
		/* already logged in */
		if (connlist_find_connection(username) &&
		    prefs_get_kick_old_login()==0)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (already logged in)",conn_get_socket(c),username);
		    bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_ALREADY);
		}
		else
#ifdef WITH_BITS
                    if (!bits_master)
                    {
			if (account_name_is_unknown(username)) {
			    if (bits_va_lock_account(username)<0) {
				eventlog(eventlog_level_error,"handle_bnet_packet","bits_va_lock_account failed");
			   	packet_del_ref(rpacket);
				break;
			    }
			}
			account = accountlist_find_account(username);
                    }
#endif
		    /* fail if no account */
		    if (!(account = accountlist_find_account(username)))
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (no such account)",conn_get_socket(c),username);
			bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_BADPASS);
		    }
		    else if (account_get_auth_bnetlogin(account)==0) /* default to true */
		    {
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (no bnet access)",conn_get_socket(c),username);
			bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_BADPASS);
		    }
		    else if (conn_get_sessionkey(c)!=bn_int_get(packet->u.client_loginreq2.sessionkey))
		    {
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] login for \"%s\" refused (expected session key 0x%08x, got 0x%08x)",conn_get_socket(c),username,conn_get_sessionkey(c),bn_int_get(packet->u.client_loginreq2.sessionkey));
			bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_BADPASS);
		    }
		    else
		    {
			struct
			{
			    bn_int   ticks;
			    bn_int   sessionkey;
			    bn_int   passhash1[5];
			}            temp;
			char const * oldstrhash1;
			t_hash       oldpasshash1;
			t_hash       oldpasshash2;
			t_hash       trypasshash2;
			char const * tname;
			
			if ((oldstrhash1 = account_get_pass(account)))
			{
			    bn_int_set(&temp.ticks,bn_int_get(packet->u.client_loginreq2.ticks));
			    bn_int_set(&temp.sessionkey,bn_int_get(packet->u.client_loginreq2.sessionkey));
			    if (hash_set_str(&oldpasshash1,oldstrhash1)<0)
			    {
				account_unget_pass(oldstrhash1);
				eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (corrupted passhash1?)",conn_get_socket(c),(tname = account_get_name(account)));
				account_unget_name(tname);
				bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_BADPASS);
			    }
			    else
			    {
				account_unget_pass(oldstrhash1);
				hash_to_bnhash((t_hash const *)&oldpasshash1,temp.passhash1); /* avoid warning */
				    
				bnet_hash(&oldpasshash2,sizeof(temp),&temp); /* do the double hash */
				bnhash_to_hash(packet->u.client_loginreq2.password_hash2,&trypasshash2);
				    
				if (hash_eq(trypasshash2,oldpasshash2)==1)
				{
#ifdef WITH_BITS
				int rc = 0;

				if (bits_master) {
				    bn_int ct;
				    int sid;

				    if (conn_get_clienttag(c))
				        memcpy(&ct,conn_get_clienttag(c),4);
				    else
				        bn_int_set(&ct,0);
				    tname = account_get_name(account);
				    rc = bits_loginlist_add(account_get_uid(account),
				                            BITS_ADDR_MASTER,
				                            (sid = bits_va_loginlist_get_free_sessionid()),
				                            ct,
				                            conn_get_game_addr(c),
				                            conn_get_game_port(c),
				                            tname);
				    account_unget_name(tname);
				    conn_set_sessionid(c,sid);
				    /* Invoke the playerinfo update logic */
				    conn_get_playerinfo(c);				    
				/* Maybe it was already ready for use before: check if we have to create a query */
				} else if ((!query_current)||(query_get_type(query_current) != bits_query_type_client_loginreq_2)) { /* slave/client server */
				    t_query * q;

			    	    q = query_create(bits_query_type_client_loginreq_2);
			    	    if (!q)
					eventlog(eventlog_level_error,"handle_bnet_packet","could not create bits query");
			    	    query_attach_conn(q,"client",c);
			    	    query_attach_packet_const(q,"request",packet);
			    	    query_attach_account(q,"account",account);
			    	    packet_del_ref(rpacket);
				    send_bits_va_loginreq(query_get_qid(q),
	                                                  account_get_uid(account),
			                                  conn_get_game_addr(c),
			                                  conn_get_game_port(c),
			                                  conn_get_clienttag(c));
				    break;
				}
				if (rc == 0) {
#endif
				    conn_set_account(c,account);
				    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" logged in (correct password)",conn_get_socket(c),(tname = conn_get_username(c)));
				    conn_unget_username(c,tname);
				    bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_SUCCESS);
#ifdef WITH_BITS
				} else {
					eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (bits_loginlist_add returned %d)",conn_get_socket(c),(tname = account_get_name(account)),rc);
					account_unget_name(tname);
					bn_int_set(&rpacket->u.server_loginreply1.message,SERVER_LOGINREPLY1_MESSAGE_FAIL);
				}
#endif
				}
				else
				{
				    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] login for \"%s\" refused (wrong password)",conn_get_socket(c),(tname = account_get_name(account)));
				    account_unget_name(tname);
				    bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_BADPASS);
				}
			    }
			}
			else
			{
			    conn_set_account(c,account);
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" logged in (no password)",conn_get_socket(c),(tname = account_get_name(account)));
			    account_unget_name(tname);
			    bn_int_set(&rpacket->u.server_loginreply2.message,SERVER_LOGINREPLY2_MESSAGE_SUCCESS);
			}
		    }
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	default:
	    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown (unlogged in) bnet packet type 0x%04hx, len %u",conn_get_socket(c),packet_get_type(packet),packet_get_size(packet));
	}
	break;
	
    case conn_state_loggedin:
	
	switch (packet_get_type(packet))
	{
	case CLIENT_REALMLISTREQ:
	    if (packet_get_size(packet)<sizeof(t_client_realmlistreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMLISTREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_realmlistreq),packet_get_size(packet));
		break;
	    }
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		t_elem const *               curr;
		t_realm const *              realm;
		t_server_realmlistreply_data realmdata;
		unsigned int                 count;
		
		packet_set_size(rpacket,sizeof(t_server_realmlistreply));
		packet_set_type(rpacket,SERVER_REALMLISTREPLY);
		bn_int_set(&rpacket->u.server_realmlistreply.unknown1,SERVER_REALMLISTREPLY_UNKNOWN1);
		count = 0;
		LIST_TRAVERSE_CONST(realmlist(),curr)
		{
		    realm = elem_get_data(curr);
                    if (!realm_get_active(realm))
                    	continue;
		    bn_int_set(&realmdata.unknown3,SERVER_REALMLISTREPLY_DATA_UNKNOWN3);
		    bn_int_set(&realmdata.unknown4,SERVER_REALMLISTREPLY_DATA_UNKNOWN4);
		    bn_int_set(&realmdata.unknown5,SERVER_REALMLISTREPLY_DATA_UNKNOWN5);
		    bn_int_set(&realmdata.unknown6,SERVER_REALMLISTREPLY_DATA_UNKNOWN6);
		    bn_int_set(&realmdata.unknown7,SERVER_REALMLISTREPLY_DATA_UNKNOWN7);
		    bn_int_set(&realmdata.unknown8,SERVER_REALMLISTREPLY_DATA_UNKNOWN8);
		    bn_int_set(&realmdata.unknown9,SERVER_REALMLISTREPLY_DATA_UNKNOWN9);
		    packet_append_data(rpacket,&realmdata,sizeof(realmdata));
		    packet_append_string(rpacket,realm_get_name(realm));
		    packet_append_string(rpacket,realm_get_description(realm));
		    count++;
		}
		bn_int_set(&rpacket->u.server_realmlistreply.count,count);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_REALMJOINREQ:
	    if (packet_get_size(packet)<sizeof(t_client_realmjoinreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMJOINREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_realmjoinreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *    realmname;
		t_realm const * realm;
		
		if (!(realmname = packet_get_str_const(packet,sizeof(t_client_realmjoinreq),REALM_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMJOINREQ (missing or too long realmname)",conn_get_socket(c));
		    break;
		}
		
		if (!(realm = realmlist_find_realm(realmname)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMJOINREQ (missing or too long realmname)",conn_get_socket(c));
		    if ((rpacket = packet_create(packet_class_bnet)))
		    {
			packet_set_size(rpacket,sizeof(t_server_realmjoinreply));
			packet_set_type(rpacket,SERVER_REALMJOINREPLY);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown1,bn_int_get(packet->u.client_realmjoinreq.unknown1)); /* should this be copied on error? */
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown2,0);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown3,0); /* reg auth */
			bn_int_set(&rpacket->u.server_realmjoinreply.sessionkey,0);
			bn_int_nset(&rpacket->u.server_realmjoinreply.addr,0);
			bn_short_nset(&rpacket->u.server_realmjoinreply.port,0);
			bn_short_set(&rpacket->u.server_realmjoinreply.unknown6,0);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown7,0);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown8,0);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown9,0);
			bn_int_set(&rpacket->u.server_realmjoinreply.secret_hash[0],0);
			bn_int_set(&rpacket->u.server_realmjoinreply.secret_hash[1],0);
			bn_int_set(&rpacket->u.server_realmjoinreply.secret_hash[2],0);
			bn_int_set(&rpacket->u.server_realmjoinreply.secret_hash[3],0);
			bn_int_set(&rpacket->u.server_realmjoinreply.secret_hash[4],0);
			packet_append_string(rpacket,"");
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		    }
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
		    
		    conn_set_realmname(c,realm_get_name(realm)); /* FIXME: should we only set this after they log in to the realm server? */
		    
		    /* "password" to verify auth connection later, not sure what real Battle.net does */
		    salt = rand();
		    bn_int_set(&temp.salt,salt);
		    bn_int_set(&temp.sessionkey,conn_get_sessionkey(c));
		    bn_int_set(&temp.sessionnum,conn_get_sessionnum(c));
		    bn_int_set(&temp.secret,conn_get_secret(c));
		    bnet_hash(&secret_hash,sizeof(temp),&temp);
		    
		    if ((rpacket = packet_create(packet_class_bnet)))
		    {
			char const * tname;
			
			packet_set_size(rpacket,sizeof(t_server_realmjoinreply));
			packet_set_type(rpacket,SERVER_REALMJOINREPLY);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown1,bn_int_get(packet->u.client_realmjoinreq.unknown1));
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown2,SERVER_REALMJOINREPLY_UNKNOWN2);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown3,SERVER_REALMJOINREPLY_UNKNOWN3); /* reg auth */
			bn_int_set(&rpacket->u.server_realmjoinreply.sessionkey,conn_get_sessionkey(c));
			bn_int_nset(&rpacket->u.server_realmjoinreply.addr,realm_get_ip(realm));
			bn_short_nset(&rpacket->u.server_realmjoinreply.port,realm_get_port(realm));
			bn_short_set(&rpacket->u.server_realmjoinreply.unknown6,SERVER_REALMJOINREPLY_UNKNOWN6);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown7,SERVER_REALMJOINREPLY_UNKNOWN7);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown8,SERVER_REALMJOINREPLY_UNKNOWN8);
			bn_int_set(&rpacket->u.server_realmjoinreply.unknown9,salt);
			hash_to_bnhash((t_hash const *)&secret_hash,rpacket->u.server_realmjoinreply.secret_hash); /* avoid warning */
			packet_append_string(rpacket,(tname = conn_get_username(c)));
			conn_unget_username(c,tname);
			queue_push_packet(conn_get_out_queue(c),rpacket);
			packet_del_ref(rpacket);
		    }
		}
	    }
	    break;
	    
        case CLIENT_REALMJOINREQ_109:
	    if (packet_get_size(packet)<sizeof(t_client_realmjoinreq_109))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMJOINREQ_109 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_realmjoinreq_109),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *    realmname;
		t_realm * realm;
		
		if (!(realmname = packet_get_str_const(packet,sizeof(t_client_realmjoinreq_109),REALM_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad REALMJOINREQ_109 (missing or too long realmname)",conn_get_socket(c));
		    break;
		}
		
		if ((realm = realmlist_find_realm(realmname)))
                {
		    unsigned int salt;
		    struct
		    {
			bn_int   salt;
			bn_int   sessionkey;
			bn_int   sessionnum;
			bn_int   secret;
			bn_int   passhash[5];
		    } temp;
		    char const * pass_str;
		    t_hash       secret_hash;
		    t_hash       passhash;
            	    char const * prev_realm;
		    
		    /* FIXME: should we only set this after they log in to the realm server? */
                    prev_realm = conn_get_realmname(c);
                    if (prev_realm)
                    {
                         if (strcasecmp(prev_realm,realm_get_name(realm)))
                         {
                               realm_add_player_number(realm,1);
                               realm_add_player_number(realmlist_find_realm(prev_realm),-1);
                               conn_set_realmname(c,realm_get_name(realm));
			 }
		    }
                    else
                    {
                        realm_add_player_number(realm,1);
                        conn_set_realmname(c,realm_get_name(realm));
		    }
		    
		    if ((pass_str = account_get_pass(conn_get_account(c))))
		    {
			if (hash_set_str(&passhash,pass_str)==0)
			{
			    account_unget_pass(pass_str);
			    hash_to_bnhash((t_hash const *)&passhash,temp.passhash);
			    salt = bn_int_get(packet->u.client_realmjoinreq_109.seqno);
			    bn_int_set(&temp.salt,salt);
			    bn_int_set(&temp.sessionkey,conn_get_sessionkey(c));
			    bn_int_set(&temp.sessionnum,conn_get_sessionnum(c));
			    bn_int_set(&temp.secret,conn_get_secret(c));
			    bnet_hash(&secret_hash,sizeof(temp),&temp);
			    
			    if ((rpacket = packet_create(packet_class_bnet)))
			    {
				char const * tname;
				
				packet_set_size(rpacket,sizeof(t_server_realmjoinreply_109));
				packet_set_type(rpacket,SERVER_REALMJOINREPLY_109);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.seqno,salt);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.u1,0x0);
				bn_short_set(&rpacket->u.server_realmjoinreply_109.u3,0x0); /* reg auth */
				bn_int_set(&rpacket->u.server_realmjoinreply_109.bncs_addr1,0x0);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.sessionnum,conn_get_sessionnum(c));
				bn_int_nset(&rpacket->u.server_realmjoinreply_109.addr,realm_get_ip(realm));
				bn_short_nset(&rpacket->u.server_realmjoinreply_109.port,realm_get_port(realm));
				bn_int_set(&rpacket->u.server_realmjoinreply_109.sessionkey,conn_get_sessionkey(c));
				bn_int_set(&rpacket->u.server_realmjoinreply_109.u5,0);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.u6,0);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.bncs_addr2,0);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.u7,0);
				bn_int_set(&rpacket->u.server_realmjoinreply_109.versionid,conn_get_versionid(c));
				bn_int_tag_set(&rpacket->u.server_realmjoinreply_109.clienttag,conn_get_clienttag(c));
				hash_to_bnhash((t_hash const *)&secret_hash,rpacket->u.server_realmjoinreply_109.secret_hash); /* avoid warning */
				packet_append_string(rpacket,(tname = conn_get_username(c)));
				conn_unget_username(c,tname);
				queue_push_packet(conn_get_out_queue(c),rpacket);
				packet_del_ref(rpacket);
			    }
			    break;
			}
			else
			{
			    char const * tname;
			    
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] realm join for \"%s\" failed (unable to hash password)",conn_get_socket(c),(tname = account_get_name(conn_get_account(c))));
			    account_unget_name(tname);
			    account_unget_pass(pass_str);
			}
		    }
		    else
		    {
			char const * tname;
			
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] realm join for \"%s\" failed (no password)",conn_get_socket(c),(tname = account_get_name(conn_get_account(c))));
			account_unget_name(tname);
		    }
		}
		else
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] could not find active realm \"%s\"",conn_get_socket(c),realmname);
		
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_realmjoinreply_109));
		    packet_set_type(rpacket,SERVER_REALMJOINREPLY_109);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.seqno,bn_int_get(packet->u.client_realmjoinreq_109.seqno));
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.u1,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.sessionnum,0);
		    bn_short_set(&rpacket->u.server_realmjoinreply_109.u3,0);
		    bn_int_nset(&rpacket->u.server_realmjoinreply_109.addr,0);
		    bn_short_nset(&rpacket->u.server_realmjoinreply_109.port,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.sessionkey,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.u5,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.u6,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.u7,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.bncs_addr1,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.bncs_addr2,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.versionid,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.clienttag,0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.secret_hash[0],0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.secret_hash[1],0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.secret_hash[2],0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.secret_hash[3],0);
		    bn_int_set(&rpacket->u.server_realmjoinreply_109.secret_hash[4],0);
		    packet_append_string(rpacket,"");
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_UNKNOWN_37: /* CHARLISTREQ? (closed?) */
	    if (packet_get_size(packet)<sizeof(t_client_unknown_37))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad UNKNOWN_37 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_unknown_37),packet_get_size(packet));
		break;
	    }
		/*
		  0x0070:                           83 80 ff ff ff ff ff 2f    t,taran,......./
		  0x0080: ff ff ff ff ff ff ff ff   ff ff 03 ff ff ff ff ff    ................
		  0x0090: ff ff ff ff ff ff ff ff   ff ff ff 07 80 80 80 80    ................
		  0x00a0: ff ff ff 00
		*/
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		char const * charlist;
		char *       temp;
		
		packet_set_size(rpacket,sizeof(t_server_unknown_37));
		packet_set_type(rpacket,SERVER_UNKNOWN_37);
		bn_int_set(&rpacket->u.server_unknown_37.unknown1,SERVER_UNKNOWN_37_UNKNOWN1);
		bn_int_set(&rpacket->u.server_unknown_37.unknown2,SERVER_UNKNOWN_37_UNKNOWN2);
		
		if (!(charlist = account_get_closed_characterlist(conn_get_account(c),conn_get_clienttag(c),conn_get_realmname(c))))
		{
		    bn_int_set(&rpacket->u.server_unknown_37.count,0);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		    break;
		}
		if (!(temp = strdup(charlist)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unable to allocate memory for characterlist",conn_get_socket(c));
		    bn_int_set(&rpacket->u.server_unknown_37.count,0);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		    account_unget_closed_characterlist(conn_get_account(c),charlist);
		    break;
		}
		account_unget_closed_characterlist(conn_get_account(c),charlist);
		
		{
		    char const *        tok1;
		    char const *        tok2;
		    t_character const * ch;
		    unsigned int        count;
		    
		    count = 0;
		    tok1 = (char const *)strtok(temp,","); /* strtok modifies the string it is passed */
		    tok2 = strtok(NULL,",");
		    while (tok1)
		    {
			if (!tok2)
			{
			    char const * tname;
			    
			    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] account \"%s\" has bad character list \"%s\"",conn_get_socket(c),(tname = conn_get_username(c)),temp);
			    conn_unget_username(c,tname);
			    break;
			}
			
			if ((ch = characterlist_find_character(tok1,tok2)))
			{
			    packet_append_ntstring(rpacket,character_get_realmname(ch));
			    packet_append_ntstring(rpacket,",");
			    packet_append_string(rpacket,character_get_name(ch));
			    packet_append_string(rpacket,character_get_playerinfo(ch));
			    packet_append_string(rpacket,character_get_guildname(ch));
			    count++;
			}
			else
			    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] character \"%s\" is missing",conn_get_socket(c),tok2);
			tok1 = strtok(NULL,",");
			tok2 = strtok(NULL,",");
		    }
		    free(temp);
		    
		    bn_int_set(&rpacket->u.server_unknown_37.count,count);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_UNKNOWN_39:
	    if (packet_get_size(packet)<sizeof(t_client_unknown_39))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad UNKNOWN_39 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_unknown_39),packet_get_size(packet));
		break;
	    }
	    /* FIXME: what do we do with this one? Is this the chosen character? */
	    break;
	    
	case CLIENT_ECHOREPLY:
	    if (packet_get_size(packet)<sizeof(t_client_echoreply))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ECHOREPLY packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_echoreply),packet_get_size(packet));
		break;
	    }
	    
	    {
		unsigned int now;
		unsigned int then;
		
		now = get_ticks();
		then = bn_int_get(packet->u.client_echoreply.ticks);
		if (!now || !then || now<then)
		    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] bad timing in echo reply: now=%u then=%u",conn_get_socket(c),now,then);
		else
		    conn_set_latency(c,now-then);
	    }
	    break;
	    
	case CLIENT_PINGREQ:
	    if (packet_get_size(packet)<sizeof(t_client_pingreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PINGREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_pingreq),packet_get_size(packet));
		break;
	    }
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_pingreply));
		packet_set_type(rpacket,SERVER_PINGREPLY);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_ADREQ:
	    if (packet_get_size(packet)<sizeof(t_client_adreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ADREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_adreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		t_adbanner * ad;
		
		if (!(ad = adbanner_pick(c,bn_int_get(packet->u.client_adreq.prev_adid))))
		    break;
		
		/*		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] picking ad file=\"%s\" id=0x%06x tag=%u",conn_get_socket(c),adbanner_get_filename(ad),adbanner_get_id(ad),adbanner_get_extensiontag(ad));*/
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_adreply));
		    packet_set_type(rpacket,SERVER_ADREPLY);
		    bn_int_set(&rpacket->u.server_adreply.adid,adbanner_get_id(ad));
		    bn_int_set(&rpacket->u.server_adreply.extensiontag,adbanner_get_extensiontag(ad));
		    file_to_mod_time(adbanner_get_filename(ad),&rpacket->u.server_adreply.timestamp);
		    packet_append_string(rpacket,adbanner_get_filename(ad));
		    packet_append_string(rpacket,adbanner_get_link(ad));
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_ADACK:
	    if (packet_get_size(packet)<sizeof(t_client_adack))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ADACK packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_adack),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * tname;
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] ad acknowledgement for adid 0x%04hx from \"%s\"",conn_get_socket(c),bn_int_get(packet->u.client_adack.adid),(tname = conn_get_chatname(c)));
		conn_unget_chatname(c,tname);
	    }
	    break;
	    
	case CLIENT_ADCLICK:
	    if (packet_get_size(packet)<sizeof(t_client_adclick))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ADCLICK packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_adclick),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * tname;
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] ad click for adid 0x%04hx from \"%s\"",
			 conn_get_socket(c),bn_int_get(packet->u.client_adclick.adid),(tname = conn_get_username(c)));
		conn_unget_username(c,tname);
	    }
	    break;
	    
	case CLIENT_ADCLICK2:
	    if (packet_get_size(packet)<sizeof(t_client_adclick2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad ADCLICK2 packet (expected %u bytes, got %u)",conn_get_socket(c), sizeof(t_client_adclick2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * tname;
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] ad click2 for adid 0x%04hx from \"%s\"", conn_get_socket(c),bn_int_get(packet->u.client_adclick2.adid),(tname = conn_get_username(c)));
		conn_unget_username(c,tname);
	    }
	    
	    {
		t_adbanner * ad;
		
		if (!(ad = adbanner_pick(c,bn_int_get(packet->u.client_adclick2.adid))))
		    break;
		
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    packet_set_size(rpacket,sizeof(t_server_adclickreply2));
		    packet_set_type(rpacket,SERVER_ADCLICKREPLY2);
		    bn_int_set(&rpacket->u.server_adclickreply2.adid,adbanner_get_id(ad));
		    packet_append_string(rpacket,adbanner_get_link(ad));
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
	    }
	    break;
	    
	case CLIENT_STATSREQ:
	    if (packet_get_size(packet)<sizeof(t_client_statsreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_statsreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * name;
		char const * key;
		unsigned int name_count;
		unsigned int key_count;
		unsigned int i,j;
		unsigned int name_off;
		unsigned int keys_off;
		unsigned int key_off;
		t_account *  account;
		char const * tval;
		char const * tname;
		
		name_count = bn_int_get(packet->u.client_statsreq.name_count);
		key_count = bn_int_get(packet->u.client_statsreq.key_count);
		
		for (i=0,name_off=sizeof(t_client_statsreq);
		     i<name_count && (name = packet_get_str_const(packet,name_off,128));
		     i++,name_off+=strlen(name)+1);
		if (i<name_count)
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSREQ packet (only %u names of %u)",conn_get_socket(c),i,name_count);
		    break;
		}
		keys_off = name_off;
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_statsreply));
		packet_set_type(rpacket,SERVER_STATSREPLY);
		bn_int_set(&rpacket->u.server_statsreply.name_count,name_count);
		bn_int_set(&rpacket->u.server_statsreply.key_count,key_count);
		bn_int_set(&rpacket->u.server_statsreply.unknown1,bn_int_get(packet->u.client_statsreq.unknown1));
		
		for (i=0,name_off=sizeof(t_client_statsreq);
		     i<name_count && (name = packet_get_str_const(packet,name_off,128));
		     i++,name_off+=strlen(name)+1)
		{
		    if (name[0]=='\0')
			account = conn_get_account(c);
		    else
			account = accountlist_find_account(name);
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] sending account info for account \"%s\" (\"%s\")",conn_get_socket(c),account_get_name(account),name);
		    for (j=0,key_off=keys_off;
			 j<key_count && (key = packet_get_str_const(packet,key_off,512));
			 j++,key_off+=strlen(key)+1)
			if (account && strcasecmp(key,"BNET\\acct\\passhash1")==0)
			{
			    packet_append_string(rpacket,"hello");
			    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] requested key \"%s\" \"%s\"",conn_get_socket(c),key,(tname = account_get_name(account)));
			    account_unget_name(tname);
			}
			else if (account && (tval = account_get_strattr(account,key)))
			{
			    packet_append_string(rpacket,tval);
			    account_unget_strattr(tval);
			}
			else
			{
			    packet_append_string(rpacket,""); /* FIXME: what should really happen here? */
			    if (account && key[0]!='\0')
			    {
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] no entry \"%s\" or no account \"%s\"",conn_get_socket(c),key,(tname = account_get_name(account)));
				account_unget_name(tname);
			    }
			}
		}
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_STATSUPDATE:
	    if (packet_get_size(packet)<sizeof(t_client_statsupdate))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSUPDATE packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_statsupdate),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * name;
		char const * key;
		char const * val;
		unsigned int name_count;
		unsigned int key_count;
		unsigned int i,j;
		unsigned int name_off;
		unsigned int keys_off;
		unsigned int key_off;
		unsigned int vals_off;
		unsigned int val_off;
		t_account *  account;
		
		name_count = bn_int_get(packet->u.client_statsupdate.name_count);
		key_count = bn_int_get(packet->u.client_statsupdate.key_count);
		
		if (name_count!=1)
		    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] got suspicious STATSUPDATE packet (name_count=%u)",conn_get_socket(c),name_count);
		
		for (i=0,name_off=sizeof(t_client_statsupdate);
		     i<name_count && (name = packet_get_str_const(packet,name_off,128));
		     i++,name_off+=strlen(name)+1);
		if (i<name_count)
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSUPDATE packet (only %u names of %u)",conn_get_socket(c),i,name_count);
		    break;
		}
		keys_off = name_off;
		
		for (i=0,key_off=keys_off;
		     i<key_count && (key = packet_get_str_const(packet,key_off,1024));
		     i++,key_off+=strlen(key)+1);
		if (i<key_count)
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STATSUPDATE packet (only %u keys of %u)",conn_get_socket(c),i,key_count);
		    break;
		}
		vals_off = key_off;
		
		if ((account = conn_get_account(c)))
		{
		    char const * tname;
			
		    if (account_get_auth_changeprofile(account)==0) /* default to true */
		    {
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] stats update for \"%s\" refused (no change profile access)",conn_get_socket(c),(tname = conn_get_username(c)));
			conn_unget_username(c,tname);
			break;
		    }
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] updating player profile for \"%s\"",conn_get_socket(c),(tname = conn_get_username(c)));
		    conn_unget_username(c,tname);
		    
		    for (i=0,name_off=sizeof(t_client_statsupdate);
			 i<name_count && (name = packet_get_str_const(packet,name_off,128));
			 i++,name_off+=strlen(name)+1)
			for (j=0,key_off=keys_off,val_off=vals_off;
			     j<key_count && (key = packet_get_str_const(packet,key_off,1024)) && (val = packet_get_str_const(packet,val_off,1024));
			     j++,key_off+=strlen(key)+1,val_off+=strlen(val)+1)
			    if (strlen(key)<9 || strncasecmp(key,"profile\\",8)!=0)
				eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got STATSUPDATE with suspicious key \"%s\" value \"%s\"",conn_get_socket(c),key,val);
			    else
				account_set_strattr(account,key,val);
		}
	    }
	    break;
	    
	case CLIENT_PLAYERINFOREQ:
	    if (packet_get_size(packet)<sizeof(t_client_playerinforeq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PLAYERINFOREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_playerinforeq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * username;
		char const * info;
		t_account *  account;
		
		if (!(username = packet_get_str_const(packet,sizeof(t_client_playerinforeq),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PLAYERINFOREQ (missing or too long username)",conn_get_socket(c));
		    break;
		}
		if (!(info = packet_get_str_const(packet,sizeof(t_client_playerinforeq)+strlen(username)+1,1024)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PLAYERINFOREQ (missing or too long info)",conn_get_socket(c));
		    break;
		}
		
		if (info[0]!='\0')
		    conn_set_playerinfo(c,info);
		
		account = conn_get_account(c);
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_playerinforeply));
		packet_set_type(rpacket,SERVER_PLAYERINFOREPLY);
		
		if (account)
		{
		    char const * tname;
		    char const * str;
		    
		    if (!(tname = account_get_name(account)))
			str = username;
		    else
			str = tname;
		    packet_append_string(rpacket,str);
		    packet_append_string(rpacket,conn_get_playerinfo(c));
		    packet_append_string(rpacket,str);
		    if (tname)
			account_unget_name(tname);
		}
		else
		{
		    packet_append_string(rpacket,"");
		    packet_append_string(rpacket,"");
		    packet_append_string(rpacket,"");
		}
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_PROGIDENT2:
	    if (packet_get_size(packet)<sizeof(t_client_progident2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad PROGIDENT2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_progident2),packet_get_size(packet));
		break;
	    }
	    
	    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] CLIENT_PROGIDENT2 clienttag=0x%08x",conn_get_socket(c),bn_int_get(packet->u.client_progident2.clienttag));
	    
	    /* Hmm... no archtag.  Hope we get it in CLIENT_AUTHREQ1 (but we won't if we use the shortcut) */
	    
	    if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_STARCRAFT)==0)
		conn_set_clienttag(c,CLIENTTAG_STARCRAFT);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_BROODWARS)==0)
		conn_set_clienttag(c,CLIENTTAG_BROODWARS);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_SHAREWARE)==0)
		conn_set_clienttag(c,CLIENTTAG_SHAREWARE);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_DIABLORTL)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLORTL);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_DIABLOSHR)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLOSHR);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_WARCIIBNE)==0)
		conn_set_clienttag(c,CLIENTTAG_WARCIIBNE);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_DIABLO2DV)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLO2DV);
	    else if (bn_int_tag_eq(packet->u.client_progident2.clienttag,CLIENTTAG_DIABLO2XP)==0)
		conn_set_clienttag(c,CLIENTTAG_DIABLO2XP);
	    else
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown client program type 0x%08x, don't expect this to work",conn_get_socket(c),bn_int_get(packet->u.client_progident2.clienttag));
	    
	    if ((rpacket = packet_create(packet_class_bnet)))
	    {
		packet_set_size(rpacket,sizeof(t_server_channellist));
		packet_set_type(rpacket,SERVER_CHANNELLIST);
		{
		    t_channel *    ch;
		    t_elem const * curr;
		    
		    LIST_TRAVERSE_CONST(channellist(),curr)
		    {
			ch = elem_get_data(curr);
			if ((!prefs_get_hide_temp_channels() || channel_get_permanent(ch)) &&
			    (!channel_get_clienttag(ch) || strcmp(channel_get_clienttag(ch),conn_get_clienttag(c))==0))
			    packet_append_string(rpacket,channel_get_name(ch));
		    }
		}
		packet_append_string(rpacket,"");
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_JOINCHANNEL:
	    if (packet_get_size(packet)<sizeof(t_client_joinchannel))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad JOINCHANNEL packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_joinchannel),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * cname;
		int          found=1;
		
		if (!(cname = packet_get_str_const(packet,sizeof(t_client_joinchannel),CHANNEL_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad JOINCHANNEL (missing or too long cname)",conn_get_socket(c));
		    break;
		}
		
		switch (bn_int_get(packet->u.client_joinchannel.channelflag))
		{
		case CLIENT_JOINCHANNEL_NORMAL:
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_JOINCHANNEL_NORMAL channel \"%s\"",conn_get_socket(c),cname);
		    
		    /* Does that channel exist? */
		    if (channellist_find_channel_by_name(cname,conn_get_country(c),conn_get_realmname(c)))
			break; /* just join it */
		    else if (prefs_get_ask_new_channel())
		    {
			found=0;
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] didn't find channel \"%s\" to join",conn_get_socket(c),cname);
			message_send_text(c,message_type_channeldoesnotexist,c,cname);
		    }
		    break;
		case CLIENT_JOINCHANNEL_GENERIC:
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_JOINCHANNEL_GENERIC channel \"%s\"",conn_get_socket(c),cname);
		    /* don't have to do anything here */
		    break;
		case CLIENT_JOINCHANNEL_CREATE:
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_JOINCHANNEL_CREATE channel \"%s\"",conn_get_socket(c),cname);
		    /* don't have to do anything here */
		    break;
		}
		if (found && conn_set_channel(c,cname)<0)
		    conn_set_channel(c,CHANNEL_NAME_BANNED); /* should not fail */
	    }
	    break;
	    
	case CLIENT_MESSAGE:
	    if (packet_get_size(packet)<sizeof(t_client_message))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MESSAGE packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_message),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *      text;
		t_channel const * channel;
		
		if (!(text = packet_get_str_const(packet,sizeof(t_client_message),MAX_MESSAGE_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MESSAGE (missing or too long text)",conn_get_socket(c));
		    break;
		}
		
		conn_set_idletime(c);
		
		if ((channel = conn_get_channel(c)))
		    channel_message_log(channel,c,1,text);
		/* we don't log game commands currently */
		
		if (text[0]=='/')
		    handle_command(c,text);
		else
		    if (channel && !conn_quota_exceeded(c,text))
			channel_message_send(channel,message_type_talk,c,text);
		    /* else discard */
	    }
	    break;
	    
	case CLIENT_GAMELISTREQ:
	    if (packet_get_size(packet)<sizeof(t_client_gamelistreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad GAMELISTREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_gamelistreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *                gamename;
		unsigned short              bngtype;
		t_game_type                 gtype;
#ifndef WITH_BITS
		t_game *                    game;
		t_server_gamelistreply_game glgame;
		unsigned int                addr;
		unsigned short              port;
#endif
		
		if (!(gamename = packet_get_str_const(packet,sizeof(t_client_gamelistreq),GAME_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad GAMELISTREQ (missing or too long gamename)",conn_get_socket(c));
		    break;
		}
		
		bngtype = bn_short_get(packet->u.client_gamelistreq.gametype);
		gtype = bngreqtype_to_gtype(conn_get_clienttag(c),bngtype);
#ifdef WITH_BITS
		/* FIXME: what about maxgames? */
		bits_game_handle_client_gamelistreq(c,gtype,0,gamename);
#else		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_gamelistreply));
		packet_set_type(rpacket,SERVER_GAMELISTREPLY);
		
		/* specific game requested? */
		if (gamename[0]!='\0')
		{
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] GAMELISTREPLY looking for specific game tag=\"%s\" bngtype=0x%08x gtype=%d name=\"%s\"",conn_get_socket(c),conn_get_clienttag(c),bngtype,(int)gtype,gamename);
		    if ((game = gamelist_find_game(gamename,gtype)))
		    {
			bn_int_set(&glgame.unknown7,SERVER_GAMELISTREPLY_GAME_UNKNOWN7);
			bn_short_set(&glgame.gametype,gtype_to_bngtype(game_get_type(game)));
			bn_short_set(&glgame.unknown1,SERVER_GAMELISTREPLY_GAME_UNKNOWN1);
			bn_short_set(&glgame.unknown3,SERVER_GAMELISTREPLY_GAME_UNKNOWN3);
			addr = game_get_addr(game);
			port = game_get_port(game);
			gametrans_net(conn_get_addr(c),conn_get_port(c),conn_get_local_addr(c),conn_get_local_port(c),&addr,&port);
			bn_short_nset(&glgame.port,port);
			bn_int_nset(&glgame.game_ip,addr);
			bn_int_set(&glgame.unknown4,SERVER_GAMELISTREPLY_GAME_UNKNOWN4);
			bn_int_set(&glgame.unknown5,SERVER_GAMELISTREPLY_GAME_UNKNOWN5);
			switch (game_get_status(game))
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
			    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] game \"%s\" has bad status %d",conn_get_socket(c),game_get_name(game),game_get_status(game));
			    bn_int_set(&glgame.status,0);
			}
			bn_int_set(&glgame.unknown6,SERVER_GAMELISTREPLY_GAME_UNKNOWN6);
			    
			packet_append_data(rpacket,&glgame,sizeof(glgame));
			packet_append_string(rpacket,game_get_name(game));
			packet_append_string(rpacket,game_get_pass(game));
			packet_append_string(rpacket,game_get_info(game));
			bn_int_set(&rpacket->u.server_gamelistreply.gamecount,1);
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] GAMELISTREPLY found it",conn_get_socket(c));
		    }
		    else
		    {
			bn_int_set(&rpacket->u.server_gamelistreply.gamecount,0);
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] GAMELISTREPLY doesn't seem to exist",conn_get_socket(c));
		    }
		}
		else /* list all public games of this type */
		{
		    unsigned int   counter=0;
		    unsigned int   tcount;
		    t_elem const * curr;
		    
		    if (gtype==game_type_all)
			eventlog(eventlog_level_debug,"handle_bnet_packet","GAMELISTREPLY looking for public games tag=\"%s\" bngtype=0x%08x gtype=all",conn_get_clienttag(c),bngtype);
		    else
			eventlog(eventlog_level_debug,"handle_bnet_packet","GAMELISTREPLY looking for public games tag=\"%s\" bngtype=0x%08x gtype=%d",conn_get_clienttag(c),bngtype,(int)gtype);
		    
		    counter = 0;
		    tcount = 0;
		    LIST_TRAVERSE_CONST(gamelist(),curr)
		    {
			game = elem_get_data(curr);
			tcount++;
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] considering listing game=\"%s\", pass=\"%s\" clienttag=\"%s\" gtype=%d",conn_get_socket(c),game_get_name(game),game_get_pass(game),game_get_clienttag(game),(int)game_get_type(game));
			
			if (prefs_get_hide_pass_games() && strcmp(game_get_pass(game),"")!=0)
			{
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] not listing because game is private",conn_get_socket(c));
			    continue;
			}
			if (prefs_get_hide_started_games() && game_get_status(game)!=game_status_open)
			{
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] not listing because game is started",conn_get_socket(c));
			    continue;
			}
			if (strcmp(game_get_clienttag(game),conn_get_clienttag(c))!=0)
			{
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] not listing because game is for a different client",conn_get_socket(c));
			    continue;
			}
			if (gtype!=game_type_all && game_get_type(game)!=gtype)
			{
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] not listing because game is wrong type",conn_get_socket(c));
			    continue;
			}
			
			bn_int_set(&glgame.unknown7,SERVER_GAMELISTREPLY_GAME_UNKNOWN7);
			bn_short_set(&glgame.gametype,gtype_to_bngtype(game_get_type(game)));
			bn_short_set(&glgame.unknown1,SERVER_GAMELISTREPLY_GAME_UNKNOWN1);
			bn_short_set(&glgame.unknown3,SERVER_GAMELISTREPLY_GAME_UNKNOWN3);
			addr = game_get_addr(game);
			port = game_get_port(game);
			gametrans_net(conn_get_addr(c),conn_get_port(c),conn_get_local_addr(c),conn_get_local_port(c),&addr,&port);
			bn_short_nset(&glgame.port,port);
			bn_int_nset(&glgame.game_ip,addr);
			bn_int_set(&glgame.unknown4,SERVER_GAMELISTREPLY_GAME_UNKNOWN4);
			bn_int_set(&glgame.unknown5,SERVER_GAMELISTREPLY_GAME_UNKNOWN5);
			switch (game_get_status(game))
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
			    eventlog(eventlog_level_warn,"handle_bnet_packet","[%d] game \"%s\" has bad status=%d",conn_get_socket(c),game_get_name(game),(int)game_get_status(game));
			    bn_int_set(&glgame.status,0);
			}
			bn_int_set(&glgame.unknown6,SERVER_GAMELISTREPLY_GAME_UNKNOWN6);
			
			if (packet_get_size(rpacket)+
			    sizeof(glgame)+
			    strlen(game_get_name(game))+1+
			    strlen(game_get_pass(game))+1+
			    strlen(game_get_info(game))+1>MAX_PACKET_SIZE)
			{
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] out of room for games",conn_get_socket(c));
			    break; /* no more room */
			}
			
			packet_append_data(rpacket,&glgame,sizeof(glgame));
			packet_append_string(rpacket,game_get_name(game));
			packet_append_string(rpacket,game_get_pass(game));
			packet_append_string(rpacket,game_get_info(game));
			counter++;
		    }
		    
		    bn_int_set(&rpacket->u.server_gamelistreply.gamecount,counter);
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] GAMELISTREPLY sent %u of %u games",conn_get_socket(c),counter,tcount);
		}
		
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
#endif  /* WITH_BITS */
	    }
	    break;
	    
	case CLIENT_JOIN_GAME:
	    if (packet_get_size(packet)<sizeof(t_client_join_game))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad JOIN_GAME packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_join_game),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * gamename;
		char const * gamepass;
		char const * tname;
#ifndef WITH_BITS
		t_game *     game;
#else
		t_bits_game * game;
#endif
		t_game_type  gtype;
		
		if (!(gamename = packet_get_str_const(packet,sizeof(t_client_join_game),GAME_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CLIENT_JOIN_GAME (missing or too long gamename)",conn_get_socket(c));
		    break;
		}
		if (!(gamepass = packet_get_str_const(packet,sizeof(t_client_join_game)+strlen(gamename)+1,GAME_PASS_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad CLIENT_JOIN_GAME packet (missing or too long gamepass)",conn_get_socket(c));
		    break;
		}
		
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] trying to join game \"%s\" pass=\"%s\"",conn_get_socket(c),gamename,gamepass);
#ifndef WITH_BITS
		if (!(game = gamelist_find_game(gamename,game_type_all)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unable to find game \"%s\" for user to join",conn_get_socket(c),gamename);
		    break;
		}
		gtype = game_get_type(game);
		gamename = game_get_name(game);
#else /* WITH_BITS */
		if (!(game = bits_gamelist_find_game(gamename,game_type_all)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unable to find game \"%s\" for user to join",conn_get_socket(c),gamename);
		    break;
		}
		gtype = bits_game_get_type(game);
		gamename = bits_game_get_name(game);
#endif /* WITH_BITS */
		/* bits: since the connection needs the account it should already be loaded so there should be no problem */		
		if ((gtype==game_type_ladder && account_get_auth_joinladdergame(conn_get_account(c))==0) || /* default to true */
		    (gtype!=game_type_ladder && account_get_auth_joinnormalgame(conn_get_account(c))==0)) /* default to true */
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game join for \"%s\" to \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)),gamename);
		    conn_unget_username(c,tname);
		    /* If the user is not in a game, then map authorization
		       will fail and keep them from playing. */
		    break;
		}
		
		if (conn_set_game(c,gamename,gamepass,"",gtype,STARTVER_UNKNOWN)<0)
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" joined game \"%s\", but could not be recorded on server",conn_get_socket(c),(tname = conn_get_username(c)),gamename);
		else
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] \"%s\" joined game \"%s\"",conn_get_socket(c),(tname = conn_get_username(c)),gamename);
		conn_unget_username(c,tname);
	    }
	    break;
	    
	case CLIENT_STARTGAME1:
	    if (packet_get_size(packet)<sizeof(t_client_startgame1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_startgame1),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *   gamename;
		char const *   gamepass;
		char const *   gameinfo;
		unsigned short bngtype;
		unsigned int   status;
#ifndef WITH_BITS
		t_game *       currgame;
#endif
		
		if (!(gamename = packet_get_str_const(packet,sizeof(t_client_startgame1),GAME_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME1 packet (missing or too long gamename)",conn_get_socket(c));
		    break;
		}
		if (!(gamepass = packet_get_str_const(packet,sizeof(t_client_startgame1)+strlen(gamename)+1,GAME_PASS_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME1 packet (missing or too long gamepass)",conn_get_socket(c));
		    break;
		}
		if (!(gameinfo = packet_get_str_const(packet,sizeof(t_client_startgame1)+strlen(gamename)+1+strlen(gamepass)+1,GAME_INFO_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME1 packet (missing or too long gameinfo)",conn_get_socket(c));
		    break;
		}
		
		bngtype = bn_short_get(packet->u.client_startgame1.gametype);
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got startgame1 status for game \"%s\" is 0x%08x (gametype = 0x%04hx)",conn_get_socket(c),gamename,bn_int_get(packet->u.client_startgame1.status),bngtype);
		status = bn_int_get(packet->u.client_startgame1.status)&CLIENT_STARTGAME1_STATUSMASK;

#ifdef WITH_BITS
		bits_game_handle_startgame1(c,gamename,gamepass,gameinfo,bngtype,status);
#else
		if ((currgame = conn_get_game(c)))
		{
		    switch (status)
		    {
		    case CLIENT_STARTGAME1_STATUS_STARTED:
			game_set_status(currgame,game_status_started);
			break;
		    case CLIENT_STARTGAME1_STATUS_FULL:
			game_set_status(currgame,game_status_full);
			break;
		    case CLIENT_STARTGAME1_STATUS_OPEN:
			game_set_status(currgame,game_status_open);
			break;
		    case CLIENT_STARTGAME1_STATUS_DONE:
			game_set_status(currgame,game_status_done);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
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
			    
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
			    conn_unget_username(c,tname);
			}
			else
			    conn_set_game(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW1);
			
			if ((rpacket = packet_create(packet_class_bnet)))
			{
			    packet_set_size(rpacket,sizeof(t_server_startgame1_ack));
			    packet_set_type(rpacket,SERVER_STARTGAME1_ACK);
			    
			    if (conn_get_game(c))
				bn_int_set(&rpacket->u.server_startgame1_ack.reply,SERVER_STARTGAME1_ACK_OK);
			    else
				bn_int_set(&rpacket->u.server_startgame1_ack.reply,SERVER_STARTGAME1_ACK_NO);
			    
			    queue_push_packet(conn_get_out_queue(c),rpacket);
			    packet_del_ref(rpacket);
			}
		    }
		    else
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client tried to set game status to DONE",conn_get_socket(c));
#endif /* !WITH_BITS */
	    }
	    break;
	    
	case CLIENT_STARTGAME3:
	    if (packet_get_size(packet)<sizeof(t_client_startgame3))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME3 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_startgame3),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *   gamename;
		char const *   gamepass;
		char const *   gameinfo;
		unsigned short bngtype;
		unsigned int   status;
#ifndef WITH_BITS
		t_game *       currgame;
#endif
		
		if (!(gamename = packet_get_str_const(packet,sizeof(t_client_startgame3),GAME_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME3 packet (missing or too long gamename)",conn_get_socket(c));
		    break;
		}
		if (!(gamepass = packet_get_str_const(packet,sizeof(t_client_startgame3)+strlen(gamename)+1,GAME_PASS_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME3 packet (missing or too long gamepass)",conn_get_socket(c));
		    break;
		}
		if (!(gameinfo = packet_get_str_const(packet,sizeof(t_client_startgame3)+strlen(gamename)+1+strlen(gamepass)+1,GAME_INFO_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME3 packet (missing or too long gameinfo)",conn_get_socket(c));
		    break;
		}
		
		bngtype = bn_short_get(packet->u.client_startgame3.gametype);
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got startgame3 status for game \"%s\" is 0x%08x (gametype = 0x%04hx)",conn_get_socket(c),gamename,bn_int_get(packet->u.client_startgame3.status),bngtype);
		status = bn_int_get(packet->u.client_startgame3.status)&CLIENT_STARTGAME3_STATUSMASK;
		
#ifdef WITH_BITS
		bits_game_handle_startgame3(c,gamename,gamepass,gameinfo,bngtype,status);
#else
		if ((currgame = conn_get_game(c)))
		{
		    switch (status)
		    {
		    case CLIENT_STARTGAME3_STATUS_STARTED:
			game_set_status(currgame,game_status_started);
			break;
		    case CLIENT_STARTGAME3_STATUS_FULL:
			game_set_status(currgame,game_status_full);
			break;
		    case CLIENT_STARTGAME3_STATUS_OPEN1:
		    case CLIENT_STARTGAME3_STATUS_OPEN:
			game_set_status(currgame,game_status_open);
			break;
		    case CLIENT_STARTGAME3_STATUS_DONE:
			game_set_status(currgame,game_status_done);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
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
			    
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
			    conn_unget_username(c,tname);
			}
			else
			    conn_set_game(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW3);
			
			if ((rpacket = packet_create(packet_class_bnet)))
			{
			    packet_set_size(rpacket,sizeof(t_server_startgame3_ack));
			    packet_set_type(rpacket,SERVER_STARTGAME3_ACK);
			    
			    if (conn_get_game(c))
				bn_int_set(&rpacket->u.server_startgame3_ack.reply,SERVER_STARTGAME3_ACK_OK);
			    else
				bn_int_set(&rpacket->u.server_startgame3_ack.reply,SERVER_STARTGAME3_ACK_NO);
			    
			    queue_push_packet(conn_get_out_queue(c),rpacket);
			    packet_del_ref(rpacket);
			}
		    }
		    else
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client tried to set game status to DONE",conn_get_socket(c));
#endif /* !WITH_BITS */
	    }
	    break;
	    
	case CLIENT_STARTGAME4:
	    if (packet_get_size(packet)<sizeof(t_client_startgame4))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME4 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_startgame4),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const *   gamename;
		char const *   gamepass;
		char const *   gameinfo;
		unsigned short bngtype;
		unsigned int   status;
		unsigned short option;
#ifndef WITH_BITS
		t_game *       currgame;
#endif
		
		if (!(gamename = packet_get_str_const(packet,sizeof(t_client_startgame4),GAME_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME4 packet (missing or too long gamename)",conn_get_socket(c));
		    break;
		}
		if (!(gamepass = packet_get_str_const(packet,sizeof(t_client_startgame4)+strlen(gamename)+1,GAME_PASS_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME4 packet (missing or too long gamepass)",conn_get_socket(c));
		    break;
		}
		if (!(gameinfo = packet_get_str_const(packet,sizeof(t_client_startgame4)+strlen(gamename)+1+strlen(gamepass)+1,GAME_INFO_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad STARTGAME4 packet (missing or too long gameinfo)",conn_get_socket(c));
		    break;
		}
		
		bngtype = bn_short_get(packet->u.client_startgame4.gametype);
		option = bn_short_get(packet->u.client_startgame4.option);
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got startgame4 status for game \"%s\" is 0x%08x (gametype=0x%04hx option=0x%04x)",conn_get_socket(c),gamename,bn_int_get(packet->u.client_startgame4.status),bngtype,option);
		status = bn_int_get(packet->u.client_startgame4.status)&CLIENT_STARTGAME4_STATUSMASK;
		
#ifdef WITH_BITS
		bits_game_handle_startgame4(c,gamename,gamepass,gameinfo,bngtype,status,option);
#else
		if ((currgame = conn_get_game(c)))
		{
		    switch (status)
		    {
		    case CLIENT_STARTGAME4_STATUS_STARTED:
			game_set_status(currgame,game_status_started);
			break;
		    case CLIENT_STARTGAME4_STATUS_FULL1:
		    case CLIENT_STARTGAME4_STATUS_FULL2:
			game_set_status(currgame,game_status_full);
			break;
		    case CLIENT_STARTGAME4_STATUS_INIT:
		    case CLIENT_STARTGAME4_STATUS_OPEN1:
		    case CLIENT_STARTGAME4_STATUS_OPEN2:
		    case CLIENT_STARTGAME4_STATUS_OPEN3:
			game_set_status(currgame,game_status_open);
			break;
		    case CLIENT_STARTGAME4_STATUS_DONE1:
		    case CLIENT_STARTGAME4_STATUS_DONE2:
			game_set_status(currgame,game_status_done);
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game \"%s\" is finished",conn_get_socket(c),gamename);
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
				
			    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] game start for \"%s\" refused (no authority)",conn_get_socket(c),(tname = conn_get_username(c)));
			    conn_unget_username(c,tname);
			}
			else
			    if (conn_set_game(c,gamename,gamepass,gameinfo,gtype,STARTVER_GW4)==0)
				game_set_option(conn_get_game(c),bngoption_to_goption(conn_get_clienttag(c),gtype,option));
			
			if ((rpacket = packet_create(packet_class_bnet)))
			{
			    packet_set_size(rpacket,sizeof(t_server_startgame4_ack));
			    packet_set_type(rpacket,SERVER_STARTGAME4_ACK);
				
			    if (conn_get_game(c))
				bn_int_set(&rpacket->u.server_startgame4_ack.reply,SERVER_STARTGAME4_ACK_OK);
			    else
				bn_int_set(&rpacket->u.server_startgame4_ack.reply,SERVER_STARTGAME4_ACK_NO);
			    queue_push_packet(conn_get_out_queue(c),rpacket);
			    packet_del_ref(rpacket);
			}
		    }
		    else
			eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client tried to set game status to DONE",conn_get_socket(c));
#endif /* !WITH_BITS */
	    }
	    break;
	    
	case CLIENT_CLOSEGAME:
	    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] client closing game",conn_get_socket(c));
	    conn_set_game(c,NULL,NULL,NULL,game_type_none,0);
	    break;
	    
	case CLIENT_GAME_REPORT:
	    if (packet_get_size(packet)<sizeof(t_client_game_report))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad GAME_REPORT packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_game_report),packet_get_size(packet));
		break;
	    }

#ifdef WITH_BITS
	    if (!bits_master) {
	    	/* send the packet to the master server */
		send_bits_game_report(c,packet);
	    } else
#endif	    
	    {
		t_account *                         my_account;
		t_account *                         other_account;
		t_game_result                       my_result=game_result_none;
		t_game *                            game;
		unsigned int                        player_count;
		unsigned int                        i;
		t_client_game_report_result const * result_data;
		unsigned int                        result_off;
		t_game_result                       result;
		char const *                        player;
		unsigned int                        player_off;
		char const *                        tname;
		
		player_count = bn_int_get(packet->u.client_gamerep.count);
		
		if (!(game = conn_get_game(c)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got GAME_REPORT when not in a game for user \"%s\"",conn_get_socket(c),(tname = conn_get_username(c)));
		    conn_unget_username(c,tname);
		    break;
		}
		
		eventlog(eventlog_level_info,"handle_bnet_packet","[%d] CLIENT_GAME_REPORT: %s (%u players)",conn_get_socket(c),(tname = conn_get_username(c)),player_count);
		my_account = conn_get_account(c);
		
		for (i=0,result_off=sizeof(t_client_game_report),player_off=sizeof(t_client_game_report)+player_count*sizeof(t_client_game_report_result);
		     i<player_count;
		     i++,result_off+=sizeof(t_client_game_report_result),player_off+=strlen(player)+1)
		{
		    if (!(result_data = packet_get_data_const(packet,result_off,sizeof(t_client_game_report_result))))
		    {
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got corrupt GAME_REPORT packet (missing results %u-%u)",conn_get_socket(c),i+1,player_count);
			break;
		    }
		    if (!(player = packet_get_str_const(packet,player_off,USER_NAME_MAX)))
		    {
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got corrupt GAME_REPORT packet (missing players %u-%u)",conn_get_socket(c),i+1,player_count);
			break;
		    }
		    
		    result = bngresult_to_gresult(bn_int_get(result_data->result));
		    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got player %d (\"%s\") result 0x%08x",conn_get_socket(c),i,player,result);
		    
		    if (player[0]=='\0') /* empty slots have empty player name */
			continue;
		    
		    if (!(other_account = accountlist_find_account(player)))
		    {
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got GAME_REPORT with unknown player \"%s\"",conn_get_socket(c),player);
			break;
		    }
		    
		    if (my_account==other_account)
			my_result = result;
		    else
			if (game_check_result(game,other_account,result)<0)
			    break;
		}
		
		conn_unget_username(c,tname);
		
		if (i==player_count) /* if everything checked out... */
		{
		    char const * head;
		    char const * body;
		    
		    if (!(head = packet_get_str_const(packet,player_off,2048)))
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got GAME_REPORT with missing or too long report head",conn_get_socket(c));
		    else
		    {
			player_off += strlen(head)+1;
			if (!(body = packet_get_str_const(packet,player_off,8192)))
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] got GAME_REPORT with missing or too long report body",conn_get_socket(c));
			else
			    game_set_result(game,my_account,my_result,head,body); /* finally we can report the info */
		    }
		}
		
		eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] finished parsing result... now leaving game",conn_get_socket(c));
		conn_set_game(c,NULL,NULL,NULL,game_type_none,0);
	    }
	    break;
	    
	case CLIENT_LEAVECHANNEL:
	    /* If this user in a channel, notify everyone that the user has left */
	    if (conn_get_channel(c))
		conn_set_channel(c,NULL);
	    break;
	    
	case CLIENT_LADDERREQ:
	    if (packet_get_size(packet)<sizeof(t_client_ladderreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LADDERREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_ladderreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		t_ladder_entry entry;
		unsigned int   i;
		unsigned int   type;
		unsigned int   start;
		unsigned int   count;
		unsigned int   idnum;
		t_account *    account;
		char const *   clienttag;
		char const *   tname;
		char const *   timestr;
		t_bnettime     bt;
		t_ladder_id    id;
		
		clienttag = conn_get_clienttag(c);
		
		type = bn_int_get(packet->u.client_ladderreq.type);
		start = bn_int_get(packet->u.client_ladderreq.startplace);
		count = bn_int_get(packet->u.client_ladderreq.count);
		idnum = bn_int_get(packet->u.client_ladderreq.id);
		
		/* eventlog(eventlog_level_debug,"handle_bnet_packet","got LADDERREQ type=%u start=%u count=%u id=%u",type,start,count,id); */
		
		switch (idnum)
		{
		case CLIENT_LADDERREQ_ID_STANDARD:
		    id = ladder_id_normal;
		    break;
		case CLIENT_LADDERREQ_ID_IRONMAN:
		    id = ladder_id_ironman;
		    break;
		default:
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got unknown ladder ladderreq.id=0x%08x",conn_get_socket(c),idnum);
		    id = ladder_id_normal;
		}
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_ladderreply));
		packet_set_type(rpacket,SERVER_LADDERREPLY);
		
		bn_int_tag_set(&rpacket->u.server_ladderreply.clienttag,clienttag);
		bn_int_set(&rpacket->u.server_ladderreply.id,idnum);
		bn_int_set(&rpacket->u.server_ladderreply.type,type);
		bn_int_set(&rpacket->u.server_ladderreply.startplace,start);
		bn_int_set(&rpacket->u.server_ladderreply.count,count);
		
		for (i=start; i<start+count; i++)
		{
		    switch (type)
		    {
		    case CLIENT_LADDERREQ_TYPE_HIGHESTRATED:
			if (!(account = ladder_get_account_by_rank(i+1,ladder_sort_highestrated,ladder_time_active,clienttag,id)))
			    account = ladder_get_account_by_rank(i+1,ladder_sort_highestrated,ladder_time_current,clienttag,id);
			break;
		    case CLIENT_LADDERREQ_TYPE_MOSTWINS:
			if (!(account = ladder_get_account_by_rank(i+1,ladder_sort_mostwins,ladder_time_active,clienttag,id)))
			    account = ladder_get_account_by_rank(i+1,ladder_sort_mostwins,ladder_time_current,clienttag,id);
			break;
		    case CLIENT_LADDERREQ_TYPE_MOSTGAMES:
			if (!(account = ladder_get_account_by_rank(i+1,ladder_sort_mostgames,ladder_time_active,clienttag,id)))
			    account = ladder_get_account_by_rank(i+1,ladder_sort_mostgames,ladder_time_current,clienttag,id);
			break;
		    default:
			account = NULL;
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got unknown value for ladderreq.type=%u",conn_get_socket(c),type);
		    }
		    
		    if (account)
		    {
			bn_int_set(&entry.active.wins,account_get_ladder_active_wins(account,clienttag,id));
			bn_int_set(&entry.active.loss,account_get_ladder_active_losses(account,clienttag,id));
			bn_int_set(&entry.active.disconnect,account_get_ladder_active_disconnects(account,clienttag,id));
			bn_int_set(&entry.active.rating,account_get_ladder_active_rating(account,clienttag,id));
			bn_int_set(&entry.active.unknown,10); /* FIXME: rank,draws,?! */
			if (!(timestr = account_get_ladder_active_last_time(account,clienttag,id)))
			    timestr = BNETD_LADDER_DEFAULT_TIME;
			bnettime_set_str(&bt,timestr);
			bnettime_to_bn_long(bt,&entry.lastgame_active);
			
			bn_int_set(&entry.current.wins,account_get_ladder_wins(account,clienttag,id));
			bn_int_set(&entry.current.loss,account_get_ladder_losses(account,clienttag,id));
			bn_int_set(&entry.current.disconnect,account_get_ladder_disconnects(account,clienttag,id));
			bn_int_set(&entry.current.rating,account_get_ladder_rating(account,clienttag,id));
			bn_int_set(&entry.current.unknown,5); /* FIXME: rank,draws,?! */
			if (!(timestr = account_get_ladder_last_time(account,clienttag,id)))
			    timestr = BNETD_LADDER_DEFAULT_TIME;
			bnettime_set_str(&bt,timestr);
			bnettime_to_bn_long(bt,&entry.lastgame_current);
		    }
		    else
		    {
			bn_int_set(&entry.active.wins,0);
			bn_int_set(&entry.active.loss,0);
			bn_int_set(&entry.active.disconnect,0);
			bn_int_set(&entry.active.rating,0);
			bn_int_set(&entry.active.unknown,0);
			bn_long_set_a_b(&entry.lastgame_active,0,0);
			
			bn_int_set(&entry.current.wins,0);
			bn_int_set(&entry.current.loss,0);
			bn_int_set(&entry.current.disconnect,0);
			bn_int_set(&entry.current.rating,0);
			bn_int_set(&entry.current.unknown,0);
			bn_long_set_a_b(&entry.lastgame_current,0,0);
		    }
		    
		    bn_int_set(&entry.ttest[0],0); /* FIXME: what is ttest? */
		    bn_int_set(&entry.ttest[1],0);
		    bn_int_set(&entry.ttest[2],0);
		    bn_int_set(&entry.ttest[3],0);
		    bn_int_set(&entry.ttest[4],0);
		    bn_int_set(&entry.ttest[5],0);
		    
		    packet_append_data(rpacket,&entry,sizeof(entry));
		    
		    if (account)
		    {
			packet_append_string(rpacket,(tname = account_get_name(account)));
			account_unget_name(tname);
		    }
		    else
			packet_append_string(rpacket," "); /* use a space so the client won't show the user's own account when double-clicked on */
		}
		
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_LADDERSEARCHREQ:
	    if (packet_get_size(packet)<sizeof(t_client_laddersearchreq))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LADDERSEARCHREQ packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_laddersearchreq),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * playername;
		t_account *  account;
		unsigned int idnum;
		unsigned int rank; /* starts at zero */
		t_ladder_id  id;
		
		idnum = bn_int_get(packet->u.client_laddersearchreq.id);
		
		switch (idnum)
		{
		case CLIENT_LADDERREQ_ID_STANDARD:
		    id = ladder_id_normal;
		    break;
		case CLIENT_LADDERREQ_ID_IRONMAN:
		    id = ladder_id_ironman;
		    break;
		default:
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got unknown ladder laddersearchreq.id=0x%08x",conn_get_socket(c),idnum);
		    id = ladder_id_normal;
		}
		
		if (!(playername = packet_get_str_const(packet,sizeof(t_client_laddersearchreq),USER_NAME_MAX)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad LADDERSEARCHREQ packet (missing or too long playername)",conn_get_socket(c));
		    break;
		}
		
		if (!(account = accountlist_find_account(playername)))
		    rank = SERVER_LADDERSEARCHREPLY_RANK_NONE;
		else
		{
		    switch (bn_int_get(packet->u.client_laddersearchreq.type))
		    {
		    case CLIENT_LADDERSEARCHREQ_TYPE_HIGHESTRATED:
			if ((rank = ladder_get_rank_by_account(account,ladder_sort_highestrated,ladder_time_active,conn_get_clienttag(c),id))==0)
			{
			    rank = ladder_get_rank_by_account(account,ladder_sort_highestrated,ladder_time_current,conn_get_clienttag(c),id);
			    if (ladder_get_account_by_rank(rank,ladder_sort_highestrated,ladder_time_active,conn_get_clienttag(c),id))
				rank = 0;
			}
			break;
		    case CLIENT_LADDERSEARCHREQ_TYPE_MOSTWINS:
			if ((rank = ladder_get_rank_by_account(account,ladder_sort_mostwins,ladder_time_active,conn_get_clienttag(c),id))==0)
			{
			    rank = ladder_get_rank_by_account(account,ladder_sort_mostwins,ladder_time_current,conn_get_clienttag(c),id);
			    if (ladder_get_account_by_rank(rank,ladder_sort_mostwins,ladder_time_active,conn_get_clienttag(c),id))
				rank = 0;
			}
			break;
		    case CLIENT_LADDERSEARCHREQ_TYPE_MOSTGAMES:
			if ((rank = ladder_get_rank_by_account(account,ladder_sort_mostgames,ladder_time_active,conn_get_clienttag(c),id))==0)
			{
			    rank = ladder_get_rank_by_account(account,ladder_sort_mostgames,ladder_time_current,conn_get_clienttag(c),id);
			    if (ladder_get_account_by_rank(rank,ladder_sort_mostgames,ladder_time_active,conn_get_clienttag(c),id))
				rank = 0;
			}
			break;
		    default:
			rank = 0;
			eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got unknown ladder search type %u",conn_get_socket(c),bn_int_get(packet->u.client_laddersearchreq.type));
		    }
		    
		    if (rank==0)
			rank = SERVER_LADDERSEARCHREPLY_RANK_NONE;
		    else
			rank--;
		}
		
		if (!(rpacket = packet_create(packet_class_bnet)))
		    break;
		packet_set_size(rpacket,sizeof(t_server_laddersearchreply));
		packet_set_type(rpacket,SERVER_LADDERSEARCHREPLY);
		bn_int_set(&rpacket->u.server_laddersearchreply.rank,rank);
		queue_push_packet(conn_get_out_queue(c),rpacket);
		packet_del_ref(rpacket);
	    }
	    break;
	    
	case CLIENT_MAPAUTHREQ1:
	    if (packet_get_size(packet)<sizeof(t_client_mapauthreq1))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MAPAUTHREQ1 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_mapauthreq1),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * mapname;
		t_game *     game;
		    
		if (!(mapname = packet_get_str_const(packet,sizeof(t_client_mapauthreq1),MAP_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MAPAUTHREQ1 packet (missing or too long mapname)",conn_get_socket(c));
		    break;
		}
		
#ifndef WITH_BITS
		game = conn_get_game(c);
#else
		game = bits_game_create_temp(conn_get_bits_game(c));
		/* Hope this works --- Typhoon */
#endif
		
		if (game)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] map auth requested for map \"%s\" gametype \"%s\"",conn_get_socket(c),mapname,game_type_get_str(game_get_type(game)));
		    game_set_mapname(game,mapname);
		}
		else
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] map auth requested when not in a game",conn_get_socket(c));
		
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    unsigned int val;
			
		    if (!game)
		    {
			val = SERVER_MAPAUTHREPLY1_NO;
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] map authorization denied (not in a game)",conn_get_socket(c));
		    }
		    else
			if (strcasecmp(game_get_mapname(game),mapname)!=0)
			{
			    val = SERVER_MAPAUTHREPLY1_NO;
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] map authorization denied (map name \"%s\" does not match game map name \"%s\")",conn_get_socket(c),mapname,game_get_mapname(game));
			}
			else
			{
			    game_set_status(game,game_status_started);
			    
			    if (game_get_type(game)==game_type_ladder)
			    {
				val = SERVER_MAPAUTHREPLY1_LADDER_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map ladder authorization (in a ladder game)",conn_get_socket(c));
			    }
			    else if (ladder_check_map(game_get_mapname(game),game_get_maptype(game),conn_get_clienttag(c)))
			    {
				val = SERVER_MAPAUTHREPLY1_LADDER_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map ladder authorization (is a ladder map)",conn_get_socket(c));
			    }
			    else
			    {
				val = SERVER_MAPAUTHREPLY1_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map normal authorization",conn_get_socket(c));
			    }
			}
			
		    packet_set_size(rpacket,sizeof(t_server_mapauthreply1));
		    packet_set_type(rpacket,SERVER_MAPAUTHREPLY1);
		    bn_int_set(&rpacket->u.server_mapauthreply1.response,val);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
#ifdef WITH_BITS
		if (game) bits_game_destroy_temp(game);
#endif		
	    }
	    break;
		
	case CLIENT_MAPAUTHREQ2:
	    if (packet_get_size(packet)<sizeof(t_client_mapauthreq2))
	    {
		eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MAPAUTHREQ2 packet (expected %u bytes, got %u)",conn_get_socket(c),sizeof(t_client_mapauthreq2),packet_get_size(packet));
		break;
	    }
	    
	    {
		char const * mapname;
		t_game *     game;
		    
		if (!(mapname = packet_get_str_const(packet,sizeof(t_client_mapauthreq2),MAP_NAME_LEN)))
		{
		    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] got bad MAPAUTHREQ2 packet (missing or too long mapname)",conn_get_socket(c));
		    break;
		}
		
#ifndef WITH_BITS
		game = conn_get_game(c);
#else
		game = bits_game_create_temp(conn_get_bits_game(c));
		/* Hope this works --- Typhoon */
#endif
		
		if (game)
		{
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] map auth requested for map \"%s\" gametype \"%s\"",conn_get_socket(c),mapname,game_type_get_str(game_get_type(game)));
		    game_set_mapname(game,mapname);
		}
		else
		    eventlog(eventlog_level_info,"handle_bnet_packet","[%d] map auth requested when not in a game",conn_get_socket(c));
		
		if ((rpacket = packet_create(packet_class_bnet)))
		{
		    unsigned int val;
			
		    if (!game)
		    {
			val = SERVER_MAPAUTHREPLY2_NO;
			eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] map authorization denied (not in a game)",conn_get_socket(c));
		    }
		    else
			if (strcasecmp(game_get_mapname(game),mapname)!=0)
			{
			    val = SERVER_MAPAUTHREPLY2_NO;
			    eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] map authorization denied (map name \"%s\" does not match game map name \"%s\")",conn_get_socket(c),mapname,game_get_mapname(game));
			}
			else
			{
			    game_set_status(game,game_status_started);
			    
			    if (game_get_type(game)==game_type_ladder)
			    {
				val = SERVER_MAPAUTHREPLY2_LADDER_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map ladder authorization (in a ladder game)",conn_get_socket(c));
			    }
			    else if (ladder_check_map(game_get_mapname(game),game_get_maptype(game),conn_get_clienttag(c)))
			    {
				val = SERVER_MAPAUTHREPLY2_LADDER_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map ladder authorization (is a ladder map)",conn_get_socket(c));
			    }
			    else
			    {
				val = SERVER_MAPAUTHREPLY2_OK;
				eventlog(eventlog_level_debug,"handle_bnet_packet","[%d] giving map normal authorization",conn_get_socket(c));
			    }
			}
			
		    packet_set_size(rpacket,sizeof(t_server_mapauthreply2));
		    packet_set_type(rpacket,SERVER_MAPAUTHREPLY2);
		    bn_int_set(&rpacket->u.server_mapauthreply2.response,val);
		    queue_push_packet(conn_get_out_queue(c),rpacket);
		    packet_del_ref(rpacket);
		}
#ifdef WITH_BITS
		if (game) bits_game_destroy_temp(game);
#endif		
	    }
	    break;
		
	default:
	    eventlog(eventlog_level_error,"handle_bnet_packet","[%d] unknown (logged in) bnet packet type 0x%04hx, len %u",conn_get_socket(c),packet_get_type(packet),packet_get_size(packet));
	}
	break;
	    
    default:
	eventlog(eventlog_level_error,"handle_bnet_packet","[%d] invalid login state %d",conn_get_socket(c),conn_get_state(c));
    }
    
    return 0;
}

/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Some BITS modifications:
 *          Copyright (C) 1999,2000,2001  Marco Ziech (mmz@gmx.net)
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
#ifndef INCLUDED_PACKET_TYPES
#define INCLUDED_PACKET_TYPES

#ifdef JUST_NEED_TYPES
# include "common/field_sizes.h"
# include "common/init_protocol.h"
# include "common/bnet_protocol.h"
# include "common/file_protocol.h"
# include "common/bot_protocol.h"
# include "common/bits_protocol.h"
# include "common/udp_protocol.h"
# include "common/d2game_protocol.h"
# include "d2cs/d2cs_protocol.h"
# include "d2cs/d2cs_d2gs_protocol.h"
# include "d2cs/d2cs_bnetd_protocol.h"
# include "common/auth_protocol.h"
#else
# define JUST_NEED_TYPES
# include "common/field_sizes.h"
# include "common/init_protocol.h"
# include "common/bnet_protocol.h"
# include "common/file_protocol.h"
# include "common/bot_protocol.h"
# include "common/bits_protocol.h"
# include "common/udp_protocol.h"
# include "common/d2game_protocol.h"
# include "d2cs/d2cs_protocol.h"
# include "d2cs/d2cs_d2gs_protocol.h"
# include "d2cs/d2cs_bnetd_protocol.h"
# include "common/auth_protocol.h"
# undef JUST_NEED_TYPES
#endif

typedef enum
{
    packet_class_none,
    packet_class_init,
    packet_class_bnet,
    packet_class_file,
    packet_class_raw,
    packet_class_bits,
    packet_class_udp,
    packet_class_d2game,
    packet_class_d2gs,
    packet_class_d2cs,
    packet_class_d2cs_bnetd,
    packet_class_auth
} t_packet_class;


typedef enum
{
    packet_dir_from_client,
    packet_dir_from_server
} t_packet_dir;


/* These aren't really packets so much as records in a TCP stream. They are variable-
 * length structures which make up the Battle.net protocol. It is just easier to call
 * them "packets".
 */
typedef struct
{
    unsigned int   ref;   /* reference count */
    t_packet_class class;
    unsigned int   flags; /* user-defined flags (used to mark UDP in bnproxy) */
    unsigned int   len;   /* raw packets have no header, so we use this */
    
    /* next part looks just like it would on the network (no padding, byte for byte) */
    union
    {
        char data[MAX_PACKET_SIZE];
        
        t_bnet_generic   bnet;
        t_file_generic   file;
	t_bits_generic   bits;
        t_udp_generic    udp;
        t_d2game_generic d2game;
	t_auth_generic   auth;
        
	t_client_initconn client_initconn;
	
        t_server_authreply1         server_authreply1;
        t_server_authreq1           server_authreq1;
        t_client_authreq1           client_authreq1;
        t_server_authreply_109      server_authreply_109;
        t_server_authreq_109        server_authreq_109;
        t_client_authreq_109        client_authreq_109;
        t_client_cdkey              client_cdkey;
        t_server_cdkeyreply         server_cdkeyreply;
        t_client_statsreq           client_statsreq;
        t_server_statsreply         server_statsreply;
        t_client_loginreq1          client_loginreq1;
        t_server_loginreply1        server_loginreply1;
        t_client_progident          client_progident;
        t_client_progident2         client_progident2;
        t_client_joinchannel        client_joinchannel;
        t_server_channellist        server_channellist;
        t_server_serverlist         server_serverlist;
        t_server_message            server_message;
        t_client_message            client_message;
        t_client_gamelistreq        client_gamelistreq;
        t_server_gamelistreply      server_gamelistreply;
        t_client_udpok              client_udpok;
        t_client_unknown_1b         client_unknown_1b;
        t_client_startgame1         client_startgame1;
        t_server_startgame1_ack     server_startgame1_ack;
        t_client_startgame3         client_startgame3;
        t_server_startgame3_ack     server_startgame3_ack;
        t_client_startgame4         client_startgame4;
        t_server_startgame4_ack     server_startgame4_ack;
        t_client_leavechannel       client_leavechannel;
        t_client_closegame          client_closegame;
        t_client_mapauthreq1        client_mapauthreq1;
        t_server_mapauthreply1      server_mapauthreply1;
        t_client_mapauthreq2        client_mapauthreq2;
        t_server_mapauthreply2      server_mapauthreply2;
        t_client_ladderreq          client_ladderreq;
        t_server_ladderreply        server_ladderreply;
	t_client_laddersearchreq    client_laddersearchreq;
	t_server_laddersearchreply  server_laddersearchreply;
        t_client_adreq              client_adreq;
        t_server_adreply            server_adreply;
        t_client_adack              client_adack;
	t_client_adclick            client_adclick;
	t_client_adclick2           client_adclick2;
	t_server_adclickreply2      server_adclickreply2;
        t_client_game_report        client_gamerep;
        t_server_sessionkey1        server_sessionkey1;
        t_server_sessionkey2        server_sessionkey2;
        t_client_createacctreq1     client_createacctreq1;
        t_server_createacctreply1   server_createacctreply1;
        t_client_changepassreq      client_changepassreq;
        t_server_changepassack      server_changepassack;
        t_client_iconreq            client_iconreq;
        t_server_iconreply          server_iconreply;
        t_client_tosreq             client_tosreq;
        t_server_tosreply           server_tosreply;
        t_client_statsupdate        client_statsupdate;
        t_client_countryinfo1       client_countryinfo1;
        t_client_countryinfo_109    client_countryinfo_109;
        t_client_unknown_2b         client_unknown_2b;
        t_client_compinfo1          client_compinfo1;
        t_client_compinfo2          client_compinfo2;
        t_server_compreply          server_compreply;
        t_server_echoreq            server_echoreq;
        t_client_echoreply          client_echoreply;
        t_client_playerinforeq      client_playerinforeq;
        t_server_playerinforeply    server_playerinforeply;
	t_client_pingreq            client_pingreq;
	t_server_pingreply          server_pingreply;
        t_client_cdkey2             client_cdkey2;
        t_server_cdkeyreply2        server_cdkeyreply2;
        t_client_cdkey3             client_cdkey3;
        t_server_cdkeyreply3        server_cdkeyreply3;
        t_server_regsnoopreq        server_regsnoopreq;
        t_client_regsnoopreply      client_regsnoopreply;
        t_client_realmlistreq       client_realmlistreq;
        t_server_realmlistreply     server_realmlistreply;
        t_client_realmjoinreq       client_realmjoinreq;
        t_server_realmjoinreply     server_realmjoinreply;
        t_client_realmjoinreq_109   client_realmjoinreq_109;
        t_server_realmjoinreply_109 server_realmjoinreply_109;
        t_client_unknown_37         client_unknown_37;
        t_server_unknown_37         server_unknown_37;
        t_client_unknown_39         client_unknown_39;
        t_client_loginreq2          client_loginreq2;
        t_server_loginreply2        server_loginreply2;
	t_client_createacctreq2     client_createacctreq2;
	t_server_createacctreply2   server_createacctreply2;
	
	t_client_file_req          client_file_req;
	t_server_file_reply        server_file_reply;
	
	t_server_udptest           server_udptest;
	t_client_udpping           client_udpping;
	t_client_sessionaddr1      client_sessionaddr1;
	t_client_sessionaddr2      client_sessionaddr2;
	
	t_bits_session_request     bits_session_request;
	t_bits_session_reply       bits_session_reply;
        
	t_bits_auth_request        bits_auth_request;
	t_bits_auth_reply          bits_auth_reply;
        
	t_bits_va_create_req         bits_va_create_req;
	t_bits_va_create_reply       bits_va_create_reply;
	t_bits_va_set_attr           bits_va_set_attr;
	t_bits_va_get_attr           bits_va_get_attr;
	t_bits_va_attr               bits_va_attr;
	t_bits_va_get_allattr        bits_va_get_allattr;
	t_bits_va_allattr            bits_va_allattr;
	t_bits_va_lock               bits_va_lock;
	t_bits_va_lock_ack           bits_va_lock_ack;
	t_bits_va_unlock             bits_va_unlock;
	t_bits_va_loginreq           bits_va_loginreq;
	t_bits_va_loginreply         bits_va_loginreply;
	t_bits_va_logout             bits_va_logout;
	t_bits_va_login              bits_va_login;
	t_bits_va_update_playerinfo  bits_va_update_playerinfo;
        
	t_bits_net_discover        bits_net_discover;
	t_bits_net_undiscover      bits_net_undiscover;
	t_bits_net_ping            bits_net_ping;
	t_bits_net_pong            bits_net_pong;
	t_bits_net_motd            bits_net_motd;
        
        t_bits_chat_user                      bits_chat_user;
        t_bits_chat_channellist_add           bits_chat_channellist_add;
        t_bits_chat_channellist_del           bits_chat_channellist_del;
        t_bits_chat_channel_join_request      bits_chat_channel_join_request;
        t_bits_chat_channel_join_new_request  bits_chat_channel_join_new_request;
        t_bits_chat_channel_join_perm_request bits_chat_channel_join_perm_request;
        t_bits_chat_channel_join_reply        bits_chat_channel_join_reply;
        t_bits_chat_channel_leave             bits_chat_channel_leave;
        t_bits_chat_channel_join              bits_chat_channel_join;
        t_bits_chat_channel_server_join       bits_chat_channel_server_join;
        t_bits_chat_channel_server_leave      bits_chat_channel_server_leave;
        t_bits_chat_channel                   bits_chat_channel;

	t_bits_gamelist_add        bits_gamelist_add;
	t_bits_gamelist_del        bits_gamelist_del;
	t_bits_game_update         bits_game_update;
	t_bits_game_join_request   bits_game_join_request;
	t_bits_game_join_reply     bits_game_join_reply;
	t_bits_game_leave          bits_game_leave;
	t_bits_game_create_request bits_game_create_request;
	t_bits_game_create_reply   bits_game_create_reply;
	t_bits_game_report	   bits_game_report;
	
	t_client_authloginreq      client_authloginreq;
	t_server_authloginreply    server_authloginreply;
	t_client_createcharreq     client_createcharreq;
	t_server_createcharreply   server_createcharreply;
	t_client_creategamereq     client_creategamereq;
	t_server_creategamereply   server_creategamereply;
	t_client_joingamereq2      client_joingamereq2;
	t_server_joingamereply2    server_joingamereply2;
	t_server_creategame_wait   server_creategame_wait;
	t_client_cancel_create     client_cancel_create;
	t_client_d2gamelistreq     client_d2gamelistreq;
	t_server_d2gamelistreply   server_d2gamelistreply;
	t_client_gameinforeq       client_gameinforeq;
	t_server_gameinforeply     server_gameinforeply;
	t_client_charloginreq      client_charloginreq;
	t_server_charloginreply    server_charloginreply;
	t_client_deletecharreq     client_deletecharreq;
	t_server_deletecharreply   server_deletecharreply;
	t_client_ladderreq2        client_ladderreq2;
	t_server_ladderreply2      server_ladderreply2;
	t_client_authmotdreq       client_authmotdreq;
	t_server_authmotdreply     server_authmotdreply;
	t_client_charlistreq       client_charlistreq;
	t_server_charlistreply     server_charlistreply;
	t_client_convertcharreq    client_convertcharreq;
	t_server_convertcharreply  server_convertcharreply;
        t_d2cs_bnetd_generic            d2cs_bnetd;
        t_bnetd_d2cs_authreq            bnetd_d2cs_authreq;
        t_d2cs_bnetd_authreply          d2cs_bnetd_authreply;
        t_bnetd_d2cs_authreply          bnetd_d2cs_authreply;
        t_d2cs_bnetd_accountloginreq    d2cs_bnetd_accountloginreq;
        t_bnetd_d2cs_accountloginreply  bnetd_d2cs_accountloginreply;
        t_d2cs_bnetd_charloginreq       d2cs_bnetd_charloginreq;
        t_bnetd_d2cs_charloginreply     bnetd_d2cs_charloginreply;

        t_d2cs_d2gs_generic             d2cs_d2gs;
        t_d2cs_d2gs_authreq             d2cs_d2gs_authreq;
        t_d2gs_d2cs_authreply           d2gs_d2cs_authreply;
        t_d2cs_d2gs_authreply           d2cs_d2gs_authreply;
        t_d2gs_d2cs_setgsinfo           d2gs_d2cs_setgsinfo;
        t_d2cs_d2gs_creategamereq       d2cs_d2gs_creategamereq;
        t_d2gs_d2cs_creategamereply     d2gs_d2cs_creategamereply;
        t_d2cs_d2gs_joingamereq         d2cs_d2gs_joingamereq;
        t_d2gs_d2cs_joingamereply       d2gs_d2cs_joingamereply;
        t_d2gs_d2cs_updategameinfo      d2gs_d2cs_updategameinfo;
        t_d2gs_d2cs_closegame           d2gs_d2cs_closegame;
        t_d2cs_d2gs_echoreq             d2cs_d2gs_echoreq;
        t_d2gs_d2cs_echoreply           d2gs_d2cs_echoreply;

        t_d2cs_client_generic           d2cs_client;
        t_client_d2cs_loginreq          client_d2cs_loginreq;
        t_d2cs_client_loginreply        d2cs_client_loginreply;
        t_client_d2cs_createcharreq     client_d2cs_createcharreq;
        t_d2cs_client_createcharreply   d2cs_client_createcharreply;
        t_client_d2cs_creategamereq     client_d2cs_creategamereq;
        t_d2cs_client_creategamereply   d2cs_client_creategamereply;
        t_client_d2cs_joingamereq       client_d2cs_joingamereq;
        t_d2cs_client_joingamereply     d2cs_client_joingamereply;
        t_client_d2cs_gamelistreq       client_d2cs_gamelistreq;
        t_d2cs_client_gamelistreply     d2cs_client_gamelistreply;
        t_client_d2cs_gameinforeq       client_d2cs_gameinforeq;
        t_d2cs_client_gameinforeply     d2cs_client_gameinforeply;
        t_client_d2cs_charloginreq      client_d2cs_charloginreq;
        t_d2cs_client_charloginreply    d2cs_client_charloginreply;
        t_client_d2cs_deletecharreq     client_d2cs_deletecharreq;
        t_d2cs_client_deletecharreply   d2cs_client_deletecharreply;
        t_client_d2cs_ladderreq         client_d2cs_ladderreq;
        t_d2cs_client_ladderreply       d2cs_client_ladderreply;
        t_client_d2cs_motdreq           client_d2cs_motdreq;
        t_d2cs_client_motdreply         d2cs_client_motdreply;
        t_client_d2cs_cancelcreategame  client_d2cs_cancelcreategame;
        t_d2cs_client_creategamewait    d2cs_client_creategamewait;
        t_client_d2cs_charladderreq     client_d2cs_charladderreq;
        t_client_d2cs_charlistreq       client_d2cs_charlistreq;
        t_d2cs_client_charlistreply     d2cs_client_charlistreply;
        t_client_d2cs_convertcharreq    client_d2cs_convertcharreq;
        t_d2cs_client_convertcharreply  d2cs_client_convertcharreply;
    } u;
} t_packet;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_PACKET_PROTOS
#define INCLUDED_PACKET_PROTOS

#ifdef USE_CHECK_ALLOC
extern t_packet * packet_create_real(t_packet_class class, char const * fn, unsigned int ln) MALLOC_ATTR();
# define packet_create(C) packet_create_real(C,__FILE__"{packet_create}",__LINE__)
#else
extern t_packet * packet_create(t_packet_class class) MALLOC_ATTR();
#endif
extern void packet_destroy(t_packet const * packet);
extern t_packet * packet_add_ref(t_packet * packet);
extern void packet_del_ref(t_packet * packet);
extern t_packet_class packet_get_class(t_packet const * packet);
extern char const * packet_get_class_str(t_packet const * packet);
extern int packet_set_class(t_packet * packet, t_packet_class class);
extern unsigned int packet_get_type(t_packet const * packet);
extern char const * packet_get_type_str(t_packet const * packet, t_packet_dir dir);
extern int packet_set_type(t_packet * packet, unsigned int type);
extern unsigned int packet_get_size(t_packet const * packet);
extern int packet_set_size(t_packet * packet, unsigned int size);
extern unsigned int packet_get_header_size(t_packet const * packet);
extern unsigned int packet_get_flags(t_packet const * packet);
extern int packet_set_flags(t_packet * packet, unsigned int flags);
extern int packet_append_string(t_packet * packet, char const * str);
extern int packet_append_ntstring(t_packet * packet, char const * str);
extern int packet_append_data(t_packet * packet, void const * data, unsigned int len);
extern void const * packet_get_raw_data_const(t_packet const * packet, unsigned int offset);
extern void * packet_get_raw_data(t_packet * packet, unsigned int offset);
extern void * packet_get_raw_data_build(t_packet * packet, unsigned int offset);
extern char const * packet_get_str_const(t_packet const * packet, unsigned int offset, unsigned int maxlen);
extern void const * packet_get_data_const(t_packet const * packet, unsigned int offset, unsigned int len);
extern t_packet * packet_duplicate(t_packet const * src);

#endif
#endif

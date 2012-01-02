/*
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_CONNECTION_TYPES
#define INCLUDED_CONNECTION_TYPES

#ifdef CONNECTION_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "game.h"
# include "common/queue.h"
# include "channel.h"
# include "account.h"
# include "quota.h"
# include "character.h"
# include "versioncheck.h"
# ifdef WITH_BITS
#   include "bits.h"
#   include "bits_ext.h"
# endif
#else
# define JUST_NEED_TYPES
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "game.h"
# include "common/queue.h"
# include "channel.h"
# include "account.h"
# include "quota.h"
# include "character.h"
# include "versioncheck.h"
# ifdef WITH_BITS
#   include "bits.h"
#   include "bits_ext.h"
# endif
# undef JUST_NEED_TYPES
#endif

#endif

#ifdef CONNECTION_INTERNAL_ACCESS
typedef struct
{
    char const *  away;
    char const *  dnd;
    t_account * * ignore_list;
    unsigned int  ignore_count;
    t_quota	  quota;
} t_usersettings;
#endif

typedef enum
{
    conn_class_init,
    conn_class_defer,
    conn_class_bnet,
    conn_class_file,
    conn_class_bot,
    conn_class_bits,
    conn_class_remote,	/* Connection on a remote bits server */
    conn_class_auth,
    conn_class_telnet,
    conn_class_irc,     /* Internet Relay Chat */
    conn_class_d2cs_bnetd,
    conn_class_none
} t_conn_class;

typedef enum
{
    conn_state_empty,
    conn_state_initial,
    conn_state_connected,
    conn_state_loggedin,
    conn_state_destroy,
    conn_state_bot_username,
    conn_state_bot_password
} t_conn_state;

typedef struct connection
#ifdef CONNECTION_INTERNAL_ACCESS
{
    int                           tcp_sock;
    unsigned int                  tcp_addr;
    unsigned short                tcp_port;
    int                           udp_sock;
    unsigned int                  udp_addr;
    unsigned short                udp_port;
    unsigned int                  local_addr;
    unsigned short                local_port;
    unsigned int                  real_local_addr;
    unsigned short                real_local_port;
    t_conn_class                  class;
    t_conn_state                  state;
    unsigned int                  sessionkey;
    unsigned int                  sessionnum;
    unsigned int                  secret; /* random number... never sent over net unencrypted */
    unsigned int                  flags;
    unsigned int                  latency;
    t_usersettings                settings;
    char const *                  archtag;
    char const *                  clienttag;
    char const *                  clientver;
    unsigned long                 versionid; /* AKA bnversion */
    char const *                  country;
    int                           tzbias;
    t_account *                   account;
    t_channel *                   channel;
    t_game *                      game;
    t_queue *                     outqueue;  /* packets waiting to be sent */
    unsigned int                  outsize;   /* amount sent from the current output packet */
    t_queue *                     inqueue;   /* packet waiting to be processed */
    unsigned int                  insize;    /* amount received into the current input packet */
    int                           welcomed;  /* 1 = sent welcome message, 0 = have not */
    char const *                  host;
    char const *                  user;
    char const *                  clientexe;
    char const *                  owner;
    char const *                  cdkey;
    char const *                  botuser;   /* username for remote connections (not taken from account) */
    time_t                        last_message;
    char const *                  realmname; /* to remember until character is created */
    t_character *                 character;
    struct connection *           bound; /* matching Diablo II auth connection */
    char const *                  lastsender; /* last person to whisper to this connection */
    char const *                  realminfo;
    char const *                  charname;
    t_versioncheck const *        versioncheck; /* equation and MPQ file used to validate game checksum */
    char const *	          ircline; /* line cache for IRC connections */
    unsigned int		  ircping; /* value of last ping */
    char const *		  ircpass; /* hashed password for PASS authentication */
#ifdef WITH_BITS
    unsigned int		  bits_game; /* game is always NULL, this is used instead (0==no game)*/
    unsigned int                  sessionid; /* unique sessionid for this connection on the network */
    t_bits_connection_extension	* bits;      /* extended connection info for bits connections (conn_class_bits) */
#endif
    int                           udpok;     /* udp packets can be received by client */
}
#endif
t_connection;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_CONNECTION_PROTOS
#define INCLUDED_CONNECTION_PROTOS

#define JUST_NEED_TYPES
#include "common/packet.h"
#include "common/queue.h"
#include "channel.h"
#include "game.h"
#include "account.h"
#include "common/list.h"
#include "character.h"
#include "versioncheck.h"
#include "timer.h"
#undef JUST_NEED_TYPES

extern void conn_test_latency(t_connection * c, time_t now, t_timer_data delta);
extern char const * conn_class_get_str(t_conn_class class) CONST_ATTR();
extern char const * conn_state_get_str(t_conn_state state) CONST_ATTR();

extern t_connection * conn_create(int tsock, int usock, unsigned int real_local_addr, unsigned short real_local_port, unsigned int local_addr, unsigned short local_port, unsigned int addr, unsigned short port) MALLOC_ATTR();
extern void conn_destroy(t_connection * c);
extern int conn_match(t_connection const * c, char const * user);
extern t_conn_class conn_get_class(t_connection const * c) PURE_ATTR();
extern void conn_set_class(t_connection * c, t_conn_class class);
extern t_conn_state conn_get_state(t_connection const * c) PURE_ATTR();
extern void conn_set_state(t_connection * c, t_conn_state state);
#ifdef WITH_BITS
extern void conn_set_sessionid(t_connection * c, unsigned int sessionid);
extern unsigned int conn_get_sessionid(t_connection const * c);
extern int conn_set_bits_game(t_connection * c, unsigned int gameid);
extern unsigned int conn_get_bits_game(t_connection const * c);
#endif
extern unsigned int conn_get_sessionkey(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_sessionnum(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_secret(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_addr(t_connection const * c) PURE_ATTR();
extern unsigned short conn_get_port(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_local_addr(t_connection const * c);
extern unsigned short conn_get_local_port(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_real_local_addr(t_connection const * c) PURE_ATTR();
extern unsigned short conn_get_real_local_port(t_connection const * c) PURE_ATTR();
extern unsigned int conn_get_game_addr(t_connection const * c) PURE_ATTR();
extern int conn_set_game_addr(t_connection * c, unsigned int game_addr);
extern unsigned short conn_get_game_port(t_connection const * c) PURE_ATTR();
extern int conn_set_game_port(t_connection * c, unsigned short game_port);
extern void conn_set_host(t_connection * c, char const * host);
extern void conn_set_user(t_connection * c, char const * user);
extern const char * conn_get_user(t_connection const * c);
extern void conn_set_owner(t_connection * c, char const * owner);
extern const char * conn_get_owner(t_connection const * c);
extern void conn_set_cdkey(t_connection * c, char const * cdkey);
extern char const * conn_get_clientexe(t_connection const * c) PURE_ATTR();
extern void conn_set_clientexe(t_connection * c, char const * clientexe);
extern char const * conn_get_archtag(t_connection const * c) PURE_ATTR();
extern void conn_set_archtag(t_connection * c, char const * archtag);
extern char const * conn_get_clienttag(t_connection const * c) PURE_ATTR();
extern char const * conn_get_fake_clienttag(t_connection const * c) PURE_ATTR();
extern void conn_set_clienttag(t_connection * c, char const * clienttag);
extern unsigned long conn_get_versionid(t_connection const * c) PURE_ATTR();
extern int conn_set_versionid(t_connection * c, unsigned long versionid);
extern char const * conn_get_clientver(t_connection const * c) PURE_ATTR();
extern void conn_set_clientver(t_connection * c, char const * clientver);
extern int conn_get_tzbias(t_connection const * c) PURE_ATTR();
extern void conn_set_tzbias(t_connection * c, int tzbias);
extern int conn_set_botuser(t_connection * c, char const * username);
extern char const * conn_get_botuser(t_connection const * c);
extern unsigned int conn_get_flags(t_connection const * c) PURE_ATTR();
extern int conn_set_flags(t_connection * c, unsigned int flags);
extern void conn_add_flags(t_connection * c, unsigned int flags);
extern void conn_del_flags(t_connection * c, unsigned int flags);
extern unsigned int conn_get_latency(t_connection const * c) PURE_ATTR();
extern void conn_set_latency(t_connection * c, unsigned int ms);
extern char const * conn_get_awaystr(t_connection const * c) PURE_ATTR();
extern int conn_set_awaystr(t_connection * c, char const * away);
extern char const * conn_get_dndstr(t_connection const * c) PURE_ATTR();
extern int conn_set_dndstr(t_connection * c, char const * dnd);
extern int conn_add_ignore(t_connection * c, t_account * account);
extern int conn_del_ignore(t_connection * c, t_account const * account);
extern int conn_add_watch(t_connection * c, t_account * account);
extern int conn_del_watch(t_connection * c, t_account * account);
extern t_channel * conn_get_channel(t_connection const * c) PURE_ATTR();
extern int conn_set_channel_var(t_connection * c, t_channel * channel);
extern int conn_set_channel(t_connection * c, char const * channelname);
extern t_game * conn_get_game(t_connection const * c) PURE_ATTR();
extern int conn_set_game(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version);
#ifdef WITH_BITS
extern int conn_set_game_bits(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version, t_game_option option);
#endif
extern t_queue * * conn_get_in_queue(t_connection * c) PURE_ATTR();
extern int conn_get_in_size(t_connection const * c) PURE_ATTR();
extern void conn_set_in_size(t_connection * c, unsigned int size);
extern t_queue * * conn_get_out_queue(t_connection * c) PURE_ATTR();
extern int conn_get_out_size(t_connection const * c) PURE_ATTR();
extern void conn_set_out_size(t_connection * c, unsigned int size);
extern int conn_check_ignoring(t_connection const * c, char const * me) PURE_ATTR();
extern t_account * conn_get_account(t_connection const * c) PURE_ATTR();
extern void conn_set_account(t_connection * c, t_account * account);
extern int conn_get_socket(t_connection const * c) PURE_ATTR();
extern int conn_get_game_socket(t_connection const * c) PURE_ATTR();
extern int conn_set_game_socket(t_connection * c, int usock);
extern char const * conn_get_username(t_connection const * c);
extern void conn_unget_username(t_connection const * c, char const * name);
extern char const * conn_get_chatname(t_connection const * c);
extern int conn_unget_chatname(t_connection const * c, char const * name);
extern char const * conn_get_chatcharname(t_connection const * c, t_connection const * dst);
extern int conn_unget_chatcharname(t_connection const * c, char const * name);
extern unsigned int conn_get_userid(t_connection const * c);
extern char const * conn_get_playerinfo(t_connection const * c);
extern int conn_set_playerinfo(t_connection const * c, char const * playerinfo);
extern char const * conn_get_realminfo(t_connection const * c);
extern int conn_set_realminfo(t_connection * c, char const * realminfo);
extern char const * conn_get_charname(t_connection const * c);
extern int conn_set_charname(t_connection * c, char const * charname);
extern int conn_set_idletime(t_connection * c);
extern unsigned int conn_get_idletime(t_connection const * c) PURE_ATTR();
extern char const * conn_get_realmname(t_connection const * c);
extern int conn_set_realmname(t_connection * c, char const * realmname);
extern int conn_set_character(t_connection * c, t_character * ch);
extern int conn_bind(t_connection * c1, t_connection * c2);
extern void conn_set_country(t_connection * c, char const * country);
extern char const * conn_get_country(t_connection const * c);
extern int conn_quota_exceeded(t_connection * c, char const * message);
extern int conn_set_lastsender(t_connection * c, char const * sender);
extern char const * conn_get_lastsender(t_connection const * c);
extern t_versioncheck const * conn_get_versioncheck(t_connection const * c) PURE_ATTR();
extern int conn_set_versioncheck(t_connection * c, t_versioncheck const * versioncheck);
extern int conn_set_ircline(t_connection * c, char const * line);
extern char const * conn_get_ircline(t_connection const * c);
extern int conn_set_ircpass(t_connection * c, char const * pass);
extern char const * conn_get_ircpass(t_connection const * c);
extern int conn_set_ircping(t_connection * c, unsigned int ping);
extern unsigned int conn_get_ircping(t_connection const * c);
extern int conn_set_udpok(t_connection * c);

extern int connlist_create(void);
extern int connlist_destroy(void);
extern t_list * connlist(void) PURE_ATTR();
extern t_connection * connlist_find_connection(char const * username);
extern t_connection * connlist_find_connection_by_sessionkey(unsigned int sessionkey);
extern t_connection * connlist_find_connection_by_sessionnum(unsigned int sessionnum);
extern t_connection * connlist_find_connection_by_name(char const * name, char const * realmname);
extern t_connection * connlist_find_connection_by_charname(char const * charname, char const * realmname);
#ifdef WITH_BITS
extern t_connection * connlist_find_connection_by_sessionid(unsigned int sessionid);
#endif
extern int connlist_get_length(void) PURE_ATTR();
extern int connlist_login_get_length(void) PURE_ATTR();
extern int connlist_total_logins(void) PURE_ATTR();

#endif
#endif

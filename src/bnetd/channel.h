/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2000  Marco Ziech (mmz@gmx.net)
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
#ifndef INCLUDED_CHANNEL_TYPES
#define INCLUDED_CHANNEL_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# undef JUST_NEED_TYPES
#endif

#ifdef CHANNEL_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# include <stdio.h>
# include "connection.h"
# include "common/list.h"
#else
# define JUST_NEED_TYPES
# include <stdio.h>
# include "connection.h"
# include "common/list.h"
# undef JUST_NEED_TYPES
#endif

#endif

#ifdef WITH_BITS
typedef struct
{
    int		 sessionid;       /* Session ID */
    unsigned int latency;   	  /* Latency */
    unsigned int flags;           /* Flags */
} t_bits_channelmember;
#endif

#ifdef CHANNEL_INTERNAL_ACCESS
typedef struct channelmember
{
#ifndef WITH_BITS
    /* standalone mode */
    t_connection *         connection;
#else
    /* network mode */
    /* We don't have a t_connection struct for every user */
    /* in this channel. A remote connection (rconn) would */
    /* require a loaded account for every user in this channel. */
    t_bits_channelmember member; /* not a pointer */
#endif
    struct channelmember * next;
} t_channelmember;
#endif

typedef enum
{
    channel_flags_none=0x00,
    channel_flags_public=0x01,
    channel_flags_moderated=0x02,
    channel_flags_restricted=0x04,
    channel_flags_thevoid=0x08,
    channel_flags_system=0x10,
    channel_flags_official=0x20
} t_channel_flags;

/* Just a quick note for BITS: Only the master server decides whether
 * to remove or add users to channels. The 'subscribed' clients just
 * have to follow the instructions from the master server. 
 * The same is valid for almost everything else (eg. operator, channelname, ...)
 * in channels. 
 * There are two types of channels for bits clients: 
 *
 *  1) unsubscribed channels (ref==0)
 *    The bits client is only notified when this channel is created or
 *    removed from the channellist. So clients can maintain their
 *    own channellist. This mode is the default when there are no local
 *    users (or users on 'sub-servers') in this channel. There is
 *    only low bits traffic.
 *    
 *  2) subscribed channels (ref>0)
 *    The bits client receives any messages for this channel (joins,leaves,chats,...).
 *    Since this produces high traffic on the bits network it's only
 *    used when it's really needed. It's only needed if there are local users
 *    in this channel or users on 'sub-servers'.
 *    The server subscribes to that channel as soon as the ref level is higher than
 *    0. The ref level _could_ be calculated by adding the number of local users
 *    in this channel to the number of 'sub-servers' where this channel is needed.
 *    
 * I hope that described the mechanism good enough - Marco
 */

typedef struct channel
#ifdef CHANNEL_INTERNAL_ACCESS
{
    char const *      name;
    char const *      shortname;  /* short "alias" for permanent channels, NULL if none */
    char const *      country;
    char const *      realmname;
    int               permanent;  /* FIXME: merge with flags */
    int               allowbots;  /* FIXME: merge with flags */
    int               allowopers; /* FIXME: merge with flags */
    t_channel_flags   flags;
    unsigned int      maxmembers;
    unsigned int      currmembers;
    char const *      clienttag;
    unsigned int      id;
    t_channelmember * memberlist;
    t_list *          banlist;    /* of char * */
#ifdef WITH_BITS
    unsigned int      opr;        /* bits master: sessionid of operator */
    unsigned int      next_opr;   /* bits master: sessionid of designated operator */
#else
    t_connection *    opr;        /* operator */
    t_connection *    next_opr;   /* designated operator */
#endif
    char *            logname;    /* NULL if not logged */
    FILE *            log;        /* NULL if not logging */
#ifdef WITH_BITS
    int               ref;        /* how many local users + remote (bits) connections use this channel */
#endif
}
#endif
t_channel;

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_CHANNEL_PROTOS
#define INCLUDED_CHANNEL_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#include "message.h"
#include "common/list.h"
#undef JUST_NEED_TYPES

#define CHANNEL_NAME_BANNED "THE VOID"
#define CHANNEL_NAME_KICKED "THE VOID"
#define CHANNEL_NAME_CHAT   "Chat"

extern t_channel * channel_create(char const * fullname, char const * shortname, char const * clienttag, int permflag, int botflag, int operflag, int logflag, char const * country, char const * realmname, int maxmembers) MALLOC_ATTR();
extern int channel_destroy(t_channel * channel);
extern char const * channel_get_name(t_channel const * channel);
extern char const * channel_get_shortname(t_channel const * channel);
extern char const * channel_get_clienttag(t_channel const * channel);
extern t_channel_flags channel_get_flags(t_channel const * channel);
extern int channel_get_permanent(t_channel const * channel);
extern unsigned int channel_get_channelid(t_channel const * channel);
extern int channel_set_channelid(t_channel * channel, unsigned int channelid);
extern int channel_add_connection(t_channel * channel, t_connection * connection);
extern int channel_del_connection(t_channel * channel, t_connection * connection);
extern void channel_update_latency(t_connection * conn);
extern void channel_update_flags(t_connection * conn);
extern void channel_message_log(t_channel const * channel, t_connection * me, int fromuser, char const * text);
extern void channel_message_send(t_channel const * channel, t_message_type type, t_connection * conn, char const * text);
extern t_connection * channel_get_operator(t_channel const * channel);
extern int channel_choose_operator(t_channel * channel, t_connection * tryop);
extern int channel_set_next_operator(t_channel * channel, t_connection * conn);
extern int channel_ban_user(t_channel * channel, char const * user);
extern int channel_unban_user(t_channel * channel, char const * user);
extern int channel_check_banning(t_channel const * channel, t_connection const * user);
extern t_list * channel_get_banlist(t_channel const * channel);
extern int channel_get_length(t_channel const * channel);
#ifndef WITH_BITS
extern t_connection * channel_get_first(t_channel const * channel);
extern t_connection * channel_get_next(void);
#else
extern t_bits_channelmember * channel_get_first(t_channel const * channel);
extern t_bits_channelmember * channel_get_next(void);

extern int channel_add_member(t_channel * channel, int sessionid, int flags, int latency);
extern int channel_del_member(t_channel * channel, t_bits_channelmember * member);
extern t_bits_channelmember * channel_find_member_bysessionid(t_channel const * channel, int sessionid);

extern int channel_add_ref(t_channel * channel);
extern int channel_del_ref(t_channel * channel);
#endif

extern int channellist_create(void);
extern int channellist_destroy(void);
extern t_list * channellist(void);
extern t_channel * channellist_find_channel_by_name(char const * name, char const * locale, char const * realmname);
extern t_channel * channellist_find_channel_bychannelid(unsigned int channelid);
extern int channellist_get_length(void);

#endif
#endif

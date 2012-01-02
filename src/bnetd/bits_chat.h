/*
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
 
#ifndef INCLUDED_BITS_CHAT_TYPES
#define INCLUDED_BITS_CHAT_TYPES

#if 0
#ifdef JUST_NEED_TYPES
# include "account.h"
#else
# define JUST_NEED_TYPES
# include "account.h"
# undef JUST_NEED_TYPES
#endif

/*typedef struct {
	t_account *sa;
	t_account *da;
	int type;
	unsigned int flags;
	unsigned int latency;
	char const * clienttag;
	char * text;
} t_bits_chat_user_query_data;*/
#endif

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_CHAT_PROTOS
#define INCLUDED_BITS_CHAT_PROTOS

#define JUST_NEED_TYPES
#include "channel.h"
#undef JUST_NEED_TYPES

extern int bits_chat_user(char const * srcname, char const *destname, int type, unsigned int latency, unsigned int flags, char const * text);
extern int send_bits_chat_user(int src, int dst, int type, unsigned int latency, unsigned int flags, char const * text);
extern int send_bits_chat_channellist_add(t_channel * channel, t_connection * conn);
extern int send_bits_chat_channellist_del(int channelid);
extern int bits_chat_channellist_send_all(t_connection * conn);
extern int send_bits_chat_channel_server_join(int channelid);
extern int send_bits_chat_channel_server_leave(int channelid);
extern int send_bits_chat_channel_join(t_connection * conn, int channelid, t_bits_channelmember *member);
extern int send_bits_chat_channel_leave(int channelid, unsigned int sessionid);
extern int send_bits_chat_channel(t_bits_channelmember * member, int channelid, int type, char const * text);
extern int send_bits_chat_channel_join_perm_request(t_uint32 qid, unsigned int sessionid, unsigned int latency, unsigned int flags, char const * name, char const * country);

#endif
#endif


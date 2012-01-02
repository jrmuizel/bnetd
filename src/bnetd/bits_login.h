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
#ifndef INCLUDED_BITS_LOGIN_TYPES
#define INCLUDED_BITS_LOGIN_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/field_sizes.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/field_sizes.h"
# undef JUST_NEED_TYPES
#endif

typedef struct t_bits_loginlist_entry {
	struct t_bits_loginlist_entry *prev;
	struct t_bits_loginlist_entry *next;
	unsigned int 		uid;
	t_uint16		loginhost;
	unsigned int		sessionid;
	bn_int			clienttag;
    	unsigned int     	game_addr; 
       	unsigned short   	game_port; 
	char			chatname[USER_NAME_MAX+1];
	char		*	playerinfo;
} t_bits_loginlist_entry;

#endif

/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_LOGIN_PROTOS
#define INCLUDED_BITS_LOGIN_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

extern t_bits_loginlist_entry * bits_loginlist;

extern int send_bits_va_logout(int sessionid);
extern int bits_loginlist_add(t_uint32 uid, t_uint16 loginhost, t_uint32 sessionid, bn_int const clienttag, unsigned int game_addr, unsigned short game_port, char const *chatname);
extern int bits_loginlist_del(t_uint32 sessionid);
extern int bits_loginlist_set_playerinfo(unsigned int sessionid, char const * playerinfo);
extern int send_bits_va_update_playerinfo(unsigned int sessionid, char const * playerinfo, t_connection * c);
extern int send_bits_va_masterlogout(t_bits_loginlist_entry * lle, t_connection * c);
extern int send_bits_va_login(t_bits_loginlist_entry * lle, t_connection * c);
extern int send_bits_va_loginreq(unsigned int qid, unsigned int uid, unsigned int gameaddr, unsigned short gameport, char const * clienttag);
extern int bits_va_loginlist_get_free_sessionid(void);
extern t_bits_loginlist_entry * bits_loginlist_bysessionid(unsigned int sessionid);
extern t_bits_loginlist_entry * bits_loginlist_byname(char const * name);
extern char const * bits_loginlist_get_name_bysessionid(unsigned int sessionid);
extern int bits_va_loginlist_sendall(t_connection * c);

#endif
#endif

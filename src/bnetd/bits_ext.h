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

#ifdef WITH_BITS

#ifndef INCLUDED_BITS_EXT_TYPES
#define INCLUDED_BITS_EXT_TYPES

/* FIXME: replace lists with t_list */
typedef struct t_va_locklist_entry {
	struct t_va_locklist_entry * next;
	struct t_va_locklist_entry * prev;
	char username[USER_NAME_MAX+1];
	int uid;
} t_va_locklist_entry;

typedef struct t_bits_channellist_entry {
	struct t_bits_channellist_entry * next;
	struct t_bits_channellist_entry * prev;
	int channelid;
} t_bits_channellist_entry;

typedef enum {
	bits_to_master, /* connection to uplink server */
	bits_to_slave,  /* connection to other bits clients */
	bits_loop       /* bits loopback connection */
} t_bits_connection_type;

typedef struct {
	t_bits_connection_type    type;
	t_uint16                  myaddr;
	t_va_locklist_entry      *va_locklist;
	t_bits_channellist_entry *channellist;
} t_bits_connection_extension;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_EXT_PROTOS
#define INCLUDED_BITS_EXT_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

extern int create_bits_ext(t_connection *c, t_bits_connection_type type);
extern void destroy_bits_ext(t_connection *c);
extern t_va_locklist_entry * bits_va_locklist_byname(t_connection * c, char const * name);
extern t_va_locklist_entry * bits_va_locklist_byuid(t_connection * c, int uid);
extern int bits_va_locklist_add(t_connection * c, const char *name, int uid);
extern int bits_va_locklist_del(t_connection * c, t_va_locklist_entry * e);
extern t_connection * bits_va_locklist_is_locked_by(int uid);

extern int bits_ext_channellist_add(t_connection * c, int channelid);
extern int bits_ext_channellist_del(t_connection * c, int channelid);
extern t_bits_channellist_entry * bits_ext_channellist_find(t_connection * c, int channelid);
extern t_connection * bits_ext_channellist_is_needed(int channelid);


#endif
#endif

#endif

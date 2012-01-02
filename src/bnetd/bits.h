/*
 * Copyright (C) 1999,2000,2001  Marco Ziech (mmz@gmx.net)
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

#ifndef INCLUDED_BITS_TYPES
#define INCLUDED_BITS_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/field_sizes.h"
# include "common/bnethash.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# include "common/field_sizes.h"
# include "common/bnethash.h"
# undef JUST_NEED_TYPES
#endif

typedef struct {
	t_uint16	address;
	char		*name;
} t_bits_hostlist_entry;

typedef struct {
	int is_plain;
	char plain[256];
	t_hash hash;
} t_bits_auth_password;

typedef struct {
	bn_int ticks;
	bn_int sessionkey;
	bn_int passhash[5];
} t_bits_auth_bnethash;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_PROTOS
#define INCLUDED_BITS_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#include "common/packet.h"
#include "common/bn_type.h"
#include "common/list.h"
#include "compat/uint.h"
#undef JUST_NEED_TYPES

extern t_connection	* bits_uplink_connection;
extern t_connection	* bits_loopback_connection;
extern t_list * bits_hostlist;
extern int bits_debug; /* 1: extended debugging on (for testing only) (FIXME: remove this)*/
extern int bits_master; /* 0: BITS client; 1: BITS master; -1: not specified */

extern unsigned short bits_new_dyn_addr(void);
extern t_bits_auth_password * bits_get_user_password_from_file(char const *user);
extern t_uint16 bits_get_myaddr(void);
extern int bits_auth_bnethash_second(t_hash * outhash, t_hash const inhash, t_uint32 sessionkey, t_uint32 ticks);
extern int bits_auth_bnethash_eq(unsigned int sessionkey, t_hash myhash, t_hash tryhash, t_uint32 ticks);
extern int bits_send_authreq(t_connection * c,const char *name, const char *password);
extern int bits_send_authreply(t_connection * c, unsigned char status, t_uint16 address, t_uint16 dest, t_uint32 qid);
extern int bits_check_bits_auth_request(t_connection * conn, t_packet const * packet);
extern void bits_setup_client(t_connection * conn);
extern int bits_status_ok(const bn_byte status);
extern int handle_bits_packet(t_connection * conn,t_packet const * const packet);
extern int bits_init(void);
extern int bits_destroy(void);

#endif
#endif

#endif /* WITH_BITS */

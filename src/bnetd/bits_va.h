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

#ifndef INCLUDED_BITS_VA_TYPES
#define INCLUDED_BITS_VA_TYPES

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_VA_PROTOS
#define INCLUDED_BITS_VA_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#include "connection.h"
#include "common/bn_type.h"
#undef JUST_NEED_TYPES

extern int bits_va_lock_account(const char *name);
extern int bits_va_unlock_account(t_account * account);
extern int bits_va_lock_check(void);
extern int bits_va_command_with_account_name(t_connection * c, char const * command, char const * name);
extern int send_bits_va_unlock(unsigned int uid);
extern int send_bits_va_create_req(t_connection *c, const char * username, bn_int const passhash1[5]);
extern int send_bits_va_create_reply(const char * username, t_uint16 dest, unsigned char status, t_uint32 uid, t_uint32 qid);
extern int send_bits_va_get_allattr(unsigned int uid);
extern int send_bits_va_allattr(t_account * ac, unsigned int uid, t_uint16 dest);
extern int send_bits_va_set_attr(unsigned int uid, char const * name, char const * val, t_connection * from);

#endif
#endif

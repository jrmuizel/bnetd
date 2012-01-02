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

#ifndef INCLUDED_BITS_QUERY_TYPES
#define INCLUDED_BITS_QUERY_TYPES

extern int bits_query_type_bits_va_create; 
extern int bits_query_type_bits_va_lock;
extern int bits_query_type_client_loginreq_1;
extern int bits_query_type_client_loginreq_2;
extern int bits_query_type_handle_command_account_locked;
extern int bits_query_type_handle_command_account_loaded;
extern int bits_query_type_handle_packet_account_loaded;
extern int bits_query_type_bot_loginreq;
extern int bits_query_type_chat_channel_join;
extern int bits_query_type_chat_channel_join_new;
extern int bits_query_type_chat_channel_join_perm;
extern int bits_query_type_bits_auth;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_QUERY_PROTOS
#define INCLUDED_BITS_QUERY_PROTOS

extern void bits_query_init(void);
extern void bits_query_destroy(void);

#endif
#endif

/*
 * bits_packet.h - some functions for handling bits packets and
 *                 bits routing tables
 *
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

#ifndef INCLUDED_BITS_PACKET_TYPES
#define INCLUDED_BITS_PACKET_TYPES

#ifdef JUST_NEED_TYPES
# include "connection.h"
#else
# define JUST_NEED_TYPES
# include "connection.h"
# undef JUST_NEED_TYPES
#endif

/* BITS routing stuff */

typedef struct t_bits_routing_table_entry
{
	t_connection *conn; /* the connection to send packets to */
	t_uint16 bits_addr;  /* bits host address */
} t_bits_routing_table_entry;


#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_PACKET_PROTOS
#define INCLUDED_BITS_PACKET_PROTOS

#define JUST_NEED_TYPES
#include "common/packet.h"
#include "common/bn_type.h"
#include "common/list.h"
#undef JUST_NEED_TYPES

/* This is the BITS routing table */
extern t_list * bits_routing_table;

extern int bits_route_add(t_uint16 bits_addr, t_connection * conn);
extern t_bits_routing_table_entry * bits_route_find(t_uint16 bits_addr);
extern int bits_route_del(t_uint16 bits_addr);
extern void bits_route_del_conn(t_connection * c);
extern int bits_route_delentry(t_bits_routing_table_entry * entry);

extern int bits_packet_generic(t_packet *packet, t_uint16 dest);
extern t_uint16 bits_packet_get_src(const t_packet *p);
extern t_uint16 bits_packet_get_dst(const t_packet *p);
extern int send_bits_packet(t_packet * p);
extern int send_bits_packet_up(t_packet * p);
extern int send_bits_packet_on(t_packet * p, t_connection * c);
extern int bits_packet_forward_bcast(t_connection * conn, t_packet * packet);
extern int send_bits_packet_down(t_packet * packet);
extern int send_bits_packet_for_account(t_packet * packet, int uid, t_connection * from);
extern int send_bits_packet_for_channel(t_packet * packet, int channelid, t_connection * from);
extern int bits_packet_get_ttl(const t_packet * p);
extern int bits_packet_set_ttl(t_packet * p, unsigned int ttl);

#endif
#endif

#endif

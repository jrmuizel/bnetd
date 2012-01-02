/*
 * bits_packet.c - some functions for handling bits packets and
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
#define CONNECTION_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef WITH_BITS
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include <errno.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strerror.h"
#include "common/bn_type.h"
#include "common/packet.h"
#include "common/queue.h"
#include "common/list.h"
#include "connection.h"
#include "common/eventlog.h"
#include "bits.h"
#include "bits_ext.h"
#include "bits_packet.h"
#include "common/setup_after.h"


t_list * bits_routing_table = NULL;

extern int bits_route_add(t_uint16 bits_addr, t_connection * conn)
{
	t_bits_routing_table_entry * rte;

	if (!conn) {
		eventlog(eventlog_level_error,"bits_route_add","got NULL connection");
		return -1;
	}
	if ((bits_addr==BITS_ADDR_PEER)||(bits_addr==BITS_ADDR_MASTER)||(bits_addr==BITS_ADDR_BCAST)) {
	/*	eventlog(eventlog_level_warn,"bits_route_add","adding routes for special addresses would make no sense"); */
		return 0;
	}
	rte = bits_route_find(bits_addr);
	if (rte) {
		if (rte->conn == conn) return 0;
		eventlog(eventlog_level_debug,"bits_route_add","changing existing route for 0x%04x from [%d] to [%d].",bits_addr,conn_get_socket(rte->conn),conn_get_socket(conn));
		rte->conn = conn;
	} else {
		eventlog(eventlog_level_debug,"bits_route_add","adding route for 0x%04x to [%d].",bits_addr,conn_get_socket(conn));
		rte = malloc(sizeof(t_bits_routing_table_entry));
		if (!rte) {
			eventlog(eventlog_level_error,"bits_route_add","malloc failed: %s",strerror(errno));
			return -1;
		}
		rte->bits_addr = bits_addr;
		rte->conn = conn;
		list_append_data(bits_routing_table,rte);
	}
	return 0;
}

extern t_bits_routing_table_entry * bits_route_find(t_uint16 bits_addr)
{
    t_elem const * curr;

    LIST_TRAVERSE_CONST(bits_routing_table,curr) {
	t_bits_routing_table_entry * rte = elem_get_data(curr);

	if (rte->bits_addr==bits_addr) return rte;
    }
    return NULL;
}

extern int bits_route_del(t_uint16 bits_addr)
{
	t_bits_routing_table_entry * rte;

	rte = bits_route_find(bits_addr);
	if (rte) return bits_route_delentry(rte);
	list_purge(bits_routing_table);
	return 0;
}

extern void bits_route_del_conn(t_connection * c)
{
    t_elem * curr;

    LIST_TRAVERSE(bits_routing_table,curr) {
	t_bits_routing_table_entry * e = elem_get_data(curr);

	if (e->conn == c)
	    bits_route_delentry(e);
    }
    list_purge(bits_routing_table);
}

extern int bits_route_delentry(t_bits_routing_table_entry * entry)
{
	if (!entry) {
		eventlog(eventlog_level_error,"bits_route_delentry","got NULL entry");
		return -1;
	}
	eventlog(eventlog_level_debug,"bits_route_delentry","removing table entry for bits host 0x%04x",entry->bits_addr);
	list_remove_data(bits_routing_table,entry);
	free(entry);
	return 0;
}



extern int bits_packet_generic(t_packet *packet, t_uint16 dest)
{
	if (packet == NULL) {
		eventlog(eventlog_level_error,"bits_packet_generic","got NULL packet");
		return -1;
	}
	bn_short_set(&packet->u.bits.h.src_addr,bits_get_myaddr());
	bn_short_set(&packet->u.bits.h.dst_addr,dest);
	bn_byte_set(&packet->u.bits.h.ttl,BITS_DEFAULT_TTL);
	return 0;	
}

extern t_uint16 bits_packet_get_src(const t_packet *p)
{
	return bn_short_get(p->u.bits.h.src_addr);
}

extern t_uint16 bits_packet_get_dst(const t_packet *p)
{
	return bn_short_get(p->u.bits.h.dst_addr);
}

extern int send_bits_packet(t_packet * p) {
	t_connection * conn = NULL;
	t_uint16 addr;
	t_bits_routing_table_entry * rte;

	if (!p) {
		eventlog(eventlog_level_error,"send_bits_packet","got NULL packet");
		return -1;
	}
	if (packet_get_class(p)!= packet_class_bits) {
		eventlog(eventlog_level_error,"send_bits_packet","this is not a bits packet");
		return -1;
	}
	addr = bits_packet_get_dst(p);
	if (bits_debug) eventlog(eventlog_level_debug,"send_bits_packet","sending BITS packet with type 0x%04x (%s) to 0x%04x (%d bytes)",packet_get_type(p),packet_get_type_str(p,packet_dir_from_server),addr,packet_get_size(p));
	if (addr == BITS_ADDR_MASTER) {
		conn = bits_uplink_connection;
	} else {
		rte = bits_route_find(addr);
		if (rte) conn = rte->conn;
	}
	if ((addr == BITS_ADDR_MASTER)&&(!bits_uplink_connection)) {
		eventlog(eventlog_level_debug,"send_bits_packet","sending packet to myself");
		handle_bits_packet(bits_loopback_connection,p);
		return 0;
	}
	if (!conn) {
		t_connection *c;
		t_elem const * curr;

		LIST_TRAVERSE_CONST(connlist(),curr)
		{
			c = elem_get_data(curr);
			if (conn_get_class(c) == conn_class_bits) {
				if (bits_debug) eventlog(eventlog_level_debug,"send_bits_packet","DEBUG: sending packet on [%d].",conn_get_socket(c));	
				queue_push_packet(conn_get_out_queue(c),p);
			}
		}
	} else {
		if (bits_debug) eventlog(eventlog_level_debug,"send_bits_packet","DEBUG: sending packet on [%d].",conn_get_socket(conn));	
		queue_push_packet(conn_get_out_queue(conn),p);
	}
	return 0;
}

extern int send_bits_packet_up(t_packet * p) {
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_packet_up","got NULL packet");
		return -1;
	}
	if (packet_get_class(p)!= packet_class_bits) {
		eventlog(eventlog_level_error,"send_bits_packet_up","this is not a bits packet");
		return -1;
	}
	if (!bits_uplink_connection) {
		eventlog(eventlog_level_error,"send_bits_packet_up","bits_uplink_connection is NULL");
	}
	if (bits_debug) eventlog(eventlog_level_debug,"send_bits_packet_up","DEBUG: sending packet on [%d].",conn_get_socket(bits_uplink_connection));
	queue_push_packet(conn_get_out_queue(bits_uplink_connection),p);		
	return 0;
}

extern int send_bits_packet_on(t_packet * p, t_connection * c) {
	if (!p) {
		eventlog(eventlog_level_error,"send_bits_packet_on","got NULL packet");
		return -1;
	}
	if (!c) {
		eventlog(eventlog_level_error,"send_bits_packet_on","got NULL connection");
		return -1;
	}
	if (conn_get_class(c) != conn_class_bits)
		eventlog(eventlog_level_warn,"send_bits_packet_on","This connection is not a BITS connection.");
	if (packet_get_class(p)!= packet_class_bits) {
		eventlog(eventlog_level_error,"send_bits_packet_on","this is not a bits packet");
		return -1;
	}
	if (bits_debug) eventlog(eventlog_level_debug,"send_bits_packet_on","DEBUG: sending packet on [%d].",conn_get_socket(c));
	queue_push_packet(conn_get_out_queue(c),p);		
	return 0;
}

extern int bits_packet_forward_bcast(t_connection * conn, t_packet * packet) {
	t_connection *c;
	t_elem const * curr;
	
	if (!packet) {
		eventlog(eventlog_level_error,"bits_packet_forward_bcast","got NULL packet");
		return -1;
	}
	
	LIST_TRAVERSE_CONST(connlist(),curr)
	{
		c = elem_get_data(curr);
		if (conn_get_class(c) == conn_class_bits) {
			if (c!=conn) send_bits_packet_on(packet,c);
		}
	}
	return 0;
}

extern int send_bits_packet_down(t_packet * packet) {
	return bits_packet_forward_bcast(bits_uplink_connection,packet);
}

extern int send_bits_packet_for_account(t_packet * packet, int uid, t_connection * from) {
	/* from may be NULL */
	t_connection *c;
	t_elem const * curr;
	
	if (!packet) {
		eventlog(eventlog_level_error,"send_bits_packet_for_account","got NULL packet");
		return -1;
	}
	LIST_TRAVERSE_CONST(connlist(),curr)
	{
		c = elem_get_data(curr);
		if (conn_get_class(c) == conn_class_bits) {
			t_va_locklist_entry * e;
			e = bits_va_locklist_byuid(c,uid);
			if (((e)||(c==bits_uplink_connection))&&(c!=from))
				send_bits_packet_on(packet,c);
		}
	}
	return 0;	
}

extern int send_bits_packet_for_channel(t_packet * packet, int channelid, t_connection * from) {
	/* from may be NULL */
	t_connection *c;
	t_elem const * curr;
	
	if (!packet) {
		eventlog(eventlog_level_error,"send_bits_packet_for_channel","got NULL packet");
		return -1;
	}
	LIST_TRAVERSE_CONST(connlist(),curr)
	{
		c = elem_get_data(curr);
		if (conn_get_class(c) == conn_class_bits) {
			if (((bits_ext_channellist_find(c,channelid))||(c==bits_uplink_connection))&&(c!=from))
				send_bits_packet_on(packet,c);
		}
	}
	return 0;	
}

extern int bits_packet_get_ttl(const t_packet * p)
{
	if (!p) {
		eventlog(eventlog_level_error,"bits_packet_get_ttl","got NULL packet");
		return -1;
	}
	return bn_byte_get(p->u.bits.h.ttl);
} 

extern int bits_packet_set_ttl(t_packet * p, unsigned int ttl)
{
	if (!p) {
		eventlog(eventlog_level_error,"bits_packet_set_ttl","got NULL packet");
		return -1;
	}
	if (ttl > 0xff) {
		eventlog(eventlog_level_warn,"bits_packet_set_ttl","ttl out of range, reverting to default ttl");
		bn_byte_set(&p->u.bits.h.ttl,BITS_DEFAULT_TTL);
	} else {
		bn_byte_set(&p->u.bits.h.ttl,ttl);
	}
	return 0;
} 

#endif

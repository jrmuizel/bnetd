/*
 * bits_net.c - function for sending bits_net messages
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
#include "common/setup_before.h"
#ifdef WITH_BITS
#include "common/packet.h"
#include "common/eventlog.h"
#include "bits_packet.h"
#include "common/bits_protocol.h"
#include "bits_net.h"
#include "common/setup_after.h"

extern int bits_net_send_discover(void) {
	t_packet * p;
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"bits_net_send_discover","Could not create a packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_net_discover));
	packet_set_type(p,BITS_NET_DISCOVER);
	bits_packet_generic(p,BITS_ADDR_BCAST);
	send_bits_packet(p);
	packet_del_ref(p);
	return 0;
}

extern int bits_net_send_undiscover(void) {
	t_packet * p;
	p = packet_create(packet_class_bits);
	if (!p) {
		eventlog(eventlog_level_error,"bits_net_send_undiscover","Could not create a packet");
		return -1;
	}
	packet_set_size(p,sizeof(t_bits_net_undiscover));
	packet_set_type(p,BITS_NET_UNDISCOVER);
	bits_packet_generic(p,BITS_ADDR_BCAST);
	send_bits_packet(p);
	packet_del_ref(p);
	return 0;
}

#endif /* WITH_BITS */

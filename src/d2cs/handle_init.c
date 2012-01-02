/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
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
#include "setup.h"

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif

#include "connection.h"
#include "handle_init.h"
#include "handle_d2gs.h"
#include "d2gs.h"
#include "common/init_protocol.h"
#include "common/addr.h"
#include "common/packet.h"
#include "common/queue.h"
#include "common/eventlog.h"
#include "common/bn_type.h"
#include "common/setup_after.h"

static int on_d2gs_initconn(t_connection * c);
static int on_d2cs_initconn(t_connection * c);

extern int handle_init_packet(t_connection * c, t_packet * packet)
{
	int	class;
	int	retval;

	ASSERT(c,-1);
	ASSERT(packet,-1);
	class=bn_byte_get(packet->u.client_initconn.class);
	switch (class) {
		case CLIENT_INITCONN_CLASS_D2CS:
			retval=on_d2cs_initconn(c);
			break;
		case CLIENT_INITCONN_CLASS_D2GS:
			retval=on_d2gs_initconn(c);
			break;
		default:
			log_error("got bad connection class %d",class);
			retval=-1;
			break;
	}
	return retval;
}

static int on_d2gs_initconn(t_connection * c)
{
	t_d2gs * gs;

	log_info("[%d] client initiated d2gs connection",conn_get_socket(c));
	if (!(gs=d2gslist_find_gs_by_ip(conn_get_addr(c)))) {
		log_error("d2gs connection from invalid ip address %s",\
			addr_num_to_ip_str(conn_get_addr(c)));
		return -1;
	}
	conn_set_class(c,conn_class_d2gs);
	conn_set_state(c,conn_state_connected);
	conn_set_d2gs_id(c,d2gs_get_id(gs));
	if (handle_d2gs_init(c)<0) {
		log_error("failed to init d2gs connection");
		return -1;
	}
	return 0;
}

static int on_d2cs_initconn(t_connection * c)
{
	log_info("[%d] client initiated d2cs connection",conn_get_socket(c));
	conn_set_class(c,conn_class_d2cs);
	conn_set_state(c,conn_state_connected);
	return 0;
}

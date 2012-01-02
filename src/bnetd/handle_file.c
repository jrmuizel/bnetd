/*
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999,2000  Rob Crittenden (rcrit@greyoak.com)
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
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include "common/packet.h"
#include "common/file_protocol.h"
#include "common/eventlog.h"
#include "connection.h"
#include "common/queue.h"
#include "file.h"
#include "common/bn_type.h"
#include "handle_file.h"
#include "common/setup_after.h"


extern int handle_file_packet(t_connection * c, t_packet const * const packet)
{
    if (!c)
    {
	eventlog(eventlog_level_error,"handle_file_packet","[%d] got NULL connection",conn_get_socket(c));
	return -1;
    }
    if (!packet)
    {
	eventlog(eventlog_level_error,"handle_file_packet","[%d] got NULL packet",conn_get_socket(c));
	return -1;
    }
    if (packet_get_class(packet)!=packet_class_file)
    {
        eventlog(eventlog_level_error,"handle_file_packet","[%d] got bad packet (class %d)",conn_get_socket(c),(int)packet_get_class(packet));
        return -1;
    }
    
    switch (conn_get_state(c))
    {
    case conn_state_connected:
	switch (packet_get_type(packet))
	{
	case CLIENT_FILE_REQ:
	{
	    char const * rawname;
	    
	    if (!(rawname = packet_get_str_const(packet,sizeof(t_client_file_req),2048)))
	    {
		eventlog(eventlog_level_error,"handle_file_packet","[%d] got bad FILE_REQ (missing or too long filename)",conn_get_socket(c));
		
		return -1;
	    }
	    
	    file_send(c,rawname,
		      bn_int_get(packet->u.client_file_req.adid),
		      bn_int_get(packet->u.client_file_req.extensiontag),
		      bn_int_get(packet->u.client_file_req.startoffset),
		      1);
	}
	break;
	
	default:
	    eventlog(eventlog_level_error,"handle_file_packet","[%d] unknown file packet type 0x%04hx, len %u",conn_get_socket(c),packet_get_type(packet),packet_get_size(packet));
	    
	    break;
	}
	break;
	
    default:
	eventlog(eventlog_level_error,"handle_file_packet","[%d] unknown file connection state %d",conn_get_socket(c),(int)conn_get_state(c));
    }
    
    return 0;
}

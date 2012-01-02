/*
 * Copyright (C) 2001  Marco Ziech (mmz@gmx.net)
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

#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#include <errno.h>
#include "compat/strerror.h"
#include "bits_motd.h"
#include "bits_packet.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "common/list.h"
#include "common/bits_protocol.h"
#include "connection.h"
#include "message.h"
#include "prefs.h"
#include "bits.h"
#include "common/packet.h"
#include "common/setup_after.h"


t_bits_motd bits_motd = {0, NULL};

extern int bits_motd_load_from_file(void)
{
    FILE * f;
    char * buff;

    if (bits_uplink_connection) return -1; /* This function is only useful on master servers */
    if (bits_motd.lines) bits_motd_destroy();
    bits_motd.numlines=0;
    f = fopen(prefs_get_bits_motd_file(), "r");
    if (!f) {
    	eventlog(eventlog_level_error,"bits_motd_load_from_file","could not open bits motd file (\"%s\"): %s",prefs_get_bits_motd_file(),strerror(errno));
	return -1;
    }
    bits_motd.lines = list_create();
    while ((buff = file_get_line(f)))
    {
    	list_append_data(bits_motd.lines,buff);
	bits_motd.numlines++;
    }
    fclose(f);
    send_bits_net_motd(NULL);
    eventlog(eventlog_level_debug,"bits_motd_load_from_file","loaded bits motd from \"%s\"",prefs_get_bits_motd_file());
    return 0;
}

extern int bits_motd_load_from_packet(t_packet const * p)
{
    char const * buff;
    int pos = sizeof(t_bits_net_motd);

    if (!p) {
	eventlog(eventlog_level_error,"bits_motd_load_from_packet","got NULL packet");
	return -1;
    }
    if (!bits_uplink_connection) return -1; /* This function is only useful on slave servers */
    if (bits_motd.lines) bits_motd_destroy();
    bits_motd.numlines=0;
    bits_motd.lines = list_create();
    while (pos<packet_get_size(p))
    {
    	buff = packet_get_str_const(p,pos,MAX_MESSAGE_LEN);
    	if (!buff) break;
    	list_append_data(bits_motd.lines,strdup(buff));
	bits_motd.numlines++;
	pos = pos + strlen(buff) + 1;
    }
    eventlog(eventlog_level_debug,"bits_motd_load_from_packet","got bits motd from 0x%04x",bits_packet_get_src(p));
    return 0;
}

extern int send_bits_net_motd(t_connection * c)
{
    t_packet * p;
    t_elem const * curr;

    if (!bits_motd.lines) {
	eventlog(eventlog_level_warn,"send_bits_net_motd","nothing to send");
	return -1;
    }
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"send_bits_net_motd","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_net_motd));
    packet_set_type(p,BITS_NET_MOTD);
    bits_packet_generic(p,BITS_ADDR_BCAST);
    LIST_TRAVERSE_CONST(bits_motd.lines,curr)
	packet_append_string(p,elem_get_data(curr));
    if (!c)
	send_bits_packet(p);
    else
    	send_bits_packet_on(p,c);
    packet_del_ref(p);
    return 0;
}

/* The following function is a slightly modified copy of the
 * message_send_file() function. I don't like duplicated code
 * but I don't like too much #ifdefs either. - Typhoon */
extern void bits_motd_send(t_connection * c)
{
    char * buff;
    t_elem const * curr;    

    if (!c)
    {
	eventlog(eventlog_level_error,"bits_motd_send","got NULL connection");
	return;
    }
    LIST_TRAVERSE_CONST(bits_motd.lines,curr) {
	buff = strdup(elem_get_data(curr));
	if ((buff = message_format_line(c,buff)))
	{
	    /* empty messages can crash the clients */
	    switch (buff[0])
	    {
	    case 'C':
		handle_command(c,&buff[1]);
		break;
	    case 'B':
		message_send_text(c,message_type_broadcast,c,&buff[1]);
		break;
	    case 'E':
		message_send_text(c,message_type_error,c,&buff[1]);
		break;
	    case 'M':
		message_send_text(c,message_type_talk,c,&buff[1]);
		break;
	    case 'T':
		message_send_text(c,message_type_emote,c,&buff[1]);
		break;
	    case 'I':
	    case 'W':
		message_send_text(c,message_type_info,c,&buff[1]);
		break;
	    default:
		eventlog(eventlog_level_error,"bits_motd_send","unknown message type '%c'",buff[0]);
	    }
	}
	free(buff);
    }
}

extern void bits_motd_destroy(void)
{
    t_elem * curr;

    if (!bits_motd.lines) return;
    LIST_TRAVERSE(bits_motd.lines,curr) {
	t_bits_motd_line * l;
	l = elem_get_data(curr);
	if (l) {
	    free(l);
	} else {
	    eventlog(eventlog_level_warn,"bits_motd_destroy","NULL line in motd");
	}
	list_remove_data(bits_motd.lines,l);
    }
    list_purge(bits_motd.lines);
    list_destroy(bits_motd.lines);
    bits_motd.numlines=0;
}

#endif /* WITH_BITS */


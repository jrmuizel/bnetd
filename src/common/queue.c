/*
 * Copyright (C) 1998,1999,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#define QUEUE_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "common/packet.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/setup_after.h"


extern t_packet * queue_pull_packet(t_queue * * queue)
{
    t_queue *  temp;
    t_packet * packet;
    
    if (!queue)
    {
	eventlog(eventlog_level_error,"queue_pull_packet","got NULL queue pointer");
        return NULL;
    }
    if (!*queue)
        return NULL;
    
    temp = *queue;
    *queue = (*queue)->next;
    packet = temp->packet;
    free(temp);
    
    if (!packet)
    {
	eventlog(eventlog_level_error,"queue_pull_packet","NULL packet in queue");
        return NULL;
    }
    
    return packet;
}


extern t_packet * queue_peek_packet(t_queue const * const * queue)
{
    t_packet * packet;
    
    if (!queue)
    {
	eventlog(eventlog_level_error,"queue_peek_packet","got NULL queue pointer");
        return NULL;
    }
    if (!*queue)
        return NULL;
    
    packet = (*queue)->packet;
    
    if (!packet)
    {
        eventlog(eventlog_level_error,"queue_peek_packet","NULL packet in queue");
        return NULL;
    }
    
    return packet;
}


extern void queue_push_packet(t_queue * * queue, t_packet * packet)
{
    t_queue * temp;
    
    if (!queue)
    {
	eventlog(eventlog_level_error,"queue_push_packet","got NULL queue pointer");
        return;
    }
    if (!packet)
    {
	eventlog(eventlog_level_error,"queue_push_packet","got NULL packet");
        return;
    }
    
    temp = *queue;
    
    if (!temp)
    {
        if (!(temp = malloc(sizeof(t_queue))))
        {
	    eventlog(eventlog_level_error,"queue_push_packet","could not allocate memory for head of queue");
            return;
        }
	temp->next = NULL;
        *queue = temp;
    }
    else
    {
        /* add packet at end of queue */
	for (; temp->next; temp=temp->next);
	if (!(temp->next = malloc(sizeof(t_queue))))
	{
	    eventlog(eventlog_level_error,"queue_push_packet","could not allocate memory for tail of queue");
	    return;
	}
	temp->next->next = NULL;
	temp = temp->next;
    }
    
    temp->packet = packet_add_ref(packet);
}


extern int queue_get_length(t_queue const * const * queue)
{
    t_queue const * temp;
    int             count;
    
    if (!queue)
    {
	eventlog(eventlog_level_error,"queue_get_length","got NULL queue pointer");
	return 0;
    }
    
    for (temp=*queue,count=0; temp; temp=temp->next,count++);
    
    return count;
}


extern void queue_clear(t_queue * * queue)
{
    t_packet * temp;
    
    if (!queue)
    {
	eventlog(eventlog_level_error,"queue_clear","got NULL queue pointer");
	return;
    }
    
    while ((temp = queue_pull_packet(queue)))
	packet_del_ref(temp);
    if (*queue!=NULL)
	eventlog(eventlog_level_error,"queue_clear","could not clear queue");
}

/*
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#define TIMER_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "common/list.h"
#include "connection.h"
#include "common/eventlog.h"
#include "timer.h"
#include "common/setup_after.h"


static t_list * timerlist_head=NULL;


extern int timerlist_add_timer(t_connection * owner, time_t when, t_timer_cb cb, t_timer_data data)
{
    t_timer * timer;
    
    if (!owner)
    {
	eventlog(eventlog_level_error,"timerlist_add_timer","got NULL owner");
	return -1;
    }
    
    if (!(timer = malloc(sizeof(t_timer))))
    {
	eventlog(eventlog_level_error,"timerlist_add_timer","could not allocate memory for timer");
	return -1;
    }
    timer->owner = owner;
    timer->when  = when;
    timer->cb    = cb;
    timer->data  = data;
    
    if (list_prepend_data(timerlist_head,timer)<0)
    {
	free(timer);
	eventlog(eventlog_level_error,"timerlist_add_timer","could not prepend temp");
	return -1;
    }
    
    return 0;
}


extern int timerlist_del_all_timers(t_connection * owner)
{
    t_elem *  curr;
    t_timer * timer;
    
    if (!owner)
    {
	eventlog(eventlog_level_error,"timerlist_del_all_timers","got NULL owner");
	return -1;
    }
    
    LIST_TRAVERSE(timerlist_head,curr)
    {
	timer = elem_get_data(curr);
	if (!timer) /* should not happen */
	{
	    eventlog(eventlog_level_error,"timerlist_del_all_timers","timerlist contains NULL item");
	    return -1;
	}
	if (timer->owner==owner)
	{
	    if (timer->cb)
		timer->cb(timer->owner,(time_t)0,timer->data);
	    timer->cb = NULL;
	    timer->owner = NULL;
	    if (list_remove_elem(timerlist_head,curr)<0)
		eventlog(eventlog_level_error,"timerlist_del_all_timers","could not remove timer");
	    else
		free(timer);
	}
    }
    
    return 0;
}


extern int timerlist_check_timers(time_t when)
{
    t_elem *  curr;
    t_timer * timer;
    
    LIST_TRAVERSE(timerlist_head,curr)
    {
	timer = elem_get_data(curr);
	if (!timer) /* should not happen */
	{
	    eventlog(eventlog_level_error,"timerlist_check_timers","timerlist contains NULL item");
	    return -1;
	}
	if (timer->owner && timer->when<when)
	{
	    if (timer->cb)
		timer->cb(timer->owner,timer->when,timer->data);
	    timer->cb = NULL; /* mark for deletion on next timerlist_clean_timers() */
	    timer->owner = NULL;
	    if (list_remove_elem(timerlist_head,curr)<0)
		eventlog(eventlog_level_error,"timerlist_check_timers","could not remove timer");
	    else
		free(timer);
	}
    }
    
    list_purge(timerlist_head);
    
    return 0;
}

extern int timerlist_create(void)
{
    if (!(timerlist_head = list_create()))
	return -1;
    return 0;
}


extern int timerlist_destroy(void)
{
    t_elem *  curr;
    t_timer * timer;
    
    if (timerlist_head)
    {
	LIST_TRAVERSE(timerlist_head,curr)
	{
	    timer = elem_get_data(curr);
	    if (!timer) /* should not happen */
	    {
		eventlog(eventlog_level_error,"timerlist_destroy","timerlist contains NULL item");
		continue;
	    }
	    
	    if (list_remove_elem(timerlist_head,curr)<0)
		eventlog(eventlog_level_error,"timerlist_destroy","could not remove item from list");
	    free(timer);
	}
	
	if (list_destroy(timerlist_head)<0)
	    return -1;
	timerlist_head = NULL;
    }
    
    return 0;
}

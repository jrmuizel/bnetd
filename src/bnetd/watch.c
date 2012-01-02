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
#define WATCH_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include "common/field_sizes.h"
#include "common/list.h"
#include "account.h"
#include "connection.h"
#include "common/eventlog.h"
#include "message.h"
#include "watch.h"
#include "common/setup_after.h"


static t_list * watchlist_head=NULL;


/* who == NULL means anybody */
extern int watchlist_add_events(t_connection * owner, t_account * who, t_watch_event events)
{
    t_elem const * curr;
    t_watch_pair * pair;
    
    if (!owner)
    {
	eventlog(eventlog_level_error,"watchlist_add_events","got NULL owner");
	return -1;
    }
    
    LIST_TRAVERSE_CONST(watchlist_head,curr)
    {
	pair = elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,"watchlist_add_events","watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner && pair->who==who)
	{
	    pair->what |= events;
	    return 0;
	}
    }
    
    if (!(pair = malloc(sizeof(t_watch_pair))))
    {
	eventlog(eventlog_level_error,"watchlist_add_events","could not allocate memory for pair");
	return -1;
    }
    pair->owner = owner;
    pair->who   = who;
    pair->what  = events;
    
    if (list_prepend_data(watchlist_head,pair)<0)
    {
	free(pair);
	eventlog(eventlog_level_error,"watchlist_add_events","could not prepend temp");
	return -1;
    }
    
    return 0;
}


/* who == NULL means anybody */
extern int watchlist_del_events(t_connection * owner, t_account * who, t_watch_event events)
{
    t_elem *       curr;
    t_watch_pair * pair;
    
    if (!owner)
    {
	eventlog(eventlog_level_error,"watchlist_del_events","got NULL owner");
	return -1;
    }
    
    LIST_TRAVERSE(watchlist_head,curr)
    {
	pair = elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,"watchlist_del_events","watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner && pair->who==who)
	{
	    pair->what &= ~events;
	    if (pair->what==0)
	    {
		if (list_remove_elem(watchlist_head,curr)<0)
		{
		    eventlog(eventlog_level_error,"watchlist_del_events","could not remove item");
		    pair->owner = NULL;
		}
		else
		    free(pair);
	    }
	    
	    list_purge(watchlist_head);
	    
	    return 0;
	}
    }
    
    return -1; /* not found */
}


/* this differs from del_events because it doesn't return an error if nothing was found */
extern int watchlist_del_all_events(t_connection * owner)
{
    t_elem *       curr;
    t_watch_pair * pair;
    
    if (!owner)
    {
	eventlog(eventlog_level_error,"watchlist_del_all_events","got NULL owner");
	return -1;
    }
    
    LIST_TRAVERSE(watchlist_head,curr)
    {
	pair = elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,"watchlist_del_all_events","watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner==owner)
	    if (list_remove_elem(watchlist_head,curr)<0)
	    {
		eventlog(eventlog_level_error,"watchlist_del_all_events","could not remove item");
		pair->owner = NULL;
	    }
	    else
		free(pair);
    }
    
    list_purge(watchlist_head);
    
    return 0;
}


extern int watchlist_notify_event(t_account * who, t_watch_event event)
{
    t_elem const * curr;
    t_watch_pair * pair;
    char const *   tname;
    char           msgtemp[62+32]; /* tname + msg */
    
    if (!who)
    {
	eventlog(eventlog_level_error,"watchlist_notify_event","got NULL who");
	return -1;
    }
    if (!(tname = account_get_name(who)))
    {
	eventlog(eventlog_level_error,"watchlist_notify_event","could not get account name");
	return -1;
    }
    switch (event)
    {
    case watch_event_login:
	sprintf(msgtemp,"%.64s has logged in.",tname);
	break;
    case watch_event_logout:
	sprintf(msgtemp,"%.64s has logged out.",tname);
	break;
    case watch_event_joingame:
	sprintf(msgtemp,"%.64s has joined a game.",tname);
	break;
    case watch_event_leavegame:
	sprintf(msgtemp,"%.64s has left a game.",tname);
	break;
    default:
	account_unget_name(tname);
	eventlog(eventlog_level_error,"watchlist_notify_event","got unknown event %u",(unsigned int)event);
	return -1;
    }
    account_unget_name(tname);
    
    LIST_TRAVERSE_CONST(watchlist_head,curr)
    {
	pair = elem_get_data(curr);
	if (!pair) /* should not happen */
	{
	    eventlog(eventlog_level_error,"watchlist_notify_event","watchlist contains NULL item");
	    return -1;
	}
	if (pair->owner && (pair->who==who || !pair->who) && (pair->what&event))
	    message_send_text(pair->owner,message_type_info,pair->owner,msgtemp);
    }
    
    return 0;
}

extern int watchlist_create(void)
{
    if (!(watchlist_head = list_create()))
        return -1;
    return 0;
}


extern int watchlist_destroy(void)
{
    t_elem *       curr;
    t_watch_pair * pair;
    
    LIST_TRAVERSE(watchlist_head,curr)
    {
        pair = elem_get_data(curr);
        if (!pair) /* should not happen */
        {
            eventlog(eventlog_level_error,"watchlist_destroy","watchlist contains NULL item");
            continue;
        }
	
        if (list_remove_elem(watchlist_head,curr)<0)
            eventlog(eventlog_level_error,"watchlist_destroy","could not remove item from list");
        free(pair);
    }
    
    if (list_destroy(watchlist_head)<0)
        return -1;
    watchlist_head = NULL;
    return 0;
}


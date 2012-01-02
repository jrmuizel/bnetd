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
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "common/packet.h"
#include "connection.h"
#include "command.h"
#include "channel.h"
#include "common/eventlog.h"
#include "common/queue.h"
#include "common/network.h"
#include "account.h"
#include "query.h"
#include "common/list.h"
#include "common/bn_type.h"
#include "common/setup_after.h"

static t_list       * query_list_head = NULL;
static t_uint32       current_query_id = 1;
t_query * query_current = NULL;
static int query_ttl_default = 120; /* FIXME: read from config */

static int query_numhandlers = 0;
static t_query_handler * query_handlers = NULL; /* array with (quaery_numhandlers) entries */

extern char const * query_get_rc_str(t_query_rc rc)
{
    switch (rc) {
	case query_rc_keep:
	    return "query_rc_keep";
	case query_rc_delete:
	    return "query_rc_delete";
	case query_rc_error:
	    return "query_rc_error";
	case query_rc_unknown:
	    return "query_rc_unknown";
	default:
	    return "UNKNOWN";
    }
}

extern t_query_handler * query_get_handler(int type)
{
    if (!query_handlers) {
	eventlog(eventlog_level_error,"query_get_handler","query_handlers is NULL");
	return NULL;
    }
    if ((type > 0)&&(type<=query_numhandlers)) {
	if (query_handlers[type-1].valid) {
	    return &query_handlers[type-1];
	}
    } else {
	eventlog(eventlog_level_error,"query_get_handler","got invalid type %d (query_numhandlers=%d)",type,query_numhandlers);
    }
    return NULL;
}

extern int query_register_handler(t_query_func query_handler, t_query_condition default_condition, int always_process, int default_ttl, char const * name)
{
    t_query_handler * temp;
    int i;

    /* first search if there's a free slot in our array */
    if (query_handlers)
    	for (i=0; i<query_numhandlers; i++) {
	    if (!query_handlers[i].valid) {
		/* we really found one */
		query_handlers[i].valid = 1;
		query_handlers[i].query_handler = query_handler;
		query_handlers[i].default_condition = default_condition;
		query_handlers[i].always_process = always_process;
		query_handlers[i].default_ttl = default_ttl;
		if (name)
		    query_handlers[i].name = name;
		else
		    query_handlers[i].name = "(null)";
		eventlog(eventlog_level_debug,"query_register_handler","registered \"%s\" with query type %d (reused) (query_handler=%p,default_condition=%d,always_process=%d,default_ttl=%d)",query_handlers[i].name,i,query_handlers[i].query_handler,query_handlers[i].default_condition,query_handlers[i].always_process,query_handlers[i].default_ttl);
		return i+1; /* zero is invalid we have to add one */
	    }
    	}
    /* we have epand our array */
    if (!(temp = malloc((query_numhandlers+1)*sizeof(t_query_handler)))) {
	eventlog(eventlog_level_error,"query_register_handler","could not allocate memory: %s",strerror(errno));
	return 0;
    }
    /* we don't even think about using realloc() :) */
    if (query_handlers) {
        memcpy(temp,query_handlers,sizeof(t_query_handler)*query_numhandlers);
	free(query_handlers); /* free the old array */
    }
    query_handlers = temp;
    i = query_numhandlers++;
    query_handlers[i].valid = 1;
    query_handlers[i].query_handler = query_handler;
    query_handlers[i].default_condition = default_condition;
    query_handlers[i].always_process = always_process;
    query_handlers[i].default_ttl = default_ttl;
    if (name)
	query_handlers[i].name = name;
    else
	query_handlers[i].name = "(null)";
    eventlog(eventlog_level_debug,"query_register_handler","registered \"%s\" with query type %d (query_handler=%p,default_condition=%d,always_process=%d,default_ttl=%d)",query_handlers[i].name,i,query_handlers[i].query_handler,query_handlers[i].default_condition,query_handlers[i].always_process,query_handlers[i].default_ttl);
    return i+1; /* zero is invalid we have to add one */
}

extern int query_unregister_handler(int type)
{
    if ((type > 0)&&(type<=query_numhandlers)) {
	query_handlers[type-1].valid = 0;
	return 0;
    } else {
	eventlog(eventlog_level_error,"query_unregister_handler","got invalid type %d (query_numhandlers=%d)",type,query_numhandlers);
	return -1;
    }    
}

extern t_query * query_create(int type)
{
    t_query * query;
    t_query_handler * query_handler;
    
    query_handler = query_get_handler(type);
    if (!query_handler) {
	eventlog(eventlog_level_error,"query_create","could not find query handler for querytype=%d",type);
	return NULL;
    }
    query = malloc(sizeof(t_query));
    if (!query) {
	eventlog(eventlog_level_error,"query_create","malloc failed: %s",strerror(errno));
	return NULL;
    }
    query->type = type;
    query->qid = current_query_id++;
    query->processed = 0;
    if (query_handler->default_ttl==QUERY_TTL_DEFAULT) {
	query->ttl = query_ttl_default;
    } else {
	query->ttl = query_handler->default_ttl;
    }
    query->timestamp = time(NULL);
    query->attachments = list_create();
    query->condition = query_handler->default_condition;
    query->handler = (struct t_query_handler *) query_handler;
    list_append_data(query_list_head,query);
    eventlog(eventlog_level_debug,"query_create","created query %u (type=%d:\"%s\")",query_get_qid(query),query_get_type(query),query_get_name(query));
    return query;
}

extern int query_delete(t_query * query)
{
    t_elem * curr;

    if (!query) {
	eventlog(eventlog_level_error,"query_delete","got NULL query");
	return -1;
    }
    eventlog(eventlog_level_debug,"query_delete","deleting query %u",query_get_qid(query));
    if (query->condition!=query_condition_never)
	((t_query_handler *)query->handler)->query_handler(query,query_reason_delete);
    else
	eventlog(eventlog_level_debug,"query_delete","not calling delete routine of query %d",query_get_qid(query));
    LIST_TRAVERSE(query->attachments,curr) {
	t_query_attachment * a = elem_get_data(curr);
	if (a->type == query_attachment_type_packet)
	    packet_del_ref(a->u.packet);
	if (a->type == query_attachment_type_string)
	    free(a->u.string);
	/* FIXME: ungrab account here (del_ref) */
	/* FIXME: ungrab connection here (del_ref) */
	free(a);
	list_remove_data(query->attachments,a);
    }
    list_purge(query->attachments);
    list_destroy(query->attachments);
    list_remove_data(query_list_head,query);
    list_purge(query_list_head);
    free(query);
    return 0;
}

extern int query_exec(t_query * query, t_query_reason reason)
{
    t_query_rc rc;
    t_query * save;
    
    if (!query) {
    	eventlog(eventlog_level_error,"query_exec","got NULL query");
	return -1;
    }
    if (query->condition == query_condition_never) {
	eventlog(eventlog_level_debug,"query_exec","not processing query %d",query_get_qid(query));
	return 0;
    }
    eventlog(eventlog_level_debug,"query_exec","processing query %d (type=%d)",query_get_qid(query),query_get_type(query));
    save = query_current;
    query_current = query;
    rc = ((t_query_handler *)query->handler)->query_handler(query,reason);
    query_current = save;
    eventlog(eventlog_level_debug,"query_exec","query %d (type=%d) finished with rc %d (%s)",query_get_qid(query),query_get_type(query),rc,query_get_rc_str(rc));
    if (rc==query_rc_delete) {
	query_delete(query);
    } else if (rc==query_rc_error) {
	eventlog(eventlog_level_debug,"query_exec","query %u (type=%d) exited with error for reason %d",query_get_qid(query),query_get_type(query),reason);
	rc = ((t_query_handler *)query->handler)->query_handler(query,query_reason_error);
	if ((rc==query_rc_unknown)||(rc=query_rc_delete)||(rc==query_rc_error))
	    query_delete(query);
    } else if (rc==query_rc_unknown) {
	eventlog(eventlog_level_debug,"query_exec","query %u (type=%d) replied unknown for reason %d",query_get_qid(query),query_get_type(query),reason);
    }
    return 0;
}

extern int query_tick(void) {
    int processed_queries = 1;
    t_elem * curr;


    while (processed_queries > 0) {
	processed_queries = 0;
	LIST_TRAVERSE(query_list(),curr) {
	    t_query * q;
	    t_query_rc rc;

	    q = elem_get_data(curr);
	    if (((t_query_handler *)q->handler)->always_process) {
		if (!q->processed) {
		    t_query * save;
		    
		    processed_queries++;
		    q->processed = 1;
    		    if (q->condition == query_condition_never) {
			eventlog(eventlog_level_debug,"query_tick","not processing query %d",query_get_qid(q));
		    } else {
		        eventlog(eventlog_level_debug,"query_tick","processing query %d (type=%d)",query_get_qid(q),query_get_type(q));
			save = query_current;
			query_current = q;
			rc = ((t_query_handler *)q->handler)->query_handler(q,query_reason_tick);
			query_current = save;
			eventlog(eventlog_level_debug,"query_tick","query %d (type=%d) finished with rc %d (%s)",query_get_qid(q),query_get_type(q),rc,query_get_rc_str(rc));
			if (rc==query_rc_delete) {
			    query_delete(q);
			    break;
			} else if (rc==query_rc_error) {
			    eventlog(eventlog_level_debug,"query_tick","query %u (type=%d) exited with error",query_get_qid(q),query_get_type(q));
			    rc = ((t_query_handler *)q->handler)->query_handler(q,query_reason_error);
			    if ((rc==query_rc_unknown)||(rc=query_rc_delete)||(rc==query_rc_error)) {
	  	  		query_delete(q);
				break;
			    }
    			} else if (rc==query_rc_unknown) {
			    eventlog(eventlog_level_debug,"query_tick","query %u (type=%d) replied unknown for reason tick",query_get_qid(q),query_get_type(q));
		        }
		    }
		}
	    }
	    /* check timeout */
	    if (difftime(time(NULL),q->timestamp)>q->ttl) {
	    	/* query timed out */
		t_query * save;

	    	eventlog(eventlog_level_debug,"query_tick","query %u (type=%d) timed out",query_get_qid(q),query_get_type(q));
 		if (q->condition == query_condition_never) {
		    eventlog(eventlog_level_debug,"query_tick","not execution query %d for timeout",query_get_qid(q));
		    query_delete(q);
		    break;
		} else {
		    eventlog(eventlog_level_debug,"query_tick","processing query %d (type=%d)",query_get_qid(q),query_get_type(q));
		    save = query_current;
		    query_current = q;
		    rc = ((t_query_handler *)q->handler)->query_handler(q,query_reason_timeout);
		    query_current = save;
		    eventlog(eventlog_level_debug,"query_tick","query %d (type=%d) timed out with rc %d",query_get_qid(q),query_get_type(q),rc);
		    if (rc==query_rc_keep) {
			eventlog(eventlog_level_debug,"query_tick","keeping query %d (type=%d) after timeout",query_get_qid(q),query_get_type(q));
		    } else {
		 	query_delete(q);
			break;
		    }
		}
	    }
	}
    }
    LIST_TRAVERSE(query_list(),curr) {
	t_query * q;

	q = elem_get_data(curr);
	q->processed = 0;
    }
    query_current = NULL; /* we assume this function is not called from a query */
    return 0;
}

extern int query_answer(t_query * query, t_packet const * packet)
{
    t_packet * dpacket;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_answer","got NULL query");
	return -1;
    }
    if (!packet) {
	eventlog(eventlog_level_error,"query_answer","got NULL packet");
    }
    dpacket = packet_duplicate(packet);
    query_attach_packet(query,"reply",dpacket);
    packet_del_ref(dpacket);
    if (query->condition != query_condition_wait)
    	eventlog(eventlog_level_warn,"query_answer","query %u (type=%d) doesn't wait for a reply",query_get_qid(query),query_get_type(query));
    query_exec(query,query_reason_condition);
    return 0;
}

extern int query_attach(t_query * query, char const * name, void const * data)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_data;
    a->u.data = data;
    list_append_data(query->attachments,a);
    return 0;
}

extern int query_attach_num(t_query * query, char const * name, int num)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach_num","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach_num","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach_num","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_num;
    a->u.num = num;
    list_append_data(query->attachments,a);
    return 0;
}

extern int query_attach_string(t_query * query, char const * name, char const * str)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach_string","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach_string","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach_string","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_string;
    a->u.string = strdup(str);
    list_append_data(query->attachments,a);
    return 0;
}

extern int query_attach_packet(t_query * query, char const * name, t_packet * packet)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach_packet","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach_packet","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach_packet","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_packet;
    a->u.packet = packet;
    packet_add_ref(packet);
    list_append_data(query->attachments,a);
    return 0;
}

extern int query_attach_packet_const(t_query * query, char const * name, t_packet const * packet)
{
    t_packet * dpacket;

    dpacket = packet_duplicate(packet);
    query_attach_packet(query,name,dpacket);
    packet_del_ref(dpacket);
    return 0;
}

extern int query_attach_account(t_query * query, char const * name, t_account * account)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach_account","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach_account","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach_account","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_account;
    a->u.account = account;
    /* FIXME: grab account here (add_ref) */
    list_append_data(query->attachments,a);
    return 0;
}

extern int query_attach_conn(t_query * query, char const * name, t_connection * conn)
{
    t_query_attachment * a;

    if (!query) {
	eventlog(eventlog_level_error,"query_attach_conn","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_attach_conn","got NULL name");
	return -1;
    }
    a = malloc(sizeof(t_query_attachment));
    if (!a) {
	eventlog(eventlog_level_error,"query_attach_conn","malloc failed: %s",strerror(errno));
	return -1;
    }
    a->name = name;
    a->type = query_attachment_type_conn;
    a->u.conn = conn;
    /* FIXME: grab connection here (add_ref) */
    list_append_data(query->attachments,a);
    return 0;
}

extern t_query_attachment * query_get_pure_attachment(t_query const * query, char const * name)
{
    t_elem * curr;
    if (!query) {
	eventlog(eventlog_level_error,"query_get_pure_attachment","got NULL query");
	return NULL;
    }
    LIST_TRAVERSE(query->attachments,curr) {
	t_query_attachment * a = elem_get_data(curr);
	if (strcmp(name,a->name)==0) {
		return a;
	}
    }
    return NULL;
}

extern void const * query_get_attachment(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment","got NULL query");
	return NULL;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment","got NULL name");
	return NULL;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_data)
    	    eventlog(eventlog_level_warn,"query_get_attachment","returning wrong attachment type");
	return a->u.data;
    }
    return NULL;
}

extern int query_get_attachment_num(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment_num","got NULL query");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment_num","got NULL name");
	return -1;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_num)
    	    eventlog(eventlog_level_warn,"query_get_attachment_num","returning wrong attachment type");
	return a->u.num;
    }
    return -1;
}

extern char * query_get_attachment_string(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment_string","got NULL query");
	return NULL;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment_string","got NULL name");
	return NULL;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_string)
    	    eventlog(eventlog_level_warn,"query_get_attachment_string","returning wrong attachment type");
	return a->u.string;
    }
    return NULL;
}

extern t_packet * query_get_attachment_packet(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment_packet","got NULL query");
	return NULL;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment_packet","got NULL name");
	return NULL;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_packet)
    	    eventlog(eventlog_level_warn,"query_get_attachment_packet","returning wrong attachment type");
	return a->u.packet;
    }
    return NULL;
}

extern t_account * query_get_attachment_account(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment_account","got NULL query");
	return NULL;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment_account","got NULL name");
	return NULL;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_account)
    	    eventlog(eventlog_level_warn,"query_get_attachment_account","returning wrong attachment type");
	return a->u.account;
    }
    return NULL;
}

extern t_connection * query_get_attachment_conn(t_query const * query, char const * name)
{
    t_query_attachment * a;
    
    if (!query) {
	eventlog(eventlog_level_error,"query_get_attachment_conn","got NULL query");
	return NULL;
    }
    if (!name) {
	eventlog(eventlog_level_error,"query_get_attachment_conn","got NULL name");
	return NULL;
    }
    a = query_get_pure_attachment(query,name);
    if (a) {
    	if (a->type!=query_attachment_type_conn)
    	    eventlog(eventlog_level_warn,"query_get_attachment_conn","returning wrong attachment type");
	return a->u.conn;
    }
    return NULL;
}

extern t_uint32 query_get_qid(t_query const * query)
{
    if (!query) {
	eventlog(eventlog_level_error,"query_get_qid","got NULL query");
	return 0;
    }
    return query->qid;
}

extern int query_set_qid(t_query * query, t_uint32 qid)
{
    if (!query) {
	eventlog(eventlog_level_error,"query_set_qid","got NULL query");
	return -1;
    }
    query->qid = qid;
    return 0;
}

extern int query_get_type(t_query const * query)
{
    if (!query) {
	eventlog(eventlog_level_error,"query_get_type","got NULL query");
	return 0;
    }
    return query->type;    
}

extern int query_set_type(t_query * query, int type)
{
    t_query_handler * qh;

    if (!query) {
	eventlog(eventlog_level_error,"query_set_type","got NULL query");
	return -1;
    }
    qh = query_get_handler(type);
    if (!qh) {
	eventlog(eventlog_level_error,"query_set_type","could not find query handler for type %d",type);
	return -1;
    }
    
    query->type = type;
    query->handler = (struct t_query_handler *)qh;
    query->condition = qh->default_condition;
    query->timestamp = time(NULL);
    if (qh->default_ttl==QUERY_TTL_DEFAULT) {
	query->ttl = query_ttl_default;
    } else {
	query->ttl = qh->default_ttl;
    }
    eventlog(eventlog_level_debug,"query_set_type","setting type for query %d to %d:\"%s\"",query_get_qid(query),query_get_type(query),query_get_name(query));    
    return 0;
}

extern char const * query_get_name(t_query const * query)
{
    if (!query) {
	eventlog(eventlog_level_error,"query_get_name","got NULL query");
	return "(null)"; /* we want to be nice ... some printf()s can't handle NULL */
    }
    if ((!query_handlers)||(query_get_type(query)<1)||(query_get_type(query)>query_numhandlers)) {
	eventlog(eventlog_level_error,"query_get_name","query handler for query type %d not found",query_get_type(query));
	return "(null)";
    }
    return query_handlers[query_get_type(query)-1].name;
}

extern t_query * query_find_byqid(t_uint32 qid)
{
    t_elem * curr;

    LIST_TRAVERSE(query_list_head,curr) {
	t_query *query = elem_get_data(curr);
	if (query->qid == qid)
	    return query;
    }
    return NULL;
}

/* Finds the first query ... */
extern t_query * query_find_byattachednum(char const * name, int value)
{
    t_elem * curr;

    LIST_TRAVERSE(query_list_head,curr) {
	t_query *query = elem_get_data(curr);
	t_query_attachment *a = query_get_pure_attachment(query,name);
	if (a) {
	    if ((a->type==query_attachment_type_num)&&(a->u.num==value))
		return query;
	}
    }
    return NULL;
}

extern t_list * query_list(void)
{
    return query_list_head;
}

extern void query_init(void) {
    query_list_head = list_create();
    eventlog(eventlog_level_debug,"query_init","query initialized");
}

extern void query_destroy(void) {
    t_elem * curr;
    
    LIST_TRAVERSE(query_list_head,curr) {
	t_query * q = elem_get_data(curr);

    	query_delete(q);
    }
    list_purge(query_list_head);
    list_destroy(query_list_head);
    eventlog(eventlog_level_debug,"query_destroy","query finished");
}


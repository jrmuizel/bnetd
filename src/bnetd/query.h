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
#ifndef INCLUDED_QUERY_TYPES
#define INCLUDED_QUERY_TYPES

#ifdef JUST_NEED_TYPES
# include "connection.h"
# include "account.h"
# include "common/packet.h"
# include "common/list.h"
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
#else
# define JUST_NEED_TYPES
# include "connection.h"
# include "account.h"
# include "common/packet.h"
# include "common/list.h"
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
# undef JUST_NEED_TYPES
#endif


/* What are BITS queries?
 * 
 * BITS queries are used to wait for ceratin events to occure.
 * Such events could be arriving replies for certain requests,
 * a 'tick' or everything else. What I call a 'tick' here is
 * just a cycle in the bnetd event loop. A query could therefore
 * be used as a dynamic addition to the eventloop.
 * Each query gets an unique query id (qid) when it is created.
 * This qid can be used to assign reply packets to their
 * corresponding request. (eg. "notify query with qid x that
 * the following reply arrived")
 * A query mostly has an query handler which is called upon these
 * events. Note: A query doesn't need to have an query handler 
 * if it's only there to store data. 
 */

typedef enum {
    query_condition_wait,    /* waits for an answer */
    query_condition_never,   /* query handler is probably NULL */
    query_condition_none     /* no certain condition */
} t_query_condition;

typedef enum {
    query_reason_init = 0,   /* not yet implemented */
    query_reason_timeout,    /* query has timed out */
    query_reason_tick,       /* invoked on every loop, if set by query handler*/
    query_reason_condition,  /* a certain condition is set (eg. reply to a request) */
    query_reason_error,      /* an error occured */
    query_reason_delete      /* query will be deleted: free any attached data, which isn't freed automatically */
} t_query_reason;

typedef enum {
    query_attachment_type_data,       /* (void const *) */
    query_attachment_type_num,        /* (int) */
    query_attachment_type_string,     /* (char const *) */
    query_attachment_type_packet,     /* (t_packet *) */
    query_attachment_type_account,    /* (t_account *) */
    query_attachment_type_conn        /* (t_connection *) */
} t_query_attachment_type;

typedef struct { /* Data attached to a query */
    char const * name;                 /* name of the object */
    t_query_attachment_type type;      /* data type of the object */
    union {
	void const * data;
	int num;
	char * string;
	t_packet * packet;
	t_account * account;
	t_connection * conn;
    } u;
} t_query_attachment;

typedef struct {
    t_uint32	            qid;           /* query id */
    int                     type;          /* type of the bits request */
    int                     ttl;           /* "Time To Life" for that query */
    time_t                  timestamp;     /* timestamp from the creation/type change*/
    int	                    processed;	   /* 1==processed; 0==not processed */
    t_query_condition       condition;     /* when should the query be executed */
    struct t_query_handler  * handler;     /* a quick reference to the query handler */
    t_list                  * attachments;   /* attachments/attributes */
} t_query;

typedef enum {
    query_rc_keep = 0, /* keep the query in the query list */
    query_rc_delete,   /* remove the query */
    query_rc_error,    /* an error occured: call error subroutine */
    query_rc_unknown   /* query handler doesn't know how to react for this reason */
} t_query_rc;

typedef t_query_rc (*t_query_func)(t_query *q, t_query_reason reason);

typedef struct {
	int valid; /* non-zero: handler is valid. zero: handler is invalid */
	t_query_func query_handler;
	t_query_condition default_condition;
	int always_process; /* non-zero: always process the query on each round (tick reason) */
	int default_ttl; /* the default timeout for that query type */
	char const * name; /* for debugging purposes */
} t_query_handler;


#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_QUERY_PROTOS
#define INCLUDED_BITS_QUERY_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#include "common/packet.h"
#undef JUST_NEED_TYPES

#define QUERY_TTL_DEFAULT 0
#define QUERY_TTL_INFINITE -1

extern t_query * query_current;

extern char const * query_get_rc_str(t_query_rc rc);
extern t_query_handler * query_get_handler(int type);
extern int query_register_handler(t_query_func query_handler, t_query_condition default_condition, int always_process, int default_ttl, char const * name);
extern int query_unregister_handler(int type);
extern t_query * query_create(int type);
extern int query_delete(t_query * query);
extern int query_exec(t_query * query, t_query_reason reason);
extern int query_tick(void);
extern int query_answer(t_query * query, t_packet const * packet);
extern int query_attach(t_query * query, char const * name, void const * data);
extern int query_attach_num(t_query * query, char const * name, int num);
extern int query_attach_string(t_query * query, char const * name, char const * str);
extern int query_attach_packet(t_query * query, char const * name, t_packet * packet);
extern int query_attach_packet_const(t_query * query, char const * name, t_packet const * packet);
extern int query_attach_account(t_query * query, char const * name, t_account * account);
extern int query_attach_conn(t_query * query, char const * name, t_connection * conn);
extern t_query_attachment * query_get_pure_attachment(t_query const * query, char const * name);
extern void const * query_get_attachment(t_query const * query, char const * name);
extern int query_get_attachment_num(t_query const * query, char const * name);
extern char * query_get_attachment_string(t_query const * query, char const * name);
extern t_packet * query_get_attachment_packet(t_query const * query, char const * name);
extern t_account * query_get_attachment_account(t_query const * query, char const * name);
extern t_connection * query_get_attachment_conn(t_query const * query, char const * name);
extern t_uint32 query_get_qid(t_query const * query);
extern int query_set_qid(t_query * query, t_uint32 qid);
extern int query_get_type(t_query const * query);
extern int query_set_type(t_query * query, int type);
extern char const * query_get_name(t_query const * query);

extern t_query * query_find_byqid(t_uint32 qid);
extern t_query * query_find_byattachednum(char const * name, int value);
extern t_list * query_list(void);

extern void query_init(void);
extern void query_destroy(void);

#endif
#endif

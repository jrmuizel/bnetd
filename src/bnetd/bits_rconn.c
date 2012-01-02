/*
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
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
#include "connection.h"
#include "common/eventlog.h"
#include "bits.h"
#include "bits_rconn.h"
#include "bits_login.h"
#include "account.h"
#include "common/bn_type.h"
#include "common/tag.h"
#include "common/setup_after.h"

extern t_connection * bits_rconn_create(unsigned int latency, unsigned int flags, unsigned int sessionid)
{
	t_connection * c;

	c = bits_rconn_create_by_sessionid(sessionid);
	c->flags = flags;
	c->latency = latency;
	/* FIXME: need account simulation */
	return c;
}

extern t_connection * bits_rconn_create_by_sessionid(unsigned int sessionid)
{
    t_bits_loginlist_entry * lle;
    t_connection * c;
    char ct[5];
    char temp[32];
    t_account * account;

    lle = bits_loginlist_bysessionid(sessionid);
    if (!lle) {
	eventlog(eventlog_level_error,"bits_rconn_create_by_sessionid","could not find loginlist entry wit sessionid 0x%08x",sessionid);
	return NULL;
    }
    c = malloc(sizeof(t_connection));
    if (!c) {
	eventlog(eventlog_level_error,"bits_rconn_create","malloc failed: %s",strerror(errno));
	return NULL;
    }
    memset(c,0,sizeof(t_connection));
    memcpy(ct,lle->clienttag,4);
    ct[4] = '\0';
    conn_set_clienttag(c,ct);
    c->class = conn_class_remote;
    c->sessionid = sessionid;
    c->udp_addr = lle->game_addr;
    c->tcp_addr = lle->game_addr; /* just a guess ... */
    c->udp_port = lle->game_port;
    c->flags = 0;
    c->latency = 0;
    conn_set_botuser(c,lle->chatname);
    c->state = conn_state_loggedin;
    sprintf(temp,UID_FORMAT,lle->uid);
    account = accountlist_find_account(temp);
    if (account_is_ready_for_use(account)) {
	c->account = account;
    } else {
	c->account = NULL;
    }
    /* FIXME: grab account ? */
    return c;
}

extern int bits_rconn_set_game(t_connection * c, t_game * g)
{
    if (!c) {
	eventlog(eventlog_level_error,"bits_rconn_set_game","got NULL connection");
	return -1;
    }
    c->game = g;
    return 0;
}

extern int bits_rconn_destroy(t_connection * c)
{
	/* FIXME: use conn_destroy instead? */

	if (!c) {
		eventlog(eventlog_level_error,"bits_rconn_destroy","got NULL connection");
		return -1;
	}
	if (conn_get_class(c)!=conn_class_remote) {
		eventlog(eventlog_level_error,"bits_rconn_destroy","wrong connection class");
		return -1;
	}
	if (c->clienttag) free((void *) c->clienttag); /* avoid warning */
	if (c->botuser) free((void *) c->botuser); /* avoid warning */
	free(c);
	return 0;
}


#endif

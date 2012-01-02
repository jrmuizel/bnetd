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
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/char_bit.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h> /* needed to include netinet/in.h */
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h> /* FIXME: probably not needed... do some systems put types in here or someth
ing? */
#endif
#include "compat/psock.h"

#include "d2gs.h"
#include "game.h"
#include "net.h"
#include "bit.h"
#include "prefs.h"
#include "connection.h"
#include "common/introtate.h"
#include "common/addr.h"
#include "common/list.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static t_list		* d2gslist_head=NULL;
static unsigned int	d2gs_id=0;
static unsigned int	total_d2gs=0;

extern t_list *	d2gslist(void)
{
	return d2gslist_head;
}

extern int d2gslist_create(void)
{
	if (!(d2gslist_head=list_create())) return -1;
	return d2gslist_reload(prefs_get_d2gs_list());
}

extern int d2gslist_reload(char const * gslist)
{
	char		* templist;
	char		* s, * temp;
	t_d2gs		* gs;
	unsigned int	ip;

	if (!d2gslist_head) return -1;

	BEGIN_LIST_TRAVERSE_DATA(d2gslist_head,gs)
	{
		BIT_CLR_FLAG(gs->flag, D2GS_FLAG_VALID);
	}
	END_LIST_TRAVERSE_DATA()

/* FIXME: use addr.h addrlist code */
	if (!(templist=strdup(gslist))) return -1;
	temp=templist;
	while ((s=strsep(&temp, ","))) {
		if ((ip=net_inet_addr(s))==~0UL) {
			log_error("got bad ip address %s",s);
			continue;
		}
		if (!(gs=d2gslist_find_gs_by_ip(ntohl(ip)))) {
			gs=d2gs_create(s);
		}
		if (gs) BIT_SET_FLAG(gs->flag, D2GS_FLAG_VALID);
	}
	free(templist);

	BEGIN_LIST_TRAVERSE_DATA(d2gslist_head,gs)
	{
		if (!BIT_TST_FLAG(gs->flag, D2GS_FLAG_VALID)) {
			d2gs_destroy(gs);
		}
	}
	END_LIST_TRAVERSE_DATA()
	return 0;
}

extern int d2gslist_destroy(void)
{
	t_d2gs	* gs;

	BEGIN_LIST_TRAVERSE_DATA_CONST(d2gslist_head,gs)
	{
		d2gs_destroy(gs);
	}
	END_LIST_TRAVERSE_DATA_CONST()

	if (list_destroy(d2gslist_head)<0) {
		log_error("error destroy d2gs list");
		return -1;
	}
	d2gslist_head=NULL;
	return 0;
}

extern t_d2gs * d2gslist_find_gs_by_ip(unsigned int ip)
{
	t_d2gs	* gs;

	BEGIN_LIST_TRAVERSE_DATA_CONST(d2gslist_head,gs)
	{
		if (gs->ip==ip) return gs;
	}
	END_LIST_TRAVERSE_DATA_CONST()
	return NULL;
}

extern t_d2gs * d2gslist_find_gs(unsigned int id)
{
	t_d2gs	* gs;

	BEGIN_LIST_TRAVERSE_DATA_CONST(d2gslist_head,gs)
	{
		if (gs->id==id) return gs;
	}
	END_LIST_TRAVERSE_DATA_CONST()
	return NULL;
}

extern t_d2gs * d2gs_create(char const * ipaddr)
{
	t_d2gs 	* gs;
	int	ip;

	ASSERT(ipaddr,NULL);
	if ((ip=net_inet_addr(ipaddr))==INADDR_NONE) {
		log_error("got bad ip address %s",ipaddr);
		return NULL;
	}
	if (d2gslist_find_gs_by_ip(ntohl(ip))) {
		log_error("game server %s already in list",ipaddr);
		return NULL;
	}
	if (!(gs=malloc(sizeof(t_d2gs)))) {;
		log_error("error allocate memory");
		return NULL;
	}
	gs->ip=ntohl(ip);
	gs->id=++d2gs_id;
	gs->active=0;
	gs->token=0;
	gs->state=d2gs_state_none;
	gs->gamenum=0;
	gs->maxgame=0;
	gs->connection=NULL;

	if (list_append_data(d2gslist_head,gs)<0) {
		log_error("error add gs to list");
		free(gs);
		return NULL;
	}
	log_info("added game server %s (id: %d) to list",ipaddr,gs->id);
	return gs;
}

extern int d2gs_destroy(t_d2gs * gs)
{
	ASSERT(gs,-1);
	if (list_remove_data(d2gslist_head,gs)<0) {
		log_error("error remove gs from list");
		return -1;
	}
	if (gs->active && gs->connection) {
		conn_set_state(gs->connection, conn_state_destroy);
		d2gs_deactive(gs, gs->connection);
	}
	log_info("removed game server %s (id: %d) from list",addr_num_to_ip_str(gs->ip),gs->id);
	free(gs);
	return 0;
}

extern t_d2gs * d2gslist_get_server_by_id(unsigned int id)
{
	t_d2gs	* gs;

	BEGIN_LIST_TRAVERSE_DATA_CONST(d2gslist_head,gs)
	{
		if (gs->id==id) return gs;
	}
	END_LIST_TRAVERSE_DATA_CONST()
	return NULL;
}
	  
extern t_d2gs * d2gslist_choose_server(void)
{
	t_d2gs			* gs;
	t_d2gs			* ogs;
	unsigned int		percent;
	unsigned int		min_percent=100;

	ogs=NULL;
	BEGIN_LIST_TRAVERSE_DATA_CONST(d2gslist_head,gs)
	{
		if (!gs->active) continue;
		if (!gs->connection) continue;
		if (gs->state!=d2gs_state_authed) continue;
		if (!gs->maxgame) continue;
		if (gs->gamenum>=gs->maxgame) continue;
		percent=100*gs->gamenum/gs->maxgame;
		if (percent<min_percent) {
			min_percent=percent;
			ogs=gs;
		}
	}
	END_LIST_TRAVERSE_DATA_CONST()
	return ogs;
}

extern int d2gs_set_state(t_d2gs * gs, t_d2gs_state state)
{
	ASSERT(gs,-1);
	gs->state=state;
	return 0;
}

extern t_d2gs_state d2gs_get_state(t_d2gs const * gs)
{
	ASSERT(gs,d2gs_state_none);
	return gs->state;
}

extern int d2gs_add_gamenum(t_d2gs * gs, int number)
{
	ASSERT(gs,-1);
	gs->gamenum += number;
	return 0;
}

extern unsigned int d2gs_get_gamenum(t_d2gs const * gs)
{
	ASSERT(gs,0);
	return gs->gamenum;
}


extern int d2gs_set_maxgame(t_d2gs * gs,unsigned int maxgame)
{
	ASSERT(gs,-1);
	gs->maxgame=maxgame;
	return 0;
}

extern unsigned int d2gs_get_maxgame(t_d2gs const * gs)
{
	ASSERT(gs,0);
	return gs->maxgame;
}

extern unsigned int d2gs_get_id(t_d2gs const * gs)
{
	ASSERT(gs,0);
	return gs->id;
}

extern unsigned int d2gs_get_ip(t_d2gs const * gs)
{
	ASSERT(gs,0);
	return gs->ip;
}

extern unsigned int d2gs_get_token(t_d2gs const * gs)
{
	return gs->token;
}

extern unsigned int d2gs_add_token(t_d2gs * gs)
{
	return ++(gs->token);
}

extern t_connection * d2gs_get_connection(t_d2gs const * gs)
{
	ASSERT(gs,NULL);
	return gs->connection;
}

extern int d2gs_active(t_d2gs * gs, t_connection * c)
{
	ASSERT(gs,-1);
	ASSERT(c,-1);

	if (gs->active && gs->connection) {
		log_warn("game server %d is already actived, deactive previous connection first",gs->id);
		d2gs_deactive(gs, gs->connection);
	}
	total_d2gs++;
	log_info("game server %s (id: %d) actived (%d total)",addr_num_to_addr_str(conn_get_addr(c),\
		conn_get_port(c)),gs->id,total_d2gs);
	gs->state=d2gs_state_authed;
	gs->connection=c;
	gs->active=1;
	gs->gamenum=0;
	gs->maxgame=0;
	return 0;
}

extern int d2gs_deactive(t_d2gs * gs, t_connection * c)
{
	t_game * game;

	ASSERT(gs,-1);
	if (!gs->active || !gs->connection) {
		log_warn("game server %d is not actived yet", gs->id);
		return -1;
	}
	if (gs->connection != c) {
		log_debug("game server %d connection mismatch,ignore it", gs->id);
		return 0;
	}
	total_d2gs--;
	log_info("game server %s (id: %d) deactived (%d left)",addr_num_to_addr_str(conn_get_addr(gs->connection),conn_get_port(gs->connection)),gs->id,total_d2gs);
	gs->state=d2gs_state_none;
	gs->connection=NULL;
	gs->active=0;
	gs->maxgame=0;
	log_info("destroying all games on game server %d",gs->id);
	BEGIN_LIST_TRAVERSE_DATA(gamelist(),game)
	{
		if (game_get_d2gs(game)==gs) game_destroy(game);
	}
	END_LIST_TRAVERSE_DATA()
	if (gs->gamenum!=0) {
		log_error("game server %d deactived but still with games left",gs->id);
	}
	gs->gamenum=0;
	return 0;
}

extern unsigned int d2gs_calc_checksum(t_connection * c)
{
	unsigned int	sessionnum, checksum, port, addr;
	unsigned int	i, len, ch;
	char const	* realmname;
	char const	* password;

	ASSERT(c,0);
	sessionnum=conn_get_sessionnum(c);
	checksum=prefs_get_d2gs_checksum();
	port=conn_get_port(c);
	addr=conn_get_addr(c);
	realmname=prefs_get_realmname();
	password=prefs_get_d2gs_password();

	len=strlen(realmname);
	for (i=0; i<len ; i++) {
		ch = (unsigned int)(unsigned char) realmname[i];
		checksum ^= ROTL(sessionnum,i, sizeof(unsigned int) * CHAR_BIT);
		checksum ^= ROTL(port , ch, sizeof(unsigned int) * CHAR_BIT);
	}
	len=strlen(password);
	for (i=0; i<len ; i++) {
		ch = (unsigned int)(unsigned char) password[i];
		checksum ^= ROTL(sessionnum,i, sizeof(unsigned int) * CHAR_BIT);
		checksum ^= ROTL(port , ch, sizeof(unsigned int) * CHAR_BIT);
	}
	checksum ^= addr;
	return checksum;
}

extern int d2gs_keepalive(void)
{
	t_packet	* packet;
	t_d2gs *	gs;

	if (!(packet=packet_create(packet_class_d2gs))) {
		log_error("error creating packet");
		return -1;
	}
	packet_set_size(packet,sizeof(t_d2cs_d2gs_echoreq));
	packet_set_type(packet,D2CS_D2GS_ECHOREQ);
	BEGIN_LIST_TRAVERSE_DATA(d2gslist_head,gs)
	{
		if (gs->active && gs->connection) {
			queue_push_packet(conn_get_out_queue(gs->connection),packet);
		}
	}
	END_LIST_TRAVERSE_DATA()
	packet_del_ref(packet);
	return 0;
}

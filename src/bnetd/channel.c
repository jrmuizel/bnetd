/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#define CHANNEL_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
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
#include "compat/strrchr.h"
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
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
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "connection.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "message.h"
#include "account.h"
#include "common/util.h"
#include "prefs.h"
#include "common/token.h"
#ifdef WITH_BITS
# include "query.h"
# include "bits.h"
# include "bits_chat.h"
# include "bits_rconn.h"
# include "bits_query.h"
# include "bits_packet.h"
# include "bits_login.h"
# include "common/bn_type.h"
# include "common/packet.h"
# include "common/queue.h"
#endif
#include "channel.h"
#include "common/setup_after.h"


static t_list * channellist_head=NULL;

static t_channelmember * memberlist_curr=NULL;
static int totalcount=0;


static int channellist_load_permanent(char const * filename);
#ifndef WITH_BITS
extern int channellist_reload(void);
#endif
static t_channel * channellist_find_channel_by_fullname(char const * name);
static char * channel_format_name(char const * sname, char const * country, char const * realmname, unsigned int id);


extern t_channel * channel_create(char const * fullname, char const * shortname, char const * clienttag, int permflag, int botflag, int operflag, int logflag, char const * country, char const * realmname, int maxmembers)
{
    t_channel * channel;
    
    if (!fullname)
    {
        eventlog(eventlog_level_error,"channel_create","got NULL fullname");
	return NULL;
    }
    if (fullname[0]=='\0')
    {
        eventlog(eventlog_level_error,"channel_create","got empty fullname");
	return NULL;
    }
    if (shortname && shortname[0]=='\0')
    {
        eventlog(eventlog_level_error,"channel_create","got empty shortname");
	return NULL;
    }
    if (clienttag && strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"channel_create","client tag has bad length (%u chars)",strlen(clienttag));
	return NULL;
    }
    
    /* non-permanent already checks for this in conn_set_channel */
    if (permflag)
    {
	if (channellist_find_channel_by_fullname(fullname))
	{
	    eventlog(eventlog_level_error,"channel_create","could not create duplicate permanent channel (fullname \"%s\")",fullname);
	    return NULL;
	}
    }
    
    if (!(channel = malloc(sizeof(t_channel))))
    {
        eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel");
        return NULL;
    }
    
    if (permflag)
    {
	channel->flags = channel_flags_public;
	if (clienttag && maxmembers!=-1) /* approximation.. we want things like "Starcraft USA-1" */
	    channel->flags |= channel_flags_system;
    }
    else
	channel->flags = channel_flags_none;
    
    eventlog(eventlog_level_debug,"channel_create","creating new channel \"%s\" shortname=%s%s%s clienttag=%s%s%s country=%s%s%s realm=%s%s%s",fullname,
	     shortname?"\"":"(", /* all this is doing is printing the name in quotes else "none" in parens */
	     shortname?shortname:"none",
	     shortname?"\"":")",
	     clienttag?"\"":"(",
	     clienttag?clienttag:"none",
	     clienttag?"\"":")",
	     country?"\"":"(",
             country?country:"none",
	     country?"\"":")",
	     realmname?"\"":"(",
             realmname?realmname:"none",
             realmname?"\"":")");

    
    if (!(channel->name = strdup(fullname)))
    {
        eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->name");
	free(channel);
	return NULL;
    }
    
    if (!shortname)
	channel->shortname = NULL;
    else
	if (!(channel->shortname = strdup(shortname)))
	{
	    eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->shortname");
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
    
    if (clienttag)
    {
	if (!(channel->clienttag = strdup(clienttag)))
	{
	    eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel->clienttag");
	    if (channel->shortname)
		free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
    }
    else
	channel->clienttag = NULL;

    if (country)
    {
	if (!(channel->country = strdup(country)))
	{
            eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->country");
	    if (channel->clienttag)
	        free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
	        free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
    	}
    }
    else
	channel->country = NULL;
	
    if (realmname)
    {
	if (!(channel->realmname = strdup(realmname)))
        {
            eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->realmname");
	    if (channel->country)
	    	free((void *)channel->country); /* avoid warning */
	    if (channel->clienttag)
	        free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
	        free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
    	}
    }
    else
        channel->realmname=NULL;
	
    if (!(channel->banlist = list_create()))
    {
	eventlog(eventlog_level_error,"channel_create","could not create list");
	if (channel->country)
	    free((void *)channel->country); /* avoid warning */
        if (channel->realmname)
            free((void *)channel->realmname); /*avoid warining */
	if (channel->clienttag)
	    free((void *)channel->clienttag); /* avoid warning */
	if (channel->shortname)
	    free((void *)channel->shortname); /* avoid warning */
	free((void *)channel->name); /* avoid warning */
	free(channel);
	return NULL;
    }
    
    totalcount++;
    if (totalcount==0) /* if we wrap (yeah right), don't use id 0 */
	totalcount = 1;
    channel->id = totalcount;
    channel->maxmembers = maxmembers;
    channel->currmembers = 0;
    channel->memberlist = NULL;
    
#ifdef WITH_BITS
    channel->opr        = 0;
    channel->next_opr   = 0;
#else
    channel->opr        = NULL;
    channel->next_opr   = NULL;
#endif
    
#ifdef WITH_BITS
    if (bits_master)
	send_bits_chat_channellist_add(channel,NULL);
    channel->ref = 0;
#endif
    
    channel->permanent  = permflag;
    channel->allowbots  = botflag;
    channel->allowopers = operflag;
    
    if (logflag)
    {
	time_t      now;
	struct tm * tmnow;
	char        dstr[64];
	char        timetemp[CHANLOG_TIME_MAXLEN];
	
	now = time(NULL);
	
	if (!(tmnow = gmtime(&now)))
	    dstr[0] = '\0';
	else
	    sprintf(dstr,"%04d%02d%02d%02d%02d%02d",
		    1900+tmnow->tm_year,
		    tmnow->tm_mon+1,
		    tmnow->tm_mday,
		    tmnow->tm_hour,
		    tmnow->tm_min,
		    tmnow->tm_sec);
	
	if (!(channel->logname = malloc(strlen(prefs_get_chanlogdir())+9+strlen(dstr)+1+6+1))) /* dir + "/chanlog-" + dstr + "-" + id + NUL */
	{
	    eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel->logname");
	    list_destroy(channel->banlist);
	    if (channel->country)
		free((void *)channel->country); /* avoid warning */
            if (channel->realmname)
                free((void *) channel->realmname); /* avoid warning */
	    if (channel->clienttag)
		free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
		free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
	sprintf(channel->logname,"%s/chanlog-%s-%06u",prefs_get_chanlogdir(),dstr,channel->id);
	
	if (!(channel->log = fopen(channel->logname,"w")))
	    eventlog(eventlog_level_error,"channel_create","could not open channel log \"%s\" for writing (fopen: %s)",channel->logname,strerror(errno));
	else
	{
	    fprintf(channel->log,"name=\"%s\"\n",channel->name);
	    if (channel->shortname)
		fprintf(channel->log,"shortname=\"%s\"\n",channel->shortname);
	    else
		fprintf(channel->log,"shortname=none\n");
	    fprintf(channel->log,"permanent=\"%s\"\n",channel->permanent?"true":"false");
	    fprintf(channel->log,"allowbotse=\"%s\"\n",channel->allowbots?"true":"false");
	    fprintf(channel->log,"allowopers=\"%s\"\n",channel->allowopers?"true":"false");
	    if (channel->clienttag)
		fprintf(channel->log,"clienttag=\"%s\"\n",channel->clienttag);
	    else
		fprintf(channel->log,"clienttag=none\n");
	    
	    if (tmnow)
		strftime(timetemp,sizeof(timetemp),CHANLOG_TIME_FORMAT,tmnow);
	    else
		strcpy(timetemp,"?");
	    fprintf(channel->log,"created=\"%s\"\n\n",timetemp);
	    fflush(channel->log);
	}
    }
    else
    {
	channel->logname = NULL;
	channel->log = NULL;
    }
    
    if (list_prepend_data(channellist_head,channel)<0)
    {
        eventlog(eventlog_level_error,"channel_create","could not prepend temp");
	if (channel->log)
	    if (fclose(channel->log)<0)
		eventlog(eventlog_level_error,"channel_create","could not close channel log \"%s\" after writing (fclose: %s)",channel->logname,strerror(errno));
	if (channel->logname)
	    free((void *)channel->logname); /* avoid warning */
	list_destroy(channel->banlist);
	if (channel->country)
	    free((void *)channel->country); /* avoid warning */
        if (channel->realmname)
            free((void *) channel->realmname); /* avoid warning */
	if (channel->clienttag)
	    free((void *)channel->clienttag); /* avoid warning */
	if (channel->shortname)
	    free((void *)channel->shortname); /* avoid warning */
	free((void *)channel->name); /* avoid warning */
	free(channel);
        return NULL;
    }
    
    eventlog(eventlog_level_debug,"channel_create","channel created successfully");
    return channel;
}


extern int channel_destroy(t_channel * channel)
{
    t_elem * ban;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_destroy","got NULL channel");
	return -1;
    }
    
    if (channel->memberlist)
    {
	eventlog(eventlog_level_debug,"channel_destroy","channel is not empty, deferring");
	channel->permanent = 0; /* make it go away when the last person leaves */
	return -1;
    }
    
#ifdef WITH_BITS
    send_bits_chat_channellist_del(channel_get_channelid(channel));
#endif
    
    if (list_remove_data(channellist_head,channel)<0)
    {
        eventlog(eventlog_level_error,"channel_destroy","could not remove item from list");
        return -1;
    }
    
    eventlog(eventlog_level_info,"channel_destroy","destroying channel \"%s\"",channel->name);
    
    LIST_TRAVERSE(channel->banlist,ban)
    {
	char const * banned;
	
	if (!(banned = elem_get_data(ban)))
	    eventlog(eventlog_level_error,"channel_destroy","found NULL name in banlist");
	else
	    free((void *)banned); /* avoid warning */
	if (list_remove_elem(channel->banlist,ban)<0)
	    eventlog(eventlog_level_error,"channel_destroy","unable to remove item from list");
    }
    list_destroy(channel->banlist);
    
    if (channel->log)
    {
	time_t      now;
	struct tm * tmnow;
	char        timetemp[CHANLOG_TIME_MAXLEN];
	
	now = time(NULL);
	if ((!(tmnow = gmtime(&now))))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),CHANLOG_TIME_FORMAT,tmnow);
	fprintf(channel->log,"\ndestroyed=\"%s\"\n",timetemp);
	
	if (fclose(channel->log)<0)
	    eventlog(eventlog_level_error,"channel_destroy","could not close channel log \"%s\" after writing (fclose: %s)",channel->logname,strerror(errno));
    }
    
    if (channel->logname)
	free((void *)channel->logname); /* avoid warning */
    
    if (channel->country)
	free((void *)channel->country); /* avoid warning */
    
    if (channel->realmname)
	free((void *)channel->realmname); /* avoid warning */

    if (channel->clienttag)
	free((void *)channel->clienttag); /* avoid warning */
    
    if (channel->shortname)
	free((void *)channel->shortname); /* avoid warning */

    free((void *)channel->name); /* avoid warning */
    
    free(channel);
    
    return 0;
}


extern char const * channel_get_name(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_name","got NULL channel");
	return "";
    }
    
    return channel->name;
}


extern char const * channel_get_clienttag(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_clienttag","got NULL channel");
	return "";
    }
    
    return channel->clienttag;
}


extern t_channel_flags channel_get_flags(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_flags","got NULL channel");
	return channel_flags_none;
    }
    
    return channel->flags;
}


extern int channel_get_permanent(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_permanent","got NULL channel");
	return 0;
    }
    
    return channel->permanent;
}


extern unsigned int channel_get_channelid(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_channelid","got NULL channel");
	return 0;
    }
    return channel->id;
}


extern int channel_set_channelid(t_channel * channel, unsigned int channelid)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_set_channelid","got NULL channel");
	return -1;
    }
    channel->id = channelid;
    return 0;
}


extern int channel_add_connection(t_channel * channel, t_connection * connection)
{
#ifdef WITH_BITS
    t_packet * p;
    t_query *q;

    if (!channel) {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL channel");
	return -1;
    }
    if (!connection) {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL connection");
	return -1;
    }
    
    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"channel_add_connection","could not create packet");
	return -1;
    }
    channel_add_ref(channel);
    channel->currmembers++;
    packet_set_size(p,sizeof(t_bits_chat_channel_join_request));
    packet_set_type(p,BITS_CHAT_CHANNEL_JOIN_REQUEST);
    bits_packet_generic(p,BITS_ADDR_MASTER);
    bn_int_set(&p->u.bits_chat_channel_join_request.channelid,channel_get_channelid(channel));
    bn_int_set(&p->u.bits_chat_channel_join_request.sessionid,conn_get_sessionid(connection));
    bn_int_set(&p->u.bits_chat_channel_join_request.flags,conn_get_flags(connection));
    bn_int_set(&p->u.bits_chat_channel_join_request.latency,conn_get_latency(connection));
    q = query_create(bits_query_type_chat_channel_join);
    if (!q) {
	eventlog(eventlog_level_error,"channel_add_connection","could not create query");
	packet_del_ref(p);
	return -1;
    }
    query_attach_conn(q,"client",connection);
    bn_int_set(&p->u.bits_chat_channel_join_request.qid,query_get_qid(q));
    send_bits_packet(p);
    packet_del_ref(p);
    return 0;
#else
    t_channelmember * member;
    t_connection *    user;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL channel");
	return -1;
    }
    if (!connection)
    {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL connection");
	return -1;
    }
    
    if (channel_check_banning(channel,connection))
    {
	channel_message_log(channel,connection,0,"JOIN FAILED (banned)");
	return -1;
    }
    
    if (!(member = malloc(sizeof(t_channelmember))))
    {
	eventlog(eventlog_level_error,"channel_add_connection","could not allocate memory for channelmember");
	return -1;
    }
    member->connection = connection;
    member->next = channel->memberlist;
    channel->memberlist = member;
    channel->currmembers++;
    
    channel_message_log(channel,connection,0,"JOINED");
    
    message_send_text(connection,message_type_channel,connection,channel_get_name(channel));
    for (user=channel_get_first(channel); user; user=channel_get_next())
    {
	message_send_text(connection,message_type_adduser,user,NULL);
	if (user!=connection)
	    message_send_text(user,message_type_join,connection,NULL);
    }
    
    /* please don't remove this notice */
    if (channel->log)
	message_send_text(connection,message_type_info,connection,prefs_get_log_notice());
    
    eventlog(eventlog_level_debug,"channel_add_connection","choosing operator");
    if (!channel->opr)
	channel_choose_operator(channel,connection); /* try using this account */
    
    return 0;
#endif
}


extern int channel_del_connection(t_channel * channel, t_connection * connection)
{
#ifdef WITH_BITS
    t_packet * p;

    if (!channel) {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL channel");
	return -1;
    }
    if (!connection) {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL connection");
	return -1;
    }

    p = packet_create(packet_class_bits);
    if (!p) {
	eventlog(eventlog_level_error,"channel_del_connection","could not create packet");
	return -1;
    }
    packet_set_size(p,sizeof(t_bits_chat_channel_leave));
    packet_set_type(p,BITS_CHAT_CHANNEL_LEAVE);
    bn_int_set(&p->u.bits_chat_channel_leave.channelid,channel_get_channelid(channel));
    bn_int_set(&p->u.bits_chat_channel_leave.sessionid,conn_get_sessionid(connection));
    bits_packet_generic(p,BITS_ADDR_MASTER);
    send_bits_packet(p);
    packet_del_ref(p);
    channel_del_ref(channel);
    channel->currmembers--;
    list_purge(channellist_head);
    
    return 0;
#else
    t_channelmember * curr;
    t_channelmember * temp;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL channel");
        return -1;
    }
    if (!connection)
    {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL connection");
        return -1;
    }
    
    channel_message_log(channel,connection,0,"PARTED");
    
    channel_message_send(channel,message_type_part,connection,NULL);
    
    curr = channel->memberlist;
    if (curr->connection==connection)
    {
        channel->memberlist = channel->memberlist->next;
        free(curr);
    }
    else
    {
        while (curr->next && curr->next->connection!=connection)
            curr = curr->next;
        
        if (curr->next)
        {
            temp = curr->next;
            curr->next = curr->next->next;
            free(temp);
        }
	else
	{
	    eventlog(eventlog_level_error,"channel_del_connection","[%d] connection not in channel member list",conn_get_socket(connection));
	    return -1;
	}
    }
    channel->currmembers--;
    
    if (channel->next_opr==connection)
	channel_set_next_operator(channel,NULL);
    if (channel->opr==connection)
	channel_choose_operator(channel,NULL);
    
    if (!channel->memberlist && !channel->permanent) /* if channel is empty, delete it unless it's a permanent channel */
    {
	channel_destroy(channel);
	list_purge(channellist_head);
    }
    
    return 0;
#endif
}


extern void channel_update_latency(t_connection * me)
{
    t_channel *    channel;
#ifndef WITH_BITS
    t_message *    message;
    t_connection * c;
#endif
    
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_update_latency","got NULL connection");
        return;
    }
    if (!(channel = conn_get_channel(me)))
    {
	eventlog(eventlog_level_error,"channel_update_latency","connection has no channel");
        return;
    }
    
#ifndef WITH_BITS
    if (!(message = message_create(message_type_userflags,me,NULL,NULL))) /* handles NULL text */
	return;

    for (c=channel_get_first(channel); c; c=channel_get_next())
        if (conn_get_class(c)==conn_class_bnet)
            message_send(message,c);
    message_destroy(message);
#else
    /* FIXME: check whether an update is needed */
    channel_message_send(channel,message_type_userflags,me,NULL); /* handles NULL text */
#endif
}


extern void channel_update_flags(t_connection * me)
{
    t_channel *    channel;
#ifndef WITH_BITS
    t_message *    message;
    t_connection * c;
#endif
    
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_update_flags","got NULL connection");
        return;
    }
    if (!(channel = conn_get_channel(me)))
    {
	eventlog(eventlog_level_error,"channel_update_flags","connection has no channel");
        return;
    }
    
#ifndef WITH_BITS
    if (!(message = message_create(message_type_userflags,me,NULL,NULL))) /* handles NULL text */
	return;
    
    for (c=channel_get_first(channel); c; c=channel_get_next())
	message_send(message,c);
    
    message_destroy(message);
#else
    /* FIXME: check whether an update is needed */
    	/* TODO */
    channel_message_send(channel,message_type_userflags,me,NULL); /* handles NULL text */
#endif
}


extern void channel_message_log(t_channel const * channel, t_connection * me, int fromuser, char const * text)
{
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_message_log","got NULL channel");
        return;
    }
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_message_log","got NULL connection");
        return;
    }
    
    if (channel->log)
    {
	time_t       now;
	struct tm *  tmnow;
	char         timetemp[CHANLOG_TIME_MAXLEN];
	char const * tname;
	
	now = time(NULL);
	if ((!(tmnow = gmtime(&now))))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),CHANLOGLINE_TIME_FORMAT,tmnow);
	
	if (fromuser)
	    fprintf(channel->log,"%s: \"%s\" \"%s\"\n",timetemp,(tname = conn_get_username(me)),text);
	else
	    fprintf(channel->log,"%s: \"%s\" %s\n",timetemp,(tname = conn_get_username(me)),text);
	conn_unget_username(me,tname);
	fflush(channel->log);
    }
}


extern void channel_message_send(t_channel const * channel, t_message_type type, t_connection * me, char const * text)
{
    t_connection * c;
    unsigned int   heard;
    t_message *    message;
#ifdef WITH_BITS
    t_bits_channelmember * member;
    t_elem const * curr;
#else
    char const *   tname;
#endif
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_message_send","got NULL channel");
        return;
    }
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_message_send","got NULL connection");
        return;
    }
    
    if (!(message = message_create(type,me,NULL,text)))
    {
	eventlog(eventlog_level_error,"channel_message_send","could not create message");
	return;
    }
    
#ifndef WITH_BITS
    heard = 0;
    tname = conn_get_chatname(me);
    for (c=channel_get_first(channel); c; c=channel_get_next())
    {
	if (c==me && (type==message_type_talk || type==message_type_join))
	    continue; /* ignore ourself */
	if ((type==message_type_talk || type==message_type_whisper || type==message_type_emote || type==message_type_broadcast) &&
	    conn_check_ignoring(c,tname)==1)
	    continue; /* ignore squelched players */
	
	if (message_send(message,c)==0 && c!=me)
	    heard = 1;
    }
    conn_unget_chatname(me,tname);
#else
    heard = 1; /* FIXME: check if there is more than 1 member in the channel */
    member = channel_find_member_bysessionid(channel,conn_get_sessionid(me));
    if (!member) {
	char const * username;
	eventlog(eventlog_level_error,"channel_message_send","could not find sending user \"%s\" with sessionid 0x%08x",(username = conn_get_username(me)),conn_get_sessionid(me));
	conn_unget_username(me,username);
    } else {
	send_bits_chat_channel(member,channel_get_channelid(channel),type,text);
    }
    LIST_TRAVERSE_CONST(connlist(),curr)
    {
    	c = elem_get_data(curr);
	if (c==me && (type==message_type_talk || type==message_type_join))
	    continue; /* ignore ourself */
	/* FIXME: what about ignore? */
        if ((conn_get_class(c) == conn_class_bnet)||(conn_get_class(c) == conn_class_bot)) {
	    if (conn_get_channel(c) == channel) {
		message_send(message,c); 
	    }
	}
    }
#endif
    message_destroy(message);
    
    if (!heard && (type==message_type_talk || type==message_type_emote))
	message_send_text(me,message_type_info,me,"No one hears you.");
}


extern int channel_choose_operator(t_channel * channel, t_connection * tryop)
{
#ifdef WITH_BITS
    eventlog(eventlog_level_error,"channel_choose_operator","FIXME");
    return 0;
#else
    t_connection *          oldop;
    t_channelmember const * curr;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_choose_operator","got NULL channel");
	return -1;
    }
    
    oldop = channel->opr;
    
    if (channel->next_opr) /* someone was designated */
    {
	if (conn_get_channel(channel->next_opr)!=channel)
	{
	    eventlog(eventlog_level_error,"channel_choose_operator","designated next operator not in this channel");
	    return -1;
	}
	channel->opr = channel->next_opr;
	channel->next_opr = NULL;
    }
    else
    {
        t_connection * newop;
	t_account *    account;
        
	newop = channel->opr;
	
	/* choose the designee, or the last (oldest) member of the channel
           that is eligible */
	for (curr=channel->memberlist; curr; curr=curr->next)
	    if (curr->connection!=channel->opr)
	    {
		if (!(account = conn_get_account(curr->connection)))
		{
		    eventlog(eventlog_level_error,"channel_choose_operator","connection without account in channel \"%s\"",channel->name);
		    continue;
		}
		/* designation overrides everything else... */
		if (tryop && curr->connection!=tryop)
		    continue;
		/* specific account setting overrides other settings... */
		if (account_get_auth_operator(account,channel->name)==1) /* default to false */
		{
        	    newop = curr->connection;
		    continue;
		}
		/* channel overrides general account settings... */
		if (!channel->allowopers)
		    continue;
		/* finally, check general account settings */
		if (account_get_auth_operator(account,NULL)==1) /* default to false */
        	    newop = curr->connection;
	    }
	
	channel->opr = newop;
    }
    
    /* the requested connection was not chosen */
    if (tryop && channel->opr!=tryop)
	return -1;
    
    /* there is nobody else who can be an op */
    if (oldop==channel->opr)
        channel->opr = NULL;
    
    /* update flags */
    if (oldop)
    {
	channel_message_log(channel,oldop,0,"NO LONGER OPERATOR");
        conn_del_flags(oldop,MF_GAVEL);
    }
    if (channel->opr)
    {
	channel_message_log(channel,channel->opr,0,"NOW OPERATOR");
	conn_add_flags(channel->opr,MF_GAVEL);
	message_send_text(channel->opr,message_type_info,channel->opr,"You are now the operator for this channel.");
    }
    
    return 0;
#endif
}


extern t_connection * channel_get_operator(t_channel const * channel)
{
#ifdef WITH_BITS
    eventlog(eventlog_level_error,"channel_get_operator","BITS FIXME");
    return NULL;
#else
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_get_operator","got NULL channel");
	return NULL;
    }
    
    return channel->opr;
#endif
}


extern int channel_set_next_operator(t_channel * channel, t_connection * conn)
{
#ifdef WITH_BITS
    eventlog(eventlog_level_error,"channel_set_next_operator","BITS FIXME");
    return 0;
#else
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_set_next_operator","got NULL channel");
	return -1;
    }
    
    if (conn && channel->opr==conn)
	return -1;
    
    channel->next_opr = conn;
    return 0;
#endif
}


extern int channel_ban_user(t_channel * channel, char const * user)
{
    t_elem const * curr;
    char *         temp;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got NULL user");
	return -1;
    }
    if (!channel->name)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got channel with NULL name");
	return -1;
    }
    
    if (strcasecmp(channel->name,CHANNEL_NAME_BANNED)==0 ||
	strcasecmp(channel->name,CHANNEL_NAME_KICKED)==0)
        return -1;
    
    LIST_TRAVERSE_CONST(channel->banlist,curr)
        if (strcasecmp(elem_get_data(curr),user)==0)
            return 0;
    
    if (!(temp = strdup(user)))
    {
        eventlog(eventlog_level_error,"channel_ban_user","could not allocate memory for temp");
        return -1;
    }
    if (list_append_data(channel->banlist,temp)<0)
    {
	free(temp);
        eventlog(eventlog_level_error,"channel_ban_user","unable to append to list");
        return -1;
    }
    return 0;
}


extern int channel_unban_user(t_channel * channel, char const * user)
{
    t_elem * curr;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_unban_user","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_unban_user","got NULL user");
	return -1;
    }
    
    LIST_TRAVERSE(channel->banlist,curr)
    {
	char const * banned;
	
	if (!(banned = elem_get_data(curr)))
	{
            eventlog(eventlog_level_error,"channel_unban_user","found NULL name in banlist");
	    continue;
	}
        if (strcasecmp(banned,user)==0)
        {
            if (list_remove_elem(channel->banlist,curr)<0)
            {
                eventlog(eventlog_level_error,"channel_unban_user","unable to remove item from list");
                return -1;
            }
            free((void *)banned); /* avoid warning */
            return 0;
        }
    }
    
    return -1;
}


extern int channel_check_banning(t_channel const * channel, t_connection const * user)
{
    t_elem const * curr;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_check_banning","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_check_banning","got NULL user");
	return -1;
    }
    
    if (!channel->allowbots && conn_get_class(user)==conn_class_bot)
	return 1;
    
    LIST_TRAVERSE_CONST(channel->banlist,curr)
        if (conn_match(user,elem_get_data(curr))==1)
            return 1;
    
    return 0;
}


extern int channel_get_length(t_channel const * channel)
{
    t_channelmember const * curr;
    int                     count;
    
    for (curr=channel->memberlist,count=0; curr; curr=curr->next,count++);
    
    return count;
}


#ifndef WITH_BITS
extern t_connection * channel_get_first(t_channel const * channel)
#else
extern t_bits_channelmember * channel_get_first(t_channel const * channel)
#endif
{
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_get_first","got NULL channel");
        return NULL;
    }
    
    memberlist_curr = channel->memberlist;
    
    return channel_get_next();
}

#ifndef WITH_BITS
extern t_connection * channel_get_next(void)
#else
extern t_bits_channelmember * channel_get_next(void)
#endif
{

    t_channelmember * member;
    
    if (memberlist_curr)
    {
        member = memberlist_curr;
        memberlist_curr = memberlist_curr->next;
        
#ifndef WITH_BITS
        return member->connection;
#else
	return &member->member;
#endif
    }
    return NULL;
}


extern t_list * channel_get_banlist(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_banlist","got NULL channel");
	return NULL;
    }
    
    return channel->banlist;
}


extern char const * channel_get_shortname(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_shortname","got NULL channel");
	return NULL;
    }
    
    return channel->shortname;
}


#ifdef WITH_BITS
extern int channel_add_member(t_channel * channel, int sessionid, int flags, int latency)
{
	/* BITS version of channel_add_connection() */
    t_channelmember * member;
    t_bits_channelmember * user;
    t_connection  * joinc;
    t_bits_loginlist_entry * lle;

    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_add_member","got NULL channel");
	return -1;
    }
    lle = bits_loginlist_bysessionid(sessionid);
    if (!lle) {
	eventlog(eventlog_level_error,"channel_add_member","joining user is not logged in (sessionid=0x%08x)",sessionid);
	return -1;
    }
    if (!(member = malloc(sizeof(t_channelmember))))
    {
	eventlog(eventlog_level_error,"channel_add_member","could not allocate memory for channelmember");
	return -1;
    }

/* defaults */
    eventlog(eventlog_level_debug,"channel_add_member","sessionid=#%u flags=0x%08x latency=%d",sessionid,flags,latency);
    member->member.sessionid = sessionid;
    member->member.latency = latency;
    member->member.flags = flags;
    member->next = channel->memberlist;
/* operator */
    if ((bits_master) && (channel->allowopers)) {
	if ((!channel->opr)||(!channel->memberlist)) {
	    member->member.flags |= MF_GAVEL;
	    channel->opr = sessionid;
	}
    }
/* add*/
    channel->memberlist = member;
    send_bits_chat_channel_join(NULL,channel_get_channelid(channel),&member->member);

    /* send message_type_join to local users */
    joinc = connlist_find_connection_by_sessionid(sessionid);
    if (!joinc) {
	joinc = bits_rconn_create(latency,flags,lle->sessionid);
	conn_set_botuser(joinc,lle->chatname);
    }
    for (user=channel_get_first(channel); user; user=channel_get_next())
    {
    	t_connection * userc;
    	userc = connlist_find_connection_by_sessionid(user->sessionid);
    	if (userc)
	  message_send_text(userc,message_type_join,joinc,NULL);
    }
    if (conn_get_class(joinc)==conn_class_remote) {
	bits_rconn_destroy(joinc);
    }
    return 0;
}

extern int channel_del_member(t_channel * channel, t_bits_channelmember * member)
{
	/* BITS version of channel_del_connection() */
    t_channelmember * curr;
    t_channelmember * prev = NULL;
    t_bits_loginlist_entry * lle;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_del_member","got NULL channel");
        return -1;
    }
    if (!member)
    {
	eventlog(eventlog_level_error,"channel_del_member","got NULL member");
        return -1;
    }
    lle = bits_loginlist_bysessionid(member->sessionid);
    if (!lle) {
	eventlog(eventlog_level_error,"channel_del_member","leaving user is not logged in");
	return -1;
    }

    eventlog(eventlog_level_debug,"channel_del_member","sessionid=0x%08x",member->sessionid);
    send_bits_chat_channel_leave(channel_get_channelid(channel),member->sessionid);
    curr = channel->memberlist;
    while (curr) {
        if (&curr->member == member) { /* compare the pointers */
            t_bits_channelmember * user;
   	    t_connection * joinc;
   	    int op_has_changed = 0;

   	    /* operator, part 1 */
            if ((bits_master)&&(channel->allowopers)) {
            	if (channel->opr == 0) {
            	    channel->opr = channel->next_opr;
            	    channel->next_opr = 0;
            	    op_has_changed = 1;
            	}
             	if (channel->next_opr != 0) {
            	    if (channel->next_opr == member->sessionid) {
		        channel->next_opr = 0;
   	            }
   	        }
           	if (channel->opr != 0) {
            	    if (channel->opr == member->sessionid) {
		        channel->opr = channel->next_opr;
		        channel->next_opr = 0;
            	        op_has_changed = 1;
   	            }
   	        }
   	    }
	    /* send message_type_part to local conns */
   	    joinc = connlist_find_connection_by_sessionid(curr->member.sessionid);
            if (!joinc) {
	        joinc = bits_rconn_create(curr->member.latency,curr->member.flags,lle->sessionid);
	        conn_set_botuser(joinc,lle->chatname);
            }
            for (user=channel_get_first(channel); user; user=channel_get_next())
            {
    	        t_connection * userc;
    	        userc = connlist_find_connection_by_sessionid(user->sessionid);
      	        if (userc)
		    message_send_text(userc,message_type_part,joinc,NULL);
	    }
	    if (conn_get_class(joinc)==conn_class_remote) {
	        bits_rconn_destroy(joinc);
	    }
	    
	    if (prev) {
		prev->next = curr->next;
	    } else {
	    	channel->memberlist = curr->next;
	    }
	    free(curr);
	    /* operator, part 2 */
            if ((bits_master)&&(channel->allowopers)) {
		if ((channel->opr == 0)&&(channel->memberlist)) {
		    channel->opr = channel->memberlist->member.sessionid;
               	    op_has_changed = 1;
		}
		if ((op_has_changed)&&(channel->opr)) {
		    t_bits_channelmember * m;

		    m = channel_find_member_bysessionid(channel,channel->opr);
		    m->flags |= MF_GAVEL;
		    send_bits_chat_channel_join(NULL,channel_get_channelid(channel),m);
        	}
        	if ((!channel->memberlist)&&(!channel->permanent)) {
		/* destroy the channel */
		    channel_destroy(channel);
        	}
            }
            /* --- */
	    return 0;
    	}
	prev = curr;
	curr = curr->next;
    }
    return 1;
}

extern t_bits_channelmember * channel_find_member_bysessionid(t_channel const * channel, int sessionid)
{
	t_bits_channelmember * member;

	if (!channel) {
		eventlog(eventlog_level_error,"channel_find_member_bysessionid","got NULL channel");
		return NULL;
	}
	member = channel_get_first(channel);
	while (member) {
		if (member->sessionid == sessionid) {
			return member;
		}
		member = channel_get_next();
	}
	return NULL;
}

extern int channel_add_ref(t_channel * channel) {
	if (!channel) {
		eventlog(eventlog_level_error,"channel_add_ref","got NULL channel");
		return -1;
	}
	if (!bits_uplink_connection) return 0;
	if (channel->ref == 0) {
		eventlog(eventlog_level_debug,"channel_add_ref","joining channel \"%s\" (#%u)",channel_get_name(channel),channel_get_channelid(channel));
		send_bits_chat_channel_server_join(channel_get_channelid(channel));
	}
	channel->ref++;
	return 0;
}

extern int channel_del_ref(t_channel * channel) {
	if (!channel) {
		eventlog(eventlog_level_error,"channel_del_ref","got NULL channel");
		return -1;
	}
	if (!bits_uplink_connection) return 0;
	if (channel->ref < 1) {
		eventlog(eventlog_level_error,"channel_del_ref","reference counter <= 0");
		return -1;
	}
	if (channel->ref == 1) {
		eventlog(eventlog_level_debug,"channel_del_ref","leaving channel \"%s\" (#%u)",channel_get_name(channel),channel_get_channelid(channel));
		send_bits_chat_channel_server_leave(channel_get_channelid(channel));
		/* delete all members */
		while (channel->memberlist)
			channel_del_member(channel,&channel->memberlist->member);
	}
	channel->ref--;
	return 0;
}

#endif


static int channellist_load_permanent(char const * filename)
{
    FILE *       fp;
    unsigned int line;
    unsigned int pos;
    int          botflag;
    int          operflag;
    int          logflag;
    char *       buff;
    char *       name;
    char *       sname;
    char *       tag;
    char *       bot;
    char *       oper;
    char *       log;
    char *       country;
    char *       max;
    char *       newname;
    char *       realmname;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,"channellist_load_permanent","got NULL filename");
	return -1;
    }
    
#ifdef WITH_BITS
    if (bits_uplink_connection)
    {
	eventlog(eventlog_level_info,"channellist_load_permanent","not loading permanent channels from file");
	return 0;
    }
#endif
    if (!(fp = fopen(filename,"r")))
    {
	eventlog(eventlog_level_error,"channellist_load_permanent","could not open channel file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	return -1;
    }
    
    for (line=1; (buff = file_get_line(fp)); line++)
    {
	if (buff[0]=='#' || buff[0]=='\0')
	{
	    free(buff);
	    continue;
	}
        pos = 0;
	if (!(name = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing name in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(sname = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing sname in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(tag = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing tag in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(bot = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing bot in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(oper = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing oper in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(log = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing log in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(country = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing country in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
        if (!(realmname = next_token(buff,&pos)))
        {
           eventlog(eventlog_level_error,"channellist_load_permanent","missing realmname in line %u in file \"%s\"",line,filename);
           free(buff);
           continue;
        }
	if (!(max = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"channellist_load_permanent","missing max in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	
	switch (str_get_bool(bot))
	{
	case 1:
	    botflag = 1;
	    break;
	case 0:
	    botflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 4 on line %u in file \"%s\"",bot,line,filename);
	    free(buff);
	    continue;
        }
	
	switch (str_get_bool(oper))
	{
	case 1:
	    operflag = 1;
	    break;
	case 0:
	    operflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 5 on line %u in file \"%s\"",oper,line,filename);
	    free(buff);
	    continue;
        }
	
	switch (str_get_bool(log))
	{
	case 1:
	    logflag = 1;
	    break;
	case 0:
	    logflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 5 on line %u in file \"%s\"",log,line,filename);
	    free(buff);
	    continue;
        }
	
	if (strcmp(sname,"NULL") == 0)
	    sname = NULL;
	if (strcmp(tag,"NULL") == 0)
	    tag = NULL;
        if (strcmp(name,"NONE") == 0)
	    name = NULL;
        if (strcmp(country, "NULL") == 0)
            country = NULL;
        if (strcmp(realmname,"NULL") == 0)
            realmname = NULL;
	
	if (name)
	    {
            channel_create(name,sname,tag,1,botflag,operflag,logflag,country,realmname,atoi(max));
	    }
	else
	    {
            newname = channel_format_name(sname,country,realmname,1);
            if (newname)
		{
                   channel_create(newname,sname,tag,1,botflag,operflag,logflag,country,realmname,atoi(max));
                   free(newname);
	    }
            else
	    {
                   eventlog(eventlog_level_error,"channellist_load_permanent","cannot format channel name");
		}
            }
	
	/* FIXME: call channel_delete() on current perm channels and do a
	   channellist_find_channel() and set the long name, perm flag, etc,
	   otherwise call channel_create(). This will make HUPing the server
           handle re-reading this file correctly. */
	free(buff);
    }
    
    if (fclose(fp)<0)
	eventlog(eventlog_level_error,"channellist_load_permanent","could not close channel file \"%s\" after reading (fclose: %s)",filename,strerror(errno));
    return 0;
}

static char * channel_format_name(char const * sname, char const * country, char const * realmname, unsigned int id)
{
    char * fullname;
    unsigned int len;

    if (!sname)
    {
        eventlog(eventlog_level_error,"channel_format_name","got NULL sname");
        return NULL;
    }
    len = strlen(sname)+1; /* FIXME: check lengths and format */
    if (country) 
    	len = len + strlen(country) + 1;
    if (realmname)
    	len = len + strlen(realmname) + 1;
    len = len + 32 + 1;

    if (!(fullname=malloc(len)))
    {
        eventlog(eventlog_level_error,"channel_format_name","could not allocate memory for fullname");
        return NULL;
    }
    sprintf(fullname,"%s%s%s%s%s-%d",
            realmname?realmname:"",
            realmname?" ":"",
            sname,
            country?" ":"",
            country?country:"",
            id);
    return fullname;
}

#ifndef WITH_BITS 
extern int channellist_reload(void)
{
  t_elem * curr;
  t_channel * channel, * old_channel;
  t_channelmember * memberlist, * member;
  t_list * channellist_old=NULL;

  if (channellist_head)
    {

      if (!(channellist_old = list_create()))
        return -1;

      LIST_TRAVERSE(channellist_head,curr)
      {
	if (!(channel = elem_get_data(curr)))
	{
	  eventlog(eventlog_level_error,"channellist_reload","channel list contains NULL item");
	  continue;
	}

	if (channel->memberlist)
	{
	  /* we need only channel name and memberlist */

	  old_channel = (t_channel *) malloc(sizeof(t_channel));
	  old_channel->shortname = strdup(channel->shortname);
	  old_channel->memberlist = channel->memberlist;
	  old_channel->currmembers = channel->currmembers;
	  /* avoid deleting these values which are needed */
	  channel->currmembers = 0;
	  channel->memberlist = NULL;

	  if (list_prepend_data(channellist_old,old_channel)<0)
	  {
	    eventlog(eventlog_level_error,"channellist_reload","error reloading channel list, destroying channellist_old");
	    /* revert changes */
	    channel->memberlist = old_channel->memberlist;
	    channel->currmembers = old_channel->currmembers;
	    free((void*)old_channel);
	    /* and destroy old list */
	    LIST_TRAVERSE(channellist_old,curr)
	    {
	      if (!(channel = elem_get_data(curr)))
	      {
		eventlog(eventlog_level_error,"channellist_reload","old channel list contains NULL item");
		continue;
	      }
	      if (channel->shortname)
		free((void*)channel->shortname);
	      memberlist = channel->memberlist;
	      while (memberlist)
	      {
		member = memberlist;
		memberlist = memberlist->next;
		free((void*)member);
	      }
	    }

	    return -1;
	  }

	}
	
	/* Destroy old channel */
	if (channel_destroy(channel)<0)
	  eventlog(eventlog_level_error,"channellist_reload","could not destroy channel");
      }

      if (list_destroy(channellist_head)<0)
	return -1;

      channellist_head = NULL;
      channellist_create();
      
      /* Now put all users on their previous channel */
      LIST_TRAVERSE(channellist_old,curr)
      {
	if (!(channel = elem_get_data(curr)))
	{
	  eventlog(eventlog_level_error,"channellist_reload","old channel list contains NULL item");
	  continue;
	}

	memberlist = channel->memberlist;
	while (memberlist)
	{
	  member = memberlist;
	  memberlist = memberlist->next;
	  /* I known is not the most beautyfull thing of the heart but should avoid problems */
	  conn_set_channel_var(member->connection, NULL);
	  conn_set_channel(member->connection, channel->shortname);
	  /* FIX-ME: remove flags i.e. moderator */
	  free((void*)member);
	}
	if (channel->shortname)
	  free((void*)channel->shortname);
	free((void*)channel);
      
      }
      if (list_destroy(channellist_old)<0)
	return -1;
      channellist_old = NULL;
    }
  return 0;

}     
#endif

extern int channellist_create(void)
{
    if (!(channellist_head = list_create()))
	return -1;
    
    return channellist_load_permanent(prefs_get_channelfile());
}


extern int channellist_destroy(void)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    if (!(channel = elem_get_data(curr))) /* should not happen */
	    {
		eventlog(eventlog_level_error,"channellist_destroy","channel list contains NULL item");
		continue;
	    }
	    
	    channel_destroy(channel);
	}
	
	if (list_destroy(channellist_head)<0)
	    return -1;
	channellist_head = NULL;
    }
    
    return 0;
}


extern t_list * channellist(void)
{
    return channellist_head;
}


extern int channellist_get_length(void)
{
    return list_get_length(channellist_head);
}


static t_channel * channellist_find_channel_by_fullname(char const * name)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_by_fullname","found channel with NULL name");
		continue;
	    }

	    if (strcasecmp(channel->name,name)==0)
		return channel;
	}
    }
    
    return NULL;
}


/* Find a channel based on the name. 
 * Create a new channel if it is a permanent-type channel and all others
 * are full.
 */
extern t_channel * channellist_find_channel_by_name(char const * name, char const * country, char const * realmname)
{
    t_channel *    channel;
    t_elem const * curr;
    int            foundperm;
    int            maxchannel; /* the number of "rollover" channels that exist */
    char const *   saveshortname;
    char const *   savetag;
    int            savebotflag;
    int            saveoperflag;
    int            savelogflag;
    char const *   savecountry;
    char const *   saverealmname;
    int            savemaxmembers;
    int            match;
    
    maxchannel = 0;
    foundperm = 0;
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_by_name","found channel with NULL name");
		continue;
	    }

	    if (strcasecmp(channel->name,name)==0)
	    {
		eventlog(eventlog_level_debug,"channellist_find_channel_by_name","found exact match for \"%s\"",name);
		return channel;
	    }
	    
            if (channel->shortname && strcasecmp(channel->shortname,name)==0)
	    {
                if ( ((!channel->country && !country) || 
		      (channel->country && country && (strcmp(channel->country, country)==0))) &&
	             ((!channel->realmname && !realmname) || 
		      (channel->realmname && realmname && (strcmp(channel->realmname, realmname)==0))) )

		{
		    if (channel->maxmembers==-1 || channel->currmembers<channel->maxmembers) 
		    {
			eventlog(eventlog_level_debug,"channellist_find_channel_by_name","found permanent channel \"%s\" for \"%s\"",channel->name,name);
			return channel;
		    }
		    maxchannel++;
		}
		else
		    eventlog(eventlog_level_debug,"channellist_find_channel_by_name","countries didn't match");
		
#ifdef WITH_BITS
		if (!bits_master) /* BITS slaves have to handle permanent, rollover channel seperately with an extra */
		    return channel; /* request to the master. Only the master has to decide whether to create a new instance ... */
#endif

		foundperm = 1;
		
		/* save off some info in case we need to create a new copy */
		saveshortname = channel->shortname;
		savetag = channel->clienttag;
		savebotflag = channel->allowbots;
		saveoperflag = channel->allowopers;
		if (channel->logname)
		    savelogflag = 1;
                else
                    savelogflag = 0;
                if (country)
		    savecountry = country;
		else
		    savecountry = channel->country;
                if (realmname)
                    saverealmname = realmname;
                else
                    saverealmname = channel->realmname;
		savemaxmembers = channel->maxmembers;
	    } 
	}
    }
    
    /* we've gone thru the whole list and either there was no match or the
     * channels are all full.
     */

    if (foundperm) /* All the channels were full, create a new one */
    {		/* Only BITS masters should reach this point. */
	char * channelname;
	
        if (!(channelname=channel_format_name(saveshortname,savecountry,saverealmname,maxchannel+1)))
                return NULL;

        channel = channel_create(channelname,saveshortname,savetag,1,savebotflag,saveoperflag,savelogflag,savecountry,saverealmname,savemaxmembers);
        free(channelname);
	
	eventlog(eventlog_level_debug,"channellist_find_channel_by_name","created copy \"%s\" of channel \"%s\"",(channel)?(channel->name):("<failed>"),name);
        return channel;
    }
    
    /* no match */
    eventlog(eventlog_level_debug,"channellist_find_channel_by_name","could not find channel \"%s\"",name);
    return NULL;
}


extern t_channel * channellist_find_channel_bychannelid(unsigned int channelid)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_bychannelid","found channel with NULL name");
		continue;
	    }
	    if (channel->id==channelid)
		return channel;
	}
    }
    
    return NULL;
}

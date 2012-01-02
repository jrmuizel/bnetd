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
#include "compat/strerror.h"
#include "common/irc_protocol.h"
#include "common/packet.h"
#include "common/eventlog.h"
#include "connection.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/addr.h"
#include "common/version.h"
#include "common/queue.h"
#include "common/list.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/tag.h"
#include "message.h"
#include "account.h"
#include "channel.h"
#include "irc.h"
#include "prefs.h"
#include "server.h"
#include "tick.h"
#include "common/setup_after.h"

typedef struct {
    char const * nick;
    char const * user;
    char const * host;
} t_irc_message_from;


static char ** irc_split_elems(char * list, int separator, int ignoreblank);
static int irc_unget_elems(char ** elems);
static char * irc_message_preformat(t_irc_message_from const * from, char const * command, char const * dest, char const * text);


extern int irc_send_cmd(t_connection * conn, char const * command, char const * params)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN+1];
    int len;
    char const * ircname = prefs_get_servername(); 
    char const * nick;
    
    if (!conn) {
	eventlog(eventlog_level_error,"irc_send_cmd","got NULL connection");
	return -1;
    }
    if (!command) {
	eventlog(eventlog_level_error,"irc_send_cmd","got NULL command");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,"irc_send_cmd","could not create packet");
	return -1;
    }

    nick = conn_get_botuser(conn);
    if (!nick)
    	nick = ""; /* FIXME: Is this good? */
    /* snprintf isn't portable -> check message length first */
    if (params) {
        len = 1+strlen(ircname)+1+strlen(command)+1+strlen(nick)+1+strlen(params)+2;
	if (len > MAX_IRC_MESSAGE_LEN)
	    eventlog(eventlog_level_error,"irc_send_cmd","message to send is too large (%d bytes)",len);
	else
	    sprintf(data,":%s %s %s %s\r\n",ircname,command,nick,params);
    } else {
        len = 1+strlen(ircname)+1+strlen(command)+1+strlen(nick)+1+2;
	if (len > MAX_IRC_MESSAGE_LEN)
	    eventlog(eventlog_level_error,"irc_send_cmd","message to send is too large (%d bytes)",len);
	else
	sprintf(data,":%s %s %s\r\n",ircname,command,nick);
    }
    packet_set_size(p,0);
    packet_append_data(p,data,len);
    eventlog(eventlog_level_debug,"irc_send_cmd","[%d] sent \"%s\"",conn_get_socket(conn),data);
    queue_push_packet(conn_get_out_queue(conn),p);
    packet_del_ref(p);
    return 0;
}

extern int irc_send(t_connection * conn, int code, char const * params)
{
    char temp[4]; /* '000\0' */
    
    if (!conn) {
	eventlog(eventlog_level_error,"irc_send","got NULL connection");
	return -1;
    }
    if ((code>999)||(code<0)) { /* more than 3 digits or negative */
	eventlog(eventlog_level_error,"irc_send","invalid message code (%d)",code);
	return -1;
    }
    sprintf(temp,"%03u",code);
    return irc_send_cmd(conn,temp,params);
}

extern int irc_send_ping(t_connection * conn)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN];
    
    if (!conn) {
	eventlog(eventlog_level_error,"irc_send_ping","got NULL connection");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,"irc_send_ping","could not create packet");
	return -1;
    }
    conn_set_ircping(conn,get_ticks());
    if (conn_get_state(conn)==conn_state_bot_username)
	sprintf(data,"PING :%u\r\n",conn_get_ircping(conn)); /* Undernet doesn't reveal the servername yet ... so do we */
    else if ((6+strlen(prefs_get_servername())+2+1)<=MAX_IRC_MESSAGE_LEN)
    	sprintf(data,"PING :%s\r\n",prefs_get_servername());
    else
    	eventlog(eventlog_level_error,"irc_send_ping","maximum message length exceeded");
    eventlog(eventlog_level_debug,"irc_send_ping","[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,strlen(data));
    queue_push_packet(conn_get_out_queue(conn),p);
    packet_del_ref(p);
    return 0;
}

extern int irc_send_pong(t_connection * conn, char const * params)
{
    t_packet * p;
    char data[MAX_IRC_MESSAGE_LEN];
    
    if (!conn) {
	eventlog(eventlog_level_error,"irc_send_pong","got NULL connection");
	return -1;
    }
    if ((1+strlen(prefs_get_servername())+1+4+1+strlen(prefs_get_servername())+((params)?(2+strlen(params)):(0))+2+1) > MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_error,"irc_send_pong","max message length exceeded");
	return -1;
    }
    if (!(p = packet_create(packet_class_raw))) {
	eventlog(eventlog_level_error,"irc_send_pong","could not create packet");
	return -1;
    }
    if (params)
    	sprintf(data,":%s PONG %s :%s\r\n",prefs_get_servername(),prefs_get_servername(),params);
    else
    	sprintf(data,":%s PONG %s\r\n",prefs_get_servername(),prefs_get_servername());
    eventlog(eventlog_level_debug,"irc_send_pong","[%d] sent \"%s\"",conn_get_socket(conn),data);
    packet_set_size(p,0);
    packet_append_data(p,data,strlen(data));
    queue_push_packet(conn_get_out_queue(conn),p);
    packet_del_ref(p);
    return 0;
}

extern int irc_authenticate(t_connection * conn, char const * passhash)
{
    t_hash h1;
    t_hash h2;
    t_account * a;
    char const * temphash;

    if (!conn) {
	eventlog(eventlog_level_error,"irc_authenticate","got NULL connection");
	return 0;
    }
    if (!passhash) {
	eventlog(eventlog_level_error,"irc_authenticate","got NULL passhash");
	return 0;
    }
	
    a = conn_get_account(conn);
    hash_set_str(&h1,passhash);
    temphash = account_get_pass(a);	
    hash_set_str(&h2,temphash);
    account_unget_pass(temphash);
    if (hash_eq(h1,h2)) {
        conn_set_state(conn,conn_state_loggedin);
        conn_set_clienttag(conn,CLIENTTAG_BNCHATBOT); /* CHAT hope here is ok */
        irc_send_cmd(conn,"NOTICE",":Authentication successful. You are now logged in.");
	return 1;
    } else {
        /* FIXME: TODO: let the client wait ... */
        irc_send_cmd(conn,"NOTICE",":Authentication failed.");
    }
    return 0;
}

extern int irc_welcome(t_connection * conn)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    time_t temptime;
    char const * tempname;
    char const * temptimestr;
    
    if (!conn) {
	eventlog(eventlog_level_error,"irc_send_welcome","got NULL connection");
	return -1;
    }

    tempname = conn_get_botuser(conn);


    if ((34+strlen(tempname)+1)<=MAX_IRC_MESSAGE_LEN)
        /* sprintf(temp,":Welcome to the Internet Relay Network %s!%s@%s",conn_get_botuser(conn),conn_get_user(conn),addr_num_to_ip_str(conn_get_addr(conn)));*/
        sprintf(temp,":Welcome to the BNETD IRC Network %s",tempname);
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_WELCOME,temp);
    
    if ((14+strlen(prefs_get_servername())+24+strlen(BNETD_VERSION)+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,":Your host is %s, running version bnetd-" BNETD_VERSION,prefs_get_servername());
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_YOURHOST,temp);

    temptime = server_get_starttime(); /* FIXME: This should be build time */
    temptimestr = ctime(&temptime);
    if ((25+strlen(temptimestr)+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,":This server was created %s",temptimestr); /* FIXME: is ctime() portable? */
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_CREATED,temp);
    if ((strlen(prefs_get_servername())+7+strlen(BNETD_VERSION)+9+1)<=MAX_IRC_MESSAGE_LEN)
        sprintf(temp,"%s bnetd-" BNETD_VERSION " aroO Oon",prefs_get_servername()); /* FIXME: be honest about modes :) */
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_MYINFO,temp);
    /* 251 is here */
    /* 255 is here */
    /* FIXME: show a real MOTD */
    if ((3+strlen(prefs_get_servername())+22+1)<=MAX_IRC_MESSAGE_LEN)
    	sprintf(temp,":- %s Message of the day - ",prefs_get_servername());
    else
        sprintf(temp,":Maximum length exceeded");
    irc_send(conn,RPL_MOTDSTART,temp);
    irc_send(conn,RPL_MOTD,":- This is an experimental service and a MOTD is          ");
    irc_send(conn,RPL_MOTD,":- therefore not yet supported.                           ");
    irc_send(conn,RPL_MOTD,":-                                                        ");
    irc_send(conn,RPL_MOTD,":- ====================================================== ");
    irc_send(conn,RPL_MOTD,":-     ....   .   .  .....  .....  ....                   ");
    irc_send(conn,RPL_MOTD,":-     :   :  ::  :  :        :    :   :                  ");
    irc_send(conn,RPL_MOTD,":- ....****...*.*.*..*****....*....*...*....p.r.o.j.e.c.t ");
    irc_send(conn,RPL_MOTD,":-     O   O  O  OO  O        O    O   O                  ");
    irc_send(conn,RPL_MOTD,":-     oooo   o   o  ooooo    o    oooo                   ");
    irc_send(conn,RPL_MOTD,":- ====================================================== ");
    irc_send(conn,RPL_MOTD,":-                 http://www.bnetd.org                   ");
    irc_send(conn,RPL_MOTD,":- ====================================================== ");
    irc_send(conn,RPL_MOTD,":-                                                        ");
    irc_send(conn,RPL_MOTD,":-                           - Typhoon (mziech@bnetd.org) ");
    irc_send(conn,RPL_ENDOFMOTD,":End of /MOTD command");
    irc_send_cmd(conn,"NOTICE",":This is an experimental service.");
    if (connlist_find_connection(conn_get_botuser(conn))) {
	irc_send_cmd(conn,"NOTICE","You are already logged in.");
	return -1;
    }
#ifdef WITH_BITS
    /* FIXME: lock account here*/
#else
    conn_set_account(conn,accountlist_find_account(conn_get_botuser(conn)));
    conn_set_botuser(conn,(tempname = conn_get_username(conn)));
    conn_unget_username(conn,tempname);
    irc_send_cmd(conn,"NOTICE",":Account ready.");
    conn_set_state(conn,conn_state_bot_password);
    if (conn_get_ircpass(conn)) {
	irc_send_cmd(conn,"NOTICE",":Trying to authenticate with PASS ...");
	irc_authenticate(conn,conn_get_ircpass(conn));
    } else {
    	irc_send_cmd(conn,"NOTICE",":No PASS command received. Please identify yourself by /msg NICKSERV identify <password>.");
    }
#endif
    return 0;
}

/* Channel name conversion rules: */
/* Not allowed in IRC (RFC2812): NUL, BELL, CR, LF, ' ', ':' and ','*/
/*   ' '  -> '_'      */
/*   '_'  -> '%_'     */
/*   '%'  -> '%%'     */
/*   '\b' -> '%b'     */
/*   '\n' -> '%n'     */
/*   '\r' -> '%r'     */
/*   ':'  -> '%='     */
/*   ','  -> '%-'     */
/* In IRC a channel can be specified by '#'+channelname or '!'+channelid */
 
extern char const * irc_convert_channel(t_channel const * channel)
{
    char const * bname;
    static char out[CHANNEL_NAME_LEN];
    int outpos;
    int i;
    
    if (!channel)
	return "*";

    memset(out,0,sizeof(out));
    out[0] = '#';
    outpos = 1;
    bname = channel_get_name(channel);
    for (i=0; bname[i]!='\0'; i++) {
	if (bname[i]==' ') {
	    out[outpos++] = '_';
	} else if (bname[i]=='_') { 
	    out[outpos++] = '%';
	    out[outpos++] = '_';
	} else if (bname[i]=='%') {
	    out[outpos++] = '%';
	    out[outpos++] = '%';
	} else if (bname[i]=='\b') {
	    out[outpos++] = '%';
	    out[outpos++] = 'b';
	} else if (bname[i]=='\n') {
	    out[outpos++] = '%';
	    out[outpos++] = 'n';
	} else if (bname[i]=='\r') {
	    out[outpos++] = '%';
	    out[outpos++] = 'r';
	} else if (bname[i]==':') {
	    out[outpos++] = '%';
	    out[outpos++] = '=';
	} else if (bname[i]==',') {
	    out[outpos++] = '%';
	    out[outpos++] = '-';
	} else {
	    out[outpos++] = bname[i];
	}
	if ((outpos)>=(sizeof(out)+2)) {
	    sprintf(out,"!%u",channel_get_channelid(channel));
	    return out;
	}
    }
    return out;
}

extern char const * irc_convert_ircname(char const * pircname)
{
    static char out[CHANNEL_NAME_LEN];
    int outpos;
    int special;
    int i;
    char const * ircname = pircname + 1;
    
    if (!ircname) {
	eventlog(eventlog_level_error,"irc_convert_ircname","got NULL ircname");
	return NULL;
    }

    outpos = 0;
    memset(out,0,sizeof(out));
    special = 0;
    if (pircname[0]=='!') {
	t_channel * channel;

	channel = channellist_find_channel_bychannelid(atoi(ircname));
	if (channel)
	    return channel_get_name(channel);
	else
	    return NULL;
    } else if (pircname[0]!='#') {
	return NULL;
    }
    for (i=0; ircname[i]!='\0'; i++) {
    	if (ircname[i]=='_') {
	    out[outpos++] = ' ';
    	} else if (ircname[i]=='%') {
	    if (special) {
		out[outpos++] = '%';
		special = 0;
	    } else {
		special = 1;
	    }
    	} else if (special) {
	    if (ircname[i]=='_') {
		out[outpos++] = '_';
	    } else if (ircname[i]=='b') {
		out[outpos++] = '\b';
	    } else if (ircname[i]=='n') {
		out[outpos++] = '\n';
	    } else if (ircname[i]=='r') {
		out[outpos++] = '\r';
	    } else if (ircname[i]=='=') {
		out[outpos++] = ':';
	    } else if (ircname[i]=='-') {
		out[outpos++] = ',';
	    } else {
		/* maybe it's just a typo :) */
		out[outpos++] = '%';
		out[outpos++] = ircname[i];
	    }
    	} else {
    	    out[outpos++] = ircname[i];
    	}
	if ((outpos)>=(sizeof(out)+2)) {
	    return NULL;
	}
    }
    return out;
}

/* splits an string list into its elements */
/* (list will be modified) */
static char ** irc_split_elems(char * list, int separator, int ignoreblank)
{
    int i;
    int count;
    char ** out;
    
    if (!list) {
	eventlog(eventlog_level_error,"irc_get_elems","got NULL list");
	return NULL;
    }

    for (count=0,i=0;list[i]!='\0';i++) {
	if (list[i]==separator) {
	    count++;
	    if (ignoreblank) {
	        /* ignore more than one separators "in a row" */
		while ((list[i+1]!='\0')&&(list[i]==separator)) i++;
	    }
	}
    }
    count++; /* count separators -> we have one more element ... */
    /* we also need a terminating element */
    out = malloc((count+1)*sizeof(char *));
    if (!out) {
	eventlog(eventlog_level_error,"irc_get_elems","could not allocate memory for elements: %s",strerror(errno));
	return NULL;
    }

    out[0] = list;
    if (count>1) {
	for (i=1;i<count;i++) {
	    out[i] = strchr(out[i-1],separator);
	    if (!out[i]) {
		eventlog(eventlog_level_error,"irc_get_elems","BUG: wrong number of separators");
		free(out);
		return NULL;
	    }
	    if (ignoreblank)
	    	while ((*out[i]+1)==separator) out[i]++;
	    *out[i]++ = '\0';
	}
	if ((ignoreblank)&&(out[count-1])&&(*out[count-1]=='\0')) {
	    out[count-1] = NULL; /* last element is blank */
	}
    } else if ((ignoreblank)&&(*out[0]=='\0')) {
	out[0] = NULL; /* now we have 2 terminators ... never mind */
    }
    out[count] = NULL; /* terminating element */
    return out;
}

static int irc_unget_elems(char ** elems)
{
    if (!elems) {
	eventlog(eventlog_level_error,"irc_unget_elems","got NULL elems");
	return -1;
    }
    free(elems);
    return 0;
}

extern char ** irc_get_listelems(char * list)
{
    return irc_split_elems(list,',',0);
}

extern int irc_unget_listelems(char ** elems)
{
    return irc_unget_elems(elems);
}

extern char ** irc_get_paramelems(char * list)
{
    return irc_split_elems(list,' ',1);
}

extern int irc_unget_paramelems(char ** elems)
{
    return irc_unget_elems(elems);
}

static char * irc_message_preformat(t_irc_message_from const * from, char const * command, char const * dest, char const * text)
{
    char * myfrom = NULL;
    char const * mydest = "";
    char const * mytext = "";
    int len;
    char * msg;

    if (!command) {
	eventlog(eventlog_level_error,"irc_message_preformat","got NULL command");
	return NULL;
    }
    if (from) {
	if ((!from->nick)||(!from->user)||(!from->host)) {
	    eventlog(eventlog_level_error,"irc_message_preformat","got malformed from");
	    return NULL;
	}
	myfrom = malloc(strlen(from->nick)+1+strlen(from->user)+1+strlen(from->host)+1); /* nick + "!" + user + "@" + host + "\0" */
	if (!myfrom) {
	    eventlog(eventlog_level_error,"irc_message_preformat","could not allocate memory: %s",strerror(errno));
	    return NULL;
	}
	sprintf(myfrom,"%s!%s@%s",from->nick,from->user,from->host);
    } else
    	myfrom = strdup(prefs_get_servername());
    if (dest)
    	mydest = dest;
    if (text)
    	mytext = text;

    len = 1+strlen(myfrom)+1+
    	  strlen(command)+1+
    	  strlen(mydest)+1+
    	  1+strlen(mytext)+1;

     
    if (!(msg = malloc(len))) {
        eventlog(eventlog_level_error,"irc_message_preformat","could not allocate memory for message: %s",strerror(errno));
        free(myfrom);
        return NULL;
    }
    sprintf(msg,":%s\n%s\n%s\n:%s",myfrom,command,mydest,mytext);
    return msg;
}

extern int irc_message_postformat(t_packet * packet, t_connection const * dest)
{
    int len;
    /* the four elements */
    char * e1;
    char * e2;
    char * e3;
    char * e4;
    char const * tname = NULL;
    char const * toname = "AUTH"; /* fallback name */

    if (!packet) {
	eventlog(eventlog_level_error,"irc_message_postformat","got NULL packet");
	return -1;
    }
    if (!dest) {
	eventlog(eventlog_level_error,"irc_message_postformat","got NULL dest");
	return -1;
    }

    e1 = packet_get_raw_data(packet,0);
    e2 = strchr(e1,'\n');
    if (!e2) {
	eventlog(eventlog_level_warn,"irc_message_postformat","malformed message (e2 missing)");
	return -1;
    }
    *e2++ = '\0';
    e3 = strchr(e2,'\n');
    if (!e3) {
	eventlog(eventlog_level_warn,"irc_message_postformat","malformed message (e3 missing)");
	return -1;
    }
    *e3++ = '\0';
    e4 = strchr(e3,'\n');
    if (!e4) {
	eventlog(eventlog_level_warn,"irc_message_postformat","malformed message (e4 missing)");
	return -1;
    }
    *e4++ = '\0';

    if (e3[0]=='\0') { /* fill in recipient */
    	if ((tname = conn_get_chatname(dest)))
    	    toname = tname;
    } else
    	toname = e3;

    if (strcmp(toname,"\r")==0) {
	toname = ""; /* HACK: the target field is really empty */
    }
    	
    len = (strlen(e1)+1+strlen(e2)+1+strlen(toname)+1+strlen(e4)+2+1);
    if (len<=MAX_IRC_MESSAGE_LEN) {
	char msg[MAX_IRC_MESSAGE_LEN+1];

	sprintf(msg,"%s %s %s %s\r\n",e1,e2,toname,e4);
	eventlog(eventlog_level_debug,"irc_message_postformat","sent \"%s\"",msg);
	packet_set_size(packet,0);
	packet_append_data(packet,msg,strlen(msg));
	if (tname)
	    conn_unget_chatname(dest,tname);
	return 0;
    } else {
	/* FIXME: split up message? */
    	eventlog(eventlog_level_warn,"irc_message_postformat","maximum IRC message length exceeded");
	if (tname)
	    conn_unget_chatname(dest,tname);
	return -1;
    }
}

extern int irc_message_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags)
{
    char * msg;
    t_irc_message_from from;
    
    if (!packet)
    {
	eventlog(eventlog_level_error,"message_irc_format","got NULL packet");
	return -1;
    }

    msg = NULL;
        
    switch (type)
    {
    /* case message_type_adduser: this is sent manually in handle_irc */
    case message_type_join:
    	from.nick = conn_get_chatname(me);
    	from.user = conn_get_clienttag(me);
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"JOIN","\r",irc_convert_channel(conn_get_channel(me)));
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_type_part:
    	from.nick = conn_get_chatname(me);
    	from.user = conn_get_clienttag(me);
    	from.host = addr_num_to_ip_str(conn_get_addr(me));
    	msg = irc_message_preformat(&from,"PART","\r",irc_convert_channel(conn_get_channel(me)));
    	conn_unget_chatname(me,from.nick);
    	break;
    case message_type_talk:
    case message_type_whisper:
    	{
    	    char const * dst;
    	    from.nick = conn_get_chatname(me);
            from.user = conn_get_clienttag(me);
    	    from.host = addr_num_to_ip_str(conn_get_addr(me));
    	    if (type==message_type_talk)
    	    	dst = irc_convert_channel(conn_get_channel(me)); /* FIXME: support more channels and choose right one! */
	    else
	        dst = ""; /* will be replaced with username in postformat */
    	    msg = irc_message_preformat(&from,"PRIVMSG",dst,text);
    	    conn_unget_chatname(me,from.nick);
    	}
        break;
    case message_type_emote:
    	{
    	    char const * dst;
	    char temp[MAX_IRC_MESSAGE_LEN];

    	    /* "\001ACTION " + text + "\001" + \0 */
	    if ((8+strlen(text)+1+1)<=MAX_IRC_MESSAGE_LEN) {
		sprintf(temp,"\001ACTION %s\001",text);
	    } else {
		sprintf(temp,"\001ACTION (maximum message length exceeded)\001");
	    }
    	    from.nick = conn_get_chatname(me);
            from.user = conn_get_clienttag(me);
    	    from.host = addr_num_to_ip_str(conn_get_addr(me));
    	    /* FIXME: also supports whisper emotes? */
    	    dst = irc_convert_channel(conn_get_channel(me)); /* FIXME: support more channels and choose right one! */
	    msg = irc_message_preformat(&from,"PRIVMSG",dst,temp);
    	    conn_unget_chatname(me,from.nick);
    	}
        break;
    case message_type_broadcast:
    case message_type_info:
    case message_type_error:
	msg = irc_message_preformat(NULL,"NOTICE",NULL,text);
	break;
    case message_type_channel:
    	/* ignore it */
	break;
    default:
    	eventlog(eventlog_level_warn,"irc_message_format","%d not yet implemented",type);
	return -1;
    }

    if (msg) {
	packet_append_string(packet,msg);
	free(msg);
        return 0;
    }
    return -1;
}

extern int irc_send_rpl_namreply(t_connection * c, t_channel const * channel)
{
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * ircname;
    int first = 1;
#ifdef WITH_BITS
    t_bits_channelmember * m;
#else
    t_connection * m;
#endif

    if (!c) {
	eventlog(eventlog_level_error,"irc_send_rpl_namreply","got NULL connection");
	return -1;
    }
    if (!channel) {
	eventlog(eventlog_level_error,"irc_send_rpl_namreply","got NULL channel");
	return -1;
    }
    memset(temp,0,sizeof(temp));
    ircname = irc_convert_channel(channel);
    if (!ircname) {
	eventlog(eventlog_level_error,"irc_send_rpl_namreply","channel has NULL ircname");
	return -1;
    }
    /* '@' = secret; '*' = private; '=' = public */
    if ((1+1+strlen(ircname)+2+1)<=MAX_IRC_MESSAGE_LEN) {
	sprintf(temp,"%c %s :",((channel_get_permanent(channel))?('='):('*')),ircname);
    } else {
	eventlog(eventlog_level_warn,"irc_send_rpl_namreply","maximum message length exceeded");
	return -1;
    }
    /* FIXME: Add per user flags (@(op) and +(voice))*/
#ifdef WITH_BITS  
    for (m = channel_get_first(channel);m;m = channel_get_next()) {
	char const * name = bits_loginlist_get_name_bysessionid(m->sessionid);
	char flg[3] = "";
	
	if (!name)
	    continue;
	if (m->flags & MF_GAVEL)
	    strcat(flg,"@"); 
	if (m->flags & MF_VOICE)
	    strcat(flg,"+"); 
	if ((strlen(temp)+((!first)?(1):(0))+strlen(flg)+strlen(name)+1)<=sizeof(temp)) {
	    if (!first) strcat(temp," ");
	    strcat(temp,flg);
	    strcat(temp,name);
	    first = 0;
	}
    } 
#else
    for (m = channel_get_first(channel);m;m = channel_get_next()) {
	char const * name = conn_get_chatname(m);
	char flg[3] = "";
	
	if (!name)
	    continue;
	if (conn_get_flags(m) & MF_GAVEL)
	    strcat(flg,"@"); 
	if (conn_get_flags(m) & MF_VOICE)
	    strcat(flg,"+"); 
	if ((strlen(temp)+((!first)?(1):(0))+strlen(flg)+strlen(name)+1)<=sizeof(temp)) {
	    if (!first) strcat(temp," ");
	    strcat(temp,flg);
	    strcat(temp,name);
	    first = 0;
	}
	conn_unget_chatname(m,name);
    } 
#endif
    irc_send(c,RPL_NAMREPLY,temp);
    return 0;
}

static int irc_who_connection(t_connection * dest, t_connection * c)
{
    /* FIXME: maybe we can give some more accurate info with BITS ... */
    t_account * a;
    char const * tempuser;
    char const * tempowner;
    char const * tempname;
    char const * tempip;
    char const * tempflags = "@"; /* FIXME: that's dumb */
    char temp[MAX_IRC_MESSAGE_LEN];
    char const * tempchannel;
    
    if (!dest) {
	eventlog(eventlog_level_error,"irc_who_connection","got NULL destination");
	return -1;
    }
    if (!c) {
	eventlog(eventlog_level_error,"irc_who_connection","got NULL connection");
	return -1;
    }
    a = conn_get_account(c);
    /*tempuser = account_get_ll_user(a);*/
    tempuser = conn_get_clienttag(c);
    tempowner = account_get_ll_owner(a);
    tempname = conn_get_username(c);
    tempip = addr_num_to_ip_str(conn_get_addr(c));
    tempchannel = irc_convert_channel(conn_get_channel(c));
    if ((strlen(tempchannel)+1+strlen(tempuser)+1+strlen(tempip)+1+strlen(prefs_get_servername())+1+strlen(tempname)+1+1+strlen(tempflags)+4+strlen(tempowner)+1)>MAX_IRC_MESSAGE_LEN) {
	eventlog(eventlog_level_info,"irc_who_connection","WHO reply too long");
    } else
        sprintf(temp,"%s %s %s %s %s %c%s :0 %s",tempchannel,tempuser,tempip,prefs_get_servername(),tempname,'H',tempflags,tempowner);
    /*account_unget_ll_user(tempuser);*/
    account_unget_ll_owner(tempowner);
    conn_unget_username(c,tempname);
    irc_send(dest,RPL_WHOREPLY,temp);
    return 0;
}

extern int irc_who(t_connection * c, char const * name)
{
    /* FIXME: support wildcards! */
    /* FIXME: support BITS! */

    if (!c) {
	eventlog(eventlog_level_error,"irc_who","got NULL connection");
	return -1;
    }
    if (!name) {
	eventlog(eventlog_level_error,"irc_who","got NULL name");
	return -1;
    }
    if ((name[0]=='#')||(name[0]=='&')||(name[0]=='!')) {
	/* it's a channel */
	t_connection * info;
	t_channel * channel;
	char const * ircname;
	
	ircname = irc_convert_ircname(name);
	channel = channellist_find_channel_by_name(ircname,NULL,NULL);
	if (!channel) {
	    char temp[MAX_IRC_MESSAGE_LEN];

	    if ((strlen(":No such channel")+1+strlen(name)+1)<=MAX_IRC_MESSAGE_LEN) {
		sprintf(temp,":No such channel %s",name);
		irc_send(c,ERR_NOSUCHCHANNEL,temp);
	    } else {
		irc_send(c,ERR_NOSUCHCHANNEL,":No such channel");
	    }
	    return 0;
	}
	for (info = channel_get_first(channel);info;info = channel_get_next()) {
	    irc_who_connection(c,info);
	}
    } else {
	/* it's just one user */
	t_connection * info;
	
	if ((info = connlist_find_connection(name)))
	    return irc_who_connection(c,info);	
    }
    return 0;
}


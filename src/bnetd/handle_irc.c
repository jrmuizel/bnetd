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
#define CONNECTION_INTERNAL_ACCESS
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
#include "message.h"
#include "account.h"
#include "channel.h"
#include "handle_irc.h"
#include "irc.h"
#include "prefs.h"
#include "tick.h"
#include "timer.h"
#include "common/setup_after.h"

static int handle_irc_line(t_connection * conn, char const * ircline)
{   /* [:prefix] <command> [[param1] [param2] ... [paramN]] [:<text>]*/
    char * line; /* copy of ircline */
    char * prefix = NULL; /* optional; mostly NULL */
    char * command = NULL; /* mandatory */
    char ** params = NULL; /* optional (array of params) */
    char * text = NULL; /* optional */
    int unrecognized_before = 0;

    int numparams = 0;
    char * tempparams;

    if (!conn) {
	eventlog(eventlog_level_error,"handle_irc_line","got NULL connection");
	return -1;
    }
    if (!ircline) {
	eventlog(eventlog_level_error,"handle_irc_line","got NULL ircline");
	return -1;
    }
    if (ircline[0] == '\0') {
	eventlog(eventlog_level_error,"handle_irc_line","got empty ircline");
	return -1;
    }
    if (!(line = strdup(ircline))) {
	eventlog(eventlog_level_error,"handle_irc_line","could not allocate memory for line");
	return -1;
    }

    /* split the message */
    if (line[0] == ':') {
	/* The prefix is optional and is rarely provided */
	prefix = line;
	if (!(command = strchr(line,' '))) {
	    eventlog(eventlog_level_warn,"handle_irc_line","got malformed line (missing command)");
	    /* FIXME: send message instead of eventlog? */
	    free(line);
	    return -1;
	}
	*command++ = '\0';
    } else {
	/* In most cases command is the first thing on the line */
	command = line;
    }
    
    tempparams = strchr(command,' ');
    if (tempparams) {
	int i;
	
	*tempparams++ = '\0';
	if (tempparams[0]==':') {
	    text = tempparams+1; /* theres just text, no params. skip the colon */
	} else {
	    for (i=0;tempparams[i]!='\0';i++) {
	    	if ((tempparams[i]==' ')&&(tempparams[i+1]==':')) {
		    text = tempparams+i; 
		    *text++ = '\0';
		    text++; /* skip the colon */
		    break; /* text found, stop search */
	    	}
	    }
	    params = irc_get_paramelems(tempparams);
	}
    }

    if (params) {
	/* count parameters */
	for (numparams=0;params[numparams];numparams++);
    }

    /*eventlog(eventlog_level_debug,"handle_irc_line","[%d] got \"%s\"",conn_get_socket(conn),ircline);*/
    {
	int i;
    	char paramtemp[MAX_IRC_MESSAGE_LEN*2];
	int first = 1;

	memset(paramtemp,0,sizeof(paramtemp));
    	for (i=0;((numparams>0)&&(params[i]));i++) {
	    if (!first) strcat(paramtemp," ");
	    strcat(paramtemp,"\"");
	    strcat(paramtemp,params[i]);
	    strcat(paramtemp,"\"");
	    first = 0;
    	}
    	eventlog(eventlog_level_debug,"handle_irc_line","[%d] got \"%s\" \"%s\" [%s] \"%s\"",conn_get_socket(conn),((prefix)?(prefix):("")),command,paramtemp,((text)?(text):("")));
    }
    
    if (conn_get_state(conn)==conn_state_initial) {
	t_timer_data temp;
	
	conn_set_state(conn,conn_state_bot_username);
	temp.n = prefs_get_irc_latency();
	conn_test_latency(conn,time(NULL),temp);
    }

    if (strcmp(command,"NICK")==0) {
    	/* FIXME: more strict param checking */
	char const * temp;

	if ((temp = conn_get_botuser(conn))) {
	    irc_send(conn,ERR_RESTRICTED,":You can only set your nickname once");
	} else {
	    if ((params)&&(params[0]))
		conn_set_botuser(conn,params[0]);
	    else if (text)
		conn_set_botuser(conn,text);
	    else
	        irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to NICK");
	    if ((conn_get_user(conn))&&(conn_get_botuser(conn)))
		irc_welcome(conn); /* only send the welcome if we have USER and NICK */
	}
    } else if (strcmp(command,"USER")==0) {
	/* RFC 2812 says: */
	/* <user> <mode> <unused> :<realname>*/
	/* ircII and X-Chat say: */
	/* mz SHODAN localhost :Marco Ziech */
	/* BitchX says: */
	/* mz +iws mz :Marco Ziech */
	/* What does this tell us? Ditch mode and unused, they don't contain what they should. */
	char * user = NULL;
	char * mode = NULL;
	char * unused = NULL;
	char * realname = NULL;

	if ((numparams>=3)&&(params[0])&&(params[1])&&(params[2])&&(text)) {
	    user = params[0];
	    mode = params[1];
	    unused = params[2];
	    realname = text;
            if (conn_get_user(conn)) {
		irc_send(conn,ERR_ALREADYREGISTRED,":You are already registred");
            } else {
		eventlog(eventlog_level_debug,"handle_irc_line","[%d] got USER: user=\"%s\" mode=\"%s\" unused=\"%s\" realname=\"%s\"",conn_get_socket(conn),user,mode,unused,realname);
		conn_set_user(conn,user);
		conn_set_owner(conn,realname);
		if (conn_get_botuser(conn))
		    irc_welcome(conn); /* only send the welcome if we have USER and NICK */
	    }
        } else {
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to USER");
        }
    } else if (strcmp(command,"PING")==0) {
	/* NOTE: RFC2812 doesn't seem to be very expressive about this ... */
	if (text) {
	    if (strcmp(text,prefs_get_servername())!=0)
	        irc_send(conn,ERR_NOSUCHSERVER,":No such server"); /* We don't know other servers */
	    else
	        irc_send_pong(conn,params[0]);
	}
    } else if (strcmp(command,"PONG")==0) {
	/* NOTE: RFC2812 doesn't seem to be very expressive about this ... */
	if (conn_get_ircping(conn)==0) {
	    eventlog(eventlog_level_warn,"handle_irc_line","[%d] PONG without PING",conn_get_socket(conn));
	} else {
	    unsigned int val = 0;

	    if (text) val = atoi(text);
	    if (numparams>=1) val = atoi(params[0]);

	    if (conn_get_ircping(conn) != val) {
	    	if ((text)&&(strcmp(text,prefs_get_servername())!=0)) {
		    /* Actually the servername should not be always accepted but we aren't that pedantic :) */
		    eventlog(eventlog_level_warn,"handle_irc_line","[%d] got bad PONG (%u!=%u)",conn_get_socket(conn),val,conn_get_ircping(conn));
		    return -1;
		}
	    }
	    conn_set_latency(conn,get_ticks()-conn_get_ircping(conn));
	    eventlog(eventlog_level_debug,"handle_irc_line","[%d] latency is now %d (%u-%u)",conn_get_socket(conn),get_ticks()-conn_get_ircping(conn),get_ticks(),conn_get_ircping(conn));
	    conn_set_ircping(conn,0);
	}
    } else if (strcmp(command,"PASS")==0) {
    	if ((!conn_get_ircpass(conn))&&(conn_get_state(conn)==conn_state_bot_username)) {
	    t_hash h;

	    if (numparams>=1) {
		bnet_hash(&h,strlen(params[0]),params[0]);
		conn_set_ircpass(conn,hash_get_str(h));
	    } else
		irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to PASS");
    	} else {
	    eventlog(eventlog_level_warn,"handle_irc_line","[%d] client tried to set password twice with PASS",conn_get_socket(conn));
    	}
    } else if (strcmp(command,"PRIVMSG")==0) {
	if ((numparams>=1)&&(text)) {
	    int i;
	    char ** e;
	
	    e = irc_get_listelems(params[0]);
	    /* FIXME: support wildcards! */
	    for (i=0;((e)&&(e[i]));i++) {
	    	if ((conn_get_state(conn)==conn_state_bot_password)&&(strcasecmp(e[i],"NICKSERV")==0)) {
		    char * pass;
		    pass = strchr(text,' ');
		    if (pass)
		    	*pass++ = '\0';
		    if ((pass)&&(strcasecmp(text,"identify")==0)) {
		    	t_hash h;

		    	bnet_hash(&h,strlen(pass),pass);
		    	irc_authenticate(conn,hash_get_str(h));
		    } else {
		        irc_send_cmd(conn,"NOTICE",":Invalid arguments for NICKSERV");
		    }
	        } else if (conn_get_state(conn)==conn_state_loggedin) {
		    if (e[i][0]=='#') {
		        /* channel message */
			t_channel * channel;

			/* HACK: we have to filter CTCP ACTION messages since 
			 * most not-IRC clients won't support them. However,
			 * since the original CTCP specification (http://www.irchelp.org/irchelp/rfs/ctcpspec.html)
			 * describes the users of this command as "losers" we might have
			 * to do some other stuff here, like for example setting
			 * a special loser flag in the account or showing them
			 * a "winners guide" to at least help them to win games ;)
			 */
			if ((channel = channellist_find_channel_by_name(irc_convert_ircname(e[i]),NULL,NULL))) {
			    if ((strlen(text)>=9)&&(strncmp(text,"\001ACTION ",8)==0)&&(text[strlen(text)-1]=='\001')) { /* at least "\001ACTION \001" */
				/* it's a CTCP ACTION message */
				text = text + 8;
				text[strlen(text)-1] = '\0';
				channel_message_send(channel,message_type_emote,conn,text);
			    } else
				channel_message_send(channel,message_type_talk,conn,text);
			} else {
			    irc_send(conn,ERR_NOSUCHCHANNEL,":No such channel");
			}				
	    	    } else {
			/* whisper */
			t_connection * user;
#ifdef WITH_BITS
			/* FIXME: support BITS! */
#endif

			if ((user = connlist_find_connection(e[i]))) {
			    message_send_text(user,message_type_whisper,conn,text);
			} else {
			    irc_send(conn,ERR_NOSUCHNICK,":No such user");
			}
	    	    }
	        }
	    }
	    if (e)
	         irc_unget_listelems(e);
	} else
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to PRIVMSG");
    } else if (conn_get_state(conn)!=conn_state_loggedin) {
	irc_send(conn,ERR_UNKNOWNCOMMAND,":Unrecognized command (before login)");
    } else {
        /* command is handled later */
	unrecognized_before = 1;
    }
    /* --- The following should only be executable after login --- */
    if ((conn_get_state(conn)==conn_state_loggedin)&&(unrecognized_before)) {
    if (strcmp(command,"WHO")==0) {
	if (numparams>=1) {
	    int i;
	    char ** e;
	    int oflag = 0;

	    if ((numparams>=2)&&(strcmp(params[1],"o"))==0)
	    	oflag = 1;
	    e = irc_get_listelems(params[0]);
	    for (i=0; ((e)&&(e[i]));i++) {
	    	irc_who(conn,e[i]);
	    }
	    irc_send(conn,RPL_ENDOFWHO,":End of WHO list"); /* RFC2812 only requires this to be sent if a list of names was given. Undernet seems to always send it, so do we :) */
            if (e)
                 irc_unget_listelems(e);
	} else 
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to WHO");
    } else if (strcmp(command,"LIST")==0) {
        char temp[MAX_IRC_MESSAGE_LEN];

	irc_send(conn,RPL_LISTSTART,"Channel :Users  Name"); /* backward compatibility */
	if (numparams==0) {
 	    t_elem const * curr;
 	    
   	    LIST_TRAVERSE_CONST(channellist(),curr) {
    	        t_channel const * channel = elem_get_data(curr);
	        char const * tempname;
	        char const * temptopic = "Topic not yet implemented."; /* FIXME: */

	        tempname = irc_convert_channel(channel);
	        if (strlen(tempname)+1+20+1+1+strlen(temptopic)<MAX_IRC_MESSAGE_LEN)
		    sprintf(temp,"%s %u :%s",tempname,channel_get_length(channel),temptopic);
	        else
	            eventlog(eventlog_level_warn,"handle_irc_line","LISTREPLY length exceeded");
	        irc_send(conn,RPL_LIST,temp);
    	    }
    	} else if (numparams>=1) {
    	    int i;
    	    char ** e;
 
	    /* FIXME: support wildcards! */
	    e = irc_get_listelems(params[0]);
	    for (i=0;((e)&&(e[i]));i++) {
	    	t_channel const * channel;
	    	char const * verytemp; /* another good example for creative naming conventions :) */
	        char const * tempname;
	        char const * temptopic = "Topic not yet implemented."; /* FIXME: */
		
		verytemp = irc_convert_ircname(e[i]);
		if (!verytemp)
		    continue; /* something is wrong with the name ... */
		channel = channellist_find_channel_by_name(verytemp,NULL,NULL);
		if (!channel)
		    continue; /* channel doesn't exist */
	        tempname = irc_convert_channel(channel);
	        if (strlen(tempname)+1+20+1+1+strlen(temptopic)<MAX_IRC_MESSAGE_LEN)
		    sprintf(temp,"%s %u :%s",tempname,channel_get_length(channel),temptopic);
	        else
	            eventlog(eventlog_level_warn,"handle_irc_line","LISTREPLY length exceeded");
	        irc_send(conn,RPL_LIST,temp);
	    }
            if (e)
                 irc_unget_listelems(e);
    	}
    	irc_send(conn,RPL_LISTEND,":End of LIST command");
    } else if (strcmp(command,"JOIN")==0) {
	if (numparams>=1) {
	    char ** e;

	    e = irc_get_listelems(params[0]);
	    /* FIXME: support being in more than one channel at once */
	    if ((e)&&(e[0])) {
	    	char const * ircname = irc_convert_ircname(e[0]);
		if (conn_set_channel(conn,ircname)<0) {
		    irc_send(conn,ERR_NOSUCHCHANNEL,":JOIN failed"); /* FIXME: be more precise; what is the real error code for that? */
		} else {
		    t_channel * channel;

		    channel = conn_get_channel(conn);
		    if (channel) {
		    	char temp[MAX_IRC_MESSAGE_LEN];
		    	
			if ((strlen(ircname)+1+strlen(":Not yet implemented.")+1)<MAX_IRC_MESSAGE_LEN) {
			    sprintf(temp,"%s :Not yet implemented.",ircname);
			    irc_send(conn,RPL_TOPIC,temp);
			}
			if ((strlen(ircname)+1+strlen("FIXME 0")+1)<MAX_IRC_MESSAGE_LEN) {
			    sprintf(temp,"%s FIXME 0",ircname);
			    irc_send(conn,RPL_TOPICWHOTIME,temp); /* FIXME: this in an undernet extension but other servers support it too */
			}
			message_send_text(conn,message_type_join,conn,NULL); /* we have to send the JOIN acknowledgement */
			irc_send_rpl_namreply(conn,channel);
			irc_send(conn,RPL_ENDOFNAMES,":End of NAMES list");
		    }
		}
	    }
            if (e)
                 irc_unget_listelems(e);
	} else 
	    irc_send(conn,ERR_NEEDMOREPARAMS,":Too few arguments to JOIN");
    } else if (strcmp(command,"MODE")==0) {
	/* FIXME: Not yet implemented */
    } else if (strcmp(command,"USERHOST")==0) {
	/* FIXME: Send RPL_USERHOST */
    } else if (strcmp(command,"QUIT")==0) {
	/* FIXME: Not yet implemented */
    } else {
	irc_send(conn,ERR_UNKNOWNCOMMAND,":Unrecognized command");
    }
    } /* loggedin */
    if (params)
	irc_unget_paramelems(params);
    free(line);
    return 0;
}

extern int handle_irc_packet(t_connection * conn, t_packet const * const packet)
{
    int i;
    char ircline[MAX_IRC_MESSAGE_LEN];
    int ircpos;
    char const * data;

    if (!packet) {
	eventlog(eventlog_level_error,"handle_irc_packet","got NULL packet");
	return -1;
    }
    if (conn_get_class(conn) != conn_class_irc ) {
	eventlog(eventlog_level_error,"handle_irc_packet","FIXME: handle_irc_packet without any reason (conn->class != conn_class_irc)");
	return -1;
    }
	
    /* eventlog(eventlog_level_debug,"handle_irc_packet","got \"%s\"",packet_get_raw_data_const(packet,0)); */

    memset(ircline,0,sizeof(ircline));
    data = conn_get_ircline(conn); /* fetch current status */
    if (data)
	strcpy(ircline,data);
    ircpos = strlen(ircline);
    data = packet_get_raw_data_const(packet,0);	
    for (i=0; i < packet_get_size(packet); i++) {
	if ((data[i] == '\r')||(data[i] == '\0')) {
	    /* kindly ignore \r and NUL ... */
	} else if (data[i] == '\n') {
	    /* end of line */
	    handle_irc_line(conn,ircline);
	    memset(ircline,0,sizeof(ircline));
	    ircpos = 0;
	} else {
	    if (ircpos < MAX_IRC_MESSAGE_LEN-1)
		ircline[ircpos++] = data[i];
	    else {
		ircpos++; /* for the statistic :) */
	    	eventlog(eventlog_level_warn,"handle_irc_packet","[%d] client exceeded maximum allowed message length by %d characters",conn_get_socket(conn),ircpos-MAX_IRC_MESSAGE_LEN);
		if ((ircpos-MAX_IRC_MESSAGE_LEN)>100) {
		    /* automatic flood protection */
		    eventlog(eventlog_level_error,"handle_irc_packet","[%d] excess flood",conn_get_socket(conn));
		    return -1;
		}
	    }
	}
    }
    conn_set_ircline(conn,ircline); /* write back current status */
    return 0;
}


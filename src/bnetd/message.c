/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
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
#define MESSAGE_INTERNAL_ACCESS
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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/gethostname.h"
#include <errno.h>
#include "compat/strerror.h"
#include "connection.h"
#include "common/bn_type.h"
#include "common/queue.h"
#include "common/packet.h"
#include "common/bot_protocol.h"
#include "common/bnet_protocol.h"
#include "common/field_sizes.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/util.h"
#include "common/version.h"
#include "common/addr.h"
#include "account.h"
#include "game.h"
#include "channel.h"
#include "channel_conv.h"
#include "command.h"
#include "irc.h"
#include "message.h"
#include "common/setup_after.h"


static int message_telnet_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags);
static int message_bot_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags);
static int message_bnet_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags);
static t_packet * message_cache_lookup(t_message * message, t_conn_class class, unsigned int flags);

static char const * message_type_get_str(t_message_type type)
{
    switch (type)
    {
    case message_type_adduser:
        return "adduser";
    case message_type_join:
        return "join";
    case message_type_part:
        return "part";
    case message_type_whisper:
        return "whisper";
    case message_type_talk:
        return "talk";
    case message_type_broadcast:
        return "broadcast";
    case message_type_channel:
        return "channel";
    case message_type_userflags:
        return "userflags";
    case message_type_whisperack:
        return "whisperack";
    case message_type_channelfull:
        return "channelfull";
    case message_type_channeldoesnotexist:
        return "channeldoesnotexist";
    case message_type_channelrestricted:
        return "channelrestricted";
    case message_type_info:
        return "info";
    case message_type_error:
        return "error";
    case message_type_emote:
        return "emote";
    case message_type_uniqueid:
        return "uniqueid";
    case message_type_null:
        return "null";
    default:
        return "UNKNOWN";
    }
}


/* make sure none of the expanded format symbols is longer than this (with null) */
#define MAX_INC 64

extern char * message_format_line(t_connection const * c, char const * in)
{
    char *       out;
    unsigned int inpos;
    unsigned int outpos;
    unsigned int outlen=MAX_INC;
    
    if (!(out = malloc(outlen+1)))
    {
	free((void *)in); /* avoid warning */
	return NULL;
    }
    
    out[0] = 'I';
    for (inpos=0,outpos=1; inpos<strlen(in); inpos++)
    {
        if (in[inpos]!='%')
	{
	    out[outpos] = in[inpos];
	    outpos += 1;
	}
        else
	    switch (in[++inpos])
	    {
	    case '%':
		out[outpos++] = '%';
		break;
		
	    case 'a':
		sprintf(&out[outpos],"%d",accountlist_get_length());
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'c':
		sprintf(&out[outpos],"%d",channellist_get_length());
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'g':
		sprintf(&out[outpos],"%d",gamelist_get_length());
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'h':
    		if (gethostname(&out[outpos],MAX_INC)<0)
    		{
		    eventlog(eventlog_level_error,"message_format_line","could not get hostname (gethostname: %s)",strerror(errno));
		    strcpy(&out[outpos],"localhost"); /* not much else you can do */
    		}
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'i':
		sprintf(&out[outpos],UID_FORMAT,conn_get_userid(c));
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'l':
	        {
		    char const * tname;
		    
		    strncpy(&out[outpos],(tname = conn_get_chatname(c)),USER_NAME_MAX);
		    conn_unget_chatname(c,tname);
		}
		out[outpos+USER_NAME_MAX-1] = '\0';
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'r':
		strncpy(&out[outpos],addr_num_to_ip_str(conn_get_addr(c)),MAX_INC);
		out[outpos+MAX_INC-1] = '\0';
		outpos += strlen(&out[outpos]);
		break;
		
	    case 't':
		sprintf(&out[outpos],"%s",conn_get_clienttag(c));
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'u':
		sprintf(&out[outpos],"%d",connlist_login_get_length());
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'v':
		strcpy(&out[outpos],BNETD_VERSION);
		outpos += strlen(&out[outpos]);
		break;
		
	    case 'C': /* simulated command */
		out[0] = 'C';
		break;
		
	    case 'B': /* BROADCAST */
		out[0] = 'B';
		break;
		
	    case 'E': /* ERROR */
		out[0] = 'E';
		break;
		
	    case 'I': /* INFO */
		out[0] = 'I';
		break;
		
	    case 'M': /* MESSAGE */
		out[0] = 'M';
		break;
		
	    case 'T': /* EMOTE */
		out[0] = 'T';
		break;
		
	    case 'W': /* INFO */
		out[0] = 'W';
		break;
		
	    default:
		eventlog(eventlog_level_warn,"message_format_line","bad formatter \"%%%c\"",in[inpos-1]);
	    }
	
	if ((outpos+MAX_INC)>=outlen)
	{
	    char * newout;
	    
	    outlen += MAX_INC;
	    if (!(newout = realloc(out,outlen)))
	    {
		free((void *)in); /* avoid warning */
		free(out);
		return NULL;
	    }
	    out = newout;
	}
    }
    out[outpos] = '\0';
    
    free((void *)in); /* avoid warning */
    
    return out;
}


static int message_telnet_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags)
{
    char * msgtemp;
    
    if (!packet)
    {
	eventlog(eventlog_level_error,"message_telnet_format","got NULL packet");
	return -1;
    }
    
    switch (type)
    {
    case message_type_uniqueid:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (!(msgtemp = malloc(strlen(text)+32)))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
        sprintf(msgtemp,"Your unique name: %s\r\n",text);
	break;
    case message_type_adduser:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    if (!(msgtemp = malloc(strlen(tname)+32)))
	    {
		eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"[%s is here]\r\n",tname);
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_join:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    if (!(msgtemp = malloc(strlen(tname)+32)))
	    {
		eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"[%s enters]\r\n",tname);
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_part:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    if (!(msgtemp = malloc(strlen(tname)+32)))
	    {
		eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"[%s leaves]\r\n",tname);
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_whisper:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
	{
	    char const * tname;
	    char const * newtext;
	    
	    tname = conn_get_chatcharname(me, dst);
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(strlen(tname)+8+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    free((void *)newtext); /* avoid warning */
		    return -1;
		}
		sprintf(msgtemp,"<from %s> %s\r\n",tname,newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(16+strlen(tname))))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<from %s> \r\n",tname);
	    }
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_talk:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
	{
	    char const * tname;
	    char const * newtext;
	    
	    tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(strlen(tname)+4+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<%s> %s\r\n",tname,newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(strlen(tname)+8)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<%s> \r\n",tname);
	    }
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_broadcast:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
	{
	    char const * newtext;
	    
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(16+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"Broadcast: %s\r\n",newtext); /* FIXME: show source? */
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(16)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"Broadcast: \r\n"); /* FIXME: show source? */
	    }
	}
	break;
    case message_type_channel:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (!(msgtemp = malloc(strlen(text)+32)))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
	sprintf(msgtemp,"Joining channel: \"%s\"\r\n",text);
	break;
    case message_type_userflags:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	/* FIXME: should we show any of these? */
	if (!(msgtemp = strdup("")))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
	break;
    case message_type_whisperack:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * tname;
	    char const * newtext;
	    
	    tname = conn_get_chatcharname(me, dst);
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(strlen(tname)+8+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<to %s> %s\r\n",tname,newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(strlen(tname)+8+strlen(text)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<to %s> %s\r\n",tname,text);
	    }
	    conn_unget_chatcharname(me,tname);
	}
	break;
    case message_type_channelfull:
	/* FIXME */
	if (!(msgtemp = strdup("")))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
	break;
    case message_type_channeldoesnotexist:
	/* FIXME */
	if (!(msgtemp = strdup("")))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
	break;
    case message_type_channelrestricted:
	/* FIXME */
	if (!(msgtemp = strdup("")))
	{
	    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
	    return -1;
	}
	break;
    case message_type_info:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * newtext;
	    
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%s\r\n",newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(strlen(text)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%s\r\n",text);
	    }
	}
	break;
    case message_type_error:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	{
	    char const * newtext;
	    
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(8+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"ERROR: %s\r\n",newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(8+strlen(text)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"ERROR: %s\r\n",text);
	    }
	}
	break;
    case message_type_emote:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_telnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
	{
	    char const * tname;
	    char const * newtext;
	    
	    tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
	    if ((newtext = escape_chars(text,strlen(text))))
	    {
		if (!(msgtemp = malloc(strlen(tname)+4+strlen(newtext)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<%s %s>\r\n",tname,newtext);
		free((void *)newtext); /* avoid warning */
	    }
	    else
	    {
		if (!(msgtemp = malloc(strlen(tname)+4+strlen(text)+4)))
		{
		    eventlog(eventlog_level_error,"message_telnet_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"<%s %s>\r\n",tname,text);
	    }
	    conn_unget_chatcharname(me,tname);
	}
	break;
    default:
	eventlog(eventlog_level_error,"message_telnet_format","got bad message type %d",(int)type);
	return -1;
    }
    
    {
	int retval;
	
	retval = packet_append_ntstring(packet,msgtemp);
	free(msgtemp);
	return retval;
    }
}


static int message_bot_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags)
{
    char * msgtemp;
    
    if (!packet)
    {
	eventlog(eventlog_level_error,"message_bot_format","got NULL packet");
	return -1;
    }
    
    /* special-case the login banner so it doesn't have numbers
     * at the start of each line
     */
    if (me &&
        conn_get_state(me)!=conn_state_loggedin &&
        conn_get_state(me)!=conn_state_destroy)
    {
	if (!text)
        {
	    if (type==message_type_null)
	    	return 0; /* don't display null messages during the login */
            eventlog(eventlog_level_error,"message_bot_format","got NULL text for non-loggedin state");
            return -1;
        }
	if (!(msgtemp = malloc(strlen(text)+4)))
	{
	    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
	    return -1;
	}
        sprintf(msgtemp,"%s\r\n",text);
    }
    else
	switch (type)
	{
	case message_type_null:
	    if (!(msgtemp = malloc(32)))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s\r\n",EID_NULL,"NULL");
	    break;
	case message_type_uniqueid: /* FIXME: need to send this for some bots, also needed to support guest accounts */
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!(msgtemp = malloc(strlen(text)+32)))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s %s\r\n",EID_UNIQUENAME,"NAME",text);
	    break;
	case message_type_adduser:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+32)))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x [%s]\r\n",EID_SHOWUSER,"USER",tname,conn_get_flags(me)|dstflags,conn_get_fake_clienttag(me));
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_join:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+32)))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x [%s]\r\n",EID_JOIN,"JOIN",tname,conn_get_flags(me)|dstflags,conn_get_fake_clienttag(me));
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_part:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+32)))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x\r\n",EID_LEAVE,"LEAVE",tname,conn_get_flags(me)|dstflags);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_whisper:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (dstflags&MF_X)
		return -1; /* player is ignored */
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+32+strlen(text))))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x \"%s\"\r\n",EID_WHISPER,"WHISPER",tname,conn_get_flags(me)|dstflags,text);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_talk:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (dstflags&MF_X)
		return -1; /* player is ignored */
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
		if (!(msgtemp = malloc(32+strlen(tname)+32+strlen(text))))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x \"%s\"\r\n",EID_TALK,"TALK",tname,conn_get_flags(me)|dstflags,text);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_broadcast:
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (dstflags&MF_X)
		return -1; /* player is ignored */
	    if (!(msgtemp = malloc(32+32+strlen(text))))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s \"%s\"\r\n",EID_BROADCAST,"_",text); /* FIXME: what does this look like on Battle.net? */
	    break;
	case message_type_channel:
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!(msgtemp = malloc(32+strlen(text))))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s \"%s\"\r\n",EID_CHANNEL,"CHANNEL",text);
	    break;
	case message_type_userflags:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+16)))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x\r\n",EID_USERFLAGS,"USER",tname,conn_get_flags(me)|dstflags);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_whisperack:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, dst);
		if (!(msgtemp = malloc(32+strlen(tname)+32+strlen(text))))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x \"%s\"\r\n",EID_WHISPERSENT,"WHISPER",tname,conn_get_flags(me)|dstflags,text);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	case message_type_channelfull:
	    if (!(msgtemp = malloc(32)))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u \r\n",EID_CHANNELFULL); /* FIXME */
	    break;
	case message_type_channeldoesnotexist:
	    if (!(msgtemp = malloc(32)))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u \r\n",EID_CHANNELDOESNOTEXIST); /* FIXME */
	    break;
	case message_type_channelrestricted:
	    if (!(msgtemp = malloc(32)))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u \r\n",EID_CHANNELRESTRICTED); /* FIXME */
	    break;
	case message_type_info:
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!(msgtemp = malloc(32+16+strlen(text))))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s \"%s\"\r\n",EID_INFO,"INFO",text);
	    break;
	case message_type_error:
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!(msgtemp = malloc(32+16+strlen(text))))
	    {
		eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		return -1;
	    }
	    sprintf(msgtemp,"%u %s \"%s\"\r\n",EID_ERROR,"ERROR",text);
	    break;
	case message_type_emote:
	    if (!me)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL connection for %s",message_type_get_str(type));
		return -1;
	    }
	    if (!text)
	    {
		eventlog(eventlog_level_error,"message_bot_format","got NULL text for %s",message_type_get_str(type));
		return -1;
	    }
	    if (dstflags&MF_X)
		return -1; /* player is ignored */
	    {
		char const * tname;
		
		tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
		if (!(msgtemp = malloc(32+strlen(tname)+32+strlen(text))))
		{
		    eventlog(eventlog_level_error,"message_bot_format","could not allocate memory for msgtemp");
		    return -1;
		}
		sprintf(msgtemp,"%u %s %s %04x \"%s\"\r\n",EID_EMOTE,"EMOTE",tname,conn_get_flags(me)|dstflags,text);
		conn_unget_chatcharname(me,tname);
	    }
	    break;
	default:
	    eventlog(eventlog_level_error,"message_bot_format","got bad message type %d",(int)type);
	    return -1;
	}
    
    if (strlen(msgtemp)>MAX_MESSAGE_LEN)
	msgtemp[MAX_MESSAGE_LEN] = '\0'; /* now truncate to max size */
    
    {
	int retval;
	
	retval = packet_append_ntstring(packet,msgtemp);
	free(msgtemp);
	return retval;
    }
}


static int message_bnet_format(t_packet * packet, t_message_type type, t_connection * me, t_connection * dst, char const * text, unsigned int dstflags)
{
    if (!packet)
    {
	eventlog(eventlog_level_error,"message_bnet_format","got NULL packet");
	return -1;
    }
    
    if (text && text[0]=='\0')
        text = " "; /* empty messages crash some clients, just send whitespace */
    
    packet_set_size(packet,sizeof(t_server_message));
    packet_set_type(packet,SERVER_MESSAGE);
    bn_int_set(&packet->u.server_message.unknown1,SERVER_MESSAGE_UNKNOWN1);
    bn_int_nset(&packet->u.server_message.player_ip,SERVER_MESSAGE_PLAYER_IP_DUMMY);
    bn_int_set(&packet->u.server_message.unknown3,SERVER_MESSAGE_UNKNOWN3);
    
    switch (type)
    {
    case message_type_adduser:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_ADDUSER);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * playerinfo;
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    if (!(playerinfo = conn_get_playerinfo(me)))
		playerinfo = "";
	    packet_append_string(packet,playerinfo);
	}
	break;
    case message_type_join:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_JOIN);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * playerinfo;
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    if (!(playerinfo = conn_get_playerinfo(me)))
		playerinfo = "";
	    packet_append_string(packet,playerinfo); /* FIXME: should we just send "" here instead of playerinfo? */
	}
	break;
    case message_type_part:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_PART);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,"");
	}
	break;
    case message_type_whisper:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_WHISPER);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_talk:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_TALK);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_broadcast:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_BROADCAST);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_channel:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_CHANNEL);
	{
	    t_channel const * channel;
	    
	    if (!(channel = conn_get_channel(me)))
		bn_int_set(&packet->u.server_message.flags,0);
	    else
		bn_int_set(&packet->u.server_message.flags,cflags_to_bncflags(channel_get_flags(channel)));
	}
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatname(me);
	    packet_append_string(packet,tname);
	    conn_unget_chatname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_userflags:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_USERFLAGS);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * playerinfo;
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    if (!(playerinfo = conn_get_playerinfo(me)))
		playerinfo = "";
#if 0 /* FIXME: which is correct? does it depend on the client type? */
	    packet_append_string(packet,"");
#else
	    packet_append_string(packet,playerinfo);
#endif
	}
	break;
    case message_type_whisperack:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_WHISPERACK);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, dst);
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_channelfull: /* FIXME */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_CHANNELFULL);
	bn_int_set(&packet->u.server_message.flags,0);
	bn_int_set(&packet->u.server_message.latency,0);
	packet_append_string(packet,"");
	packet_append_string(packet,"");
	break;
    case message_type_channeldoesnotexist:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_CHANNELDOESNOTEXIST);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatname(me);
	    packet_append_string(packet,tname);
	    conn_unget_chatname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    case message_type_channelrestricted: /* FIXME */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_CHANNELRESTRICTED);
	bn_int_set(&packet->u.server_message.flags,0);
	bn_int_set(&packet->u.server_message.latency,0);
	packet_append_string(packet,"");
	packet_append_string(packet,"");
	break;
    case message_type_info:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_INFO);
	bn_int_set(&packet->u.server_message.flags,0);
	bn_int_set(&packet->u.server_message.latency,0);
	packet_append_string(packet,"");
	packet_append_string(packet,text);
	break;
    case message_type_error:
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_ERROR);
	bn_int_set(&packet->u.server_message.flags,0);
	bn_int_set(&packet->u.server_message.latency,0);
	packet_append_string(packet,"");
	packet_append_string(packet,text);
	break;
    case message_type_emote:
	if (!me)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL connection for %s",message_type_get_str(type));
	    return -1;
	}
	if (!text)
	{
	    eventlog(eventlog_level_error,"message_bnet_format","got NULL text for %s",message_type_get_str(type));
	    return -1;
	}
	if (dstflags&MF_X)
	    return -1; /* player is ignored */
        bn_int_set(&packet->u.server_message.type,SERVER_MESSAGE_TYPE_EMOTE);
	bn_int_set(&packet->u.server_message.flags,conn_get_flags(me)|dstflags);
	bn_int_set(&packet->u.server_message.latency,conn_get_latency(me));
	{
	    char const * tname;
	    
	    tname = conn_get_chatcharname(me, me); /* FIXME: second should be dst but cache gets in the way */
	    packet_append_string(packet,tname);
	    conn_unget_chatcharname(me,tname);
	    packet_append_string(packet,text);
	}
	break;
    default:
	eventlog(eventlog_level_error,"message_bnet_format","got bad message type %d",(int)type);
	return -1;
    }
    
    return 0;
}
    

extern t_message * message_create(t_message_type type, t_connection * src, t_connection * dst, char const * text)
{
    t_message * message;
    
    if (!(message = malloc(sizeof(t_message))))
    {
	eventlog(eventlog_level_error,"message_create","got NULL packet");
	return NULL;
    }
    message->num_cached = 0;
    message->packets    = NULL;
    message->classes    = NULL;
    message->dstflags   = NULL;
    message->type       = type;
    message->src        = src;
    message->dst        = dst;
    message->text       = text;
    
    return message;
}


extern int message_destroy(t_message * message)
{
    unsigned int i;
    
    if (!message)
    {
	eventlog(eventlog_level_error,"message_destroy","got NULL message");
	return -1;
    }
    
    for (i=0; i<message->num_cached; i++)
	if (message->packets[i])
	    packet_del_ref(message->packets[i]);
    if (message->packets)
	free(message->packets);
    if (message->classes)
	free(message->classes);
    if (message->dstflags)
	free(message->dstflags);
    free(message);
    
    return 0;
}


static t_packet * message_cache_lookup(t_message * message, t_conn_class class, unsigned int dstflags)
{
    unsigned int i;
    t_packet * packet;
    
    if (!message)
    {
	eventlog(eventlog_level_error,"message_cache_lookup","got NULL message");
	return NULL;
    }
    
    for (i=0; i<message->num_cached; i++)
        if (message->classes[i]==class && message->dstflags[i]==dstflags)
	    return message->packets[i];
    
    {
	t_packet * *   temp_packets;
	t_conn_class * temp_classes;
	unsigned int * temp_dstflags;
	
	if (!message->packets)
	    temp_packets = malloc(sizeof(t_packet *)*(message->num_cached+1));
	else
	    temp_packets = realloc(message->packets,sizeof(t_packet *)*(message->num_cached+1));
	if (!temp_packets)
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","unable to allocate memory for packets");
	    return NULL;
	}
	
	if (!message->classes)
	    temp_classes = malloc(sizeof(t_conn_class)*(message->num_cached+1));
	else
	    temp_classes = realloc(message->classes,sizeof(t_conn_class)*(message->num_cached+1));
	if (!temp_classes)
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","unable to allocate memory for classes");
	    return NULL;
	}
	
	if (!message->dstflags)
	    temp_dstflags = malloc(sizeof(unsigned int)*(message->num_cached+1));
	else
	    temp_dstflags = realloc(message->dstflags,sizeof(unsigned int)*(message->num_cached+1));
	if (!temp_dstflags)
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","unable to allocate memory for dstflags");
	    return NULL;
	}
	
	message->packets = temp_packets;
	message->classes = temp_classes;
	message->dstflags = temp_dstflags;
    }
    
    switch (class)
    {
    case conn_class_telnet:
	if (!(packet = packet_create(packet_class_raw)))
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","could not create packet");
	    return NULL;
	}
	if (message_telnet_format(packet,message->type,message->src,message->dst,message->text,dstflags)<0)
	{
	    packet_del_ref(packet);
	    packet = NULL; /* we can cache the NULL too */
	}
	break;
    case conn_class_bot:
	if (!(packet = packet_create(packet_class_raw)))
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","could not create packet");
	    return NULL;
	}
	if (message_bot_format(packet,message->type,message->src,message->dst,message->text,dstflags)<0)
	{
	    packet_del_ref(packet);
	    packet = NULL; /* we can cache the NULL too */
	}
	break;
    case conn_class_bnet:
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","could not create packet");
	    return NULL;
	}
	if (message_bnet_format(packet,message->type,message->src,message->dst,message->text,dstflags)<0)
	{
	    packet_del_ref(packet);
	    packet = NULL; /* we can cache the NULL too */
	}
	break;
     case conn_class_irc:
	if (!(packet = packet_create(packet_class_raw)))
	{
	    eventlog(eventlog_level_error,"message_cache_lookup","could not create packet");
	    return NULL;
	}
	/* irc_message_format() is in irc.c */
	if (irc_message_format(packet,message->type,message->src,message->dst,message->text,dstflags)<0)
	{
	    packet_del_ref(packet);
	    packet = NULL; /* we can cache the NULL too */
	}
	break;
   default:
	eventlog(eventlog_level_error,"message_cache_lookup","unsupported connection class %d",(int)class);
	packet = NULL; /* we can cache the NULL too */
    }
    
    message->num_cached++;
    message->packets[i] = packet;
    message->classes[i] = class;
    message->dstflags[i] = dstflags;
    
    return packet;
}


extern int message_send(t_message * message, t_connection * dst)
{
    t_packet *   packet;
    unsigned int dstflags;
    
    if (!message)
    {
	eventlog(eventlog_level_error,"message_send","got NULL message");
	return -1;
    }
    if (!dst)
    {
	eventlog(eventlog_level_error,"message_send","got NULL dst connection");
	return -1;
    }
    
    dstflags = 0;
    if (message->src)
    {
	char const * tname;
	
	if ((tname = conn_get_chatname(message->src)) && conn_check_ignoring(dst,tname)==1)
	{
	    conn_unget_chatname(message->src,tname);
	    dstflags |= MF_X;
	}
	if (tname)
	    conn_unget_chatname(message->src,tname);
    }
    
    if (!(packet = message_cache_lookup(message,conn_get_class(dst),dstflags)))
	return -1;

    /* FIXME: this is not needed now, message has dst */
    if (conn_get_class(dst)==conn_class_irc) {
    	/* HACK: IRC message always need the recipient and are therefore bad to cache. */
	/*       So we only cache a pseudo packet and convert it to a real packet later ... */
	packet = packet_duplicate(packet); /* we want to modify packet so we have to create a copy ... */
    	if (irc_message_postformat(packet,dst)<0) {
	    packet_del_ref(packet); /* we don't need the previously created copy anymore ... */
    	    return -1;
	}
    }
    
    queue_push_packet(conn_get_out_queue(dst),packet);

    if (conn_get_class(dst)==conn_class_irc)
    	packet_del_ref(packet); /* we don't need the previously created copy anymore ... */

    return 0;
}


extern int message_send_all(t_message * message)
{
    t_connection * c;
    t_elem const * curr;
    int            rez;
    
    if (!message)
    {
	eventlog(eventlog_level_error,"message_send_all","got NULL message");
	return -1;
    }
    
    rez = -1;
    LIST_TRAVERSE_CONST(connlist(),curr)
    {
	c = elem_get_data(curr);
	if (conn_get_class(c)==conn_class_bits) continue;
	if (message_send(message,c)==0)
	    rez = 0;
    }
    
    return rez;
}


extern int message_send_text(t_connection * dst, t_message_type type, t_connection * src, char const * text)
{
    t_message * message;
    int         rez;
    
    if (!dst)
    {
	eventlog(eventlog_level_error,"message_send_text","got NULL connection");
	return -1;
    }
    
    if (!(message = message_create(type,src,dst,text)))
	return -1;
    rez = message_send(message,dst);
    message_destroy(message);
    
    return rez;
}


extern int message_send_file(t_connection * dst, FILE * fd)
{
    char * buff;
    
    if (!dst)
    {
	eventlog(eventlog_level_error,"message_send_file","got NULL connection");
	return -1;
    }
    if (!fd)
    {
	eventlog(eventlog_level_error,"message_send_file","got NULL fd");
	return -1;
    }
    
    while ((buff = file_get_line(fd)))
	if ((buff = message_format_line(dst,buff)))
	{
	    /* empty messages can crash the clients */
	    switch (buff[0])
	    {
	    case 'C':
		handle_command(dst,&buff[1]);
		break;
	    case 'B':
		message_send_text(dst,message_type_broadcast,dst,&buff[1]);
		break;
	    case 'E':
		message_send_text(dst,message_type_error,dst,&buff[1]);
		break;
	    case 'M':
		message_send_text(dst,message_type_talk,dst,&buff[1]);
		break;
	    case 'T':
		message_send_text(dst,message_type_emote,dst,&buff[1]);
		break;
	    case 'I':
	    case 'W':
		message_send_text(dst,message_type_info,dst,&buff[1]);
		break;
	    default:
		eventlog(eventlog_level_error,"message_send_file","unknown message type '%c'",buff[0]);
	    }
	    free(buff);
	}
    
    return 0;
}

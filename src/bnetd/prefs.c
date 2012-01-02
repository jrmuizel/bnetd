/*
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 1999  Mark Baysinger (mbaysing@ucsd.edu)
 * Some BITS modifications:
 *          Copyright (C) 1999,2000  Marco Ziech (mmz@gmx.net)
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
#define PREFS_INTERNAL_ACCESS
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
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
#include <ctype.h>
#include "common/util.h"
#include "common/eventlog.h"
#include "prefs.h"
#include "common/setup_after.h"


static int processDirective(char const * directive, char const * value, unsigned int curLine);
static char const * get_char_conf(char const * directive);
static unsigned int get_int_conf(char const * directive);
static int get_bool_conf(char const * directive);


#define NONE 0
#define ACT NULL, 0

/*    directive                 type               defcharval            defintval                 */
static Bconf_t conf_table[] =
{
    { "filedir",                conf_type_char,    BNETD_FILE_DIR,       NONE,                  ACT },
    { "userdir",                conf_type_char,    BNETD_USER_DIR,       NONE,                  ACT },
    { "logfile",                conf_type_char,    BNETD_LOG_FILE,       NONE,                  ACT },
    { "loglevels",              conf_type_char,    BNETD_LOG_LEVELS,     NONE,                  ACT },
    { "defacct",                conf_type_char,    BNETD_TEMPLATE_FILE,  NONE,                  ACT },
    { "motdfile",               conf_type_char,    BNETD_MOTD_FILE,      NONE,                  ACT },
    { "newsfile",               conf_type_char,    BNETD_NEWS_DIR,       NONE,                  ACT },
    { "channelfile",            conf_type_char,    BNETD_CHANNEL_FILE,   NONE,                  ACT },
    { "pidfile",                conf_type_char,    BNETD_PID_FILE,       NONE,                  ACT },
    { "adfile",                 conf_type_char,    BNETD_AD_FILE,        NONE,                  ACT },
    { "usersync",               conf_type_int,     NULL,                 BNETD_USERSYNC,        ACT },
    { "userflush",              conf_type_int,     NULL,                 BNETD_USERFLUSH,       ACT },
    { "servername",             conf_type_char,    "",                   NONE,                  ACT },
    { "track",                  conf_type_int,     NULL,                 BNETD_TRACK_TIME,      ACT },
    { "location",               conf_type_char,    "",                   NONE,                  ACT },
    { "description",            conf_type_char,    "",                   NONE,                  ACT },
    { "url",                    conf_type_char,    "",                   NONE,                  ACT },
    { "contact_name",           conf_type_char,    "",                   NONE,                  ACT },
    { "contact_email",          conf_type_char,    "",                   NONE,                  ACT },
    { "latency",                conf_type_int,     NULL,                 BNETD_LATENCY,         ACT },
    { "irc_latency",            conf_type_int,     NULL,                 BNETD_IRC_LATENCY,     ACT },
    { "shutdown_delay",         conf_type_int,     NULL,                 BNETD_SHUTDELAY,       ACT },
    { "shutdown_decr",          conf_type_int,     NULL,                 BNETD_SHUTDECR,        ACT },
    { "new_accounts",           conf_type_bool,    NULL,                 1,                     ACT },
    { "kick_old_login",         conf_type_bool,    NULL,                 1,                     ACT },
    { "ask_new_channel",        conf_type_bool,    NULL,                 1,                     ACT },
    { "hide_pass_games",        conf_type_bool,    NULL,                 1,                     ACT },
    { "hide_started_games",     conf_type_bool,    NULL,                 0,                     ACT },
    { "hide_temp_channels",     conf_type_bool,    NULL,                 1,                     ACT },
    { "hide_addr",              conf_type_bool,    NULL,                 1,                     ACT },
    { "enable_conn_all",        conf_type_bool,    NULL,                 0,                     ACT },
    { "extra_commands",         conf_type_bool,    NULL,                 0,                     ACT },
    { "reportdir",              conf_type_char,    BNETD_REPORT_DIR,     NONE,                  ACT },
    { "report_all_games",       conf_type_bool,    NULL,                 0,                     ACT },
    { "report_diablo_games",    conf_type_bool,    NULL,                 0,                     ACT },
    { "iconfile",               conf_type_char,    BNETD_ICON_FILE,      NONE,                  ACT },
    { "tosfile",                conf_type_char,    BNETD_TOS_FILE,       NONE,                  ACT },
    { "mpqfile",                conf_type_char,    BNETD_MPQ_FILE,       NONE,                  ACT },
    { "trackaddrs",             conf_type_char,    BNETD_TRACK_ADDRS,    NONE,                  ACT },
    { "servaddrs",              conf_type_char,    BNETD_SERV_ADDRS,     NONE,                  ACT },
    { "ircaddrs",               conf_type_char,    BNETD_IRC_ADDRS,      NONE,                  ACT },
    { "use_keepalive",          conf_type_bool,    NULL,                 0,                     ACT },
    { "udptest_port",           conf_type_int,     NULL,                 BNETD_DEF_TEST_PORT,   ACT },
    { "do_uplink",              conf_type_bool,    NULL,                 BITS_DO_UPLINK,        ACT },
    { "uplink_server",          conf_type_char,    BITS_UPLINK_SERVER,   NONE,                  ACT },
    { "uplink_username",        conf_type_char,    BITS_UPLINK_USERNAME, NONE,                  ACT },
    { "allow_uplink",           conf_type_bool,    NULL,                 BITS_ALLOW_UPLINK,     ACT },
    { "bits_password_file",     conf_type_char,    BITS_PASSWORD_FILE,   NONE,                  ACT },
    { "ipbanfile",              conf_type_char,    BNETD_IPBAN_FILE,     NONE,                  ACT },
    { "bits_debug",             conf_type_bool,    NULL,                 BITS_DEBUG,            ACT },
    { "disc_is_loss",           conf_type_bool,    NULL,                 0,                     ACT },
    { "helpfile",               conf_type_char,    BNETD_HELP_FILE,      NONE,                  ACT },
    { "fortunecmd",             conf_type_char,    BNETD_FORTUNECMD,     NONE,                  ACT },
    { "transfile",              conf_type_char,    BNETD_TRANS_FILE,     NONE,                  ACT },
    { "chanlog",                conf_type_bool,    NULL         ,        BNETD_CHANLOG,         ACT },
    { "chanlogdir",             conf_type_char,    BNETD_CHANLOG_DIR,    NONE,                  ACT },
    { "quota",                  conf_type_bool,    NULL,                 0,                     ACT },
    { "quota_lines",            conf_type_int,     NULL,                 BNETD_QUOTA_LINES,     ACT },
    { "quota_time",             conf_type_int,     NULL,                 BNETD_QUOTA_TIME,      ACT },
    { "quota_wrapline",	        conf_type_int,     NULL,                 BNETD_QUOTA_WLINE,     ACT },
    { "quota_maxline",	        conf_type_int,     NULL,                 BNETD_QUOTA_MLINE,     ACT },
    { "ladder_init_rating",     conf_type_int,     NULL,                 BNETD_LADDER_INIT_RAT, ACT },
    { "quota_dobae",            conf_type_int,     NULL,                 BNETD_QUOTA_DOBAE,     ACT },
    { "realmfile",              conf_type_char,    BNETD_REALM_FILE,     NONE,                  ACT },
    { "issuefile",              conf_type_char,    BNETD_ISSUE_FILE,     NONE,                  ACT },
    { "bits_motd_file",         conf_type_char,    BITS_MOTD_FILE,       NONE,                  ACT },
    { "bits_use_plaintext",     conf_type_bool,    NULL,                 BITS_USE_PLAINTEXT,    ACT },
    { "effective_user",         conf_type_char,    NULL,                 NONE,                  ACT },
    { "effective_group",        conf_type_char,    NULL,                 NONE,                  ACT },
    { "nullmsg",                conf_type_int,     NULL,                 BNETD_DEF_NULLMSG,     ACT },
    { "mail_support",           conf_type_bool,    NULL,                 BNETD_MAIL_SUPPORT,    ACT },
    { "mail_quota",             conf_type_int,     NULL,                 BNETD_MAIL_QUOTA,      ACT },
    { "maildir",                conf_type_char,    BNETD_MAIL_DIR,       NONE,                  ACT },
    { "log_notice",             conf_type_char,    BNETD_LOG_NOTICE,     NONE,                  ACT },
    { "savebyname",             conf_type_bool,    NULL,                 1,                     ACT },
    { "skip_versioncheck",      conf_type_bool,    NULL,                 0,                     ACT },
    { "allow_bad_version",      conf_type_bool,    NULL,                 0,                     ACT },
    { "allow_unknown_version",  conf_type_bool,    NULL,                 0,                     ACT },
    { "versioncheck_file",      conf_type_char,    BNETD_VERSIONCHECK,   NONE,                  ACT },
    { "d2cs_version",           conf_type_int,     NULL,                 0,                     ACT },
    { "allow_d2cs_setname",     conf_type_bool,    NULL,                 1,                     ACT },
    { "hashtable_size",         conf_type_int,     NULL,                 BNETD_HASHTABLE_SIZE,  ACT },
    { "telnetaddrs",            conf_type_char,    BNETD_TELNET_ADDRS,   NONE,                  ACT },
    { NULL,                     conf_type_none,    NULL,                 NONE,                  ACT }
};


char const * preffile=NULL;


static int processDirective(char const * directive, char const * value, unsigned int curLine)
{
    unsigned int i;
    
    if (!directive)
    {
	eventlog(eventlog_level_error,"processDirective","got NULL directive");
	return -1;
    }
    if (!value)
    {
	eventlog(eventlog_level_error,"processDirective","got NULL value");
	return -1;
    }
    
    for (i=0; conf_table[i].directive; i++)
        if (strcasecmp(conf_table[i].directive,directive)==0)
	{
            switch (conf_table[i].type)
            {
	    case conf_type_char:
		{
		    char const * temp;
		    
		    if (!(temp = strdup(value)))
		    {
			eventlog(eventlog_level_error,"processDirective","could not allocate memory for value");
			return -1;
		    }
		    if (conf_table[i].charval)
			free((void *)conf_table[i].charval); /* avoid warning */
		    conf_table[i].charval = temp;
		}
		break;
		
	    case conf_type_int:
		{
		    unsigned int temp;
		    
		    if (str_to_uint(value,&temp)<0)
			eventlog(eventlog_level_error,"processDirective","invalid integer value \"%s\" for element \"%s\" at line %u",value,directive,curLine);
		    else
                	conf_table[i].intval = temp;
		}
		break;
		
	    case conf_type_bool:
		switch (str_get_bool(value))
		{
		case 1:
		    conf_table[i].intval = 1;
		    break;
		case 0:
		    conf_table[i].intval = 0;
		    break;
		default:
		    eventlog(eventlog_level_error,"processDirective","invalid boolean value for element \"%s\" at line %u",directive,curLine);
		}
		break;
		
	    default:
		eventlog(eventlog_level_error,"processDirective","invalid type %d in table",(int)conf_table[i].type);
	    }
	    return 0;
	}
    
    eventlog(eventlog_level_error,"processDirective","unknown element \"%s\" at line %u",directive,curLine);
    return -1;
}


static char const * get_char_conf(char const * directive)
{
    unsigned int i;
    
    for (i=0; conf_table[i].directive; i++)
	if (conf_table[i].type==conf_type_char && strcasecmp(conf_table[i].directive,directive)==0)
	    return conf_table[i].charval;
    
    return NULL;
}


static unsigned int get_int_conf(char const * directive)
{
    unsigned int i;
    
    for (i=0; conf_table[i].directive; i++)
	if (conf_table[i].type==conf_type_int && strcasecmp(conf_table[i].directive,directive)==0)
	    return conf_table[i].intval;
    
    return 0;
}


static int get_bool_conf(char const * directive)
{
    unsigned int i;
    
    for (i=0; conf_table[i].directive; i++)
	if (conf_table[i].type==conf_type_bool && strcasecmp(conf_table[i].directive,directive)==0)
	    return conf_table[i].intval;
    
    return 0;
}


extern int prefs_load(char const * filename)
{
    /* restore defaults */
    {
	unsigned int i;
	
	for (i=0; conf_table[i].directive; i++)
	    switch (conf_table[i].type)
	    {
	    case conf_type_int:
	    case conf_type_bool:
		conf_table[i].intval = conf_table[i].defintval;
		break;
		
	    case conf_type_char:
		if (conf_table[i].charval)
		    free((void *)conf_table[i].charval); /* avoid warning */
		if (!conf_table[i].defcharval)
		    conf_table[i].charval = NULL;
		else
		    if (!(conf_table[i].charval = strdup(conf_table[i].defcharval)))
		    {
			eventlog(eventlog_level_error,"prefs_load","could not allocate memory for conf_table[i].charval");
			return -1;
		    }
		break;
		
	    default:
		eventlog(eventlog_level_error,"prefs_load","invalid type %d in table",(int)conf_table[i].type);
		return -1;
	    }
    }
    
    /* load file */
    if (filename)
    {
	FILE *       fp;
	char *       buff;
	char *       cp;
	char *       temp;
	unsigned int currline;
	unsigned int j;
	char const * directive;
	char const * value;
	char *       rawvalue;
	
        if (!(fp = fopen(filename,"r")))
        {
            eventlog(eventlog_level_error,"prefs_load","could not open file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
            return -1;
        }
	
	/* Read the configuration file */
	for (currline=1; (buff = file_get_line(fp)); currline++)
	{
	    cp = buff;
	    
            while (*cp=='\t' || *cp==' ') cp++;
	    if (*cp=='\0' || *cp=='#')
	    {
		free(buff);
		continue;
	    }
	    temp = cp;
	    while (*cp!='\t' && *cp!=' ' && *cp!='\0') cp++;
	    if (*cp!='\0')
	    {
		*cp = '\0';
		cp++;
	    }
	    if (!(directive = strdup(temp)))
	    {
		eventlog(eventlog_level_error,"prefs_load","could not allocate memory for directive");
		free(buff);
		continue;
	    }
            while (*cp=='\t' || *cp==' ') cp++;
	    if (*cp!='=')
	    {
		eventlog(eventlog_level_error,"prefs_load","missing = on line %u",currline);
		free((void *)directive); /* avoid warning */
		free(buff);
		continue;
	    }
	    cp++;
	    while (*cp=='\t' || *cp==' ') cp++;
	    if (*cp=='\0')
	    {
		eventlog(eventlog_level_error,"prefs_load","missing value after = on line %u",currline);
		free((void *)directive); /* avoid warning */
		free(buff);
		continue;
	    }
	    if (!(rawvalue = strdup(cp)))
	    {
		eventlog(eventlog_level_error,"prefs_load","could not allocate memory for rawvalue");
		free((void *)directive); /* avoid warning */
		free(buff);
		continue;
	    }
	    
	    if (rawvalue[0]=='"')
	    {
		char prev;
		
		for (j=1,prev='\0'; rawvalue[j]!='\0'; j++)
		{
		    switch (rawvalue[j])
		    {
		    case '"':
			if (prev!='\\')
			    break;
			prev = '"';
			continue;
		    case '\\':
			if (prev=='\\')
			    prev = '\0';
			else
			    prev = '\\';
			continue;
		    default:
			prev = rawvalue[j];
			continue;
		    }
		    break;
		}
		if (rawvalue[j]!='"')
		{
		    eventlog(eventlog_level_error,"prefs_load","missing end quote for value of element \"%s\" on line %u",directive,currline);
		    free(rawvalue);
		    free((void *)directive); /* avoid warning */
		    free(buff);
		    continue;
		}
		rawvalue[j] = '\0';
		if (rawvalue[j+1]!='\0' && rawvalue[j+1]!='#')
		{
		    eventlog(eventlog_level_error,"prefs_load","extra characters after the value for element \"%s\" on line %u",directive,currline);
		    free(rawvalue);
		    free((void *)directive); /* avoid warning */
		    free(buff);
		    continue;
		}
		value = &rawvalue[1];
            }
	    else
	    {
		unsigned int k;
		
		for (j=0; rawvalue[j]!='\0' && rawvalue[j]!=' ' && rawvalue[j]!='\t'; j++);
		k = j;
		while (rawvalue[k]==' ' || rawvalue[k]=='\t') k++;
		if (rawvalue[k]!='\0' && rawvalue[k]!='#')
		{
		    eventlog(eventlog_level_error,"prefs_load","extra characters after the value for element \"%s\" on line %u (%s)",directive,currline,&rawvalue[k]);
		    free(rawvalue);
		    free((void *)directive); /* avoid warning */
		    free(buff);
		    continue;
		}
		rawvalue[j] = '\0';
		value = rawvalue;
	    }
            
	    processDirective(directive,value,currline);
	    
	    free(rawvalue);
	    free((void *)directive); /* avoid warning */
	    free(buff);
	}
	if (fclose(fp)<0)
	    eventlog(eventlog_level_error,"prefs_load","could not close prefs file \"%s\" (fclose: %s)",filename,strerror(errno));
    }
    
    return 0;
}


extern void prefs_unload(void)
{
    unsigned int i;
    
    for (i=0; conf_table[i].directive; i++)
	switch (conf_table[i].type)
	{
	case conf_type_int:
	case conf_type_bool:
	    break;
	    
	case conf_type_char:
	    if (conf_table[i].charval)
	    {
		free((void *)conf_table[i].charval); /* avoid warning */
		conf_table[i].charval = NULL;
	    }
	    break;
	    
	default:
	    eventlog(eventlog_level_error,"prefs_unload","invalid type %d in table",(int)conf_table[i].type);
	    break;
	}
}


extern char const * prefs_get_userdir(void)
{
    return get_char_conf("userdir");
}


extern char const * prefs_get_filedir(void)
{
    return get_char_conf("filedir");
}


extern char const * prefs_get_logfile(void)
{
    return get_char_conf("logfile");
}


extern char const * prefs_get_loglevels(void)
{
    return get_char_conf("loglevels");
}


extern char const * prefs_get_defacct(void)
{
    return get_char_conf("defacct");
}


extern char const * prefs_get_motdfile(void)
{
    return get_char_conf("motdfile");
}


extern char const * prefs_get_newsfile(void)
{
    return get_char_conf("newsfile");
}


extern char const * prefs_get_adfile(void)
{
    return get_char_conf("adfile");
}


extern unsigned int prefs_get_user_sync_timer(void)
{
    return get_int_conf("usersync");
}


extern unsigned int prefs_get_user_flush_timer(void)
{
    return get_int_conf("userflush");
}

extern char const * prefs_get_servername(void)
{
    return get_char_conf("servername");
}

extern unsigned int prefs_get_track(void)
{
    unsigned int rez;
    
    rez = get_int_conf("track");
    if (rez>0 && rez<60) rez = 60;
    return rez;
}


extern char const * prefs_get_location(void)
{
    return get_char_conf("location");
}


extern char const * prefs_get_description(void)
{
    return get_char_conf("description");
}


extern char const * prefs_get_url(void)
{
    return get_char_conf("url");
}


extern char const * prefs_get_contact_name(void)
{
    return get_char_conf("contact_name");
}


extern char const * prefs_get_contact_email(void)
{
    return get_char_conf("contact_email");
}


extern unsigned int prefs_get_latency(void)
{
    return get_int_conf("latency");
}


extern unsigned int prefs_get_irc_latency(void)
{
    return get_int_conf("irc_latency");
}


extern unsigned int prefs_get_shutdown_delay(void)
{
    return get_int_conf("shutdown_delay");
}


extern unsigned int prefs_get_shutdown_decr(void)
{
    return get_int_conf("shutdown_decr");
}


extern unsigned int prefs_get_allow_new_accounts(void)
{
    return get_bool_conf("new_accounts");
}


extern unsigned int prefs_get_kick_old_login(void)
{
    return get_bool_conf("kick_old_login");
}


extern char const * prefs_get_channelfile(void)
{
    return get_char_conf("channelfile");
}


extern unsigned int prefs_get_ask_new_channel(void)
{
    return get_bool_conf("ask_new_channel");
}


extern unsigned int prefs_get_hide_pass_games(void)
{
    return get_bool_conf("hide_pass_games");
}


extern unsigned int prefs_get_hide_started_games(void)
{
    return get_bool_conf("hide_started_games");
}


extern unsigned int prefs_get_hide_temp_channels(void)
{
    return get_bool_conf("hide_temp_channels");
}


extern unsigned int prefs_get_hide_addr(void)
{
    return get_bool_conf("hide_addr");
}


extern unsigned int prefs_get_enable_conn_all(void)
{
    return get_bool_conf("enable_conn_all");
}


extern unsigned int prefs_get_extra_commands(void)
{
    return get_bool_conf("extra_commands");
}


extern char const * prefs_get_reportdir(void)
{
    return get_char_conf("reportdir");
}


extern unsigned int prefs_get_report_all_games(void)
{
    return get_bool_conf("report_all_games");
}

extern unsigned int prefs_get_report_diablo_games(void)
{
    return get_bool_conf("report_diablo_games");
}

extern char const * prefs_get_pidfile(void)
{
    return get_char_conf("pidfile");
}


extern char const * prefs_get_iconfile(void)
{
    return get_char_conf("iconfile");
}


extern char const * prefs_get_tosfile(void)
{
    return get_char_conf("tosfile");
}


extern char const * prefs_get_mpqfile(void)
{
    return get_char_conf("mpqfile");
}


extern char const * prefs_get_trackserv_addrs(void)
{
    return get_char_conf("trackaddrs");
}


extern char const * prefs_get_bnetdserv_addrs(void)
{
    return get_char_conf("servaddrs");
}


extern char const * prefs_get_irc_addrs(void)
{
    return get_char_conf("ircaddrs");
}


extern unsigned int prefs_get_use_keepalive(void)
{
    return get_bool_conf("use_keepalive");
}


extern unsigned int prefs_get_udptest_port(void)
{
    return get_int_conf("udptest_port");
}


extern unsigned int prefs_get_do_uplink(void)
{
    return get_bool_conf("do_uplink");
}


extern char const * prefs_get_uplink_server(void)
{
    return get_char_conf("uplink_server");
}


extern unsigned int prefs_get_allow_uplink(void)
{
    return get_bool_conf("allow_uplink");
}


extern char const * prefs_get_bits_password_file(void)
{
    return get_char_conf("bits_password_file");
}


extern char const * prefs_get_uplink_username(void)
{
    return get_char_conf("uplink_username");
}


extern char const * prefs_get_ipbanfile(void)
{
    return get_char_conf("ipbanfile");
}


extern unsigned int prefs_get_bits_debug(void)
{
    return get_bool_conf("bits_debug");
}


extern unsigned int prefs_get_discisloss(void)
{
    return get_bool_conf("disc_is_loss");
}


extern char const * prefs_get_helpfile(void)
{
    return get_char_conf("helpfile");
}


extern char const * prefs_get_fortunecmd(void)
{
    return get_char_conf("fortunecmd");
}


extern char const * prefs_get_transfile(void)
{
    return get_char_conf("transfile");
}


extern unsigned int prefs_get_chanlog(void)
{
    return get_bool_conf("chanlog");
}


extern char const * prefs_get_chanlogdir(void)
{
    return get_char_conf("chanlogdir");
}


extern unsigned int prefs_get_quota(void)
{
    return get_bool_conf("quota");
}


extern unsigned int prefs_get_quota_lines(void)
{
    unsigned int rez;
    
    rez=get_int_conf("quota_lines");
    if (rez<1) rez = 1;
    if (rez>100) rez = 100;
    return rez;
}


extern unsigned int prefs_get_quota_time(void)
{
    unsigned int rez;
    
    rez=get_int_conf("quota_time");
    if (rez<1) rez = 1;
    if (rez>10) rez = 60;
    return rez;
}


extern unsigned int prefs_get_quota_wrapline(void)
{
    unsigned int rez;
    
    rez=get_int_conf("quota_wrapline");
    if (rez<1) rez = 1;
    if (rez>256) rez = 256;
    return rez;
}


extern unsigned int prefs_get_quota_maxline(void)
{
    unsigned int rez;
    
    rez=get_int_conf("quota_maxline");
    if (rez<1) rez = 1;
    if (rez>256) rez = 256;
    return rez;
}


extern unsigned int prefs_get_ladder_init_rating(void)
{
    return get_int_conf("ladder_init_rating");
}


extern unsigned int prefs_get_quota_dobae(void)
{
    unsigned int rez;

    rez=get_int_conf("quota_dobae");
    if (rez<1) rez = 1;
    if (rez>100) rez = 100;
    return rez;
}


extern char const * prefs_get_realmfile(void)
{
    return get_char_conf("realmfile");
}


extern char const * prefs_get_issuefile(void)
{
    return get_char_conf("issuefile");
}


extern char const * prefs_get_bits_motd_file(void)
{
    return get_char_conf("bits_motd_file");
}


extern unsigned int prefs_get_bits_use_plaintext(void)
{
    return get_bool_conf("bits_use_plaintext");
}

extern char const * prefs_get_effective_user(void)
{
    return get_char_conf("effective_user");
}


extern char const * prefs_get_effective_group(void)
{
    return get_char_conf("effective_group");
}


extern unsigned int prefs_get_nullmsg(void)
{
    return get_int_conf("nullmsg");
}


extern unsigned int prefs_get_mail_support(void)
{
    return get_bool_conf("mail_support");
}


extern unsigned int prefs_get_mail_quota(void)
{
    unsigned int rez;
    
    rez=get_int_conf("mail_quota");
    if (rez<1) rez = 1;
    if (rez>30) rez = 30;
    return rez;
}


extern char const * prefs_get_maildir(void)
{
    return get_char_conf("maildir");
}


extern char const * prefs_get_log_notice(void)
{
    return get_char_conf("log_notice");
}


extern unsigned int prefs_get_savebyname(void)
{
    return get_bool_conf("savebyname");
}


extern unsigned int prefs_get_skip_versioncheck(void)
{
    return get_bool_conf("skip_versioncheck");
}


extern unsigned int prefs_get_allow_bad_version(void)
{
    return get_bool_conf("allow_bad_version");
}


extern unsigned int prefs_get_allow_unknown_version(void)
{
    return get_bool_conf("allow_unknown_version");
}


extern char const * prefs_get_versioncheck_file(void)
{
    return get_char_conf("versioncheck_file");
}


extern unsigned int prefs_allow_d2cs_setname(void)
{
        return get_bool_conf("allow_d2cs_setname");
}


extern unsigned int prefs_get_d2cs_version(void)
{
        return get_int_conf("d2cs_version");
}


extern unsigned int prefs_get_hashtable_size(void)
{
    return get_int_conf("hashtable_size");
}


extern char const * prefs_get_telnet_addrs(void)
{
    return get_char_conf("telnetaddrs");
}


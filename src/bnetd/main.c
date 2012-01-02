/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Some BITS modifications:
 * 		Copyright (C) 2000  Marco Ziech (mmz@gmx.net)
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
#include "compat/exitstatus.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strdup.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/stdfileno.h"
#include "common/hexdump.h"
#include "channel.h"
#include "game.h"
#include "server.h"
#include "common/eventlog.h"
#include "account.h"
#include "connection.h"
#include "game.h"
#include "common/version.h"
#include "prefs.h"
#include "ladder.h"
#include "adbanner.h"
#ifdef WITH_BITS
# include "bits.h"
# include "bits_query.h"
#endif
#include "ipban.h"
#include "gametrans.h"
#include "autoupdate.h"
#include "helpfile.h"
#include "timer.h"
#include "watch.h"
#include "common/check_alloc.h"
#include "common/tracker.h"
#include "realm.h"
#include "character.h"
#include "common/give_up_root_privileges.h"
#include "versioncheck.h"
#include "common/setup_after.h"


FILE * hexstrm=NULL;

static void usage(char const * progname);


static void usage(char const * progname)
{
    fprintf(stderr,
	    "usage: %s [<options>]\n"
	    "    -c FILE, --config=FILE   use FILE as configuration file (default is " BNETD_DEFAULT_CONF_FILE ")\n"
	    "    -d FILE, --hexdump=FILE  do hex dump of packets into FILE\n"
#ifdef DO_DAEMONIZE
	    "    -f, --foreground         don't daemonize\n"
#else
	    "    -f, --foreground         don't daemonize (default)\n"
#endif
	    "    -h, --help, --usage      show this information and exit\n"
	    "    -v, --version            print version number and exit\n",
	    progname);
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    int          a;
    char const * pidfile;
    char const * hexfile=NULL;
    int          foreground=0;
    
    if (argc<1 || !argv || !argv[0])
    {
        fprintf(stderr,"bad arguments\n");
        return STATUS_FAILURE;
    }
    
    preffile = NULL;
    for (a=1; a<argc; a++)
        if (strncmp(argv[a],"--config=",9)==0)
        {
            if (preffile)
            {
                fprintf(stderr,"%s: configuration file was already specified as \"%s\"\n",argv[0],preffile);
                usage(argv[0]);
            }
	    preffile = &argv[a][9];
	}
        else if (strcmp(argv[a],"-c")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (preffile)
            {
                fprintf(stderr,"%s: configuration file was already specified as \"%s\"\n",argv[0],preffile);
                usage(argv[0]);
            }
            a++;
	    preffile = argv[a];
        }
        else if (strncmp(argv[a],"--hexdump=",10)==0)
        {
            if (hexfile)
            {
                fprintf(stderr,"%s: configuration file was already specified as \"%s\"\n",argv[0],hexfile);
                usage(argv[0]);
            }
            hexfile = &argv[a][10];
        }
        else if (strcmp(argv[a],"-d")==0)
        {
            if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
            if (hexfile)
            {
                fprintf(stderr,"%s: configuration file was already specified as \"%s\"\n",argv[0],hexfile);
                usage(argv[0]);
            }
            a++;
            hexfile = argv[a];
        }
        else if (strcmp(argv[a],"-f")==0 || strcmp(argv[a],"--foreground")==0)
            foreground = 1;
        else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
	{
            printf("version "BNETD_VERSION"\n");
            return STATUS_SUCCESS;
	}
        else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")==0)
            usage(argv[0]);
	else if (strcmp(argv[a],"--config")==0 || strcmp(argv[a],"--hexdump")==0)
	{
            fprintf(stderr,"%s: option \"%s\" requires and argument.\n",argv[0],argv[a]);
            usage(argv[0]);
	}
	else
        {
            fprintf(stderr,"%s: bad option \"%s\"\n",argv[0],argv[a]);
            usage(argv[0]);
        }
    
    eventlog_set(stderr);
    /* errors to eventlog from here on... */
    
#ifdef USE_CHECK_ALLOC
    if (foreground)
	check_set_file(stderr);
    else
	eventlog(eventlog_level_warn,"main","memory allocation checking only available in foreground mode");
#endif
    
    if (preffile)
    {
	if (prefs_load(preffile)<0)
	{
	    eventlog(eventlog_level_fatal,"main","could not parse specified configuration file (exiting)");
	    return STATUS_FAILURE;
	}
    }
    else
	if (prefs_load(BNETD_DEFAULT_CONF_FILE)<0)
	    eventlog(eventlog_level_warn,"main","using default configuration");
    
    {
	char const * levels;
	char *       temp;
	char const * tok;
	
	eventlog_clear_level();
	if ((levels = prefs_get_loglevels()))
	{
	    if (!(temp = strdup(levels)))
	    {
		eventlog(eventlog_level_fatal,"main","could not allocate memory for temp (exiting)");
		return STATUS_FAILURE;
	    }
	    tok = strtok(temp,","); /* strtok modifies the string it is passed */
	    while (tok)
	    {
		if (eventlog_add_level(tok)<0)
		    eventlog(eventlog_level_error,"main","could not add log level \"%s\"",tok);
		tok = strtok(NULL,",");
	    }
	    free(temp);
	}
    }
    
    if (eventlog_open(prefs_get_logfile())<0)
    {
	if (prefs_get_logfile())
	    eventlog(eventlog_level_fatal,"main","could not use file \"%s\" for the eventlog (exiting)",prefs_get_logfile());
	else
	    eventlog(eventlog_level_fatal,"main","no logfile specified in configuration file \"%s\" (exiting)",preffile);
	return STATUS_FAILURE;
    }
    
    /* eventlog goes to log file from here on... */
    
#ifdef DO_DAEMONIZE
    if (!foreground)
    {
	if (chdir("/")<0)
	{
	    eventlog(eventlog_level_error,"main","could not change working directory to / (chdir: %s)",strerror(errno));
	    return STATUS_FAILURE;
	}
	
	switch (fork())
	{
	case -1:
	    eventlog(eventlog_level_error,"main","could not fork (fork: %s)",strerror(errno));
	    return STATUS_FAILURE;
	case 0: /* child */
	    break;
	default: /* parent */
	    return STATUS_SUCCESS;
	}
	
	close(STDINFD);
	close(STDOUTFD);
	close(STDERRFD);
#ifdef USE_CHECK_ALLOC
	check_set_file(NULL);
#endif
    
# ifdef HAVE_SETPGID
	if (setpgid(0,0)<0)
	{
	    eventlog(eventlog_level_error,"main","could not create new process group (setpgid: %s)",strerror(errno));
	    return STATUS_FAILURE;
	}
# else
#  ifdef HAVE_SETPGRP
#   ifdef SETPGRP_VOID
        if (setpgrp()<0)
        {
            eventlog(eventlog_level_error,"main","could not create new process group (setpgrp: %s)",strerror(errno));
            return STATUS_FAILURE;
        }
#   else
	if (setpgrp(0,0)<0)
	{
	    eventlog(eventlog_level_error,"main","could not create new process group (setpgrp: %s)",strerror(errno));
	    return STATUS_FAILURE;
	}
#   endif
#  else
#   ifdef HAVE_SETSID
	if (setsid()<0)
	{
	    eventlog(eventlog_level_error,"main","could not create new process group (setsid: %s)",strerror(errno));
	    return STATUS_FAILURE;
	}
#   else
#    error "One of setpgid(), setpgrp(), or setsid() is required"
#   endif
#  endif
# endif
    }
#endif
    
    pidfile = strdup(prefs_get_pidfile());
    if (pidfile[0]=='\0')
    {
	free((void *)pidfile); /* avoid warning */
	pidfile = NULL;
    }
    
    if (pidfile)
    {
#ifdef HAVE_GETPID
	FILE * fp;
	
	if (!(fp = fopen(pidfile,"w")))
	{
	    eventlog(eventlog_level_error,"main","unable to open pid file \"%s\" for writing (fopen: %s)",pidfile,strerror(errno));
	    free((void *)pidfile); /* avoid warning */
	    pidfile = NULL;
	}
	else
	{
	    fprintf(fp,"%u",(unsigned int)getpid());
	    if (fclose(fp)<0)
		eventlog(eventlog_level_error,"main","could not close pid file \"%s\" after writing (fclose: %s)",pidfile,strerror(errno));
	}
#else
	eventlog(eventlog_level_warn,"main","no getpid() system call, disable pid file in bnetd.conf");
	free((void *)pidfile); /* avoid warning */
	pidfile = NULL;
#endif
    }
    
    /* Hakan: That's way too late to give up root privileges... Have to look for a better place */
    if (give_up_root_privileges(prefs_get_effective_user(),
                                prefs_get_effective_group())<0)
    {
	eventlog(eventlog_level_fatal,"main","could not give up privileges (exiting)");
	return STATUS_FAILURE;
    }
    
#ifdef HAVE_GETPID
    eventlog(eventlog_level_info,"main","bnetd version "BNETD_VERSION" process %u",(unsigned int)getpid());
#else
    eventlog(eventlog_level_info,"main","bnetd version "BNETD_VERSION);
#endif
    eventlog(eventlog_level_info,"main","logging event levels: %s",prefs_get_loglevels());
    
    if (hexfile)
	if (!(hexstrm = fopen(hexfile,"w")))
	    eventlog(eventlog_level_error,"main","could not open file \"%s\" for writing the hexdump (fopen: %s)",hexfile,strerror(errno));
	else
	    fprintf(hexstrm,"# dump generated by bnetd version "BNETD_VERSION"\n");
    
    connlist_create();
    gamelist_create();
    query_init();
#ifdef WITH_BITS
    bits_query_init();
    if (bits_init()<0) {
	eventlog(eventlog_level_error,"main","BITS initialization failed");
	goto bits_fail; /* Typhoon: Hmmm, I don't like labels ... */
    }
#endif
    channellist_create();
    if (helpfile_init(prefs_get_helpfile())<0)
	eventlog(eventlog_level_error,"main","could not load helpfile");
    if (ipbanlist_load(prefs_get_ipbanfile())<0)
	eventlog(eventlog_level_error,"main","could not load IP ban list");
    if (adbannerlist_create(prefs_get_adfile())<0)
	eventlog(eventlog_level_error,"main","could not load adbanner list");
    if (gametrans_load(prefs_get_transfile())<0)
	eventlog(eventlog_level_error,"main","could not load gametrans list");
    if (autoupdate_load(prefs_get_mpqfile())<0)
	eventlog(eventlog_level_error,"main","could not load autoupdate list");
    if (versioncheck_load(prefs_get_versioncheck_file())<0)
	eventlog(eventlog_level_error,"main","could not load versioncheck list");
    timerlist_create();
    watchlist_create();
    accountlist_load_default();
    accountlist_create();
    ladderlist_create();
    if (realmlist_create(prefs_get_realmfile())<0)
	eventlog(eventlog_level_error,"main","could not load realm list");
    if (characterlist_create("")<0)
	eventlog(eventlog_level_error,"main","could not load character list");
    if (prefs_get_track()) /* setup the tracking mechanism */
        tracker_set_servers(prefs_get_trackserv_addrs());
    
    /* now process connections and network traffic */
    if (server_process()<0)
    {
        eventlog(eventlog_level_fatal,"main","failed to initialize network (exiting)");
	if (pidfile)
	{
	    if (remove(pidfile)<0)
		eventlog(eventlog_level_error,"main","could not remove pid file \"%s\" (remove: %s)",pidfile,strerror(errno));
	    free((void *)pidfile); /* avoid warning */
	}
        return STATUS_FAILURE;
    }
    
    tracker_set_servers(NULL);
    characterlist_destroy();
    realmlist_destroy();
    ladderlist_destroy();
    accountlist_destroy();
    accountlist_unload_default();
    watchlist_destroy();
    timerlist_destroy();
    autoupdate_unload();
    gametrans_unload();
    adbannerlist_destroy();
    ipbanlist_unload();
    helpfile_unload();
    channellist_destroy();
#ifdef WITH_BITS
    bits_destroy();
  bits_fail:
    bits_query_destroy();
#endif
    query_destroy();
    gamelist_destroy();
    connlist_destroy();
    
    if (hexstrm)
    {
	fprintf(hexstrm,"# end of dump\n");
	if (fclose(hexstrm)<0)
	    eventlog(eventlog_level_error,"main","could not close hexdump file \"%s\" after writing (fclose: %s)",hexfile,strerror(errno));
    }
    
    eventlog(eventlog_level_info,"main","server has shut down");
    if (pidfile)
    {
	if (remove(pidfile)<0)
	    eventlog(eventlog_level_error,"main","could not remove pid file \"%s\" (remove: %s)",pidfile,strerror(errno));
	free((void *)pidfile); /* avoid warning */
    }
    
    prefs_unload();
    
#ifdef USE_CHECK_ALLOC
    check_cleanup();
#endif
    
    return STATUS_SUCCESS;
}

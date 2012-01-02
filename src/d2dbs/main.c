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

#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
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
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#include "server.h"
#include "prefs.h"
#include "cmdline_parse.h"
#include "version.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static FILE * eventlog_fp;
#ifdef USE_CHECK_ALLOC
static FILE * memlog_fp;
#endif

static int init(void);
static int cleanup(void);
static int config_init(int argc, char * * argv);
static int config_cleanup(void);
static int setup_daemon(void);

static int setup_daemon(void)
{
	if (chdir("/")<0) {
		log_error("can not change working directory to root directory (chdir: %s)",strerror(errno));
		return -1;
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if (!cmdline_get_logstderr()) {
		close(STDERR_FILENO);
	}
	switch (fork()) {
		case 0:
			break;
		case -1:
			log_error("error create child process (fork: %s)",strerror(errno));
			return -1;
		default:
			return -1;
	}
	umask(0);
	setsid();
	return 0;
}

static int init(void)
{
	return 0;
}

static int cleanup(void)
{
	return 0;
}
	
static int config_init(int argc, char * * argv)
{
    char const * levels;
    char *       temp;
    char const * tok;

	if (cmdline_parse(argc, argv)<0) {
		return -1;
	}
	if (cmdline_get_version()) {
		cmdline_show_version();
		return -1;
	}
	if (cmdline_get_help()) {
		cmdline_show_help();
		return -1;
	}
	if (!cmdline_get_foreground()) {
		if (setup_daemon()<0) {
			return -1;
		}
	}
	if (prefs_load(cmdline_get_prefs_file())<0) {
		log_error("error loading configuration file %s",cmdline_get_prefs_file());
		return -1;
	}

    eventlog_clear_level();
    if ((levels = prefs_get_loglevels()))
    {
        if (!(temp = strdup(levels)))
        {
         eventlog(eventlog_level_fatal,"main","could not allocate memory for temp (exiting)");
         return -1;
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


	if (cmdline_get_logstderr()) {
		eventlog_set(stderr);
	} else if (cmdline_get_logfile()) {
		if (eventlog_open(cmdline_get_logfile())<0) {
			log_error("error open eventlog file %s",cmdline_get_logfile());
			return -1;
		}
	} else {
		if (eventlog_open(prefs_get_logfile())<0) {
			log_error("error open eventlog file %s",prefs_get_logfile());
			return -1;
		}
	}
#ifdef USE_CHECK_ALLOC
	memlog_fp=fopen(cmdline_get_memlog_file(),"a");
	if (!memlog_fp) {
		log_warn("error open file %s for memory debug logging",cmdline_get_memlog_file());
	} else {
		check_set_file(memlog_fp);
	}
#endif
	return 0;
}

static int config_cleanup(void)
{
	prefs_unload();
	cmdline_cleanup(); 
	if (eventlog_fp) fclose(eventlog_fp);
	return 0;
}

int main(int argc, char * * argv)
{
#ifdef USE_CHECK_ALLOC
	check_set_file(stderr);
#endif
	eventlog_set(stderr);
	if (config_init(argc, argv)<0) {
		return 1;
	}
	log_info(D2DBS_VERSION);
	if (init()<0) {
		log_error("failed to init");
		return 1;
	} else {
		log_info("server initialized");
	}
	if (server_process()<0) {
		log_error("failed to run server");
		return 1;
	}
	cleanup();
	config_cleanup();
#ifdef USE_CHECK_ALLOC
	check_cleanup();
	if (memlog_fp) fclose(memlog_fp);
#endif
	return 0;
}

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

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
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
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef DO_POSIXSIG
# include <signal.h>
# include "compat/signal.h"
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "dbserver.h"
#include "prefs.h"
#include "d2ladder.h"
#include "cmdline_parse.h"
#include "handle_signal.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static void on_signal(int s);

static volatile struct
{
	unsigned char	do_quit;
	unsigned char	cancel_quit;
	unsigned char	reload_config;
	unsigned char	save_ladder;
	unsigned int	exit_time;
} signal_data ={ 0, 0, 0, 0, 0 };

extern int handle_signal(void)
{
	time_t		now;
    char const * levels;
    char *       temp;
    char const * tok;


	if (signal_data.cancel_quit) {
		signal_data.cancel_quit=0;
		if (!signal_data.exit_time) {
			log_info("there is no previous shutdown to be canceled");
		} else {
			signal_data.exit_time=0;
			log_info("shutdown was canceled due to signal");
		}
	}
	if (signal_data.do_quit) {
		signal_data.do_quit=0;
		now=time(NULL);
		if (!signal_data.exit_time) {
			signal_data.exit_time=now+prefs_get_shutdown_delay();
		} else {
			signal_data.exit_time-=prefs_get_shutdown_decr();
		}
		if (now >= signal_data.exit_time) {
			log_info("shutdown server due to signal");
			return -1;
		}
		log_info("the server is going to shutdown in %lu minutes",(signal_data.exit_time-now)/60);
	}
	if (signal_data.reload_config) {
		signal_data.reload_config=0;
		log_info("reloading configuartion file due to signal");
		if (prefs_reload(cmdline_get_prefs_file())<0) {
			log_error("error reload configuration file,exitting");
			return -1;
		}
        eventlog_clear_level();
        if ((levels = prefs_get_loglevels()))
        {
          if (!(temp = strdup(levels)))
          {
           eventlog(eventlog_level_fatal,"handle_signal","could not allocate memory for temp (exiting)");
         return -1;
          }

          tok = strtok(temp,","); /* strtok modifies the string it is passed */

          while (tok)
          {
          if (eventlog_add_level(tok)<0)
              eventlog(eventlog_level_error,"handle_signal","could not add log level \"%s\"",tok);
          tok = strtok(NULL,",");
          }
          free(temp);
        }

		if (!cmdline_get_logstderr()) eventlog_open(prefs_get_logfile());
	}
	if (signal_data.save_ladder) {
		signal_data.save_ladder=0;
		log_info("save ladder data due to signal");
		d2ladder_saveladder();
	}
	return 0;
}

extern int handle_signal_init(void)
{
	signal(SIGINT,on_signal);
	signal(SIGTERM,on_signal);
	signal(SIGABRT,on_signal);
	signal(SIGHUP,on_signal);
	signal(SIGUSR1,on_signal);
	signal(SIGPIPE,on_signal);
	return 0;
}

static void on_signal(int s)
{
	switch (s) {
		case SIGINT:
			log_debug("sigint received");
			signal_data.do_quit=1;
			break;
		case SIGTERM:
			log_debug("sigint received");
			signal_data.do_quit=1;
			break;
		case SIGABRT:
			log_debug("sigabrt received");
			signal_data.cancel_quit=1;
			break;
		case SIGHUP:
			log_debug("sighup received");
			signal_data.reload_config=1;
			break;
		case SIGUSR1:
			log_debug("sigusr1 received");
			signal_data.save_ladder=1;
			break;
		case SIGPIPE:
			log_debug("sigpipe received");
			break;
	}
	signal(s,on_signal);
}

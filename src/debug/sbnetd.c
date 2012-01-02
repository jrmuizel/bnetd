/*
 * Copyright (C) 1999  stacker (stacker@mail.ee.ntou.edu.tw)
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include "compat/exitstatus.h"
#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include "common/setup_after.h"

#if !defined(HAVE_FORK) || !defined(HAVE_PIPE) || !defined(HAVE_WAITPID)
# error "This program requires fork(), pipe(), and waitpid()"
#endif

/*
 * This is a simple front-end to restart bnetd if it crashes or is killed.
 * It * will also log the reason bnetd died.  PLEASE report crashing bugs
 * (well, any kind of bug) if you see them!  We want the server to be stable.
 *
 * bnetd is started with the following options:
 *  BNETD_BIN -f -c BNETD_CONF
 * Log messages from this program are appended to the bnetd.log.
 * The values for those paths are defined below...
 */

/* Change the define below to the main bnetd directory */          
/*#define BNETD_HOME "/home/bnet"*/
#define BNETD_HOME "."

/* FIXME: make command-line options */
/* Change the defines below to match the paths for these files */
#define BNETD_LOG  BNETD_HOME"/bnetd.log"
#define BNETD_CONF BNETD_DEFAULT_CONF_FILE
/*#define BNETD_CONF BNETD_HOME"/conf/bnetd.conf"*/
#define BNETD_BIN  BNETD_HOME"/sbin/bnetd"


extern int main(int argc, char * argv[])
{
    FILE *       log_file;
    pid_t        bnetd_pid;
    int          status;
    char const * progname;
    int          err;
    
    if (argc<1 || !argv || !argv[0])
	progname = "sbnetd";
    else
	progname = argv[0];
    
    switch (fork())
    {
    case -1:        /* error occurred. */
	fprintf(stderr,"%s: could not background (fork: %s)\n",progname,strerror(errno));
	return STATUS_FAILURE;
	
    case 0:         /* child. */
	break;
	
    default:        /* parent. */
	return STATUS_SUCCESS;
    }
    
    close(0);
    close(1);
    close(2);                
    
    for (;;)
    {
	bnetd_pid = fork();
	if (bnetd_pid > 0)
	{
	    /* wait until child finished. */
	    if (waitpid(bnetd_pid,&status,0)!=bnetd_pid)
	    {
		log_file = fopen(BNETD_LOG,"a");
		if (log_file)
		{
		    fprintf(log_file,"%s: could not obtain exit status for bnetd process %u (waitpid: %s)\n",progname,(unsigned int)bnetd_pid,strerror(errno));
		    fclose(log_file);
		}
	    }
	    else
	    {
		log_file = fopen(BNETD_LOG,"a");
		if (log_file)
		{
		    if (WIFEXITED(status))
		    {
			fprintf(log_file,"%s: bnetd process %u has exited normally with status %u\n",progname,(unsigned int)bnetd_pid,(unsigned int)WEXITSTATUS(status));
			sleep(1); /* avoid filling up filesystem with errors */
		    }
		    else if(WIFSIGNALED(status)) {
			fprintf(log_file,"%s: bnetd process %u has exited due to signal %u\n",progname,(unsigned int)bnetd_pid,(unsigned int)WTERMSIG(status));
		    }
		    else
		    {
			fprintf(log_file,"%s: bnetd process %u has exited for an unknown reason\n",progname,(unsigned int)bnetd_pid);
		    }
		    fclose(log_file);
		}
	    }
	}
	else if (bnetd_pid == 0)
	{       /* this is the child process. */
	    execl(BNETD_BIN,"bnetd","-f","-c",BNETD_CONF,(char *)NULL);
	    err = errno;
	    
	    log_file = fopen(BNETD_LOG,"a");
	    if (log_file)
	    {
		fprintf(log_file,"%s: error starting bnetd (execl: %s)\n",progname,strerror(err));
		fclose(log_file);
	    }
	    sleep(1); /* avoid filling up filesystem with errors */
	    return STATUS_FAILURE; /* return failure */
	}
	else if (bnetd_pid < 0)
	{        /* error occurred. */
	    err = errno;
	    
	    log_file = fopen(BNETD_CONF,"a");
	    if (log_file)
	    {
		fprintf(log_file,"%s: error forking child process (fork: %s)\n",progname,strerror(err));
		fclose(log_file);
	    }
	}
    }
}

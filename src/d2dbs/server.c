/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
 * Copyright (C) 2001		sousou	(liupeng.cs@263.net)
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

#include "dbserver.h"
#include "server.h"
#include "prefs.h"
#include "handle_signal.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

extern int server_process(void)
{
	char		* p, * servaddr;
	unsigned short	port;

	handle_signal_init();
	if (!(servaddr=strdup(prefs_get_servaddrs()))) {
		return -1;
	}
	p=strchr(servaddr,':');
	if (p) {
		port=(unsigned short)strtoul(p+1,NULL,10);
		*p='\0';
	} else {
		port=DEFAULT_LISTEN_PORT;
	}
	dbs_server_main(servaddr, port);
	free(servaddr);
	return 0;
}

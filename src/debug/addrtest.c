/*
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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#include "common/eventlog.h"
#include "common/addr.h"
#include "common/list.h"
#include "common/setup_after.h"


extern int main(int argc, char * argv[])
{
    t_addrlist *   addrlist;
    t_addr const * addr;
    t_elem const * curr;
    char           temp1[32];
    char           temp2[256];
    
    if (argc<1 || !argv || !argv[0])
    {
        fprintf(stderr,"bad arguments\n");
        return STATUS_FAILURE;
    }
    
    eventlog_set(stderr);
    
    if (argc!=2)
    {
	fprintf(stderr,"usage: %s <addresslist>\n",argv[0]);
	return STATUS_FAILURE;
    }
    
    if (!(addrlist = addrlist_create(argv[1],INADDR_LOOPBACK,1234)))
    {
	fprintf(stderr,"%s: could not create addrlist\n",argv[0]);
	return STATUS_FAILURE;
    }
    
    LIST_TRAVERSE_CONST(addrlist,curr)
    {
	addr = elem_get_data(curr);
	if (!addr_get_addr_str(addr,temp1,sizeof(temp1)))
	    strcpy(temp1,"x.x.x.x:x");
	if (!addr_get_host_str(addr,temp2,sizeof(temp2)))
	    strcpy(temp2,"unknown:x");
	printf(" \"%s\" \"%s\"\n",temp1,temp2);
    }
    
    return STATUS_SUCCESS;
}

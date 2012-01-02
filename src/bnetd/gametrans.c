/*
 * Copyright (C) 2000 Ross Combs (rocombs@cs.nmsu.edu)
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
#define GAMETRANS_INTERNAL_ACCESS
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
#include "compat/strrchr.h"
#include <errno.h>
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/addr.h"
#include "common/util.h"
#include "gametrans.h"
#include "common/setup_after.h"


static t_list * gametrans_head=NULL;


extern int gametrans_load(char const * filename)
{
    FILE *        fp;
    unsigned int  line;
    unsigned int  pos;
    char *        buff;
    char *        temp;
    char const *  viewer;
    char const *  client;
    char const *  output;
    char const *  exclude;
    t_gametrans * entry;
    
    if (!filename)
    {
        eventlog(eventlog_level_error,"gametrans_load","got NULL filename");
        return -1;
    }
    
    if (!(gametrans_head = list_create()))
    {
        eventlog(eventlog_level_error,"gametrans_load","could not create list");
        return -1;
    }
    if (!(fp = fopen(filename,"r")))
    {
        eventlog(eventlog_level_error,"gametrans_load","could not open file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	list_destroy(gametrans_head);
	gametrans_head = NULL;
        return -1;
    }
    
    for (line=1; (buff = file_get_line(fp)); line++)
    {
        for (pos=0; buff[pos]=='\t' || buff[pos]==' '; pos++);
        if (buff[pos]=='\0' || buff[pos]=='#')
        {
            free(buff);
            continue;
        }
        if ((temp = strrchr(buff,'#')))
        {
	    unsigned int len;
	    unsigned int endpos;
	    
            *temp = '\0';
	    len = strlen(buff)+1;
            for (endpos=len-1; buff[endpos]=='\t' || buff[endpos]==' '; endpos--);
            buff[endpos+1] = '\0';
        }
	
	if (!(viewer = strtok(buff," \t"))) /* strtok modifies the string it is passed */
	{
	    eventlog(eventlog_level_error,"gametrans_load","missing viewer on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(client = strtok(NULL," \t")))
	{
	    eventlog(eventlog_level_error,"gametrans_load","missing client on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(output = strtok(NULL," \t")))
	{
	    eventlog(eventlog_level_error,"gametrans_load","missing output on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(exclude = strtok(NULL," \t")))
	    exclude = "0.0.0.0/0"; /* no excluded network address */
	
	if (!(entry = malloc(sizeof(t_gametrans))))
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not allocate memory for entry");
	    free(buff);
	    continue;
	}
	if (!(entry->viewer = addr_create_str(viewer,0,0)))
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not allocate memory for viewer address");
	    free(entry);
	    free(buff);
	    continue;
	}
	if (!(entry->client = addr_create_str(client,0,6112)))
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not allocate memory for client address");
	    addr_destroy(entry->viewer);
	    free(entry);
	    free(buff);
	    continue;
	}
	if (!(entry->output = addr_create_str(output,0,6112)))
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not allocate memory for output address");
	    addr_destroy(entry->client);
	    addr_destroy(entry->viewer);
	    free(entry);
	    free(buff);
	    continue;
	}
	if (!(entry->exclude = netaddr_create_str(exclude)))
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not allocate memory for exclude address");
	    addr_destroy(entry->output);
	    addr_destroy(entry->client);
	    addr_destroy(entry->viewer);
	    free(entry);
	    free(buff);
	    continue;
	}
	
	free(buff);
	
	if (list_append_data(gametrans_head,entry)<0)
	{
	    eventlog(eventlog_level_error,"gametrans_load","could not append item");
	    netaddr_destroy(entry->exclude);
	    addr_destroy(entry->output);
	    addr_destroy(entry->client);
	    addr_destroy(entry->viewer);
	    free(entry);
	}
    }
    
    return 0;
}


extern int gametrans_unload(void)
{
    t_elem *      curr;
    t_gametrans * entry;
    
    if (gametrans_head)
    {
	LIST_TRAVERSE(gametrans_head,curr)
	{
	    if (!(entry = elem_get_data(curr)))
		eventlog(eventlog_level_error,"gametrans_unload","found NULL entry in list");
	    else
	    {
		netaddr_destroy(entry->exclude);
		addr_destroy(entry->output);
		addr_destroy(entry->client);
		addr_destroy(entry->viewer);
		free(entry);
	    }
	    list_remove_elem(gametrans_head,curr);
	}
	list_destroy(gametrans_head);
	gametrans_head = NULL;
    }
    
    return 0;
}


extern void gametrans_net(unsigned int vaddr, unsigned short vport, unsigned int laddr, unsigned short lport, unsigned int * addr, unsigned short * port)
{
    t_elem const * curr;
    t_gametrans *  entry;
#ifdef DEBUGGAMETRANS
    char           temp1[32];
    char           temp2[32];
    char           temp3[32];
    char           temp4[32];
#endif
    
#ifdef DEBUGGAMETRANS
    eventlog(eventlog_level_debug,"gametrans_net","checking client %s (viewer on %s connected to %s)...",
	     addr_num_to_addr_str(*addr,*port),
	     addr_num_to_addr_str(vaddr,vport),
	     addr_num_to_addr_str(laddr,lport));
#endif
    if (gametrans_head)
    {
	LIST_TRAVERSE_CONST(gametrans_head,curr)
	{
	    if (!(entry = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error,"gametrans_net","found NULL entry in list");
		continue;
	    }
	    
#ifdef DEBUGGAMETRANS
	    eventlog(eventlog_level_debug,"gametrans_net","against entry viewerint=%s client=%s output=%s viewerex=%s",
		     addr_get_addr_str(entry->viewer,temp1,sizeof(temp1)),
		     addr_get_addr_str(entry->client,temp2,sizeof(temp2)),
		     addr_get_addr_str(entry->output,temp3,sizeof(temp3)),
		     netaddr_get_addr_str(entry->exclude,temp4,sizeof(temp4)));
#endif
	    
	    if (addr_get_ip(entry->viewer)!=0 && addr_get_ip(entry->viewer)!=laddr)
	    {
#ifdef DEBUGGAMETRANS
		eventlog(eventlog_level_debug,"gametrans_net","viewer is not on right interface IP");
#endif
		continue;
	    }
	    if (addr_get_port(entry->viewer)!=0 && addr_get_port(entry->viewer)!=lport)
	    {
#ifdef DEBUGGAMETRANS
		eventlog(eventlog_level_debug,"gametrans_net","view is not on right interface port");
#endif
		continue;
	    }
	    
	    if (addr_get_ip(entry->client)!=0 && addr_get_ip(entry->client)!=*addr)
	    {
#ifdef DEBUGGAMETRANS
		eventlog(eventlog_level_debug,"gametrans_net","client is not on the right IP");
#endif
		continue;
	    }
	    if (addr_get_port(entry->client)!=0 && addr_get_port(entry->client)!=*port)
	    {
#ifdef DEBUGGAMETRANS
		eventlog(eventlog_level_debug,"gametrans_net","client is not on the right port");
#endif
		continue;
	    }
	    
	    if (netaddr_contains_addr_num(entry->exclude,vaddr)==1)
	    {
#ifdef DEBUGGAMETRANS
		eventlog(eventlog_level_debug,"gametrans_net","viewer is in the excluded network");
#endif
		continue;
	    }
	    
	    *addr = addr_get_ip(entry->output);
	    *port = addr_get_port(entry->output);
	    
#ifdef DEBUGGAMETRANS
	    eventlog(eventlog_level_debug,"gametrans_net","did translation");
#endif
	    
	    return;
	}
    }
}

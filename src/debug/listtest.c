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
/* A simple program to stress-test the linked list routines in list.c */
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
#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strdup.h"
#include "eventlog.h"
#include "list.h"
#include "common/setup_after.h"


extern int main(int argc, char * argv[])
{
    t_list *     head=NULL;
    t_elem *     curr;
    char   *     data;
    unsigned int choose;
    unsigned int count=0;
    
    if (argc<1 || !argv || !argv[0])
    {
	fprintf(stderr,"bad arguments\n");
	return STATUS_FAILURE;
    }
    
    eventlog_set(stderr);
    
    for (;;)
    {
	if (!head)
	{
	    if (!(head = list_create()))
	    {
		fprintf(stderr,"%s: unable to create list\n",argv[0]);
		abort();
	    }
/*	    printf("create\n");*/
	}
	
	choose = rand()%14;
	switch (choose)
	{
	case 0: /* purge */
	case 1:
	    if (list_purge(head)<0)
	    {
		fprintf(stderr,"%s: unable to purge list\n",argv[0]);
		abort();
	    }
/*	    printf("purge\n");*/
	    break;
	case 2: /* delete all by elem, destroy */
	    LIST_TRAVERSE(head,curr)
	    {
		if (!(data = elem_get_data(curr)))
		{
		    fprintf(stderr,"%s: unable to obtain data\n",argv[0]);
		    abort();
		}
		free(data);
		if (list_remove_elem(head,curr)<0)
		{
		    fprintf(stderr,"%s: unable to remove element\n",argv[0]);
		    abort();
		}
	    }
	    if (list_destroy(head)<0)
	    {
		fprintf(stderr,"%s: unable to destroy list\n",argv[0]);
		abort();
	    }
/*	    printf("destroy\n");*/
	    head = NULL;
	    break;
	case 3: /* delete all by data, destroy */
	    LIST_TRAVERSE(head,curr)
	    {
		if (!(data = elem_get_data(curr)))
		{
		    fprintf(stderr,"%s: unable to obtain data\n",argv[0]);
		    abort();
		}
		free(data);
		if (list_remove_data(head,data)<0)
		{
		    fprintf(stderr,"%s: unable to remove element\n",argv[0]);
		    abort();
		}
	    }
	    if (list_destroy(head)<0)
	    {
		fprintf(stderr,"%s: unable to destroy list\n",argv[0]);
		abort();
	    }
/*	    printf("destroy\n");*/
	    head = NULL;
	    break;
	case 4: /* delete random element by elem */
	case 5:
	    if (list_get_length(head)==0)
		break;
	    {
		unsigned int pos;
		
		pos = rand()%list_get_length(head);
		if (!(data = list_get_data_by_pos(head,pos)))
		{
		    fprintf(stderr,"%s: unable to obtain element %u\n",argv[0],pos);
		    abort();
		}
		if (!(curr = list_get_elem_by_data(head,data)))
		{
		    fprintf(stderr,"%s: unable to find element for \"%s\"\n",argv[0],data);
		    abort();
		}
		if (list_remove_elem(head,curr)<0)
		{
		    fprintf(stderr,"%s: unable to remove element\n",argv[0]);
		    abort();
		}
/*		printf("delete %s\n",data);*/
		free(data);
	    }
	    break;
	case 6: /* delete random element by data */
	case 7:
	    if (list_get_length(head)==0)
		break;
	    {
		unsigned int pos;
		
		pos = rand()%list_get_length(head);
		if (!(data = list_get_data_by_pos(head,pos)))
		{
		    fprintf(stderr,"%s: unable to obtain data %u\n",argv[0],pos);
		    abort();
		}
		if (list_remove_data(head,data)<0)
		{
		    fprintf(stderr,"%s: unable to remove data\n",argv[0]);
		    abort();
		}
/*		printf("delete %s\n",data);*/
		free(data);
	    }
	    break;
	case 8: /* prepend random string */
	case 9:
	case 10:
	    {
		char temp[32];
		
		sprintf(temp,"%u",count++);
		if (!(data = strdup(temp)))
		{
		    fprintf(stderr,"%s: unable to duplicate string\n",argv[0]);
		    abort();
		}
		if (list_prepend_data(head,data)<0)
		{
		    fprintf(stderr,"%s: unable to prepend element\n",argv[0]);
		    abort();
		}
/*		printf("prepend %s\n",data);*/
	    }
	    break;
	case 11: /* append random string */
	case 12:
	case 13:
	    {
		char temp[32];
		
		sprintf(temp,"%u",count++);
		if (!(data = strdup(temp)))
		{
		    fprintf(stderr,"%s: unable to duplicate string\n",argv[0]);
		    abort();
		}
		if (list_append_data(head,data)<0)
		{
		    fprintf(stderr,"%s: unable to append element\n",argv[0]);
		    abort();
		}
/*		printf("append %s\n",data);*/
	    }
	    break;
	}
	
	if (head && list_check(head)<0)
	{
	    fprintf(stderr,"%s: list did not check, count=%u\n",argv[0],count);
	    abort();
	}
    }
}

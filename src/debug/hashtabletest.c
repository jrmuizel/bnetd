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
/* A simple program to stress-test the hash table routines in hashtable.c */
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
#include <ctype.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#include "compat/char_bit.h"
#include "eventlog.h"
#include "hashtable.h"
#include "common/introtate.h"
#include "common/setup_after.h"


#define ROWS 5

#define dprintf if (0) printf


static unsigned int account_hash(char const * username)
{
    unsigned int i;
    unsigned int pos;
    unsigned int hash;
    unsigned int ch;

    if (!username)
    {
        eventlog(eventlog_level_error,"account_hash","got NULL username");
        return 0;
    }

    for (hash=0,pos=0,i=0; i<strlen(username); i++)
    {
        if (isascii((int)username[i]))
            ch = (unsigned int)(unsigned char)tolower((int)username[i]);
        else
            ch = (unsigned int)(unsigned char)username[i];
        hash ^= ROL(ch,pos,sizeof(unsigned int)*CHAR_BIT);
        pos += CHAR_BIT-1;
    }

    return hash;
}


extern int main(int argc, char * argv[])
{
    t_hashtable * table=NULL;
    t_entry *     curr;
    char   *      data;
    unsigned int  choose;
    unsigned int  count=0;
    
    if (argc<1 || !argv || !argv[0])
    {
	fprintf(stderr,"bad arguments\n");
	return STATUS_FAILURE;
    }
    
    eventlog_set(stderr);
#ifdef USE_CHECK_ALLOC
    check_set_file(stderr);
#endif
    
    for (;;)
    {
	if (!table)
	{
	    if (!(table = hashtable_create(ROWS)))
	    {
		fprintf(stderr,"%s: unable to create hashtable\n",argv[0]);
		abort();
	    }
	    dprintf("create %u\n",ROWS);
	}
	
	choose = rand()%12;
	switch (choose)
	{
	case 0: /* purge */
	case 1:
	    if (hashtable_purge(table)<0)
	    {
		fprintf(stderr,"%s: unable to purge hashtable\n",argv[0]);
		abort();
	    }
	    dprintf("purge\n");
	    break;
	case 2: /* delete all by entry, destroy */
	    HASHTABLE_TRAVERSE(table,curr)
	    {
		if (!(data = entry_get_data(curr)))
		{
		    fprintf(stderr,"%s: unable to obtain data\n",argv[0]);
		    abort();
		}
		free(data);
		if (hashtable_remove_entry(table,curr)<0)
		{
		    fprintf(stderr,"%s: unable to remove entry\n",argv[0]);
		    abort();
		}
	    }
	    if (hashtable_destroy(table)<0)
	    {
		fprintf(stderr,"%s: unable to destroy hashtable\n",argv[0]);
		abort();
	    }
	    dprintf("destroy\n");
	    table = NULL;
	    break;
	case 3: /* delete all by data, destroy */
	    HASHTABLE_TRAVERSE(table,curr)
	    {
		if (!(data = entry_get_data(curr)))
		{
		    fprintf(stderr,"%s: unable to obtain data\n",argv[0]);
		    abort();
		}
		if (hashtable_remove_data(table,data,account_hash(data))<0)
		{
		    fprintf(stderr,"%s: unable to remove entry\n",argv[0]);
		    abort();
		}
		free(data);
	    }
	    if (hashtable_destroy(table)<0)
	    {
		fprintf(stderr,"%s: unable to destroy hashtable\n",argv[0]);
		abort();
	    }
	    dprintf("destroy\n");
	    table = NULL;
	    break;
	case 4: /* delete random entry by entry */
	case 5:
	    if (hashtable_get_length(table)==0)
		break;
	    {
		unsigned int pos;
		
		pos = rand()%hashtable_get_length(table);
		if (!(data = hashtable_get_data_by_pos(table,pos)))
		{
		    fprintf(stderr,"%s: unable to obtain entry %u\n",argv[0],pos);
		    abort();
		}
		if (!(curr = hashtable_get_entry_by_data(table,data,account_hash(data))))
		{
		    fprintf(stderr,"%s: unable to find entry for \"%s\"\n",argv[0],data);
		    abort();
		}
		if (hashtable_remove_entry(table,curr)<0)
		{
		    fprintf(stderr,"%s: unable to remove entry\n",argv[0]);
		    abort();
		}
		dprintf("delete %s,%u\n",data,account_hash(data));
		free(data);
	    }
	    break;
	case 6: /* delete random entry by data */
	case 7:
	    if (hashtable_get_length(table)==0)
		break;
	    {
		unsigned int pos;
		
		pos = rand()%hashtable_get_length(table);
		if (!(data = hashtable_get_data_by_pos(table,pos)))
		{
		    fprintf(stderr,"%s: unable to obtain data %u\n",argv[0],pos);
		    abort();
		}
		if (hashtable_remove_data(table,data,account_hash(data))<0)
		{
		    fprintf(stderr,"%s: unable to remove data\n",argv[0]);
		    abort();
		}
		dprintf("delete %s,%u\n",data,account_hash(data));
		free(data);
	    }
	    break;
	case 8: /* insert random string */
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
		if (hashtable_insert_data(table,data,account_hash(temp))<0)
		{
		    fprintf(stderr,"%s: unable to insert entry\n",argv[0]);
		    abort();
		}
		dprintf("insert %s,%u\n",data,account_hash(temp));
	    }
	    break;
        case 11:
	    if (count>10000)
	    {
		if (hashtable_purge(table)<0)
		{
		    fprintf(stderr,"%s: unable to purge hashtable\n",argv[0]);
		    abort();
		}
		printf("length = %u\n",hashtable_get_length(table));
#ifdef USE_CHECK_ALLOC
		check_cleanup();
#endif
		return STATUS_SUCCESS;
	    }
	}
	
	if (table && hashtable_check(table)<0)
	{
	    fprintf(stderr,"%s: hashtable did not check, count=%u\n",argv[0],count);
	    abort();
	}
    }
}

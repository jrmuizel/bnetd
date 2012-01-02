/*
 * Copyright (C) 2000  Gediminas (gugini@fortas.ktu.lt)
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
#define IPBAN_INTERNAL_ACCESS
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
#include "compat/strchr.h"
#include "compat/strdup.h"
#include <errno.h>
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "ipban.h"
#include "common/setup_after.h"


static t_ipban_entry * base=NULL;


extern int ipban_check(char const * ipaddr)
{
    char *          whole;
    char const *    ip1;
    char const *    ip2;
    char const *    ip3;
    char const *    ip4;
    t_ipban_entry * temp;
    
    if (!ipaddr)
    {
	eventlog(eventlog_level_warn,"ipban_check","got NULL ipaddr");
	return -1;
    }
    
    if (!(whole = strdup(ipaddr)))
    {
	eventlog(eventlog_level_warn,"ipban_check","could not allocate memory to check ip against wildcard");
	return -1;
    }
    ip1 = strtok(whole,".");
    ip2 = strtok(NULL,".");
    ip3 = strtok(NULL,".");
    ip4 = strtok(NULL,".");
    
    if (!ip1 || !ip2 || !ip3 || !ip4)
    {
	eventlog(eventlog_level_warn,"ipban_check","got bad IP address \"%s\"",ipaddr);
	free(whole);
	return -1;
    }
    
    eventlog(eventlog_level_debug,"ipban_check","checking %s.%s.%s.%s",ip1,ip2,ip3,ip4);
    
    for (temp=base; temp; temp=temp->next)
	switch (temp->type)
	{
	case ipban_type_exact:
	    if (strcmp(temp->info1,ipaddr)==0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s matched exact %s",ipaddr,temp->info1);
		free(whole);
		return 1;
	    }
	    eventlog(eventlog_level_debug,"ipban_check","address %s does not match exact %s",ipaddr,temp->info1);
	    continue;
	    
	case ipban_type_wildcard:
	    if (strcmp(temp->info1,"*")!=0 && strcmp(ip1,temp->info1)!=0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s does not match part 1 of wildcard %s.%s.%s.%s",ipaddr,temp->info1,temp->info2,temp->info3,temp->info4);
		continue;
	    }
	    if (strcmp(temp->info2,"*")!=0 && strcmp(ip2,temp->info2)!=0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s does not match part 2 of wildcard %s.%s.%s.%s",ipaddr,temp->info1,temp->info2,temp->info3,temp->info4);
		continue;
	    }
	    if (strcmp(temp->info3,"*")!=0 && strcmp(ip3,temp->info3)!=0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s does not match part 3 of wildcard %s.%s.%s.%s",ipaddr,temp->info1,temp->info2,temp->info3,temp->info4);
		continue;
	    }
	    if (strcmp(temp->info4,"*")!=0 && strcmp(ip4,temp->info4)!=0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s does not match part 4 of wildcard %s.%s.%s.%s",ipaddr,temp->info1,temp->info2,temp->info3,temp->info4);
		continue;
	    }
	    
	    eventlog(eventlog_level_debug,"ipban_check","address %s matched wildcard %s.%s.%s.%s",ipaddr,temp->info1,temp->info2,temp->info3,temp->info4);
	    free(whole);
	    return 1;
	    
	case ipban_type_range:
	    /* FIXME: this assumes constant length, ie.
	     *  128.123.99.09 < 128.123.099.014
	     *  128.123.99.9  > 128.123.99.14
	     */
	    if (strcmp(ipaddr,temp->info1)>=0 && strcmp(ipaddr,temp->info2)<=0)
	    {
		eventlog(eventlog_level_debug,"ipban_check","address %s matched range %s-%s",ipaddr,temp->info1,temp->info2);
		free(whole);
		return 1;
	    }
	    eventlog(eventlog_level_debug,"ipban_check","address %s does not match range %s-%s",ipaddr,temp->info1,temp->info2);
	    continue;
	    
	default:  /* unknown type */
	    eventlog(eventlog_level_warn,"ipban_check","found bad ban type %d",(int)temp->type);
	}
    
    free(whole);
    return 0;
}


extern int ipbanlist_load(char const * filename)
{
    FILE *          fp;
    char *          buff;
    char *          cp;
    char *          matched;
    char *          whole;
    unsigned int    currline;
    t_ipban_entry * temp;
    
    base = NULL;
    
    if (!filename)
    {
        eventlog(eventlog_level_error,"ipbanlist_load","got NULL filename");
	return -1;
    }
    
    if (!(fp = fopen(filename,"r")))
    {
        eventlog(eventlog_level_error,"ipbanlist_load","could not open banlist file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	return -1;
    }
    
    for (currline=1; (buff = file_get_line(fp)); currline++)
    {
	cp = buff;
	
	/* eat whitespace in front */
	while (*cp=='\t' || *cp==' ') cp++;
	if (*cp=='\0' || *cp=='#')
	{
	    free(buff);
	    continue;
	}
	
	/* eat whitespace in back */
	while (strlen(cp)>0 && (cp[strlen(cp)-1]==' ' || cp[strlen(cp)-1]=='\t'))
	    cp[strlen(cp)-1] = '\0';
	
	if (!(temp = malloc(sizeof(t_ipban_entry))))
	{
	    eventlog(eventlog_level_error,"ipbanlist_load","could not allocate memory for entry on line %u",currline);
	    free(buff);
	    continue;
	}
	
	if ((matched = strchr(cp,'-'))) /* range */
	{
	    temp->type = ipban_type_range;
	    
	    matched[0] = '\0';
	    if (!(temp->info1 = strdup(cp))) /* start of range */
	    {
		eventlog(eventlog_level_error,"ipbanlist_load","could not allocate memory for info on entry on line %u",currline);
		free(temp);
		continue;
	    }
	    if (!(temp->info2 = strdup(&matched[1]))) /* end of range */
	    {
		eventlog(eventlog_level_error,"ipbanlist_load","could not allocate memory for info on entry on line %u",currline);
		free(temp->info1);
		free(temp);
		continue;
	    }
	    temp->info3 = NULL;
	    temp->info4 = NULL;
	}
	else
	    if (strchr(cp,'*')) /* wildcard */
	    {
		temp->type = ipban_type_wildcard;
		
		/* only free() info1! */
		if (!(whole = strdup(cp)))
		{
		    eventlog(eventlog_level_error,"ipbanlist_load","could not allocate memory for info on entry on line %u",currline);
		    free(temp);
		    continue;
		}
		temp->info1 = strtok(whole,".");
		temp->info2 = strtok(NULL,".");
		temp->info3 = strtok(NULL,".");
		temp->info4 = strtok(NULL,".");
	    }
	    else /* exact */
	    {
		temp->type = ipban_type_exact;
		
		if (!(temp->info1 = strdup(cp)))
		{
		    eventlog(eventlog_level_error,"ipbanlist_load","could not allocate memory for info on entry on line %u",currline);
		    free(temp);
		    continue;
		}
		temp->info2 = NULL;
		temp->info3 = NULL;
		temp->info4 = NULL;
	    }
	
	/* insert */
	temp->next = base;
	base = temp;
	
	free(buff);
    }
    
    if (fclose(fp)<0)
        eventlog(eventlog_level_error,"ipbanlist_load","could not close banlist file \"%s\" after reading (fclose: %s)",filename,strerror(errno));
    
    return 0;
}


extern int ipbanlist_unload(void)
{
    t_ipban_entry * temp;
    t_ipban_entry * next;
    
    for (temp=base; temp; temp=next)
    {
	next = temp->next;
	
	switch (temp->type)
	{
	case ipban_type_exact:
	    if (temp->info1)
		free(temp->info1);
	    break;
	case ipban_type_wildcard:
	    if (temp->info1)
		free(temp->info1);
	    break;
	case ipban_type_range:
	    if (temp->info1)
		free(temp->info1);
	    if (temp->info2)
		free(temp->info2);
	    break;
	default:  /* unknown type */
	    eventlog(eventlog_level_warn,"ipbanlist_unload","found bad ban type %d",(int)temp->type);
	}
	free(temp);
    }
    
    base = NULL;
    return 0;
}

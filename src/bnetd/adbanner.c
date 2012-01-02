/*
 * Copyright (C) 1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#define ADBANNER_INTERNAL_ACCESS
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
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/util.h"
#include "connection.h"
#include "adbanner.h"
#include "common/setup_after.h"


static t_list * adbannerlist_init_head=NULL;
static t_list * adbannerlist_start_head=NULL;
static t_list * adbannerlist_norm_head=NULL;
static unsigned int adbannerlist_init_count=0;
static unsigned int adbannerlist_start_count=0;
static unsigned int adbannerlist_norm_count=0;


static t_adbanner * adbanner_create(unsigned int id, unsigned int next_id, unsigned int delay, bn_int tag, char const * filename, char const * link);
static int adbanner_destroy(t_adbanner const * ad);
static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id);
static t_adbanner * adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id);


static t_adbanner * adbanner_create(unsigned int id, unsigned int next_id, unsigned int delay, bn_int tag, char const * filename, char const * link)
{
    t_adbanner * ad;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,"adbanner_create","got NULL filename");
	return NULL;
    }
    if (!link)
    {
	eventlog(eventlog_level_error,"adbanner_create","got NULL link");
	return NULL;
    }
    
    if (!(ad = malloc(sizeof(t_adbanner))))
    {
	eventlog(eventlog_level_error,"adbanner_create","could not allocate memory for ad");
	return NULL;
    }
    
    ad->id           = id;
    ad->extensiontag = bn_int_get(tag);
    ad->delay        = delay;
    ad->next         = next_id;
    if (!(ad->filename = strdup(filename)))
    {
	eventlog(eventlog_level_error,"adbanner_create","could not allocate memory for filename");
	free(ad);
	return NULL;
    }
    if (!(ad->link = strdup(link)))
    {
	eventlog(eventlog_level_error,"adbanner_create","could not allocate memory for link");
	free((void *)ad->filename); /* avoid warning */
	free(ad);
	return NULL;
    }
    
    eventlog(eventlog_level_debug,"adbanner_create","created ad id=0x%08x filename=\"%s\" extensiontag=0x%04x delay=%u link=\"%s\" next_id=0x%08x",ad->id,ad->filename,ad->extensiontag,ad->delay,ad->link,ad->next);
    return ad;
}


static int adbanner_destroy(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,"adbanner_destroy","got NULL ad");
	return -1;
    }
    
    free((void *)ad->filename); /* avoid warning */
    free((void *)ad->link); /* avoid warning */
    free((void *)ad); /* avoid warning */
    
    return 0;
}


extern t_adbanner * adbanner_pick(t_connection const * c, unsigned int prev_id)
{
    t_adbanner const * prev;
    unsigned int       next_id;
    
    if (!c)
    {
	eventlog(eventlog_level_error,"adbanner_pick","got NULL connection");
	return NULL;
    }
    
    /* eventlog(eventlog_level_debug,"adbanner_choose","prev_id=%u init_count=%u start_count=%u norm_count=%u",prev_id,adbannerlist_init_count,adbannerlist_start_count,adbannerlist_norm_count); */
    /* if this is the first ad, randomly choose an init sequence (if there is one) */
    if (prev_id==0 && adbannerlist_init_count>0)
        return list_get_data_by_pos(adbannerlist_init_head,rand()%adbannerlist_init_count);
    /* eventlog(eventlog_level_debug,"adbanner_choose","not sending init banner"); */
    
    /* find the previous adbanner */
    if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,prev_id)))
	next_id = prev->next;
    else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,prev_id)))
	next_id = prev->next;
    else if ((prev = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,prev_id)))
	next_id = prev->next;
    else
	next_id = 0;
    
    /* return its next ad if there is one */
    if (next_id)
    {
	t_adbanner * curr;
	
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_init_head,next_id)))
	    return curr;
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_start_head,next_id)))
	    return curr;
	if ((curr = adbannerlist_find_adbanner_by_id(adbannerlist_norm_head,next_id)))
	    return curr;
	
	eventlog(eventlog_level_error,"adbanner_pick","could not locate next requested ad with id 0x%06x",next_id);
    }
    /* eventlog(eventlog_level_debug,"adbanner_choose","not sending next banner"); */
    
    /* otherwise choose another starting point randomly */
    if (adbannerlist_start_count>0)
	return list_get_data_by_pos(adbannerlist_start_head,rand()%adbannerlist_start_count);
    
    /* eventlog(eventlog_level_debug,"adbanner_choose","not sending start banner... nothing to return"); */
    return NULL; /* nothing else to return */
}


extern unsigned int adbanner_get_id(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,"adbanner_get_id","got NULL ad");
	return 0;
    }
    return ad->id;
}


extern unsigned int adbanner_get_extensiontag(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,"adbanner_get_extensiontag","got NULL ad");
	return 0;
    }
    return ad->extensiontag;
}


extern char const * adbanner_get_filename(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,"adbanner_get_filename","got NULL ad");
	return NULL;
    }
    return ad->filename;
}


extern char const * adbanner_get_link(t_adbanner const * ad)
{
    if (!ad)
    {
	eventlog(eventlog_level_error,"adbanner_get_link","got NULL ad");
	return NULL;
    }
    return ad->link;
}


static t_adbanner * adbannerlist_find_adbanner_by_id(t_list const * head, unsigned int id)
{
    t_elem const * curr;
    t_adbanner *   temp;
    
    if (!head)
	return NULL;
    
    LIST_TRAVERSE_CONST(head,curr)
    {
        if (!(temp = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error,"adbannerlist_find_adbanner_by_id","found NULL adbanner in list");
	    continue;
	}
	if (temp->id==id)
	    return temp;
    }
    
    return NULL;
}


static int adbannerlist_insert(t_list * head, unsigned int * count, char const * filename, unsigned int delay, char const * link, unsigned int next_id)
{
    t_adbanner * ad;
    unsigned int id;
    char *       ext;
    bn_int       bntag;
    
    if (!head)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got NULL head");
	return -1;
    }
    if (!count)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got NULL count");
	return -1;
    }
    if (!filename)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got NULL filename");
	return -1;
    }
    if (!link)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got NULL link");
	return -1;
    }
    if (strlen(filename)<7)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got bad ad filename \"%s\"",filename);
	return -1;
    }
    
    if (!(ext = malloc(strlen(filename))))
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","could not allocate memory for ext");
	return -1;
    }
    
    if (sscanf(filename,"%*c%*c%x.%s",&id,ext)!=2)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","got bad ad filename \"%s\"",filename);
	free(ext);
	return -1;
    }
    
    if (strcasecmp(ext,"pcx")==0)
        bn_int_tag_set(&bntag,EXTENSIONTAG_PCX);
    else if (strcasecmp(ext,"smk")==0)
        bn_int_tag_set(&bntag,EXTENSIONTAG_SMK);
    else
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","unknown extension on filename \"%s\"",filename);
	free(ext);
	return -1;
    }
    free(ext);
    
    if (!(ad = adbanner_create(id,next_id,delay,bntag,filename,link)))
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","could not create ad");
	return -1;
    }
    
    if (list_prepend_data(head,ad)<0)
    {
	eventlog(eventlog_level_error,"adbannerlist_insert","could not insert ad");
	adbanner_destroy(ad);
	return -1;
    }
    (*count)++;
    
    return 0;
}


extern int adbannerlist_create(char const * filename)
{
    FILE *          fp;
    unsigned int    line;
    unsigned int    pos;
    unsigned int    len;
    char *          buff;
    char *          name;
    char *          when;
    char *          link;
    char *          temp;
    unsigned int    delay;
    unsigned int    next_id;
    
    if (!filename)
    {
        eventlog(eventlog_level_error,"adbannerlist_create","got NULL filename");
        return -1;
    }
    
    if (!(adbannerlist_init_head = list_create()))
    {
        eventlog(eventlog_level_error,"adbannerlist_create","could not create init list");
        return -1;
    }
    if (!(adbannerlist_start_head = list_create()))
    {
        eventlog(eventlog_level_error,"adbannerlist_create","could not create init list");
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head = NULL;
        return -1;
    }
    if (!(adbannerlist_norm_head = list_create()))
    {
        eventlog(eventlog_level_error,"adbannerlist_create","could not create init list");
	list_destroy(adbannerlist_start_head);
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head=adbannerlist_start_head = NULL;
        return -1;
    }
    
    if (!(fp = fopen(filename,"r")))
    {
        eventlog(eventlog_level_error,"adbannerlist_create","could not open adbanner file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	list_destroy(adbannerlist_norm_head);
	list_destroy(adbannerlist_start_head);
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head=adbannerlist_start_head=adbannerlist_norm_head = NULL;
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
	    unsigned int endpos;
	    
            *temp = '\0';
	    len = strlen(buff)+1;
            for (endpos=len-1;  buff[endpos]=='\t' || buff[endpos]==' '; endpos--);
            buff[endpos+1] = '\0';
        }
        len = strlen(buff)+1;
	
        if (!(name = malloc(len)))
        {
            eventlog(eventlog_level_error,"adbannerlist_create","could not allocate memory for name");
            free(buff);
            continue;
        }
        if (!(when = malloc(len)))
        {
            eventlog(eventlog_level_error,"adbannerlist_create","could not allocate memory for name");
            free(name);
            free(buff);
            continue;
        }
        if (!(link = malloc(len)))
        {
            eventlog(eventlog_level_error,"adbannerlist_create","could not allocate memory for link");
            free(name);
            free(when);
            free(buff);
            continue;
        }
	
	if (sscanf(buff," \"%[^\"]\" %[a-z] %u \"%[^\"]\" %x",name,when,&delay,link,&next_id)!=5)
	    if (sscanf(buff," \"%[^\"]\" %[a-z] %u \"\" %x",name,when,&delay,&next_id)==4)
		link[0] = '\0';
	    else
	    {
		eventlog(eventlog_level_error,"adbannerlist_create","malformed line %u in file \"%s\"",line,filename);
		free(link);
		free(name);
         	free(when);
		free(buff);
		continue;
	    }
	
	if (strcmp(when,"init")==0)
	    adbannerlist_insert(adbannerlist_init_head,&adbannerlist_init_count,name,delay,link,next_id);
	else if (strcmp(when,"start")==0)
	    adbannerlist_insert(adbannerlist_start_head,&adbannerlist_start_count,name,delay,link,next_id);
	else if (strcmp(when,"norm")==0)
	    adbannerlist_insert(adbannerlist_norm_head,&adbannerlist_norm_count,name,delay,link,next_id);
	else
	{
	    eventlog(eventlog_level_error,"adbannerlist_create","when field has unknown value on line %u in file \"%s\"",line,filename);
	    free(link);
	    free(name);
            free(when);
	    free(buff);
	    continue;
	}
	
	free(link);
	free(name);
        free(when);
	free(buff);
    }
    
    if (fclose(fp)<0)
	eventlog(eventlog_level_error,"adbannerlist_create","could not close adbanner file \"%s\" after reading (fclose: %s)",filename,strerror(errno));
    return 0;
}


extern int adbannerlist_destroy(void)
{
    t_elem *     curr;
    t_adbanner * ad;
    
    if (adbannerlist_init_head)
    {
	LIST_TRAVERSE(adbannerlist_init_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,"adbannerlist_destroy","found NULL adbanner in init list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_init_head,curr);
	}
	list_destroy(adbannerlist_init_head);
	adbannerlist_init_head = NULL;
	adbannerlist_init_count = 0;
    }
    
    if (adbannerlist_start_head)
    {
	LIST_TRAVERSE(adbannerlist_start_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,"adbannerlist_destroy","found NULL adbanner in start list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_start_head,curr);
	}
	list_destroy(adbannerlist_start_head);
	adbannerlist_start_head = NULL;
	adbannerlist_start_count = 0;
    }
    
    if (adbannerlist_norm_head)
    {
	LIST_TRAVERSE(adbannerlist_norm_head,curr)
	{
	    if (!(ad = elem_get_data(curr)))
		eventlog(eventlog_level_error,"adbannerlist_destroy","found NULL adbanner in norm list");
	    else
		adbanner_destroy(ad);
	    list_remove_elem(adbannerlist_norm_head,curr);
	}
	list_destroy(adbannerlist_norm_head);
	adbannerlist_norm_head = NULL;
	adbannerlist_norm_count = 0;
    }
    
    return 0;
}

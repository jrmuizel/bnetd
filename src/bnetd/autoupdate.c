/*
 * Copyright (C) 2000 Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 2002 Gianluigi Tiesi (sherpya@netfarm.it)
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
#define AUTOUPDATE_INTERNAL_ACCESS
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
#include "compat/strtoul.h"
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
#include "common/list.h"
#include "common/util.h"
#include "common/proginfo.h"
#include "autoupdate.h"
#include "common/setup_after.h"


static t_list * autoupdate_head=NULL;


/* Open the autoupdate configuration file, create a linked list of the
 * clienttag and the update file for it.  The format of the file is:
 * clienttag<tab>update file<tab>min_version<tab>max_version
 *
 * Comments begin with # and are ignored.
 *
 * The server assumes that the update file is in the "files" directory
 * so do not include "/" in the filename, it won't be sent
 * (a security risk).
 */
extern int autoupdate_load(char const * filename)
{
    FILE *         fp;
    unsigned int   line;
    unsigned int   pos;
    char *         buff;
    char *         temp;
    char const *   archtag;
    char const *   clienttag;
    char const *   mpqfile;
    char const *   version;
    char const *   version_min;
    char const *   version_max;
    char const *   versiontag;
    t_autoupdate * entry;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,"autoupdate_load","got NULL filename");
	return -1;
    }
    
    if (!(autoupdate_head = list_create()))
    {
	eventlog(eventlog_level_error,"autoupdate_load","could create list");
	return -1;
    }
    if (!(fp = fopen(filename,"r")))
    {
	eventlog(eventlog_level_error,"autoupdate_load","could not open file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	list_destroy(autoupdate_head);
	autoupdate_head = NULL;
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
	    for (endpos=len-1;  buff[endpos]=='\t' || buff[endpos]==' '; endpos--);
	    buff[endpos+1] = '\0';
	}
	
	if (!(archtag = strtok(buff, " \t"))) /* strtok modifies the string it is passed */
	{
	    eventlog(eventlog_level_error,"autoupdate_load","missing archtag on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(clienttag = strtok(NULL," \t"))) 
	{
	    eventlog(eventlog_level_error,"autoupdate_load","missing clienttag on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(mpqfile = strtok(NULL," \t")))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","missing mpqfile on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(version = strtok(NULL," \t")))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","missing upgrade version on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(version_min = strtok(NULL," \t")))
	    version_min = "0.0.0.0";
	if (!(version_max = strtok(NULL," \t")))
	    version_max = version;
        
	if (!(versiontag = strtok(NULL, " \t")))
	  versiontag = NULL;
	
	if (!(entry = malloc(sizeof(t_autoupdate))))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","could not allocate memory for entry");
	    free(buff);
	    continue;
	}
	
	if (!(entry->archtag = strdup(archtag)))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","could not allocate memory for archtag");
	    free(entry);
	    free(buff);
	    continue;
	}
	
	if (!(entry->clienttag = strdup(clienttag)))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","could not allocate memory for clienttag");
	    free(entry);
	    free(buff);
	    continue;
	}

	if (!(entry->mpqfile = strdup(mpqfile)))
	{
	    eventlog(eventlog_level_error,"autoupdate_load","could not allocate memory for mpqfile");
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	}

	if (versiontag)
	{
	  if (!(entry->versiontag = strdup(versiontag)))
	  {
	    eventlog(eventlog_level_error,"autoupdate_load","malformed versiontag line %u of file \"%s\"",line,filename);
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	  }
	}
	else
	  entry->versiontag = NULL;
	    
	
	if (verstr_to_vernum(version,&entry->version)<0)
	{
	    eventlog(eventlog_level_error,"autoupdate_load","malformed upgrade version on line %u of file \"%s\"",line,filename);
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	}
	if (verstr_to_vernum(version_min,&entry->version_min)<0)
	{
	    eventlog(eventlog_level_error,"autoupdate_load","malformed min version on line %u of file \"%s\"",line,filename);
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	}
	if (verstr_to_vernum(version_max,&entry->version_max)<0)
	{
	    eventlog(eventlog_level_error,"autoupdate_load","malformed max version on line %u of file \"%s\"",line,filename);
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	}
	if (entry->version_min>entry->version_max)
	{
	    eventlog(eventlog_level_error,"autoupdate_load","min version greater than max version on line %u of file \"%s\"",line,filename);
	    free((void *)entry->clienttag); /* avoid warning */
	    free(entry);
	    free(buff);
	    continue;
	}

	eventlog(eventlog_level_debug,"autoupdate_load","update %s with file %s",clienttag,entry->mpqfile);
	eventlog(eventlog_level_debug,"autoupdate_load"," min v%s(0x%08lx)",vernum_to_verstr(entry->version_min),entry->version_min);
	eventlog(eventlog_level_debug,"autoupdate_load"," max v%s(0x%08lx)",vernum_to_verstr(entry->version_max),entry->version_max);
	eventlog(eventlog_level_debug,"autoupdate_load"," new v%s(0x%08lx)",vernum_to_verstr(entry->version),entry->version);
	eventlog(eventlog_level_debug,"autoupdate_load"," versiontag %s",entry->versiontag);
	
	free(buff);
	
	if (list_append_data(autoupdate_head,entry)<0)
	{
	    eventlog(eventlog_level_error,"autoupdate_load","could not append item");
	    free((void *)entry->mpqfile); /* avoid warning */
	    free((void *)entry->clienttag); /* avoid warning */
	    if (entry->versiontag)
	      free((void *)entry->versiontag);
	    free(entry);
	    continue;
	}
    }
    
    return 0;
}


/* Free up all of the elements in the linked list */
extern int autoupdate_unload(void)
{
    t_elem *       curr;
    t_autoupdate * entry;
    
    if (autoupdate_head)
    {    
	LIST_TRAVERSE(autoupdate_head,curr)
	{
	    if (!(entry = elem_get_data(curr)))
		eventlog(eventlog_level_error,"autoupdate_unload","found NULL entry in list");
	    else
	    {
	      free((void *)entry->archtag); /* avoid warning */
	      free((void *)entry->clienttag); /* avoid warning */
	      free((void *)entry->mpqfile); /* avoid warning */
	      if (entry->versiontag)
		free((void *)entry->versiontag);
	      free(entry);
	    }
	    list_remove_elem(autoupdate_head,curr);
	}
	
	if (list_destroy(autoupdate_head)<0)
	    return -1;
	autoupdate_head = NULL;
    }
    
    return 0;
}


/* Look to see if there is an update file for a particular clienttag.  Return
 * NULL if one is not found.
 */
extern char const * autoupdate_file(char const * archtag, char const * clienttag, char const * new_version, char const * versiontag)
{
    t_elem const * curr;
    t_autoupdate * entry;
    unsigned long  version;
    
    if (autoupdate_head)
    {
        if (verstr_to_vernum(new_version,&version)<0)
	    return NULL;
	LIST_TRAVERSE_CONST(autoupdate_head,curr)
	{
	    if (!(entry = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error,"autoupdate_file","found NULL entry in list");
		continue;
	    }
	    
	    if (strcmp(entry->archtag, archtag) != 0)
	      continue;
	    if (strcmp(entry->clienttag, clienttag) !=0)
	      continue;
	    
	    /* if versiontag is matched only version >= version_min */
	    if (versiontag)
	    {
	      if ((version >= entry->version_min) && (entry->versiontag) &&
		  (strcmp(entry->versiontag,versiontag) == 0))
		return entry->mpqfile;
	      else
		continue;
	    }

	    if (version >= entry->version_min && version < entry->version_max)
	      return entry->mpqfile;
	    else
	      return NULL;
	    	      
	}
    }
    
    return NULL;
}


/* Look to see if there is an update version for a particular clienttag.
 * Return NULL if one is not found.
 */
extern char const * autoupdate_version(char const * archtag, char const * clienttag, char const * curr_version, char const * versiontag)
{
    t_elem const * curr;
    t_autoupdate * entry;
    unsigned long  version;
    
    if (autoupdate_head)
    {
        if (verstr_to_vernum(curr_version,&version)<0)
	    return NULL;
	LIST_TRAVERSE_CONST(autoupdate_head,curr)
	{
	    if (!(entry = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error,"autoupdate_version","found NULL entry in list");
		continue;
	    }

	    if (strcmp(entry->archtag, archtag) != 0)
	      continue;
	    if (strcmp(entry->clienttag, clienttag) !=0)
	      continue;

	    /* if versiontag is matched only version >= version_min */
	    if (versiontag)
	    {
	      if ((version >= entry->version_min) && (entry->versiontag) &&
		  (strcmp(entry->versiontag,versiontag) == 0))
		return vernum_to_verstr(entry->version);
	      else
		continue;
	    }

	    if (version >= entry->version_min && version < entry->version_max)
	      return vernum_to_verstr(entry->version);
	    else
	      return NULL;

	}
    }

    return NULL;
}

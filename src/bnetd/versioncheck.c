/*
 * Copyright (C) 2000 Onlyer (onlyer@263.net)
 * Copyright (C) 2001 Ross Combs (ross@bnetd.org)
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
#define VERSIONCHECK_INTERNAL_ACCESS
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
#include "common/token.h"
#include "versioncheck.h"
#include "common/setup_after.h"


static t_list * versioninfo_head=NULL;
static t_versioncheck dummyvc={ "A=3845581634 B=880823580 C=1363937103 4 A=A-S B=B-C C=C-A A=A-B", "IX86ver1.mpq" };


extern t_versioncheck * versioncheck_create(char const * archtag, char const * clienttag)
{
    t_elem const *   curr;
    t_versioninfo *  vi;
    t_versioncheck * vc;
    
    LIST_TRAVERSE_CONST(versioninfo_head,curr)
    {
        if (!(vi = elem_get_data(curr))) /* should not happen */
        {
            eventlog(eventlog_level_error,"versioncheck_create","version list contains NULL item");
            continue;
        }
	
	eventlog(eventlog_level_debug,"versioncheck_create","version check entry archtag=%s, clienttag=%s",vi->archtag,vi->clienttag);
	if (strcmp(vi->archtag,archtag)!=0)
	    continue;
	if (strcmp(vi->clienttag,clienttag)!=0)
	    continue;
	
	/* FIXME: randomize the selection */
	if (!(vc = malloc(sizeof(t_versioncheck))))
	{
	    eventlog(eventlog_level_error,"versioncheck_create","unable to allocate memory for vc");
	    return &dummyvc;
	}
	if (!(vc->eqn = strdup(vi->eqn)))
	{
	    eventlog(eventlog_level_error,"versioncheck_create","unable to allocate memory for eqn");
	    free(vc);
	    return &dummyvc;
	}
	if (!(vc->mpqfile = strdup(vi->mpqfile)))
	{
	    eventlog(eventlog_level_error,"versioncheck_create","unable to allocate memory for mpqfile");
	    free((void *)vc->eqn); /* avoid warning */
	    free(vc);
	    return &dummyvc;
	}
	
	return vc;
    }

    return &dummyvc;
}


extern int versioncheck_destroy(t_versioncheck * vc)
{
    if (!vc)
    {
	eventlog(eventlog_level_error,"versioncheck_destroy","got NULL vc");
	return -1;
    }
    
    if (vc==&dummyvc)
	return 0;
    
    free((void *)vc->mpqfile);
    free((void *)vc->eqn);
    free(vc);
    
    return 0;
}


extern char const * versioncheck_get_mpqfile(t_versioncheck const * vc)
{
    if (!vc)
    {
	eventlog(eventlog_level_error,"versioncheck_get_mpqfile","got NULL vc");
	return NULL;
    }
    
    return vc->mpqfile;
}


extern char const * versioncheck_get_eqn(t_versioncheck const * vc)
{
    if (!vc)
    {
	eventlog(eventlog_level_error,"versioncheck_get_mpqfile","got NULL vc");
	return NULL;
    }
    
    return vc->eqn;
}


extern int versioncheck_validate(t_versioncheck const * vc, char const * archtag, char const * clienttag, char const * exeinfo, unsigned long versionid, unsigned long gameversion, unsigned long checksum, char const ** versiontag)
{
    t_elem const *   curr;
    t_versioninfo *  vi;
    unsigned int bad = 0;

    *versiontag = NULL;

    if (!vc)
    {
	eventlog(eventlog_level_error,"versioncheck_validate","got NULL vc");
	return -1;
    }
    
    if (vc==&dummyvc)
	return 0;


    LIST_TRAVERSE_CONST(versioninfo_head,curr)
    {
        if (!(vi = elem_get_data(curr))) /* should not happen */
        {
	  eventlog(eventlog_level_error,"versioncheck_validate","version list contains NULL item");
	  continue;
        }

	if (strcmp(vi->eqn,vc->eqn)!=0)
	  continue;
	if (strcmp(vi->mpqfile,vc->mpqfile)!=0)
	  continue;
	if (strcmp(vi->archtag,archtag)!=0)
	  continue;
	if (strcmp(vi->clienttag,clienttag)!=0)
	  continue;

	if (vi->versionid != versionid)
	  continue;
	
	if ((vi->gameversion) && (vi->gameversion != gameversion))
	  continue;

	if (vi->exeinfo)
	{
	  if (strcmp(vi->exeinfo,exeinfo) == 0)
	    /* exeinfo matches */
	    bad = 0;
	  else
	    /* exeinfo mismatches */
	    bad = 1;
	}

	/* Checksum and exeinfo disabled, entry matches */
	if ((!vi->checksum) && !bad)
	{
	  if (vi->versiontag)
	    *versiontag = vi->versiontag;
	  return 1;
	}

	if (vi->checksum == checksum)
	{
	  /* ok version and checksum matches */
	  if (vi->versiontag)
	    *versiontag = vi->versiontag;
	  return 1;
	}
	else
	{
	  /*
	    Found the first match with same version bad different checksum
	    we need to rember this because if no other matching versions are found
	    we will return badversion
	  */
	  bad = 1;
	  if (vi->versiontag)
	    *versiontag = vi->versiontag;
	  continue;
	}
    }
    
    if (bad)
      /* a matching version is found but no matching exeinfo/checksum */
      return -1;
    else
      /* No match in list */
    return 0;
}


extern int versioncheck_load(char const * filename)
{
    FILE *	    fp;
    unsigned int    line;
    unsigned int    pos;
    char *	    buff;
    char *	    temp;
    char const *    eqn;
    char const *    mpqfile;
    char const *    archtag;
    char const *    clienttag;
    char const *    exeinfo;
    char const *    versionid;
    char const *    gameversion;
    char const *    checksum;
    char const *    versiontag;

    t_versioninfo * vi;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,"versioncheck_load","got NULL filename");
	return -1;
    }
    
    if (!(versioninfo_head = list_create()))
    {
	eventlog(eventlog_level_error,"versioncheck_load","could create list");
	return -1;
    }
    if (!(fp = fopen(filename,"r")))
    {
	eventlog(eventlog_level_error,"versioncheck_load","could not open file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	list_destroy(versioninfo_head);
	versioninfo_head = NULL;
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

	if (!(eqn = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing eqn on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(mpqfile = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing mpqfile on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(archtag = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing archtag on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(clienttag = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing clienttag on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(exeinfo = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing exeinfo on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(versionid = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing versionid on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(gameversion = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing gameversion on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(checksum = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","missing checksum on line %u of file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(versiontag = next_token(buff,&pos)))
	{
	    versiontag = NULL;
	}
	if (!(vi = malloc(sizeof(t_versioninfo))))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for vi");
	    free(buff);
	    continue;
	}
	if (!(vi->eqn = strdup(eqn)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for eqn");
	    free(vi);
	    free(buff);
	    continue;
	}
	if (!(vi->mpqfile = strdup(mpqfile)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for mpqfile");
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	}
	if (strlen(archtag)!=4)
	{
	    eventlog(eventlog_level_error,"versioncheck_load","invalid arch tag on line %u of file \"%s\"",line,filename);
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	}
	if (!(vi->archtag = strdup(archtag)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for archtag");
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	}
	if (strlen(clienttag)!=4)
	{
	    eventlog(eventlog_level_error,"versioncheck_load","invalid client tag on line %u of file \"%s\"",line,filename);
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	}
	if (!(vi->clienttag = strdup(clienttag)))
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for clienttag");
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	}
	if (strcmp(exeinfo, "NULL") == 0)
	  vi->exeinfo = NULL;
	else
	{
	  if (!(vi->exeinfo = strdup(exeinfo)))
	  {
	    eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for exeinfo");
	    free((void *)vi->clienttag); /* avoid warning */
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
	  }
	}
	vi->versionid = strtoul(versionid,NULL,0);
	if (verstr_to_vernum(gameversion,&vi->gameversion)<0)
	{
	    eventlog(eventlog_level_error,"versioncheck_load","malformed version on line %u of file \"%s\"",line,filename);
	    free((void *)vi->exeinfo); /* avoid warning */
	    free((void *)vi->clienttag); /* avoid warning */
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    free(buff);
	    continue;
        }

	vi->checksum = strtoul(checksum,NULL,0);
	if (versiontag)
	{
	  if (!(vi->versiontag = strdup(versiontag)))
	    {
	      eventlog(eventlog_level_error,"versioncheck_load","could not allocate memory for versiontag");
	      free((void *)vi->exeinfo); /* avoid warning */
	      free((void *)vi->clienttag); /* avoid warning */
	      free((void *)vi->archtag); /* avoid warning */
	      free((void *)vi->mpqfile); /* avoid warning */
	      free((void *)vi->eqn); /* avoid warning */
	      free(vi);
	      free(buff);
	      continue;
	    }
	}
	else
	  vi->versiontag = NULL;

	free(buff);
	
	if (list_append_data(versioninfo_head,vi)<0)
	{
	    eventlog(eventlog_level_error,"versioncheck_load","could not append item");
	    if (vi->versiontag)
	      free((void *)vi->versiontag); /* avoid warning */
	    free((void *)vi->exeinfo); /* avoid warning */
	    free((void *)vi->clienttag); /* avoid warning */
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    free(vi);
	    continue;
	}
    }
    
    if (fclose(fp)<0)
	eventlog(eventlog_level_error,"versioncheck_load","could not close versioncheck file \"%s\" after reading (fclose: %s)",filename,strerror(errno));
    
    return 0;
}


extern int versioncheck_unload(void)
{
    t_elem *	    curr;
    t_versioninfo * vi;
    
    if (versioninfo_head)
    {
	LIST_TRAVERSE(versioninfo_head,curr)
	{
	    if (!(vi = elem_get_data(curr))) /* should not happen */
	    {
		eventlog(eventlog_level_error,"versioncheck_unload","version list contains NULL item");
		continue;
	    }
	    
	    if (list_remove_elem(versioninfo_head,curr)<0)
		eventlog(eventlog_level_error,"versioncheck_unload","could not remove item from list");

	    if (vi->exeinfo)
	      free((void *)vi->exeinfo); /* avoid warning */
	    free((void *)vi->clienttag); /* avoid warning */
	    free((void *)vi->archtag); /* avoid warning */
	    free((void *)vi->mpqfile); /* avoid warning */
	    free((void *)vi->eqn); /* avoid warning */
	    if (vi->versiontag)
	      free((void *)vi->versiontag); /* avoid warning */
	    free(vi);
	}
	
	if (list_destroy(versioninfo_head)<0)
	    return -1;
	versioninfo_head = NULL;
    }
    
    return 0;
}


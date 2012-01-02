/*
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999,2000  D.Moreaux (vapula@linuxbe.org)
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
#define LADDER_INTERNAL_ACCESS
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
#include "common/field_sizes.h"
#include "account.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "game.h"
#include "common/tag.h"
#include "common/list.h"
#include "common/bnettime.h"
#include "prefs.h"
#include "common/hashtable.h"
#include "ladder_calc.h"
#include "ladder.h"
#include "common/setup_after.h"


static t_list * ladderlist_head=NULL;

static char const * compare_clienttag=NULL; /* used in compare routines */
static t_ladder_id  compare_id; /* used in compare routines */


static int compare_current_highestrated(void const * a, void const * b);
static int compare_current_mostwins(void const * a, void const * b);
static int compare_current_mostgames(void const * a, void const * b);
static int compare_active_highestrated(void const * a, void const * b);
static int compare_active_mostwins(void const * a, void const * b);
static int compare_active_mostgames(void const * a, void const * b);

static int ladder_create(char const * clienttag, t_ladder_id id);
static int ladder_destroy(t_ladder const * ladder);
static int ladder_insert_account(t_ladder * ladder, t_account * account);
static int ladder_sort(t_ladder * ladder);
static int ladder_make_active(t_ladder const * ladder);

static t_ladder * ladderlist_find_ladder(t_ladder_time ltime, char const * clienttag, t_ladder_id id);



static int ladder_create(char const * clienttag, t_ladder_id id)
{
    t_ladder * current;
    t_ladder * active;
    
    if (!(current = malloc(sizeof(t_ladder))))
    {
        eventlog(eventlog_level_error,"ladder_create","could not allocate memory for current");
        return -1;
    }
    current->clienttag    = clienttag;
    current->id           = id;
    current->ltime        = ladder_time_current;
    current->highestrated = NULL;
    current->mostwins     = NULL;
    current->mostgames    = NULL;
    current->len          = 0;
    if (list_append_data(ladderlist_head,current)<0)
    {
        free(current);
        eventlog(eventlog_level_error,"ladder_create","could not append current");
        return -1;
    }
    
    if (!(active = malloc(sizeof(t_ladder))))
    {
        eventlog(eventlog_level_error,"ladder_create","could not allocate memory for active");
        return -1;
    }
    active->clienttag    = clienttag;
    active->id           = id;
    active->ltime        = ladder_time_active;
    active->highestrated = NULL;
    active->mostwins     = NULL;
    active->mostgames    = NULL;
    active->len          = 0;
    if (list_append_data(ladderlist_head,active)<0)
    {
        free(active);
        eventlog(eventlog_level_error,"ladder_create","could not append active");
        return -1;
    }
    
    return 0;
}


static int ladder_destroy(t_ladder const * ladder)
{
    if (!ladder)
    {
	eventlog(eventlog_level_error,"ladder_destroy","got NULL ladder");
	return -1;
    }
    
    if (list_remove_data(ladderlist_head,ladder)<0)
    {
        eventlog(eventlog_level_error,"ladder_destroy","could not remove item from list");
        return -1;
    }
    /* no need to list_purge ladder because ladder_destroy only called from ladderlist_destroy */
    
    if (ladder->highestrated)
	free(ladder->highestrated);
    if (ladder->mostwins)
	free(ladder->mostwins);
    if (ladder->mostgames)
	free(ladder->mostgames);
    free((void *)ladder); /* avoid warning */
    
    return 0;
}


/*
 * Insert an account into the proper ladder arrays. This routine
 * does _not_ do the sorting and ranking updates.
 */
static int ladder_insert_account(t_ladder * ladder, t_account * account)
{
    t_account * * temp_ladder_highestrated;
    t_account * * temp_ladder_mostwins;
    t_account * * temp_ladder_mostgames;
    
    if (!ladder)
    {
	eventlog(eventlog_level_error,"ladder_insert_account","got NULL ladder");
	return -1;
    }
    if (!account)
    {
	eventlog(eventlog_level_error,"ladder_insert_account","got NULL account");
	return -1;
    }
    
    if (ladder->highestrated) /* some realloc()s are broken, so this gets to be ugly */
    {
	if (!(temp_ladder_highestrated = realloc(ladder->highestrated,sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_highestrated");
	    return -1;
	}
    }
    else
	if (!(temp_ladder_highestrated = malloc(sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_highestrated");
	    return -1;
	}
    
    if (ladder->mostwins)
    {
	if (!(temp_ladder_mostwins = realloc(ladder->mostwins,sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_mostwins");
	    ladder->highestrated = temp_ladder_highestrated;
	    return -1;
	}
    }
    else
	if (!(temp_ladder_mostwins = malloc(sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_mostwins");
	    ladder->highestrated = temp_ladder_highestrated;
	    return -1;
	}
    
    if (ladder->mostgames)
    {
	if (!(temp_ladder_mostgames = realloc(ladder->mostgames,sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_mostgames");
	    ladder->mostwins     = temp_ladder_mostwins;
	    ladder->highestrated = temp_ladder_highestrated;
	    return -1;
	}
    }
    else
	if (!(temp_ladder_mostgames = malloc(sizeof(t_account *)*(ladder->len+1))))
	{
	    eventlog(eventlog_level_error,"ladder_insert","could not allocate memory for temp_ladder_mostgames");
	    ladder->mostwins     = temp_ladder_mostwins;
	    ladder->highestrated = temp_ladder_highestrated;
	    return -1;
	}
    
    temp_ladder_highestrated[ladder->len] = account;
    temp_ladder_mostwins[ladder->len]     = account;
    temp_ladder_mostgames[ladder->len]    = account;
    
    ladder->highestrated = temp_ladder_highestrated;
    ladder->mostwins     = temp_ladder_mostwins;
    ladder->mostgames    = temp_ladder_mostgames;
    
    ladder->len++;
    
    return 0;
}


static int compare_current_highestrated(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_current_highestrated","got NULL account");
	return 0;
    }
    
    xrating = account_get_ladder_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xwins = account_get_ladder_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xoldrank = account_get_ladder_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


static int compare_current_mostwins(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_current_mostwins","got NULL account");
	return 0;
    }
    
    xwins = account_get_ladder_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xrating = account_get_ladder_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xoldrank = account_get_ladder_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


static int compare_current_mostgames(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xgames;
    unsigned int ygames;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_current_mostgames","got NULL account");
	return 0;
    }
    
    xgames = account_get_ladder_wins(x,compare_clienttag,compare_id)+
	     account_get_ladder_losses(x,compare_clienttag,compare_id)+
	     account_get_ladder_draws(x,compare_clienttag,compare_id)+
	     account_get_ladder_disconnects(x,compare_clienttag,compare_id);
    ygames = account_get_ladder_wins(y,compare_clienttag,compare_id)+
	     account_get_ladder_losses(y,compare_clienttag,compare_id)+
	     account_get_ladder_draws(y,compare_clienttag,compare_id)+
	     account_get_ladder_disconnects(y,compare_clienttag,compare_id);
    if (xgames>ygames)
        return -1;
    if (xgames<ygames)
        return +1;
    
    xrating = account_get_ladder_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xwins = account_get_ladder_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xoldrank = account_get_ladder_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


static int compare_active_highestrated(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_active_highestrated","got NULL account");
	return 0;
    }
    
    xrating = account_get_ladder_active_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_active_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xwins = account_get_ladder_active_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_active_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xoldrank = account_get_ladder_active_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_active_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


static int compare_active_mostwins(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_active_mostwins","got NULL account");
	return 0;
    }
    
    xwins = account_get_ladder_active_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_active_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xrating = account_get_ladder_active_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_active_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xoldrank = account_get_ladder_active_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_active_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


static int compare_active_mostgames(void const * a, void const * b)
{
    t_account *  x=*(t_account * const *)a;
    t_account *  y=*(t_account * const *)b;
    unsigned int xgames;
    unsigned int ygames;
    unsigned int xrating;
    unsigned int yrating;
    unsigned int xwins;
    unsigned int ywins;
    unsigned int xoldrank;
    unsigned int yoldrank;
    
    if (!x || !y)
    {
	eventlog(eventlog_level_error,"compare_active_mostgames","got NULL account");
	return 0;
    }
    
    xgames = account_get_ladder_active_wins(x,compare_clienttag,compare_id)+
	     account_get_ladder_active_losses(x,compare_clienttag,compare_id)+
	     account_get_ladder_active_draws(x,compare_clienttag,compare_id)+
	     account_get_ladder_active_disconnects(x,compare_clienttag,compare_id);
    ygames = account_get_ladder_active_wins(y,compare_clienttag,compare_id)+
	     account_get_ladder_active_losses(y,compare_clienttag,compare_id)+
	     account_get_ladder_active_draws(y,compare_clienttag,compare_id)+
	     account_get_ladder_active_disconnects(y,compare_clienttag,compare_id);
    if (xgames>ygames)
        return -1;
    if (xgames<ygames)
        return +1;
    
    xrating = account_get_ladder_active_rating(x,compare_clienttag,compare_id);
    yrating = account_get_ladder_active_rating(y,compare_clienttag,compare_id);
    if (xrating>yrating)
        return -1;
    if (xrating<yrating)
        return +1;
    
    xwins = account_get_ladder_active_wins(x,compare_clienttag,compare_id);
    ywins = account_get_ladder_active_wins(y,compare_clienttag,compare_id);
    if (xwins>ywins)
        return -1;
    if (xwins<ywins)
        return +1;
    
    xoldrank = account_get_ladder_active_rank(x,compare_clienttag,compare_id);
    yoldrank = account_get_ladder_active_rank(y,compare_clienttag,compare_id);
    if (xoldrank<yoldrank)
        return -1;
    if (xoldrank>yoldrank)
        return +1;
    
    return 0;
}


/*
 * This routine will reorder the members of the account arrays to reflect the proper rankings.
 */
static int ladder_sort(t_ladder * ladder)
{
    if (!ladder)
    {
	eventlog(eventlog_level_error,"ladder_sort","got NULL ladder");
	return -1;
    }
    
    compare_id = ladder->id;
    compare_clienttag = ladder->clienttag;
    
    switch (ladder->ltime)
    {
    case ladder_time_current:
	qsort(ladder->highestrated,ladder->len,sizeof(t_account *),compare_current_highestrated);
	qsort(ladder->mostwins,    ladder->len,sizeof(t_account *),compare_current_mostwins);
	qsort(ladder->mostgames,   ladder->len,sizeof(t_account *),compare_current_mostgames);
	break;
    case ladder_time_active:
	qsort(ladder->highestrated,ladder->len,sizeof(t_account *),compare_active_highestrated);
	qsort(ladder->mostwins,    ladder->len,sizeof(t_account *),compare_active_mostwins);
	qsort(ladder->mostgames,   ladder->len,sizeof(t_account *),compare_active_mostgames);
	break;
    default:
	eventlog(eventlog_level_error,"ladder_sort","ladder has bad ltime (%u)",(unsigned int)ladder->ltime);
	return -1;
    }
    
    return 0;
}


/*
 * Make the current ladder statistics the active ones.
 */
static int ladder_make_active(t_ladder const * ladder)
{
    t_ladder *    active_ladder;
    t_account * * temp_ladder_highestrated;
    t_account * * temp_ladder_mostwins;
    t_account * * temp_ladder_mostgames;
    t_account *   account;
    char const *  timestr;
    unsigned int  i;
    t_bnettime    bt;
    
    if (!ladder)
    {
	eventlog(eventlog_level_error,"ladder_make_active","got NULL ladder");
	return -1;
    }
    if (ladder->ltime!=ladder_time_current)
    {
	eventlog(eventlog_level_error,"ladder_make_active","got non-current ladder (ltime=%u)",(unsigned int)ladder->ltime);
	return -1;
    }
#ifdef LADDER_DEBUG
    if (ladder->len!=0 && (!ladder->highestrated || !ladder->mostwins || !ladder->mostgames))
    {
	eventlog(eventlog_level_error,"ladder_make_active","found ladder with len=%u with NULL array",ladder->len);
	return -1;
    }
#endif
    
    if (!(active_ladder = ladderlist_find_ladder(ladder_time_active,ladder->clienttag,ladder->id)))
    {
	eventlog(eventlog_level_error,"ladder_make_active","could not locate existing active ladder");
	return -1;
    }
    
    if (ladder->len)
    {
	if (!(temp_ladder_highestrated = malloc(sizeof(t_account *)*ladder->len)))
	{
	    eventlog(eventlog_level_error,"ladder_make_active","unable to allocate memory for temp_ladder_highestrated");
	    return -1;
	}
	if (!(temp_ladder_mostwins = malloc(sizeof(t_account *)*ladder->len)))
	{
	    eventlog(eventlog_level_error,"ladder_make_active","unable to allocate memory for temp_ladder_mostwins");
	    free(temp_ladder_highestrated);
	    return -1;
	}
	if (!(temp_ladder_mostgames = malloc(sizeof(t_account *)*ladder->len)))
	{
	    eventlog(eventlog_level_error,"ladder_make_active","unable to allocate memory for temp_ladder_mostgames");
	    free(temp_ladder_mostwins);
	    free(temp_ladder_highestrated);
	    return -1;
	}
	
	for (i=0; i<ladder->len; i++)
	{
	    temp_ladder_highestrated[i] = ladder->highestrated[i];
	    temp_ladder_mostwins[i]     = ladder->mostwins[i];
	    temp_ladder_mostgames[i]    = ladder->mostgames[i];
	    
	    account = ladder->highestrated[i];
	    
	    account_set_ladder_active_wins(account,ladder->clienttag,ladder->id,
					   account_get_ladder_wins(account,ladder->clienttag,ladder->id));
	    account_set_ladder_active_losses(account,ladder->clienttag,ladder->id,
					     account_get_ladder_losses(account,ladder->clienttag,ladder->id));
	    account_set_ladder_active_draws(account,ladder->clienttag,ladder->id,
					    account_get_ladder_draws(account,ladder->clienttag,ladder->id));
	    account_set_ladder_active_disconnects(account,ladder->clienttag,ladder->id,
						  account_get_ladder_disconnects(account,ladder->clienttag,ladder->id));
	    account_set_ladder_active_rating(account,ladder->clienttag,ladder->id,
					     account_get_ladder_rating(account,ladder->clienttag,ladder->id));
	    account_set_ladder_active_rank(account,ladder->clienttag,ladder->id,
					   account_get_ladder_rank(account,ladder->clienttag,ladder->id));
            if (!(timestr = account_get_ladder_last_time(account,ladder->clienttag,ladder->id)))
                timestr = BNETD_LADDER_DEFAULT_TIME;
            bnettime_set_str(&bt,timestr);
	    account_set_ladder_active_last_time(account,ladder->clienttag,ladder->id,bt);
	}
    }
    else
    {
	eventlog(eventlog_level_debug,"ladder_make_active","ladder_len is zero so active ladder will be empty");
	temp_ladder_highestrated = NULL;
	temp_ladder_mostwins     = NULL;
	temp_ladder_mostgames    = NULL;
    }
    
    if (active_ladder->highestrated)
	free(active_ladder->highestrated);
    if (active_ladder->mostwins)
	free(active_ladder->mostwins);
    if (active_ladder->mostgames)
	free(active_ladder->mostgames);
    
    active_ladder->highestrated = temp_ladder_highestrated;
    active_ladder->mostwins     = temp_ladder_mostwins;
    active_ladder->mostgames    = temp_ladder_mostgames;
    active_ladder->len          = ladder->len;
    
    return 0;
}


extern int ladderlist_make_all_active(void)
{
    t_elem const *   curr;
    t_ladder const * ladder;
    
    LIST_TRAVERSE_CONST(ladderlist_head,curr)
    {
	ladder = elem_get_data(curr);
	if (ladder->ltime!=ladder_time_current)
	    continue;
	if (ladder_make_active(ladder)<0)
	    eventlog(eventlog_level_warn,"ladderlist_make_all_active","unable to activate lader for \"%s\"",ladder->clienttag);
    }
    
    return 0;
}


/*
 * Prepare an account for first ladder play if necessary.
 */
extern int ladder_init_account(t_account * account, char const * clienttag, t_ladder_id id)
{
    char const * tname;
    t_ladder *   ladder;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"ladder_init_account","got NULL account");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_init_account","got bad clienttag");
	return -1;
    }
    
    if (account_get_ladder_rating(account,clienttag,id)==0)
    {
	if (account_get_ladder_wins(account,clienttag,id)+
	    account_get_ladder_losses(account,clienttag,id)>0) /* no ladder games so far... */
	{
	    eventlog(eventlog_level_warn,"ladder_init_account","account for \"%s\" (%s) has %u wins and %u losses but has zero rating",(tname = account_get_name(account)),clienttag,account_get_ladder_wins(account,clienttag,id),account_get_ladder_losses(account,clienttag,id));
	    account_unget_name(tname);
	    return -1;
	}
	account_adjust_ladder_rating(account,clienttag,id,prefs_get_ladder_init_rating());
	
	if (!(ladder = ladderlist_find_ladder(ladder_time_current,clienttag,id)))
	{
	    eventlog(eventlog_level_error,"ladder_init_account","could not locate existing ladder for \"%s\" (unsupported clienttag?)",clienttag);
	    return -1;
	}
	ladder_insert_account(ladder,account);
	eventlog(eventlog_level_info,"ladder_init_account","initialized account for \"%s\" for \"%s\" ladder",(tname = account_get_name(account)),clienttag);
	account_unget_name(tname);
    }
    
    return 0;
}


/*
 * Update player ratings, rankings, etc due to game results.
 */
extern int ladder_update(char const * clienttag, t_ladder_id id, unsigned int count, t_account * * players, t_game_result * results, t_ladder_info * info, t_ladder_option opns)
{
    unsigned int curr;
    t_account *  sorted[8];
    unsigned int winners=0;
    unsigned int losers=0;
    unsigned int draws=0;
    
    if (count<2 || count>8)
    {
	eventlog(eventlog_level_error,"ladder_update","got invalid player count %u",count);
	return -1;
    }
    if (!players)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL players");
	return -1;
    }
    if (!results)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL results");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_update","got bad clienttag");
	return -1;
    }
    if (!info)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL info");
	return -1;
    }
    
    for (curr=0; curr<count; curr++)
    {
	if (!players[curr])
	{
	    eventlog(eventlog_level_error,"ladder_update","got NULL player[%u] (of %u)",curr,count);
	    return -1;
	}
	
	switch (results[curr])
	{
	case game_result_win:
	    winners++;
	    break;
	case game_result_loss:
	    losers++;
	    break;
	case game_result_draw:
	    draws++;
	    break;
	case game_result_disconnect:
	    if (opns&ladder_option_disconnectisloss)
		losers++;
	    break;
	default:
	    eventlog(eventlog_level_error,"ladder_update","bad results[%u]=%u",curr,(unsigned int)results[curr]);
	    return -1;
	}
    }
    
    if (draws>0)
    {
	if (draws!=count)
	{
	    eventlog(eventlog_level_error,"ladder_update","some, but not all players had a draw count=%u (winners=%u losers=%u draws=%u)",count,winners,losers,draws);
	    return -1;
	}
	
	return 0; /* no change in case of draw */
    }
    if (winners!=1 || losers<1)
    {
	eventlog(eventlog_level_error,"ladder_update","missing winner or loser for count=%u (winners=%u losers=%u draws=%u)",count,winners,losers,draws);
	return -1;
    }
    
    if (ladder_calc_info(clienttag,id,count,players,sorted,results,info)<0)
    {
	eventlog(eventlog_level_error,"ladder_update","unable to calculate info from game results");
	return -1;
    }
    
    for (curr=0; curr<count; curr++)
	account_adjust_ladder_rating(players[curr],clienttag,id,info[curr].adj);
    
    {
	t_ladder *   ladder;
	unsigned int i;
	
	if (!(ladder = ladderlist_find_ladder(ladder_time_current,clienttag,id)))
	{
	    eventlog(eventlog_level_error,"ladder_update","got unknown clienttag \"%s\", did not sort",clienttag);
	    return 0; /* FIXME: should this be -1? */
	}
	
	/* re-rank accounts in the arrays */
	ladder_sort(ladder);
	
	/* now fixup the rankings stored in the accounts */
	for (i=0; i<ladder->len; i++)
	    if (account_get_ladder_rank(ladder->highestrated[i],clienttag,id)!=i+1)
		account_set_ladder_rank(ladder->highestrated[i],clienttag,id,i+1);
    }
    
    return 0;
}


extern int ladder_check_map(char const * mapname, t_game_maptype maptype, char const * clienttag)
{
    if (!mapname)
    {
	eventlog(eventlog_level_error,"ladder_check_map","got NULL mapname");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_check_map","got bad clienttag");
	return -1;
    }
    
    eventlog(eventlog_level_debug,"ladder_check_map","checking mapname \"%s\" maptype=%d",mapname,(int)maptype);
    if (maptype==game_maptype_ladder) /* FIXME: what about Ironman? */
	return 1;
    
    return 0;
}


extern t_account * ladder_get_account_by_rank(unsigned int rank, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id)
{
    t_ladder const * ladder;
    t_account * *    accounts;
    
    if (rank<1)
    {
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got zero rank");
	return NULL;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got bad clienttag");
	return NULL;
    }
    
    if (!(ladder = ladderlist_find_ladder(ltime,clienttag,id)))
    {
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","could not locate ladder");
	return NULL;
    }
    switch (lsort)
    {
    case ladder_sort_highestrated:
	accounts = ladder->highestrated;
	break;
    case ladder_sort_mostwins:
	accounts = ladder->mostwins;
	break;
    case ladder_sort_mostgames:
	accounts = ladder->mostgames;
	break;
    default:
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got bad ladder sort %u",(unsigned int)lsort);
	return NULL;
    }
    
    if (rank>ladder->len)
	return NULL;
    return accounts[rank-1];
}


extern unsigned int ladder_get_rank_by_account(t_account * account, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id)
{
    t_ladder const * ladder;
    t_account * *    accounts;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got NULL account");
	return 0;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got bad clienttag");
	return 0;
    }
    
    if (!(ladder = ladderlist_find_ladder(ltime,clienttag,id)))
    {
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","could not locate ladder");
	return 0;
    }
    switch (lsort)
    {
    case ladder_sort_highestrated:
	accounts = ladder->highestrated;
	break;
    case ladder_sort_mostwins:
	accounts = ladder->mostwins;
	break;
    case ladder_sort_mostgames:
	accounts = ladder->mostgames;
	break;
    default:
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got bad ladder sort %u",(unsigned int)lsort);
	return 0;
    }
    
    /* assume it is faster to search in memory than to potentially have to
       load the account to read the rank attribute */
    {
	unsigned int i;
	
	for (i=0; i<ladder->len; i++) /* FIXME: this does not work for active because accounts may have been deleted */
	    if (accounts[i]==account)
		return i+1;
    }
    
    return 0;
}


/*
 * Load the current user accounts with non-zero ladder rankings into
 * the current ladder arrays.
 */
extern int ladderlist_create(void)
{
    t_entry *      curra;
    t_elem const * currl;
    t_account *    account;
    t_ladder *     ladder;
    unsigned int   i;
    
    if (!(ladderlist_head = list_create()))
    {
	eventlog(eventlog_level_error,"ladderlist_create","could not create list");
	return -1;
    }
    
    /* FIXME: hard-code for now, but make a config file in future.. no more updating this
       file to support new ladders :) */
    ladder_create(CLIENTTAG_STARCRAFT,ladder_id_normal);
    ladder_create(CLIENTTAG_BROODWARS,ladder_id_normal);
    ladder_create(CLIENTTAG_WARCIIBNE,ladder_id_normal);
    ladder_create(CLIENTTAG_WARCIIBNE,ladder_id_ironman);
    
    eventlog(eventlog_level_info,"ladderlist_create","created %u local ladders",list_get_length(ladderlist_head));
    
    HASHTABLE_TRAVERSE(accountlist(),curra)
    {
	account = entry_get_data(curra);
	
	LIST_TRAVERSE_CONST(ladderlist_head,currl)
	{
	    ladder = elem_get_data(currl);
	    
	    if (ladder->ltime==ladder_time_current)
	    {
		if (account_get_ladder_rating(account,ladder->clienttag,ladder->id)>0)
		    ladder_insert_account(ladder,account);
	    }
	    else
	    {
		if (account_get_ladder_active_rating(account,CLIENTTAG_STARCRAFT,ladder_id_normal)>0)
		    ladder_insert_account(ladder,account);
	    }
	}
    }
    
    LIST_TRAVERSE_CONST(ladderlist_head,currl)
    {
	ladder = elem_get_data(currl);
	ladder_sort(ladder);
	
	/* in case the user files were changed outside of bnetd update the rating based ranks... */
	if (ladder->ltime==ladder_time_current)
	{
	    for (i=0; i<ladder->len; i++)
		if (account_get_ladder_rank(ladder->highestrated[i],ladder->clienttag,ladder->id)!=i+1)
		    account_set_ladder_rank(ladder->highestrated[i],ladder->clienttag,ladder->id,i+1);
	    eventlog(eventlog_level_info,"ladderlist_create","added %u accounts to current ladder \"%s\":%u",ladder->len,ladder->clienttag,(unsigned int)ladder->id);
	}
	else /* don't change anything for active ladders */
	    eventlog(eventlog_level_info,"ladderlist_create","added %u accounts to active ladder \"%s\":%u",ladder->len,ladder->clienttag,(unsigned int)ladder->id);
    }
    
    return 0;
}


extern int ladderlist_destroy(void)
{
    t_ladder *     ladder;
    t_elem const * curr;
    
    if (ladderlist_head)
    {
	LIST_TRAVERSE(ladderlist_head,curr)
	{
	    ladder = elem_get_data(curr);
	    ladder_destroy(ladder);
	}
	
	if (list_destroy(ladderlist_head)<0)
	    return -1;
	ladderlist_head = NULL;
    }
    
    return 0;
}


static t_ladder * ladderlist_find_ladder(t_ladder_time ltime, char const * clienttag, t_ladder_id id)
{
    t_elem const * curr;
    t_ladder *     ladder;
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladderlist_find_ladder","got bad clienttag");
	return NULL;
    }
    
    if (ladderlist_head)
    {
	LIST_TRAVERSE_CONST(ladderlist_head,curr)
	    {
	    ladder = elem_get_data(curr);
	    
#ifdef LADDER_DEBUG
	    if (ladder->len!=0 && (!ladder->highestrated || !ladder->mostwins || !ladder->mostgames))
	    {
		eventlog(eventlog_level_error,"ladderlist_find_ladder","found ladder with len=%u with NULL array",ladder->len);
		continue;
	    }
#endif
	    if (strcmp(ladder->clienttag,clienttag)==0 &&
		ladder->id==id &&
		ladder->ltime==ltime)
		return ladder;
	}
    }
    
/*    eventlog(eventlog_level_debug,"ladderlist_find_ladder","ladder not found"); */
    return NULL;
}

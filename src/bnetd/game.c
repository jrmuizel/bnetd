/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000 Ross Combs (rocombs@cs.nmsu.edu)
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
#define GAME_INTERNAL_ACCESS
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
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include "compat/difftime.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "common/eventlog.h"
#include "prefs.h"
#include "connection.h"
#include "account.h"
#include "ladder.h"
#include "ladder_calc.h"
#include "common/bnettime.h"
#include "common/util.h"
#include "common/list.h"
#include "common/tag.h"
#include "common/addr.h"
#include "realm.h"
#include "game_conv.h"
#include "game.h"
#ifdef WITH_BITS
# include "bits_login.h"
# include "bits_game.h"
#endif
#include "common/setup_after.h"

static t_list * gamelist_head=NULL;
static int totalcount=0;


static void game_choose_host(t_game * game);
static void game_destroy(t_game const * game);
static int game_report(t_game * game);


static void game_choose_host(t_game * game)
{
    unsigned int i;
    
    if (game->count<1)
    {
	eventlog(eventlog_level_error,"game_choose_host","game has had no connections?!");
	return;
    }
    if (!game->connections)
    {
	eventlog(eventlog_level_error,"game_choose_host","game has NULL connections array");
	return;
    }
    
    for (i=0; i<game->count; i++)
	if (game->connections[i])
	{
#ifdef WITH_BITS
	    t_bits_loginlist_entry * lle;

#endif
	    game->owner = game->connections[i];
#ifdef WITH_BITS
	    lle = bits_loginlist_bysessionid(game->connections[i]);
	    if (lle) {
	    	game->addr  = lle->game_addr;
	    	game->port  = lle->game_port;
	    	game_set_owner(game,game->connections[i]);
	    } else {
		eventlog(eventlog_level_error,"game_choose_host","user with sessionid 0x%08x is not in the loginlist",game->connections[i]);
	    }
#else
	    game->addr  = conn_get_game_addr(game->connections[i]);
	    game->port  = conn_get_game_port(game->connections[i]);
#endif
	    return;
	}
    eventlog(eventlog_level_warn,"game_choose_host","no valid connections found");
}


extern char const * game_type_get_str(t_game_type type)
{
    switch (type)
    {
    case game_type_none:
	return "NONE";
	
    case game_type_melee:
	return "melee";
	
    case game_type_topvbot:
	return "top vs bottom";
	
    case game_type_ffa:
	return "free for all";
	
    case game_type_oneonone:
	return "one on one";
	
    case game_type_ctf:
	return "capture the flag";
	
    case game_type_greed:
	return "greed";
	
    case game_type_slaughter:
	return "slaughter";
	
    case game_type_sdeath:
	return "sudden death";
	
    case game_type_ladder:
	return "ladder";
	
    case game_type_ironman:
	return "ironman";
	
    case game_type_mapset:
	return "mapset";
	
    case game_type_teammelee:
	return "team melee";
	
    case game_type_teamffa:
	return "team free for all";
	
    case game_type_teamctf:
	return "team capture the flag";
	
    case game_type_pgl:
	return "PGL";
	
    case game_type_diablo:
	return "Diablo";
	
    case game_type_diablo2open:
	return "Diablo II (open)";
	
    case game_type_diablo2closed:
	return "Diablo II (closed)";
	
    case game_type_all:
    default:
	return "UNKNOWN";
    }
}


extern char const * game_status_get_str(t_game_status status)
{
    switch (status)
    {
    case game_status_started:
	return "started";
	
    case game_status_full:
	return "full";
	
    case game_status_open:
	return "open";
	
    case game_status_done:
	return "done";
	
    default:
	return "UNKNOWN";
    }
}


extern char const * game_result_get_str(t_game_result result)
{
    switch (result)
    {
    case game_result_none:
        return "NONE";
	
    case game_result_win:
        return "WIN";
	
    case game_result_loss:
        return "LOSS";
	
    case game_result_draw:
        return "DRAW";
	
    case game_result_disconnect:
        return "DISCONNECT";
	
    case game_result_observer:
	return "OBSERVER";
	
    default:
        return "UNKNOWN";
    }
}


extern char const * game_option_get_str(t_game_option option)
{
    switch (option)
    {
    case game_option_melee_normal:
	return "normal";
    case game_option_ffa_normal:
	return "normal";
    case game_option_oneonone_normal:
	return "normal";
    case game_option_ctf_normal:
	return "normal";
    case game_option_greed_10000:
	return "10000 minerals";
    case game_option_greed_7500:
	return "7500 minerals";
    case game_option_greed_5000:
	return "5000 minerals";
    case game_option_greed_2500:
	return "2500 minerals";
    case game_option_slaughter_60:
	return "60 minutes";
    case game_option_slaughter_45:
	return "45 minutes";
    case game_option_slaughter_30:
	return "30 minutes";
    case game_option_slaughter_15:
	return "15 minutes";
    case game_option_sdeath_normal:
	return "normal";
    case game_option_ladder_countasloss:
	return "count as loss";
    case game_option_ladder_nopenalty:
	return "no penalty";
    case game_option_mapset_normal:
	return "normal";
    case game_option_teammelee_4:
	return "4 teams";
    case game_option_teammelee_3:
	return "3 teams";
    case game_option_teammelee_2:
	return "2 teams";
    case game_option_teamffa_4:
	return "4 teams";
    case game_option_teamffa_3:
	return "3 teams";
    case game_option_teamffa_2:
	return "2 teams";
    case game_option_teamctf_4:
	return "4 teams";
    case game_option_teamctf_3:
	return "3 teams";
    case game_option_teamctf_2:
	return "2 teams";
    case game_option_none:
	return "none";
    default:
	return "UNKNOWN";
    }
}


extern char const * game_maptype_get_str(t_game_maptype maptype)
{
    switch (maptype)
    {
    case game_maptype_selfmade:
	return "Self-Made";
    case game_maptype_blizzard:
	return "Blizzard";
    case game_maptype_ladder:
	return "Ladder";
    case game_maptype_pgl:
	return "PGL";
    default:
	return "Unknown";
    }
}


extern char const * game_tileset_get_str(t_game_tileset tileset)
{
    switch (tileset)
    {
    case game_tileset_badlands:
	return "Badlands";
    case game_tileset_space:
	return "Space";
    case game_tileset_installation:
	return "Installation";
    case game_tileset_ashworld:
	return "Ash World";
    case game_tileset_jungle:
	return "Jungle";
    case game_tileset_desert:
	return "Desert";
    case game_tileset_ice:
	return "Ice";
    case game_tileset_twilight:
	return "Twilight";
    default:
	return "Unknown";
    }
}


extern char const * game_speed_get_str(t_game_speed speed)
{
    switch (speed)
    {
    case game_speed_slowest:
	return "slowest";
    case game_speed_slower:
	return "slower";
    case game_speed_slow:
	return "slow";
    case game_speed_normal:
	return "normal";
    case game_speed_fast:
	return "fast";
    case game_speed_faster:
	return "faster";
    case game_speed_fastest:
	return "fastest";
    default:
	return "unknown";
    }
}


extern char const * game_difficulty_get_str(t_game_difficulty difficulty)
{
    switch (difficulty)
    {
    case game_difficulty_normal:
	return "normal";
    case game_difficulty_nightmare:
	return "nightmare";
    case game_difficulty_hell:
	return "hell";
    case game_difficulty_hardcore_normal:
	return "hardcore normal";
    case game_difficulty_hardcore_nightmare:
	return "hardcore nightmare";
    case game_difficulty_hardcore_hell:
	return "hardcore hell";
    default:
	return "unknown";
    }
}


extern t_game * game_create(char const * name, char const * pass, char const * info, t_game_type type, int startver, char const * clienttag)
{
    t_game * game;
    time_t now;

    now = time(NULL);
    
    if (!name)
    {
	eventlog(eventlog_level_info,"game_create","got NULL game name");
	return NULL;
    }
    if (!pass)
    {
	eventlog(eventlog_level_info,"game_create","got NULL game pass");
	return NULL;
    }
    if (!info)
    {
	eventlog(eventlog_level_info,"game_create","got NULL game info");
	return NULL;
    }
    
    if (gamelist_find_game(name,game_type_all))
    {
	eventlog(eventlog_level_info,"game_create","game \"%s\" not created because it already exists",name);
	return NULL; /* already have a game by that name */
    }
    
    if (!(game = malloc(sizeof(t_game))))
    {
	eventlog(eventlog_level_error,"game_create","could not allocate memory for game");
	return NULL;
    }
    
    if (!(game->name = strdup(name)))
    {
	free(game);
	return NULL;
    }
    if (!(game->pass = strdup(pass)))
    {
	free((void *)game->name); /* avoid warning */
	free(game);
	return NULL;
    }
    if (!(game->info = strdup(info)))
    {
	free((void *)game->pass); /* avoid warning */
	free((void *)game->name); /* avoid warning */
	free(game);
	return NULL;
    }
    if (!(game->clienttag = strdup(clienttag)))
    {
	eventlog(eventlog_level_error,"game_create","could not allocate memory for game->clienttag");
	free((void *)game->info); /* avoid warning */
	free((void *)game->pass); /* avoid warning */
	free((void *)game->name); /* avoid warning */
	free(game);
	return NULL;
    }

    game->type          = type;
    game->addr          = 0; /* will be set by first player */
    game->port          = 0; /* will be set by first player */
    game->version       = 0; /* FIXME! */
    game->startver      = startver; /* start packet version */
    game->status        = game_status_open;
    game->realm         = 0;
    game->realmname     = NULL;
    game->id            = ++totalcount;
    game->mapname       = NULL;
    game->ref           = 0;
    game->count         = 0;
#ifdef WITH_BITS
    game->owner		= 0; /* FIXME: sessionid of creator should be here */
#else
    game->owner         = NULL;
#endif
    game->connections   = NULL;
    game->players       = NULL;
    game->results       = NULL;
    game->report_heads  = NULL;
    game->report_bodies = NULL;
    game->create_time   = now;
    game->start_time    = (time_t)0;
    game->lastaccess_time = now;
    game->option        = game_option_none;
    game->maptype       = game_maptype_none;
    game->tileset       = game_tileset_none;
    game->speed         = game_speed_none;
    game->mapsize_x     = 0;
    game->mapsize_y     = 0;
    game->maxplayers    = 0;
    game->bad           = 0;
    game->description   = NULL;

    game_parse_info(game,info);
    
    if (list_prepend_data(gamelist_head,game)<0)
    {
	eventlog(eventlog_level_error,"game_create","could not insert game");
	free((void *)game->clienttag); /* avoid warning */
	free((void *)game->info); /* avoid warning */
	free((void *)game->pass); /* avoid warning */
	free((void *)game->name); /* avoid warning */
	free(game);
	return NULL;
    }
    /* bits_game must be announced later! */

    eventlog(eventlog_level_info,"game_create","game \"%s\" (pass \"%s\") type %hu(%s) startver %d created",name,pass,(unsigned short)type,game_type_get_str(type),startver);
    
    return game;
}


static void game_destroy(t_game const * game)
{
    unsigned int i;
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_destroy","got NULL game");
	return;
    }
    
    if (list_remove_data(gamelist_head,game)<0)
    {
	eventlog(eventlog_level_error,"game_destroy","could not find game \"%s\" in list",game_get_name(game));
        return;
    }
    
    if (game->realmname)
    { 
        realm_add_game_number(realmlist_find_realm(game->realmname),-1); 
    } 

#ifdef WITH_BITS
    send_bits_gamelist_del(NULL,game);
#endif
    eventlog(eventlog_level_debug,"game_destroy","game \"%s\" (count=%u ref=%u) removed from list...",game_get_name(game),game->count,game->ref);
    
    for (i=0; i<game->count; i++)
    {
	if (game->report_bodies && game->report_bodies[i])
	    free((void *)game->report_bodies[i]); /* avoid warning */
	if (game->report_heads && game->report_heads[i])
	    free((void *)game->report_heads[i]); /* avoid warning */
    }
    if (game->realmname)
	free((void *)game->realmname); /* avoid warining */
    if (game->report_bodies)
	free((void *)game->report_bodies); /* avoid warning */
    if (game->report_heads)
	free((void *)game->report_heads); /* avoid warning */
    if (game->results)
	free((void *)game->results); /* avoid warning */
    if (game->connections)
	free((void *)game->connections); /* avoid warning */
    if (game->players)
	free((void *)game->players); /* avoid warning */
    if (game->mapname)
	free((void *)game->mapname); /* avoid warning */
    free((void *)game->info); /* avoid warning */
    free((void *)game->pass); /* avoid warning */
    free((void *)game->name); /* avoid warning */
    free((void *)game); /* avoid warning */
    
    eventlog(eventlog_level_info,"game_destroy","game deleted");
    
    return;
}


static int game_report(t_game * game)
{
    FILE *          fp;
    char *          realname;
    char *          tempname;
    char const *    tname;
    unsigned int    i;
    unsigned int    realcount;
    time_t          now=time(NULL);
    t_ladder_info * ladder_info=NULL;
    int             discisloss;
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_report","got NULL game");
	return -1;
    }
    if (!game->clienttag || strlen(game->clienttag)!=4)
    {
	eventlog(eventlog_level_error,"game_report","got bad clienttag");
	return -1;
    }
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_report","player array is NULL");
	return -1;
    }
    if (!game->results)
    {
	eventlog(eventlog_level_error,"game_report","results array is NULL");
	return -1;
    }
    
    if (prefs_get_discisloss()==1 || game->option==game_option_ladder_countasloss)
	discisloss = 1;
    else
	discisloss = 0;
    
    if (strcmp(game->clienttag,CLIENTTAG_DIABLOSHR)==0 ||
        strcmp(game->clienttag,CLIENTTAG_DIABLORTL)==0 ||
        strcmp(game->clienttag,CLIENTTAG_DIABLO2ST)==0 ||
        strcmp(game->clienttag,CLIENTTAG_DIABLO2DV)==0 ||
        strcmp(game->clienttag,CLIENTTAG_DIABLO2XP)==0)
    {
      if (prefs_get_report_diablo_games() == 1)
	/* diablo games have transient players and no reported winners/losers */
	realcount = 0;
      else
      {
	eventlog(eventlog_level_info,"game_report","diablo gamereport disabled: ignoring game");
	return 0;
      }
    }
    else
    {
	/* "compact" the game; move all the real players to the top... */
	realcount = 0;
	for (i=0; i<game->count; i++)
	{
	    if (!game->players[i])
	    {
		eventlog(eventlog_level_error,"game_report","player slot %u has NULL account",i);
		continue;
	    }
	    
	    if (game->results[i]!=game_result_none)
	    {
		game->players[realcount]       = game->players[i];
		game->results[realcount]       = game->results[i];
		game->report_heads[realcount]  = game->report_heads[i];
		game->report_bodies[realcount] = game->report_bodies[i];
		realcount++;
	    }
	}
	
	/* then nuke duplicate players after the real players */
	for (i=realcount; i<game->count; i++)
	{
	    game->players[i]       = NULL;
	    game->results[i]       = game_result_none;
	    game->report_heads[i]  = NULL;
	    game->report_bodies[i] = NULL;
	}
	
	if (realcount<1)
	{
	    eventlog(eventlog_level_info,"game_report","ignoring game");
	    return -1;
	}
    }
    
    eventlog(eventlog_level_debug,"game_report","realcount=%d count=%u",realcount,game->count);
    
    if (realcount>=1 && !game->bad)
	if (game_get_type(game)==game_type_ladder ||
	    game_get_type(game)==game_type_ironman)
	{
	    t_ladder_id id;
	    
	    if (game_get_type(game)==game_type_ladder)
		id = ladder_id_normal;
	    else
		id = ladder_id_ironman;
	    
	    for (i=0; i<realcount; i++)
	    {
		eventlog(eventlog_level_debug,"game_report","realplayer %u result=%u",i+1,(unsigned int)game->results[i]);
		
		ladder_init_account(game->players[i],game->clienttag,id);
		
		switch (game->results[i])
		{
		case game_result_win:
		    account_inc_ladder_wins(game->players[i],game->clienttag,id);
		    account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_win));
		    break;
		case game_result_loss:
		    account_inc_ladder_losses(game->players[i],game->clienttag,id);
		    account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_loss));
		    break;
		case game_result_draw:
		    account_inc_ladder_draws(game->players[i],game->clienttag,id);
		    account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_draw));
		    break;
		case game_result_disconnect:
		    if (discisloss)
		    {
			account_inc_ladder_losses(game->players[i],game->clienttag,id);
			account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_loss));
		    }
		    else
		    {
/* FIXME: do the first disconnect only stuff like below */
			account_inc_ladder_disconnects(game->players[i],game->clienttag,id);
			account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_disconnect));
		    }
		    break;
		default:
		    eventlog(eventlog_level_error,"game_report","bad ladder game realplayer results[%u] = %u",i,game->results[i]);
		    account_inc_ladder_disconnects(game->players[i],game->clienttag,id);
		    account_set_ladder_last_result(game->players[i],game->clienttag,id,game_result_get_str(game_result_disconnect));
		}
		account_set_ladder_last_time(game->players[i],game->clienttag,id,bnettime());
	    }
	    
	    if (!(ladder_info = malloc(sizeof(t_ladder_info)*realcount)))
		eventlog(eventlog_level_error,"game_report","unable to allocate memory for ladder_info, ladder ratings will not be updated");
	    else
		if (ladder_update(game->clienttag,id,
		    realcount,game->players,game->results,ladder_info,
		    discisloss?ladder_option_disconnectisloss:ladder_option_none)<0)
		{
		    eventlog(eventlog_level_error,"game_report","unable to update ladder stats");
		    free(ladder_info);
		    ladder_info = NULL;
		}
	}
	else
	{
	    int disc_set=0;
	    
	    for (i=0; i<realcount; i++)
	    {
		switch (game->results[i])
		{
		case game_result_win:
		    account_inc_normal_wins(game->players[i],game->clienttag);
		    account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_win));
		    break;
		case game_result_loss:
		    account_inc_normal_losses(game->players[i],game->clienttag);
		    account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_loss));
		    break;
		case game_result_draw:
		    account_inc_normal_draws(game->players[i],game->clienttag);
		    account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_draw));
		    break;
		case game_result_disconnect:
		    if (discisloss)
		    {
			account_inc_normal_losses(game->players[i],game->clienttag);
			account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_loss));
		    }
		    else
		    {
/* FIXME: Is the missing player always the first one in this array?  It seems like it should be
   the person that created the game */
			if (!disc_set)
			{
			    account_inc_normal_disconnects(game->players[i],game->clienttag);
			    disc_set = 1;
			}
			account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_disconnect));
		    }
		    break;
		default:
		    eventlog(eventlog_level_error,"game_report","bad normal game realplayer results[%u] = %u",i,game->results[i]);
/* FIXME: Jung-woo fixed this here but we should find out what value results[i] has...
   and why "discisloss" isn't set above in game_result_disconnect */
#if 0
		    /* commented out for loose disconnect policy */
		    /* account_inc_normal_disconnects(game->players[i],game->clienttag); */
#endif
		    account_inc_normal_disconnects(game->players[i],game->clienttag);
		    account_set_normal_last_result(game->players[i],game->clienttag,game_result_get_str(game_result_disconnect));
		}
		account_set_normal_last_time(game->players[i],game->clienttag,bnettime());
	    }
    }
    
    if (game_get_type(game)!=game_type_ladder && prefs_get_report_all_games()!=1)
    {
	eventlog(eventlog_level_debug,"game_report","not reporting normal games");
	return 0;
    }
    
    {
	struct tm * tmval;
	char        dstr[64];
	
	if (!(tmval = gmtime(&now)))
	    dstr[0] = '\0';
	else
	    sprintf(dstr,"%04d%02d%02d%02d%02d%02d",
		    1900+tmval->tm_year,
		    tmval->tm_mon+1,
		    tmval->tm_mday,
		    tmval->tm_hour,
		    tmval->tm_min,
		    tmval->tm_sec);
	
	if (!(tempname = malloc(strlen(prefs_get_reportdir())+1+1+5+1+2+1+strlen(dstr)+1+6+1)))
	{
	    eventlog(eventlog_level_error,"game_report","could not allocate memory for tempname");
	    if (ladder_info)
		free(ladder_info);
	    return -1;
	}
	sprintf(tempname,"%s/.bnetd-gr_%s_%06u",prefs_get_reportdir(),dstr,game->id);
	if (!(realname = malloc(strlen(prefs_get_reportdir())+1+2+1+strlen(dstr)+1+6+1)))
	{
	    eventlog(eventlog_level_error,"game_report","could not allocate memory for realname");
	    free(tempname);
	    if (ladder_info)
		free(ladder_info);
	    return -1;
	}
	sprintf(realname,"%s/gr_%s_%06u",prefs_get_reportdir(),dstr,game->id);
    }
    
    if (!(fp = fopen(tempname,"w")))
    {
	eventlog(eventlog_level_error,"game_report","could not open report file \"%s\" for writing (fopen: %s)",tempname,strerror(errno));
	if (ladder_info)
	    free(ladder_info);
	free(realname);
	free(tempname);
	return -1;
    }
    
    if (game->bad)
	fprintf(fp,"[ game results ignored due to inconsistencies ]\n\n");
    fprintf(fp,"name=\"%s\" id="GAMEID_FORMAT"\n",
	    game->name,
	    game->id);
    fprintf(fp,"clienttag=%4s type=\"%s\" option=\"%s\"\n",
	    game->clienttag,
	    game_type_get_str(game->type),
	    game_option_get_str(game->option));
    {
	struct tm * gametime;
	char        timetemp[GAME_TIME_MAXLEN];
	
	if (!(gametime = gmtime(&game->create_time)))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),GAME_TIME_FORMAT,gametime);
	fprintf(fp,"created=\"%s\" ",timetemp);
	
	if (!(gametime = gmtime(&game->start_time)))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),GAME_TIME_FORMAT,gametime);
	fprintf(fp,"started=\"%s\" ",timetemp);
	
	if (!(gametime = gmtime(&now)))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),GAME_TIME_FORMAT,gametime);
	fprintf(fp,"ended=\"%s\"\n",timetemp);
    }
    {
	char const * mapname;
	
	if (!(mapname = game_get_mapname(game)))
	    mapname = "?";
	
	fprintf(fp,"mapfile=\"%s\" mapauth=\"%s\" mapsize=%ux%u tileset=\"%s\"\n",
		mapname,
		game_maptype_get_str(game_get_maptype(game)),
		game_get_mapsize_x(game),game_get_mapsize_y(game),
		game_tileset_get_str(game_get_tileset(game)));
    }
    fprintf(fp,"joins=%u maxplayers=%u\n",
	    game_get_count(game),
	    game_get_maxplayers(game));
    
    if (!prefs_get_hide_addr())
	fprintf(fp,"host=%s\n",addr_num_to_addr_str(game_get_addr(game),game_get_port(game)));
    
    fprintf(fp,"\n\n");
    
    if (strcmp(game->clienttag,CLIENTTAG_DIABLORTL)==0)
	for (i=0; i<game->count; i++)
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"%-16s JOINED\n",tname);
	    account_unget_name(tname);
	}
    else
	if (ladder_info)
	    for (i=0; i<realcount; i++)
	    {
		tname = account_get_name(game->players[i]);
		fprintf(fp,"%-16s %-8s rating=%u [#%05u]  prob=%4.1f%%  K=%2u  adj=%+d\n",
			tname,
			game_result_get_str(game->results[i]),
			ladder_info[i].oldrating,
			ladder_info[i].oldrank,
			ladder_info[i].prob*100.0,
			ladder_info[i].k,
			ladder_info[i].adj);
		account_unget_name(tname);
	    }
	else
	    for (i=0; i<realcount; i++)
	    {
		tname = account_get_name(game->players[i]);
		fprintf(fp,"%-16s %-8s\n",
			tname,
			game_result_get_str(game->results[i]));
		account_unget_name(tname);
	    }
    fprintf(fp,"\n\n");
    
    if (ladder_info)
	free(ladder_info);
    
    for (i=0; i<realcount; i++)
    {
	if (game->report_heads[i])
	    fprintf(fp,"%s\n",game->report_heads[i]);
	else
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"[ game report header not available for player %u (\"%s\") ]\n",i+1,tname);
	    account_unget_name(tname);
	}
	if (game->report_bodies[i])
	    fprintf(fp,"%s\n",game->report_bodies[i]);
	else
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"[ game report body not available for player %u (\"%s\") ]\n\n",i+1,tname);
	    account_unget_name(tname);
	}
    }
    fprintf(fp,"\n\n");
    
    if (strcmp(game->clienttag,CLIENTTAG_STARCRAFT)==0 ||
	strcmp(game->clienttag,CLIENTTAG_SHAREWARE)==0 ||
        strcmp(game->clienttag,CLIENTTAG_BROODWARS)==0 ||
        strcmp(game->clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	for (i=0; i<realcount; i++)
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"%s's normal record is now %u/%u/%u (%u draws)\n",
		    tname,
		    account_get_normal_wins(game->players[i],game->clienttag),
		    account_get_normal_losses(game->players[i],game->clienttag),
		    account_get_normal_disconnects(game->players[i],game->clienttag),
		    account_get_normal_draws(game->players[i],game->clienttag));
	    account_unget_name(tname);
	}
    }
    if (strcmp(game->clienttag,CLIENTTAG_STARCRAFT)==0 ||
        strcmp(game->clienttag,CLIENTTAG_BROODWARS)==0 ||
        strcmp(game->clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	fprintf(fp,"\n");
	for (i=0; i<realcount; i++)
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"%s's standard ladder record is now %u/%u/%u (rating %u [#%05u]) (%u draws)\n",
		    tname,
		    account_get_ladder_wins(game->players[i],game->clienttag,ladder_id_normal),
		    account_get_ladder_losses(game->players[i],game->clienttag,ladder_id_normal),
		    account_get_ladder_disconnects(game->players[i],game->clienttag,ladder_id_normal),
		    account_get_ladder_rating(game->players[i],game->clienttag,ladder_id_normal),
		    account_get_ladder_rank(game->players[i],game->clienttag,ladder_id_normal),
		    account_get_ladder_draws(game->players[i],game->clienttag,ladder_id_normal));
	    account_unget_name(tname);
	}
    }
    if (strcmp(game->clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	fprintf(fp,"\n");
	for (i=0; i<realcount; i++)
	{
	    tname = account_get_name(game->players[i]);
	    fprintf(fp,"%s's ironman ladder record is now %u/%u/%u (rating %u [#%05u]) (%u draws)\n",
		    tname,
		    account_get_ladder_wins(game->players[i],game->clienttag,ladder_id_ironman),
		    account_get_ladder_losses(game->players[i],game->clienttag,ladder_id_ironman),
		    account_get_ladder_disconnects(game->players[i],game->clienttag,ladder_id_ironman),
		    account_get_ladder_rating(game->players[i],game->clienttag,ladder_id_ironman),
		    account_get_ladder_rank(game->players[i],game->clienttag,ladder_id_ironman),
		    account_get_ladder_draws(game->players[i],game->clienttag,ladder_id_ironman));
	    account_unget_name(tname);
	}
    }
    
    fprintf(fp,"\nThis game lasted %lu minutes (elapsed).\n",((unsigned long int)difftime(now,game->start_time))/60);
    
    if (fclose(fp)<0)
    {
	eventlog(eventlog_level_error,"game_report","could not close report file \"%s\" after writing (fclose: %s)",tempname,strerror(errno));
	free(realname);
	free(tempname);
	return -1;
    }
    
    if (rename(tempname,realname)<0)
    {
	eventlog(eventlog_level_error,"game_report","could not rename report file to \"%s\" (rename: %s)",realname,strerror(errno));
	free(realname);
	free(tempname);
	return -1;
    }
    
    eventlog(eventlog_level_debug,"game_report","game report saved as \"%s\"",realname);
    free(realname);
    free(tempname);
    return 0;
}


extern unsigned int game_get_id(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_id","got NULL game");
        return 0;
    }
    return game->id;
}


extern char const * game_get_name(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_name","got NULL game");
        return NULL;
    }
    return game->name;
}


extern t_game_type game_get_type(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_type","got NULL game");
        return 0;
    }
    return game->type;
}


extern t_game_maptype game_get_maptype(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_maptype","got NULL game");
        return game_maptype_none;
    }
    return game->maptype;
}


extern int game_set_maptype(t_game * game, t_game_maptype maptype)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_maptype","got NULL game");
        return -1;
    }
    game->maptype = maptype;
    return 0;
}


extern t_game_tileset game_get_tileset(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_tileset","got NULL game");
        return game_tileset_none;
    }
    return game->tileset;
}


extern int game_set_tileset(t_game * game, t_game_tileset tileset)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_tileset","got NULL game");
        return -1;
    }
    game->tileset = tileset;
    return 0;
}


extern t_game_speed game_get_speed(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_speed","got NULL game");
        return game_speed_none;
    }
    return game->speed;
}


extern int game_set_speed(t_game * game, t_game_speed speed)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_speed","got NULL game");
        return -1;
    }
    game->speed = speed;
    return 0;
}


extern unsigned int game_get_mapsize_x(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_mapsize_x","got NULL game");
        return 0;
    }
    return game->mapsize_x;
}


extern int game_set_mapsize_x(t_game * game, unsigned int x)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_mapsize_x","got NULL game");
        return -1;
    }
    game->mapsize_x = x;
    return 0;
}


extern unsigned int game_get_mapsize_y(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_mapsize_y","got NULL game");
        return 0;
    }
    return game->mapsize_y;
}


extern int game_set_mapsize_y(t_game * game, unsigned int y)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_mapsize_y","got NULL game");
        return -1;
    }
    game->mapsize_y = y;
    return 0;
}


extern unsigned int game_get_maxplayers(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_maxplayers","got NULL game");
        return 0;
    }
    return game->maxplayers;
}


extern int game_set_maxplayers(t_game * game, unsigned int maxplayers)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_maxplayers","got NULL game");
        return -1;
    }
    game->maxplayers = maxplayers;
    return 0;
}


extern unsigned int game_get_difficulty(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_difficulty","got NULL game");
        return 0;
    }
    return game->difficulty;
}


extern int game_set_difficulty(t_game * game, unsigned int difficulty)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_maxplayers","got NULL game");
        return -1;
    }
    game->difficulty = difficulty;
    return 0;
}


extern char const * game_get_description(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_description","got NULL game");
        return NULL;
    }
    return game->description;
}


extern int game_set_description(t_game * game, char const * description)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_description","got NULL game");
        return -1;
    }
    if (!description)
    {
	eventlog(eventlog_level_error,"game_set_description","got NULL description");
	return -1;
    }
    
    if (!(game->description = strdup(description)))
    {
	eventlog(eventlog_level_error,"game_set_description","could not allocate memory for description");
	return -1;
    }
    
    return 0;
}


extern char const * game_get_pass(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_pass","got NULL game");
        return NULL;
    }
    return game->pass;
}


extern char const * game_get_info(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_info","got NULL game");
        return NULL;
    }
    return game->info;
}


extern int game_get_startver(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_startver","got NULL game");
        return 0;
    }
    return game->startver;
}


extern unsigned long game_get_version(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_version","got NULL game");
        return 0;
    }
    return game->version;
}


extern unsigned int game_get_ref(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_ref","got NULL game");
        return 0;
    }
    return game->ref;
}


extern unsigned int game_get_count(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_count","got NULL game");
        return 0;
    }
    return game->count;
}


extern void game_set_status(t_game * game, t_game_status status)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_status","got NULL game");
        return;
    }
    if (status==game_status_started && game->start_time==(time_t)0)
	game->start_time = time(NULL);
    game->status = status;
#ifdef WITH_BITS
    bits_game_update_status(game_get_id(game),status);
#endif
}


extern t_game_status game_get_status(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_status","got NULL game");
        return 0;
    }
    return game->status;
}


extern unsigned int game_get_addr(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_addr","got NULL game");
        return 0;
    }
    
    return game->addr; /* host byte order */
}


extern unsigned short game_get_port(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_port","got NULL game");
        return 0;
    }
    
    return game->port; /* host byte order */
}


extern unsigned int game_get_latency(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_latency","got NULL game");
        return 0;
    }
    if (game->ref<1)
    {
	eventlog(eventlog_level_error,"game_get_latency","game \"%s\" has no players",game->name);
	return 0;
    }
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_get_latency","game \"%s\" has NULL players array (ref=%u)",game->name,game->ref);
	return 0;
    }
    if (!game->players[0])
    {
	eventlog(eventlog_level_error,"game_get_latency","game \"%s\" has NULL players[0] entry (ref=%u)",game->name,game->ref);
	return 0;
    }
    
    return 0; /* conn_get_latency(game->players[0]); */
}


extern char const * game_get_clienttag(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_clienttag","got NULL game");
        return NULL;
    }
    return game->clienttag;
}

#ifndef WITH_BITS
extern int game_add_player(t_game * game, char const * pass, int startver, t_connection * c)
#else
extern int game_add_player(t_game * game, char const * pass, int startver, unsigned int sessionid)
#endif
{
#ifndef WITH_BITS
    t_connection * * tempc;
#else
    unsigned int * tempc;
#endif
    t_account * *    tempp;
    t_game_result *  tempr;
    char const * *   temprh;
    char const * *   temprb;
#ifdef WITH_BITS
    t_bits_loginlist_entry * lle;
    t_account * tempa;
    char ct[5];
#endif	
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_add_player","got NULL game");
        return -1;
    }
#ifndef WITH_BITS
    /* BITS: Password should not be checked by the master server */
    if (!pass)
    {
	eventlog(eventlog_level_error,"game_add_player","got NULL password");
	return -1;
    }
#endif
    if (startver!=STARTVER_UNKNOWN && startver!=STARTVER_GW1 && startver!=STARTVER_GW3 && startver!=STARTVER_GW4 && startver!=STARTVER_REALM1)
    {
	eventlog(eventlog_level_error,"game_add_player","got bad game startver %d",startver);
	return -1;
    }
#ifndef WITH_BITS
    if (!c)
    {
	eventlog(eventlog_level_error,"game_add_player","got NULL connection");
        return -1;
    }
    if (game->type==game_type_ladder && account_get_normal_wins(conn_get_account(c),conn_get_clienttag(c))<10)
#else	
    if (!(lle = bits_loginlist_bysessionid(sessionid))) {
	eventlog(eventlog_level_error,"game_add_player","got invalid sessionid 0x%08x",sessionid);
	return -1;
    }

    ct[4]='\0';
    memcpy(ct,lle->clienttag,4);

    tempa = accountlist_find_account(lle->chatname);
    if (game->type==game_type_ladder && account_get_normal_wins(tempa,ct)<10)
#endif
    /* if () ... */
    {
	eventlog(eventlog_level_error,"game_add_player","can not join ladder game without 10 normal wins");
	return -1;
    }
    
    {
	char const * gt;
	
	if (!(gt = game_get_clienttag(game)))
	{
	    eventlog(eventlog_level_error,"game_add_player","could not get clienttag for game");
	    return -1;
	}
#ifndef WITH_BITS
/* FIXME: What's wrong with this?  *** I dunno, when does it print this message? */
/*
	if (strcmp(conn_get_clienttag(c),gt)!=0)
	{
	    eventlog(eventlog_level_error,"game_add_player","player clienttag (\"%s\") does not match game clienttag (\"%s\")",conn_get_clienttag(c),gt);
	    return -1;
	}
*/
#else
	if (strcmp(ct,gt)!=0)
	{
	    eventlog(eventlog_level_error,"game_add_player","player clienttag (\"%s\") does not match game clienttag (\"%s\")",ct,gt);
	    return -1;
	}
#endif
    }
    
#ifndef WITH_BITS
    /* BITS: Password is checked earlier */
    if (strcasecmp(game->pass,pass)!=0)
    {
        eventlog(eventlog_level_debug,"game_add_player","game \"%s\" password mismatch \"%s\"!=\"%s\"",game->name,game->pass,pass); 
	return -1;
    }

#endif
    
    if (!game->connections) /* some realloc()s are broken */
    {
#ifndef WITH_BITS
	if (!(tempc = malloc((game->count+1)*sizeof(t_connection *))))
#else
	if (!(tempc = malloc((game->count+1)*sizeof(unsigned int))))
#endif
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game connections");
	    return -1;
	}
    }
    else
    {
#ifndef WITH_BITS
	if (!(tempc = realloc(game->connections,(game->count+1)*sizeof(t_connection *))))
#else
	if (!(tempc = realloc(game->connections,(game->count+1)*sizeof(unsigned int))))
#endif
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game connections");
	    return -1;
	}
    }
    game->connections = tempc;
    if (!game->players) /* some realloc()s are broken */
    {
	if (!(tempp = malloc((game->count+1)*sizeof(t_account *)))) /* FIXME: BITS shouldn't use this */
	{                                                           /* Typhoon: since this function is only used by the master server it should be ok to use accounts */
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game players");
	    return -1;
	}
    }
    else
    {
	if (!(tempp = realloc(game->players,(game->count+1)*sizeof(t_account *))))
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game players");
	    return -1;
	}
    }
    game->players = tempp;
    
    if (!game->results) /* some realloc()s are broken */
    {
	if (!(tempr = malloc((game->count+1)*sizeof(t_game_result))))
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game results");
	    return -1;
	}
    }
    else
    {
	if (!(tempr = realloc(game->results,(game->count+1)*sizeof(t_game_result))))
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game results");
	    return -1;
	}
    }
    game->results = tempr;
    
    if (!game->report_heads) /* some realloc()s are broken */
    {
	if (!(temprh = malloc((game->count+1)*sizeof(char const *))))
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game report headers");
	    return -1;
	}
    }
    else
    {
	if (!(temprh = realloc((void *)game->report_heads,(game->count+1)*sizeof(char const *)))) /* avoid compiler warning */
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game report headers");
	    return -1;
	}
    }
    game->report_heads = temprh;
    
    if (!game->report_bodies) /* some realloc()s are broken */
    {
	if (!(temprb = malloc((game->count+1)*sizeof(char const *))))
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game report bodies");
	    return -1;
	}
    }
    else
    {
	if (!(temprb = realloc((void *)game->report_bodies,(game->count+1)*sizeof(char const *)))) /* avoid compiler warning */
	{
	    eventlog(eventlog_level_error,"game_add_player","unable to allocate memory for game report bodies");
	    return -1;
	}
    }
    game->report_bodies = temprb;
    
#ifndef WITH_BITS
    game->connections[game->count]   = c;
    game->players[game->count]       = conn_get_account(c);
#else
    game->connections[game->count]   = sessionid;
    game->players[game->count]       = tempa;
#endif
    game->results[game->count]       = game_result_none;
    game->report_heads[game->count]  = NULL;
    game->report_bodies[game->count] = NULL;
    
    game->count++;
    game->ref++;
    game->lastaccess_time = time(NULL);
    
    if (game->startver!=startver)
    {
#ifndef WITH_BITS	
	char const * tname;

	eventlog(eventlog_level_error,"game_add_player","player \"%s\" client \"%s\" startver %u joining game startver %u (count=%u ref=%u)",(tname = account_get_name(conn_get_account(c))),conn_get_clienttag(c),startver,game->startver,game->count,game->ref);
	account_unget_name(tname);
#else
	eventlog(eventlog_level_error,"game_add_player","player \"%s\" client \"%s\" startver %u joining game startver %u (count=%u ref=%u)",lle->chatname,ct,startver,game->startver,game->count,game->ref);
#endif
    }
    
    game_choose_host(game);
    
    return 0;
}

#ifndef WITH_BITS
extern int game_del_player(t_game * game, t_connection * c)
#else
extern int game_del_player(t_game * game, unsigned int sessionid)
#endif
{
    char const * tname;
    unsigned int i;
    t_account *  account;
#ifdef WITH_BITS
    t_bits_loginlist_entry * lle;
#endif
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_del_player","got NULL game");
        return -1;
    }
#ifndef WITH_BITS
    if (!c)
    {
	eventlog(eventlog_level_error,"game_del_player","got NULL connection");
	return -1;
    }
#endif
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_del_player","player array is NULL");
	return -1;
    }
    if (!game->results)
    {
	eventlog(eventlog_level_error,"game_del_player","results array is NULL");
	return -1;
    }
    
#ifndef WITH_BITS
    account = conn_get_account(c);
#else
    lle = bits_loginlist_bysessionid(sessionid);
    if (!lle) {
	eventlog(eventlog_level_error,"game_del_player","cannot find a loginlist entry with sessionid 0x%08x",sessionid);
	return -1;
    }
    account = accountlist_find_account(lle->chatname);
#endif
    
    for (i=0; i<game->count; i++)
	if (game->players[i]==account && game->connections[i])
	{
	    eventlog(eventlog_level_debug,"game_del_player","removing player #%u \"%s\" from \"%s\", %u players left",i,(tname = account_get_name(account)),game_get_name(game),game->ref-1);
#ifndef WITH_BITS
	    game->connections[i] = NULL;
#else
	    game->connections[i] = 0;
#endif	    
	    if (game->results[i]!=game_result_win &&
		game->results[i]!=game_result_loss &&
		game->results[i]!=game_result_draw &&
		game->results[i]!=game_result_disconnect &&
		game->results[i]!=game_result_observer &&
	        game->results[i]!=game_result_none)
		eventlog(eventlog_level_error,"game_del_player","player \"%s\" leaving with bad result %u",tname,game->results[i]);
	    account_unget_name(tname);
	    
	    eventlog(eventlog_level_debug,"game_del_player","player deleted... (ref=%u)",game->ref);
	    
	    if (game->ref<2)
	    {
	        eventlog(eventlog_level_debug,"game_del_player","no more players, reporting game");
		game_report(game);
	        eventlog(eventlog_level_debug,"game_del_player","no more players, destroying game");
		game_destroy(game);
		list_purge(gamelist_head);
	        return 0;
	    }
	    
	    game->ref--;
            game->lastaccess_time = time(NULL);
	    
	    game_choose_host(game);
	    
	    return 0;
	}
    
    eventlog(eventlog_level_error,"game_del_player","player \"%s\" was not in the game",(tname = account_get_name(account)));
    account_unget_name(tname);
    return -1;
}


extern int game_check_result(t_game * game, t_account * account, t_game_result result)
{
    unsigned int pos;
    char const * tname;
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_check_result","got NULL game");
	return -1;
    }
    if (!account)
    {
	eventlog(eventlog_level_error,"game_check_result","got NULL account");
	return -1;
    }
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_check_result","player array is NULL");
	return -1;
    }
    if (!game->results)
    {
	eventlog(eventlog_level_error,"game_check_result","results array is NULL");
	return -1;
    }
    if (result!=game_result_win &&
	result!=game_result_loss &&
	result!=game_result_draw &&
	result!=game_result_disconnect &&
	result!=game_result_observer &&
	result!=game_result_playing)
    {
	eventlog(eventlog_level_error,"game_check_result","got bad result %u for player \"%s\"",(unsigned int)result,(tname = account_get_name(account)));
	account_unget_name(tname);
	return -1;
    }
    
    {
	unsigned int i;
	
	pos = game->count;
	for (i=0; i<game->count; i++)
	    if (game->players[i]==account)
		pos = i;
    }
    if (pos==game->count)
    {
	eventlog(eventlog_level_error,"game_check_result","could not find player \"%s\" to check result",(tname = account_get_name(account)));
	account_unget_name(tname);
	game->bad = 1;
	return 0; /* return success even though it didn't check */
    }
    
    if (game->results[pos]==result)
    {
	eventlog(eventlog_level_debug,"game_check_result","result %u agrees with previous for player in slot %u",(unsigned int)result,pos);
	return 0;
    }
    if (game->results[pos]==game_result_none ||
	game->results[pos]==game_result_playing)
    {
	if (result==game_result_playing)
	    eventlog(eventlog_level_debug,"game_check_result","player in slot %u reported as \"still playing\", assuming that is right",pos);
	game->results[pos] = result;
	eventlog(eventlog_level_debug,"game_check_result","result %u initially obtained for player in slot %u",(unsigned int)result,pos);
	return 0;
    }
    
    eventlog(eventlog_level_error,"game_check_result","got inconsistent results for player in slot %u (\"%s\") previous=%u current=%u",pos,(tname = account_get_name(account)),(unsigned int)game->results[pos],(unsigned int)result);
    account_unget_name(tname);
    game->bad = 1;
    return 0; /* return success even though it didn't check */
}


extern int game_set_result(t_game * game, t_account * account, t_game_result result, char const * rephead, char const * repbody)
{
    unsigned int pos;
    char const * tname;
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_result","got NULL game");
	return -1;
    }
    if (!account)
    {
	eventlog(eventlog_level_error,"game_set_result","got NULL account");
	return -1;
    }
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_set_result","player array is NULL");
	return -1;
    }
    if (!game->results)
    {
	eventlog(eventlog_level_error,"game_set_result","results array is NULL");
	return -1;
    }
    if (!game->report_heads)
    {
	eventlog(eventlog_level_error,"game_set_result","report_heads array is NULL");
	return -1;
    }
    if (!game->report_bodies)
    {
	eventlog(eventlog_level_error,"game_set_result","report_bodies array is NULL");
	return -1;
    }
    if (result!=game_result_win &&
	result!=game_result_loss &&
	result!=game_result_draw &&
	result!=game_result_observer &&
	result!=game_result_disconnect)
    {
	eventlog(eventlog_level_error,"game_set_result","got bad result %u for player \"%s\"",(unsigned int)result,(tname = account_get_name(account)));
	account_unget_name(tname);
	return -1;
    }
    if (!rephead)
    {
	eventlog(eventlog_level_error,"game_set_result","report head is NULL");
	return -1;
    }
    if (!repbody)
    {
	eventlog(eventlog_level_error,"game_set_result","report body is NULL");
	return -1;
    }
    
    {
	unsigned int i;
	
	pos = game->count;
	for (i=0; i<game->count; i++)
	    if (game->players[i]==account)
		pos = i;
    }
    if (pos==game->count)
    {
	eventlog(eventlog_level_error,"game_set_result","could not find player \"%s\" to set result",(tname = account_get_name(account)));
	account_unget_name(tname);
	return -1;
    }
    
    if (!(game->report_heads[pos] = strdup(rephead)))
    {
	eventlog(eventlog_level_error,"game_set_result","could not allocate memory for report_heads in slot %u",pos);
	return -1;
    }
    if (!(game->report_bodies[pos] = strdup(repbody)))
    {
	eventlog(eventlog_level_error,"game_set_result","could not allocate memory for report_bodies in slot %u",pos);
	return -1;
    }
	
    if (game_check_result(game,account,result)<0)
    {
	eventlog(eventlog_level_error,"game_set_result","self-reported result did not check out with previous for player in slot %u",pos);
	return -1;
    }
    
    return 0;
}


extern t_game_result game_get_result(t_game * game, t_account * account)
{
    unsigned int pos;
    
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_result","got NULL game");
	return game_result_none;
    }
    if (!account)
    {
	eventlog(eventlog_level_error,"game_get_result","got NULL account");
	return game_result_none;
    }
    if (!game->players)
    {
	eventlog(eventlog_level_error,"game_get_result","player array is NULL");
	return game_result_none;
    }
    if (!game->results)
    {
	eventlog(eventlog_level_error,"game_get_result","results array is NULL");
	return game_result_none;
    }
    
    {
	unsigned int i;
	
	pos = game->count;
	for (i=0; i<game->count; i++)
	    if (game->players[i]==account)
		pos = i;
    }
    if (pos==game->count)
    {
	char const * tname;
	
	eventlog(eventlog_level_error,"game_get_result","could not find player \"%s\" to return result",(tname = account_get_name(account)));
	account_unget_name(tname);
	
	return game_result_none;
    }
    
    return game->results[pos];
}


extern char const * game_get_mapname(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_mapname","got NULL game");
	return NULL;
    }
    
    return game->mapname;
}


extern int game_set_mapname(t_game * game, char const * mapname)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_mapname","got NULL game");
	return -1;
    }
    if (!mapname)
    {
	eventlog(eventlog_level_error,"game_set_mapname","got NULL mapname");
	return -1;
    }
    
    if (!(game->mapname = strdup(mapname)))
    {
	eventlog(eventlog_level_error,"game_set_mapname","could not allocate memory for mapname");
	return -1;
    }
    
    return 0;
}


extern t_connection * game_get_owner(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_owner","got NULL game");
	return NULL;
    }
#ifdef WITH_BITS
    eventlog(eventlog_level_info,"game_get_owner","BITS: this function is broken");
    return NULL;
#else    
    return game->owner;
#endif
}


#ifdef WITH_BITS
extern void game_set_owner(t_game * game, unsigned int owner)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_owner","got NULL game");
        return;
    }
    game->owner = owner;
    bits_game_update_owner(game_get_id(game),owner);
}
#endif


extern time_t game_get_create_time(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_create_time","got NULL game");
	return (time_t)0;
    }
    
    return game->create_time;
}


extern time_t game_get_start_time(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_start_time","got NULL game");
	return (time_t)0;
    }
    
    return game->start_time;
}


extern int game_set_option(t_game * game, t_game_option option)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_set_option","got NULL game");
	return -1;
    }
    
    game->option = option;
    return 0;
}


extern t_game_option game_get_option(t_game const * game)
{
    if (!game)
    {
	eventlog(eventlog_level_error,"game_get_option","got NULL game");
	return game_option_none;
    }
    
    return game->option;
}


extern int gamelist_create(void)
{
    if (!(gamelist_head = list_create()))
	return -1;
    return 0;
}


extern int gamelist_destroy(void)
{
    /* FIXME: if called with active games, games are not freed */
    if (gamelist_head)
    {
	if (list_destroy(gamelist_head)<0)
	    return -1;
	gamelist_head = NULL;
    }
    
    return 0;
}


extern int gamelist_get_length(void)
{
    return list_get_length(gamelist_head);
}


extern t_game * gamelist_find_game(char const * name, t_game_type type)
{
    t_elem const * curr;
    t_game *       game;
    
    if (gamelist_head)
	LIST_TRAVERSE_CONST(gamelist_head,curr)
	{
	    game = elem_get_data(curr);
	    if ((type==game_type_all || game->type==type) && strcasecmp(name,game->name)==0)
		return game;
	}
    
    return NULL;
}


extern t_game * gamelist_find_game_byid(unsigned int id)
{
    t_elem const * curr;
    t_game *       game;
    
    if (gamelist_head)
	LIST_TRAVERSE_CONST(gamelist_head,curr)
	{
	    game = elem_get_data(curr);
	    if (game->id==id)
		return game;
	}
    
    return NULL;
}


extern t_list * gamelist(void)
{
    return gamelist_head;
}


extern int gamelist_total_games(void)
{
    return totalcount;
}

extern int game_set_realm(t_game * game, unsigned int realm) 
{ 
    if (!game)
    { 
          eventlog(eventlog_level_error,"game_set_realm","got NULL game"); 
          return -1; 
    } 
    game->realm = realm; 
    return 0;
} 
    
extern unsigned int game_get_realm(t_game const * game) 
{ 
    if (!game)
    { 
          eventlog(eventlog_level_error,"game_get_realm","got NULL game"); 
          return 0; 
    } 
    return game->realm; 
} 
    
extern int game_set_realmname(t_game * game, char const * realmname) 
{ 
    char const * temp; 
    
    if (!game)
    { 
           eventlog(eventlog_level_error,"game_set_realmname","got NULL game"); 
           return -1; 
    } 

    if (realmname)
      {
	if (!(temp=strdup(realmname)))
	  {
	    eventlog(eventlog_level_error,"game_set_realmname","could not allocate memory for new realmname");
	    return -1;
	  }
      }
    else
      temp=NULL; 

    if (game->realmname)
      free((void *)game->realmname); /* avoid warning */
    game->realmname = temp; 
    return 0; 
} 
    
extern  char const * game_get_realmname(t_game const * game) 
{ 
    if (!game)
    { 
           eventlog(eventlog_level_error,"game_get_realmname","got NULL game"); 
           return NULL; 
    } 
    return game->realmname; 
} 

extern void gamelist_check_voidgame(void) 
{ 
    t_elem const    * curr; 
    t_game          * game; 
    time_t          now; 
    
    now = time(NULL); 
    LIST_TRAVERSE_CONST(gamelist_head, curr) 
    { 
    	game = elem_get_data(curr); 
        if (!game)
            continue; 
        if (!game->realm)
            continue;
        if (game->ref >= 1)
            continue;
        if ((now - game->lastaccess_time) > MAX_GAME_EMPTY_TIME)
            game_destroy(game); 
    }
}

/*
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
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
#ifndef INCLUDED_LADDER_TYPES
#define INCLUDED_LADDER_TYPES

#ifdef LADDER_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# include "account.h"
#else
# define JUST_NEED_TYPES
# include "account.h"
# undef JUST_NEED_TYPES
#endif

#endif

typedef enum
{
    ladder_sort_highestrated,
    ladder_sort_mostwins,
    ladder_sort_mostgames
} t_ladder_sort;

typedef enum
{
    ladder_time_active,
    ladder_time_current
} t_ladder_time;

typedef enum
{
    ladder_id_normal=1,
    ladder_id_ironman=3
} t_ladder_id;

typedef enum
{
    ladder_option_none=0,
    ladder_option_disconnectisloss=1
} t_ladder_option;

#ifdef LADDER_INTERNAL_ACCESS
typedef struct
{
    char const *  clienttag;
    t_ladder_id   id;
    t_ladder_time ltime;
    t_account * * highestrated;
    t_account * * mostwins;
    t_account * * mostgames;
    unsigned int  len;
} t_ladder;
#endif

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_LADDER_PROTOS
#define INCLUDED_LADDER_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#include "game.h"
#include "ladder_calc.h"
#undef JUST_NEED_TYPES

extern int ladder_init_account(t_account * account, char const * clienttag, t_ladder_id id);
extern int ladder_check_map(char const * mapname, t_game_maptype maptype, char const * clienttag);

extern t_account * ladder_get_account_by_rank(unsigned int rank, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id);
extern unsigned int ladder_get_rank_by_account(t_account * account, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id);

extern int ladder_update(char const * clienttag, t_ladder_id id, unsigned int count, t_account * * players, t_game_result * results, t_ladder_info * info, t_ladder_option opns);

extern int ladderlist_create(void);
extern int ladderlist_destroy(void);
extern int ladderlist_make_all_active(void);

#endif
#endif

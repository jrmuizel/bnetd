/*
 * FIXME: Do we still need this file?
 *
 * Copyright (C) 2000  Marco Ziech (mmz@gmx.net)
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
#ifndef INCLUDED_GAME_RULE_TYPES
#define INCLUDED_GAME_RULE_TYPES

typedef enum
{
	rule_type_none = 0,
	rule_type_string = 1,
	rule_type_int = 2
} t_game_rule_type;

typedef struct {
	t_game_rule_type type;
	char const * name;
} t_rule_name;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_GAME_RULE_PROTOS
#define INCLUDED_GAME_RULE_PROTOS

#define JUST_NEED_TYPES
#include "game.h"
#undef JUST_NEED_TYPES

extern int game_rule_get_int(t_game const * game, char const * rule);
extern char const * game_rule_get_string(t_game const * game, char const * rule);
extern void game_rule_unget_string(char const * value);
extern int game_rule_set_string(t_game * game, char const * rule, char const * value);
extern int game_rule_set_string_const(t_game * game, char const * rule, char const * value);
extern int game_rule_set_int(t_game * game, char const * rule, int value);
extern t_rule_name const * game_rule_get_first(t_game const * game);
extern t_rule_name const * game_rule_get_next(t_game const * game);

#endif
#endif

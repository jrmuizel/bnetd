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
#define GAME_INTERNAL_ACCESS
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
#include "compat/strdup.h"
#include "game.h"
#include "common/eventlog.h"
#include "game_rule.h"
#include "common/setup_after.h"

static t_rule_name const native_rules[] = {
	{rule_type_int, "type"},
	{rule_type_int, "addr"},
	{rule_type_int, "port"},
	{rule_type_int, "version"},
	{rule_type_int, "status"},
	{rule_type_int, "ref"},
	{rule_type_int, "count"},
	{rule_type_int, "id"},
	{rule_type_int, "option"},
	{rule_type_int, "maptype"},
	{rule_type_int, "tileset"},
	{rule_type_int, "speed"},
	{rule_type_int, "mapsize_x"},
	{rule_type_int, "mapsize_y"},
	{rule_type_int, "maxplayers"},
	{rule_type_int, "create_time"},
	{rule_type_int, "start_time"},
	{rule_type_int, "bad"},
	{rule_type_int, "owner"},

	{rule_type_string, "name"},
	{rule_type_string, "pass"},
	{rule_type_string, "info"},
	{rule_type_string, "clienttag"},
	{rule_type_string, "mapname"},
	
	{rule_type_none, NULL}
};

extern int game_rule_get_int(t_game const * game, char const * rule)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_get_int","got NULL game");
		return -1;
	}
	if (!rule) {
		eventlog(eventlog_level_error,"game_rule_get_int","got NULL rule");
		return -1;
	}
	
	if (strcmp(rule,"type") == 0)
		return game->type;
	if (strcmp(rule,"addr") == 0)
		return game->addr;
	if (strcmp(rule,"port") == 0)
		return game->port;
	if (strcmp(rule,"startver") == 0)
		return game->startver;
	if (strcmp(rule,"version") == 0)
		return game->version;
	if (strcmp(rule,"status") == 0)
		return game->status;
	if (strcmp(rule,"ref") == 0)
		return game->ref;
	if (strcmp(rule,"count") == 0)
		return game->count;
	if (strcmp(rule,"id") == 0)
		return game->id;
	if (strcmp(rule,"option") == 0)
		return game->option;
	if (strcmp(rule,"maptype") == 0)
		return game->maptype;
	if (strcmp(rule,"tileset") == 0)
		return game->tileset;
	if (strcmp(rule,"speed") == 0)
		return game->speed;
	if (strcmp(rule,"mapsize_x") == 0)
		return game->mapsize_x;
	if (strcmp(rule,"mapsize_y") == 0)
		return game->mapsize_y;
	if (strcmp(rule,"maxplayers") == 0)
		return game->maxplayers;
	if (strcmp(rule,"create_time") == 0)
		return game->create_time;
	if (strcmp(rule,"start_time") == 0)
		return game->start_time;
	if (strcmp(rule,"bad") == 0)
		return game->bad;
	if (strcmp(rule,"owner") == 0)
		/* TODO sessionid */
		return 0;
	eventlog(eventlog_level_error,"game_rule_get_int","FIXME: custom game rules are not (yet?) supported");
	return 0;
}

extern char const * game_rule_get_string(t_game const * game, char const * rule)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_get_string","got NULL game");
		return NULL;
	}
	if (!rule) {
		eventlog(eventlog_level_error,"game_rule_get_string","got NULL rule");
		return NULL;
	}
	if (strcmp(rule,"name") == 0) {
		return strdup(game->name);
	} else if (strcmp(rule,"pass") == 0) {
		return strdup(game->pass);
	} else if (strcmp(rule,"info") == 0) {
		return strdup(game->info);
	} else if (strcmp(rule,"clienttag") == 0) {
		return strdup(game->clienttag);
	} else if (strcmp(rule,"mapname") == 0) {
		return strdup(game->mapname);
	} else {
		eventlog(eventlog_level_error,"game_rule_get_string","FIXME: custom game rules are not (yet?) supported");
		return 0;
	}
}

extern void game_rule_unget_string(char const * value)
{
	if (!value) {
		eventlog(eventlog_level_error,"game_rule_unget_string","got NULL value");
		return;
	}
	free((void *)value);
}

extern int game_rule_set_string(t_game * game, char const * rule, char const * value)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_set_string","got NULL game");
		return -1;
	}
	if (!rule) {
		eventlog(eventlog_level_error,"game_rule_set_string","got NULL rule");
		return -1;
	}
	if (!value) {
		eventlog(eventlog_level_error,"game_rule_set_string","got NULL value");
		return -1;
	}
	if (strcmp(rule,"name") == 0) {
		if (game->name) free((void *)game->name);
		game->name = strdup(value);
	} else if (strcmp(rule,"pass") == 0) {
		if (game->pass) free((void *)game->pass);
		game->pass = strdup(value);
	} else if (strcmp(rule,"info") == 0) {
		if (game->info) free((void *)game->info);
		game->info = strdup(value);
	} else if (strcmp(rule,"clienttag") == 0) {
		if (game->clienttag) free((void *)game->clienttag);
		game->clienttag = strdup(value);
	} else if (strcmp(rule,"mapname") == 0) {
		if (game->mapname) free((void *)game->mapname);
		game->mapname = strdup(value);
	} else {
		eventlog(eventlog_level_error,"game_rule_get_string","FIXME: custom game rules are not (yet?) supported");
		return 0;
	}
	return -1;
}

extern int game_rule_set_string_const(t_game * game, char const * rule, char const * value)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_set_string_const","got NULL game");
		return -1;
	}
	if (!rule) {
		eventlog(eventlog_level_error,"game_rule_set_string_const","got NULL rule");
		return -1;
	}
	if (!value) {
		eventlog(eventlog_level_error,"game_rule_set_string_const","got NULL value");
		return -1;
	}
	if (strcmp(rule,"name") == 0) {
		game->name = value;
	} else if (strcmp(rule,"pass") == 0) {
		game->pass = value;
	} else if (strcmp(rule,"info") == 0) {
		game->info = value;
	} else if (strcmp(rule,"clienttag") == 0) {
		game->clienttag = value;
	} else if (strcmp(rule,"mapname") == 0) {
		game->mapname = value;
	} else {
		eventlog(eventlog_level_error,"game_rule_get_string_const","FIXME: custom game rules are not (yet?) supported");
		return 0;
	}
	return -1;
}

extern int game_rule_set_int(t_game * game, char const * rule, int value)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_set_int","got NULL game");
		return -1;
	}
	if (!rule) {
		eventlog(eventlog_level_error,"game_rule_set_int","got NULL rule");
		return -1;
	}
	
	if (strcmp(rule,"type") == 0) {
		game->type = value;
	} else if (strcmp(rule,"addr") == 0) {
		game->addr = value;
	} else if (strcmp(rule,"port") == 0) {
		game->port = value;
	} else if (strcmp(rule,"startver") == 0) {
		game->startver = value;
	} else if (strcmp(rule,"version") == 0) {
		game->version= value;
	} else if (strcmp(rule,"status") == 0) {
		game->status = value;
	} else if (strcmp(rule,"ref") == 0) {
		game->ref = value;
	} else if (strcmp(rule,"count") == 0) { 
		game->count = value;
	} else if (strcmp(rule,"id") == 0) {
		game->id = value;
	} else if (strcmp(rule,"option") == 0) {
		game->option = value;
	} else if (strcmp(rule,"maptype") == 0) {
		game->maptype = value;
	} else if (strcmp(rule,"tileset") == 0) {
		game->tileset = value;
	} else if (strcmp(rule,"speed") == 0) {
		game->speed = value;
	} else if (strcmp(rule,"mapsize_x") == 0) {
		game->mapsize_x = value;
	} else if (strcmp(rule,"mapsize_y") == 0) {
		game->mapsize_y = value;
	} else if (strcmp(rule,"maxplayers") == 0) {
		game->maxplayers = value;
	} else if (strcmp(rule,"create_time") == 0) {
		game->create_time = value;
	} else if (strcmp(rule,"start_time") == 0) {
		game->start_time = value;
	} else if (strcmp(rule,"bad") == 0) {
		game->bad = value;
	} else if (strcmp(rule,"owner") == 0) {
		/* TODO sessionid */
	} else {
		eventlog(eventlog_level_error,"game_rule_set_int","FIXME: custom game rules are not (yet?) supported");
		return -1;
	}
	return 0;
}

/* These functions may need to be changed ... */
int current_rule;


/* I know the game param is not needed. I wanted to make
 * these functions game dependent, but didn't want want to
 * modify the t_game struct yet. --- Typhoon
 */

extern t_rule_name const * game_rule_get_first(t_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_get_first","got NULL game!");
		return NULL;
	}
	current_rule = 0;
	return &native_rules[0];
}

extern t_rule_name const * game_rule_get_next(t_game const * game)
{
	if (!game) {
		eventlog(eventlog_level_error,"game_rule_get_next","got NULL game!");
		return NULL;
	}
	if (native_rules[current_rule].type != rule_type_none) {
		current_rule++;
	} else {
		return NULL;
	}
	return &native_rules[current_rule];
}



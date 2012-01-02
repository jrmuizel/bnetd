/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_GAME_CONV_PROTOS
#define INCLUDED_GAME_CONV_PROTOS

#define JUST_NEED_TYPES
#include "game.h"
#undef JUST_NEED_TYPES

extern t_game_type bngreqtype_to_gtype(char const * clienttag, unsigned short bngtype) PURE_ATTR();
extern t_game_type bngtype_to_gtype(char const * clienttag, unsigned short bngtype) PURE_ATTR();
extern unsigned short gtype_to_bngtype(t_game_type gtype) CONST_ATTR();
extern t_game_option bngoption_to_goption(char const * clienttag, t_game_type gtype, unsigned short bngoption) PURE_ATTR();
extern t_game_result bngresult_to_gresult(unsigned int bngresult) CONST_ATTR();
extern t_game_maptype bngmaptype_to_gmaptype(unsigned int bngmaptype) CONST_ATTR();
extern t_game_tileset bngtileset_to_gtileset(unsigned int bngtileset) CONST_ATTR();
extern t_game_speed bngspeed_to_gspeed(unsigned int bngspeed) CONST_ATTR();
extern t_game_difficulty bngdifficulty_to_gdifficulty(unsigned int bngdifficulty) CONST_ATTR();
extern int game_parse_info(t_game * game, char const * gameinfo);

#endif
#endif

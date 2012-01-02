/*
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
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

#ifndef INCLUDED_BITS_GAME_TYPES
#define INCLUDED_BITS_GAME_TYPES

#ifdef JUST_NEED_TYPES
# include "common/bn_type.h"
# include "game.h"
#else
# define JUST_NEED_TYPES
# include "common/bn_type.h"
# include "game.h"
# undef JUST_NEED_TYPES
#endif

typedef struct {
	/* all strings are dynamically allocated to save memory */
	char const * name;	/* game name */
	char const * password;	/* game password */
	char const * info;	/* game info */
	unsigned int id;	/* unique id (see game->id) */
	t_game_type type;	/* game->type */
	t_game_status status;	/* current status */
	bn_int	clienttag;	/* game->clienttag */
	unsigned int owner;	/* game->owner */
} t_bits_game;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_GAME_PROTOS
#define INCLUDED_BITS_GAME_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

extern int send_bits_gamelist_add(t_connection * c, t_game const * game);
extern int send_bits_gamelist_add_bits(t_connection * c, t_bits_game const * game);
extern int send_bits_gamelist_del(t_connection * c, t_game const * game);
extern int bits_game_sendlist(t_connection * c);
extern int bits_game_update_status(unsigned int id, t_game_status status);
extern int bits_game_update_owner(unsigned int id, unsigned int owner);
extern int bits_game_update_option(unsigned int id, int option);
extern int bits_game_handle_client_gamelistreq(t_connection * conn, t_game_type gtype, int maxgames, char const * gamename);

/* The bits_game_handle_startgame* functions should only be called from handle_bnet_packet */
extern void bits_game_handle_startgame1(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status);
extern void bits_game_handle_startgame3(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status);
extern void bits_game_handle_startgame4(t_connection * c, char const * gamename, char const * gamepass, char const * gameinfo, unsigned short bngtype, unsigned int status, unsigned short option);

extern t_game * bits_game_create_temp(unsigned int gameid);
extern int bits_game_destroy_temp(t_game * game);

extern int send_bits_game_join_request(unsigned int sessionid, unsigned int gameid, int version);
extern int send_bits_game_leave(unsigned int sessionid, unsigned int gameid);
extern int send_bits_game_create_request(char const * gamename, char const * gamepass, char const * gameinfo, t_game_type type, int version, unsigned int sessionid, t_game_option option);
extern int send_bits_game_report(t_connection * c, t_packet const * packet);

extern t_game_type bits_game_get_type(t_bits_game const * game);
extern t_game_status bits_game_get_status(t_bits_game const * game);
extern int bits_game_set_status(t_bits_game * game, t_game_status status);
extern char const * bits_game_get_name(t_bits_game const * game);
extern char const * bits_game_get_info(t_bits_game const * game);
extern char const * bits_game_get_pass(t_bits_game const * game);
extern unsigned int bits_game_get_id(t_bits_game const * game);
extern unsigned int bits_game_get_owner(t_bits_game const * game);
extern unsigned int bits_game_get_addr(t_bits_game const * game);
extern unsigned short bits_game_get_port(t_bits_game const * game);

/* --- list stuff --- */
extern int bits_gamelist_create(void);
extern int bits_gamelist_destroy(void);
extern int bits_gamelist_get_length(void);
extern int bits_gamelist_add(t_bits_game * game);
extern int bits_gamelist_del(t_bits_game const * game);
extern t_bits_game * bits_gamelist_find_game(char const * name, t_game_type type);
extern t_bits_game * bits_gamelist_find_game_byid(unsigned int id);
extern t_list * bits_gamelist(void);

#endif
#endif

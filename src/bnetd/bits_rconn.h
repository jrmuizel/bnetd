/*
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

#ifndef INCLUDED_BITS_RCONN_TYPES
#define INCLUDED_BITS_RCONN_TYPES


#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_RCONN_PROTOS
#define INCLUDED_BITS_RCONN_PROTOS

#define JUST_NEED_TYPES
#include "connection.h"
#undef JUST_NEED_TYPES

extern t_connection * bits_rconn_create(unsigned int latency, unsigned int flags, unsigned int sessionid);
extern t_connection * bits_rconn_create_by_sessionid(unsigned int sessionid);
extern int bits_rconn_set_game(t_connection * c, t_game * g);
extern int bits_rconn_destroy(t_connection * c);

#endif
#endif


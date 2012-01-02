/*
 * Copyright (C) 2001  Marco Ziech (mmz@gmx.net)
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

#ifdef WITH_BITS

#ifndef INCLUDED_BITS_MOTD_TYPES
#define INCLUDED_BITS_MOTD_TYPES

#include "common/list.h"
#include "common/field_sizes.h"
#include "connection.h"

typedef char * t_bits_motd_line;

typedef struct {
    int numlines;
    t_list * lines;
} t_bits_motd;

extern t_bits_motd bits_motd; /* No, it's not a pointer :) */

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_MOTD_PROTOS
#define INCLUDED_BITS_MOTD_PROTOS

extern int bits_motd_load_from_file(void);
extern int bits_motd_load_from_packet(t_packet const * p);
extern int send_bits_net_motd(t_connection * c);
extern void bits_motd_send(t_connection * c);
extern void bits_motd_destroy(void);

#endif
#endif

#endif /* WITH_BITS */


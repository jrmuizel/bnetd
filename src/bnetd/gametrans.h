/*
 * Copyright (C) 2000  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_GAMETRANS_TYPES
#define INCLUDED_GAMETRANS_TYPES

#ifdef GAMETRANS_INTERNAL_ACCESS

#ifdef JUST_NEED_TYPES
# include "common/addr.h"
#else
# define JUST_NEED_TYPES
# include "common/addr.h"
# undef JUST_NEED_TYPES
#endif

typedef struct
{
    t_addr *    viewer;
    t_addr *    client;
    t_addr *    output;
    t_netaddr * exclude;
} t_gametrans;

#endif

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_GAMETRANS_PROTOS
#define INCLUDED_GAMETRANS_PROTOS

extern int gametrans_load(char const * filename);
extern int gametrans_unload(void);
extern void gametrans_net(unsigned int vaddr, unsigned short vport, unsigned int laddr, unsigned short lport, unsigned int * addr, unsigned short * port);

#endif
#endif

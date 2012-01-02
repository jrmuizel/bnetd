/*
 * bits_net.h - function for sending bits_net messages
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

#ifdef WITH_BITS

#ifndef INCLUDED_BITS_NET_TYPES
#define INCLUDED_BITS_NET_TYPES


#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_BITS_NET_PROTOS
#define INCLUDED_BITS_NET_PROTOS

extern int bits_net_send_discover(void);
extern int bits_net_send_undiscover(void);

#endif
#endif

#endif

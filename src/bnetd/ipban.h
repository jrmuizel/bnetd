/*
 * Copyright (C) 2000  Gediminas (gugini@fortas.ktu.lt)
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
#ifndef INCLUDED_IPBAN_TYPES
#define INCLUDED_IPBAN_TYPES

#ifdef IPBAN_INTERNAL_ACCESS
typedef enum
{
    ipban_type_exact,     /* 192.168.0.10              */
    ipban_type_wildcard,  /* 192.168.*.*               */
    ipban_type_range      /* 192.168.0.10-192.168.0.25 */
} t_ipban_type;

typedef struct ipban_entry_struct
{
    char *                      info1; /* third octet */
    char *                      info2; /* third octet */
    char *                      info3; /* third octet */
    char *                      info4; /* fourth octet */
    int                         type;
    struct ipban_entry_struct * next;
} t_ipban_entry;
#endif

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_IPBAN_PROTOS
#define INCLUDED_IPBAN_PROTOS

extern int ipban_check(char const * addr);

extern int ipbanlist_load(char const * filename);
extern int ipbanlist_unload(void);

#endif
#endif

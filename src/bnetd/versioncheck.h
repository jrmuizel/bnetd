/*
 * Copyright (C) 2000  Onlyer (onlyer@263.net)
 * Copyright (C) 2001  Ross Combs (ross@bnetd.org)
 * Copyright (C) 2002 Gianluigi Tiesi (sherpya@netfarm.it)
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
#ifndef INCLUDED_VERSIONCHECK_TYPES
#define INCLUDED_VERSIONCHECK_TYPES

#ifdef VERSIONCHECK_INTERNAL_ACCESS
typedef struct
{
    char const *  eqn;
    char const *  mpqfile;
    char const *  archtag;
    char const *  clienttag;
    char const *  versiontag;
    char const *  exeinfo;
    unsigned long versionid;
    unsigned long gameversion;
    unsigned long checksum;
} t_versioninfo;
#endif

typedef struct s_versioncheck
#ifdef VERSIONCHECK_INTERNAL_ACCESS
{
    char const * eqn;
    char const * mpqfile;
}
#endif
t_versioncheck;

#endif

#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_VERSIONCHECK_PROTOS
#define INCLUDED_VERSIONCHECK_PROTOS

extern t_versioncheck * versioncheck_create(char const * archtag, char const * clienttag);
extern int versioncheck_destroy(t_versioncheck * vc);
extern char const * versioncheck_get_mpqfile(t_versioncheck const * vc);
extern char const * versioncheck_get_eqn(t_versioncheck const * vc);
extern int versioncheck_validate(t_versioncheck const * vc, char const * archtag, char const * clienttag, char const * exeinfo, unsigned long versionid, unsigned long gameversion, unsigned long checksum, char const ** versiontag);

extern int versioncheck_load(char const * filename);
extern int versioncheck_unload(void);

#endif
#endif

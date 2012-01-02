/*
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifndef INCLUDED_ACCOUNT_TYPES
#define INCLUDED_ACCOUNT_TYPES

#ifdef ACCOUNT_INTERNAL_ACCESS
typedef struct attribute_struct
{
    char const *              key;
    char const *              val;
    struct attribute_struct * next;
} t_attribute;
#endif

#ifdef WITH_BITS
typedef enum
{
    account_state_invalid, /* account does not exist */
    account_state_delete,  /* account will be removed from cache */
    account_state_pending, /* account state is still unknown (request sent) */
    account_state_valid,   /* account is valid and locked (server receives notifications/changes for it) */
    account_state_unknown  /* account state is unknown and no request has been sent */
} t_bits_account_state;
#endif

typedef struct account_struct
#ifdef ACCOUNT_INTERNAL_ACCESS
{
    t_attribute * attrs;
    unsigned int  namehash; /* cached from attrs */
    unsigned int  uid;      /* cached from attrs */
    int           dirty;    /* 1==needs to be saved, 0==clean */
    int           loaded;   /* 1==loaded, 0==only on disk */
    int           accessed; /* 1==yes, 0==no */
    unsigned int  age; /* number of times it has not been accessed */
    char const *  filename; /* for BITS: NULL means it's a "virtual" account */
#ifdef WITH_BITS
    t_bits_account_state bits_state;
 /*   int           locked;    0==lock request not yet answered,
    				1==account exists & locked,
    				-1==no lock request sent,
    				2==invalid,
    				3==to be deleted (enum?) */
#endif
}
#endif
t_account;

#endif


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_ACCOUNT_PROTOS
#define INCLUDED_ACCOUNT_PROTOS

#define JUST_NEED_TYPES
#include "common/hashtable.h"
#undef JUST_NEED_TYPES


extern t_account * account_create(char const * username, char const * passhash1) MALLOC_ATTR();
extern t_account * create_vaccount(const char *username, unsigned int uid);
extern void account_destroy(t_account * account);
extern unsigned int account_get_uid(t_account const * account);
extern int account_match(t_account * account, char const * username);
extern int account_save(t_account * account, unsigned int delta);
#ifdef DEBUG_ACCOUNT
extern char const * account_get_strattr_real(t_account * account, char const * key, char const * fn, unsigned int ln);
#define account_get_strattr(A,K) account_get_strattr_real(A,K,__FILE__,__LINE__)
#else
extern char const * account_get_strattr(t_account * account, char const * key);
#endif
extern int account_unget_strattr(char const * val);
extern int account_set_strattr(t_account * account, char const * key, char const * val);
#ifdef WITH_BITS
extern int account_set_strattr_nobits(t_account * account, char const * key, char const * val);
extern int accountlist_remove_account(t_account const * account);
extern int account_name_is_unknown(char const * name);
extern int account_state_is_pending(t_account const * account);
extern int account_is_ready_for_use(t_account const * account);
extern int account_is_invalid(t_account const * account);
extern t_bits_account_state account_get_bits_state(t_account const * account);
extern int account_set_bits_state(t_account * account, t_bits_account_state state);
extern int account_set_accessed(t_account * account, int accessed);
extern int account_is_loaded(t_account const * account);
extern int account_set_loaded(t_account * account, int loaded);
extern int account_set_uid(t_account * account, int uid);
#endif

extern char const * account_get_first_key(t_account * account);
extern char const * account_get_next_key(t_account * account, char const * key);

extern int accountlist_create(void);
extern int accountlist_destroy(void);
extern t_hashtable * accountlist(void);
extern int accountlist_load_default(void);
extern void accountlist_unload_default(void);
extern int accountlist_get_length(void);
extern int accountlist_save(unsigned int delta);
extern t_account * accountlist_find_account(char const * username);
extern t_account * accountlist_add_account(t_account * account);

#endif
#endif

#include "account_wrap.h"

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
#define CHARACTER_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
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
# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif
#endif
#include "compat/memcpy.h"
#include "compat/strcasecmp.h"
#include <errno.h>
#include "compat/strerror.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "compat/uint.h"
#include "common/bnet_protocol.h"
#include "account.h"
#include "common/bn_type.h"
#include "common/util.h"
#include "character.h"
#include "common/setup_after.h"


static t_list * characterlist_head=NULL;


static t_character_class bncharacter_class_to_character_class(t_uint8 class)
{
    switch (class)
    {
    case D2CHAR_INFO_CLASS_AMAZON:
	return character_class_amazon;
    case D2CHAR_INFO_CLASS_SORCERESS:
	return character_class_sorceress;
    case D2CHAR_INFO_CLASS_NECROMANCER:
	return character_class_necromancer;
    case D2CHAR_INFO_CLASS_PALADIN:
	return character_class_paladin;
    case D2CHAR_INFO_CLASS_BARBARIAN:
	return character_class_barbarian;
    case D2CHAR_INFO_CLASS_DRUID:
        return character_class_druid;
    case D2CHAR_INFO_CLASS_ASSASSIN:
        return character_class_assassin;
    default:
	return character_class_none;
    }
}


static t_uint8 character_class_to_bncharacter_class(t_character_class class)
{
    switch (class)
    {
    case character_class_amazon:
	return D2CHAR_INFO_CLASS_AMAZON;
    case character_class_sorceress:
	return D2CHAR_INFO_CLASS_SORCERESS;
    case character_class_necromancer:
	return D2CHAR_INFO_CLASS_NECROMANCER;
    case character_class_paladin:
	return D2CHAR_INFO_CLASS_PALADIN;
    case character_class_barbarian:
	return D2CHAR_INFO_CLASS_BARBARIAN;
    case character_class_druid:
	return D2CHAR_INFO_CLASS_DRUID;
    case character_class_assassin:
	return D2CHAR_INFO_CLASS_ASSASSIN;
    default:
	eventlog(eventlog_level_error,"character_class_to_bncharacter_class","got unknown class %d",(int)class);
    case character_class_none:
	return D2CHAR_INFO_FILLER;
    }
}


static const char * character_class_to_classname (t_character_class class)
{
    switch (class)
    {
    case character_class_amazon:
        return "Amazon";
    case character_class_sorceress:
        return "Sorceress";
    case character_class_necromancer:
        return "Necromancer";
    case character_class_paladin:
        return "Paladin";
    case character_class_barbarian:
        return "Barbarian";
    case character_class_druid:
        return "Druid";
    case character_class_assassin:
        return "Assassin";
    default:
        return "Unknown";
    }
}


static const char * character_expansion_to_expansionname (t_character_expansion expansion)
{
    switch (expansion)
    {
    case character_expansion_classic:
	return "Classic";
    case character_expansion_lod:
        return "LordOfDestruction";
    default:
        return "Unknown";
    }
}


static void decode_character_data(t_character * ch)
{
    ch->unknownb1   = D2CHAR_INFO_UNKNOWNB1;
    ch->unknownb2   = D2CHAR_INFO_UNKNOWNB2;
    ch->helmgfx     = D2CHAR_INFO_FILLER;
    ch->bodygfx     = D2CHAR_INFO_FILLER;
    ch->leggfx      = D2CHAR_INFO_FILLER;
    ch->lhandweapon = D2CHAR_INFO_FILLER;
    ch->lhandgfx    = D2CHAR_INFO_FILLER;
    ch->rhandweapon = D2CHAR_INFO_FILLER;
    ch->rhandgfx    = D2CHAR_INFO_FILLER;
    ch->unknownb3   = D2CHAR_INFO_FILLER;
    ch->unknownb4   = D2CHAR_INFO_FILLER;
    ch->unknownb5   = D2CHAR_INFO_FILLER;
    ch->unknownb6   = D2CHAR_INFO_FILLER;
    ch->unknownb7   = D2CHAR_INFO_FILLER;
    ch->unknownb8   = D2CHAR_INFO_FILLER;
    ch->unknownb9   = D2CHAR_INFO_FILLER;
    ch->unknownb10  = D2CHAR_INFO_FILLER;
    ch->unknownb11  = D2CHAR_INFO_FILLER;
    ch->unknown1    = 0xffffffff;
    ch->unknown2    = 0xffffffff;
    ch->unknown3    = 0xffffffff;
    ch->unknown4    = 0xffffffff;
    ch->level       = 0x01;
    ch->status      = 0x80;
    ch->title       = 0x80;
    ch->unknownb13  = 0x80;
    ch->emblembgc   = 0x80;
    ch->emblemfgc   = 0xff;
    ch->emblemnum   = 0xff;
    ch->unknownb14  = D2CHAR_INFO_FILLER;

/*
b1 b2 hg bg lg lw lg rw rg b3 b4 b5 b6 b7 b8 b9 bA bB cl u1 u1 u1 u1 u2 u2 u2 u2 u3 u3 u3 u3 u4 u4 u4 u4 lv st ti bC eb ef en bD \0
amazon_qwer.log:
83 80 ff ff ff ff ff 43 ff 1b ff ff ff ff ff ff ff ff 01 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 01 80 80 80 80 ff ff ff 00
sor_Bent.log:
83 80 ff ff ff ff ff 53 ff ff ff ff ff ff ff ff ff ff 02 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 01 80 80 80 80 ff ff ff 00
necro_Thorsen.log:
83 80 ff ff ff ff ff 2b ff ff ff ff ff ff ff ff ff ff 03 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 01 80 80 80 80 ff ff ff 00
pal_QlexTEST.log:
87 80 01 01 01 01 01 ff ff ff 01 01 ff ff ff ff ff ff 04 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 01 80 80 ff ff ff 80 80 00
barb_Qlex.log:
83 80 ff ff ff ff ff 2f ff 1b ff ff ff ff ff ff ff ff 05 ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff 01 80 80 80 80 ff ff ff 00
*/
}


static int load_initial_data (t_character * character, t_character_class class, t_character_expansion expansion)
{
    char const * data_in_hex;

    eventlog(eventlog_level_debug,"load_initial_data","Initial Data for %s, %s %s",
	     character->name,
	     character_expansion_to_expansionname(expansion),
	     character_class_to_classname(class));

    /* Ideally, this would be loaded from bnetd_default_user, but I don't want to hack account.c just now */

    /* The "default" character info if everything else messes up; */
    data_in_hex = NULL; /* FIXME: what should we do if expansion or class isn't known... */

    switch (expansion)
    {
    case character_expansion_classic:
        switch (class)
	{
	case character_class_amazon:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 01 FF FF FF FF FF FF FF FF FF FF FF 01 81 80 80 80 FF FF FF";
	    break;
	case character_class_sorceress:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 02 FF FF FF FF FF FF FF FF FF FF FF 01 81 80 80 80 FF FF FF";
	    break;
	case character_class_necromancer:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 03 FF FF FF FF FF FF FF FF FF FF FF 01 81 80 80 80 FF FF FF";
	    break;
	case character_class_paladin:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 04 FF FF FF FF FF FF FF FF FF FF FF 01 81 80 80 80 FF FF FF";
	    break;
	case character_class_barbarian:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 05 FF FF FF FF FF FF FF FF FF FF FF 01 81 80 80 80 FF FF FF";
	    break;
	}
	break;
    case character_expansion_lod:
        switch (class)
	{
	case character_class_amazon:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 01 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_sorceress:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 02 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_necromancer:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 03 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_paladin:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 04 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_barbarian:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 05 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_druid:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 06 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	case character_class_assassin:
	    data_in_hex = "84 80 FF FF FF FF FF FF FF FF FF FF FF 07 FF FF FF FF FF FF FF FF FF FF FF 01 A1 80 80 80 FF FF FF";
	    break;
	}
    }

    character->datalen = hex_to_str(data_in_hex, character->data, 33);

    decode_character_data(character);
}


extern int character_create(t_account * account, char const * clienttag, char const * realmname, char const * name, t_character_class class, t_character_expansion expansion)
{
    t_character * ch;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"character_create","got NULL account");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"character_create","got bad clienttag");
	return -1;
    }
    if (!realmname)
    {
	eventlog(eventlog_level_error,"character_create","got NULL realmname");
	return -1;
    }
    if (!name)
    {
	eventlog(eventlog_level_error,"character_create","got NULL name");
	return -1;
    }
    
    if (!(ch = malloc(sizeof(t_character))))
    {
	eventlog(eventlog_level_error,"character_create","could not allocate memory for character");
	return -1;
    }
    if (!(ch->name = strdup(name)))
    {
	eventlog(eventlog_level_error,"character_create","could not allocate memory for name");
	free(ch);
	return -1;
    }
    if (!(ch->realmname = strdup(realmname)))
    {
	eventlog(eventlog_level_error,"character_create","could not allocate memory for realmname");
	free((void *)ch->name); /* avoid warning */
	free(ch);
	return -1;
    }
    if (!(ch->guildname = strdup(""))) /* FIXME: how does this work on Battle.net? */
    {
	eventlog(eventlog_level_error,"character_create","could not allocate memory for guildname");
	free((void *)ch->realmname); /* avoid warning */
	free((void *)ch->name); /* avoid warning */
	free(ch);
	return -1;
    }

    if (account_check_closed_character(account, clienttag, realmname, name))
    {
	eventlog(eventlog_level_error,"character_create","a character with the name \"%s\" does already exist in realm \"%s\"",name,realmname);
	free((void *)ch->realmname); /* avoid warning */
	free((void *)ch->name); /* avoid warning */
	free(ch);
	return -1;
    }

    load_initial_data (ch, class, expansion);

    account_add_closed_character(account, clienttag, ch);
    
    return 0;
}


extern char const * character_get_name(t_character const * ch)
{
    if (!ch)
    {
	eventlog(eventlog_level_error,"character_get_name","got NULL character");
	return NULL;
    }
    return ch->name;
}


extern char const * character_get_realmname(t_character const * ch)
{
    if (!ch)
    {
	eventlog(eventlog_level_error,"character_get_realmname","got NULL character");
	return NULL;
    }
    return ch->realmname;
}


extern t_character_class character_get_class(t_character const * ch)
{
    if (!ch)
    {
        eventlog(eventlog_level_error,"character_get_class","got NULL character");
        return character_class_none;
    }
    return bncharacter_class_to_character_class(ch->class);
}


extern char const * character_get_playerinfo(t_character const * ch)
{
    t_d2char_info d2char_info;
    static char   playerinfo[sizeof(t_d2char_info)+4];
    
    if (!ch)
    {
	eventlog(eventlog_level_error,"character_get_playerinfo","got NULL character");
	return NULL;
    }
    
/*
                                              ff 0f 68 00                ..h.
0x0040: 01 00 00 00 00 00 00 00   10 00 00 00 00 00 00 00    ................
0x0050: d8 94 f6 08 b1 65 77 02   65 76 69 6c 67 72 75 73    .....ew.evilgrus
0x0060: 73 6c 65 72 00 56 44 32   44 42 65 74 61 57 65 73    sler.VD2DBetaWes
0x0070: 74 2c 74 61 72 61 6e 2c   83 80 ff ff ff ff ff 2f    t,taran,......./
0x0080: ff ff ff ff ff ff ff ff   ff ff 03 ff ff ff ff ff    ................
0x0090: ff ff ff ff ff ff ff ff   ff ff ff 07 80 80 80 80    ................
0x00a0: ff ff ff 00

*/
    bn_byte_set(&d2char_info.unknownb1,ch->unknownb1);
    bn_byte_set(&d2char_info.unknownb2,ch->unknownb2);
    bn_byte_set(&d2char_info.helmgfx,ch->helmgfx);
    bn_byte_set(&d2char_info.bodygfx,ch->bodygfx);
    bn_byte_set(&d2char_info.leggfx,ch->leggfx);
    bn_byte_set(&d2char_info.lhandweapon,ch->lhandweapon);
    bn_byte_set(&d2char_info.lhandgfx,ch->lhandgfx);
    bn_byte_set(&d2char_info.rhandweapon,ch->rhandweapon);
    bn_byte_set(&d2char_info.rhandgfx,ch->rhandgfx);
    bn_byte_set(&d2char_info.unknownb3,ch->unknownb3);
    bn_byte_set(&d2char_info.unknownb4,ch->unknownb4);
    bn_byte_set(&d2char_info.unknownb5,ch->unknownb5);
    bn_byte_set(&d2char_info.unknownb6,ch->unknownb6);
    bn_byte_set(&d2char_info.unknownb7,ch->unknownb7);
    bn_byte_set(&d2char_info.unknownb8,ch->unknownb8);
    bn_byte_set(&d2char_info.unknownb9,ch->unknownb9);
    bn_byte_set(&d2char_info.unknownb10,ch->unknownb10);
    bn_byte_set(&d2char_info.unknownb11,ch->unknownb11);
    bn_byte_set(&d2char_info.class,ch->class);
    bn_int_set(&d2char_info.unknown1,ch->unknown1);
    bn_int_set(&d2char_info.unknown2,ch->unknown2);
    bn_int_set(&d2char_info.unknown3,ch->unknown3);
    bn_int_set(&d2char_info.unknown4,ch->unknown4);
    bn_byte_set(&d2char_info.level,ch->level);
    bn_byte_set(&d2char_info.status,ch->status); 
    bn_byte_set(&d2char_info.title,ch->title);
    bn_byte_set(&d2char_info.unknownb13,ch->unknownb13);
    bn_byte_set(&d2char_info.emblembgc,ch->emblembgc);
    bn_byte_set(&d2char_info.emblemfgc,ch->emblemfgc);
    bn_byte_set(&d2char_info.emblemnum,ch->emblemnum);
    bn_byte_set(&d2char_info.unknownb14,ch->unknownb14);
    
    memcpy(playerinfo,&d2char_info,sizeof(d2char_info));
    strcpy(&playerinfo[sizeof(d2char_info)],ch->guildname);
    
    return playerinfo;
}


extern char const * character_get_guildname(t_character const * ch)
{
    if (!ch)
    {
	eventlog(eventlog_level_error,"character_get_guildname","got NULL character");
	return NULL;
    }
    return ch->guildname;
}


extern int character_verify_charlist(t_character const * ch, char const * charlist)
{
    char *       temp;
    char const * tok1;
    char const * tok2;
    
    if (!ch)
    {
	eventlog(eventlog_level_error,"character_verify_charlist","got NULL character");
	return -1;
    }
    if (!charlist)
    {
	eventlog(eventlog_level_error,"character_verify_charlist","got NULL character");
	return -1;
    }
    
    if (!(temp = strdup(charlist)))
    {
	eventlog(eventlog_level_error,"character_verify_charlist","unable to allocate memory for characterlist");
        return -1;
    }
    
    tok1 = (char const *)strtok(temp,","); /* strtok modifies the string it is passed */
    tok2 = strtok(NULL,",");
    while (tok1)
    {
	if (!tok2)
	{
	    eventlog(eventlog_level_error,"character_verify_charlist","bad character list \"%s\"",temp);
	    break;
	}
	
	if (strcasecmp(tok1,ch->realmname)==0 && strcasecmp(tok2,ch->name)==0)
	{
	    free(temp);
	    return 0;
	}
	
        tok1 = strtok(NULL,",");
        tok2 = strtok(NULL,",");
    }
    free(temp);
    
    return -1;
}


extern int characterlist_create(char const * dirname)
{
    if (!(characterlist_head = list_create()))
        return -1;
    return 0;
}


extern int characterlist_destroy(void)
{
    t_elem *      curr;
    t_character * ch;
    
    if (characterlist_head)
    {
        LIST_TRAVERSE(characterlist_head,curr)
        {
            ch = elem_get_data(curr);
            if (!ch) /* should not happen */
            {
                eventlog(eventlog_level_error,"characterlist_destroy","characterlist contains NULL item");
                continue;
            }
            
            if (list_remove_elem(characterlist_head,curr)<0)
                eventlog(eventlog_level_error,"characterlist_destroy","could not remove item from list");
            free(ch);
        }

        if (list_destroy(characterlist_head)<0)
            return -1;
        characterlist_head = NULL;
    }
    
    return 0;
}


extern t_character * characterlist_find_character(char const * realmname, char const * charname)
{
    t_elem *      curr;
    t_character * ch;
    
    if (!realmname)
    {
	eventlog(eventlog_level_error,"characterlist_find_character","got NULL realmname");
	return NULL;
    }
    if (!charname)
    {
	eventlog(eventlog_level_error,"characterlist_find_character","got NULL charname");
	return NULL;
    }

    LIST_TRAVERSE(characterlist_head,curr)
    {
        ch = elem_get_data(curr);
        if (strcasecmp(ch->name,charname)==0 && strcasecmp(ch->realmname,realmname)==0)
            return ch;
    }
    
    return NULL;
}

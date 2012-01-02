/*
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
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
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "common/eventlog.h"
#include "common/util.h"
#include "common/bnettime.h"
#include "ladder.h"
#include "account.h"
#include "character.h"
#include "common/setup_after.h"


#ifdef DEBUG_ACCOUNT
extern unsigned int account_get_numattr_real(t_account * account, char const * key, char const * fn, unsigned int ln)
#else
extern unsigned int account_get_numattr(t_account * account, char const * key)
#endif
{
    char const * temp;
    unsigned int val;
    
    if (!account)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_get_numattr","got NULL account (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_get_numattr","got NULL account");
#endif
	return 0;
    }
    if (!key)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_get_numattr","got NULL key (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_get_numattr","got NULL key");
#endif
	return 0;
    }
    
    if (!(temp = account_get_strattr(account,key)))
	return 0;
    
    if (str_to_uint(temp,&val)<0)
    {
	eventlog(eventlog_level_error,"account_get_numattr","not a numeric string \"%s\" for key \"%s\"",temp,key);
	account_unget_strattr(temp);
	return 0;
    }
    account_unget_strattr(temp);
    
    return val;
}


extern int account_set_numattr(t_account * account, char const * key, unsigned int val)
{
    char temp[32]; /* should be more than enough room */
    
    if (!account)
    {
	eventlog(eventlog_level_error,"account_set_numattr","got NULL account");
	return -1;
    }
    if (!key)
    {
	eventlog(eventlog_level_error,"account_set_numattr","got NULL key");
	return -1;
    }
    
    sprintf(temp,"%u",val);
    return account_set_strattr(account,key,temp);
}


#ifdef DEBUG_ACCOUNT
extern int account_get_boolattr_real(t_account * account, char const * key, char const * fn, unsigned int ln)
#else
extern int account_get_boolattr(t_account * account, char const * key)
#endif
{
    char const * temp;
    
    if (!account)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_get_boolattr","got NULL account (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_get_boolattr","got NULL account");
#endif
	return -1;
    }
    if (!key)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_get_boolattr","got NULL key (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_get_boolattr","got NULL key");
#endif
	return -1;
    }
    
    if (!(temp = account_get_strattr(account,key)))
	return -1;
    
    switch (str_get_bool(temp))
    {
    case 1:
	account_unget_strattr(temp);
	return 1;
    case 0:
	account_unget_strattr(temp);
	return 0;
    default:
	account_unget_strattr(temp);
	eventlog(eventlog_level_error,"account_get_boolattr","bad boolean value \"%s\" for key \"%s\"",temp,key);
	return -1;
    }
}


extern int account_set_boolattr(t_account * account, char const * key, int val)
{
    if (!account)
    {
	eventlog(eventlog_level_error,"account_set_boolattr","got NULL account");
	return -1;
    }
    if (!key)
    {
	eventlog(eventlog_level_error,"account_set_boolattr","got NULL key");
	return -1;
    }
    
    return account_set_strattr(account,key,val?"true":"false");
}


/****************************************************************/


#ifdef DEBUG_ACCOUNT
extern char const * account_get_name_real(t_account * account, char const * fn, unsigned int ln)
#else
extern char const * account_get_name(t_account * account)
#endif
{
    char const * temp;
    
    if (!account)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_get_name","got NULL account (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_get_name","got NULL account");
#endif
	return NULL; /* FIXME: places assume this can't fail */
    }
    
    if (!(temp = account_get_strattr(account,"BNET\\acct\\username")))
	eventlog(eventlog_level_error,"account_get_name","account has no username");
    return temp;
}


#ifdef DEBUG_ACCOUNT
extern int account_unget_name_real(char const * name, char const * fn, unsigned int ln)
#else
extern int account_unget_name(char const * name)
#endif
{
    if (!name)
    {
#ifdef DEBUG_ACCOUNT
	eventlog(eventlog_level_error,"account_unget_name","got NULL name (from %s:%u)",fn,ln);
#else
	eventlog(eventlog_level_error,"account_unget_name","got NULL name");
#endif
	return -1;
    }
    
    return account_unget_strattr(name);
}


extern char const * account_get_pass(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\passhash1");
}


extern int account_set_pass(t_account * account, char const * passhash1)
{
    return account_set_strattr(account,"BNET\\acct\\passhash1",passhash1);
}


extern int account_unget_pass(char const * pass)
{
    return account_unget_strattr(pass);
}


/****************************************************************/


extern int account_get_auth_admin(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\admin");
}


extern int account_get_auth_announce(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\announce");
}


extern int account_get_auth_botlogin(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\botlogin");
}


extern int account_get_auth_bnetlogin(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\normallogin");
}


extern int account_get_auth_operator(t_account * account, char const * channelname)
{
    char temp[256];
    
    if (!channelname)
	return account_get_boolattr(account,"BNET\\auth\\operator");
    
    sprintf(temp,"BNET\\auth\\operator\\%.100s",channelname);
    return account_get_boolattr(account,temp);
}


extern int account_get_auth_changepass(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\changepass");
}


extern int account_get_auth_changeprofile(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\changeprofile");
}


extern int account_get_auth_createnormalgame(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\createnormalgame");
}


extern int account_get_auth_joinnormalgame(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\joinnormalgame");
}


extern int account_get_auth_createladdergame(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\createladdergame");
}


extern int account_get_auth_joinladdergame(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\joinladdergame");
}


extern int account_get_auth_lock(t_account * account)
{
    return account_get_boolattr(account,"BNET\\auth\\lock");
}


extern int account_set_auth_lock(t_account * account, int val)
{
    return account_set_boolattr(account,"BNET\\auth\\lock",val);
}




/****************************************************************/


extern char const * account_get_sex(t_account * account)
{
    char const * temp;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"account_get_sex","got NULL account");
	return NULL;
    }
    
    if (!(temp = account_get_strattr(account,"profile\\sex")))
	return "";
    return temp;
}


extern int account_unget_sex(char const * sex)
{
    return account_unget_strattr(sex);
}


extern char const * account_get_age(t_account * account)
{
    char const * temp;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"account_get_age","got NULL account");
	return NULL;
    }
    
    if (!(temp = account_get_strattr(account,"profile\\age")))
	return "";
    return temp;
}


extern int account_unget_age(char const * age)
{
    return account_unget_strattr(age);
}


extern char const * account_get_loc(t_account * account)
{
    char const * temp;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"account_get_loc","got NULL account");
	return NULL;
    }
    
    if (!(temp = account_get_strattr(account,"profile\\location")))
	return "";
    return temp;
}


extern int account_unget_loc(char const * loc)
{
    return account_unget_strattr(loc);
}


extern char const * account_get_desc(t_account * account)
{
    char const * temp;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"account_get_desc","got NULL account");
	return NULL;
    }
    
    if (!(temp = account_get_strattr(account,"profile\\description")))
	return "";
    return temp;
}


extern int account_unget_desc(char const * desc)
{
    return account_unget_strattr(desc);
}


/****************************************************************/


extern unsigned int account_get_fl_time(t_account * account)
{
    return account_get_numattr(account,"BNET\\acct\\firstlogin_time");
}


extern int account_set_fl_time(t_account * account, unsigned int t)
{
    return account_set_numattr(account,"BNET\\acct\\firstlogin_time",t);
}


extern char const * account_get_fl_clientexe(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_clientexe");
}


extern int account_unget_fl_clientexe(char const * clientexe)
{
    return account_unget_strattr(clientexe);
}


extern int account_set_fl_clientexe(t_account * account, char const * exefile)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_clientexe",exefile);
}


extern char const * account_get_fl_clientver(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_clientver");
}


extern int account_unget_fl_clientver(char const * clientver)
{
    return account_unget_strattr(clientver);
}


extern int account_set_fl_clientver(t_account * account, char const * version)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_clientver",version);
}


extern char const * account_get_fl_clienttag(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_clienttag");
}


extern int account_unget_fl_clienttag(char const * clienttag)
{
    return account_unget_strattr(clienttag);
}


extern int account_set_fl_clienttag(t_account * account, char const * clienttag)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_clienttag",clienttag);
}


extern unsigned int account_get_fl_connection(t_account * account)
{
    return account_get_numattr(account,"BNET\\acct\\firstlogin_connection");
}


extern int account_set_fl_connection(t_account * account, unsigned int connection)
{
    return account_set_numattr(account,"BNET\\acct\\firstlogin_connection",connection);
}


extern char const * account_get_fl_host(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_host");
}


extern int account_unget_fl_host(char const * host)
{
    return account_unget_strattr(host);
}


extern int account_set_fl_host(t_account * account, char const * host)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_host",host);
}


extern char const * account_get_fl_user(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_user");
}


extern int account_unget_fl_user(char const * user)
{
    return account_unget_strattr(user);
}


extern int account_set_fl_user(t_account * account, char const * user)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_user",user);
}


extern char const * account_get_fl_owner(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_owner");
}


extern int account_unget_fl_owner(char const * owner)
{
    return account_unget_strattr(owner);
}


extern int account_set_fl_owner(t_account * account, char const * owner)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_owner",owner);
}


extern char const * account_get_fl_cdkey(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\firstlogin_cdkey");
}


extern int account_unget_fl_cdkey(char const * cdkey)
{
    return account_unget_strattr(cdkey);
}


extern int account_set_fl_cdkey(t_account * account, char const * cdkey)
{
    return account_set_strattr(account,"BNET\\acct\\firstlogin_cdkey",cdkey);
}


/****************************************************************/


extern unsigned int account_get_ll_time(t_account * account)
{
    return account_get_numattr(account,"BNET\\acct\\lastlogin_time");
}


extern int account_set_ll_time(t_account * account, unsigned int t)
{
    return account_set_numattr(account,"BNET\\acct\\lastlogin_time",t);
}


extern char const * account_get_ll_clientexe(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_clientexe");
}


extern int account_unget_ll_clientexe(char const * clientexe)
{
    return account_unget_strattr(clientexe);
}


extern int account_set_ll_clientexe(t_account * account, char const * exefile)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_clientexe",exefile);
}


extern char const * account_get_ll_clientver(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_clientver");
}


extern int account_unget_ll_clientver(char const * clientver)
{
    return account_unget_strattr(clientver);
}


extern int account_set_ll_clientver(t_account * account, char const * version)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_clientver",version);
}


extern char const * account_get_ll_clienttag(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_clienttag");
}


extern int account_unget_ll_clienttag(char const * clienttag)
{
    return account_unget_strattr(clienttag);
}


extern int account_set_ll_clienttag(t_account * account, char const * clienttag)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_clienttag",clienttag);
}


extern unsigned int account_get_ll_connection(t_account * account)
{
    return account_get_numattr(account,"BNET\\acct\\lastlogin_connection");
}


extern int account_set_ll_connection(t_account * account, unsigned int connection)
{
    return account_set_numattr(account,"BNET\\acct\\lastlogin_connection",connection);
}


extern char const * account_get_ll_host(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_host");
}


extern int account_unget_ll_host(char const * host)
{
    return account_unget_strattr(host);
}


extern int account_set_ll_host(t_account * account, char const * host)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_host",host);
}


extern char const * account_get_ll_user(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_user");
}


extern int account_unget_ll_user(char const * user)
{
    return account_unget_strattr(user);
}


extern int account_set_ll_user(t_account * account, char const * user)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_user",user);
}


extern char const * account_get_ll_owner(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_owner");
}


extern int account_unget_ll_owner(char const * owner)
{
    return account_unget_strattr(owner);
}


extern int account_set_ll_owner(t_account * account, char const * owner)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_owner",owner);
}


extern char const * account_get_ll_cdkey(t_account * account)
{
    return account_get_strattr(account,"BNET\\acct\\lastlogin_cdkey");
}


extern int account_unget_ll_cdkey(char const * cdkey)
{
    return account_unget_strattr(cdkey);
}


extern int account_set_ll_cdkey(t_account * account, char const * cdkey)
{
    return account_set_strattr(account,"BNET\\acct\\lastlogin_cdkey",cdkey);
}


/****************************************************************/


extern unsigned int account_get_normal_wins(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_wins","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\0\\wins",clienttag);
    return account_get_numattr(account,key);
}


extern int account_inc_normal_wins(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_inc_normal_wins","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\wins",clienttag);
    return account_set_numattr(account,key,account_get_normal_wins(account,clienttag)+1);
}


extern unsigned int account_get_normal_losses(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_losses","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\0\\losses",clienttag);
    return account_get_numattr(account,key);
}


extern int account_inc_normal_losses(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_inc_normal_losses","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\losses",clienttag);
    return account_set_numattr(account,key,account_get_normal_losses(account,clienttag)+1);
}


extern unsigned int account_get_normal_draws(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_draws","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\0\\draws",clienttag);
    return account_get_numattr(account,key);
}


extern int account_inc_normal_draws(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_inc_normal_draws","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\draws",clienttag);
    return account_set_numattr(account,key,account_get_normal_draws(account,clienttag)+1);
}


extern unsigned int account_get_normal_disconnects(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_disconnects","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\0\\disconnects",clienttag);
    return account_get_numattr(account,key);
}


extern int account_inc_normal_disconnects(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_inc_normal_disconnects","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\disconnects",clienttag);
    return account_set_numattr(account,key,account_get_normal_disconnects(account,clienttag)+1);
}


extern int account_set_normal_last_time(t_account * account, char const * clienttag, t_bnettime t)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_last_time","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\last game",clienttag);
    return account_set_strattr(account,key,bnettime_get_str(t));
}


extern int account_set_normal_last_result(t_account * account, char const * clienttag, char const * result)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_last_result","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\0\\last game result",clienttag);
    return account_set_strattr(account,key,result);
}


/****************************************************************/


extern unsigned int account_get_ladder_active_wins(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_wins","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active wins",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_wins(t_account * account, char const * clienttag, t_ladder_id id, unsigned int wins)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_wins","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active wins",clienttag,(int)id);
    return account_set_numattr(account,key,wins);
}


extern unsigned int account_get_ladder_active_losses(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_losses","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active losses",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_losses(t_account * account, char const * clienttag, t_ladder_id id, unsigned int losses)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_losses","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active losses",clienttag,(int)id);
    return account_set_numattr(account,key,losses);
}


extern unsigned int account_get_ladder_active_draws(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_draws","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active draws",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_draws(t_account * account, char const * clienttag, t_ladder_id id, unsigned int draws)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_draws","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active draws",clienttag,(int)id);
    return account_set_numattr(account,key,draws);
}


extern unsigned int account_get_ladder_active_disconnects(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_disconnects","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active disconnects",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_disconnects(t_account * account, char const * clienttag, t_ladder_id id, unsigned int disconnects)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_disconnects","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active disconnects",clienttag,(int)id);
    return account_set_numattr(account,key,disconnects);
}


extern unsigned int account_get_ladder_active_rating(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_rating","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active rating",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_rating(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rating)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_rating","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active rating",clienttag,(int)id);
    return account_set_numattr(account,key,rating);
}


extern unsigned int account_get_ladder_active_rank(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_rank","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\active rank",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_active_rank(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rank)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_rank","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active rank",clienttag,(int)id);
    return account_set_numattr(account,key,rank);
}


extern char const * account_get_ladder_active_last_time(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_active_last_time","got bad clienttag");
	return NULL;
    }
    sprintf(key,"Record\\%s\\%d\\active last game",clienttag,(int)id);
    return account_get_strattr(account,key);
}


extern int account_set_ladder_active_last_time(t_account * account, char const * clienttag, t_ladder_id id, t_bnettime t)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_active_last_time","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\active last game",clienttag,(int)id);
    return account_set_strattr(account,key,bnettime_get_str(t));
}


/****************************************************************/


extern unsigned int account_get_ladder_wins(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_wins","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\wins",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_inc_ladder_wins(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_inc_ladder_wins","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\wins",clienttag,(int)id);
    return account_set_numattr(account,key,account_get_ladder_wins(account,clienttag,id)+1);
}


extern unsigned int account_get_ladder_losses(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_losses","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\losses",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_inc_ladder_losses(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
       eventlog(eventlog_level_error,"account_inc_ladder_losses","got bad clienttag");
       return -1;
    }
    sprintf(key,"Record\\%s\\%d\\losses",clienttag,(int)id);
    return account_set_numattr(account,key,account_get_ladder_losses(account,clienttag,id)+1);
}


extern unsigned int account_get_ladder_draws(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_draws","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\draws",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_inc_ladder_draws(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
       eventlog(eventlog_level_error,"account_inc_ladder_draws","got bad clienttag");
       return -1;
    }
    sprintf(key,"Record\\%s\\%d\\draws",clienttag,(int)id);
    return account_set_numattr(account,key,account_get_ladder_draws(account,clienttag,id)+1);
}


extern unsigned int account_get_ladder_disconnects(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_disconnects","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\disconnects",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_inc_ladder_disconnects(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
       eventlog(eventlog_level_error,"account_inc_ladder_disconnects","got bad clienttag");
       return -1;
    }
    sprintf(key,"Record\\%s\\%d\\disconnects",clienttag,(int)id);
    return account_set_numattr(account,key,account_get_ladder_disconnects(account,clienttag,id)+1);
}


extern unsigned int account_get_ladder_rating(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_rating","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\rating",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_adjust_ladder_rating(t_account * account, char const * clienttag, t_ladder_id id, int delta)
{
    char         key[256];
    unsigned int oldrating;
    unsigned int newrating;
    int          retval=0;
    
    if (!clienttag || strlen(clienttag)!=4)
    {
       eventlog(eventlog_level_error,"account_adjust_ladder_rating","got bad clienttag");
       return -1;
    }
    sprintf(key,"Record\\%s\\%d\\rating",clienttag,(int)id);
    /* don't allow rating to go below 1 */
    oldrating = account_get_ladder_rating(account,clienttag,id);
    if (delta<0 && oldrating<=(unsigned int)-delta)
        newrating = 1;
    else
        newrating = oldrating+delta;
    if (account_set_numattr(account,key,newrating)<0)
	retval = -1;
    
    if (newrating>account_get_ladder_high_rating(account,clienttag,id))
    {
	sprintf(key,"Record\\%s\\%d\\high rating",clienttag,(int)id);
	if (account_set_numattr(account,key,newrating)<0)
	    retval = -1;
    }
    
    return retval;
}


extern unsigned int account_get_ladder_rank(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_rank","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\rank",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_rank(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rank)
{
    char         key[256];
    unsigned int oldrank;
    int          retval=0;
    
    if (!clienttag || strlen(clienttag)!=4)
    {
       eventlog(eventlog_level_error,"account_set_ladder_rank","got bad clienttag");
       return -1;
    }
    if (rank==0)
        eventlog(eventlog_level_warn,"account_set_ladder_rank","setting rank to zero?");
    sprintf(key,"Record\\%s\\%d\\rank",clienttag,(int)id);
    if (account_set_numattr(account,key,rank)<0)
	retval = -1;
    
    oldrank = account_get_ladder_high_rank(account,clienttag,id);
    if (oldrank==0 || rank<oldrank)
    {
	sprintf(key,"Record\\%s\\%d\\high rank",clienttag,(int)id);
	if (account_set_numattr(account,key,rank)<0)
	    retval = -1;
    }
    
    return retval;
}


extern unsigned int account_get_ladder_high_rating(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_high_rating","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\high rating",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern unsigned int account_get_ladder_high_rank(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_high_rank","got bad clienttag");
	return 0;
    }
    sprintf(key,"Record\\%s\\%d\\high_rank",clienttag,(int)id);
    return account_get_numattr(account,key);
}


extern int account_set_ladder_last_time(t_account * account, char const * clienttag, t_ladder_id id, t_bnettime t)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_last_time","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\last game",clienttag,(int)id);
    return account_set_strattr(account,key,bnettime_get_str(t));
}


extern char const * account_get_ladder_last_time(t_account * account, char const * clienttag, t_ladder_id id)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_ladder_last_time","got bad clienttag");
	return NULL;
    }
    sprintf(key,"Record\\%s\\%d\\last game",clienttag,(int)id);
    return account_get_strattr(account,key);
}


extern int account_set_ladder_last_result(t_account * account, char const * clienttag, t_ladder_id id, char const * result)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_ladder_last_result","got bad clienttag");
	return -1;
    }
    sprintf(key,"Record\\%s\\%d\\last game result",clienttag,(int)id);
    return account_set_strattr(account,key,result);
}


/****************************************************************/


extern unsigned int account_get_normal_level(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_level","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\level",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_level(t_account * account, char const * clienttag, unsigned int level)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_level","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\level",clienttag);
    return account_set_numattr(account,key,level);
}


extern unsigned int account_get_normal_class(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_class","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\class",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_class(t_account * account, char const * clienttag, unsigned int class)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_class","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\class",clienttag);
    return account_set_numattr(account,key,class);
}


extern unsigned int account_get_normal_diablo_kills(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_diablo_kills","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\diablo kills",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_diablo_kills(t_account * account, char const * clienttag, unsigned int diablo_kills)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_diablo_kills","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\diablo kills",clienttag);
    return account_set_numattr(account,key,diablo_kills);
}


extern unsigned int account_get_normal_strength(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_strength","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\strength",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_strength(t_account * account, char const * clienttag, unsigned int strength)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_strength","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\strength",clienttag);
    return account_set_numattr(account,key,strength);
}


extern unsigned int account_get_normal_magic(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_magic","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\magic",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_magic(t_account * account, char const * clienttag, unsigned int magic)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_magic","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\magic",clienttag);
    return account_set_numattr(account,key,magic);
}


extern unsigned int account_get_normal_dexterity(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_dexterity","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\dexterity",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_dexterity(t_account * account, char const * clienttag, unsigned int dexterity)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_dexterity","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\dexterity",clienttag);
    return account_set_numattr(account,key,dexterity);
}


extern unsigned int account_get_normal_vitality(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_vitality","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\vitality",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_vitality(t_account * account, char const * clienttag, unsigned int vitality)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_vitality","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\vitality",clienttag);
    return account_set_numattr(account,key,vitality);
}


extern unsigned int account_get_normal_gold(t_account * account, char const * clienttag)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_normal_gold","got bad clienttag");
	return 0;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\gold",clienttag);
    return account_get_numattr(account,key);
}


extern int account_set_normal_gold(t_account * account, char const * clienttag, unsigned int gold)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_normal_gold","got bad clienttag");
	return -1;
    }
    sprintf(key,"BNET\\Record\\%s\\0\\gold",clienttag);
    return account_set_numattr(account,key,gold);
}


/****************************************************************/


extern int account_check_closed_character(t_account * account, char const * clienttag, char const * realmname, char const * charname)
{
    char const * charlist = account_get_closed_characterlist (account, clienttag, realmname);
    char         tempname[32];

    if (charlist == NULL)
    {
        eventlog(eventlog_level_debug,"account_check_closed_character","no characters in Realm %s",realmname);
    }
    else
    {
        char const * start;
	char const * next_char;
	int    list_len;
	int    name_len;
	int    i;

	eventlog(eventlog_level_debug,"account_check_closed_character","got characterlist \"%s\" for Realm %s",charlist,realmname);

	list_len  = strlen(charlist);
	start     = charlist;
	next_char = start;
	for (i = 0; i < list_len; i++, next_char++)
	{
	    if (',' == *next_char)
	    {
	        name_len = next_char - start;

	        strncpy(tempname, start, name_len);
		tempname[name_len] = '\0';

	        eventlog(eventlog_level_debug,"account_check_closed_character","found character \"%s\"",tempname);

		if (strcmp(tempname, charname) == 0)
		    return 1;

		start = next_char + 1;
	    }
	}

	name_len = next_char - start;

	strncpy(tempname, start, name_len);
	tempname[name_len] = '\0';

	eventlog(eventlog_level_debug,"account_check_closed_character","found tail character \"%s\"",tempname);

	if (strcmp(tempname, charname) == 0)
	    return 1;

	start = next_char + 1;
    }
    
    return 0;
}


extern char const * account_get_closed_characterlist(t_account * account, char const * clienttag, char const * realmname)
{
    char realmkey[256];
    int  realmkeylen;
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_get_closed_characterlist","got bad clienttag");
	return NULL;
    }

    if (!realmname)
    {
        eventlog(eventlog_level_error,"account_get_closed_characterlist","got NULL realmname");
	return NULL;
    }

    if (!account)
    {
        eventlog(eventlog_level_error,"account_get_closed_characterlist","got NULL account");
	return NULL;
    }

    sprintf(realmkey,"BNET\\CharacterList\\%s\\%s\\0",clienttag,realmname);
    realmkeylen=strlen(realmkey);
    eventlog(eventlog_level_debug,"account_get_closed_characterlist","looking for '%s'",realmkey);

    return account_get_strattr(account, realmkey);
}


extern int account_unget_closed_characterlist(t_account * account, char const * charlist)
{
    return account_unget_strattr(charlist);
}


extern int account_set_closed_characterlist(t_account * account, char const * clienttag, char const * charlist)
{
    char key[256];
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_set_closed_characterlist","got bad clienttag");
	return -1;
    }

    eventlog(eventlog_level_debug,"account_set_closed_characterlist, clienttag='%s', charlist='%s'",clienttag,charlist);

    sprintf(key,"BNET\\Characters\\%s\\0",clienttag);
    return account_set_strattr(account,key,charlist);
}

extern int account_add_closed_character(t_account * account, char const * clienttag, t_character * ch)
{
    char key[256];
    char hex_buffer[356];
    char chars_in_realm[256];
    char const * old_list;
    
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"account_add_closed_character","got bad clienttag");
	return -1;
    }

    if (!ch)
    {
	eventlog(eventlog_level_error,"account_add_closed_character","got NULL character");
	return -1;
    }

    eventlog(eventlog_level_debug,"account_add_closed_character","clienttag=\"%s\", realm=\"%s\", name=\"%s\"",clienttag,ch->realmname,ch->name);

    sprintf(key,"BNET\\CharacterList\\%s\\%s\\0",clienttag,ch->realmname);
    old_list = account_get_strattr(account, key);
    if (old_list)
    {
        sprintf(chars_in_realm, "%s,%s", old_list, ch->name);
    }
    else
    {
        sprintf(chars_in_realm, "%s", ch->name);
    }

    eventlog(eventlog_level_debug,"account_add_closed_character","new character list for realm \"%s\" is \"%s\"", ch->realmname, chars_in_realm);
    account_set_strattr(account, key, chars_in_realm);

    sprintf(key,"BNET\\Characters\\%s\\%s\\%s\\0",clienttag,ch->realmname,ch->name);
    str_to_hex(hex_buffer, ch->data, ch->datalen);
    account_set_strattr(account,key,hex_buffer);

    /*
    eventlog(eventlog_level_debug,"account_add_closed_character","key \"%s\"", key);
    eventlog(eventlog_level_debug,"account_add_closed_character","value \"%s\"", hex_buffer);
    */

    return 0;
}

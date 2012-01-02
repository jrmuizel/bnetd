/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 1999  Mark Baysinger (mbaysing@ucsd.edu)
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
#ifndef INCLUDED_PREFS_TYPES
#define INCLUDED_PREFS_TYPES

#ifdef PREFS_INTERNAL_ACCESS
typedef enum
{
    conf_type_none,
    conf_type_int,
    conf_type_char,
    conf_type_bool
} conf_type_t;

typedef struct
{
    char const * directive;
    conf_type_t  type;
    char const * defcharval;
    unsigned int defintval; /* shared with bool */
    char const * charval;
    unsigned int intval;  /* shared with bool */
} Bconf_t;
#endif

#endif


#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_PREFS_PROTOS
#define INCLUDED_PREFS_PROTOS

extern char const * preffile;

extern int prefs_load(char const * filename);
extern void prefs_unload(void);
extern char const * prefs_get_userdir(void) PURE_ATTR();
extern char const * prefs_get_filedir(void) PURE_ATTR();
extern char const * prefs_get_logfile(void) PURE_ATTR();
extern char const * prefs_get_loglevels(void) PURE_ATTR();
extern char const * prefs_get_defacct(void) PURE_ATTR();
extern char const * prefs_get_motdfile(void) PURE_ATTR();
extern char const * prefs_get_newsfile(void) PURE_ATTR();
extern char const * prefs_get_adfile(void) PURE_ATTR();
extern unsigned int prefs_get_user_sync_timer(void) PURE_ATTR();
extern unsigned int prefs_get_user_flush_timer(void) PURE_ATTR();
extern char const * prefs_get_servername(void) PURE_ATTR();
extern unsigned int prefs_get_track(void) PURE_ATTR();
extern char const * prefs_get_location(void) PURE_ATTR();
extern char const * prefs_get_description(void) PURE_ATTR();
extern char const * prefs_get_url(void) PURE_ATTR();
extern char const * prefs_get_contact_name(void) PURE_ATTR();
extern char const * prefs_get_contact_email(void) PURE_ATTR();
extern unsigned int prefs_get_latency(void) PURE_ATTR();
extern unsigned int prefs_get_irc_latency(void) PURE_ATTR();
extern unsigned int prefs_get_shutdown_delay(void) PURE_ATTR();
extern unsigned int prefs_get_shutdown_decr(void) PURE_ATTR();
extern unsigned int prefs_get_allow_new_accounts(void) PURE_ATTR();
extern unsigned int prefs_get_kick_old_login(void) PURE_ATTR();
extern char const * prefs_get_channelfile(void) PURE_ATTR();
extern unsigned int prefs_get_ask_new_channel(void) PURE_ATTR();
extern unsigned int prefs_get_hide_pass_games(void) PURE_ATTR();
extern unsigned int prefs_get_hide_started_games(void) PURE_ATTR();
extern unsigned int prefs_get_hide_temp_channels(void) PURE_ATTR();
extern unsigned int prefs_get_hide_addr(void) PURE_ATTR();
extern unsigned int prefs_get_enable_conn_all(void) PURE_ATTR();
extern unsigned int prefs_get_extra_commands(void) PURE_ATTR();
extern unsigned int prefs_get_udptest_port(void) PURE_ATTR();
extern char const * prefs_get_reportdir(void) PURE_ATTR();
extern unsigned int prefs_get_report_all_games(void) PURE_ATTR();
extern unsigned int prefs_get_report_diablo_games(void) PURE_ATTR();
extern char const * prefs_get_pidfile(void) PURE_ATTR();
extern char const * prefs_get_iconfile(void) PURE_ATTR();
extern char const * prefs_get_tosfile(void) PURE_ATTR();
extern char const * prefs_get_mpqauthfile(void) PURE_ATTR();
extern char const * prefs_get_mpqfile(void) PURE_ATTR();
extern char const * prefs_get_trackserv_addrs(void) PURE_ATTR();
extern char const * prefs_get_bnetdserv_addrs(void) PURE_ATTR();
extern char const * prefs_get_irc_addrs(void) PURE_ATTR();
extern unsigned int prefs_get_use_keepalive(void) PURE_ATTR();
extern unsigned int prefs_get_do_uplink(void) PURE_ATTR();
extern char const * prefs_get_uplink_server(void) PURE_ATTR();
extern unsigned int prefs_get_allow_uplink(void) PURE_ATTR();
extern char const * prefs_get_bits_password_file(void) PURE_ATTR();
extern char const * prefs_get_uplink_username(void) PURE_ATTR();
extern char const * prefs_get_ipbanfile(void) PURE_ATTR();
extern unsigned int prefs_get_bits_debug(void) PURE_ATTR();
extern unsigned int prefs_get_discisloss(void) PURE_ATTR();
extern char const * prefs_get_helpfile(void) PURE_ATTR();
extern char const * prefs_get_fortunecmd(void) PURE_ATTR();
extern char const * prefs_get_transfile(void) PURE_ATTR();
extern unsigned int prefs_get_chanlog(void) PURE_ATTR();
extern char const * prefs_get_chanlogdir(void) PURE_ATTR();
extern unsigned int prefs_get_quota(void) PURE_ATTR();
extern unsigned int prefs_get_quota_lines(void) PURE_ATTR();
extern unsigned int prefs_get_quota_time(void) PURE_ATTR();
extern unsigned int prefs_get_quota_wrapline(void) PURE_ATTR();
extern unsigned int prefs_get_quota_maxline(void) PURE_ATTR();
extern unsigned int prefs_get_ladder_init_rating(void) PURE_ATTR();
extern unsigned int prefs_get_quota_dobae(void) PURE_ATTR();
extern char const * prefs_get_realmfile(void) PURE_ATTR();
extern char const * prefs_get_issuefile(void) PURE_ATTR();
extern char const * prefs_get_bits_motd_file(void) PURE_ATTR();
extern unsigned int prefs_get_bits_use_plaintext(void) PURE_ATTR();
extern char const * prefs_get_effective_user(void) PURE_ATTR();
extern char const * prefs_get_effective_group(void) PURE_ATTR();
extern unsigned int prefs_get_nullmsg(void) PURE_ATTR();
extern unsigned int prefs_get_mail_support(void) PURE_ATTR();
extern unsigned int prefs_get_mail_quota(void) PURE_ATTR();
extern char const * prefs_get_maildir(void) PURE_ATTR();
extern char const * prefs_get_log_notice(void) PURE_ATTR();
extern unsigned int prefs_get_savebyname(void) PURE_ATTR();
extern unsigned int prefs_get_skip_versioncheck(void) PURE_ATTR();
extern unsigned int prefs_get_allow_bad_version(void) PURE_ATTR();
extern unsigned int prefs_get_allow_unknown_version(void) PURE_ATTR();
extern char const * prefs_get_versioncheck_file(void) PURE_ATTR();
extern unsigned int prefs_get_hashtable_size(void) PURE_ATTR();
extern char const * prefs_get_telnet_addrs(void) PURE_ATTR();

#endif
#endif

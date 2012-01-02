/*
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
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
#ifdef INCLUDED_SETUP_AFTER_H
# error "This file must be included before all other header files"
#endif
#ifndef INCLUDED_SETUP_BEFORE_H
#define INCLUDED_SETUP_BEFORE_H

/* get autoconf defines */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* This file contains compile-time configuration parameters including
 * debugging options, default configuration values, and format strings.
 */

/***************************************************************/
/* Debugging options */

/* print bad calling locations of account functions */
#define DEBUG_ACCOUNT

/* make bnchat, etc. print debug messages */
#define CLIENTDEBUG

/* make the routines call xxx_check() and notice
   xxx_purge() calls during list traversal */
#undef LIST_DEBUG
#undef HASHTABLE_DEBUG

/* do we want to track malloc() and free()? */
#undef USE_CHECK_ALLOC

/* this will use GCC evensions to verify that all module arguments to
   eventlog() are correct. */
#undef DEBUGMODSTRINGS
/*
   After you compile, use this script:
   strings bnetd |
   egrep '@\([^@]*@@[a-z_]*\)@' |
   sed -e 's/@(\([^:]*\):\([a-z_]*\)@@\([a-z_]*\))@/\1 \2 \3/g' |
   awk '{ if ( $2 != $3 ) printf("%s: %s->%s\n",$1,$2,$3); }'
*/

/* this will test get/unget memory management in account.c */
#undef TESTUNGET

/* check ladders for impossible combinations when returning them */
#define LADDER_DEBUG

/***************************************************************/
/* compile-time options */

/* how long do we wait for the UDP test reply? */
#define CLIENT_MAX_UDPTEST_WAIT 5

/* length of listen socket queue */
#define LISTEN_QUEUE 10

/* the format for account numbers */
#define UID_FORMAT "#%08u"
#define UID_MAXLEN 8

/* the format for game ids */
#define GAMEID_FORMAT "#%06u"
#define GAMEID_MAXLEN 8

/* the format of timestamps in the logfile */
#define EVENT_TIME_FORMAT "%b %d %H:%M:%S"
#define EVENT_TIME_MAXLEN 32

/* the format of the stat times in bnstat */
#define STAT_TIME_FORMAT "%Y %b %d %H:%M:%S"
#define STAT_TIME_MAXLEN 32

/* the format of the file modification time in bnftp */
#define FILE_TIME_FORMAT "%Y %b %d %H:%M:%S"
#define FILE_TIME_MAXLEN 32

/* the format of the dates in the game report and for /gameinfo */
#define GAME_TIME_FORMAT "%a %b %d %H:%M:%S %Z"
#define GAME_TIME_MAXLEN 32

/* the format of the timestamps for the start/end of channel log files */
#define CHANLOG_TIME_FORMAT "%Y %b %d %H:%M:%S %Z"
#define CHANLOG_TIME_MAXLEN 32

/* the format of the timestamps for lines in the channel log files */
#define CHANLOGLINE_TIME_FORMAT "%b %d %H:%M:%S"
#define CHANLOGLINE_TIME_MAXLEN 32

/* adjustable constants */
#define BNETD_LADDER_DEFAULT_TIME "19764578 0" /* 0:00 1 Jan 1970 GMT */

/* for clients if ioctl(TIOCGWINSZ) fails and $LINES and $COLUMNS aren't set */
#define DEF_SCREEN_WIDTH  80
#define DEF_SCREEN_HEIGHT 24

/***************************************************************/
/* default values for bnetd.conf */

/* default Boolean setup values */
#define BNETD_CHANLOG           0

/* default path configuration values */
#ifndef BNETD_DEFAULT_CONF_FILE
# define BNETD_DEFAULT_CONF_FILE "conf/bnetd.conf"
#endif
#define BNETD_FILE_DIR          "files"
#define BNETD_USER_DIR          "users"
#define BNETD_REPORT_DIR        "reports"
#define BNETD_LOG_FILE          "logs/bnetd.log"
#define BNETD_TEMPLATE_FILE     "conf/account_template.txt"
#define BNETD_MOTD_FILE         "conf/bnmotd.txt"
#define BNETD_NEWS_DIR          "news"
#define BNETD_AD_FILE           "conf/ad.conf"
#define BNETD_CHANNEL_FILE      "conf/channel.list"
#define BNETD_PID_FILE          ""  /* this means "none" */
#define BNETD_ACCOUNT_TMP       ".bnetd_acct_temp"
#define BNETD_IPBAN_FILE        "conf/bnban"
#define BNETD_HELP_FILE         "conf/bnhelpfile"
#define BNETD_FORTUNECMD        "/usr/games/fortune"
#define BNETD_TRANS_FILE        "conf/gametrans"
#define BNETD_CHANLOG_DIR       "chanlogs"
#define BNETD_REALM_FILE        "conf/realm.list"
#define BNETD_ISSUE_FILE        "conf/bnissue.txt"
#define BNETD_MAIL_DIR          "var/bnmail"
#define BNETD_VERSIONCHECK      "conf/versioncheck"
#define BITS_PASSWORD_FILE      "conf/bits_passwd"
#define BITS_MOTD_FILE          "conf/bits_motd"

/* default files relative to FILE_DIR */
#define BNETD_TOS_FILE     "tos.txt"
#define BNETD_ICON_FILE    "icons.bni"
#define BNETD_MPQ_FILE     "autoupdate"

/* other default configuration values */
#define BNETD_LOG_LEVELS      "warn,error"
#define BNETD_SERV_ADDRS      ":" /* this means "all" */
#define BNETD_SERV_PORT       6112
#define BNETD_IRC_ADDRS       "" /* this means none */
#define BNETD_IRC_PORT        6667 /* used if port not specified */
#define BNETD_TRACK_ADDRS     "track.bnetd.org"
#define BNETD_TRACK_PORT      6114 /* use this port if not specified */
#define BNETD_DEF_TEST_PORT   6112 /* default guess for UDP test port */
#define BNETD_MIN_TEST_PORT   6112
#define BNETD_MAX_TEST_PORT   6500
#define BNETD_USERSYNC        300 /* s */
#define BNETD_USERFLUSH       1000
#define BNETD_LATENCY         10 /* s */
#define BNETD_IRC_LATENCY     180 /* s */ /* Ping timeout for IRC connections */
#define BNETD_DEF_NULLMSG     120 /* s */
#define BNETD_TRACK_TIME      0
#define BNETD_POLL_INTERVAL   20 /* 20 ms */
#define BNETD_SHUTDELAY       300 /* s */
#define BNETD_SHUTDECR        60 /* s */
#define BNETD_DEFAULT_OWNER   "Bob"
#define BNETD_DEFAULT_KEY     "3310541526205"
#define BNETD_DEFAULT_HOST    "localhost"
#define BNETD_QUOTA_DOBAE     7 /* lines */
#define BNETD_QUOTA_LINES     5 /* lines */
#define BNETD_QUOTA_TIME      5 /* s */
#define BNETD_QUOTA_WLINE     40 /* chars */
#define BNETD_QUOTA_MLINE     200 /* chars */
#define BNETD_LADDER_INIT_RAT 1000
#define BNETD_MAIL_SUPPORT    0
#define BNETD_MAIL_QUOTA      5
#define BNETD_LOG_NOTICE      "*** Please note this channel is logged! ***"
#define BNETD_HASHTABLE_SIZE  61
#define BNETD_REALM_PORT      6113  /* where D2CS listens */
#define BNETD_TELNET_ADDRS    "" /* this means none */
#define BNETD_TELNET_PORT     23 /* used if port not specified */

/* BITS (uplink) defaults */
#define BITS_DO_UPLINK        0
#define BITS_UPLINK_SERVER    "localhost"
#define BITS_UPLINK_USERNAME  "anonymous"
#define BITS_ALLOW_UPLINK     0
#define BITS_DEBUG            1
#define BITS_USE_PLAINTEXT    0

/***************************************************************/
/* default values for the tracking server */
#define BNTRACKD_EXPIRE      600
#define BNTRACKD_UPDATE      150
#define BNTRACKD_GRANULARITY 5
#define BNTRACKD_SERVER_PORT 6114
#define BNTRACKD_PIDFILE     "" /* this means "none" */
#define BNTRACKD_OUTFILE     "bnetdlist.txt"
#define BNTRACKD_PROCESS     "scripts/process.pl"
#define BNTRACKD_LOGFILE     "logs/bntrackd.log"

/***************************************************************/
/* platform dependent features */

/* conditionally enabled features */

#if defined(HAVE_SIGACTION) && defined(HAVE_SIGPROCMASK) && defined(HAVE_SIGADDSET)
# define DO_POSIXSIG
#endif

#if defined(HAVE_FORK) && defined(HAVE_PIPE)
# define DO_SUBPROC
#endif

#if defined(HAVE_FORK) && defined(HAVE_CHDIR) && (defined(HAVE_SETPGID) || defined(HAVE_SETPGRP))
# define DO_DAEMONIZE
#endif


/* GCC attributes */

/* enable format mismatch warnings from gcc */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
# if __GNUC__ == 2 && __GNUC_MINOR__ < 7
/* namespace clean versions were available starting in 2.6.4 */
#  define __format__ format
#  define __printf__ printf
# endif
# define PRINTF_ATTR(FMTARG,VARG) __attribute__((__format__(printf,FMTARG,VARG)))
#else
# define PRINTF_ATTR(FMTARG,VARG)
#endif

/* returns a pointer which can not alias any other object */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 96) || __GNUC__ > 2)
# define MALLOC_ATTR() __attribute__((__malloc__))
#else
# define MALLOC_ATTR()
#endif

/* must only read arguments and non-volatile global variables and change nothing
   except for local variables and its own return value (and must eventually return) */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 96) || __GNUC__ > 2)
# define PURE_ATTR() __attribute__((__pure__))
#else
# define PURE_ATTR()
#endif

/* must only read direct arguments (no following pointers) and change nothing except
   for local variables and its own return value (and it must have a non-void return
   type and eventually return) */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
# define CONST_ATTR() __attribute__((__const__))
#else
# define CONST_ATTR()
#endif

/* must never return and have void return type */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 5) || __GNUC__ > 2)
# define NORETURN_ATTR() __attribute__((__noreturn__))
#else
# define NORETURN_ATTR()
#endif

/* type attributes */

/* set GCC machine storage mode */
#if defined(__GNUC__) && ((__GNUC__ == 2 && __GNUC_MINOR__ >= 7) || __GNUC__ > 2)
# define MODE_ATTR(M) __attribute__((__mode__(M)))
# define HAVE_MODE_ATTR
#else
# define MODE_ATTR(M)
#endif

/* avoid using padding */
#if defined(__GNUC__) /* FIXME: which gcc versions? */
# define PACKED_ATTR() __attribute__((__packed__))
#else
# define PACKED_ATTR()
#endif

#define BNETD_MAX_SOCKETS 4096

/*
 * select() hackery... works most places, need to add autoconf checks
 * because some systems may redefine FD_SETSIZE, have it as a variable,
 * or not have the concept of such a value.
 */
/* Win32 defaults to 64, BSD and Linux default to 1024 */
/* FIXME: how big can this be before things break? */
#ifndef FD_SETSIZE
# define FD_SETSIZE BNETD_MAX_SOCKETS
#endif

#endif

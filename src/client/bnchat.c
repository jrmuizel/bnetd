/*
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999  Oleg Drokin (green@ccssu.ccssu.crimea.ua)
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
#include "compat/exitstatus.h"
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strchr.h"
#include "compat/strdup.h"
#include <ctype.h>
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include "compat/termios.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SIGACTION
# include <signal.h>
# include "compat/signal.h"
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include "compat/socket.h"
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "compat/netinet_in.h"
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include "compat/inet_ntoa.h"
#include "compat/psock.h"
#include "common/packet.h"
#include "common/init_protocol.h"
#include "common/udp_protocol.h"
#include "common/bnet_protocol.h"
#include "common/file_protocol.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/network.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "ansi_term.h"
#include "common/version.h"
#include "common/util.h"
#ifdef CLIENTDEBUG
# include "common/eventlog.h"
#endif
#include "client.h"
#include "udptest.h"
#include "client_connect.h"
#include "common/setup_after.h"


#ifdef CLIENTDEBUG
# define dprintf printf
#else
# define dprintf if (0) printf
#endif

#define CHANNEL_STARCRAFT "Starcraft"
#define CHANNEL_BROODWARS "Brood War"
#define CHANNEL_SHAREWARE "Starcraft Shareware"
#define CHANNEL_DIABLORTL "Diablo Retail"
#define CHANNEL_DIABLOSHR "Diablo Shareware" /* FIXME: is this one right? */
#define CHANNEL_WARCIIBNE "War2BNE"
#define CHANNEL_DIABLO2DV "Diablo II"
#define CHANNEL_DIABLO2XP "Diablo II"


volatile int handle_winch=0;

typedef enum { mode_chat, mode_command, mode_waitstat, mode_waitgames, mode_waitladder, mode_gamename, mode_gamepass, mode_gamewait, mode_gamestop } t_mode;


static char const * mflags_get_str(unsigned int flags);
static char const * cflags_get_str(unsigned int flags);
static char const * mode_get_prompt(t_mode mode);
static int print_file(struct sockaddr_in * saddr, char const * filename, char const * progname, char const * clienttag);
static void usage(char const * progname);
#ifdef HAVE_SIGACTION
static void winch_sig_handle(int unused);
#endif


#ifdef HAVE_SIGACTION
static void winch_sig_handle(int unused)
{
    handle_winch = 1;
}
#endif


static char const * mflags_get_str(unsigned int flags)
{
    static char buffer[32];
    
    buffer[0]=buffer[1] = '\0';
    if (flags&MF_BLIZZARD)
	strcat(buffer,",Blizzard");
    if (flags&MF_GAVEL)
	strcat(buffer,",Gavel");
    if (flags&MF_VOICE)
	strcat(buffer,",Megaphone");
    if (flags&MF_BNET)
	strcat(buffer,",BNET");
    if (flags&MF_PLUG)
	strcat(buffer,",Plug");
    if (flags&MF_X)
	strcat(buffer,",X");
    if (flags&MF_SHADES)
	strcat(buffer,",Shades");
    if (flags&MF_PGLPLAY)
	strcat(buffer,",PGL_Player");
    if (flags&MF_PGLOFFL)
	strcat(buffer,",PGL_Official");
    buffer[0] = '[';
    strcat(buffer,"]");
    
    return buffer;
}


static char const * cflags_get_str(unsigned int flags)
{
    static char buffer[32];
    
    buffer[0]=buffer[1] = '\0';
    if (flags&CF_PUBLIC)
	strcat(buffer,",Public");
    if (flags&CF_MODERATED)
	strcat(buffer,",Moderated");
    if (flags&CF_RESTRICTED)
	strcat(buffer,",Restricted");
    if (flags&CF_THEVOID)
	strcat(buffer,",The Void");
    if (flags&CF_SYSTEM)
	strcat(buffer,",System");
    if (flags&CF_OFFICIAL)
	strcat(buffer,",Official");
    buffer[0] = '[';
    strcat(buffer,"]");
    
    return buffer;
}


static char const * mode_get_prompt(t_mode mode)
{
    switch (mode)
    {
    case mode_chat:
	return "] ";
    case mode_command:
	return "command> ";
    case mode_waitstat:
    case mode_waitgames:
    case mode_waitladder:
    case mode_gamewait:
	return "*please wait* ";
    case mode_gamename:
	return "Name: ";
    case mode_gamepass:
	return "Password: ";
    case mode_gamestop:
	return "[Return to kill game] ";
    default:
	return "? ";
    }
}


static int print_file(struct sockaddr_in * saddr, char const * filename, char const * progname, char const * clienttag)
{
    int          sd;
    t_packet *   ipacket;
    t_packet *   packet;
    t_packet *   rpacket;
    t_packet *   fpacket;
    unsigned int currsize;
    unsigned int filelen;
    
    if (!saddr || !filename || !progname)
	return -1;
    
    if ((sd = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_STREAM,0))<0)
    {
	fprintf(stderr,"%s: could not create socket (psock_socket: %s)\n",progname,strerror(psock_errno()));
	return -1;
    }
    
    if (psock_connect(sd,(struct sockaddr *)saddr,sizeof(*saddr))<0)
    {
	fprintf(stderr,"%s: could not connect to server (psock_connect: %s)\n",progname,strerror(psock_errno()));
	return -1;
    }
    
    if (!(ipacket = packet_create(packet_class_init)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	return -1;
    }
    bn_byte_set(&ipacket->u.client_initconn.class,CLIENT_INITCONN_CLASS_FILE);
    client_blocksend_packet(sd,ipacket);
    packet_del_ref(ipacket);
    
    if (!(rpacket = packet_create(packet_class_file)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	return -1;
    }
    
    if (!(fpacket = packet_create(packet_class_raw)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	return -1;
    }
    
    if (!(packet = packet_create(packet_class_file)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	return -1;
    }
    packet_set_size(packet,sizeof(t_client_file_req));
    packet_set_type(packet,CLIENT_FILE_REQ);
    bn_int_tag_set(&packet->u.client_file_req.archtag,ARCHTAG_X86);
    bn_int_tag_set(&packet->u.client_file_req.clienttag,clienttag);
    bn_int_set(&packet->u.client_file_req.adid,0);
    bn_int_set(&packet->u.client_file_req.extensiontag,0);
    bn_int_set(&packet->u.client_file_req.startoffset,0);
    bn_long_set_a_b(&packet->u.client_file_req.timestamp,0x00000000,0x00000000);
    packet_append_string(packet,filename);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    do
	if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed file connection\n",progname);
	    packet_del_ref(fpacket);
	    packet_del_ref(rpacket);
	    return -1;
	}
    while (packet_get_type(rpacket)!=SERVER_FILE_REPLY);
    
    filelen = bn_int_get(rpacket->u.server_file_reply.filelen);
    packet_del_ref(rpacket);
    
    for (currsize=0; currsize+MAX_PACKET_SIZE<=filelen; currsize+=MAX_PACKET_SIZE)
    {
	if (client_blockrecv_raw_packet(sd,fpacket,MAX_PACKET_SIZE)<0)
	{
	    fflush(stdout);
	    fprintf(stderr,"%s: server closed file connection\n",progname);
	    packet_del_ref(fpacket);
	    return -1;
	}
	str_print_term(stdout,packet_get_raw_data_const(fpacket,0),MAX_PACKET_SIZE,1);
    }
    filelen -= currsize;
    if (filelen)
    {
	if (client_blockrecv_raw_packet(sd,fpacket,filelen)<0)
	{
	    fflush(stdout);
	    fprintf(stderr,"%s: server closed file connection\n",progname);
	    packet_del_ref(fpacket);
	    return -1;
	}
	str_print_term(stdout,packet_get_raw_data_const(fpacket,0),filelen,1);
    }
    fflush(stdout);
    
    psock_close(sd);
    
    packet_del_ref(fpacket);
    
    return 0;
}


static void usage(char const * progname)
{
    fprintf(stderr,"usage: %s [<options>] [<servername> [<TCP portnumber>]]\n",progname);
    fprintf(stderr,
	    "    -a, --ansi-color            use ANSI colors\n"
            "    -n, --new-account           create a new account\n"
            "    -c, --change-password       change account password\n"
            "    -b, --client=SEXP           report client as Brood Wars\n"
            "    -d, --client=DRTL           report client as Diablo Retail\n"
            "    --client=DSHR               report client as Diablo Shareware\n");
    fprintf(stderr,
            "    -s, --client=STAR           report client as Starcraft (default)\n"
            "    --client=SSHR               report client as Starcraft Shareware\n"
            "    -w, --client=W2BN           report client as Warcraft II BNE\n"
            "    --client=D2DV               report client as Diablo II\n"
            "    --client=D2XP               report client as Diablo II : LoD\n");
    fprintf(stderr,
	    "    -o NAME, --owner=NAME       report CD owner as NAME\n"
	    "    -k KEY, --cdkey=KEY         report CD key as KEY\n"
            "    -h, --help, --usage         show this information and exit\n"
            "    -v, --version               print version number and exit\n");
    exit(STATUS_FAILURE);
}


extern int main(int argc, char * argv[])
{
    int                a;
    int                useansi=0;
    int                newacct=0;
    int                changepass=0;
    int                sd;
    struct sockaddr_in saddr;
    t_packet *         packet;
    t_packet *         rpacket;
    char const *       cdowner=NULL;
    char const *       cdkey=NULL;
    char const *       clienttag=NULL;
    char const *       channel=NULL;
    char const *       servname=NULL;
    unsigned short     servport=0;
    char               player[MAX_MESSAGE_LEN];
    char               text[MAX_MESSAGE_LEN];
    unsigned int       currsize;
    unsigned int       commpos;
    struct termios     in_attr_old;
    struct termios     in_attr_new;
    int                changed_in;
    unsigned int       sessionkey;
    unsigned int       sessionnum;
    int                fd_stdin;
    char const * *     channellist;
    t_mode             mode;
    unsigned int       statsmatch=24; /* any random number that is rare in uninitialized fields */
    unsigned int       screen_width,screen_height;
    int                munged;
    char               curr_gamename[GAME_NAME_LEN];
    char               curr_gamepass[GAME_PASS_LEN];
    
    if (argc<1 || !argv || !argv[0])
    {
	fprintf(stderr,"bad arguments\n");
	return STATUS_FAILURE;
    }
    
    for (a=1; a<argc; a++)
	if (servname && isdigit((int)argv[a][0]) && a+1>=argc)
	{
            if (str_to_ushort(argv[a],&servport)<0)
            {
                fprintf(stderr,"%s: \"%s\" should be a positive integer\n",argv[0],argv[a]);
                usage(argv[0]);
            }
	}
	else if (!servname && argv[a][0]!='-' && a+2>=argc)
	    servname = argv[a];
        else if (strcmp(argv[a],"-a")==0 || strcmp(argv[a],"--use-ansi")==0)
	    useansi = 1;
        else if (strcmp(argv[a],"-n")==0 || strcmp(argv[a],"--new-account")==0)
	{
	    if (changepass)
	    {
		fprintf(stderr,"%s: can not create new account when changing passwords\n",argv[0]);
		usage(argv[0]);
	    }
	    newacct = 1;
	}
        else if (strcmp(argv[a],"-c")==0 || strcmp(argv[a],"--change-password")==0)
	{
	    if (newacct)
	    {
		fprintf(stderr,"%s: can not change passwords when creating a new account\n",argv[0]);
		usage(argv[0]);
	    }
	    changepass = 1;
	}
        else if (strcmp(argv[a],"-b")==0 || strcmp(argv[a],"--client=SEXP")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_BROODWARS;
	    channel = CHANNEL_BROODWARS;
	}
        else if (strcmp(argv[a],"-d")==0 || strcmp(argv[a],"--client=DRTL")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLORTL;
	    channel = CHANNEL_DIABLORTL;
	}
        else if (strcmp(argv[a],"--client=DSHR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLOSHR;
	    channel = CHANNEL_DIABLOSHR;
	}
        else if (strcmp(argv[a],"-s")==0 || strcmp(argv[a],"--client=STAR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_STARCRAFT;
	    channel = CHANNEL_STARCRAFT;
	}
        else if (strcmp(argv[a],"--client=SSHR")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_SHAREWARE;
	    channel = CHANNEL_SHAREWARE;
	}
        else if (strcmp(argv[a],"-w")==0 || strcmp(argv[a],"--client=W2BN")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_WARCIIBNE;
	    channel = CHANNEL_WARCIIBNE;
	}
        else if (strcmp(argv[a],"--client=D2DV")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLO2DV;
	    channel = CHANNEL_DIABLO2DV;
	}
        else if (strcmp(argv[a],"--client=D2XP")==0)
	{
	    if (clienttag)
	    {
		fprintf(stderr,"%s: client type was already specified as \"%s\"\n",argv[0],clienttag);
		usage(argv[0]);
	    }
	    clienttag = CLIENTTAG_DIABLO2XP;
	    channel = CHANNEL_DIABLO2XP;
	}
	else if (strncmp(argv[a],"--client=",9)==0)
	{
	    fprintf(stderr,"%s: unknown client tag \"%s\"\n",argv[0],&argv[a][9]);
	    usage(argv[0]);
	}
	else if (strcmp(argv[a],"-o")==0)
	{
	    if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
	    if (cdowner)
	    {
		fprintf(stderr,"%s: CD owner was already specified as \"%s\"\n",argv[0],cdowner);
		usage(argv[0]);
	    }
	    cdowner = argv[++a];
	}
	else if (strncmp(argv[a],"--owner=",8)==0)
	{
	    if (cdowner)
	    {
		fprintf(stderr,"%s: CD owner was already specified as \"%s\"\n",argv[0],cdowner);
		usage(argv[0]);
	    }
	    cdowner = &argv[a][8];
	}
	else if (strcmp(argv[a],"-k")==0)
	{
	    if (a+1>=argc)
            {
                fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
                usage(argv[0]);
            }
	    if (cdkey)
	    {
		fprintf(stderr,"%s: CD key was already specified as \"%s\"\n",argv[0],cdkey);
		usage(argv[0]);
	    }
	    cdkey = argv[++a];
	}
	else if (strncmp(argv[a],"--cdkey=",8)==0)
	{
	    if (cdkey)
	    {
		fprintf(stderr,"%s: CD key was already specified as \"%s\"\n",argv[0],cdkey);
		usage(argv[0]);
	    }
	    cdkey = &argv[a][8];
	}
	else if (strcmp(argv[a],"-v")==0 || strcmp(argv[a],"--version")==0)
	{
            printf("version "BNETD_VERSION"\n");
            return STATUS_SUCCESS;
	}
	else if (strcmp(argv[a],"-h")==0 || strcmp(argv[a],"--help")==0 || strcmp(argv[a],"--usage")
==0)
            usage(argv[0]);
        else if (strcmp(argv[a],"--client")==0 || strcmp(argv[a],"--owner")==0 || strcmp(argv[a],"--cdkey")==0)
	{
	    fprintf(stderr,"%s: option \"%s\" requires an argument\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
	else
	{
	    fprintf(stderr,"%s: unknown option \"%s\"\n",argv[0],argv[a]);
	    usage(argv[0]);
	}
    
    if (servport==0)
	servport = BNETD_SERV_PORT;
    if (!cdowner)
	cdowner = BNETD_DEFAULT_OWNER;
    if (!cdkey)
	cdkey = BNETD_DEFAULT_KEY;
    if (!clienttag)
    {
	clienttag = CLIENTTAG_STARCRAFT;
	channel = CHANNEL_STARCRAFT;
    }
    if (!servname)
	servname = BNETD_DEFAULT_HOST;
    
    fd_stdin = fileno(stdin);
    if (tcgetattr(fd_stdin,&in_attr_old)>=0)
    {
        in_attr_new = in_attr_old;
        in_attr_new.c_lflag &= ~(ECHO | ICANON); /* turn off ECHO and ICANON */
	in_attr_new.c_cc[VMIN]  = 0; /* don't require reads to return data */
        in_attr_new.c_cc[VTIME] = 1; /* timeout = .1 seconds */
        tcsetattr(fd_stdin,TCSANOW,&in_attr_new);
        changed_in = 1;
    }
    else
    {
	fprintf(stderr,"%s: could not set terminal attributes for stdin\n",argv[0]);
	changed_in = 0;
    }
    
#ifdef HAVE_SIGACTION
    {
        struct sigaction winch_action;
	
        winch_action.sa_handler = winch_sig_handle;
        sigemptyset(&winch_action.sa_mask);
        winch_action.sa_flags = SA_RESTART;
	
        sigaction(SIGWINCH,&winch_action,NULL);
    }
#endif


    if (client_get_termsize(fd_stdin,&screen_width,&screen_height)<0)
    {
	fprintf(stderr,"%s: could not determine screen size\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    if (useansi)
    {
	ansi_text_reset();
	ansi_screen_clear();
	ansi_cursor_move_home();
	fflush(stdout);
    }
    
    if ((sd = client_connect(argv[0],
			     servname,servport,cdowner,cdkey,clienttag,
			     &saddr,&sessionkey,&sessionnum,ARCHTAG_X86))<0)
    {
	fprintf(stderr,"%s: fatal error during handshake\n",argv[0]);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    /* reuse this same packet over and over */
    if (!(rpacket = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_set_size(packet,sizeof(t_client_tosreq));
    packet_set_type(packet,CLIENT_TOSREQ);
    bn_int_set(&packet->u.client_tosreq.type,CLIENT_TOSREQ_TOSFILE);
    bn_int_set(&packet->u.client_tosreq.unknown2,CLIENT_TOSREQ_UNKNOWN2);
    
    if ((clienttag == CLIENTTAG_DIABLO2DV) || (clienttag == CLIENTTAG_DIABLO2XP))
    {
        packet_append_string(packet,CLIENT_TOSREQ_FILETOSUNICODEUSA);
    }
    else
    {
        packet_append_string(packet,CLIENT_TOSREQ_FILETOSUSA);
    }
    
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	   fprintf(stderr,"%s: server closed connection\n",argv[0]);
	   psock_close(sd);
	   if (changed_in)
	       tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	   return STATUS_FAILURE;
	}
    while (packet_get_type(rpacket)!=SERVER_TOSREPLY);
    
    /* real client would also send statsreq on past logins here */
    
    printf("----\n");
    if (newacct)
    {
	if (useansi)
	    ansi_text_color_fore(ansi_text_color_red);
	printf("###### Terms Of Service ######\n");
	print_file(&saddr,packet_get_str_const(rpacket,sizeof(t_server_tosreply),1024),argv[0],clienttag);
	printf("##############################\n\n");
	if (useansi)
	    ansi_text_reset();
    }
	    
    for (;;)
    {
        char         password[MAX_MESSAGE_LEN];
	unsigned int i;
	int          status;
        
	if (newacct)
	{
	    char   passwordvrfy[MAX_MESSAGE_LEN];
            t_hash passhash1;
	    
	    printf("Enter information for your new account\n");
	    munged = 1;
	    commpos = 0;
	    player[0] = '\0';
	    while ((status = client_get_comm("Username: ",player,MAX_MESSAGE_LEN,&commpos,1,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    if (strchr(player,' ')  || strchr(player,'\t') ||
		strchr(player,'\r') || strchr(player,'\n') )
	    {
		printf("Spaces are not allowed in usernames. Try again.\n");
		continue;
	    }
	    /* we could use strcspn() but it doesn't exist everywhere */
	    if (strchr(player,'#')  ||
		strchr(player,'%')  ||
		strchr(player,'&')  ||
		strchr(player,'*')  ||
		strchr(player,'\\') ||
		strchr(player,'"')  ||
		strchr(player,',')  ||
		strchr(player,'<')  ||
		strchr(player,'/')  ||
		strchr(player,'>')  ||
		strchr(player,'?'))
	    {
		printf("The special characters #%%&*\\\",</>? are allowed in usernames. Try again.\n");
	    }
	    if (strlen(player)>USER_NAME_MAX)
	    {
		printf("Usernames must not be more than %u characters long. Try again.\n",USER_NAME_MAX);
		continue;
	    }
	    
	    munged = 1;
	    commpos = 0;
	    password[0] = '\0';
	    while ((status = client_get_comm("Password: ",password,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(password); i++)
		if (isupper((int)password[i]))
		    password[i] = tolower((int)password[i]);
	    
	    munged = 1;
	    commpos = 0;
	    passwordvrfy[0] = '\0';
	    while ((status = client_get_comm("Retype password: ",passwordvrfy,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(passwordvrfy); i++)
		passwordvrfy[i] = tolower((int)passwordvrfy[i]);
	    
	    if (strcmp(password,passwordvrfy)!=0)
	    {
		printf("Passwords do not match. Try again.\n");
		continue;
	    }
            
	    bnet_hash(&passhash1,strlen(password),password); /* do the single hash */
	    
	    if (!(packet = packet_create(packet_class_bnet)))
	    {
		fprintf(stderr,"%s: could not create packet\n",argv[0]);
		psock_close(sd);
		if (changed_in)
		    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		return STATUS_FAILURE;
	    }
	    packet_set_size(packet,sizeof(t_client_createacctreq1));
	    packet_set_type(packet,CLIENT_CREATEACCTREQ1);
	    hash_to_bnhash((t_hash const *)&passhash1,packet->u.client_createacctreq1.password_hash1); /* avoid warning */
	    packet_append_string(packet,player);
            client_blocksend_packet(sd,packet);
	    packet_del_ref(packet);
	    
	    do
		if (client_blockrecv_packet(sd,rpacket)<0)
		{
		   fprintf(stderr,"%s: server closed connection\n",argv[0]);
		   psock_close(sd);
		   if (changed_in)
		       tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		   return STATUS_FAILURE;
		}
	    while (packet_get_type(rpacket)!=SERVER_CREATEACCTREPLY1);
	    dprintf("Got CREATEACCTREPLY1\n");
	    if (bn_int_get(rpacket->u.server_createacctreply1.result)==SERVER_CREATEACCTREPLY1_RESULT_NO)
	    {
		printf("Could not create an account under that name. Try another one.\n");
		continue;
	    }
	    printf("Account created.\n");
	}
	else if (changepass)
	{
	    char         passwordprev[MAX_MESSAGE_LEN];
	    char         passwordvrfy[MAX_MESSAGE_LEN];
            struct
            {
                bn_int ticks;
                bn_int sessionkey;
                bn_int passhash1[5];
            }            temp;
            t_hash       oldpasshash1;
            t_hash       oldpasshash2;
            t_hash       newpasshash1;
	    unsigned int ticks;
	    
            printf("Enter your old and new login information\n");
	    
	    munged = 1;
	    commpos = 0;
	    player[0] = '\0';
	    while ((status = client_get_comm("Username: ",player,MAX_MESSAGE_LEN,&commpos,1,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    if (strchr(player,' '))
	    {
		printf("Spaces not allowed in username. Try again.\n");
		continue;
	    }
	    if (strlen(player)>USER_NAME_MAX)
	    {
		printf("Usernames must not be more than %u characters long. Try again.\n",USER_NAME_MAX);
		continue;
	    }
	    
	    munged = 1;
	    commpos = 0;
	    passwordprev[0] = '\0';
	    while ((status = client_get_comm("Old password: ",passwordprev,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(passwordprev); i++)
		passwordprev[i] = tolower((int)passwordprev[i]);
	    
	    munged = 1;
	    commpos = 0;
	    password[0] = '\0';
	    while ((status = client_get_comm("New password: ",password,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(password); i++)
		password[i] = tolower((int)password[i]);
	    
	    munged = 1;
	    commpos = 0;
	    passwordvrfy[0] = '\0';
	    while ((status = client_get_comm("Retype new password: ",passwordvrfy,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(passwordvrfy); i++)
		passwordvrfy[i] = tolower((int)passwordvrfy[i]);
	    
	    if (strcmp(password,passwordvrfy)!=0)
	    {
		printf("New passwords do not match. Try again.\n");
		continue;
	    }
	    
            ticks = 0; /* FIXME: what to use here? */
            bn_int_set(&temp.ticks,ticks);
            bn_int_set(&temp.sessionkey,sessionkey);
            bnet_hash(&oldpasshash1,strlen(passwordprev),passwordprev); /* do the single hash for old */
            hash_to_bnhash((t_hash const *)&oldpasshash1,temp.passhash1); /* avoid warning */
            bnet_hash(&oldpasshash2,sizeof(temp),&temp); /* do the double hash for old */
	    bnet_hash(&newpasshash1,strlen(password),password); /* do the single hash for new */
	    
	    if (!(packet = packet_create(packet_class_bnet)))
	    {
		fprintf(stderr,"%s: could not create packet\n",argv[0]);
		psock_close(sd);
		if (changed_in)
		    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		return STATUS_FAILURE;
	    }
	    packet_set_size(packet,sizeof(t_client_changepassreq));
	    packet_set_type(packet,CLIENT_CHANGEPASSREQ);
	    bn_int_set(&packet->u.client_changepassreq.ticks,ticks);
	    bn_int_set(&packet->u.client_changepassreq.sessionkey,sessionkey);
	    hash_to_bnhash((t_hash const *)&oldpasshash2,packet->u.client_changepassreq.oldpassword_hash2); /* avoid warning */
	    hash_to_bnhash((t_hash const *)&newpasshash1,packet->u.client_changepassreq.newpassword_hash1); /* avoid warning */
	    packet_append_string(packet,player);
            client_blocksend_packet(sd,packet);
	    packet_del_ref(packet);
	    
	    do
		if (client_blockrecv_packet(sd,rpacket)<0)
		{
		   fprintf(stderr,"%s: server closed connection\n",argv[0]);
		   psock_close(sd);
		   if (changed_in)
		       tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		   return STATUS_FAILURE;
		}
	    while (packet_get_type(rpacket)!=SERVER_CHANGEPASSACK);
	    dprintf("Got CHANGEPASSACK\n");
	    if (bn_int_get(rpacket->u.server_changepassack.message)==SERVER_CHANGEPASSACK_MESSAGE_FAIL)
	    {
		printf("Could not change password. Try again.\n");
		continue;
	    }
	    printf("Password changed.\n");
	}
	else
	{
            printf("Enter your login information\n");
	    
	    munged = 1;
	    commpos = 0;
	    player[0] = '\0';
	    while ((status = client_get_comm("Username: ",player,MAX_MESSAGE_LEN,&commpos,1,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    if (strchr(player,' '))
	    {
		printf("Spaces not allowed in username. Try again.\n");
		continue;
	    }
	    if (strlen(player)>USER_NAME_MAX)
	    {
		printf("Usernames must not be more than %u characters long. Try again.\n",USER_NAME_MAX);
		continue;
	    }
	    
	    munged = 1;
	    commpos = 0;
	    password[0] = '\0';
	    while ((status = client_get_comm("Password: ",password,MAX_MESSAGE_LEN,&commpos,0,munged,screen_width))==0)
		if (handle_winch)
		{
		    client_get_termsize(fd_stdin,&screen_width,&screen_height);
		    printf(" \r");
		    munged = 1;
		    handle_winch = 0;
		}
		else
		    munged = 0;
	    printf("\n");
	    if (status<0)
		continue;
	    for (i=0; i<strlen(password); i++)
		password[i] = tolower((int)password[i]);
	}

	/* now login */
        {
            struct
            {
                bn_int ticks;
                bn_int sessionkey;
                bn_int passhash1[5];
            }            temp;
            t_hash       passhash1;
            t_hash       passhash2;
	    unsigned int ticks;
            
            ticks = 0; /* FIXME: what to use here? */
            bn_int_set(&temp.ticks,ticks);
            bn_int_set(&temp.sessionkey,sessionkey);
            bnet_hash(&passhash1,strlen(password),password); /* do the single hash */
            hash_to_bnhash((t_hash const *)&passhash1,temp.passhash1); /* avoid warning */
            bnet_hash(&passhash2,sizeof(temp),&temp); /* do the double hash */
	    
	    if (!(packet = packet_create(packet_class_bnet)))
	    {
		fprintf(stderr,"%s: could not create packet\n",argv[0]);
		psock_close(sd);
		if (changed_in)
		    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		return STATUS_FAILURE;
	    }
	    packet_set_size(packet,sizeof(t_client_loginreq1));
	    packet_set_type(packet,CLIENT_LOGINREQ1);
	    bn_int_set(&packet->u.client_loginreq1.ticks,ticks);
	    bn_int_set(&packet->u.client_loginreq1.sessionkey,sessionkey);
	    hash_to_bnhash((t_hash const *)&passhash2,packet->u.client_loginreq1.password_hash2); /* avoid warning */
	    packet_append_string(packet,player);
            client_blocksend_packet(sd,packet);
	    packet_del_ref(packet);
	}
	
        do
            if (client_blockrecv_packet(sd,rpacket)<0)
	    {
	       fprintf(stderr,"%s: server closed connection\n",argv[0]);
	       psock_close(sd);
	       if (changed_in)
	           tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	       return STATUS_FAILURE;
	    }
        while (packet_get_type(rpacket)!=SERVER_LOGINREPLY1);
	if (bn_int_get(rpacket->u.server_loginreply1.message)==SERVER_LOGINREPLY1_MESSAGE_SUCCESS)
	    break;
	fprintf(stderr,"Login incorrect.\n");
    }
    
    fprintf(stderr,"Logged in.\n");
    printf("----\n");
    
    if (newacct && (strcmp(clienttag,CLIENTTAG_DIABLORTL)==0 ||
		    strcmp(clienttag,CLIENTTAG_DIABLOSHR)==0))
    {
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",argv[0]);
	    if (changed_in)
		tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	}
	else
	{
	    packet_set_size(packet,sizeof(t_client_playerinforeq));
	    packet_set_type(packet,CLIENT_PLAYERINFOREQ);
	    packet_append_string(packet,player);
	    packet_append_string(packet,"LTRD 1 2 0 20 25 15 20 100 0"); /* FIXME: don't hardcode */
	    client_blocksend_packet(sd,packet);
	    packet_del_ref(packet);
	}
    }
    
#if 0 /* FIXME: what was this for? */
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_set_size(packet,sizeof(t_client_playerinforeq));
    packet_set_type(packet,CLIENT_PLAYERINFOREQ);
    packet_append_string(packet,player);
    packet_append_string(packet,"");
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    /* wait for playerinforeply */
#endif
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_set_size(packet,sizeof(t_client_progident2));
    packet_set_type(packet,CLIENT_PROGIDENT2);
    bn_int_tag_set(&packet->u.client_progident2.clienttag,clienttag);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",argv[0]);
	    psock_close(sd);
	    if (changed_in)
		tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	    return STATUS_FAILURE;
	}
    while (packet_get_type(rpacket)!=SERVER_CHANNELLIST);
    
    {
	unsigned int i;
	unsigned int chann_off;
	char const * chann;
	
	if (!(channellist = malloc(sizeof(char const *)*1)))
	{
	    fprintf(stderr,"%s: could not allocate memory for channellist\n",argv[0]);
            psock_close(sd);
            if (changed_in)
                tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	    return STATUS_FAILURE;
	}
	for (i=0,chann_off=sizeof(t_server_channellist);
	     (chann = packet_get_str_const(rpacket,chann_off,128));
	     i++,chann_off+=strlen(chann)+1)
        {
	    if (!(channellist = realloc(channellist,sizeof(char const *)*(i+2))))
	    {
		fprintf(stderr,"%s: could not allocate memory for channellist\n",argv[0]);
		psock_close(sd);
		if (changed_in)
		    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		return STATUS_FAILURE;
	    }
	    if (!(channellist[i] = strdup(chann)))
	    {
                fprintf(stderr,"%s: could not allocate memory for channellist[i]\n",argv[0]);
                psock_close(sd);
                if (changed_in)
                    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
                return STATUS_FAILURE;
	    }
	}
	channellist[i] = NULL;
    }
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",argv[0]);
	psock_close(sd);
	if (changed_in)
	    tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
	return STATUS_FAILURE;
    }
    packet_set_size(packet,sizeof(t_client_joinchannel));
    packet_set_type(packet,CLIENT_JOINCHANNEL);
    bn_int_set(&packet->u.client_joinchannel.channelflag,CLIENT_JOINCHANNEL_GENERIC);
    packet_append_string(packet,channel);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    if (psock_ctl(sd,PSOCK_NONBLOCK)<0)
	fprintf(stderr,"%s: could not set TCP socket to non-blocking mode (psock_ctl: %s)\n",argv[0],strerror(psock_errno()));
    
    mode = mode_chat;
    
    {
	unsigned int   i;
	int            highest_fd;
	t_psock_fd_set rfds;
	
	PSOCK_FD_ZERO(&rfds);
	highest_fd = fd_stdin;
	if (sd>highest_fd)
	    highest_fd = sd;
	
	currsize = 0;
	
	munged = 1; /* == need to draw prompt */
	commpos = 0;
	text[0] = '\0';
	
	for (;;)
	{
	    if (handle_winch)
	    {
		client_get_termsize(fd_stdin,&screen_width,&screen_height);
		handle_winch = 0;
		printf(" \r");
		munged = 1;
	    }
	    
	    if (munged)
	    {
		printf("%s%s",mode_get_prompt(mode),text + ((screen_width <= strlen(mode_get_prompt(mode)) + commpos ) ? strlen(mode_get_prompt(mode)) + commpos + 1 - screen_width : 0));
		fflush(stdout);
		munged = 0;
	    }
	    
	    PSOCK_FD_SET(fd_stdin,&rfds);
	    PSOCK_FD_SET(sd,&rfds);
	    
	    if (psock_select(highest_fd+1,&rfds,NULL,NULL,NULL)<0)
	    {
		if (errno!=PSOCK_EINTR)
		{
		    if (!munged)
		    {
			printf("\r");
			for (i=0; i<strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			printf("\r");
		    }
		    printf("Select failed (select: %s)\n",strerror(errno));
		}
		continue;
	    }
	    
	    if (PSOCK_FD_ISSET(fd_stdin,&rfds)) /* got keyboard data */
	    {
		munged = 0;
		
		switch (client_get_comm(mode_get_prompt(mode),text,MAX_MESSAGE_LEN,&commpos,1,0,screen_width))
		{
		case -1: /* cancel */
		    printf("\r");
		    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
			printf(" ");
		    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
			printf(" ");
		    printf("\r");
		    munged = 1;
		    if (mode==mode_command)
			mode = mode_chat;
		    else
			mode = mode_command;
		    commpos = 0;
		    text[0] = '\0';
		    break;
		    
		case 0: /* timeout */
		    break;
		    
		case 1:
		    switch (mode)
		    {
/*
9: recv class=normal[0x0001] type=CLIENT_STARTGAME4[0x1cff] length=76
0000:   FF 1C 4C 00 00 00 00 00   00 00 00 00 06 00 03 00    ..L.............
0010:   00 00 00 00 00 00 00 00   67 72 65 65 64 37 35 30    ........greed750
0020:   30 00 00 2C 33 34 2C 31   32 2C 2C 31 2C 36 2C 33    0..,34,12,,1,6,3
0030:   2C 33 65 33 37 61 38 34   63 2C 2C 52 6F 73 73 0D    ,3e37a84c,,Ross.
0040:   43 68 61 6C 6C 65 6E 67   65 72 0D 00                Challenger..
9: send class=normal[0x0001] type=SERVER_STARTGAME4_ACK[0x1cff] length=8
0000:   FF 1C 08 00 00 00 00 00                              ........        
9: recv class=normal[0x0001] type=CLIENT_LEAVECHANNEL[0x10ff] length=4
0000:   FF 10 04 00                                          ....            
9: send class=normal[0x0001] type=SERVER_MESSAGE[0x0fff] length=34
0000:   FF 0F 22 00 03 00 00 00   00 00 00 00 86 00 00 00    ..".............
0010:   00 00 00 00 00 00 00 00   00 00 00 00 52 6F 73 73    ............Ross
0020:   00 00                                                ..
9: recv class=normal[0x0001] type=CLIENT_CLOSEGAME[0x02ff] length=4
0000:   FF 02 04 00                                          ....
9: recv class=normal[0x0001] type=CLIENT_PLAYERINFOREQ[0x0aff] length=10
0000:   FF 0A 0A 00 52 6F 73 73   00 00                      ....Ross..
9: send class=normal[0x0001] type=SERVER_PLAYERINFOREPLY[0x0aff] length=30
0000:   FF 0A 1E 00 52 6F 73 73   00 50 58 45 53 20 30 20    ....Ross.PXES 0 
0010:   30 20 31 30 20 30 20 30   00 52 6F 73 73 00          0 10 0 0.Ross.
9: recv class=normal[0x0001] type=CLIENT_PROGIDENT2[0x0bff] length=8
0000:   FF 0B 08 00 50 58 45 53                              ....PXES
9: send class=normal[0x0001] type=SERVER_CHANNELLIST[0x0bff] length=35
0000:   FF 0B 23 00 42 6E 65 74   64 20 53 75 70 70 6F 72    ..#.Bnetd Suppor
0010:   74 00 42 72 6F 6F 64 20   57 61 72 20 55 53 41 2D    t.Brood War USA-
0020:   31 00 00                                             1..
9: recv class=normal[0x0001] type=CLIENT_JOINCHANNEL[0x0cff] length=24
0000:   FF 0C 18 00 02 00 00 00   42 72 6F 6F 64 20 57 61    ........Brood Wa
0010:   72 20 55 53 41 2D 31 00                              r USA-1.
*/
		    case mode_gamename:
			if (text[0]=='\0')
			{
			    printf("\r");
			    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    printf("\rGames must have a name.\n");
			    munged = 1;
			    break;
			}
			printf("\n");
			munged = 1;
			strncpy(curr_gamename,text,sizeof(curr_gamename));
			curr_gamename[sizeof(curr_gamename)-1] = '\0';
			mode = mode_gamepass;
			break;
		    case mode_gamepass:
			printf("\n");
			munged = 1;
			strncpy(curr_gamepass,text,sizeof(curr_gamepass));
			curr_gamepass[sizeof(curr_gamepass)-1] = '\0';
			
			if (!(packet = packet_create(packet_class_bnet)))
			{
			    printf("Packet creation failed.\n");
			    mode = mode_command;
			}
			else
			{
			    packet_set_size(packet,sizeof(t_client_startgame4));
			    packet_set_type(packet,CLIENT_STARTGAME4);
			    bn_int_set(&packet->u.client_startgame4.status,CLIENT_STARTGAME4_STATUS_INIT);
			    bn_int_set(&packet->u.client_startgame4.unknown2,CLIENT_STARTGAME4_UNKNOWN2);
			    bn_short_set(&packet->u.client_startgame4.gametype,CLIENT_GAMELISTREQ_MELEE);
			    bn_short_set(&packet->u.client_startgame4.option,CLIENT_STARTGAME4_OPTION_MELEE_NORMAL);
			    bn_int_set(&packet->u.client_startgame4.unknown4,CLIENT_STARTGAME4_UNKNOWN4);
			    bn_int_set(&packet->u.client_startgame4.unknown5,CLIENT_STARTGAME4_UNKNOWN5);
			    packet_append_string(packet,curr_gamename);
			    packet_append_string(packet,curr_gamepass);
			    packet_append_string(packet,",,,,1,3,1,3e37a84c,7,Player\rAshrigo\r");
			    client_blocksend_packet(sd,packet);
			    packet_del_ref(packet);
			    mode = mode_gamewait;
			}
			break;
		    case mode_gamestop:
			printf("\n");
			munged = 1;
			
			if (!(packet = packet_create(packet_class_bnet)))
			    printf("Packet creation failed.\n");
			else
			{
			    packet_set_size(packet,sizeof(t_client_closegame));
			    packet_set_type(packet,CLIENT_CLOSEGAME);
			    client_blocksend_packet(sd,packet);
			    packet_del_ref(packet);
			    printf("Game closed.\n");
			    mode = mode_command;
			}
			break;
		    case mode_command:
			if (text[0]=='\0')
			    break;
			printf("\n");
			munged = 1;
			if (strstart(text,"channel")==0)
			{
			    printf("Available channels:\n");
			    if (useansi)
				ansi_text_color_fore(ansi_text_color_yellow);
			    for (i=0; channellist[i]; i++)
				printf(" %s\n",channellist[i]);
			    if (useansi)
				ansi_text_reset();
			}
			else if (strstart(text,"create")==0)
			{
			    printf("Enter new game information\n");
			    mode = mode_gamename;
			}
			else if (strstart(text,"join")==0)
			{
			    printf("Not implemented yet.\n");
			}
			else if (strstart(text,"ladder")==0)
			{
			    printf("Not implemented yet.\n");
			}
			else if (strstart(text,"stats")==0)
			{
			    printf("Not implemented yet.\n");
			}
			else if (strstart(text,"help")==0 || strcmp(text,"?")==0)
			{
			    printf("Available commands:\n"
				   " channel        - join or create a channel\n"
				   " create         - create a new game\n"
				   " join           - list current games\n"
				   " ladder         - list ladder rankings\n"
				   " help           - show this text\n"
				   " info <PLAYER>  - print a player's profile\n"
				   " chinfo         - modify your profile\n"
				   " quit           - exit bnchat\n"
				   " stats <PLAYER> - print a player's game record\n"
				   "Use the escape key to toggle between chat and command modes.\n");
			}
			else if (strstart(text,"info")==0)
			{
			    for (i=4; text[i]==' ' || text[i]=='\t'; i++);
			    if (text[i]=='\0')
			    {
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_red);
				printf("You must specify the player.\n");
				if (useansi)
				    ansi_text_reset();
			    }
			    else
			    {
				if (!(packet = packet_create(packet_class_bnet)))
				    fprintf(stderr,"%s: could not create packet\n",argv[0]);
				else
				{
				    printf("Profile info for %s:\n",&text[i]);
				    packet_set_size(packet,sizeof(t_client_statsreq));
				    packet_set_type(packet,CLIENT_STATSREQ);
				    bn_int_set(&packet->u.client_statsreq.name_count,1);
				    bn_int_set(&packet->u.client_statsreq.key_count,4);
				    statsmatch = (unsigned int)time(NULL);
				    bn_int_set(&packet->u.client_statsreq.unknown1,statsmatch);
				    packet_append_string(packet,&text[i]);
#if 0
				    packet_append_string(packet,"BNET\\acct\\username");
				    packet_append_string(packet,"BNET\\acct\\userid");
#endif
				    packet_append_string(packet,"profile\\sex");
				    packet_append_string(packet,"profile\\age");
				    packet_append_string(packet,"profile\\location");
				    packet_append_string(packet,"profile\\description");
				    client_blocksend_packet(sd,packet);
				    packet_del_ref(packet);
				    
				    mode = mode_waitstat;
				}
			    }
			}
			else if (strstart(text,"chinfo")==0)
			{
			    printf("Not implemented yet.\n");
			}
			else if (strstart(text,"quit")==0)
			{
			    psock_close(sd);
			    if (changed_in)
				tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
			    return STATUS_SUCCESS;
			}
			else
			{
			    if (useansi)
				ansi_text_color_fore(ansi_text_color_red);
			    printf("Unknown local command \"%s\".\n",text);
			    if (useansi)
				ansi_text_reset();
			}
			break;
			
		    case mode_chat:
			if (text[0]=='\0')
			    break;
			if (text[0]=='/')
			{
			    printf("\r");
			    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    printf("\r");
			    munged = 1;
			}
			else
			{
			    if (useansi)
				ansi_text_color_fore(ansi_text_color_blue);
			    printf("\r<%s>",player);
			    if (useansi)
				ansi_text_reset();
			    printf(" ");
			    str_print_term(stdout,text,0,0);
			    printf("\n");
			    munged = 1;
			}
			
			if (!(packet = packet_create(packet_class_bnet)))
			{
			    fprintf(stderr,"%s: could not create packet\n",argv[0]);
			    psock_close(sd);
			    if (changed_in)
				tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
			    return STATUS_FAILURE;
			}
			packet_set_size(packet,sizeof(t_client_message));
			packet_set_type(packet,CLIENT_MESSAGE);
			packet_append_string(packet,text);
			client_blocksend_packet(sd,packet);
			packet_del_ref(packet);
			break;
			
		    default:
			/* one of the wait states; erase what they typed */
			printf("\r");
			for (i=0; i<strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			printf("\r");
			munged = 1;
			break;
		    }
		    
		    commpos = 0;
		    text[0] = '\0';
		    break;
		}
	    }
	    
	    if (PSOCK_FD_ISSET(sd,&rfds)) /* got network data */
	    {
		/* rpacket is from server, packet is from client */
		switch (net_recv_packet(sd,rpacket,&currsize))
		{
		case 0: /* nothing */
		    break;
		    
		case 1:
		    switch (packet_get_type(rpacket))
		    {
		    case SERVER_ECHOREQ: /* might as well support it */
			if (packet_get_size(rpacket)<sizeof(t_server_echoreq))
			{
			    if (!munged)
			    {
				printf("\r");
				for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				printf("\r");
				munged = 1;
			    }
			    printf("Got bad SERVER_ECHOREQ packet (expected %u bytes, got %u)\n",sizeof(t_server_echoreq),packet_get_size(rpacket));
			    break;
			}
			
			if (!(packet = packet_create(packet_class_bnet)))
			{
			    if (!munged)
			    {
				printf("\r");
				for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				printf("\r");
				munged = 1;
			    }
			    fprintf(stderr,"%s: could not create packet\n",argv[0]);
			    psock_close(sd);
			    if (changed_in)
				tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
			    return STATUS_FAILURE;
			}
			packet_set_size(packet,sizeof(t_client_echoreply));
			packet_set_type(packet,CLIENT_ECHOREPLY);
			bn_int_set(&packet->u.client_echoreply.ticks,bn_int_get(rpacket->u.server_echoreq.ticks));
			client_blocksend_packet(sd,packet);
			packet_del_ref(packet);
			
			break;
			
		    case SERVER_STARTGAME4_ACK:
			if (mode==mode_gamewait)
			{
			    if (!munged)
			    {
				printf("\r");
				for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				printf("\r");
				munged = 1;
			    }
			    
			    if (bn_int_get(rpacket->u.server_startgame4_ack.reply)==SERVER_STARTGAME4_ACK_OK)
			    {
				printf("Game created.\n");
				mode = mode_gamestop;
			    }
			    else
			    {
				printf("Game could not be created, try another name.\n");
				mode = mode_gamename;
			    }
			}
			break;
			
		    case SERVER_STATSREPLY:
			if (mode==mode_waitstat)
			{
			    unsigned int names,keys;
			    unsigned int match;
			    unsigned int strpos;
			    char const * temp;
			    
			    if (!munged)
			    {
				printf("\r");
				for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				printf("\r");
				munged = 1;
			    }
			    
			    names = bn_int_get(rpacket->u.server_statsreply.name_count);
			    keys  = bn_int_get(rpacket->u.server_statsreply.key_count);
			    match = bn_int_get(rpacket->u.server_statsreply.unknown1);
			    
			    if (names!=1 || keys!=4 || match!=statsmatch)
				printf("mangled reply (name_count=%u key_count=%u unknown1=%u)\n",
				       names,keys,match);
			    
			    strpos = sizeof(t_server_statsreply);
			    if ((temp = packet_get_str_const(rpacket,strpos,256)))
			    {
				printf(" Sex: ");
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("%s\n",temp);
				if (useansi)
				    ansi_text_reset();
				strpos += strlen(temp)+1;
			    }
			    if ((temp = packet_get_str_const(rpacket,strpos,256)))
			    {
				printf(" Age: ");
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("%s\n",temp);
				if (useansi)
				    ansi_text_reset();
				strpos += strlen(temp)+1;
			    }
			    if ((temp = packet_get_str_const(rpacket,strpos,256)))
			    {
				printf(" Location: ");
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("%s\n",temp);
				if (useansi)
				    ansi_text_reset();
				strpos += strlen(temp)+1;
			    }
			    if ((temp = packet_get_str_const(rpacket,strpos,256)))
			    {
				char   msgtemp[1024];
				char * tok;
				
				printf(" Description: \n");
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				strncpy(msgtemp,temp,sizeof(msgtemp));
				msgtemp[sizeof(msgtemp)-1] = '\0';
				for (tok=strtok(msgtemp,"\r\n"); tok; tok=strtok(NULL,"\r\n"))
				    printf("  %s\n",tok);
				if (useansi)
				    ansi_text_reset();
				strpos += strlen(temp)+1;
			    }
			    
			    if (match==statsmatch)
				mode = mode_command;
			}
			break;
			
		    case SERVER_PLAYERINFOREPLY: /* hmm. we didn't ask for one... */
			break;
			
		    case SERVER_MESSAGE:
			if (packet_get_size(rpacket)<sizeof(t_server_message))
			{
			    if (!munged)
			    {
				printf("\r");
				for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				    printf(" ");
				printf("\r");
				munged = 1;
			    }
			    printf("Got bad SERVER_MESSAGE packet (expected %u bytes, got %u)",sizeof(t_server_message),packet_get_size(rpacket));
			    break;
			}
			
			{
			    char const * speaker;
			    char const * message;
			    
			    if (!(speaker = packet_get_str_const(rpacket,sizeof(t_server_message),32)))
			    {
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				printf("Got SERVER_MESSAGE packet with bad or missing speaker\n");
				break;
			    }
			    if (!(message = packet_get_str_const(rpacket,sizeof(t_server_message)+strlen(speaker)+1,1024)))
			    {
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				printf("Got SERVER_MESSAGE packet with bad or missing message (speaker=\"%s\" start=%u len=%u)\n",speaker,sizeof(t_server_message)+strlen(speaker)+1,packet_get_size(rpacket));
				break;
			    }
			    
			    switch (bn_int_get(rpacket->u.server_message.type))
			    {
			    case SERVER_MESSAGE_TYPE_JOIN:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_green);
				printf("\"");
				str_print_term(stdout,speaker,0,0);
				printf("\" %s enters\n",mflags_get_str(bn_int_get(rpacket->u.server_message.flags)));
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_CHANNEL:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_green);
				printf("Joining channel: \"");
				str_print_term(stdout,message,0,0);
				printf("\" %s\n",cflags_get_str(bn_int_get(rpacket->u.server_message.flags)));
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_ADDUSER:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_green);
				printf("\"");
				str_print_term(stdout,speaker,0,0);
				printf("\" %s is here\n",mflags_get_str(bn_int_get(rpacket->u.server_message.flags)));
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_USERFLAGS:
				break;
			    case SERVER_MESSAGE_TYPE_PART:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_green);
				printf("\"");
				str_print_term(stdout,speaker,0,0);
				printf("\" %s leaves\n",mflags_get_str(bn_int_get(rpacket->u.server_message.flags)));
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_WHISPER:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_blue);
				printf("<From: ");
				str_print_term(stdout,speaker,0,0);
				printf(">");
				if (useansi)
				    ansi_text_reset();
				printf(" ");
				str_print_term(stdout,message,0,0);
				printf("\n");
				break;
			    case SERVER_MESSAGE_TYPE_WHISPERACK:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_blue);
				printf("<To: ");
				str_print_term(stdout,speaker,0,0);
				printf(">");
				if (useansi)
				    ansi_text_reset();
				printf(" ");
				str_print_term(stdout,message,0,0);
				printf("\n");
				break;
			    case SERVER_MESSAGE_TYPE_BROADCAST:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("<Broadcast: ");
				str_print_term(stdout,speaker,0,0);
				printf("> ");
				str_print_term(stdout,message,0,0);
				printf("\n");
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_ERROR:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_red);
				printf("<Error> ");
				str_print_term(stdout,message,0,0);
				printf("\n");
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_INFO:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("<Info> ");
				str_print_term(stdout,message,0,0);
				printf("\n");
				if (useansi)
				    ansi_text_reset();
				break;
			    case SERVER_MESSAGE_TYPE_EMOTE:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("<");
				str_print_term(stdout,speaker,0,0);
				printf(" ");
				str_print_term(stdout,message,0,0);
				printf(">\n");
				if (useansi)
				    ansi_text_reset();
				break;
			    default:
			    case SERVER_MESSAGE_TYPE_TALK:
				if (!munged)
				{
				    printf("\r");
				    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
					printf(" ");
				    printf("\r");
				    munged = 1;
				}
				if (useansi)
				    ansi_text_color_fore(ansi_text_color_yellow);
				printf("<");
				str_print_term(stdout,speaker,0,0);
				printf(">");
				if (useansi)
				    ansi_text_reset();
				printf(" ");
				str_print_term(stdout,message,0,0);
				printf("\n");
			    }
			}
			break;
			
		    default:
			if (!munged)
			{
			    printf("\r");
			    for (i=0; i<strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
				printf(" ");
			    printf("\r");
			    munged = 1;
			}
			printf("Unsupported server packet type 0x%04x\n",packet_get_type(rpacket));
		    }
		    
		    currsize = 0;
		    break;
		    
		case -1: /* error (probably connection closed) */
		default:
		    if (!munged)
		    {
			printf("\r");
			for (i=0; i<strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			for (i=0; i<strlen(text) && i<screen_width-strlen(mode_get_prompt(mode)); i++)
			    printf(" ");
			printf("\r");
			munged = 1;
		    }
		    printf("----\nConnection closed by server.\n");
		    psock_close(sd);
		    if (changed_in)
			tcsetattr(fd_stdin,TCSAFLUSH,&in_attr_old);
		    return STATUS_SUCCESS;
		}
	    }
	}
    }
    
    /* not reached */
}

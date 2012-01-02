/*
 * Copyright (C) 1998,1999,2000,2001  Ross Combs (rocombs@cs.nmsu.edu)
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
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
#include "compat/memset.h"
#include "compat/memcpy.h"
#include <ctype.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
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
#include "compat/gethostname.h"
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
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
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include "compat/psock.h"
#include "common/packet.h"
#include "common/init_protocol.h"
#include "common/udp_protocol.h"
#include "common/bnet_protocol.h"
#include "common/file_protocol.h"
#include "common/tag.h"
#include "common/bn_type.h"
#include "common/field_sizes.h"
#include "common/bnethash.h"
#include "common/bnethashconv.h"
#include "common/util.h"
#ifdef CLIENTDEBUG
# include "common/eventlog.h"
#endif
#include "client.h"
#include "udptest.h"
#include "common/bnettime.h"
#include "common/proginfo.h"
#include "client_connect.h"
#include "common/setup_after.h"


#define COMPNAMELEN 128

#ifdef CLIENTDEBUG
# define dprintf printf
#else
# define dprintf if (0) printf
#endif


static int key_interpret(char const * cdkey, unsigned int * productid, unsigned int * keyvalue1, unsigned int * keyvalue2)
{
    /* FIXME: implement... have example code from Eurijk! */
    *productid = 0;
    *keyvalue1 = 0;
    *keyvalue2 = 0;
    return 0;
}


static int get_defversioninfo(char const * progname, char const * clienttag, unsigned int * versionid, unsigned int * gameversion, char const * * exeinfo)
{
    if (strcmp(clienttag,CLIENTTAG_DIABLORTL)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_DRTL;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_DRTL;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_DRTL;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_STARCRAFT)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_STAR;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_STAR;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_STAR;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_SHAREWARE)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_SSHR;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_SSHR;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_SSHR;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_BROODWARS)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_SEXP;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_SEXP;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_SEXP;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_W2BN;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_W2BN;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_W2BN;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_DIABLO2DV)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_D2DV;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_D2DV;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_D2DV;
	return 0;
    }
    
    if (strcmp(clienttag,CLIENTTAG_DIABLO2XP)==0)
    {
	*versionid = CLIENT_AUTHREQ_VERSIONID_D2XP;
    *gameversion = CLIENT_AUTHREQ_GAMEVERSION_D2XP;
	*exeinfo = CLIENT_AUTHREQ_EXEINFO_D2XP;
	return 0;
    }
    
    *versionid = 0;
    *gameversion = 0;
    *exeinfo = "";
    
    fprintf(stderr,"%s: unsupported clienttag \"%s\"\n",progname,clienttag);
}


extern int client_connect(char const * progname, char const * servname, unsigned short servport, char const * cdowner, char const * cdkey, char const * clienttag, struct sockaddr_in * saddr, unsigned int * sessionkey, unsigned int * sessionnum, char const * archtag)
{
    struct hostent * host;
    char const *     username;
    int              sd;
    t_packet *       packet;
    t_packet *       rpacket;
    char             compname[COMPNAMELEN];
    int              lsock;
    unsigned short   lsock_port;
    
    if (!progname)
    {
	fprintf(stderr,"got NULL progname\n");
	return -1;
    }
    if (!saddr)
    {
	fprintf(stderr,"%s: got NULL saddr\n",progname);
	return -1;
    }
    if (!sessionkey)
    {
	fprintf(stderr,"%s: got NULL sessionkey\n",progname);
	return -1;
    }
    if (!sessionnum)
    {
	fprintf(stderr,"%s: got NULL sessionnum\n",progname);
	return -1;
    }
    
    if (psock_init()<0)
    {
	fprintf(stderr,"%s: could not inialialize socket functions\n",progname);
	return -1;
    }
    
    if (!(host = gethostbyname(servname)))
    {
	fprintf(stderr,"%s: unknown host \"%s\"\n",progname,servname);
	return -1;
    }
    if (host->h_addrtype!=PSOCK_AF_INET)
	fprintf(stderr,"%s: host \"%s\" is not in IPv4 address family\n",progname,servname);
    
    if (gethostname(compname,COMPNAMELEN)<0)
    {
	fprintf(stderr,"%s: could not get host name (gethostname: %s)\n",progname,strerror(errno));
	return -1;
    }
#ifdef HAVE_GETLOGIN
    if (!(username = getlogin()))
#endif
    {
	username = "unknown";
	dprintf("%s: could not get login name, using \"%s\" (getlogin: %s)\n",progname,username,strerror(errno));
    }
    
    if ((sd = psock_socket(PSOCK_PF_INET,PSOCK_SOCK_STREAM,0))<0)
    {
	fprintf(stderr,"%s: could not create socket (psock_socket: %s)\n",progname,strerror(psock_errno()));
	return -1;
    }
    
    memset(saddr,0,sizeof(*saddr));
    saddr->sin_family = PSOCK_AF_INET;
    saddr->sin_port   = htons(servport);
    memcpy(&saddr->sin_addr.s_addr,host->h_addr_list[0],host->h_length);
    if (psock_connect(sd,(struct sockaddr *)saddr,sizeof(*saddr))<0)
    {
	fprintf(stderr,"%s: could not connect to server \"%s\" port %hu (psock_connect: %s)\n",progname,servname,servport,strerror(psock_errno()));
	goto error_sd;
    }
    
    printf("Connected to %s:%hu.\n",inet_ntoa(saddr->sin_addr),servport);
    
#ifdef CLIENTDEBUG
    eventlog_set(stderr);
#endif
        
    if ((lsock = client_udptest_setup(progname,&lsock_port))>=0)
	dprintf("Got UDP data on port %hu\n",lsock_port);
    
    if (!(packet = packet_create(packet_class_init)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_lsock;
    }
    bn_byte_set(&packet->u.client_initconn.class,CLIENT_INITCONN_CLASS_DEFER); /* == bnet */
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
#ifdef REALLY_OLD_PROT
    if (lsock>=0)
	if ((lsock = client_udptest_recv(progname,lsock,lsock_port,CLIENT_MAX_UDPTEST_WAIT))>=0)
	    dprintf("Got UDP data on port %hu\n",lsock_port);
#endif
    
    /* reuse this same packet over and over */
    if (!(rpacket = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_lsock;
    }
    
    if (strcmp(clienttag,CLIENTTAG_DIABLOSHR)==0 ||
        strcmp(clienttag,CLIENTTAG_DIABLORTL)==0)
    {
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",progname);
	    goto error_rpacket;
	}
	packet_set_size(packet,sizeof(t_client_unknown_1b));
	packet_set_type(packet,CLIENT_UNKNOWN_1B);
	bn_short_set(&packet->u.client_unknown_1b.unknown1,CLIENT_UNKNOWN_1B_UNKNOWN3);
	bn_short_nset(&packet->u.client_unknown_1b.port,lsock_port);
	bn_int_nset(&packet->u.client_unknown_1b.ip,0); /* FIXME */
	bn_int_set(&packet->u.client_unknown_1b.unknown2,CLIENT_UNKNOWN_1B_UNKNOWN3);
	bn_int_set(&packet->u.client_unknown_1b.unknown3,CLIENT_UNKNOWN_1B_UNKNOWN3);
	client_blocksend_packet(sd,packet);
	packet_del_ref(packet);
    }
    
#ifdef OLD_PROT
# ifdef REALLY_OLD_PROT
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_compinfo1));
    packet_set_type(packet,CLIENT_COMPINFO1);
    bn_int_set(&packet->u.client_compinfo1.reg_version,CLIENT_COMPINFO1_REG_VERSION);
    bn_int_set(&packet->u.client_compinfo1.reg_auth,CLIENT_COMPINFO1_REG_AUTH);
    bn_int_set(&packet->u.client_compinfo1.client_id,CLIENT_COMPINFO1_CLIENT_ID);
    bn_int_set(&packet->u.client_compinfo1.client_token,CLIENT_COMPINFO1_CLIENT_TOKEN);
    packet_append_string(packet,compname);
    packet_append_string(packet,username);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
# else
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_compinfo2));
    packet_set_type(packet,CLIENT_COMPINFO2);
    bn_int_set(&packet->u.client_compinfo2.unknown1,CLIENT_COMPINFO2_UNKNOWN1);
    bn_int_set(&packet->u.client_compinfo2.reg_version,CLIENT_COMPINFO2_REG_VERSION);
    bn_int_set(&packet->u.client_compinfo2.reg_auth,CLIENT_COMPINFO2_REG_AUTH);
    bn_int_set(&packet->u.client_compinfo2.client_id,CLIENT_COMPINFO2_CLIENT_ID);
    bn_int_set(&packet->u.client_compinfo2.client_token,CLIENT_COMPINFO2_CLIENT_TOKEN);
    packet_append_string(packet,compname);
    packet_append_string(packet,username);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
# endif
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_countryinfo1));
    packet_set_type(packet,CLIENT_COUNTRYINFO1);
    {
	t_bnettime  btsystem;
	t_bnettime  btlocal;
	int         bias;
	
        bias = local_tzbias();
	btsystem = bnettime();
	btlocal = bnettime_add_tzbias(btsystem,bias);
        
	bnettime_to_bn_long(btsystem,&packet->u.client_countryinfo1.systemtime);
	bnettime_to_bn_long(btlocal,&packet->u.client_countryinfo1.localtime);
	
	bn_int_set(&packet->u.client_countryinfo1.bias,(unsigned int)bias); /* rely on 2's complement */
	dprintf("my tzbias = %d (0x%08hx)\n",bias,(unsigned int)bias);
    }
    /* FIXME: determine from locale */
    bn_int_set(&packet->u.client_countryinfo1.langid1,CLIENT_COUNTRYINFO1_LANGID_USENGLISH);
    bn_int_set(&packet->u.client_countryinfo1.langid2,CLIENT_COUNTRYINFO1_LANGID_USENGLISH);
    bn_int_set(&packet->u.client_countryinfo1.langid3,CLIENT_COUNTRYINFO1_LANGID_USENGLISH);
    packet_append_string(packet,CLIENT_COUNTRYINFO1_LANGSTR_USENGLISH);
    /* FIXME: determine from locale+timezone... from domain name... nothing really would
       work.  Maybe add some command-line options */
    packet_append_string(packet,CLIENT_COUNTRYINFO1_COUNTRYCODE_USA);
    packet_append_string(packet,CLIENT_COUNTRYINFO1_COUNTRYABB_USA);
    packet_append_string(packet,CLIENT_COUNTRYINFO1_COUNTRYNAME_USA);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
#else
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_countryinfo_109));
    packet_set_type(packet,CLIENT_COUNTRYINFO_109);
    bn_int_set(&packet->u.client_countryinfo_109.unknown1,CLIENT_COUNTRYINFO_109_UNKNOWN1);
    bn_int_tag_set(&packet->u.client_countryinfo_109.archtag,archtag);
    bn_int_tag_set(&packet->u.client_countryinfo_109.clienttag,clienttag);
    bn_int_set(&packet->u.client_countryinfo_109.versionid,CLIENT_COUNTRYINFO_109_VERSIONID);
    bn_int_set(&packet->u.client_countryinfo_109.unknown2,CLIENT_COUNTRYINFO_109_UNKNOWN2);
    bn_int_set(&packet->u.client_countryinfo_109.unknown3,CLIENT_COUNTRYINFO_109_UNKNOWN3);
    {
	int bias;
	
        bias = local_tzbias();
        
	bn_int_set(&packet->u.client_countryinfo_109.bias,(unsigned int)bias); /* rely on 2's complement */
	dprintf("my tzbias = %d (0x%08hx)\n",bias,(unsigned int)bias);
    }
    /* FIXME: determine from locale */
    bn_int_set(&packet->u.client_countryinfo_109.lcid,CLIENT_COUNTRYINFO_109_LANGID_USENGLISH);
    bn_int_set(&packet->u.client_countryinfo_109.langid,CLIENT_COUNTRYINFO_109_LANGID_USENGLISH);
    packet_append_string(packet,CLIENT_COUNTRYINFO_109_LANGSTR_USENGLISH);
    /* FIXME: determine from locale+timezone... from domain name... nothing really would
       work.  Maybe add some command-line options */
    packet_append_string(packet,CLIENT_COUNTRYINFO_109_COUNTRYNAME_USA);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
#endif
    
    /* FIXME: did old clients send this?  Was it only Diablo I & II? */
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_unknown_2b));
    packet_set_type(packet,CLIENT_UNKNOWN_2B);
    bn_int_set(&packet->u.client_unknown_2b.unknown1,CLIENT_UNKNOWN_2B_UNKNOWN1);
    bn_int_set(&packet->u.client_unknown_2b.unknown2,CLIENT_UNKNOWN_2B_UNKNOWN2);
    bn_int_set(&packet->u.client_unknown_2b.unknown3,CLIENT_UNKNOWN_2B_UNKNOWN3);
    bn_int_set(&packet->u.client_unknown_2b.unknown4,CLIENT_UNKNOWN_2B_UNKNOWN4);
    bn_int_set(&packet->u.client_unknown_2b.unknown5,CLIENT_UNKNOWN_2B_UNKNOWN5);
    bn_int_set(&packet->u.client_unknown_2b.unknown6,CLIENT_UNKNOWN_2B_UNKNOWN6);
    bn_int_set(&packet->u.client_unknown_2b.unknown7,CLIENT_UNKNOWN_2B_UNKNOWN7);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
#ifdef OLD_PROT
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_progident));
    packet_set_type(packet,CLIENT_PROGIDENT);
    bn_int_tag_set(&packet->u.client_progident.archtag,archtag);
    bn_int_tag_set(&packet->u.client_progident.clienttag,clienttag);
    if (strcmp(clienttag,CLIENTTAG_DIABLORTL)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_DRTL);
    else if (strcmp(clienttag,CLIENTTAG_STARCRAFT)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_STAR);
    else if (strcmp(clienttag,CLIENTTAG_SHAREWARE)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_SSHR);
    else if (strcmp(clienttag,CLIENTTAG_BROODWARS)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_SEXP);
    else if (strcmp(clienttag,CLIENTTAG_WARCIIBNE)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_W2BN);
    else if (strcmp(clienttag,CLIENTTAG_DIABLO2DV)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_D2DV);
    else if (strcmp(clienttag,CLIENTTAG_DIABLO2XP)==0)
	bn_int_set(&packet->u.client_progident.versionid,CLIENT_PROGIDENT_VERSIONID_D2XP);
    else
    {
	fprintf(stderr,"%s: unsupported clienttag \"%s\"\n",progname,clienttag);
	bn_int_set(&packet->u.client_progident.versionid,0);
    }
    bn_int_set(&packet->u.client_progident.unknown1,CLIENT_PROGIDENT_UNKNOWN1); /* FIXME: spawn? */
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_COMPREPLY);
    dprintf("Got COMPREPLY\n");
    
# ifdef REALLY_OLD_PROT
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_SESSIONKEY1);
    *sessionkey = bn_int_get(rpacket->u.server_sessionkey1.sessionkey);
    dprintf("Got SESSIONKEY1 (0x%08x)\n",*sessionkey);
# else
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_SESSIONKEY2);
    *sessionkey = bn_int_get(rpacket->u.server_sessionkey2.sessionkey);
    *sessionnum = bn_int_get(rpacket->u.server_sessionkey2.sessionnum);
    dprintf("Got SESSIONKEY2 (0x%08x)\n",*sessionkey);
# endif
    
    /* do the UDPTEST stuff */
    if (!(packet = packet_create(packet_class_udp)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_sessionaddr2));
    packet_set_type(packet,CLIENT_SESSIONADDR2);
    /* FIXME: now that session keys are delayed, we should do this later.  When do the real clients send it? */
    bn_int_set(&packet->u.client_sessionaddr2.sessionkey,*sessionkey);
    bn_int_set(&packet->u.client_sessionaddr2.sessionnum,*sessionnum);
    /* send it twice in case one gets lost */
    if (psock_sendto(lsock,
		     packet_get_raw_data_const(packet,0),packet_get_size(packet),
		     0,(struct sockaddr *)saddr,(psock_t_socklen)sizeof(*saddr))!=(int)packet_get_size(packet))
	fprintf(stderr,"%s: failed to send SESSIONADDR (psock_sendto: %s)",progname,strerror(psock_errno()));
    if (psock_sendto(lsock,
		     packet_get_raw_data_const(packet,0),packet_get_size(packet),
		     0,(struct sockaddr *)saddr,(psock_t_socklen)sizeof(*saddr))!=(int)packet_get_size(packet))
	fprintf(stderr,"%s: failed to send SESSIONADDR (psock_sendto: %s)",progname,strerror(psock_errno()));
    packet_del_ref(packet);
    
    if (lsock>=0)
	if ((lsock = client_udptest_recv(progname,lsock,lsock_port,CLIENT_MAX_UDPTEST_WAIT))>=0)
	    dprintf("Got UDP data on port %hu\n",lsock_port);
#endif
    
#ifdef OLD_PROT
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_AUTHREQ1 &&
	   packet_get_type(rpacket)!=SERVER_AUTHREPLY1);
    
    if (packet_get_type(rpacket)==SERVER_AUTHREQ1) /* hmm... server wants to check the version number */
    {
	dprintf("Got AUTHREQ1\n");
	/* FIXME: get filename and equation */
	
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",progname);
	    goto error_rpacket;
	}
	packet_set_size(packet,sizeof(t_client_authreq1));
	packet_set_type(packet,CLIENT_AUTHREQ1);
	bn_int_tag_set(&packet->u.client_authreq1.archtag,archtag);
	bn_int_tag_set(&packet->u.client_authreq1.clienttag,clienttag);
	
	{
	    unsigned int  versionid;
	    unsigned int  gameversion;
            char const *  exeinfo;
	    
	    get_defversioninfo(progname,clienttag,&versionid,&gameversion,&exeinfo);
	    
	    bn_int_set(&packet->u.client_authreq1.versionid,versionid);
	    bn_int_set(&packet->u.client_authreq1.gameversion,gameversion);
	    packet_append_string(packet,exeinfo);
	}
	
	bn_int_set(&packet->u.client_authreq1.checksum,0x12345678); /* FIXME: bogus value */
	client_blocksend_packet(sd,packet);
	packet_del_ref(packet);
	
	/* now wait for reply */
	do
	    if (client_blockrecv_packet(sd,rpacket)<0)
	    {
		fprintf(stderr,"%s: server closed connection\n",progname);
		goto error_rpacket;
	    }
	while (packet_get_type(rpacket)!=SERVER_AUTHREPLY1);
    }
    dprintf("Got AUTHREPLY1\n");
#else
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_AUTHREQ_109 &&
	   packet_get_type(rpacket)!=SERVER_AUTHREPLY_109);
    
    if (packet_get_type(rpacket)==SERVER_AUTHREQ_109) /* hmm... server wants to check the version number */
    {
	dprintf("Got AUTHREQ_109\n");
	*sessionkey = bn_int_get(rpacket->u.server_authreq_109.sessionkey);
	*sessionnum = bn_int_get(rpacket->u.server_authreq_109.sessionnum);
	/* FIXME: also get filename and equation */

	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",progname);
	    goto error_rpacket;
	}
	packet_set_size(packet,sizeof(t_client_authreq_109));
	packet_set_type(packet,CLIENT_AUTHREQ_109);
	bn_int_set(&packet->u.client_authreq_109.ticks,time(NULL));
	
	{
	    unsigned int versionid;
	    unsigned int gameversion;
            char const * exeinfo;
            t_cdkey_info cdkey_info;
	    
	    get_defversioninfo(progname,clienttag,&versionid,&gameversion,&exeinfo);
	    
	    bn_int_set(&packet->u.client_authreq_109.gameversion,gameversion);

	    bn_int_set(&packet->u.client_authreq_109.cdkey_number,1); /* only one */
	    memset(&cdkey_info,'0',sizeof(cdkey_info));
	    packet_append_data(packet,&cdkey,sizeof(cdkey_info)); /* cdkey - bogus for now */
	    packet_append_string(packet,exeinfo);
	    packet_append_string(packet,cdowner);
	}
	
	bn_int_set(&packet->u.client_authreq_109.checksum,0x12345678); /* FIXME: bogus value */
	client_blocksend_packet(sd,packet);
	packet_del_ref(packet);
	
	/* now wait for reply */
	do
	    if (client_blockrecv_packet(sd,rpacket)<0)
	    {
		fprintf(stderr,"%s: server closed connection\n",progname);
		goto error_rpacket;
	    }
	while (packet_get_type(rpacket)!=SERVER_AUTHREPLY_109);
    }
    else
	fprintf(stderr,"We didn't get a sessionkey, don't expect login to work!");
    dprintf("Got AUTHREPLY_109\n");
#endif
    
    if (!(packet = packet_create(packet_class_bnet)))
    {
	fprintf(stderr,"%s: could not create packet\n",progname);
	goto error_rpacket;
    }
    packet_set_size(packet,sizeof(t_client_iconreq));
    packet_set_type(packet,CLIENT_ICONREQ);
    client_blocksend_packet(sd,packet);
    packet_del_ref(packet);
    
    do
        if (client_blockrecv_packet(sd,rpacket)<0)
	{
	    fprintf(stderr,"%s: server closed connection\n",progname);
	    goto error_rpacket;
	}
    while (packet_get_type(rpacket)!=SERVER_ICONREPLY);
    dprintf("Got ICONREPLY\n");
    
    if (strcmp(clienttag,CLIENTTAG_STARCRAFT)==0 ||
	strcmp(clienttag,CLIENTTAG_BROODWARS)==0 ||
	strcmp(clienttag,CLIENTTAG_WARCIIBNE)==0)
    {
	if (!(packet = packet_create(packet_class_bnet)))
	{
	    fprintf(stderr,"%s: could not create packet\n",progname);
	    goto error_rpacket;
	}
#ifdef OLD_CDKEY
	packet_set_size(packet,sizeof(t_client_cdkey));
	packet_set_type(packet,CLIENT_CDKEY);
	bn_int_set(&packet->u.client_cdkey.spawn,CLIENT_CDKEY_SPAWN_FALSE); /* FIXME: add option */
	packet_append_string(packet,cdkey);
	packet_append_string(packet,cdowner);
	client_blocksend_packet(sd,packet);
	packet_del_ref(packet);
	
	do
	    if (client_blockrecv_packet(sd,rpacket)<0)
	    {
		fprintf(stderr,"%s: server closed connection\n",progname);
		goto error_rpacket;
	    }
	while (packet_get_type(rpacket)!=SERVER_CDKEYREPLY);
	dprintf("Got CDKEYREPLY (%u)\n",bn_int_get(rpacket->u.server_cdkeyreply.message));
#else
        {
            struct
            {
                bn_int sessionkey;
                bn_int ticks;
                bn_int productid;
                bn_int keyvalue1;
                bn_int keyvalue2;
            }            temp;
            t_hash       key_hash;
            unsigned int ticks;
            unsigned int keyvalue1,keyvalue2,productid;
            
            if (key_interpret(cdkey,&productid,&keyvalue1,&keyvalue2)<0)
	    {
                fprintf(stderr,"%s: specified key is not valid, sending junk\n",progname);
                productid = 0;
                keyvalue1 = 0;
                keyvalue2 = 0;
	    }
	    
	    ticks = 0; /* FIXME: what to use here? */
	    bn_int_set(&temp.ticks,ticks);
	    bn_int_set(&temp.sessionkey,*sessionkey);
	    bn_int_set(&temp.productid,productid);
	    bn_int_set(&temp.keyvalue1,keyvalue1);
	    bn_int_set(&temp.keyvalue2,keyvalue2);
	    bnet_hash(&key_hash,sizeof(temp),&temp);
	    
	    packet_set_size(packet,sizeof(t_client_cdkey2));
	    packet_set_type(packet,CLIENT_CDKEY2);
	    bn_int_set(&packet->u.client_cdkey2.spawn,CLIENT_CDKEY2_SPAWN_FALSE); /* FIXME: add option */
	    bn_int_set(&packet->u.client_cdkey2.keylen,strlen(cdkey));
	    bn_int_set(&packet->u.client_cdkey2.productid,productid);
	    bn_int_set(&packet->u.client_cdkey2.keyvalue1,keyvalue1);
	    bn_int_set(&packet->u.client_cdkey2.sessionkey,*sessionkey);
	    bn_int_set(&packet->u.client_cdkey2.ticks,ticks);
	    hash_to_bnhash((t_hash const *)&key_hash,packet->u.client_cdkey2.key_hash); /* avoid warning */
	    packet_append_string(packet,cdowner);
	    client_blocksend_packet(sd,packet);
	    packet_del_ref(packet);
	}

        do
            if (client_blockrecv_packet(sd,rpacket)<0)
            {
                fprintf(stderr,"%s: server closed connection\n",progname);
		goto error_rpacket;
            }
        while (packet_get_type(rpacket)!=SERVER_CDKEYREPLY2);
        dprintf("Got CDKEYREPLY2 (%u)\n",bn_int_get(rpacket->u.server_cdkeyreply2.message));
#endif
    }
    
    packet_destroy(rpacket);
    return sd;
    
    /* error cleanup */
    
    error_rpacket:
    
	packet_destroy(rpacket);
    
    error_lsock:
    
	if (lsock>=0)
	    psock_close(lsock);
    
    error_sd:
    
	psock_close(sd);
    
    return -1;
}

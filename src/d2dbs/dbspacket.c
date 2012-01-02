/*
 * Copyright (C) 2001		sousou	(liupeng.cs@263.net)
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
#include "setup.h"

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#include "compat/statmacros.h"
#include "compat/mkdir.h"
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
#include "compat/memmove.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
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
#include "compat/psock.h"

#include "dbserver.h"
#include "dbspacket.h"
#include "d2ladder.h"
#include "charlock.h"
#include "prefs.h"
#include "d2cs/d2cs_d2gs_character.h"
#include "common/bn_type.h"
#include "common/list.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static unsigned int dbs_packet_savedata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen);
static unsigned int dbs_packet_savedata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen);
static unsigned int dbs_packet_getdata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize);
static unsigned int dbs_packet_getdata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize);
static unsigned int dbs_packet_echoreply(t_d2dbs_connection* conn);
static unsigned int dbs_packet_getdata(t_d2dbs_connection* conn);
static unsigned int dbs_packet_savedata(t_d2dbs_connection* conn);
static unsigned int dbs_packet_charlock(t_d2dbs_connection* conn);
static unsigned int dbs_packet_updateladder(t_d2dbs_connection* conn);
static int dbs_verify_ipaddr(char const * addrlist,t_d2dbs_connection * c);

static unsigned int dbs_packet_savedata_charsave(t_d2dbs_connection* conn, char * AccountName,char * CharName,char * data,unsigned int datalen)
{
	char filename[MAX_PATH];
	char savefile[MAX_PATH];
	char bakfile[MAX_PATH];
	unsigned short curlen,readlen,leftlen,writelen;
	int	fd;

	sprintf(filename,"%s/.%s.tmp",prefs_get_charsave_dir(),CharName);
	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE );
	if (fd<=0) {
		log_error("open() failed : %s",filename);
		return 0;
	}
	curlen=0;
	leftlen=datalen;
	while(curlen<datalen) {
		if(leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=write(fd,data+curlen,writelen);
		if (readlen<=0) {
			close(fd);
			log_error("write() failed error : %s",strerror(errno));
			return 0;
		}
		curlen+=readlen;
		leftlen-=readlen;
	}
	close(fd);

	sprintf(bakfile,"%s/%s",prefs_get_charsave_bak_dir(),CharName);
	sprintf(savefile,"%s/%s",prefs_get_charsave_dir(),CharName);
	if (rename(savefile, bakfile)==-1) {
		log_warn("error rename %s to %s", savefile, bakfile);
	}
	if (rename(filename, savefile)==-1) {
		log_error("error rename %s to %s", filename, savefile);
		return 0;
	}
	log_info("saved charsave %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return datalen;
}

static unsigned int dbs_packet_savedata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int datalen)
{
	char savefile[MAX_PATH];
	char bakfile[MAX_PATH];
	char filepath[MAX_PATH];
	char filename[MAX_PATH];
	int  fd;
	unsigned short curlen,readlen,leftlen,writelen;
	struct stat statbuf;

	sprintf(filepath,"%s/%s",prefs_get_charinfo_bak_dir(),AccountName);
	if(stat(filepath,&statbuf)==-1) {
		mkdir(filepath,S_IRWXU|S_IRWXG|S_IRWXO );
		log_info("created charinfo directory: %s",filepath);
	}

	sprintf(filename,"%s/%s/.%s.tmp",prefs_get_charinfo_dir(),AccountName,CharName);
	fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE );
	if (fd<=0) {
		log_error("open() failed : %s",filename);
		return 0;
	}

	curlen=0;
	leftlen=datalen;
	while(curlen<datalen) {
		if(leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=write(fd,data+curlen,writelen);
		if (readlen<=0) {
			close(fd);
			log_error("write() failed error : %s",strerror(errno));
			return 0;
		}
		curlen+=readlen;
		leftlen-=readlen;
	}
	close(fd);

	sprintf(bakfile,"%s/%s/%s",prefs_get_charinfo_bak_dir(),AccountName,CharName);
	sprintf(savefile,"%s/%s/%s",prefs_get_charinfo_dir(),AccountName,CharName);
	if (rename(savefile, bakfile)==-1) {
		log_info("error rename %s to %s", savefile, bakfile);
	}
	if (rename(filename, savefile)==-1) {
		log_error("error rename %s to %s", filename, savefile);
		return 0;
	}
	log_info("saved charinfo %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return datalen;
}

static unsigned int dbs_packet_getdata_charsave(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize)
{
	char filename[MAX_PATH];
	int				fd;
	unsigned short curlen,readlen,leftlen,writelen;
	long filesize;
	sprintf(filename,"%s/%s",prefs_get_charsave_dir(),CharName);
	fd = open(filename, O_RDONLY);
	if (fd<=0) {
		log_error("open() failed : %s",filename);
		return 0;
	}
	filesize=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	if(filesize==-1) {
		close(fd);
		log_error("lseek() failed");
		return 0;
	}
	if(bufsize < filesize) {
		close(fd);
		log_error("not enough buffer");
		return 0;
	}

	curlen=0;
	leftlen=filesize;
	while(curlen < filesize) {
		if(leftlen>2000) writelen=2000;
		else writelen=leftlen;
		readlen=read(fd,data+curlen,writelen);
		if (readlen<=0) {
			close(fd);
			log_error("read() failed error : %s",strerror(errno));
			return 0;
		}
		leftlen-=readlen;
		curlen+=readlen;
	}
	close(fd);
	log_info("loaded charsave %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return filesize;
}

static unsigned int dbs_packet_getdata_charinfo(t_d2dbs_connection* conn,char * AccountName,char * CharName,char * data,unsigned int bufsize)
{
	char filename[MAX_PATH];
	int  fd;
	unsigned short curlen,readlen,leftlen,writelen;
	long filesize;

	sprintf(filename,"%s/%s/%s",prefs_get_charinfo_dir(),AccountName,CharName);
	fd = open(filename, O_RDONLY);
	if (fd<=0) {
		log_error("open() failed : %s",filename);
		return 0;
	}
	filesize=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	if(filesize==-1) {
		close(fd);
		log_error("lseek() failed");
		return 0;
	}
	if(bufsize < filesize) {
		close(fd);
		log_error("not enough buffer");
		return 0;
	}

	curlen=0;
	leftlen=filesize;
	while(curlen < filesize) {
		if(leftlen>2000)
			writelen=2000;
		else
			writelen=leftlen;
		readlen=read(fd,data+curlen,writelen);
	    if (readlen<=0)
		{
			close(fd);
			log_error("read() failed error : %s",strerror(errno));
			return 0;
		}
		leftlen-=readlen;
		curlen+=readlen;
	}
	close(fd);
	log_info("loaded charinfo %s(*%s) for gs %s(%d)", CharName, AccountName, conn->serverip, conn->serverid);
	return filesize;
}

static unsigned int dbs_packet_savedata(t_d2dbs_connection * conn)
{
	unsigned short writelen;
	unsigned short      datatype;
	unsigned short      datalen; 
	unsigned int        result; 
	char AccountName[MAX_NAME_LEN+16];
	char CharName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_save_data_request	* savecom; 
	t_d2dbs_d2gs_save_data_reply	* saveret; 
	char * readpos;
	unsigned char * writepos;
	
	readpos=conn->ReadBuf;
	savecom=(t_d2gs_d2dbs_save_data_request	*)readpos;
	datatype=bn_short_get(savecom->datatype);
	datalen=bn_short_get(savecom->datalen);

	readpos+=sizeof(*savecom);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if(readpos+datalen!=conn->ReadBuf+bn_short_get(savecom->h.size)) {
		log_error("request packet size error");
		return -1;
	}

	if(datatype==D2GS_DATA_CHARSAVE) {
		if(dbs_packet_savedata_charsave(conn,AccountName,CharName,readpos,datalen)>0) {
			result=D2DBS_SAVE_DATA_SUCCESS;
		} else {
			datalen=0;
			result=D2DBS_SAVE_DATA_FAILED    ;
		}
	} else if(datatype==D2GS_DATA_PORTRAIT) {
		if(dbs_packet_savedata_charinfo(conn,AccountName,CharName,readpos,datalen)>0) {
			result=D2DBS_SAVE_DATA_SUCCESS;
		} else {
			datalen=0;
			result=D2DBS_SAVE_DATA_FAILED;
		}
	} else {
		log_error("unknown data type %d",datatype);
		return -1;
	}
	writelen=sizeof(*saveret)+strlen(CharName)+1;
	if(writelen > kBufferSize-conn->nCharsInWriteBuffer) return 0;
	writepos=conn->WriteBuf+conn->nCharsInWriteBuffer;
	saveret=(t_d2dbs_d2gs_save_data_reply *)writepos;
	bn_short_set(&saveret->h.type, D2DBS_D2GS_SAVE_DATA_REPLY);
	bn_short_set(&saveret->h.size,writelen);
	bn_int_set(&saveret->h.seqno,bn_int_get(savecom->h.seqno));
	bn_short_set(&saveret->datatype,bn_short_get(savecom->datatype));
	bn_int_set(&saveret->result,result);
	writepos+=sizeof(*saveret);
	strncpy(writepos,CharName,MAX_NAME_LEN);
	conn->nCharsInWriteBuffer += writelen;
	return 1;
}

static unsigned int dbs_packet_echoreply(t_d2dbs_connection * conn)
{
	conn->last_active=time(NULL);
	return 1;
}

static unsigned int dbs_packet_getdata(t_d2dbs_connection * conn)
{
	unsigned short	writelen;
	unsigned short	datatype;
	unsigned short	datalen; 
	unsigned int	result; 
	char		AccountName[MAX_NAME_LEN+16];
	char		CharName[MAX_NAME_LEN+16];
	char		RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_get_data_request	* getcom; 
	t_d2dbs_d2gs_get_data_reply	* getret; 
	char		* readpos;
	char		* writepos;
	char		databuf[kBufferSize ];
	t_d2charinfo_file charinfo;
	unsigned short	charinfolen;
	unsigned int	gsid;

	readpos=conn->ReadBuf;
	getcom=(t_d2gs_d2dbs_get_data_request *)readpos;
	datatype=bn_short_get(getcom->datatype);
	
	readpos+=sizeof(*getcom);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if(readpos != conn->ReadBuf+bn_short_get(getcom->h.size)) {
		log_error("request packet size error");
		return -1;
	}
	writepos=conn->WriteBuf+conn->nCharsInWriteBuffer;
	getret=(t_d2dbs_d2gs_get_data_reply *)writepos;
	datalen=0;
	if(datatype==D2GS_DATA_CHARSAVE) {
		if(cl_query_charlock_status(CharName,RealmName,&gsid)!=0) {
			log_warn("char %s(*%s)@%s is already locked on gs %u",CharName,AccountName,RealmName,gsid);
			result=D2DBS_GET_DATA_CHARLOCKED;
		} else if (cl_lock_char(CharName,RealmName,conn->serverid) != 0) {
			log_error("failed to lock char %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
			result=D2DBS_GET_DATA_CHARLOCKED;
		} else {
			log_info("lock char %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
			datalen=dbs_packet_getdata_charsave(conn,AccountName,CharName,databuf,kBufferSize );
			if(datalen>0) {
				result=D2DBS_GET_DATA_SUCCESS;
				charinfolen=dbs_packet_getdata_charinfo(conn,AccountName,CharName,(char *)&charinfo,sizeof(charinfo));
				if(charinfolen>0) {
					result=D2DBS_GET_DATA_SUCCESS;
				} else {
					result=D2DBS_GET_DATA_FAILED;
					if (cl_unlock_char(CharName,RealmName)!=0) {
						log_error("failed to unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
							AccountName,RealmName,conn->serverip,conn->serverid);
					} else {
						log_info("unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
							AccountName,RealmName,conn->serverip,conn->serverid);
					}
				}
			} else {
				datalen=0;
				result=D2DBS_GET_DATA_FAILED;
				if (cl_unlock_char(CharName,RealmName)!=0) {
					log_error("faled to unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
						AccountName,RealmName,conn->serverip,conn->serverid);
				} else {
					log_info("unlock char %s(*%s)@%s for gs %s(%d)",CharName,\
						AccountName,RealmName,conn->serverip,conn->serverid);
				}
					
			}
		}
		if (result==D2DBS_GET_DATA_SUCCESS) {
			bn_int_set(&getret->charcreatetime,bn_int_get(charinfo.header.create_time));
			/* FIXME: this should be rewritten to support string formatted time */
			if (bn_int_get(charinfo.header.create_time)>=prefs_get_ladderinit_time()) {
				bn_int_set(&getret->allowladder,1);
			} else {
				bn_int_set(&getret->allowladder,0);
			}
		} else {
			bn_int_set(&getret->charcreatetime,0);
			bn_int_set(&getret->allowladder,0);
		}
	} else if(datatype==D2GS_DATA_PORTRAIT) {
		datalen=dbs_packet_getdata_charinfo(conn,AccountName,CharName,databuf,kBufferSize );
		if(datalen>0) result=D2DBS_GET_DATA_SUCCESS    ;
		else { 
			datalen=0;
			result=D2DBS_GET_DATA_FAILED    ;
		}
	} else {
		log_error("unknown data type %d",datatype);
		return -1;
	}
	writelen=datalen+sizeof(*getret)+strlen(CharName)+1;
	if(writelen > kBufferSize-conn->nCharsInWriteBuffer) return 0;
	bn_short_set(&getret->h.type,D2DBS_D2GS_GET_DATA_REPLY);
	bn_short_set(&getret->h.size,writelen);
	bn_int_set(&getret->h.seqno,bn_int_get(getcom->h.seqno));
	bn_short_set(&getret->datatype,bn_short_get(getcom->datatype));
	bn_int_set(&getret->result,result);
	bn_short_set(&getret->datalen,datalen);
	writepos+=sizeof(*getret);
	strncpy(writepos,CharName,MAX_NAME_LEN);
	writepos+=strlen(CharName)+1;
	if (datalen) memcpy(writepos,databuf,datalen);
	conn->nCharsInWriteBuffer += writelen;
	return 1;
}

static unsigned int dbs_packet_updateladder(t_d2dbs_connection * conn)
{
	char CharName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_update_ladder	* updateladder; 
	char * readpos;
	t_d2ladder_info			charladderinfo;

	readpos=conn->ReadBuf;
	updateladder=(t_d2gs_d2dbs_update_ladder *)readpos;
	
	readpos+=sizeof(*updateladder);
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;
	if(readpos != conn->ReadBuf+bn_short_get(updateladder->h.size)) {
		log_error("request packet size error");
		return -1;
	}

	strcpy(charladderinfo.charname,CharName);
	charladderinfo.experience=bn_int_get(updateladder->charexplow);
	charladderinfo.level=bn_int_get(updateladder->charlevel);
	charladderinfo.status=bn_short_get(updateladder->charstatus);
	charladderinfo.class=bn_short_get(updateladder->charclass);
	log_info("update ladder for %s@%s for gs %s(%d)",CharName,RealmName,conn->serverip,conn->serverid);
	d2ladder_update(&charladderinfo);
	return 1;
}

static unsigned int dbs_packet_charlock(t_d2dbs_connection * conn)
{
	char CharName[MAX_NAME_LEN+16];
	char AccountName[MAX_NAME_LEN+16];
	char RealmName[MAX_NAME_LEN+16];
	t_d2gs_d2dbs_char_lock * charlock; 
	char * readpos;

	readpos=conn->ReadBuf;
	charlock=(t_d2gs_d2dbs_char_lock*)readpos;
	
	readpos+=sizeof(*charlock);
	strncpy(AccountName,readpos,MAX_NAME_LEN);
	AccountName[MAX_NAME_LEN]=0;
	readpos+=strlen(AccountName)+1;
	strncpy(CharName,readpos,MAX_NAME_LEN);
	CharName[MAX_NAME_LEN]=0;
	readpos+=strlen(CharName)+1;
	strncpy(RealmName,readpos,MAX_NAME_LEN);
	RealmName[MAX_NAME_LEN]=0;
	readpos+=strlen(RealmName)+1;

	if(readpos != conn->ReadBuf+ bn_short_get(charlock->h.size)) {
		log_error("request packet size error");
		return -1;
	}

	if(bn_int_get(charlock->lockstatus)) {
		if (cl_lock_char(CharName,RealmName,conn->serverid)!=0) {
			log_error("failed to lock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		} else {
			log_info("lock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		}
	} else {
		if (cl_unlock_char(CharName,RealmName) != 0) {
			log_error("failed to unlock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		} else {
			log_info("unlock character %s(*%s)@%s for gs %s(%d)",CharName,AccountName,RealmName,conn->serverip,conn->serverid);
		}
	}
	return 1;
}

/*
	return value:
	1  :  process one or more packet
	0  :  not get a whole packet,do nothing
	-1 :  error
*/
extern int dbs_packet_handle(t_d2dbs_connection* conn) 
{
	unsigned short		readlen,writelen;
	t_d2dbs_d2gs_header	* readhead;
	unsigned short		retval; 

	if(conn->stats==0) {
		if(conn->nCharsInReadBuffer<sizeof(t_d2gs_d2dbs_connect)) {
			return 0;
		}
		conn->stats=1;
		conn->type=conn->ReadBuf[0];

		if(conn->type==CONNECT_CLASS_D2GS_TO_D2DBS) {
			if (dbs_verify_ipaddr(prefs_get_d2gs_list(),conn)<0) {
				log_error("d2gs connection from unknown ip address");
				return -1;
			}
			readlen=1;
			writelen=0;
			log_info("set connection type for gs %s(%d) on socket %d", conn->serverip, conn->serverid, conn->sd);
			log_d2gs("set connection type for gs %s(%d) on socket %d", conn->serverip, conn->serverid, conn->sd);
		} else {
			log_error("unknown connection type");
			return -1;
		}
		conn->nCharsInReadBuffer -= readlen;
		memmove(conn->ReadBuf,conn->ReadBuf+readlen,conn->nCharsInReadBuffer);
	} else if(conn->stats==1) {
		if(conn->type==CONNECT_CLASS_D2GS_TO_D2DBS) {
			while (conn->nCharsInReadBuffer >= sizeof(*readhead)) {
				readhead=(t_d2dbs_d2gs_header *)conn->ReadBuf;
				readlen=bn_short_get(readhead->size);
				if(conn->nCharsInReadBuffer < readlen) break;
				switch(bn_short_get(readhead->type)) {
					case D2GS_D2DBS_SAVE_DATA_REQUEST:
						retval=dbs_packet_savedata(conn);
						break;
					case D2GS_D2DBS_GET_DATA_REQUEST:
						retval=dbs_packet_getdata(conn);
						break;
					case D2GS_D2DBS_UPDATE_LADDER:
						retval=dbs_packet_updateladder(conn);
						break;
					case D2GS_D2DBS_CHAR_LOCK:
						retval=dbs_packet_charlock(conn);
						break;
					case D2GS_D2DBS_ECHOREPLY:
						retval=dbs_packet_echoreply(conn);
						break;
					default:
						log_error("unknown request type %d",\
							bn_short_get(readhead->type));
						retval=-1;
				}
				if (retval!=1) return retval;
				conn->nCharsInReadBuffer -= readlen;
				memmove(conn->ReadBuf,conn->ReadBuf+readlen,conn->nCharsInReadBuffer);
			}
		} else {
			log_error("unknown connection type %d",conn->type);
			return -1;
		}
	} else {
		log_error("unknown connection stats");
		return -1;
	}
	return 1;
}

/* FIXME: we should save client ipaddr into c->ipaddr after accept */
static int dbs_verify_ipaddr(char const * addrlist,t_d2dbs_connection * c)
{
	struct	in_addr		in;
	char			* adlist;
	char const		* ipaddr;
	char			* s, * temp;
	t_elem			* elem;
	t_d2dbs_connection	* tempc;
	unsigned int		valid;

	in.s_addr=htonl(c->ipaddr);
	ipaddr=inet_ntoa(in);
	if (!(adlist = strdup(addrlist))) return -1;
	temp=adlist;
	valid=0;
	while ((s=strsep(&temp, ","))) {
		if (!strcmp(ipaddr,s)) {
			valid=1;
			break;
		}
	}
	free(adlist);
	if (valid) {
		log_info("ip address %s is valid",ipaddr);
		LIST_TRAVERSE(dbs_server_connection_list,elem)
		{
			if (!(tempc=elem_get_data(elem))) continue;
			if (tempc !=c && tempc->ipaddr==c->ipaddr) {
				log_info("destroying previous connection %d",tempc->serverid);
				dbs_server_shutdown_connection(tempc);
				list_remove_elem(dbs_server_connection_list,elem);
			}
		}
		c->verified = 1;
		return 0;
	} else {
		log_info("ip address %s is invalid",ipaddr);
	}
	return -1;
}

int dbs_check_timeout(void)
{
	t_elem				*elem;
	t_d2dbs_connection		*tempc;
	time_t				now;
	unsigned int			timeout;

	now=time(NULL);
	timeout=prefs_get_idletime();
	LIST_TRAVERSE(dbs_server_connection_list,elem)
	{
		if (!(tempc=elem_get_data(elem))) continue;
		if (now-tempc->last_active>timeout) {
			log_debug("connection %d timed out",tempc->serverid);
			dbs_server_shutdown_connection(tempc);
			list_remove_elem(dbs_server_connection_list,elem);
			continue;
		}
	}
	return 0;
}

int dbs_keepalive(void)
{
	t_elem				*elem;
	t_d2dbs_connection		*tempc;
	t_d2dbs_d2gs_echorequest	*echoreq;
	unsigned short			writelen;
	unsigned char			*writepos;
	time_t				now;

	writelen = sizeof(t_d2dbs_d2gs_echorequest);
	now=time(NULL);
	LIST_TRAVERSE(dbs_server_connection_list,elem)
	{
		if (!(tempc=elem_get_data(elem))) continue;
		if (writelen > kBufferSize - tempc->nCharsInWriteBuffer) continue;
		writepos = tempc->WriteBuf + tempc->nCharsInWriteBuffer;
		echoreq  = (t_d2dbs_d2gs_echorequest*)writepos;
		bn_short_set(&echoreq->h.type, D2DBS_D2GS_ECHOREQUEST);
		bn_short_set(&echoreq->h.size, writelen);
		/* FIXME: sequence number not set */
		bn_int_set(&echoreq->h.seqno,  0);
		tempc->nCharsInWriteBuffer += writelen;
	}
	return 0;
}

/*
 * Copyright (C) 2000,2001	Onlyer	(onlyer@263.net)
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

#include <stdio.h>
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
#include "compat/memset.h"

#include "xstring.h"
#include "conf.h"
#include "common/util.h"
#include "common/eventlog.h"
#include "common/setup_after.h"

static int conf_set_default(t_conf_table * conf_table, void * param_data, int datalen);
static int conf_set_value(t_conf_table * conf, void * param_data, char * value);
static int conf_int_set(void * data, int value);
static int conf_bool_set(void * data, int value);
static int conf_str_set(void * data, char * value);
static int conf_hexstr_set(void * data, char * value);
static int conf_type_get_size(e_conf_type type);

static int conf_int_set(void * data, int value)
{
	int * p;

	p=(int *)data;
	*p=value;
	return 0;
}

static int conf_bool_set(void * data, int value)
{
	char * p;

	p=(char *)data;
	*p=tf(value);
	return 0;
}

static int conf_str_set(void * data, char * value)
{
	char * * p;
	char	* tmp;
	
	p=(char * *)data;
	tmp=NULL;
	if (value && !(tmp=strdup(value))) return -1;
	if (*p) free(*p);
	*p=tmp;
	return 0;
}

static int conf_hexstr_set(void * data, char * value)
{
	char * * p;
	char * tmp;

	p=(char * *)data;
	tmp=NULL;
	if (value && !(tmp=hexstrdup(value))) return -1;
	if (*p) free(*p);
	*p=tmp;
	return 0;
}

static int conf_set_default(t_conf_table * conf_table, void * param_data, int datalen)
{
	unsigned int	i;
	char		* p;

	ASSERT(conf_table,-1);
	ASSERT(param_data,-1);
	memset(param_data,0,datalen);
	for (i=0; conf_table[i].name; i++) {
		if (conf_table[i].offset + conf_type_get_size(conf_table[i].type) > datalen) {
			log_error("conf table item %d bad offset %d exceed %d",i,conf_table[i].offset,datalen);
			return -1;
		}
		p=(char *)param_data+conf_table[i].offset;
		switch (conf_table[i].type) {
			CASE(conf_type_bool, conf_bool_set(p, conf_table[i].def_value));
			CASE(conf_type_int,  conf_int_set(p, conf_table[i].def_value));
			CASE(conf_type_str,  conf_str_set(p, (char *)conf_table[i].def_value));
			CASE(conf_type_hexstr,  conf_hexstr_set(p, (char *)conf_table[i].def_value));
			default:
				log_error("conf table item %d bad type %d",i,conf_table[i].type);
				return -1;
		}
	}
	return 0;
}

static int conf_set_value(t_conf_table * conf, void * param_data, char * value)
{
	char * p;

	ASSERT(conf,-1);
	ASSERT(param_data,-1);
	ASSERT(value,-1);
	p=(char *)param_data+conf->offset;
	switch (conf->type) {
		CASE(conf_type_bool, conf_bool_set(p, strtoul(value,NULL,0)));
		CASE(conf_type_int,  conf_int_set(p, strtoul(value,NULL,0)));
		CASE(conf_type_str,  conf_str_set(p, value));
		CASE(conf_type_hexstr,  conf_hexstr_set(p, value));
		default:
			log_error("got bad conf type %d",conf->type);
			return -1;
	}
	return 0;
}

static int conf_type_get_size(e_conf_type type)
{
	switch (type) {
		case conf_type_bool:
			return sizeof(char);
		case conf_type_int:
			return sizeof(int);
		case conf_type_str:
		case conf_type_hexstr:
			return sizeof(char *);
		default:
			log_error("got bad conf type %d",type);
			return 0;
	}
}

extern int conf_parse_param(int argc, char ** argv, t_conf_table * conf_table, void * data,  int datalen)
{
	unsigned int	i, j;
	char		* p;
	int		match;

	ASSERT(argc,-1);
	ASSERT(argv,-1);
	ASSERT(data, -1);
	if (conf_set_default(conf_table, data, datalen)<0) {
		log_error("error setting default values");
		return -1;
	}
	for (i=1; i< argc; i++) {
		match=0;
		for (j=0; conf_table[j].name; j++) {
			if (!strcmp(conf_table[j].name,argv[i])) {
				match=1;
				p=(char *)data + conf_table[j].offset;
				switch (conf_table[j].type) {
					case conf_type_bool:
						conf_bool_set(p,1);
						break;
					case conf_type_int:
						if (i+1>=argc) {
							log_error("got bad int conf %s without value",argv[i]);
							break;
						}
						i++;
						conf_int_set(p,strtoul(argv[i],NULL,0));
						break;
					case conf_type_str:
						if (i+1>=argc) {
							log_error("got bad str conf %s without value",argv[i]);
							break;
						}
						i++;
						conf_str_set(p,argv[i]);
						break;
					case conf_type_hexstr:
						if (i+1>=argc) {
							log_error("got bad hexstr conf %s without value",argv[i]);
							break;
						}
						i++;
						conf_hexstr_set(p,argv[i]);
						break;
					default:
						log_error("got bad conf type %d",conf_table[i].type);
						break;
				}
				break;
			}
		}
		if (!match) {
			log_error("bad option \"%s\"",argv[i]);
			return -1;
		}
	}
	return 0;
}

extern int conf_load_file(char const * filename, t_conf_table * conf_table, void * param_data, int datalen)
{
	FILE		* fp;
	unsigned int	i, line, count, match;
	char		* buff;
	char		* * item;

	ASSERT(conf_table,-1);
	ASSERT(param_data,-1);
	if (conf_set_default(conf_table, param_data, datalen)<0) {
		log_error("error setting default values");
		return -1;
	}
	if (!filename) return 0;
	if (!(fp=fopen(filename,"r"))) {
		log_error("error open file %s",filename);
		return -1;
	}
	for (line=1; (buff=file_get_line(fp)); line++) {
		if (buff[0]=='#') {
			free(buff);
			continue;
		}
		if (!(item=strtoargv(buff,&count))) {
			free(buff);
			continue;
		}
		free(buff);
		if (!count) {
			free(item);
			continue;
		}
		if (count!=3) {
			log_error("bad item count %d in file %s line %d",count,filename,line);
			free(item);
			continue;
		}
		if (strcmp(item[1],"=")) {
			log_error("missing '=' in file %s line %d",filename,line);
			free(item);
			continue;
		}
		match=0;
		for (i=0; conf_table[i].name; i++) {
			if (!strcasecmp(conf_table[i].name,item[0])) {
				conf_set_value(conf_table+i, param_data, item[2]);
				match=1;
			}
		}
		if (!match) {
			log_warn("got unknown field \"%s\" in line %d,(ignored)",item[0],line);
		}
		free(item);
	}
	fclose(fp);
	return 0;
}

extern int conf_cleanup(t_conf_table * conf_table, void * param_data, int datalen)
{
	unsigned int	i;
	char		* p;

	ASSERT(conf_table,-1);
	ASSERT(param_data,-1);
	for (i=0; conf_table[i].name; i++) {
		if (conf_table[i].offset + conf_type_get_size(conf_table[i].type) > datalen) {
			log_error("conf table item %d bad offset %d exceed %d",i,conf_table[i].offset,datalen);
			return -1;
		}
		
		p=(char *)param_data+conf_table[i].offset;
		switch (conf_table[i].type) {
			CASE(conf_type_bool, conf_bool_set(p, 0));
			CASE(conf_type_int,  conf_int_set(p, 0));
			CASE(conf_type_str,  conf_str_set(p, NULL));
			CASE(conf_type_hexstr,  conf_hexstr_set(p, NULL));
			default:
				log_error("conf table item %d bad type %d",i,conf_table[i].type);
				return -1;
		}
	}
	memset(param_data,0,datalen);
	return 0;
}

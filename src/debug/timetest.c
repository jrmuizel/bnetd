/*
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
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
/* A simple program to test the time conversion routines in bnettime.c */
#include "common/setup_before.h"
#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include "compat/exitstatus.h"
#include <stdio.h>
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
#include "bn_type.h"
#include "eventlog.h"
#include "bnettime.h"
#include "common/setup_after.h"


static void timeinfo(t_bnettime bnt)
{
    time_t  t;
    char    out[1024];
    bn_long num;
    
    bnettime_to_bn_long(bnt,&num);
    t = bnettime_to_time(bnt);
    strftime(out,sizeof(out),"%Y %b %d %H:%M:%S",localtime(&t));
    printf("%20s  %g   u:0x%08x l:0x%08x\n",out,(double)t,bn_long_get_a(num),bn_long_get_b(num));
}


extern int main(int argc, char * argv[])
{
    bn_long    num;
    t_bnettime bnt;
    
    if (argc<1 || !argv || !argv[0])
    {
        fprintf(stderr,"bad arguments\n");
        return STATUS_FAILURE;
    }
    
    eventlog_set(stderr);
    
    bnt = bnettime();
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01bedd2d,0xff4e2712);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01bf4444,0x09090909);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01be370a,0xffffffff);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01be8765,0x4321fedc);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01bb0000,0x40000000);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01bb0000,0x80000000);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x01bb0000,0xf0000000);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    bn_long_set_a_b(&num,0x004b0000,0xf0000000);
    bn_long_to_bnettime(num,&bnt);
    timeinfo(bnt);
    
    return STATUS_SUCCESS;
}

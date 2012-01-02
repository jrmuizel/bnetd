# Makefile for Diablo2 Control Server

basedir		= ../
commondir	= ${basedir}/common
compatdir	= ${basedir}/compat

SHELL		= /bin/sh

TOUCH		= touch

RMF		= rm -f

ECHO		= echo

TAGS		= /usr/bin/ctags

CC		= gcc
COMPILE		= ${CC} -DD2CS -DHAVE_CONFIG_H -DDEFAULT_CONF_FILE=\"${confdir}/d2cs.conf\" -I. -I"${basedir}"
OPTIONS		= -g2 -O2 -Wall
DEBUG_OPTIONS	= -Wshadow -Wredundant-decls -Wnested-externs -Wwrite-strings -Wcast-align\
-Wstrict-prototypes -Wmissing-declarations
LINK		= ${CC} -g2 -O2 -Wall

d2cs_SBIN	= ../../sbin/d2cs
d2cs_OBJECTS	= main.o d2gs.o handle_d2gs.o serverqueue.o connection.o game.o server.o\
handle_init.o handle_d2cs.o d2charfile.o xstring.o gamequeue.o conf.o prefs.o cmdline_parse.o\
bnetd.o handle_bnetd.o s2s.o net.o d2ladder.o handle_signal.o\
${commondir}/check_alloc.o ${commondir}/hashtable.o ${commondir}/hexdump.o\
${commondir}/eventlog.o ${commondir}/list.o ${commondir}/packet.o ${commondir}/bn_type.o\
${commondir}/addr.o ${commondir}/util.o ${commondir}/queue.o ${commondir}/network.o\
${compatdir}/inet_aton.o ${compatdir}/inet_ntoa.o ${compatdir}/strdup.o ${compatdir}/strerror.o \
${compatdir}/strtoul.o ${compatdir}/strftime.o ${compatdir}/strcasecmp.o ${compatdir}/strncasecmp.o\
${compatdir}/psock.o ${compatdir}/uname.o ${compatdir}/gettimeofday.o ${compatdir}/difftime.o \
${compatdir}/pdir.o

TARGET		= ${d2cs_SBIN}
OBJECTS		= ${d2cs_OBJECTS}

.PHONY: all
all: ${TARGET} 

.PHONY: clean
clean:
	@-${RMF} core
	@-${RMF} .tags
	@-${RMF} .depend
	@-${RMF} ${SBINS}
	@-${RMF} ${OBJECTS}

.c.o:
	${COMPILE} ${OPTIONS} ${DEBUG_OPTIONS} -c $< -o $@

${d2cs_SBIN}:${d2cs_OBJECTS}
	@${RMF} $@
	${LINK} ${d2cs_OBJECTS} ${LINK_LIBS} -o $@

tags:
	@-$(TAGS) --langmap=c -f .tags *.c *.h

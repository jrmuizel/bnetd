# Makefile for Database server

basedir		= ../
commondir	= ${basedir}/common
d2csdir		= ${basedir}/d2cs
compatdir	= ${basedir}/compat

SHELL		= /bin/sh

TOUCH		= touch

RMF		= rm -f

ECHO		= echo

TAGS		= /usr/bin/ctags

CC		= gcc
COMPILE		= ${CC} -DD2DBS -DHAVE_CONFIG_H -DDEFAULT_CONF_FILE=\"${confdir}/d2dbs.conf\" -I. -I"${basedir}"
OPTIONS		= -g2 -O2 -Wall
DEBUG_OPTIONS	= -Wshadow -Wredundant-decls -Wnested-externs -Wwrite-strings -Wcast-align\
-Wstrict-prototypes -Wmissing-declarations
LINK		= ${CC} -g2 -O2 -Wall

dbs_SBIN	= ../../sbin/d2dbs
dbs_OBJECTS	= main.o charlock.o d2ladder.o dbserver.o dbspacket.o cmdline_parse.o prefs.o\
handle_signal.o server.o\
${d2csdir}/conf.o ${d2csdir}/xstring.o\
${commondir}/list.o ${commondir}/eventlog.o ${commondir}/bn_type.o\
${commondir}/check_alloc.o ${commondir}/util.o\
${compatdir}/psock.o ${compatdir}/strerror.o

TARGET		= ${dbs_SBIN}
OBJECTS		= ${dbs_OBJECTS}

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

${dbs_SBIN}:${dbs_OBJECTS}
	@${RMF} $@
	${LINK} ${dbs_OBJECTS} ${LINK_LIBS} -o $@

tags:
	@-$(TAGS) --langmap=c -f .tags *.c *.h

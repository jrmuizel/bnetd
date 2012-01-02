
CC=gcc
CFLAGS=-g -O -DHAVE_CONFIG_H -I.. -I/usr/include/pcap
LDFLAGS=
LINK_LIBS=-lpcap

LINK=$(CC) $(LDFLAGS)
COMPILE=$(CC) $(CFLAGS) -c


BNPCAP_OBJS=bnpcap.o \
            ../common/hexdump.o \
            ../common/list.o \
            ../common/eventlog.o \
            ../common/packet.o \
            ../common/bn_type.o

all: bnpcap

bnpcap: $(BNPCAP_OBJS)
	$(LINK) -o bnpcap $(BNPCAP_OBJS) $(LINK_LIBS)

.c.o:
	${COMPILE} $< -o $@

clean:
	rm -f bnpcap bnpcap.o

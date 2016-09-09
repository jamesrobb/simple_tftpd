mCC = gcc
CPPFLAGS =
CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2 -Wall -Wextra -Wformat=2
#CFLAGS = -std=c11 -D_XOPEN_SOURCE=700 -O2
OBJFLAGS = -c
LDFLAGS =
LOADLIBES =
LDLIBS =

.DEFAULT: all
.PHONY: all
all: tftpd

tftpd: tftpd.o packet.o util.o clientconn.o
	$(mCC) tftpd.o packet.o util.o clientconn.o -o tftpd

tftpd.o: tftpd.c
	$(mCC) $(OBJFLAGS) $(CFLAGS) tftpd.c

packet.o: packet.c
	$(mCC) $(OBJFLAGS) $(CFLAGS) packet.c

util.o: util.c
	$(mCC) $(OBJFLAGS) $(CFLAGS) util.c

clientconn.o: clientconn.c
	$(mCC) $(OBJFLAGS) $(CFLAGS) clientconn.c

clean:
	rm -f *.o
	rm tftpd

distclean: clean
	rm -f tftpd

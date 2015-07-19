CFLAGS  += -Wall -W -pedantic -std=gnu99
LDFLAGS += -lm -lX11

PREFIX=/usr/local

PACKAGE=docker-ng
VERSION=0.1

sources=docker.c dclock.c dxkb.c dvol.c list.c dtray.c dwin.c icons.c net.c
headers=list.h dtray.h dwin.h icons.h net.ch version.h docker.h

all: prepare docker dclock dxkb dvol

docker: docker.o icons.o net.o list.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

dclock: dclock.o dtray.o dwin.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

dxkb: dxkb.o dtray.o dwin.o list.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

dvol: dvol.o dtray.o dwin.o list.o
	$(CC) $(CFLAGS) $(LDFLAGS) -lXpm -lasound $^ -o $@


%.o: %.c
	$(CC) -c $(CFLAGS) $<

prepare: version.h.in
	sed -e "s/@VERSION@/$(VERSION)/" version.h.in > version.h

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 docker $(DESTDIR)$(PREFIX)/bin/dockerng
	install -m 755 dclock $(DESTDIR)$(PREFIX)/bin/dclock
	install -m 755 dxkb $(DESTDIR)$(PREFIX)/bin/dxkb

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dockerng
	rm -f $(DESTDIR)$(PREFIX)/bin/dclock
	rm -f $(DESTDIR)$(PREFIX)/bin/dxkb

clean:
	rm -f *.o docker dclock dxkb dvol version.h

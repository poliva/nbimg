DESTDIR?=/
SHELL = /bin/sh
CC?=gcc
CFLAGS=-Wall -Wextra -Wwrite-strings -O -g 
INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644
PROGNAME = nbimg

srcdir = .
prefix = $(DESTDIR)
bindir = $(prefix)/usr/bin
docdir = $(prefix)/usr/share/doc
mandir = $(prefix)/usr/share/man

all: nbimg win32

nbimg: nbimg.c
	$(CC) $< $(CFLAGS) -o $@

win32:
	i586-mingw32msvc-gcc $(CFLAGS) $(PROGNAME).c -o $(PROGNAME).exe	

install: all
	mkdir -p $(bindir)
	$(INSTALL) $(PROGNAME) $(bindir)/$(PROGNAME)
	mkdir -p $(docdir)/$(PROGNAME)/
	$(INSTALLDATA) $(srcdir)/README.md $(docdir)/$(PROGNAME)/
	$(INSTALLDATA) $(srcdir)/LICENSE $(docdir)/$(PROGNAME)/

uninstall:
	rm -rf $(bindir)/$(PROGNAME)
	rm -rf $(docdir)/$(PROGNAME)/


clean:
	rm -f *.o $(PROGNAME) $(PROGNAME).exe

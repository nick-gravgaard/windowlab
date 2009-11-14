# Makefile for WindowLab

# Comment out to remove shape support (for X11R5 or just a tiny bin)
DEFINES += -DSHAPE
EXTRA_LIBS += -lXext

# Set this to the hardcoded location of all files if it's not /
PREFIX = /usr/local

# Set this to the directory, below PREFIX, where man pages 
# are expected. Below this directory, the target "install"
# will put "windowlab.1x" in section "man1".
MANBASE = /man

# Set this to the location of the X installation you want to compile against
XROOT = /usr/X11R6

# Some flexibility for configuration location
CONFPREFIX = $(PREFIX)
CONFDIR = /etc/X11/windowlab

# Set this to the location of the global configuration files
SYSCONFDIR = $(CONFPREFIX)$(CONFDIR)

# Information about the location of the menurc file
ifndef MENURC
MENURC = $(SYSCONFDIR)/windowlab.menurc
endif

DEFINES += -DDEF_MENURC="\"$(MENURC)\""

# Uncomment to add MWM hints support
#DEFINES += -DMWM_HINTS

# Uncomment to add freetype support (requires XFree86 4.0.2 or later)
# This needs -lXext above, even if you have disabled shape support
#DEFINES += -DXFT
#EXTRA_INC += `pkg-config --cflags xft`
#EXTRA_LIBS += `pkg-config --libs xft`

# Uncomment for debugging info (abandon all hope, ye who enter here)
#DEFINES += -DDEBUG

# --------------------------------------------------------------------

CC = gcc
ifndef CFLAGS
CFLAGS = -g -O2 -Wall -W
endif

BINDIR = $(DESTDIR)$(PREFIX)/bin
MANDIR = $(DESTDIR)$(PREFIX)$(MANBASE)/man1
CFGDIR = $(DESTDIR)$(SYSCONFDIR)
INCLUDES = -I$(XROOT)/include $(EXTRA_INC)
LDPATH = -L$(XROOT)/lib
LIBS = -lX11 $(EXTRA_LIBS)

PROG = windowlab
MANPAGE = windowlab.1x
OBJS = main.o events.o client.o new.o manage.o misc.o taskbar.o menufile.o
HEADERS = windowlab.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	mkdir -p $(BINDIR) && install -m 755 -s $(PROG) $(BINDIR)
	mkdir -p $(MANDIR) && install -m 644 $(MANPAGE) $(MANDIR) && gzip -9vfn $(MANDIR)/$(MANPAGE)
	mkdir -p $(CFGDIR) && cp -i windowlab.menurc $(CFGDIR)/windowlab.menurc && chmod 644 $(CFGDIR)/windowlab.menurc

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all install clean

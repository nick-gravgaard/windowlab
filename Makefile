# WindowLab - an X11 window manager
# by Nick Gravgaard (me at nickgravgaard.com)
# the following lines are from the original creator of this makefile and the
# window manager aewm by Decklin Foster
###
# aewm - a minimalistic X11 window manager.
# Copyright (c) 1998-2001 Decklin Foster <decklin@red-bean.com>
# Free software! Please see README for details and license.
###

# Comment out to remove shape support (for X11R5 or just a tiny bin)
DEFINES += -DSHAPE
EXTRA_LIBS += -lXext

# Uncomment to add MWM hints supports (needs Lesstif headers)
#DEFINES += -DMWM_HINTS

# Uncomment to add freetype support (requires XFree86 4.0.2 or later)
# This needs -lXext above, even if you have disabled shape support.
#DEFINES += -DXFT
#EXTRA_INC += `pkg-config --cflags xft`
#EXTRA_LIBS += `pkg-config --libs xft`

# Uncomment for debugging info (abandon all hope, ye who enter here)
#DEFINES += -DDEBUG

# This should be set to the location of the X installation you want to
# compile against.
XROOT = /usr/X11R6

# --------------------------------------------------------------------

CC       = gcc
CFLAGS   = -g -O2 -Wall

BINDIR   = $(DESTDIR)$(XROOT)/bin
MANDIR   = $(DESTDIR)$(XROOT)/man/man1
CFGDIR   = $(DESTDIR)/etc/X11/windowlab
INCLUDES = -I$(XROOT)/include $(EXTRA_INC)
LDPATH   = -L$(XROOT)/lib
LIBS     = -lX11 $(EXTRA_LIBS)

PROG     = windowlab
MANPAGE  = windowlab.1x
OBJS     = main.o events.o client.o new.o manage.o misc.o taskbar.o menufile.o
HEADERS  = windowlab.h

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	install -s $(PROG) $(BINDIR)
	install -m 644 $(MANPAGE) $(MANDIR)
	gzip -9vf $(MANDIR)/$(MANPAGE)
	mkdir -p $(CFGDIR) && cp menurc.sample $(CFGDIR)/menurc

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all install clean




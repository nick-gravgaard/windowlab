/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2003 Nick Gravgaard
 * me at nickgravgaard.com
 * http://nickgravgaard.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
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

#ifndef WINDOWLAB_H
#define WINDOWLAB_H

#define VERSION "1.3"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef MWM_HINTS
#include <Xm/MwmUtil.h>
#endif
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

/* Here are the default settings. Change to suit your taste.  If you
 * aren't sure about DEF_FONT, change it to "fixed"; almost all X
 * installations will have that available. */

#ifdef XFT
#define DEF_FONT	"-*-arial-medium-r-*-*-*-80-*-*-*-*-*-*"
#else
#define DEF_FONT	"lucidasans-10"
#endif
#define DEF_FG		"black"
#define DEF_BG		"gold" // focused
#define DEF_FC		"darkgray"
#define DEF_BD		"black"
#define DEF_NEW1	""
#define DEF_NEW2	""
#define DEF_NEW3	"xterm"
#define DEF_BW		2
#define SPACE		3
#define MINSIZE		15
#define MINWINWIDTH	80
#define MINWINHEIGHT	80

/* A few useful masks made up out of X's basic ones. `ChildMask' is a
 * silly name, but oh well. */

#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)
#define ButtonMask (ButtonPressMask|ButtonReleaseMask)
#define MouseMask (ButtonMask|PointerMotionMask)

/* Shorthand for wordy function calls */

#define setmouse(w, x, y) XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y)
#define ungrab() XUngrabPointer(dpy, CurrentTime)
#define grab(w, mask, curs) (XGrabPointer(dpy, w, False, mask, \
	GrabModeAsync, GrabModeAsync, None, curs, CurrentTime) == GrabSuccess)

/* I wanna know who the morons who prototyped these functions as
 * implicit int are...  */

#define lower_win(c) ((void) XLowerWindow(dpy, (c)->frame))
#define raise_win(c) ((void) XRaiseWindow(dpy, (c)->frame))

/* Border width accessor to handle hints/no hints */

#ifdef MWM_HINTS
#define BW(c) ((c)->has_border ? opt_bw : 0)
#else
#define BW(c) (opt_bw)
#endif

// Bar height
#ifdef XFT
#define BARHEIGHT() (xftfont->ascent + xftfont->descent + 2*SPACE + 1)
#else
#define BARHEIGHT() (font->ascent + font->descent + 2*SPACE + 1)
#endif

/* Multipliers for calling gravitate */

#define APPLY_GRAVITY 1
#define REMOVE_GRAVITY -1

/* Modes to call get_incsize with */

#define PIXELS 0
#define INCREMENTS 1

/* Modes for find_client */

#define WINDOW 0
#define FRAME 1

/* And finally modes for remove_client. */

#define WITHDRAW 0
#define REMAP 1

/* This structure keeps track of top-level windows (hereinafter
 * 'clients'). The clients we know about (i.e. all that don't set
 * override-redirect) are kept track of in linked list starting at the
 * global pointer called, appropriately, 'clients'. 
 *
 * window and parent refer to the actual client window and the larger
 * frame into which we will reparent it respectively. trans is set to
 * None for regular windows, and the window's 'owner' for a transient
 * window. Currently, we don't actually do anything with the owner for
 * transients; it's just used as a boolean.
 *
 * ignore_unmap is for our own purposes and doesn't reflect anything
 * from X. Whenever we unmap a window intentionally, we increment
 * ignore_unmap. This way our unmap event handler can tell when it
 * isn't supposed to do anything. */

typedef struct _Client Client;

struct _Client
{
	Client *next;
	char *name;
	XSizeHints *size;
	Window window, frame, trans;
	Colormap cmap;
	int x, y, width, height;
	int ignore_unmap;
#ifdef SHAPE
	Bool has_been_shaped;
#endif
#ifdef MWM_HINTS
	Bool has_title, has_border;
#endif
#ifdef XFT
	XftDraw *xftdraw;
#endif
};

typedef struct _Rect Rect;

struct _Rect
{
	int x, y, width, height;
};

/* Below here are (mainly generated with cproto) declarations and
 * prototypes for each file. */

//main.c
extern Display *dpy;
extern Window root;
extern int screen;
extern Client *head_client, *last_focused_client, *topmost_client, *last_button_in_client;
extern int last_button;
extern XFontStruct *font;
#ifdef XFT
extern XftFont *xftfont;
extern XftColor xft_fg;
#endif
extern GC string_gc, border_gc, active_gc, inactive_gc;
extern XColor fg, bg, fc, bd;
extern Cursor move_curs, resizestart_curs, resizeend_curs;
extern Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins;
#ifdef MWM_HINTS
extern Atom mwm_hints;
#endif
extern char *opt_font, *opt_fg, *opt_bg, *opt_fc, *opt_bd, *opt_new1, *opt_new2, *opt_new3;
extern int opt_bw;
#ifdef SHAPE
extern int shape, shape_event;
#endif

//events.c
extern void do_event_loop(void);

//client.c
extern Client *find_client(Window, int);
extern void set_wm_state(Client *, int);
extern long get_wm_state(Client *);
extern void send_config(Client *);
extern void remove_client(Client *, int);
extern void redraw(Client *);
extern void gravitate(Client *, int);
#ifdef SHAPE
extern void set_shape(Client *);
#endif
extern void check_focus(Client *);

//new.c
extern void make_new_client(Window);

//manage.c
extern void move(Client *);
extern void raise_lower(Client *);
extern void resize(Client *);
extern void hide(Client *);
extern void send_wm_delete(Client *);

//misc.c
void err(const char *, ...);
void fork_exec(char *);
void sig_handler(int);
int handle_xerror(Display *, XErrorEvent *);
int ignore_xerror(Display *, XErrorEvent *);
int send_xmessage(Window, Atom, long);
void get_mouse_position(int *, int *);
void fix_position(Client *);
#ifdef DEBUG
extern void show_event(XEvent);
extern void dump(Client *);
extern void dump_clients(void);
#endif

// taskbar.c
extern Window taskbar;
#ifdef XFT
extern XftDraw *tbxftdraw;
#endif
extern void make_taskbar(void);
extern void click_taskbar(unsigned int);
extern void redraw_taskbar(void);
float get_button_width(void);

#endif /* WINDOWLAB_H */


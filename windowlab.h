/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2003 Nick Gravgaard
 * me at nickgravgaard.com
 * http://nickgravgaard.com/windowlab/
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

#define VERSION "1.13"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef MWM_HINTS
#include <Xm/MwmUtil.h>
#endif
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

/* Here are the default settings. Change to suit your taste. If you
 * aren't sure about DEF_FONT, change it to "fixed"; almost all X
 * installations will have that available. */

#ifdef XFT
#define DEF_FONT "-*-arial-medium-r-*-*-*-80-*-*-*-*-*-*"
#else
#define DEF_FONT "lucidasans-10"
#endif
// use named colours, #rgb, #rrggbb or #rrrgggbbb format
#define DEF_DETAIL "#000"
#define DEF_ACTIVE "#fd0"
#define DEF_INACTIVE "#aaa"
#define DEF_MENU "#ddd"
#define DEF_SELECTED "#aad"
#define DEF_BORDERWIDTH 2
#define ACTIVE_SHADOW 0x2000 // eg #fff becomes #ddd
#define SPACE 3
#define MINSIZE 15
#define MINWINWIDTH 80
#define MINWINHEIGHT 80
// keys may be used by other apps, so change them here
#define KEY_CYCLEPREV XK_F9
#define KEY_CYCLENEXT XK_F10
#define KEY_FULLSCREEN XK_F11
#define KEY_TOGGLEZ XK_F12

/* A few useful masks made up out of X's basic ones. `ChildMask' is a
 * silly name, but oh well. */

#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)
#define ButtonMask (ButtonPressMask|ButtonReleaseMask)
#define MouseMask (ButtonMask|PointerMotionMask)

// Shorthand for wordy function calls

#define setmouse(w, x, y) XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y)
#define ungrab() XUngrabPointer(dpy, CurrentTime)
#define grab(w, mask, curs) \
	(XGrabPointer(dpy, w, False, mask, GrabModeAsync, GrabModeAsync, None, curs, CurrentTime) == GrabSuccess)
#define grab_keysym(w, mask, keysym) \
	XGrabKey(dpy, XKeysymToKeycode(dpy, keysym), mask, w, True, GrabModeAsync, GrabModeAsync); \
	XGrabKey(dpy, XKeysymToKeycode(dpy, keysym), LockMask|mask, w, True, GrabModeAsync, GrabModeAsync); \
	if (numlockmask) \
	{ \
		XGrabKey(dpy, XKeysymToKeycode(dpy, keysym), numlockmask|mask, w, True, GrabModeAsync, GrabModeAsync); \
		XGrabKey(dpy, XKeysymToKeycode(dpy, keysym), numlockmask|LockMask|mask, w, True, GrabModeAsync, GrabModeAsync); \
	}

/* I wanna know who the morons who prototyped these functions as
 * implicit int are...  */

#define lower_win(c) ((void) XLowerWindow(dpy, (c)->frame))
#define raise_win(c) ((void) XRaiseWindow(dpy, (c)->frame))

// Border width accessor to handle hints/no hints

#ifdef MWM_HINTS
#define BORDERWIDTH(c) ((c)->has_border ? opt_borderwidth : 0)
#else
#define BORDERWIDTH(c) (opt_borderwidth)
#endif

// Bar height
#ifdef XFT
#define BARHEIGHT() (xftfont->ascent + xftfont->descent + 2*SPACE + 1)
#else
#define BARHEIGHT() (font->ascent + font->descent + 2*SPACE + 1)
#endif

// Multipliers for calling gravitate

#define APPLY_GRAVITY 1
#define REMOVE_GRAVITY -1

// Modes to call get_incsize with

#define PIXELS 0
#define INCREMENTS 1

// Modes for find_client

#define WINDOW 0
#define FRAME 1

// Modes for remove_client

#define WITHDRAW 0
#define REMAP 1

// Stuff for the menu file

#define MAX_MENUITEMS 64
#define STR_SIZE 64
#define NO_MENU_LABEL "xterm"
#define NO_MENU_COMMAND "xterm"

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
	int iconic;
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
	unsigned int x, y, width, height;
};

typedef struct _MenuItem MenuItem;

struct _MenuItem
{
	char *command, *label;
	unsigned int x, width;
};

/* Below here are (mainly generated with cproto) declarations and
 * prototypes for each file. */

// main.c
extern Display *dpy;
extern Window root;
extern int screen;
extern Client *head_client, *last_focused_client, *fullscreen_client;
extern Rect fs_prevdims;
extern XFontStruct *font;
#ifdef XFT
extern XftFont *xftfont;
extern XftColor xft_detail;
#endif
extern GC string_gc, border_gc, active_gc, depressed_gc, inactive_gc, menu_gc, selected_gc;
extern XColor detail_col, active_col, depressed_col, inactive_col, menu_col, selected_col;
extern Cursor move_curs, resizestart_curs, resizeend_curs;
extern Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins;
#ifdef MWM_HINTS
extern Atom mwm_hints;
#endif
extern char *opt_font, *opt_detail, *opt_active, *opt_inactive, *opt_menu, *opt_selected;
extern int opt_borderwidth;
#ifdef SHAPE
extern int shape, shape_event;
#endif
extern unsigned int numlockmask;

// events.c
extern void do_event_loop(void);

// client.c
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

// new.c
extern void make_new_client(Window);

// manage.c
extern void move(Client *);
extern void raise_lower(Client *);
extern void resize(Client *);
extern void hide(Client *);
extern void unhide(Client *);
void toggle_fullscreen(Client *);
extern void send_wm_delete(Client *);
extern void write_titletext(Client *, Window);

// misc.c
void err(const char *, ...);
void fork_exec(char *);
void sig_handler(int);
int handle_xerror(Display *, XErrorEvent *);
int ignore_xerror(Display *, XErrorEvent *);
int send_xmessage(Window, Atom, long);
void get_mouse_position(int *, int *);
void fix_position(Client *);
void refix_position(Client *, XConfigureRequestEvent *);
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
extern void cycle_previous(void);
extern void cycle_next(void);
extern void rclick_taskbar(void);
extern void rclick_root(void);
extern void redraw_taskbar(void);
float get_button_width(void);

// menufile.c
extern MenuItem menuitems[MAX_MENUITEMS];
extern unsigned int num_menuitems;
extern void get_menuitems(void);
extern void free_menuitems(void);
#endif /* WINDOWLAB_H */


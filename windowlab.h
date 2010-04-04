/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2010 Nick Gravgaard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WINDOWLAB_H
#define WINDOWLAB_H

#define VERSION "1.40"
#define RELEASEDATE "2010-04-04"

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef MWM_HINTS
// These definitions are taken from LessTif 0.95.0's MwmUtil.h.

#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE (1L << 2)
#define MWM_HINTS_STATUS (1L << 3)

#define MWM_DECOR_ALL (1L << 0)
#define MWM_DECOR_BORDER (1L << 1)
#define MWM_DECOR_RESIZEH (1L << 2)
#define MWM_DECOR_TITLE (1L << 3)
#define MWM_DECOR_MENU (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define _XA_MWM_HINTS "_MOTIF_WM_HINTS"

#define PROP_MWM_HINTS_ELEMENTS	5

typedef struct PropMwmHints
{
	CARD32 flags;
	CARD32 functions;
	CARD32 decorations;
	INT32 inputMode;
	CARD32 status;
} PropMwmHints;
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// here are the default settings - change to suit your taste

// if you aren't sure about DEF_FONT, change it to "fixed"; almost all X installations will have that available
#ifdef XFT
#define DEF_FONT "-bitstream-bitstream vera sans-medium-r-*-*-*-100-*-*-*-*-*-*"
#else
#define DEF_FONT "-b&h-lucida-medium-r-*-*-10-*-*-*-*-*-*-*"
#endif

// use named colours, #rgb, #rrggbb or #rrrgggbbb format
#define DEF_BORDER "#000"
#define DEF_TEXT "#000"
#define DEF_ACTIVE "#fd0"
#define DEF_INACTIVE "#aaa"
#define DEF_MENU "#ddd"
#define DEF_SELECTED "#aad"
#define DEF_EMPTY "#000"
#define DEF_BORDERWIDTH 2
#define ACTIVE_SHADOW 0x2000 // eg #fff becomes #ddd
#define SPACE 3

// change MODIFIER to None to remove the need to hold down a modifier key
// the Windows key should be Mod4Mask and the Alt key is Mod1Mask
#define MODIFIER Mod1Mask

// keys may be used by other apps, so change them here
#define KEY_CYCLEPREV XK_Tab
#define KEY_CYCLENEXT XK_q
#define KEY_FULLSCREEN XK_F11
#define KEY_TOGGLEZ XK_F12

// max time between clicks in double click
#define DEF_DBLCLKTIME 400

// a few useful masks made up out of X's basic ones. `ChildMask' is a silly name, but oh well.
#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)
#define ButtonMask (ButtonPressMask|ButtonReleaseMask)
#define MouseMask (ButtonMask|PointerMotionMask)
#define KeyMask (KeyPressMask|KeyReleaseMask)

#define ABS(x) (((x) < 0) ? -(x) : (x))

// shorthand for wordy function calls
#define setmouse(w, x, y) XWarpPointer(dsply, None, w, 0, 0, 0, 0, x, y)
#define ungrab() XUngrabPointer(dsply, CurrentTime)
#define grab(w, mask, curs) \
	(XGrabPointer(dsply, w, False, mask, GrabModeAsync, GrabModeAsync, None, curs, CurrentTime) == GrabSuccess)
#define grab_keysym(w, mask, keysym) \
	XGrabKey(dsply, XKeysymToKeycode(dsply, keysym), mask, w, True, GrabModeAsync, GrabModeAsync); \
	XGrabKey(dsply, XKeysymToKeycode(dsply, keysym), LockMask|mask, w, True, GrabModeAsync, GrabModeAsync); \
	if (numlockmask) \
	{ \
		XGrabKey(dsply, XKeysymToKeycode(dsply, keysym), numlockmask|mask, w, True, GrabModeAsync, GrabModeAsync); \
		XGrabKey(dsply, XKeysymToKeycode(dsply, keysym), numlockmask|LockMask|mask, w, True, GrabModeAsync, GrabModeAsync); \
	}

// I wanna know who the morons who prototyped these functions as implicit int are...
#define lower_win(c) ((void) XLowerWindow(dsply, (c)->frame))
#define raise_win(c) ((void) XRaiseWindow(dsply, (c)->frame))

// border width accessor to handle hints/no hints
#ifdef MWM_HINTS
#define BORDERWIDTH(c) ((c)->has_border ? DEF_BORDERWIDTH : 0)
#else
#define BORDERWIDTH(c) (DEF_BORDERWIDTH)
#endif

// bar height
#ifdef XFT
#define BARHEIGHT() (xftfont->ascent + xftfont->descent + 2*SPACE + 2)
#else
#define BARHEIGHT() (font->ascent + font->descent + 2*SPACE + 2)
#endif

// minimum window width and height, enough for 3 buttons and a bit of titlebar
#define MINWINWIDTH (BARHEIGHT() * 4)
#define MINWINHEIGHT (BARHEIGHT() * 4)

// multipliers for calling gravitate
#define APPLY_GRAVITY 1
#define REMOVE_GRAVITY -1

// modes to call get_incsize with
#define PIXELS 0
#define INCREMENTS 1

// modes for find_client
#define WINDOW 0
#define FRAME 1

// modes for remove_client
#define WITHDRAW 0
#define REMAP 1

// stuff for the menu file
#define MAX_MENUITEMS 24
#define MAX_MENUITEMS_SIZE (sizeof(MenuItem) * MAX_MENUITEMS)
#define STR_SIZE 128
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

typedef struct Client
{
	struct Client *next;
	char *name;
	XSizeHints *size;
	Window window, frame, trans;
	Colormap cmap;
	int x, y;
	int width, height;
	int ignore_unmap;
	unsigned int hidden;
	unsigned int was_hidden;
	unsigned int focus_order;
#ifdef SHAPE
	Bool has_been_shaped;
#endif
#ifdef MWM_HINTS
	Bool has_title, has_border;
#endif
#ifdef XFT
	XftDraw *xftdraw;
#endif
} Client;

typedef struct Rect
{
	int x, y;
	int width, height;
} Rect;

typedef struct MenuItem
{
	char *command, *label;
	int x;
	int width;
} MenuItem;

// Below here are (mainly generated with cproto) declarations and prototypes for each file.

// main.c
extern Display *dsply;
extern Window root;
extern int screen;
extern Client *head_client, *focused_client, *topmost_client, *fullscreen_client;
extern unsigned int in_taskbar, showing_taskbar, focus_count;
extern Rect fs_prevdims;
extern XFontStruct *font;
#ifdef XFT
extern XftFont *xftfont;
extern XftColor xft_detail;
#endif
extern GC border_gc, text_gc, active_gc, depressed_gc, inactive_gc, menu_gc, selected_gc, empty_gc;
extern XColor border_col, text_col, active_col, depressed_col, inactive_col, menu_col, selected_col, empty_col;
extern Cursor resize_curs;
extern Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins;
#ifdef MWM_HINTS
extern Atom mwm_hints;
#endif
extern char *opt_font, *opt_border, *opt_text, *opt_active, *opt_inactive, *opt_menu, *opt_selected, *opt_empty;
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
extern Client *get_prev_focused(void);
extern void draw_hide_button(Client *, GC *, GC *);
extern void draw_toggledepth_button(Client *, GC *, GC *);
extern void draw_close_button(Client *, GC *, GC *);

// new.c
extern void make_new_client(Window);

// manage.c
extern void move(Client *);
extern void raise_lower(Client *);
extern void resize(Client *, int, int);
extern void hide(Client *);
extern void unhide(Client *);
extern void toggle_fullscreen(Client *);
extern void send_wm_delete(Client *);
extern void write_titletext(Client *, Window);

// misc.c
extern void err(const char *, ...);
extern void fork_exec(char *);
extern void sig_handler(int);
extern int handle_xerror(Display *, XErrorEvent *);
extern int ignore_xerror(Display *, XErrorEvent *);
extern int send_xmessage(Window, Atom, long);
extern void get_mouse_position(int *, int *);
extern void fix_position(Client *);
extern void refix_position(Client *, XConfigureRequestEvent *);
extern void copy_dims(Rect *, Rect *);
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
extern void cycle_previous(void);
extern void cycle_next(void);
extern void lclick_taskbar(int);
extern void rclick_taskbar(int);
extern void rclick_root(void);
extern void redraw_taskbar(void);
extern float get_button_width(void);

// menufile.c
extern int do_menuitems;
extern MenuItem* menuitems;
extern unsigned int num_menuitems;
extern void get_menuitems(void);
extern void free_menuitems(void);
#endif /* WINDOWLAB_H */

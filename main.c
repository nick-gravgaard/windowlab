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

#include "windowlab.h"
#include <string.h>
#include <signal.h>
#include <X11/cursorfont.h>

Display *dpy;
Window root;
int screen;
XFontStruct *font;
#ifdef XFT
XftFont *xftfont;
XftColor xft_fg;
#endif
GC string_gc, border_gc, active_gc, depressed_gc, inactive_gc, menubar_gc, menusel_gc;
XColor fg, bg, db, fc, bd, mb, sm;
Cursor move_curs, resizestart_curs, resizeend_curs;
Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins;
#ifdef MWM_HINTS
Atom mwm_hints;
#endif
Client *head_client, *last_focused_client, *topmost_client;
char *opt_font = DEF_FONT;
char *opt_fg = DEF_FG;
char *opt_bg = DEF_BG;
char *opt_db = DEF_DB;
char *opt_fc = DEF_FC;
char *opt_bd = DEF_BD;
char *opt_mb = DEF_MB;
char *opt_sm = DEF_SM;
int opt_bw = DEF_BW;
#ifdef SHAPE
Bool shape;
int shape_event;
#endif

static void scan_wins(void);
static void setup_display(void);

int main(int argc, char **argv)
{
	int i;
	struct sigaction act;

#define OPT_STR(name, variable)				\
	if (strcmp(argv[i], name) == 0 && i+1<argc)	\
	{						\
		variable = argv[++i];			\
		continue;				\
	}
#define OPT_INT(name, variable)				\
	if (strcmp(argv[i], name) == 0 && i+1<argc)	\
	{						\
		variable = atoi(argv[++i]);		\
		continue;				\
	}

	for (i = 1; i < argc; i++)
	{
		OPT_STR("-fn", opt_font)
		OPT_STR("-fg", opt_fg)
		OPT_STR("-bg", opt_bg)
		OPT_STR("-db", opt_db)
		OPT_STR("-fc", opt_fc)
		OPT_STR("-bd", opt_bd)
		OPT_STR("-mb", opt_mb)
		OPT_STR("-sm", opt_sm)
		OPT_INT("-bw", opt_bw)
		if (strcmp(argv[i], "-version") == 0)
		{
			printf("windowlab: version " VERSION "\n");
			exit(0);
		}
		// shouldn't get here; must be a bad option
		err("usage: windowlab [options]\n"
			"       options are: -fn <font>, -fg|-bg|-db|-fc|-bd|-mb|-sm <color>, -bw <width>");
		return 2;
	}

	act.sa_handler = sig_handler;
	act.sa_flags = 0;
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);

	setup_display();
	get_menuitems();
	head_client = 0;
	make_taskbar();
	scan_wins();
	do_event_loop();
	return 1; //just another brick in the -Wall
}

static void scan_wins(void)
{
	unsigned int nwins, i;
	Window dummyw1, dummyw2, *wins;
	XWindowAttributes attr;

	XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
	for (i = 0; i < nwins; i++)
	{
		XGetWindowAttributes(dpy, wins[i], &attr);
		if (!attr.override_redirect && attr.map_state == IsViewable)
		{
			make_new_client(wins[i]);
		}
	}
	XFree(wins);
}

static void setup_display(void)
{
	XColor dummyc;
	XGCValues gv;
	XSetWindowAttributes sattr;
#ifdef SHAPE
	int dummy;
#endif

	dpy = XOpenDisplay(NULL);

	if (!dpy)
	{
		err("can't open display! check your DISPLAY variable.");
		exit(1);
	}

	XSetErrorHandler(handle_xerror);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	wm_state = XInternAtom(dpy, "WM_STATE", False);
	wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wm_cmapwins = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
#ifdef MWM_HINTS
	mwm_hints = XInternAtom(dpy, _XA_MWM_HINTS, False);
#endif

	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fg, &fg, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bg, &bg, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_db, &db, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_fc, &fc, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_bd, &bd, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_mb, &mb, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), opt_sm, &sm, &dummyc);

	font = XLoadQueryFont(dpy, opt_font);
	if (!font)
	{
		err("font '%s' not found", opt_font); exit(1);
	}

#ifdef XFT
	xft_fg.color.red = fg.red;
	xft_fg.color.green = fg.green;
	xft_fg.color.blue = fg.blue;
	xft_fg.color.alpha = 0xffff;
	xft_fg.pixel = fg.pixel;

	xftfont = XftFontOpenXlfd(dpy, DefaultScreen(dpy), opt_font);
	if (!xftfont)
	{
		err("font '%s' not found", opt_font); exit(1);
	}
#endif

#ifdef SHAPE
	shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
#endif

	move_curs = XCreateFontCursor(dpy, XC_fleur);
	resizestart_curs = XCreateFontCursor(dpy, XC_ul_angle);
	resizeend_curs = XCreateFontCursor(dpy, XC_lr_angle); //XC_bottom_right_corner

	gv.function = GXcopy;
	gv.foreground = fg.pixel;
	gv.font = font->fid;
	string_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCFont, &gv);

	gv.foreground = bd.pixel;
	gv.line_width = opt_bw;
	border_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCLineWidth, &gv);

	gv.foreground = bg.pixel;
	active_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gv);

	gv.foreground = db.pixel;
	depressed_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gv);

	gv.foreground = fc.pixel;
	inactive_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gv);

	gv.foreground = mb.pixel;
	menubar_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gv);

	gv.foreground = sm.pixel;
	menusel_gc = XCreateGC(dpy, root, GCFunction|GCForeground, &gv);

	sattr.event_mask = ChildMask|ColormapChangeMask|ButtonMask;
	XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);
}




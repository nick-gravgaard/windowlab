/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2002 Nick Gravgaard
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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void quit_nicely(void);

void err(const char *fmt, ...)
{
	va_list argp;

	fprintf(stderr, "windowlab: ");
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	fprintf(stderr, "\n");
}

void fork_exec(char *cmd)
{
	pid_t pid = fork();

	switch (pid)
	{
		case 0:
			execlp("/bin/sh", "sh", "-c", cmd, NULL);
			err("exec failed, cleaning up child");
			exit(1);
		case -1:
			err("can't fork");
	}
}

void sig_handler(int signal)
{
	switch (signal)
	{
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			quit_nicely();
			break;
		case SIGCHLD:
			wait(NULL);
			break;
	}
}

int handle_xerror(Display *dpy, XErrorEvent *e)
{
	Client *c = find_client(e->resourceid, WINDOW);

	if (e->error_code == BadAccess && e->resourceid == root)
	{
		err("root window unavailible (maybe another wm is running?)");
		exit(1);
	}
	else
	{
		char msg[255];
		XGetErrorText(dpy, e->error_code, msg, sizeof msg);
		err("X error (%#lx): %s", e->resourceid, msg);
	}

	if (c)
	{
		remove_client(c, WITHDRAW);
	}
	return 0;
}

/* Ick. Argh. You didn't see this function. */

int ignore_xerror(Display *dpy, XErrorEvent *e)
{
	return 0;
}

/* Currently, only send_wm_delete uses this one... */

int send_xmessage(Window w, Atom a, long x)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = w;
	e.xclient.message_type = a;
	e.xclient.format = 32;
	e.xclient.data.l[0] = x;
	e.xclient.data.l[1] = CurrentTime;

	return XSendEvent(dpy, w, False, NoEventMask, &e);
}

void get_mouse_position(int *x, int *y)
{
	Window mouse_root, mouse_win;
	int win_x, win_y;
	unsigned int mask;

	XQueryPointer(dpy, root, &mouse_root, &mouse_win,
		x, y, &win_x, &win_y, &mask);
}

#ifdef DEBUG

/* Bleh, stupid macro names. I'm not feeling creative today. */

#define SHOW_EV(name, memb) \
	case name: s = #name; w = e.memb.window; break;
#define SHOW(name) \
	case name: return #name;

void show_event(XEvent e)
{
	char *s, buf[20];
	Window w;
	Client *c;

	switch (e.type)
	{
		SHOW_EV(ButtonPress, xbutton)
		SHOW_EV(ButtonRelease, xbutton)
		SHOW_EV(ClientMessage, xclient)
		SHOW_EV(ColormapNotify, xcolormap)
		SHOW_EV(ConfigureNotify, xconfigure)
		SHOW_EV(ConfigureRequest, xconfigurerequest)
		SHOW_EV(CreateNotify, xcreatewindow)
		SHOW_EV(DestroyNotify, xdestroywindow)
		SHOW_EV(EnterNotify, xcrossing)
		SHOW_EV(Expose, xexpose)
		SHOW_EV(MapNotify, xmap)
		SHOW_EV(MapRequest, xmaprequest)
		SHOW_EV(MappingNotify, xmapping)
		SHOW_EV(MotionNotify, xmotion)
		SHOW_EV(PropertyNotify, xproperty)
		SHOW_EV(ReparentNotify, xreparent)
		SHOW_EV(ResizeRequest, xresizerequest)
		SHOW_EV(UnmapNotify, xunmap)
		default:
			if (shape && e.type == shape_event)
			{
				s = "ShapeNotify"; w = ((XShapeEvent *)&e)->window;
			}
			else
			{
				s = "unknown event"; w = None;
			}
			break;
	}

	c = find_client(w, WINDOW);
	snprintf(buf, sizeof buf, c ? c->name : "(none)");
	err("%#-10lx: %-20s: %s", w, buf, s);
}

static const char *show_state(Client *c)
{
	switch (get_wm_state(c))
	{
		SHOW(WithdrawnState)
		SHOW(NormalState)
		SHOW(IconicState)
		default: return "unknown state";
	}
}

static const char *show_grav(Client *c)
{
	if (!c->size || !(c->size->flags & PWinGravity))
	{
		return "no grav (NW)";
	}

	switch (c->size->win_gravity)
	{
		SHOW(UnmapGravity)
		SHOW(NorthWestGravity)
		SHOW(NorthGravity)
		SHOW(NorthEastGravity)
		SHOW(WestGravity)
		SHOW(CenterGravity)
		SHOW(EastGravity)
		SHOW(SouthWestGravity)
		SHOW(SouthGravity)
		SHOW(SouthEastGravity)
		SHOW(StaticGravity)
		default: return "unknown grav";
	}
}

void dump(Client *c)
{
	err("%s\n\t%s, %s, ignore %d\n"
		"\tframe %#lx, win %#lx, geom %dx%d+%d+%d",
		c->name, show_state(c), show_grav(c), c->ignore_unmap,
		c->frame, c->window, c->width, c->height, c->x, c->y);
}

void dump_clients()
{
	Client *c;
	for (c = head_client; c; c = c->next)
	{
		dump(c);
	}
}
#endif

/* We use XQueryTree here to preserve the window stacking order,
 * since the order in our linked list is different. */

static void quit_nicely(void)
{
	unsigned int nwins, i;
	Window dummyw1, dummyw2, *wins;
	Client *c;

	XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
	for (i = 0; i < nwins; i++)
	{
		c = find_client(wins[i], FRAME);
		if (c)
		{
			remove_client(c, REMAP);
		}
	}
	XFree(wins);

	XFreeFont(dpy, font);
#ifdef XFT
	XftFontClose(dpy, xftfont);
#endif
	XFreeCursor(dpy, move_curs);
	XFreeCursor(dpy, resizestart_curs);
	XFreeCursor(dpy, resizeend_curs);
	XFreeGC(dpy, invert_gc);
	XFreeGC(dpy, border_gc);
	XFreeGC(dpy, string_gc);

	XInstallColormap(dpy, DefaultColormap(dpy, screen));
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);

	XCloseDisplay(dpy);
	exit(0);
}

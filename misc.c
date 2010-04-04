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

#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "windowlab.h"

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
	char *envshell, *envshellname;
	pid_t pid = fork();

	switch (pid)
	{
  		case 0:
			setsid();
			envshell = getenv("SHELL");
			if (envshell == NULL)
			{
				envshell = "/bin/sh";
			}
			envshellname = strrchr(envshell, '/');
			if (envshellname == NULL)
			{
				envshellname = envshell;
			}
			else
			{
				/* move to the character after the slash */
				envshellname++;
			}
			execlp(envshell, envshellname, "-c", cmd, NULL);
			err("exec failed, cleaning up child");
			exit(1);
			break;
		case -1:
			err("can't fork");
			break;
	}
}

void sig_handler(int signal)
{
	pid_t pid;
	int status;

	switch (signal)
	{
		case SIGINT:
		case SIGTERM:
			quit_nicely();
			break;
		case SIGHUP:
			do_menuitems = 1;
			break;
		case SIGCHLD:
			while ((pid = waitpid(-1, &status, WNOHANG)) != 0)
			{
				if ((pid == -1) && (errno != EINTR))
				{
					break;
				}
				else
				{
					continue;
				}
			}
			break;
	}
}

int handle_xerror(Display *dsply, XErrorEvent *e)
{
	Client *c = find_client(e->resourceid, WINDOW);

	if (e->error_code == BadAccess && e->resourceid == root)
	{
		err("root window unavailable (maybe another wm is running?)");
		exit(1);
	}
	else
	{
		char msg[255];
		XGetErrorText(dsply, e->error_code, msg, sizeof msg);
		err("X error (%#lx): %s", e->resourceid, msg);
	}

	if (c != NULL)
	{
		remove_client(c, WITHDRAW);
	}
	return 0;
}

/* Ick. Argh. You didn't see this function. */

int ignore_xerror(Display *dsply, XErrorEvent *e)
{
	(void) dsply;
	(void) e;
	return 0;
}

/* Currently, only send_wm_delete uses this one... */

int send_xmessage(Window w, Atom a, long x)
{
	XClientMessageEvent e;

	e.type = ClientMessage;
	e.window = w;
	e.message_type = a;
	e.format = 32;
	e.data.l[0] = x;
	e.data.l[1] = CurrentTime;

	return XSendEvent(dsply, w, False, NoEventMask, (XEvent *)&e);
}

void get_mouse_position(int *x, int *y)
{
	Window mouse_root, mouse_win;
	int win_x, win_y;
	unsigned int mask;

	XQueryPointer(dsply, root, &mouse_root, &mouse_win, x, y, &win_x, &win_y, &mask);
}

/* If this is the fullscreen client we don't take BARHEIGHT() into account
 * because the titlebar isn't being drawn on the window. */

void fix_position(Client *c)
{
	int xmax = DisplayWidth(dsply, screen);
	int ymax = DisplayHeight(dsply, screen);
	int titlebarheight;

#ifdef DEBUG
	fprintf(stderr, "fix_position(): client was (%d, %d)-(%d, %d)\n", c->x, c->y, c->x + c->width, c->y + c->height);
#endif
	
	titlebarheight = (fullscreen_client == c) ? 0 : BARHEIGHT();

	if (c->width < MINWINWIDTH)
	{
		c->width = MINWINWIDTH;
	}
	if (c->height < MINWINHEIGHT)
	{
		c->height = MINWINHEIGHT;
	}
	
	if (c->width > xmax)
	{
		c->width = xmax;
	}
	if (c->height + (BARHEIGHT() + titlebarheight) > ymax)
	{
		c->height = ymax - (BARHEIGHT() + titlebarheight);
	}

	if (c->x < 0)
	{
		c->x = 0;
	}
	if (c->y < BARHEIGHT())
	{
		c->y = BARHEIGHT();
	}

	if (c->x + c->width + BORDERWIDTH(c) >= xmax)
	{
		c->x = xmax - c->width;
	}
	if (c->y + c->height + BARHEIGHT() >= ymax)
	{
		c->y = (ymax - c->height) - BARHEIGHT();
	}

#ifdef DEBUG
	fprintf(stderr, "fix_position(): client is (%d, %d)-(%d, %d)\n", c->x, c->y, c->x + c->width, c->y + c->height);
#endif

	c->x -= BORDERWIDTH(c);
	c->y -= BORDERWIDTH(c);
}

void refix_position(Client *c, XConfigureRequestEvent *e)
{
	Rect olddims;
	olddims.x = c->x - BORDERWIDTH(c);
	olddims.y = c->y - BORDERWIDTH(c);
	olddims.width = c->width;
	olddims.height = c->height;
	fix_position(c);
	if (olddims.x != c->x)
	{
		e->value_mask |= CWX;
	}
	if (olddims.y != c->y)
	{
		e->value_mask |= CWY;
	}
	if (olddims.width != c->width)
	{
		e->value_mask |= CWWidth;
	}
	if (olddims.height != c->height)
	{
		e->value_mask |= CWHeight;
	}
}

void copy_dims(Rect *sourcedims, Rect *destdims)
{
	destdims->x = sourcedims->x;
	destdims->y = sourcedims->y;
	destdims->width = sourcedims->width;
	destdims->height = sourcedims->height;
}

#ifdef DEBUG

/* Bleh, stupid macro names. I'm not feeling creative today. */

#define SHOW_EV(name, memb) \
	case name: \
		s = #name; \
		w = e.memb.window; \
		break;
#define SHOW(name) \
	case name: \
		return #name;

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
				s = "ShapeNotify";
				w = ((XShapeEvent *)&e)->window;
			}
			else
			{
				s = "unknown event";
				w = None;
			}
			break;
	}

	c = find_client(w, WINDOW);
	snprintf(buf, sizeof buf, c != NULL ? c->name : "(none)");
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
	if (c->size == NULL || !(c->size->flags & PWinGravity))
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
	if (c != NULL)
	{
		err("%s\n\t%s, %s, ignore %d, was_hidden %d\n\tframe %#lx, win %#lx, geom %dx%d+%d+%d", c->name, show_state(c), show_grav(c), c->ignore_unmap, c->was_hidden, c->frame, c->window, c->width, c->height, c->x, c->y);
	}
}

void dump_clients(void)
{
	Client *c = head_client;
	while (c != NULL)
	{
		dump(c);
		c = c->next;
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

	free_menuitems();

	XQueryTree(dsply, root, &dummyw1, &dummyw2, &wins, &nwins);
	for (i = 0; i < nwins; i++)
	{
		c = find_client(wins[i], FRAME);
		if (c != NULL)
		{
			remove_client(c, REMAP);
		}
	}
	XFree(wins);

	if (font)
	{
		XFreeFont(dsply, font);
	}
#ifdef XFT
	if (xftfont)
	{
		XftFontClose(dsply, xftfont);
	}
#endif
	XFreeCursor(dsply, resize_curs);
	XFreeGC(dsply, border_gc);
	XFreeGC(dsply, text_gc);

	XInstallColormap(dsply, DefaultColormap(dsply, screen));
	XSetInputFocus(dsply, PointerRoot, RevertToNone, CurrentTime);

	XCloseDisplay(dsply);
	exit(0);
}

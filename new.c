/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2004 Nick Gravgaard
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

#include "windowlab.h"

static void init_position(Client *);
static void reparent(Client *);
#ifdef MWM_HINTS
static PropMwmHints *get_mwm_hints(Window);
#endif

/* Set up a client structure for the new (not-yet-mapped) window. The
 * confusing bit is that we have to ignore 2 unmap events if the
 * client was already mapped but has IconicState set (for instance,
 * when we are the second window manager in a session).  That's
 * because there's one for the reparent (which happens on all viewable
 * windows) and then another for the unmapping itself. */

void make_new_client(Window w)
{
	Client *c, *p;
	XWindowAttributes attr;
	XWMHints *hints;
#ifdef MWM_HINTS
	PropMwmHints *mhints;
#endif
	long dummy;

	c = (Client *)malloc(sizeof *c);
	if (head_client == NULL)
	{
		head_client = c;
	}
	else
	{
		p = head_client;
		while (p->next != NULL)
		{
			p = p->next;
		}
		p->next = c;
	}
	c->next = NULL;

	if (last_focused_client == NULL)
	{
		last_focused_client = c; // check every time? This should only be done at the start
	}

	XGrabServer(dpy);

	XGetTransientForHint(dpy, w, &c->trans);
	XFetchName(dpy, w, &c->name);
	XGetWindowAttributes(dpy, w, &attr);

	c->window = w;
	c->ignore_unmap = 0;
	c->iconic = 0;
#ifdef SHAPE
	c->has_been_shaped = 0;
#endif
	c->x = attr.x;
	c->y = attr.y;
	c->width = attr.width;
	c->height = attr.height;
	c->cmap = attr.colormap;
	c->size = XAllocSizeHints();
	XGetWMNormalHints(dpy, c->window, c->size, &dummy);
#ifdef MWM_HINTS
	c->has_title = 1;
	c->has_border = 1;

	if ((mhints = get_mwm_hints(c->window)))
	{
		if (mhints->flags & MWM_HINTS_DECORATIONS
				&& !(mhints->decorations & MWM_DECOR_ALL))
		{
			c->has_title  = mhints->decorations & MWM_DECOR_TITLE;
			c->has_border = mhints->decorations & MWM_DECOR_BORDER;
		}
		XFree(mhints);
	}
#endif

	if (attr.map_state == IsViewable)
	{
		c->ignore_unmap++;
	}
	else
	{
		init_position(c);
		set_wm_state(c, NormalState);
		if ((hints = XGetWMHints(dpy, w)))
		{
			if (hints->flags & StateHint)
			{
				set_wm_state(c, hints->initial_state);
			}
			XFree(hints);
		}
	}

	fix_position(c);
	gravitate(c, APPLY_GRAVITY);
	reparent(c);

#ifdef XFT
	c->xftdraw = XftDrawCreate(dpy, (Drawable) c->frame,
		DefaultVisual(dpy, DefaultScreen(dpy)),
		DefaultColormap(dpy, DefaultScreen(dpy)));
#endif

	if (attr.map_state == IsViewable)
	{
		if (get_wm_state(c) == IconicState)
		{
			c->ignore_unmap++;
			c->iconic = 1;
			XUnmapWindow(dpy, c->window);
		}
		else
		{
			XMapWindow(dpy, c->window);
			XMapRaised(dpy, c->frame);
			set_wm_state(c, NormalState);
		}
	}
	else
	{
		if (get_wm_state(c) == NormalState)
		{
			XMapWindow(dpy, c->window);
			XMapRaised(dpy, c->frame);
		}
	}

	check_focus(c);
	topmost_client = c;

#ifdef DEBUG
	dump(c);
#endif

	XSync(dpy, False);
	XUngrabServer(dpy);

	redraw_taskbar();
}

/* This one does -not- free the data coming back from Xlib; it just
 * sends back the pointer to what was allocated. */

#ifdef MWM_HINTS
static PropMwmHints *get_mwm_hints(Window w)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	unsigned char *data;

	if (XGetWindowProperty(dpy, w, mwm_hints, 0L, 20L, False,
		mwm_hints, &real_type, &real_format, &items_read, &items_left,
		&data) == Success
		&& items_read >= PROP_MOTIF_WM_HINTS_ELEMENTS)
	{
		return (PropMwmHints *)data;
	}
	else
	{
		return NULL;
	}
}
#endif

/* Figure out where to map the window. c->x, c->y, c->width, and
 * c->height actually start out with values in them (whatever the
 * client passed to XCreateWindow).  Program-specified hints will
 * override these, but anything set by the program will be
 * sanity-checked before it is used. PSize is ignored completely,
 * because GTK sets it to 200x200 for almost everything. User-
 * specified hints will of course override anything the program says.
 *
 * If we can't find a reasonable position hint, we make up a position
 * using the relative mouse co-ordinates and window size. To account
 * for window gravity while doing this, we add BARHEIGHT() into the
 * calculation and then degravitate. Don't think about it too hard, or
 * your head will explode. */

static void init_position(Client *c)
{
	int mousex, mousey;

	if (c->size->flags & (USSize))
	{
		c->width = c->size->width;
		c->height = c->size->height;
	}
	else
	{
		// we would check PSize here, if GTK didn't blow goats
		// make sure it's big enough for the 3 buttons and a bit of bar
		if (c->width < 4 * BARHEIGHT())
		{
			c->width = 4 * BARHEIGHT();
		}
		if (c->height < BARHEIGHT())
		{
			c->height = BARHEIGHT();
		}
	}

	if (c->size->flags & USPosition)
	{
		c->x = c->size->x;
		c->y = c->size->y;
	}
	else
	{
		if (c->size->flags & PPosition)
		{
			c->x = c->size->x;
			c->y = c->size->y;
		}
	}

	if (c->x == 0 && c->y == 0)
	{
		get_mouse_position(&mousex, &mousey);
		c->x = mousex;
		c->y = mousey + BARHEIGHT();
		gravitate(c, REMOVE_GRAVITY);
	}
}

static void reparent(Client *c)
{
	XSetWindowAttributes pattr;

	pattr.override_redirect = True;
	pattr.background_pixel = empty_col.pixel;
	pattr.border_pixel = border_col.pixel;
	pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	c->frame = XCreateWindow(dpy, root,
		c->x, c->y - BARHEIGHT(), c->width, c->height + BARHEIGHT(), BORDERWIDTH(c),
		DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);

#ifdef SHAPE
	if (shape)
	{
		XShapeSelectInput(dpy, c->window, ShapeNotifyMask);
		set_shape(c);
	}
#endif

	XAddToSaveSet(dpy, c->window);
	XSelectInput(dpy, c->window, ColormapChangeMask|PropertyChangeMask);
	XSetWindowBorderWidth(dpy, c->window, 0);
	XResizeWindow(dpy, c->window, c->width, c->height);
	XReparentWindow(dpy, c->window, c->frame, 0, BARHEIGHT());

	send_config(c);
}

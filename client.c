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

#include <X11/Xmd.h>
#include "windowlab.h"

Client *find_client(Window w, int mode)
{
	Client *c = head_client;
	if (mode == FRAME)
	{
		while (c != NULL)
		{
			if (c->frame == w)
			{
				return c;
			}
			c = c->next;
		}
	}
	else //WINDOW
	{
		while (c != NULL)
		{
			if (c->window == w)
			{
				return c;
			}
			c = c->next;
		}
	}
	return NULL;
}

/* Attempt to follow the ICCCM by explicity specifying 32 bits for
 * this property. Does this goof up on 64 bit systems? */

void set_wm_state(Client *c, int state)
{
	CARD32 data[2];

	data[0] = state;
	data[1] = None; //Icon? We don't need no steenking icon.

	XChangeProperty(dpy, c->window, wm_state, wm_state,
		32, PropModeReplace, (unsigned char *)data, 2);

	if (state == IconicState)
	{
		if (c == last_focused_client)
		{
			last_focused_client = 0;
		}
	}
}

/* If we can't find a WM_STATE we're going to have to assume
 * Withdrawn. This is not exactly optimal, since we can't really
 * distinguish between the case where no WM has run yet and when the
 * state was explicitly removed (Clients are allowed to either set the
 * atom to Withdrawn or just remove it... yuck.) */

long get_wm_state(Client *c)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	long *data, state = WithdrawnState;

	if (XGetWindowProperty(dpy, c->window, wm_state, 0L, 2L, False,
		wm_state, &real_type, &real_format, &items_read, &items_left,
		(unsigned char **) &data) == Success && items_read)
	{
		state = *data;
		XFree(data);
	}
	return state;
}

/* This will need to be called whenever we update our Client stuff.
 * Yeah, yeah, stop yelling at me about OO. */

void send_config(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.event = c->window;
	ce.window = c->window;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->width;
	ce.height = c->height;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = 0;

	XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

/* After pulling my hair out trying to find some way to tell if a
 * window is still valid, I've decided to instead carefully ignore any
 * errors raised by this function. We know that the X calls are, and
 * we know the only reason why they could fail -- a window has removed
 * itself completely before the Unmap and Destroy events get through
 * the queue to us. It's not absolutely perfect, but it works.
 *
 * The 'withdrawing' argument specifes if the client is actually
 * (destroying itself||being destroyed by us) or if we are merely
 * cleaning up its data structures when we exit mid-session. */

void remove_client(Client *c, int mode)
{
	Client *p;

	XGrabServer(dpy);
	XSetErrorHandler(ignore_xerror);

#ifdef DEBUG
	err("removing %s, %d: %d left", c->name, mode, XPending(dpy));
#endif

	if (mode == WITHDRAW)
	{
		set_wm_state(c, WithdrawnState);
	}
	else //REMAP
	{
		XMapWindow(dpy, c->window);
	}
	gravitate(c, REMOVE_GRAVITY);
	XReparentWindow(dpy, c->window, root, c->x, c->y);
#ifdef MWM_HINTS
	if (c->has_border)
	{
		XSetWindowBorderWidth(dpy, c->window, 1);
	}
#else
	XSetWindowBorderWidth(dpy, c->window, 1);
#endif
#ifdef XFT
	XftDrawDestroy(c->xftdraw);
#endif
	XRemoveFromSaveSet(dpy, c->window);
	XDestroyWindow(dpy, c->frame);

	if (head_client == c)
	{
		head_client = c->next;
	}
	else
	{
		for (p = head_client; p && p->next; p = p->next)
		{
			if (p->next == c)
			{
				p->next = c->next;
			}
		}
	}
	if (c->name)
	{
		XFree(c->name);
	}
	if (c->size)
	{
		XFree(c->size);
	}
	if (c == last_focused_client)
	{
		last_focused_client = 0;
	}
	free(c);

	XSync(dpy, False);
	XSetErrorHandler(handle_xerror);
	XUngrabServer(dpy);

	redraw_taskbar();
}

void redraw(Client *c)
{
#ifdef MWM_HINTS
	if (!c->has_title)
	{
		return;
	}
#endif
	// clear text part of bar
	if (c == last_focused_client)
	{
		XFillRectangle(dpy, c->frame, active_gc, 0, 0, c->width - (BARHEIGHT() * 3), BARHEIGHT());
	}
	else
	{
		XFillRectangle(dpy, c->frame, inactive_gc, 0, 0, c->width - (BARHEIGHT() * 3), BARHEIGHT());
	}
	if (!c->trans && c->name)
	{
#ifdef XFT
		XftDrawString8(c->xftdraw, &xft_fg,
			xftfont, SPACE, SPACE + xftfont->ascent,
			c->name, strlen(c->name));
#else
		XDrawString(dpy, c->frame, string_gc,
			SPACE, SPACE + font->ascent,
			c->name, strlen(c->name));
#endif
	}
	// clear button part of bar
	if (c == last_focused_client)
	{
		XFillRectangle(dpy, c->frame, active_gc, c->width - (BARHEIGHT() * 3), 0, BARHEIGHT() * 3, BARHEIGHT());
	}
	else
	{
		XFillRectangle(dpy, c->frame, inactive_gc, c->width - (BARHEIGHT() * 3), 0, BARHEIGHT() * 3, BARHEIGHT());
	}
	XDrawLine(dpy, c->frame, border_gc,
		0, BARHEIGHT() - BW(c) + BW(c)/2,
		c->width, BARHEIGHT() - BW(c) + BW(c)/2);
	XDrawLine(dpy, c->frame, border_gc,
		c->width - BARHEIGHT() + BW(c) / 2, 0,
		c->width - BARHEIGHT() + BW(c) / 2, BARHEIGHT());
	XDrawLine(dpy, c->frame, border_gc,
		c->width - (BARHEIGHT() * 2) + BW(c) / 2, 0,
		c->width - (BARHEIGHT() * 2) + BW(c) / 2, BARHEIGHT());
	XDrawLine(dpy, c->frame, border_gc,
		c->width - (BARHEIGHT() * 3) + BW(c) / 2, 0,
		c->width - (BARHEIGHT() * 3) + BW(c) / 2, BARHEIGHT());
}

/* Window gravity is a mess to explain, but we don't need to do much
 * about it since we're using X borders. For NorthWest et al, the top
 * left corner of the window when there is no WM needs to match up
 * with the top left of our fram once we manage it, and likewise with
 * SouthWest and the bottom right (these are the only values I ever
 * use, but the others should be obvious.) Our titlebar is on the top
 * so we only have to adjust in the first case. */

void gravitate(Client *c, int multiplier)
{
	int dy = 0;
	int gravity = (c->size->flags & PWinGravity) ?
		c->size->win_gravity : NorthWestGravity;

	switch (gravity)
	{
		case NorthWestGravity:
		case NorthEastGravity:
		case NorthGravity:
			dy = BARHEIGHT();
			break;
		case CenterGravity:
			dy = BARHEIGHT()/2;
			break;
	}

	c->y += multiplier * dy;
}

/* Well, the man pages for the shape extension say nothing, but I was
 * able to find a shape.PS.Z on the x.org FTP site. What we want to do
 * here is make the window shape be a boolean OR (or union, if you
 * prefer) of the client's shape and our titlebar. The titlebar
 * requires both a bound and a clip because it has a border -- the X
 * server will paint the border in the region between the two. (I knew
 * that using X borders would get me eventually... ;-)) */

#ifdef SHAPE
void set_shape(Client *c)
{
	int n, order;
	XRectangle temp, *dummy;

	dummy = XShapeGetRectangles(dpy, c->window, ShapeBounding, &n, &order);
	if (n > 1)
	{
		XShapeCombineShape(dpy, c->frame, ShapeBounding,
			0, BARHEIGHT(), c->window, ShapeBounding, ShapeSet);
		temp.x = -BW(c);
		temp.y = -BW(c);
		temp.width = c->width + 2*BW(c);
		temp.height = BARHEIGHT() + BW(c);
		XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
			0, 0, &temp, 1, ShapeUnion, YXBanded);
		temp.x = 0;
		temp.y = 0;
		temp.width = c->width;
		temp.height = BARHEIGHT() - BW(c);
		XShapeCombineRectangles(dpy, c->frame, ShapeClip,
			0, BARHEIGHT(), &temp, 1, ShapeUnion, YXBanded);
		c->has_been_shaped = 1;
	}
	else
	{
		if (c->has_been_shaped)
		{
			//I can't find a 'remove all shaping' function...
			temp.x = -BW(c);
			temp.y = -BW(c);
			temp.width = c->width + 2*BW(c);
			temp.height = c->height + BARHEIGHT() + 2*BW(c);
			XShapeCombineRectangles(dpy, c->frame, ShapeBounding,
				0, 0, &temp, 1, ShapeSet, YXBanded);
		}
	}
	XFree(dummy);
}
#endif

void check_focus(Client *c)
{
	Client *old_focused;
	XSetInputFocus(dpy, c->window, RevertToNone, CurrentTime);
	XInstallColormap(dpy, c->cmap);
	if (c != last_focused_client)
	{
		old_focused = last_focused_client;
		last_focused_client = c;
		redraw(c);
		if (old_focused)
		{
			redraw(old_focused);
		}
		redraw_taskbar();
	}
}

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

static void drag(Client *);
static void sweep(Client *);
static void recalc_sweep(Client *, int, int, Rect *);
static int get_incsize(Client *, int *, int *, Rect *, int);

Window resize_win;

void move(Client *c)
{
	drag(c);
}

void raise_lower(Client *c)
{
	if (c != topmost_client)
	{
		topmost_client = c;
		raise_win(c);
	}
	else
	{
		topmost_client = NULL; //lazy but amiwm does similar
		lower_win(c);
	}
}

void resize(Client *c)
{
	sweep(c);
	XMoveResizeWindow(dpy, c->frame,
		c->x, c->y - BARHEIGHT(), c->width, c->height + BARHEIGHT());
	XMoveResizeWindow(dpy, c->window,
		0, BARHEIGHT(), c->width, c->height);
	send_config(c);
}

void hide(Client *c)
{
	if (!c->ignore_unmap)
	{
		c->ignore_unmap++;
	}
	XUnmapWindow(dpy, c->frame);
	XUnmapWindow(dpy, c->window);
	set_wm_state(c, IconicState);
}

/* The name of this function is a bit misleading: if the client
 * doesn't listen to WM_DELETE then we just terminate it with extreme
 * prejudice. */

void send_wm_delete(Client *c)
{
	int i, n, found = 0;
	Atom *protocols;

	if (XGetWMProtocols(dpy, c->window, &protocols, &n))
	{
		for (i=0; i<n; i++)
		{
			if (protocols[i] == wm_delete)
			{
				found++;
			}
		}
		XFree(protocols);
	}
	if (found)
	{
		send_xmessage(c->window, wm_protos, wm_delete);
	}
	else
	{
		XKillClient(dpy, c->window);
	}
}

static void drag(Client *c)
{
	XEvent ev;
	int old_cx = c->x;
	int old_cy = c->y;
	int x1, y1;
	Client *exposed_c;

	int boundx, boundy, boundw, boundh, mousex, mousey, dw, dh;
	Window constraint_win;
	XSetWindowAttributes pattr;

	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);
	get_mouse_position(&mousex, &mousey);

	boundx = (mousex - c->x) - BW(c);
	boundw = (dw - boundx - (c->width - boundx)) + 1;
	boundy = mousey - c->y;
	boundh = (dh - boundy - (c->height - boundy)) + 1;
	boundy += (BARHEIGHT() + BARHEIGHT());
	boundh += c->height - (BARHEIGHT() + BARHEIGHT());

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, move_curs, CurrentTime) == GrabSuccess))
	{
		return;
	}
	get_mouse_position(&x1, &y1);

	for (;;)
	{
		XMaskEvent(dpy, ExposureMask|MouseMask, &ev);
		switch (ev.type)
		{
			case Expose:
				exposed_c = find_client(ev.xexpose.window, FRAME);
				if (exposed_c)
				{
					redraw(exposed_c);
				}
				break;
			case MotionNotify:
				c->x = old_cx + (ev.xmotion.x - x1);
				c->y = old_cy + (ev.xmotion.y - y1);
				XMoveWindow(dpy, c->frame, c->x, c->y - BARHEIGHT());
				send_config(c);
				break;
			case ButtonRelease:
				ungrab();

				XUnmapWindow(dpy, constraint_win);
				XDestroyWindow(dpy, constraint_win);
				return;
		}
	}
}

static void sweep(Client *c)
{
	XEvent ev;
	Client *exposed_c;
	Rect newdims;

	int boundx, boundy, boundw, boundh, mousex, mousey, dw, dh, minw, minh;
	Window constraint_win;
	XSetWindowAttributes pattr, resize_pattr;

	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);
	get_mouse_position(&mousex, &mousey);

	boundx = 1;
	boundw = dw;
	boundy = BARHEIGHT() + BW(c) + 1;
	boundh = dh - BARHEIGHT() - BW(c);

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, resizestart_curs, CurrentTime) == GrabSuccess))
	{
		return;
	}

	for (;;)
	{
		XMaskEvent(dpy, MouseMask, &ev);
		if (ev.type == ButtonPress)
		{
			get_mouse_position(&newdims.x, &newdims.y);
			break;
		}
	}

	newdims.x -= (BW(c) + 1);
	newdims.y -= (BW(c) + 1);
	newdims.y += BARHEIGHT();

	XUnmapWindow(dpy, constraint_win);
	XDestroyWindow(dpy, constraint_win);

	minw = c->size->min_width > MINWINWIDTH ? c->size->min_width : MINWINWIDTH;
	minh = c->size->min_height > MINWINHEIGHT ? c->size->min_height : MINWINHEIGHT;

	recalc_sweep(c, newdims.x + minw, newdims.y + minh, &newdims);

	if ((newdims.x + newdims.width) > dw)
	{
		newdims.x = ((dw - newdims.width) - 1) - BW(c);
	}
	if ((newdims.y + newdims.height) > dh)
	{
		newdims.y = ((dh - newdims.height) - 1) - BW(c);
	}

	boundx = (newdims.x + newdims.width);
	boundy = (newdims.y + newdims.height);
	boundw = (dw - boundx);
	boundh = (dh - boundy);

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, resizeend_curs, CurrentTime) == GrabSuccess))
	{
		return;
	}

	// create and map resize window
	resize_pattr.override_redirect = True;
	resize_pattr.background_pixel = fc.pixel;
	resize_pattr.border_pixel = bd.pixel;
	resize_pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	resize_win = XCreateWindow(dpy, root,
		newdims.x, newdims.y - BARHEIGHT(), newdims.width + 1, newdims.height + BARHEIGHT() + 1, DEF_BW,
		DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &resize_pattr);
	XMapWindow(dpy, resize_win);

	for (;;)
	{
		XMaskEvent(dpy, ExposureMask|MouseMask, &ev);
		switch (ev.type)
		{
			case Expose:
				exposed_c = find_client(ev.xexpose.window, FRAME);
				if (exposed_c)
				{
					redraw(exposed_c);
				}
				break;
			case MotionNotify:
				recalc_sweep(c, ev.xmotion.x, ev.xmotion.y, &newdims);
				XMoveResizeWindow(dpy, resize_win, newdims.x, newdims.y - BARHEIGHT(), newdims.width + 1, newdims.height + BARHEIGHT() + 1);
				break;
			case ButtonRelease:
				c->x = newdims.x;
				c->y = newdims.y;
				c->width = newdims.width + 1;
				c->height = newdims.height + 1;
				XUngrabServer(dpy);
				ungrab();
				XUnmapWindow(dpy, constraint_win);
				XDestroyWindow(dpy, constraint_win);
				XUnmapWindow(dpy, resize_win);
				XDestroyWindow(dpy, resize_win);
				return;
		}
	}
}

static void recalc_sweep(Client *c, int x2, int y2, Rect *newdims)
{
	int dw, dh;
	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);
	newdims->width = (x2 - newdims->x) - BW(c);
	newdims->height = (y2 - newdims->y) - BW(c);
	get_incsize(c, &newdims->width, &newdims->height, newdims, PIXELS);

	if (c->size->flags & PMinSize)
	{
		if (newdims->width < c->size->min_width)
		{
			newdims->width = c->size->min_width;
		}
		if (newdims->height < c->size->min_height)
		{
			newdims->height = c->size->min_height;
		}
	}

	if (c->size->flags & PMaxSize)
	{
		if (newdims->width > c->size->max_width)
		{
			newdims->width = c->size->max_width;
		}
		if (newdims->height > c->size->max_height)
		{
			newdims->height = c->size->max_height;
		}
	}

	if (newdims->width < MINWINWIDTH)
	{
		newdims->width = MINWINWIDTH;
	}
	if (newdims->height < MINWINHEIGHT)
	{
		newdims->height = MINWINHEIGHT;
	}

	if (newdims->width > (dw - 1))
	{
		newdims->width = dw - 1;
	}
	if (newdims->height > dh - ((BARHEIGHT() * 2) + BW(c)) - 1)
	{
		newdims->height = dh - ((BARHEIGHT() * 2) + BW(c)) - 1;
	}
}

/* If the window in question has a ResizeInc int, then it wants to be
 * resized in multiples of some (x,y). Here we set x_ret and y_ret to
 * the number of multiples (if mode == INCREMENTS) or the correct size
 * in pixels for said multiples (if mode == PIXELS). */

static int get_incsize(Client *c, int *x_ret, int *y_ret, Rect *newdims, int mode)
{
	int basex, basey;
	if (c->size->flags & PResizeInc)
	{
		basex = (c->size->flags & PBaseSize) ? c->size->base_width :
			(c->size->flags & PMinSize) ? c->size->min_width : 0;
		basey = (c->size->flags & PBaseSize) ? c->size->base_height :
			(c->size->flags & PMinSize) ? c->size->min_height : 0;
		if (mode == PIXELS)
		{
			*x_ret = newdims->width - ((newdims->width - basex) % c->size->width_inc);
			*y_ret = newdims->height - ((newdims->height - basey) % c->size->height_inc);
		}
		else //INCREMENTS
		{
			*x_ret = (newdims->width - basex) / c->size->width_inc;
			*y_ret = (newdims->height - basey) / c->size->height_inc;
		}
		return 1;
	}
	return 0;
}



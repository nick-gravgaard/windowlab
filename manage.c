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

static void drag(Client *);
static void sweep(Client *);
static void recalc_sweep(Client *, int, int, int, int);
static void draw_outline(Client *);
static int get_incsize(Client *, int *, int *, int);

void move(Client *c)
{
	drag(c);
//	XMoveWindow(dpy, c->frame, c->x, c->y - theight(c));
//	send_config(c);
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
		c->x, c->y - theight(c), c->width, c->height + theight(c));
	XMoveResizeWindow(dpy, c->window,
		0, theight(c), c->width, c->height);
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
	int x1, y1;
	int old_cx = c->x;
	int old_cy = c->y;
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
	boundy += (BARHEIGHT() + theight(c));
	boundh += c->height - (BARHEIGHT() + theight(c));

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
//	constraint_win = XCreateSimpleWindow(dpy, root, boundx, boundy, boundw, boundh, 0, 0, ParentRelative);
//	XLowerWindow(dpy, constraint_win);
	XMapWindow(dpy, constraint_win);

//	if (!grab(root, MouseMask, move_curs))
//	{
//		return;
//	}

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, move_curs, CurrentTime) == GrabSuccess))
	{
		return;
	}
//	XGrabServer(dpy);
	get_mouse_position(&x1, &y1);

//	draw_outline(c);
	for (;;)
	{
//		XMaskEvent(dpy, MouseMask, &ev);
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
//				draw_outline(c); // clear
				c->x = old_cx + (ev.xmotion.x - x1);
				c->y = old_cy + (ev.xmotion.y - y1);
				XMoveWindow(dpy, c->frame, c->x, c->y - theight(c));
				send_config(c);
//				draw_outline(c);
				break;
			case ButtonRelease:
//				draw_outline(c); // clear
//				XUngrabServer(dpy);
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
	int old_cx = c->x;
	int old_cy = c->y;
//	int old_cx;// = c->x;
//	int old_cy;// = c->y;

	int boundx, boundy, boundw, boundh, mousex, mousey, dw, dh;
	Window constraint_win;
	XSetWindowAttributes pattr;

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

//	if (!grab(root, MouseMask, resize_curs))
//	{
//		return;
//	}
//	XGrabServer(dpy);

//	do
//	{
//		XMaskEvent(dpy, MouseMask, &ev);
//	}
//	while (ev.type != ButtonRelease);

	for (;;)
	{
		XMaskEvent(dpy, MouseMask, &ev);
		if (ev.type == ButtonPress)
		{
			get_mouse_position(&old_cx, &old_cy);
			break;
		}
	}

	old_cx -= (BW(c) + 1);
	old_cy -= (BW(c) + 1);

	XUnmapWindow(dpy, constraint_win);
	XDestroyWindow(dpy, constraint_win);

	boundx = old_cx + MINWINWIDTH + BW(c);
	boundw = dw - boundx - 1;
	boundy = old_cy + MINWINHEIGHT + BW(c) + theight(c);
	boundh = dh - boundy - 1;

	if (boundx > dw)
	{
		old_cx = dw - MINWINWIDTH - BW(c);
		boundx = dw - 1;
		boundw = 1;
	}
	if (boundy > dh)
	{
		old_cy = dh - MINWINHEIGHT - BW(c) - theight(c);
		boundy = dh - 1;
		boundh = 1;
	}

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, resizeend_curs, CurrentTime) == GrabSuccess))
	{
		return;
	}

	recalc_sweep(c, old_cx, old_cy, ev.xmotion.x, ev.xmotion.y); //ng today
	draw_outline(c);
//	setmouse(c->window, c->width, c->height);
	for (;;)
	{
		XMaskEvent(dpy, MouseMask, &ev);
		switch (ev.type)
		{
			case MotionNotify:
				draw_outline(c); //clear
				recalc_sweep(c, old_cx, old_cy, ev.xmotion.x, ev.xmotion.y);
				draw_outline(c);
				break;
			case ButtonRelease:
				draw_outline(c); //clear
				XUngrabServer(dpy);
				ungrab();

				XUnmapWindow(dpy, constraint_win);
				XDestroyWindow(dpy, constraint_win);
				return;

		}
	}
}

static void recalc_sweep(Client *c, int x1, int y1, int x2, int y2)
{
	c->width = abs(x1 - x2);
	c->height = abs(y1 - y2);
	c->height -= theight(c);

	get_incsize(c, &c->width, &c->height, PIXELS);

	if (c->size->flags & PMinSize)
	{
		if (c->width < c->size->min_width)
		{
			c->width = c->size->min_width;
		}
		if (c->height < c->size->min_height)
		{
			c->height = c->size->min_height;
		}
	}

	if (c->size->flags & PMaxSize)
	{
		if (c->width > c->size->max_width)
		{
			c->width = c->size->max_width;
		}
		if (c->height > c->size->max_height)
		{
			c->height = c->size->max_height;
		}
	}

	if (c->width < MINWINWIDTH)
	{
		c->width = MINWINWIDTH;
	}
	if (c->height < MINWINHEIGHT)
	{
		c->height = MINWINHEIGHT;
	}

	c->x = (x1 <= x2) ? x1 : x1 - c->width;
	c->y = (y1 <= y2) ? y1 : y1 - c->height - theight(c);

	c->y += theight(c);
}

static void draw_outline(Client *c)
{
//	char buf[32];
	int width, height;

	XFillRectangle(dpy, root, invert_gc,
		c->x, c->y - theight(c),
		c->width + BW(c), c->height + theight(c) + BW(c));

/*	XDrawRectangle(dpy, root, invert_gc,
		c->x + BW(c)/2, c->y - theight(c) + BW(c)/2,
		c->width + BW(c), c->height + theight(c) + BW(c));
	XDrawLine(dpy, root, invert_gc, c->x + BW(c), c->y + BW(c)/2,
		c->x + c->width + BW(c), c->y + BW(c)/2);
*/
	if (!get_incsize(c, &width, &height, INCREMENTS))
	{
		width = c->width;
		height = c->height;
	}
/*
	gravitate(c, REMOVE_GRAVITY);
	snprintf(buf, sizeof buf, "%dx%d+%d+%d", width, height, c->x, c->y);
	gravitate(c, APPLY_GRAVITY);
	XDrawString(dpy, root, invert_gc,
		c->x + c->width - XTextWidth(font, buf, strlen(buf)) - SPACE,
		c->y + c->height - SPACE,
		buf, strlen(buf));
*/
}

/* If the window in question has a ResizeInc int, then it wants to be
 * resized in multiples of some (x,y). Here we set x_ret and y_ret to
 * the number of multiples (if mode == INCREMENTS) or the correct size
 * in pixels for said multiples (if mode == PIXELS). */

static int get_incsize(Client *c, int *x_ret, int *y_ret, int mode)
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
			*x_ret = c->width - ((c->width - basex) % c->size->width_inc);
			*y_ret = c->height - ((c->height - basey) % c->size->height_inc);
		}
		else //INCREMENTS
		{
			*x_ret = (c->width - basex) / c->size->width_inc;
			*y_ret = (c->height - basey) / c->size->height_inc;
		}
		return 1;
	}

	return 0;
}

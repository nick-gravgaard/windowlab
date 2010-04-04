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
	else // WINDOW
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

/* Attempt to follow the ICCCM by explicitly specifying 32 bits for
 * this property. Does this goof up on 64 bit systems? */

void set_wm_state(Client *c, int state)
{
	CARD32 data[2];

	data[0] = state;
	data[1] = None; //Icon? We don't need no steenking icon.

	XChangeProperty(dsply, c->window, wm_state, wm_state, 32, PropModeReplace, (unsigned char *)data, 2);
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
	long state = WithdrawnState;
	unsigned long items_read, items_left;
	unsigned char *data;

	if (XGetWindowProperty(dsply, c->window, wm_state, 0L, 2L, False, wm_state, &real_type, &real_format, &items_read, &items_left, &data) == Success && items_read)
	{
		state = *(long *)data;
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

	XSendEvent(dsply, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

/* After pulling my hair out trying to find some way to tell if a
 * window is still valid, I've decided to instead carefully ignore any
 * errors raised by this function. We know that the X calls are, and
 * we know the only reason why they could fail -- a window has removed
 * itself completely before the Unmap and Destroy events get through
 * the queue to us. It's not absolutely perfect, but it works.
 *
 * The 'withdrawing' argument specifies if the client is actually
 * (destroying itself||being destroyed by us) or if we are merely
 * cleaning up its data structures when we exit mid-session. */

void remove_client(Client *c, int mode)
{
	Client *p;

	XGrabServer(dsply);
	XSetErrorHandler(ignore_xerror);

#ifdef DEBUG
	err("removing %s, %d: %d left", c->name, mode, XPending(dsply));
#endif

	if (mode == WITHDRAW)
	{
		set_wm_state(c, WithdrawnState);
	}
	else //REMAP
	{
		XMapWindow(dsply, c->window);
	}
	gravitate(c, REMOVE_GRAVITY);
	XReparentWindow(dsply, c->window, root, c->x, c->y);
#ifdef MWM_HINTS
	if (c->has_border)
	{
		XSetWindowBorderWidth(dsply, c->window, 1);
	}
#else
	XSetWindowBorderWidth(dsply, c->window, 1);
#endif
#ifdef XFT
	XftDrawDestroy(c->xftdraw);
#endif
	XRemoveFromSaveSet(dsply, c->window);
	XDestroyWindow(dsply, c->frame);

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
	if (c->name != NULL)
	{
		XFree(c->name);
	}
	if (c->size)
	{
		XFree(c->size);
	}
	if (c == fullscreen_client)
	{
		fullscreen_client = NULL;
	}
	if (c == focused_client)
	{
		focused_client = NULL;
		check_focus(get_prev_focused());
	}
	free(c);

	XSync(dsply, False);
	XSetErrorHandler(handle_xerror);
	XUngrabServer(dsply);

	redraw_taskbar();
}

void redraw(Client *c)
{
	if (c == fullscreen_client)
	{
		return;
	}
#ifdef MWM_HINTS
	if (!c->has_title)
	{
		return;
	}
#endif
	XDrawLine(dsply, c->frame, border_gc, 0, BARHEIGHT() - DEF_BORDERWIDTH + DEF_BORDERWIDTH / 2, c->width, BARHEIGHT() - DEF_BORDERWIDTH + DEF_BORDERWIDTH / 2);
	// clear text part of bar
	if (c == focused_client)
	{
		XFillRectangle(dsply, c->frame, active_gc, 0, 0, c->width - ((BARHEIGHT() - DEF_BORDERWIDTH) * 3), BARHEIGHT() - DEF_BORDERWIDTH);
	}
	else
	{
		XFillRectangle(dsply, c->frame, inactive_gc, 0, 0, c->width - ((BARHEIGHT() - DEF_BORDERWIDTH) * 3), BARHEIGHT() - DEF_BORDERWIDTH);
	}
	if (!c->trans && c->name != NULL)
	{
#ifdef XFT
		XftDrawString8(c->xftdraw, &xft_detail, xftfont, SPACE, SPACE + xftfont->ascent, (unsigned char *)c->name, strlen(c->name));
#else
		XDrawString(dsply, c->frame, text_gc, SPACE, SPACE + font->ascent, c->name, strlen(c->name));
#endif
	}
	if (c == focused_client)
	{
		draw_hide_button(c, &text_gc, &active_gc);
		draw_toggledepth_button(c, &text_gc, &active_gc);
		draw_close_button(c, &text_gc, &active_gc);
	}
	else
	{
		draw_hide_button(c, &text_gc, &inactive_gc);
		draw_toggledepth_button(c, &text_gc, &inactive_gc);
		draw_close_button(c, &text_gc, &inactive_gc);
	}
}

/* Window gravity is a mess to explain, but we don't need to do much
 * about it since we're using X borders. For NorthWest et al, the top
 * left corner of the window when there is no WM needs to match up
 * with the top left of our fram once we manage it, and likewise with
 * SouthWest and the bottom right (these are the only values I ever
 * use, but the others should be obvious). Our titlebar is on the top
 * so we only have to adjust in the first case. */

void gravitate(Client *c, int multiplier)
{
	int dy = 0;
	int gravity = (c->size->flags & PWinGravity) ? c->size->win_gravity : NorthWestGravity;

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

	dummy = XShapeGetRectangles(dsply, c->window, ShapeBounding, &n, &order);
	if (n > 1)
	{
		XShapeCombineShape(dsply, c->frame, ShapeBounding, 0, BARHEIGHT(), c->window, ShapeBounding, ShapeSet);
		temp.x = -BORDERWIDTH(c);
		temp.y = -BORDERWIDTH(c);
		temp.width = c->width + (2 * BORDERWIDTH(c));
		temp.height = BARHEIGHT() + BORDERWIDTH(c);
		XShapeCombineRectangles(dsply, c->frame, ShapeBounding, 0, 0, &temp, 1, ShapeUnion, YXBanded);
		temp.x = 0;
		temp.y = 0;
		temp.width = c->width;
		temp.height = BARHEIGHT() - BORDERWIDTH(c);
		XShapeCombineRectangles(dsply, c->frame, ShapeClip, 0, BARHEIGHT(), &temp, 1, ShapeUnion, YXBanded);
		c->has_been_shaped = 1;
	}
	else
	{
		if (c->has_been_shaped)
		{
			// I can't find a 'remove all shaping' function...
			temp.x = -BORDERWIDTH(c);
			temp.y = -BORDERWIDTH(c);
			temp.width = c->width + (2 * BORDERWIDTH(c));
			temp.height = c->height + BARHEIGHT() + (2 * BORDERWIDTH(c));
			XShapeCombineRectangles(dsply, c->frame, ShapeBounding, 0, 0, &temp, 1, ShapeSet, YXBanded);
		}
	}
	XFree(dummy);
}
#endif

void check_focus(Client *c)
{
	if (c != NULL)
	{
		XSetInputFocus(dsply, c->window, RevertToNone, CurrentTime);
		XInstallColormap(dsply, c->cmap);
	}
	if (c != focused_client)
	{
		Client *old_focused = focused_client;
		focused_client = c;
		focus_count++;
		if (c != NULL)
		{
			c->focus_order = focus_count;
			redraw(c);
		}
		if (old_focused != NULL)
		{
			redraw(old_focused);
		}
		redraw_taskbar();
	}
}

Client *get_prev_focused(void)
{
	Client *c = head_client;
	Client *prev_focused = NULL;
	unsigned int highest = 0;

	while (c != NULL)
	{
		if (!c->hidden && c->focus_order > highest)
		{
			highest = c->focus_order;
			prev_focused = c;
		}
		c = c->next;
	}
	return prev_focused;
}

void draw_hide_button(Client *c, GC *detail_gc, GC *background_gc)
{
	int x, topleft_offset;
	x = c->width - ((BARHEIGHT() - DEF_BORDERWIDTH) * 3);
	topleft_offset = (BARHEIGHT() / 2) - 5; // 5 being ~half of 9
	XFillRectangle(dsply, c->frame, *background_gc, x, 0, BARHEIGHT() - DEF_BORDERWIDTH, BARHEIGHT() - DEF_BORDERWIDTH);

	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 4, topleft_offset + 2, x + topleft_offset + 4, topleft_offset + 0);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 6, topleft_offset + 2, x + topleft_offset + 7, topleft_offset + 1);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 6, topleft_offset + 4, x + topleft_offset + 8, topleft_offset + 4);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 6, topleft_offset + 6, x + topleft_offset + 7, topleft_offset + 7);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 4, topleft_offset + 6, x + topleft_offset + 4, topleft_offset + 8);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 2, topleft_offset + 6, x + topleft_offset + 1, topleft_offset + 7);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 2, topleft_offset + 4, x + topleft_offset + 0, topleft_offset + 4);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 2, topleft_offset + 2, x + topleft_offset + 1, topleft_offset + 1);
}

void draw_toggledepth_button(Client *c, GC *detail_gc, GC *background_gc)
{
	int x, topleft_offset;
	x = c->width - ((BARHEIGHT() - DEF_BORDERWIDTH) * 2);
	topleft_offset = (BARHEIGHT() / 2) - 6; // 6 being ~half of 11
	XFillRectangle(dsply, c->frame, *background_gc, x, 0, BARHEIGHT() - DEF_BORDERWIDTH, BARHEIGHT() - DEF_BORDERWIDTH);

	XDrawRectangle(dsply, c->frame, *detail_gc, x + topleft_offset, topleft_offset, 7, 7);
	XDrawRectangle(dsply, c->frame, *detail_gc, x + topleft_offset + 3, topleft_offset + 3, 7, 7);
}

void draw_close_button(Client *c, GC *detail_gc, GC *background_gc)
{
	int x, topleft_offset;
	x = c->width - (BARHEIGHT() - DEF_BORDERWIDTH);
	topleft_offset = (BARHEIGHT() / 2) - 5; // 5 being ~half of 9
	XFillRectangle(dsply, c->frame, *background_gc, x, 0, BARHEIGHT() - DEF_BORDERWIDTH, BARHEIGHT() - DEF_BORDERWIDTH);

	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 1, topleft_offset, x + topleft_offset + 8, topleft_offset + 7);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 1, topleft_offset + 1, x + topleft_offset + 7, topleft_offset + 7);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset, topleft_offset + 1, x + topleft_offset + 7, topleft_offset + 8);

	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset, topleft_offset + 7, x + topleft_offset + 7, topleft_offset);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 1, topleft_offset + 7, x + topleft_offset + 7, topleft_offset + 1);
	XDrawLine(dsply, c->frame, *detail_gc, x + topleft_offset + 1, topleft_offset + 8, x + topleft_offset + 8, topleft_offset + 1);
}

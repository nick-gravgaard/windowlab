/* WindowLab - an X11 window manager
 * Copyright (c) 2001-2005 Nick Gravgaard
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

static void limit_size(Client *, Rect *);
static int get_incsize(Client *, unsigned int *, unsigned int *, Rect *, int);

void raise_lower(Client *c)
{
	if (c == topmost_client)
	{
		lower_win(c);
		topmost_client = NULL; // lazy but amiwm does similar
	}
	else
	{
		raise_win(c);
		topmost_client = c;
	}
}

/* Increment ignore_unmap here and decrement it in handle_unmap_event in
 * events.c */

void hide(Client *c)
{
	if (!c->hidden)
	{
		c->ignore_unmap++;
		c->hidden = 1;
		if (c == topmost_client)
		{
			topmost_client = NULL;
		}
		XUnmapWindow(dpy, c->frame);
		XUnmapWindow(dpy, c->window);
		set_wm_state(c, IconicState);
		check_focus(get_prev_focused());
	}
}

void unhide(Client *c)
{
	if (c->hidden)
	{
		c->hidden = 0;
		topmost_client = c;
		XMapWindow(dpy, c->window);
		XMapRaised(dpy, c->frame);
		set_wm_state(c, NormalState);
	}
}

void toggle_fullscreen(Client *c)
{
	int xoffset, yoffset;
	int maxwinwidth, maxwinheight;
	if (c != NULL && !c->trans)
	{
		if (c == fullscreen_client) // reset to original size
		{
			c->x = fs_prevdims.x;
			c->y = fs_prevdims.y;
			c->width = fs_prevdims.width;
			c->height = fs_prevdims.height;
			XMoveResizeWindow(dpy, c->frame, c->x, c->y - BARHEIGHT(), c->width, c->height + BARHEIGHT());
			XMoveResizeWindow(dpy, c->window, 0, BARHEIGHT(), c->width, c->height);
			send_config(c);
			fullscreen_client = NULL;
			showing_taskbar = 1;
		}
		else // make fullscreen
		{
			xoffset = yoffset = 0;
			maxwinwidth = DisplayWidth(dpy, screen);
			maxwinheight = DisplayHeight(dpy, screen) - BARHEIGHT();
			if (fullscreen_client != NULL) // reset existing fullscreen window to original size
			{
				fullscreen_client->x = fs_prevdims.x;
				fullscreen_client->y = fs_prevdims.y;
				fullscreen_client->width = fs_prevdims.width;
				fullscreen_client->height = fs_prevdims.height;
				XMoveResizeWindow(dpy, fullscreen_client->frame, fullscreen_client->x, fullscreen_client->y - BARHEIGHT(), fullscreen_client->width, fullscreen_client->height + BARHEIGHT());
				XMoveResizeWindow(dpy, fullscreen_client->window, 0, BARHEIGHT(), fullscreen_client->width, fullscreen_client->height);
				send_config(fullscreen_client);
			}
			fs_prevdims.x = c->x;
			fs_prevdims.y = c->y;
			fs_prevdims.width = c->width;
			fs_prevdims.height = c->height;
			c->x = 0 - BORDERWIDTH(c);
			c->y = BARHEIGHT() - DEF_BORDERWIDTH;
			c->width = maxwinwidth;
			c->height = maxwinheight;
			if (c->size->flags & PMaxSize || c->size->flags & PResizeInc)
			{
				if (c->size->flags & PResizeInc)
				{
					Rect maxwinsize;
					maxwinsize.x = xoffset;
					maxwinsize.width = maxwinwidth;
					maxwinsize.y = yoffset;
					maxwinsize.height = maxwinheight;
					get_incsize(c, (unsigned int *)&c->size->max_width, (unsigned int *)&c->size->max_height, &maxwinsize, PIXELS);
				}
				if (c->size->max_width < maxwinwidth)
				{
					c->width = c->size->max_width;
					xoffset = (maxwinwidth - c->width) / 2;
				}
				if (c->size->max_height < maxwinheight)
				{
					c->height = c->size->max_height;
					yoffset = (maxwinheight - c->height) / 2;
				}
			}
			XMoveResizeWindow(dpy, c->frame, c->x, c->y, maxwinwidth, maxwinheight);
			XMoveResizeWindow(dpy, c->window, xoffset, yoffset, c->width, c->height);
			send_config(c);
			fullscreen_client = c;
			showing_taskbar = in_taskbar;
		}
		redraw_taskbar();
	}
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
		for (i = 0; i < n; i++)
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

void move(Client *c)
{
	XEvent ev;
	int old_cx = c->x;
	int old_cy = c->y;
	int mousex, mousey, dw, dh;
	Client *exposed_c;
	Rect bounddims;
	Window constraint_win;
	XSetWindowAttributes pattr;

	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);
	get_mouse_position(&mousex, &mousey);

	bounddims.x = (mousex - c->x) - BORDERWIDTH(c);
	bounddims.width = (dw - bounddims.x - (c->width - bounddims.x)) + 1;
	bounddims.y = mousey - c->y;
	bounddims.height = (dh - bounddims.y - (c->height - bounddims.y)) + 1;
	bounddims.y += (BARHEIGHT() * 2) - DEF_BORDERWIDTH;
	bounddims.height += c->height - ((BARHEIGHT() * 2) - DEF_BORDERWIDTH);

	constraint_win = XCreateWindow(dpy, root, bounddims.x, bounddims.y, bounddims.width, bounddims.height, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
#ifdef DEBUG
	fprintf(stderr, "move() : constraint_win is (%d, %d)-(%d, %d)\n", bounddims.x, bounddims.y, bounddims.x + bounddims.width, bounddims.y + bounddims.height);
#endif
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, moveresize_curs, CurrentTime) == GrabSuccess))
	{
		XDestroyWindow(dpy, constraint_win);
		return;
	}

	do
	{
		XMaskEvent(dpy, ExposureMask|MouseMask, &ev);
		switch (ev.type)
		{
			case Expose:
				exposed_c = find_client(ev.xexpose.window, FRAME);
				if (exposed_c != NULL)
				{
					redraw(exposed_c);
				}
				break;
			case MotionNotify:
				c->x = old_cx + (ev.xmotion.x - mousex);
				c->y = old_cy + (ev.xmotion.y - mousey);
				XMoveWindow(dpy, c->frame, c->x, c->y - BARHEIGHT());
				send_config(c);
				break;
		}
	}
	while (ev.type != ButtonRelease);

	ungrab();
	XDestroyWindow(dpy, constraint_win);
}

void resize(Client *c, int x, int y)
{
	XEvent ev;
	Client *exposed_c;
	Rect newdims, recalceddims, bounddims;
	unsigned int dragging_outwards, dw, dh;
	Window constraint_win, resize_win, resizebar_win;
	XSetWindowAttributes pattr, resize_pattr, resizebar_pattr;

	if (x > c->x + BORDERWIDTH(c) && x < (c->x + c->width) - BORDERWIDTH(c) && y > (c->y - BARHEIGHT()) + BORDERWIDTH(c) && y < (c->y + c->height) - BORDERWIDTH(c))
	{
		// inside the window, dragging outwards
		dragging_outwards = 1;
	}
	else
	{
		// outside the window, dragging inwards
		dragging_outwards = 0;
	}

	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);

	bounddims.x = 0;
	bounddims.width = dw;
	bounddims.y = 0;
	bounddims.height = dh;

	constraint_win = XCreateWindow(dpy, root, bounddims.x, bounddims.y, bounddims.width, bounddims.height, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, moveresize_curs, CurrentTime) == GrabSuccess))
	{
		XDestroyWindow(dpy, constraint_win);
		return;
	}

	newdims.x = c->x;
	newdims.y = c->y - BARHEIGHT();
	newdims.width = c->width;
	newdims.height = c->height + BARHEIGHT();

	copy_dims(&newdims, &recalceddims);

	// create and map resize window
	resize_pattr.override_redirect = True;
	resize_pattr.background_pixel = menu_col.pixel;
	resize_pattr.border_pixel = border_col.pixel;
	resize_pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	resize_win = XCreateWindow(dpy, root, newdims.x, newdims.y, newdims.width, newdims.height, DEF_BORDERWIDTH, DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen), CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &resize_pattr);
	XMapRaised(dpy, resize_win);

	resizebar_pattr.override_redirect = True;
	resizebar_pattr.background_pixel = active_col.pixel;
	resizebar_pattr.border_pixel = border_col.pixel;
	resizebar_pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	resizebar_win = XCreateWindow(dpy, resize_win, -DEF_BORDERWIDTH, -DEF_BORDERWIDTH, newdims.width, BARHEIGHT() - DEF_BORDERWIDTH, DEF_BORDERWIDTH, DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen), CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &resizebar_pattr);
	XMapRaised(dpy, resizebar_win);

#ifdef XFT
	// temporarily swap drawables in order to draw on the resize windows XFT context
	XftDrawChange(c->xftdraw, (Drawable) resizebar_win);
#endif

	// hide real windows frame
	XUnmapWindow(dpy, c->frame);

	do
	{
		XMaskEvent(dpy, ExposureMask|MouseMask, &ev);
		switch (ev.type)
		{
			case Expose:
				if (ev.xexpose.window == resizebar_win)
				{
					write_titletext(c, resizebar_win);
				}
				else
				{
					exposed_c = find_client(ev.xexpose.window, FRAME);
					if (exposed_c)
					{
						redraw(exposed_c);
					}
				}
				break;
			case MotionNotify:
				{
					unsigned int in_taskbar = 1, leftedge_changed = 0, rightedge_changed = 0, topedge_changed = 0, bottomedge_changed = 0;
					int newwidth, newheight;
					// warping the pointer is wrong - wait until it leaves the taskbar
					if (ev.xmotion.y < BARHEIGHT())
					{
						in_taskbar = 1;
					}
					else
					{
						if (in_taskbar == 1) // first time outside taskbar
						{
							in_taskbar = 0;
							bounddims.x = 0;
							bounddims.width = dw;
							bounddims.y = BARHEIGHT();
							bounddims.height = dh - BARHEIGHT();
							XMoveResizeWindow(dpy, constraint_win, bounddims.x, bounddims.y, bounddims.width, bounddims.height);
							in_taskbar = 0;
						}
						// inside the window, dragging outwards
						if (dragging_outwards)
						{
							if (ev.xmotion.x < newdims.x + BORDERWIDTH(c))
							{
								newdims.width += newdims.x + BORDERWIDTH(c) - ev.xmotion.x;
								newdims.x = ev.xmotion.x - BORDERWIDTH(c);
								leftedge_changed = 1;
							}
							else if (ev.xmotion.x > newdims.x + newdims.width + BORDERWIDTH(c))
							{
								newdims.width = (ev.xmotion.x - newdims.x - BORDERWIDTH(c)) + 1;
								rightedge_changed = 1;
							}
							if (ev.xmotion.y < newdims.y + BORDERWIDTH(c))
							{
								newdims.height += newdims.y + BORDERWIDTH(c) - ev.xmotion.y;
								newdims.y = ev.xmotion.y - BORDERWIDTH(c);
								topedge_changed = 1;
							}
							else if (ev.xmotion.y > newdims.y + newdims.height + BORDERWIDTH(c))
							{
								newdims.height = (ev.xmotion.y - newdims.y - BORDERWIDTH(c)) + 1;
								bottomedge_changed = 1;
							}
						}
						// outside the window, dragging inwards
						else
						{
							unsigned int above_win, below_win, leftof_win, rightof_win;
							unsigned int in_win;

							above_win = (ev.xmotion.y < newdims.y + BORDERWIDTH(c));
							below_win = (ev.xmotion.y > newdims.y + newdims.height + BORDERWIDTH(c));
							leftof_win = (ev.xmotion.x < newdims.x + BORDERWIDTH(c));
							rightof_win = (ev.xmotion.x > newdims.x + newdims.width + BORDERWIDTH(c));

							in_win = ((!above_win) && (!below_win) && (!leftof_win) && (!rightof_win));

							if (in_win)
							{
								unsigned int from_left, from_right, from_top, from_bottom;
								from_left = ev.xmotion.x - newdims.x - BORDERWIDTH(c);
								from_right = newdims.x + newdims.width + BORDERWIDTH(c) - ev.xmotion.x;
								from_top = ev.xmotion.y - newdims.y - BORDERWIDTH(c);
								from_bottom = newdims.y + newdims.height + BORDERWIDTH(c) - ev.xmotion.y;
								if (from_left < from_right && from_left < from_top && from_left < from_bottom)
								{
									newdims.width -= ev.xmotion.x - newdims.x - BORDERWIDTH(c);
									newdims.x = ev.xmotion.x - BORDERWIDTH(c);
									leftedge_changed = 1;
								}
								else if (from_right < from_top && from_right < from_bottom)
								{
									newdims.width = ev.xmotion.x - newdims.x - BORDERWIDTH(c);
									rightedge_changed = 1;
								}
								else if (from_top < from_bottom)
								{
									newdims.height -= ev.xmotion.y - newdims.y - BORDERWIDTH(c);
									newdims.y = ev.xmotion.y - BORDERWIDTH(c);
									topedge_changed = 1;
								}
								else
								{
									newdims.height = ev.xmotion.y - newdims.y - BORDERWIDTH(c);
									bottomedge_changed = 1;
								}
							}
						}
						// coords have changed
						if (leftedge_changed || rightedge_changed || topedge_changed || bottomedge_changed)
						{
							copy_dims(&newdims, &recalceddims);
							recalceddims.height -= BARHEIGHT();

							if (get_incsize(c, (unsigned int *)&newwidth, (unsigned int *)&newheight, &recalceddims, PIXELS))
							{
								if (leftedge_changed)
								{
									recalceddims.x = (recalceddims.x + recalceddims.width) - newwidth;
									recalceddims.width = newwidth;
								}
								else if (rightedge_changed)
								{
									recalceddims.width = newwidth;
								}

								if (topedge_changed)
								{
									recalceddims.y = (recalceddims.y + recalceddims.height) - newheight;
									recalceddims.height = newheight;
								}
								else if (bottomedge_changed)
								{
									recalceddims.height = newheight;
								}
							}

							recalceddims.height += BARHEIGHT();
							limit_size(c, &recalceddims);

							XMoveResizeWindow(dpy, resize_win, recalceddims.x, recalceddims.y, recalceddims.width, recalceddims.height);
							XResizeWindow(dpy, resizebar_win, recalceddims.width, BARHEIGHT() - DEF_BORDERWIDTH);
						}
					}
				}
				break;
		}
	}
	while (ev.type != ButtonRelease);

	XUngrabServer(dpy);
	ungrab();
	c->x = recalceddims.x;
	c->y = recalceddims.y + BARHEIGHT();
	c->width = recalceddims.width;
	c->height = recalceddims.height - BARHEIGHT();

	XMoveResizeWindow(dpy, c->frame, c->x, c->y - BARHEIGHT(), c->width, c->height + BARHEIGHT());
	XResizeWindow(dpy, c->window, c->width, c->height);

	// unhide real windows frame
	XMapWindow(dpy, c->frame);

	XSetInputFocus(dpy, c->window, RevertToNone, CurrentTime);

	send_config(c);
	XDestroyWindow(dpy, constraint_win);

#ifdef XFT
	// reset the drawable
	XftDrawChange(c->xftdraw, (Drawable) c->frame);
#endif
	
	XDestroyWindow(dpy, resizebar_win);
	XDestroyWindow(dpy, resize_win);
}

static void limit_size(Client *c, Rect *newdims)
{
	int dw, dh;
	dw = DisplayWidth(dpy, screen);
	dh = DisplayHeight(dpy, screen);

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

	if (newdims->width > dw)
	{
		newdims->width = dw;
	}
	if (newdims->height > (dh - BARHEIGHT()))
	{
		newdims->height = (dh - BARHEIGHT());
	}
}

/* If the window in question has a ResizeInc int, then it wants to be
 * resized in multiples of some (x,y). Here we set x_ret and y_ret to
 * the number of multiples (if mode == INCREMENTS) or the correct size
 * in pixels for said multiples (if mode == PIXELS). */

static int get_incsize(Client *c, unsigned int *x_ret, unsigned int *y_ret, Rect *newdims, int mode)
{
	int basex, basey;
	if (c->size->flags & PResizeInc)
	{
		basex = (c->size->flags & PBaseSize) ? c->size->base_width : (c->size->flags & PMinSize) ? c->size->min_width : 0;
		basey = (c->size->flags & PBaseSize) ? c->size->base_height : (c->size->flags & PMinSize) ? c->size->min_height : 0;
		// work around broken apps that set their resize increments to 0
		if (mode == PIXELS)
		{
			if (c->size->width_inc != 0)
			{
				*x_ret = newdims->width - ((newdims->width - basex) % c->size->width_inc);
			}
			if (c->size->height_inc != 0)
			{
				*y_ret = newdims->height - ((newdims->height - basey) % c->size->height_inc);
			}
		}
		else // INCREMENTS
		{
			if (c->size->width_inc != 0)
			{
				*x_ret = (newdims->width - basex) / c->size->width_inc;
			}
			if (c->size->height_inc != 0)
			{
				*y_ret = (newdims->height - basey) / c->size->height_inc;
			}
		}
		return 1;
	}
	return 0;
}

void write_titletext(Client *c, Window bar_win)
{
#ifdef MWM_HINTS
	if (!c->has_title)
	{
		return;
	}
#endif
	if (!c->trans && c->name != NULL)
	{
#ifdef XFT
		XftDrawString8(c->xftdraw, &xft_detail, xftfont, SPACE, SPACE + xftfont->ascent, (unsigned char *)c->name, strlen(c->name));
#else
		XDrawString(dpy, bar_win, text_gc, SPACE, SPACE + font->ascent, c->name, strlen(c->name));
#endif
	}
}

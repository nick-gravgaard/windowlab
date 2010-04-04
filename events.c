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

#include <X11/Xatom.h>
#include "windowlab.h"

static void handle_key_press(XKeyEvent *);
static void handle_button_press(XButtonEvent *);
static void handle_windowbar_click(XButtonEvent *, Client *);
static unsigned int box_clicked(Client *, int);
static void draw_button(Client *, GC *, GC *, unsigned int);
static void handle_configure_request(XConfigureRequestEvent *);
static void handle_map_request(XMapRequestEvent *);
static void handle_unmap_event(XUnmapEvent *);
static void handle_destroy_event(XDestroyWindowEvent *);
static void handle_client_message(XClientMessageEvent *);
static void handle_property_change(XPropertyEvent *);
static void handle_enter_event(XCrossingEvent *);
static void handle_colormap_change(XColormapEvent *);
static void handle_expose_event(XExposeEvent *);
#ifdef SHAPE
static void handle_shape_change(XShapeEvent *);
#endif

static int interruptible_XNextEvent(XEvent *event);

/* We may want to put in some sort of check for unknown events at some
 * point. TWM has an interesting and different way of doing this... */

void do_event_loop(void)
{
	XEvent ev;

	for (;;)
	{
		interruptible_XNextEvent(&ev);
#ifdef DEBUG
		show_event(ev);
#endif
		/* check to see if menu rebuild has been requested */
		if (do_menuitems)
		{
			free_menuitems();
			get_menuitems();
		}

		switch (ev.type)
		{
			case KeyPress:
				handle_key_press(&ev.xkey);
				break;
			case ButtonPress:
				handle_button_press(&ev.xbutton);
				break;
			case ConfigureRequest:
				handle_configure_request(&ev.xconfigurerequest);
				break;
			case MapRequest:
				handle_map_request(&ev.xmaprequest);
				break;
			case UnmapNotify:
				handle_unmap_event(&ev.xunmap);
				break;
			case DestroyNotify:
				handle_destroy_event(&ev.xdestroywindow);
				break;
			case ClientMessage:
				handle_client_message(&ev.xclient);
				break;
			case ColormapNotify:
				handle_colormap_change(&ev.xcolormap);
				break;
			case PropertyNotify:
				handle_property_change(&ev.xproperty);
				break;
			case EnterNotify:
				handle_enter_event(&ev.xcrossing);
				break;
			case Expose:
				handle_expose_event(&ev.xexpose);
				break;
#ifdef SHAPE
			default:
				if (shape && ev.type == shape_event)
				{
					handle_shape_change((XShapeEvent *)&ev);
				}
#endif
		}
	}
}

static void handle_key_press(XKeyEvent *e)
{
	KeySym key = XKeycodeToKeysym(dsply, e->keycode, 0);
	switch (key)
	{
		case KEY_CYCLEPREV:
			cycle_previous();
			break;
		case KEY_CYCLENEXT:
			cycle_next();
			break;
		case KEY_FULLSCREEN:
			toggle_fullscreen(focused_client);
			break;
		case KEY_TOGGLEZ:
			raise_lower(focused_client);
			break;
	}
}

/* Someone clicked a button. If it was on the root, we get the click
 * by default. If it's on a window frame, we get it as well. If it's
 * on a client window, it may still fall through to us if the client
 * doesn't select for mouse-click events. */

static void handle_button_press(XButtonEvent *e)
{
	Client *c;

	if (e->state & MODIFIER)
	{
		if (focused_client != NULL && focused_client != fullscreen_client)
		{
			resize(focused_client, e->x_root, e->y_root);
		}
		else
		{
			// pass event on
			XAllowEvents(dsply, ReplayPointer, CurrentTime);
		}
	}
	else if (e->window == root)
	{
#ifdef DEBUG
		dump_clients();
#endif
		if (e->button == Button3)
		{
			rclick_root();
		}
	}
	else if (e->window == taskbar)
	{
		switch (e->button)
		{
			case Button1: // left mouse button
				lclick_taskbar(e->x);
				break;
			case Button3: // right mouse button
				rclick_taskbar(e->x);
				break;
			case Button4: // mouse wheel up
				cycle_previous();
				break;
			case Button5: // mouse wheel down
				cycle_next();
				break;
		}
	}
	else
	{
		// pass event on
		XAllowEvents(dsply, ReplayPointer, CurrentTime);
		if (e->button == Button1)
		{
			c = find_client(e->window, FRAME);
			if (c != NULL)
			{
				// click-to-focus
				check_focus(c);
				if (e->y < BARHEIGHT() && c != fullscreen_client)
				{
					handle_windowbar_click(e, c);
				}
			}
		}
		else if (e->button == Button3)
		{
			rclick_root();
		}
	}
}

static void handle_windowbar_click(XButtonEvent *e, Client *c)
{
	static Client * first_click_c;
	static Time first_click_time;
	unsigned int in_box, in_box_down, in_box_up;
	int win_ypos;
	XEvent ev;

	in_box_down = box_clicked(c, e->x);
	if (in_box_down <= 2)
	{
		if (!grab(root, MouseMask, None))
		{
			return;
		}

		XGrabServer(dsply);

		in_box = 1;

		draw_button(c, &text_gc, &depressed_gc, in_box_down);

		do
		{
			XMaskEvent(dsply, MouseMask, &ev);
			in_box_up = box_clicked(c, ev.xbutton.x - (c->x + DEF_BORDERWIDTH));
			win_ypos = (ev.xbutton.y - c->y) + BARHEIGHT();
			if (ev.type == MotionNotify)
			{
				if ((win_ypos <= BARHEIGHT()) && (win_ypos >= DEF_BORDERWIDTH) && (in_box_up == in_box_down))
				{
					in_box = 1;
					draw_button(c, &text_gc, &depressed_gc, in_box_down);
				}
				else
				{
					in_box = 0;
					draw_button(c, &text_gc, &active_gc, in_box_down);
				}
			}
		}
		while (ev.type != ButtonRelease);
		draw_button(c, &text_gc, &active_gc, in_box_down);

		XUngrabServer(dsply);
		ungrab();
		if (in_box)
		{
			switch (in_box_up)
			{
				case 0:
					send_wm_delete(c);
					break;
				case 1:
					raise_lower(c);
					break;
				case 2:
					hide(c);
					break;
			}
		}
	}
	else if (in_box_down != UINT_MAX)
	{
		if (first_click_c == c && (e->time - first_click_time) < DEF_DBLCLKTIME)
		{
			raise_lower(c);
			first_click_c = NULL; // prevent 3rd clicks counting as double clicks
		}
		else
		{
			first_click_c = c;
		}
		first_click_time = e->time;
		move(c);
	}
}

/* Return which button was clicked - this is a multiple of BARHEIGHT()
 * from the right hand side. We only care about 0, 1 and 2. */

static unsigned int box_clicked(Client *c, int x)
{
	int pix_from_right = c->width - x;
	if (pix_from_right < 0)
	{
		return UINT_MAX; // outside window
	}
	else
	{
		return (pix_from_right / (BARHEIGHT() - DEF_BORDERWIDTH));
	}
}

static void draw_button(Client *c, GC *detail_gc, GC *background_gc, unsigned int which_box)
{
	switch (which_box)
	{
		case 0:
			draw_close_button(c, detail_gc, background_gc);
			break;
		case 1:
			draw_toggledepth_button(c, detail_gc, background_gc);
			break;
		case 2:
			draw_hide_button(c, detail_gc, background_gc);
			break;
	}
}

/* Because we are redirecting the root window, we get ConfigureRequest
 * events from both clients we're handling and ones that we aren't.
 * For clients we manage, we need to fiddle with the frame and the
 * client window, and for unmanaged windows we have to pass along
 * everything unchanged. Thankfully, we can reuse (a) the
 * XWindowChanges struct and (b) the code to configure the client
 * window in both cases.
 *
 * Most of the assignments here are going to be garbage, but only the
 * ones that are masked in by e->value_mask will be looked at by the X
 * server.
 *
 * We ignore managed clients that want their z-order changed and
 * managed fullscreen clients that want their size and/or position
 * changed (except to update their size and/or position for when
 * fullscreen mode is toggled off). From what I can remember, clients
 * are supposed to have been written so that they are aware that their
 * requirements may not be met by the window manager. */

static void handle_configure_request(XConfigureRequestEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	XWindowChanges wc;

	if (fullscreen_client != NULL && c == fullscreen_client)
	{
		if (e->value_mask & CWX)
		{
			fs_prevdims.x = e->x;
		}
		if (e->value_mask & CWY)
		{
			fs_prevdims.y = e->y;
		}
		if (e->value_mask & CWWidth)
		{
			fs_prevdims.width = e->width;
		}
		if (e->value_mask & CWHeight)
		{
			fs_prevdims.height = e->height;
		}
		return;
	}

	if (c != NULL)
	{
		gravitate(c, REMOVE_GRAVITY);
		if (e->value_mask & CWX)
		{
			c->x = e->x;
		}
		if (e->value_mask & CWY)
		{
			c->y = e->y;
		}
		if (e->value_mask & CWWidth)
		{
			c->width = e->width;
		}
		if (e->value_mask & CWHeight)
		{
			c->height = e->height;
		}
		refix_position(c, e);
		gravitate(c, APPLY_GRAVITY);
		// configure the frame
		wc.x = c->x;
		wc.y = c->y - BARHEIGHT();
		wc.width = c->width;
		wc.height = c->height + BARHEIGHT();
		wc.border_width = DEF_BORDERWIDTH;
		//wc.sibling = e->above;
		//wc.stack_mode = e->detail;
		XConfigureWindow(dsply, c->frame, e->value_mask, &wc);
#ifdef SHAPE
		if (e->value_mask & (CWWidth|CWHeight))
		{
			set_shape(c);
		}
#endif
		send_config(c);
		// start setting up the next call
		wc.x = 0;
		wc.y = BARHEIGHT();
	}
	else
	{
		wc.x = e->x;
		wc.y = e->y;
	}

	wc.width = e->width;
	wc.height = e->height;
	//wc.sibling = e->above;
	//wc.stack_mode = e->detail;
	XConfigureWindow(dsply, e->window, e->value_mask, &wc);
}

/* Two possibilities if a client is asking to be mapped. One is that
 * it's a new window, so we handle that if it isn't in our clients
 * list anywhere. The other is that it already exists and wants to
 * de-iconify, which is simple to take care of. */

static void handle_map_request(XMapRequestEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	if (c != NULL)
	{
		unhide(c);
	}
	else
	{
		make_new_client(e->window);
	}
}

/* See windowlab.h for the intro to this one. If this is a window we
 * unmapped ourselves, decrement c->ignore_unmap and casually go on as
 * if nothing had happened. If the window unmapped itself from under
 * our feet, however, get rid of it.
 *
 * If you spend a lot of time with -DDEBUG on, you'll realize that
 * because most clients unmap and destroy themselves at once, they're
 * gone before we even get the Unmap event, never mind the Destroy
 * one. This will necessitate some extra caution in remove_client.
 *
 * Personally, I think that if Map events are intercepted, Unmap
 * events should be intercepted too. No use arguing with a standard
 * that's almost as old as I am though. :-( */

static void handle_unmap_event(XUnmapEvent *e)
{
	Client *c = find_client(e->window, WINDOW);

	if (c != NULL)
	{
		if (c->ignore_unmap)
		{
			c->ignore_unmap--;
		}
		else
		{
			remove_client(c, WITHDRAW);
		}
	}
}

/* This happens when a window is iconified and destroys itself. An
 * Unmap event wouldn't happen in that case because the window is
 * already unmapped. */

static void handle_destroy_event(XDestroyWindowEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	if (c != NULL)
	{
		remove_client(c, WITHDRAW);
	}
}

/* If a client wants to iconify itself (boo! hiss!) it must send a
 * special kind of ClientMessage. We might set up other handlers here
 * but there's nothing else required by the ICCCM. */

static void handle_client_message(XClientMessageEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	if (c != NULL && e->message_type == wm_change_state && e->format == 32 && e->data.l[0] == IconicState)
	{
		hide(c);
	}
}

/* All that we have cached is the name and the size hints, so we only
 * have to check for those here. A change in the name means we have to
 * immediately wipe out the old name and redraw; size hints only get
 * used when we need them. */

static void handle_property_change(XPropertyEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	long dummy;

	if (c != NULL)
	{
		switch (e->atom)
		{
			case XA_WM_NAME:
				if (c->name)
				{
					XFree(c->name);
					c->name = NULL;
				}
				XFetchName(dsply, c->window, &c->name);
				redraw(c);
				redraw_taskbar();
				break;
			case XA_WM_NORMAL_HINTS:
				XGetWMNormalHints(dsply, c->window, c->size, &dummy);
				break;
		}
	}
}

/* X's default focus policy is follows-mouse, but we have to set it
 * anyway because some sloppily written clients assume that (a) they
 * can set the focus whenever they want or (b) that they don't have
 * the focus unless the keyboard is grabbed to them. OTOH it does
 * allow us to keep the previous focus when pointing at the root,
 * which is nice.
 *
 * We also implement a colormap-follows-mouse policy here. That, on
 * the third hand, is *not* X's default. */

static void handle_enter_event(XCrossingEvent *e)
{
	Client *c = NULL;
	if (e->window == taskbar)
	{
		in_taskbar = 1;
		if (showing_taskbar == 0)
		{
			showing_taskbar = 1;
			redraw_taskbar();
		}
	}
	else
	{
		in_taskbar = 0;
		if (fullscreen_client != NULL)
		{
			if (showing_taskbar == 1)
			{
				showing_taskbar = 0;
				redraw_taskbar();
			}
		}
		else // no fullscreen client
		{
			if (showing_taskbar == 0)
			{
				showing_taskbar = 1;
				redraw_taskbar();
			}
		}

		c = find_client(e->window, FRAME);
		if (c != NULL)
		{
			XGrabButton(dsply, AnyButton, AnyModifier, c->frame, False, ButtonMask, GrabModeSync, GrabModeSync, None, None);
		}
	}
}

/* Here's part 2 of our colormap policy: when a client installs a new
 * colormap on itself, set the display's colormap to that. Arguably,
 * this is bad, because we should only set the colormap if that client
 * has the focus. However, clients don't usually set colormaps at
 * random when you're not interacting with them, so I think we're
 * safe. If you have an 8-bit display and this doesn't work for you,
 * by all means yell at me, but very few people have 8-bit displays
 * these days. */

static void handle_colormap_change(XColormapEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	//if (c != NULL && e->c_new) // use c_new for c++
	if (c != NULL && e->new)
	{
		c->cmap = e->colormap;
		XInstallColormap(dsply, c->cmap);
	}
}

/* If we were covered by multiple windows, we will usually get
 * multiple expose events, so ignore them unless e->count (the number
 * of outstanding exposes) is zero. */

static void handle_expose_event(XExposeEvent *e)
{
	if (e->window == taskbar)
	{
		if (e->count == 0)
		{
			redraw_taskbar();
		}
	}
	else
	{
		Client *c = find_client(e->window, FRAME);
		if (c != NULL && e->count == 0)
		{
			redraw(c);
		}
	}
}

#ifdef SHAPE
static void handle_shape_change(XShapeEvent *e)
{
	Client *c = find_client(e->window, WINDOW);
	if (c != NULL)
	{
		set_shape(c);
	}
}
#endif

/* interruptibleXNextEvent() was originally taken from Blender's source code
 * and came with the following copyright notice: */

/* Copyright (c) Mark J. Kilgard, 1994, 1995, 1996. */

/* This program is freely distributable without licensing fees
 * and is provided without guarantee or warrantee expressed or
 * implied. This program is -not- in the public domain. */

/* Unlike XNextEvent, if a signal arrives, interruptibleXNextEvent will
 * return zero. */

static int interruptible_XNextEvent(XEvent *event)
{
	fd_set fds;
	int rc;
	int dsply_fd = ConnectionNumber(dsply);
	for (;;)
	{
		if (XPending(dsply))
		{
			XNextEvent(dsply, event);
			return 1;
		}
		FD_ZERO(&fds);
		FD_SET(dsply_fd, &fds);
		rc = select(dsply_fd + 1, &fds, NULL, NULL, NULL);
		if (rc < 0)
		{
			if (errno == EINTR)
			{
				return 0;
			}
			return 1;
		}
	}
}

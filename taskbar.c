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

#include "windowlab.h"

static void draw_menubar(void);
static unsigned int update_menuitem(unsigned int);
static void draw_menuitem(unsigned int, unsigned int);

Window taskbar;
#ifdef XFT
XftDraw *tbxftdraw;
#endif

void make_taskbar(void)
{
	XSetWindowAttributes pattr;

	pattr.override_redirect = True;
	pattr.background_pixel = empty_col.pixel;
	pattr.border_pixel = border_col.pixel;
	pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	taskbar = XCreateWindow(dpy, root,
		0 - DEF_BORDERWIDTH, 0 - DEF_BORDERWIDTH, DisplayWidth(dpy, screen), BARHEIGHT() - DEF_BORDERWIDTH, DEF_BORDERWIDTH,
		DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);

	XMapWindow(dpy, taskbar);

#ifdef XFT
	tbxftdraw = XftDrawCreate(dpy, (Drawable) taskbar,
		DefaultVisual(dpy, DefaultScreen(dpy)),
		DefaultColormap(dpy, DefaultScreen(dpy)));
#endif
}

void cycle_previous()
{
	Client *c = last_focused_client;
	Client *original_c = c;
	if (head_client != NULL && head_client->next != NULL) // at least 2 windows exist
	{
		if (c == NULL) c = head_client;
		if (c == head_client) original_c = NULL;
		do
		{
			if (c->next == NULL) c = head_client;
			else c = c->next;
		}
		while (c->next != original_c);
		check_focus(c);
	}
}

void cycle_next()
{
	Client *c = last_focused_client;
	if (head_client != NULL && head_client->next != NULL) // at least 2 windows exist
	{
		if (c == NULL || c->next == NULL) c = head_client;
		else c = c->next;
		check_focus(c);
	}
}

void lclick_taskbar(unsigned int x)
{
	XEvent ev;
	int boundx, boundy, boundw, boundh, mousex, mousey;
	Window constraint_win;
	XSetWindowAttributes pattr;

	float button_width;
	unsigned int button_clicked, old_button_clicked, i;
	Client *c, *exposed_c;
	if (head_client)
	{
		get_mouse_position(&mousex, &mousey);

		boundx = 0;
		boundy = 0;
		boundw = DisplayWidth(dpy, screen);
		boundh = BARHEIGHT();

		constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
		XMapWindow(dpy, constraint_win);

		if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, None, CurrentTime) == GrabSuccess))
		{
			return;
		}

		button_width = get_button_width();

		button_clicked = (unsigned int)(x / button_width);
		for (i = 0, c = head_client; i < button_clicked; i++) c = c->next;
		check_focus(c);
		raise_lower(c);

		do
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
					old_button_clicked = button_clicked;
					button_clicked = (unsigned int)(ev.xmotion.x / button_width);
					if (button_clicked != old_button_clicked)
					{
						for (i = 0, c = head_client; i < button_clicked; i++) c = c->next;
						check_focus(c);
						raise_lower(c);
					}
					break;
			}
		}
		while (ev.type != ButtonPress && ev.type != ButtonRelease);

		XUnmapWindow(dpy, constraint_win);
		XDestroyWindow(dpy, constraint_win);
		ungrab();
	}
}

void rclick_taskbar(unsigned int x)
{
	XEvent ev;
	int boundx, boundy, boundw, boundh, mousex, mousey;
	unsigned int current_item = UINT_MAX;
	Window constraint_win;
	XSetWindowAttributes pattr;

	get_mouse_position(&mousex, &mousey);

	boundx = 0;
	boundy = 0;
	boundw = DisplayWidth(dpy, screen);
	boundh = BARHEIGHT();

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, None, CurrentTime) == GrabSuccess))
	{
		return;
	}
	draw_menubar();
	update_menuitem(UINT_MAX); // force initial highlight
	current_item = update_menuitem(x);
	do
	{
		XMaskEvent(dpy, MouseMask, &ev);
		switch (ev.type)
		{
			case MotionNotify:
				current_item = update_menuitem(ev.xmotion.x);
				break;
			case ButtonRelease:
				if (current_item != UINT_MAX)
				{
					fork_exec(menuitems[current_item].command);
				}
				break;
		}
	}
	while (ev.type != ButtonPress && ev.type != ButtonRelease);

	redraw_taskbar();
	XUnmapWindow(dpy, constraint_win);
	XDestroyWindow(dpy, constraint_win);
	ungrab();
}

void rclick_root(void)
{
	XEvent ev;
	if (!grab(root, MouseMask, None))
	{
		return;
	}
	draw_menubar();
	do
	{
		XMaskEvent(dpy, MouseMask, &ev);
		if (ev.type == MotionNotify)
		{
			if (ev.xmotion.y < BARHEIGHT())
			{
				ungrab();
				rclick_taskbar(ev.xmotion.x);
				return;
			}
		}
	}
	while (ev.type != ButtonRelease);

	redraw_taskbar();
	ungrab();
}

void redraw_taskbar(void)
{
	unsigned int i, button_startx, button_iwidth;
	float button_width;
	Client *c;

	button_width = get_button_width();
	XClearWindow(dpy, taskbar);

	if (!showing_taskbar) return;

	for (c = head_client, i=0; c; c = c->next, i++)
	{
		button_startx = (unsigned int)(i * button_width);
		button_iwidth = (unsigned int)(((i + 1) * button_width) - button_startx);
		if (button_startx != 0)
		{
			XDrawLine(dpy, taskbar, border_gc, button_startx - 1, 0, button_startx - 1, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		if (c == last_focused_client)
		{
			XFillRectangle(dpy, taskbar, active_gc, button_startx, 0, button_iwidth, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		else
		{
			XFillRectangle(dpy, taskbar, inactive_gc, button_startx, 0, button_iwidth, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		if (!c->trans && c->name)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_detail,
				xftfont, button_startx + SPACE, SPACE + xftfont->ascent,
				c->name, strlen(c->name));
#else
			XDrawString(dpy, taskbar, text_gc,
				button_startx + SPACE, SPACE + font->ascent,
				c->name, strlen(c->name));
#endif
		}
	}
}

void draw_menubar(void)
{
	unsigned int i, dw;
	dw = DisplayWidth(dpy, screen);
	XFillRectangle(dpy, taskbar, menu_gc, 0, 0, dw, BARHEIGHT() - DEF_BORDERWIDTH);

	for (i = 0; i < num_menuitems; i++)
	{
		if (menuitems[i].label && menuitems[i].command)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_detail, xftfont,
				menuitems[i].x + (SPACE * 2), xftfont->ascent + SPACE,
				menuitems[i].label, strlen(menuitems[i].label));
#else
			XDrawString(dpy, taskbar, text_gc,
				menuitems[i].x + (SPACE * 2), font->ascent + SPACE,
				menuitems[i].label, strlen(menuitems[i].label));
#endif
		}
	}
}

unsigned int update_menuitem(unsigned int mousex)
{
	static unsigned int last_item; // retain value from last call
	unsigned int i;
	if (mousex == UINT_MAX) // entered function to set last_item
	{
		last_item = num_menuitems;
		return UINT_MAX;
	}
	for (i = 0; i < num_menuitems; i++)
	{
		if ((mousex >= menuitems[i].x) && (mousex <= (menuitems[i].x + menuitems[i].width)))
		{
			break;
		}
	}

	if (i != last_item) // don't redraw if same
	{
		if (last_item != num_menuitems)
		{
			draw_menuitem(last_item, 0);
		}
		if (i != num_menuitems)
		{
			draw_menuitem(i, 1);
		}
		last_item = i; // set to new menu item
	}

	if (i != num_menuitems)
	{
		return i;
	}
	else // no item selected
	{
		return UINT_MAX;
	}
}

void draw_menuitem(unsigned int index, unsigned int active)
{
	if (active)
	{
		XFillRectangle(dpy, taskbar, selected_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT() - DEF_BORDERWIDTH);
	}
	else
	{
		XFillRectangle(dpy, taskbar, menu_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT() - DEF_BORDERWIDTH);
	}
#ifdef XFT
	XftDrawString8(tbxftdraw, &xft_detail, xftfont,
		menuitems[index].x + (SPACE * 2), xftfont->ascent + SPACE,
		menuitems[index].label, strlen(menuitems[index].label));
#else
	XDrawString(dpy, taskbar, text_gc,
		menuitems[index].x + (SPACE * 2), font->ascent + SPACE,
		menuitems[index].label, strlen(menuitems[index].label));
#endif
}

float get_button_width(void)
{
	unsigned int nwins = 0;
	Client *c = head_client;
	while (c != NULL)
	{
		nwins++;
		c = c->next;
	}
	return ((float)(DisplayWidth(dpy, screen) + DEF_BORDERWIDTH)) / nwins;
}



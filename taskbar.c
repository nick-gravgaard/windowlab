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
static int update_menuitem(int);
static void draw_menuitem(int, int);

Window taskbar;
#ifdef XFT
XftDraw *tbxftdraw;
#endif

void make_taskbar(void)
{
	XSetWindowAttributes pattr;

	pattr.override_redirect = True;
	pattr.background_pixel = fc.pixel;
	pattr.border_pixel = bd.pixel;
	pattr.event_mask = ChildMask|ButtonPressMask|ExposureMask|EnterWindowMask;
	taskbar = XCreateWindow(dpy, root,
		0 - DEF_BW, 0 - DEF_BW, DisplayWidth(dpy, screen), BARHEIGHT(), DEF_BW,
		DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
		CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);

	XMapWindow(dpy, taskbar);

#ifdef XFT
	tbxftdraw = XftDrawCreate(dpy, (Drawable) taskbar,
		DefaultVisual(dpy, DefaultScreen(dpy)),
		DefaultColormap(dpy, DefaultScreen(dpy)));
#endif
}

void click_taskbar(unsigned int x)
{
	float button_width;
	unsigned int button_clicked, i;
	Client *c;
	if (head_client)
	{
		c = head_client;
		button_width = get_button_width();
		button_clicked = (unsigned int)(x / button_width);
		for (i = 0; i < button_clicked; i++)
		{
			if (c->iconic)
			{
				button_clicked++;
			}
			c = c->next;
		}
		if (!c->iconic)
		{
			check_focus(c);
			raise_lower(c);
		}
	}
}

void rclick_taskbar(void)
{
	XEvent ev;

	int boundx, boundy, boundw, boundh, mousex, mousey, dw, current_item = -1;
	Window constraint_win;
	XSetWindowAttributes pattr;

	dw = DisplayWidth(dpy, screen);
	get_mouse_position(&mousex, &mousey);

	boundx = 0;
	boundw = dw;
	boundy = 0;
	boundh = BARHEIGHT();

	constraint_win = XCreateWindow(dpy, root, boundx, boundy, boundw, boundh, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dpy, constraint_win);

	if (!(XGrabPointer(dpy, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, None, CurrentTime) == GrabSuccess))
	{
		return;
	}
	draw_menubar();
	update_menuitem(0xffff); // force initial highlight
	do
	{
		XMaskEvent(dpy, MouseMask, &ev);
		switch (ev.type)
		{
			case MotionNotify:
				current_item = update_menuitem(ev.xmotion.x);
				break;
			case ButtonRelease:
				if (current_item != -1)
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
				rclick_taskbar();
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

	for (c = head_client, i=0; c; c = c->next, i++)
	{
		if (c->iconic)
		{
			i--;
			continue;
		}
		button_startx = (unsigned int)(i * button_width);
		button_iwidth = (unsigned int)(((i+1) * button_width) - button_startx);
		if (button_startx != 0)
		{
			XDrawLine(dpy, taskbar, border_gc, button_startx, 0, button_startx, BARHEIGHT());
		}
		if (c == last_focused_client)
		{
			XFillRectangle(dpy, taskbar, active_gc, button_startx+1, 0, button_iwidth-2, BARHEIGHT());
		}
		else
		{
			XFillRectangle(dpy, taskbar, inactive_gc, button_startx+1, 0, button_iwidth-2, BARHEIGHT());
		}
		if (!c->trans && c->name)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_fg,
				xftfont, button_startx + SPACE + (DEF_BW/2), SPACE + xftfont->ascent,
				c->name, strlen(c->name));
#else
			XDrawString(dpy, taskbar, string_gc,
				button_startx + SPACE + (DEF_BW/2), SPACE + font->ascent,
				c->name, strlen(c->name));
#endif
		}
	}
}

void draw_menubar(void)
{
	unsigned int i, dw;
	dw = DisplayWidth(dpy, screen);
	XFillRectangle(dpy, taskbar, menubar_gc, 0, 0, dw, BARHEIGHT());

	for (i = 0; i < num_menuitems; i++)
	{
		if (menuitems[i].label && menuitems[i].command)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_fg, xftfont,
				menuitems[i].x + (SPACE * 2), xftfont->ascent + SPACE,
				menuitems[i].label, strlen(menuitems[i].label));
#else
			XDrawString(dpy, taskbar, string_gc,
				menuitems[i].x + (SPACE * 2), font->ascent + SPACE,
				menuitems[i].label, strlen(menuitems[i].label));
#endif
		}
	}
}

int update_menuitem(int mousex)
{
	static unsigned int last_item; // retain value from last call
	unsigned int i;
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
		return -1;
	}
}

void draw_menuitem(int index, int active)
{
	if (active)
	{
		XFillRectangle(dpy, taskbar, menusel_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT());
	}
	else
	{
		XFillRectangle(dpy, taskbar, menubar_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT());
	}
#ifdef XFT
	XftDrawString8(tbxftdraw, &xft_fg, xftfont,
		menuitems[index].x + (SPACE * 2), xftfont->ascent + SPACE,
		menuitems[index].label, strlen(menuitems[index].label));
#else
	XDrawString(dpy, taskbar, string_gc,
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
		if (!c->iconic)
		{
			nwins++;
		}
		c = c->next;
	}
	return (((float)DisplayWidth(dpy, screen)) / nwins);
}





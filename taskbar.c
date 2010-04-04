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

static void draw_menubar(void);
static unsigned int update_menuitem(int);
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
	taskbar = XCreateWindow(dsply, root, 0 - DEF_BORDERWIDTH, 0 - DEF_BORDERWIDTH, DisplayWidth(dsply, screen), BARHEIGHT() - DEF_BORDERWIDTH, DEF_BORDERWIDTH, DefaultDepth(dsply, screen), CopyFromParent, DefaultVisual(dsply, screen), CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);

	XMapWindow(dsply, taskbar);

#ifdef XFT
	tbxftdraw = XftDrawCreate(dsply, (Drawable) taskbar, DefaultVisual(dsply, DefaultScreen(dsply)), DefaultColormap(dsply, DefaultScreen(dsply)));
#endif
}

void remember_hidden(void)
{
	Client *c;
	for (c = head_client; c != NULL; c = c->next)
	{
		c->was_hidden = c->hidden;
	}
}

void forget_hidden(void)
{
	Client *c;
	for (c = head_client; c != NULL; c = c->next)
	{
		if (c == focused_client)
		{
			c->was_hidden = c->hidden;
		}
		else
		{
			c->was_hidden = 0;
		}
	}
}

void lclick_taskbutton(Client *old_c, Client *c)
{
	if (old_c != NULL)
	{
		if (old_c->was_hidden)
		{
			hide(old_c);
		}
	}

	if (c->hidden)
	{
		unhide(c);
	}
	else
	{
		if (c->was_hidden)
		{
			hide(c);
		}
		else
		{
			raise_lower(c);
		}
	}
	check_focus(c);
}

void lclick_taskbar(int x)
{
	XEvent ev;
	int mousex, mousey;
	Rect bounddims;
	Window constraint_win;
	XSetWindowAttributes pattr;

	float button_width;
	unsigned int button_clicked, old_button_clicked, i;
	Client *c, *exposed_c, *old_c;
	if (head_client != NULL)
	{
		remember_hidden();

		get_mouse_position(&mousex, &mousey);

		bounddims.x = 0;
		bounddims.y = 0;
		bounddims.width = DisplayWidth(dsply, screen);
		bounddims.height = BARHEIGHT();

		constraint_win = XCreateWindow(dsply, root, bounddims.x, bounddims.y, bounddims.width, bounddims.height, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
		XMapWindow(dsply, constraint_win);

		if (!(XGrabPointer(dsply, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, None, CurrentTime) == GrabSuccess))
		{
			XDestroyWindow(dsply, constraint_win);
			return;
		}

		button_width = get_button_width();

		button_clicked = (unsigned int)(x / button_width);
		for (i = 0, c = head_client; i < button_clicked; i++)
		{
			c = c->next;
		}

		lclick_taskbutton(NULL, c);

		do
		{
			XMaskEvent(dsply, ExposureMask|MouseMask|KeyMask, &ev);
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
						old_c = c;
						for (i = 0, c = head_client; i < button_clicked; i++)
						{
							c = c->next;
						}
						lclick_taskbutton(old_c, c);
					}
					break;
				case KeyPress:
					XPutBackEvent(dsply, &ev);
					break;
			}
		}
		while (ev.type != ButtonPress && ev.type != ButtonRelease && ev.type != KeyPress);

		XUnmapWindow(dsply, constraint_win);
		XDestroyWindow(dsply, constraint_win);
		ungrab();

		forget_hidden();
	}
}

void rclick_taskbar(int x)
{
	XEvent ev;
	int mousex, mousey;
	Rect bounddims;
	unsigned int current_item = UINT_MAX;
	Window constraint_win;
	XSetWindowAttributes pattr;

	get_mouse_position(&mousex, &mousey);

	bounddims.x = 0;
	bounddims.y = 0;
	bounddims.width = DisplayWidth(dsply, screen);
	bounddims.height = BARHEIGHT();

	constraint_win = XCreateWindow(dsply, root, bounddims.x, bounddims.y, bounddims.width, bounddims.height, 0, CopyFromParent, InputOnly, CopyFromParent, 0, &pattr);
	XMapWindow(dsply, constraint_win);

	if (!(XGrabPointer(dsply, root, False, MouseMask, GrabModeAsync, GrabModeAsync, constraint_win, None, CurrentTime) == GrabSuccess))
	{
		XDestroyWindow(dsply, constraint_win);
		return;
	}
	draw_menubar();
	update_menuitem(INT_MAX); // force initial highlight
	current_item = update_menuitem(x);
	do
	{
		XMaskEvent(dsply, MouseMask|KeyMask, &ev);
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
			case KeyPress:
				XPutBackEvent(dsply, &ev);
				break;
		}
	}
	while (ev.type != ButtonPress && ev.type != ButtonRelease && ev.type != KeyPress);

	redraw_taskbar();
	XUnmapWindow(dsply, constraint_win);
	XDestroyWindow(dsply, constraint_win);
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
		XMaskEvent(dsply, MouseMask|KeyMask, &ev);
		switch (ev.type)
		{
			case MotionNotify:
				if (ev.xmotion.y < BARHEIGHT())
				{
					ungrab();
					rclick_taskbar(ev.xmotion.x);
					return;
				}
				break;
			case KeyPress:
				XPutBackEvent(dsply, &ev);
				break;
		}
	}
	while (ev.type != ButtonRelease && ev.type != KeyPress);

	redraw_taskbar();
	ungrab();
}

void redraw_taskbar(void)
{
	unsigned int i;
	int button_startx, button_iwidth;
	float button_width;
	Client *c;

	button_width = get_button_width();
	XClearWindow(dsply, taskbar);

	if (showing_taskbar == 0)
	{
		return;
	}

	for (c = head_client, i = 0; c != NULL; c = c->next, i++)
	{
		button_startx = (int)(i * button_width);
		button_iwidth = (unsigned int)(((i + 1) * button_width) - button_startx);
		if (button_startx != 0)
		{
			XDrawLine(dsply, taskbar, border_gc, button_startx - 1, 0, button_startx - 1, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		if (c == focused_client)
		{
			XFillRectangle(dsply, taskbar, active_gc, button_startx, 0, button_iwidth, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		else
		{
			XFillRectangle(dsply, taskbar, inactive_gc, button_startx, 0, button_iwidth, BARHEIGHT() - DEF_BORDERWIDTH);
		}
		if (!c->trans && c->name != NULL)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_detail, xftfont, button_startx + SPACE, SPACE + xftfont->ascent, (unsigned char *)c->name, strlen(c->name));
#else
			XDrawString(dsply, taskbar, text_gc, button_startx + SPACE, SPACE + font->ascent, c->name, strlen(c->name));
#endif
		}
	}
}

void draw_menubar(void)
{
	unsigned int i, dw;
	dw = DisplayWidth(dsply, screen);
	XFillRectangle(dsply, taskbar, menu_gc, 0, 0, dw, BARHEIGHT() - DEF_BORDERWIDTH);

	for (i = 0; i < num_menuitems; i++)
	{
		if (menuitems[i].label && menuitems[i].command)
		{
#ifdef XFT
			XftDrawString8(tbxftdraw, &xft_detail, xftfont, menuitems[i].x + (SPACE * 2), xftfont->ascent + SPACE, (unsigned char *)menuitems[i].label, strlen(menuitems[i].label));
#else
			XDrawString(dsply, taskbar, text_gc, menuitems[i].x + (SPACE * 2), font->ascent + SPACE, menuitems[i].label, strlen(menuitems[i].label));
#endif
		}
	}
}

unsigned int update_menuitem(int mousex)
{
	static unsigned int last_item; // retain value from last call
	unsigned int i;
	if (mousex == INT_MAX) // entered function to set last_item
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
		XFillRectangle(dsply, taskbar, selected_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT() - DEF_BORDERWIDTH);
	}
	else
	{
		XFillRectangle(dsply, taskbar, menu_gc, menuitems[index].x, 0, menuitems[index].width, BARHEIGHT() - DEF_BORDERWIDTH);
	}
#ifdef XFT
	XftDrawString8(tbxftdraw, &xft_detail, xftfont, menuitems[index].x + (SPACE * 2), xftfont->ascent + SPACE, (unsigned char *)menuitems[index].label, strlen(menuitems[index].label));
#else
	XDrawString(dsply, taskbar, text_gc, menuitems[index].x + (SPACE * 2), font->ascent + SPACE, menuitems[index].label, strlen(menuitems[index].label));
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
	return ((float)(DisplayWidth(dsply, screen) + DEF_BORDERWIDTH)) / nwins;
}

void cycle_previous(void)
{
	Client *c = focused_client;
	Client *original_c = c;
	if (head_client != NULL && head_client->next != NULL) // at least 2 windows exist
	{
		if (c == NULL)
		{
			c = head_client;
		}
		if (c == head_client)
		{
			original_c = NULL;
		}
		do
		{
			if (c->next == NULL)
			{
				c = head_client;
			}
			else
			{
				c = c->next;
			}
		}
		while (c->next != original_c);
		lclick_taskbutton(NULL, c);
	}
}

void cycle_next(void)
{
	Client *c = focused_client;
	if (head_client != NULL && head_client->next != NULL) // at least 2 windows exist
	{
		if (c == NULL || c->next == NULL)
		{
			c = head_client;
		}
		else c = c->next;
		lclick_taskbutton(NULL, c);
	}
}

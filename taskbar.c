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
#include <X11/Xmd.h>

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
			c = c->next;
		}
		check_focus(c);
		raise_lower(c);
	}
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
		button_startx = (unsigned int)(i * button_width);
		button_iwidth = (unsigned int)(((i+1) * button_width) - button_startx);
		if (c != head_client)
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

float get_button_width(void)
{
	unsigned int nwins = 0;
	Client *c = head_client;
	while (c != NULL)
	{
		nwins++;
		c = c->next;
	}
	return (((float)DisplayWidth(dpy, screen)) / nwins);
}



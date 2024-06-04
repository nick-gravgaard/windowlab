# WindowLab

## Rethinking the window manager

Copyright (c) 2001 Nick Gravgaard

![WindowLab screenshot](screenshot.png)


> ... thank you for creating such a wonderful WM as WindowLab. It's so extremely clean and simple and fast and easy, just what I was looking for.
>
> ... I must say, I'm very impressed. It has a lot of new implementations of features which tend to be rather stalely implememented in other 'new' windowmanagers.
>
> Your window manager rocks ...
>
> Having worked with it a little bit, I can honestly say that WindowLab is a great application. It's fast and responsive, and building the menu is simple, fast, and very straightforward. Congratulations on creating such an excellent product!
>
> I just downloaded, compiled and installed your excellent window manager. Its cleanness, innovation and attractive appearance blew me away. When I switched to linux two years ago, this is what I imagined it would be like: fast and clean.


## Description

WindowLab is a small and simple window manager of novel design.

It has a click-to-focus but not raise-on-focus policy, a window resizing mechanism that allows one or many edges of a window to be changed in one action, and an innovative menubar that shares the same part of the screen as the taskbar. Window titlebars are prevented from going off the edge of the screen by constraining the mouse pointer, and when appropriate the pointer is also constrained to the taskbar/menubar in order to make target menu items easier to hit.


## Advantages

* It allows the focused window to be below other windows that you still need to look at (click-to-focus but not raise-on-focus) without using a convoluted Windows style always-on-top mode
* One or many edges of a window can be changed in one action without having to click on thin window borders. As with much in WindowLab this allows faster use with less accuracy
* It's very quick and easy to launch an application from the menu - it's easier to slam the pointer to the top of the screen and then make small adjustments to hit the target then it is to use a Windows style hierarchical "start" menu or a Blackbox style popup menu, and by constraining the pointer to the menu bar you can be less accurate (and thus faster) because once the pointer is in the menu bar you don't have to worry about vertical movements
* You can quickly access a window and bring it to the front by clicking on it in the taskbar - this solves the only problem with the click-to-focus but not raise-on-focus model - having to slide partially obscured windows around to get to their toggle-depth buttons
* When you have many windows open and lose track of which window you want next you can click on a taskbar item, and if it's not the right one, slide the pointer over the other items in the taskbar (with the mouse button still depressed) to see the other windows. As with the menubar, the pointer's constrained to the taskbar so that you can make faster and less careful mouse movements. With many windows open this is faster than CoolSwitch (alt-tabbing) in Windows (although WindowLab does have a similar keyboard shortcut in alt-tab/alt-q) and some Mac OS X users have told me that it beats Exposé too
* Constraining windows titlebars to the screen makes it feel snappier and more responsive - try flinging a window around the screen. This also means that you'll never have only a tiny part of a window remaining on screen


## Latest version

The latest version of WindowLab is held at https://github.com/nick-gravgaard/windowlab


## Author

WindowLab was written by Nick Gravgaard (https://nick-gravgaard.com)


## Licence

WindowLab is Free Software and has been released under the GPL. Please see the LICENCE file for more information. The licence for aewm (which includes that of 9wm) from which this code has been derived is included at the end of this README document.


## Installation

Before compiling, check the default (DEF_foo) options in windowlab.h, and the defines in the Makefile. DEF_FONT is of particular interest; make sure that it is defined to something that exists on your system. You can turn -DSHAPE off if you don't have the Shape extension, and -DMWM_HINTS on if you have the Lesstif or Motif headers installed.

"make" will compile everything, and "make install" will install it.

To make WindowLab your default window manager, edit ~/.xinitrc (if you start X from the console by typing "startx") or ~/.xsession (if you start with a graphical login manager) and change the last line to "exec windowlab".

If you use FreeBSD, you can get WindowLab from /usr/ports/x11-wm/windowlab/


## Usage

WindowLab places a taskbar at the top of the screen and adds a titlebar to the top of each window. These titlebars consist of a draggable area, and three icons on the right hand side. When left clicked, these icons:

* hide the window
* toggle the window's Z order Amiga style (if it's not at the front, bring it to the front, otherwise send it to the back)
* close the window

Another way of toggling a window's Z order (depth) is by double left clicking on the draggable part of its titlebar.

Windows' titlebars are prevented from leaving the screen and cannot overlap the taskbar.

The taskbar should list all windows currently in use. Left clicking on a window's taskbar item will give that window focus and toggle its Z order (depth).

To resize the active window hold down alt and push against the window's edges with the left mouse button down.

If you right click outside a client window, WindowLab's taskbar becomes a menubar. Releasing the right mouse button over a selected menu item will start a corresponding external program. WindowLab will look in each of the following files in turn for definitions of the menu labels and commands:

* ~/.windowlab/windowlab.menurc
* ../etc/windowlab.menurc (from the directory containing the executable)
* /etc/X11/windowlab/windowlab.menurc

Each line in the menurc file should have the menu label, a colon, and then the corresponding command, eg:

The GIMP:gimp

New windows (that don't specify their location) are positioned according to the coordinates of the mouse - the top-left hand corner of a new window is set to the location of the mouse pointer (if necessary the window will be moved to ensure that all of it is on the screen).

WindowLab has the following keyboard controls. Hold down alt and press:

* tab to give focus to the previous window
* q to give focus to the next window
* F11 to toggle fullscreen mode on and off for non transient windows
* F12 to toggle the windows depth. This is the same as left clicking a window's middle icon


## Helping

If you find a bug please tell me about it (if you know how to fix it, even better). If you really like WindowLab, please tell others about it so that the number of potential users/contributors may increase.


## Design rationale

Before I started WindowLab, I'd been an Amiga user who had switched to Linux around 1998. To my mind, window managers (like text editors and file managers) are one of the most important parts of any system for the user, and I wasn't satisfied with those that already existed. At that time FVWM2 was the most commonly used Linux window manager but it was far too large for my tastes, and I suspected that one of the reasons for this was that it was designed to handle so many different options.

So I started work on my own window manager, basing it on aewm. The most important feature to my mind was to implement the Amiga's click-to-focus but not raise-on-focus behaviour, where a focused window is not necessarily in front of other windows. Realising that it can be hard to get to a windows buttons if it is behind another window, I added a simplified Windows 95 style taskbar so that users could easily access each window.

Another neat feature of the Amiga's GUI that had not been done before in an X window manager was the constraining of the pointer when the user tried to drag a window beyond the edge of the screen. In this way, WindowLab treats windows like they are Amiga screens in that a window's titlebar is constrained to the physical screen. At this point I started to think about the issue of launching programs from WindowLab. aewm's approach was that taskbars and launching files was not part of the window manager's job, and I respected the minimalist reasoning behind this, but since my constrained windows took it for granted that a taskbar existed at the top of the screen, I reasoned that the taskbar was best kept within the window manager. With the taskbar taking up as much space as a menu bar would, I realised that I could reuse space just like the Amiga had done by making a menu bar available when the right mouse button is held down. My twist was that rather than have drop down menus, my menubar would consist of "bang" menu items a little like the "Quick Launch" toolbar in Windows 98, and by keeping the menu one level deep, I could reuse the constrained mouse pointer trick to make target menu items easier to hit (infinitely tall according to Fitts's law).

I also wanted windows to be able to act like screens on the Amiga (where the title bar and client area can take up all of the screen), and this forced me to rule out any useful kind of draggable window borders. Earlier versions (up to 1.20) used an 8½ (from Plan 9) style resizing mechanism, but some users complained that it meant that each edge of the window changed even if only a small resize was intended. The mechanism that replaced it is completely original, and quicker and easier to use than traditional draggable borders (allowing one or many edges of a window to be changed in one action without the user having to click on thin window borders).


## Tips and tricks

* It is not the window manager's job to set the root window's pointer cursor or background image but you can use "xsetroot -cursor_name top_left_arrow" to set the pointer and xv, xloadimage or xpmroot to set a background image.
* WindowLab does not have virtual desktops, but you can use [vdesk](http://offog.org/code/vdesk/), a command-line driven virtual workspace manager that I've been told works acceptably. This can be combined with a separate application launcher (or WindowLab's built-in menubar) to give most of the functionality needed.
* If you are locked into the menubar and want to get out of it, click the left mouse button
* Use Alt + F11 to toggle fullscreen mode on before watching video applications

If you know any other tips for use with WindowLab, please get in contact with me so that I can list them here.


## Acknowledgements

Thanks to Decklin Foster who wrote the original [aewm](http://www.red-bean.com/decklin/aewm/) (v1.1.2 to be precise) on which WindowLab is based. He's done a superb job of writing a minimal window manager and was good enough to release it under a liberal licence that allows anyone to add their favourite GPL or BSD flavour as they see fit.


## Inherited licences

### aewm

> Copyright (c) 1998-2001 Decklin Foster.
>
> THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS", WITHOUT ANY EXPRESS
> OR IMPLIED WARRANTIES OF ANY KIND. IN NO EVENT SHALL THE AUTHOR BE
> HELD LIABLE FOR ANY DAMAGES CONNECTED WITH THE USE OF THIS PROGRAM.
>
> You are granted permission to copy, publish, distribute, and/or sell
> copies of this program and any modified versions or derived works,
> provided that this copyright and notice are not removed or altered.

### 9wm

> Copyright (c) 1994 David Hogan  
> Copyright (c) 2009 The Estate of David Hogan
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicence, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
> THE SOFTWARE.

